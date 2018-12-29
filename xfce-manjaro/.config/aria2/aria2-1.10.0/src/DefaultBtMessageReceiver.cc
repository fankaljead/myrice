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
#include "DefaultBtMessageReceiver.h"

#include <cstring>

#include "BtHandshakeMessage.h"
#include "message.h"
#include "DownloadContext.h"
#include "Peer.h"
#include "PeerConnection.h"
#include "BtMessageDispatcher.h"
#include "BtMessageFactory.h"
#include "Logger.h"
#include "LogFactory.h"
#include "bittorrent_helper.h"
#include "BtPieceMessage.h"

namespace aria2 {

DefaultBtMessageReceiver::DefaultBtMessageReceiver():
  handshakeSent_(false)
{}

SharedHandle<BtHandshakeMessage>
DefaultBtMessageReceiver::receiveHandshake(bool quickReply)
{
  unsigned char data[BtHandshakeMessage::MESSAGE_LENGTH];
  size_t dataLength = BtHandshakeMessage::MESSAGE_LENGTH;
  bool retval = peerConnection_->receiveHandshake(data, dataLength);
  // To handle tracker's NAT-checking feature
  if(!handshakeSent_ && quickReply && dataLength >= 48) {
    handshakeSent_ = true;
    // check info_hash
    if(memcmp(bittorrent::getInfoHash(downloadContext_), &data[28],
              INFO_HASH_LENGTH) == 0) {
      sendHandshake();
    }
  }
  if(!retval) {
    return SharedHandle<BtHandshakeMessage>();
  }
  SharedHandle<BtHandshakeMessage> msg =
    messageFactory_->createHandshakeMessage(data, dataLength);
  msg->validate();
  return msg;
}

SharedHandle<BtHandshakeMessage>
DefaultBtMessageReceiver::receiveAndSendHandshake()
{
  return receiveHandshake(true);
}

void DefaultBtMessageReceiver::sendHandshake() {
  SharedHandle<BtMessage> msg =
    messageFactory_->createHandshakeMessage
    (bittorrent::getInfoHash(downloadContext_), bittorrent::getStaticPeerId());
  dispatcher_->addMessageToQueue(msg);
  dispatcher_->sendMessages();
}

BtMessageHandle DefaultBtMessageReceiver::receiveMessage() {
  size_t dataLength = 0;
  // Give 0 to PeerConnection::receiveMessage() to prevent memcpy.
  if(!peerConnection_->receiveMessage(0, dataLength)) {
    return SharedHandle<BtMessage>();
  }
  BtMessageHandle msg =
    messageFactory_->createBtMessage(peerConnection_->getBuffer(), dataLength);
  msg->validate();
  if(msg->getId() == BtPieceMessage::ID) {
    SharedHandle<BtPieceMessage> piecemsg =
      static_pointer_cast<BtPieceMessage>(msg);
    piecemsg->setRawMessage(peerConnection_->detachBuffer());
  }
  return msg;
}

void DefaultBtMessageReceiver::setDownloadContext
(const SharedHandle<DownloadContext>& downloadContext)
{
  downloadContext_ = downloadContext;
}

void DefaultBtMessageReceiver::setPeerConnection
(const WeakHandle<PeerConnection>& peerConnection)
{
  peerConnection_ = peerConnection;
}

void DefaultBtMessageReceiver::setDispatcher
(const WeakHandle<BtMessageDispatcher>& dispatcher)
{
  dispatcher_ = dispatcher;
}

void DefaultBtMessageReceiver::setBtMessageFactory
(const WeakHandle<BtMessageFactory>& factory)
{
  messageFactory_ = factory;
}

} // namespace aria2
