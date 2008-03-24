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

/// \file   cyclic.cpp
/// \author Wei Hu
/// \date   
/// 
/// \brief  
/// 
/// ======================================================
///                 Eliminating cycles
/// ======================================================
///
/// A grammar has a cycle if there is a variable A s.t.
///
/// A => A
///
/// We'll call such variables cyclic.
///
/// If a grammar has no epsilon-productions, then all cycles
/// can be eliminated from G without affecting the language
/// generated.
///
/// Given a CFG G = (V, Omega, S, P), the main step in the
/// cycle elimination algorithm is to replace the set P of
/// productions with the set P* of productions obtained
/// from P by replacing 
/// 
/// 1) each production A -> B where B is cyclic
/// 2) with new productions A -> 'alpha' s.t. 'alpha' is
/// not a cyclic variable and there is a production C ->
/// 'alpha' s.t. B =*=>C.
/// 
/// The resulting CFG is G* = (V, Omega, S, P*).
///
/// Ex:
///
/// S -> X | Xb | SS
/// X -> S | a
///
/// S & X are cyclic.
///
/// will become
///
/// S -> a | Xb | SS
/// X -> Xb | SS | a

void
analyser_environment_t::detect_cyclic_nonterminal_for_one_rule(
  node_t * const node,
  std::list<node_t *> &stack,
  std::list<std::list<node_t *> > &cyclic_set)
{
  assert(true == node->is_rule_head());
  
  BOOST_FOREACH(node_t * const alternative_start, node->next_nodes())
  {
    bool find_a_cyclic = false;
    
    // Try to find an alternative like 'A->B'.
    if ((false == alternative_start->is_terminal()) &&
        (alternative_start->is_my_next_node(
          alternative_start->rule_node()->rule_end_node()
                                            )))
    {
      assert(alternative_start->nonterminal_rule_node() != 0);
      
      // search the nodes in 'stack' to see if one of
      // them is the nonterminal node of the current
      // alternative start node.
      for (std::list<node_t *>::const_iterator iter2 = stack.begin();
           iter2 != stack.end();
           ++iter2)
      {
        assert(true == (*iter2)->is_rule_head());
        
        if ((*iter2) == alternative_start->nonterminal_rule_node())
        {
          // If yes, this means:
          //
          // Ex:
          //
          // The current rule and the alternative are of
          // the form 'A->B', and the stack contains node
          // 'B'.
          //
          // This means I found a cyclic condition.
          //
          // Ex:
          // 
          // A -> B   ------ put 'B' into the stack
          //   -> D e ------ this alternative will never be
          //                 part of a cyclic, hence I will
          //                 not examine this alternative.
          // B -> C   ------ put 'C' into the stack.
          // C -> B   ------ find a cyclic condition
          //                 between 'B' & 'C'
          std::list<node_t *> cyclic;
            
          for (std::list<node_t *>::const_iterator iter3 = iter2;
               iter3 != stack.end();
               ++iter3)
          {
            cyclic.push_back(*iter3);
          }
            
          cyclic_set.push_back(cyclic);
          
          find_a_cyclic = true;
        }
      }
      
      if (true == find_a_cyclic)
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
        //   -> D e ------ this alternative will never be
        //                 part of a cyclic, hence I will
        //                 not examine this alternative.
        // B -> C   ------ put 'C' into the stack.
        stack.push_back(alternative_start->nonterminal_rule_node());
        
        detect_cyclic_nonterminal_for_one_rule(
          alternative_start->nonterminal_rule_node(),
          stack,
          cyclic_set);
          
        assert(stack.back() == alternative_start->nonterminal_rule_node());
          
        stack.pop_back();
      }
    }
  }
}

void
analyser_environment_t::merge_relative_set_into_final_set(
  std::list<std::list<node_t *> > &set)
{
  for (std::list<std::list<node_t *> >::iterator iter = set.begin();
       iter != set.end();
       ++iter)
  {
    bool find_intersection = false;
    
    std::list<std::list<node_t *> >::iterator iter2 = iter;
    
    ++iter2;
    
    for (;
         iter2 != set.end();
         )
    {
      BOOST_FOREACH(node_t * const node, *(iter))
      {
        std::list<node_t *>::iterator check_iter =
          check_if_one_node_is_belong_to_set_and_match_some_condition(
            (*iter2),
            node,
            std::vector<check_node_func>());
        
        if (check_iter == (*iter2).end())
        {
          continue;
        }
        else
        {
          (*iter2).erase(check_iter);
          
          find_intersection = true;
          
          break;
        }
      }
      
      if (true == find_intersection)
      {
        (*iter).splice((*iter).end(), (*iter2));
        iter2 = set.erase(iter2);
      }
      else
      {
        ++iter2;
      }
    }
  }
}

void
analyser_environment_t::detect_cyclic_nonterminal()
{
  BOOST_FOREACH(node_t * const rule_node, m_top_level_nodes)
  {
    if (false == rule_node->is_cyclic())
    {
      std::list<std::list<node_t *> > cyclic_set;
      std::list<node_t *> stack;
      
      stack.push_back(rule_node);
      
      (void)detect_cyclic_nonterminal_for_one_rule(rule_node, stack, cyclic_set);
      
      merge_relative_set_into_final_set(cyclic_set);
      
      if (0 == cyclic_set.size())
      {
        // It is not a cyclic nonterminal.
      }
      else
      {
        BOOST_FOREACH(std::list<node_t *> &cyclic, cyclic_set)
        {
          BOOST_FOREACH(node_t * const node, cyclic)
          {
            assert(true == node->is_rule_head());
            
            node->assign_cyclic_set(cyclic);
          }
        }
      }
    }
  }
}

namespace
{
  void
  copy_subtree_to_node(node_t * const target_rule_node,
                       std::list<node_t *> &stack,
                       node_t * const node1,
                       node_t * const node2)
  {
    BOOST_FOREACH(node_t * const node2_child, node2->next_nodes())
    {
      if (true == node2->is_rule_head())
      {
        // cyclic remove stage is after the epsilon
        // production ellimination, thus I have already
        // performed epsilon production ellimination now.
        assert(node2_child != node2->rule_end_node());
        assert(1 == node2_child->next_nodes().size());
        
        // find a special condition, for example:
        //
        // A -> x  x  B
        //         ^  ^
        //     node2  node2_child
        if ((node2_child->next_nodes().front() == node2->rule_end_node()) &&
            (false == node2_child->is_terminal()) &&
            (check_if_one_node_is_belong_to_set_and_match_some_condition
             (
               target_rule_node->cyclic_set(),
               node2_child->nonterminal_rule_node(),
               std::vector<check_node_func>()
              ) != target_rule_node->cyclic_set().end()))
        {
          if (stack.end() ==
              check_if_one_node_is_belong_to_set_and_match_some_condition
              (
                stack,
                (node2_child)->nonterminal_rule_node(),
                std::vector<check_node_func>()
               )
              )
          {
            stack.push_back((node2_child)->nonterminal_rule_node());
            
            // This is still a cyclic node, copy recursively.
            copy_subtree_to_node(target_rule_node,
                                 stack,
                                 target_rule_node,
                                 (node2_child)->nonterminal_rule_node());
            
            assert(stack.back() == (node2_child)->nonterminal_rule_node());
            
            stack.pop_back();
          }
          
          continue;
        }
      }
    
      if ((node2_child) == (node2_child)->rule_node()->rule_end_node())
      {
        target_rule_node->add_temp_prepend_to_end_node(node1);
        return;
      }
      else
      {
        node_t * const node = new node_t(node2_child);
        assert(node != 0);
      
        node->set_rule_node(node1->rule_node());
        if (false == (node2_child)->is_terminal())
        {
          assert((node2_child)->nonterminal_rule_node() != 0);
          (node2_child)->nonterminal_rule_node()->add_refer_to_me_node(node);
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
      
        copy_subtree_to_node(target_rule_node, stack, node, node2_child);
      }
    }
  }
}

void
analyser_environment_t::remove_cyclic()
{
  for (std::list<node_t *>::const_iterator iter = m_top_level_nodes.begin();
       iter != m_top_level_nodes.end();
       ++iter)
  {
    for (std::list<node_t *>::const_iterator iter2 = (*iter)->next_nodes().begin();
         iter2 != (*iter)->next_nodes().end();
         ++iter2)
    {
      bool replace_cyclic = false;
      
      if ((false == (*iter2)->is_terminal()) &&
          ((*iter2)->is_my_next_node((*iter)->rule_end_node())))
      {
        assert((*iter2)->name().compare((*iter)->name()) != 0);
        
        for (std::list<node_t *>::const_iterator iter3 =
               (*iter)->cyclic_set().begin();
             iter3 != (*iter)->cyclic_set().end();
             ++iter3)
        {
          assert((*iter3)->is_rule_head());
          if ((*iter3) == (*iter2)->nonterminal_rule_node())
          {
            assert(0 == (*iter3)->name().compare((*iter2)->name()));
            replace_cyclic = true;
            break;
          }
        }
        
        if (true == replace_cyclic)
        {
          std::list<node_t *> stack;
          stack.push_back(*iter);
		  stack.push_back((*iter2)->nonterminal_rule_node());
          copy_subtree_to_node(*iter, stack, *iter, (*iter2)->nonterminal_rule_node());
        }
      }
    }
  }
  
  for (std::list<node_t *>::iterator iter = m_top_level_nodes.begin();
       iter != m_top_level_nodes.end();
       ++iter)
  {
    assert((*iter)->temp_append_to_rule_node().size() ==
           (*iter)->temp_prepend_to_end_node().size());
    for (std::list<node_t *>::const_iterator iter2 =
           (*iter)->temp_append_to_rule_node().begin();
         iter2 != (*iter)->temp_append_to_rule_node().end();
         ++iter2)
    {
      (*iter)->append_node(*iter2);
      (*iter2)->prepend_node(*iter);
    }
    for (std::list<node_t *>::const_iterator iter2 =
           (*iter)->temp_prepend_to_end_node().begin();
         iter2 != (*iter)->temp_prepend_to_end_node().end();
         ++iter2)
    {
      (*iter)->rule_end_node()->prepend_node(*iter2);
      (*iter2)->append_node((*iter)->rule_end_node());
    }
    
    for (std::list<node_t *>::const_iterator iter2 = (*iter)->next_nodes().begin();
         iter2 != (*iter)->next_nodes().end();
         )
    {
      if (((*iter2)->next_nodes().front() == (*iter)->rule_end_node()) &&
          (false == (*iter2)->is_terminal()) &&
          (check_if_one_node_is_belong_to_set_and_match_some_condition(
            (*iter)->cyclic_set(),
            (*iter2)->nonterminal_rule_node(),
            std::vector<check_node_func>()) != (*iter)->cyclic_set().end()
           ))
      {
        assert((*iter2)->nonterminal_rule_node() != (*iter));
        
        (*iter2)->nonterminal_rule_node()->remove_refer_to_me_node(*iter2);
        (*iter)->rule_end_node()->break_prepend_node(*iter2);
        node_t *del = *iter2;
        iter2 = (*iter)->break_append_node(*iter2);
        boost::checked_delete(del);
      }
      else
      {
        ++iter2;
      }
    }
  }
  
  // Remove rule nodes which doesn't have any child after
  // removing cyclic.
  for (std::list<node_t *>::iterator iter = m_top_level_nodes.begin();
       iter != m_top_level_nodes.end();
       )
  {
    if (0 == (*iter)->next_nodes().size())
    {
      // Because this rule node doesn't have any child,
      // thus there should no any other nodes refer to this
      // rule node, otherwise, this should be a bug in the
      // above removing cyclic codes.
      assert(0 == (*iter)->refer_to_me_nodes().size());
      
      node_t *del = (*iter);
      iter = m_top_level_nodes.erase(iter);
      boost::checked_delete(del);
    }
    else
    {
      ++iter;
    }
  }
}

void
analyser_environment_t::remove_direct_cyclic()
{
  for (std::list<node_t *>::const_iterator iter = m_top_level_nodes.begin();
       iter != m_top_level_nodes.end();
       ++iter)
  {
    for (std::list<node_t *>::const_iterator iter2 = (*iter)->next_nodes().begin();
         iter2 != (*iter)->next_nodes().end();
         )
    {
      assert((*iter2)->rule_node() == (*iter));
      if ((false == (*iter2)->is_terminal()) &&
          (*iter2)->is_my_next_node((*iter)->rule_end_node()) &&
          ((*iter2)->nonterminal_rule_node() == (*iter)))
      {
        (*iter)->rule_end_node()->break_prepend_node(*iter2);
        (*iter)->remove_refer_to_me_node(*iter2);
        boost::checked_delete(*iter2);
        iter2 = (*iter)->break_append_node(*iter2);
      }
      else
      {
        ++iter2;
      }
    }
  }
  
  // Remove rule nodes which doesn't have any child after
  // removing cyclic.
  for (std::list<node_t *>::iterator iter = m_top_level_nodes.begin();
       iter != m_top_level_nodes.end();
       )
  {
    if (0 == (*iter)->next_nodes().size())
    {
      // Because this rule node doesn't have any child,
      // thus there should no any other nodes refer to this
      // rule node, otherwise, this should be a bug in the
      // above removing cyclic codes.
      assert(0 == (*iter)->refer_to_me_nodes().size());
      
      node_t *del = (*iter);
      iter = m_top_level_nodes.erase(iter);
      boost::checked_delete(del);
    }
    else
    {
      ++iter;
    }
  }
}
