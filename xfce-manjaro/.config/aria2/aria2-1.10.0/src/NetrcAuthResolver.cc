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
#include "NetrcAuthResolver.h"
#include "AuthConfig.h"
#include "Netrc.h"

namespace aria2 {

NetrcAuthResolver::NetrcAuthResolver():ignoreDefault_(false) {}

SharedHandle<AuthConfig>
NetrcAuthResolver::resolveAuthConfig(const std::string& hostname)
{
  if(getUserDefinedAuthConfig().isNull()) {
    return findNetrcAuthenticator(hostname);
  } else {
    return getUserDefinedAuthConfig();
  }
}

SharedHandle<AuthConfig>
NetrcAuthResolver::findNetrcAuthenticator(const std::string& hostname) const
{
  if(netrc_.isNull()) {
    return getDefaultAuthConfig();
  } else {
    SharedHandle<Authenticator> auth = netrc_->findAuthenticator(hostname);
    if(auth.isNull()) {
      return getDefaultAuthConfig();
    } else {
      if(ignoreDefault_ && auth->getMachine().empty()) {
        return getDefaultAuthConfig();
      } else {
        return SharedHandle<AuthConfig>
          (new AuthConfig(auth->getLogin(), auth->getPassword()));
      }
    }
  }
}

void NetrcAuthResolver::setNetrc(const SharedHandle<Netrc>& netrc)
{
  netrc_ = netrc;
}

void NetrcAuthResolver::ignoreDefault()
{
  ignoreDefault_ = true;
}

void NetrcAuthResolver::useDefault()
{
  ignoreDefault_ = false;
}

} // namespace aria2