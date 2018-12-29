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
#ifndef _D_DHT_CONNECTION_IMPL_H_
#define _D_DHT_CONNECTION_IMPL_H_

#include "DHTConnection.h"
#include "SharedHandle.h"
#include "IntSequence.h"

namespace aria2 {

class SocketCore;
class Logger;

class DHTConnectionImpl:public DHTConnection {
private:
  SharedHandle<SocketCore> socket_;

  Logger* logger_;
public:
  DHTConnectionImpl();

  virtual ~DHTConnectionImpl();

  /**
   * Binds port. All number in ports are tried.
   * If successful, the binded port is assigned to port and returns true.
   * Otherwise return false and port is undefined in this case.
   */
  bool bind(uint16_t& port, IntSequence& ports);
  
  /**
   * Binds port. The port number specified by port is used to bind.
   * If successful, the binded port is assigned to port and returns true.
   * Otherwise return false and port is undefined in this case.
   *
   * If you want to bind arbitrary port, give 0 as port and if successful,
   * the binded port is assigned to port.
   */
  bool bind(uint16_t& port);

  virtual ssize_t receiveMessage(unsigned char* data, size_t len,
                                 std::string& host, uint16_t& port);

  virtual ssize_t sendMessage(const unsigned char* data, size_t len,
                              const std::string& host, uint16_t port);

  const SharedHandle<SocketCore>& getSocket() const
  {
    return socket_;
  }
};

} // namespace aria2

#endif // _D_DHT_CONNECTION_IMPL_H_
