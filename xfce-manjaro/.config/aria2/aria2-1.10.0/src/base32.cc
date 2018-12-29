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
#include "base32.h"
#include "util.h"

namespace aria2 {

namespace base32 {

static const char B32TABLE[] = {
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
  'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
  'Y', 'Z', '2', '3', '4', '5', '6', '7'
};

std::string encode(const std::string& src)
{
  std::string ret;
  size_t count = 0;
  uint64_t buf = 0;
  for(size_t i = 0; i < src.size(); ++i) {
    buf <<= 8;
    buf += src[i]&0xff;
    ++count;
    if(count == 5) {
      char temp[8];
      for(size_t j = 0; j < 8; ++j) {
        temp[7-j] = B32TABLE[buf&0x1f];
        buf >>= 5;
      }
      ret += std::string(&temp[0], &temp[8]);
      count = 0;
      buf = 0;      
    }
  }
  size_t r = 0;
  if(count == 1) {
    buf <<= 2;
    r = 2;
  } else if(count == 2) {
    buf <<= 4;
    r = 4;
  } else if(count == 3) {
    buf <<= 1;
    r = 5;
  } else if(count == 4) {
    buf <<= 3;
    r = 7;
  }
  char temp[7];
  for(size_t j = 0; j < r; ++j) {
    temp[r-1-j] = B32TABLE[buf&0x1f];
    buf >>= 5;
  }
  ret += std::string(&temp[0], &temp[r]);
  if(r) {
    ret += std::string(8-r, '=');
  }
  return ret;
}

std::string decode(const std::string& src)
{
  std::string ret;
  if(src.size()%8) {
    return ret;
  }
  bool done = false;
  for(size_t i = 0; i < src.size() && !done; i += 8) {
    uint64_t buf = 0;
    size_t bits = 0;
    for(size_t j = 0; j < 8; ++j) {
      char ch = src[i+j];
      unsigned char value;
      if('A' <= ch && ch <= 'Z') {
        value = ch-'A';
      } else if('2' <= ch && ch <= '7') {
        value = ch-'2'+26;
      } else if(ch == '=') {
        done = true;
        break;
      } else {
        ret.clear();
        return ret;
      }
      buf <<= 5;
      buf += value;
      bits += 5;
    }
    buf >>= (bits%8);
    bits = bits/8*8;
    buf = hton64(buf);
    char* p = reinterpret_cast<char*>(&buf);
    ret += std::string(&p[(64-bits)/8], &p[8]);
  }
  return ret;
}

} // namespace base32

} // namespace aria2
