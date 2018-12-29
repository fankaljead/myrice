/* <!-- copyright */
/*
 * aria2 - The high speed download utility
 *
 * Copyright (C) 2010 Tatsuhiro Tsujikawa
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
#include "KqueueEventPoll.h"

#include <cstring>
#include <algorithm>
#include <numeric>

#include "Command.h"
#include "LogFactory.h"
#include "Logger.h"

#ifdef KEVENT_UDATA_INTPTR_T
# define PTR_TO_UDATA(X) (reinterpret_cast<intptr_t>(X))
#else // !KEVENT_UDATA_INTPTR_T
# define PTR_TO_UDATA(X) (X)
#endif // !KEVENT_UDATA_INTPTR_T

namespace aria2 {

KqueueEventPoll::KSocketEntry::KSocketEntry(sock_t s):
  SocketEntry<KCommandEvent, KADNSEvent>(s) {}

int accumulateEvent(int events, const KqueueEventPoll::KEvent& event)
{
  return events|event.getEvents();
}

size_t KqueueEventPoll::KSocketEntry::getEvents
(struct kevent* eventlist)
{
  int events;
#ifdef ENABLE_ASYNC_DNS
  events =
    std::accumulate(adnsEvents_.begin(),
                    adnsEvents_.end(),
                    std::accumulate(commandEvents_.begin(),
                                    commandEvents_.end(), 0, accumulateEvent),
                    accumulateEvent);
#else // !ENABLE_ASYNC_DNS
  events =
    std::accumulate(commandEvents_.begin(), commandEvents_.end(), 0,
                    accumulateEvent);
#endif // !ENABLE_ASYNC_DNS
  EV_SET(&eventlist[0], socket_, EVFILT_READ,
         EV_ADD|((events&KqueueEventPoll::IEV_READ)?EV_ENABLE:EV_DISABLE),
         0, 0, PTR_TO_UDATA(this));
  EV_SET(&eventlist[1], socket_, EVFILT_WRITE,
         EV_ADD|((events&KqueueEventPoll::IEV_WRITE)?EV_ENABLE:EV_DISABLE),
         0, 0, PTR_TO_UDATA(this));
  return 2;
}

KqueueEventPoll::KqueueEventPoll():
  kqEventsSize_(KQUEUE_EVENTS_MAX),
  kqEvents_(new struct kevent[kqEventsSize_]),
  logger_(LogFactory::getInstance())
{
  kqfd_ = kqueue();
}

KqueueEventPoll::~KqueueEventPoll()
{
  if(kqfd_ != -1) {
    int r;
    while((r = close(kqfd_)) == -1 && errno == EINTR);
    if(r == -1) {
      logger_->error("Error occurred while closing kqueue file descriptor"
                     " %d: %s",
                     kqfd_, strerror(errno));
    }
  }
  delete [] kqEvents_;
}

bool KqueueEventPoll::good() const
{
  return kqfd_ != -1;
}

void KqueueEventPoll::poll(const struct timeval& tv)
{
  struct timespec timeout = { tv.tv_sec, tv.tv_usec*1000 };
  int res;
  while((res = kevent(kqfd_, kqEvents_, 0, kqEvents_, kqEventsSize_, &timeout))
        == -1 && errno == EINTR);
  if(res > 0) {
    for(int i = 0; i < res; ++i) {
      KSocketEntry* p = reinterpret_cast<KSocketEntry*>(kqEvents_[i].udata);
      int events = 0;
      int filter = kqEvents_[i].filter;
      if(filter == EVFILT_READ) {
        events = KqueueEventPoll::IEV_READ;
      } else if(filter == EVFILT_WRITE) {
        events = KqueueEventPoll::IEV_WRITE;
      }
      p->processEvents(events);
    }
  }

#ifdef ENABLE_ASYNC_DNS
  // It turns out that we have to call ares_process_fd before ares's
  // own timeout and ares may create new sockets or closes socket in
  // their API. So we call ares_process_fd for all ares_channel and
  // re-register their sockets.
  for(std::deque<SharedHandle<KAsyncNameResolverEntry> >::iterator i =
        nameResolverEntries_.begin(), eoi = nameResolverEntries_.end();
      i != eoi; ++i) {
    (*i)->processTimeout();
    (*i)->removeSocketEvents(this);
    (*i)->addSocketEvents(this);
  }
#endif // ENABLE_ASYNC_DNS

  // TODO timeout of name resolver is determined in Command(AbstractCommand,
  // DHTEntryPoint...Command)
}

static int translateEvents(EventPoll::EventType events)
{
  int newEvents = 0;
  if(EventPoll::EVENT_READ&events) {
    newEvents |= KqueueEventPoll::IEV_READ;
  }
  if(EventPoll::EVENT_WRITE&events) {
    newEvents |= KqueueEventPoll::IEV_WRITE;
  }
  return newEvents;
}

bool KqueueEventPoll::addEvents
(sock_t socket, const KqueueEventPoll::KEvent& event)
{
  SharedHandle<KSocketEntry> socketEntry(new KSocketEntry(socket));
  std::deque<SharedHandle<KSocketEntry> >::iterator i =
    std::lower_bound(socketEntries_.begin(), socketEntries_.end(), socketEntry);
  int r = 0;
  struct timespec zeroTimeout = { 0, 0 };
  struct kevent changelist[2];
  size_t n;
  if(i != socketEntries_.end() && (*i) == socketEntry) {
    event.addSelf(*i);
    n = (*i)->getEvents(changelist);
  } else {
    socketEntries_.insert(i, socketEntry);
    if(socketEntries_.size() > kqEventsSize_) {
      kqEventsSize_ *= 2;
      delete [] kqEvents_;
      kqEvents_ = new struct kevent[kqEventsSize_];
    }
    event.addSelf(socketEntry);
    n = socketEntry->getEvents(changelist);
  }
  r = kevent(kqfd_, changelist, n, changelist, 0, &zeroTimeout);
  if(r == -1) {
    if(logger_->debug()) {
      logger_->debug("Failed to add socket event %d:%s",
                     socket, strerror(errno));
    }
    return false;
  } else {
    return true;
  }
}

bool KqueueEventPoll::addEvents(sock_t socket, Command* command,
                               EventPoll::EventType events)
{
  int kqEvents = translateEvents(events);
  return addEvents(socket, KCommandEvent(command, kqEvents));
}

#ifdef ENABLE_ASYNC_DNS
bool KqueueEventPoll::addEvents(sock_t socket, Command* command, int events,
                               const SharedHandle<AsyncNameResolver>& rs)
{
  return addEvents(socket, KADNSEvent(rs, command, socket, events));
}
#endif // ENABLE_ASYNC_DNS

bool KqueueEventPoll::deleteEvents(sock_t socket,
                                  const KqueueEventPoll::KEvent& event)
{
  SharedHandle<KSocketEntry> socketEntry(new KSocketEntry(socket));
  std::deque<SharedHandle<KSocketEntry> >::iterator i =
    std::lower_bound(socketEntries_.begin(), socketEntries_.end(), socketEntry);
  if(i != socketEntries_.end() && (*i) == socketEntry) {
    event.removeSelf(*i);
    int r = 0;
    struct timespec zeroTimeout = { 0, 0 };
    struct kevent changelist[2];
    size_t n = (*i)->getEvents(changelist);
    r = kevent(kqfd_, changelist, n, changelist, 0, &zeroTimeout);
    if((*i)->eventEmpty()) {
      socketEntries_.erase(i);
    }
    if(r == -1) {
      if(logger_->debug()) {
        logger_->debug("Failed to delete socket event:%s", strerror(errno));
      }
      return false;
    } else {
      return true;
    }
  } else {
    if(logger_->debug()) {
      logger_->debug("Socket %d is not found in SocketEntries.", socket);
    }
    return false;
  }
}

#ifdef ENABLE_ASYNC_DNS
bool KqueueEventPoll::deleteEvents(sock_t socket, Command* command,
                                  const SharedHandle<AsyncNameResolver>& rs)
{
  return deleteEvents(socket, KADNSEvent(rs, command, socket, 0));
}
#endif // ENABLE_ASYNC_DNS

bool KqueueEventPoll::deleteEvents(sock_t socket, Command* command,
                                  EventPoll::EventType events)
{
  int kqEvents = translateEvents(events);
  return deleteEvents(socket, KCommandEvent(command, kqEvents));
}

#ifdef ENABLE_ASYNC_DNS
bool KqueueEventPoll::addNameResolver
(const SharedHandle<AsyncNameResolver>& resolver, Command* command)
{
  SharedHandle<KAsyncNameResolverEntry> entry
    (new KAsyncNameResolverEntry(resolver, command));
  std::deque<SharedHandle<KAsyncNameResolverEntry> >::iterator itr =
    std::find(nameResolverEntries_.begin(), nameResolverEntries_.end(), entry);
  if(itr == nameResolverEntries_.end()) {
    nameResolverEntries_.push_back(entry);
    entry->addSocketEvents(this);
    return true;
  } else {
    return false;
  }
}

bool KqueueEventPoll::deleteNameResolver
(const SharedHandle<AsyncNameResolver>& resolver, Command* command)
{
  SharedHandle<KAsyncNameResolverEntry> entry
    (new KAsyncNameResolverEntry(resolver, command));
  std::deque<SharedHandle<KAsyncNameResolverEntry> >::iterator itr =
    std::find(nameResolverEntries_.begin(), nameResolverEntries_.end(), entry);
  if(itr == nameResolverEntries_.end()) {
    return false;
  } else {
    (*itr)->removeSocketEvents(this);
    nameResolverEntries_.erase(itr);
    return true;
  }
}
#endif // ENABLE_ASYNC_DNS

} // namespace aria2