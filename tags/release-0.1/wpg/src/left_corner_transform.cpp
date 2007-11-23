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

///
/// @file   left_corner_transform.cpp
/// @author Wei Hu
/// @date
/// 
/// @brief  
/// 
/// =============================================================================
///                            left corner transform
/// =============================================================================
///
/// The LC (Left Corner) transform can be used to eliminate
/// the left recursion from a grammar which contains no
/// cyclic. 
///
/// The original LC transform is from:
/// "J. Rosenkrantz and P.M. Lewis. 1970. Deterministic left
/// corner parser." 
///
/// and the algorithm which introduces is:
///
/// 1) If a terminal symbol a is a proper left corner of A
///    in the original grammar, add A->aA-a to the
///    transformed grammar. 
/// 2) If (B is a proper left corner of A) or (B == A), and
///    B->X\beta is a production of the original grammar,
///    add A-X->\betaA-B to the transformed grammar. 
/// 3) For any nonterminal A in the original grammar, add
///    A-A->\epsilon to the transformed grammar.
///
/// In 1998, Johnson ("Finite state approximation of
/// constraint-based grammars using left-corner grammar
/// transforms.") modified the origianl LC transform
/// algorithm slightly: 
///
/// -1) If a terminal symbol a is a proper left corner of A
///    in the original grammar, add A->aA-a to the
///    transformed grammar. 
/// -2) If (B is a proper left corner of A) or (B == A and if
///    A is a proper left corner of itself), and B->X\beta
///    is a production of the original grammar, add
///    A-X->\betaA-B to the transformed grammar. 
/// -3) If X is a proper left corner of A and A->X\beta is a
///    production of the original grammar, add A-X->\beta to
///    the transformed grammar. 
///
/// For example, here is an example grammar:
///
///   S -> NP VP
///   NP -> DET N
///   VP -> V ADV
///
///   DET, N, V, ADV are terminals.
///
/// The left corner symbols of S are: NP, DET
/// The left corner symbols of NP are: DET
/// The left corner symbols of VP are: V
///
/// After applying the origianl LC transforms, the
/// transformed grammar is: 
///
/// from LC transform rule 1):
///   S -> DET S-DET            ------(1)
///   NP -> DET NP-DET          ------(2)
///   VP -> V VP-V              ------(3)
///
/// from LC transform rule 2):
///   S-NP -> VP S-S            ------(4)
///   S-DET -> N S-NP           ------(5)
///   NP-DET -> N NP-NP         ------(6)
///   VP-V -> ADV VP-VP         ------(7)
///
/// from LC transform rule 3):
///   S-S -> \epsilon           ------(8)
///   NP-NP -> \epsilon         ------(9)
///   VP-VP -> \epsilon         ------(10)
///
/// However, new grammar rules (2), (6), (9) can not be
/// reached from the start symbol S, so that we can remove
/// these 3 production rules from the transformed grammar.
///
/// If the origianl grammar is transformed by the new LC
/// transform algorithm introduced by Johnson, the new
/// transformed grammar is: 
///
/// from LC transform rule 1):
///   S -> DET S-DET            ------(1)
///   NP -> DET NP-DET          ------(2)
///   VP -> V VP-V              ------(3)
///
/// from LC transform rule 2):
///   S-DET -> N S-NP           ------(4)
///
/// from LC transform rule 3):
///   S-NP -> VP                ------(5)
///   NP-DET -> N               ------(6)
///   VP-V -> ADV               ------(7)
///
/// The production rules (2), (6) are not reached from S, so
/// that we can remove them from the new transformed
/// grammar. According to "Robert C. Moore: Removing left
/// recursion from context-free grammars", it says that in
/// the johnson's version of the LC transform, a new
/// nonterminal of the form A-X is useless unless X is a
/// proper left corner of A. Moore further notes that a new
/// nonterminal of the form A-X, as well as the original
/// nonterminal A, is useless in the transformed grammar,
/// unless A is either the top nonterminal of the grammar or
/// appears on the right-hand side of an original grammar
/// production in other than the left-most
/// position. Therefore, we will call the original
/// nonterminals meeting this condition "retained
/// nonterminals" and restrict the LC transform so that
/// productions involving nonterminals of the form A-X are
/// created only if A is a retained nonterminal.
///
/// << Please see section 5 of the paper "Robert C. Moore:
/// Removing left recursion from context-free grammars" to
/// get more information of the folling algorithm >>
/// 
/// The paper, "Robert C. Moore: Removing left recursion
/// from context-free grammars", describes a modified LC
/// transform: 
///
/// -1) If a terminal symbol or non-left-recursive
///    nonterminal X is a proper left corner of a retained
///    left-recursive nonterminal A in the original grammar,
///    add A->XA-X to the transformed grammar. 
/// -2) If B is a left-recursive proper left corner of a
///    retained left-recursive nonterminal A and B->X\beta
///    is a production of the original grammar, add
///    A-X->\betaA-B to the transformed grammar. 
/// -3) If X is a proper left corner of a retained
///    left-recursive nonterminal A and A->X\beta is a
///    production of the original grammar, add A-X->\beta to
///    the transformed grammar. 
/// -4) If A is a non-left-recursive nonterminal and A->\beta
///    is a production of the original grammar, add A->\beta
///    to the transformed grammar. 
///
void
analyser_environment_t::find_left_corners_for_node(node_t * const rule_node,
                                                   node_t * const node)
{
  assert(false == node->is_terminal());
  
  for (std::list<node_t *>::const_iterator iter = node->next_nodes().begin();
       iter != node->next_nodes().end();
       ++iter)
  {
    if ((*iter) == (*iter)->rule_node()->rule_end_node())
    {
      /// It shouldn't have any epsilon productions appear in this stage.
      assert(0);
    }
    if (true == (*iter)->traversed())
    {
      return;
    }
    else
    {
      (*iter)->set_traversed(true);
      
      if (false == (*iter)->is_terminal())
      {
        rule_node->add_left_corner_set(*iter);
        find_left_corners_for_node(rule_node, (*iter)->nonterminal_rule_node());
      }
    }
  }
}

void
analyser_environment_t::find_left_corners()
{
  for (std::list<node_t *>::const_iterator iter = m_top_level_nodes.begin();
       iter != m_top_level_nodes.end();
       ++iter)
  {
    traverse_all_nodes(clear_node_state, 0, 0);
    
    find_left_corners_for_node(*iter, *iter);
  }
}

void
analyser_environment_t::lc_transform()
{
}
