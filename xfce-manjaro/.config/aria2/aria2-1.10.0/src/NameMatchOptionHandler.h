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
#ifndef _D_NAME_MATCH_OPTION_HANDLER_H_
#define _D_NAME_MATCH_OPTION_HANDLER_H_

#include "OptionHandler.h"

#include <strings.h>

#include <algorithm>
#include <sstream>
#include <iterator>
#include <vector>

#include "A2STR.h"
#include "util.h"
#include "OptionHandlerException.h"
#include "a2functional.h"

#define NO_DESCRIPTION A2STR::NIL
#define NO_DEFAULT_VALUE A2STR::NIL

namespace aria2 {

class Option;

class NameMatchOptionHandler : public OptionHandler {
protected:
  std::string optName_;

  std::string description_;

  std::string defaultValue_;

  std::vector<std::string> tags_;

  int id_;

  OptionHandler::ARG_TYPE argType_;

  char shortName_;

  bool hidden_;

  virtual void parseArg(Option& option, const std::string& arg) = 0;
public:
  NameMatchOptionHandler(const std::string& optName,
                         const std::string& description = NO_DESCRIPTION,
                         const std::string& defaultValue = NO_DEFAULT_VALUE,
                         ARG_TYPE argType = REQ_ARG,
                         char shortName = 0):
    optName_(optName),
    description_(description),
    defaultValue_(defaultValue),
    id_(0),
    argType_(argType),
    shortName_(shortName),
    hidden_(false) {}

  virtual ~NameMatchOptionHandler() {}
  
  virtual bool canHandle(const std::string& optName)
  {
    return strcasecmp(optName_.c_str(), optName.c_str()) == 0;
  }

  virtual void parse(Option& option, const std::string& arg)
  {
    try {
      parseArg(option, arg);
    } catch(Exception& e) {
      throw OPTION_HANDLER_EXCEPTION2(optName_, e);
    }
  }

  virtual bool hasTag(const std::string& tag) const
  {
    return std::find(tags_.begin(), tags_.end(), tag) != tags_.end();
  }

  virtual void addTag(const std::string& tag)
  {
    tags_.push_back(tag);
  }

  virtual std::string toTagString() const
  {
    return strjoin(tags_.begin(), tags_.end(), ", ");
  }

  virtual const std::string& getName() const
  {
    return optName_;
  }

  virtual const std::string& getDescription() const
  {
    return description_;
  }

  virtual const std::string& getDefaultValue() const
  {
    return defaultValue_;
  }

  virtual bool isHidden() const
  {
    return hidden_;
  }

  virtual void hide()
  {
    hidden_ = true;
  }

  virtual char getShortName() const
  {
    return shortName_;
  }

  virtual int getOptionID() const
  {
    return id_;
  }

  virtual void setOptionID(int id)
  {
    id_ = id;
  }

  virtual OptionHandler::ARG_TYPE getArgType() const
  {
    return argType_;
  }
};

typedef SharedHandle<NameMatchOptionHandler> NameMatchOptionHandlerHandle;

} // namespace aria2

#endif // _D_NAME_MATCH_OPTION_HANDLER_H_
