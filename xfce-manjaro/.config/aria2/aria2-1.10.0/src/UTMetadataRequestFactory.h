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
#ifndef _UT_METADATA_REQUEST_FACTORY_H_
#define _UT_METADATA_REQUEST_FACTORY_H_

#include "common.h"

#include <vector>

#include "SharedHandle.h"

namespace aria2 {

class PieceStorage;
class DownloadContext;
class Peer;
class BtMessageDispatcher;
class BtMessageFactory;
class UTMetadataRequestTracker;
class BtMessage;
class Logger;

class UTMetadataRequestFactory {
private:
  SharedHandle<DownloadContext> dctx_;

  SharedHandle<Peer> peer_;

  WeakHandle<BtMessageDispatcher> dispatcher_;

  WeakHandle<BtMessageFactory> messageFactory_;

  WeakHandle<UTMetadataRequestTracker> tracker_;

  Logger* logger_;
public:
  UTMetadataRequestFactory();

  // Creates at most num of ut_metadata request message and appends
  // them to msgs. pieceStorage is used to identify missing piece.
  void create(std::vector<SharedHandle<BtMessage> >& msgs, size_t num,
              const SharedHandle<PieceStorage>& pieceStorage);

  void setDownloadContext(const SharedHandle<DownloadContext>& dctx)
  {
    dctx_ = dctx;
  }

  void setBtMessageDispatcher(const WeakHandle<BtMessageDispatcher>& disp)
  {
    dispatcher_ = disp;
  }

  void setBtMessageFactory(const WeakHandle<BtMessageFactory>& factory)
  {
    messageFactory_ = factory;
  }

  void setPeer(const SharedHandle<Peer>& peer)
  {
    peer_ = peer;
  }

  void setUTMetadataRequestTracker
  (const WeakHandle<UTMetadataRequestTracker>& tracker)
  {
    tracker_ = tracker;
  }
};

} // namespace aria2

#endif // _UT_METADATA_REQUEST_FACTORY_H_
