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
#include "lexer.hpp"
#include "ga_exception.hpp"
#include "alternative.hpp"
#include "regex.hpp"

#include "wcl_memory_debugger\memory_debugger.h"

namespace
{
  void
  update_regex_range_info_for_list(
    std::list<regex_info_t> &list)
  {
    BOOST_FOREACH(regex_info_t &regex_info, list)
    {
      BOOST_FOREACH(regex_range_t &regex_range, regex_info.m_ranges)
      {
        bool only_one_node_in_range = false;
        
        if (regex_range.mp_start_node == regex_range.mp_end_node)
        {
          only_one_node_in_range = true;
        }
      
        regex_range.mp_start_node =
          regex_range.mp_start_node->brother_node_to_update_regex();
      
        if (false == only_one_node_in_range)
        {
          regex_range.mp_end_node =
            regex_range.mp_end_node->brother_node_to_update_regex();
        }
        else
        {
          regex_range.mp_end_node = regex_range.mp_start_node;
        }
      }
    }
  }
  
  /// 
  ///
  /// \param node 
  ///
  /// \return have to be 'true', this return value is used
  /// in 'node_for_each' template.
  ///
  bool
  update_regex_range_info_for_one_node(
    node_t * const node)
  {
    update_regex_range_info_for_list(node->regex_info());
    update_regex_range_info_for_list(node->tmp_regex_info());
    
    node->check_if_regex_info_is_correct();
    
    return true;
  }
}

void
update_regex_info_for_one_alternative(
  node_t * const alternative_start)
{
  assert(alternative_start != 0);
  
  // Ex:
  //
  // A : (a)*
  //
  // will become:
  //
  // A : a | ...epsilon...
  //
  // Because of the 'epsilon', I have to add the following
  // check.
  if (alternative_start->name().size() != 0)
  {
    assert(alternative_start->alternative_start() == alternative_start);
  }
  
  node_t *curr_node = alternative_start;
  while (curr_node->name().size() != 0)
  {
    update_regex_range_info_for_one_node(curr_node);
    
    curr_node->check_if_regex_info_is_correct();
    
    assert(1 == curr_node->next_nodes().size());
    curr_node = curr_node->next_nodes().front();
  }
}

void
update_regex_info_for_node_range(
  node_t * const range_start_node,
  node_t * const range_end_node)
{
  (void)node_for_each(range_start_node,
                      range_end_node,
                      update_regex_range_info_for_one_node);
}

bool
check_if_regex_info_is_correct_for_one_node(
  node_t * const node)
{
  node->check_if_regex_info_is_correct();
  
  return true;
}

/// 
///
/// \param regex_start_node The first node of the regex
/// specified by the second parameter: regex_info.
/// \param regex_info 
///
void
check_if_regex_info_is_correct(
  node_t const * const regex_start_node,
  regex_info_t const &regex_info)
{
#if defined(_DEBUG)
  switch (regex_info.m_type)
  {
  case REGEX_TYPE_ZERO_OR_MORE:
  case REGEX_TYPE_ZERO_OR_ONE:
  case REGEX_TYPE_ONE_OR_MORE:
  case REGEX_TYPE_ONE:
    assert(1 == regex_info.m_ranges.size());
    
    assert(regex_info.m_ranges.front().mp_start_node != 0);
    assert(regex_info.m_ranges.front().mp_end_node != 0);
    
    assert(regex_info.m_ranges.front().mp_start_node->name().size() != 0);
    assert(regex_info.m_ranges.front().mp_end_node->name().size() != 0);
    
    assert(1 == regex_info.m_ranges.front().mp_end_node->next_nodes().size());
    
    if (regex_start_node != 0)
    {
      assert(regex_start_node == regex_info.m_ranges.front().mp_start_node);
    }
    
    if (regex_info.m_ranges.front().mp_start_node !=
        regex_info.m_ranges.front().mp_end_node)
    {
      assert(true == regex_info.m_ranges.front().mp_start_node->
             is_my_descendant_node(regex_info.m_ranges.front().mp_end_node));
    }
    break;
    
  case REGEX_TYPE_OR:
    {
      node_t *prev_end_node = 0;
      
      BOOST_FOREACH(regex_range_t const &regex_range, regex_info.m_ranges)
      {
        assert(regex_range.mp_end_node != 0);
        assert(regex_range.mp_end_node->name().size() != 0);
        
        assert(1 == regex_range.mp_end_node->next_nodes().size());
        
        if (prev_end_node != 0)
        {
          // The regex OR group should be continus.
          // ie. (A B | C D | E F)
          // The starting node of (C D) should follow the
          // end node of (A B).
          assert(prev_end_node->next_nodes().front() == regex_range.mp_start_node);
        }
        else
        {
          if (regex_start_node != 0)
          {
            assert(regex_start_node == regex_range.mp_start_node);
          }
        }
        
        if (regex_range.mp_start_node != regex_range.mp_end_node)
        {
          assert(true == regex_range.mp_start_node->
                 is_my_descendant_node(regex_range.mp_end_node));
        }
        
        prev_end_node = regex_range.mp_end_node;
      }
    }
    break;
    
  default:
    assert(0);
    break;
  }
#endif
}

namespace
{
  class collect_nodes : public std::unary_function<node_t, bool>
  {
  private:
    
    std::list<node_t *> &m_nodes;
    
  public:
    
    collect_nodes(std::list<node_t *> &nodes)
      : m_nodes(nodes)
    { }
    
    bool
    operator()(
      node_t *node)
    {
      m_nodes.push_back(node);
      
      return true;
    }
  };
  
  void
  collect_regex_nodes(
    std::list<node_t *> &nodes,
    regex_info_t const &regex_info)
  {
    check_if_regex_info_is_correct(0, regex_info);
    
    BOOST_FOREACH(regex_range_t const &regex_range, regex_info.m_ranges)
    {
      (void)node_for_each(regex_range.mp_start_node,
                          regex_range.mp_end_node,
                          collect_nodes(nodes));
    }
  }
  
  void
  collect_regex_nodes(
    std::list<node_t *> &nodes,
    regex_range_t const &regex_range)
  {
    (void)node_for_each(regex_range.mp_start_node,
                        regex_range.mp_end_node,
                        collect_nodes(nodes));
  }
  
  struct set_in_regex_OR_group : public std::unary_function<node_t, bool>
  {
    bool
    operator()(
      node_t * const node)
    {
      node->is_in_regex_OR_group() = true;
      
      return true;
    }
  };
  
  class patch_tmp_regex_info_for_prev_nodes : public std::unary_function<node_t, bool>
  {
  private:
  
    node_t * const mp_if_this_node;
    node_t * const mp_change_to_this_node;
    std::list<regex_info_t *> * const mp_changed_regex_info;
  
  public:
  
    patch_tmp_regex_info_for_prev_nodes(
      node_t * const if_this_node,
      node_t * const change_to_this_node,
      std::list<regex_info_t *> * const changed_regex_info)
      : mp_if_this_node(if_this_node),
        mp_change_to_this_node(change_to_this_node),
        mp_changed_regex_info(changed_regex_info)
    { }
  
    bool operator()(
      node_t * const node)
    {
      BOOST_FOREACH(regex_info_t &regex_info,
                    node->tmp_regex_info())
      {
        assert(1 == regex_info.m_ranges.size());
        assert(regex_info.m_ranges.front().mp_start_node != mp_if_this_node);
      
        if (regex_info.m_ranges.front().mp_end_node == mp_if_this_node)
        {
          regex_info.m_ranges.front().mp_end_node = mp_change_to_this_node;
          
          if (mp_changed_regex_info != 0)
          {
            mp_changed_regex_info->push_back(&regex_info);
          }
        }
      }
      
      return true;
    }
  };
  
  bool
  link_to_the_corresponding_node_in_main_regex_alternative(
    node_t * const node)
  {
    assert(node != 0);
    
    assert(1 == node->corresponding_node_between_fork().size());
    
    node_t * const corresponding_node_in_main_regex_alternative =
      node->corresponding_node_between_fork().front()->corresponding_node_in_main_regex_alternative();
    
    assert(corresponding_node_in_main_regex_alternative->alternative_start() != 0);
    assert(corresponding_node_in_main_regex_alternative->alternative_start() ==
           node->rule_node()->main_regex_alternative());
    assert(node->alternative_start() != node->rule_node()->main_regex_alternative());
    
    node->corresponding_node_in_main_regex_alternative() =
      corresponding_node_in_main_regex_alternative;
    
    corresponding_node_in_main_regex_alternative->
      refer_to_me_nodes_in_regex_expansion_alternatives().push_back(
        node);
    
    return true;
  }
}

///
/// \brief handle regular expression OR statement.
/// 
/// Ex:
/// A (B C | D E | F G) H
/// 
/// will become
/// 
/// A (B C) H
/// A (D E) H
/// A (F G) H
///
///
/// \param alternative_start_node 
/// \param need_to_delete_alternative 
/// \param regex_info 
///
void
analyser_environment_t::handle_regex_OR_stmt(
  node_t * const alternative_start_node,
  std::list<node_t *> &need_to_delete_alternative,
  regex_info_t const &regex_info)
{
  assert(REGEX_TYPE_OR == regex_info.m_type);
  
  node_t * const regex_OR_stmt_start_node =
    regex_info.m_ranges.front().mp_start_node;
  node_t * const regex_OR_stmt_end_node =
    regex_info.m_ranges.back().mp_end_node;
  
  assert(regex_OR_stmt_start_node != regex_OR_stmt_end_node);
  
  node_t * const rule_node = alternative_start_node->rule_node();
  
  std::list<node_t *> nodes_in_regex;
  
  BOOST_FOREACH(regex_range_t const &curr_focus_range,
                regex_info.m_ranges)
  {
    // copy nodes in range.
    //
    // copy one range each time.
    //
    // Ex:
    //
    // A (B C | D E | F) G
    //
    // I will focus one range at a time, ie. (B
    // C), (D E), (F). That is to say, I want to preserve
    // nodes in one range, and discard nodes in other
    // ranges. Hence, 'nodes_in_regex' will contain the
    // nodes which will be discarded.
    nodes_in_regex.clear();
      
    BOOST_FOREACH(regex_range_t const &range1,
                  regex_info.m_ranges)
    {
      if (range1.mp_start_node != curr_focus_range.mp_start_node)
      {
        collect_regex_nodes(nodes_in_regex, range1);
      }
      else
      {
        (void)node_for_each(curr_focus_range.mp_start_node,
                            curr_focus_range.mp_end_node,
                            set_in_regex_OR_group());
      }
    }
    
    // patch 1:
    //
    // Check to see if this node is the starting node of
    // outer regexes, if yes, then I have to patch the
    // starting node of these outer regexes, because the
    // original starting node may be missing.
    //
    // Ex:
    //
    // ((A B | C)* D)*
    //  ^       ^
    //
    // If I am handling the regex OR group pointed by '^',
    // then I have to patch the starting node of previous
    // regexes.
    //
    // The same condition can be applied to regex end
    // node, too.
    if (regex_OR_stmt_start_node->tmp_regex_info().size() != 0)
    {
      // patch the starting node or end node of each used
      // regex range type structure.
      BOOST_FOREACH(regex_info_t &regex_info,
                    regex_OR_stmt_start_node->tmp_regex_info())
      {
        // The elements in the 'tmp_regex_info' are those I
        // have already processed, and regex OR stmt should
        // not put into 'tmp_regex_info'. Hence, I have the
        // following assertion.
        assert(1 == regex_info.m_ranges.size());
        
        // The node range of each regex in 'tmp_regex_info'
        // must be equal or larger than that of this regex
        // OR stmt, that is to say, the starting node of
        // each regex in 'tmp_regex_info' must be equal or
        // prior to the starting of this regex OR stmt, and
        // the end node of this regex OR stmt must be behind
        // the starting node of this regex OR stmt, hence,
        // the starting node of each element in
        // 'tmp_regex_info' must be prior to the end node of
        // this regex OR stmt.
        //
        // The same principle can be applied to the end node
        // of each element in 'tmp_regex_info', too.
        assert(regex_info.m_ranges.front().mp_start_node != regex_OR_stmt_end_node);
        assert(regex_info.m_ranges.front().mp_end_node != regex_OR_stmt_start_node);
        
        assert(regex_info.m_ranges.front().mp_start_node == regex_OR_stmt_start_node);
        
        // Ex:
        //
        // ((a | b)* c)*
        //   ^   ^
        //
        // Change from 'a' to 'b'.
        regex_info.m_ranges.front().mp_start_node = curr_focus_range.mp_start_node;
        
        // Ex:
        //
        // ((a | b)* c)*
        //  ^     ^
        //
        // The REGEX_TYPE_ZERO_OR_MORE regex pointed by '^'.
        if (regex_info.m_ranges.front().mp_end_node == regex_OR_stmt_end_node)
        {
          regex_info.m_ranges.front().mp_end_node = curr_focus_range.mp_end_node;
        }
      }
    }
    
    // patch 2:
    //
    // transfer the result of the above patching.
    //
    // Ex:
    //
    // A ((B | C) D)*
    // 
    // will becomes
    //
    // A
    // A (B D)*
    // A (C D)*
    // 
    // The original regex_range info of node 'C' is
    // empty, so that I have to copy the regex_range
    // info from 'B' to 'C'.
    unsigned int copied_count = 0;
    if (curr_focus_range.mp_start_node != regex_OR_stmt_start_node)
    {
      if (regex_OR_stmt_start_node->tmp_regex_info().size() != 0)
      {
        std::list<regex_info_t>::const_reverse_iterator iter =
          regex_OR_stmt_start_node->tmp_regex_info().rbegin();
      
        std::list<regex_info_t>::const_reverse_iterator iter_end =
          regex_OR_stmt_start_node->tmp_regex_info().rend();
      
        for (; iter != iter_end; ++iter)
        {
          curr_focus_range.mp_start_node->tmp_regex_info().push_front(
            *iter);
        
          ++copied_count;
        }
      }
    }
    
    // patch 3:
    //
    // Ex:
    //
    // (a (b | c)*)*
    //  +------+
    //
    // one regex is started from 'a' to 'c', if I remove 'c'
    // because of regex OR stmt removal, then I have to
    // change this regex from 'a'->'c' to 'a'->'b'.
    std::list<regex_info_t *> changed_regex_info;
    // check if the regex OR stmt is the first regex of this
    // alternative, if it is, then I don't need to perform
    // this patch 3.
    if (regex_OR_stmt_start_node != alternative_start_node)
    {
      assert(1 == regex_OR_stmt_start_node->prev_nodes().size());
      
      node_t * const prior_to_regex_OR_stmt_start_node =
        regex_OR_stmt_start_node->prev_nodes().front();
      
      if (curr_focus_range.mp_end_node != regex_OR_stmt_end_node)
      {
        (void)node_for_each(alternative_start_node,
                            prior_to_regex_OR_stmt_start_node,
                            patch_tmp_regex_info_for_prev_nodes(
                              regex_OR_stmt_end_node,
                              curr_focus_range.mp_end_node,
                              &changed_regex_info));
      }
    }
    
    assert(nodes_in_regex.size() != 0);
    
    bool const create_new_alternative =
      ::fork_alternative(alternative_start_node,
                         alternative_start_node->rule_node(),
                         alternative_start_node->rule_node()->rule_end_node(),
                         nodes_in_regex,
                         false);
    
    // roll back the patch 1 above.
    if (regex_OR_stmt_start_node->tmp_regex_info().size() != 0)
    {
      BOOST_FOREACH(regex_info_t &regex_info,
                    regex_OR_stmt_start_node->tmp_regex_info())
      {
        assert(1 == regex_info.m_ranges.size());
        
        assert(regex_info.m_ranges.front().mp_start_node ==
               curr_focus_range.mp_start_node);
        
        regex_info.m_ranges.front().mp_start_node = regex_OR_stmt_start_node;
        
        if (regex_info.m_ranges.front().mp_end_node == curr_focus_range.mp_end_node)
        {
          regex_info.m_ranges.front().mp_end_node = regex_OR_stmt_end_node;
        }
      }
    }
    
    // roll back the patch 2 above.
    for (unsigned int i = 0; i < copied_count; ++i)
    {
      curr_focus_range.mp_start_node->tmp_regex_info().pop_front();
    }
    
    // roll back the patch 3 above.
    BOOST_FOREACH(regex_info_t * const regex_info,
                  changed_regex_info)
    {
      assert(1 == regex_info->m_ranges.size());
      
      regex_info->m_ranges.front().mp_end_node = regex_OR_stmt_end_node;
    }
    
    if (true == create_new_alternative)
    {
      node_t * const new_alternative_start_node =
        rule_node->next_nodes().back();
      
      update_regex_info_for_one_alternative(new_alternative_start_node);
      
      (void)node_for_each(new_alternative_start_node,
                          0,
                          check_if_regex_info_is_correct_for_one_node);
      
      (void)node_for_each(new_alternative_start_node,
                          0,
                          link_to_the_corresponding_node_in_main_regex_alternative);
    }
  }
  
  // In the normal case, after regex OR statement expansion,
  // I will delete the original one. However, if this
  // deleting alternative is the 'main_regex_alternative',
  // then I have to keep this one.
  need_to_delete_alternative.push_back(
    regex_OR_stmt_start_node->alternative_start()
                                       );
}

void
analyser_environment_t::remove_whole_regex(
  node_t * const alternative_start_node,
  std::list<node_t *> const &nodes_in_regex)
{
  node_t * const rule_node = alternative_start_node->rule_node();
  
  // If it is in a regular expression, and its type
  // is ZERO_OR_MORE, then I will create a new
  // alternative which doesn't contain these nodes.
  // 
  // Ex:
  // A (B C)* D
  // 
  // will become
  // A (B C)* D
  // A D
  // 
  // so that I can have lookahead symbols for 'B' &
  // 'D'.
  
  // It's time to perform the forking.
  bool const create_new_alternative =
    ::fork_alternative(
      alternative_start_node,
      alternative_start_node->rule_node(),
      alternative_start_node->rule_node()->rule_end_node(),
      // use 'nodes_in_regex' as the 'discard_nodes'
      // parameter of this function.
      nodes_in_regex,
      false);
  
  if (true == create_new_alternative)
  {
    // The newly created alternative should be the last
    // one.
    node_t * const new_alternative_start_node =
      rule_node->next_nodes().back();
    
    update_regex_info_for_one_alternative(new_alternative_start_node);
    
    (void)node_for_each(new_alternative_start_node,
                        0,
                        check_if_regex_info_is_correct_for_one_node);
    
    (void)node_for_each(new_alternative_start_node,
                        0,
                        link_to_the_corresponding_node_in_main_regex_alternative);
  }
}

/// 
///
/// \param alternative_start_node 
/// \param nodes_in_regex The nodes in this list should be
/// contained in the alternative specified by the first
/// parameter, \p alternative_start_node .
///
void
analyser_environment_t::duplicate_whole_regex(
  node_t * const alternative_start_node,
  std::list<node_t *> &nodes_in_regex)
{
  BOOST_FOREACH(node_t * const node,
                nodes_in_regex)
  {
    if (node != alternative_start_node)
    {
      assert(true == alternative_start_node->is_my_descendant_node(node));
    }
  }
  
  node_t * const rule_node = alternative_start_node->rule_node();
  
  // decide how many times I should duplicate the whole
  // regex.
  //
  // This function will only consider those regex_info in
  // the 'm_regex_info' list, _NOT_ the ones in the
  // 'm_tmp_regex_info' list.
  //
  // My intention here is to count the minimum number of
  // nodes in the regex which contains the nodes in
  // 'nodes_in_regex', and I have already moved that regex
  // from 'm_regex_info' to 'm_tmp_regex_info', so that I
  // can call the following function directly, no need to
  // perform other operations.
  unsigned int const minimum_symbols_in_regex = 
    count_minimum_number_of_nodes_in_node_range(
      nodes_in_regex.front(),
      nodes_in_regex.back());
  
  unsigned int should_duplicated_times;
  
  if (m_max_lookahead_searching_depth < minimum_symbols_in_regex)
  {
    should_duplicated_times = 0;
  }
  else
  {
    should_duplicated_times =
      (m_max_lookahead_searching_depth - minimum_symbols_in_regex) /
      minimum_symbols_in_regex;
    
    if ((m_max_lookahead_searching_depth % minimum_symbols_in_regex) != 0)
    {
      ++should_duplicated_times;
    }
  }
  
  {
    node_t *new_alternative_start_node = alternative_start_node;
    
    for (unsigned int i = 0;
         i < should_duplicated_times;
         ++i)
    {
      std::list<node_t *> new_nodes_because_dup;
      
      // The 'corresponding_node_between_fork' relationship is
      // as following:
      //
      // Ex:
      //
      // (a (b)*)*
      //
      // will become:
      //
      // (a (b)*)*
      //     ^
      //     |\__
      //     |   \
      //     v    v
      // (a (b)* (b)*)*              (2)
      //     ^    ^
      //     |    |\__
      //     |    |   \
      //     v    v    v
      // (a (b)* (b)* (b)*)*         (3)
      //          .    ..
      //
      // In order to enlarge the outer regex's range, I have
      // to find all the references to the old last node of it
      // (pointed by '.') and change them to the new one
      // (pointed by '..').
      //
      // So that I can not just use the last one in the
      // 'corresponding_node_between_fork' of 2nd node 'b' in
      // the alternative (2) to find the corresponding
      // node of it in the alternative (3), because it
      // will always point to the last node 'b' of the
      // alternative (3). If I want to find the 2nd node
      // 'b' in the alternative (3), I will need another
      // method. Ex: remember the last iterator of the
      // 'corresponding_node_between_fork' of the 2nd
      // node 'b' in the alternative (2) before the
      // forking, and then performing the forking.
      std::list<std::pair<std::list<node_t *>::iterator, bool> > orig_iters;
      
      BOOST_FOREACH(node_t * const node, nodes_in_regex)
      {
        std::list<node_t *>::iterator orig_iter;
        bool valid;
        
        orig_iter = node->corresponding_node_between_fork().end();
        
        if (node->corresponding_node_between_fork().size() != 0)
        {
          --orig_iter;
          
          valid = true;
        }
        else
        {
          valid = false;
        }
        
        orig_iters.push_back(std::pair<std::list<node_t *>::iterator, bool>(orig_iter, valid));
      }
      
      assert(orig_iters.size() == nodes_in_regex.size());
      
      fork_alternative_with_some_duplicated_nodes(
        new_alternative_start_node,
        nodes_in_regex.front(),
        nodes_in_regex.back(),
        nodes_in_regex.back(),
        new_nodes_because_dup);
      
      assert(new_nodes_because_dup.size() != 0);
      
      // The newly created alternative should be the last
      // one.
      new_alternative_start_node = rule_node->next_nodes().back();
      assert(new_alternative_start_node != alternative_start_node);
      
      (void)node_for_each(new_alternative_start_node,
                          0,
                          link_to_the_corresponding_node_in_main_regex_alternative);
      
      std::list<node_t *>::iterator nodes_in_regex_iter = nodes_in_regex.begin();
      
      typedef std::pair<std::list<node_t *>::iterator, bool> pair_type;
      
      BOOST_FOREACH(pair_type &orig_iter, orig_iters)
      {
        assert((*nodes_in_regex_iter)->corresponding_node_between_fork().size() != 0);
        
        if (true == orig_iter.second)
        {
          ++(orig_iter.first);
          
          assert(orig_iter.first != (*nodes_in_regex_iter)->corresponding_node_between_fork().end());
        }
        else
        {
          orig_iter.first = (*nodes_in_regex_iter)->corresponding_node_between_fork().begin();
        }
        
        ++nodes_in_regex_iter;
      }
      
      {
        // Originally, nodes in 'nodes_in_regex' are in the
        // original alternative. I have to update then to
        // the nodes in the new alternative to prepare for
        // the next forking.
        std::list<std::pair<std::list<node_t *>::iterator, bool> >::iterator orig_iter =
          orig_iters.begin();
        
        for (std::list<node_t *>::iterator iter = nodes_in_regex.begin();
             iter != nodes_in_regex.end();
             ++iter)
        {
          assert((*iter)->alternative_start() == alternative_start_node);
          assert((*iter) != (*iter)->corresponding_node_between_fork().back());
          
          (*iter) = *((*orig_iter).first);
          
          assert((*iter)->alternative_start() == new_alternative_start_node);
          if ((*iter)->alternative_start() != (*iter))
          {
            assert(true == new_alternative_start_node->is_my_descendant_node(*iter));
          }
        }
      }
      
      // Check if this regex is not at the beginning of this
      // alternative.
      if (nodes_in_regex.front()->alternative_start() != nodes_in_regex.front())
      {
        assert(1 == nodes_in_regex.front()->prev_nodes().size());
        
        node_t * const node_just_prior_to_regex =
          nodes_in_regex.front()->prev_nodes().front();
        
        assert(true == node_just_prior_to_regex->is_my_next_node(
                 nodes_in_regex.front()));
        
        if (new_alternative_start_node != node_just_prior_to_regex)
        {
          assert(true == new_alternative_start_node->
                 is_my_descendant_node(node_just_prior_to_regex));
        }
        
        // Ex:
        //
        // (a (b)*)*
        // ^      ^
        //
        // duplicate 'b' will become:
        //
        // (a (b)* (b)*)*
        // ^           ^
        //
        // The end node of regex pointed by '^' will change to the
        // 2nd 'b' from the 1st 'b'.
        (void)node_for_each(new_alternative_start_node,
                            node_just_prior_to_regex,
                            patch_tmp_regex_info_for_prev_nodes(
                              nodes_in_regex.back(),
                              new_nodes_because_dup.back(),
                              0));
      }
    }
  }
}

/** \page regex_expansion_page regular expression expansion
 * There are 3 things need to be done to build a new
 * alternative for this current alternative because of the
 * regular expressions.
 * 
 * \section remove_whole_regex remove the whole regular
 * expression part
 *
 * Ex:
 *
\verbatim
A (B C)* D
\endverbatim
 *
 * will become
 *
\verbatim
A D
\endverbatim
 *
 * However, I will mark this type of expansion specially,
 * because these new alternatives doesn't lead the rule
 * parsing process, they just participate the lookahead
 * searching process. For more info, see section
 * \ref build_parsing_codes.
 *
 * \section duplicate_regex_OR duplicate alternatives because of type 'OR' regular expression
 * Ex:
 *
\verbatim
A (B C | D)* E  ---------(1)
\endverbatim
 *
 * will become:
 *
\verbatim
A B C E ---------(2)
A D E   ---------(3)
\endverbatim
 *
 * I will delete the original alternative (ie. (1)), and set
 * one of the new alternatives (ie. (2), (3)) as the primary
 * one for parsing code gererating, and others will be
 * marked specially. For more info, see section
 * \ref build_parsing_codes.
 *
 * \section duplicate_whole_regex duplicate the whole regular expression multiple times
 * According to the desired lookahead depth, duplicate
 * the whole regex as many times as the value of A, and the
 * value of A is as following:
 *
 * A = ('the desired lookahead depth' - 'the number of the
 * minimum tokens of this regex') / 'the number of the
 * minimum tokens of this regex'.
 *
 * If the result has floating point, then discard the
 * floating point and add 1.
 *
 * Ex 1:
 *
\verbatim
A (B C)* D
\endverbatim
 *
 * if the current desired lookahead depth is 2, then it
 * will become (The minimum number of tokens of (B C)* is
 * 2, so I will duplicate the regex ((2 - 2) / 2) => 0 times)
 *
\verbatim
A B C D
\endverbatim
 *
 * Reason:
 *
 * The whole expansion tree of this example is:
 *
\verbatim
  A (B C)*  D
--------------------
  A  D
  A  B C    D
     ^
\endverbatim
 * 
 * If I am at the '^' point, I have to ensure that all
 * possible patterns which may follow 'B' at that point are
 * present in the lookahead searching tree.
 *
 * Ex 2:
 *
\verbatim
A (B)* D
\endverbatim
 *
 * if the current desired lookahead depth is 2, then it
 * will become (The minimum number of tokens of (B)* is
 * 1, so I will duplicate the regex ((2 - 1) / 1) => 1 times)
 *
\verbatim
A B B D
\endverbatim
 *
 * Ex 3:
 *
\verbatim
A (B C | D)* E
\endverbatim
 *
 * if the current desired lookahead depth is 2, then it
 * will become (The minimum number of tokens of (B C|D)* is
 * 1, so I will duplicate the regex ((2 - 1) / 1) => 1 times)
 *
\verbatim
A (B C | D) (B C | D) E
\endverbatim
 *
 * and it will transfer to
 *
\verbatim
A B C (B C | D)* E
A D (B C | D)* E
\endverbatim
 *
 * and it will transfer to
 *
\verbatim
A B C B C E
A B C D E
A D B C E
A D D E
\endverbatim
 *
 * However, I will mark this type of expansion specially,
 * because these new alternatives doesn't lead the rule
 * parsing process, they just participate the lookahead
 * searching process. For more info, see section
 * \ref build_parsing_codes.
 *
 * \section build_parsing_codes build parsing codes example
 * Ex:
 *
\verbatim
A (B C | D)* E
\endverbatim
 *
 * will become:
 *
\verbatim
* A E
  A B C E
* A D E
* A B C B C E
* A B C D E
* A D B C E
* A D D E
\endverbatim
 *
 * The six alternative prefixed with symbol '*' will be
 * ignored during the construction of the parsing
 * codes. The parsing codes will be as the following.
 *
 * \code
 * void
 * ddd()
 * {
 *   ensure_next_token_is(TOKEN_A);
 *   
 *   token * const token_1 = lexer_peek_token(1);
 *   switch (token_1->get_type())
 *   {
 *   case TOKEN_B:
 *   case TOKEN_D:
 *     {
 *       while (1)
 *       {
 *         bool finish = false;
 *         
 *         switch (token_1->get_type())
 *         {
 *         case TOKEN_B:
 *           add_node_to_nodes(nodes, lexer_consume_token(consume));
 *           ensure_next_token_is(TOKEN_C);
 *           break;
 *           
 *         case TOKEN_D:
 *           add_node_to_nodes(nodes, lexer_consume_token(consume));
 *           break;
 *           
 *         default:
 *           assert(0);
 *           break;
 *         }
 *         
 *         // To mark the end of one regex loop.
 *         add_node_to_nodes(nodes, 0);
 *
 *         token * const token_1 = lexer_peek_token(1);
 *         switch (token_1->get_type())
 *         {
 *         case TOKEN_B:
 *         case TOKEN_D:
 *           {
 *             continue;
 *           }
 *           break;
 *           
 *         default:
 *           {
 *             finish = true;
 *           }
 *           break;
 *         }
 *         
 *         if (true == finish)
 *         {
 *           break;
 *         }
 *       }
 *     }
 *     break;
 *     
 *   case TOKEN_E:
 *     // To mark the end of one regex loop.
 *     add_node_to_nodes(nodes, 0);
 *     break;
 *     
 *   default:
 *     throw ga_exception_t();
 *   }
 *   
 *   ensure_next_token_is(TOKEN_E);
 * }
 * \endcode
 */

/// \brief build a new alternative according to the current
/// one in order to remove or expand the regular expression
/// part.
///
/// The internal of this function could be found in
/// \ref regex_expansion_page.
///
/// \param alternative_start_node the starting node of the
/// current alternative.
/// \param need_to_delete_alternative
///
void
analyser_environment_t::check_and_fork_alternative_for_regex_node(
  node_t * const alternative_start_node,
  std::list<node_t *> &need_to_delete_alternative)
{
  node_t *node_in_this_alternative = alternative_start_node;
  node_t * const rule_node = alternative_start_node->rule_node();
  
#if defined(_DEBUG)
  // Besides rule_end_node, each node in an alternative
  // should have only one parent node.
  if (alternative_start_node != rule_node->rule_end_node())
  {
    assert(1 == alternative_start_node->prev_nodes().size());
    assert(1 == alternative_start_node->next_nodes().size());
  }
  else
  {
    assert(0 == alternative_start_node->next_nodes().size());
  }
#endif
  
  regex_info_t *prev_regex_info = 0;
  
  while (node_in_this_alternative != rule_node->rule_end_node())
  {
    // Check to see if this node is the starting node of a
    // regex.
    if (node_in_this_alternative->regex_info().size() != 0)
    {
      // I will expand the regular expression from outside
      // to inside.
      //
      // Ex:
      // A ((B | C) D)*
      // 
      // will become
      // A (B | C) D
      // A
      // 
      // and then
      // A B D
      // A C D
      // A
      // 
      // so that I will use 'regex_info().back()'
      // first.
      
      // In the duplicating stage later,
      // I want to skip this outer most regex group,
      // because I don't want to copy this regex_info
      // during the alternative forking.
      // 
      // Ex:
      // 
      // A ((B | C) D)*
      // 
      // will become
      // 
      // A
      // A (B D)*
      // A (C D)*
      // 
      // if I don't move it first, then the 'A (B D)*' will
      // expand to 'A' again, and this is wrong.
      //
      // Hence I have to move this outer most regex group
      // info into the 'tmp' one.
      //
      // Ex:
      //
      // A ((B C)* D)* F
      //
      // After the duplicating stage, I want to have
      // something similar the following one.
      //
      // A (B C)* D (B C)* D F
      node_in_this_alternative->move_one_regex_info_to_tmp();
      
      regex_info_t &regex_info =
        node_in_this_alternative->tmp_regex_info().front();
      
      check_if_regex_info_is_correct(node_in_this_alternative,
                                     regex_info);
      
      if (REGEX_TYPE_OR != regex_info.m_type)
      {
        prev_regex_info = &regex_info;
      }
      
      switch (regex_info.m_type)
      {
      case REGEX_TYPE_ZERO_OR_ONE:
      case REGEX_TYPE_ZERO_OR_MORE:
        // Ex: (A)*
        {
          std::list<node_t *> nodes_in_regex;
          
          // fill nodes_in_regex
          collect_regex_nodes(nodes_in_regex, regex_info);
          assert(nodes_in_regex.size() != 0);
          
          // patch 1:
          //
          // I will remove the whole regex, if the starting
          // node of this regex has previously handled
          // regex_info (ie. tmp_regex_info), then I have to
          // copy them (besides the first one of
          // 'tmp_regex_info', because it means the
          // currently processing regex) into the node just
          // after this regex.
          //
          // Ex:
          //
          // (((a)* b c)* d)*
          //
          // will become:
          //
          // ((b c)* d)*
          // ^^
          //
          // The starting node of the two regex_info pointed
          // by '^' will changed to 'b' from 'a'.
          assert(1 == regex_info.m_ranges.back().mp_end_node->next_nodes().size());
          
          node_t * const just_after_this_regex =
            regex_info.m_ranges.back().mp_end_node->next_nodes().front();
          
          unsigned int count = 0;
          
          // Check if some processed regex is started from
          // this node.
          if (regex_info.m_ranges.front().mp_start_node->tmp_regex_info().size() != 0)
          {
            std::list<regex_info_t>::const_reverse_iterator iter =
              regex_info.m_ranges.front().mp_start_node->tmp_regex_info().rbegin();
            
            // The first one of 'tmp_regex_info' is the
            // current using one.
            std::list<regex_info_t>::const_reverse_iterator iter_end =
              regex_info.m_ranges.front().mp_start_node->tmp_regex_info().rend();
            --iter_end;
            
            for (; iter != iter_end; ++iter)
            {
              just_after_this_regex->tmp_regex_info().push_front(*iter);
              
              regex_info_t &copied_regex_info =
                just_after_this_regex->tmp_regex_info().front();
              
              // I will handle REGEX_TYPE_OR on the fly,
              // this means this type of regex_info will not
              // exist in the 'tmp_regex_info'.
              assert(copied_regex_info.m_type != REGEX_TYPE_OR);
              
              // The type of this regex_info is
              // REGEX_TYPE_ZERO_OR_MORE or
              // REGEX_TYPE_ZERO_OR_ONE.
              assert(1 == regex_info.m_ranges.size());
              
              // patch the start & end node of this copied
              // regex_info.
              assert(copied_regex_info.m_ranges.front().mp_start_node ==
                     regex_info.m_ranges.front().mp_start_node);
              
              copied_regex_info.m_ranges.front().mp_start_node =
                just_after_this_regex;
              
              // I will not accept ((a)*)+ or ((a)*)* or
              // ((a)+)* or something like that, this type
              // of regex will result the following
              // assertion.
              assert(copied_regex_info.m_ranges.back().mp_end_node !=
                     regex_info.m_ranges.front().mp_start_node);
              
              ++count;
            }
          }
          
          // patch 2:
          //
          // Check if some processed regex_info is ended at
          // this node.
          //
          // Ex:
          //
          // (a b (c)*)*
          // ^
          // will become:
          //
          // (a b)*
          //
          // Hence the end node of the regex pointed by '^'
          // will change to 'b' from 'c'.
          std::list<regex_info_t *> changed_regex_info;
          
          node_t * const regex_start_node =
            regex_info.m_ranges.front().mp_start_node;
          
          node_t * const regex_end_node =
            regex_info.m_ranges.back().mp_end_node;
          
          {
            if (regex_start_node->alternative_start() != regex_start_node)
            {
              assert(1 == regex_start_node->prev_nodes().size());
              
              node_t * const prior_to_regex_start_node =
                regex_start_node->prev_nodes().front();
              
              node_t * const alternative_start =
                regex_start_node->alternative_start();
              
              assert(alternative_start->alternative_start() == alternative_start);
              
              (void)node_for_each(alternative_start,
                                  prior_to_regex_start_node,
                                  patch_tmp_regex_info_for_prev_nodes(
                                    regex_end_node,
                                    prior_to_regex_start_node,
                                    &changed_regex_info));
            }
          }
          
          remove_whole_regex(alternative_start_node,
                             nodes_in_regex);
          
          // roll back the above patch 1.
          for (unsigned int i = 0; i < count; ++i)
          {
            just_after_this_regex->tmp_regex_info().pop_front();
          }
          
          // roll back the above patch 2.
          {
            BOOST_FOREACH(regex_info_t * const regex_info,
                          changed_regex_info)
            {
              assert(1 == regex_info->m_ranges.size());
              
              regex_info->m_ranges.front().mp_end_node =
                regex_end_node;
            }
          }
          
          // I don't need to duplicate the whole regex if
          // the type of this regex_info is
          // REGEX_TYPE_ZERO_OR_ONE.
          if (REGEX_TYPE_ZERO_OR_MORE == regex_info.m_type)
          {
            duplicate_whole_regex(alternative_start_node,
                                  nodes_in_regex);
          }
        }
        break;
        
      case REGEX_TYPE_ONE_OR_MORE:
        // Ex: A (B C)+
        //
        // I don't need to discard any nodes in this regex
        // type.
        {
          std::list<node_t *> nodes_in_regex;
          
          // fill nodes_in_regex
          collect_regex_nodes(nodes_in_regex, regex_info);
          assert(nodes_in_regex.size() != 0);
          
          duplicate_whole_regex(alternative_start_node,
                                nodes_in_regex);
        }
        break;
        
      case REGEX_TYPE_ONE:
        // Ex: A (B C)
        //
        // I don't need to discard any nodes in this regex
        // type.
        //
        // I don't need to duplicate any nodes in this regex
        // type.
        
        // If the type of a regex is REGEX_TYPE_ONE, then
        // after regex expanding, I can remove this
        // REGEX_TYPE_ONE group.
        //
        // Ex 1:
        //
        // A (B C)
        // 
        // will becomes
        //
        // A B C
        // 
        // Ex 2:
        //
        // A ((B C | D) F)*
        // 
        // will becomes
        //
        // A (B C F)*
        // A (D F)*
        // 
        // If I don't want to copy this REGEX_TYPE_ONE, then I
        // will remove it from the 'node->regex_info()'.
        //
        // However, if this alternative is the
        // 'main_regex_alternative' and this regex_info
        // indeed contains a regex_OR_info, then I should
        // not delete this REGEX_TYPE_ONE regex_info,
        // because I will need it in the code generating
        // stage.
        if ((alternative_start_node ==
             alternative_start_node->rule_node()->main_regex_alternative()) &&
            (REGEX_TYPE_OR ==
             node_in_this_alternative->regex_info().back().m_type))
        {
          // Do nothing
        }
        else
        {
          node_in_this_alternative->tmp_regex_info().pop_front();
          
          prev_regex_info = 0;
        }
        continue;
        
      case REGEX_TYPE_OR:
        // Ex: (B C | D)
        //
        // I don't need to discard or duplicate any nodes in
        // REGEX_TYPE_OR type, because this type of regex
        // info must follow one REGEX_TYPE_ZERO_OR_MORE,
        // REGEX_TYPE_ONE_OR_MORE, REGEX_TYPE_ZERO_OR_ONE,
        // or REGEX_TYPE_ONE, and the node discarding stage
        // is done at that type of regex info.
        {
          // The newly duplicated alternative doesn't need
          // to inherit the REGEX_TYPE_OR regexs from its
          // mother, thus I will remove this kind of regex,
          // and then do the forking.
          //
          // However, if I am handling the
          // 'main_regex_alternative', then this
          // REGEX_TYPE_OR regex will be added into the rule
          // node, and the prev regex will point to it, so
          // that in the code generating stage later, I
          // would have enough information to emit codes
          // according to the regex_OR_group.
          //
          // The relationship between the current regex and
          // the previous one is as following:
          //
          // A (B C | D)*
          //
          // I will split the above regex to 2 regex_info,
          // the one is for ()*, and the other is for (|).
          // The one for regex_OR_group is in front of the
          // one for regex_group. Thus, when I handle the
          // regex_info from back() to begin(), I will
          // handle regex_group first, and then
          // regex_OR_group later. Thus, when I handle a
          // REGEX_TYPE_OR regex, the previous one should be
          // its mother regex.
          
          regex_info_t tmp = regex_info;
          
          node_in_this_alternative->tmp_regex_info().pop_front();
          
          assert(alternative_start_node->rule_node() != 0);
          assert(alternative_start_node->rule_node()->main_regex_alternative() != 0);
          
          if (alternative_start_node ==
              alternative_start_node->rule_node()->main_regex_alternative())
          {
            alternative_start_node->rule_node()->regex_OR_info().push_back(tmp);
            
            assert(prev_regex_info != 0);
            
            prev_regex_info->mp_regex_OR_info =
              &(alternative_start_node->rule_node()->regex_OR_info().back());
            
            prev_regex_info = 0;
          }
          
          assert(tmp.m_ranges.front().mp_start_node == node_in_this_alternative);
          
          handle_regex_OR_stmt(alternative_start_node,
                               need_to_delete_alternative,
                               tmp);
          
          // I have already delete this alternative (note:
          // in handle_regex_OR_stmt), thus I can just
          // return from this function now.
          return;
        }
        break;
        
      default:
        assert(0);
        break;
      }
    }
    else
    {
      // Check to see if this node is the starting node of
      // another regex.
      //
      // Ex:
      //
      // A ((B C)* D)*
      //     ^
      //
      // If not, then advance to the next node.
      assert(1 == node_in_this_alternative->next_nodes().size());
      
      node_in_this_alternative = node_in_this_alternative->next_nodes().front();
      
      // I move to a new node, thus I have to clear the
      // 'prev_regex_info'.
      prev_regex_info = 0;
    }
  }
}

void
analyser_environment_t::build_new_alternative_for_regex_nodes(
  node_t * const alternative_start)
{
  node_t * const rule_node = alternative_start->rule_node();
  
  // A special behavior of std::list is that when the
  // insertion or deletion operation doesn't touch the
  // node, then the iterator which points to that node will
  // still be valid.
  // 
  // And after 'check_and_fork_alternative_for_regex_node',
  // there will be new alternatives created and are
  // appended after the end of the original alternative
  // list, so that I will remember the iterator which
  // points to the origianl end, and use that as the
  // starting iterator to iterate all newly created
  // alternatives.
  std::list<node_t *>::const_iterator iter = rule_node->next_nodes().end();
  --iter;
  
  std::list<node_t *> need_to_delete_alternative;
  
  check_and_fork_alternative_for_regex_node(alternative_start,
                                            need_to_delete_alternative);
  
  ++iter;
  
  // check newly created productions.
  for (; iter != rule_node->next_nodes().end(); ++iter)
  {
    check_and_fork_alternative_for_regex_node(*iter,
                                              need_to_delete_alternative);
  }
    
  // Remove the alternatives which should be deleted. For
  // example:
  //
  // A (B | C)* D
  //
  // will become:
  //
  // A D
  // A B D
  // A C D
  //
  // the first 'A (B | C)* D' should be deleted now (If it
  // is not the 'main_regex_alternative' of this rule).
#if defined(_DEBUG)
  if (rule_node->rule_contain_regex() != RULE_CONTAIN_NO_REGEX)
  {
    assert(rule_node->main_regex_alternative() != 0);
  }
  else
  {
    assert(0 == rule_node->main_regex_alternative());
  }
#endif
  
  if (RULE_CONTAIN_REGEX_OR == rule_node->rule_contain_regex())
  {
    assert(rule_node->main_regex_alternative() != 0);
    
#if defined(_DEBUG)
    bool find = false;
#endif
    
    // Search 'need_to_delete_alternative' to find if one of
    // them is the 'main_regex_alternative'. If finded, then
    // I can not delete this alternative, hence, I should
    // remove it from the 'need_to_delete_alternative'.
    for (std::list<node_t *>::iterator iter = need_to_delete_alternative.begin();
         iter != need_to_delete_alternative.end();
         )
    {
      assert((*iter)->alternative_start() == (*iter));
      
      if ((*iter) == rule_node->main_regex_alternative())
      {
#if defined(_DEBUG)
        find = true;
#endif
        
        iter = need_to_delete_alternative.erase(iter);
        
        // The only possibility that I would remove
        // 'main_regex_alternative' is that this alternative
        // contains regex OR statement, hence, when I still
        // keep this alternative, I should remove it from
        // the 'rule_node->next_nodes()' because this
        // alternative will not participate with the
        // lookahead finding process.
        
        // Break the link between the rule node and the
        // starting node of 'main_regex_alternative.
        (void)(rule_node->break_append_node(rule_node->main_regex_alternative()));
        rule_node->main_regex_alternative()->break_prepend_node(rule_node);
        
        node_t * const alternative_end = find_alternative_end(rule_node->main_regex_alternative());
        
        rule_node->rule_end_node()->break_prepend_node(alternative_end);
        
        // In many places, I use the 'rule_end_node' to
        // indicate that I am reaching the end of one
        // alternative, thus I have to preserve the one-way
        // link between the 'alternative_end' and the
        // rule_end_node, such that I can reach the
        // rule_end_node through this alternative. Hence I
        // have to comment out the following line.
        //
        //alternative_end->break_append_node(rule_node->rule_end_node());
      }
      else
      {
        ++iter;
      }
    }
    
#if defined(_DEBUG)
    assert(true == find);
#endif
  }
  
  ::remove_alternatives(this,
                        need_to_delete_alternative,
                        false,
                        false,
                        true);
}

namespace
{
  bool
  restore_each_node_regex_info(
    analyser_environment_t const * const,
    node_t * const node,
    void *)
  {
    node->restore_regex_info();
    
    return true;
  }
  
  bool
  restore_each_node_regex_info_for_regex_main_alternative(
    analyser_environment_t const * const,
    node_t * const node,
    void *param)
  {
    assert(true == node->is_rule_head());
    
    switch (node->rule_contain_regex())
    {
    case RULE_CONTAIN_NO_REGEX:
      // I don't have any regex alternatives needed to be
      // done here.
      assert(0 == node->main_regex_alternative());
      break;
      
    case RULE_CONTAIN_REGEX:
      // Because the regex main alternative of this kind of
      // rule is mixed with 'rule_node->next_nodes()', thus
      // I don't need to expand it specially.
      assert(node->main_regex_alternative() != 0);
      break;
      
    case RULE_CONTAIN_REGEX_OR:
      // The regex main alternative is moved to
      // 'rule_node->main_regex_alternative' from
      // 'rule_node->next_nodes()', thus I have to handle it
      // specially.
      assert(node->main_regex_alternative() != 0);
      
      restore_regex_info_for_one_alternative(node->main_regex_alternative());
      break;
    }
    
    return true;
  }
}

void
analyser_environment_t::restore_regex_info() const
{
  traverse_all_nodes(restore_each_node_regex_info,
                     restore_each_node_regex_info_for_regex_main_alternative,
                     0);
}

class push_front_regex_info : public std::unary_function<node_t, bool>
{
private:
  
  node_t * const mp_start_node;
  node_t * const mp_end_node;
  regex_info_t * const mp_regex_OR_info;
  regex_type_t const m_type;
  
public:
  
  push_front_regex_info(
    node_t * const start_node,
    node_t * const end_node,
    regex_info_t * const regex_OR_info,
    regex_type_t const type)
    : mp_start_node(start_node),
      mp_end_node(end_node),
      mp_regex_OR_info(regex_OR_info),
      m_type(type)
  { }
  
  bool
  operator()(
    node_t *node)
  {
    node->regex_info().push_front(regex_info_t(mp_start_node, mp_end_node, mp_regex_OR_info, m_type));
    
    return true;
  }
};

namespace
{
  /// This function has to be called after optional & regex
  /// nodes expansion.
  ///
  /// After the regex expansion, it shouldn't have any
  /// regex_OR info in the regex_info of each node. However,
  /// because I have to handle the 'main_regex_alternative'
  /// of the alternative with RULE_CONTAIN_REGEX_OR type,
  /// and it will contain regex_OR info, thus this function
  /// have to handle regex_OR info in this function, too.
  ///
  /// The way this function handles regex_OR info is to
  /// move it from the 'regex_info' list to the rule node
  /// and the original outer regex will link to it. In the
  /// reading grammar stage, it will spilt the regex_OR info
  /// from its original outer regex for handling the regex
  /// expansion convinently. But now, this function will
  /// link them again.
  ///
  /// The reason why I need to move it to the rule node is
  /// because I will move the regex_info elements from
  /// 'regex_info' list to 'tmp_regex_info' list and
  /// back. So that the address of every regex_info elements
  /// will change. Hence, I have to move the regex_OR_info
  /// into one place and then take its address.
  ///
  /// This function will not fill regex_OR info to every
  /// nodes this regex group contains.
  ///
  /// \param ae useless, can be 0.
  /// \param node 
  /// \param param useless, can be 0.
  ///
  /// \return 
  ///
  bool
  fill_regex_info_to_relative_nodes(
    analyser_environment_t const * const,
    node_t * const node,
    void *param)
  {
    // This variable is used for the regex_OR combination.
    regex_info_t *prev_regex_info = 0;
    
    // From the end to the beginning.
    for (std::list<regex_info_t>::reverse_iterator iter = node->tmp_regex_info().rbegin();
         iter != node->tmp_regex_info().rend();
         )
    {
      assert((*iter).m_type != REGEX_TYPE_OR);
      
      assert(1 == (*iter).m_ranges.size());
      
      regex_range_t const &regex_range = (*iter).m_ranges.front();
      
      if (regex_range.mp_start_node == regex_range.mp_end_node)
      {
        // If this range only contains one node, then I
        // don't need to expand it.
        assert(node == regex_range.mp_start_node);
          
        prev_regex_info = 0;
      }
      else
      {
        if (node == regex_range.mp_start_node)
        {
          // If this is the start node of this regex
          // group.
          node_t *curr_node = regex_range.mp_start_node;
            
          assert(curr_node != 0);
          assert(curr_node->name().size() != 0);
          
          assert(1 == curr_node->next_nodes().size());
          
          curr_node = curr_node->next_nodes().front();
          assert(curr_node->name().size() != 0);
          
          (void)node_for_each(curr_node,
                              regex_range.mp_end_node,
                              ::push_front_regex_info(regex_range.mp_start_node,
                                                      regex_range.mp_end_node,
                                                      (*iter).mp_regex_OR_info,
                                                      (*iter).m_type));
          
          prev_regex_info = &(*iter);
        }
        else
        {
          prev_regex_info = 0;
        }
      }
      
      ++iter;
    }
    
    return true;
  }
  
  bool fill_regex_info_to_relative_nodes_for_node_each(
    node_t * const node)
  {
    fill_regex_info_to_relative_nodes(0, node, 0);
    
    return true;
  }
  
  bool
  move_all_regex_info_and_split_regex_OR_to_tmp_front(
    node_t * const node)
  {
    node->tmp_regex_info().splice(
      node->tmp_regex_info().begin(),
      node->regex_info());
    
    bool see_a_regex_OR = false;
    
    for (std::list<regex_info_t>::iterator iter = node->tmp_regex_info().begin();
         iter != node->tmp_regex_info().end();
         )
    {
      if (REGEX_TYPE_OR == (*iter).m_type)
      {
        assert(false == see_a_regex_OR);
        
        assert(node->rule_node() != 0);
        
        node->rule_node()->regex_OR_info().push_back(*iter);
        
        iter = node->tmp_regex_info().erase(iter);
        
        see_a_regex_OR = true;
      }
      else
      {
        if (true == see_a_regex_OR)
        {
          assert(node->rule_node() != 0);
          
          (*iter).mp_regex_OR_info = 
            &(node->rule_node()->regex_OR_info().back());
          
          see_a_regex_OR = false;
        }
        
        ++iter;
      }
    }
    
    return true;
  }
  
  bool
  fill_regex_info_to_relative_nodes_for_regex_main_alternative(
    analyser_environment_t const * const,
    node_t * const node,
    void *param)
  {
    assert(true == node->is_rule_head());
    
    switch (node->rule_contain_regex())
    {
    case RULE_CONTAIN_NO_REGEX:
      // I don't have any regex alternatives needed to be
      // done here.
      assert(0 == node->main_regex_alternative());
      break;
      
    case RULE_CONTAIN_REGEX:
      // Because the regex main alternative of this kind of
      // rule is mixed with 'rule_node->next_nodes()', thus
      // I don't need to expand it specially.
      assert(node->main_regex_alternative() != 0);
      break;
      
    case RULE_CONTAIN_REGEX_OR:
      // The regex main alternative is moved to
      // 'rule_node->main_regex_alternative' from
      // 'rule_node->next_nodes()', thus I have to handle it
      // specially.
      assert(node->main_regex_alternative() != 0);
      
      (void)node_for_each(node->main_regex_alternative(),
                          0,
                          move_all_regex_info_and_split_regex_OR_to_tmp_front);
      
      (void)node_for_each(node->main_regex_alternative(),
                          0,
                          fill_regex_info_to_relative_nodes_for_node_each);
      break;
    }
    
    return true;
  }
}

/// This function has to be called after regex nodes expansion.
void
analyser_environment_t::fill_regex_info_to_relative_nodes_for_each_regex_group() const
{
  traverse_all_nodes(fill_regex_info_to_relative_nodes,
                     fill_regex_info_to_relative_nodes_for_regex_main_alternative,
                     0);
}

bool
move_all_regex_info_to_tmp_front(
  node_t * const node)
{
  node->tmp_regex_info().splice(
    node->tmp_regex_info().begin(),
    node->regex_info());
  
  return true;
}

bool
move_all_tmp_regex_info_to_orig_end(
  node_t * const node)
{
  node->regex_info().splice(
    node->regex_info().end(),
    node->tmp_regex_info());
  
  return true;
}

/// Ex:
///
/// a ((b c)* d)
///   ^        ^
///
/// The possible starting node of the regex pointed by '^'
/// not only contains 'b', but also contains 'd', because
/// regex '(b c)*' may be skipped due to the type of it is
/// REGEX_TYPE_ZERO_OR_MORE.
///
/// \param node 
/// \param regex_info 
///
/// \return 
///
bool
check_if_this_node_is_the_possible_starting_node_of_a_regex(
  node_t const * const node,
  regex_info_t const * const regex_info)
{
  assert(regex_info->m_type != REGEX_TYPE_OR);
  assert(1 == regex_info->m_ranges.size());
  
  assert(true == check_if_node_in_node_range(
           node,
           regex_info->m_ranges.front().mp_start_node,
           regex_info->m_ranges.front().mp_end_node));
  
  node_t const * const regex_start_node = regex_info->m_ranges.front().mp_start_node;
  
  regex_info_t const * const inner_regex = 
    regex_start_node->find_regex_info_is_started_by_me<find_outer_most>(regex_info);
  
  if (0 == inner_regex)
  {
    // If there are no other inner regexes, then whether
    // if this 'node' is the possible starting node
    // of this regex is depend on the position of it.
    //
    // Ex:
    //
    // (a b c)*
    //
    // only 'a' is the possible starting node.
    if (regex_start_node == node)
    {
      return true;
    }
    else
    {
      return false;
    }
  }
  else
  {
    // On the other hands, if there are inner regexes,
    // then the number of starting nodes may vary.
    //
    // Ex:
    //
    // ((a b)* c)*
    //
    // both 'a' and 'c' are the possible starting nodes.
    assert(1 == inner_regex->m_ranges.size());
    assert(regex_start_node == inner_regex->m_ranges.front().mp_start_node);
    
    while (1)
    {
      if (true == check_if_node_in_node_range(
            node,
            inner_regex->m_ranges.front().mp_start_node,
            inner_regex->m_ranges.front().mp_end_node))
      {
        // If the node is also in the more inner regex, then
        // the answer of this function is the answer of this
        // function with the more inner regex.
        //
        // Ex:
        //
        // ((a b)* c)*
        // ^#   #   ^
        //
        // Whether the node 'a' or 'b' are the possible
        // starting node of regex pointed by '^' is
        // corresponding to whether them are the possible
        // starting node of regex pointed by '#'.
        return check_if_this_node_is_the_possible_starting_node_of_a_regex(node, inner_regex);
      }
      else
      {
        switch (inner_regex->m_type)
        {
        case REGEX_TYPE_ONE:
          // Ex:
          //
          // ((a b) c)*
          // ^       ^
          //
          // 'c' can not be the possible starting node of
          // the regex pointed by '^', because there is a
          // REGEX_TYPE_ONE regex in front of it.
        case REGEX_TYPE_ONE_OR_MORE:
          // Ex:
          //
          // ((a b)+ c)*
          // ^        ^
          //
          // 'c' can not be the possible starting node of
          // the regex pointed by '^', because there is a
          // REGEX_TYPE_ONE_OR_MORE regex in front of it.
          return false;
          
        case REGEX_TYPE_ZERO_OR_MORE:
        case REGEX_TYPE_ZERO_OR_ONE:
          // Ex:
          //
          // ((a b)* c)*
          // ((a b)? c)*
          {
            assert(1 == inner_regex->m_ranges.size());
            assert(1 == inner_regex->m_ranges.front().mp_end_node->next_nodes().size());
            
            node_t const * const next_node = inner_regex->m_ranges.front().mp_end_node->next_nodes().front();
            
            regex_info_t const * const next_node_inner_regex = 
              next_node->find_regex_info_is_started_by_me<find_outer_most>(0);
            
            if (0 == next_node_inner_regex)
            {
              if (next_node == node)
              {
                return true;
              }
              else
              {
                return false;
              }
            }
            else
            {
              return check_if_this_node_is_the_possible_starting_node_of_a_regex(node, next_node_inner_regex);
            }
          }
          break;
          
        case REGEX_TYPE_OR:
        default:
          assert(0);
          break;
        }
      }
    }
  }
  
  assert(0);
}

std::list<regex_info_t>::const_reverse_iterator
find_regex_info_using_its_begin_and_end_node(
  std::list<regex_info_t> &regex_info_list,
  node_t * const start_node,
  node_t * const end_node)
{
  for (std::list<regex_info_t>::const_reverse_iterator iter = regex_info_list.rbegin();
       iter != regex_info_list.rend();
       ++iter)
  {
    if (((*iter).m_ranges.front().mp_start_node == start_node) &&
        ((*iter).m_ranges.front().mp_end_node == end_node))
    {
      return iter;
    }
  }
  
  return regex_info_list.rend();
}

/// Ex:
///
/// a (b c | d e) f
///
/// The immediate starting node of a regex includes 'b',
/// 'd'.
///
/// \return 
///
regex_range_t const *
find_regex_range_which_this_node_belongs_to(
  regex_info_t const * const regex_info,
  node_t const * const node,
  bool * const is_regex_OR_range)
{
  assert(regex_info != 0);
  assert(node != 0);
  
  if (regex_info->mp_regex_OR_info != 0)
  {
    BOOST_FOREACH(regex_range_t const &regex_range,
                  regex_info->mp_regex_OR_info->m_ranges)
    {
      if (true == check_if_node_in_node_range(
            node,
            regex_range.mp_start_node,
            regex_range.mp_end_node))
      {
        if (is_regex_OR_range != 0)
        {
          (*is_regex_OR_range) = true;
        }
        
        return &regex_range;
      }
    }
  }
  else
  {
    assert(1 == regex_info->m_ranges.size());
    
    if (true == check_if_node_in_node_range(
          node,
          regex_info->m_ranges.front().mp_start_node,
          regex_info->m_ranges.front().mp_end_node))
    {
      if (is_regex_OR_range != 0)
      {
        (*is_regex_OR_range) = false;
      }
      
      return &(regex_info->m_ranges.front());
    }
  }
  
  return 0;
}

void
find_possible_starting_nodes_for_regex(
  node_t * const node,
  std::list<regex_info_t>::const_reverse_iterator const node_regex_info_iter,
  std::list<node_t *> &nodes)
{
  assert(node != 0);
  
  for (std::list<regex_info_t>::const_reverse_iterator iter = node_regex_info_iter;
       iter != node->regex_info().rend();
       ++iter)
  {
    // If a regex is started from a node 'A', then every
    // smaller regex which is in this regex and contains
    // node 'A' must be started from node 'A'.
    assert((*iter).m_ranges.front().mp_start_node == node);
    
    // Add the trivial one.
    add_to_set_unique(nodes, (*iter).m_ranges.front().mp_start_node);
    
    std::list<regex_info_t>::const_reverse_iterator iter2 = iter;
    ++iter2;
    
    if (iter2 != node->regex_info().rend())
    {
      switch ((*iter2).m_type)
      {
      case REGEX_TYPE_ONE_OR_MORE:
      case REGEX_TYPE_ONE:
        break;
        
      case REGEX_TYPE_ZERO_OR_MORE:
      case REGEX_TYPE_ZERO_OR_ONE:
        {
          // Ex:
          //
          // ((b c)? d)*
          // ((b c)* d)*
          //         ^
          //
          // Besides node 'b', node 'd' is the starting node
          // of the outer regex, too.
          assert((*iter2).m_ranges.front().mp_end_node->name().size() != 0);
          assert(1 == (*iter2).m_ranges.front().mp_end_node->next_nodes().size());
          assert((*iter2).m_ranges.front().mp_end_node->next_nodes().front()->name().size() != 0);
          
          node_t * const possible_starting_node = 
            (*iter2).m_ranges.front().mp_end_node->next_nodes().front();
          
          // Check if the following situation:
          //
          // ((a)* | b)
          //
          // The possible_starting_node is 'b' now, and it
          // will be covered by the codes below to handle
          // regex_OR, not here.
          if (true == check_if_this_node_is_the_starting_one_of_a_regex_OR_range(
                &(*iter),
                possible_starting_node))
          {
            // Do nothing.
          }
          else
          {
            add_to_set_unique(nodes, possible_starting_node);
            
            // Ex:
            //
            // ((b c)* (d e)* f)*
            // ^               ^
            //
            // The starting node of the regex pointed by '^'
            // are 'b', 'd' & 'f'. Hence, I have to
            // recursively find the starting nodes of the
            // following regexes until I find a node which
            // doesn't belong to any inner regex relative to
            // this one.
            //
            // Ex:
            //
            // ((b c)* g (d e)* f)*
            // ^                 ^
            std::list<regex_info_t>::const_reverse_iterator new_iter =
              find_regex_info_using_its_begin_and_end_node(
                possible_starting_node->regex_info(),
                (*iter).m_ranges.front().mp_start_node,
                (*iter).m_ranges.front().mp_end_node);
            
            assert(new_iter != possible_starting_node->regex_info().rend());
            
            ++new_iter;
            
            // At this point, 'new_iter' may equal to
            // 'possible_starting_node->regex_info().rend()',
            // however, the following recursively called
            // 'find_possible_starting_nodes_for_regex'
            // would handle this situation.
            find_possible_starting_nodes_for_regex(possible_starting_node,
                                                   new_iter,
                                                   nodes);
          }
        }
        break;
        
      default:
        assert(0);
        break;
      }
    }
    
    if ((*iter).mp_regex_OR_info != 0)
    {
      // Ex:
      //
      // "A"
      // : ("B" ("d" "e")* | "c")* "a"
      //   ^                    ^
      // ;
      //
      // The starting nodes of the regex pointed by '^' are
      // 'B' & 'c'.
      assert((*iter).mp_regex_OR_info->m_ranges.size() != 0);
      assert((*iter).mp_regex_OR_info->m_ranges.front().mp_start_node ==
             (*iter).m_ranges.front().mp_start_node);
      
      // Find the starting nodes of each regex_OR range.
      std::list<regex_range_t>::iterator OR_range_iter = (*iter).mp_regex_OR_info->m_ranges.begin();
      
      // Skip the first range, because it is already handled
      // by above codes.
      ++OR_range_iter;
      
      for (;
           OR_range_iter != (*iter).mp_regex_OR_info->m_ranges.end();
           ++OR_range_iter)
      {
        node_t * const possible_starting_node = (*OR_range_iter).mp_start_node;
        
        add_to_set_unique(nodes, possible_starting_node);
        
        std::list<regex_info_t>::const_reverse_iterator new_iter =
          find_regex_info_using_its_begin_and_end_node(
            possible_starting_node->regex_info(),
            (*iter).m_ranges.front().mp_start_node,
            (*iter).m_ranges.front().mp_end_node);
        
        ++new_iter;
        
        find_possible_starting_nodes_for_regex(possible_starting_node,
                                               new_iter,
                                               nodes);
      }
    }
  }
}

bool check_if_this_node_is_the_starting_one_of_a_regex_OR_range(
  regex_info_t const * const regex_info,
  node_t const * const node)
{
  assert(regex_info != 0);
  assert(node != 0);
  
  if (0 == regex_info->mp_regex_OR_info)
  {
    return false;
  }
  else
  {
    BOOST_FOREACH(regex_range_t const &regex_range,
                  regex_info->mp_regex_OR_info->m_ranges)
    {
      if (regex_range.mp_start_node == node)
      {
        return true;
      }
    }
    
    return false;
  }
  
  return true;
}

void
find_possible_end_nodes_for_regex(
  node_t * const node,
  std::list<regex_info_t>::const_reverse_iterator const node_regex_info_iter,
  std::list<node_t *> &nodes)
{
  assert(node != 0);
  assert((*node_regex_info_iter).m_ranges.front().mp_start_node == node);
  
  switch ((*node_regex_info_iter).m_type)
  {
  case REGEX_TYPE_ONE:
  case REGEX_TYPE_ZERO_OR_ONE:
    break;
    
  case REGEX_TYPE_ONE_OR_MORE:
  case REGEX_TYPE_ZERO_OR_MORE:
    {
      // Ex:
      //
      // ("d" "e")*
      //
      // can be expended to
      //
      // ("d" "e") ("d" "e")
      //
      // Hence, the starting nodes of this regex are the end
      // nodes of this one, too.
      find_possible_starting_nodes_for_regex(
        node,
        node_regex_info_iter,
        nodes);
    }
    break;
    
  case REGEX_TYPE_OR:
  default:
    assert(0);
    break;
  }
  
  // Check if this regex is in an outer regex which
  // contains regex_OR.
  //
  // Ex 1:
  //
  // "A"
  // : ("B" ("d" "e")* | "c")* "a"
  //        ^       ^
  // ;
  //
  // The end node of the regex pointed by '^' is node 'a'
  // rather than node 'c'. (The entire end nodes set are
  // {a, c, B}, node 'c' here is because it is the starting
  // node of the outer regex, not because it is the next
  // node after this regex).
  //
  // Ex 2:
  //
  // "A"
  // : ("B" ("d" "e")*)* "a"
  //        ^       ^
  // ;
  //
  // The end node of the regex pointed by '^' is node 'a'
  // & 'B'.
  
  std::list<regex_info_t>::const_reverse_iterator iter2 = node_regex_info_iter;
  
  bool be_the_end_of_this_regex;
  
  if (node_regex_info_iter == node->regex_info().rbegin())
  {
    // There is no other regex, then add the trivial one.
    //
    // Ex:
    //
    // "A"
    // : "B" ("d" "e")* "a"
    //       ^       ^
    be_the_end_of_this_regex = false;
  }
  else
  {
    // Find the outer regex.
    --iter2;
    
    if (0 == (*iter2).mp_regex_OR_info)
    {
      // If this outer regex doesn't have any regex_OR
      // range.
      
      if ((*iter2).m_ranges.front().mp_end_node != node)
      {
        // If there are other nodes between the end of the
        // outer regex and this one.
        //
        // Add the trivial one.
        //
        // Ex:
        //
        // "A"
        // : ("B" ("d" "e")* "f")* "a"
        //        ^       ^
        // ;
        be_the_end_of_this_regex = false;
      }
      else
      {
        // Ex 1:
        //
        // "A"
        // : ("B" ("d" "e")*)* "a"
        //        ^       ^
        // ;
        //
        // The end node of the regex pointed by '^' are
        // node 'a', 'B'.
        //
        // Ex 2:
        //
        // "A"
        // : ("C" ("B" ("d" "e")*)*)* "a"
        //             ^       ^
        // ;
        //
        // The end node of the regex pointed by '^' are
        // node 'a', 'B' & 'C'.
        be_the_end_of_this_regex = true;
      }
    }
    else
    {
      // If this outer regex have regex_OR ranges.
      
      assert((*node_regex_info_iter).m_ranges.front().mp_end_node != 0);
      assert(1 == (*node_regex_info_iter).m_ranges.front().mp_end_node->next_nodes().size());
      
      node_t const * const possible_end_node =
        (*node_regex_info_iter).m_ranges.front().mp_end_node->next_nodes().front();
      
      assert(possible_end_node != 0);
      
      if (((*iter2).m_ranges.front().mp_end_node != possible_end_node) &&
          (false == check_if_this_node_is_the_starting_one_of_a_regex_OR_range(
            &(*iter2),
            possible_end_node)))
      {
        // Ex 1:
        //
        // "A"
        // : ("B" ("d" "e")* "f" | "g")* "a"
        //        ^       ^
        // ;
        //
        // Ex 2:
        //
        // "A"
        // : ("B" "f" | ("d" "e")* "g")* "a"
        //              ^       ^
        // ;
        //
        // Add the trivial one.
        be_the_end_of_this_regex = false;
      }
      else
      {
        // Ex 1:
        //
        // "A"
        // : ("B" ("d" "e")* | "g")* "a"
        //        ^       ^
        // ;
        //
        // Ex 2:
        //
        // "A"
        // : ("B" "f" | ("d" "e")*)* "a"
        //              ^       ^
        // ;
        be_the_end_of_this_regex = true;
      }
    }
  }
  
  if (false == be_the_end_of_this_regex)
  {
    assert((*node_regex_info_iter).m_ranges.front().mp_end_node != 0);
    assert(1 == (*node_regex_info_iter).m_ranges.front().mp_end_node->next_nodes().size());
    
    add_to_set_unique(nodes, (*node_regex_info_iter).m_ranges.front().mp_end_node->next_nodes().front());
  }
  else
  {
    node_t * const outer_regex_starting_node =
      (*iter2).m_ranges.front().mp_start_node;
        
    std::list<regex_info_t>::const_reverse_iterator new_iter =
      find_regex_info_using_its_begin_and_end_node(
        outer_regex_starting_node->regex_info(),
        (*iter2).m_ranges.front().mp_start_node,
        (*iter2).m_ranges.front().mp_end_node);
        
    switch ((*iter2).m_type)
    {
    case REGEX_TYPE_ONE:
    case REGEX_TYPE_ZERO_OR_ONE:
      break;
          
    case REGEX_TYPE_ZERO_OR_MORE:
    case REGEX_TYPE_ONE_OR_MORE:
      find_possible_starting_nodes_for_regex(
        outer_regex_starting_node,
        new_iter,
        nodes);
      break;

    case REGEX_TYPE_OR:
    default:
      assert(0);
      break;
    }
        
    find_possible_end_nodes_for_regex(
      outer_regex_starting_node,
      new_iter,
      nodes);
  }
}

void
collect_candidate_nodes_for_lookahead_calculation_for_EBNF_rule(
  node_t const * const rule_node,
  std::list<regex_info_t>::const_reverse_iterator iter,
  std::list<node_t *> &nodes)
{
  assert(rule_node != 0);
  assert(true == rule_node->is_rule_head());
  
  node_t * const main_regex_alternative = rule_node->main_regex_alternative();
  assert(main_regex_alternative != 0);
  rule_contain_regex_t const rule_regex_type = rule_node->rule_contain_regex();
  
  /////////////////////////////////////////
  // Collect regex_first_node.          
  {
    std::list<node_t *> regex_start_nodes;
    
    find_possible_starting_nodes_for_regex(
      (*iter).m_ranges.front().mp_start_node,
      iter,
      regex_start_nodes);
    
    BOOST_FOREACH(node_t * const regex_start_node, regex_start_nodes)
    {
      BOOST_FOREACH(node_t * const node,
                    regex_start_node->refer_to_me_nodes_in_regex_expansion_alternatives())
      {
        if ((node->alternative_start() == main_regex_alternative) &&
            (RULE_CONTAIN_REGEX_OR == rule_regex_type))
        {
        }
        else
        {
          nodes.push_back(node);
        }
      }
      
      // The lookahead tree of 'regex_start_node' could
      // be empty.
      //
      // Ex:
      //
      // a ((b | c) d)*
      //
      // Because this rule contains REGEX_TYPE_OR regex,
      // thus I have to perform regex_OR_expansion for
      // this rule. As a result, the origianl
      // alternative will not participate in the
      // lookahead searching process, thus, the
      // lookahead tree of the 'regex_start_node' in the
      // original alternative will be empty.
      if ((regex_start_node->alternative_start() == main_regex_alternative) &&
          (RULE_CONTAIN_REGEX_OR == rule_regex_type))
      {
      }
      else
      {
        nodes.push_back(regex_start_node);
      }
    }
  }
  
  /////////////////////////////////////////
  // Collect regex_end_node.
  {
    std::list<node_t *> regex_end_nodes;
    
    find_possible_end_nodes_for_regex(
      (*iter).m_ranges.front().mp_start_node,
      iter,
      regex_end_nodes);
    
    BOOST_FOREACH(node_t * const regex_end_node, regex_end_nodes)
    {
      BOOST_FOREACH(node_t * const node,
                    regex_end_node->refer_to_me_nodes_in_regex_expansion_alternatives())
      {
        // The reason why the lookahead tree of some
        // relative node of the 'regex_end_node' could be
        // 0 can see the below example.
        //
        // Ex:
        //
        // "A"
        // : "a" ("B" | "c")* "b" "c"
        // ;                   ^
        // 
        // "B"
        // : "e"
        // | "d"
        // ;
        //
        // will becomes:
        //
        // A[0] A_END[1]
        //   0) a[2] b[3][b;] c[4] 
        //   1) a[5] (B[6].)*[e;d;] b[7][b;] c[8] 
        //   2) a[9] (c[10].)*[c;] b[11][b;] c[12] 
        //   3) a[13] (B[14].)*[e;d;] (B[15].)*[e;d;] b[16] c[17] 
        //                                            ^
        //   4) a[18] (B[19].)*[e;d;] (c[20].)*[c;] b[21] c[22] 
        //   5) a[23] (c[24].)*[c;] (B[25].)*[e;d;] b[26] c[27] 
        //   6) a[28] (c[29].)*[c;] (c[30].)*[c;] b[31] c[32] 
        // 
        // B[33] B_END[34]
        //   0) e[35][e;] 
        //   1) d[36][d;] 
        if ((node->alternative_start() == main_regex_alternative) &&
            (RULE_CONTAIN_REGEX_OR == rule_regex_type))
        {
        }
        else
        {
          nodes.push_back(node);
        }
      }
      
      // 'regex_end_node' in the main regex alternative
      // may not have any lookahead info attached to it.
      //
      // Ex:
      //
      // Origianl grammar:
      //
      // "A"
      // : "a" ("B" "B")* "b" "c"
      // ;
      //
      // The lookahead-node relationship is:
      //
      // A[0] A_END[1]
      //   0) a[2] (B[3][b,b;b,d;d,b;d,d;] B[4])* b[5] c[6] 
      //   1) a[7] b[8][b,c;] c[9]                ^
      // 
      // The 'regex_end_node' in the main regex
      // alternative (pointed by '^') doesn't have any
      // lookahead into attached to it.
      if ((regex_end_node->alternative_start() == main_regex_alternative) &&
          (RULE_CONTAIN_REGEX_OR == rule_regex_type))
      {
      }
      else
      {
        nodes.push_back(regex_end_node);
      }
    }
  }
}

bool
check_if_this_node_is_after_this_regex(
  node_t const * const node,
  regex_info_t const * const regex_info)
{
  assert(node != 0);
  assert(regex_info != 0);
  
  node_t const * const first_node_after_this_regex = 
    regex_info->m_ranges.front().mp_end_node->next_nodes().front();
  
  if (true == check_if_node_in_node_range(
        node,
        first_node_after_this_regex,
        0))
  {
    return true;
  }
  else
  {
    return false;
  }
}
