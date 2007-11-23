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

#define USAGE_MESSAGE                               \
  "grammar_analyser.exe [options] grammar_file\n\
   option lists:\n\
   -h:\n\
   --help:\n\
      dump this message.\n\
   -o <output file name>:\n\
      specify a file where output messages will go."

bool
analyser_environment_t::parse_command_line(int argc, char **argv)
{
  int i;
  bool correct = true;
  
  for (i = 1; i < argc; ++i)
  {
    wchar_t * const tmp = fmtstr_mbstowcs(argv[i], 0);
    assert(tmp != 0);
    boost::shared_ptr<wchar_t> parm_ptr(tmp, fmtstr_delete);
    
    if ((0 == wcscmp(L"-h", parm_ptr.get())) ||
        (0 == wcscmp(L"--help", parm_ptr.get())))
    {
      fwprintf(stderr, L"%s\n", USAGE_MESSAGE);
    }
#if defined(_DEBUG)
    else if (0 == wcscmp(L"-cmp_ans", parm_ptr.get()))
    {
      m_cmp_ans = true;
    }
#endif
    else if (0 == wcscmp(L"-o", parm_ptr.get()))
    {
      wchar_t * const tmp = fmtstr_mbstowcs(argv[++i], 0);
      assert(tmp != 0);
      parm_ptr.reset(tmp, fmtstr_delete);
      
      wchar_t * const filename = fmtstr_new(L"%s%d",
                                            parm_ptr.get(),
                                            m_curr_output_filename_idx);
      mp_output_file = new std::wfstream(filename,
                                         std::ios_base::out |
                                         std::ios_base::binary);
      fmtstr_delete(filename);
      if (false == mp_output_file->is_open())
      {
        fwprintf(stderr, L"Can not open output file: %s\n", argv[i]);
        return false;
      }
      m_output_filename = parm_ptr.get();
    }
    else
    {
      if (L'-' == *parm_ptr)
      {
        fprintf(stderr, "unknown option: %s\n", parm_ptr.get());
        correct = false;
      }
      else
      {
        // The only possible now is source file,
        // then try to find one.
        m_grammar_file_name = parm_ptr.get();
        m_grammar_file.open(parm_ptr.get(), std::ios_base::in | std::ios_base::binary);
        
        if (false == m_grammar_file.is_open())
        {
          fwprintf(stderr, L"Can not open grammar file: %s\n", argv[i]);
          return false;
        }
      }
    }
  }
  
  if (false == correct)
  {
    return false;
  }
  if (false == m_grammar_file.is_open())
  {
    fprintf(stderr, "You must specify a grammar file.\n");
    return false;
  }
  
  return true;
}

bool
check_not_optional(analyser_environment_t const * const /* ae */,
                   node_t * const node,
				   void * const param)
{
  if (node->is_optional())
  {
    return false;
  }
  else
  {
    return true;
  }
}

bool
clear_cyclic_set(analyser_environment_t const * const /* ae */,
                 node_t * const node,
                 void * const param)
{
  node->clear_cyclic_set();
  return true;
}

bool
check_no_epsilon(analyser_environment_t const * const /* ae */,
                 node_t * const node,
                 void * const param)
{
  if (node->is_rule_head())
  {
    for (std::list<node_t *>::const_iterator iter = node->next_nodes().begin();
         iter != node->next_nodes().end();
         ++iter)
    {
      if ((*iter) == node->rule_end_node())
      {
        return false;
      }
    }
  }
  return true;
}

bool
link_nonterminal(analyser_environment_t const * const ae,
                 node_t * const node,
				 void * const param)
{
  if ((false == node->is_rule_head()) &&
      (false == node->is_terminal()) &&
      (false == node->name().empty()))
  {
    node_t * const nonterminal_node =
      ae->nonterminal_rule_node(node->name());
    if (0 == nonterminal_node)
    {
      ae->log(L"<ERROR>: can not find the rule for the nonterminal '%s'\n",
                 node->name().c_str());
      return false;
    }
    else
    {
      assert(false == nonterminal_node->name().empty());
      assert(0 == nonterminal_node->name().compare(node->name()));
    }
    node->set_nonterminal_rule_node(nonterminal_node);
    nonterminal_node->add_refer_to_me_node(node);
  }
  return true;
}

int
main(int argc, char **argv)
{
#if defined(_DEBUG)
  {
#endif
    boost::shared_ptr<analyser_environment_t> ae(
      new analyser_environment_t);
    
    if (false == ae->parse_command_line(argc, argv))
    {
      return 1;
    }
    
    std::list<keyword_t> keywords;
    keywords.push_back(keyword_t(L"_apostrophe", KEYWORD_POS_BACK));
    
    try
    {
      // ====================================================
      //                    read grammar
      // ====================================================
      ae->read_grammar(keywords);
      
      //ae->dump_tree(L"1_orig_tree.dot");
      assert(true == ae->traverse_all_nodes(check_not_optional, 0, 0));
      
      // ====================================================
      //                  link non-terminal
      // ====================================================
      if (false == ae->traverse_all_nodes(link_nonterminal, 0, 0))
      {
        return 1;
      }
      else
      {
        //ae->dump_tree(L"2_link_nonterminal.dot");
      }
      
      ae->check_nonterminal_linking();
      
      if (false == ae->check_grammar())
      {
        ae->log(L"<ERROR>: grammar checking failed.\n");
        return 1;
      }
    }
    catch (ga_exception_t const &)
    {
      ae->log(L"<ERROR>: wrong grammar file.\n");
      return 1;
    }
    
    assert(true == ae->traverse_all_nodes(check_if_regex_info_is_empty, 0, 0));
    assert(true == ae->traverse_all_nodes(check_if_regex_info_is_correct_for_traverse_all_nodes, 0, 0));
    
    // After regex OR statements expansion, I can delete those
    // regexs of type REGEX_TYPE_ONE.
    //
    // Ex:
    //
    // A (B C | D) E
    //
    // will become:
    //
    // A (B C) E
    // A (D) E
    //
    // And this will be equivalent to the following form:
    //
    // A B C E
    // A D E
    ae->traverse_all_nodes(delete_regex_type_one, 0, 0);
    
    // I have to expand regex info before the restoring
    // below.
    //
    // Ex:
    //
    // After reading the grammar, the contents of the
    // regex_info & tmp_regex_info are as the following:
    //
    //                  ((a b (c (d e)*)* f)* g)
    // -------------------------------------------
    // regex_info
    // tmp_regex_info     .    .  .              <- back()
    //                    .                      <- front()
    //
    // There are 2 elements in the 'tmp_regex_info' of node
    // 'a', and 1 element in the 'tmp_regex_info' of node
    // 'c'.
    //
    // If I restore the regex first, and then expand each
    // regex by iterate all the nodes in a regex and
    // push_back() & push_front() each regex_info at the
    // starting node of that regex into the rest nodes of
    // that regex, then the problem will be occurred at node
    // 'd'. When I expand the regex starting at node 'a',
    // the 'tmp_regex_info' of node 'd' will become:
    //
    // back() -> .  <- from node 'a'
    //           .  <- from node 'a'
    //           .  <- original
    //
    // Then if I want to expand the regex starting from node
    // 'c', then the desired result will be:
    //
    // back() -> .  <- from node 'a'
    //           .  <- from node 'a'
    //           .  <- from node 'c'
    //           .  <- original
    //
    // This will force me to remember the front of
    // the latest expansion operation.
    //
    // However, if I expand the regex before the restoring,
    // then I just need to use push_front() all the time.
    ae->fill_regex_info_to_relative_nodes_for_each_regex_group();
    
    // restore each node's 'regex_info' from
    // 'tmp_regex_info'.
    ae->restore_regex_info();
    
    // ====================================================
    //               remove useless rules
    // ====================================================
    ae->log(L"<INFO>: Removing useless rules.\n");
    ae->remove_useless_rule();
    if (0 == ae->top_level_nodes().size())
    {
      return 1;
    }
    
    ae->check_nonterminal_linking();
    
    // ====================================================
    //                detect left recursion
    // ====================================================
    std::list<std::list<node_t *> > left_recursion_set;
    
    bool const has_left_recursion = ae->detect_left_recursion(left_recursion_set);
    
    if ((true == has_left_recursion) ||
        (true == ae->use_paull_algo_to_remove_left_recursion()))
    {
      // Paull's algorithm (remove left recursion)
      if (true == ae->use_paull_algo_to_remove_left_recursion())
      {
        // ====================================================
        //            detect nullable non-terminals
        // ====================================================
        ae->log(L"<INFO>: Detect nullable nonterminals.\n");
        ae->detect_nullable_nonterminal();
        //ae->dump_tree(L"3_detect_nullable.dot");
    
        ae->check_nonterminal_linking();
    
        // ====================================================
        //           remove epsilon production
        // ====================================================
        ae->remove_epsilon_production();
        if (false == ae->check_grammar())
        {
          ae->log(L"<ERROR>: grammar checking failed.\n");
          return 1;
        }
    
        ae->check_nonterminal_linking();
    
        assert(true == ae->traverse_all_nodes(check_no_epsilon, 0, 0));
        assert(true == ae->traverse_all_nodes(check_not_optional, 0, 0));
  
        //ae->dump_tree(L"4_remove_epsilon.dot");
  
        // ====================================================
        //              remove direct cyclic
        // ====================================================
        ae->log(L"<INFO>: Removing direct cyclic.\n");
        ae->remove_direct_cyclic();
    
        if (0 == ae->top_level_nodes().size())
        {
          ae->log(L"<WARN>: Reach an empty grammar after removing direct cyclic.\n");
          return 0;
        }
    
        ae->check_nonterminal_linking();
    
        assert(true == ae->check_grammar());
        assert(true == ae->traverse_all_nodes(check_no_epsilon, 0, 0));
  
        //ae->dump_tree(L"5_remove_direct_cyclic.dot");
  
        // ====================================================
        //              detect cyclic non-terminals
        // ====================================================
        ae->log(L"<INFO>: Detect cyclic nonterminals.\n");
        ae->detect_cyclic_nonterminal();
        //ae->dump_tree(L"6_detect_cyclic.dot");
  
        ae->check_nonterminal_linking();
    
        // ====================================================
        //               remove cyclic
        // ====================================================
        ae->log(L"<INFO>: Removing cyclic.\n");
        ae->remove_cyclic();
    
        if (0 == ae->top_level_nodes().size())
        {
          ae->log(L"<WARN>: Reach an empty grammar after removing cyclic.\n");
          return 0;
        }
    
        assert(true == ae->check_grammar());
        ae->check_nonterminal_linking();
    
        ae->traverse_all_nodes(clear_cyclic_set, 0, 0);
        ae->detect_cyclic_nonterminal();
    
        assert(true == ae->traverse_all_nodes(check_not_cyclic, 0, 0));  
        assert(true == ae->traverse_all_nodes(check_no_epsilon, 0, 0));
        //ae->dump_tree(L"7_remove_cyclic.dot");
    
        // ====================================================
        //               find left corners
        // ====================================================
        ae->find_left_corners();
        ae->order_nonterminal_decrease_number_of_distinct_left_corner();
      
        ae->check_nonterminal_linking();
      
        // ====================================================
        //               remove left recursion
        // ====================================================
        ae->log(L"<INFO>: Removing left recursion.\n");
        ae->remove_left_recur();
      
        assert(true == ae->check_grammar());
      
        //ae->dump_tree(L"8_remove_left_recurison.dot");
        ae->check_nonterminal_linking();
      
        // ====================================================
        //               remove useless rule
        // ====================================================
        ae->log(L"<INFO>: Removing useless rule.\n");
        ae->remove_useless_rule();
        
#if defined(_DEBUG)
        // ====================================================
        //                detect left recursion
        // ====================================================
        {
          left_recursion_set.clear();
          
          bool const has_left_recursion = ae->detect_left_recursion(left_recursion_set);
          
          assert(false == has_left_recursion);
        }
#endif
      }
      else
      {
        ae->log(L"<ERROR>: There are left recursions among:\n");
        
        int i = 0;
        
        BOOST_FOREACH(std::list<node_t *> const &sets, left_recursion_set)
        {
          ae->log(L"%d) ", i);
          
          BOOST_FOREACH(node_t const * const node, sets)
          {
            ae->log(L"%s ", node->name().c_str());
          }
          
          ae->log(L"\n");
        }
        
        return 1;
      }
    }
    
    std::wstring final_grammar_filename(L"final_grammar.gra");
    
    ae->dump_grammar(final_grammar_filename.c_str());
        
    ae->log(L"<INFO>: Determine the position of each node.\n");
    ae->determine_node_position();
    
    ae->log(L"<INFO>: Finding EOF situation.\n");
    ae->find_eof();
    
    // ====================================================
    //               compute lookahead set
    // ====================================================
    ae->log(L"<INFO>: Compute lookahead terminals for each node.\n");
    ae->compute_lookahead_set();
    
    std::wstring final_grammar_lookahead_filename(L"final_grammar_lookahead.gra");
    
    ae->dump_grammar(final_grammar_lookahead_filename.c_str());
    //ae->dump_tree(L"9_final_grammar_tree.dot");
    
    ae->close_output_file();
    
    // ====================================================
    //               compare answer
    // ====================================================
#if defined(_DEBUG)
    if (true == ae->cmp_ans())
    {
      if (true == ae->read_answer_file())
      {
        if (false == ae->perform_answer_comparison())
        {
          ae->log(L"<ERROR>: Answer comparison failed.\n");
          return 1;
        }
        else
        {
          ae->log(L"<INFO>: Answer comparison success.\n");
        }
      }
      else
      {
        ae->log(L"<ERROR>: Answer file can not be opened.\n");
        return 1;
      }
    }
#endif
    
    // ====================================================
    //        calculate appear times for each node in
    //                   each alternative
    // ====================================================
    {
      BOOST_FOREACH(node_t const * const node, ae->top_level_nodes())
      {
        if (false == ae->using_pure_BNF())
        {
          assert(node->main_regex_alternative() != 0);
          
          count_appear_times_for_node_range(node->main_regex_alternative(), 0);
        }
        else
        {
          BOOST_FOREACH(node_t * const alternative_start,
                        node->next_nodes())
          {
            // Check if this alternative is an empty one.
            if (alternative_start != node->rule_end_node())
            {
              count_appear_times_for_node_range(alternative_start, 0);
            }
          }
        }
      }
    }
    
    // ====================================================
    //               dump generated codes
    // ====================================================
    {
      ae->log(L"<INFO>: Dump parser_basic_types.hpp\n");
      
      std::wstring filename(L"parser_basic_types.hpp");
      
      std::auto_ptr<std::wfstream> const file(
        new std::wfstream(filename.c_str(),
                          std::ios_base::out | std::ios_base::binary));
      ae->dump_gen_parser_basic_types_hpp(*file);
      file->close();
    }
    
    {
      ae->log(L"<INFO>: Dump parser_nodes.hpp\n");
      
      std::wstring filename(L"parser_nodes.hpp");
      
      std::auto_ptr<std::wfstream> const file(
        new std::wfstream(filename.c_str(),
                          std::ios_base::out | std::ios_base::binary));
      ae->dump_gen_parser_nodes_hpp(*file);
      file->close();
    }
    
    {
      ae->log(L"<INFO>: Dump parser.cpp\n");
      ae->dump_gen_parser_cpp();
    }
    
    {
      ae->log(L"<INFO>: Dump frontend.hpp\n");
      
      std::wstring filename(L"frontend.hpp");
      
      std::auto_ptr<std::wfstream> const file(
        new std::wfstream(filename.c_str(),
                          std::ios_base::out | std::ios_base::binary));
      
      ae->dump_gen_frontend_hpp(*file);
      file->close();
    }
    
    {
      ae->log(L"<INFO>: Dump frontend.cpp\n");
      
      std::wstring filename(L"frontend.cpp");
      
      std::auto_ptr<std::wfstream> const file(
        new std::wfstream(filename.c_str(),
                          std::ios_base::out | std::ios_base::binary));
      
      ae->dump_gen_frontend_cpp(*file);
      file->close();
    }
    
    {
      ae->log(L"<INFO>: Dump token.hpp\n");
      
      std::wstring filename(L"token.hpp");
      
      std::auto_ptr<std::wfstream> const file(
        new std::wfstream(filename.c_str(),
                          std::ios_base::out | std::ios_base::binary));
      
      ae->dump_gen_token_hpp(*file);
      file->close();
    }
    
    {
      ae->log(L"<INFO>: Dump main.cpp\n");
      
      std::wstring filename(L"main.cpp");
      
      std::auto_ptr<std::wfstream> const file(
        new std::wfstream(filename.c_str(),
                          std::ios_base::out | std::ios_base::binary));
      
      ae->dump_gen_main_cpp(*file);
      file->close();
    }
    
#if defined(_DEBUG)
  }
  if (false == dump_unfreed())
  {
    return 2;
  }
  if (fmtstr_false == fmtstr_dump_unfreed())
  {
    return 2;
  }
#endif
  
  return 0;
}
