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
#ifndef _D_METALINK_RESOURCE_H_
#define _D_METALINK_RESOURCE_H_

#include "common.h"
#include <string>

namespace aria2 {

class MetalinkResource {
public:
  enum TYPE {
    TYPE_FTP = 0,
    TYPE_HTTP,
    TYPE_HTTPS,
    TYPE_BITTORRENT,
    TYPE_NOT_SUPPORTED,
    TYPE_UNKNOWN
  };

  static std::string type2String[];

  static const std::string HTTP;

  static const std::string HTTPS;

  static const std::string FTP;

  static const std::string BITTORRENT;

  static const std::string TORRENT; // Metalink4Spec

public:
  std::string url;
  TYPE type;
  std::string location;
  int priority;
  int maxConnections; // Metalink3Spec
public:
  MetalinkResource();
  ~MetalinkResource();

  MetalinkResource& operator=(const MetalinkResource& metalinkResource) {
    if(this != &metalinkResource) {
      this->url = metalinkResource.url;
      this->type = metalinkResource.type;
      this->location = metalinkResource.location;
      this->priority = metalinkResource.priority;
      this->maxConnections = metalinkResource.maxConnections;
    }
    return *this;
  }

  static const std::string& getTypeString(TYPE type)
  {
    return type2String[type];
  }

  static int getLowestPriority()
  {
    return 999999;
  }
};

} // namespace aria2

#endif // _D_METALINK_RESOURCE_H_
