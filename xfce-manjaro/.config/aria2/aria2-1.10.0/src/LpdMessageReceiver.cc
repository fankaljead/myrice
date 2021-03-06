/*
 * aria2 - The high speed download utility
 *
 * Copyright (C) 2010 Tatsuhiro Tsujikawa
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
#include "LpdMessageReceiver.h"
#include "SocketCore.h"
#include "Logger.h"
#include "LogFactory.h"
#include "HttpHeaderProcessor.h"
#include "HttpHeader.h"
#include "util.h"
#include "LpdMessage.h"
#include "RecoverableException.h"

namespace aria2 {

LpdMessageReceiver::LpdMessageReceiver
(const std::string& multicastAddress, uint16_t multicastPort):
  multicastAddress_(multicastAddress),
  multicastPort_(multicastPort),
  logger_(LogFactory::getInstance()) {}

bool LpdMessageReceiver::init(const std::string& localAddr)
{
  try {
    socket_.reset(new SocketCore(SOCK_DGRAM));
#ifdef __MINGW32__
    // Binding multicast address fails under Windows.
    socket_->bindWithFamily(multicastPort_, AF_INET);
#else // !__MINGW32__
    socket_->bind(multicastAddress_, multicastPort_);
#endif // !__MINGW32__
    if(logger_->debug()) {
      logger_->debug("Joining multicast group. %s:%u, localAddr=%s",
                     multicastAddress_.c_str(), multicastPort_,
                     localAddr.c_str());
    }
    socket_->joinMulticastGroup(multicastAddress_, multicastPort_, localAddr);
    socket_->setNonBlockingMode();
    localAddress_ = localAddr;
    logger_->info("Listening multicast group (%s:%u) packet",
                  multicastAddress_.c_str(), multicastPort_);
    return true;
  } catch(RecoverableException& e) {
    logger_->error("Failed to initialize LPD message receiver.", e);
  }
  return false;
}

SharedHandle<LpdMessage> LpdMessageReceiver::receiveMessage()
{
  SharedHandle<LpdMessage> msg;
  try {
    unsigned char buf[200];
    std::pair<std::string, uint16_t> peerAddr;
    ssize_t length = socket_->readDataFrom(buf, sizeof(buf), peerAddr);
    if(length == 0) {
      return msg;
    }
    HttpHeaderProcessor proc;
    proc.update(buf, length);
    if(!proc.eoh()) {
      msg.reset(new LpdMessage());
      return msg;
    }
    SharedHandle<HttpHeader> header = proc.getHttpRequestHeader();
    std::string infoHashString = header->getFirst("Infohash");
    uint16_t port = header->getFirstAsUInt("Port");
    logger_->info("LPD message received infohash=%s, port=%u from %s",
                  infoHashString.c_str(), port, peerAddr.first.c_str());
    std::string infoHash;
    if(infoHashString.size() != 40 ||
       (infoHash = util::fromHex(infoHashString)).empty() ||
       port == 0) {
      logger_->info("LPD bad request. infohash=%s", infoHashString.c_str());
      msg.reset(new LpdMessage());
      return msg;
    }
    SharedHandle<Peer> peer(new Peer(peerAddr.first, port, false));
    if(util::inPrivateAddress(peerAddr.first)) {
      peer->setLocalPeer(true);
    }
    msg.reset(new LpdMessage(peer, infoHash));
    return msg;
  } catch(RecoverableException& e) {
    logger_->info("Failed to receive LPD message.", e);
    msg.reset(new LpdMessage());
    return msg;
  }
}

} // namespace aria2
