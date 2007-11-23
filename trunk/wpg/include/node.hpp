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

#ifndef __node_h__
#define __node_h__

#include "global.hpp"
#include "lookahead.hpp"
#include "regex.hpp"
#include "regex_info_with_arranged_lookahead.hpp"

#define NOT_CAL_ALTER_LENGTH_YET (0xFFFFFFFF)

class analyser_environment_t;

enum rule_contain_regex_t
{
  RULE_CONTAIN_NO_REGEX,
  RULE_CONTAIN_REGEX,
  RULE_CONTAIN_REGEX_OR
};
typedef enum rule_contain_regex_t rule_contain_regex_t;

class node_t
{
public:
  
  node_t(
    analyser_environment_t * const ae,
    node_t * const rule_node,
    std::wstring const &str = std::wstring());
  
  // copy constructor
  node_t(
    node_t const * const node);
  
  bool is_my_prev_node(
    node_t const * const node) const;
  
  bool is_my_next_node(
    node_t const * const node) const;
  
  bool is_my_descendant_node(
    node_t const * const node) const;
  
  bool is_my_ancestor_node(
    node_t const * const node) const;
  
  void set_is_rule_head(
    bool const status)
  { m_is_rule_head = status; }
  
  bool is_rule_head() const
  { return m_is_rule_head; }
  
  void set_rule_end_node(
    node_t * const node);

  node_t *rule_end_node() const
  {
    assert(true == m_is_rule_head);
    return mp_rule_end_node;
  }

  void append_node(
    node_t * const node);

  void prepend_node(
    node_t * const node);

  std::list<node_t *> const &next_nodes() const
  { return m_next_nodes; }

  std::list<node_t *> const &prev_nodes() const
  { return m_prev_nodes; }

  std::list<node_t *> &next_nodes()
  { return m_next_nodes; }
  
  std::list<node_t *> &prev_nodes()
  { return m_prev_nodes; }
  
  node_t *node_before_the_rule_end_node();
  
  std::wstring const &name() const
  { return m_name; }
  
  void append_name(
    wchar_t const * const str);
  
  void set_name(
    std::wstring const &str)
  { m_name = str; }

  void set_node_before_the_rule_end_node(
    node_t * const node);

  std::list<node_t *>::iterator break_append_node(
    node_t * const node);

  std::list<node_t *>::iterator break_prepend_node(
    node_t * const node);

  bool is_optional() const
  { return m_optional; }

  void set_optional(
    bool const optional)
  { m_optional = optional; }
  
  bool is_terminal() const
  { return m_is_terminal; }
  
  void set_create_dot_node_line(
    bool const status);
  
  bool create_dot_node_line() const
  { return m_create_dot_node_line; }
  
  unsigned int &overall_idx()
  { return m_overall_idx; }
  
  unsigned int overall_idx() const
  { return m_overall_idx; }
  
  unsigned int &name_postfix_by_appear_times()
  { return m_name_postfix_by_appear_times; }
  
  unsigned int name_postfix_by_appear_times() const
  { return m_name_postfix_by_appear_times; }
  
  bool traversed() const
  { return m_traversed; }

  void set_traversed(
    bool const status)
  { m_traversed = status; }
  
  void set_nonterminal_rule_node(
    node_t * const node)
  { mp_nonterminal_rule_node = node; }
  
  void add_refer_to_me_node(
    node_t * const node);
  
  void remove_refer_to_me_node(
    node_t * const node);

  bool is_refer_to_me_node(
    node_t const * const node) const;

  std::list<node_t *> const &refer_to_me_nodes() const
  { return m_refer_to_me_nodes; }
  
  node_t *nonterminal_rule_node() const
  { return mp_nonterminal_rule_node; }

  void add_same_rule_node(
    node_t * const node);

  void clear_same_rule_nodes()
  { m_same_rule_nodes.clear(); }

  std::list<node_t *> const &same_rule_nodes() const
  { return m_same_rule_nodes; }

  node_t *rule_node() const
  { return mp_rule_node; }

  void set_rule_node(
    node_t * const rule_node)
  { mp_rule_node = rule_node; }

  bool is_nullable() const
  { return m_is_nullable; }
  
  bool is_cyclic() const
  { return (m_cyclic_set.size() > 0) ? true : false; }
  
  bool is_left_recursion() const
  { return (m_left_recursion_set.size() > 0) ? true : false; }
  
  void set_nullable(
    bool const nullable)
  { assert(true == m_is_rule_head); m_is_nullable = nullable; }

  analyser_environment_t *ae() const
  { return mp_ae; }

  void init();

  std::list<node_t *> const &temp_append_to_rule_node() const
  { return m_temp_append_to_rule_node; }

  std::list<node_t *> const &temp_prepend_to_end_node() const
  { return m_temp_prepend_to_end_node; }
  
  void
  add_temp_append_to_rule_node(
    node_t * const node)
  {
    assert(true == m_is_rule_head);
    
    node->alternative_start() = node;
    m_temp_append_to_rule_node.push_back(node);
  }
  
  void add_temp_prepend_to_end_node(
    node_t * const node)
  { assert(m_is_rule_head); m_temp_prepend_to_end_node.push_back(node); }
  
  void clear_temp_append_to_rule_node()
  { assert(m_is_rule_head); m_temp_append_to_rule_node.clear(); }

  void clear_temp_prepend_to_end_node()
  { assert(m_is_rule_head); m_temp_prepend_to_end_node.clear(); }

  void assign_cyclic_set(
    std::list<node_t *> const &set);

  std::list<node_t *> const &cyclic_set() const
  { return m_cyclic_set; }

  std::list<node_t *> &cyclic_set()
  { return m_cyclic_set; }
  
  void assign_left_recursion_set(
    std::list<node_t *> const &set);
  
  std::list<node_t *> const &left_recursion_set() const
  { return m_left_recursion_set; }

  std::list<node_t *> &left_recursion_set()
  { return m_left_recursion_set; }
  
  void clear_cyclic_set() { m_cyclic_set.clear(); }
  
  void clear_left_recursion_set() { m_left_recursion_set.clear(); }

  void set_apostrophe_node(
    node_t * const node)
  { assert(node != 0); mp_apostrophe_node = node; }

  void set_apostrophe_orig_node(
    node_t * const node)
  { assert(node != 0); mp_apostrophe_orig_node = node; }

  node_t *apostrophe_node() const
  { return mp_apostrophe_node; }

  node_t *apostrophe_orig_node() const
  { return mp_apostrophe_orig_node; }  

  lookahead_set_t &lookahead_set()
  { return m_lookahead_set; }

  lookahead_set_t const &lookahead_set() const
  { return m_lookahead_set; }

  void add_left_corner_set(
    node_t * const node)
  { m_left_corner_set.push_back(node); }

  std::list<node_t *> const &left_corner_set() const
  { return m_left_corner_set; }

  node_t *find_alternative_last_node();

  void add_ambigious_set(
    node_t * const node);

  bool is_ambigious_to(
    node_t * const node) const;

  bool is_ambigious() const
  { return (0 == m_ambigious_set.size()) ? false : true; }

  std::list<node_t *> const &ambigious_set() const
  { return m_ambigious_set; }
  
  void set_lookahead_depth(
    unsigned int const depth)
  { m_lookahead_depth = depth; }

  unsigned int lookahead_depth() const
  { return m_lookahead_depth; }

  void set_starting_rule()
  { m_starting_rule = true; }

  bool is_starting_rule() const
  { return m_starting_rule; }
  
  typedef node_t *node_t_ptr;
  
  node_t_ptr &alternative_start()
  { return mp_alternative_start; }
  
  node_t_ptr const &alternative_start() const
  { return mp_alternative_start; }
  
  void calculate_lookahead_waiting_pool_for_pure_BNF_rule(
    std::list<todo_nodes_t> &todo_nodes_set_2) const;

  void calculate_lookahead_waiting_pool_for_EBNF_rule(
    std::list<todo_nodes_t> &todo_nodes_set_2) const;
  
  bool compare_lookahead_set(
    node_t * const node) const;
  
  void add_token_name_as_terminal_during_lookahead(
    std::wstring const &str);

  std::list<std::wstring> const &token_name_as_terminal_during_lookahead() const;
  
  unsigned int count_node_number_between_this_node_and_rule_end_node() const;
  
  unsigned int distance_from_rule_head() const
  { return m_distance_from_rule_head; }
  
  unsigned int distance_to_rule_end_node() const
  { return m_distance_to_rule_end_node; }
  
  void set_distance_from_rule_head(
    unsigned int const distance)
  { m_distance_from_rule_head = distance; }
  
  void set_distance_to_rule_end_node(
    unsigned int const distance)
  { m_distance_to_rule_end_node = distance; }
  
  bool is_eof() const
  { return m_is_eof; }

  void set_eof()
  { m_is_eof = true; }
  
  void dump_gen_parser_header(
    std::wfstream &file);
  
  void dump_gen_parser_src(
    std::wfstream &file) const;
  
  void dump_gen_parser_src_for_not_regex_alternative(
    std::wfstream &file) const;
  
  void dump_gen_parser_src_for_regex_alternative(
    std::wfstream &file) const;
  
  void dump_gen_parser_src_translate(
    std::wfstream &file) const;
  
  typedef node_t *node_t_ptr;
  
  void push_back_regex_info(
    std::list<regex_range_t> const &regex_range_list,
    regex_type_t const type)
  {
    m_regex_info.push_back(regex_info_t(regex_range_list, type));
  }
  
  void push_back_regex_info(
    node_t * const start,
    node_t * const end,
    regex_info_t * const regex_OR_info,
    regex_type_t const type)
  {
    m_regex_info.push_back(regex_info_t(start, end, regex_OR_info, type));
  }
  
  void push_front_regex_info(
    node_t * const start,
    node_t * const end,
    regex_info_t * const regex_OR_info,
    regex_type_t const type)
  {
    m_regex_info.push_front(regex_info_t(start, end, regex_OR_info, type));
  }
  
  std::list<regex_info_t> const &regex_info() const
  { return m_regex_info; }
  
  std::list<regex_info_t> &regex_info()
  { return m_regex_info; }
  
  std::list<regex_info_t> const &tmp_regex_info() const
  { return m_tmp_regex_info; }
  
  std::list<regex_info_t> &tmp_regex_info()
  { return m_tmp_regex_info; }
  
  void restore_regex_info();
  
  /// pop a 'regex_info_t' from the back of
  /// 'm_regex_info', and push it to the front of
  /// 'm_tmp_regex_info'.
  ///
  /// ie.
  ///
  ///     +----+----+----+----+----+----+
  ///     |    |    |    |    |    |    |   m_regex_info
  ///     |    |    |    |    |    |    |
  ///     +----+----+----+----+----+----+
  ///                                 v
  ///                                 |
  ///       +-------------------------+
  ///       |
  ///       v
  ///          +----+----+----+----+----+
  ///          |    |    |    |    |    |   m_tmp_regex_info
  ///          |    |    |    |    |    |
  ///          +----+----+----+----+----+
  ///
  ///
  void move_one_regex_info_to_tmp()
  {
    regex_info_t const &regex = m_regex_info.back();
    m_tmp_regex_info.push_front(regex);
    m_regex_info.pop_back();
  }
  
  void restore_regex_info_from_tmp()
  {
    m_regex_info = m_tmp_regex_info;
    m_tmp_regex_info.clear();
  }
  
  unsigned int alternative_length() const
  { return m_alternative_length; }

  unsigned int &alternative_length()
  { return m_alternative_length; }
  
  /** @{ */
  /// \brief Link 2 nodes between the forking one and the
  /// original one.
  /// 
  /// Ex:
  /// I fork the following production:
  /// 
  /// A B C D
  /// 
  /// thus I will have:
  /// 
  /// A1 B1 C1 D1
  /// A2 B2 C2 D2
  /// 
  /// the 'corresponding_node_between_fork' relationship
  /// between these 2 productions are:
  /// 
  /// A1 <--> A2
  /// B1 <--> B2
  /// C1 <--> C2
  /// D1 <--> D2
  /// 
  /// \return the corresponding node of this node in the
  /// forking production.
  ///
  std::list<node_t *> const &corresponding_node_between_fork() const
  { return mp_corresponding_node_between_fork; }
  
  std::list<node_t *> &corresponding_node_between_fork()
  { return mp_corresponding_node_between_fork; }
  /** @} */
  
  node_t_ptr const &brother_node_to_update_regex() const
  { return mp_brother_node_to_update_regex; }
  
  node_t_ptr &brother_node_to_update_regex()
  { return mp_brother_node_to_update_regex; }
  
  node_t_ptr const &corresponding_node_in_main_regex_alternative() const
  { return mp_corresponding_node_in_main_regex_alternative; }
  
  node_t_ptr &corresponding_node_in_main_regex_alternative()
  { return mp_corresponding_node_in_main_regex_alternative; }
  
  std::list<node_t *> const &refer_to_me_nodes_in_regex_expansion_alternatives() const
  { return mp_refer_to_me_nodes_in_regex_expansion_alternatives; }
  
  std::list<node_t *> &refer_to_me_nodes_in_regex_expansion_alternatives()
  { return mp_refer_to_me_nodes_in_regex_expansion_alternatives; }
  
  bool const &is_in_regex_OR_group() const
  { return m_is_in_regex_OR_group; }
  
  bool &is_in_regex_OR_group()
  { return m_is_in_regex_OR_group; }
  
  void check_if_regex_info_is_correct() const;
  
  rule_contain_regex_t &rule_contain_regex()
  { return m_rule_contain_regex; }
  
  rule_contain_regex_t const &rule_contain_regex() const
  { return m_rule_contain_regex; }
  
  typedef node_t *node_ptr;
  
  node_ptr &main_regex_alternative()
  { return mp_main_regex_alternative; }
  
  node_ptr const &main_regex_alternative() const
  { return mp_main_regex_alternative; }
  
  std::list<regex_info_t> &regex_OR_info()
  { return m_regex_OR_info; }
  
  std::list<regex_info_t> const &regex_OR_info() const
  { return m_regex_OR_info; }
  
  template<typename T_traits>
  regex_info_t const *find_regex_info_is_started_by_me(
    regex_info_t const * const starting_regex) const;
  
  bool const &contains_ambigious() const
  { return m_contains_ambigious; }
  
  bool &contains_ambigious()
  { return m_contains_ambigious; }
  
private:
  
  analyser_environment_t *mp_ae;
  bool m_is_rule_head;
  bool m_is_terminal;
  bool m_is_eof;
  std::wstring m_name;
  std::list<node_t *> m_prev_nodes;
  std::list<node_t *> m_next_nodes;
  
  /// This variable is only meaningful in the rule node.
  rule_contain_regex_t m_rule_contain_regex;
  
  /// This variable is used to remember the main regex
  /// alternative. Ex:
  ///
  /// (A B)* C  <-- main_regex_alternative
  ///
  /// will become
  /// 
  /// (A B)* C  <-- main_regex_alternative
  /// C
  ///
  /// Then I can emit source & header codes according to the
  /// main_regex_alternative.
  ///
  /// Ex:
  ///
  /// (A B | C)* D  <-- main_regex_alternative
  /// 
  /// will become
  /// 
  /// (A B | C)* D  <-- main_regex_alternative
  /// (A B)* C
  /// (C)* C
  /// D
  ///
  /// In the normal situaction, I should delete the original
  /// alternative becuase of the regex OR statement
  /// expansion. However, if the deleting alternative is the
  /// main_regex_alternative, then I should cancel this
  /// deletion, so that I can emit the source and header
  /// codes according to it.
  node_t *mp_main_regex_alternative;
  
  bool m_traversed;
  bool m_create_dot_node_line;
  
  unsigned int m_overall_idx;
  unsigned int m_name_postfix_by_appear_times;
  
  node_t *mp_rule_node;
  node_t *mp_rule_end_node;
  node_t *mp_alternative_start;
  bool m_is_nullable;
  
  unsigned int m_lookahead_depth;
  lookahead_set_t m_lookahead_set;
  
  node_t *mp_last_edge_last_node;
  node_t *mp_apostrophe_node;
  node_t *mp_apostrophe_orig_node;
  
  /// This field is only useful in rule nodes.
  ///
  /// In the rule nodes:
  ///
  /// A : a | b | c
  ///
  /// B : A | C
  ///     ^
  ///
  /// Then the 'refer_to_me_nodes' of the rule node 'A' will
  /// contain the node 'A' pointed by '^'. The inverse
  /// relationship of this idea is the
  /// 'nonterminal_rule_node' field.
  std::list<node_t *> m_refer_to_me_nodes;
  
  std::list<node_t *> m_same_rule_nodes;
  std::list<node_t *> m_cyclic_set;
  std::list<node_t *> m_left_recursion_set;
  std::list<node_t *> m_left_corner_set;
  std::list<node_t *> m_temp_append_to_rule_node;
  std::list<node_t *> m_temp_prepend_to_end_node;
  std::list<node_t *> m_ambigious_set;
  
  /// Only useful for rule node.
  bool m_contains_ambigious;
  
  node_t *mp_nonterminal_rule_node;
  node_t *mp_node_before_the_rule_end_node;
  bool m_optional;
  
  bool m_starting_rule;
  
  std::list<std::wstring> m_token_name_as_terminal_during_lookahead;
  
  unsigned int m_distance_from_rule_head;
  unsigned int m_distance_to_rule_end_node;
  
  std::list<regex_info_t> m_regex_info;
  std::list<regex_info_t> m_tmp_regex_info;
  
  ///
  /// Used for code gen.
  ///
  std::list<regex_info_t> m_regex_OR_info;
  
  unsigned int m_alternative_length;
  
  node_t *mp_corresponding_node_in_main_regex_alternative;
  std::list<node_t *> mp_refer_to_me_nodes_in_regex_expansion_alternatives;
  
  std::list<node_t *> mp_corresponding_node_between_fork;
  node_t *mp_brother_node_to_update_regex;
  
  bool m_is_in_regex_OR_group;
  
  void dump_gen_parser_header_for_regex_alternative(
    std::wfstream &file,
    node_t * const alternative_start,
    std::wstring const &rule_node_name);
  
  void dump_gen_parser_header_for_not_regex_alternative(
    std::wfstream &file,
    node_t * const alternative_start,
    std::wstring const &rule_node_name,
    int const alternative_id) const;
  
  void dump_gen_parser_src__fill_nodes__for_regex_alternative(
    std::wfstream &file) const;
  
  void dump_gen_parser_src__fill_nodes__for_node_range(
    std::wfstream &file) const;

  void dump_gen_parser_src__parse_XXX__for_node_range(
    std::wfstream &file) const;
  
  void dump_gen_parser_src__parse_XXX__for_regex_alternative(
    std::wfstream &file) const;
  
  void dump_gen_parser_src__parse_XXX__real(
    std::wfstream &file) const;
  
  void collect_regex_info_into_regex_stack(
    std::wfstream &file,
    node_t const * const curr_node,
    std::list<regex_info_with_arranged_lookahead_t> &regex_stack,
    unsigned int &indent_depth,
    node_t * const default_node) const;
  
  void dump_gen_parser_src__parse_XXX__each_regex_level_header_codes(
    std::wfstream &file,
    std::list<regex_info_with_arranged_lookahead_t> &regex_stack,
    unsigned int &indent_depth,
    node_t * const default_node) const;
};
typedef class node_t node_t;

extern bool node_for_each_not_reach_end(
  node_t const * const curr_node,
  node_t const * const end_node);

template<typename Function>
node_t *
node_for_each(
  node_t * const start_node,
  node_t * const end_node,
  Function f)
{
  assert(start_node != 0);
  
  assert(start_node->name().size() != 0);
  
  node_t *real_end_node = 0;
  
  if (end_node != 0)
  {
    assert(end_node->name().size() != 0);
    assert(1 == end_node->next_nodes().size());
    real_end_node = end_node->next_nodes().front();
  }
  
  node_t *curr_node = start_node;
  
  while (true == node_for_each_not_reach_end(curr_node, real_end_node))
  {
    if (false == f(curr_node))
    {
      return curr_node;
    }
    
    assert(1 == curr_node->next_nodes().size());
    curr_node = curr_node->next_nodes().front();
  }
  
  return 0;
}

struct find_inner_most
{
  typedef std::list<regex_info_t>::const_iterator iter_type;
  
  static iter_type begin(
    std::list<regex_info_t> const &regex_info)
  {
    return regex_info.begin();
  }
  
  static iter_type end(
    std::list<regex_info_t> const &regex_info)
  {
    return regex_info.end();
  }
};
typedef struct find_inner_most find_inner_most;

struct find_outer_most
{
  typedef std::list<regex_info_t>::const_reverse_iterator iter_type;
  
  static iter_type begin(
    std::list<regex_info_t> const &regex_info)
  {
    return regex_info.rbegin();
  }
  
  static iter_type end(
    std::list<regex_info_t> const &regex_info)
  {
    return regex_info.rend();
  }
};
typedef struct find_outer_most find_outer_most;

template<typename T_traits>
regex_info_t const *
node_t::find_regex_info_is_started_by_me(
  regex_info_t const * const starting_regex) const
{
  regex_info_t const *answer = 0;
  bool find_starting_regex = false;
  
  for (T_traits::iter_type iter = T_traits::begin(m_regex_info);
       iter != T_traits::end(m_regex_info);
       ++iter)
  {
    assert((*iter).m_type != REGEX_TYPE_OR);
    assert(1 == (*iter).m_ranges.size());
    
    assert(starting_regex->m_type != REGEX_TYPE_OR);
    assert(1 == starting_regex->m_ranges.size());
    
    if (starting_regex != 0)
    {
      if (((*iter).m_ranges.front().mp_start_node == starting_regex->m_ranges.front().mp_start_node) &&
          ((*iter).m_ranges.front().mp_end_node == starting_regex->m_ranges.front().mp_end_node))
      {
        find_starting_regex = true;
      }
      else
      {
        if (false == find_starting_regex)
        {
          continue;
        }
        else
        {
          if ((*iter).m_ranges.front().mp_start_node == this)
          {
            answer = &(*iter);
            break;
          }
        }
      }
    }
    else
    {
      // 0 == starting_regex
      if ((*iter).m_ranges.front().mp_start_node == this)
      {
        answer = &(*iter);
        break;
      }
    }
  }
  
  if (starting_regex != 0)
  {
    assert(true == find_starting_regex);
  }
  
  return answer;
}

extern bool check_if_regex_info_is_empty(
  analyser_environment_t const * const,
  node_t * const node,
  void *param);

extern bool check_if_regex_info_is_correct_for_traverse_all_nodes(
  analyser_environment_t const * const,
  node_t * const node,
  void *param);

extern bool delete_regex_type_one(
  analyser_environment_t const * const,
  node_t * const node,
  void *param);

extern bool clear_node_state(
  analyser_environment_t const * const ae,
  node_t * const node,
  void * const param);

#endif
