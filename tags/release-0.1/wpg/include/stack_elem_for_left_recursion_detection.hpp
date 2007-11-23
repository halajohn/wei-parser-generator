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

#ifndef __stack_elem_for_left_recursion_detection_hpp__
#define __stack_elem_for_left_recursion_detection_hpp__

class node_t;

struct stack_elem_for_left_recursion_detection
{
  node_t *mp_rule_node;
  node_t *mp_source_node;
  
  stack_elem_for_left_recursion_detection(
    node_t * const rule_node,
    node_t * const source_node)
    : mp_rule_node(rule_node),
      mp_source_node(source_node)
  { }
};
typedef struct stack_elem_for_left_recursion_detection stack_elem_for_left_recursion_detection;

#endif
