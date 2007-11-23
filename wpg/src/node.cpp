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
#include "ga_exception.hpp"
#include "global.hpp"
#include "alternative.hpp"

#include "wcl_memory_debugger\memory_debugger.h"

void
node_t::init()
{
  m_traversed = false;
  m_create_dot_node_line = false;
  m_is_nullable = false;
  mp_apostrophe_node = 0;
  mp_apostrophe_orig_node = 0;
  mp_alternative_start = 0;
  m_lookahead_depth = 0;
  m_starting_rule = false;
  m_is_eof = false;
  
  m_rule_contain_regex = RULE_CONTAIN_NO_REGEX;
  mp_main_regex_alternative = 0;
  
  mp_corresponding_node_in_main_regex_alternative = this;
  mp_brother_node_to_update_regex = 0;
  
  m_lookahead_set.mp_orig_node = this;
  
  // I use NOT_CAL_ALTER_LENGTH_YEY to indicate that I don't
  // calculate the length of this alternative yet.
  m_alternative_length = NOT_CAL_ALTER_LENGTH_YET;
  
  m_contains_ambigious = false;
}

node_t::node_t(analyser_environment_t * const ae,
               node_t * const rule_node,
               std::wstring const &str)
  : mp_ae(ae),
    m_is_rule_head(false),
    m_name(str),
    mp_last_edge_last_node(0),
    mp_rule_end_node(0),
    mp_nonterminal_rule_node(0),
    m_optional(false),
    m_lookahead_set(0, 0, 0),
    m_overall_idx(0),
    m_is_in_regex_OR_group(false)
{
  init();
  
  if (0 == rule_node)
  {
    /* This is a rule node. */
    mp_rule_node = this;
  }
  else
  {
    mp_rule_node = rule_node;
  }
  
  m_is_terminal = ae->is_terminal(str);
}

/// @brief Copy constructor
///
/// I will store objects of this type to
/// boost::multi_index_container, and boost requires that
/// such objects have to be copyable, and at least the field
/// used for the hash key has to be copied correctly, thus I
/// copy 'm_overall_idx' field here.
///
/// @param node 
///
node_t::node_t(node_t const * const node)
  : mp_ae(node->ae()),
    m_is_rule_head(node->is_rule_head()),
    m_is_terminal(node->is_terminal()),
    m_name(node->name()),
    mp_nonterminal_rule_node(node->nonterminal_rule_node()),
    m_optional(node->is_optional()),
    m_lookahead_set(0, 0, 0),
    m_overall_idx(node->overall_idx()),
    m_is_in_regex_OR_group(node->is_in_regex_OR_group())
{
  init();
}

bool
node_t::is_my_prev_node(node_t const * const node) const
{
  BOOST_FOREACH(node_t * const compared_node, m_prev_nodes)
  {
    if (node == compared_node)
    {
      return true;
    }
  }
  return false;
}

bool
node_t::is_my_next_node(node_t const * const node) const
{
  BOOST_FOREACH(node_t * const compared_node, m_next_nodes)
  {
    if (node == compared_node)
    {
      return true;
    }
  }
  return false;
}

bool
node_t::is_my_descendant_node(node_t const * const node) const
{
  if (true == is_my_next_node(node))
  {
    return true;
  }
  else
  {
    BOOST_FOREACH(node_t * const next_node, m_next_nodes)
    {
      if (true == next_node->is_my_descendant_node(node))
      {
        return true;
      }
    }
    
    return false;
  }
}

bool
node_t::is_my_ancestor_node(node_t const * const node) const
{
  if (true == is_my_prev_node(node))
  {
    return true;
  }
  else
  {
    BOOST_FOREACH(node_t * const prev_node, m_prev_nodes)
    {
      if (true == prev_node->is_my_ancestor_node(node))
      {
        return true;
      }
    }
    
    return false;
  }
}

bool
node_t::is_refer_to_me_node(node_t const * const node) const
{
  assert(true == m_is_rule_head);
  
  BOOST_FOREACH(node_t * const refer_to_me_node,
                m_refer_to_me_nodes)
  {
    if (refer_to_me_node == node)
    {
      return true;
    }
  }
  return false;
}

void
node_t::append_node(node_t * const node)
{
  assert(false == is_my_next_node(node));
  
  m_next_nodes.push_back(node);
  
  if (true == m_is_rule_head)
  {
    // This is a rule head, the appending node is an alternative start or
    // a rule end node.
    if (node->name().size() != 0)
    {
      node->alternative_start() = node;
    }
  }
  else if (0 == node->name().size())
  {
    // The appending node is a rule end node, which can not set the
    // 'mp_alternative_start'.
    return;
  }
  else
  {
    assert(mp_alternative_start != 0);
    
    node->alternative_start() = mp_alternative_start;
  }
}

void
node_t::prepend_node(node_t * const node)
{
  assert(false == is_my_prev_node(node));
  
  m_prev_nodes.push_back(node);
}

std::list<node_t *>::iterator 
node_t::break_append_node(node_t * const node)
{
  std::list<node_t *>::iterator iter;
  
  for (iter = m_next_nodes.begin();
       iter != m_next_nodes.end();
       ++iter)
  {
    if (node == *iter)
    {
      return m_next_nodes.erase(iter);
    }
  }
  assert(0);
  return m_next_nodes.begin();
}

std::list<node_t *>::iterator 
node_t::break_prepend_node(node_t * const node)
{
  std::list<node_t *>::iterator iter;
  
  for (iter = m_prev_nodes.begin();
       iter != m_prev_nodes.end();
       ++iter)
  {
    if (node == *iter)
    {
      return m_prev_nodes.erase(iter);
    }
  }
  assert(0);
  return m_prev_nodes.begin();
}

void
node_t::set_rule_end_node(node_t * const node)
{
  assert(true == m_is_rule_head);
  mp_rule_end_node = node;
}

void
node_t::set_node_before_the_rule_end_node(node_t * const node)
{
  if (node->is_rule_head())
  {
    assert(0);
    return;
  }
  if (0 == node)
  {
    assert(0);
    return;
  }
  mp_node_before_the_rule_end_node = node;
}

node_t *
node_t::node_before_the_rule_end_node()
{
  return mp_node_before_the_rule_end_node;
}

void
node_t::set_create_dot_node_line(bool const status)
{
  m_create_dot_node_line = status;
}

void
node_t::add_same_rule_node(node_t * const node)
{
  m_same_rule_nodes.push_back(node);
}

void
node_t::remove_refer_to_me_node(node_t * const node)
{
  for (std::list<node_t *>::iterator iter = m_refer_to_me_nodes.begin();
       iter != m_refer_to_me_nodes.end();
       ++iter)
  {
    if ((*iter) == node)
    {
      m_refer_to_me_nodes.erase(iter);
      return;
    }
  }
  assert(0);
}

void
node_t::add_refer_to_me_node(node_t * const node)
{
  assert(this != node);
  
  m_refer_to_me_nodes.push_back(node);
  
#if defined(_DEBUG)
  // Check to see there are no two equal nodes could be
  // stored in the 'refer_to_me_nodes' set.
  for (std::list<node_t *>::const_iterator iter = m_refer_to_me_nodes.begin();
       iter != m_refer_to_me_nodes.end();
       ++iter)
  {
    std::list<node_t *>::const_iterator tmp = iter;
    ++tmp;
    for (std::list<node_t *>::const_iterator iter2 = tmp;
         iter2 != m_refer_to_me_nodes.end();
         ++iter2)
    {
      assert((*iter) != (*iter2));
    }
  }
#endif
}

void
node_t::assign_cyclic_set(std::list<node_t *> const &set)
{
  BOOST_FOREACH(node_t * const node, set)
  {
    m_cyclic_set.push_back(node);
  }
}

void
node_t::assign_left_recursion_set(std::list<node_t *> const &set)
{
  BOOST_FOREACH(node_t * const node, set)
  {
    m_left_recursion_set.push_back(node);
  }
}

node_t *
node_t::find_alternative_last_node()
{
  assert(m_name.size() != 0);
  assert(1 == m_next_nodes.size());
  
  node_t *curr_node = this;
  node_t const * const rule_node = curr_node->rule_node();
  
  assert(1 == curr_node->next_nodes().size());
  
  while (curr_node->next_nodes().front() != rule_node->rule_end_node())
  {
    assert(curr_node->rule_node() == rule_node);
    assert(1 == curr_node->next_nodes().size());
    
    curr_node = curr_node->next_nodes().front();
  }
  
  assert(1 == curr_node->next_nodes().size());
  assert(curr_node->next_nodes().front() == rule_node->rule_end_node());

  return curr_node;
}

void
node_t::add_ambigious_set(
  node_t * const node)
{
  assert(node != this);
  
  for (std::list<node_t *>::const_iterator iter = m_ambigious_set.begin();
       iter != m_ambigious_set.end();
       ++iter)
  {
    if ((*iter) == node)
    {
      return;
    }
  }
  
  m_ambigious_set.push_back(node);
}

bool
node_t::is_ambigious_to(node_t * const node) const
{
  assert(node != this);
  for (std::list<node_t *>::const_iterator iter = m_ambigious_set.begin();
       iter != m_ambigious_set.end();
       ++iter)
  {
    if ((*iter) == node)
    {
      return true;
    }
  }
  return false;
}

void
node_t::append_name(wchar_t const * const str)
{
  assert(str != 0);
  
  m_name += str;
}

namespace
{
  bool
  compare_lookahead_set_for_each_lookahead_set(
    lookahead_set_t const &lookahead_set_a,
    lookahead_set_t const &lookahead_set_b)
  {
    if (lookahead_set_a.m_next_level.size() !=
        lookahead_set_b.m_next_level.size())
    {
      return false;
    }
    
    std::list<lookahead_set_t const *> pool_a;
    for (std::list<lookahead_set_t>::const_iterator iter =
           lookahead_set_a.m_next_level.begin();
         iter != lookahead_set_a.m_next_level.end();
         ++iter)
    {
      pool_a.push_back(&(*iter));
    }
    std::list<lookahead_set_t const *> pool_b;
    for (std::list<lookahead_set_t>::const_iterator iter =
           lookahead_set_b.m_next_level.begin();
         iter != lookahead_set_b.m_next_level.end();
         ++iter)
    {
      pool_b.push_back(&(*iter));
    }
    
    while (pool_a.size() != 0)
    {
      bool find_in_pool_b = false;
      
      lookahead_set_t const * const lookahead_set_pool_a = pool_a.front();
      for (std::list<lookahead_set_t const *>::iterator iter =
             pool_b.begin();
           iter != pool_b.end();
           ++iter)
      {
        if (0 == lookahead_set_pool_a->mp_node->name().compare(
              (*iter)->mp_node->name()))
        {
          find_in_pool_b = true;
          
          if (false == compare_lookahead_set_for_each_lookahead_set(
                *lookahead_set_pool_a,
                **iter))
          {
            return false;
          }
          else
          {
            pool_b.erase(iter);
          }
          break;
        }
      }
      if (false == find_in_pool_b)
      {
        return false;
      }
      pool_a.pop_front();
    }
    
    return true;
  }
}

bool
node_t::compare_lookahead_set(node_t * const node) const
{
  return compare_lookahead_set_for_each_lookahead_set(
    m_lookahead_set,
    node->lookahead_set());
}

void
node_t::add_token_name_as_terminal_during_lookahead(std::wstring const &str)
{
  if (m_is_rule_head != true)
  {
    throw ga_exception_t();
  }
  m_token_name_as_terminal_during_lookahead.push_back(str);
}

std::list<std::wstring> const &
node_t::token_name_as_terminal_during_lookahead() const
{
  if (m_is_rule_head != true)
  {
    throw ga_exception_t();
  }
  return m_token_name_as_terminal_during_lookahead;
}

unsigned int
node_t::count_node_number_between_this_node_and_rule_end_node() const
{
  assert(false == m_is_rule_head);
  assert(1 == m_next_nodes.size());
  
  unsigned int count = 1;
  node_t *curr_node = m_next_nodes.front();
  
  while (curr_node->name().size() != 0)
  {
    assert(1 == curr_node->next_nodes().size());
    
    ++count;
    curr_node = curr_node->next_nodes().front();
  }

  return count;
}

void
node_t::restore_regex_info()
{
  m_regex_info.splice(m_regex_info.begin(),
                      m_tmp_regex_info);
}

void
node_t::check_if_regex_info_is_correct() const
{
#if defined(_DEBUG)
  node_t *prev_start_node = 0;
  node_t *prev_end_node = 0;
  regex_type_t prev_regex_type;
  
  BOOST_FOREACH(regex_info_t const &regex_info, m_regex_info)
  {
    ::check_if_regex_info_is_correct(this, regex_info);
    
    node_t *real_start_node;
    node_t *real_end_node;
    
    switch (regex_info.m_type)
    {
    case REGEX_TYPE_ZERO_OR_MORE:
    case REGEX_TYPE_ZERO_OR_ONE:
    case REGEX_TYPE_ONE_OR_MORE:
    case REGEX_TYPE_ONE:
      real_start_node = regex_info.m_ranges.front().mp_start_node;
      real_end_node = regex_info.m_ranges.front().mp_end_node;
      break;
      
    case REGEX_TYPE_OR:
      real_start_node = regex_info.m_ranges.front().mp_start_node;
      real_end_node = regex_info.m_ranges.back().mp_end_node;
      break;
      
    default:
      assert(0);
      break;
    }
    
    if (prev_start_node != 0)
    {
      assert(prev_end_node != 0);
      assert(prev_start_node == real_start_node);
      
      if (REGEX_TYPE_OR == prev_regex_type)
      {
        assert(prev_end_node == real_end_node);
      }
      else
      {
        assert(prev_end_node != real_end_node);
      }
      
      // because I examine the regex_range from first to
      // last, thus the later one should contain the
      // previous one.
      assert(true == check_if_node_in_node_range(
               prev_end_node,
               real_start_node,
               real_end_node));
    }
    
    prev_start_node = real_start_node;
    prev_end_node = real_end_node;
    prev_regex_type = regex_info.m_type;
  }
#endif
}

bool
check_if_regex_info_is_empty(
  analyser_environment_t const * const,
  node_t * const node,
  void *param)
{
  if (0 == node->regex_info().size())
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool
check_if_regex_info_is_correct_for_traverse_all_nodes(
  analyser_environment_t const * const,
  node_t * const node,
  void *param)
{
  node->check_if_regex_info_is_correct();
  
  return true;
}

bool
delete_regex_type_one(
  analyser_environment_t const * const,
  node_t * const node,
  void *param)
{
  for (std::list<regex_info_t>::iterator iter = node->tmp_regex_info().begin();
       iter != node->tmp_regex_info().end();
       )
  {
    assert((*iter).m_type != REGEX_TYPE_OR);
    
    if (REGEX_TYPE_ONE == (*iter).m_type)
    {
      iter = node->tmp_regex_info().erase(iter);
    }
    else
    {
      ++iter;
    }
  }
  
  return true;
}

bool
clear_node_state(
  analyser_environment_t const * const ae,
  node_t * const node,
  void * const param)
{
  node->set_traversed(false);
  node->set_create_dot_node_line(false);
  node->clear_same_rule_nodes();
  
  if (node->is_rule_head())
  {
    node->clear_temp_append_to_rule_node();
    node->clear_temp_prepend_to_end_node();
  }
  
  return true;
}

bool
node_for_each_not_reach_end(
  node_t const * const curr_node,
  node_t const * const end_node)
{
  if (end_node != 0)
  {
    if (curr_node != end_node)
    {
      return true;
    }
    else
    {
      return false;
    }
  }
  else
  {
    if (curr_node->name().size() != 0)
    {
      return true;
    }
    else
    {
      return false;
    }
  }
}
