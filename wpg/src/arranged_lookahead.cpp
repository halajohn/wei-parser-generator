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
#include "arranged_lookahead.hpp"
#include "ga_exception.hpp"

#include "wcl_memory_debugger\memory_debugger.h"

arranged_lookahead_t *
arranged_lookahead_t::find_first_child_node()
{
  // Ensure this is the top level node.
  assert(0 == mp_parent);
  
  arranged_lookahead_t *parent = this;
  
  while (1)
  {
    if (0 == parent->children().size())
    {
      return parent;
    }
    else
    {
      parent = &(parent->children().front());
    }
  }
  
  assert(0);
  return 0;
}

arranged_lookahead_t::iterator &
arranged_lookahead_t::iterator::operator++()
{
  // Because the arranged_lookahead tree enumeration is
  // only for its leaf nodes, so that I have to ensure
  // that 'mp_arranged_lookahead' is a leaf node
  // everytime.
  assert(mp_arranged_lookahead != 0);
  assert(0 == mp_arranged_lookahead->children().size());
  
#if defined(_DEBUG)
  arranged_lookahead_t *arranged_lookahead = mp_arranged_lookahead;
  
  std::list<std::list<arranged_lookahead_t>::iterator>::reverse_iterator iter = m_children_iter.rbegin();
  
  while (arranged_lookahead != 0)
  {
    if (arranged_lookahead->parent() != 0)
    {
      assert(&(**iter) == arranged_lookahead);
      
      assert(iter != m_children_iter.rend());
      ++iter;
      
      arranged_lookahead = arranged_lookahead->parent();
    }
    else
    {
      break;
    }
  }
#endif
  
  arranged_lookahead_t *node = mp_arranged_lookahead;
  arranged_lookahead_t *parent = 0;
  
  while (1)
  {
    if (0 == node->parent())
    {
      assert(0 == m_children_iter.size());
      
      // I have already reached the top level of this
      // tree.
      parent = 0;
      break;
    }
    else
    {
      assert(m_children_iter.back() != node->parent()->children().end());
      ++(m_children_iter.back());
      
      if (m_children_iter.back() == node->parent()->children().end())
      {
        // I have finished enumerating this parent
        // node.
        node = node->parent();
        
        m_children_iter.pop_back();
      }
      else
      {
        parent = node->parent();
        break;
      }
    }
  }
  
  while (1)
  {
    if (0 == parent)
    {
      assert(0 == m_children_iter.size());
      
      mp_arranged_lookahead = 0;
      break;
    }
    else
    {
      assert(m_children_iter.size() != 0);
      
      if (0 == ((*(m_children_iter.back())).children().size()))
      {
        // I find a new child node, return it now.
        mp_arranged_lookahead = &(*(m_children_iter.back()));
        break;
      }
      else
      {
        parent = &(*(m_children_iter.back()));
        
        m_children_iter.push_back(parent->children().begin());
      }
    }
  }
  
  return *this;
}

namespace
{
  void
  add_lookahead_set_into_arranged_lookahead(
    node_with_order_t const &orig_node,
    lookahead_set_t const * const lookahead_set,
    arranged_lookahead_t * const arranged_lookahead,
    node_t const * const default_node)
  {
    assert(lookahead_set->m_next_level.size() != 0);
    
    if (0 == arranged_lookahead->children().size())
    {
      // put 'default' node to this level first.
      arranged_lookahead->children().push_back(
        arranged_lookahead_t(arranged_lookahead, default_node));
    }
    else
    {
      // The latest child of 'arranged_lookahead' should be
      // the 'default_node'.
      assert(1 == arranged_lookahead->children().back().lookahead_nodes().size());
      assert(0 == arranged_lookahead->children().back().lookahead_nodes().front()->name().size());
    }
    
    BOOST_FOREACH(lookahead_set_t const &la, lookahead_set->m_next_level)
    {
      bool find = false;
      
      arranged_lookahead_t *child_arranged_lookahead = 0;
      
      // because the name of 'default' node is empty, thus
      // don't compare '(*iter).mp_lookahead_node' with
      // the 'default' node, so that the empty name node
      // (which I used for the EOF node) can be put into the
      // arranged lookahead tree.
      std::list<arranged_lookahead_t>::iterator last_iter =
        arranged_lookahead->children().end();
      
      // Skip the 'default_node'.
      --last_iter;
      
      // Check if there already exist a arranged_lookahead node whose
      // name equals to the current lookahead node. If no,
      // then add this lookahead node into the
      // arranged_lookahead tree, otherwise, go to that
      // arranged_lookahead node and search from that node.
      for (std::list<arranged_lookahead_t>::iterator iter =
             arranged_lookahead->children().begin();
           iter != last_iter;
           ++iter)
      {
        assert(1 == (*iter).lookahead_nodes().size());
        
        if (0 == (*iter).lookahead_nodes().front()->name().compare(
              la.mp_node->name()))
        {
          child_arranged_lookahead = &(*iter);
          find = true;
          break;
        }
      }
      
      arranged_lookahead_t *target_arranged_lookahead;
      
      if (false == find)
      {
        // There are no arranged_lookahead node with such
        // name, thus I have to add this lookahead node into
        // the arranged_lookahead tree.
        
        // I have to ensure that the default node is the
        // last node of this level, thus I use push_front()
        // here. 
        arranged_lookahead->children().push_front(
          arranged_lookahead_t(arranged_lookahead, la.mp_node)
                                                  );
        
        target_arranged_lookahead = &(arranged_lookahead->children().front());
      }
      else
      {
        // There already exist an arranged_lookahead node
        // whose name equals to the current lookahead node,
        // the go to that arranged_lookahead node, and
        // search from there.
        assert(child_arranged_lookahead != 0);
        
        target_arranged_lookahead = child_arranged_lookahead;
      }
      
      if (la.m_next_level.size() != 0)
      {
        // If this lookahead node has children lookahead
        // node, then add these lookahead children node
        // into the arragned_lookahead tree (started from
        // 'target_arranged_lookahead'.)
        add_lookahead_set_into_arranged_lookahead(
          orig_node,
          &la,
          target_arranged_lookahead,
          default_node);
      }
      else
      {
        // If there are no more lookahead node, then I can
        // finish the searching this time.
        
        // The size of 'target_nodes' should
        // be 1. If it is greater than 1, it means an
        // ambiguity is happened, and this rule node should
        // not be dumpped.
        {
          assert(target_arranged_lookahead->target_nodes().size() <= 1);
          
          if (0 == target_arranged_lookahead->target_nodes().size())
          {
            target_arranged_lookahead->add_target_node(orig_node);
          }
          else
          {
            node_t const * const existed_node =
              target_arranged_lookahead->target_nodes().front().mp_node;
            
            assert(0 == existed_node->name().compare(orig_node.mp_node->name()));
          }
          
          assert(1 == target_arranged_lookahead->target_nodes().size());
        }
        
#if defined(_DEBUG)
        std::list<node_with_order_t> const &tmp =
          target_arranged_lookahead->target_nodes();
        
        BOOST_FOREACH(node_with_order_t const &node1, tmp)
        {
          // the node's name in the
          // 'm_target_nodes' should be the
          // same.
          if (node1.mp_node->name().compare(tmp.front().mp_node->name()) != 0)
          {
            throw ga_exception_meet_ambiguity_t();
          }
          
          unsigned int same_count = 0;
          
          BOOST_FOREACH(node_with_order_t const &node2, tmp)
          {
            if (node1.mp_node == node2.mp_node)
            {
              ++same_count;
            }
          }
          
          assert(1 == same_count);
        }
#endif
      }
    }
  }
}

/// Collect and arranged lookahead nodes searched from
/// each node in \p nodes, and put the result into an
/// arranged_lookahead tree.
///
/// \param nodes 
/// \param default_node 
///
/// \return 
///
boost::shared_ptr<arranged_lookahead_t>
collect_lookahead_into_arrange_lookahead(
  std::list<node_with_order_t> const &nodes,
  node_t const * const default_node)
{
  boost::shared_ptr<arranged_lookahead_t> arranged_lookahead(
    new arranged_lookahead_t(0, 0));
  assert(arranged_lookahead != 0);
  
  BOOST_FOREACH(node_with_order_t const &node, nodes)
  {
    // There could be possible that the lookahead tree of a
    // candidate nodes is empty.
    //
    // Ex:
    //
    // "A"
    // : "a" ("B" "B")* "b" "c"
    // ;
    //
    // will expand to
    //
    // "A"
    // : "a" ("B" "B")* "b" "c"
    //         ^         ^
    // : "a" "b" "c"
    //        ^
    // ;
    //
    // The candidate nodes which may participate in the
    // lookahead searching process are: {B, b, b} (pointed
    // by '^').
    //
    // However, the node "b" in the second alternative
    // doesn't have comparsion nodes in the same one, so
    // that I will not find lookahead nodes for it. Hence,
    // it's lookahead tree will be empty.
    if (node.mp_node->lookahead_depth() > 0)
    {
      lookahead_set_t const * const lookahead_set = &(node.mp_node->lookahead_set());
      
      add_lookahead_set_into_arranged_lookahead(
        node,
        lookahead_set,
        arranged_lookahead.get(),
        default_node);
    }
  }
  
  return arranged_lookahead;
}
  
/// Merge arranged_lookahead tree node which has equal
/// corresponding original node.
///
/// a --- b --- c [g]
///          +
///          +- e [g]
///
/// a --- b --- c, e [g]
///
/// \param tree 
/// \param default_node 
///
void
merge_arranged_lookahead_tree_leaf_node_if_they_have_same_target_node(
  arranged_lookahead_t * const tree,
  node_t const * const default_node)
{
  for (std::list<arranged_lookahead_t>::iterator iter = 
         tree->children().begin();
       iter != tree->children().end();
       ++iter)
  {
    // Check to see if this node is the 'default'
    // node. If it is, then skip merging this node.
    if ((1 == (*iter).lookahead_nodes().size()) &&
        ((*iter).lookahead_nodes().front() == default_node))
    {
      assert(&(*iter) == &(tree->children().back()));
      return;
    }
    else
    {
      if ((*iter).children().size() != 0)
      {
        // There should no target node in the intermediate
        // lookahead nodes.
        //
        // However, in the EBNF form, this situation could
        // be happened.
        //
        // Ex:
        //
        // "A"
        // : "a" ("e" | "b" "d")* "b" ("c" | "e")
        // ;
        //
        // will expand to:
        //
        // "A"
        // : "a" "b" ("c" | "e")
        // | "a" ("e") "b" ("c" | "e")
        //              ^
        // | "a" ("b" "d") "b" ("c" | "e")
        //                  ^
        // | ...
        // ;
        //
        // The lookahead set for the node 'b' in the second
        // alternative will be {b}, but the lookahead tree
        // for the node 'b' in the third alternative will be
        // {b, c; b, e}.
        //
        // Hence, there will be 'target_nodes'
        // in the intermedia level in the lookahead tree for
        // the node 'b'.
        //
        // i.e.
        //
        // b ["b"] -+- c ["b"]
        //          |
        //          +- e ["b"]
        //
        // However, although in this situation, the same
        // target node (relative to the main regex
        // alternative) should be appear in the children
        // node of this lookahead tree. The reason why is
        // explained below:
        //
        // Based on this example, one permutation of the
        // node 'b' and its following nodes would be
        //
        // ... "b" "c"
        //
        // It would be faced with every permutation and
        // combination for other starting nodes:
        //
        // "e"     "b" "c"
        // "b" "d" "b" "c"
        // 
        // so that, if other starting nodes needs more
        // lookahead tree level, then the node 'b' in that
        // searching operation would needs the same level of
        // lookahead tree. So that, I don't need to search
        // as many level as lookahead tree nodes in every
        // path relative to the node "b" and its following
        // nodes.
        if ((*iter).target_nodes().size() != 0)
        {
          BOOST_FOREACH(node_with_order_t const &node_with_order,
                        (*iter).target_nodes())
          {
            assert(true == (*iter).leaf_node_has_target_node_relative_to_main_regex_alternative(
                     node_with_order.mp_node));
          }
        }
        
        // I will not merge intermediate lookahead node.
        merge_arranged_lookahead_tree_leaf_node_if_they_have_same_target_node(
          &(*iter), default_node);
      }
      else
      {
        // The reason why this value may greater than 1
        // is because of the following situation:
        // 
        // S[0] S_END[1]
        //   a[2][a;] A[3][b;] a[4] 
        //   b[5][b;] A[6][b,b;] b[7] a[8] 
        //   a[9][a;] a[10][a;] 
        //   b[11][b;] b[12][b,a;] a[13] 
        // 
        // See the first column (a[2], b[5], a[9], b[11]).
        assert((*iter).target_nodes().size() >= 1);
          
        if ((*iter).target_nodes().size() > 1)
        {
          continue;
        }
        else
        {
          std::list<arranged_lookahead_t>::iterator tmp = iter;
          ++tmp;
          
          for (std::list<arranged_lookahead_t>::iterator iter2 = tmp;
               iter2 != tree->children().end();
               )
          {
            if ((*iter2).children().size() != 0)
            {
              // There should no target node in the intermediate
              // lookahead nodes.
              //
              // However, ... (please see the explaination above)
              if ((*iter2).target_nodes().size() != 0)
              {
                BOOST_FOREACH(node_with_order_t const &node_with_order,
                              (*iter2).target_nodes())
                {
                  assert(true == (*iter2).leaf_node_has_target_node_relative_to_main_regex_alternative(
                           node_with_order.mp_node));
                }
              }
              
              // I will not merge intermediate lookahead node.
              ++iter2;
            }
            else
            {
              // Check to see if this node is the 'default'
              // node. If it is, then skip merging this node.
              if ((1 == (*iter2).lookahead_nodes().size()) &&
                  ((*iter2).lookahead_nodes().front() == default_node))
              {
                assert(&(*iter2) == &(tree->children().back()));
                ++iter2;
              }
              else
              {
                // The reason why this value may be
                // greater than 1 is to see above comment
                // for (iter).
                assert((*iter2).target_nodes().size() >= 1);
                  
                if ((*iter2).target_nodes().size() > 1)
                {
                  ++iter2;
                  continue;
                }
                else
                {
                  // S[0] S_END[1]
                  //   A[3][b,c;b,d;b,e;b,f;] a b
                  if ((*iter).target_nodes().front().mp_node ==
                      (*iter2).target_nodes().front().mp_node)
                  {
                    assert(1 == (*iter2).lookahead_nodes().size());
                    
                    (*iter).add_lookahead_node((*iter2).lookahead_nodes().front());
                    
                    iter2 = tree->children().erase(iter2);
                  }
                  else
                  {
                    ++iter2;
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}

void
change_target_node_of_arranged_lookahead_tree_nodes_to_its_corresponding_one_in_main_regex_alternative(
  arranged_lookahead_t * const tree,
  node_t const * const default_node)
{
  for (std::list<arranged_lookahead_t>::iterator iter = 
         tree->children().begin();
       iter != tree->children().end();
       ++iter)
  {
    // Check to see if this node is the 'default'
    // node. If it is, then skip changing this node.
    if ((1 == (*iter).lookahead_nodes().size()) &&
        ((*iter).lookahead_nodes().front() == default_node))
    {
      assert(&(*iter) == &(tree->children().back()));
      return;
    }
    else
    {
      if ((*iter).children().size() != 0)
      {
        // There should no target node in the intermediate
        // lookahead nodes.
        //
        // However, ... (please see the explaination above)
        if ((*iter).target_nodes().size() != 0)
        {
          BOOST_FOREACH(node_with_order_t const &node_with_order,
                        (*iter).target_nodes())
          {
            assert(true == (*iter).leaf_node_has_target_node_relative_to_main_regex_alternative(
                     node_with_order.mp_node));
          }
        }
        
        change_target_node_of_arranged_lookahead_tree_nodes_to_its_corresponding_one_in_main_regex_alternative(
          &(*iter), default_node);
      }
      else
      {
        // The reason why this value may greater than 1
        // is because of the following situation:
        // 
        // S[0] S_END[1]
        //   a[2][a;] A[3][b;] a[4] 
        //   b[5][b;] A[6][b,b;] b[7] a[8] 
        //   a[9][a;] a[10][a;] 
        //   b[11][b;] b[12][b,a;] a[13] 
        // 
        // See the first column (a[2], b[5], a[9], b[11]).
        assert((*iter).target_nodes().size() >= 1);
        
        BOOST_FOREACH(node_with_order_t &node_with_order,
                      (*iter).target_nodes())
        {
          node_with_order.mp_node =
            node_with_order.mp_node->corresponding_node_in_main_regex_alternative();
        }
      }
    }
  }
}

void
arranged_lookahead_t::find_lookahead_sequence(
  std::list<arranged_lookahead_t const *> &lookahead_sequence)
{
  lookahead_sequence.clear();
  
  arranged_lookahead_t *curr_arranged_lookahead_node = this;
  
  while (curr_arranged_lookahead_node != 0)
  {
    if (1 == curr_arranged_lookahead_node->lookahead_nodes().size())
    {
      if (0 == curr_arranged_lookahead_node->lookahead_nodes().front())
      {
        // The root node of the arranged_lookahead tree is useless.
        assert(0 == curr_arranged_lookahead_node->parent());
        break;
      }
    }
    
    lookahead_sequence.push_front(curr_arranged_lookahead_node);
    
    curr_arranged_lookahead_node = curr_arranged_lookahead_node->parent();
  }
}

void
find_first_different_arranged_lookahead(
  std::list<arranged_lookahead_t const *> &tree1,
  std::list<arranged_lookahead_t const *> &tree2,
  std::list<arranged_lookahead_t const *>::iterator &iter1,
  std::list<arranged_lookahead_t const *>::iterator &iter2)
{
  iter1 = tree1.begin();
  iter2 = tree2.begin();
  
  while (1)
  {
    if ((iter1 != tree1.end()) && (iter2 != tree2.end()))
    {
      if ((*iter1) != (*iter2))
      {
        break;
      }
      else
      {
        ++iter1;
        ++iter2;
      }
    }
    else
    {
      break;
    }
  }
}

bool
arranged_lookahead_t::leaf_node_has_target_node_relative_to_main_regex_alternative(
  node_t const * const node) const
{
  assert(node != 0);
  
  if (mp_children.size() != 0)
  {
    BOOST_FOREACH(arranged_lookahead_t const &tree,
                  mp_children)
    {
      if (true == tree.leaf_node_has_target_node_relative_to_main_regex_alternative(node))
      {
        return true;
      }
      else
      {
        continue;
      }
    }
  }
  else
  {
    BOOST_FOREACH(node_with_order_t const &node_with_order,
                  mp_target_nodes)
    {
      if (node_with_order.mp_node->corresponding_node_in_main_regex_alternative() ==
          node->corresponding_node_in_main_regex_alternative())
      {
        return true;
      }
    }
    
    return false;
  }

  return false;
}
