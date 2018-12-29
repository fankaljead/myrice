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
#ifndef _D_FTP_NEGOTIATION_COMMAND_H_
#define _D_FTP_NEGOTIATION_COMMAND_H_

#include "AbstractCommand.h"

namespace aria2 {

class FtpConnection;
class SocketCore;
class HttpConnection;

class FtpNegotiationCommand : public AbstractCommand {
public:
  enum Seq {
    SEQ_RECV_GREETING,
    SEQ_SEND_USER,
    SEQ_RECV_USER,
    SEQ_SEND_PASS,
    SEQ_RECV_PASS,
    SEQ_SEND_TYPE,
    SEQ_RECV_TYPE,
    SEQ_SEND_PWD,
    SEQ_RECV_PWD,
    SEQ_SEND_CWD_PREP,
    SEQ_SEND_CWD,
    SEQ_RECV_CWD,
    SEQ_SEND_MDTM,
    SEQ_RECV_MDTM,
    SEQ_SEND_SIZE,
    SEQ_RECV_SIZE,
    SEQ_PREPARE_SERVER_SOCKET,
    SEQ_SEND_PORT,
    SEQ_RECV_PORT,
    SEQ_SEND_PASV,
    SEQ_RECV_PASV,
    SEQ_RESOLVE_PROXY,
    SEQ_SEND_TUNNEL_REQUEST,
    SEQ_RECV_TUNNEL_RESPONSE,
    SEQ_SEND_REST_PASV,
    SEQ_SEND_REST,
    SEQ_RECV_REST,
    SEQ_SEND_RETR,
    SEQ_RECV_RETR,
    SEQ_WAIT_CONNECTION,
    SEQ_NEGOTIATION_COMPLETED,
    SEQ_RETRY,
    SEQ_HEAD_OK,
    SEQ_DOWNLOAD_ALREADY_COMPLETED,
    SEQ_FILE_PREPARATION, // File allocation after SIZE command
    SEQ_EXIT
  };
private:
  bool recvGreeting();
  bool sendUser();
  bool recvUser();
  bool sendPass();
  bool recvPass();
  bool sendType();
  bool recvType();
  bool sendPwd();
  bool recvPwd();
  bool sendCwdPrep();
  bool sendCwd();
  bool recvCwd();
  bool sendMdtm();
  bool recvMdtm();
  bool sendSize();
  bool recvSize();
  bool prepareServerSocket();
  bool sendPort();
  bool recvPort();
  bool sendPasv();
  bool recvPasv();
  bool resolveProxy();
  bool sendTunnelRequest();
  bool recvTunnelResponse();
  bool sendRest(const SharedHandle<Segment>& segment);
  bool sendRestPasv(const SharedHandle<Segment>& segment);
  bool recvRest(const SharedHandle<Segment>& segment);
  bool sendRetr();
  bool recvRetr();
  bool waitConnection();
  bool processSequence(const SharedHandle<Segment>& segment);

  void afterFileAllocation();

  void poolConnection() const;

  bool onFileSizeDetermined(uint64_t totalLength);

  void onDryRunFileFound();

  SharedHandle<SocketCore> dataSocket_;
  SharedHandle<SocketCore> serverSocket_;
  Seq sequence_;
  SharedHandle<FtpConnection> ftp_;
  // For tunneling
  SharedHandle<HttpConnection> http_;
  // IP, Port pair in pasv response
  std::pair<std::string, uint16_t> dataConnAddr_;
  // Resolved address for proxy
  std::string proxyAddr_;

  std::string connectedHostname_;
  std::string connectedAddr_;
  uint16_t connectedPort_;

  std::deque<std::string> cwdDirs_;
protected:
  virtual bool executeInternal();
public:
  FtpNegotiationCommand(cuid_t cuid,
                        const SharedHandle<Request>& req,
                        const SharedHandle<FileEntry>& fileEntry,
                        RequestGroup* requestGroup,
                        DownloadEngine* e,
                        const SharedHandle<SocketCore>& s,
                        Seq seq = SEQ_RECV_GREETING,
                        const std::string& baseWorkingDir = "/");
  virtual ~FtpNegotiationCommand();

  void setConnectedAddr
  (const std::string& hostname, const std::string& addr, uint16_t port)
  {
    connectedHostname_ = hostname;
    connectedAddr_ = addr;
    connectedPort_ = port;
  }
};

} // namespace aria2

#endif // _D_FTP_NEGOTIATION_COMMAND_H_
