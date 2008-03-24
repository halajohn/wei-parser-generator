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

#ifndef __alternative_hpp__
#define __alternative_hpp__

class analyser_environment_t;
class node_t;

extern bool find_same_alternative_in_one_rule(
  node_t * const alternative_start,
  std::list<node_t *> * const duplicated_alternatives);

extern bool find_same_alternative_in_one_rule(
  node_t * const alternative_start);

extern bool fork_alternative(
  node_t * const alternative_start,
  node_t * const target_node,
  node_t * const target_end_node,
  std::list<node_t *> const &discard_node,
  bool const after_link_nonterminal);

extern void remove_duplicated_alternatives_for_one_rule(
  analyser_environment_t * const ae,
  node_t * const node);

extern void remove_duplicated_alternatives_for_selected_alternative(
  analyser_environment_t * const ae,
  node_t * const node);

extern void remove_alternatives(
  analyser_environment_t * const ae,
  std::list<node_t *> &alternatives,
  bool const call_in_ae_destructor,
  bool const after_link_nonterminal,
  bool const still_in_rule_node_next_nodes_group);

extern unsigned int count_alternative_length(
  node_t * const alternative_start);

extern void fork_alternative_with_some_duplicated_nodes(
  node_t * const alternative_start,
  node_t * const dup_range_start,
  node_t * const dup_range_end,
  node_t * const attached_pos,
  std::list<node_t *> &new_dup_nodes);

extern node_t *find_alternative_end(
  node_t * const node);

extern void remove_same_alternative_for_one_rule(
  analyser_environment_t * const ae,
  node_t * const rule_node);

extern void restore_regex_info_for_one_alternative(
  node_t * const node);

#endif
