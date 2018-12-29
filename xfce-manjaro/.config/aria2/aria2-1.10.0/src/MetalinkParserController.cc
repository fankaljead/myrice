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
#include "MetalinkParserController.h"

#include <algorithm>

#include "Metalinker.h"
#include "MetalinkEntry.h"
#include "MetalinkResource.h"
#include "MetalinkMetaurl.h"
#include "FileEntry.h"
#include "a2functional.h"
#include "A2STR.h"
#ifdef ENABLE_MESSAGE_DIGEST
# include "Checksum.h"
# include "ChunkChecksum.h"
# include "messageDigest.h"
#endif // ENABLE_MESSAGE_DIGEST
#include "Signature.h"
#include "util.h"

namespace aria2 {

MetalinkParserController::MetalinkParserController():
  metalinker_(new Metalinker())
{}

MetalinkParserController::~MetalinkParserController() {}

void MetalinkParserController::newEntryTransaction()
{
  tEntry_.reset(new MetalinkEntry());
  tResource_.reset();
  tMetaurl_.reset();
#ifdef ENABLE_MESSAGE_DIGEST
  tChecksum_.reset();
  tChunkChecksumV4_.reset();
  tChunkChecksum_.reset();
#endif // ENABLE_MESSAGE_DIGEST
}

void MetalinkParserController::setFileNameOfEntry(const std::string& filename)
{
  if(tEntry_.isNull()) {
    return;
  }
  if(tEntry_->file.isNull()) {
    tEntry_->file.reset(new FileEntry(util::escapePath(filename), 0, 0));
  } else {
    tEntry_->file->setPath(util::escapePath(filename));
  }
}

void MetalinkParserController::setFileLengthOfEntry(uint64_t length)
{
  if(tEntry_.isNull()) {
    return;
  }
  if(tEntry_->file.isNull()) {
    return;
  }
  tEntry_->file->setLength(length);
  tEntry_->sizeKnown = true;
}

void MetalinkParserController::setVersionOfEntry(const std::string& version)
{
  if(tEntry_.isNull()) {
    return;
  }
  tEntry_->version = version;
}

void MetalinkParserController::setLanguageOfEntry(const std::string& language)
{
  if(tEntry_.isNull()) {
    return;
  }
  tEntry_->languages.push_back(language);
}

void MetalinkParserController::setOSOfEntry(const std::string& os)
{
  if(tEntry_.isNull()) {
    return;
  }
  tEntry_->oses.push_back(os);
}

void MetalinkParserController::setMaxConnectionsOfEntry(int maxConnections)
{
  if(tEntry_.isNull()) {
    return;
  }
  tEntry_->maxConnections = maxConnections;
}

void MetalinkParserController::commitEntryTransaction()
{
  if(tEntry_.isNull()) {
    return;
  }
  commitResourceTransaction();
  commitMetaurlTransaction();
  commitChecksumTransaction();
  commitChunkChecksumTransactionV4();
  commitChunkChecksumTransaction();
  commitSignatureTransaction();
  metalinker_->addEntry(tEntry_);
  tEntry_.reset();
}

void MetalinkParserController::cancelEntryTransaction()
{
  cancelResourceTransaction();
  cancelMetaurlTransaction();
  cancelChecksumTransaction();
  cancelChunkChecksumTransactionV4();
  cancelChunkChecksumTransaction();
  cancelSignatureTransaction();
  tEntry_.reset();
}

void MetalinkParserController::newResourceTransaction()
{
  if(tEntry_.isNull()) {
    return;
  }
  tResource_.reset(new MetalinkResource());
}

void MetalinkParserController::setURLOfResource(const std::string& url)
{
  if(tResource_.isNull()) {
    return;
  }
  tResource_->url = url;
  // Metalink4Spec
  if(tResource_->type == MetalinkResource::TYPE_UNKNOWN) {
    // guess from URI sheme
    std::string::size_type pos = url.find("://");
    if(pos != std::string::npos) {
      setTypeOfResource(url.substr(0, pos));
    }
  }
}

void MetalinkParserController::setTypeOfResource(const std::string& type)
{
  if(tResource_.isNull()) {
    return;
  }
  if(type == MetalinkResource::FTP) {
    tResource_->type = MetalinkResource::TYPE_FTP;
  } else if(type == MetalinkResource::HTTP) {
    tResource_->type = MetalinkResource::TYPE_HTTP;
  } else if(type == MetalinkResource::HTTPS) {
    tResource_->type = MetalinkResource::TYPE_HTTPS;
  } else if(type == MetalinkResource::BITTORRENT) {
    tResource_->type = MetalinkResource::TYPE_BITTORRENT;
  } else if(type == MetalinkResource::TORRENT) { // Metalink4Spec
    tResource_->type = MetalinkResource::TYPE_BITTORRENT;
  } else {
    tResource_->type = MetalinkResource::TYPE_NOT_SUPPORTED;
  }
}

void MetalinkParserController::setLocationOfResource(const std::string& location)
{
  if(tResource_.isNull()) {
    return;
  }
  tResource_->location = location;
}

void MetalinkParserController::setPriorityOfResource(int priority)
{
  if(tResource_.isNull()) {
    return;
  }
  tResource_->priority = priority;
}

void MetalinkParserController::setMaxConnectionsOfResource(int maxConnections)
{
  if(tResource_.isNull()) {
    return;
  }
  tResource_->maxConnections = maxConnections;
}

void MetalinkParserController::commitResourceTransaction()
{
  if(tResource_.isNull()) {
    return;
  }
#ifdef ENABLE_BITTORRENT
  if(tResource_->type == MetalinkResource::TYPE_BITTORRENT) {
    SharedHandle<MetalinkMetaurl> metaurl(new MetalinkMetaurl());
    metaurl->url = tResource_->url;
    metaurl->priority = tResource_->priority;
    metaurl->mediatype = MetalinkMetaurl::MEDIATYPE_TORRENT;
    tEntry_->metaurls.push_back(metaurl);
  } else {
    tEntry_->resources.push_back(tResource_);
  }
#else // !ENABLE_BITTORRENT
  tEntry_->resources.push_back(tResource_);
#endif // !ENABLE_BITTORRENT
  tResource_.reset();
}

void MetalinkParserController::cancelResourceTransaction()
{
  tResource_.reset();
}

void MetalinkParserController::newChecksumTransaction()
{
#ifdef ENABLE_MESSAGE_DIGEST
  if(tEntry_.isNull()) {
    return;
  }
  tChecksum_.reset(new Checksum());
#endif // ENABLE_MESSAGE_DIGEST
}

void MetalinkParserController::setTypeOfChecksum(const std::string& type)
{
#ifdef ENABLE_MESSAGE_DIGEST
  if(tChecksum_.isNull()) {
    return;
  }
  std::string calgo = MessageDigestContext::getCanonicalAlgo(type);
  if(MessageDigestContext::supports(calgo)) {
    tChecksum_->setAlgo(calgo);
  } else {
    cancelChecksumTransaction();
  }
#endif // ENABLE_MESSAGE_DIGEST
}

void MetalinkParserController::setHashOfChecksum(const std::string& md)
{
#ifdef ENABLE_MESSAGE_DIGEST
  if(tChecksum_.isNull()) {
    return;
  }
  if(MessageDigestContext::isValidHash(tChecksum_->getAlgo(), md)) {
    tChecksum_->setMessageDigest(md);
  } else {
    cancelChecksumTransaction();
  }
#endif // ENABLE_MESSAGE_DIGEST
}

void MetalinkParserController::commitChecksumTransaction()
{
#ifdef ENABLE_MESSAGE_DIGEST
  if(tChecksum_.isNull()) {
    return;
  }
  if(tEntry_->checksum.isNull() ||
     MessageDigestContext::isStronger(tChecksum_->getAlgo(),
                                      tEntry_->checksum->getAlgo())) {
    tEntry_->checksum = tChecksum_;
  }
  tChecksum_.reset();
#endif // ENABLE_MESSAGE_DIGEST
}

void MetalinkParserController::cancelChecksumTransaction()
{
#ifdef ENABLE_MESSAGE_DIGEST
  tChecksum_.reset();
#endif // ENABLE_MESSAGE_DIGEST
}

void MetalinkParserController::newChunkChecksumTransactionV4()
{
#ifdef ENABLE_MESSAGE_DIGEST
  if(tEntry_.isNull()) {
    return;
  }
  tChunkChecksumV4_.reset(new ChunkChecksum());
  tempChunkChecksumsV4_.clear();
#endif // ENABLE_MESSAGE_DIGEST
}

void MetalinkParserController::setTypeOfChunkChecksumV4(const std::string& type)
{
#ifdef ENABLE_MESSAGE_DIGEST
  if(tChunkChecksumV4_.isNull()) {
    return;
  }
  std::string calgo = MessageDigestContext::getCanonicalAlgo(type);
  if(MessageDigestContext::supports(calgo)) {
    tChunkChecksumV4_->setAlgo(calgo);
  } else {
    cancelChunkChecksumTransactionV4();
  }
#endif // ENABLE_MESSAGE_DIGEST
}

void MetalinkParserController::setLengthOfChunkChecksumV4(size_t length)
{
#ifdef ENABLE_MESSAGE_DIGEST
  if(tChunkChecksumV4_.isNull()) {
    return;
  }
  if(length > 0) {
    tChunkChecksumV4_->setChecksumLength(length);
  } else {
    cancelChunkChecksumTransactionV4();
  }
#endif // ENABLE_MESSAGE_DIGEST
}

void MetalinkParserController::addHashOfChunkChecksumV4(const std::string& md)
{
#ifdef ENABLE_MESSAGE_DIGEST
  if(tChunkChecksumV4_.isNull()) {
    return;
  }
  if(MessageDigestContext::isValidHash(tChunkChecksumV4_->getAlgo(), md)) {
    tempChunkChecksumsV4_.push_back(md);
  } else {
    cancelChunkChecksumTransactionV4();
  }
#endif // ENABLE_MESSAGE_DIGEST  
}

void MetalinkParserController::commitChunkChecksumTransactionV4()
{
#ifdef ENABLE_MESSAGE_DIGEST
  if(tChunkChecksumV4_.isNull()) {
    return;
  }
  if(tEntry_->chunkChecksum.isNull() ||
     MessageDigestContext::isStronger(tChunkChecksumV4_->getAlgo(),
                                      tEntry_->chunkChecksum->getAlgo())) {
    std::vector<std::string> checksums(tempChunkChecksumsV4_.begin(),
				      tempChunkChecksumsV4_.end());
    tChunkChecksumV4_->setChecksums(checksums);
    tEntry_->chunkChecksum = tChunkChecksumV4_;
  }
  tChunkChecksumV4_.reset();
#endif // ENABLE_MESSAGE_DIGEST
}

void MetalinkParserController::cancelChunkChecksumTransactionV4()
{
#ifdef ENABLE_MESSAGE_DIGEST
  tChunkChecksumV4_.reset();
#endif // ENABLE_MESSAGE_DIGEST
}

void MetalinkParserController::newChunkChecksumTransaction()
{
#ifdef ENABLE_MESSAGE_DIGEST
  if(tEntry_.isNull()) {
    return;
  }
  tChunkChecksum_.reset(new ChunkChecksum());
  tempChunkChecksums_.clear();
#endif // ENABLE_MESSAGE_DIGEST
}

void MetalinkParserController::setTypeOfChunkChecksum(const std::string& type)
{
#ifdef ENABLE_MESSAGE_DIGEST
  if(tChunkChecksum_.isNull()) {
    return;
  }
  std::string calgo = MessageDigestContext::getCanonicalAlgo(type);
  if(MessageDigestContext::supports(calgo)) {
    tChunkChecksum_->setAlgo(calgo);
  } else {
    cancelChunkChecksumTransaction();
  }
#endif // ENABLE_MESSAGE_DIGEST
}

void MetalinkParserController::setLengthOfChunkChecksum(size_t length)
{
#ifdef ENABLE_MESSAGE_DIGEST
  if(tChunkChecksum_.isNull()) {
    return;
  }
  if(length > 0) {
    tChunkChecksum_->setChecksumLength(length);
  } else {
    cancelChunkChecksumTransaction();
  }
#endif // ENABLE_MESSAGE_DIGEST
}

void MetalinkParserController::addHashOfChunkChecksum(size_t order, const std::string& md)
{
#ifdef ENABLE_MESSAGE_DIGEST
  if(tChunkChecksum_.isNull()) {
    return;
  }
  if(MessageDigestContext::isValidHash(tChunkChecksum_->getAlgo(), md)) {
    tempChunkChecksums_.push_back(std::make_pair(order, md));
  } else {
    cancelChunkChecksumTransaction();
  }
#endif // ENABLE_MESSAGE_DIGEST
}

void MetalinkParserController::createNewHashOfChunkChecksum(size_t order)
{
#ifdef ENABLE_MESSAGE_DIGEST
  if(tChunkChecksum_.isNull()) {
    return;
  }
  tempHashPair_.first = order;
#endif // ENABLE_MESSAGE_DIGEST
}

void MetalinkParserController::setMessageDigestOfChunkChecksum(const std::string& md)
{
#ifdef ENABLE_MESSAGE_DIGEST
  if(tChunkChecksum_.isNull()) {
    return;
  }
  if(MessageDigestContext::isValidHash(tChunkChecksum_->getAlgo(), md)) {
    tempHashPair_.second = md;
  } else {
    cancelChunkChecksumTransaction();
  }
#endif // ENABLE_MESSAGE_DIGEST
}

void MetalinkParserController::addHashOfChunkChecksum()
{
#ifdef ENABLE_MESSAGE_DIGEST
  if(tChunkChecksum_.isNull()) {
    return;
  }
  tempChunkChecksums_.push_back(tempHashPair_);
#endif // ENABLE_MESSAGE_DIGEST
}

void MetalinkParserController::commitChunkChecksumTransaction()
{
#ifdef ENABLE_MESSAGE_DIGEST
  if(tChunkChecksum_.isNull()) {
    return;
  }
  if(tEntry_->chunkChecksum.isNull() ||
     MessageDigestContext::isStronger(tChunkChecksum_->getAlgo(),
                                      tEntry_->chunkChecksum->getAlgo())) {
    std::sort(tempChunkChecksums_.begin(), tempChunkChecksums_.end(),
              Ascend1st<std::pair<size_t, std::string> >());
    std::vector<std::string> checksums;
    std::transform(tempChunkChecksums_.begin(), tempChunkChecksums_.end(),
                   std::back_inserter(checksums),
                   select2nd<std::pair<size_t, std::string> >());
    tChunkChecksum_->setChecksums(checksums);
    tEntry_->chunkChecksum = tChunkChecksum_;
  }
  tChunkChecksum_.reset();
#endif // ENABLE_MESSAGE_DIGEST
}

void MetalinkParserController::cancelChunkChecksumTransaction()
{
#ifdef ENABLE_MESSAGE_DIGEST
  tChunkChecksum_.reset();
#endif // ENABLE_MESSAGE_DIGEST
}

void MetalinkParserController::newSignatureTransaction()
{
  if(tEntry_.isNull()) {
    return;
  }
  tSignature_.reset(new Signature());
}

void MetalinkParserController::setTypeOfSignature(const std::string& type)
{
  if(tSignature_.isNull()) {
    return;
  }
  tSignature_->setType(type);
}

void MetalinkParserController::setFileOfSignature(const std::string& file)
{
  if(tSignature_.isNull()) {
    return;
  }
  tSignature_->setFile(file);
}

void MetalinkParserController::setBodyOfSignature(const std::string& body)
{
  if(tSignature_.isNull()) {
    return;
  }
  tSignature_->setBody(body);
}

void MetalinkParserController::commitSignatureTransaction()
{
  if(tSignature_.isNull()) {
    return;
  }
  tEntry_->setSignature(tSignature_);
  tSignature_.reset();
}

void MetalinkParserController::cancelSignatureTransaction()
{
  tSignature_.reset();
}

void MetalinkParserController::newMetaurlTransaction()
{
  if(tEntry_.isNull()) {
    return;
  }
  tMetaurl_.reset(new MetalinkMetaurl());
}

void MetalinkParserController::setURLOfMetaurl(const std::string& url)
{
  if(tMetaurl_.isNull()) {
    return;
  }
  tMetaurl_->url = url;
}

void MetalinkParserController::setMediatypeOfMetaurl
(const std::string& mediatype)
{
  if(tMetaurl_.isNull()) {
    return;
  }
  tMetaurl_->mediatype = mediatype;
}

void MetalinkParserController::setPriorityOfMetaurl(int priority)
{
  if(tMetaurl_.isNull()) {
    return;
  }
  tMetaurl_->priority = priority;
}

void MetalinkParserController::setNameOfMetaurl(const std::string& name)
{
  if(tMetaurl_.isNull()) {
    return;
  }
  tMetaurl_->name = name;
}

void MetalinkParserController::commitMetaurlTransaction()
{
  if(tMetaurl_.isNull()) {
    return;
  }
#ifdef ENABLE_BITTORRENT
  if(tMetaurl_->mediatype == MetalinkMetaurl::MEDIATYPE_TORRENT) {
    tEntry_->metaurls.push_back(tMetaurl_);
  }
#endif // ENABLE_BITTORRENT
  tMetaurl_.reset();
}

void MetalinkParserController::cancelMetaurlTransaction()
{
  tMetaurl_.reset();
}

} // namespace aria2
