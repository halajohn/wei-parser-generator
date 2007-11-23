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

#ifndef __lookahead_hpp__
#define __lookahead_hpp__

#include "global.hpp"

class node_t;
struct lookahead_set_t;

enum LOOKAHEAD_COMPARE_RESULT
{
  LOOKAHEAD_OK,
  LOOKAHEAD_NEED_MORE_SEARCH,
  LOOKAHEAD_AMBIGIOUS
};
typedef enum LOOKAHEAD_COMPARE_RESULT LOOKAHEAD_COMPARE_RESULT;

class recur_lookahead_t
{
private:
  
  node_t * const mp_node;
  unsigned int * const mp_prev_level_walk_count;
  
public:
  
  recur_lookahead_t(
    node_t * const node,
    unsigned int * const prev_level_walk_count)
    : mp_node(node),
      mp_prev_level_walk_count(prev_level_walk_count)
  {
    assert(node != 0);
  }
  
  node_t *node() const { return mp_node; }
  unsigned int *prev_level_walk_count() const { return mp_prev_level_walk_count; }
};
typedef class recur_lookahead_t recur_lookahead_t;

struct lookahead_set_t
{
  node_t * mp_orig_node; /* original grammar node for lookahead search */
  node_t * const mp_node; /* lookahead terminal */
  unsigned int const m_level;
  std::list<lookahead_set_t> m_next_level; /* next level lookahead terminals */
  
  lookahead_set_t(
    node_t * const orig_node,
    node_t * const node,
    unsigned int const level)
    : mp_orig_node(orig_node),
      mp_node(node),
      m_level(level)
  { }
  
  lookahead_set_t *find_lookahead(node_t * const node);
  lookahead_set_t *insert_lookahead(node_t * const node);
};
typedef struct lookahead_set_t lookahead_set_t;

struct todo_lookahead_t
{
  lookahead_set_t * const mp_lookahead_a; /* need to find more lookahead terminals
                                           * from this lookahead terminal.
                                           */
  
  lookahead_set_t * const mp_lookahead_b; /* need to find more lookahead terminals
                                           * from this lookahead terminal.
                                           */
  
  todo_lookahead_t(
    lookahead_set_t * const lookahead_a,
    lookahead_set_t * const lookahead_b)
    : mp_lookahead_a(lookahead_a),
      mp_lookahead_b(lookahead_b)
  { }
};
typedef struct todo_lookahead_t todo_lookahead_t;

/// 
/// \typedef node_with_order_t
/// 
/// The 'first' component in the std::pair of this type is
/// to remember the alternative index of this alternative.
/// Ex:
/// 
///  A
///    a b
///    c d
///    d e
/// 
/// then the 'first' value of 'a' is 0, and
/// the 'first' value of 'c' is 1, and
/// the 'first' value of 'd' is 2.
/// 
/// However, if the alternative starting from 'c' is
/// expended from the alternative starting from 'a' because
/// of regular expression expansion, then the 'first' value
/// of 'c' is equal to that of 'a', i.e. 0, and the 'first'
/// value of 'd' is 1, not 2.
///
struct node_with_order_t
{
  size_t m_idx;
  node_t *mp_node;
  
  node_with_order_t(size_t const idx,
                    node_t * const node)
    : m_idx(idx),
      mp_node(node)
  { }
};
typedef struct node_with_order_t node_with_order_t;

#endif
