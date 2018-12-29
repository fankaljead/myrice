/* <!-- copyright */
/*
 * aria2 - The high speed download utility
 *
 * Copyright (C) 2006 Tatsuhiro Tsujikawa
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
/* copyright --> */
#include "HttpResponseCommand.h"
#include "DownloadEngine.h"
#include "DownloadContext.h"
#include "FileEntry.h"
#include "RequestGroup.h"
#include "RequestGroupMan.h"
#include "Request.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "HttpConnection.h"
#include "SegmentMan.h"
#include "Segment.h"
#include "HttpDownloadCommand.h"
#include "DiskAdaptor.h"
#include "PieceStorage.h"
#include "DefaultBtProgressInfoFile.h"
#include "DownloadFailureException.h"
#include "DlAbortEx.h"
#include "util.h"
#include "File.h"
#include "Option.h"
#include "Logger.h"
#include "Socket.h"
#include "message.h"
#include "prefs.h"
#include "StringFormat.h"
#include "HttpSkipResponseCommand.h"
#include "HttpHeader.h"
#include "LogFactory.h"
#include "CookieStorage.h"
#include "AuthConfigFactory.h"
#include "AuthConfig.h"
#include "a2functional.h"
#include "URISelector.h"
#include "ServerStatMan.h"
#include "FileAllocationEntry.h"
#include "CheckIntegrityEntry.h"

namespace aria2 {

static SharedHandle<Decoder> getTransferEncodingDecoder
(const SharedHandle<HttpResponse>& httpResponse);

static SharedHandle<Decoder> getContentEncodingDecoder
(const SharedHandle<HttpResponse>& httpResponse);

HttpResponseCommand::HttpResponseCommand
(cuid_t cuid,
 const SharedHandle<Request>& req,
 const SharedHandle<FileEntry>& fileEntry,
 RequestGroup* requestGroup,
 const HttpConnectionHandle& httpConnection,
 DownloadEngine* e,
 const SocketHandle& s)
  :AbstractCommand(cuid, req, fileEntry, requestGroup, e, s),
   httpConnection_(httpConnection)
{}

HttpResponseCommand::~HttpResponseCommand() {}

bool HttpResponseCommand::executeInternal()
{
  SharedHandle<HttpRequest> httpRequest =httpConnection_->getFirstHttpRequest();
  SharedHandle<HttpResponse> httpResponse = httpConnection_->receiveResponse();
  if(httpResponse.isNull()) {
    // The server has not responded to our request yet.
    // For socket->wantRead() == true, setReadCheckSocket(socket) is already
    // done in the constructor.
    setWriteCheckSocketIf(getSocket(), getSocket()->wantWrite());
    getDownloadEngine()->addCommand(this);
    return false;
  }
  // check HTTP status number
  httpResponse->validateResponse();
  httpResponse->retrieveCookie();

  SharedHandle<HttpHeader> httpHeader = httpResponse->getHttpHeader();
  // Disable persistent connection if:
  //   Connection: close is received or the remote server is not HTTP/1.1.
  // We don't care whether non-HTTP/1.1 server returns Connection: keep-alive.
  getRequest()->supportsPersistentConnection
    (httpResponse->supportsPersistentConnection());
  if(getRequest()->isPipeliningEnabled()) {
    getRequest()->setMaxPipelinedRequest
      (getOption()->getAsInt(PREF_MAX_HTTP_PIPELINING));
  }

  if(!httpResponse->getHttpRequest()->getIfModifiedSinceHeader().empty()) {
    if(httpResponse->getResponseStatus() == HttpHeader::S304) {
      uint64_t totalLength = httpResponse->getEntityLength();
      getFileEntry()->setLength(totalLength);
      getRequestGroup()->initPieceStorage();
      getPieceStorage()->markAllPiecesDone();
      getLogger()->notice(MSG_DOWNLOAD_ALREADY_COMPLETED,
                          util::itos(getRequestGroup()->getGID()).c_str(),
                          getRequestGroup()->getFirstFilePath().c_str());
      poolConnection();
      getFileEntry()->poolRequest(getRequest());
      return true;
    } else if(httpResponse->getResponseStatus() == HttpHeader::S200 ||
              httpResponse->getResponseStatus() == HttpHeader::S206) {
      // Remote file is newer than local file. We allow overwrite.
      getOption()->put(PREF_ALLOW_OVERWRITE, V_TRUE);
    }
  }
  if(httpResponse->getResponseStatus() >= HttpHeader::S300 &&
     httpResponse->getResponseStatus() != HttpHeader::S304) {
    if(httpResponse->getResponseStatus() == HttpHeader::S404) {
      getRequestGroup()->increaseAndValidateFileNotFoundCount();
    }
    return skipResponseBody(httpResponse);
  }
  if(getFileEntry()->isUniqueProtocol()) {
    // TODO redirection should be considered here. We need to parse
    // original URI to get hostname.
    getFileEntry()->removeURIWhoseHostnameIs(getRequest()->getHost());
  }
  if(getPieceStorage().isNull()) {
    uint64_t totalLength = httpResponse->getEntityLength();
    getFileEntry()->setLength(totalLength);
    if(getFileEntry()->getPath().empty()) {
      getFileEntry()->setPath
        (util::applyDir
         (getDownloadContext()->getDir(),
          util::fixTaintedBasename(httpResponse->determinFilename())));
    }
    getFileEntry()->setContentType(httpResponse->getContentType());
    getRequestGroup()->preDownloadProcessing();
    if(getDownloadEngine()->getRequestGroupMan()->
       isSameFileBeingDownloaded(getRequestGroup())) {
      throw DOWNLOAD_FAILURE_EXCEPTION
        (StringFormat(EX_DUPLICATE_FILE_DOWNLOAD,
                      getRequestGroup()->getFirstFilePath().c_str()).str());
    }
    // update last modified time
    updateLastModifiedTime(httpResponse->getLastModifiedTime());

    // If both transfer-encoding and total length is specified, we
    // assume we can do segmented downloading
    if(totalLength == 0 || shouldInflateContentEncoding(httpResponse)) {
      // we ignore content-length when inflate is required
      getFileEntry()->setLength(0);
      if(getRequest()->getMethod() == Request::METHOD_GET &&
         (totalLength != 0 ||
          !httpResponse->getHttpHeader()->defined(HttpHeader::CONTENT_LENGTH))){
        // DownloadContext::knowsTotalLength() == true only when
        // server says the size of file is 0 explicitly.
        getDownloadContext()->markTotalLengthIsUnknown();
      }
      return handleOtherEncoding(httpResponse);
    } else {
      return handleDefaultEncoding(httpResponse);
    }
  } else {
    // validate totalsize
    getRequestGroup()->validateTotalLength(getFileEntry()->getLength(),
                                       httpResponse->getEntityLength());
    // update last modified time
    updateLastModifiedTime(httpResponse->getLastModifiedTime());
    if(getRequestGroup()->getTotalLength() == 0) {
      // Since total length is unknown, the file size in previously
      // failed download could be larger than the size this time.
      // Also we can't resume in this case too.  So truncate the file
      // anyway.
      getPieceStorage()->getDiskAdaptor()->truncate(0);
      getDownloadEngine()->addCommand
        (createHttpDownloadCommand(httpResponse,
                                   getTransferEncodingDecoder(httpResponse),
                                   getContentEncodingDecoder(httpResponse)));
    } else {
      getDownloadEngine()->addCommand(createHttpDownloadCommand
                    (httpResponse, getTransferEncodingDecoder(httpResponse)));
    }
    return true;
  }
}

void HttpResponseCommand::updateLastModifiedTime(const Time& lastModified)
{
  if(getOption()->getAsBool(PREF_REMOTE_TIME)) {
    getRequestGroup()->updateLastModifiedTime(lastModified);
  }
}

bool HttpResponseCommand::shouldInflateContentEncoding
(const SharedHandle<HttpResponse>& httpResponse)
{
  // Basically, on the fly inflation cannot be made with segment
  // download, because in each segment we don't know where the date
  // should be written.  So turn off segmented downloading.
  // Meanwhile, Some server returns content-encoding: gzip for .tgz
  // files.  I think those files should not be inflated by clients,
  // because it is the original format of those files. Current
  // implementation just inflates these files nonetheless.
  const std::string& ce = httpResponse->getContentEncoding();
  return httpResponse->getHttpRequest()->acceptGZip() &&
    (ce == "gzip" || ce == "deflate");
}

bool HttpResponseCommand::handleDefaultEncoding
(const SharedHandle<HttpResponse>& httpResponse)
{
  SharedHandle<HttpRequest> httpRequest = httpResponse->getHttpRequest();
  getRequestGroup()->adjustFilename
    (SharedHandle<BtProgressInfoFile>(new DefaultBtProgressInfoFile
                                      (getDownloadContext(),
                                       SharedHandle<PieceStorage>(),
                                       getOption().get())));
  getRequestGroup()->initPieceStorage();

  if(getOption()->getAsBool(PREF_DRY_RUN)) {
    onDryRunFileFound();
    return true;
  }

  BtProgressInfoFileHandle infoFile
    (new DefaultBtProgressInfoFile(getDownloadContext(),
                                   getPieceStorage(),
                                   getOption().get()));
  if(!infoFile->exists() && getRequestGroup()->downloadFinishedByFileLength()) {
    getPieceStorage()->markAllPiecesDone();
    getLogger()->notice(MSG_DOWNLOAD_ALREADY_COMPLETED,
                        util::itos(getRequestGroup()->getGID()).c_str(),
                        getRequestGroup()->getFirstFilePath().c_str());
    return true;
  }
  getRequestGroup()->loadAndOpenFile(infoFile);
  File file(getRequestGroup()->getFirstFilePath());
  // We have to make sure that command that has Request object must
  // have segment after PieceStorage is initialized. See
  // AbstractCommand::execute()
  SharedHandle<Segment> segment =
    getSegmentMan()->getSegmentWithIndex(getCuid(), 0);
  // pipelining requires implicit range specified. But the request for
  // this response most likely dones't contains range header. This means
  // we can't continue to use this socket because server sends all entity
  // body instead of a segment.
  // Therefore, we shutdown the socket here if pipelining is enabled.
  DownloadCommand* command = 0;
  if(getRequest()->getMethod() == Request::METHOD_GET &&
     !segment.isNull() && segment->getPositionToWrite() == 0 &&
     !getRequest()->isPipeliningEnabled()) {
    command = createHttpDownloadCommand
      (httpResponse, getTransferEncodingDecoder(httpResponse));
  } else {
    getSegmentMan()->cancelSegment(getCuid());
    getFileEntry()->poolRequest(getRequest());
  }
  // After command is passed to prepareForNextAction(), it is managed
  // by CheckIntegrityEntry.
  prepareForNextAction(command);
  command = 0;
  if(getRequest()->getMethod() == Request::METHOD_HEAD) {
    poolConnection();
    getRequest()->setMethod(Request::METHOD_GET);
  }
  return true;
}

static SharedHandle<Decoder> getTransferEncodingDecoder
(const SharedHandle<HttpResponse>& httpResponse)
{
  SharedHandle<Decoder> decoder;
  if(httpResponse->isTransferEncodingSpecified()) {
    decoder = httpResponse->getTransferEncodingDecoder();
    if(decoder.isNull()) {
      throw DL_ABORT_EX
        (StringFormat(EX_TRANSFER_ENCODING_NOT_SUPPORTED,
                      httpResponse->getTransferEncoding().c_str()).str());
    }
    decoder->init();
  }
  return decoder;
}

static SharedHandle<Decoder> getContentEncodingDecoder
(const SharedHandle<HttpResponse>& httpResponse)
{
  SharedHandle<Decoder> decoder;
  if(httpResponse->isContentEncodingSpecified()) {
    decoder = httpResponse->getContentEncodingDecoder();
    if(decoder.isNull()) {
      LogFactory::getInstance()->info
        ("Content-Encoding %s is specified, but the current implementation"
         "doesn't support it. The decoding process is skipped and the"
         "downloaded content will be still encoded.",
         httpResponse->getContentEncoding().c_str());
    } else {
      decoder->init();
    }
  }
  return decoder;
}

bool HttpResponseCommand::handleOtherEncoding
(const SharedHandle<HttpResponse>& httpResponse) {
  // We assume that RequestGroup::getTotalLength() == 0 here
  SharedHandle<HttpRequest> httpRequest = httpResponse->getHttpRequest();

  if(getOption()->getAsBool(PREF_DRY_RUN)) {
    getRequestGroup()->initPieceStorage();
    onDryRunFileFound();
    return true;
  }

  if(getRequest()->getMethod() == Request::METHOD_HEAD) {
    poolConnection();
    getRequest()->setMethod(Request::METHOD_GET);
    return prepareForRetry(0);
  }

  // For zero-length file, check existing file comparing its size
  if(getRequestGroup()->downloadFinishedByFileLength()) {
    getRequestGroup()->initPieceStorage();
    getPieceStorage()->markAllPiecesDone();
    getLogger()->notice(MSG_DOWNLOAD_ALREADY_COMPLETED,
                        util::itos(getRequestGroup()->getGID()).c_str(),
                        getRequestGroup()->getFirstFilePath().c_str());
    poolConnection();
    return true;
  }

  getRequestGroup()->shouldCancelDownloadForSafety();
  getRequestGroup()->initPieceStorage();
  getPieceStorage()->getDiskAdaptor()->initAndOpenFile();
  // In this context, knowsTotalLength() is true only when the file is
  // really zero-length.
  if(getDownloadContext()->knowsTotalLength()) {
    poolConnection();
    return true;
  }
  // We have to make sure that command that has Request object must
  // have segment after PieceStorage is initialized. See
  // AbstractCommand::execute()
  getSegmentMan()->getSegmentWithIndex(getCuid(), 0);

  getDownloadEngine()->addCommand
    (createHttpDownloadCommand(httpResponse,
                               getTransferEncodingDecoder(httpResponse),
                               getContentEncodingDecoder(httpResponse)));
  return true;
}

bool HttpResponseCommand::skipResponseBody
(const SharedHandle<HttpResponse>& httpResponse)
{
  SharedHandle<Decoder> decoder = getTransferEncodingDecoder(httpResponse);
  // We don't use Content-Encoding here because this response body is just
  // thrown away.

  HttpSkipResponseCommand* command = new HttpSkipResponseCommand
    (getCuid(), getRequest(), getFileEntry(), getRequestGroup(),
     httpConnection_, httpResponse,
     getDownloadEngine(), getSocket());
  command->setTransferEncodingDecoder(decoder);

  // If request method is HEAD or the response body is zero-length,
  // set command's status to real time so that avoid read check blocking
  if(getRequest()->getMethod() == Request::METHOD_HEAD ||
     (httpResponse->getEntityLength() == 0 &&
      !httpResponse->isTransferEncodingSpecified())) {
    command->setStatusRealtime();
    // If entity length == 0, then socket read/write check must be disabled.
    command->disableSocketCheck();
    getDownloadEngine()->setNoWait(true);
  }

  getDownloadEngine()->addCommand(command);
  return true;
}

HttpDownloadCommand* HttpResponseCommand::createHttpDownloadCommand
(const SharedHandle<HttpResponse>& httpResponse,
 const SharedHandle<Decoder>& transferEncodingDecoder,
 const SharedHandle<Decoder>& contentEncodingDecoder)
{

  HttpDownloadCommand* command =
    new HttpDownloadCommand(getCuid(), getRequest(), getFileEntry(),
                            getRequestGroup(),
                            httpResponse, httpConnection_,
                            getDownloadEngine(), getSocket());
  command->setStartupIdleTime(getOption()->getAsInt(PREF_STARTUP_IDLE_TIME));
  command->setLowestDownloadSpeedLimit
    (getOption()->getAsInt(PREF_LOWEST_SPEED_LIMIT));
  command->setTransferEncodingDecoder(transferEncodingDecoder);

  if(!contentEncodingDecoder.isNull()) {
    command->setContentEncodingDecoder(contentEncodingDecoder);
    // Since the compressed file's length are returned in the response header
    // and the decompressed file size is unknown at this point, disable file
    // allocation here.
    getRequestGroup()->setFileAllocationEnabled(false);
  }

  getRequestGroup()->getURISelector()->tuneDownloadCommand
    (getFileEntry()->getRemainingUris(), command);

  return command;
}

void HttpResponseCommand::poolConnection()
{
  if(getRequest()->supportsPersistentConnection()) {
    getDownloadEngine()->poolSocket(getRequest(), createProxyRequest(),
                                    getSocket());
  }
}

void HttpResponseCommand::onDryRunFileFound()
{
  getPieceStorage()->markAllPiecesDone();
  poolConnection();
}

} // namespace aria2
