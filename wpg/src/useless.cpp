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

/// \brief remove useless rule node
///
/// Ex:
///  A -> C
///  B -> b
///  C -> c
///
///  The production rule for non-terminal symbol 'B' is
///  useless, thus this function will remove this production
///  rules.
void
analyser_environment_t::remove_useless_rule()
{
  // I mark number for all nodes just for the
  // following 'log' statements.
  mark_number_for_all_nodes();
  
  for (std::list<node_t *>::iterator iter = m_top_level_nodes.begin();
       iter != m_top_level_nodes.end();
       )
  {
    assert(true == (*iter)->is_rule_head());
    
    if ((0 == (*iter)->refer_to_me_nodes().size()) &&
        (false == (*iter)->is_starting_rule()))
    {
      log(L"<INFO>: remove useless rule %s[%d]\n", (*iter)->name().c_str(), (*iter)->overall_idx());
      
      std::list<node_t *> alternatives = (*iter)->next_nodes();
      
      ::remove_alternatives(this, alternatives, false, true, true);
      
      assert(0 == (*iter)->next_nodes().size());
      assert(0 == (*iter)->rule_end_node()->prev_nodes().size());
      
      boost::checked_delete((*iter)->rule_end_node());
      
      node_t * const tmp = (*iter);
      
      iter = m_top_level_nodes.erase(iter);
      remove_rule_head(tmp);
      
      boost::checked_delete(tmp);
    }
    else
    {
      ++iter;
    }
  }
}
