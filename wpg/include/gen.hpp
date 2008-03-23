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

#ifndef __gen_hpp__
#define __gen_hpp__

class arranged_lookahead_t;

enum nodes_have_lookahead_rel_t
{
  ALL_NODES_HAVE_LOOKAHEAD,
  PART_NODES_HAVE_LOOKAHEAD,
  NO_NODES_HAVE_LOOKAHEAD
};
typedef enum nodes_have_lookahead_rel_t nodes_have_lookahead_rel_t;

enum check_regex_group_pos_t
{
  CHECK_START,
  CHECK_END
};
typedef enum check_regex_group_pos_t check_regex_group_pos_t;

extern void dump_gen_parser_src_real(
  std::wfstream &file,
  node_t const * const rule_node,
  node_t const * const default_node,
  std::list<node_with_order_t> &nodes,
  unsigned int const indent_depth);

extern std::wstring indent_line(
  unsigned int const indent_level);

extern void dump_class_member_variable_for_one_node(
  std::wfstream &file,
  node_t const * const node,
  unsigned int const idx,
  unsigned int const indent_level);

extern void dump_class_default_constructor_for_one_node(
  std::wfstream &file,
  node_t const * const node,
  unsigned int const idx,
  unsigned int const indent_level);

extern void dump_class_destructor_for_one_node(
  std::wfstream &file,
  node_t const * const node,
  unsigned int const idx,
  unsigned int const indent_level);

extern void dump_pt_XXX_prodn_node_t_class_header(
  std::wfstream &file,
  std::wstring const &rule_node_name,
  int const alternative_id);
  
extern void dump_pt_XXX_prodn_node_t_class_footer(
  std::wfstream &file,
  std::wstring const &rule_node_name,
  int const alternative_id);

extern void dump_add_node_to_nodes_for_one_node_in_parse_XXX(
  std::wfstream &file,
  node_t const * const node,
  unsigned int const indent_depth);

#endif
