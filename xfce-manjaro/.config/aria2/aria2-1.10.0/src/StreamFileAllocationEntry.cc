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
#include "StreamFileAllocationEntry.h"

#include <algorithm>

#include "DownloadEngine.h"
#include "Option.h"
#include "prefs.h"
#include "RequestGroup.h"
#include "DownloadContext.h"
#include "Command.h"
#include "RequestGroupMan.h"
#include "CheckIntegrityEntry.h"
#include "ServerStatMan.h"

namespace aria2 {

StreamFileAllocationEntry::StreamFileAllocationEntry(RequestGroup* requestGroup,
                                                     Command* nextCommand):
  FileAllocationEntry(requestGroup, nextCommand)
{}

StreamFileAllocationEntry::~StreamFileAllocationEntry() {}

void StreamFileAllocationEntry::prepareForNextAction
(std::vector<Command*>& commands,
 DownloadEngine* e)
{
  // For DownloadContext::resetDownloadStartTime(), see also
  // RequestGroup::createInitialCommand()
  getRequestGroup()->getDownloadContext()->resetDownloadStartTime();
  if(getNextCommand()) {
    // give _nextCommand a chance to execute in the next execution loop.
    getNextCommand()->setStatus(Command::STATUS_ONESHOT_REALTIME);
    e->setNoWait(true);
    commands.push_back(popNextCommand());
    // try remaining uris
    getRequestGroup()->createNextCommandWithAdj(commands, e, -1);
  } else {
    getRequestGroup()->createNextCommandWithAdj(commands, e, 0);
  }
}

} // namespace aria2