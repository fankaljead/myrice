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
#include "CheckIntegrityDispatcherCommand.h"
#include "CheckIntegrityEntry.h"
#include "CheckIntegrityCommand.h"
#include "message.h"
#include "Logger.h"
#include "FileEntry.h"
#include "util.h"
#include "ServerStatMan.h"
#include "FileAllocationEntry.h"

namespace aria2 {

CheckIntegrityDispatcherCommand::CheckIntegrityDispatcherCommand
(cuid_t cuid,
 const SharedHandle<CheckIntegrityMan>& fileAllocMan,
 DownloadEngine* e):SequentialDispatcherCommand<CheckIntegrityEntry>
                    (cuid, fileAllocMan, e)
{
  setStatusRealtime();
}

Command* CheckIntegrityDispatcherCommand::createCommand
(const SharedHandle<CheckIntegrityEntry>& entry)
{
  cuid_t newCUID = getDownloadEngine()->newCUID();
  if(getLogger()->info()) {
    getLogger()->info("CUID#%s - Dispatching CheckIntegrityCommand CUID#%s.",
                      util::itos(getCuid()).c_str(),
                      util::itos(newCUID).c_str());
  }
  return new CheckIntegrityCommand
    (newCUID, entry->getRequestGroup(), getDownloadEngine(), entry);
}

} // namespace aria2
