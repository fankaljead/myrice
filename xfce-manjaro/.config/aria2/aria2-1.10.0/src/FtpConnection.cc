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
#include "FtpConnection.h"

#include <cstring>
#include <cstdio>
#include <cassert>

#include "Request.h"
#include "Segment.h"
#include "Option.h"
#include "util.h"
#include "message.h"
#include "prefs.h"
#include "LogFactory.h"
#include "Logger.h"
#include "AuthConfigFactory.h"
#include "AuthConfig.h"
#include "DlRetryEx.h"
#include "DlAbortEx.h"
#include "Socket.h"
#include "A2STR.h"
#include "StringFormat.h"
#include "AuthConfig.h"
#include "a2functional.h"

namespace aria2 {

const std::string FtpConnection::A("A");

const std::string FtpConnection::I("I");

FtpConnection::FtpConnection(cuid_t cuid, const SocketHandle& socket,
                             const SharedHandle<Request>& req,
                             const SharedHandle<AuthConfig>& authConfig,
                             const Option* op):
  cuid_(cuid), socket_(socket), req_(req),
  authConfig_(authConfig), option_(op),
  logger_(LogFactory::getInstance()),
  socketBuffer_(socket),
  baseWorkingDir_("/") {}

FtpConnection::~FtpConnection() {}

bool FtpConnection::sendUser()
{
  if(socketBuffer_.sendBufferIsEmpty()) {
    std::string request = "USER ";
    request += authConfig_->getUser();
    request += "\r\n";
    if(logger_->info()) {
      logger_->info(MSG_SENDING_REQUEST,
                    util::itos(cuid_).c_str(),"USER ********");
    }
    socketBuffer_.pushStr(request);
  }
  socketBuffer_.send();
  return socketBuffer_.sendBufferIsEmpty();
}

bool FtpConnection::sendPass()
{
  if(socketBuffer_.sendBufferIsEmpty()) {
    std::string request = "PASS ";
    request += authConfig_->getPassword();
    request += "\r\n";
    if(logger_->info()) {
      logger_->info(MSG_SENDING_REQUEST,
                    util::itos(cuid_).c_str(),"PASS ********");
    }
    socketBuffer_.pushStr(request);
  }
  socketBuffer_.send();
  return socketBuffer_.sendBufferIsEmpty();
}

bool FtpConnection::sendType()
{
  if(socketBuffer_.sendBufferIsEmpty()) {
    std::string type;
    if(option_->get(PREF_FTP_TYPE) == V_ASCII) {
      type = FtpConnection::A;
    } else {
      type = FtpConnection::I;
    }
    std::string request = "TYPE ";
    request += type;
    request += "\r\n";
    if(logger_->info()) {
      logger_->info(MSG_SENDING_REQUEST,
                    util::itos(cuid_).c_str(),request.c_str());
    }
    socketBuffer_.pushStr(request);
  }
  socketBuffer_.send();
  return socketBuffer_.sendBufferIsEmpty();
}

bool FtpConnection::sendPwd()
{
  if(socketBuffer_.sendBufferIsEmpty()) {
    std::string request = "PWD\r\n";
    if(logger_->info()) {
      logger_->info(MSG_SENDING_REQUEST,
                    util::itos(cuid_).c_str(),request.c_str());
    }
    socketBuffer_.pushStr(request);
  }
  socketBuffer_.send();
  return socketBuffer_.sendBufferIsEmpty();
}

bool FtpConnection::sendCwd(const std::string& dir)
{
  if(socketBuffer_.sendBufferIsEmpty()) {
    std::string request = "CWD ";
    request += util::percentDecode(dir);
    request += "\r\n";
    if(logger_->info()) {
      logger_->info(MSG_SENDING_REQUEST,
                    util::itos(cuid_).c_str(),request.c_str());
    }
    socketBuffer_.pushStr(request);
  }
  socketBuffer_.send();
  return socketBuffer_.sendBufferIsEmpty();
}

bool FtpConnection::sendMdtm()
{
  if(socketBuffer_.sendBufferIsEmpty()) {
    std::string request = "MDTM ";
    request += util::percentDecode(req_->getFile());
    request += "\r\n";
    if(logger_->info()) {
      logger_->info(MSG_SENDING_REQUEST,
                    util::itos(cuid_).c_str(), request.c_str());
    }
    socketBuffer_.pushStr(request);
  }
  socketBuffer_.send();
  return socketBuffer_.sendBufferIsEmpty();
}

bool FtpConnection::sendSize()
{
  if(socketBuffer_.sendBufferIsEmpty()) {
    std::string request = "SIZE ";
    request += util::percentDecode(req_->getFile());
    request += "\r\n";
    if(logger_->info()) {
      logger_->info(MSG_SENDING_REQUEST,
                    util::itos(cuid_).c_str(), request.c_str());
    }
    socketBuffer_.pushStr(request);
  }
  socketBuffer_.send();
  return socketBuffer_.sendBufferIsEmpty();
}

bool FtpConnection::sendPasv()
{
  if(socketBuffer_.sendBufferIsEmpty()) {
    static const std::string request("PASV\r\n");
    if(logger_->info()) {
      logger_->info(MSG_SENDING_REQUEST,
                    util::itos(cuid_).c_str(), request.c_str());
    }
    socketBuffer_.pushStr(request);
  }
  socketBuffer_.send();
  return socketBuffer_.sendBufferIsEmpty();
}

SharedHandle<SocketCore> FtpConnection::createServerSocket()
{
  SharedHandle<SocketCore> serverSocket(new SocketCore());
  serverSocket->bind(0);
  serverSocket->beginListen();
  serverSocket->setNonBlockingMode();
  return serverSocket;
}

bool FtpConnection::sendPort(const SharedHandle<SocketCore>& serverSocket)
{
  if(socketBuffer_.sendBufferIsEmpty()) {
    std::pair<std::string, uint16_t> addrinfo;
    socket_->getAddrInfo(addrinfo);
    unsigned int ipaddr[4]; 
    sscanf(addrinfo.first.c_str(), "%u.%u.%u.%u",
           &ipaddr[0], &ipaddr[1], &ipaddr[2], &ipaddr[3]);
    serverSocket->getAddrInfo(addrinfo);
    std::string request = "PORT ";
    request += util::uitos(ipaddr[0]);
    request += ",";
    request += util::uitos(ipaddr[1]);
    request += ",";
    request += util::uitos(ipaddr[2]);
    request += ",";
    request += util::uitos(ipaddr[3]);
    request += ",";
    request += util::uitos(addrinfo.second/256);
    request += ",";
    request += util::uitos(addrinfo.second%256);
    request += "\r\n";
    if(logger_->info()) {
      logger_->info(MSG_SENDING_REQUEST,
                    util::itos(cuid_).c_str(), request.c_str());
    }
    socketBuffer_.pushStr(request);
  }
  socketBuffer_.send();
  return socketBuffer_.sendBufferIsEmpty();
}

bool FtpConnection::sendRest(const SharedHandle<Segment>& segment)
{
  if(socketBuffer_.sendBufferIsEmpty()) {
    std::string request = "REST ";
    if(segment.isNull()) {
      request += "0";
    } else {
      request += util::itos(segment->getPositionToWrite());
    }
    request += "\r\n";    
    if(logger_->info()) {
      logger_->info(MSG_SENDING_REQUEST,
                    util::itos(cuid_).c_str(), request.c_str());
    }
    socketBuffer_.pushStr(request);
  }
  socketBuffer_.send();
  return socketBuffer_.sendBufferIsEmpty();
}

bool FtpConnection::sendRetr()
{
  if(socketBuffer_.sendBufferIsEmpty()) {
    std::string request = "RETR ";
    request += util::percentDecode(req_->getFile());
    request += "\r\n";
    if(logger_->info()) {
      logger_->info(MSG_SENDING_REQUEST,
                    util::itos(cuid_).c_str(), request.c_str());
    }
    socketBuffer_.pushStr(request);
  }
  socketBuffer_.send();
  return socketBuffer_.sendBufferIsEmpty();
}

unsigned int FtpConnection::getStatus(const std::string& response) const
{
  unsigned int status;
  // When the response is not like "%u %*s",
  // we return 0.
  if(response.find_first_not_of("0123456789") != 3
     || !(response.find(" ") == 3 || response.find("-") == 3)) {
    return 0;
  }
  if(sscanf(response.c_str(), "%u %*s", &status) == 1) {
    return status;
  } else {
    return 0;
  }
}

// Returns the length of the reponse if the whole response has been received.
// The length includes \r\n.
// If the whole response has not been received, then returns std::string::npos.
std::string::size_type
FtpConnection::findEndOfResponse(unsigned int status,
                                 const std::string& buf) const
{
  if(buf.size() <= 4) {
    return std::string::npos;
  }
  // if 4th character of buf is '-', then multi line response is expected.
  if(buf.at(3) == '-') {
    // multi line response
    std::string::size_type p;

    std::string endPattern = A2STR::CRLF;
    endPattern += util::uitos(status);
    endPattern += " ";
    p = buf.find(endPattern);
    if(p == std::string::npos) {
      return std::string::npos;
    }
    p = buf.find(A2STR::CRLF, p+6);
    if(p == std::string::npos) {
      return std::string::npos;
    } else {
      return p+2;
    }
  } else {
    // single line response
    std::string::size_type p = buf.find(A2STR::CRLF);    
    if(p == std::string::npos) {
      return std::string::npos;
    } else {
      return p+2;
    }
  }
}

bool FtpConnection::bulkReceiveResponse
(std::pair<unsigned int, std::string>& response)
{
  char buf[1024];  
  while(socket_->isReadable(0)) {
    size_t size = sizeof(buf);
    socket_->readData(buf, size);
    if(size == 0) {
      if(socket_->wantRead() || socket_->wantWrite()) {
        return false;
      }
      throw DL_RETRY_EX(EX_GOT_EOF);
    }
    if(strbuf_.size()+size > MAX_RECV_BUFFER) {
      throw DL_RETRY_EX
        (StringFormat("Max FTP recv buffer reached. length=%lu",
                      static_cast<unsigned long>(strbuf_.size()+size)).str());
    }
    strbuf_.append(&buf[0], &buf[size]);
  }
  unsigned int status;
  if(strbuf_.size() >= 4) {
    status = getStatus(strbuf_);
    if(status == 0) {
      throw DL_ABORT_EX(EX_INVALID_RESPONSE);
    }
  } else {
    return false;
  }
  std::string::size_type length;
  if((length = findEndOfResponse(status, strbuf_)) != std::string::npos) {
    response.first = status;
    response.second = strbuf_.substr(0, length);
    if(logger_->info()) {
      logger_->info(MSG_RECEIVE_RESPONSE,
                    util::itos(cuid_).c_str(), response.second.c_str());
    }
    strbuf_.erase(0, length);
    return true;
  } else {
    // didn't receive response fully.
    return false;
  }
}

unsigned int FtpConnection::receiveResponse()
{
  std::pair<unsigned int, std::string> response;
  if(bulkReceiveResponse(response)) {
    return response.first;
  } else {
    return 0;
  }
}

#ifdef __MINGW32__
# define LONGLONG_PRINTF "%I64d"
# define ULONGLONG_PRINTF "%I64u"
# define LONGLONG_SCANF "%I64d"
# define ULONGLONG_SCANF "%I64u"
#else
# define LONGLONG_PRINTF "%lld"
# define ULONGLONG_PRINTF "%llu"
# define LONGLONG_SCANF "%Ld"
// Mac OSX uses "%llu" for 64bits integer.
# define ULONGLONG_SCANF "%Lu"
#endif // __MINGW32__

unsigned int FtpConnection::receiveSizeResponse(uint64_t& size)
{
  std::pair<unsigned int, std::string> response;
  if(bulkReceiveResponse(response)) {
    if(response.first == 213) {
      std::pair<std::string, std::string> rp = util::split(response.second," ");
      size = util::parseULLInt(rp.second);
    }
    return response.first;
  } else {
    return 0;
  }
}

unsigned int FtpConnection::receiveMdtmResponse(Time& time)
{
  // MDTM command, specified in RFC3659.
  std::pair<unsigned int, std::string> response;
  if(bulkReceiveResponse(response)) {
    if(response.first == 213) {
      char buf[15]; // YYYYMMDDhhmmss+\0, milli second part is dropped.
      sscanf(response.second.c_str(), "%*u %14s", buf);
      if(strlen(buf) == 14) {
        // We don't use Time::parse(buf,"%Y%m%d%H%M%S") here because Mac OS X
        // and included strptime doesn't parse data for this format.
        struct tm tm;
        memset(&tm, 0, sizeof(tm));
        tm.tm_sec = util::parseInt(&buf[12]);
        buf[12] = '\0';
        tm.tm_min = util::parseInt(&buf[10]);
        buf[10] = '\0';
        tm.tm_hour = util::parseInt(&buf[8]);
        buf[8] = '\0';
        tm.tm_mday = util::parseInt(&buf[6]);
        buf[6] = '\0';
        tm.tm_mon = util::parseInt(&buf[4])-1;
        buf[4] = '\0';
        tm.tm_year = util::parseInt(&buf[0])-1900;
        time = Time(timegm(&tm));
      } else {
        time = Time::null();
      }
    }
    return response.first;
  } else {
    return 0;
  }
}

unsigned int FtpConnection::receivePasvResponse
(std::pair<std::string, uint16_t>& dest)
{
  std::pair<unsigned int, std::string> response;
  if(bulkReceiveResponse(response)) {
    if(response.first == 227) {
      // we assume the format of response is "227 Entering Passive
      // Mode (h1,h2,h3,h4,p1,p2)."
      unsigned int h1, h2, h3, h4, p1, p2;
      std::string::size_type p = response.second.find("(");
      if(p >= 4) {
        sscanf(response.second.substr(response.second.find("(")).c_str(),
               "(%u,%u,%u,%u,%u,%u).",
               &h1, &h2, &h3, &h4, &p1, &p2);
        // ip address
        dest.first = util::uitos(h1);
        dest.first += A2STR::DOT_C;
        dest.first += util::uitos(h2);
        dest.first += A2STR::DOT_C;
        dest.first += util::uitos(h3);
        dest.first += A2STR::DOT_C;
        dest.first += util::uitos(h4);
        // port number
        dest.second = 256*p1+p2;
      } else {
        throw DL_RETRY_EX(EX_INVALID_RESPONSE);
      }
    }
    return response.first;
  } else {
    return 0;
  }
}

unsigned int FtpConnection::receivePwdResponse(std::string& pwd)
{
  std::pair<unsigned int, std::string> response;
  if(bulkReceiveResponse(response)) {
    if(response.first == 257) {
      std::string::size_type first;
      std::string::size_type last;

      if((first = response.second.find("\"")) != std::string::npos &&
         (last = response.second.find("\"", ++first)) != std::string::npos) {
        pwd = response.second.substr(first, last-first);
      } else {
        throw DL_ABORT_EX(EX_INVALID_RESPONSE);
      }
    }
    return response.first;
  } else {
    return 0;
  }
}

void FtpConnection::setBaseWorkingDir(const std::string& baseWorkingDir)
{
  baseWorkingDir_ = baseWorkingDir;
}

const std::string& FtpConnection::getUser() const
{
  return authConfig_->getUser();
}

} // namespace aria2
