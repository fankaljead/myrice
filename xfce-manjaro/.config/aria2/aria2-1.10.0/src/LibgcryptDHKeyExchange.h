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
#ifndef _D_LIBGCRYPT_DH_KEY_EXCHANGE_H_
#define _D_LIBGCRYPT_DH_KEY_EXCHANGE_H_

#include "common.h"
#include "DlAbortEx.h"
#include "StringFormat.h"
#include <gcrypt.h>

namespace aria2 {

class DHKeyExchange {
private:
  size_t keyLength_;

  gcry_mpi_t prime_;

  gcry_mpi_t generator_;

  gcry_mpi_t privateKey_;

  gcry_mpi_t publicKey_;

  void handleError(gcry_error_t err) const
  {
    throw DL_ABORT_EX
      (StringFormat("Exception in libgcrypt routine(DHKeyExchange class): %s",
                    gcry_strerror(err)).str());
  }
public:
  DHKeyExchange():
    keyLength_(0),
    prime_(0),
    generator_(0),
    privateKey_(0),
    publicKey_(0) {}

  ~DHKeyExchange()
  {
    gcry_mpi_release(prime_);
    gcry_mpi_release(generator_);
    gcry_mpi_release(privateKey_);
    gcry_mpi_release(publicKey_);
  }

  void init(const unsigned char* prime, size_t primeBits,
            const unsigned char* generator,
            size_t privateKeyBits)
  {
    gcry_mpi_release(prime_);
    gcry_mpi_release(generator_);
    gcry_mpi_release(privateKey_);
    {
      gcry_error_t r = gcry_mpi_scan(&prime_, GCRYMPI_FMT_HEX,
                                     prime, 0, 0);
      if(r) {
        handleError(r);
      }
    }
    {
      gcry_error_t r = gcry_mpi_scan(&generator_, GCRYMPI_FMT_HEX,
                                     generator, 0, 0);
      if(r) {
        handleError(r);
      }
    }
    privateKey_ = gcry_mpi_new(0);
    gcry_mpi_randomize(privateKey_, privateKeyBits, GCRY_STRONG_RANDOM);
    keyLength_ = (primeBits+7)/8;
  }

  void generatePublicKey()
  {
    gcry_mpi_release(publicKey_);
    publicKey_ = gcry_mpi_new(0);
    gcry_mpi_powm(publicKey_, generator_, privateKey_, prime_);
  }

  size_t getPublicKey(unsigned char* out, size_t outLength) const
  {
    if(outLength < keyLength_) {
      throw DL_ABORT_EX
        (StringFormat("Insufficient buffer for public key. expect:%u, actual:%u",
                      keyLength_, outLength).str());
    }
    memset(out, 0, outLength);
    size_t publicKeyBytes = (gcry_mpi_get_nbits(publicKey_)+7)/8;
    size_t offset = keyLength_-publicKeyBytes;
    size_t nwritten;
    gcry_error_t r = gcry_mpi_print(GCRYMPI_FMT_USG, out+offset,
                                    outLength-offset, &nwritten, publicKey_);
    if(r) {
      handleError(r);
    }
    return nwritten;
  }

  void generateNonce(unsigned char* out, size_t outLength) const
  {
    gcry_create_nonce(out, outLength);
  }

  size_t computeSecret(unsigned char* out, size_t outLength,
                       const unsigned char* peerPublicKeyData,
                       size_t peerPublicKeyLength) const
  {
    if(outLength < keyLength_) {
      throw DL_ABORT_EX
        (StringFormat("Insufficient buffer for secret. expect:%u, actual:%u",
                      keyLength_, outLength).str());
    }
    gcry_mpi_t peerPublicKey;
    {
      gcry_error_t r = gcry_mpi_scan(&peerPublicKey, GCRYMPI_FMT_USG, peerPublicKeyData,
                                     peerPublicKeyLength, 0);
      if(r) {
        handleError(r);
      }
    }
    gcry_mpi_t secret = gcry_mpi_new(0);
    gcry_mpi_powm(secret, peerPublicKey, privateKey_, prime_);
    gcry_mpi_release(peerPublicKey);

    memset(out, 0, outLength);
    size_t secretBytes = (gcry_mpi_get_nbits(secret)+7)/8;
    size_t offset = keyLength_-secretBytes;
    size_t nwritten;
    {
      gcry_error_t r = gcry_mpi_print(GCRYMPI_FMT_USG, out+offset,
                                      outLength-offset, &nwritten, secret);
      gcry_mpi_release(secret);
      if(r) {
        handleError(r);
      }
    }
    return nwritten;
  }
};

} // namespace aria2

#endif // _D_LIBGCRYPT_DH_KEY_EXCHANGE_H_
