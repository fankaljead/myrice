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
#ifndef _D_DOWNLOAD_CONTEXT_H_
#define _D_DOWNLOAD_CONTEXT_H_

#include "common.h"

#include <cassert>
#include <string>
#include <vector>

#include "SharedHandle.h"
#include "Signature.h"
#include "TimerA2.h"
#include "A2STR.h"
#include "ValueBase.h"
#include "IntSequence.h"
#include "FileEntry.h"
#include "TorrentAttribute.h"

namespace aria2 {

class RequestGroup;

class DownloadContext
{
private:
  std::vector<SharedHandle<FileEntry> > fileEntries_;

  std::string dir_;

  std::vector<std::string> pieceHashes_;

  size_t pieceLength_;

  std::string pieceHashAlgo_;

  std::string checksum_;

  std::string checksumHashAlgo_;

  bool checksumVerified_;

  std::string basePath_;

  bool knowsTotalLength_;

  RequestGroup* ownerRequestGroup_;
  
  std::map<std::string, SharedHandle<ContextAttribute> > attrs_;

  Timer downloadStartTime_;

  Timer downloadStopTime_;

  SharedHandle<Signature> signature_;
public:
  DownloadContext();

  // Convenient constructor that creates single file download.  path
  // should be escaped with util::escapePath(...).
  DownloadContext(size_t pieceLength,
                  uint64_t totalLength,
                  const std::string& path = A2STR::NIL);

  const std::string& getPieceHash(size_t index) const
  {
    if(index < pieceHashes_.size()) {
      return pieceHashes_[index];
    } else {
      return A2STR::NIL;
    }
  }
  
  const std::vector<std::string>& getPieceHashes() const
  {
    return pieceHashes_;
  }

  template<typename InputIterator>
  void setPieceHashes(InputIterator first, InputIterator last)
  {
    pieceHashes_.assign(first, last);
  }

  uint64_t getTotalLength() const;

  bool knowsTotalLength() const { return knowsTotalLength_; }

  void markTotalLengthIsUnknown() { knowsTotalLength_ = false; }

  void markTotalLengthIsKnown() { knowsTotalLength_ = true; }

  const std::vector<SharedHandle<FileEntry> >& getFileEntries() const
  {
    return fileEntries_;
  }

  const SharedHandle<FileEntry>& getFirstFileEntry() const
  {
    assert(!fileEntries_.empty());
    return fileEntries_[0];
  }

  template<typename InputIterator>
  void setFileEntries(InputIterator first, InputIterator last)
  {
    fileEntries_.assign(first, last);
  }

  size_t getPieceLength() const { return pieceLength_; }

  void setPieceLength(size_t length) { pieceLength_ = length; }

  size_t getNumPieces() const;

  const std::string& getPieceHashAlgo() const { return pieceHashAlgo_; }

  void setPieceHashAlgo(const std::string& algo)
  {
    pieceHashAlgo_ = algo;
  }

  const std::string& getChecksum() const { return checksum_; }

  void setChecksum(const std::string& checksum)
  {
    checksum_ = checksum;
  }

  const std::string& getChecksumHashAlgo() const { return checksumHashAlgo_; }

  void setChecksumHashAlgo(const std::string& algo)
  {
    checksumHashAlgo_ = algo;
  }

  // The representative path name for this context. It is used as a
  // part of .aria2 control file. If basePath_ is set, returns
  // basePath_. Otherwise, the first FileEntry's getFilePath() is
  // returned.
  const std::string& getBasePath() const;

  void setBasePath(const std::string& basePath) { basePath_ = basePath; }

  const std::string& getDir() const { return dir_; }

  void setDir(const std::string& dir) { dir_ = dir; }

  const SharedHandle<Signature>& getSignature() const { return signature_; }

  void setSignature(const SharedHandle<Signature>& signature)
  {
    signature_ = signature;
  }

  RequestGroup* getOwnerRequestGroup() { return ownerRequestGroup_; }

  void setOwnerRequestGroup(RequestGroup* owner)
  {
    ownerRequestGroup_ = owner;
  }

  void setFileFilter(IntSequence seq);

  // Sets file path for specified index. index starts from 1. The
  // index is the same used in setFileFilter().  Please note that path
  // is not the actual file path. The actual file path is
  // getDir()+"/"+path. path is not escaped by util::escapePath() in
  // this function.
  void setFilePathWithIndex(size_t index, const std::string& path);

  // Returns true if hash check(whole file hash, not piece hash) is
  // need to be done
  bool isChecksumVerificationNeeded() const;

  void setChecksumVerified(bool f)
  {
    checksumVerified_ = f;
  }

  void setAttribute
  (const std::string& key, const SharedHandle<ContextAttribute>& value);

  const SharedHandle<ContextAttribute>& getAttribute(const std::string& key);

  bool hasAttribute(const std::string& key) const;

  void resetDownloadStartTime();

  void resetDownloadStopTime();

  const Timer& getDownloadStopTime() const
  {
    return downloadStopTime_;
  }

  int64_t calculateSessionTime() const;
  
  // Returns FileEntry at given offset. SharedHandle<FileEntry>() is
  // returned if no such FileEntry is found.
  SharedHandle<FileEntry> findFileEntryByOffset(off_t offset) const;

  void releaseRuntimeResource();
};

} // namespace aria2

#endif // _D_DOWNLOAD_CONTEXT_H_
