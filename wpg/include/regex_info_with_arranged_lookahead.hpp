#include "precompiled_header.hpp"

// wpg - an LL(k) parser generator
// Copyright (C) <2007>  Wei Hu <wei.hu.tw@gmail.com>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef __regex_info_with_arranged_lookahead_hpp__
#define __regex_info_with_arranged_lookahead_hpp__

#include "arranged_lookahead.hpp"

struct regex_info_t;

struct regex_info_with_arranged_lookahead_t
{
  regex_info_t const * const mp_regex_info;
  bool m_first_enter_this_regex;
  node_t *mp_curr_node;
  arranged_lookahead_t::iterator m_arranged_lookahead_tree_iter;
  boost::shared_ptr<arranged_lookahead_t> mp_arranged_lookahead_tree;
  std::list<arranged_lookahead_t const *> mp_current_dumpped_lookahead_nodes;
  
  regex_info_with_arranged_lookahead_t(
    regex_info_t const * const regex_info)
    : mp_regex_info(regex_info),
      m_first_enter_this_regex(true)
  { }
};
typedef struct regex_info_with_arranged_lookahead_t regex_info_with_arranged_lookahead_t;

#endif
