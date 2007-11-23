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
#include "global.hpp"
#include "lookahead.hpp"

#include "wcl_memory_debugger\memory_debugger.h"

lookahead_set_t *
lookahead_set_t::find_lookahead(node_t * const node)
{
  for (std::list<lookahead_set_t>::iterator iter = m_next_level.begin();
       iter != m_next_level.end();
       ++iter)
  {
    if (0 == (*iter).mp_node->name().compare(node->name()))
    {
      return &(*iter);
    }
  }
  return 0;
}

lookahead_set_t *
lookahead_set_t::insert_lookahead(node_t * const node)
{
#if defined(_DEBUG)
  if (node->name().size() != 0)
  {
    assert(true == node->is_terminal());
  }
#endif
  
  m_next_level.push_back(lookahead_set_t(0, node, 0));
  return &(m_next_level.back());
}

namespace
{
  bool
  find_last_symbol_stack(std::list<last_symbol_t> const &last_symbol_stack,
                         node_t * const node,
                         unsigned int const curr_last_symbol_level)
  {
    // Because the nodes stored in 'last_symbol_stack' should all be
    // the rule head node, the 'node' passed by the 2nd parameter
    // should be a rule head node, too.
    //
    assert(true == node->is_rule_head());
    
    // search nodes in 'last_symbol_stack'.
    for (std::list<last_symbol_t>::const_iterator iter = last_symbol_stack.begin();
         iter != last_symbol_stack.end();
         ++iter)
    {
      // The nodes stored in 'last_symbol_stack' should all be the
      // rule head node.
      //
      assert(true == (*iter).mp_node->is_rule_head());
      
      if (((*iter).mp_node == node) &&
          ((*iter).level == curr_last_symbol_level))
      {
        return true;
      }
    }
    return false;
  }
  
  lookahead_set_t *
  insert_lookahead(lookahead_set_t * const lookahead_set,
                   node_t * const node)
  {
    assert(lookahead_set != 0);
    assert(node != 0);
    
    // If there is a node with the same name in the
    // 'lookahead_set->m_next_level', then I don't add this node to
    // it, just return.
    //
    for (std::list<lookahead_set_t>::iterator iter =
           lookahead_set->m_next_level.begin();
         iter != lookahead_set->m_next_level.end();
         ++iter)
    {
#if defined(_DEBUG)
      if ((*iter).mp_node->name().size() != 0)
      {
        if (false == (*iter).mp_node->is_terminal())
        {
          bool find = false;
          
          for (std::list<std::wstring>::const_iterator iter2 =
                 (*iter).mp_orig_node->rule_node()->
                 token_name_as_terminal_during_lookahead().begin();
               iter2 != (*iter).mp_orig_node->rule_node()->
                 token_name_as_terminal_during_lookahead().end();
               ++iter2)
          {
            if (0 == (*iter).mp_node->name().compare(*iter2))
            {
              find = true;
            }
          }
          assert(true == find);
        }
      }
#endif
      
      // Don't insert it if there has been a node with the same name in it.
      if (0 == (*iter).mp_node->name().compare(node->name()))
      {
        return &(*iter);
      }
    }
    
    lookahead_set->m_next_level.push_back(
      lookahead_set_t(lookahead_set->mp_orig_node,
                      node,
                      lookahead_set->m_level + 1));
    
    lookahead_set->mp_orig_node->set_lookahead_depth(lookahead_set->m_level + 1);
    
    return &(lookahead_set->m_next_level.back());
  }
}

void
analyser_environment_t::add_a_terminal_to_lookahead_set(
  node_t * const node,
  lookahead_set_t * const lookahead_set,
  unsigned int * const prev_level_walk_count,
  size_t const cur_level,
  size_t const max_level,
  std::list<recur_lookahead_t> &recur_parent_stack,
  std::list<last_symbol_t> &last_symbol_stack,
  unsigned int const curr_last_symbol_level) const
{  
  lookahead_set_t * const new_lookahead_set = insert_lookahead(lookahead_set, node);
  assert(new_lookahead_set != 0);
  
#if defined(TRACING_LOOKAHEAD)
  log(L"%c\n", L'*');
#endif
  
  assert(1 == node->next_nodes().size());
  if (cur_level < (max_level - 1))
  {
    node_t * const next_node = node->next_nodes().front();
    
#if 0
    wchar_t *str;
    if (0 == next_node->name().size())
    {
      str = form_rule_end_node_name(next_node, false);
    }
    else
    {
      str = const_cast<wchar_t *>(next_node->name().c_str());
    }
    log(L"<INFO>: compute lookahead %d from node %s[%d]\n",
        cur_level + 1,
        str,
        next_node->overall_idx());
    if (0 == next_node->name().size())
    {
      fmtstr_delete(str);
    }
#endif
    
    // search more lookahead from here.
    compute_lookahead_set(next_node,
                          new_lookahead_set,
                          prev_level_walk_count,
                          cur_level + 1,
                          max_level,
                          recur_parent_stack,
                          last_symbol_stack,
                          curr_last_symbol_level + 1);
  }
}

/// @brief Compute lookahead terminals.
///
/// Compute lookahead terminals from the @p node , and put
/// them into the @p lookahead_set .
///
/// This function utilize a cache facility. In the following
/// situation:
///
/// A
///   B c
///   B
///
/// If I need to collect @e x (or any number) lookahead
/// terminal from the node A, and after tracing node B in
/// the first alternative, I have collected @e x (or any
/// number) lookahead terminal, then I don't need to trace
/// node B in the second alternative.
///
/// The way I achieve this facility is to pass a local
/// variable (initial to 0) of the function
/// compute_lookahead_set() whose first argument @p node is
/// the node B in the first alternative above to the called
/// compute_lookahead_set() for the nonterminal rule node of
/// the mentioned node B. If the compute_lookahead_set()
/// function calls chain tracing after the node B in the
/// first alternative (i.e. to the node c above), then I
/// will set the local variable to 1. Before to trace into
/// the node B in the second alternative, I will check the
/// value of the local variable specified above, if its
/// value is 1, then I still need to trace into it, besides,
/// I will just return.
///
/// @param node 
/// @param lookahead_set 
/// @param cur_level 
/// @param max_level 
/// @param recur_parent_stack 
/// @param last_symbol_stack 
/// @param curr_last_symbol_level 
///
void
analyser_environment_t::compute_lookahead_set(
  node_t * const node,
  lookahead_set_t * const lookahead_set,
  unsigned int * const prev_level_walk_count,
  size_t const cur_level,
  size_t const max_level,
  std::list<recur_lookahead_t> &recur_parent_stack,
  std::list<last_symbol_t> &last_symbol_stack,
  unsigned int const curr_last_symbol_level) const
{
  assert(max_level >= 1);
  assert(cur_level < max_level);
  
#if defined(TRACING_LOOKAHEAD)
  wchar_t *str;
  
  if (0 == node->name().size())
  {
    str = form_rule_end_node_name(node, false);
  }
  else
  {
    str = const_cast<wchar_t *>(node->name().c_str());
  }
  log(L"<INFO>: trace to %s[%d]\n", str, node->overall_idx());
  if (0 == node->name().size())
  {
    fmtstr_delete(str);
  }
#endif
  
  if (true == node->name().empty())
  {
    // =======================================
    //        This is a rule end node 
    // =======================================
    assert(0 == node->next_nodes().size());
    
    if (recur_parent_stack.size() != 0)
    {
      // =================================================
      //  A rule end node which is come from another rule
      // =================================================
      
      // I come from this rule from another rule,
      // thus I can go back to the original node to trace more
      // lookahead terminals.
      //
      node_t * const return_node = recur_parent_stack.back().node();
      
      unsigned int * const return_node_prev_level_walk_count =
        recur_parent_stack.back().prev_level_walk_count();
      
      recur_parent_stack.pop_back();
      
      // If I trace back to the upper layer, I have to set
      // the walk_count of this layer to 0.
      //
      // Ex:
      //
      //   A->B->C
      //     / ^
      //    /   \_____
      //   /          \
      //  B ->D->F---->o
      //   \          /
      //    ->D->F->G/
      // 
      // After tracing the first alternative of rule B
      // (i.e. B->D->F->o), I go back to node B in
      // A->B->C. The walk count of B->D->F->o is 2. If I
      // don't set the walk count of it from 2 to 0, then if
      // I go back to rule B and want to trace the second
      // alternative (i.e. B->D->F->G->o), I will find that
      // I can skip to trace it (but this is wrong, I have
      // to trace it) because of the first 2 symbols are
      // equal in the first and second alternatives and the
      // walk count is 2.
      //
      if (prev_level_walk_count != 0)
      {
        (*prev_level_walk_count) = 0;
      }
      
      assert(return_node->name().size() != 0);
      assert(false == return_node->is_terminal());
      assert(false == return_node->is_rule_head());
      assert(1 == return_node->next_nodes().size());
      
#if defined(TRACING_LOOKAHEAD)
      log(L"<INFO>: trace back to %s[%d]\n",
          return_node->name().c_str(),
          return_node->overall_idx());
#endif
      
      compute_lookahead_set(return_node->next_nodes().front(),
                            lookahead_set,
                            return_node_prev_level_walk_count,
                            cur_level,
                            max_level,
                            recur_parent_stack,
                            last_symbol_stack,
                            curr_last_symbol_level);
      
      // return from 'compute_lookahead_set'
      //
      // Ex:
      //
      //   A->B->C
      //     / ^
      //    /   \__
      //   /       \
      //  B ->D->F->o
      //   \       /
      //    ->E->G/
      //
      // I am at 'o', just return from node 'B'.
      // thus I should add node 'B' to the back of the
      // 'recur_parent_stack' to indicate that I can go to
      // node 'B' when I tracing other paths of rule 'B'.
      //
      recur_parent_stack.push_back(
        recur_lookahead_t(return_node,
                          return_node_prev_level_walk_count));
    }
    else
    {
      // =======================================
      //   rule end node for the initial rule
      // =======================================
      
      // I come to here without going through any previous rule,
      // so that I have to continue tracing along with
      // 'refer_to_me_nodes()'. 
      //
      if (0 == node->rule_node()->refer_to_me_nodes().size())
      {
        // =======================================
        //            EOF rule end node
        // =======================================
        
        // This is a rule end node, and I can not find any
        // other node refer to this rule node, thus, I
        // should see an EOF here. 
        //
        // In lookahead symbols, I use 'rule end nodes' as a
        // sign which represents EOF symbol.
        //
        assert(true == node->is_eof());
        
        insert_lookahead(lookahead_set, node);
        
#if defined(TRACING_LOOKAHEAD)
        log(L"%c\n", L'*');
#endif
        
        // Because I should see EOF here, thus I should not
        // find more lookahead terminals from here.
        //
      }
      else
      {
        // =======================================
        //     rule end node has 'refer_to_me'
        // =======================================
        
        // Indicate that I am trying to find what terminals
        // can follow 'node->rule_node()'
        //
        last_symbol_stack.push_back(last_symbol_t(node->rule_node(),
                                                  curr_last_symbol_level));
        
        for (std::list<node_t *>::const_iterator iter =
               node->rule_node()->refer_to_me_nodes().begin();
             iter != node->rule_node()->refer_to_me_nodes().end();
             ++iter)
        {
          assert((*iter)->name().size() != 0);
          assert(false == (*iter)->is_terminal());
          assert(false == (*iter)->is_rule_head());
          assert(1 == (*iter)->next_nodes().size());
          
          if (((*iter)->rule_node() ==
               node->rule_node()) &&
              ((*iter)->next_nodes().front() ==
               node->rule_node()->rule_end_node()))
          {
            // If I encounter a node such that it is the
            // last node of the form: 
            //
            // A -> a A
            //
            // then I will skip tracing that node to look
            // for more lookahead terminals.
            //
            // Because it is a right recursion, I can not
            // find any more lookahead terminals from the
            // right recursion. 
            //
            // Thus, in this case, I don't need to trace
            // further and the only possible token I shall
            // see here is an EOF symbol.
            //
            // In lookahead symbols, I use 'rule end nodes'
            // as a sign which represents EOF symbol.
            //
            if (true == node->is_eof())
            {
              insert_lookahead(lookahead_set,
                               node->rule_node()->rule_end_node());
            }
            continue;
          }
          
          if (((*iter)->next_nodes().front() ==
               (*iter)->rule_node()->rule_end_node()) &&
              (true == find_last_symbol_stack(last_symbol_stack,
                                              (*iter)->rule_node(),
                                              curr_last_symbol_level)))
          {
            //===========================================
            //   why I need last_symbol_stack here? 
            //===========================================
            // Consider the following grammar:
            // ---
            // S
            //   a A
            // 
            // A
            //   c
            //   f
            // 
            // E
            //   C
            //   a F
            // 
            // C
            //   a S
            // 
            // F
            //   E
            //   V
            //
            // V
            //   e
            // ---
            // If we want to find 2-lookahead symbols for
            //   the 'a' alternative of the S rule, it will
            //   be {a,c; a,f}. 
            //
            // However, if we want to find 3-lookahead for
            // this 'a', the searching sequence will be:
            //
            // S -> a -> A -> c -> C -> E -> F -> E -> F ->
            // E -> F -> ...
            //
            // This is an infinite loop !
            //
            // Indeed, if we start from 'C':
            //
            // C -> a -> S -> a -> A -> {c,f}
            //
            // The terminals which we can trace from 'S' are
            // just {a,c; a,f}. There are no the third
            // terminal we can trace from 'S'. 
            //
            // Thus, we have to remember every nonterminals
            // when using the "refer_to_me"
            // relationships. If we find that the oncoming
            // nonterminal has already been traced, then we
            // will stop the recursive searching and just
            // return. 
            //
            
            // This is a 'X_end' node, and if I need to find
            // more lookahead terminals from the 'X_end',
            // this means that I must find what terminals
            // can follow X. 
            //
            // If 'last_symbol_stack' has 'X', this means I
            // have already try to find  what terminals can
            // follow 'X'.
            //
            // Thus, in this case, I don't need to trace
            // further and the only possible token I shall
            // see here is an EOF symbol.
            //
            // In lookahead symbols, I use 'rule end nodes'
            // as a sign which represents EOF symbol.
            //
            if (true == node->is_eof())
            {
              insert_lookahead(lookahead_set,
                               node->rule_node()->rule_end_node());
            }
            continue;
          }
          else
          {
#if defined(TRACING_LOOKAHEAD)
            log(L"<INFO>: [N]trace further to %s[%d]\n",
                (*iter)->name().c_str(),
                (*iter)->overall_idx());
#endif
            
            compute_lookahead_set((*iter)->next_nodes().front(),
                                  lookahead_set,
                                  0,
                                  cur_level,
                                  max_level,
                                  recur_parent_stack,
                                  last_symbol_stack,
                                  curr_last_symbol_level);
          }
        }
        
        assert(last_symbol_stack.size() >= 1);
        assert(last_symbol_stack.back().mp_node == node->rule_node());
        
        last_symbol_stack.pop_back();
      }
    }
  }
  else if (true == node->is_terminal())
  {
    // =======================================
    //        This is a terminal node 
    // =======================================
    if (prev_level_walk_count != 0)
    {
      if (node->distance_from_rule_head() > (*prev_level_walk_count))
      {
        (*prev_level_walk_count) = node->distance_from_rule_head();
      }
    }
    
    add_a_terminal_to_lookahead_set(
      node,
      lookahead_set,
      prev_level_walk_count,
      cur_level,
      max_level,
      recur_parent_stack,
      last_symbol_stack,
      curr_last_symbol_level);
  }
  else
  {
    // ======================================
    //           nonterminal node
    // ======================================
    
    assert(false == node->is_terminal());
    
    if (prev_level_walk_count != 0)
    {
      if (node->distance_from_rule_head() > (*prev_level_walk_count))
      {
        (*prev_level_walk_count) = node->distance_from_rule_head();
      }
    }
    
    if (false == node->is_rule_head())
    {
      // ================================================
      //    Normal non-terminal node, NOT a rule node
      // ================================================
      assert(node->nonterminal_rule_node() != 0);
      
      // I design a 'treat_as_terminal' feature to avoid
      // to trace lookahead symbols depthly. However, I
      // found that this feature is not good enough to
      // solve all such problems and has significant
      // restriction.
      //
      // Ex:
      //
      //   "S" [as_terminal:A]
      //   : "A" "a" ---------(1)
      //   | "B" "b" ---------(2)
      //   ;
      //   
      //   "A"
      //   : "c" "S" "e" "f"
      //   ;
      //   
      //   "B"
      //   : "A"
      //   ;
      //
      // The fact is I can not distinguish the rule (1) &
      // rule (2) of nonterminal S, because the lookahead
      // symbols for this 2 rules are equal - ("c", "c",
      // "c", ...). However, if I use 'as_terminal:A', then
      // this algorithm will distinguish these 2 rules, and
      // this is wrong.
      //
      // So that users must be careful to use this
      // 'as_terminal' feature.
      bool treat_as_terminal = false;
      
      // To see whether I want to treat this nonterminal
      // node as a terminal node.
      for (std::list<std::wstring>::const_iterator iter =
             lookahead_set->mp_orig_node->rule_node()->
             token_name_as_terminal_during_lookahead().begin();
           iter != lookahead_set->mp_orig_node->rule_node()->
             token_name_as_terminal_during_lookahead().end();
           ++iter)
      {
        if (0 == (*iter).compare(node->name()))
        {
          // Want to treat this nonterminal node as a
          // terminal.
          treat_as_terminal = true;
          break;
        }
      }
      
      if (true == treat_as_terminal)
      {
        add_a_terminal_to_lookahead_set(
          node,
          lookahead_set,
          prev_level_walk_count,
          cur_level,
          max_level,
          recur_parent_stack,
          last_symbol_stack,
          curr_last_symbol_level);
      }
      else
      {
#if defined(TRACING_LOOKAHEAD)
        log(L"<INFO>: trace to rule - %s[%d]\n",
            node->nonterminal_rule_node()->name().c_str(),
            node->nonterminal_rule_node()->overall_idx());
#endif
        
        // indicate that I can go back to here.
        recur_parent_stack.push_back(
          recur_lookahead_t(node,
                            prev_level_walk_count));
        
        std::list<unsigned int> walk_counts(
          node->nonterminal_rule_node()->next_nodes().size(),
          0);
        
        std::list<unsigned int>::iterator iter_count =
          walk_counts.begin();
        
        for (std::list<node_t *>::const_iterator iter =
               node->nonterminal_rule_node()->next_nodes().begin();
             iter != node->nonterminal_rule_node()->next_nodes().end();
             ++iter,
             ++iter_count)
        {
          if (true == is_duplicated_alternatives(
                node->nonterminal_rule_node()->next_nodes().begin(),
                iter,
                walk_counts))
          {
          }
          else
          {
            compute_lookahead_set(*iter,
                                  lookahead_set,
                                  &(*iter_count),
                                  cur_level,
                                  max_level,
                                  recur_parent_stack,
                                  last_symbol_stack,
                                  curr_last_symbol_level);
            
            assert((*iter_count) <= (*iter)->distance_to_rule_end_node());
          }
        }
        
        assert(recur_parent_stack.size() > 0);
        assert(recur_parent_stack.back().node() == node);
        
        recur_parent_stack.pop_back();
      }
    }
    else
    {
      // ======================================
      //        Rule non-terminal node
      // ======================================
      
      std::list<unsigned int> walk_counts(
        node->next_nodes().size(),
        0);
      
      std::list<unsigned int>::iterator iter_count =
        walk_counts.begin();
      
      for (std::list<node_t *>::const_iterator iter =
             node->next_nodes().begin();
           iter != node->next_nodes().end();
           ++iter,
             ++iter_count)
      {
        // handle duplicated alternatives
        if (true == is_duplicated_alternatives(node->next_nodes().begin(),
                                               iter,
                                               walk_counts))
        {
        }
        else
        {
          compute_lookahead_set(*iter,
                                lookahead_set,
                                &(*iter_count),
                                cur_level,
                                max_level,
                                recur_parent_stack,
                                last_symbol_stack,
                                curr_last_symbol_level);
          
          assert((*iter_count) <= (*iter)->distance_to_rule_end_node());
        }
      }
    }
  }
}

void
analyser_environment_t::compute_lookahead_terminal_for_node(
  node_t * const node,
  size_t const needed_depth) const
{
  std::list<recur_lookahead_t> recur_parent_stack;
  std::list<last_symbol_t> last_symbol_stack;
  
  wchar_t *str;
  
  if (0 == node->name().size())
  {
    str = form_rule_end_node_name(node, false);
  }
  else
  {
    str = const_cast<wchar_t *>(node->name().c_str());
  }
  log(L"<INFO>: compute lookahead %d from node %s[%d]\n",
      needed_depth,
      str,
      node->overall_idx());
  if (0 == node->name().size())
  {
    fmtstr_delete(str);
  }
  
  compute_lookahead_set(node,
                        &(node->lookahead_set()),
                        0,
                        0,
                        needed_depth,
                        recur_parent_stack,
                        last_symbol_stack,
                        0);
}

namespace
{
  // pivot_node: a non rule head nonterminal node.
  // node: a rule head node.
  //
  bool
  check_two_alternative_has_common_starting_nonterminal_internal(
    node_t * const pivot_node,
    node_t * const node)
  {
    assert(false == pivot_node->is_terminal());
    assert(pivot_node->name().size() != 0);
    assert(true == node->is_rule_head());
    
    for (std::list<node_t *>::const_iterator iter = node->next_nodes().begin();
         iter != node->next_nodes().end();
         ++iter)
    {
      // Should not have empty production.
      assert((*iter)->name().size() != 0);
      
      if (false == (*iter)->is_terminal())
      {
        if (0 == pivot_node->name().compare((*iter)->name()))
        {
          return true;
        }
        else
        {
          assert((*iter)->nonterminal_rule_node() != 0);
          
          bool const result =
            check_two_alternative_has_common_starting_nonterminal_internal(
              pivot_node,
              (*iter)->nonterminal_rule_node());
          
          if (true == result)
          {
            return true;
          }
        }
      }
    }
    
    return false;
  }  
}

namespace
{
  void
  make_todo_node_set_from_nodes(
    std::list<node_t *> const &nodes,
    std::list<todo_nodes_t> &todo_nodes)
  {
    std::list<node_t *>::const_iterator iter1 = nodes.begin();
    
    assert((*iter1)->ae() != 0);
    
    bool const using_pure_BNF = (*iter1)->ae()->using_pure_BNF();
    
    // Construct todo_nodes_set:
    // 
    // o-> A...
    //     B...
    //     C...
    //     D...
    // 
    // Then, 'todo_nodes_set' will contain {A, B} {A, C} {A, D}
    // {B, C} {B, D} {C, D}
    do
    {
      // iter1 & iter2 are 2 alternative start nodes.
      std::list<node_t *>::const_iterator tmp = iter1;
      std::list<node_t *>::const_iterator iter2 = (++tmp);
      
      for (; iter2 != nodes.end(); ++iter2)
      {
        bool push_this_one = false;
        
        if (true == using_pure_BNF)
        {
          push_this_one = true;
        }
        else
        {
          assert(((*iter1)->alternative_start() != 0) ||
                 ((*iter2)->alternative_start() != 0));
          
          if (((*iter1)->alternative_start() != 0) &&
              ((*iter2)->alternative_start() != 0))
          {
            // I just need to calculate lookahead tree for 2
            // nodes in the same alternative.
            if ((*iter1)->alternative_start() == (*iter2)->alternative_start())
            {
              // I don't need to calculate lookahead tree
              // for 2 nodes which correspond to the same
              // nodes in the main regex alternative.
              if (((*iter1)->corresponding_node_in_main_regex_alternative()) !=
                  ((*iter2)->corresponding_node_in_main_regex_alternative()))
              {
                push_this_one = true;
              }
            }
          }
          else
          {
            push_this_one = true;
          }
        }
        
        if (true == push_this_one)
        {
          todo_nodes.push_back(todo_nodes_t(*iter1, *iter2));
        }
      }
      
      ++iter1;
    } while ((*iter1) != nodes.back());
  }
}

void
node_t::calculate_lookahead_waiting_pool_for_pure_BNF_rule(
  std::list<todo_nodes_t> &todo_nodes_set_2) const
{
  assert(m_next_nodes.size() > 1);
  assert(true == m_is_rule_head);
  
  std::list<todo_nodes_t> todo_nodes_set;
  
  make_todo_node_set_from_nodes(m_next_nodes, todo_nodes_set);
  
  assert(mp_ae != 0);
  
  if (true == mp_ae->enable_left_factor())
  {
    // find the first different nodes for each alternative
    // pair. This process can be though of left factoring.
    // 
    // << NOTE >>
    // However, factorization is sometimes inhibited by the fact
    // that the semantic processing of conflicting
    // alternatives differs.
    while (todo_nodes_set.size() != 0)
    {
      todo_nodes_t &todo_nodes = todo_nodes_set.front();
      
      if (0 == todo_nodes.mp_node_a->name().compare(
            todo_nodes.mp_node_b->name()))
      {
        // Two nodes with same name.
        // I will find lookahead terminals for the children of these 2 nodes.
      
        // 'mp_node_a' & 'mp_node_b' can be the rule end node at the same time,
        // because this means that these 2 alternatives are equal
        // (i.e. duplicated).
        assert(1 == todo_nodes.mp_node_a->next_nodes().size());
        assert(1 == todo_nodes.mp_node_b->next_nodes().size());
        assert(todo_nodes.mp_node_a->rule_node() == todo_nodes.mp_node_b->rule_node());
          
#if defined(_DEBUG)
        // these 2 nodes can not be equal to the rule end node, because if so,
        // this means these 2 alternatives are the same.
        // (i.e. duplicated alternatives).
        // I should remove the duplicated alternatives at eraly stage.
        if (todo_nodes.mp_node_a->next_nodes().front() ==
            todo_nodes.mp_node_a->rule_node()->rule_end_node())
        {
          assert(todo_nodes.mp_node_b->next_nodes().front() !=
                 todo_nodes.mp_node_b->rule_node()->rule_end_node());
        }
        if (todo_nodes.mp_node_b->next_nodes().front() ==
            todo_nodes.mp_node_b->rule_node()->rule_end_node())
        {
          assert(todo_nodes.mp_node_a->next_nodes().front() !=
                 todo_nodes.mp_node_a->rule_node()->rule_end_node());
        }
#endif
          
        todo_nodes_set.push_back(
          todo_nodes_t(
            todo_nodes.mp_node_a->next_nodes().front(),
            todo_nodes.mp_node_b->next_nodes().front()));
      }
      else
      {
        todo_nodes_set_2.push_back(todo_nodes);
      }
    
      todo_nodes_set.pop_front();
    }
  }
  else
  {
    todo_nodes_set_2.splice(todo_nodes_set_2.end(), todo_nodes_set);
  }
  
  assert(0 == todo_nodes_set.size());
}

namespace
{
  struct find_candidate_nodes_for_lookahead_searching : public std::unary_function<node_t, bool>
  {
  private:
    
    node_t const * const mp_rule_node;
    std::list<todo_nodes_t> &m_todo_nodes;
    
  public:
    
    find_candidate_nodes_for_lookahead_searching(
      node_t const * const rule_node,
      std::list<todo_nodes_t> &todo_nodes)
      : mp_rule_node(rule_node),
        m_todo_nodes(todo_nodes)
    {
      assert(true == mp_rule_node->is_rule_head());
    }
    
    bool
    operator()(
      node_t * const node)
    {
      for (std::list<regex_info_t>::const_reverse_iterator iter = node->regex_info().rbegin();
           iter != node->regex_info().rend();
           ++iter)
      {
        if ((*iter).m_ranges.front().mp_start_node == node)
        {
          std::list<node_t *> nodes;            
          
          collect_candidate_nodes_for_lookahead_calculation_for_EBNF_rule(
            mp_rule_node, iter, nodes);
          
          std::list<todo_nodes_t> todo_nodes_set;
          
          make_todo_node_set_from_nodes(nodes, todo_nodes_set);
          
          m_todo_nodes.splice(m_todo_nodes.end(), todo_nodes_set);
        }
      }
      
      return true;
    }
  };
}

void
node_t::calculate_lookahead_waiting_pool_for_EBNF_rule(
  std::list<todo_nodes_t> &todo_nodes_set) const
{
  assert(true == m_is_rule_head);
  assert(mp_main_regex_alternative != 0);
  
  (void)node_for_each(mp_main_regex_alternative,
                      0,
                      find_candidate_nodes_for_lookahead_searching(this, todo_nodes_set));
}

void
analyser_environment_t::compute_lookahead_set()
{
  // traverse each grammar rule.
  for (std::list<node_t *>::const_iterator iter = m_top_level_nodes.begin();
       iter != m_top_level_nodes.end();
       ++iter)
  {
    // If there are more than 1 alternative node of this rule node,
    // then I will look for lookahead terminals.
    // Otherwise, I don't need to do this because there is only one way to
    // derive.
    //
    if ((*iter)->next_nodes().size() > 1)
    {
      std::list<todo_nodes_t> todo_nodes_set;
      
      if (true == m_using_pure_BNF)
      {
        (*iter)->calculate_lookahead_waiting_pool_for_pure_BNF_rule(todo_nodes_set);
      }
      else
      {
        (*iter)->calculate_lookahead_waiting_pool_for_EBNF_rule(todo_nodes_set);
      }
      
      mark_number_for_all_nodes();
      
      while (todo_nodes_set.size() != 0)
      {
        // I will find lookahead terminals for these 2 nodes now.
        todo_nodes_t &todo_nodes = todo_nodes_set.front();
        
        // In the EBNF form, because all nodes in the
        // lookahead tree is changed to those in the main
        // regex alternative, so that it is possible that
        // the 2 comparison nodes are already ambiguity.
        if (true == todo_nodes.mp_node_a->is_ambigious_to(todo_nodes.mp_node_b))
        {
          assert(true == todo_nodes.mp_node_b->is_ambigious_to(todo_nodes.mp_node_a));
        }
        else
        {
          assert(false == todo_nodes.mp_node_b->is_ambigious_to(todo_nodes.mp_node_a));
          
          LOOKAHEAD_COMPARE_RESULT result;
          
          while ((result = compare_lookahead_set_for_node(
                    todo_nodes.mp_node_a,
                    todo_nodes.mp_node_b))
                 != LOOKAHEAD_OK)
          {
            if (LOOKAHEAD_AMBIGIOUS == result)
            {
              log(L"<INFO>: %s[%d] is ambigious to %s[%d]\n",
                  todo_nodes.mp_node_a->name().c_str(),
                  todo_nodes.mp_node_a->overall_idx(),
                  todo_nodes.mp_node_b->name().c_str(),
                  todo_nodes.mp_node_b->overall_idx());
            
              todo_nodes.mp_node_a->add_ambigious_set(todo_nodes.mp_node_b);
              todo_nodes.mp_node_b->add_ambigious_set(todo_nodes.mp_node_a);
            
              assert(todo_nodes.mp_node_a->rule_node() == todo_nodes.mp_node_b->rule_node());
            
              todo_nodes.mp_node_a->rule_node()->contains_ambigious() = true;
            
              break;
            }
            else
            {
              assert(LOOKAHEAD_NEED_MORE_SEARCH == result);
            
              // 2 lookahead terminals set are not different,
              // I have to find more in deeper depth.
              if (todo_nodes.mp_node_a->lookahead_depth() == 
                  todo_nodes.mp_node_b->lookahead_depth())
              {
                unsigned int needed_depth =
                  todo_nodes.mp_node_a->lookahead_depth() + 1;
              
                compute_lookahead_terminal_for_node(
                  todo_nodes.mp_node_a,
                  needed_depth);
              
                compute_lookahead_terminal_for_node(
                  todo_nodes.mp_node_b,
                  needed_depth);
              }
              else if (todo_nodes.mp_node_a->lookahead_depth() <
                       todo_nodes.mp_node_b->lookahead_depth())
              {
                unsigned int needed_depth =
                  todo_nodes.mp_node_a->lookahead_depth() + 1;
              
                compute_lookahead_terminal_for_node(
                  todo_nodes.mp_node_a,
                  needed_depth);
              }
              else
              {
                unsigned int needed_depth =
                  todo_nodes.mp_node_b->lookahead_depth() + 1;
              
                compute_lookahead_terminal_for_node(
                  todo_nodes.mp_node_b,
                  needed_depth);
              }
            }
          }
        }
        
        todo_nodes_set.pop_front();
      }
    }
  }
}

LOOKAHEAD_COMPARE_RESULT
analyser_environment_t::compare_lookahead_set(
  std::list<lookahead_set_t> const &lookahead_a,
  std::list<lookahead_set_t> const &lookahead_b) const
{
  assert(lookahead_a.size() != 0);
  assert(lookahead_b.size() != 0);
  
  for (std::list<lookahead_set_t>::const_iterator iter_a =
         lookahead_a.begin();
       iter_a != lookahead_a.end();
       ++iter_a)
  {
    for (std::list<lookahead_set_t>::const_iterator iter_b =
           lookahead_b.begin();
         iter_b != lookahead_b.end();
         ++iter_b)
    {
      assert((*iter_a).mp_orig_node->rule_node() ==
             (*iter_b).mp_orig_node->rule_node());
      
      if (0 == (*iter_a).mp_node->name().compare((*iter_b).mp_node->name()))
      {
        // equal name
        if (0 == (*iter_a).mp_node->name().size())
        {
          assert(0 == (*iter_b).mp_node->name().size());
          
          // two paths achieve EOF at the same time;
          // this must be ambigious, doesn't need to find more
          // lookahead terminals.
          //
          return LOOKAHEAD_AMBIGIOUS;
        }
        else if (((*iter_a).m_next_level.size() != 0) &&
                 ((*iter_b).m_next_level.size() != 0))
        {
          // There exist more lookahead terminals on 2 paths, compare
          // them now.
          //
          assert((*iter_a).mp_node->name().size() != 0);
          assert((*iter_b).mp_node->name().size() != 0);
          assert((*iter_a).m_level < m_max_lookahead_searching_depth);
          assert((*iter_b).m_level < m_max_lookahead_searching_depth);
          
          LOOKAHEAD_COMPARE_RESULT const result =
            compare_lookahead_set((*iter_a).m_next_level,
                                  (*iter_b).m_next_level);
          
          switch (result)
          {
          case LOOKAHEAD_OK:
            // continue to compare.
            continue;
            
          case LOOKAHEAD_NEED_MORE_SEARCH:
          case LOOKAHEAD_AMBIGIOUS:
            // return immediately.
            return result;
            
          default:
            assert(0);
            break;
          }
        }
        else
        {
          // There are no more lookahead terminals on at least one
          // path; return "AMBIGIOUS" or "NEED MORE SEARCH" according
          // to 'max_lookahead_searching_depth'.
          //
          assert((*iter_a).mp_node->name().size() != 0);
          assert((*iter_b).mp_node->name().size() != 0);
          
          if (m_max_lookahead_searching_depth == (*iter_a).m_level)
          {
            assert(m_max_lookahead_searching_depth == (*iter_b).m_level);
            
            return LOOKAHEAD_AMBIGIOUS;
          }
          else
          {
            assert(((*iter_a).m_level < m_max_lookahead_searching_depth) &&
                   ((*iter_b).m_level < m_max_lookahead_searching_depth));
            
            return LOOKAHEAD_NEED_MORE_SEARCH;
          }
        }
      } // equal name
    }
  }
  
  return LOOKAHEAD_OK;
}

LOOKAHEAD_COMPARE_RESULT
analyser_environment_t::compare_lookahead_set_for_node(
  node_t const * const node_a,
  node_t const * const node_b) const
{
  if ((0 == node_a->lookahead_set().m_next_level.size()) ||
      (0 == node_b->lookahead_set().m_next_level.size()))
  {
    return LOOKAHEAD_NEED_MORE_SEARCH;
  }
  
  return compare_lookahead_set(node_a->lookahead_set().m_next_level,
                               node_b->lookahead_set().m_next_level);
}

bool
analyser_environment_t::is_duplicated_alternatives(
  std::list<node_t *>::const_iterator begin_iter,
  std::list<node_t *>::const_iterator end_iter,
  std::list<unsigned int> const &walk_counts) const
{
  std::list<unsigned int>::const_iterator iter_count = walk_counts.begin();
  
  for (std::list<node_t *>::const_iterator iter = begin_iter;
       iter != end_iter;
       ++iter,
         ++iter_count)
  {
    node_t *begin_node = (*begin_iter);
    node_t *end_node = (*end_iter);
    unsigned int walk_count = (*iter_count);
    bool same = false;
    
    while (walk_count > 0)
    {
      if (begin_node->name().compare(end_node->name()) != 0)
      {
        break;
      }
      else
      {
        assert(1 == begin_node->next_nodes().size());
        assert(1 == end_node->next_nodes().size());
        
        begin_node = begin_node->next_nodes().front();
        end_node = end_node->next_nodes().front();
        
        --walk_count;
        
        if (0 == walk_count)
        {
          same = true;
        }
      }
    }
    
    if (true == same)
    {
      return true;
    }
  }
  
  return false;
}
