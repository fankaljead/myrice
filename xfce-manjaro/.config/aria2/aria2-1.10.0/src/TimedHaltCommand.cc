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
#include "TimedHaltCommand.h"
#include "DownloadEngine.h"
#include "RequestGroupMan.h"
#include "Logger.h"
#include "message.h"
#include "FileEntry.h"
#include "ServerStatMan.h"
#include "FileAllocationEntry.h"
#include "CheckIntegrityEntry.h"

namespace aria2 {

TimedHaltCommand::TimedHaltCommand(cuid_t cuid, DownloadEngine* e,
                                   time_t secondsToHalt,
                                   bool forceHalt):
  TimeBasedCommand(cuid, e, secondsToHalt, true),
  forceHalt_(forceHalt) {}

TimedHaltCommand::~TimedHaltCommand() {}

void TimedHaltCommand::preProcess()
{
  if(getDownloadEngine()->getRequestGroupMan()->downloadFinished() ||
     getDownloadEngine()->isHaltRequested()) {
    enableExit();
  }
}

void TimedHaltCommand::process()
{
  if(!getDownloadEngine()->isHaltRequested()) {
    getLogger()->notice(MSG_TIME_HAS_PASSED, getInterval());
    if(forceHalt_) {
      getLogger()->notice("This is emergency shutdown.");
      getDownloadEngine()->requestForceHalt();
    } else {
      getDownloadEngine()->requestHalt();
    }
    enableExit();
  }
}

} // namespace aria2
