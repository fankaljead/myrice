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
#include "DHTMessageFactoryImpl.h"

#include <cstring>
#include <utility>

#include "LogFactory.h"
#include "DlAbortEx.h"
#include "DHTNode.h"
#include "DHTRoutingTable.h"
#include "DHTPingMessage.h"
#include "DHTPingReplyMessage.h"
#include "DHTFindNodeMessage.h"
#include "DHTFindNodeReplyMessage.h"
#include "DHTGetPeersMessage.h"
#include "DHTGetPeersReplyMessage.h"
#include "DHTAnnouncePeerMessage.h"
#include "DHTAnnouncePeerReplyMessage.h"
#include "DHTUnknownMessage.h"
#include "DHTConnection.h"
#include "DHTMessageDispatcher.h"
#include "DHTPeerAnnounceStorage.h"
#include "DHTTokenTracker.h"
#include "DHTMessageCallback.h"
#include "bittorrent_helper.h"
#include "BtRuntime.h"
#include "util.h"
#include "Peer.h"
#include "Logger.h"
#include "StringFormat.h"

namespace aria2 {

DHTMessageFactoryImpl::DHTMessageFactoryImpl():
  logger_(LogFactory::getInstance()) {}

DHTMessageFactoryImpl::~DHTMessageFactoryImpl() {}

SharedHandle<DHTNode>
DHTMessageFactoryImpl::getRemoteNode
(const unsigned char* id, const std::string& ipaddr, uint16_t port) const
{
  SharedHandle<DHTNode> node = routingTable_->getNode(id, ipaddr, port);
  if(node.isNull()) {
    node.reset(new DHTNode(id));
    node->setIPAddress(ipaddr);
    node->setPort(port);
  }
  return node;
}

static const Dict* getDictionary(const Dict* dict, const std::string& key)
{
  const Dict* d = asDict(dict->get(key));
  if(d) {
    return d;
  } else {
    throw DL_ABORT_EX
      (StringFormat("Malformed DHT message. Missing %s", key.c_str()).str());
  }
}

static const String* getString(const Dict* dict, const std::string& key)
{
  const String* c = asString(dict->get(key));
  if(c) {
    return c;
  } else {
    throw DL_ABORT_EX
      (StringFormat("Malformed DHT message. Missing %s", key.c_str()).str());
  }
}

static const Integer* getInteger(const Dict* dict, const std::string& key)
{
  const Integer* c = asInteger(dict->get(key));
  if(c) {
    return c;
  } else {
    throw DL_ABORT_EX
      (StringFormat("Malformed DHT message. Missing %s", key.c_str()).str());
  }
}

static const String* getString(const List* list, size_t index)
{
  const String* c = asString(list->get(index));
  if(c) {
    return c;
  } else {
    throw DL_ABORT_EX
      (StringFormat("Malformed DHT message. element[%u] is not String.",
                    index).str());
  }
}

static const Integer* getInteger(const List* list, size_t index)
{
  const Integer* c = asInteger(list->get(index));
  if(c) {
    return c;
  } else {
    throw DL_ABORT_EX
      (StringFormat("Malformed DHT message. element[%u] is not Integer.",
                    index).str());
  }
}

static const List* getList(const Dict* dict, const std::string& key)
{
  const List* l = asList(dict->get(key));
  if(l) {
    return l;
  } else {
    throw DL_ABORT_EX
      (StringFormat("Malformed DHT message. Missing %s", key.c_str()).str());
  }
}

void DHTMessageFactoryImpl::validateID(const String* id) const
{
  if(id->s().size() != DHT_ID_LENGTH) {
    throw DL_ABORT_EX
      (StringFormat("Malformed DHT message. Invalid ID length."
                    " Expected:%d, Actual:%d",
                    DHT_ID_LENGTH, id->s().size()).str());
  }
}

void DHTMessageFactoryImpl::validatePort(const Integer* port) const
{
  if(!(0 < port->i() && port->i() < UINT16_MAX)) {
    throw DL_ABORT_EX
      (StringFormat("Malformed DHT message. Invalid port=%s",
                    util::itos(port->i()).c_str()).str());
  }
}

static void setVersion(const SharedHandle<DHTMessage>& msg, const Dict* dict)
{
  const String* v = asString(dict->get(DHTMessage::V));
  if(v) {
    msg->setVersion(v->s());
  } else {
    msg->setVersion(A2STR::NIL);
  }
}

SharedHandle<DHTQueryMessage> DHTMessageFactoryImpl::createQueryMessage
(const Dict* dict, const std::string& ipaddr, uint16_t port)
{
  const String* messageType = getString(dict, DHTQueryMessage::Q);
  const String* transactionID = getString(dict, DHTMessage::T);
  const String* y = getString(dict, DHTMessage::Y);
  const Dict* aDict = getDictionary(dict, DHTQueryMessage::A);
  if(y->s() != DHTQueryMessage::Q) {
    throw DL_ABORT_EX("Malformed DHT message. y != q");
  }
  const String* id = getString(aDict, DHTMessage::ID);
  validateID(id);
  SharedHandle<DHTNode> remoteNode = getRemoteNode(id->uc(), ipaddr, port);
  SharedHandle<DHTQueryMessage> msg;
  if(messageType->s() == DHTPingMessage::PING) {
    msg = createPingMessage(remoteNode, transactionID->s());
  } else if(messageType->s() == DHTFindNodeMessage::FIND_NODE) {
    const String* targetNodeID =
      getString(aDict, DHTFindNodeMessage::TARGET_NODE);
    validateID(targetNodeID);
    msg = createFindNodeMessage(remoteNode, targetNodeID->uc(),
                                transactionID->s());
  } else if(messageType->s() == DHTGetPeersMessage::GET_PEERS) {
    const String* infoHash =  getString(aDict, DHTGetPeersMessage::INFO_HASH);
    validateID(infoHash);
    msg = createGetPeersMessage(remoteNode, infoHash->uc(), transactionID->s());
  } else if(messageType->s() == DHTAnnouncePeerMessage::ANNOUNCE_PEER) {
    const String* infoHash = getString(aDict,DHTAnnouncePeerMessage::INFO_HASH);
    validateID(infoHash);
    const Integer* port = getInteger(aDict, DHTAnnouncePeerMessage::PORT);
    validatePort(port);
    const String* token = getString(aDict, DHTAnnouncePeerMessage::TOKEN);
    msg = createAnnouncePeerMessage(remoteNode, infoHash->uc(),
                                    static_cast<uint16_t>(port->i()),
                                    token->s(), transactionID->s());
  } else {
    throw DL_ABORT_EX(StringFormat("Unsupported message type: %s",
                                   messageType->s().c_str()).str());
  }
  setVersion(msg, dict);
  return msg;
}

SharedHandle<DHTResponseMessage>
DHTMessageFactoryImpl::createResponseMessage
(const std::string& messageType,
 const Dict* dict,
 const std::string& ipaddr,
 uint16_t port)
{
  const String* transactionID = getString(dict, DHTMessage::T);
  const String* y = getString(dict, DHTMessage::Y);
  if(y->s() == DHTUnknownMessage::E) {
    // for now, just report error message arrived and throw exception.
    const List* e = getList(dict, DHTUnknownMessage::E);
    if(e->size() == 2) {
      if(logger_->info()) {
        logger_->info("Received Error DHT message. code=%s, msg=%s",
                      util::itos(getInteger(e, 0)->i()).c_str(),
                      util::percentEncode(getString(e, 1)->s()).c_str());
      }
    } else {
      if(logger_->debug()) {
        logger_->debug("e doesn't have 2 elements.");
      }
    }
    throw DL_ABORT_EX("Received Error DHT message.");
  } else if(y->s() != DHTResponseMessage::R) {
    throw DL_ABORT_EX
      (StringFormat("Malformed DHT message. y != r: y=%s",
                    util::percentEncode(y->s()).c_str()).str());
  }
  const Dict* rDict = getDictionary(dict, DHTResponseMessage::R);
  const String* id = getString(rDict, DHTMessage::ID);
  validateID(id);
  SharedHandle<DHTNode> remoteNode = getRemoteNode(id->uc(), ipaddr, port);
  SharedHandle<DHTResponseMessage> msg;
  if(messageType == DHTPingReplyMessage::PING) {
    msg = createPingReplyMessage(remoteNode, id->uc(), transactionID->s());
  } else if(messageType == DHTFindNodeReplyMessage::FIND_NODE) {
    msg = createFindNodeReplyMessage(remoteNode, dict, transactionID->s());
  } else if(messageType == DHTGetPeersReplyMessage::GET_PEERS) {
    const List* valuesList =
      asList(rDict->get(DHTGetPeersReplyMessage::VALUES));
    if(valuesList) {
      msg = createGetPeersReplyMessageWithValues
        (remoteNode, dict, transactionID->s());
    } else {
      const String* nodes = asString
        (rDict->get(DHTGetPeersReplyMessage::NODES));
      if(nodes) {
        msg = createGetPeersReplyMessageWithNodes
          (remoteNode, dict, transactionID->s());
      } else {
        throw DL_ABORT_EX("Malformed DHT message: missing nodes/values");
      }
    }
  } else if(messageType == DHTAnnouncePeerReplyMessage::ANNOUNCE_PEER) {
    msg = createAnnouncePeerReplyMessage(remoteNode, transactionID->s());
  } else {
    throw DL_ABORT_EX
      (StringFormat("Unsupported message type: %s", messageType.c_str()).str());
  }
  setVersion(msg, dict);
  return msg;
}

static const std::string& getDefaultVersion()
{
  static std::string version;
  if(version.empty()) {
    uint16_t vnum16 = htons(DHT_VERSION);
    unsigned char buf[] = { 'A' , '2', 0, 0 };
    char* vnump = reinterpret_cast<char*>(&vnum16);
    memcpy(buf+2, vnump, 2);
    version.assign(&buf[0], &buf[4]);
  }
  return version;
}

void DHTMessageFactoryImpl::setCommonProperty
(const SharedHandle<DHTAbstractMessage>& m)
{
  m->setConnection(connection_);
  m->setMessageDispatcher(dispatcher_);
  m->setRoutingTable(routingTable_);
  WeakHandle<DHTMessageFactory> factory(this);
  m->setMessageFactory(factory);
  m->setVersion(getDefaultVersion());
}

SharedHandle<DHTQueryMessage> DHTMessageFactoryImpl::createPingMessage
(const SharedHandle<DHTNode>& remoteNode, const std::string& transactionID)
{
  SharedHandle<DHTPingMessage> m
    (new DHTPingMessage(localNode_, remoteNode, transactionID));
  setCommonProperty(m);
  return m;
}

SharedHandle<DHTResponseMessage> DHTMessageFactoryImpl::createPingReplyMessage
(const SharedHandle<DHTNode>& remoteNode,
 const unsigned char* id,
 const std::string& transactionID)
{
  SharedHandle<DHTPingReplyMessage> m
    (new DHTPingReplyMessage(localNode_, remoteNode, id, transactionID));
  setCommonProperty(m);
  return m;
}

SharedHandle<DHTQueryMessage> DHTMessageFactoryImpl::createFindNodeMessage
(const SharedHandle<DHTNode>& remoteNode,
 const unsigned char* targetNodeID,
 const std::string& transactionID)
{
  SharedHandle<DHTFindNodeMessage> m
    (new DHTFindNodeMessage
     (localNode_, remoteNode, targetNodeID, transactionID));
  setCommonProperty(m);
  return m;
}

SharedHandle<DHTResponseMessage>
DHTMessageFactoryImpl::createFindNodeReplyMessage
(const SharedHandle<DHTNode>& remoteNode,
 const std::vector<SharedHandle<DHTNode> >& closestKNodes,
 const std::string& transactionID)
{
  SharedHandle<DHTFindNodeReplyMessage> m
    (new DHTFindNodeReplyMessage(localNode_, remoteNode, transactionID));
  m->setClosestKNodes(closestKNodes);
  setCommonProperty(m);
  return m;
}

std::vector<SharedHandle<DHTNode> >
DHTMessageFactoryImpl::extractNodes(const unsigned char* src, size_t length)
{
  if(length%26 != 0) {
    throw DL_ABORT_EX("Nodes length is not multiple of 26");
  }
  std::vector<SharedHandle<DHTNode> > nodes;
  for(size_t offset = 0; offset < length; offset += 26) {
    SharedHandle<DHTNode> node(new DHTNode(src+offset));
    std::pair<std::string, uint16_t> addr =
      bittorrent::unpackcompact(src+offset+DHT_ID_LENGTH);
    if(addr.first.empty()) {
      continue;
    }
    node->setIPAddress(addr.first);
    node->setPort(addr.second);
    nodes.push_back(node);
  }
  return nodes;
}

SharedHandle<DHTResponseMessage>
DHTMessageFactoryImpl::createFindNodeReplyMessage
(const SharedHandle<DHTNode>& remoteNode,
 const Dict* dict,
 const std::string& transactionID)
{
  const String* nodesData =
    getString(getDictionary(dict, DHTResponseMessage::R),
              DHTFindNodeReplyMessage::NODES);
  std::vector<SharedHandle<DHTNode> > nodes =
    extractNodes(nodesData->uc(), nodesData->s().size());
  return createFindNodeReplyMessage(remoteNode, nodes, transactionID);
}

SharedHandle<DHTQueryMessage>
DHTMessageFactoryImpl::createGetPeersMessage
(const SharedHandle<DHTNode>& remoteNode,
 const unsigned char* infoHash,
 const std::string& transactionID)
{
  SharedHandle<DHTGetPeersMessage> m
    (new DHTGetPeersMessage(localNode_, remoteNode, infoHash, transactionID));
  m->setPeerAnnounceStorage(peerAnnounceStorage_);
  m->setTokenTracker(tokenTracker_);
  setCommonProperty(m);
  return m;
}

SharedHandle<DHTResponseMessage>
DHTMessageFactoryImpl::createGetPeersReplyMessageWithNodes
(const SharedHandle<DHTNode>& remoteNode,
 const Dict* dict,
 const std::string& transactionID)
{
  const Dict* rDict = getDictionary(dict, DHTResponseMessage::R);
  const String* nodesData = getString(rDict, DHTGetPeersReplyMessage::NODES);
  std::vector<SharedHandle<DHTNode> > nodes = extractNodes
    (nodesData->uc(), nodesData->s().size());
  const String* token = getString(rDict, DHTGetPeersReplyMessage::TOKEN);
  return createGetPeersReplyMessage
    (remoteNode, nodes, token->s(), transactionID);
}

SharedHandle<DHTResponseMessage>
DHTMessageFactoryImpl::createGetPeersReplyMessage
(const SharedHandle<DHTNode>& remoteNode,
 const std::vector<SharedHandle<DHTNode> >& closestKNodes,
 const std::string& token,
 const std::string& transactionID)
{
  SharedHandle<DHTGetPeersReplyMessage> m
    (new DHTGetPeersReplyMessage(localNode_, remoteNode, token, transactionID));
  m->setClosestKNodes(closestKNodes);
  setCommonProperty(m);
  return m;
}

SharedHandle<DHTResponseMessage>
DHTMessageFactoryImpl::createGetPeersReplyMessageWithValues
(const SharedHandle<DHTNode>& remoteNode,
 const Dict* dict,
 const std::string& transactionID)
{
  const Dict* rDict = getDictionary(dict, DHTResponseMessage::R);
  const List* valuesList = getList(rDict,
                                  DHTGetPeersReplyMessage::VALUES);
  std::vector<SharedHandle<Peer> > peers;
  for(List::ValueType::const_iterator i = valuesList->begin(),
        eoi = valuesList->end(); i != eoi; ++i) {
    const String* data = asString(*i);
    if(data && data->s().size() == 6) {
      std::pair<std::string, uint16_t> addr =
        bittorrent::unpackcompact(data->uc());
      SharedHandle<Peer> peer(new Peer(addr.first, addr.second));
      peers.push_back(peer);
    }
  }
  const String* token = getString(rDict, DHTGetPeersReplyMessage::TOKEN);
  return createGetPeersReplyMessage
    (remoteNode, peers, token->s(), transactionID);
}

SharedHandle<DHTResponseMessage>
DHTMessageFactoryImpl::createGetPeersReplyMessage
(const SharedHandle<DHTNode>& remoteNode,
 const std::vector<SharedHandle<Peer> >& values,
 const std::string& token,
 const std::string& transactionID)
{
  SharedHandle<DHTGetPeersReplyMessage> m
    (new DHTGetPeersReplyMessage(localNode_, remoteNode, token, transactionID));
  m->setValues(values);
  setCommonProperty(m);
  return m;
}

SharedHandle<DHTQueryMessage>
DHTMessageFactoryImpl::createAnnouncePeerMessage
(const SharedHandle<DHTNode>& remoteNode,
 const unsigned char* infoHash,
 uint16_t tcpPort,
 const std::string& token,
 const std::string& transactionID)
{
  SharedHandle<DHTAnnouncePeerMessage> m
    (new DHTAnnouncePeerMessage
     (localNode_, remoteNode, infoHash, tcpPort, token, transactionID));
  m->setPeerAnnounceStorage(peerAnnounceStorage_);
  m->setTokenTracker(tokenTracker_);
  setCommonProperty(m);
  return m;
}

SharedHandle<DHTResponseMessage>
DHTMessageFactoryImpl::createAnnouncePeerReplyMessage
(const SharedHandle<DHTNode>& remoteNode, const std::string& transactionID)
{
  SharedHandle<DHTAnnouncePeerReplyMessage> m
    (new DHTAnnouncePeerReplyMessage(localNode_, remoteNode, transactionID));
  setCommonProperty(m);
  return m;
}

SharedHandle<DHTMessage>
DHTMessageFactoryImpl::createUnknownMessage
(const unsigned char* data, size_t length,
 const std::string& ipaddr, uint16_t port)

{
  SharedHandle<DHTUnknownMessage> m
    (new DHTUnknownMessage(localNode_, data, length, ipaddr, port));
  return m;
}

void DHTMessageFactoryImpl::setRoutingTable
(const WeakHandle<DHTRoutingTable>& routingTable)
{
  routingTable_ = routingTable;
}

void DHTMessageFactoryImpl::setConnection
(const WeakHandle<DHTConnection>& connection)
{
  connection_ = connection;
}

void DHTMessageFactoryImpl::setMessageDispatcher
(const WeakHandle<DHTMessageDispatcher>& dispatcher)
{
  dispatcher_ = dispatcher;
}
  
void DHTMessageFactoryImpl::setPeerAnnounceStorage
(const WeakHandle<DHTPeerAnnounceStorage>& storage)
{
  peerAnnounceStorage_ = storage;
}

void DHTMessageFactoryImpl::setTokenTracker
(const WeakHandle<DHTTokenTracker>& tokenTracker)
{
  tokenTracker_ = tokenTracker;
}

void DHTMessageFactoryImpl::setLocalNode
(const SharedHandle<DHTNode>& localNode)
{
  localNode_ = localNode;
}

} // namespace aria2
