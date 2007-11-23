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

#include "ae.hpp"
#include "node.hpp"

#include "wcl_memory_debugger\memory_debugger.h"

///
/// @file   grammar_transform.cpp
/// @author Wei Hu
/// @date
/// 
/// @brief  
/// 
/// This file defines some grammar transformations:
/// -1) left factoring
/// -2) NLRG: non-left-recursion-grouping
/// -3) order the nonterminals according to decreasing
/// number of distinct left corners.
///

void
analyser_environment_t::left_factoring()
{
}

void
analyser_environment_t::non_left_recursion_grouping()
{
}

namespace
{
  bool
  sort_func(node_t * const lhs, node_t * const rhs)
  {
    return (lhs->left_corner_set().size() > rhs->left_corner_set().size());
  }
}

void
analyser_environment_t::order_nonterminal_decrease_number_of_distinct_left_corner()
{
  m_top_level_nodes.sort(::sort_func);
}
