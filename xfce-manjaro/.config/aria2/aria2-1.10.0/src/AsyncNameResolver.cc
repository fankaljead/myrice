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
#include "AsyncNameResolver.h"

#include <cstring>

#include "A2STR.h"

namespace aria2 {

#ifdef HAVE_LIBCARES1_5
void callback(void* arg, int status, int timeouts, struct hostent* host)
#else
  void callback(void* arg, int status, struct hostent* host)
#endif // HAVE_LIBCARES1_5
{
  AsyncNameResolver* resolverPtr = reinterpret_cast<AsyncNameResolver*>(arg);
  if(status != ARES_SUCCESS) {
    resolverPtr->error_ = ares_strerror(status);
    resolverPtr->status_ = AsyncNameResolver::STATUS_ERROR;
    return;
  }
  for(char** ap = host->h_addr_list; *ap; ++ap) {
    struct in_addr addr;
    memcpy(&addr, *ap, sizeof(in_addr));
    resolverPtr->resolvedAddresses_.push_back(inet_ntoa(addr));
  }
  resolverPtr->status_ = AsyncNameResolver::STATUS_SUCCESS;
}

AsyncNameResolver::AsyncNameResolver():
  status_(STATUS_READY)
{
  // TODO evaluate return value
  ares_init(&channel_);
}

AsyncNameResolver::~AsyncNameResolver()
{
  ares_destroy(channel_);
}

void AsyncNameResolver::resolve(const std::string& name)
{
  hostname_ = name;
  status_ = STATUS_QUERYING;
  ares_gethostbyname(channel_, name.c_str(), AF_INET, callback, this);
}

int AsyncNameResolver::getFds(fd_set* rfdsPtr, fd_set* wfdsPtr) const
{
  return ares_fds(channel_, rfdsPtr, wfdsPtr);
}

void AsyncNameResolver::process(fd_set* rfdsPtr, fd_set* wfdsPtr)
{
  ares_process(channel_, rfdsPtr, wfdsPtr);
}

#ifdef HAVE_LIBCARES

int AsyncNameResolver::getsock(sock_t* sockets) const
{
  return ares_getsock(channel_, reinterpret_cast<ares_socket_t*>(sockets),
                      ARES_GETSOCK_MAXNUM);
}

void AsyncNameResolver::process(ares_socket_t readfd, ares_socket_t writefd)
{
  ares_process_fd(channel_, readfd, writefd);
}

#endif // HAVE_LIBCARES

bool AsyncNameResolver::operator==(const AsyncNameResolver& resolver) const
{
  return this == &resolver;
}

void AsyncNameResolver::reset()
{
  hostname_ = A2STR::NIL;
  resolvedAddresses_.clear();
  status_ = STATUS_READY;
  ares_destroy(channel_);
  // TODO evaluate return value
  ares_init(&channel_);
}

} // namespace aria2
