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
#include "ReceiverMSEHandshakeCommand.h"
#include "PeerReceiveHandshakeCommand.h"
#include "PeerConnection.h"
#include "DownloadEngine.h"
#include "BtHandshakeMessage.h"
#include "DlAbortEx.h"
#include "Peer.h"
#include "message.h"
#include "Socket.h"
#include "Logger.h"
#include "prefs.h"
#include "Option.h"
#include "MSEHandshake.h"
#include "ARC4Encryptor.h"
#include "ARC4Decryptor.h"
#include "RequestGroupMan.h"
#include "BtRegistry.h"
#include "DownloadContext.h"
#include "PeerStorage.h"
#include "PieceStorage.h"
#include "BtAnnounce.h"
#include "BtRuntime.h"
#include "BtProgressInfoFile.h"
#include "ServerStatMan.h"
#include "FileAllocationEntry.h"
#include "CheckIntegrityEntry.h"

namespace aria2 {

ReceiverMSEHandshakeCommand::ReceiverMSEHandshakeCommand
(cuid_t cuid,
 const SharedHandle<Peer>& peer,
 DownloadEngine* e,
 const SharedHandle<SocketCore>& s):

  PeerAbstractCommand(cuid, peer, e, s),
  sequence_(RECEIVER_IDENTIFY_HANDSHAKE),
  mseHandshake_(new MSEHandshake(cuid, s, e->getOption()))
{
  setTimeout(e->getOption()->getAsInt(PREF_PEER_CONNECTION_TIMEOUT));
}

ReceiverMSEHandshakeCommand::~ReceiverMSEHandshakeCommand()
{
  delete mseHandshake_;
}

bool ReceiverMSEHandshakeCommand::exitBeforeExecute()
{
  return getDownloadEngine()->isHaltRequested() ||
    getDownloadEngine()->getRequestGroupMan()->downloadFinished();
}

bool ReceiverMSEHandshakeCommand::executeInternal()
{
  switch(sequence_) {
  case RECEIVER_IDENTIFY_HANDSHAKE: {
    MSEHandshake::HANDSHAKE_TYPE type = mseHandshake_->identifyHandshakeType();
    switch(type) {
    case MSEHandshake::HANDSHAKE_NOT_YET:
      break;
    case MSEHandshake::HANDSHAKE_ENCRYPTED:
      mseHandshake_->initEncryptionFacility(false);
      sequence_ = RECEIVER_WAIT_KEY;
      break;
    case MSEHandshake::HANDSHAKE_LEGACY: {
      if(getDownloadEngine()->getOption()->getAsBool(PREF_BT_REQUIRE_CRYPTO)) {
        throw DL_ABORT_EX
          ("The legacy BitTorrent handshake is not acceptable by the"
           " preference.");
      }
      SharedHandle<PeerConnection> peerConnection
        (new PeerConnection(getCuid(), getSocket()));
      peerConnection->presetBuffer(mseHandshake_->getBuffer(),
                                   mseHandshake_->getBufferLength());
      Command* c = new PeerReceiveHandshakeCommand(getCuid(),
                                                   getPeer(),
                                                   getDownloadEngine(),
                                                   getSocket(),
                                                   peerConnection);
      getDownloadEngine()->addCommand(c);
      return true;
    }
    default:
      throw DL_ABORT_EX("Not supported handshake type.");
    }
    break;
  }
  case RECEIVER_WAIT_KEY: {
    if(mseHandshake_->receivePublicKey()) {
      if(mseHandshake_->sendPublicKey()) {
        sequence_ = RECEIVER_FIND_HASH_MARKER;
      } else {
        setWriteCheckSocket(getSocket());
        sequence_ = RECEIVER_SEND_KEY_PENDING;
      }
    }
    break;
  }
  case RECEIVER_SEND_KEY_PENDING:
    if(mseHandshake_->sendPublicKey()) {
      disableWriteCheckSocket();
      sequence_ = RECEIVER_FIND_HASH_MARKER;
    }
    break;
  case RECEIVER_FIND_HASH_MARKER: {
    if(mseHandshake_->findReceiverHashMarker()) {
      sequence_ = RECEIVER_RECEIVE_PAD_C_LENGTH;
    }
    break;
  }
  case RECEIVER_RECEIVE_PAD_C_LENGTH: {
    std::vector<SharedHandle<DownloadContext> > downloadContexts;
    getDownloadEngine()->getBtRegistry()->getAllDownloadContext
      (std::back_inserter(downloadContexts));
    if(mseHandshake_->receiveReceiverHashAndPadCLength(downloadContexts)) {
      sequence_ = RECEIVER_RECEIVE_PAD_C;
    }
    break;
  }
  case RECEIVER_RECEIVE_PAD_C: {
    if(mseHandshake_->receivePad()) {
      sequence_ = RECEIVER_RECEIVE_IA_LENGTH;
    }
    break;
  }
  case RECEIVER_RECEIVE_IA_LENGTH: {
    if(mseHandshake_->receiveReceiverIALength()) {
      sequence_ = RECEIVER_RECEIVE_IA;
    }
    break;
  }
  case RECEIVER_RECEIVE_IA: {
    if(mseHandshake_->receiveReceiverIA()) {
      if(mseHandshake_->sendReceiverStep2()) {
        createCommand();
        return true;
      } else {
        setWriteCheckSocket(getSocket());
        sequence_ = RECEIVER_SEND_STEP2_PENDING;
      }
    }
    break;
  }
  case RECEIVER_SEND_STEP2_PENDING:
    if(mseHandshake_->sendReceiverStep2()) {
      disableWriteCheckSocket();
      createCommand();
      return true;
    }
    break;
  }
  getDownloadEngine()->addCommand(this);
  return false;
}

void ReceiverMSEHandshakeCommand::createCommand()
{
  SharedHandle<PeerConnection> peerConnection
    (new PeerConnection(getCuid(), getSocket()));
  if(mseHandshake_->getNegotiatedCryptoType() == MSEHandshake::CRYPTO_ARC4) {
    peerConnection->enableEncryption(mseHandshake_->getEncryptor(),
                                     mseHandshake_->getDecryptor());
  }
  if(mseHandshake_->getIALength() > 0) {
    peerConnection->presetBuffer(mseHandshake_->getIA(),
                                 mseHandshake_->getIALength());
  }
  // TODO add mseHandshake_->getInfoHash() to PeerReceiveHandshakeCommand
  // as a hint. If this info hash and one in BitTorrent Handshake does not
  // match, then drop connection.
  Command* c =
    new PeerReceiveHandshakeCommand(getCuid(), getPeer(), getDownloadEngine(),
                                    getSocket(), peerConnection);
  getDownloadEngine()->addCommand(c);
}

} // namespace aria2
