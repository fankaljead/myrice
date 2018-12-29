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
#include "DHTPeerLookupTask.h"
#include "Peer.h"
#include "DHTGetPeersReplyMessage.h"
#include "Logger.h"
#include "DHTMessageFactory.h"
#include "DHTNode.h"
#include "DHTNodeLookupEntry.h"
#include "DHTMessageDispatcher.h"
#include "DHTMessageCallback.h"
#include "PeerStorage.h"
#include "BtRuntime.h"
#include "util.h"
#include "DHTBucket.h"
#include "bittorrent_helper.h"
#include "DHTPeerLookupTaskCallback.h"
#include "DHTQueryMessage.h"

namespace aria2 {

DHTPeerLookupTask::DHTPeerLookupTask
(const SharedHandle<DownloadContext>& downloadContext):
  DHTAbstractNodeLookupTask<DHTGetPeersReplyMessage>
  (bittorrent::getInfoHash(downloadContext)) {}

void
DHTPeerLookupTask::getNodesFromMessage
(std::vector<SharedHandle<DHTNode> >& nodes,
 const DHTGetPeersReplyMessage* message)
{
  const std::vector<SharedHandle<DHTNode> >& knodes =
    message->getClosestKNodes();
  nodes.insert(nodes.end(), knodes.begin(), knodes.end());
}
  
void DHTPeerLookupTask::onReceivedInternal
(const DHTGetPeersReplyMessage* message)
{
  SharedHandle<DHTNode> remoteNode = message->getRemoteNode();
  tokenStorage_[util::toHex(remoteNode->getID(), DHT_ID_LENGTH)] =
    message->getToken();
  peerStorage_->addPeer(message->getValues());
  peers_.insert(peers_.end(),
                message->getValues().begin(), message->getValues().end());
  getLogger()->info("Received %u peers.", message->getValues().size());
}
  
SharedHandle<DHTMessage> DHTPeerLookupTask::createMessage
(const SharedHandle<DHTNode>& remoteNode)
{
  return getMessageFactory()->createGetPeersMessage(remoteNode, getTargetID());
}

SharedHandle<DHTMessageCallback> DHTPeerLookupTask::createCallback()
{
  return SharedHandle<DHTPeerLookupTaskCallback>
    (new DHTPeerLookupTaskCallback(this));
}

void DHTPeerLookupTask::onFinish()
{
  // send announce_peer message to K closest nodes
  size_t num = DHTBucket::K;
  for(std::deque<SharedHandle<DHTNodeLookupEntry> >::const_iterator i =
        getEntries().begin(), eoi = getEntries().end();
      i != eoi && num > 0; ++i, --num) {
    if((*i)->used) {
      const SharedHandle<DHTNode>& node = (*i)->node;
      SharedHandle<DHTMessage> m = 
        getMessageFactory()->createAnnouncePeerMessage
        (node,
         getTargetID(), // this is infoHash
         btRuntime_->getListenPort(),
         tokenStorage_[util::toHex(node->getID(), DHT_ID_LENGTH)]);
      getMessageDispatcher()->addMessageToQueue(m);
    }
  }
}

void DHTPeerLookupTask::setBtRuntime(const SharedHandle<BtRuntime>& btRuntime)
{
  btRuntime_ = btRuntime;
}

void DHTPeerLookupTask::setPeerStorage(const SharedHandle<PeerStorage>& ps)
{
  peerStorage_ = ps;
}

} // namespace aria2
