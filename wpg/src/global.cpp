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

void
report(std::wfstream * const out_file,
       wchar_t const * const fmt, ...)
{
  va_list ap;
  wchar_t *str;
  
  va_start(ap, fmt);
  str = fmtstr_new_valist(fmt, &ap);
  va_end(ap);
  
  if (0 == out_file)
  {
    std::wcerr << str << std::flush;
  }
  else
  {
    assert(true == out_file->is_open());
    (*out_file) << str << std::flush;
  }
  
  fmtstr_delete(str);
}

bool
check_not_cyclic(
  analyser_environment_t const * const /* ae */,
  node_t * const node,
  void * const /* param */)
{
  if (node->is_cyclic())
  {
    return false;
  }
  else
  {
    return true;
  }
}

void
add_to_set_unique(
  std::list<node_t *> &set,
  node_t * const node)
{
  assert(node != 0);
  
  for (std::list<node_t *>::const_iterator iter = set.begin();
       iter != set.end();
       ++iter)
  {
    if ((*iter) == node)
    {
      return;
    }
  }
  
  set.push_back(node);
}

std::list<node_t *>::iterator
check_if_one_node_is_belong_to_set_and_match_some_condition(
  std::list<node_t *> &set,
  node_t * const node,
  std::vector<check_node_func> const &check_func)
{
  std::list<node_t *>::iterator iter;

  for (iter = set.begin(); iter != set.end(); ++iter)
  {
    if ((*iter) == node)
    {
      if (0 == check_func.size())
      {
        return iter;
      }
      else
      {
        BOOST_FOREACH(check_node_func const &func, check_func)
        {
          if (true == func(node))
          {
            return iter;
          }
        }
      }
    }
  }
  
  assert(iter == set.end());
  
  return iter;
}

wchar_t *
form_rule_end_node_name(node_t * const node, bool const include_number)
{
  assert(0 == node->name().size());
  
  wchar_t *str =
    fmtstr_new(L"%s_%s", node->rule_node()->name().c_str(), L"END");
  
  if (true == include_number)
  {
    str = fmtstr_append(str, L"[%d]", node->overall_idx());
  }
  
  return str;
}

/// \brief Count the number of nodes in a node range.
///
/// This function count the number of nodes in a very simple
/// way. It does not consider any regex group info.
///
/// Ex:
///
///  A B C   => 3
/// (A (B)* C)* => 3
///
/// \param range_start 
/// \param range_end 
///
/// \return 
///
unsigned int
count_node_range_length(
  node_t const * const range_start,
  node_t const * const range_end)
{
  assert(range_start != 0);
  assert(range_end != 0);
  
  assert(range_start->name().size() != 0);
  assert(range_end->name().size() != 0);
  
  assert(1 == range_end->next_nodes().size());
  node_t * const real_range_end = range_end->next_nodes().front();
  
  unsigned int count = 0;
  
  node_t const *curr_node = range_start;
  
  while (curr_node != real_range_end)
  {
    ++count;
    
    assert(1 == curr_node->next_nodes().size());
    curr_node = curr_node->next_nodes().front();
  }
  
  return count;
}

bool
check_if_node_in_node_range(
  node_t const * const check_node,
  node_t const * const range_start,
  node_t const * const range_end)
{
  assert(check_node != 0);
  assert(range_start != 0);
  
  if (0 == check_node->name().size())
  {
    return false;
  }
  
  assert(range_start->name().size() != 0);
  
  if (range_end != 0)
  {
    assert(range_end->name().size() != 0);
  }
  
  assert(1 == range_start->next_nodes().size());
  
  if (range_end != 0)
  {
    assert(1 == range_end->next_nodes().size());
  }
  
  node_t const *real_range_end = 0;
  
  if (range_end != 0)
  {
    real_range_end = range_end->next_nodes().front();
  }
  
  node_t const * curr_node = range_start;
  
  while (true == node_for_each_not_reach_end(curr_node, real_range_end))
  {
    if (curr_node == check_node)
    {
      return true;
    }
    
    assert(1 == curr_node->next_nodes().size());
    curr_node = curr_node->next_nodes().front();
  }
  
  return false;
}

namespace
{
  extern unsigned int count_minimum_number_of_nodes_in_node_range_internal(
    node_t const * const range_start,
    node_t const * const range_end,
    std::list<regex_info_t> &regex_info);
  
  /// \brief count minimum number of nodes in a regex OR
  /// group.
  ///
  /// Ex:
  ///
  /// (B C | D) => 1
  ///
  /// \param regex_info 
  ///
  /// \return 
  ///
  unsigned int
  count_minimum_number_of_nodes_in_OR_regex(
    regex_info_t const &regex_OR_info,
    std::list<regex_info_t> &regex_info)
  {
    assert(REGEX_TYPE_OR == regex_OR_info.m_type);
    assert(regex_OR_info.m_ranges.size() > 1);
    
    unsigned int count = 0;
    std::list<regex_range_t>::size_type index = 0;
    
    BOOST_FOREACH(regex_range_t const &regex_OR_range,
                  regex_OR_info.m_ranges)
    {
      unsigned int partial_count;

      if (0 == index)
      {
        partial_count =
          count_minimum_number_of_nodes_in_node_range_internal(
            regex_OR_range.mp_start_node,
            regex_OR_range.mp_end_node,
            regex_info);
        
        count = partial_count;
      }
      else
      {
        std::list<regex_info_t> local_regex_info =
          regex_OR_range.mp_start_node->regex_info();
        
        partial_count =
          count_minimum_number_of_nodes_in_node_range_internal(
            regex_OR_range.mp_start_node,
            regex_OR_range.mp_end_node,
            local_regex_info);
      }
      
      if (partial_count < count)
      {
        count = partial_count;
      }
    }
    
    return count;
  }
  
  unsigned int
  count_minimum_number_of_nodes_in_node_range_internal(
    node_t const * const range_start,
    node_t const * const range_end,
    std::list<regex_info_t> &regex_info)
  {
    assert(range_start != 0);
    assert(range_end != 0);
    
    assert(range_start->name().size() != 0);
    assert(range_end->name().size() != 0);
    
    assert(1 == range_start->next_nodes().size());
    assert(1 == range_end->next_nodes().size());
    
    node_t * const real_range_end = range_end->next_nodes().front();
    unsigned int count = 0;
    node_t const *curr_node = range_start;
    
    while (curr_node != real_range_end)
    {
      if (0 == regex_info.size())
      {
        // If this node doesn't belong to any regex group,
        // then count 1.
        ++count;
      }
      else
      {
        switch (regex_info.back().m_type)
        {
        case REGEX_TYPE_ZERO_OR_MORE:
        case REGEX_TYPE_ZERO_OR_ONE:
          // (...)*
          // (...)?
          // The minimum node number of this type is 0, thus
          // I can skip it directly.
          assert(1 == regex_info.back().m_ranges.size());
          
          curr_node = regex_info.back().m_ranges.front().mp_end_node;
          break;
          
        case REGEX_TYPE_ONE_OR_MORE:
        case REGEX_TYPE_ONE:
          assert(1 == regex_info.back().m_ranges.size());
          
          {
            node_t * const local_range_start = 
              regex_info.back().m_ranges.front().mp_start_node;
            node_t * const local_range_end = 
              regex_info.back().m_ranges.front().mp_end_node;
            
            regex_info.pop_back();
            
            count += count_minimum_number_of_nodes_in_node_range_internal(
              local_range_start,
              local_range_end,
              regex_info);
            
            curr_node = local_range_end;
          }
          break;
          
        case REGEX_TYPE_OR:
          {
            regex_info_t const local_regex_info =
              regex_info.back();
            
            regex_info.pop_back();
            
            count +=
              count_minimum_number_of_nodes_in_OR_regex(local_regex_info,
                                                        regex_info);
            
            curr_node = local_regex_info.m_ranges.back().mp_end_node;
          }
          break;
          
        default:
          assert(0);
          break;
        }
      }
      
      assert(1 == curr_node->next_nodes().size());
      curr_node = curr_node->next_nodes().front();
      
      curr_node->check_if_regex_info_is_correct();
      regex_info = curr_node->regex_info();
    }
    
    return count;
  }
}

namespace
{
  bool
  find_first_node_which_has_regex_info(node_t * const node)
  {
    if (0 == node->regex_info().size())
    {
      return true;
    }
    else
    {
      return false;
    }
  }
}

/// \brief count minimum number of nodes in a node range
///
/// \warning If the starting node of a regex is belong to
/// the specified range, then the end node of the same regex
/// have to be in it, too.
///
/// \warning This function is based on those regex info in
/// 'm_regex_info' list, and not consider those ones in
/// 'm_tmp_regex_info' list.
///
/// \param range_start 
/// \param range_end 
///
/// \return 
///
unsigned int
count_minimum_number_of_nodes_in_node_range(
  node_t const * const range_start,
  node_t const * const range_end)
{
  assert(range_start != 0);
  assert(range_end != 0);
  
  assert(range_start->name().size() != 0);
  assert(range_end->name().size() != 0);
  
  assert(1 == range_start->next_nodes().size());
  assert(1 == range_end->next_nodes().size());
  
  range_start->check_if_regex_info_is_correct();
  
#if defined(_DEBUG)
  // If the starting node of a regex is belong to the
  // specified range, then the end node of the same regex
  // have to be in it, too.
  {
    // find the first node which has regex info.
    node_t * const tmp =
      node_for_each(const_cast<node_t *>(range_start),
                    const_cast<node_t *>(range_end),
                    find_first_node_which_has_regex_info);
    
    if (tmp != 0)
    {
      assert(true ==
             check_if_node_in_node_range(
               tmp->regex_info().back().m_ranges.back().mp_end_node,
               range_start,
               range_end));
    }
  }
#endif
  
  std::list<regex_info_t> local_regex_info = range_start->regex_info();
  
  return count_minimum_number_of_nodes_in_node_range_internal(
    range_start,
    range_end,
    local_regex_info);
}

unsigned int
get_max_value(unsigned int const value_1,
              unsigned int const value_2,
              unsigned int const value_3)
{
  if (value_1 < value_2)
  {
    if (value_2 < value_3)
    {
      return value_3;
    }
    else
    {
      return value_2;
    }
  }
  else
  {
    if (value_1 < value_3)
    {
      return value_3;
    }
    else
    {
      return value_1;
    }
  }
}

namespace
{
  typedef std::map<std::wstring, appear_max_and_curr_time> component_name_map_type;
  
  struct collect_node_appear_times : public std::unary_function<node_t, bool>
  {
  private:
    
    component_name_map_type &m_component_name_map;
    
    collect_node_appear_times &operator=(collect_node_appear_times const &);

  public:
    
    collect_node_appear_times(
      component_name_map_type &component_name_map)
      : m_component_name_map(component_name_map)
    { }
    
    bool
    operator()(
      node_t * const node)
    {
      m_component_name_map[node->name()].max_appear_times += 1;
      
      return true;
    }
  };
  
  struct assign_node_name_postfix_by_appear_times : public std::unary_function<node_t, bool>
  {
  private:
    
    component_name_map_type &m_component_name_map;
    
    assign_node_name_postfix_by_appear_times &operator=(assign_node_name_postfix_by_appear_times const &);

  public:
    
    assign_node_name_postfix_by_appear_times(
      component_name_map_type &component_name_map)
      : m_component_name_map(component_name_map)
    { }
    
    bool
    operator()(
      node_t * const node)
    {
      assert(node != 0);
      assert(node->name().size() != 0);
      
      node->name_postfix_by_appear_times() = m_component_name_map[node->name()].curr_appear_times;
      ++(m_component_name_map[node->name()].curr_appear_times);
      
      return true;
    }
  };
}

void
count_appear_times_for_node_range(
  node_t * const node_start,
  node_t * const node_end)
{
  component_name_map_type component_name_map;
  
  assert(node_start != 0);
  if (node_end != 0)
  {
    assert(node_start->is_my_descendant_node(node_end));
  }
  
  // Iterate all nodes in this alternative to find the
  // max number of times each node's name appears.
  //
  // This method is more efficient than using std::count()
  // on each name string.
  (void)node_for_each(node_start,
                      node_end,
                      collect_node_appear_times(component_name_map));
  
  (void)node_for_each(node_start,
                      node_end,
                      assign_node_name_postfix_by_appear_times(component_name_map));
}
