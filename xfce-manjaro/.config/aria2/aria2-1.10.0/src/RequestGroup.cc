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
#include "RequestGroup.h"

#include <cassert>
#include <algorithm>

#include "PostDownloadHandler.h"
#include "DownloadEngine.h"
#include "SegmentMan.h"
#include "NullProgressInfoFile.h"
#include "Dependency.h"
#include "prefs.h"
#include "CreateRequestCommand.h"
#include "File.h"
#include "message.h"
#include "util.h"
#include "LogFactory.h"
#include "Logger.h"
#include "DiskAdaptor.h"
#include "DiskWriterFactory.h"
#include "RecoverableException.h"
#include "StreamCheckIntegrityEntry.h"
#include "CheckIntegrityCommand.h"
#include "UnknownLengthPieceStorage.h"
#include "DownloadContext.h"
#include "DlAbortEx.h"
#include "DownloadFailureException.h"
#include "RequestGroupMan.h"
#include "DefaultBtProgressInfoFile.h"
#include "DefaultPieceStorage.h"
#include "DownloadHandlerFactory.h"
#include "MemoryBufferPreDownloadHandler.h"
#include "DownloadHandlerConstants.h"
#include "Option.h"
#include "FileEntry.h"
#include "Request.h"
#include "FileAllocationIterator.h"
#include "StringFormat.h"
#include "A2STR.h"
#include "URISelector.h"
#include "InOrderURISelector.h"
#include "PieceSelector.h"
#include "a2functional.h"
#include "SocketCore.h"
#include "SimpleRandomizer.h"
#include "ServerStatMan.h"
#include "Segment.h"
#include "FileAllocationEntry.h"
#ifdef ENABLE_MESSAGE_DIGEST
# include "CheckIntegrityCommand.h"
# include "ChecksumCheckIntegrityEntry.h"
#endif // ENABLE_MESSAGE_DIGEST
#ifdef ENABLE_BITTORRENT
# include "bittorrent_helper.h"
# include "BtRegistry.h"
# include "BtCheckIntegrityEntry.h"
# include "DefaultPeerStorage.h"
# include "DefaultBtAnnounce.h"
# include "BtRuntime.h"
# include "BtSetup.h"
# include "BtPostDownloadHandler.h"
# include "DHTSetup.h"
# include "DHTRegistry.h"
# include "BtMessageFactory.h"
# include "BtRequestFactory.h"
# include "BtMessageDispatcher.h"
# include "BtMessageReceiver.h"
# include "PeerConnection.h"
# include "ExtensionMessageFactory.h"
# include "DHTPeerAnnounceStorage.h"
# include "DHTEntryPointNameResolveCommand.h"
# include "LongestSequencePieceSelector.h"
# include "PriorityPieceSelector.h"
# include "bittorrent_helper.h"
#endif // ENABLE_BITTORRENT
#ifdef ENABLE_METALINK
# include "MetalinkPostDownloadHandler.h"
#endif // ENABLE_METALINK

namespace aria2 {

gid_t RequestGroup::gidCounter_ = 0;

RequestGroup::RequestGroup(const SharedHandle<Option>& option):
  gid_(newGID()),
  option_(new Option(*option.get())),
  numConcurrentCommand_(option->getAsInt(PREF_SPLIT)),
  numStreamConnection_(0),
  numStreamCommand_(0),
  numCommand_(0),
  saveControlFile_(true),
  progressInfoFile_(new NullProgressInfoFile()),
  preLocalFileCheckEnabled_(true),
  haltRequested_(false),
  forceHaltRequested_(false),
  haltReason_(RequestGroup::NONE),
  pauseRequested_(false),
  uriSelector_(new InOrderURISelector()),
  lastModifiedTime_(Time::null()),
  fileNotFoundCount_(0),
  timeout_(option->getAsInt(PREF_TIMEOUT)),
  inMemoryDownload_(false),
  maxDownloadSpeedLimit_(option->getAsInt(PREF_MAX_DOWNLOAD_LIMIT)),
  maxUploadSpeedLimit_(option->getAsInt(PREF_MAX_UPLOAD_LIMIT)),
  belongsToGID_(0),
  requestGroupMan_(0),
  resumeFailureCount_(0),
  logger_(LogFactory::getInstance())
{
  fileAllocationEnabled_ = option_->get(PREF_FILE_ALLOCATION) != V_NONE;
  // Add types to be sent as a Accept header value here.
  // It would be good to put this value in Option so that user can tweak
  // and add this list.
  // The mime types of Metalink is used for `transparent metalink'.
  addAcceptType(DownloadHandlerConstants::getMetalinkContentTypes().begin(),
		DownloadHandlerConstants::getMetalinkContentTypes().end());
  if(!option_->getAsBool(PREF_DRY_RUN)) {
    initializePreDownloadHandler();
    initializePostDownloadHandler();
  }
}

RequestGroup::~RequestGroup() {}

bool RequestGroup::downloadFinished() const
{
  if(pieceStorage_.isNull()) {
    return false;
  } else {
    return pieceStorage_->downloadFinished();
  }
}

bool RequestGroup::allDownloadFinished() const
{
  if(pieceStorage_.isNull()) {
    return false;
  } else {
    return pieceStorage_->allDownloadFinished();
  }
}

downloadresultcode::RESULT RequestGroup::downloadResult() const
{
  if(downloadFinished() && !downloadContext_->isChecksumVerificationNeeded())
    return downloadresultcode::FINISHED;
  else {
    if (lastUriResult_.isNull()) {
      if(haltReason_ == RequestGroup::USER_REQUEST ||
         haltReason_ == RequestGroup::SHUTDOWN_SIGNAL) {
        return downloadresultcode::IN_PROGRESS;
      } else {
        return downloadresultcode::UNKNOWN_ERROR;
      }
    } else {
      return lastUriResult_->getResult();
    }
  }    
}

void RequestGroup::closeFile()
{
  if(!pieceStorage_.isNull()) {
    pieceStorage_->getDiskAdaptor()->closeFile();
  }
}

void RequestGroup::createInitialCommand
(std::vector<Command*>& commands, DownloadEngine* e)
{
  // Start session timer here.  When file size becomes known, it will
  // be reset again in *FileAllocationEntry, because hash check and
  // file allocation takes a time.  For downloads in which file size
  // is unknown, session timer will not be reset.
  downloadContext_->resetDownloadStartTime();
#ifdef ENABLE_BITTORRENT
  {
    if(downloadContext_->hasAttribute(bittorrent::BITTORRENT)) {
      SharedHandle<TorrentAttribute> torrentAttrs =
        bittorrent::getTorrentAttrs(downloadContext_);
      bool metadataGetMode = torrentAttrs->metadata.empty();
      if(option_->getAsBool(PREF_DRY_RUN)) {
        throw DOWNLOAD_FAILURE_EXCEPTION
          ("Cancel BitTorrent download in dry-run context.");
      }
      SharedHandle<BtRegistry> btRegistry = e->getBtRegistry();
      if(!btRegistry->getDownloadContext(torrentAttrs->infoHash).isNull()) {
        // TODO If metadataGetMode == false and each FileEntry has
        // URI, then go without BT.
        throw DOWNLOAD_FAILURE_EXCEPTION
          (StringFormat
           ("InfoHash %s is already registered.",
            bittorrent::getInfoHashString(downloadContext_).c_str()).str());
      }
      if(metadataGetMode) {
        // Use UnknownLengthPieceStorage.
        initPieceStorage();
      } else {
        if(e->getRequestGroupMan()->isSameFileBeingDownloaded(this)) {
          throw DOWNLOAD_FAILURE_EXCEPTION
            (StringFormat(EX_DUPLICATE_FILE_DOWNLOAD,
                          downloadContext_->getBasePath().c_str()).str());
        }
        initPieceStorage();
        if(downloadContext_->getFileEntries().size() > 1) {
          pieceStorage_->setupFileFilter();
        }
      }
      
      SharedHandle<DefaultBtProgressInfoFile> progressInfoFile;
      if(!metadataGetMode) {
        progressInfoFile.reset(new DefaultBtProgressInfoFile(downloadContext_,
                                                             pieceStorage_,
                                                             option_.get()));
      }
        
      BtRuntimeHandle btRuntime(new BtRuntime());
      btRuntime->setMaxPeers(option_->getAsInt(PREF_BT_MAX_PEERS));
      btRuntime_ = btRuntime;
      if(!progressInfoFile.isNull()) {
        progressInfoFile->setBtRuntime(btRuntime);
      }

      SharedHandle<DefaultPeerStorage> peerStorage(new DefaultPeerStorage());
      peerStorage->setBtRuntime(btRuntime);
      peerStorage->setPieceStorage(pieceStorage_);
      peerStorage_ = peerStorage;
      if(!progressInfoFile.isNull()) {
        progressInfoFile->setPeerStorage(peerStorage);
      }

      SharedHandle<DefaultBtAnnounce> btAnnounce
        (new DefaultBtAnnounce(downloadContext_, option_.get()));
      btAnnounce->setBtRuntime(btRuntime);
      btAnnounce->setPieceStorage(pieceStorage_);
      btAnnounce->setPeerStorage(peerStorage);
      btAnnounce->setUserDefinedInterval
        (option_->getAsInt(PREF_BT_TRACKER_INTERVAL));
      btAnnounce->shuffleAnnounce();
      
      assert(btRegistry->get(gid_).isNull());
      btRegistry->put(gid_,
                      BtObject(downloadContext_,
                               pieceStorage_,
                               peerStorage,
                               btAnnounce,
                               btRuntime,
                               (progressInfoFile.isNull()?
                                progressInfoFile_:
                                SharedHandle<BtProgressInfoFile>
                                (progressInfoFile))));
      if(metadataGetMode) {
        if(option_->getAsBool(PREF_ENABLE_DHT)) {
          std::vector<Command*> dhtCommands;
          DHTSetup().setup(dhtCommands, e);
          e->addCommand(dhtCommands);
        } else {
          logger_->notice("For BitTorrent Magnet URI, enabling DHT is strongly"
                          " recommended. See --enable-dht option.");
        }

        SharedHandle<CheckIntegrityEntry> entry
          (new BtCheckIntegrityEntry(this));
        entry->onDownloadIncomplete(commands, e);
        
        return;
      }
      removeDefunctControlFile(progressInfoFile);
      {
        uint64_t actualFileSize = pieceStorage_->getDiskAdaptor()->size();
        if(actualFileSize == downloadContext_->getTotalLength()) {
          // First, make DiskAdaptor read-only mode to allow the
          // program to seed file in read-only media.
          pieceStorage_->getDiskAdaptor()->enableReadOnly();
        } else {
          // Open file in writable mode to allow the program
          // truncate the file to downloadContext_->getTotalLength()
          if(logger_->debug()) {
            logger_->debug("File size not match. File is opened in writable"
                           " mode. Expected:%s Actual:%s",
                           util::uitos(downloadContext_->getTotalLength()).c_str(),
                           util::uitos(actualFileSize).c_str());
          }
        }
      }
      // Call Load, Save and file allocation command here
      if(progressInfoFile->exists()) {
        // load .aria2 file if it exists.
        progressInfoFile->load();
        pieceStorage_->getDiskAdaptor()->openFile();
      } else {
        if(pieceStorage_->getDiskAdaptor()->fileExists()) {
          if(!option_->getAsBool(PREF_CHECK_INTEGRITY) &&
             !option_->getAsBool(PREF_ALLOW_OVERWRITE) &&
             !option_->getAsBool(PREF_BT_SEED_UNVERIFIED)) {
            // TODO we need this->haltRequested = true?
            throw DOWNLOAD_FAILURE_EXCEPTION
              (StringFormat
               (MSG_FILE_ALREADY_EXISTS,
                downloadContext_->getBasePath().c_str()).str());
          } else {
            pieceStorage_->getDiskAdaptor()->openFile();
          }
          if(option_->getAsBool(PREF_BT_SEED_UNVERIFIED)) {
            pieceStorage_->markAllPiecesDone();
          }
        } else {
          pieceStorage_->getDiskAdaptor()->openFile();
        }
      }
      progressInfoFile_ = progressInfoFile;

      if(!torrentAttrs->privateTorrent && option_->getAsBool(PREF_ENABLE_DHT)) {
        std::vector<Command*> dhtCommands;
        DHTSetup().setup(dhtCommands, e);
        e->addCommand(dhtCommands);
        const std::vector<std::pair<std::string, uint16_t> >& nodes =
          torrentAttrs->nodes;
        if(!nodes.empty() && DHTSetup::initialized()) {
          std::vector<std::pair<std::string, uint16_t> > entryPoints(nodes);
          DHTEntryPointNameResolveCommand* command =
            new DHTEntryPointNameResolveCommand(e->newCUID(), e, entryPoints);
          command->setTaskQueue(DHTRegistry::getData().taskQueue);
          command->setTaskFactory(DHTRegistry::getData().taskFactory);
          command->setRoutingTable(DHTRegistry::getData().routingTable);
          command->setLocalNode(DHTRegistry::getData().localNode);
          e->addCommand(command);
        }
      }
      SharedHandle<CheckIntegrityEntry> entry(new BtCheckIntegrityEntry(this));
      // --bt-seed-unverified=true is given and download has completed, skip
      // validation for piece hashes.
      if(option_->getAsBool(PREF_BT_SEED_UNVERIFIED) &&
         pieceStorage_->downloadFinished()) {
        entry->onDownloadFinished(commands, e);
      } else {
        processCheckIntegrityEntry(commands, entry, e);
      }
      return;
    }
  }
#endif // ENABLE_BITTORRENT
  if(downloadContext_->getFileEntries().size() == 1) {
    // TODO I assume here when totallength is set to DownloadContext and it is
    // not 0, then filepath is also set DownloadContext correctly....
    if(option_->getAsBool(PREF_DRY_RUN) ||
       downloadContext_->getTotalLength() == 0) {
      createNextCommand(commands, e, 1);
    }else {
      if(e->getRequestGroupMan()->isSameFileBeingDownloaded(this)) {
        throw DOWNLOAD_FAILURE_EXCEPTION
          (StringFormat(EX_DUPLICATE_FILE_DOWNLOAD,
                        downloadContext_->getBasePath().c_str()).str());
      }
      adjustFilename
        (SharedHandle<BtProgressInfoFile>(new DefaultBtProgressInfoFile
                                          (downloadContext_,
                                           SharedHandle<PieceStorage>(),
                                           option_.get())));
      initPieceStorage();
      BtProgressInfoFileHandle infoFile
        (new DefaultBtProgressInfoFile(downloadContext_, pieceStorage_,
                                       option_.get()));
      if(!infoFile->exists() && downloadFinishedByFileLength()) {
        pieceStorage_->markAllPiecesDone();
        logger_->notice(MSG_DOWNLOAD_ALREADY_COMPLETED,
                        util::itos(gid_).c_str(),
                        downloadContext_->getBasePath().c_str());
      } else {
        loadAndOpenFile(infoFile);
#ifdef ENABLE_MESSAGE_DIGEST
        if(downloadFinished() &&
           downloadContext_->isChecksumVerificationNeeded()) {
          if(logger_->info()) {
            logger_->info(MSG_HASH_CHECK_NOT_DONE);
          }
          SharedHandle<CheckIntegrityEntry> entry
            (new ChecksumCheckIntegrityEntry(this));
          if(entry->isValidationReady()) {
            entry->initValidator();
            entry->cutTrailingGarbage();
            e->getCheckIntegrityMan()->pushEntry(entry);
            return;
          }
        }
#endif // ENABLE_MESSAGE_DIGEST
        SharedHandle<CheckIntegrityEntry> checkIntegrityEntry
          (new StreamCheckIntegrityEntry(this));
        processCheckIntegrityEntry(commands, checkIntegrityEntry, e);
      }
    }
  } else {
    // In this context, multiple FileEntry objects are in
    // DownloadContext.
    if(e->getRequestGroupMan()->isSameFileBeingDownloaded(this)) {
      throw DOWNLOAD_FAILURE_EXCEPTION
        (StringFormat(EX_DUPLICATE_FILE_DOWNLOAD,
                      downloadContext_->getBasePath().c_str()).str());
    }
    initPieceStorage();
    if(downloadContext_->getFileEntries().size() > 1) {
      pieceStorage_->setupFileFilter();
    }
    SharedHandle<DefaultBtProgressInfoFile> progressInfoFile
      (new DefaultBtProgressInfoFile(downloadContext_,
                                     pieceStorage_,
                                     option_.get()));
    removeDefunctControlFile(progressInfoFile);
    // Call Load, Save and file allocation command here
    if(progressInfoFile->exists()) {
      // load .aria2 file if it exists.
      progressInfoFile->load();
      pieceStorage_->getDiskAdaptor()->openFile();
    } else {
      if(pieceStorage_->getDiskAdaptor()->fileExists()) {
        if(!option_->getAsBool(PREF_CHECK_INTEGRITY) &&
           !option_->getAsBool(PREF_ALLOW_OVERWRITE)) {
          // TODO we need this->haltRequested = true?
          throw DOWNLOAD_FAILURE_EXCEPTION
            (StringFormat
             (MSG_FILE_ALREADY_EXISTS,
              downloadContext_->getBasePath().c_str()).str());
        } else {
          pieceStorage_->getDiskAdaptor()->openFile();
        }
      } else {
        pieceStorage_->getDiskAdaptor()->openFile();
      }
    }
    progressInfoFile_ = progressInfoFile;
    SharedHandle<CheckIntegrityEntry> checkIntegrityEntry
      (new StreamCheckIntegrityEntry(this));
    processCheckIntegrityEntry(commands, checkIntegrityEntry, e);
  }
}

void RequestGroup::processCheckIntegrityEntry
(std::vector<Command*>& commands,
 const SharedHandle<CheckIntegrityEntry>& entry,
 DownloadEngine* e)
{
#ifdef ENABLE_MESSAGE_DIGEST
  if(option_->getAsBool(PREF_CHECK_INTEGRITY) &&
     entry->isValidationReady()) {
    entry->initValidator();
    entry->cutTrailingGarbage();
    // Don't save control file(.aria2 file) when user presses
    // control-c key while aria2 is checking hashes. If control file
    // doesn't exist when aria2 launched, the completed length in
    // saved control file will be 0 byte and this confuses user.
    // enableSaveControlFile() will be called after hash checking is
    // done. See CheckIntegrityCommand.
    disableSaveControlFile();
    e->getCheckIntegrityMan()->pushEntry(entry);
  } else
#endif // ENABLE_MESSAGE_DIGEST
    {
      entry->onDownloadIncomplete(commands, e);
    }
}

void RequestGroup::initPieceStorage()
{
  SharedHandle<PieceStorage> tempPieceStorage;
  if(downloadContext_->knowsTotalLength()) {
#ifdef ENABLE_BITTORRENT
    SharedHandle<DefaultPieceStorage> ps
      (new DefaultPieceStorage(downloadContext_, option_.get()));
    if(downloadContext_->hasAttribute(bittorrent::BITTORRENT)) {
      if(isUriSuppliedForRequsetFileEntry
         (downloadContext_->getFileEntries().begin(),
          downloadContext_->getFileEntries().end())) {
        // Use LongestSequencePieceSelector when HTTP/FTP/BitTorrent
        // integrated downloads. Currently multi-file integrated
        // download is not supported.
        if(logger_->debug()) {
          logger_->debug("Using LongestSequencePieceSelector");
        }
        ps->setPieceSelector
          (SharedHandle<PieceSelector>(new LongestSequencePieceSelector()));
      }
      if(option_->defined(PREF_BT_PRIORITIZE_PIECE)) {
        std::vector<size_t> result;
        util::parsePrioritizePieceRange
          (result, option_->get(PREF_BT_PRIORITIZE_PIECE),
           downloadContext_->getFileEntries(),
           downloadContext_->getPieceLength());
        if(!result.empty()) {
          std::random_shuffle(result.begin(), result.end(),
                              *(SimpleRandomizer::getInstance().get()));
          SharedHandle<PriorityPieceSelector> priSelector
            (new PriorityPieceSelector(ps->getPieceSelector()));
          priSelector->setPriorityPiece(result.begin(), result.end());
          ps->setPieceSelector(priSelector);
        }
      }
    }
#else // !ENABLE_BITTORRENT
    SharedHandle<DefaultPieceStorage> ps
      (new DefaultPieceStorage(downloadContext_, option_.get()));
#endif // !ENABLE_BITTORRENT
    if(!diskWriterFactory_.isNull()) {
      ps->setDiskWriterFactory(diskWriterFactory_);
    }
    tempPieceStorage = ps;
  } else {
    UnknownLengthPieceStorageHandle ps
      (new UnknownLengthPieceStorage(downloadContext_, option_.get()));
    if(!diskWriterFactory_.isNull()) {
      ps->setDiskWriterFactory(diskWriterFactory_);
    }
    tempPieceStorage = ps;
  }
  tempPieceStorage->initStorage();
  SharedHandle<SegmentMan> tempSegmentMan
    (new SegmentMan(option_.get(), downloadContext_, tempPieceStorage));

  pieceStorage_ = tempPieceStorage;
  segmentMan_ = tempSegmentMan;
}

void RequestGroup::dropPieceStorage()
{
  segmentMan_.reset();
  pieceStorage_.reset();
}

bool RequestGroup::downloadFinishedByFileLength()
{
  // assuming that a control file doesn't exist.
  if(!isPreLocalFileCheckEnabled() ||
     option_->getAsBool(PREF_ALLOW_OVERWRITE) ||
     (option_->getAsBool(PREF_CHECK_INTEGRITY) &&
      !downloadContext_->getPieceHashes().empty())) {
    return false;
  }
  if(!downloadContext_->knowsTotalLength()) {
    return false;
  }
  File outfile(getFirstFilePath());
  if(outfile.exists() && downloadContext_->getTotalLength() == outfile.size()) {
    return true;
  } else {
    return false;
  }
}

void RequestGroup::adjustFilename
(const SharedHandle<BtProgressInfoFile>& infoFile)
{
  if(!isPreLocalFileCheckEnabled()) {
    // OK, no need to care about filename.
    return;
  }
  if(!option_->getAsBool(PREF_DRY_RUN) &&
     option_->getAsBool(PREF_REMOVE_CONTROL_FILE) &&
     infoFile->exists()) {
    infoFile->removeFile();
    logger_->notice("Removed control file for %s because it is requested by"
                    " user.", infoFile->getFilename().c_str());
  }
  if(infoFile->exists()) {
    // Use current filename
  } else if(downloadFinishedByFileLength()) {
    // File was downloaded already, no need to change file name.
  } else {
    File outfile(getFirstFilePath());    
    if(outfile.exists() && option_->getAsBool(PREF_CONTINUE) &&
       outfile.size() <= downloadContext_->getTotalLength()) {
      // File exists but user decided to resume it.
    } else {
#ifdef ENABLE_MESSAGE_DIGEST
      if(outfile.exists() && option_->getAsBool(PREF_CHECK_INTEGRITY)) {
        // check-integrity existing file
      } else {
#endif // ENABLE_MESSAGE_DIGEST
        shouldCancelDownloadForSafety();
#ifdef ENABLE_MESSAGE_DIGEST
      }
#endif // ENABLE_MESSAGE_DIGEST
    }
  }
}

void RequestGroup::removeDefunctControlFile
(const SharedHandle<BtProgressInfoFile>& progressInfoFile)
{
  // Remove the control file if download file doesn't exist
  if(progressInfoFile->exists() &&
     !pieceStorage_->getDiskAdaptor()->fileExists()) {
    progressInfoFile->removeFile();
    logger_->notice(MSG_REMOVED_DEFUNCT_CONTROL_FILE,
                    progressInfoFile->getFilename().c_str(),
                    downloadContext_->getBasePath().c_str());
  }
}

void RequestGroup::loadAndOpenFile(const BtProgressInfoFileHandle& progressInfoFile)
{
  try {
    if(!isPreLocalFileCheckEnabled()) {
      pieceStorage_->getDiskAdaptor()->initAndOpenFile();
      return;
    }
    removeDefunctControlFile(progressInfoFile);
    if(progressInfoFile->exists()) {
      progressInfoFile->load();
      pieceStorage_->getDiskAdaptor()->openExistingFile();
    } else {
      File outfile(getFirstFilePath());    
      if(outfile.exists() && option_->getAsBool(PREF_CONTINUE) &&
         outfile.size() <= getTotalLength()) {
        pieceStorage_->getDiskAdaptor()->openExistingFile();
        pieceStorage_->markPiecesDone(outfile.size());
      } else {
#ifdef ENABLE_MESSAGE_DIGEST
        if(outfile.exists() && option_->getAsBool(PREF_CHECK_INTEGRITY)) {
          pieceStorage_->getDiskAdaptor()->openExistingFile();
        } else {
#endif // ENABLE_MESSAGE_DIGEST
          pieceStorage_->getDiskAdaptor()->initAndOpenFile();
#ifdef ENABLE_MESSAGE_DIGEST
        }
#endif // ENABLE_MESSAGE_DIGEST
      }
    }
    setProgressInfoFile(progressInfoFile);
  } catch(RecoverableException& e) {
    throw DOWNLOAD_FAILURE_EXCEPTION2
      (StringFormat(EX_DOWNLOAD_ABORTED).str(), e);
  }
}

// assuming that a control file does not exist
void RequestGroup::shouldCancelDownloadForSafety()
{
  if(option_->getAsBool(PREF_ALLOW_OVERWRITE)) {
    return;
  }
  File outfile(getFirstFilePath());
  if(outfile.exists()) {
    if(option_->getAsBool(PREF_AUTO_FILE_RENAMING)) {
      if(tryAutoFileRenaming()) {
        logger_->notice(MSG_FILE_RENAMED, getFirstFilePath().c_str());
      } else {
        throw DOWNLOAD_FAILURE_EXCEPTION
          (StringFormat("File renaming failed: %s",
                        getFirstFilePath().c_str()).str());
      }
    } else {
      throw DOWNLOAD_FAILURE_EXCEPTION
        (StringFormat(MSG_FILE_ALREADY_EXISTS,
                      getFirstFilePath().c_str()).str());
    }
  }
}

bool RequestGroup::tryAutoFileRenaming()
{
  std::string filepath = getFirstFilePath();
  if(filepath.empty()) {
    return false;
  }
  for(unsigned int i = 1; i < 10000; ++i) {
    File newfile(strconcat(filepath, A2STR::DOT_C, util::uitos(i)));
    File ctrlfile(newfile.getPath()+DefaultBtProgressInfoFile::getSuffix());
    if(!newfile.exists() || (newfile.exists() && ctrlfile.exists())) {
      downloadContext_->getFirstFileEntry()->setPath(newfile.getPath());
      return true;
    }
  }
  return false;
}

void RequestGroup::createNextCommandWithAdj(std::vector<Command*>& commands,
                                            DownloadEngine* e, int numAdj)
{
  int numCommand;
  if(getTotalLength() == 0) {
    numCommand = 1+numAdj;
  } else {
    numCommand = std::min(downloadContext_->getNumPieces(),
                          numConcurrentCommand_);
    numCommand += numAdj;
  }
  if(numCommand > 0) {
    createNextCommand(commands, e, numCommand);
  }
}

void RequestGroup::createNextCommand(std::vector<Command*>& commands,
                                     DownloadEngine* e)
{
  int numCommand;
  if(getTotalLength() == 0) {
    if(numStreamCommand_ > 0) {
      numCommand = 0;
    } else {
      numCommand = 1;
    }
  } else {
    if(numStreamCommand_ >= numConcurrentCommand_) {
      numCommand = 0;
    } else {
      numCommand = std::min(downloadContext_->getNumPieces(),
                            numConcurrentCommand_-numStreamCommand_);
    }
  }
  if(numCommand > 0) {
    createNextCommand(commands, e, numCommand);
  }
}

void RequestGroup::createNextCommand(std::vector<Command*>& commands,
                                     DownloadEngine* e,
                                     unsigned int numCommand)
{
  for(; numCommand--; ) {
    Command* command = new CreateRequestCommand(e->newCUID(), this, e);
    commands.push_back(command);
  }
  if(!commands.empty()) {
    e->setNoWait(true);
  }
}

std::string RequestGroup::getFirstFilePath() const
{
  assert(!downloadContext_.isNull());
  if(inMemoryDownload()) {
    static const std::string DIR_MEMORY("[MEMORY]");
    return DIR_MEMORY+File(downloadContext_->getFirstFileEntry()->getPath()).getBasename();
  } else {
    return downloadContext_->getFirstFileEntry()->getPath();
  }
}

uint64_t RequestGroup::getTotalLength() const
{
  if(pieceStorage_.isNull()) {
    return 0;
  } else {
    if(pieceStorage_->isSelectiveDownloadingMode()) {
      return pieceStorage_->getFilteredTotalLength();
    } else {
      return pieceStorage_->getTotalLength();
    }
  }
}

uint64_t RequestGroup::getCompletedLength() const
{
  if(pieceStorage_.isNull()) {
    return 0;
  } else {
    if(pieceStorage_->isSelectiveDownloadingMode()) {
      return pieceStorage_->getFilteredCompletedLength();
    } else {
      return pieceStorage_->getCompletedLength();
    }
  }
}

void RequestGroup::validateFilename(const std::string& expectedFilename,
                                    const std::string& actualFilename) const
{
  if(expectedFilename.empty()) {
    return;
  }
  if(expectedFilename != actualFilename) {
    throw DL_ABORT_EX(StringFormat(EX_FILENAME_MISMATCH,
                                   expectedFilename.c_str(),
                                   actualFilename.c_str()).str());
  }
}

void RequestGroup::validateTotalLength(uint64_t expectedTotalLength,
                                       uint64_t actualTotalLength) const
{
  if(expectedTotalLength <= 0) {
    return;
  }
  if(expectedTotalLength != actualTotalLength) {
    throw DL_ABORT_EX
      (StringFormat(EX_SIZE_MISMATCH,
                    util::itos(expectedTotalLength, true).c_str(),
                    util::itos(actualTotalLength, true).c_str()).str());
  }
}

void RequestGroup::validateFilename(const std::string& actualFilename) const
{
  validateFilename(downloadContext_->getFileEntries().front()->getBasename(), actualFilename);
}

void RequestGroup::validateTotalLength(uint64_t actualTotalLength) const
{
  validateTotalLength(getTotalLength(), actualTotalLength);
}

void RequestGroup::increaseStreamCommand()
{
  ++numStreamCommand_;
}

void RequestGroup::decreaseStreamCommand()
{
  --numStreamCommand_;
}

void RequestGroup::increaseStreamConnection()
{
  ++numStreamConnection_;
}

void RequestGroup::decreaseStreamConnection()
{
  --numStreamConnection_;
}

unsigned int RequestGroup::getNumConnection() const
{
  unsigned int numConnection = numStreamConnection_;
#ifdef ENABLE_BITTORRENT
  if(!btRuntime_.isNull()) {
    numConnection += btRuntime_->getConnections();
  }
#endif // ENABLE_BITTORRENT
  return numConnection;
}

void RequestGroup::increaseNumCommand()
{
  ++numCommand_;
}

void RequestGroup::decreaseNumCommand()
{
  --numCommand_;
  if(!numCommand_ && requestGroupMan_) {
    if(logger_->debug()) {
      logger_->debug("GID#%s - Request queue check", util::itos(gid_).c_str());
    }
    requestGroupMan_->requestQueueCheck();
  }
}


TransferStat RequestGroup::calculateStat() const
{
  TransferStat stat;
#ifdef ENABLE_BITTORRENT
  if(!peerStorage_.isNull()) {
    stat = peerStorage_->calculateStat();
  }
#endif // ENABLE_BITTORRENT
  if(!segmentMan_.isNull()) {
    stat.setDownloadSpeed
      (stat.getDownloadSpeed()+segmentMan_->calculateDownloadSpeed());
    stat.setSessionDownloadLength
      (stat.getSessionDownloadLength()+
       segmentMan_->calculateSessionDownloadLength());
  }
  return stat;
}

void RequestGroup::setHaltRequested(bool f, HaltReason haltReason)
{
  haltRequested_ = f;
  if(haltRequested_) {
    pauseRequested_ = false;
    haltReason_ = haltReason;
  }
#ifdef ENABLE_BITTORRENT
  if(!btRuntime_.isNull()) {
    btRuntime_->setHalt(f);
  }
#endif // ENABLE_BITTORRENT
}

void RequestGroup::setForceHaltRequested(bool f, HaltReason haltReason)
{
  setHaltRequested(f, haltReason);
  forceHaltRequested_ = f;
}

void RequestGroup::setPauseRequested(bool f)
{
  pauseRequested_ = f;
}

void RequestGroup::releaseRuntimeResource(DownloadEngine* e)
{
#ifdef ENABLE_BITTORRENT
  e->getBtRegistry()->remove(gid_);
#endif // ENABLE_BITTORRENT
  if(!pieceStorage_.isNull()) {
    pieceStorage_->removeAdvertisedPiece(0);
  }
  // Don't reset segmentMan_ and pieceStorage_ here to provide
  // progress information via XML-RPC
  progressInfoFile_.reset();
  downloadContext_->releaseRuntimeResource();
}

void RequestGroup::preDownloadProcessing()
{
  if(logger_->debug()) {
    logger_->debug("Finding PreDownloadHandler for path %s.",
                   getFirstFilePath().c_str());
  }
  try {
    for(std::vector<SharedHandle<PreDownloadHandler> >::const_iterator itr =
          preDownloadHandlers_.begin(), eoi = preDownloadHandlers_.end();
        itr != eoi; ++itr) {
      if((*itr)->canHandle(this)) {
        (*itr)->execute(this);
        return;
      }
    }
  } catch(RecoverableException& ex) {
    logger_->error(EX_EXCEPTION_CAUGHT, ex);
    return;
  }
  if(logger_->debug()) {
    logger_->debug("No PreDownloadHandler found.");
  }
  return;
}

void RequestGroup::postDownloadProcessing
(std::vector<SharedHandle<RequestGroup> >& groups)
{
  if(logger_->debug()) {
    logger_->debug("Finding PostDownloadHandler for path %s.",
                   getFirstFilePath().c_str());
  }
  try {
    for(std::vector<SharedHandle<PostDownloadHandler> >::const_iterator itr =
          postDownloadHandlers_.begin(), eoi = postDownloadHandlers_.end();
        itr != eoi; ++itr) {
      if((*itr)->canHandle(this)) {
        (*itr)->getNextRequestGroups(groups, this);
        return;
      }
    }
  } catch(RecoverableException& ex) {
    logger_->error(EX_EXCEPTION_CAUGHT, ex);
  }
  if(logger_->debug()) {
    logger_->debug("No PostDownloadHandler found.");
  }
}

void RequestGroup::initializePreDownloadHandler()
{
#ifdef ENABLE_BITTORRENT
  if(option_->get(PREF_FOLLOW_TORRENT) == V_MEM) {
    preDownloadHandlers_.push_back(DownloadHandlerFactory::getBtPreDownloadHandler());
  }
#endif // ENABLE_BITTORRENT
#ifdef ENABLE_METALINK
  if(option_->get(PREF_FOLLOW_METALINK) == V_MEM) {
    preDownloadHandlers_.push_back(DownloadHandlerFactory::getMetalinkPreDownloadHandler());
  }
#endif // ENABLE_METALINK
}

void RequestGroup::initializePostDownloadHandler()
{
#ifdef ENABLE_BITTORRENT
  if(option_->getAsBool(PREF_FOLLOW_TORRENT) ||
     option_->get(PREF_FOLLOW_TORRENT) == V_MEM) {
    postDownloadHandlers_.push_back(DownloadHandlerFactory::getBtPostDownloadHandler());
  }
#endif // ENABLE_BITTORRENT
#ifdef ENABLE_METALINK
  if(option_->getAsBool(PREF_FOLLOW_METALINK) ||
     option_->get(PREF_FOLLOW_METALINK) == V_MEM) {
    postDownloadHandlers_.push_back(DownloadHandlerFactory::getMetalinkPostDownloadHandler());
  }
#endif // ENABLE_METALINK
}

bool RequestGroup::isDependencyResolved()
{
  if(dependency_.isNull()) {
    return true;
  }
  return dependency_->resolve();
}

void RequestGroup::dependsOn(const DependencyHandle& dep)
{
  dependency_ = dep;
}

void RequestGroup::setDiskWriterFactory(const DiskWriterFactoryHandle& diskWriterFactory)
{
  diskWriterFactory_ = diskWriterFactory;
}

void RequestGroup::addPostDownloadHandler
(const SharedHandle<PostDownloadHandler>& handler)
{
  postDownloadHandlers_.push_back(handler);
}

void RequestGroup::addPreDownloadHandler
(const SharedHandle<PreDownloadHandler>& handler)
{
  preDownloadHandlers_.push_back(handler);
}

void RequestGroup::clearPostDownloadHandler()
{
  postDownloadHandlers_.clear();
}

void RequestGroup::clearPreDownloadHandler()
{
  preDownloadHandlers_.clear();
}

void RequestGroup::setPieceStorage(const PieceStorageHandle& pieceStorage)
{
  pieceStorage_ = pieceStorage;
}

void RequestGroup::setProgressInfoFile(const BtProgressInfoFileHandle& progressInfoFile)
{
  progressInfoFile_ = progressInfoFile;
}

bool RequestGroup::needsFileAllocation() const
{
  return isFileAllocationEnabled() &&
    (uint64_t)option_->getAsLLInt(PREF_NO_FILE_ALLOCATION_LIMIT) <= getTotalLength() &&
    !pieceStorage_->getDiskAdaptor()->fileAllocationIterator()->finished();
}

DownloadResultHandle RequestGroup::createDownloadResult() const
{
  if(logger_->debug()) {
    logger_->debug("GID#%s - Creating DownloadResult.",
                   util::itos(gid_).c_str());
  }
  TransferStat st = calculateStat();
  SharedHandle<DownloadResult> res(new DownloadResult());
  res->gid = gid_;
  res->fileEntries = downloadContext_->getFileEntries();
  res->inMemoryDownload = inMemoryDownload_;
  res->sessionDownloadLength = st.getSessionDownloadLength();
  res->sessionTime = downloadContext_->calculateSessionTime();
  res->result = downloadResult();
  res->followedBy = followedByGIDs_;
  res->belongsTo = belongsToGID_;
  res->option = option_;
  res->metadataInfo = metadataInfo_;
  res->totalLength = getTotalLength();
  res->completedLength = getCompletedLength();
  res->uploadLength = st.getAllTimeUploadLength();
  if(!pieceStorage_.isNull()) {
    if(pieceStorage_->getBitfieldLength() > 0) {
      res->bitfieldStr = util::toHex(pieceStorage_->getBitfield(),
                                     pieceStorage_->getBitfieldLength());
    }
  }
#ifdef ENABLE_BITTORRENT
  if(downloadContext_->hasAttribute(bittorrent::BITTORRENT)) {
    res->infoHashStr = bittorrent::getInfoHashString(downloadContext_);
  }
#endif // ENABLE_BITTORRENT
  res->pieceLength = downloadContext_->getPieceLength();
  res->numPieces = downloadContext_->getNumPieces();
  res->dir = downloadContext_->getDir();
  return res;
}
  
void RequestGroup::reportDownloadFinished()
{
  logger_->notice(MSG_FILE_DOWNLOAD_COMPLETED,
                  downloadContext_->getBasePath().c_str());
  uriSelector_->resetCounters();
#ifdef ENABLE_BITTORRENT
  if(downloadContext_->hasAttribute(bittorrent::BITTORRENT)) {
    TransferStat stat = calculateStat();
    double shareRatio =
      ((stat.getAllTimeUploadLength()*10)/getCompletedLength())/10.0;
    SharedHandle<TorrentAttribute> attrs =
      bittorrent::getTorrentAttrs(downloadContext_);
    if(!attrs->metadata.empty()) {
      logger_->notice(MSG_SHARE_RATIO_REPORT,
                      shareRatio,
                      util::abbrevSize(stat.getAllTimeUploadLength()).c_str(),
                      util::abbrevSize(getCompletedLength()).c_str());
    }
  }
#endif // ENABLE_BITTORRENT
}

void RequestGroup::addAcceptType(const std::string& type)
{
  if(std::find(acceptTypes_.begin(), acceptTypes_.end(), type) == acceptTypes_.end()) {
    acceptTypes_.push_back(type);
  }
}

void RequestGroup::removeAcceptType(const std::string& type)
{
  acceptTypes_.erase(std::remove(acceptTypes_.begin(), acceptTypes_.end(), type),
                     acceptTypes_.end());
}

void RequestGroup::setURISelector(const SharedHandle<URISelector>& uriSelector)
{
  uriSelector_ = uriSelector;
}

void RequestGroup::applyLastModifiedTimeToLocalFiles()
{
  if(!pieceStorage_.isNull() && lastModifiedTime_.good()) {
    time_t t = lastModifiedTime_.getTime();
    logger_->info("Applying Last-Modified time: %s in local time zone",
                  ctime(&t));
    size_t n =
      pieceStorage_->getDiskAdaptor()->utime(Time(), lastModifiedTime_);
    logger_->info("Last-Modified attrs of %lu files were updated.",
                  static_cast<unsigned long>(n));
  }
}

void RequestGroup::updateLastModifiedTime(const Time& time)
{
  if(time.good() && lastModifiedTime_ < time) {
    lastModifiedTime_ = time;
  }
}

void RequestGroup::increaseAndValidateFileNotFoundCount()
{
  ++fileNotFoundCount_;
  const unsigned int maxCount = option_->getAsInt(PREF_MAX_FILE_NOT_FOUND);
  if(maxCount > 0 && fileNotFoundCount_ >= maxCount &&
     (segmentMan_.isNull() ||
      segmentMan_->calculateSessionDownloadLength() == 0)) {
    throw DOWNLOAD_FAILURE_EXCEPTION2
      (StringFormat("Reached max-file-not-found count=%u", maxCount).str(),
       downloadresultcode::MAX_FILE_NOT_FOUND);
  }
}

void RequestGroup::markInMemoryDownload()
{
  inMemoryDownload_ = true;
}

void RequestGroup::setTimeout(time_t timeout)
{
  timeout_ = timeout;
}

bool RequestGroup::doesDownloadSpeedExceed()
{
  return maxDownloadSpeedLimit_ > 0 &&
    maxDownloadSpeedLimit_ < calculateStat().getDownloadSpeed();
}

bool RequestGroup::doesUploadSpeedExceed()
{
  return maxUploadSpeedLimit_ > 0 &&
    maxUploadSpeedLimit_ < calculateStat().getUploadSpeed();
}

void RequestGroup::setLastUriResult
(const std::string uri, downloadresultcode::RESULT result)
{
  lastUriResult_.reset(new URIResult(uri, result));
}

void RequestGroup::saveControlFile() const
{
  if(saveControlFile_) {
    progressInfoFile_->save();
  }
}

void RequestGroup::removeControlFile() const
{
  progressInfoFile_->removeFile();
}

void RequestGroup::setDownloadContext
(const SharedHandle<DownloadContext>& downloadContext)
{
  downloadContext_ = downloadContext;
  if(!downloadContext_.isNull()) {
    downloadContext_->setOwnerRequestGroup(this);
  }
}

bool RequestGroup::p2pInvolved() const
{
#ifdef ENABLE_BITTORRENT
  return downloadContext_->hasAttribute(bittorrent::BITTORRENT);
#else // !ENABLE_BITTORRENT
  return false;
#endif // !ENABLE_BITTORRENT
}

gid_t RequestGroup::newGID()
{
  if(gidCounter_ == INT64_MAX) {
    gidCounter_ = 0;
  }
  return ++gidCounter_;
}

} // namespace aria2
