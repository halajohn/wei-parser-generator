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
#include "stack_elem_for_left_recursion_detection.hpp"

#include "wcl_memory_debugger\memory_debugger.h"

namespace
{
  /// copy all the subtrees of 'node2' to 'target_rule_node'.
  void
  copy_subtree_to_node(node_t * const target_rule_node,
                       node_t * const node1,
                       node_t * const node2)
  {
    for (std::list<node_t *>::const_iterator iter = node2->next_nodes().begin();
         iter != node2->next_nodes().end();
         ++iter)
    {
      if ((*iter) == (*iter)->rule_node()->rule_end_node())
      {
        target_rule_node->add_temp_prepend_to_end_node(node1);
        return;
      }
      else
      {
        node_t * const node = new node_t(*iter);
        assert(node != 0);
        
        node->set_rule_node(node1->rule_node());
        if (false == (*iter)->is_terminal())
        {
          assert((*iter)->nonterminal_rule_node() != 0);
          (*iter)->nonterminal_rule_node()->add_refer_to_me_node(node);
        }
        
        if (target_rule_node == node1)
        {
          target_rule_node->add_temp_append_to_rule_node(node);
        }
        else
        {
          node1->append_node(node);
          node->prepend_node(node1);
        }
        
        copy_subtree_to_node(target_rule_node, node, *iter);
      }
    }
  }
}

void
analyser_environment_t::replace_first_smaller_index_nonterminal(
  node_t * const rule_node,
  node_t * const target_rule_node)
{
  assert(true == rule_node->is_rule_head());
  assert(true == target_rule_node->is_rule_head());
  
  for (std::list<node_t *>::const_iterator iter = rule_node->next_nodes().begin();
       iter != rule_node->next_nodes().end();
       )
  {
    rule_node->clear_temp_append_to_rule_node();
    rule_node->clear_temp_prepend_to_end_node();
    
    if ((*iter)->nonterminal_rule_node() == target_rule_node)
    {
      copy_subtree_to_node(rule_node, rule_node, target_rule_node);
      
      assert(rule_node->temp_append_to_rule_node().size() ==
             rule_node->temp_prepend_to_end_node().size());
      
      for (std::list<node_t *>::const_iterator iter2 =
             rule_node->temp_append_to_rule_node().begin();
           iter2 != rule_node->temp_append_to_rule_node().end();
           ++iter2)
      {
        rule_node->append_node(*iter2);
        (*iter2)->prepend_node(rule_node);
      }
      for (std::list<node_t *>::const_iterator iter2 =
             rule_node->temp_prepend_to_end_node().begin();
           iter2 != rule_node->temp_prepend_to_end_node().end();
           ++iter2)
      {
        node_t *copy_node_attached_node = *iter2;
        assert(1 == (*iter)->next_nodes().size());
        node_t *curr_node = (*iter)->next_nodes().front();
        while (false == curr_node->name().empty())
        {
          node_t * const copy_node = new node_t(curr_node);
          assert(copy_node != 0);
          
          copy_node->set_rule_node(curr_node->rule_node());
          if (false == curr_node->is_terminal())
          {
            assert(curr_node->nonterminal_rule_node() != 0);
            curr_node->nonterminal_rule_node()->add_refer_to_me_node(copy_node);
          }
          copy_node_attached_node->append_node(copy_node);
          copy_node->prepend_node(copy_node_attached_node);
          
          assert(1 == curr_node->next_nodes().size());
          copy_node_attached_node = copy_node;
          curr_node = curr_node->next_nodes().front();
        }
        copy_node_attached_node->append_node(rule_node->rule_end_node());
        rule_node->rule_end_node()->prepend_node(copy_node_attached_node);
      }
      // remove this alternative.
      node_t *curr_node = (*iter);
      while (curr_node->next_nodes().front() != rule_node->rule_end_node())
      {
        curr_node = curr_node->next_nodes().front();
      }
      rule_node->rule_end_node()->break_prepend_node(curr_node);
      while (curr_node != (*iter))
      {
        if (false == curr_node->is_terminal())
        {
          curr_node->nonterminal_rule_node()->remove_refer_to_me_node(curr_node);
        }
        node_t *del = curr_node;
        assert(1 == curr_node->prev_nodes().size());
        curr_node = curr_node->prev_nodes().front();
        boost::checked_delete(del);
      }
      // for (*iter) node.
      if (false == curr_node->is_terminal())
      {
        curr_node->nonterminal_rule_node()->remove_refer_to_me_node(curr_node);
      }
      iter = rule_node->break_append_node(curr_node);
      boost::checked_delete(curr_node);
    }
    else
    {
      ++iter;
    }
  }
}

void
analyser_environment_t::remove_left_recur()
{
  assert(m_top_level_nodes.size() != 0);
  
  remove_immediate_left_recursion(m_top_level_nodes.front());
  
#if 0
  // dump tree.
  std::wstringstream ss;
  std::wstring message, tmp;
  ss.str(L"");
  ss.clear();
  ss << idx;
  ss >> tmp;
  message.append(L"8-");
  message.append(tmp);
  message.append(L"_remove_immediate_recursion_for_");
  message.append(m_top_level_nodes.front()->name());
  message.append(L".dot");
  dump_tree(message);
  message.clear();
  ++idx;
#endif
  
  std::list<node_t *>::const_iterator iter = m_top_level_nodes.begin();
  ++iter;
  
  for (; iter != m_top_level_nodes.end(); ++iter)
  {
    std::wstring const apostrophe_str = L"_apostrophe";
    
    // If this alternative is one of the newly created apostrophe nodes,
    // then skip it.
    if (((*iter)->name().size() >= apostrophe_str.size()) &&
        (0 == (*iter)->name().compare((*iter)->name().size() - apostrophe_str.size(),
                                      apostrophe_str.size(),
                                      apostrophe_str)))
    {
      continue;
    }
    else
    {
      for (std::list<node_t *>::const_iterator iter2 = m_top_level_nodes.begin();
           iter2 != iter;
           ++iter2)
      {
        replace_first_smaller_index_nonterminal(*iter, *iter2);
      }
      
      assert(true == traverse_all_nodes(check_not_cyclic, 0, 0));
      
#if 0
      // dump tree.
      ss.str(L"");
      ss.clear();
      ss << idx;
      ss >> tmp;
      message.append(L"8-");
      message.append(tmp);
      message.append(L"_replace_nonterminal_");
      message.append((*iter)->name());
      message.append(L"_first_symbol.dot");
      dump_tree(message);
      message.clear();
      ++idx;
#endif
      
      remove_immediate_left_recursion(*iter);
      
#if 0
      // dump tree.
      ss.str(L"");
      ss.clear();
      ss << idx;
      ss >> tmp;
      message.append(L"8-");
      message.append(tmp);
      message.append(L"_remove_immediate_recursion_for_");
      message.append((*iter)->name());
      message.append(L".dot");
      dump_tree(message);
      message.clear();
      ++idx;
#endif
    }
  }
}

namespace
{
  void
  remove_immediate_left_recursion_append_apostrophe(node_t * const rule_node,
                                                    node_t * const apostrophe_node)
  {
    rule_node->clear_temp_append_to_rule_node();
    rule_node->clear_temp_prepend_to_end_node();
    
    /// copy the original subtrees before appending
    /// _apostrophe node for each alternative. 
    copy_subtree_to_node(rule_node, rule_node, rule_node);
    
    /// append apostrophe node after each alternative of the origianl node.
    for (std::list<node_t *>::const_iterator iter = rule_node->next_nodes().begin();
         iter != rule_node->next_nodes().end();
         ++iter)
    {
      /// find the last node of this alternative.
      node_t * const curr_node = (*iter)->find_alternative_last_node();
      node_t * const new_node = new node_t(apostrophe_node);
      assert(new_node != 0);
      
      new_node->set_is_rule_head(false);
      new_node->set_rule_node(rule_node);
      new_node->set_nonterminal_rule_node(apostrophe_node);
      assert(false == new_node->is_terminal());
      apostrophe_node->add_refer_to_me_node(new_node);
      
      curr_node->break_append_node(rule_node->rule_end_node());
      rule_node->rule_end_node()->break_prepend_node(curr_node);
      
      curr_node->append_node(new_node);
      new_node->prepend_node(curr_node);
      
      new_node->append_node(rule_node->rule_end_node());
      rule_node->rule_end_node()->prepend_node(new_node);
    }
    for (std::list<node_t *>::const_iterator iter =
           rule_node->temp_append_to_rule_node().begin();
         iter != rule_node->temp_append_to_rule_node().end();
         ++iter)
    {
      rule_node->append_node(*iter);
      (*iter)->prepend_node(rule_node);
    }
    for (std::list<node_t *>::const_iterator iter =
           rule_node->temp_prepend_to_end_node().begin();
         iter != rule_node->temp_prepend_to_end_node().end();
         ++iter)
    {
      rule_node->rule_end_node()->prepend_node(*iter);
      (*iter)->append_node(rule_node->rule_end_node());
    }
  }
}

/// 
///
/// Let \f$A\f$ -> \f$A\alpha_1\f$ | ... | \f$A\alpha_r\f$
/// be the set of all directly left recursive A-productions,
/// and let \f$A\f$ -> \f$\beta_1\f$ | ... | \f$\beta_s\f$
/// be the remaining A-productions. Replace all these
/// productions with \f$A\f$ -> \f$\beta_1\f$ |
/// \f$\beta_1A^{'}\f$ | ... | \f$\beta_s\f$ |
/// \f$\beta_sA^{'}\f$, and \f$A^{'}\f$ -> \f$\alpha_1\f$ |
/// \f$\alpha_1A^{'}\f$ | ... | \f$\alpha_s\f$ |
/// \f$\alpha_sA^{'}\f$,
/// where \f$A^{'}\f$ is a new nonterminal not used
/// elsewhere in the grammar. 
///
/// @param rule_node 
///
void
analyser_environment_t::remove_immediate_left_recursion(
  node_t * const rule_node)
{
  /// walk all the alternative nodes of this rule node.
  /// If this node has immediate left recursion, then create
  /// a new nonterminal node with "_apostrophe" and move the
  /// immediate left recursive alternative to the new node. 
  ///
  for (std::list<node_t *>::const_iterator iter =
         rule_node->next_nodes().begin();
       iter != rule_node->next_nodes().end();
       )
  {
    if ((false == (*iter)->is_terminal()) &&
        ((*iter)->nonterminal_rule_node() == rule_node))
    {
      if (0 == rule_node->apostrophe_node())
      {
        /// create new nonterminal node with "_apostrophe".
        std::wstring apostrophe_name = rule_node->name();
        apostrophe_name.append(L"_apostrophe");
        
        node_t * const apostrophe_node =
          new node_t(this, 0, apostrophe_name);
        assert(apostrophe_node != 0);
        
        node_t * const rule_end_node =
          new node_t(this, apostrophe_node);
        assert(rule_end_node != 0);
        
        apostrophe_node->set_is_rule_head(true);
        apostrophe_node->set_rule_end_node(rule_end_node);
        
        rule_node->set_apostrophe_node(apostrophe_node);
        apostrophe_node->set_apostrophe_orig_node(rule_node);
        
        add_top_level_nodes(apostrophe_node);
        hash_rule_head(apostrophe_node);
      }
      
      // find the last node of this alternative.
      node_t *curr_node = (*iter);
      assert(curr_node->rule_node() == rule_node);
      assert(1 == curr_node->next_nodes().size());
      while (curr_node->next_nodes().front() != rule_node->rule_end_node())
      {
        assert(curr_node->rule_node() == rule_node);
        assert(1 == curr_node->next_nodes().size());
        
        // change the rule node for it.
        curr_node->set_rule_node(rule_node->apostrophe_node());
        
        curr_node = curr_node->next_nodes().front();
      }
      // change the rule node for the last node.
      curr_node->set_rule_node(rule_node->apostrophe_node());
      
      assert(1 == curr_node->next_nodes().size());
      assert(curr_node->next_nodes().front() == rule_node->rule_end_node());
      
      // move immediate left recursion candidate nodes to new apostrophe node.
      node_t * const alternative_start = (*iter);
      
      rule_node->rule_end_node()->break_prepend_node(curr_node);
      curr_node->break_append_node(rule_node->rule_end_node());
      
      (*iter)->break_prepend_node(rule_node);
      iter = rule_node->break_append_node(*iter);
      
      assert(1 == alternative_start->next_nodes().size());
      rule_node->apostrophe_node()->append_node(alternative_start->next_nodes().front());
      alternative_start->next_nodes().front()->break_prepend_node(alternative_start);
      alternative_start->next_nodes().front()->prepend_node(rule_node->apostrophe_node());
      
      curr_node->append_node(rule_node->apostrophe_node()->rule_end_node());
      rule_node->apostrophe_node()->rule_end_node()->prepend_node(curr_node);
      
      assert(false == alternative_start->is_terminal());
      assert(alternative_start->nonterminal_rule_node() == rule_node);
      rule_node->remove_refer_to_me_node(alternative_start);
      
      boost::checked_delete(alternative_start);
    }
    else
    {
      /// This is a \beta node, thus I don't need to process it.
      ++iter;
    }
  }
  
  if (rule_node->apostrophe_node() != 0)
  {
    assert(rule_node->apostrophe_node()->is_rule_head());
    
    remove_immediate_left_recursion_append_apostrophe(rule_node,
                                                      rule_node->apostrophe_node());
    remove_immediate_left_recursion_append_apostrophe(rule_node->apostrophe_node(),
                                                      rule_node->apostrophe_node());
  }
}

namespace
{
  bool
  find_left_recursion_in_stack(
    node_t * const node,
    std::list<stack_elem_for_left_recursion_detection> &stack,
    std::list<std::list<node_t *> > &left_recursion_set)
  {
    bool find_a_left_recursion = false;
    
    for (std::list<stack_elem_for_left_recursion_detection>::const_iterator iter2 = stack.begin();
         iter2 != stack.end();
         ++iter2)
    {
      assert(true == (*iter2).mp_rule_node->is_rule_head());
      
      if ((*iter2).mp_rule_node == node->nonterminal_rule_node())
      {
        // If yes, this means:
        //
        // Ex:
        //
        // The current rule and the alternative are of
        // the form 'A->B x ...', and the stack contains
        // node 'B'.
        //
        // This means I found a left recursion.
        //
        // Ex:
        // 
        // A -> B   ------ put 'B' into the stack
        //   -> D e
        // B -> C   ------ put 'C' into the stack.
        // C -> B   ------ find a left recursion
        //                 between 'B' & 'C'
        std::list<node_t *> left_recursion;
        
        for (std::list<stack_elem_for_left_recursion_detection>::const_iterator iter3 = iter2;
             iter3 != stack.end();
             ++iter3)
        {
          left_recursion.push_back((*iter3).mp_rule_node);
        }
        
        left_recursion_set.push_back(left_recursion);
        
        find_a_left_recursion = true;
      }
    }
    
    return find_a_left_recursion;
  }
}

void
analyser_environment_t::detect_left_recursion_nonterminal_for_one_rule(
  node_t * const node,
  std::list<stack_elem_for_left_recursion_detection> &stack,
  std::list<std::list<node_t *> > &left_recursion_set)
{
  assert(true == node->is_rule_head());
  
  BOOST_FOREACH(node_t * const alternative_start, node->next_nodes())
  {
    if (0 == alternative_start->name().size())
    {
      assert(stack.size() != 0);
      
      if (0 == stack.back().mp_source_node)
      {
        assert(1 == stack.size());
        
        continue;
      }
      else
      {
        assert(1 == stack.back().mp_source_node->next_nodes().size());
        
        std::list<stack_elem_for_left_recursion_detection> tmp_stack;
        
        bool can_not_find_meaningful_node = false;
        node_t *next_meaningful_node = 0;
      
        for (;;)
        {
          if (1 == stack.size())
          {
            assert(0 == stack.back().mp_source_node);
            
            stack.splice(stack.end(), tmp_stack);
            
            can_not_find_meaningful_node = true;
            
            break;
          }
          else
          {
            tmp_stack.push_front(stack.back());
            stack.pop_back();
            
            next_meaningful_node = tmp_stack.front().mp_source_node->next_nodes().front();
            
            if (next_meaningful_node->name().size() != 0)
            {
              break;
            }
          }
        }
      
        if (true == can_not_find_meaningful_node)
        {
          continue;
        }
        
        assert(next_meaningful_node != 0);
        
        if (false == next_meaningful_node->is_terminal())
        {
          assert(next_meaningful_node->nonterminal_rule_node() != 0);
        
          bool const find_a_left_recursion = find_left_recursion_in_stack(
            next_meaningful_node, stack, left_recursion_set);
        
          if (true == find_a_left_recursion)
          {
            stack.splice(stack.end(), tmp_stack);
          
            continue;
          }
          else
          {
            stack.push_back(stack_elem_for_left_recursion_detection(
                              next_meaningful_node->nonterminal_rule_node(),
                              next_meaningful_node));
          
            detect_left_recursion_nonterminal_for_one_rule(
              next_meaningful_node->nonterminal_rule_node(),
              stack,
              left_recursion_set);
          
            assert(stack.back().mp_rule_node == next_meaningful_node->nonterminal_rule_node());
          
            stack.pop_back();
          
            stack.splice(stack.end(), tmp_stack);
          }
        }
        else
        {
          stack.splice(stack.end(), tmp_stack);
        
          continue;
        }
      }
    }
    else
    {
      // Try to find an alternative whose first node is
      // non-terminal.
      if (false == alternative_start->is_terminal())
      {
        assert(alternative_start->nonterminal_rule_node() != 0);
        
        // search the nodes in 'stack' to see if one of
        // them is the nonterminal node of the current
        // alternative start node.
        bool const find_a_left_recursion = find_left_recursion_in_stack(
          alternative_start, stack, left_recursion_set);
        
        if (true == find_a_left_recursion)
        {
          // continue to scan the next alternative of this
          // rule.
          continue;
        }
        else
        {
          // If I traverse a _new_ nonterminal alternative
          // start node, then put this node into the stack.
          //
          // Ex:
          //
          // A -> B   ------ put 'B' into the stack
          //   -> D e
          // B -> C   ------ put 'C' into the stack.
          stack.push_back(stack_elem_for_left_recursion_detection(
                            alternative_start->nonterminal_rule_node(),
                            alternative_start));
          
          detect_left_recursion_nonterminal_for_one_rule(
            alternative_start->nonterminal_rule_node(),
            stack,
            left_recursion_set);
          
          assert(stack.back().mp_rule_node == alternative_start->nonterminal_rule_node());
          
          stack.pop_back();
        }
      }
      else
      {
        continue;
      }
    }
  }
}

bool
analyser_environment_t::detect_left_recursion(
  std::list<std::list<node_t *> > &left_recursion_set)
{
  bool has_left_recursion = false;
  
  BOOST_FOREACH(node_t * const rule_node, m_top_level_nodes)
  {
    if (false == rule_node->is_left_recursion())
    {
      {
        std::list<stack_elem_for_left_recursion_detection> stack;
        
        stack.push_back(stack_elem_for_left_recursion_detection(rule_node, 0));
        
        (void)detect_left_recursion_nonterminal_for_one_rule(rule_node, stack, left_recursion_set);
      }
      
      merge_relative_set_into_final_set(left_recursion_set);
      
      if (0 == left_recursion_set.size())
      {
        // It is not a left_recursion.
      }
      else
      {
        BOOST_FOREACH(std::list<node_t *> &left_recursion, left_recursion_set)
        {
          BOOST_FOREACH(node_t * const node, left_recursion)
          {
            assert(true == node->is_rule_head());
            
            node->assign_left_recursion_set(left_recursion);
          }
        }
        
        has_left_recursion = true;
      }
    }
  }
  
  return has_left_recursion;
}
