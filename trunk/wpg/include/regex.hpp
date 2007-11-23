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

#ifndef __regex_hpp__
#define __regex_hpp__

class node_t;

enum regex_type_t
{
  ///
  /// ()*
  ///
  REGEX_TYPE_ZERO_OR_MORE,
  
  ///
  /// ()?
  ///
  REGEX_TYPE_ZERO_OR_ONE,
  
  ///
  /// ()+
  ///
  REGEX_TYPE_ONE_OR_MORE,
  
  ///
  /// ()
  ///
  REGEX_TYPE_ONE,
  
  ///
  /// (|)
  ///
  REGEX_TYPE_OR
};
typedef enum regex_type_t regex_type_t;

typedef int regex_group_id_t;
#define REGEX_GROUP_ID_NONE ((int)-1)

struct regex_range_t
{
  node_t *mp_start_node;
  node_t *mp_end_node;
  
  ///
  /// used for code gen.
  ///
  regex_group_id_t m_regex_group_id;
  
  void
  init()
  {
    m_regex_group_id = REGEX_GROUP_ID_NONE;
  }
  
  /// \brief default constructor
  ///
  /// \param start_node 
  /// \param end_node 
  ///
  /// \return 
  ///
  regex_range_t(node_t * const start_node,
                node_t * const end_node)
    : mp_start_node(start_node),
      mp_end_node(end_node)
  {
    init();
  }
};
typedef struct regex_range_t regex_range_t;

struct regex_stack_elem_t
{
  node_t *mp_node;
  std::list<regex_range_t> m_regex_OR_range;
  
  regex_stack_elem_t()
    : mp_node(0)
  { }
};
typedef struct regex_stack_elem_t regex_stack_elem_t;

struct regex_info_t
{
  std::list<regex_range_t> m_ranges;
  
  ///
  /// Used for code gen.
  ///
  regex_info_t *mp_regex_OR_info;
  
  regex_type_t m_type;
  
  regex_info_t(
    std::list<regex_range_t> const &range,
    regex_type_t const type)
    : m_ranges(range),
      mp_regex_OR_info(0),
      m_type(type)
  {
  }
  
  regex_info_t(
    node_t * const begin,
    node_t * const end,
    regex_info_t * const regex_OR_info,
    regex_type_t const type)
    : mp_regex_OR_info(regex_OR_info),
      m_type(type)
  {
    m_ranges.push_back(regex_range_t(begin, end));
  }
  
  /// \brief default constructor
  ///
  ///
  /// \return 
  ///
  regex_info_t()
    : mp_regex_OR_info(0),
      m_type(REGEX_TYPE_ONE_OR_MORE)
  {
  }
};
typedef struct regex_info_t regex_info_t;

extern void update_regex_info_for_one_alternative(
  node_t * const alternative_start);

extern void update_regex_info_for_node_range(
  node_t * const range_start_node,
  node_t * const range_end_node);

extern bool check_if_regex_info_is_correct_for_one_node(
  node_t * const node);

extern void check_if_regex_info_is_correct(
  node_t const * const regex_start_node,
  regex_info_t const &regex_info);

extern bool move_all_regex_info_to_tmp_front(
  node_t * const node);

extern bool move_all_tmp_regex_info_to_orig_end(
  node_t * const node);

extern std::list<regex_info_t>::const_reverse_iterator find_regex_info_using_its_begin_and_end_node(
  std::list<regex_info_t> &regex_info_list,
  node_t * const start_node,
  node_t * const end_node);

extern regex_range_t const *find_regex_range_which_this_node_belongs_to(
  regex_info_t const * const regex_info,
  node_t const * const node,
  bool * const is_regex_OR_range);

extern void find_possible_starting_nodes_for_regex(
  node_t * const node,
  std::list<regex_info_t>::const_reverse_iterator const node_regex_info_iter,
  std::list<node_t *> &nodes);

extern void find_possible_end_nodes_for_regex(
  node_t * const node,
  std::list<regex_info_t>::const_reverse_iterator const node_regex_info_iter,
  std::list<node_t *> &nodes);

extern void collect_candidate_nodes_for_lookahead_calculation_for_EBNF_rule(
  node_t const * const rule_node,
  std::list<regex_info_t>::const_reverse_iterator iter,
  std::list<node_t *> &nodes);

extern bool check_if_this_node_is_the_starting_one_of_a_regex_OR_range(
  regex_info_t const * const regex_info,
  node_t const * const node);

extern bool check_if_this_node_is_after_this_regex(
  node_t const * const node,
  regex_info_t const * const regex_info);

#endif
