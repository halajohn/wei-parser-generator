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

#include "node.hpp"
#include "gen.hpp"
#include "lookahead.hpp"
#include "ga_exception.hpp"
#include "alternative.hpp"
#include "arranged_lookahead.hpp"

#include "wcl_memory_debugger\memory_debugger.h"

namespace
{
  regex_group_id_t
  check_if_meet_regex_OR_end(
    node_t const * const curr_node,
    regex_info_t const * const regex_info,
    bool &meet_regex_OR_one_end,
    bool &meet_regex_OR_total_end)
  {
    assert(curr_node != 0);
    assert(regex_info != 0);
    
    meet_regex_OR_one_end = false;
    meet_regex_OR_total_end = false;
    
    // Check if this regex contains regex_OR.
    if (regex_info->mp_regex_OR_info != 0)
    {
      BOOST_FOREACH(regex_range_t const &regex_range,
                    regex_info->mp_regex_OR_info->m_ranges)
      {
        if (curr_node == regex_range.mp_end_node)
        {
          meet_regex_OR_one_end = true;
          
          if (regex_range.m_regex_group_id ==
              regex_info->mp_regex_OR_info->m_ranges.back().m_regex_group_id)
          {
            meet_regex_OR_total_end = true;
          }
          
          return regex_range.m_regex_group_id;
        }
      }
    }
    
    return REGEX_GROUP_ID_NONE;
  }
  
  regex_group_id_t
  find_regex_group_id_of_regex_OR_range_which_curr_node_reside(
    node_t const * const curr_node,
	std::list<regex_range_t> const &regex_ranges)
  {
    BOOST_FOREACH(regex_range_t const &regex_range, regex_ranges)
    {
      if (true == check_if_node_in_node_range(
            curr_node,
            regex_range.mp_start_node,
            regex_range.mp_end_node))
      {
        return regex_range.m_regex_group_id;
      }
    }
    
    return REGEX_GROUP_ID_NONE;
  }
  
  void
  emit_regex_OR_end_codes(
    std::wfstream &file,
    node_t const * const curr_node,
    std::wstring const &rule_node_name,
    regex_info_t const * const regex_info,
    bool &emit_empty_line_this_time,
    unsigned int &indent_depth)
  {
    assert(curr_node != 0);
    assert(regex_info != 0);
    
    bool meet_regex_OR_one_end;
    bool meet_regex_OR_total_end;
    
    regex_group_id_t const regex_group_id =
      check_if_meet_regex_OR_end(
        curr_node,
        regex_info,
        meet_regex_OR_one_end,
        meet_regex_OR_total_end);
    
    if (true == meet_regex_OR_one_end)
    {
      if (true == emit_empty_line_this_time)
      {
        file << std::endl;
        
        emit_empty_line_this_time = false;
      }
      
      file << indent_line(indent_depth) << "pt_"
           << rule_node_name << "_prod0_regex" << regex_info->m_ranges.front().m_regex_group_id
           << ".mp_regex_OR_node = pt_" << rule_node_name << "_prod0_regex"
           << regex_group_id << ";" << std::endl
           << indent_line(indent_depth) << "pt_"
           << rule_node_name << "_prod0_regex" << regex_info->m_ranges.front().m_regex_group_id
           << ".regex_OR_type_id = " << regex_group_id << ";" << std::endl;
      
      assert(indent_depth > 0);
      --indent_depth;
    
      // Dump the '}' of the above 'case xxx: {'
      // statement.
      file << indent_line(indent_depth) << "}" << std::endl
           << indent_line(indent_depth) << "break;" << std::endl
           << std::endl;
          
      assert(indent_depth > 0);
      --indent_depth;
      
      emit_empty_line_this_time = false;
    }
    
    if (true == meet_regex_OR_total_end)
    {
      // Dump the '}' of the 'switch' statement
      // of regex_OR_groups.
      file << indent_line(indent_depth) << "default:" << std::endl
           << indent_line(indent_depth + 1) << "assert(0);" << std::endl
           << indent_line(indent_depth + 1) << "break;" << std::endl
           << indent_line(indent_depth) << "}" << std::endl
           << std::endl;
          
      emit_empty_line_this_time = false;
    }
  }
}

void
node_t::dump_gen_parser_src__fill_nodes__for_node_range(
  std::wfstream &file) const
{
  std::wstring const &rule_node_name = m_name;
  unsigned int indent_depth = 1;
  std::list<regex_info_t const *> regex_stack;
  bool emit_empty_line_this_time;
  
  // Loop each node of this node range.
  node_t const *curr_node = mp_main_regex_alternative;
  while (curr_node != mp_rule_end_node)
  {
    // If this node belongs to at least one regex group.
    if (curr_node->regex_info().size() != 0)
    {
      // Loop each regex from back to front (ie. from outer
      // most to inner most)
      for (std::list<regex_info_t>::const_reverse_iterator iter =
             curr_node->regex_info().rbegin();
           iter != curr_node->regex_info().rend();
           ++iter)
      {
        assert((*iter).m_type != REGEX_TYPE_OR);
        assert(1 == (*iter).m_ranges.size());
        
        if ((*iter).m_ranges.front().mp_start_node == curr_node)
        {
          // I find a regex started at this node.
          //
          // 'regex_stack' contains regex_groups where the
          // curr_node occurs.
          regex_stack.push_front(&(*iter));
          
          // Everytime when I find a regex started at this
          // node, emit 'while (1)' codes here.
          if ((*iter).m_type != REGEX_TYPE_ONE)
          {
            file << indent_line(indent_depth) << "while (1)" << std::endl
                 << indent_line(indent_depth) << "{" << std::endl;
            
            ++indent_depth;
          }
          
          // If I see a regex, whether its type is
          // REGEX_TYPE_ONE or not, I will emit a local
          // variable of its type.
          file << indent_line(indent_depth) << "pt_" << rule_node_name
               << "_prod0_regex" << (*iter).m_ranges.front().m_regex_group_id << "_t pt_"
               << rule_node_name << "_prod0_regex" << (*iter).m_ranges.front().m_regex_group_id << ";" << std::endl
               << std::endl;
          
          if ((*iter).m_type != REGEX_TYPE_ONE)
          {
            file << indent_line(indent_depth) << "assert(iter != nodes.end());" << std::endl
                 << indent_line(indent_depth) << "assert((*iter) != 0);" << std::endl
                 << std::endl
                 << indent_line(indent_depth) << "pt_regex_chooser_t const * const pt_" << rule_node_name
                 << "_prod0_regex" << (*iter).m_ranges.front().m_regex_group_id
                 << "_chooser = dynamic_cast<pt_regex_chooser_t *>(*iter);" << std::endl
                 << indent_line(indent_depth) << "assert(pt_" << rule_node_name
                 << "_prod0_regex" << (*iter).m_ranges.front().m_regex_group_id
                 << "_chooser != 0);" << std::endl
                 << std::endl;
            
            file << indent_line(indent_depth) << "if (true == pt_" << rule_node_name
                 << "_prod0_regex" << (*iter).m_ranges.front().m_regex_group_id
                 << "_chooser->bypass)" << std::endl
                 << indent_line(indent_depth) << "{" << std::endl;
            
            ++indent_depth;
            
            file << indent_line(indent_depth) << "++iter;" << std::endl
                 << indent_line(indent_depth) << "break;" << std::endl;
            
            --indent_depth;
            
            file << indent_line(indent_depth) << "}" << std::endl
                 << indent_line(indent_depth) << "else" << std::endl
                 << indent_line(indent_depth) << "{" << std::endl;
            
            ++indent_depth;
          }
          
          if ((*iter).mp_regex_OR_info != 0)
          {
            // If this regex contains regex_OR, I will
            // emit a switch statement.
            file << indent_line(indent_depth) << "assert(iter != nodes.end());" << std::endl
                 << indent_line(indent_depth) << "assert((*iter) != 0);" << std::endl
                 << std::endl
                 << indent_line(indent_depth) << "pt_regex_OR_chooser_t const * const chooser = dynamic_cast<pt_regex_OR_chooser_t *>(*iter);" << std::endl
                 << indent_line(indent_depth) << "assert(chooser != 0);" << std::endl
                 << std::endl
                 << indent_line(indent_depth) << "++iter;" << std::endl
                 << std::endl
                 << indent_line(indent_depth) << "switch (chooser->regex_typename_id)" << std::endl
                 << indent_line(indent_depth) << "{" << std::endl;
          }
        }
      }
    }
    
    // If this node is the starting node of a regex_OR, then
    // I will emit a 'case xxx:' statement.
    if (regex_stack.size() != 0)
    {
      if (regex_stack.front()->mp_regex_OR_info != 0)
      {
        BOOST_FOREACH(regex_range_t const &regex_range,
                      regex_stack.front()->mp_regex_OR_info->m_ranges)
        {
          if (curr_node == regex_range.mp_start_node)
          {
            file << indent_line(indent_depth) << "case " << regex_range.m_regex_group_id << ":" << std::endl;
            
            ++indent_depth;
            
            file << indent_line(indent_depth) << "{" << std::endl;
            
            ++indent_depth;
            
            file << indent_line(indent_depth) << "pt_" << rule_node_name
                 << "_prod0_regex" << regex_range.m_regex_group_id << "_t * const pt_"
                 << rule_node_name << "_prod0_regex" << regex_range.m_regex_group_id << " = new pt_"
                 << rule_node_name << "_prod0_regex" << regex_range.m_regex_group_id << "_t;"
                 << std::endl
                 << indent_line(indent_depth) << "assert(pt_" << rule_node_name << "_prod0_regex"
                 << regex_range.m_regex_group_id << " != 0);" << std::endl
                 << std::endl;
            
            break;
          }
        }
      }
    }
    
    regex_group_id_t regex_group_id = REGEX_GROUP_ID_NONE;
    bool this_regex_group_id_represent_regex_OR = false;
    bool meet_regex_OR_one_end = false;
    bool meet_regex_OR_total_end = false;
    
    // Find the correct regex_group_id of the parent regex.
    if (regex_stack.size() != 0)
    {
      // Check if this regex contains regex_OR.
      if (regex_stack.front()->mp_regex_OR_info != 0)
      {
        regex_group_id =
          find_regex_group_id_of_regex_OR_range_which_curr_node_reside(
            curr_node,
            regex_stack.front()->mp_regex_OR_info->m_ranges);
        
        this_regex_group_id_represent_regex_OR = true;
      }
      else
      {
        regex_group_id = regex_stack.front()->m_ranges.front().m_regex_group_id;
        
        this_regex_group_id_represent_regex_OR = false;
      }
      
      assert(regex_group_id != REGEX_GROUP_ID_NONE);
    }
    
    // Dump codes for 'curr_node'.
    {
      file << indent_line(indent_depth) << "assert(iter != nodes.end());" << std::endl
           << indent_line(indent_depth) << "assert((*iter) != 0);" << std::endl
           << std::endl;
      
      file << indent_line(indent_depth);
      
      if (regex_group_id != REGEX_GROUP_ID_NONE)
      {
        file << "pt_" << rule_node_name << "_prod0_regex" << regex_group_id;
        
        if (true == this_regex_group_id_represent_regex_OR)
        {
          file << "->";
        }
        else
        {
          file << ".";
        }
      }
      
      file << "mp_" << curr_node->name() << "_node_"
           << curr_node->name_postfix_by_appear_times();
      
      file << " = dynamic_cast<pt_"
           << curr_node->name() << "_node_t *>(*iter);" << std::endl
           << indent_line(indent_depth) << "++iter;" << std::endl;
    }
    
    emit_empty_line_this_time = true;
    
    if (regex_stack.size() != 0)
    {
      for (std::list<regex_info_t const *>::iterator iter = regex_stack.begin();
           iter != regex_stack.end();
           )
      {
        assert(1 == (*iter)->m_ranges.size());
        
        // Check if this regex contains regex_OR, and
        // 'curr_node' is the end of one of them.
        if ((*iter)->mp_regex_OR_info != 0)
        {
          emit_regex_OR_end_codes(file,
                                  curr_node,
                                  rule_node_name,
                                  *iter,
                                  emit_empty_line_this_time,
                                  indent_depth);
        }
        
        // Check if 'curr_node' is the end node of some regex.
        if ((*iter)->m_ranges.front().mp_end_node == curr_node)
        {
          if (true == emit_empty_line_this_time)
          {
            file << std::endl;
            
            emit_empty_line_this_time = false;
          }
          
          regex_info_t const * const used_regex_info = regex_stack.front();
          
          iter = regex_stack.erase(iter);
          
          file << indent_line(indent_depth);
          
          if (iter != regex_stack.end())
          {
            regex_group_id_t regex_group_id = REGEX_GROUP_ID_NONE;
            bool this_regex_group_id_represent_regex_OR;
            
            if ((*iter)->mp_regex_OR_info != 0)
            {
              regex_group_id = find_regex_group_id_of_regex_OR_range_which_curr_node_reside(
                curr_node,
                (*iter)->mp_regex_OR_info->m_ranges);
              
              this_regex_group_id_represent_regex_OR = true;
            }
            else
            {
              regex_group_id = (*iter)->m_ranges.front().m_regex_group_id;
              
              this_regex_group_id_represent_regex_OR = false;
            }
            
            assert(regex_group_id != REGEX_GROUP_ID_NONE);
            
            file << "pt_" << rule_node_name << "_prod0_regex" << regex_group_id;
            
            if (true == this_regex_group_id_represent_regex_OR)
            {
              file << "->";
            }
            else
            {
              file << ".";
            }
          }
          
          file << "m_regex" << used_regex_info->m_ranges.front().m_regex_group_id;
          
          switch (used_regex_info->m_type)
          {
          case REGEX_TYPE_ZERO_OR_MORE:
          case REGEX_TYPE_ZERO_OR_ONE:
          case REGEX_TYPE_ONE_OR_MORE:
            file << ".push_back(";
            break;
            
          case REGEX_TYPE_ONE:
            file << " = ";
            break;
            
          default:
            assert(0);
            break;
          }
          
          file << "pt_" << rule_node_name << "_prod0_regex" << used_regex_info->m_ranges.front().m_regex_group_id;
          
          switch (used_regex_info->m_type)
          {
          case REGEX_TYPE_ZERO_OR_MORE:
          case REGEX_TYPE_ZERO_OR_ONE:
          case REGEX_TYPE_ONE_OR_MORE:
            file << ")";
            break;
            
          case REGEX_TYPE_ONE:
            break;
            
          default:
            assert(0);
            break;
          }
          
          file << ";" << std::endl;
          
          emit_empty_line_this_time = true;
          
          if (used_regex_info->m_type != REGEX_TYPE_ONE)
          {
            assert(indent_depth > 0);
            --indent_depth;
            
            // Dump '}' for the 'if (true ==
            // pt_X_prod0_regexX_chooser->bypass)' above.
            file << indent_line(indent_depth) << "}" << std::endl;
            
            assert(indent_depth > 0);
            --indent_depth;
            
            // Dump the '}' of the 'while (1) {'
            // statement of each regex group.
            file << indent_line(indent_depth) << "}" << std::endl
                 << std::endl;
            
            emit_empty_line_this_time = false;
          }
        }
        else
        {
          ++iter;
        }
      }
    }
    
    if (true == emit_empty_line_this_time)
    {
      file << std::endl;
    }
    
    assert(1 == curr_node->next_nodes().size());
    curr_node = curr_node->next_nodes().front();
  }
}

namespace
{
  boost::shared_ptr<arranged_lookahead_t>
  collect_lookahead_tree_for_one_regex_stack(
    node_t const * const rule_node,
    std::list<regex_info_t>::const_reverse_iterator iter,
    node_t * const default_node)
  {
    assert(true == rule_node->is_rule_head());
    
    // Collect lookahead nodes for this regex, and
    // calculate the arranged_lookahead tree for this
    // node group.
    //
    // Ex:
    //
    // a (b c)* d
    //
    // Then collect lookahead nodes for 'b' and 'd'.
    //
    // a (b | c)* d
    //
    // Then collect lookahead nodes for 'b', 'c', and 'd'.
    std::list<node_t *> nodes;            
    
    collect_candidate_nodes_for_lookahead_calculation_for_EBNF_rule(
      rule_node, iter, nodes);
    
    std::list<node_with_order_t> nodes_with_order;
    
    BOOST_FOREACH(node_t * const node, nodes)
    {
      nodes_with_order.push_back(node_with_order_t(0, node));
    }
    
    nodes.clear();
    
    // Calculate the arranged_lookahead tree for
    // this node group.
    boost::shared_ptr<arranged_lookahead_t> arranged_lookahead(
      collect_lookahead_into_arrange_lookahead(nodes_with_order, default_node));
    
    // This step is very important, because I need all
    // nodes in this arranged_lookahead tree belong the
    // main regex alternative, then I can use it as a
    // hint to traverse the whole main regex
    // alternative.
    change_target_node_of_arranged_lookahead_tree_nodes_to_its_corresponding_one_in_main_regex_alternative(
      arranged_lookahead.get(),
      default_node);
        
    merge_arranged_lookahead_tree_leaf_node_if_they_have_same_target_node(
      arranged_lookahead.get(),
      default_node);
    
    return arranged_lookahead;
  }
}

void
node_t::collect_regex_info_into_regex_stack(
  std::wfstream &file,
  node_t const * const curr_node,
  std::list<regex_info_with_arranged_lookahead_t> &regex_stack,
  unsigned int &indent_depth,
  node_t * const default_node) const
{
  std::wstring const &rule_node_name = m_name;
  
  // Loop each regex from back to front (ie. from outer
  // most to inner most)
  for (std::list<regex_info_t>::const_reverse_iterator iter =
         curr_node->regex_info().rbegin();
       iter != curr_node->regex_info().rend();
       ++iter)
  {
    assert((*iter).m_type != REGEX_TYPE_OR);
    assert(1 == (*iter).m_ranges.size());
    
    if ((*iter).m_ranges.front().mp_start_node == curr_node)
    {
      // I find a regex started at this node.
      //
      // 'regex_stack' contains regex_groups where the
      // curr_node occurs.
      regex_stack.push_front(&(*iter));
      
      boost::shared_ptr<arranged_lookahead_t> arranged_lookahead(
        collect_lookahead_tree_for_one_regex_stack(this, iter, default_node));
      
      // Save this newly created arranged_lookahead
      // tree into 'regex_stack'.
      regex_stack.front().mp_arranged_lookahead_tree = arranged_lookahead;
      
      // Initialize the 'arranged_lookahead_tree_iter' to
      // the beginning of this arranged_lookahead tree.
      regex_stack.front().m_arranged_lookahead_tree_iter = arranged_lookahead->begin();
      
      // The lookahead tree can not be empty.
      assert(regex_stack.front().m_arranged_lookahead_tree_iter !=
             arranged_lookahead->end());
    }
  }
}

void
node_t::dump_gen_parser_src__parse_XXX__each_regex_level_header_codes(
  std::wfstream &file,
  std::list<regex_info_with_arranged_lookahead_t> &regex_stack,
  unsigned int &indent_depth,
  node_t * const default_node) const
{
  std::wstring const &rule_node_name = m_name;
  
  if (regex_stack.back().m_arranged_lookahead_tree_iter ==
      regex_stack.back().mp_arranged_lookahead_tree->begin())
  {
    if (regex_stack.back().mp_regex_info->m_type != REGEX_TYPE_ONE)
    {
      file << indent_line(indent_depth) << "while (1)" << std::endl
           << indent_line(indent_depth) << "{" << std::endl;
      
      ++indent_depth;
      
      file << indent_line(indent_depth) << "pt_regex_chooser_t * const pt_" << rule_node_name
           << "_prod0_regex" << regex_stack.back().mp_regex_info->m_ranges.front().m_regex_group_id
           << "_chooser = new pt_regex_chooser_t();" << std::endl
           << indent_line(indent_depth) << "assert(pt_" << rule_node_name
           << "_prod0_regex" << regex_stack.back().mp_regex_info->m_ranges.front().m_regex_group_id
           << "_chooser != 0);" << std::endl
           << std::endl
           << indent_line(indent_depth) << "pt_" << rule_node_name
           << "_prod0_regex" << regex_stack.back().mp_regex_info->m_ranges.front().m_regex_group_id
           << "_chooser->bypass = false;" << std::endl
           << std::endl
           << indent_line(indent_depth) << "add_node_to_nodes(nodes, pt_" << rule_node_name
           << "_prod0_regex" << regex_stack.back().mp_regex_info->m_ranges.front().m_regex_group_id
           << "_chooser);" << std::endl
           << std::endl;
    }
  }
      
  // I have already found a new leaf node of this
  // arranged_lookahead tree.
  std::list<arranged_lookahead_t const *> lookahead_sequence;
  
  // Find the lookahead nodes sequence of this new
  // arranged_lookahead tree leaf node.
  (*(regex_stack.back().m_arranged_lookahead_tree_iter))->
    find_lookahead_sequence(lookahead_sequence);
  
  std::list<arranged_lookahead_t const *>::iterator iter1;
  std::list<arranged_lookahead_t const *>::iterator iter2;
  
  // Find the differences of this new lookahead nodes
  // sequence and the one which has already dumpped.
  find_first_different_arranged_lookahead(
    regex_stack.back().mp_current_dumpped_lookahead_nodes,
    lookahead_sequence,
    iter1,
    iter2);
  
  // I just need to count the number between 'iter1' and
  // the end of this list, however, STL count_if needs
  // the 3rd argument which is a predict
  // function. Thus, I just a very simple boost::lambda
  // (_1 != 0) to fill this slot.
  size_t const need_to_delete_count = std::count_if(
    iter1,
    regex_stack.back().mp_current_dumpped_lookahead_nodes.end(),
    boost::lambda::_1 != static_cast<arranged_lookahead_t const *>(0));
  
  // The value of 'need_to_delete_count' should be 1 or
  // 2. If it is 2, that is to say, I am travelling from
  // a 'default' node to a new node.
  // 
  // Ex:
  //
  // arranged_lookahead tree:
  //
  // * --- b --- c --- d
  //   |     |     |
  //   |     |     +-- e
  //   |     |     |
  //   |     |     +-- default
  //   |     |
  //   |     +-- f --- g
  //   |     |     |
  //   |     |     +-- default
  //   |     |
  //   |     +-- default
  //   |
  //   +-- default
  //
  // If I traverse from 'd' to 'e', then I have to emit
  // some codes to close the codes for node 'd'.
  assert((0 == need_to_delete_count) ||
         (1 == need_to_delete_count) ||
         (2 == need_to_delete_count));
  
  // Check if I need to finish the code segment of the
  // previous dumpped lookahead nodes. i.e. 'case XXX: {'
  // statement.
  if (need_to_delete_count > 0)
  {
    assert(indent_depth > 0);
    --indent_depth;
    
    file << indent_line(indent_depth) << "}" << std::endl
         << indent_line(indent_depth) << "break;" << std::endl
         << std::endl;
    
    assert(indent_depth > 0);
    --indent_depth;
  }
  
  regex_stack.back().mp_current_dumpped_lookahead_nodes.erase(
    iter1,
    regex_stack.back().mp_current_dumpped_lookahead_nodes.end());
  
  lookahead_sequence.erase(lookahead_sequence.begin(), iter2);
  
  // Dump codes for the new lookahead nodes.
  {
    size_t i = regex_stack.back().mp_current_dumpped_lookahead_nodes.size();
    
    ++i;
    
    // Loop each level of the lookahead tree. The order in
    // 'lookahead_sequence' is as following:
    //
    // Ex:
    //
    // lookahead_sequence:
    //
    // ------>
    // d c b a
    //
    // arranged_lookahead tree:
    //
    // <------------------
    //
    // a --- b --- c --- d
    //   |
    //   +-- e --- f --- g
    //         |
    //         +-- h --- i
    //
    BOOST_FOREACH(arranged_lookahead_t const *arranged_lookahead,
                  lookahead_sequence)
    {
      assert(arranged_lookahead->parent() != 0);
      
      // Before the first lookahead symbol, I need to emit
      // codes to fetch new token at runtime.
      if (arranged_lookahead == &(arranged_lookahead->parent()->children().front()))
      {
        file << indent_line(indent_depth) << "token_t * const token_" << i
             << " = lexer_peek_token("<< i << ");" << std::endl
             << indent_line(indent_depth) << "switch (token_" << i << "->get_type())" << std::endl
             << indent_line(indent_depth) << "{" << std::endl;
      }
            
      // Start to dump the case statments for each lookahead
      // symbol.
      BOOST_FOREACH(node_t const *node,
                    arranged_lookahead->lookahead_nodes())
      {
        if (node == default_node)
        {
          assert(1 == arranged_lookahead->lookahead_nodes().size());
          
          file << indent_line(indent_depth) << "default";
        }
        else
        {
          file << indent_line(indent_depth) << "case WDS_TOKEN_TYPE_";
          
          if (0 == node->name().size())
          {
            file << "EOF";
          }
          else
          {
            std::wstring name = node->name();
            
            // transform it to all capital letters.
            std::transform(name.begin(), name.end(), name.begin(), towupper);
            
            file << name;
          }
        }
        
        file << ":" << std::endl;
      }
      
      ++indent_depth;
      
      file << indent_line(indent_depth) << "{" << std::endl;
      
      ++indent_depth;
      
      ++i;
    }
  }
  
  regex_stack.back().mp_current_dumpped_lookahead_nodes.splice(
    regex_stack.back().mp_current_dumpped_lookahead_nodes.end(),
    lookahead_sequence);
  
  assert(regex_stack.back().mp_current_dumpped_lookahead_nodes.back()->
         target_nodes().size() <= 1);
}

namespace
{
  /// Ex:
  ///
  /// ((a)* (b)* c)
  /// ^           ^
  ///
  /// The previous successive inner regexes before node 'c' at
  /// regex pointed by '^' are:
  /// 
  /// (a)* & (b)*
  ///
  /// Ex:
  ///
  /// (a (b)* c)
  /// ^           ^
  ///
  /// The previous successive inner regexes before node 'c' at
  /// regex pointed by '^' are empty.
  ///
  /// \param node 
  /// \param node_regex_info_iter 
  /// \param nodes 
  ///
  void
  find_previous_successive_inner_regex(
    node_t * const node,
    regex_info_t const * const regex_info,
    std::list<regex_info_t const *> &prev_regex_info_list)
  {
    assert(node != 0);
    assert(regex_info != 0);
    
    node_t * const regex_start_node = regex_info->m_ranges.front().mp_start_node;
    node_t * const regex_end_node = regex_info->m_ranges.front().mp_end_node;
    
    bool is_regex_OR_range;
    
    regex_range_t const *regex_range = 
      find_regex_range_which_this_node_belongs_to(
        regex_info,
        node,
        &is_regex_OR_range);
    
    assert(regex_range != 0);
    
    node_t *checking_node = regex_range->mp_start_node;
    
    while (1)
    {
      std::list<regex_info_t>::const_reverse_iterator checking_node_iter =
        find_regex_info_using_its_begin_and_end_node(
          checking_node->regex_info(),
          regex_start_node,
          regex_end_node);
      
      std::list<regex_info_t>::const_reverse_iterator end_iter =
        checking_node->regex_info().rend();
      
      assert(checking_node_iter != end_iter);
      
      --end_iter;
      
      if (checking_node_iter != end_iter)
      {
        ++checking_node_iter;
        
        node_t const * const checking_node_inner_regex_start_node =
          (*checking_node_iter).m_ranges.front().mp_start_node;
        
        node_t const * const checking_node_inner_regex_end_node =
          (*checking_node_iter).m_ranges.front().mp_end_node;
        
        if (true == check_if_node_in_node_range(
              node,
              checking_node_inner_regex_start_node,
              checking_node_inner_regex_end_node))
        {
          // I have already found that node.
          return;
        }
        else
        {
          prev_regex_info_list.push_back(&(*checking_node_iter));
          
          assert(checking_node_inner_regex_end_node->name().size() != 0);
          assert(1 == checking_node_inner_regex_end_node->next_nodes().size());
          
          checking_node = checking_node_inner_regex_end_node->next_nodes().front();
        }
      }
      else
      {
        return;
      }
    }
  }
}

void
node_t::dump_gen_parser_src__parse_XXX__real(
  std::wfstream &file) const
{
  std::wstring const &rule_node_name = m_name;
  unsigned int indent_depth = 1;
  std::list<regex_info_with_arranged_lookahead_t> regex_stack;
  std::list<regex_info_with_arranged_lookahead_t> tmp_regex_stack;
  
  // Used in arranged_lookahead tree.
  boost::shared_ptr<node_t> default_node(
    new node_t(mp_ae, mp_rule_node, std::wstring()));
  assert(default_node.get() != 0);
  
  node_t const *curr_node = mp_main_regex_alternative;
  
  while (1)
  {
    bool has_node_before_any_regex = false;
    
    // Loop each node of this alternative before any regexs.
    while (curr_node != mp_rule_end_node)
    {
      if (curr_node->regex_info().size() != 0)
      {
        break;
      }
      else
      {
        dump_add_node_to_nodes_for_one_node_in_parse_XXX(file, curr_node, indent_depth);
        
        has_node_before_any_regex = true;
      }
      
      assert(1 == curr_node->next_nodes().size());
      curr_node = curr_node->next_nodes().front();
    }
    
    if (true == has_node_before_any_regex)
    {
      file << std::endl;
    }
    
    if (curr_node == mp_rule_end_node)
    {
      break;
    }
    
    assert(curr_node->regex_info().size() != 0);
    assert(1 == curr_node->regex_info().back().m_ranges.front().mp_end_node->next_nodes().size());
  
    node_t const * const first_node_beyond_this_regex =
      curr_node->regex_info().back().m_ranges.front().mp_end_node->next_nodes().front();
  
    collect_regex_info_into_regex_stack(
      file, curr_node, regex_stack, indent_depth, default_node.get());
  
    while (regex_stack.size() != 0)
    {
      bool restart = false;
    
      // Check if I have finished this regex.
      if (regex_stack.back().m_arranged_lookahead_tree_iter !=
          regex_stack.back().mp_arranged_lookahead_tree->end())
      {
        if (true == regex_stack.back().m_first_enter_this_regex)
        {
          // Dump codes for token fetching at runtime and
          // 'switch' and 'case' statements.
          dump_gen_parser_src__parse_XXX__each_regex_level_header_codes(
            file, regex_stack, indent_depth, default_node.get());
        }
        
        arranged_lookahead_t const * const curr_dumpped_lookahead =
          regex_stack.back().mp_current_dumpped_lookahead_nodes.back();
        
        // 0 means the 'default' node.
        if (0 == curr_dumpped_lookahead->target_nodes().size())
        {
          assert(curr_dumpped_lookahead->lookahead_nodes().front() == default_node.get());
          
          file << indent_line(indent_depth) << "throw std::exception();" << std::endl;
          
          --indent_depth;
        
          // Because there is no other lookahead symbol in
          // this regex could end the codes for this 'default'
          // node lookahead symbol, so that I have to close it
          // here now.
          file << indent_line(indent_depth) << "}" << std::endl
               << indent_line(indent_depth) << "break;" << std::endl;
        
          --indent_depth;
        
          // Close the 'switch (XXX) {' statement for this
          // regex.
          file << indent_line(indent_depth) << "}" << std::endl;
        }
        else
        {
          // This value must be 1, otherwise an ambiguity
          // condition.
          assert(1 == curr_dumpped_lookahead->target_nodes().size());
          
          // Check if I am in the current regex.
          if (true == check_if_node_in_node_range(
                curr_dumpped_lookahead->target_nodes().front().mp_node,
                regex_stack.back().mp_regex_info->m_ranges.front().mp_start_node,
                regex_stack.back().mp_regex_info->m_ranges.front().mp_end_node))
          {
            assert(regex_stack.back().mp_curr_node != 0);
            
            if (true == regex_stack.back().m_first_enter_this_regex)
            {
              // Remember where am I in this regex.
              regex_stack.back().mp_curr_node =
                curr_dumpped_lookahead->target_nodes().front().mp_node;
            }
            else
            {
              // If I reach here, this means I traverse this
              // regex_range after travsing some inner regex.
              //
              // Ex:
              //
              // a (b (c d)* e)*
              //             ^
              //
              // When I in 'e', I will enter this
              // situation. In this situation, the
              // 'mp_curr_node' of this regex will be set when
              // I move element from 'tmp_regex_stack' to
              // 'regex_stack'.
            }
            
            // I don't need to do anything about the rule end
            // node.
            if (regex_stack.back().mp_curr_node->name().size() != 0)
            {
              bool is_regex_OR_range;
              
              regex_range_t const * regex_range =
                find_regex_range_which_this_node_belongs_to(
                  regex_stack.back().mp_regex_info,
                  regex_stack.back().mp_curr_node,
                  &is_regex_OR_range);
              
              // I have already split the condition which will 
              // leave this current regex in the previous
              // codes, so that I should always find a
              // regex_range here.
              assert(regex_range != 0);
              
              assert(regex_range->mp_end_node !=0);
              assert(1 == regex_range->mp_end_node->next_nodes().size());
              
              if (true == is_regex_OR_range)
              {
                if (regex_stack.back().mp_curr_node == regex_range->mp_start_node)
                {
                  file << indent_line(indent_depth) << "pt_regex_OR_chooser_t * const chooser = new pt_regex_OR_chooser_t();" << std::endl
                       << indent_line(indent_depth) << "assert(chooser != 0);" << std::endl
                       << std::endl
                       << indent_line(indent_depth) << "chooser->regex_typename_id = " << regex_range->m_regex_group_id << ";" << std::endl
                       << std::endl
                       << indent_line(indent_depth) << "add_node_to_nodes(nodes, chooser);" << std::endl
                       << std::endl;
                }
              }
              
              node_t * const real_end_node = regex_range->mp_end_node->next_nodes().front();
              
              bool has_dumpped_prev_skip_regex_when_first_enter_this_regex = false;
              
              // Start to dump codes for each node.
              while (regex_stack.back().mp_curr_node != real_end_node)
              {
                if (regex_stack.back().mp_curr_node->regex_info().size() > 0)
                {
                  // This node belongs to at least one regex,
                  // and I should check if this node contains
                  // any more inner regex. If yes, I should
                  // emit codes of this newly found inner
                  // regex, otherwise, I should emit codes for
                  // this node directly.
                  std::list<regex_info_t>::const_reverse_iterator iter =
                    find_regex_info_using_its_begin_and_end_node(
                      regex_stack.back().mp_curr_node->regex_info(),
                      regex_stack.back().mp_regex_info->m_ranges.front().mp_start_node,
                      regex_stack.back().mp_regex_info->m_ranges.front().mp_end_node);
                  
                  if (&(*iter) != &(regex_stack.back().mp_curr_node->regex_info().front()))
                  {
                    // If I see a new inner regex, and this
                    // regex is not added to the
                    // regex_stack, then I will add it now.
                    assert(regex_stack.size() != 0);
                    
                    // One condition which I can use to
                    // determine whether the regexes of this
                    // node is needed to add to the
                    // regex_stack or not is whether the
                    // size of the regex_stack is 1 or
                    // not.
                    //
                    // If the current size of 'regex_stack'
                    // is 1, that is to say the inner most
                    // regex I have met until now is the one
                    // located in 'regex_stack', and if I
                    // found new inner regex in 'curr_node',
                    // this means I should add it into
                    // 'regex_stack'.
                    if (1 == regex_stack.size())
                    {
                      std::list<regex_info_t>::const_reverse_iterator iter2 = iter;
                      
                      ++iter2;
                      
                      assert((*iter2).m_ranges.front().mp_start_node == regex_stack.back().mp_curr_node);
                      
                      for (;
                           iter2 != regex_stack.back().mp_curr_node->regex_info().rend();
                           ++iter2)
                      {
                        regex_stack.push_front(&(*iter2));
                        
                        boost::shared_ptr<arranged_lookahead_t> arranged_lookahead(
                          collect_lookahead_tree_for_one_regex_stack(this, iter2, default_node.get()));
                        
                        // Save this newly created arranged_lookahead
                        // tree into 'regex_stack'.
                        regex_stack.front().mp_arranged_lookahead_tree = arranged_lookahead;
                        
                        // Initialize the 'arranged_lookahead_tree_iter' to
                        // the beginning of this arranged_lookahead tree.
                        regex_stack.front().m_arranged_lookahead_tree_iter = arranged_lookahead->begin();
                      }
                    }
                    
                    regex_stack.back().m_first_enter_this_regex = false;
                    
                    // Move the current regex_info into
                    // 'tmp_regex_stack', so that making the
                    // inner regex as the top (back) of
                    // 'regex_stack'.
                    tmp_regex_stack.push_front(regex_stack.back());
                    regex_stack.pop_back();
                    
                    restart = true;
                    
                    break;
                  }
                  else
                  {
                    if ((true == regex_stack.back().m_first_enter_this_regex) &&
                        (false == has_dumpped_prev_skip_regex_when_first_enter_this_regex))
                    {
                      // Ex:
                      //
                      // ((a b)* (c d)* e)
                      // ^               ^
                      // When I dump codes for the regex
                      // pointed by '^', I have to consider
                      // if the first token is node 'e'. If
                      // it is, then I have to emit codes to
                      // bypass the first regex (ie. (a b)*)
                      // and the second regex (ie. (c d)*)
                      std::list<regex_info_t const *> prev_regex_info_list;
                      
                      find_previous_successive_inner_regex(
                        regex_stack.back().mp_curr_node,
                        regex_stack.back().mp_regex_info,
                        prev_regex_info_list);
                      
                      BOOST_FOREACH(regex_info_t const * const passed_regex_info,
                                    prev_regex_info_list)
                      {
                        file << indent_line(indent_depth) << "{" << std::endl;
                        
                        ++indent_depth;
                        
                        file << indent_line(indent_depth) << "pt_regex_chooser_t * const pt_" << rule_node_name
                             << "_prod0_regex" << passed_regex_info->m_ranges.front().m_regex_group_id
                             << "_chooser = new pt_regex_chooser_t();" << std::endl
                             << indent_line(indent_depth) << "assert(pt_" << rule_node_name << "_prod0_regex"
                             << passed_regex_info->m_ranges.front().m_regex_group_id << "_chooser != 0);"
                             << std::endl
                             << std::endl
                             << indent_line(indent_depth) << "pt_" << rule_node_name << "_prod0_regex"
                             << passed_regex_info->m_ranges.front().m_regex_group_id << "_chooser->bypass = true;"
                             << std::endl
                             << std::endl
                             << indent_line(indent_depth) << "add_node_to_nodes(nodes, pt_" << rule_node_name << "_prod0_regex"
                             << passed_regex_info->m_ranges.front().m_regex_group_id << "_chooser);" << std::endl;
                        
                        --indent_depth;
                        
                        file << indent_line(indent_depth) << "}" << std::endl
                             << std::endl;
                      }
                      
                      has_dumpped_prev_skip_regex_when_first_enter_this_regex = true;                      
                    }
                    
                    dump_add_node_to_nodes_for_one_node_in_parse_XXX(file, regex_stack.back().mp_curr_node, indent_depth);
                  }
                  
                  assert(1 == regex_stack.back().mp_curr_node->next_nodes().size());
                  regex_stack.back().mp_curr_node = regex_stack.back().mp_curr_node->next_nodes().front();
                }
                else
                {
                  // This node doesn't belong to any regex, so
                  // in the normal case, I should emit codes
                  // for it directly.
                  if (0 == regex_stack.back().mp_curr_node->name().size())
                  {
                    // This is the rule end node, and I don't
                    // need to emit any codes for this kind of
                    // node.
                    assert(0 == regex_stack.back().mp_curr_node->next_nodes().size());
                    break;
                  }
                  else
                  {
                    dump_add_node_to_nodes_for_one_node_in_parse_XXX(file, regex_stack.back().mp_curr_node, indent_depth);
                    
                    assert(1 == regex_stack.back().mp_curr_node->next_nodes().size());
                    regex_stack.back().mp_curr_node = regex_stack.back().mp_curr_node->next_nodes().front();
                  }
                }
                
                assert(regex_stack.back().mp_curr_node != 0);
              }
            }
          }
          else
          {
            switch (regex_stack.back().mp_regex_info->m_type)
            {
            case REGEX_TYPE_ZERO_OR_ONE:
            case REGEX_TYPE_ONE_OR_MORE:
            case REGEX_TYPE_ZERO_OR_MORE:
              file << indent_line(indent_depth) << "pt_" << rule_node_name
                   << "_prod0_regex" << regex_stack.back().mp_regex_info->m_ranges.front().m_regex_group_id
                   << "_chooser->bypass = true;" << std::endl;
              break;
              
            case REGEX_TYPE_ONE:
              break;
              
            case REGEX_TYPE_OR:
            default:
              assert(0);
              break;
            }
          }
        }
        
        if (true == restart)
        {
          continue;
        }
        else
        {
          ++(regex_stack.back().m_arranged_lookahead_tree_iter);
          
          // This is a new lookahead symbol, and I have to
          // initialize the 'mp_curr_node' of this regex_info
          // to the first node of the regex_range it belongs
          // to.
          regex_stack.back().m_first_enter_this_regex = true;
        }
      }
      else
      {
        // I have already finished the lookahead tree for this
        // regex, thus I will kick off this regex.
        //
        // :NOTE:
        //
        // When I finish dumpping a regex, this means the
        // regex_stack _only_ contains this regex_info, and
        // others are in 'tmp_regex_stack' (if any).
        assert(1 == regex_stack.size());
        
        if (regex_stack.back().mp_regex_info->m_type != REGEX_TYPE_ONE)
        {
          file << std::endl
               << indent_line(indent_depth) << "if (true == pt_" << rule_node_name
               << "_prod0_regex" << regex_stack.back().mp_regex_info->m_ranges.front().m_regex_group_id
               << "_chooser->bypass)" << std::endl
               << indent_line(indent_depth) << "{" << std::endl;
          
          ++indent_depth;
        
          file << indent_line(indent_depth) << "break;" << std::endl;
        
          --indent_depth;
        
          file << indent_line(indent_depth) << "}" << std::endl;
        
          assert(indent_depth > 0);
          --indent_depth;
          
          // Close the 'while (1)' codes for this regex.
          file << indent_line(indent_depth) << "}" << std::endl;
          
          // Decide if I need to dump a new line here or
          // not.
          if (0 == tmp_regex_stack.size())
          {
            file << std::endl;
          }
          else
          {
            regex_info_t const * const last_regex_info = regex_stack.back().mp_regex_info;
            regex_info_t const * const just_before_last_regex_info = tmp_regex_stack.front().mp_regex_info;
            
            if (last_regex_info->m_ranges.front().mp_end_node !=
                just_before_last_regex_info->m_ranges.front().mp_end_node)
            {
              file << std::endl;
            }                
          }
        }
        
        // If there are other outer regex, then I have to
        // handle it now.
        if (tmp_regex_stack.size() != 0)
        {
          // Determine the 'curr_node' of this outer regex.
          tmp_regex_stack.front().mp_curr_node = regex_stack.back().mp_regex_info->m_ranges.front().mp_end_node;
          
          // If the 'curr_node' is the last node of this
          // outer regex, then I don't need to advance it.
          //
          // Ex:
          //
          // (a (b c)*) d
          //    ^   ^
          //
          // If I just finish the regex pointed by '^', then
          // I can _NOT_ advance the 'curr_node' of the outer
          // regex to the next node (i.e. node 'd'), the
          // 'curr_node' of the outer node should still stay
          // in the node 'c'.
          bool is_regex_OR_range;
          
          regex_range_t const * regex_range =
            find_regex_range_which_this_node_belongs_to(
              tmp_regex_stack.front().mp_regex_info,
              tmp_regex_stack.front().mp_curr_node,
              &is_regex_OR_range);
          
          if (regex_range->mp_end_node == tmp_regex_stack.front().mp_curr_node)
          {
            ++(tmp_regex_stack.front().m_arranged_lookahead_tree_iter);
            
            // This is a new lookahead symbol, and I have to
            // initialize the 'mp_curr_node' of this regex_info
            // to the first node of the regex_range it belongs
            // to.
            tmp_regex_stack.front().m_first_enter_this_regex = true;
          }
          else
          {
            // advance the 'curr_node' of this 'tmp_regex_stack'.
            tmp_regex_stack.front().mp_curr_node = tmp_regex_stack.front().mp_curr_node->next_nodes().front();
          }
          
          // Move this outer regex from 'tmp_regex_stack' to
          // 'regex_stack'.
          regex_stack.push_back(tmp_regex_stack.front());
          tmp_regex_stack.pop_front();
        }
        
        regex_stack.pop_front();
      }
    }
    
    if (0 == first_node_beyond_this_regex->name().size())
    {
      break;
    }
    else
    {
      curr_node = first_node_beyond_this_regex;
    }
  }
}

void
node_t::dump_gen_parser_src__fill_nodes__for_regex_alternative(
  std::wfstream &file) const
{
  // Dump function header for the 'fill_nodes' function.
  file << indent_line(0) << "void" << std::endl
       << indent_line(0) << "pt_" << m_name << "_prod0" << "_node_t::fill_nodes(std::list<pt_node_t *> const &nodes)" << std::endl
       << indent_line(0) << "{" << std::endl
       << indent_line(1) << "std::list<pt_node_t *>::const_iterator iter = nodes.begin();" << std::endl
       << std::endl;
  
  dump_gen_parser_src__fill_nodes__for_node_range(file);
  
  // Dump function footer for the 'fill_nodes' function.
  file << indent_line(1) << "assert(iter == nodes.end());" << std::endl
       << "}" << std::endl;
}

void
node_t::dump_gen_parser_src__parse_XXX__for_regex_alternative(
  std::wfstream &file) const
{
  file << "pt_node_t *" << std::endl
       << "frontend::parse_" << m_name << "(bool const consume)" << std::endl
       << "{" << std::endl
       << "  std::list<pt_node_t *> nodes;" << std::endl
       << "  pt_" << m_name << "_prod_node_t *prod_node = new pt_" << m_name << "_prod0_node_t;" << std::endl
       << std::endl
       << "  assert(prod_node != 0);" << std::endl
       << std::endl;
  
  if (true == m_contains_ambigious)
  {
    file << indent_line(1) << "// ambiguity" << std::endl;
  }
  else
  {
    dump_gen_parser_src__parse_XXX__real(file);
  }
  
  file << indent_line(1) << "if (true == consume)" << std::endl
       << indent_line(1) << "{" << std::endl
       << indent_line(2) << "prod_node->fill_nodes(nodes);" << std::endl
       << std::endl
       << indent_line(2) << "pt_" << m_name << "_node_t * const node = new pt_" << m_name << "_node_t;" << std::endl
       << indent_line(2) << "assert(node != 0);" << std::endl
       << std::endl
       << indent_line(2) << "node->set_prod_node(prod_node);" << std::endl
       << std::endl
       << indent_line(2) << "return node;" << std::endl
       << indent_line(1) << "}" << std::endl
       << indent_line(1) << "else" << std::endl
       << indent_line(1) << "{" << std::endl
       << indent_line(2) << "return 0;" << std::endl
       << indent_line(1) << "}" << std::endl
       << "}" << std::endl;
}

void
node_t::dump_gen_parser_src_for_regex_alternative(
  std::wfstream &file) const
{
  assert(true == m_is_rule_head);
  assert(m_rule_contain_regex != RULE_CONTAIN_NO_REGEX);
  assert(mp_main_regex_alternative != 0);
  
  // ===========================================
  //         Dump 'default constructor'
  // ===========================================
  file << indent_line(0) << "pt_" << m_name << "_prod0" << "_node_t::"
       << "pt_" << m_name << "_prod0" << "_node_t()" << std::endl;
  
  assert(mp_main_regex_alternative->name().size() != 0);
  
  {
    bool first_time = true;
    
    node_t const *curr_node = mp_main_regex_alternative;
    while (curr_node->name().size() != 0)
    {
      if (curr_node->regex_info().size() != 0)
      {
        assert(curr_node->regex_info().back().m_type != REGEX_TYPE_OR);
        assert(1 == curr_node->regex_info().back().m_ranges.size());
        
        // skip to the end node of this regex.
        curr_node = curr_node->regex_info().back().m_ranges.front().mp_end_node;
      }
      else
      {
        // If this node isn't the starting node of
        // any regex, then I will emit its codes
        // normally and directly.
        if (false == first_time)
        {
          file << "," << std::endl
               << indent_line(0) << "   ";
        }
        else
        {
          file << " : ";
        }
        
        dump_class_default_constructor_for_one_node(file, curr_node, 0, 0);
        
        first_time = false;
      }
      
      assert(1 == curr_node->next_nodes().size());
      curr_node = curr_node->next_nodes().front();
    }
    
    if (false == first_time)
    {
      file << std::endl;
    }
  }
  
  file << indent_line(0) << "{" << std::endl
       << indent_line(0) << "}" << std::endl
       << std::endl;
  
  // ===========================================
  //         Dump destructor
  // ===========================================
  file << indent_line(0) << "pt_" << m_name << "_prod0" << "_node_t::"
       << "~pt_" << m_name << "_prod0" << "_node_t()" << std::endl
       << indent_line(0) << "{" << std::endl;
  
  assert(mp_main_regex_alternative->name().size() != 0);
  
  {
    node_t const *curr_node = mp_main_regex_alternative;
    while (curr_node->name().size() != 0)
    {
      if (curr_node->regex_info().size() != 0)
      {
        regex_info_t const &regex_info = curr_node->regex_info().back();
        
        assert(regex_info.m_type != REGEX_TYPE_OR);
        assert(1 == regex_info.m_ranges.size());
        
        switch (regex_info.m_type)
        {
        case REGEX_TYPE_ONE_OR_MORE:
        case REGEX_TYPE_ZERO_OR_MORE:
        case REGEX_TYPE_ZERO_OR_ONE:
          file << indent_line(1) << "m_regex" << regex_info.m_ranges.front().m_regex_group_id
               << ".clear();" << std::endl;
          break;
          
        case REGEX_TYPE_ONE:
          break;
          
        case REGEX_TYPE_OR:
        default:
          assert(0);
          break;
        }
        
        // skip to the end node of this regex.
        curr_node = regex_info.m_ranges.front().mp_end_node;
      }
      else
      {
        // If this node isn't the starting node of
        // any regex, then I will emit its codes
        // normally and directly.
        dump_class_destructor_for_one_node(file, curr_node, 0, 1);
      }
      
      assert(1 == curr_node->next_nodes().size());
      curr_node = curr_node->next_nodes().front();
    }
  }
  
  file << indent_line(0) << "}" << std::endl
       << std::endl;
  
  // ===========================================
  //         Dump 'fill_nodes'
  // ===========================================
  dump_gen_parser_src__fill_nodes__for_regex_alternative(file);
  
  file << std::endl;
  
  // ===========================================
  //         Dump 'parse_XXX'
  // ===========================================
  dump_gen_parser_src__parse_XXX__for_regex_alternative(file);
}

namespace
{  
  void
  emit_one_line_for_regex(
    std::wfstream &file,
    unsigned int const indent_depth,
    std::wstring const &rule_node_name,
    regex_info_t const &regex_info)
  {
    assert(1 == regex_info.m_ranges.size());
    assert(regex_info.m_ranges.front().m_regex_group_id != -1);
    
    file << indent_line(indent_depth);
    
    if (regex_info.m_type != REGEX_TYPE_ONE)
    {
      file << "std::vector<";
    }
    
    file << "pt_" << rule_node_name
         << "_prod0_regex" << regex_info.m_ranges.front().m_regex_group_id
         << "_t";
    
    if (regex_info.m_type != REGEX_TYPE_ONE)
    {
      file << ">";
    }
    
    file << " m_regex"
         << regex_info.m_ranges.front().m_regex_group_id << ";"
         << std::endl;
  }
    
  /// 
  ///
  /// \return The next number which regex_group_id can use.
  ///
  int
  dump_struct_for_each_regex_group(
    std::wfstream &file,
    std::wstring const &rule_node_name,
    int regex_group_id,
    node_t * const start_node,
    node_t * const end_node,
    bool const emit_codes_for_this_regex,
    int * const place_to_save_regex_group_id_for_this_regex,
    bool const inherit_from_pt_regex_t)
  {
    assert(1 == end_node->next_nodes().size());
    node_t * const real_end_node = end_node->next_nodes().front();
    
    std::list<regex_info_t>::reverse_iterator outer_regex_iter;
    
    node_t *curr_node = start_node;
    
    if (true == emit_codes_for_this_regex)
    {
      assert(place_to_save_regex_group_id_for_this_regex != 0);
      
      // I need 'outer_regex_iter' only when I want to emit
      // codes for this regex.
      if (curr_node->tmp_regex_info().size() != 0)
      {
        outer_regex_iter = curr_node->tmp_regex_info().rend();
        
        --outer_regex_iter;
      }
      else
      {
        // If I don't handle the regex of this node before,
        // Ex: The 2nd, 3rd, 4th... part of a
        // regex_OR_group, then I will set
        // 'outer_regex_iter' to the 'rbegin' of the
        // 'tmp_regex_info'.
        outer_regex_iter = curr_node->tmp_regex_info().rbegin();
      }
    }
    
    while (curr_node != real_end_node)
    {
      while (curr_node->regex_info().size() != 0)
      {
        regex_info_t regex_info = curr_node->regex_info().back();
        
        assert(regex_info.m_type != REGEX_TYPE_OR);
        assert(1 == regex_info.m_ranges.size());
          
        if (regex_info.m_ranges.front().mp_start_node == curr_node)
        {
          // I find another regex is started from this
          // node, then I will recursively call this
          // function to handle this new found regex.
          curr_node->move_one_regex_info_to_tmp();
          
          if (0 == regex_info.mp_regex_OR_info)
          {
            // Normal case, without regex_OR_info
            
            regex_group_id =
              dump_struct_for_each_regex_group(
                file,
                rule_node_name,
                regex_group_id,
                regex_info.m_ranges.front().mp_start_node,
                regex_info.m_ranges.front().mp_end_node,
                true,
                &(curr_node->tmp_regex_info().front().m_ranges.front().m_regex_group_id),
                false);
          }
          else
          {
            // Special case, with regex_OR_info
            
            BOOST_FOREACH(regex_range_t &regex_range,
                          regex_info.mp_regex_OR_info->m_ranges)
            {
              regex_group_id =
                dump_struct_for_each_regex_group(
                  file,
                  rule_node_name,
                  regex_group_id,
                  regex_range.mp_start_node,
                  regex_range.mp_end_node,
                  true,
                  &(regex_range.m_regex_group_id),
                  true);
            }
            
            // emit struct for this regex contained
            // regex_OR_info
            {
              file << "struct pt_" << rule_node_name << "_prod0_regex"
                   << regex_group_id << "_t" << std::endl
                   << "{" << std::endl
                   << indent_line(1) << "wds_uint32 regex_OR_type_id;" << std::endl
                   << std::endl
                   << indent_line(1) << "pt_regex_t *mp_regex_OR_node;" << std::endl
                   << "};" << std::endl
                   << "typedef struct pt_" << rule_node_name
                   << "_prod0_regex" << regex_group_id << "_t pt_"
                   << rule_node_name << "_prod0_regex" << regex_group_id
                   << "_t;"  << std::endl
                   << std::endl;
              
              assert(1 == curr_node->tmp_regex_info().front().m_ranges.size());
              
              curr_node->tmp_regex_info().front().m_ranges.front().m_regex_group_id = regex_group_id;
              
              ++regex_group_id;
            }
          }
          
          curr_node = regex_info.m_ranges.front().mp_end_node;
          
          break;
        }
        else
        {
          curr_node->move_one_regex_info_to_tmp();
        }
      }
      
      assert(1 == curr_node->next_nodes().size());
      curr_node = curr_node->next_nodes().front();
    }
    
    if (true == emit_codes_for_this_regex)
    {
      curr_node = start_node;
      
      // Emit class header for this regex class.
      file << "struct pt_" << rule_node_name << "_prod0_regex"
           << regex_group_id << "_t";
      
      if (true == inherit_from_pt_regex_t)
      {
        file << " : public pt_regex_t";
      }
      
      file << std::endl
           << "{" << std::endl;
      
      // Find regex from the 'outer_regex_iter' at 'curr_node'
      std::list<regex_info_t>::reverse_iterator r_iter = outer_regex_iter;
      
      if (r_iter != curr_node->tmp_regex_info().rend())
      {
        ++r_iter;
      }
      
      bool find = false;
      
      for (;
           r_iter != curr_node->tmp_regex_info().rend();
           ++r_iter)
      {
        assert((*r_iter).m_type != REGEX_TYPE_OR);
        assert(1 == (*r_iter).m_ranges.size());
        
        if ((*r_iter).m_ranges.front().mp_start_node == curr_node)
        {
          // I find a regex starting from this node, and I
          // should have already emited codes for this regex,
          // thus the 'm_regex_group_id' for this regex should
          // not be '-1'.
          emit_one_line_for_regex(
            file,
            1,
            rule_node_name,
            *r_iter);
        
          assert(1 == (*r_iter).m_ranges.size());
        
          // skip to the end node of this regex.
          curr_node = (*r_iter).m_ranges.front().mp_end_node;
        
          find = true;
        
          break;
        }
      }
    
      if (false == find)
      {
        // If this node isn't the starting node of
        // any regex, then I will emit its codes
        // normally and directly.
        dump_class_member_variable_for_one_node(file, curr_node, 0, 1);
      }
      
      assert(1 == curr_node->next_nodes().size());
      curr_node = curr_node->next_nodes().front();
      
      find = false;
      
      while (curr_node != real_end_node)
      {
        for (r_iter = curr_node->tmp_regex_info().rbegin();
             r_iter != curr_node->tmp_regex_info().rend();
             ++r_iter)
        {
          assert((*r_iter).m_type != REGEX_TYPE_OR);
          assert(1 == (*r_iter).m_ranges.size());
        
          if ((*r_iter).m_ranges.front().mp_start_node == curr_node)
          {
            // I find a new regex starting from this node.
          
            emit_one_line_for_regex(
              file,
              1,
              rule_node_name,
              *r_iter);
          
            assert(1 == (*r_iter).m_ranges.size());
          
            // skip to the end node of this regex.
            curr_node = (*r_iter).m_ranges.front().mp_end_node;
          
            find = true;
          
            break;
          }
        }
      
        if (false == find)
        {
          // If this node isn't the starting node of
          // any regex, then I will emit its codes
          // normally and directly.
          dump_class_member_variable_for_one_node(file, curr_node, 0, 1);
        }
        
        assert(1 == curr_node->next_nodes().size());
        curr_node = curr_node->next_nodes().front();
      }
      
      // Emit class footer for this regex class.
      file << "};" << std::endl
           << "typedef struct pt_" << rule_node_name
           << "_prod0_regex" << regex_group_id << "_t pt_"
           << rule_node_name << "_prod0_regex" << regex_group_id
           << "_t;"  << std::endl
           << std::endl;
      
      (*place_to_save_regex_group_id_for_this_regex) = regex_group_id;
      
      ++regex_group_id;
    }
    
    return regex_group_id;
  }    
}

namespace
{
  bool
  restore_each_node_regex_info(
    node_t * const node)
  {
    node->restore_regex_info();
    
    return true;
  }
}

/// Ex:
///
/// A B ((C D | E)* F G)* H
///
/// The class for this regex alternative is as following:
///
/// \code
/// // For (C D | E)
/// class T_internal_regex_1
/// {
///   pt_node_t *mp_node_1;
///   pt_node_t *mp_node_2;
/// };
///
/// // For ((C D | E)* F G)
/// class T_internal_regex_2
/// {
///   std::vector<T_internal_regex_1> m_regex_1;
///
///   pt_F_node_t *mp_F_node;
///   pt_G_node_t *mp_G_node;
/// };
///
/// class T
/// {
///   pt_A_node_t *mp_A_node;
///   pt_B_node_t *mp_B_node;
///
///   std::vector<T_internal_regex_2> m_regex_1;
/// };
/// \endcode
///
/// \param file 
/// \param alternative_start 
/// \param rule_node_name 
///
void
node_t::dump_gen_parser_header_for_regex_alternative(
  std::wfstream &file,
  node_t * const alternative_start,
  std::wstring const &rule_node_name)
{
  node_t * const alternative_end = find_alternative_end(alternative_start);
  
  dump_struct_for_each_regex_group(
    file,
    rule_node_name,
    0,
    alternative_start,
    alternative_end,
    false,
    0,
    false);
  
  // Dump class header for this alternative.
  dump_pt_XXX_prodn_node_t_class_header(file, rule_node_name, 0);
  
  (void)node_for_each(alternative_start,
                      0,
                      restore_each_node_regex_info);
  
  // Loop each node of this alternative.
  node_t *curr_node = alternative_start;
  while (curr_node->name().size() != 0)
  {
    // If this node belongs to at least one regex group.
    if (curr_node->regex_info().size() != 0)
    {
      assert(curr_node->regex_info().back().m_type != REGEX_TYPE_OR);
      
      emit_one_line_for_regex(file, 1, rule_node_name, curr_node->regex_info().back());
      
      assert(1 == curr_node->regex_info().back().m_ranges.size());
      
      // skip to the end node of this regex.
      curr_node = curr_node->regex_info().back().m_ranges.front().mp_end_node;
    }
    else
    {
      // If this node isn't the starting node of
      // any regex, then I will emit its codes
      // normally and directly.
      dump_class_member_variable_for_one_node(file, curr_node, 0, 1);
    }
    
    assert(1 == curr_node->next_nodes().size());
    curr_node = curr_node->next_nodes().front();
  }
  
  // Dump class footer for this alternative.
  dump_pt_XXX_prodn_node_t_class_footer(file, rule_node_name, 0);
}
