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
#include "ga_exception.hpp"
#include "gen.hpp"
#include "lookahead.hpp"
#include "arranged_lookahead.hpp"

#include "wcl_memory_debugger\memory_debugger.h"

#define INDENT_OFFSET 2

typedef node_t *node_t_ptr;

namespace
{    
  void
  go_further_one_node(std::list<node_with_order_t> &nodes)
  {
    for (std::list<node_with_order_t>::iterator iter = nodes.begin();
         iter != nodes.end();
         )
    {
      if (0 == (*iter).mp_node->next_nodes().size())
      {
        /* This is a rule end node, I can not trace
         * further from this node, thus kick it out
         * from the tracing list.
         */
        assert(0 == (*iter).mp_node->name().size());
        iter = nodes.erase(iter);
      }
      else
      {
        /* I have finish this node in one
         * alternative, thus walk to the next node.
         */
        assert(1 == (*iter).mp_node->next_nodes().size());
        
        (*iter).mp_node = (*iter).mp_node->next_nodes().front();
        
        ++iter;
      }
    }
  }
  
  void
  go_further_one_node(node_t_ptr &node)
  {
    assert(1 == node->next_nodes().size());
    node = node->next_nodes().front();
  }
      
  nodes_have_lookahead_rel_t
  all_nodes_have_lookahead(std::list<node_with_order_t> const &nodes)
  {
    std::list<node_with_order_t>::size_type have_lk_node_count = 0;
    std::list<node_with_order_t>::size_type havent_lk_node_count = 0;
    
    BOOST_FOREACH(node_with_order_t const &node, nodes)
    {
      if (node.mp_node->lookahead_depth() != 0)
      {
        ++have_lk_node_count;
      }
      else
      {
        ++havent_lk_node_count;
      }
    }
    
    assert(nodes.size() == (have_lk_node_count + havent_lk_node_count));
    
    if (0 == have_lk_node_count)
    {
      return NO_NODES_HAVE_LOOKAHEAD;
    }
    else if (0 == havent_lk_node_count)
    {
      return ALL_NODES_HAVE_LOOKAHEAD;
    }
    else
    {
      return PART_NODES_HAVE_LOOKAHEAD;
    }
  }
  
  void
  check_ambiguity(std::list<node_with_order_t> const &nodes)
  {
    BOOST_FOREACH(node_with_order_t const &node, nodes)
    {    
      if (true == node.mp_node->is_ambigious())
      {
        throw ga_exception_meet_ambiguity_t();
      }
    }
  }
    
  // \brief Dump leaf node group in the arranged lookahead
  // tree
  //
  // All nodes in this node group must have the same name
  // and equal lookahead depth and symbols.
  //
  // \param file
  // \param tmp
  // \param indent_depth
  void
  dump_leaf_node_group_in_lookahead_tree(
    std::wfstream &file,
    std::list<node_with_order_t> &nodes,
    unsigned int const indent_depth)
  {
    assert(nodes.size() != 0);
    
#if defined(_DEBUG)
    for (std::list<node_with_order_t>::const_iterator iter = nodes.begin();
         iter != nodes.end();
         ++iter)
    {
      assert(0 == (*iter).mp_node->name().compare(nodes.front().mp_node->name()));
      assert((*iter).mp_node->lookahead_depth() ==
             nodes.front().mp_node->lookahead_depth());
    }
#endif
    
    dump_add_node_to_nodes_for_one_node_in_parse_XXX(file, nodes.front().mp_node, indent_depth);
  }
      
  void
  dump_case_stmt_for_node(
    std::wfstream &file,
    arranged_lookahead_t const &child_arranged_lookahead,
    node_t const * const default_node,
    unsigned int const indent_depth)
  {
    BOOST_FOREACH(node_t const *node, child_arranged_lookahead.lookahead_nodes())
    {
      // 'default' node should not be merge with any
      // other nodes.
      assert(node != default_node);
      
      file << indent_line(indent_depth) << "case WDS_TOKEN_TYPE_";
      
      std::wstring name = node->name();
      
      if (0 == name.size())
      {
        // EOF
        file << "EOF:" << std::endl;
        
        // if one of the last lookahead symbol is EOF, then
        // 'target_nodes()' should be 1,
        // otherwise, this will result a duplicate
        // alternative error.
        assert(1 == child_arranged_lookahead.lookahead_nodes().size());
        assert(0 == child_arranged_lookahead.lookahead_nodes().
               front()->name().size());
        
        // If there is an EOF node here, then the nodes
        // which can be merged with EOF node can not have
        // any children nodes, because an EOF node should
        // not have any children nodes, too.
        assert(0 == child_arranged_lookahead.children().size());
      }
      else
      {
        // transform it to all capital letters.
        std::transform(name.begin(), name.end(), name.begin(), towupper);
        
        file << name << ":" << std::endl;
      }
    }
  }
    
  void
  traverse_arranged_lookahead(
    std::wfstream &file,
    std::list<node_with_order_t> &nodes,
    node_t const * const rule_node,
    arranged_lookahead_t const * const arranged_lookahead_top,
    arranged_lookahead_t const * const arranged_lookahead,
    unsigned int const indent_depth,
    unsigned int const lookahead_depth,
    node_t const * const default_node)
  {
    // If I meet an ambiguious situation, I have to return.
    check_ambiguity(nodes);
    
    // emit token fetching string & the 'switch' statement head.
    file << indent_line(indent_depth) << "token_t * const token_" << lookahead_depth
         << " = lexer_peek_token(" << lookahead_depth << ");" << std::endl
         << indent_line(indent_depth) << "switch (token_" << lookahead_depth << "->get_type())" << std::endl
         << indent_line(indent_depth) << "{" << std::endl;
    
    // emit 'case' statement string for each lookahead token.
    BOOST_FOREACH(arranged_lookahead_t const &child_arranged_lookahead,
                  arranged_lookahead->children())
    {
      // Check if this is a 'default' node, if yes, then
      // emit the 'default:' string.
      if (child_arranged_lookahead.lookahead_nodes().front() == default_node)
      {
        // the number of the 'default' node should be 1.
        assert(1 == child_arranged_lookahead.lookahead_nodes().size());
        
        file << indent_line(indent_depth) << "default:" << std::endl
             << indent_line(indent_depth + 1) << "throw std::exception();" << std::endl;
      }
      else
      {
        // the number of the other nodes could be greater
        // than 1.
        //
        // emit 'case xxx:' string for each lookahead token.
        dump_case_stmt_for_node(file, child_arranged_lookahead,
                                default_node, indent_depth);
        
        file << indent_line(indent_depth + 1) << "{" << std::endl;
        
        // Check if I am the following situation.
        // Ex:
        // lookahead tree:
        //
        // A - B
        //   \ C
        //   \ D
        //
        // This should be emitted as
        //
        // switch ()
        // {
        // case A:
        //   switch ()
        //   {
        //   case B:
        //   case C:
        //   case D:
        //
        // And I am in 'A' now.
        //
        if (child_arranged_lookahead.children().size() > 0)
        {
          assert(0 == child_arranged_lookahead.target_nodes().size());
          
          // emit another fetching token string & another
          // switch statement for the child lookahead.
          traverse_arranged_lookahead(file,
                                      nodes,
                                      rule_node,
                                      arranged_lookahead_top,
                                      &(child_arranged_lookahead),
                                      indent_depth + 2,
                                      lookahead_depth + 1,
                                      default_node);
        }
        else
        {
          // This is a leaf node of the arranged
          // lookahead tree.
          std::list<node_with_order_t> same_lookahead_nodes =
            child_arranged_lookahead.target_nodes();
          
          assert(same_lookahead_nodes.size() > 0);
          
          if ((1 == same_lookahead_nodes.size()) &&
              (0 == same_lookahead_nodes.front().mp_node->name().size()))
          {
            file << indent_line(indent_depth + 2) << "if (true == consume)" << std::endl
                 << indent_line(indent_depth + 2) << "{" << std::endl
                 << indent_line(indent_depth + 3) << "assert(0 == prod_node);" << std::endl
                 << indent_line(indent_depth + 3) << "prod_node = new pt_" << rule_node->name() << "_prod" << same_lookahead_nodes.front().m_idx << "_node_t;" << std::endl
                 << indent_line(indent_depth + 3) << "assert(prod_node != 0);" << std::endl
                 << indent_line(indent_depth + 2) << "}" << std::endl;
          }
          else
          {
            // If there are more than 1 node in the
            // 'target_nodes()', then these
            // nodes' must have the same name, otherwise this
            // means that I meet an ambigious situation.
            BOOST_FOREACH(node_with_order_t const &node, same_lookahead_nodes)
            {
              if (node.mp_node->name().compare(
                    same_lookahead_nodes.front().mp_node->name()
                                               ) != 0)
              {
                // ambigious
                throw ga_exception_meet_ambiguity_t();
              }
            }
                        
            // dump string now.
            dump_leaf_node_group_in_lookahead_tree(
              file,
              same_lookahead_nodes,
              indent_depth + 2);
            
            // go further one step.
            go_further_one_node(same_lookahead_nodes);
          
            // dump generated string according to equal nodes.
            //
            // Ex:
            // A B C D
            // A B E F
            // A B G H
            //
            // Then I have to dump strings for 'A B' now.
            while (1)
            {
              bool finish = false;
            
              switch (all_nodes_have_lookahead(same_lookahead_nodes))
              {
              case NO_NODES_HAVE_LOOKAHEAD:
                // no lookahead means I can dump strings
                // immediately, and do not need to arrange and go
                // through the lookahead tree.
                if ((1 == same_lookahead_nodes.size()) &&
                    (0 == same_lookahead_nodes.front().mp_node->name().size()))
                {
                  // If we are checking only one node, and this is a
                  // rule end node, then this means we have finish
                  // this alternative, thus just return.
                
                  // If this is a one alternative rule, then finish
                  // this function.
                
                  // If this alternative finishes...
                  file << std::endl
                       << indent_line(indent_depth + 2) << "if (true == consume)" << std::endl
                       << indent_line(indent_depth + 2) << "{" << std::endl
                       << indent_line(indent_depth + 3) << "assert(0 == prod_node);" << std::endl
                       << indent_line(indent_depth + 3) << "prod_node = new pt_" << rule_node->name() << "_prod" << same_lookahead_nodes.front().m_idx << "_node_t;" << std::endl
                       << indent_line(indent_depth + 3) << "assert(prod_node != 0);" << std::endl
                       << indent_line(indent_depth + 2) << "}" << std::endl;
                
                  finish = true;
                  break;
                }
                else
                {
                  // dump string now.
                  dump_leaf_node_group_in_lookahead_tree(
                    file,
                    same_lookahead_nodes,
                    indent_depth + 2);
                }
                break;
                
              case ALL_NODES_HAVE_LOOKAHEAD:
                if (1 == same_lookahead_nodes.size())
                {
                  if (0 == same_lookahead_nodes.front().mp_node->name().size())
                  {
                    file << std::endl
                         << indent_line(indent_depth + 2) << "if (true == consume)" << std::endl
                         << indent_line(indent_depth + 2) << "{" << std::endl
                         << indent_line(indent_depth + 3) << "assert(0 == prod_node);" << std::endl
                         << indent_line(indent_depth + 3) << "prod_node = new pt_" << rule_node->name() << "_prod" << same_lookahead_nodes.front().m_idx << "_node_t;" << std::endl
                         << indent_line(indent_depth + 3) << "assert(prod_node != 0);" << std::endl
                         << indent_line(indent_depth + 2) << "}" << std::endl;
                    
                    finish = true;
                    break;
                  }
                  else
                  {
                    dump_leaf_node_group_in_lookahead_tree(
                      file,
                      same_lookahead_nodes,
                      indent_depth + 2);
                  }
                }
                else
                {
                  boost::shared_ptr<arranged_lookahead_t> new_arranged_lookahead(
                    collect_lookahead_into_arrange_lookahead(same_lookahead_nodes,
                                                             default_node));
                  
                  merge_arranged_lookahead_tree_leaf_node_if_they_have_same_target_node(
                    new_arranged_lookahead.get(),
                    default_node);
                  
                  traverse_arranged_lookahead(file,
                                              same_lookahead_nodes,
                                              rule_node,
                                              new_arranged_lookahead.get(),
                                              new_arranged_lookahead.get(),
                                              indent_depth + 2,
                                              lookahead_depth + 1,
                                              default_node);
                }
                
                finish = true;
                break;
              
              case PART_NODES_HAVE_LOOKAHEAD:
                // Should not reach here.
              default:
                assert(0);
                break;
              }
              
              if (true == finish)
              {
                break;
              }
              else
              {
                // go further one step.
                go_further_one_node(same_lookahead_nodes);
              }
            }
          }
        }
        
        file << indent_line(indent_depth + 1) << "}" << std::endl
             << indent_line(indent_depth + 1) << "break;" << std::endl << std::endl;
      }
    }
    
    file << indent_line(indent_depth) << "}" << std::endl;
  }
}

void
dump_gen_parser_src_real(
  std::wfstream &file,
  arranged_lookahead_t const * const arranged_lookahead_top,
  node_t const * const rule_node,
  node_t const * const default_node,
  std::list<node_with_order_t> &nodes,
  unsigned int const indent_depth,
  unsigned int const lookahead_depth)
{
  // If I meet an ambiguious situation, I have to return.
  check_ambiguity(nodes);
    
  // dump generated string according to equal nodes.
  //
  // Ex:
  // A B C D
  // A B E F
  // A B G H
  //
  // Then I have to dump strings for 'A B' now.
  bool dump_at_least_one = false;
  while (1)
  {
    bool finish = false;
      
    switch (all_nodes_have_lookahead(nodes))
    {
    case NO_NODES_HAVE_LOOKAHEAD:
      // no lookahead means I can dump strings
      // immediately, and do not need to arrange and go
      // through the lookahead tree.
      if ((1 == nodes.size()) &&
          (0 == nodes.front().mp_node->name().size()))
      {
        // If we are checking only one node, and this is a
        // rule end node, then this means we have finish
        // this alternative, thus just return.
        
        // If this is a one alternative rule, then finish
        // this function.
        
        // If this alternative finishes...
        file << std::endl
             << indent_line(indent_depth) << "if (true == consume)" << std::endl
             << indent_line(indent_depth) << "{" << std::endl
             << indent_line(indent_depth + 1) << "assert(0 == prod_node);" << std::endl
             << indent_line(indent_depth + 1) << "prod_node = new pt_" << rule_node->name() << "_prod" << nodes.front().m_idx << "_node_t;" << std::endl
             << indent_line(indent_depth + 1) << "assert(prod_node != 0);" << std::endl
             << indent_line(indent_depth) << "}" << std::endl;
        
        return;
      }
      else
      {
        // dump string now.
        dump_leaf_node_group_in_lookahead_tree(file, nodes, indent_depth);
        
        dump_at_least_one = true;
      }
      break;
        
    case ALL_NODES_HAVE_LOOKAHEAD:
      // I have to break this while loop and to arrange
      // and go through a lookahead tree.
      finish = true;
      break;
        
    case PART_NODES_HAVE_LOOKAHEAD:
      // Should not reach here.
    default:
      assert(0);
      break;
    }
      
    if (true == finish)
    {
      break;
    }
    else
    {
      // go further one step.
      go_further_one_node(nodes);
    }
  }
    
  if (true == dump_at_least_one)
  {
    file << std::endl;
  }
    
  // arrange lookahead tokens and go through the arranged
  // lookahead tree.
  assert(nodes.front().mp_node->lookahead_depth() != 0);
  
  boost::shared_ptr<arranged_lookahead_t> arranged_lookahead(
    collect_lookahead_into_arrange_lookahead(nodes, default_node));
  
  merge_arranged_lookahead_tree_leaf_node_if_they_have_same_target_node(
    arranged_lookahead.get(),
    default_node);
  
  traverse_arranged_lookahead(file,
                              nodes,
                              rule_node,
                              arranged_lookahead.get(),
                              arranged_lookahead.get(),
                              indent_depth,
                              1,
                              default_node);
}

std::wstring
indent_line(unsigned int const indent_level)
{
  std::wstring str;
  
  for (unsigned int i = 0; i < indent_level; ++i)
  {
    for (unsigned int j = 0; j < INDENT_OFFSET; ++j)
    {
      str.append(L" ");
    }
  }
    
  return str;
}

namespace
{
  void
  dump_name_for_one_node(
    std::wfstream &file,
    node_t const * const node,
    unsigned int const idx,
    unsigned int const indent_level)
  {
    file << "mp_";
    
    if (node != 0)
    {
      file << node->name() << "_";
    }
  
    file << "node";
  
    if (node != 0)
    {
      file << "_" << node->name_postfix_by_appear_times();
    }
    else
    {
      file << "_" << idx;
    }
  }
}

void
dump_class_member_variable_for_one_node(
  std::wfstream &file,
  node_t const * const node,
  unsigned int const idx,
  unsigned int const indent_level)
{
  file << indent_line(indent_level) << "pt_";
  
  if (node != 0)
  {
    file << node->name() << "_";
  }
  
  file << "node_t *";
  
  dump_name_for_one_node(file, node, idx, indent_level);
  
  file << ";" << std::endl;
}

void
dump_class_default_constructor_for_one_node(
  std::wfstream &file,
  node_t const * const node,
  unsigned int const idx,
  unsigned int const indent_level)
{
  file << indent_line(indent_level);

  dump_name_for_one_node(file, node, idx, indent_level);
  
  file << "(0)";
}

void
dump_class_destructor_for_one_node(
  std::wfstream &file,
  node_t const * const node,
  unsigned int const idx,
  unsigned int const indent_level)
{
  file << indent_line(indent_level)
       << "if (";
  
  dump_name_for_one_node(file, node, idx, indent_level);
  
  file << " != 0)" << std::endl
       << indent_line(indent_level) << "{" << std::endl
       << indent_line(indent_level + 1) << "delete ";
  
  dump_name_for_one_node(file, node, idx, indent_level);
  
  file << ";" << std::endl
       << indent_line(indent_level) << "}" << std::endl;
}

void
analyser_environment_t::dump_gen_parser_nodes_hpp(
  std::wfstream &file)
{
  file << "#ifndef __parser_nodes_hpp__" << std::endl
       << "#define __parser_nodes_hpp__" << std::endl << std::endl;
  
  // Dump included header files
  file << "#include <list>" << std::endl
       << "#include <vector>" << std::endl
       << "#include \"parser_basic_types.hpp\"" << std::endl
       << std::endl;
  
  // Dump base class 'pt_node_t'
  file << "class pt_node_t" << std::endl
       << "{" << std::endl
       << "public:" << std::endl
       << std::endl
       << "  virtual ~pt_node_t() {}" << std::endl
       << "};" << std::endl 
       << "typedef class pt_node_t pt_node_t;" << std::endl
       << std::endl;
  
  if (false == m_using_pure_BNF)
  {
    // Dump base class 'pt_regex_t' for each regex_info.
    file << "struct pt_regex_t" << std::endl
         << "{" << std::endl
         << "  virtual ~pt_regex_t() {}" << std::endl
         << "};" << std::endl 
         << "typedef struct pt_regex_t pt_regex_t;" << std::endl
         << std::endl;
    
    // Dump class pt_regex_chooser_t for each
    // regex_chooser.
    file << "struct pt_regex_chooser_t : public pt_node_t" << std::endl
         << "{" << std::endl
         << "public:" << std::endl
         << std::endl
         << "  bool bypass;" << std::endl
         << "};" << std::endl 
         << "typedef struct pt_regex_chooser_t pt_regex_chooser_t;" << std::endl
         << std::endl;
    
    // Dump class pt_regex_OR_chooser_t for each
    // regex_OR_chooser.
    file << "struct pt_regex_OR_chooser_t : public pt_node_t" << std::endl
         << "{" << std::endl
         << "public:" << std::endl
         << std::endl
         << "  wds_uint32 regex_typename_id;" << std::endl
         << "};" << std::endl 
         << "typedef struct pt_regex_OR_chooser_t pt_regex_OR_chooser_t;" << std::endl
         << std::endl;
  }
  
  // Dump terminal node class
  typedef terminal_hash_table_t::index<terminal_name>::type node_by_name;
  
  node_by_name::iterator iter = m_terminal_hash_table.get<terminal_name>().begin();
  while (iter != m_terminal_hash_table.get<terminal_name>().end())
  {
    file << "class pt_" << (*iter) << "_node_t : public pt_node_t" << std::endl
         << "{" << std::endl
         << "};" << std::endl 
         << "typedef class pt_" << (*iter) << "_node_t pt_" << (*iter) << "_node_t;"
         << std::endl << std::endl;
    
    ++iter;
  }
  
  // Dump nonterminal node class forward declaration
  BOOST_FOREACH(node_t const *node, m_top_level_nodes)
  {
    file << "class pt_" << node->name() << "_node_t;" << std::endl;
  }
  
  file << std::endl;
  
  // Dump nonterminal node class
  BOOST_FOREACH(node_t *node, m_top_level_nodes)
  {
    node->dump_gen_parser_header(file);
  }
  
  // Dump 'parse_XXX' function prototype.
  file << "////////////////////// Cut from here //////////////////////" << std::endl;
  
  BOOST_FOREACH(node_t const *node, m_top_level_nodes)
  {
    file << "pt_node_t *parse_" << node->name() << "(bool const consume);" << std::endl;
  }
  
  file << "////////////////////// Cut end here //////////////////////"
       << std::endl
       << std::endl;
  
  file << "#endif" << std::endl;
}

void
analyser_environment_t::dump_gen_parser_cpp() const
{
  BOOST_FOREACH(node_t const * const node, m_top_level_nodes)
  {
    std::wstring filename(L"parser_node_");
    filename.append(node->name());
    filename.append(L".cpp");
    
    std::auto_ptr<std::wfstream> const file(
      new std::wfstream(filename.c_str(),
                        std::ios_base::out | std::ios_base::binary));
    
    (*file) << "#include <cassert>" << std::endl
            << "#include <exception>" << std::endl
            << "#include \"parser_nodes.hpp\"" << std::endl
            << "#include \"parser_basic_types.hpp\"" << std::endl
            << "#include \"frontend.hpp\"" << std::endl
            << "#include \"token.hpp\"" << std::endl
            << std::endl;
    
    node->dump_gen_parser_src(*file);
    
    file->close();
  }
}

void
analyser_environment_t::dump_gen_main_cpp(
  std::wfstream &file) const
{
  file << "int" << std::endl
       << "main(int argc, char **argv)" << std::endl
       << "{" << std::endl
       << indent_line(1) << "return 0;" << std::endl
       << "}" << std::endl;
}

void
analyser_environment_t::dump_gen_frontend_hpp(
  std::wfstream &file) const
{
  file << "#include <cassert>" << std::endl
       << "#include <exception>" << std::endl
       << "#include <deque>" << std::endl
       << "#include \"parser_nodes.hpp\"" << std::endl
       << "#include \"parser_basic_types.hpp\"" << std::endl;
  
  file << std::endl;
  
  file << "class token_t;" << std::endl;
  
  file << std::endl;
  
  file << "class frontend" << std::endl
       << "{" << std::endl
       << "private:" << std::endl
       << std::endl
       << indent_line(1) << "typedef std::deque<token_t *> lookahead_queue_type;" << std::endl
       << std::endl;
  
  BOOST_FOREACH(node_t const * const node, m_top_level_nodes)
  {
    file << indent_line(1) << "pt_node_t *parse_" << node->name()
         << "(bool const consume);" << std::endl;
  }
  
  file << std::endl;
  
  file << indent_line(1) << "void add_node_to_nodes(" << std::endl
       << indent_line(2) << "std::list<pt_node_t *> &nodes," << std::endl
       << indent_line(2) << "pt_node_t * const node);" << std::endl;
  
  file << std::endl;
  
  file << indent_line(1) << "pt_node_t *ensure_next_token_is(" << std::endl
       << indent_line(2) << "wds_token_type const type," << std::endl
       << indent_line(2) << "bool const consume);" << std::endl;
  
  file << std::endl;
  
  file << indent_line(1) << "bool next_token_is(" << std::endl
       << indent_line(2) << "lookahead_queue_type::size_type const lookahead_count," << std::endl
       << indent_line(2) << "wds_token_type const type);" << std::endl;
  
  file << std::endl;
  
  file << indent_line(1) << "token_t *lexer_peek_token(" << std::endl
       << indent_line(2) << "lookahead_queue_type::size_type const lookahead_count);" << std::endl;
  
  file << std::endl;
  
  file << indent_line(1) << "pt_node_t *lexer_consume_token(" << std::endl
       << indent_line(2) << "bool const consume);" << std::endl;
  
  file << "};" << std::endl
       << "typedef class frontend frontend;" << std::endl;
}

void
analyser_environment_t::dump_gen_frontend_cpp(
  std::wfstream &file) const
{
  file << "#include <cassert>" << std::endl
       << "#include <list>" << std::endl
       << "#include <exception>" << std::endl
       << "#include \"parser_nodes.hpp\"" << std::endl
       << "#include \"frontend.hpp\"" << std::endl
       << "#include \"token.hpp\"" << std::endl;
  
  file << std::endl;
  
  file << "void" << std::endl
       << "frontend::add_node_to_nodes(" << std::endl
       << "  std::list<pt_node_t *> &nodes," << std::endl
       << "  pt_node_t * const node)" << std::endl
       << "{" << std::endl
       << "  assert(node != 0);" << std::endl
       << "  nodes.push_back(node);" << std::endl
       << "}" << std::endl;
  
  file << std::endl;
  
  file << "pt_node_t *" << std::endl
       << "frontend::ensure_next_token_is(" << std::endl
       << indent_line(1) << "wds_token_type const type," << std::endl
       << indent_line(1) << "bool const consume)" << std::endl
       << "{" << std::endl
       << indent_line(1) << "if (true == next_token_is(1, type))" << std::endl
       << indent_line(1) << "{" << std::endl
       << indent_line(2) << "return lexer_consume_token(consume);" << std::endl
       << indent_line(1) << "}" << std::endl
       << indent_line(1) << "else" << std::endl
       << indent_line(1) << "{" << std::endl
       << indent_line(2) << "throw std::exception();" << std::endl
       << indent_line(1) << "}" << std::endl
       << "}" << std::endl;
  
  file << std::endl;
  
  file << "bool" << std::endl
       << "frontend::next_token_is(" << std::endl
       << indent_line(1) << "lookahead_queue_type::size_type const lookahead_count," << std::endl
       << indent_line(1) << "wds_token_type const type)" << std::endl
       << "{" << std::endl
       << indent_line(1) << "token_t *token = lexer_peek_token(lookahead_count);" << std::endl
       << indent_line(1) << "if (type == token->get_type())" << std::endl
       << indent_line(1) << "{" << std::endl
       << indent_line(2) << "return true;" << std::endl
       << indent_line(1) << "}" << std::endl
       << indent_line(1) << "else" << std::endl
       << indent_line(1) << "{" << std::endl
       << indent_line(2) << "return false;" << std::endl
       << indent_line(1) << "}" << std::endl
       << "}" << std::endl;
  
  file << std::endl;
  
  file << "token_t *" << std::endl
       << "frontend::lexer_peek_token(" << std::endl
       << indent_line(1) << "lookahead_queue_type::size_type const lookahead_count)" << std::endl
       << "{" << std::endl
       << indent_line(1) << "return 0;" << std::endl
       << "}" << std::endl;
  
  file << std::endl;
  
  file << "pt_node_t *" << std::endl
       << "frontend::lexer_consume_token(" << std::endl
       << indent_line(1) << "bool const consume)" << std::endl
       << "{" << std::endl
       << indent_line(1) << "return 0;" << std::endl
       << "}" << std::endl;
}

void
analyser_environment_t::dump_gen_parser_basic_types_hpp(
  std::wfstream &file) const
{
  file << "#ifndef __parser_basic_types_hpp__" << std::endl
       << "#define __parser_basic_types_hpp__" << std::endl
       << std::endl;
  
  file << "enum wds_token_type" << std::endl
       << "{" << std::endl
       << "  WDS_TOKEN_TYPE_EOF," << std::endl;
  
  typedef terminal_hash_table_t::index<terminal_name>::type node_by_name;
  
  node_by_name::iterator iter = m_terminal_hash_table.get<terminal_name>().begin();
  while (iter != m_terminal_hash_table.get<terminal_name>().end())
  {
    if (iter != m_terminal_hash_table.get<terminal_name>().begin())
    {
      file << "," << std::endl;
    }
    
    std::wstring terminal_name = (*iter);
    
    std::transform(terminal_name.begin(), terminal_name.end(),
                   terminal_name.begin(),
                   towupper);
    
    file << indent_line(1) << "WDS_TOKEN_TYPE_" << terminal_name;
    
    ++iter;
  }
  
  file << std::endl << "};" << std::endl
       << "typedef enum wds_token_type wds_token_type;" << std::endl
       << std::endl;
  
  // Dump integral types.
  file << "typedef signed char  wds_int8;" << std::endl
       << "typedef signed short wds_int16;" << std::endl
       << "typedef signed int   wds_int32;" << std::endl
       << "typedef unsigned char  wds_uint8;" << std::endl
       << "typedef unsigned short wds_uint16;" << std::endl
       << "typedef unsigned int   wds_uint32;" << std::endl << std::endl;
  
  file << "#endif" << std::endl;
}

void
analyser_environment_t::dump_gen_token_hpp(
  std::wfstream &file) const
{
  file << "#ifndef __token_hpp__" << std::endl
       << "#define __token_hpp__" << std::endl
       << std::endl;
  
  file << "class token_t" << std::endl
       << "{" << std::endl
       << "private:" << std::endl
       << std::endl
       << indent_line(1) << "wds_token_type m_type;" << std::endl
       << std::endl
       << "public:" << std::endl
       << std::endl
       << indent_line(1) << "wds_token_type get_type() const" << std::endl
       << indent_line(1) << "{ return m_type; }" << std::endl
       << "};" << std::endl
       << "typedef class token_t token_t;" << std::endl
       << std::endl;
  
  file << "#endif" << std::endl;
}

void
dump_pt_XXX_prodn_node_t_class_header(
  std::wfstream &file,
  std::wstring const &rule_node_name,
  int const alternative_id)
{
  file << "class pt_" << rule_node_name << "_prod" << alternative_id
       << "_node_t : public pt_" << rule_node_name << "_prod_node_t" << std::endl
       << "{" << std::endl
       << "private:" << std::endl
       << std::endl;
}

void
dump_pt_XXX_prodn_node_t_class_footer(
  std::wfstream &file,
  std::wstring const &rule_node_name,
  int const alternative_id)
{
  file << std::endl
       << "public:" << std::endl
       << std::endl
       << "  // default constructor " << std::endl
       << "  pt_" << rule_node_name << "_prod" << alternative_id << "_node_t();" << std::endl
       << "  // destructor " << std::endl
       << "  ~pt_" << rule_node_name << "_prod" << alternative_id << "_node_t();" << std::endl
       << std::endl
       << "  virtual void fill_nodes(std::list<pt_node_t *> const &nodes);" << std::endl
       << "  virtual void check_semantic() const {};" << std::endl
       << "};" << std::endl
       << "typedef class pt_" << rule_node_name << "_prod" << alternative_id
       << "_node_t pt_" << rule_node_name << "_prod" << alternative_id << "_node_t;"
       << std::endl << std::endl;
}

void
node_t::dump_gen_parser_header(
  std::wfstream &file)
{
  assert(true == m_is_rule_head);
  
  // Dump ancestor class named 'pt_XXX_prod_node_t' for each
  // alternative of this rule node.
  file << "class pt_" << m_name << "_prod_node_t : public pt_node_t" << std::endl
       << "{" << std::endl
       << "public:" << std::endl
       << std::endl
       << "  virtual void fill_nodes(std::list<pt_node_t *> const &nodes) = 0;" << std::endl
       << "  virtual void check_semantic() const = 0;" << std::endl
       << "};" << std::endl
       << "typedef class pt_" << m_name << "_prod_node_t pt_" << m_name << "_prod_node_t;" << std::endl
       << std::endl;
  
  int alternative_id = 0;
  
  // Dump main class name 'pt_XXX_prod#_node_t' for each
  // alternative of this rule node.
  if (RULE_CONTAIN_NO_REGEX == m_rule_contain_regex)
  {
    for (std::list<node_t *>::const_iterator iter = m_next_nodes.begin();
         iter != m_next_nodes.end();
         ++iter)
    {
      dump_gen_parser_header_for_not_regex_alternative(
        file, *iter, m_name, alternative_id);
      
      ++alternative_id;
    }
  }
  else
  {
    assert(mp_main_regex_alternative != 0);
    
    dump_gen_parser_header_for_regex_alternative(
      file, mp_main_regex_alternative, m_name);
  }
  
  // Dump main class named 'pt_XXX_node_t' for this rule node.
  file << "class pt_" << m_name << "_node_t : public pt_node_t" << std::endl
       << "{" << std::endl
       << "private:" << std::endl
       << std::endl
       << "  pt_" << m_name << "_prod_node_t *mp_prod_node;" << std::endl
       << std::endl
       << "public:" << std::endl
       << std::endl
       << "  void set_prod_node(pt_" << m_name << "_prod_node_t *const &node)" << std::endl
       << "  { mp_prod_node = node; }" << std::endl
       << "  void check_semantic() const { mp_prod_node->check_semantic(); }" << std::endl
       << "};" << std::endl
       << "typedef class pt_" << m_name << "_node_t pt_" << m_name << "_node_t;" << std::endl
       << std::endl;
}

void
node_t::dump_gen_parser_src(
  std::wfstream &file) const
{
  assert(true == m_is_rule_head);
  
  if (RULE_CONTAIN_NO_REGEX == m_rule_contain_regex)
  {
    assert(0 == mp_main_regex_alternative);
    
    dump_gen_parser_src_for_not_regex_alternative(file);
  }
  else
  {
    assert(mp_main_regex_alternative != 0);
    
    (void)node_for_each(mp_main_regex_alternative,
                        0,
                        move_all_tmp_regex_info_to_orig_end);
    
    dump_gen_parser_src_for_regex_alternative(file);
  }
}

void
dump_add_node_to_nodes_for_one_node_in_parse_XXX(
  std::wfstream &file,
  node_t const * const node,
  unsigned int const indent_depth)
{
  if (true == node->is_terminal())
  {
    std::wstring terminal_name = node->name();
    
    // Transform the letters of the name to uppercase.
    std::transform(terminal_name.begin(), terminal_name.end(),
                   terminal_name.begin(),
                   towupper);
    
    file << indent_line(indent_depth)
         << "add_node_to_nodes(nodes, ensure_next_token_is(WDS_TOKEN_TYPE_"
         << terminal_name
         << ", consume));" << std::endl;
  }
  else
  {
    std::wstring const &name = node->name();
    
    file << indent_line(indent_depth)
         << "add_node_to_nodes(nodes, parse_" << name
         << "(consume));" << std::endl;
  }
}
