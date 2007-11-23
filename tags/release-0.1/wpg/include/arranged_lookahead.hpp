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

#ifndef __arranged_lookahead_hpp__
#define __arranged_lookahead_hpp__

/// This is the main class for the lookahead tree. This
/// class implements required API to fullfill the needs
/// of an STL input iterator. So that I can use simple
/// 'begin', 'end', 'operator++' to enumerate the children
/// nodes of a lookahead tree.
class arranged_lookahead_t;

typedef class arranged_lookahead_t *arranged_lookahead_t_ptr;

class arranged_lookahead_t
{
private:
  
  // The node used for lookahead.
  std::list<node_t const *> mp_lookahead_node;
  
  std::list<node_with_order_t> mp_target_nodes;
  
  arranged_lookahead_t * const mp_parent;
  
  std::list<arranged_lookahead_t> mp_children;
  
public:
  
  arranged_lookahead_t(arranged_lookahead_t * const parent,
                       node_t const * const lookahead_node)
    : mp_parent(parent)
  {
    mp_lookahead_node.push_back(lookahead_node);
  }
  
  std::list<arranged_lookahead_t> const &children() const
  { return mp_children; }
  
  std::list<arranged_lookahead_t> &children()
  { return mp_children; }
  
  arranged_lookahead_t_ptr const &parent() const
  { return mp_parent; }
  
  std::list<node_t const *> const &lookahead_nodes() const
  { return mp_lookahead_node; }
  
  void add_lookahead_node(node_t const * const node)
  {
    assert(node != 0);
    
    mp_lookahead_node.push_back(node);
  }
  
  void add_target_node(node_with_order_t const &node_with_order)
  {
    mp_target_nodes.push_back(node_with_order);
  }
  
  std::list<node_with_order_t> &target_nodes()
  { return mp_target_nodes; }
  
  std::list<node_with_order_t> const &target_nodes() const
  { return mp_target_nodes; }
  
  arranged_lookahead_t *find_first_child_node();
  
  void find_lookahead_sequence(std::list<arranged_lookahead_t const *> &lookahead_sequence);
  
  bool leaf_node_has_target_node_relative_to_main_regex_alternative(
    node_t const * const node) const;
  
  // The following API is for iterating a arranged_lookahead
  // tree.
  class iterator : public std::iterator<std::input_iterator_tag, arranged_lookahead_t *>
  {
  private:
    
    // Setting this field to 0 indicates the end of
    // enumeration.
    arranged_lookahead_t *mp_arranged_lookahead;
    std::list<std::list<arranged_lookahead_t>::iterator> m_children_iter;
    
  public:
    
    iterator(
      arranged_lookahead_t * const arranged_lookahead = 0)
      : mp_arranged_lookahead(arranged_lookahead)
    { }
    
    iterator(iterator const &iter)
      : mp_arranged_lookahead(iter.arranged_lookahead()),
        m_children_iter(iter.children_iter())
    { }
    
    std::list<std::list<arranged_lookahead_t>::iterator> const &children_iter() const
    { return m_children_iter; }
    
    std::list<std::list<arranged_lookahead_t>::iterator> &children_iter()
    { return m_children_iter; }
    
    arranged_lookahead_t_ptr const &arranged_lookahead() const
    { return mp_arranged_lookahead; }
    
    arranged_lookahead_t_ptr &arranged_lookahead()
    { return mp_arranged_lookahead; }
    
    bool operator==(iterator const &iter) const
    {
      if (mp_arranged_lookahead == iter.mp_arranged_lookahead)
      {
        return true;
      }
      else
      {
        return false;
      }
    }
    
    bool operator!=(iterator const &iter) const
    {
      return !(*this == iter);
    }
    
    arranged_lookahead_t *operator*()
    {
      return mp_arranged_lookahead;
    }
    
    iterator &operator++();
  };
  typedef class iterator iterator;
  
  iterator begin()
  {
    // Only the root node of a arranged_lookahead tree can
    // call this 'begin()' function.
    assert(0 == mp_parent);
    
    if (0 == mp_children.size())
    {
      return end();
    }
    else
    {
      iterator iter(find_first_child_node());
      
      arranged_lookahead_t *arranged_lookahead = iter.arranged_lookahead();
      
      while (arranged_lookahead != 0)
      {
        if (arranged_lookahead->parent() != 0)
        {
          iter.children_iter().push_front(arranged_lookahead->parent()->children().begin());
          
          arranged_lookahead = arranged_lookahead->parent();
        }
        else
        {
          break;
        }
      }
      
      return iter;
    }
  }
  
  iterator end()
  {
    // Only the root node of a arranged_lookahead tree can
    // call this 'end()' function.
    assert(0 == mp_parent);
    
    return iterator(0);
  }
};
typedef class arranged_lookahead_t arranged_lookahead_t;

extern boost::shared_ptr<arranged_lookahead_t> collect_lookahead_into_arrange_lookahead(
  std::list<node_with_order_t> const &nodes,
  node_t const * const default_node);

extern void merge_arranged_lookahead_tree_leaf_node_if_they_have_same_target_node(
  arranged_lookahead_t * const tree,
  node_t const * const default_node);

extern void find_first_different_arranged_lookahead(
  std::list<arranged_lookahead_t const *> &tree1,
  std::list<arranged_lookahead_t const *> &tree2,
  std::list<arranged_lookahead_t const *>::iterator &iter1,
  std::list<arranged_lookahead_t const *>::iterator &iter2);

extern void change_target_node_of_arranged_lookahead_tree_nodes_to_its_corresponding_one_in_main_regex_alternative(
  arranged_lookahead_t * const tree,
  node_t const * const default_node);

#endif
