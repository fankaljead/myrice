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
#ifndef _D_LIBSSL_ARC4_DECRYPTOR_H_
#define _D_LIBSSL_ARC4_DECRYPTOR_H_

#include "common.h"

#include <openssl/evp.h>
#include <openssl/err.h>

#include "DlAbortEx.h"
#include "LibsslARC4Context.h"
#include "StringFormat.h"

namespace aria2 {

class ARC4Decryptor {
private:
  LibsslARC4Context ctx_;

  void handleError() const
  {
    throw DL_ABORT_EX
      (StringFormat("Exception in libssl routine(ARC4Decryptor class): %s",
                    ERR_error_string(ERR_get_error(), 0)).str());
  }
public:
  ARC4Decryptor() {}

  ~ARC4Decryptor() {}

  void init(const unsigned char* key, size_t keyLength)
  {
    ctx_.init(key, keyLength, 0);
  }

  void decrypt(unsigned char* out, size_t outLength,
               const unsigned char* in, size_t inLength)
  {
    int soutLength = outLength;
    if(!EVP_CipherUpdate(ctx_.getCipherContext(), out, &soutLength, in, inLength)) {
      handleError();
    }
  }
};

} // namespace aria2

#endif // _D_LIBSSL_ARC4_DECRYPTOR_H_
