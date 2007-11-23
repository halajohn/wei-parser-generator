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

#ifndef __hash_hpp__
#define __hash_hpp__

#include "node.hpp"

using namespace boost::multi_index;

class terminal_name {};
class node_name {};
class node_overall_idx {};

typedef boost::multi_index_container<
  std::wstring,
  indexed_by<
    hashed_unique<tag<terminal_name>, identity<std::wstring> >
    >
  > terminal_hash_table_t;

typedef boost::multi_index_container<
  node_t *,
  indexed_by<
    hashed_unique<tag<node_name>,
                  const_mem_fun<node_t,
                                std::wstring const &,
                                &node_t::name> >
    >
  > rule_head_hash_table_t;

typedef boost::multi_index_container<
  node_t *,
  indexed_by<
    hashed_unique<tag<node_overall_idx>,
                  const_mem_fun<node_t,
                                unsigned int,
                                &node_t::overall_idx> >
    >
  > answer_node_hash_table_t;

#endif
