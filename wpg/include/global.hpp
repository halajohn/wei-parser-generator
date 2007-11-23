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

#ifndef __global_hpp__
#define __global_hpp__

class node_t;

enum node_rel_t
{
  NODE_REL_NORMAL,
  NODE_REL_POINT_TO_RULE_HEAD,
  NODE_REL_RULE_HEAD_REFER_TO_NODE
};
typedef enum node_rel_t node_rel_t;  

enum keyword_pos_t
{
  KEYWORD_POS_FRONT,
  KEYWORD_POS_BACK,
  KEYWORD_POS_ANYWHERE
};
typedef enum keyword_pos_t keyword_pos_t;

struct keyword_t
{
  std::wstring name;
  keyword_pos_t pos_constraint;
  
  keyword_t(
    std::wstring const &p_name,
    keyword_pos_t const p_pos)
    : name(p_name),
      pos_constraint(p_pos)
  { }
};
typedef struct keyword_t keyword_t;

class node_t;

struct todo_nodes_t
{
  node_t *mp_node_a;
  node_t *mp_node_b;
  
  todo_nodes_t(
    node_t * const node_a,
    node_t * const node_b)
    : mp_node_a(node_a),
      mp_node_b(node_b)
  { }
};
typedef struct todo_nodes_t todo_nodes_t;

struct last_symbol_t
{
  node_t * const mp_node;
  unsigned int const level;
  
  last_symbol_t(
    node_t * const node,
    unsigned int const curr_level)
    : mp_node(node),
      level(curr_level)
  {
  }
};
typedef struct last_symbol_t last_symbol_t;

struct appear_max_and_curr_time
{
  unsigned int max_appear_times;
  unsigned int curr_appear_times;
};
typedef struct appear_max_and_curr_time appear_max_and_curr_time;

class analyser_environment_t;
class node_t;

typedef bool (*check_node_func)(node_t * const node);

extern void report(
  std::wfstream * const out_file,
  wchar_t const * const fmt, ...);

extern bool check_not_cyclic(
  analyser_environment_t const * const /* ae */,
  node_t * const node,
  void * const param);

extern void add_to_set_unique(
  std::list<node_t *> &set,
  node_t * const node);

extern std::list<node_t *>::iterator check_if_one_node_is_belong_to_set_and_match_some_condition(
  std::list<node_t *> &set,
  node_t * const node,
  std::vector<check_node_func> const &check_func);

extern wchar_t *form_rule_end_node_name(
  node_t * const node,
  bool const include_number);

extern unsigned int count_node_range_length(
  node_t const * const range_start,
  node_t const * const range_end);

extern bool check_if_node_in_node_range(
  node_t const * const check_node,
  node_t const * const range_start,
  node_t const * const range_end);

extern unsigned int count_minimum_number_of_nodes_in_node_range(
  node_t const * const range_start,
  node_t const * const range_end);

extern unsigned int get_max_value(
  unsigned int const value_1,
  unsigned int const value_2,
  unsigned int const value_3);

extern void count_appear_times_for_node_range(
  node_t * const node_start,
  node_t * const node_end);

#endif
