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
#ifndef _D_REQUEST_GROUP_MAN_H_
#define _D_REQUEST_GROUP_MAN_H_

#include "common.h"

#include <string>
#include <deque>
#include <iosfwd>
#include <vector>

#include "SharedHandle.h"
#include "DownloadResult.h"
#include "TransferStat.h"
#include "RequestGroup.h"

namespace aria2 {

class DownloadEngine;
class Command;
class Logger;
struct DownloadResult;
class ServerStatMan;
class ServerStat;
class Option;

class RequestGroupMan {
private:
  std::deque<SharedHandle<RequestGroup> > requestGroups_;
  std::deque<SharedHandle<RequestGroup> > reservedGroups_;
  std::deque<SharedHandle<DownloadResult> > downloadResults_;
  Logger* logger_;
  unsigned int maxSimultaneousDownloads_;

  const Option* option_;

  SharedHandle<ServerStatMan> serverStatMan_;

  unsigned int maxOverallDownloadSpeedLimit_;

  unsigned int maxOverallUploadSpeedLimit_;

  // truf if XML-RPC is enabled.
  bool xmlRpc_;

  bool queueCheck_;

  std::string
  formatDownloadResult(const std::string& status,
                       const SharedHandle<DownloadResult>& downloadResult) const;

  void configureRequestGroup
  (const SharedHandle<RequestGroup>& requestGroup) const;
public:
  RequestGroupMan(const std::vector<SharedHandle<RequestGroup> >& requestGroups,
                  unsigned int maxSimultaneousDownloads,
                  const Option* option);

  bool downloadFinished();

  void save();

  void closeFile();
  
  void halt();

  void forceHalt();

  void removeStoppedGroup(DownloadEngine* e);

  void fillRequestGroupFromReserver(DownloadEngine* e);

  void addRequestGroup(const SharedHandle<RequestGroup>& group);

  void addReservedGroup(const std::vector<SharedHandle<RequestGroup> >& groups);

  void addReservedGroup(const SharedHandle<RequestGroup>& group);

  void insertReservedGroup
  (size_t pos, const std::vector<SharedHandle<RequestGroup> >& groups);

  void insertReservedGroup(size_t pos, const SharedHandle<RequestGroup>& group);

  size_t countRequestGroup() const;
                  
  SharedHandle<RequestGroup> getRequestGroup(size_t index) const;
  
  const std::deque<SharedHandle<RequestGroup> >& getRequestGroups() const
  {
    return requestGroups_;
  }

  SharedHandle<RequestGroup> findRequestGroup(gid_t gid) const;

  const std::deque<SharedHandle<RequestGroup> >& getReservedGroups() const
  {
    return reservedGroups_;
  }

  SharedHandle<RequestGroup> findReservedGroup(gid_t gid) const;

  enum HOW {
    POS_SET,
    POS_CUR,
    POS_END
  };

  // Changes the position of download denoted by gid.  If how is
  // POS_SET, it moves the download to a position relative to the
  // beginning of the queue.  If how is POS_CUR, it moves the download
  // to a position relative to the current position. If how is
  // POS_END, it moves the download to a position relative to the end
  // of the queue. If the destination position is less than 0 or
  // beyond the end of the queue, it moves the download to the
  // beginning or the end of the queue respectively.  Returns the
  // destination position.
  size_t changeReservedGroupPosition(gid_t gid, int pos, HOW how);

  bool removeReservedGroup(gid_t gid);

  void showDownloadResults(std::ostream& o) const;

  bool isSameFileBeingDownloaded(RequestGroup* requestGroup) const;

  TransferStat calculateStat();

  class DownloadStat {
  private:
    size_t completed_;
    size_t error_;
    size_t inProgress_;
    size_t waiting_;
    downloadresultcode::RESULT lastErrorResult_;
  public:
    DownloadStat(size_t completed,
                 size_t error,
                 size_t inProgress,
                 size_t waiting,
                 downloadresultcode::RESULT lastErrorResult =
                 downloadresultcode::FINISHED):
      completed_(completed),
      error_(error),
      inProgress_(inProgress),
      waiting_(waiting),
      lastErrorResult_(lastErrorResult) {}

    downloadresultcode::RESULT getLastErrorResult() const
    {
      return lastErrorResult_;
    }

    bool allCompleted() const
    {
      return error_ == 0 && inProgress_ == 0 && waiting_ == 0;
    }

    size_t getInProgress() const
    {
      return inProgress_;
    }
  };

  DownloadStat getDownloadStat() const;

  const std::deque<SharedHandle<DownloadResult> >& getDownloadResults() const
  {
    return downloadResults_;
  }

  SharedHandle<DownloadResult> findDownloadResult(gid_t gid) const;

  // Removes all download results.
  void purgeDownloadResult();

  SharedHandle<ServerStat> findServerStat(const std::string& hostname,
                                          const std::string& protocol) const;

  SharedHandle<ServerStat> getOrCreateServerStat(const std::string& hostname,
                                                 const std::string& protocol);

  bool addServerStat(const SharedHandle<ServerStat>& serverStat);

  void updateServerStat();

  bool loadServerStat(const std::string& filename);

  bool saveServerStat(const std::string& filename) const;

  void removeStaleServerStat(time_t timeout);

  // Returns true if current download speed exceeds
  // maxOverallDownloadSpeedLimit_.  Always returns false if
  // maxOverallDownloadSpeedLimit_ == 0.  Otherwise returns false.
  bool doesOverallDownloadSpeedExceed();

  void setMaxOverallDownloadSpeedLimit(unsigned int speed)
  {
    maxOverallDownloadSpeedLimit_ = speed;
  }

  unsigned int getMaxOverallDownloadSpeedLimit() const
  {
    return maxOverallDownloadSpeedLimit_;
  }

  // Returns true if current upload speed exceeds
  // maxOverallUploadSpeedLimit_. Always returns false if
  // maxOverallUploadSpeedLimit_ == 0. Otherwise returns false.
  bool doesOverallUploadSpeedExceed();

  void setMaxOverallUploadSpeedLimit(unsigned int speed)
  {
    maxOverallUploadSpeedLimit_ = speed;
  }

  unsigned int getMaxOverallUploadSpeedLimit() const
  {
    return maxOverallUploadSpeedLimit_;
  }

  void setMaxSimultaneousDownloads(unsigned int max)
  {
    maxSimultaneousDownloads_ = max;
  }

  // Call this function if requestGroups_ queue should be maintained.
  // This function is added to reduce the call of maintenance, but at
  // the same time, it provides fast maintenance reaction.
  void requestQueueCheck()
  {
    queueCheck_ = true;
  }

  void clearQueueCheck()
  {
    queueCheck_ = false;
  }

  bool queueCheckRequested() const
  {
    return queueCheck_;
  }

  // Returns currently used hosts and its use count.
  void getUsedHosts(std::vector<std::pair<size_t, std::string> >& usedHosts);
};

typedef SharedHandle<RequestGroupMan> RequestGroupManHandle;

} // namespace aria2

#endif // _D_REQUEST_GROUP_MAN_H_
