/* <!-- copyright */
/*
 * aria2 - The high speed download utility
 *
 * Copyright (C) 2009 Tatsuhiro Tsujikawa
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
#include "HttpServerBodyCommand.h"
#include "SocketCore.h"
#include "DownloadEngine.h"
#include "HttpServer.h"
#include "HttpHeader.h"
#include "Logger.h"
#include "RequestGroup.h"
#include "RequestGroupMan.h"
#include "RecoverableException.h"
#include "HttpServerResponseCommand.h"
#include "OptionParser.h"
#include "OptionHandler.h"
#include "XmlRpcRequestProcessor.h"
#include "XmlRpcRequestParserStateMachine.h"
#include "XmlRpcMethod.h"
#include "XmlRpcMethodFactory.h"
#include "XmlRpcResponse.h"
#include "DownloadContext.h"
#include "wallclock.h"
#include "util.h"
#include "ServerStatMan.h"
#include "FileAllocationEntry.h"
#include "CheckIntegrityEntry.h"

namespace aria2 {

HttpServerBodyCommand::HttpServerBodyCommand
(cuid_t cuid,
 const SharedHandle<HttpServer>& httpServer,
 DownloadEngine* e,
 const SharedHandle<SocketCore>& socket):
  Command(cuid),
  e_(e),
  socket_(socket),
  httpServer_(httpServer)
{
  setStatus(Command::STATUS_ONESHOT_REALTIME);
  e_->addSocketForReadCheck(socket_, this);
}

HttpServerBodyCommand::~HttpServerBodyCommand()
{
  e_->deleteSocketForReadCheck(socket_, this);
}

bool HttpServerBodyCommand::execute()
{
  if(e_->getRequestGroupMan()->downloadFinished() || e_->isHaltRequested()) {
    return true;
  }
  try {
    if(socket_->isReadable(0) || httpServer_->getContentLength() == 0) {
      timeoutTimer_ = global::wallclock;

      if(httpServer_->receiveBody()) {
        // Do something for requestpath and body
        if(httpServer_->getRequestPath() == "/rpc") {
          xmlrpc::XmlRpcRequest req =
            xmlrpc::XmlRpcRequestProcessor().parseMemory(httpServer_->getBody());
          
          SharedHandle<xmlrpc::XmlRpcMethod> method =
            xmlrpc::XmlRpcMethodFactory::create(req.methodName);
          xmlrpc::XmlRpcResponse res = method->execute(req, e_);
          bool gzip = httpServer_->supportsGZip();
          std::string responseData = res.toXml(gzip);
          httpServer_->feedResponse(responseData, "text/xml");
          Command* command =
            new HttpServerResponseCommand(getCuid(), httpServer_, e_, socket_);
          e_->addCommand(command);
          e_->setNoWait(true);
          return true;
        } else {
          return true;
        }
      } else {
        e_->addCommand(this);
        return false;
      } 
    } else {
      if(timeoutTimer_.difference(global::wallclock) >= 30) {
        getLogger()->info("HTTP request body timeout.");
        return true;
      } else {
        e_->addCommand(this);
        return false;
      }
    }
  } catch(RecoverableException& e) {
    if(getLogger()->info()) {
      getLogger()->info
        ("CUID#%s - Error occurred while reading HTTP request body",
         e, util::itos(getCuid()).c_str());
    }
    return true;
  }

}

} // namespace aria2
