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

#ifndef __ae_hpp__
#define __ae_hpp__

#include "global.hpp"
#include "lookahead.hpp"
#include "hash.hpp"
#include "stack_elem_for_left_recursion_detection.hpp"

class node_t;
enum parse_answer_state_t;

class analyser_environment_t
{
private:
  enum state_t
  {
    STATE_READ_TERMINAL,
    STATE_START_RULE,
    STATE_READ_ALTERNATIVE_START,
    STATE_READ_ALTERNATIVE
  };
  typedef enum state_t state_t;
  
public:
  
  analyser_environment_t();
  ~analyser_environment_t();
  
  void add_top_level_nodes(node_t * const node)
  { m_top_level_nodes.push_back(node); }
  
  std::list<node_t *> const &top_level_nodes() const
  { return m_top_level_nodes; }
  
  node_t *last_rule_end_node();
  
  node_t *last_rule_node();
  
  bool parse_command_line(
    int argc,
    char **argv);
  
  void read_grammar(
    std::list<keyword_t> const &keywords);
  
  void detect_nullable_nonterminal();
  
  void remove_epsilon_production();
  
  void detect_cyclic_nonterminal();
  
  void remove_cyclic();
  
  void remove_direct_cyclic();
  
  void remove_left_recur();
  
  std::wfstream *output_file() const
  { return mp_output_file; }
  
  bool traverse_all_nodes(
    bool (*traverse_node_func)(analyser_environment_t const * const,
                               node_t * const,
                               void *param),
    bool (*traverse_rule_func)(analyser_environment_t const * const,
                               node_t * const,
                               void *param),
    void * const param) const;
  
  void merge_relative_set_into_final_set(
    std::list<std::list<node_t *> > &set);
  
  void dump_grammar(
    std::wstring const &filename) const;
  
  bool check_grammar();
  
  bool check_grammar_not_all_left_recursion();
  
  bool check_grammar_not_all_right_recursion();
  
  void compute_lookahead_set();

  void left_factoring();

  void non_left_recursion_grouping();

  void order_nonterminal_decrease_number_of_distinct_left_corner();

  void find_left_corners();

  void lc_transform();
  
  void dump_tree(
    std::wstring const &filename);

  void close_output_file();
  
  void compute_lookahead_terminal_for_node(
    node_t * const node,
    size_t const depth) const;
  
  void log(wchar_t const * const fmt, ...) const;

  void remove_duplicated_alternatives_for_all_rule();

  void remove_useless_rule();

#if defined(_DEBUG)
  bool cmp_ans() const { return m_cmp_ans; }
#endif
  
  bool perform_answer_comparison() const;

  bool read_answer_file();

  void check_nonterminal_linking() const;

  void determine_node_position();
  
  bool is_duplicated_alternatives(
    std::list<node_t *>::const_iterator begin_iter,
    std::list<node_t *>::const_iterator end_iter,
    std::list<unsigned int> const &walk_counts) const;
  
  void find_eof() const;
  
  bool check_eof_condition(
    node_t const * const node,
    std::list<node_t *> &last_symbol_chain) const;
  
  void dump_gen_parser_nodes_hpp(
    std::wfstream &file);

  void dump_gen_parser_basic_types_hpp(
    std::wfstream &file) const;

  void dump_gen_parser_cpp() const;
  
  void dump_gen_frontend_cpp(
    std::wfstream &file) const;
  
  void dump_gen_frontend_hpp(
    std::wfstream &file) const;
  
  void dump_gen_token_hpp(
    std::wfstream &file) const;
  
  void dump_gen_main_cpp(
    std::wfstream &file) const;
  
  bool is_terminal(
    std::wstring const &str) const;

  void hash_terminal(
    std::wstring const &str);

  node_t *nonterminal_rule_node(
    std::wstring const &str) const;

  void hash_rule_head(
    node_t * const node);

  void remove_rule_head(
    node_t * const node);

  void hash_answer(
    node_t * const node);

  node_t *find_answer(
    node_t * const node) const;
  
  std::wstring const &grammar_file_name() const
  { return m_grammar_file_name; }
  
  //.................void lexer_put_grammar_string(
  //.................  boost::shared_ptr<std::wstring> grammar_string);
  
  boost::shared_ptr<std::wstring>
  lexer_get_grammar_string(
    std::list<wchar_t> const &delimiter);
  
  void restore_regex_info() const;

  void fill_regex_info_to_relative_nodes_for_each_regex_group() const;
  
  unsigned int max_lookahead_searching_depth() const
  { return m_max_lookahead_searching_depth; }
  
  void set_max_lookahead_searching_depth(
    unsigned int const depth)
  { m_max_lookahead_searching_depth = depth; }
  
  bool use_paull_algo_to_remove_left_recursion() const
  { return m_use_paull_algo_to_remove_left_recursion; }
  
  bool enable_left_factor() const
  { return m_enable_left_factor; }
  
  bool using_pure_BNF() const
  { return m_using_pure_BNF; }
  
  bool detect_left_recursion(
    std::list<std::list<node_t *> > &left_recursion_set);
  
private:
  
  bool parse_rule_head(
    std::wstring const &str,
	std::list<regex_stack_elem_t> &regex_stack);
  
  void parse_alternative_start(
    std::wstring const &str);

  void parse_alternative(
    std::wstring const &str);

  void parse_grammar(
    std::list<keyword_t> const &keywords,
    std::wstring const &str,
	std::list<regex_stack_elem_t> &regex_stack);
  
  void dump_alternative(
    std::wfstream &fp,
    node_t const * const rule_node,
    node_t * const node) const;
  
  node_t *create_node_by_name(
    node_t * const rule_node,
    std::wstring const &str);
  
  void mark_number_for_all_nodes() const;
  
  /// 
  /// detect nullable
  ///
  /// @param node 
  ///
  /// @return 
  ///
  bool detect_nullable_nonterminal_for_one_alternative(
    node_t * const node) const;
  
  bool detect_nullable_nonterminal_for_one_rule(
    node_t * const node) const;
  
  /// 
  /// handle optional & nullable nodes
  ///
  /// @param alternative_start 
  /// @param check_func 
  /// @param after_link_nonterminal 
  ///
  void build_new_alternative_for_optional_nodes(
    node_t * const alternative_start,
    std::vector<check_node_func> const &check_func,
    bool const after_link_nonterminal);
  
  void build_new_alternative_for_regex_nodes(
    node_t * const alternative_start);

  void check_and_fork_alternative_for_optional_node(
    node_t * const start_node,
    std::vector<check_node_func> const &check_func,
    bool const after_link_nonterminal);
  
  void check_and_fork_alternative_for_regex_node(
    node_t * const start_node,
    std::list<node_t *> &need_to_delete_alternative);
  
  void delete_last_alternative(
    node_t * const rule_node,
    bool const after_link_nonterminal);
  
  void detect_cyclic_nonterminal_for_one_rule(
    node_t * const node,
    std::list<node_t *> &stack,
    std::list<std::list<node_t *> > &cyclic_set);
  
  void replace_first_smaller_index_nonterminal(
    node_t * const rule_node,
    node_t * const target_rule_node);
  
  void dump_grammar_alternative(
    std::wfstream &fp,
    node_t * const node) const;
  
  void dump_grammar_rule(
    std::wfstream &fp,
    node_t * const node) const;
  
  void remove_immediate_left_recursion(
    node_t * const node);
  
  void add_a_terminal_to_lookahead_set(
    node_t * const node,
    lookahead_set_t * const lookahead_set,
    unsigned int * const prev_level_walk_count,
    size_t const cur_level,
    size_t const max_level,
    std::list<recur_lookahead_t> &recur_parent_stack,
    std::list<last_symbol_t> &last_symbol_stack,
    unsigned int const curr_last_symbol_level) const;
  
  void compute_lookahead_set(
    node_t * const node,
    lookahead_set_t * const lookahead_set,
    unsigned int * const prev_level_walk_count,
    size_t const cur_level,
    size_t const max_level,
    std::list<recur_lookahead_t> &recur_parent_stack,
    std::list<last_symbol_t> &last_symbol_stack,
    unsigned int const curr_last_symbol_level) const;
  
  LOOKAHEAD_COMPARE_RESULT compare_lookahead_set(
    std::list<lookahead_set_t> const &lookahead_a,
    std::list<lookahead_set_t> const &lookahead_b) const;
  
  LOOKAHEAD_COMPARE_RESULT compare_lookahead_set_for_node(
    node_t const * const node_a,
    node_t const * const node_b) const;
  
  void find_left_corners_for_node(
    node_t * const rule_node,
    node_t * const node);
  
  bool parse_answer(
    std::wstring const &str,
    node_t **node,
    parse_answer_state_t *parse_answer_state,
    bool *latest_char_is_linefeed);
  
  bool parse_answer_file(
    std::wfstream * const answer_file);
  
  void parse_answer_lookahead(
    node_t * const node,
    std::wstring const &str);
  
  bool perform_answer_comparison_for_each_node(
    node_t * const node_a,
    node_t * const node_b) const;
  
  void handle_regex_OR_stmt(
    node_t * const alternative_start_node,
    std::list<node_t *> &need_to_delete_alternative,
    regex_info_t const &regex_info);
  
  void remove_whole_regex(
    node_t * const alternative_start_node,
    std::list<node_t *> const &nodes_in_regex);
  
  void duplicate_whole_regex(
    node_t * const alternative_start_node,
    std::list<node_t *> &nodes_in_regex);
  
  void detect_left_recursion_nonterminal_for_one_rule(
    node_t * const node,
    std::list<stack_elem_for_left_recursion_detection> &stack,
    std::list<std::list<node_t *> > &left_recursion_set);
  
  unsigned int m_max_lookahead_searching_depth;
  bool m_use_paull_algo_to_remove_left_recursion;
  bool m_enable_left_factor;
  bool m_using_pure_BNF;
  
  state_t m_state;
  
  bool m_indicate_terminal_rule;
  
  std::wstring m_grammar_file_name;
  std::wfstream m_grammar_file;
  std::wstring m_output_filename;
  std::wfstream *mp_output_file;
  
  std::list<node_t *> m_top_level_nodes;
  std::list<node_t *> m_answer_nodes;
  node_t *mp_last_created_node_during_parsing;
  node_t *mp_curr_parsing_alternative_head;
  
  terminal_hash_table_t m_terminal_hash_table;
  rule_head_hash_table_t m_rule_head_hash_table;
  answer_node_hash_table_t m_answer_node_hash_table;
  
  unsigned int mutable m_curr_output_filename_idx;
  
#if defined(_DEBUG)
  bool m_cmp_ans;
#endif
  
  bool m_next_token_is_regex_OR_start_node;
};
typedef class analyser_environment_t analyser_environment_t;

#endif
