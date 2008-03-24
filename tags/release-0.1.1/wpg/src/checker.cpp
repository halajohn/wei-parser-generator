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

bool
analyser_environment_t::check_grammar_not_all_left_recursion()
{
  for (std::list<node_t *>::const_iterator iter = m_top_level_nodes.begin();
       iter != m_top_level_nodes.end();
       ++iter)
  {
    bool left_recursion_exist = false;
    bool non_left_recursion_exist = false;
    
    for (std::list<node_t *>::const_iterator iter2 = (*iter)->next_nodes().begin();
         iter2 != (*iter)->next_nodes().end();
         ++iter2)
    {
      if (false == (*iter2)->is_terminal())
      {
        if ((*iter2)->nonterminal_rule_node() == (*iter))
        {
          left_recursion_exist = true;
          continue;
        }
      }
      non_left_recursion_exist = true;
    }
    
    if ((true == left_recursion_exist) &&
        (false == non_left_recursion_exist))
    {
      log(L"<ERROR>: All alternatives of the rule \"%s\" are left recursive;\
 it can not appear in a derivation of a sentence.\n",
             (*iter)->name().c_str());
      return false;
    }
  }
  
  return true;
}

bool
analyser_environment_t::check_grammar_not_all_right_recursion()
{
  for (std::list<node_t *>::const_iterator iter = m_top_level_nodes.begin();
       iter != m_top_level_nodes.end();
       ++iter)
  {
    bool right_recursion_exist = false;
    bool non_right_recursion_exist = false;
    
    for (std::list<node_t *>::const_iterator iter2 = (*iter)->next_nodes().begin();
         iter2 != (*iter)->next_nodes().end();
         ++iter2)
    {
      if ((*iter2) == (*iter)->rule_end_node())
      {
        /// This is an epsilon production.
        continue;
      }
      
      node_t * const last_node = (*iter2)->find_alternative_last_node();
      assert(last_node != 0);
      
      if (false == last_node->is_terminal())
      {
        if (last_node->nonterminal_rule_node() == (*iter))
        {
          right_recursion_exist = true;
          continue;
        }
      }
      non_right_recursion_exist = true;
    }
    
    if ((true == right_recursion_exist) &&
        (false == non_right_recursion_exist))
    {
      log(L"<ERROR>: All alternatives of the rule \"%s\" are right recursive;\
 it can not appear in a derivation of a sentence.\n",
             (*iter)->name().c_str());
      return false;
    }
  }
  
  return true;
}

bool
analyser_environment_t::check_grammar()
{
  if (false == check_grammar_not_all_left_recursion())
  {
    return false;
  }
  if (false == check_grammar_not_all_right_recursion())
  {
    return false;
  }
  return true;
}
