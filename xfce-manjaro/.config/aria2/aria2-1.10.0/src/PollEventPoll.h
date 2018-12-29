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
#ifndef _D_POLL_EVENT_POLL_H_
#define _D_POLL_EVENT_POLL_H_

#include "EventPoll.h"

# include <poll.h>

#include <deque>

#include "Event.h"
#ifdef ENABLE_ASYNC_DNS
# include "AsyncNameResolver.h"
#endif // ENABLE_ASYNC_DNS

namespace aria2 {

class Logger;

class PollEventPoll : public EventPoll {
private:
  class KSocketEntry;

  typedef Event<KSocketEntry> KEvent;
  typedef CommandEvent<KSocketEntry, PollEventPoll> KCommandEvent;
  typedef ADNSEvent<KSocketEntry, PollEventPoll> KADNSEvent;
  typedef AsyncNameResolverEntry<PollEventPoll> KAsyncNameResolverEntry;
  friend class AsyncNameResolverEntry<PollEventPoll>;

  class KSocketEntry:
    public SocketEntry<KCommandEvent, KADNSEvent> {
  public:
    KSocketEntry(sock_t socket);

    struct pollfd getEvents();
  };

  friend int accumulateEvent(int events, const KEvent& event);

private:
  std::deque<SharedHandle<KSocketEntry> > socketEntries_;
#ifdef ENABLE_ASYNC_DNS
  std::deque<SharedHandle<KAsyncNameResolverEntry> > nameResolverEntries_;
#endif // ENABLE_ASYNC_DNS

  // Allocated the number of struct pollfd in pollfds_.
  int pollfdCapacity_;

  // The number of valid struct pollfd in pollfds_.
  int pollfdNum_;

  struct pollfd* pollfds_;

  Logger* logger_;

  bool addEvents(sock_t socket, const KEvent& event);

  bool deleteEvents(sock_t socket, const KEvent& event);

  bool addEvents(sock_t socket, Command* command, int events,
                 const SharedHandle<AsyncNameResolver>& rs);

  bool deleteEvents(sock_t socket, Command* command,
                    const SharedHandle<AsyncNameResolver>& rs);

  static int translateEvents(EventPoll::EventType events);
public:
  PollEventPoll();

  virtual ~PollEventPoll();

  virtual void poll(const struct timeval& tv);

  virtual bool addEvents(sock_t socket,
                         Command* command, EventPoll::EventType events);

  virtual bool deleteEvents(sock_t socket,
                            Command* command, EventPoll::EventType events);
#ifdef ENABLE_ASYNC_DNS

  virtual bool addNameResolver(const SharedHandle<AsyncNameResolver>& resolver,
                               Command* command);
  virtual bool deleteNameResolver
  (const SharedHandle<AsyncNameResolver>& resolver, Command* command);
#endif // ENABLE_ASYNC_DNS

  static const int IEV_READ = POLLIN;
  static const int IEV_WRITE = POLLOUT;
  static const int IEV_ERROR = POLLERR;
  static const int IEV_HUP = POLLHUP;
};

} // namespace aria2

#endif // _D_POLL_EVENT_POLL_H_
