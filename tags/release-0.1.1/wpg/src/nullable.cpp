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

#include "wcl_memory_debugger\memory_debugger.h"

/// \file   nullable.cpp
/// \author Wei Hu
/// \date   
/// 
/// \brief  
/// 
/// ======================================================
///            Eliminating epsilon-productions
/// ======================================================
///
/// Given a CFG G = (V, Omaga, S, P), a variable A belongs
/// to V is nullable if 
/// 
///   A =*=> 'epsilon'.
/// 
/// The main step in the epsilon-production elimination
/// algorithm then is that the set P of productions is
/// replaced with the set P* of all productions
/// 
///   A -> 'beta'
/// 
/// s.t. A != 'beta', 'beta' != 'epsilon', and P includes a
/// production 
/// 
///   A -> 'alpha'
/// 
/// s.t. 'beta' can be obtained from 'alpha' by deleting
/// zero or more occurrences of nullable variables.
///
/// Ex:
/// 
/// A -> a | 'epsilon'
/// B -> A C
/// 
/// becomes
/// 
/// A -> a
/// B -> A C | C
bool
analyser_environment_t::detect_nullable_nonterminal_for_one_alternative(
  node_t * const node) const
{
  if (node == node->rule_node()->rule_end_node())
  {
    // This is the direct definition of nullable productions.
    return true;
  }
  else
  {
    assert(node->alternative_start() == node);
    
    node_t *curr_node = node;
    
    // first to check if this production contains terminal symbols.
    // If it is, then this production can not be a nullable production.
    while (curr_node != node->rule_node()->rule_end_node())
    {
      if (curr_node->is_terminal())
      {
        return false;
      }
      assert(1 == curr_node->next_nodes().size());
      curr_node = curr_node->next_nodes().front();
    }
    
    // If reach here, this production contains all nonterminal symbols.
    curr_node = node;
    
    while (curr_node != node->rule_node()->rule_end_node())
    {
      assert(curr_node->nonterminal_rule_node() != 0);
      
      if (false == detect_nullable_nonterminal_for_one_rule(
            curr_node->nonterminal_rule_node()))
      {
        return false;
      }
      else
      {
        curr_node->nonterminal_rule_node()->set_nullable(true);
      }
      
      assert(1 == curr_node->next_nodes().size());
      curr_node = curr_node->next_nodes().front();
    }
    
    return true;
  }
}

bool
analyser_environment_t::detect_nullable_nonterminal_for_one_rule(
  node_t * const rule_node) const
{
  assert(true == rule_node->is_rule_head());
  
  if (true == rule_node->is_nullable())
  {
    return true;
  }
  else
  {
    if (false == rule_node->traversed())
    {
      rule_node->set_traversed(true);
      
      BOOST_FOREACH(node_t * const node, rule_node->next_nodes())
      {
        if (true == detect_nullable_nonterminal_for_one_alternative(node))
        {
          return true;
        }
      }
    }
    return false;
  }
}

void
analyser_environment_t::detect_nullable_nonterminal()
{
  BOOST_FOREACH(node_t * const node, m_top_level_nodes)
  {
    (void)traverse_all_nodes(clear_node_state, 0, 0);
    
    if (true == node->is_nullable())
    {
      continue;
    }
    
    if (true == detect_nullable_nonterminal_for_one_rule(node))
    {
      node->set_nullable(true);
    }
  }
}

namespace
{
  /// Check if this alternative is an epsilon production.
  ///
  /// \param alternative_start 
  ///
  /// \return 
  ///
  bool
  check_if_epsilon_production(node_t * const alternative_start)
  {
    assert(alternative_start != 0);
    
    if (alternative_start == alternative_start->rule_node()->rule_end_node())
    {
      assert(0 == alternative_start->name().size());
      
      return true;
    }
    else
    {
      assert(alternative_start->name().size() != 0);
      
      assert(1 == alternative_start->prev_nodes().size());
      
      assert(alternative_start->prev_nodes().front() == alternative_start->rule_node());
    }
    
    return false;
  }
  
  /// \brief Check if this alternative is a direct cyclic alternative.
  ///
  /// Direct cyclic means the following form:
  ///
  /// A -> A
  ///
  /// \param alternative_start 
  ///
  /// \return 
  ///
  bool
  check_if_direct_cyclic_production(node_t * const alternative_start)
  {
    assert(alternative_start != 0);
    
    node_t * const rule_node = alternative_start->rule_node();
    node_t * const rule_end_node = rule_node->rule_end_node();
    
    if (alternative_start != rule_end_node)
    {
      assert(1 == alternative_start->prev_nodes().size());
      assert(1 == alternative_start->next_nodes().size());
      
      assert(alternative_start->prev_nodes().front() == alternative_start->rule_node());
    
      if (alternative_start->next_nodes().front() == rule_end_node)
      {
        if (false == alternative_start->is_terminal())
        {
          assert(alternative_start->nonterminal_rule_node() != 0);
          
          if (alternative_start->nonterminal_rule_node() == rule_node)
          {
            return true;
          }
        }
      }
    }
    
    return false;
  }
}

void
analyser_environment_t::remove_epsilon_production()
{
  // I mark non-terminal nodes which may be epsilon as
  // optional, and then perform normal optional nodes
  // expansion.
  BOOST_FOREACH(node_t * const rule_node, m_top_level_nodes)
  {
    if (true == rule_node->is_nullable())
    {
      BOOST_FOREACH(node_t * const node, rule_node->refer_to_me_nodes())
      {
        node->set_optional(true);
      }
    }
  }
  
  std::vector<check_node_func> check_func;
  
  check_func.push_back(find_same_alternative_in_one_rule);
  check_func.push_back(check_if_epsilon_production);
  check_func.push_back(check_if_direct_cyclic_production);
  
  BOOST_FOREACH(node_t * const rule_node, m_top_level_nodes)
  {
    for (std::list<node_t *>::const_iterator iter2 =
           rule_node->next_nodes().begin();
         iter2 != rule_node->next_nodes().end();
         )
    {
      if (true == check_if_epsilon_production(*iter2))
      {
        // remove direct epsilon production
        //
        // Ex: remove "A -> 'epsilon'"
        rule_node->rule_end_node()->break_prepend_node(rule_node);
        iter2 = rule_node->break_append_node(rule_node->rule_end_node());
      }
      else
      {
        // build new alternative because of nullable
        // production.
        //
        // Ex:
        //
        // A -> a | 'epsilon'
        // B -> A C
        //
        // becomes
        //
        // A -> a
        // B -> A C | C
        build_new_alternative_for_optional_nodes(*iter2, check_func, true);
        ++iter2;
      }
    }
  }
}
