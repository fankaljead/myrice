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
#ifndef _D_UNION_SEED_CRITERIA_H_
#define _D_UNION_SEED_CRITERIA_H_

#include "SeedCriteria.h"
#include <algorithm>
#include <vector>

namespace aria2 {

class UnionSeedCriteria : public SeedCriteria {
private:
  std::vector<SharedHandle<SeedCriteria> > criterion_;

  class Reset {
  public:
    void operator()(SharedHandle<SeedCriteria> cri)
    {
      cri->reset();
    }
  };

  class Eval {
  public:
    bool operator()(SharedHandle<SeedCriteria> cri)
    {
      return cri->evaluate();
    }
  };
public:
  UnionSeedCriteria() {}
  virtual ~UnionSeedCriteria() {}

    
  virtual void reset()
  {
    std::for_each(criterion_.begin(), criterion_.end(), Reset());
  }

  virtual bool evaluate()
  {
    std::vector<SharedHandle<SeedCriteria> >::iterator itr =
      std::find_if(criterion_.begin(), criterion_.end(), Eval());
    return itr != criterion_.end();
  }

  void addSeedCriteria(const SharedHandle<SeedCriteria>& cri)
  {
    criterion_.push_back(cri);
  }

  const std::vector<SharedHandle<SeedCriteria> >& getSeedCriterion() const
  {
    return criterion_;
  }
};

} // namespace aria2
      
#endif // _D_UNION_SEED_CRITERIA_H_
