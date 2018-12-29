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
#ifndef _D_MESSAGE_DIGEST_HELPER_H_
#define _D_MESSAGE_DIGEST_HELPER_H_

#include "common.h"
#include "SharedHandle.h"
#include "messageDigest.h"
#include <string>

namespace aria2 {

class BinaryStream;

class MessageDigestHelper {
private:
  static MessageDigestContext* sha1Ctx_;
public:
  /**
   * Returns message digest in hexadecimal representation.
   * Digest algorithm is specified by algo.
   */
  static std::string digest(const std::string& algo, const SharedHandle<BinaryStream>& bs, off_t offset, uint64_t length);

  static std::string digest(const std::string& algo, const void* data, size_t length);

  static std::string digestString(const std::string& algo, const std::string& data)
  {
    return digest(algo, data.c_str(), data.size());
  }

  /**
   * staticSHA1DigestInit(), staticSHA1DigestFree(), staticSHA1Digest()
   * use statically declared MessageDigestContext sha1Ctx_.
   */
  /**
   * Initializes sha1Ctx_
   */
  static void staticSHA1DigestInit();

  /**
   * Frees allocated resources for sha1Ctx_
   */
  static void staticSHA1DigestFree();

  static std::string staticSHA1Digest(const SharedHandle<BinaryStream>& bs,
                                      off_t offset, uint64_t length);

  /**
   * ctx must be initialized or reseted before calling this function.
   */
  static std::string digest(MessageDigestContext* ctx,
                            const SharedHandle<BinaryStream>& bs,
                            off_t offset, uint64_t length);

  /**
   * Calculates message digest of file denoted by filename.
   * Returns message digest in hexadecimal representation.
   * Digest algorithm is specified by algo.
   */
  static std::string digest(const std::string& algo, const std::string& filename);

  /**
   * Stores *raw* message digest into md.
   * Throws exception when mdLength is less than the size of message digest.
   */
  static void digest(unsigned char* md, size_t mdLength,
                     const std::string& algo, const void* data, size_t length);
};

} // namespace aria2

#endif // _D_MESSAGE_DIGEST_HELPER_H_
