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
#include "IteratableChecksumValidator.h"

#include <cstdlib>

#include "util.h"
#include "message.h"
#include "PieceStorage.h"
#include "messageDigest.h"
#include "LogFactory.h"
#include "Logger.h"
#include "DiskAdaptor.h"
#include "FileEntry.h"
#include "BitfieldMan.h"
#include "DownloadContext.h"

namespace aria2 {

#define BUFSIZE (256*1024)
#define ALIGNMENT 512

IteratableChecksumValidator::IteratableChecksumValidator
(const SharedHandle<DownloadContext>& dctx,
 const PieceStorageHandle& pieceStorage):
  dctx_(dctx),
  pieceStorage_(pieceStorage),
  currentOffset_(0),
  logger_(LogFactory::getInstance()),
  buffer_(0) {}

IteratableChecksumValidator::~IteratableChecksumValidator()
{
#ifdef HAVE_POSIX_MEMALIGN
  free(buffer_);
#else // !HAVE_POSIX_MEMALIGN
  delete [] buffer_;
#endif // !HAVE_POSIX_MEMALIGN
}

void IteratableChecksumValidator::validateChunk()
{
  if(!finished()) {
    size_t length = pieceStorage_->getDiskAdaptor()->readData(buffer_,
                                                              BUFSIZE,
                                                              currentOffset_);
    ctx_->digestUpdate(buffer_, length);
    currentOffset_ += length;
    if(finished()) {
      std::string actualChecksum = util::toHex(ctx_->digestFinal());
      if(dctx_->getChecksum() == actualChecksum) {
        pieceStorage_->markAllPiecesDone();
      } else {
        BitfieldMan bitfield(dctx_->getPieceLength(), dctx_->getTotalLength());
        pieceStorage_->setBitfield(bitfield.getBitfield(), bitfield.getBitfieldLength());
      }
    }
  }
}

bool IteratableChecksumValidator::finished() const
{
  if((uint64_t)currentOffset_ >= dctx_->getTotalLength()) {
    pieceStorage_->getDiskAdaptor()->disableDirectIO();
    return true;
  } else {
    return false;
  }
}

uint64_t IteratableChecksumValidator::getTotalLength() const
{
  return dctx_->getTotalLength();
}

void IteratableChecksumValidator::init()
{
#ifdef HAVE_POSIX_MEMALIGN
  free(buffer_);
  buffer_ = reinterpret_cast<unsigned char*>
    (util::allocateAlignedMemory(ALIGNMENT, BUFSIZE));
#else // !HAVE_POSIX_MEMALIGN
  delete [] buffer_;
  buffer_ = new unsigned char[BUFSIZE];
#endif // !HAVE_POSIX_MEMALIGN
  pieceStorage_->getDiskAdaptor()->enableDirectIO();
  currentOffset_ = 0;
  ctx_.reset(new MessageDigestContext());
  ctx_->trySetAlgo(dctx_->getChecksumHashAlgo());
  ctx_->digestInit();
}

} // namespace aria2
