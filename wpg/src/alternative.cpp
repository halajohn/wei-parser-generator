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
#include "alternative.hpp"
#include "regex.hpp"

#include "wcl_memory_debugger\memory_debugger.h"

namespace
{
  bool
  check_two_alternatives_are_equal_in_one_rule(
    node_t * const alternative_start,
    node_t * const pivot_alternative)
  {
    assert(alternative_start != 0);
    assert(pivot_alternative != 0);
    
    assert(alternative_start->rule_node() == pivot_alternative->rule_node());
    assert(alternative_start->rule_node()->rule_end_node() ==
           pivot_alternative->rule_node()->rule_end_node());
    
    node_t *node1 = alternative_start;
    node_t *node2 = pivot_alternative;
    
    node_t * const rule_node = alternative_start->rule_node();
    
    while ((node1 != rule_node->rule_end_node()) &&
           (node2 != rule_node->rule_end_node()))
    {
      assert(node1->name().size() != 0);
      assert(node2->name().size() != 0);
      
      assert(1 == node1->prev_nodes().size());
      assert(1 == node2->prev_nodes().size());
      
      assert(1 == node1->next_nodes().size());
      assert(1 == node2->next_nodes().size());
      
      if (node1->name().compare(node2->name()) != 0)
      {
        return false;
      }
      else
      {
        node1 = node1->next_nodes().front();
        node2 = node2->next_nodes().front();
      }
    }
    
    if ((node1 == rule_node->rule_end_node()) &&
        (node2 == rule_node->rule_end_node()))
    {
      return true;
    }
    else
    {
      return false;
    }
  }
}

bool
find_same_alternative_in_one_rule(
  node_t * const alternative_start,
  std::list<node_t *> * const duplicated_alternatives)
{
  assert(alternative_start != 0);
  
  node_t * const rule_node = alternative_start->rule_node();
  assert(rule_node != 0);
  
  bool find = false;
  
  BOOST_FOREACH(node_t * const prev_alternative_start, rule_node->next_nodes())
  {
    if (prev_alternative_start != alternative_start)
    {
      if (true == check_two_alternatives_are_equal_in_one_rule(
            prev_alternative_start,
            alternative_start))
      {
        find = true;
        
        if (duplicated_alternatives != 0)
        {
          duplicated_alternatives->push_back(prev_alternative_start);
        }
      }
    }
  }
  
  return find;
}

bool
find_same_alternative_in_one_rule(
  node_t * const alternative_start)
{
  assert(alternative_start != 0);
  
  return find_same_alternative_in_one_rule(alternative_start, 0);
}

void
remove_same_alternative_for_one_rule(
  analyser_environment_t * const ae,
  node_t * const rule_node)
{
  assert(rule_node != 0);
  
  for (std::list<node_t *>::reverse_iterator iter = rule_node->next_nodes().rbegin();
       iter != rule_node->next_nodes().rend();
       ++iter)
  {
    std::list<node_t *> duplicated_alternatives;
    
    if (true == find_same_alternative_in_one_rule(*iter, &duplicated_alternatives))
    {
      // because the 'duplicated_alternatives' should not
      // contain the alternative pointed by current 'iter',
      // thus after removing, 'iter' would still be valid.
      remove_alternatives(ae, duplicated_alternatives, false, true, true);
    }
  }
}

/// \brief duplicate an alternative
///
/// Duplicate an alternative under the same rule.
/// 
/// Ex:
///
/// A -> B C D A_END
///
/// I want to duplicate alternative (B C D). The programmers
/// can use
///
/// fork_alternative('B', 'A', 'A_END', 0, true|false);
///
/// to accomplish it. The result will be
///
/// A -> B C D A_END
///      B C D
///
/// \param alternative_start 
/// \param target_node 
/// \param target_end_node 
/// \param discard_node During the duplicating, callers can
/// decide which nodes within the original alternative
/// should be be copied. Discard nodes group should not
/// include the rule node and the rule end node.
/// \param after_link_nonterminal True if this function be
/// called after the nonterminal linking stage.
///
/// \return true if a new non-empty alternative has been
/// created, false if no new non-empty alternative created
/// (Ex: \p discard_nodes contains all the nodes of this
/// alternative, and this will create an empty alternative,
/// that is to say, an epsilon alternative.).
///
bool
fork_alternative(
  node_t * const alternative_start,
  node_t * const target_node,
  node_t * const target_end_node,
  std::list<node_t *> const &discard_nodes,
  bool const after_link_nonterminal)
{
  assert(alternative_start != 0);
  assert(target_node != 0);
  
  assert(1 == alternative_start->prev_nodes().size());
  assert(1 == alternative_start->next_nodes().size());
  
#if defined(_DEBUG)
  BOOST_FOREACH(node_t * const node,
                discard_nodes)
  {
    assert(1 == node->prev_nodes().size());
    assert(1 == node->next_nodes().size());
  }
#endif
  
  node_t *node_in_orig_alternative = alternative_start;
  node_t *curr_target_node = target_node;
  
  assert(curr_target_node != 0);
  
  bool create_new_node = false;
  
  while (node_in_orig_alternative !=
         alternative_start->rule_node()->rule_end_node())
  {
    assert(1 == node_in_orig_alternative->prev_nodes().size());
    assert((1 == node_in_orig_alternative->next_nodes().size()) ||
           (0 == node_in_orig_alternative->next_nodes().size()));
    
    bool proceed_copy = true;
    
    BOOST_FOREACH(node_t * const discard_node,
                  discard_nodes)
    {
      if (node_in_orig_alternative == discard_node)
      {
        node_in_orig_alternative = node_in_orig_alternative->next_nodes().front();
        proceed_copy = false;
        break;
      }
    }
    
    if (false == proceed_copy)
    {
      continue;
    }
    else
    {
      // create a new node according to the original one.
      node_t * const new_node = new node_t(node_in_orig_alternative);
      assert(new_node != 0);
      
      create_new_node = true;
      
      new_node->regex_info() = node_in_orig_alternative->regex_info();
      new_node->tmp_regex_info() = node_in_orig_alternative->tmp_regex_info();
      
      // link these 2 nodes
      {
        assert(0 == new_node->corresponding_node_between_fork().size());
        
        new_node->corresponding_node_between_fork().push_back(node_in_orig_alternative);
        node_in_orig_alternative->corresponding_node_between_fork().push_back(new_node);
      }
      
      {
        new_node->brother_node_to_update_regex() = node_in_orig_alternative;
        node_in_orig_alternative->brother_node_to_update_regex() = new_node;
      }
      
      new_node->set_rule_node(node_in_orig_alternative->rule_node());
      if (false == new_node->is_terminal())
      {
        // I am handling a non-terminal node.
        if (true == after_link_nonterminal)
        {
          assert(new_node->nonterminal_rule_node() != 0);
          
          new_node->nonterminal_rule_node()->add_refer_to_me_node(new_node);
        }
        else
        {
          assert(0 == new_node->nonterminal_rule_node());
        }
      }
      
      curr_target_node->append_node(new_node);
      new_node->prepend_node(curr_target_node);
      
      curr_target_node = new_node;
      
      node_in_orig_alternative = node_in_orig_alternative->next_nodes().front();
    }
  }
  
  // If the 'discard_nodes' contains _ALL_ nodes in this
  // alternative, then the forking this time will be
  // useless, thus the 'curr_target_node' is already linked
  // with the 'target_end_node'. Thus, I have to check this
  // out first.
  if (false == target_end_node->is_my_prev_node(curr_target_node))
  {
    assert(false == curr_target_node->is_my_next_node(target_end_node));
    
    curr_target_node->append_node(target_end_node);
    target_end_node->prepend_node(curr_target_node);
  }
  else
  {
    assert(true == curr_target_node->is_my_next_node(target_end_node));
  }
  
  return create_new_node;
}

/// 
///
/// \param ae 
/// \param alternatives 
/// \param call_in_ae_destructor 
/// \param after_link_nonterminal 
/// \param still_in_rule_node_next_nodes_group If the
/// starting node of this alternative (ie. alternative start
/// node) is still in the 'next_nodes' group of its rule
/// node?
///
void
remove_alternatives(
  analyser_environment_t * const ae,
  std::list<node_t *> &alternatives,
  bool const call_in_ae_destructor,
  bool const after_link_nonterminal,
  bool const still_in_rule_node_next_nodes_group)
{
  BOOST_FOREACH(node_t * const alternative_start, alternatives)
  {
    node_t *curr_node = alternative_start;
    
    assert(false == curr_node->is_rule_head());
    
    if (curr_node->name().size() != 0)
    {
      if (true == still_in_rule_node_next_nodes_group)
      {
        assert(1 == curr_node->prev_nodes().size());
        assert(curr_node->prev_nodes().front() == curr_node->rule_node());
      }
      else
      {
        assert(0 == curr_node->prev_nodes().size());
      }
    }
    
    if (false == call_in_ae_destructor)
    {
      ae->log(L"<INFO>: remove duplicated alternative: %s[%d] from %s[%d]\n",
              curr_node->name().c_str(),
              curr_node->overall_idx(),
              curr_node->rule_node()->name().c_str(),
              curr_node->rule_node()->overall_idx());
    }
    
    if (true == still_in_rule_node_next_nodes_group)
    {
      curr_node->rule_node()->break_append_node(curr_node);
      
      // In case of empty alternative.
      //
      // Ex:
      //
      // A -> ...epsilon...
      //
      // In this case, I have to remove the linkage from
      // rule_end_node to rule_node here.
      if (0 == curr_node->name().size())
      {
        assert(curr_node == curr_node->rule_node()->rule_end_node());
        
        curr_node->break_prepend_node(curr_node->rule_node());
      }
    }
    
    // Look each node in this alternative.
    while (curr_node != curr_node->rule_node()->rule_end_node())
    {
      if (false == call_in_ae_destructor)
      {
        if (true == after_link_nonterminal)
        {
          if (false == curr_node->is_terminal())
          {
            assert(curr_node->nonterminal_rule_node() != 0);
            curr_node->nonterminal_rule_node()->remove_refer_to_me_node(curr_node);
          }
        }
      }
      
      node_t * const tmp = curr_node;
      assert(1 == curr_node->next_nodes().size());
      curr_node = curr_node->next_nodes().front();
      
      if (true == still_in_rule_node_next_nodes_group)
      {
        if (curr_node == curr_node->rule_node()->rule_end_node())
        {
          curr_node->rule_node()->rule_end_node()->break_prepend_node(tmp);
        }
      }
      
      if (false == call_in_ae_destructor)
      {
        assert(tmp->alternative_start() != 0);
        
        if (tmp->rule_node()->main_regex_alternative() != 0)
        {
          // I should not remove 'main_regex_alternative'.
          assert(tmp->alternative_start() != tmp->rule_node()->main_regex_alternative());
          
          tmp->corresponding_node_in_main_regex_alternative()->
            refer_to_me_nodes_in_regex_expansion_alternatives().remove(tmp);
        }
      }
      
      boost::checked_delete(tmp);
    }
  }
}

void
remove_duplicated_alternatives_for_one_rule(
  analyser_environment_t * const ae,
  node_t * const node)
{
  assert(node != 0);
  assert(true == node->is_rule_head());
  
  for (std::list<node_t *>::iterator iter = node->next_nodes().begin();
       iter != node->next_nodes().end();
       ++iter)
  {
    std::list<node_t *> duplicated_alternatives;
    
    if (true == find_same_alternative_in_one_rule(*iter, &duplicated_alternatives))
    {
      assert(duplicated_alternatives.size() != 0);
      remove_alternatives(ae, duplicated_alternatives, false, true, true);
    }
    else
    {
      assert(0 == duplicated_alternatives.size());
    }
  }
}

void
remove_duplicated_alternatives_for_selected_alternative(
  analyser_environment_t * const ae,
  node_t * const node)
{
  assert(false == node->is_rule_head());
  
  std::list<node_t *> duplicated_alternatives;
  
  if (true == find_same_alternative_in_one_rule(node, &duplicated_alternatives))
  {
    assert(duplicated_alternatives.size() != 0);
    remove_alternatives(ae, duplicated_alternatives, false, true, true);
  }
  else
  {
    assert(0 == duplicated_alternatives.size());
  }
}

void
restore_regex_info_for_one_alternative(
  node_t * const node)
{
  // Because this function would be used by
  // 'main_regex_alternative' of RULE_CONTAIN_REGEX_OR
  // rule. Thus, the 'node->prev_nodes().size()' could be
  // 0.
  if (1 == node->prev_nodes().size())
  {
    assert(node->prev_nodes().front() == node->rule_node());
  }
  else
  {
    assert(0 == node->prev_nodes().size());
  }
  
  assert(node->alternative_start() == node);
  
  node_t *curr_node = node;
  
  while (curr_node->name().size() != 0)
  {
    curr_node->restore_regex_info();
    
    assert(1 == curr_node->next_nodes().size());
    curr_node = curr_node->next_nodes().front();
  }
}

namespace
{
  bool
  check_if_corresponding_node_between_fork_size_equal_one(
    node_t * const node)
  {
    assert(1 == node->corresponding_node_between_fork().size());
    
    return true;
  }
}

///
/// \brief duplicate an alternative and duplicate some nodes
/// within it. 
///
/// Ex:
///
/// Copy the following alternative and duplicate nodes
/// between B & D, and attach to the latest 'D'.
///
/// A B C D
///
/// becomes
///
/// A B C D B C D
///         +---+
///          new
///
/// \param alternative_start the starting node of the been
/// duplicated alternative. 
/// \param dup_range_start the starting node of the
/// duplicated node range 
/// \param dup_range_end the end node of the duplicated node
/// range 
/// \param attached_pos the node position which the
/// duplicated nodes should be attached to.
void
fork_alternative_with_some_duplicated_nodes(
  node_t * const alternative_start,
  node_t * const dup_range_start,
  node_t * const dup_range_end,
  node_t * const attached_pos,
  std::list<node_t *> &new_dup_nodes)
{
  // duplicate this alternative first.
  fork_alternative(alternative_start,
                   alternative_start->rule_node(),
                   alternative_start->rule_node()->rule_end_node(),
                   std::list<node_t *>(),
                   false);
  
  // The newly created alternative should be the last
  // one.
  node_t * const new_alternative_start_node =
    alternative_start->rule_node()->next_nodes().back();
  
  update_regex_info_for_one_alternative(new_alternative_start_node);
  
  (void)node_for_each(new_alternative_start_node,
                      0,
                      check_if_regex_info_is_correct_for_one_node);
  
  (void)node_for_each(new_alternative_start_node,
                      0,
                      check_if_corresponding_node_between_fork_size_equal_one);
  
  node_t * duplicate_source_node = dup_range_start;
  
  assert(1 == dup_range_end->next_nodes().size());
  node_t * const real_dup_range_end = dup_range_end->next_nodes().front();
  
  // Ex:
  //
  // Copy the following alternative and duplicate nodes
  // between B & D. 
  //
  // A B C D E
  //   -----
  //
  // becomes
  //
  // A B C D B C D E
  //   ----- -----
  //       ^       ^
  //
  // Then the 'attach_pos_for_duplicated_node' will be 'D'
  // (pointed by the first '^'), and the
  // 'attach_pos_for_duplicated_node_end' will be 'E'
  // (pointed by the second '^'). 
  //
  // :NOTE:
  //
  // The newest 'corresponding_node_between_fork' of
  // 'attached_pos' node is the last one in the
  // 'corresponding_node_between_fork' field.
  node_t * attach_pos_for_duplicated_node =
    attached_pos->corresponding_node_between_fork().back();
  
  assert(1 == attach_pos_for_duplicated_node->next_nodes().size());
  
  node_t * const attach_pos_end_for_duplicated_node =
    attach_pos_for_duplicated_node->next_nodes().front();
  
  // Ex:
  //
  // A : (a)*
  //
  // will become:
  //
  // A : a | a a | ...epsilon...
  //           ^
  //
  // In the 2nd alternative, because the 2nd 'a' would be
  // added between the 1st 'a' and the rule end node, I have
  // to add the following check.
  if (attach_pos_end_for_duplicated_node->name().size() != 0)
  {
    assert(1 == attach_pos_end_for_duplicated_node->prev_nodes().size());
  }
  
  attach_pos_for_duplicated_node->break_append_node(attach_pos_end_for_duplicated_node);
  attach_pos_end_for_duplicated_node->break_prepend_node(attach_pos_for_duplicated_node);
  
  while (duplicate_source_node != real_dup_range_end)
  {
    node_t * const new_node = new node_t(duplicate_source_node);
    assert(new_node != 0);
    
    new_dup_nodes.push_back(new_node);
    
    // The result I want to achieve is:
    //
    // Ex:
    //
    // ((a)* b)*
    //  ^ ^
    // will become:
    // 
    // ((a)* (a)* b)*
    //  ^ ^
    // duplicate the first 'a' twice. Hence, I have to
    // copied the whole 'regex_info', and the first one of
    // the 'tmp_regex_info', because it is the current
    // processing regex_info (ie, the left/right parenthesis
    // pointed by '^')
    new_node->regex_info() = duplicate_source_node->regex_info();
    if (duplicate_source_node->tmp_regex_info().size() != 0)
    {
      new_node->tmp_regex_info().push_front(
        duplicate_source_node->tmp_regex_info().front());
    }
    
    new_node->set_rule_node(duplicate_source_node->rule_node());
    
    {
      assert(0 == new_node->corresponding_node_between_fork().size());
      
      new_node->corresponding_node_between_fork().push_back(duplicate_source_node);
      duplicate_source_node->corresponding_node_between_fork().push_back(new_node);
    }
    
    {
      new_node->brother_node_to_update_regex() = duplicate_source_node;
      duplicate_source_node->brother_node_to_update_regex() = new_node;
    }
    
    // link these 2 nodes
    attach_pos_for_duplicated_node->append_node(new_node);
    new_node->prepend_node(attach_pos_for_duplicated_node);
    
    attach_pos_for_duplicated_node = new_node;
    
    assert(1 == duplicate_source_node->next_nodes().size());
    duplicate_source_node = duplicate_source_node->next_nodes().front();
  }
  
  attach_pos_for_duplicated_node->append_node(attach_pos_end_for_duplicated_node);
  attach_pos_end_for_duplicated_node->prepend_node(attach_pos_for_duplicated_node);
  
  update_regex_info_for_node_range(new_dup_nodes.front(),
                                   new_dup_nodes.back());
    
  (void)node_for_each(new_dup_nodes.front(),
                      new_dup_nodes.back(),
                      check_if_regex_info_is_correct_for_one_node);
  
  (void)node_for_each(new_alternative_start_node,
                      0,
                      check_if_corresponding_node_between_fork_size_equal_one);
}

unsigned int
count_alternative_length(
  node_t * const alternative_start)
{
  assert(alternative_start != 0);
  assert(alternative_start->alternative_start() == alternative_start);
  
  unsigned int count = 0;
  
  node_t *curr_node = alternative_start;
  
  while (curr_node->name().size() != 0)
  {
    ++count;
    
    assert(1 == curr_node->next_nodes().size());
    curr_node = curr_node->next_nodes().front();
  }
  
  return count;
}

node_t *
find_alternative_end(node_t * const node)
{
  assert(node != 0);
  assert(node->name().size() != 0);
  assert(1 == node->next_nodes().size());
  
  node_t *curr_node = node;
  
  while (curr_node->next_nodes().front()->name().size() != 0)
  {
    curr_node = curr_node->next_nodes().front();
  }
  
  return curr_node;
}
