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

#include "wcl_memory_debugger\memory_debugger.h"

void
node_t::dump_gen_parser_src_for_not_regex_alternative(
  std::wfstream &file) const
{
  assert(true == m_is_rule_head);
  
  int i = 0;
    
  BOOST_FOREACH(node_t * const node, m_next_nodes)
  {
    // ===========================================
    //         Dump 'default constructor'
    // ===========================================
    file << indent_line(0) << "pt_" << m_name << "_prod" << i << "_node_t::"
         << "pt_" << m_name << "_prod" << i << "_node_t()" << std::endl;
    
    // Check if this alternative is an empty one, if yes, then
    // I don't need to dump any member variables.
    if (node->name().size() != 0)
    {
      bool first_time = true;
      
      file << indent_line(0) << " : ";
      
      node_t const *curr_node = node;
      while (curr_node->name().size() != 0)
      {
        if (false == first_time)
        {
          file << "," << std::endl
               << indent_line(0) << "   ";
        }
        
        dump_class_default_constructor_for_one_node(file, curr_node, 0, 0);
        
        assert(1 == curr_node->next_nodes().size());
        curr_node = curr_node->next_nodes().front();
        
        first_time = false;
      }
      
      file << std::endl;
    }
    
    file << indent_line(0) << "{" << std::endl
         << indent_line(0) << "}" << std::endl
         << std::endl;
    
    // ===========================================
    //         Dump destructor
    // ===========================================
    file << "pt_" << m_name << "_prod" << i << "_node_t::"
         << "~pt_" << m_name << "_prod" << i << "_node_t()" << std::endl
         << "{" << std::endl;
    
    // Check if this alternative is an empty one, if yes, then
    // I don't need to dump any member variables.
    if (node->name().size() != 0)
    {
      node_t const *curr_node = node;
      while (curr_node->name().size() != 0)
      {
        dump_class_destructor_for_one_node(file, curr_node, 0, 1);
        
        assert(1 == curr_node->next_nodes().size());
        curr_node = curr_node->next_nodes().front();
      }
    }
    
    file << indent_line(0) << "}" << std::endl
         << std::endl;

    // ===========================================
    //       Dump 'fill_nodes' function.
    // ===========================================
    
    file << indent_line(0) << "void" << std::endl
         << indent_line(0) << "pt_" << m_name << "_prod" << i << "_node_t::fill_nodes(std::list<pt_node_t *> const &nodes)" << std::endl
         << indent_line(0) << "{" << std::endl;
    
    // Check if this is an empty alternative.
    if (node->name().size() != 0)
    {
      file << indent_line(1) << "std::list<pt_node_t *>::const_iterator iter = nodes.begin();" << std::endl
           << std::endl;
    }
    
    node_t const *curr_node = node;
    while (curr_node->name().size() != 0)
    {
      unsigned int const indent_depth = 1;
      
      file << indent_line(indent_depth) << "assert((*iter) != 0);" << std::endl
           << indent_line(1) << "mp_" << curr_node->name() << "_node_"
           << curr_node->name_postfix_by_appear_times();
      
      file << " = dynamic_cast<pt_"
           << curr_node->name() << "_node_t *>(*iter);" << std::endl
           << indent_line(indent_depth) << "++iter;" << std::endl
           << indent_line(indent_depth);
      
      assert(1 == curr_node->next_nodes().size());
      curr_node = curr_node->next_nodes().front();
      
      if (curr_node->name().size() != 0)
      {
        file << "assert(iter != nodes.end());" << std::endl
             << std::endl;
      }
      else
      {
        file << "assert(iter == nodes.end());" << std::endl;
      }
    }
    
    file << "}" << std::endl
         << std::endl;
    
    ++i;
  }
  
  file << "pt_node_t *" << std::endl
       << "frontend::parse_" << m_name << "(bool const consume)" << std::endl
       << "{" << std::endl
       << "  std::list<pt_node_t *> nodes;" << std::endl
       << "  pt_" << m_name << "_prod_node_t *prod_node = 0;" << std::endl
       << std::endl;
  
  if (true == m_contains_ambigious)
  {
    file << indent_line(1) << "// ambiguity" << std::endl;
  }
  else
  {
    boost::shared_ptr<node_t> default_node(
      new node_t(mp_ae, mp_rule_node, std::wstring()));
    assert(default_node.get() != 0);
    
    std::map<node_t *, int> alternative_start_map;
    int i = 0;
    
    BOOST_FOREACH(node_t *alternative_start, m_next_nodes)
    {
      alternative_start_map[alternative_start] = i;
      
      ++i;
    }
    
    std::list<node_with_order_t> nodes;
    
    BOOST_FOREACH(node_t *node, m_next_nodes)
    {
      nodes.push_back(node_with_order_t(alternative_start_map[node], node));
    }
    
    dump_gen_parser_src_real(file,
                             0,
                             this,
                             default_node.get(),
                             nodes,
                             1,
                             1);
  }
    
  file << std::endl
       << indent_line(1) << "if (true == consume)" << std::endl
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
node_t::dump_gen_parser_header_for_not_regex_alternative(
  std::wfstream &file,
  node_t * const alternative_start,
  std::wstring const &rule_node_name,
  int const alternative_id) const
{
  dump_pt_XXX_prodn_node_t_class_header(file, rule_node_name, alternative_id);
  
  // Check if this alternative is an empty one, if yes, then
  // I don't need to dump any member variables.
  if (alternative_start->name().size() != 0)
  {
    node_t const *curr_node = alternative_start;
    while (curr_node->name().size() != 0)
    {
      dump_class_member_variable_for_one_node(file, curr_node, 0, 1);
      
      assert(1 == curr_node->next_nodes().size());
      curr_node = curr_node->next_nodes().front();
    }
  }
  
  dump_pt_XXX_prodn_node_t_class_footer(file, rule_node_name, alternative_id);
}
