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
#include "HttpDownloadCommand.h"
#include "RequestGroup.h"
#include "DownloadEngine.h"
#include "Request.h"
#include "HttpRequestCommand.h"
#include "HttpConnection.h"
#include "HttpRequest.h"
#include "Segment.h"
#include "Socket.h"
#include "prefs.h"
#include "Option.h"
#include "HttpResponse.h"
#include "HttpHeader.h"
#include "Range.h"
#include "DownloadContext.h"
#include "Decoder.h"
#include "RequestGroupMan.h"
#include "FileAllocationEntry.h"
#include "CheckIntegrityEntry.h"
#include "ServerStatMan.h"
#include "Logger.h"
namespace aria2 {

HttpDownloadCommand::HttpDownloadCommand
(cuid_t cuid,
 const SharedHandle<Request>& req,
 const SharedHandle<FileEntry>& fileEntry,
 RequestGroup* requestGroup,
 const SharedHandle<HttpResponse>& httpResponse,
 const HttpConnectionHandle& httpConnection,
 DownloadEngine* e,
 const SocketHandle& socket)
  :DownloadCommand(cuid, req, fileEntry, requestGroup, e, socket),
   httpResponse_(httpResponse),
   httpConnection_(httpConnection) {}

HttpDownloadCommand::~HttpDownloadCommand() {}

bool HttpDownloadCommand::prepareForNextSegment() {
  bool downloadFinished = getRequestGroup()->downloadFinished();
  if(getRequest()->isPipeliningEnabled() && !downloadFinished) {
    HttpRequestCommand* command =
      new HttpRequestCommand(getCuid(), getRequest(), getFileEntry(),
                             getRequestGroup(), httpConnection_,
                             getDownloadEngine(), getSocket());
    // Set proxy request here. aria2 sends the HTTP request specialized for
    // proxy.
    if(resolveProxyMethod(getRequest()->getProtocol()) == V_GET) {
      command->setProxyRequest(createProxyRequest());
    }
    getDownloadEngine()->addCommand(command);
    return true;
  } else {
    if(getRequest()->isPipeliningEnabled() ||
       (getRequest()->isKeepAliveEnabled() &&
        (
         // Make sure that all decoders are finished to pool socket
         ((!getTransferEncodingDecoder().isNull() &&
           getTransferEncodingDecoder()->finished()) ||
          (getTransferEncodingDecoder().isNull() &&
           !getContentEncodingDecoder().isNull() &&
           getContentEncodingDecoder()->finished())) ||
         getRequestEndOffset() ==
         getFileEntry()->gtoloff(getSegments().front()->getPositionToWrite())
         )
        )
       ) {
      // TODO What if server sends EOF when _contentEncodingDecoder is
      // used and server didn't send Connection: close? We end up to
      // pool terminated socket.  In HTTP/1.1, keep-alive is default,
      // so closing connection without Connection: close header means
      // that server is broken or not configured properly.
      getDownloadEngine()->poolSocket
        (getRequest(), createProxyRequest(), getSocket());
    }
    // The request was sent assuming that server supported pipelining, but
    // it turned out that server didn't support it.
    // We detect this situation by comparing the end byte in range header
    // of the response with the end byte of segment.
    // If it is the same, HTTP negotiation is necessary for the next request.
    if(!getRequest()->isPipeliningEnabled() &&
       getRequest()->isPipeliningHint() &&
       !downloadFinished) {
      const SharedHandle<Segment>& segment = getSegments().front();

      off_t lastOffset =getFileEntry()->gtoloff
        (std::min(static_cast<off_t>
                  (segment->getPosition()+segment->getLength()),
                  getFileEntry()->getLastOffset()));
      
      if(lastOffset ==
         httpResponse_->getHttpHeader()->getRange()->getEndByte()+1) {
        return prepareForRetry(0);
      }
    }
    return DownloadCommand::prepareForNextSegment();
  }
}

off_t HttpDownloadCommand::getRequestEndOffset() const
{
  return httpResponse_->getHttpHeader()->getRange()->getEndByte()+1;
}

} // namespace aria2
