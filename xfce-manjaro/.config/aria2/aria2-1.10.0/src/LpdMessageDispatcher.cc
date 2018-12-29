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
#include "LpdMessageDispatcher.h"
#include "SocketCore.h"
#include "A2STR.h"
#include "util.h"
#include "Logger.h"
#include "LogFactory.h"
#include "BtConstants.h"
#include "RecoverableException.h"
#include "wallclock.h"

namespace aria2 {

LpdMessageDispatcher::LpdMessageDispatcher
(const std::string& infoHash, uint16_t port,
 const std::string& multicastAddress, uint16_t multicastPort,
 time_t interval):
  infoHash_(infoHash),
  port_(port),
  multicastAddress_(multicastAddress),
  multicastPort_(multicastPort),
  timer_(0),
  interval_(interval),
  request_(bittorrent::createLpdRequest(multicastAddress_, multicastPort_,
                                        infoHash_, port_)),
  logger_(LogFactory::getInstance()) {}

bool LpdMessageDispatcher::init(const std::string& localAddr,
                                unsigned char ttl, unsigned char loop)
{
  try {
    socket_.reset(new SocketCore(SOCK_DGRAM));
    socket_->create(AF_INET);
    if(logger_->debug()) {
      logger_->debug("Setting multicast outgoing interface=%s",
                     localAddr.c_str());
    }
    socket_->setMulticastInterface(localAddr);
    if(logger_->debug()) {
      logger_->debug("Setting multicast ttl=%u",static_cast<unsigned int>(ttl));
    }
    socket_->setMulticastTtl(ttl);
    if(logger_->debug()) {
      logger_->debug("Setting multicast loop=%u",
                     static_cast<unsigned int>(loop));
    }
    socket_->setMulticastLoop(loop);
    return true;
  } catch(RecoverableException& e) {
    logger_->error("Failed to initialize LpdMessageDispatcher.", e);
  }
  return false;
}

bool LpdMessageDispatcher::sendMessage()
{
  return
    socket_->writeData(request_.c_str(), request_.size(),
                       multicastAddress_, multicastPort_)
    == (ssize_t)request_.size();
}

bool LpdMessageDispatcher::isAnnounceReady() const
{
  return timer_.difference(global::wallclock) >= interval_;
}

void LpdMessageDispatcher::resetAnnounceTimer()
{
  timer_ = global::wallclock;
}

namespace bittorrent {

std::string createLpdRequest
(const std::string& multicastAddress, uint16_t multicastPort,
 const std::string& infoHash, uint16_t port)
{
  std::string req = "BT-SEARCH * HTTP/1.1\r\n";
  strappend(req, "Host: ", multicastAddress, A2STR::COLON_C,
            util::uitos(multicastPort), A2STR::CRLF);
  strappend(req, "Port: ", util::uitos(port), A2STR::CRLF);
  strappend(req, "Infohash: ", util::toHex(infoHash), A2STR::CRLF);
  req += "\r\n\r\n";
  return req;
}

} // namespace bittorrent

} // namespac aria2
