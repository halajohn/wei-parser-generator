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
#include "regex.hpp"
#include "ga_exception.hpp"
#include "alternative.hpp"

#include "wcl_memory_debugger\memory_debugger.h"

analyser_environment_t::analyser_environment_t()
  : m_state(STATE_READ_TERMINAL),
    m_indicate_terminal_rule(false),
    mp_curr_parsing_alternative_head(0),
    m_curr_output_filename_idx(0),
    mp_output_file(0),
#if defined(_DEBUG)
    m_cmp_ans(false),
#endif
    mp_last_created_node_during_parsing(0),
    m_next_token_is_regex_OR_start_node(false),
    m_max_lookahead_searching_depth(2),
    m_use_paull_algo_to_remove_left_recursion(false),
    m_enable_left_factor(false),
    m_using_pure_BNF(false)
{
}

analyser_environment_t::~analyser_environment_t()
{
  for (std::list<node_t *>::iterator iter = m_top_level_nodes.begin();
       iter != m_top_level_nodes.end();
       )
  {
    assert(true == (*iter)->is_rule_head());
    
    std::list<node_t *> alternatives = (*iter)->next_nodes();
    ::remove_alternatives(this, alternatives, true, true, true);
    
    assert(0 == (*iter)->next_nodes().size());
    assert(0 == (*iter)->rule_end_node()->prev_nodes().size());
    
    if (RULE_CONTAIN_REGEX_OR == (*iter)->rule_contain_regex())
    {
      assert((*iter)->main_regex_alternative() != 0);
      
      alternatives.clear();
      alternatives.push_back((*iter)->main_regex_alternative());
      
      ::remove_alternatives(this, alternatives, true, true, false);
    }
    
    boost::checked_delete((*iter)->rule_end_node());
    
    node_t * const tmp = (*iter);
    iter = m_top_level_nodes.erase(iter);
    
    remove_rule_head(tmp);
    
    boost::checked_delete(tmp);
  }
  
  BOOST_FOREACH(node_t * const node, m_answer_nodes)
  {
    boost::checked_delete(node);
  }
}

namespace
{
  void
  update_rule_node_attribute_when_seeing_a_regex(
    analyser_environment_t * const ae)
  {
    assert(ae != 0);
    
    // I see a regex, thus I have to set the
    // 'rule_contain_regex' attribute of the this rule node
    // (ie. the latest one) to true.
    //
    // Note:
    //
    // I don't set this variable when I see the '('
    // token. This is because I want to set the
    // 'rule_contain_regex' & 'main_regex_alternative' at
    // the same time.
    //
    // Ex, In the following case,
    // when I see the first '(' token, I don't parse the
    // first node yet, hence I can not set
    // 'main_regex_alternative'.
    //
    // (A B)* C
    //
    // Thus, I move the setting of these 2 variables to
    // the time when I see a ')'.
    switch (ae->last_rule_node()->rule_contain_regex())
    {
    case RULE_CONTAIN_NO_REGEX:
      ae->last_rule_node()->rule_contain_regex() = RULE_CONTAIN_REGEX;
      break;
      
    case RULE_CONTAIN_REGEX:
    case RULE_CONTAIN_REGEX_OR:
      break;
      
    default:
      assert(0);
      break;
    }
    
    // If this rule is a regex rule, then it should
    // contain only 1 alternative.
    assert(1 == ae->last_rule_node()->next_nodes().size());
    
    // I have already parse some nodes of this
    // alternative.
    if (ae->last_rule_node()->main_regex_alternative() != 0)
    {
      assert(ae->last_rule_node()->main_regex_alternative() == 
             ae->last_rule_node()->next_nodes().front());
    }
    else
    {
      ae->last_rule_node()->main_regex_alternative() =
        ae->last_rule_node()->next_nodes().front();
    }
  }
}

/// 
/// \dot
/// digraph example {
/// rankdir=LR
/// rank=same;
/// {
/// rule_head_1
/// rule_head_2
/// }
/// rule_end_node_1
/// rule_end_node_2
/// alternative_start_1
/// alternative_start_2
/// alternative_1
/// alternative_2
/// alternative_start_3
/// alternative_start_4
/// alternative_3
/// alternative_4
/// rule_head_1->rule_end_node_1
/// rule_head_1->alternative_start_1->alternative_1->rule_end_node_1
/// rule_head_1->alternative_start_2->alternative_2->rule_end_node_1
/// rule_head_2->rule_end_node_2
/// rule_head_2->alternative_start_3->alternative_3->rule_end_node_2
/// rule_head_2->alternative_start_4->alternative_4->rule_end_node_2
/// rule_head_1->rule_head_2
/// }
/// \enddot
///
/// @param str 
///
/// @return 
///
bool
analyser_environment_t::parse_rule_head(
  std::wstring const &str,
  std::list<regex_stack_elem_t> &regex_stack)
{
  // If I have already parsed some rule head nodes, then I
  // have to check something and do some operations for the
  // latest rule head node.
  if (m_top_level_nodes.size() != 0)
  {
    // Check to see if this non-terminal rule node has
    // already been defined.
    if (nonterminal_rule_node(str) != 0)
    {
      log(L"<ERROR>: %s has already be defined.\n", str.c_str());
      return false; 
    }
    
    // Check to see if this alternative has already been
    // defined in this rule node.
    if (true == find_same_alternative_in_one_rule(
          last_rule_node()->next_nodes().back(), 0))
    {
      log(L"\n<ERROR>: duplicate production found.\n");
      return false;
    }
    
    // I will count the alternative length for each
    // alternative, so that I have to count it for the last
    // alternative of the last rule node here.
    last_rule_node()->next_nodes().back()->alternative_length() =
      count_alternative_length(last_rule_node()->next_nodes().back());
    
    if (false == m_using_pure_BNF)
    {
      assert(false == m_next_token_is_regex_OR_start_node);
      
      // If 'm_using_pure_BNR' is 'false', then every rule
      // will be treated as a regex rule, so that I have to
      // update its attribute.
      update_rule_node_attribute_when_seeing_a_regex(this);      
      assert(last_rule_node()->main_regex_alternative() != 0);
      
      // I have to make sure all the regex_info for the last
      // rule head is closed.
      //
      // Ex:
      //
      // S
      //  : a b c
      //  | d e
      //
      // The regex_info starting at node 'a' is not closing
      // at this point, so I have to close it now.
      if (regex_stack.size() != 0)
      {
        assert(1 == regex_stack.size());
        
        assert(regex_stack.front().m_regex_OR_range.size() != 0);
        assert(0 == regex_stack.front().m_regex_OR_range.back().mp_end_node);
        regex_stack.front().m_regex_OR_range.back().mp_end_node = mp_last_created_node_during_parsing;
        
        regex_stack.front().mp_node->push_back_regex_info(
          regex_stack.front().m_regex_OR_range,
          REGEX_TYPE_OR);
        
        assert(last_rule_node()->main_regex_alternative() != 0);        
        
        regex_stack.front().mp_node->push_back_regex_info(
          last_rule_node()->main_regex_alternative(),
          mp_last_created_node_during_parsing,
          0,
          REGEX_TYPE_ONE);
        
        regex_stack.pop_back();
      }
    }
    
    // When encounter a new rule node, I will build new
    // alternatives according to the latest alternative of
    // the latest rule node.
    {
      if (false == m_using_pure_BNF)
      {
        // build new alternatives according to this alternative
        // by regular expression.
        build_new_alternative_for_regex_nodes(last_rule_node()->next_nodes().back());
        
        remove_same_alternative_for_one_rule(this, last_rule_node());
      }
      
      {
        std::vector<check_node_func> check_func;
        
        check_func.push_back(find_same_alternative_in_one_rule);
        
        if (true == m_using_pure_BNF)
        {
          // build new alternatives according to this alternative
          // by optional nodes.
        
          // There is another facility to resolve the
          // 'optional' nodes (i.e. ?) problem in EBNF form.
          build_new_alternative_for_optional_nodes(
            last_rule_node()->next_nodes().back(),
            check_func,
            false);
        }
      }
    }
  }
  
  if (true == is_terminal(str))
  {
    throw ga_exception_t();
  }
  
  node_t * const node = new node_t(this, 0, str);
  node_t * const rule_end_node = new node_t(this, node);
  
  mp_last_created_node_during_parsing = node;
  
  node->set_is_rule_head(true);
  node->set_rule_end_node(rule_end_node);
  
  // First rule head will be the starting rule.
  if (0 == m_top_level_nodes.size())
  {
    node->set_starting_rule();
  }
  
  add_top_level_nodes(node);
  
  hash_rule_head(node);
  
  return true;
}

node_t *
analyser_environment_t::create_node_by_name(
  node_t * const rule_node,
  std::wstring const &str)
{
  node_t * const node = new node_t(this, rule_node, str);
  mp_last_created_node_during_parsing = node;
  return node;
}

void
analyser_environment_t::parse_alternative_start(
  std::wstring const &str)
{
  if (false == m_using_pure_BNF)
  {
    assert(0 == last_rule_node()->next_nodes().size());
  }
  else
  {
    // Check to see if I have already created a new
    // alternative for this rule, if yes, I will do some post
    // operations for this new alternative.
    if (last_rule_node()->next_nodes().size() != 0)
    {
      if (true == find_same_alternative_in_one_rule(
            last_rule_node()->next_nodes().back(), 0))
      {
        log(L"\n<INFO>: duplicate production found.\n");
        throw ga_exception_t();
      }
    
      last_rule_node()->next_nodes().back()->alternative_length() =
        count_alternative_length(last_rule_node()->next_nodes().back());
    
      // I will build new alternatives according to the
      // latest alternative of this rule node.
      {
        std::vector<check_node_func> check_func;
      
        check_func.push_back(find_same_alternative_in_one_rule);
      
        if (true == m_using_pure_BNF)
        {
          // build new alternatives according to this alternative
          // by optional nodes.
          build_new_alternative_for_optional_nodes(
            last_rule_node()->next_nodes().back(),
            check_func,
            false);
        }
      }
    }
  }
  
  // 'rule head'----------------------------------> 'rule end node'
  //  |  |-> 'alternative start' 'alternative' ------^   ^
  //  |  +-> 'alternative start' 'alternative' ----------+
  //  |
  // 'rule head'----------------------------------> 'rule end node'
  //     |-> 'alternative start' 'alternative' ------^   ^
  //     +-> 'alternative start' 'alternative' ----------+ 
  assert(m_top_level_nodes.size() != 0);
  
  if (str.compare(L"...epsilon...") != 0)
  {
    node_t * const node = create_node_by_name(last_rule_node(), str);
    assert(node != 0);
    
    if (false == last_rule_node()->is_my_next_node(node))
    {
      assert(false == node->is_my_prev_node(last_rule_node()));
      
      last_rule_node()->append_node(node);
      node->prepend_node(last_rule_node());
    }
    
    if (false == is_terminal(node->name()))
    {
      // It is a non-terminal.
      if (false == node->is_my_next_node(last_rule_end_node()))
      {
        assert(false == last_rule_end_node()->is_my_prev_node(node));
        
        node->append_node(last_rule_end_node());
        last_rule_end_node()->prepend_node(node);
      }
      
      last_rule_node()->next_nodes().back()->set_node_before_the_rule_end_node(node);
    }
    else
    {
      // This is a terminal.
      if (false == node->is_my_next_node(last_rule_end_node()))
      {
        assert(false == last_rule_end_node()->is_my_prev_node(node));
        
        node->append_node(last_rule_end_node());
        last_rule_end_node()->prepend_node(node);
      }
      
      last_rule_node()->next_nodes().back()->
        set_node_before_the_rule_end_node(node);
    }
    mp_curr_parsing_alternative_head = node;
  }
  else
  {
    // This is an epsilon production.
    if (false == last_rule_node()->is_my_next_node(last_rule_end_node()))
    {
      assert(false == last_rule_end_node()->is_my_prev_node(last_rule_node()));
      
      last_rule_node()->append_node(last_rule_end_node());
      last_rule_end_node()->prepend_node(last_rule_node());
    }
    
    mp_curr_parsing_alternative_head = 0;
  }
}

void
analyser_environment_t::parse_alternative(std::wstring const &str)
{
  assert(str.compare(L"...epsilon...") != 0);
  
  // 'rule head'-----------------------------------'rule end node'
  // |  --> 'alternative start' 'alternative' ------^   ^
  // |  --> 'alternative start' 'alternative' ----------|
  // |
  // 'rule head'-----------------------------------'rule end node'
  //    --> 'alternative start' 'alternative' ------^   ^
  //    --> 'alternative start' 'alternative' ----------|
  node_t * const node = create_node_by_name(last_rule_node(), str);
  
  assert(m_top_level_nodes.size() != 0);
  assert(last_rule_node()->next_nodes().size() != 0);
  
  mp_curr_parsing_alternative_head->node_before_the_rule_end_node()->
    break_append_node(last_rule_node()->rule_end_node());
  
  last_rule_node()->rule_end_node()->
    break_prepend_node(
      mp_curr_parsing_alternative_head->node_before_the_rule_end_node());
  
  if (false == mp_curr_parsing_alternative_head->node_before_the_rule_end_node()
      ->is_my_next_node(node))
  {
    assert(false == node->is_my_prev_node(mp_curr_parsing_alternative_head->
                                          node_before_the_rule_end_node()));
    
    mp_curr_parsing_alternative_head->node_before_the_rule_end_node()->append_node(node);
    node->prepend_node(mp_curr_parsing_alternative_head->node_before_the_rule_end_node());
  }
  
  if (true == is_terminal(node->name()))
  {
    if (false == node->is_my_next_node(last_rule_node()->rule_end_node()))
    {
      assert(false == last_rule_node()->rule_end_node()->is_my_prev_node(node));
      node->append_node(last_rule_node()->rule_end_node());
      last_rule_node()->rule_end_node()->prepend_node(node);
    }
    
    mp_curr_parsing_alternative_head->set_node_before_the_rule_end_node(node);
  }
  else
  {
    if (false == node->is_my_next_node(last_rule_node()->rule_end_node()))
    {
      assert(false == last_rule_node()->rule_end_node()->is_my_prev_node(node));
      node->append_node(last_rule_node()->rule_end_node());
      last_rule_node()->rule_end_node()->prepend_node(node);
    }
    
    mp_curr_parsing_alternative_head->set_node_before_the_rule_end_node(node);
  }
}

void
analyser_environment_t::parse_grammar(
  std::list<keyword_t> const &keywords,
  std::wstring const &str,
  std::list<regex_stack_elem_t> &regex_stack)
{
  for (std::list<keyword_t>::const_iterator iter = keywords.begin();
       iter != keywords.end();
       ++iter)
  {
    bool find = false;
    
    switch ((*iter).pos_constraint)
    {
    case KEYWORD_POS_FRONT:
      if (str.size() >= (*iter).name.size())
      {
        if (0 == str.compare(0, (*iter).name.size(), (*iter).name))
        {
          find = true;
        }
      }
      break;
      
    case KEYWORD_POS_BACK:
      if (str.size() >= (*iter).name.size())
      {
        if (0 == str.compare(str.size() - (*iter).name.size(),
                             (*iter).name.size(),
                             (*iter).name))
        {
          find = true;
        }
      }
      break;
      
    case KEYWORD_POS_ANYWHERE:
      if (str.find((*iter).name, 0) != std::wstring::npos)
      {
        find = true;
      }
      break;
      
    default:
      assert(0);
      break;
    }
    
    if (true == find)
    {
      std::wstring message;
      message.append((*iter).name);
      message.append(L" is a keyword which can not appear in ");
      switch ((*iter).pos_constraint)
      {
      case KEYWORD_POS_FRONT: message.append(L"the front of "); break;
      case KEYWORD_POS_BACK: message.append(L"the back of "); break;
      case KEYWORD_POS_ANYWHERE: break;
      default: assert(0); break;
      }
      message.append(L"the symbol name of the grammar.");
      log(L"%s\n", message.c_str());
      throw ga_exception_t();
    }
  }
  
  if (STATE_READ_TERMINAL == m_state)
  {
    if (true == m_indicate_terminal_rule)
    {
      log(L"<terminal>");
    }
    
    // =========================================
    //          parse terminal symbol
    // =========================================
    hash_terminal(str);
  }
  else if (STATE_START_RULE == m_state)
  {
    if (true == m_indicate_terminal_rule)
    {
      log(L"<rule>");
    }
    
    // =========================================
    //             parse rule head
    // =========================================
    if (false == parse_rule_head(str, regex_stack))
    {
      throw ga_exception_t();
    }
  }
  else if (STATE_READ_ALTERNATIVE_START == m_state)
  {
    // =========================================
    //          parse alternative start
    // =========================================
    parse_alternative_start(str);
    
    m_state = STATE_READ_ALTERNATIVE;
  }
  else if (STATE_READ_ALTERNATIVE == m_state)
  {
    // =========================================
    //             parse alternative
    // =========================================
    parse_alternative(str);
  }
  else
  {
    assert(0);
  }
}

node_t *
analyser_environment_t::last_rule_end_node()
{
  return m_top_level_nodes.back()->rule_end_node();
}

node_t *
analyser_environment_t::last_rule_node()
{
  return m_top_level_nodes.back();
}

enum PARSING_CTRL_CMD_ENUM
{
  PARSING_CTRL_CMD_NONE,
  PARSING_CTRL_CMD_AS_TERMINAL
};

enum PARSING_OPTION_CMD_ENUM
{
  PARSING_OPTION_CMD_NONE,
  PARSING_OPTION_CMD_K,
  PARSING_OPTION_CMD_USE_PAULL_ALGO,
  PARSING_OPTION_CMD_ENABLE_LEFT_FACTOR,
  PARSING_OPTION_CMD_USING_PURE_BNF
};

enum PARSING_STATE_ENUM
{
  PARSING_STATE_NORMAL,
  PARSING_STATE_GRAMMAR,
  PARSING_STATE_OPTION,
  PARSING_STATE_CONTROL
};

void
analyser_environment_t::read_grammar(
  std::list<keyword_t> const &keywords)
{
  std::list<wchar_t> normal_state_delimiter;
  normal_state_delimiter.push_back(L':');
  normal_state_delimiter.push_back(L';');
  normal_state_delimiter.push_back(L'"');
  normal_state_delimiter.push_back(L'{');
  normal_state_delimiter.push_back(L'(');
  normal_state_delimiter.push_back(L')');
  normal_state_delimiter.push_back(L'|');
  normal_state_delimiter.push_back(L'[');
  // EBNF part
  normal_state_delimiter.push_back(L'*');
  normal_state_delimiter.push_back(L'+');
  normal_state_delimiter.push_back(L'?');
  
  std::list<wchar_t> grammar_state_delimiter;
  grammar_state_delimiter.push_back(L'"');
  
  std::list<wchar_t> ctrl_state_delimiter;
  ctrl_state_delimiter.push_back(L'=');
  ctrl_state_delimiter.push_back(L',');
  ctrl_state_delimiter.push_back(L';');
  ctrl_state_delimiter.push_back(L']');
  
  std::list<wchar_t> option_state_delimiter;
  option_state_delimiter.push_back(L'=');
  option_state_delimiter.push_back(L';');
  option_state_delimiter.push_back(L'}');
  
  boost::shared_ptr<std::wstring> gram_str(new std::wstring);
  PARSING_STATE_ENUM state = PARSING_STATE_NORMAL;
  bool repeat = true;
  std::list<regex_stack_elem_t> regex_stack;
  
  do
  {
    switch (state)
    {
    case PARSING_STATE_GRAMMAR:
      {
        // read a string one at a time according to
        // 'grammar_state_delimiter'.
        boost::shared_ptr<std::wstring> str;
        
        try
        {
          str = lexer_get_grammar_string(grammar_state_delimiter);
          assert(str->size() != 0);
        }
        catch (Wcl::Lexerlib::EndOfSourceException &)
        {
          assert(0);
        }
        
        if (0 == str->compare(L"\""))
        {
          // read a '"'
          
          ///////////////////////////////////////////////////
          //             start grammar parsing
          ///////////////////////////////////////////////////          
          
          // If I have already read a grammar string, and
          // read another '"' character now, then this means
          // I have read a complete grammar string now, and
          // I will start to parse this string.
          parse_grammar(keywords, *(gram_str.get()), regex_stack);
          
          state = PARSING_STATE_NORMAL;
          
          // To see if this grammar string is the starting
          // node of one or more regular expression.
          //
          // If I see a '(' symbol, then I will add a new
          // empty 'regex_stack_elem_t' into
          // 'regex_stack'. Hence, if I read a real grammar
          // symbols later, I can check the value of the
          // element in this 'regex_stack' from end to
          // beginning to determine whether this symbol is
          // the starting one of one or more regular
          // expression.
          // 
          // If yes, store this node into the empty entry in
          // the regex stack.
          // 
          // Note: the empty entries should be appear in the
          // end of the regex stack.
          //
          // Ex:
          // 
          // A ((B C (D E)*)* F)*
          //
          // The 'regex_stack' should be:
          //
          // ----------->
          // (
          // (  (
          // (B (B
          
#if defined(_DEBUG)
          // I use this variable to ensure that the empty
          // 'regex_stack_elem_t' are continus.
          //
          // Ex:
          //
          // ------------------->
          //
          // Correct: (A (A (B  _ _ _
          //
          // Wrong:   (A  _  _ (B _ _
          bool empty_regex_search_end = false;
#endif
          
          for (std::list<regex_stack_elem_t>::reverse_iterator iter =
                 regex_stack.rbegin();
               iter != regex_stack.rend();
               ++iter)
          {
            if (0 == (*iter).mp_node)
            {
              // I just recognize one or more '(' symbols,
              // and put one or more empty
              // 'regex_stack_elem_t' into the
              // 'regex_stack'.
#if defined(_DEBUG)
              assert(false == empty_regex_search_end);
#endif
              
              // I do not support A (|B) yet.
              assert(false == m_next_token_is_regex_OR_start_node);
              
              (*iter).mp_node = mp_last_created_node_during_parsing;
            }
#if defined(_DEBUG)
            else
            {
              empty_regex_search_end = true;
            }
#endif
          }
          
          if (true == m_next_token_is_regex_OR_start_node)
          {
            // I just read a '|' symbol.
            
            // If I have already read a regex OR symbol
            // ('|'), this means I have to been in a regex
            // now (i.e. after a '(' and before a ')')
            assert(regex_stack.size() != 0);
            
            // The current regular expression is at the top
            // (back) of 'regex_stack'.
            regex_stack_elem_t &curr_regex = regex_stack.back();
            
            // Get the regex OR range info.
            std::list<regex_range_t> &regex_OR_range =
              curr_regex.m_regex_OR_range;
            
            // The second one of this newly created node
            // pair will be filled later if I read another
            // '|' or a ')' symbol.
            regex_OR_range.push_back(regex_range_t(
                                       mp_last_created_node_during_parsing, 0));
            
            m_next_token_is_regex_OR_start_node = false;
          }
        }
        else
        {
          *(gram_str.get()) += *(str.get());
        }
      }
      break;
      
    case PARSING_STATE_CONTROL:
      {
        bool ctrl_value_start = false;
        PARSING_CTRL_CMD_ENUM ctrl_cmd = PARSING_CTRL_CMD_NONE;
        
        for (;;)
        {
          boost::shared_ptr<std::wstring> ctrl_str;
          
          try
          {
            ctrl_str = lexer_get_grammar_string(ctrl_state_delimiter);
            assert(ctrl_str->size() != 0);
          }
          catch (Wcl::Lexerlib::EndOfSourceException &)
          {
            assert(0);
          }
          
          if (0 == ctrl_str->compare(L"]"))
          {
            state = PARSING_STATE_NORMAL;
            break;
          }
          else if (0 == ctrl_str->compare(L","))
          {
          }
          else if (0 == ctrl_str->compare(L";"))
          {
            ctrl_value_start = false;
            ctrl_cmd = PARSING_CTRL_CMD_NONE;
          }
          else if (0 == ctrl_str->compare(L"="))
          {
            assert(ctrl_cmd != PARSING_CTRL_CMD_NONE);
            ctrl_value_start = true;
          }
          else if (0 == ctrl_str->compare(L"as_terminal"))
          {
            ctrl_cmd = PARSING_CTRL_CMD_AS_TERMINAL;
          }
          else
          {
            assert(mp_last_created_node_during_parsing != 0);
            
            switch (ctrl_cmd)
            {
            case PARSING_CTRL_CMD_AS_TERMINAL:
              mp_last_created_node_during_parsing->
                add_token_name_as_terminal_during_lookahead(*(ctrl_str.get()));
              break;
              
            default:
              assert(0);
              break;
            }
          }
        }
      }
      break;
      
    case PARSING_STATE_NORMAL:
      {
        // read a string one at a time according to
        // 'normal_state_delimiter'.
        boost::shared_ptr<std::wstring> str;
        
        bool meet_eof = false;
        
        try
        {
          str = lexer_get_grammar_string(normal_state_delimiter);
          assert(str->size() != 0);
        }
        catch (Wcl::Lexerlib::EndOfSourceException &)
        {
          meet_eof = true;
        }
        
        if (true == meet_eof)
        {
          repeat = false;
        }
        else if (0 == str->compare(L":"))
        {
          switch (m_state)
          {
          case STATE_START_RULE:
            m_state = STATE_READ_ALTERNATIVE_START;
            break;
            
          default:
            throw ga_exception_t();
          }
        }
        else if (0 == str->compare(L";"))
        {
          switch (m_state)
          {
          case STATE_READ_TERMINAL:
          case STATE_READ_ALTERNATIVE:
          case STATE_READ_ALTERNATIVE_START:
            m_state = STATE_START_RULE;
            break;
            
          default:
            throw ga_exception_t();
          }
        }
        else if (0 == str->compare(L"\""))
        {
          // read a '"'
          state = PARSING_STATE_GRAMMAR;
          
          gram_str.get()->clear();
        }
        else if (0 == str->compare(L"("))
        {
          // This will start a regular expression, I will push
          // an empty 'regex_stack_elem_t' to the
          // 'regex_stack', and fill the real starting node
          // after. 
          //
          // Ex:
          //
          // A ( (B C)* D)
          //   1 2
          //
          // The content of 'regex_stack' will be
          //
          // ----->
          // ( (
          // 1 2
          regex_stack.push_back(regex_stack_elem_t());
        
          // Because I see a regex, then I can _not_ apply any
          // left recursion removing algorihtm to this
          // grammar. Because I can _not_ find any practical
          // algorithms which can apply to a grammar with
          // regex. ANTLR doesn't handle left recursions
          // either even if there are no regexes in the
          // grammar.
          m_use_paull_algo_to_remove_left_recursion = false;
          
          m_enable_left_factor = false;
          
          m_using_pure_BNF = false;
        }
        else if (0 == str->compare(L"|"))
        {
          if (false == m_using_pure_BNF)
          {
            if (regex_stack.size() != 0)
            {
              // I don't support A (B||C) yet.
              assert(false == m_next_token_is_regex_OR_start_node);
              
              // The current regular expression is at the top
              // (back) of 'regex_stack'.
              regex_stack_elem_t &curr_regex = regex_stack.back();
              
              // Get the current regular expression range info.
              std::list<regex_range_t> &regex_OR_range = 
                curr_regex.m_regex_OR_range;
              
              if (0 == regex_OR_range.size())
              {
                // If I first encounter a '|', then I will push the
                // starting node of this regular expression as the
                // first starting node of the OR statements.
                //
                // Ex:
                //
                // A (B C D E| F)
                //    ^-----^
                //    range_1
                regex_OR_range.push_back(regex_range_t(
                                           curr_regex.mp_node,
                                           mp_last_created_node_during_parsing));
              }
              else
              {
                // If this '|' symbols is not the first one of
                // this regex OR group, then I have already put
                // an empty pair of nodes into the
                // end of the 'regex_OR_range' stack, and I have
                // already fill the starting node (first one) of
                // this pair, thus I just need to fill the end
                // node (second one) of this pair.
                assert(0 == regex_OR_range.back().mp_end_node);
            
                regex_OR_range.back().mp_end_node =
                  mp_last_created_node_during_parsing;
              }
            }
            else
            {
              // The only situation when I encounter a '|'
              // symbol and 'regex_stack' is empty is:
              //
              // S
              //  : a b c
              //  | d e
              //
              // or
              //
              // S
              //  : a (b c)*
              //  | d e
              //
              // Thus I have to create a new regex_info
              // starting from 'a', and add a new regex_info
              // into node 'a'.
              assert(1 == last_rule_node()->next_nodes().size());
              
              regex_stack.push_back(regex_stack_elem_t());
              
              node_t * const alternative_start = last_rule_node()->next_nodes().front();
              assert(alternative_start != 0);
              
              regex_stack.back().mp_node = alternative_start;
              
              regex_stack.back().m_regex_OR_range.push_front(
                regex_range_t(alternative_start, mp_last_created_node_during_parsing));
              
              update_rule_node_attribute_when_seeing_a_regex(this);
            }
            
            m_next_token_is_regex_OR_start_node = true;
            last_rule_node()->rule_contain_regex() = RULE_CONTAIN_REGEX_OR;
          }
          else
          {
            switch (m_state)
            {
            case STATE_READ_ALTERNATIVE:
              m_state = STATE_READ_ALTERNATIVE_START;
              break;
              
            default:
              throw ga_exception_t();
            }
          }
        }
        else if (0 == str->compare(L")"))
        {
          // I have been in a regex now.
          assert(regex_stack.size() != 0);
          
          update_rule_node_attribute_when_seeing_a_regex(this);
          
          // I don't support A (B|) yet.
          assert(false == m_next_token_is_regex_OR_start_node);
          
          // This is the end of a regular expression statement,
          // I will get one more symbols to see the type of
          // this regular expression.
          boost::shared_ptr<std::wstring> str_regex_type;
          
          try
          {
            str_regex_type = lexer_get_grammar_string(normal_state_delimiter);
            assert(str_regex_type->size() != 0);
          }
          catch (Wcl::Lexerlib::EndOfSourceException &)
          {
            assert(0);
          }
          
          // The latest regex info is at the top of the
          // stack.
          regex_stack_elem_t &elem = regex_stack.back();
        
          // Get the current regex OR range info.
          std::list<regex_range_t> &regex_OR_range = 
            elem.m_regex_OR_range;
        
          if (regex_OR_range.size() != 0)
          {
            // This regex contains regex OR statments.
            assert(0 == regex_OR_range.back().mp_end_node);
            
            regex_OR_range.back().mp_end_node =
              mp_last_created_node_during_parsing;
          }
          
          regex_type_t regex_type;
          
          if (0 == str_regex_type->compare(L"*"))
          {
            regex_type = REGEX_TYPE_ZERO_OR_MORE;
          }
          else if (0 == str_regex_type->compare(L"+"))
          {
            regex_type = REGEX_TYPE_ONE_OR_MORE;
          }
          else if (0 == str_regex_type->compare(L"?"))
          {
            regex_type = REGEX_TYPE_ZERO_OR_ONE;
          }
          else
          {
            // because the 'putback_buffer' in the lexerlib
            // library has already had delimiter symbols in
            // it, so that I have to put back
            // 'str_regex_type' string in front of
            // them.
            // 
            // Ex:
            // 
            // a ((b | c) d)*
            // 
            // after reading the first ')', I will further
            // read ' d)' to determine the regex type. The
            // last '*' is a delimiter, so that it will have
            // to be put into the putback buffer first, and
            // then lexer will return ' d)' to me later.
            Wcl::Lexerlib::put_back(m_grammar_file,
                                    *(str_regex_type.get()));
            regex_type = REGEX_TYPE_ONE;
          }
        
          // Because I have read a ')' symbol, thus I have to
          // add a regex to the corresponded starting node of
          // that regex.
          node_t *curr_node = elem.mp_node;
        
          assert(curr_node != 0);
          assert(curr_node->name().size() != 0);
        
          if (elem.m_regex_OR_range.size() != 0)
          {
            // If there is regex OR info with this regex group,
            // then I will push the regex OR info first, and
            // then the regex info itself. Because I will handle
            // the regex itself first, and then the regex OR
            // info later.
            //
            // ie.  
            //                                            push
            // beginning                          back     |
            // ---------------------------------------     |
            //       O <- regex_OR   O <- regex_orig    <--+
            // ---------------------------------------
            //
            // This is because I will handle the node's
            // regex_info list from back to beginning.
            //
            // Thus, if I want to combine the regex_OR_info &
            // the original regex info, then I have to combine
            // the one after the regex_OR_info with it.
            curr_node->push_back_regex_info(elem.m_regex_OR_range,
                                            REGEX_TYPE_OR);
          }
          
          curr_node->push_back_regex_info(
            /* start node */ elem.mp_node,
            /* end node */ mp_last_created_node_during_parsing,
            /* regex_OR_info */ 0,
            /* type */ regex_type);
          
          // finish this regular expression.
          regex_stack.pop_back();
        }
        else if (0 == str->compare(L"["))
        {
          // read a '['
          state = PARSING_STATE_CONTROL;
        }
        else if (0 == str->compare(L"{"))
        {
          state = PARSING_STATE_OPTION;
        }
        else if (0 == str->compare(L"*"))
        {
          update_rule_node_attribute_when_seeing_a_regex(this);
          
          assert(mp_last_created_node_during_parsing != 0);
          assert(mp_last_created_node_during_parsing->name().size() != 0);
          
          // This should be the inner most regex_info among
          // regexes which is started by this node. So that
          // I use 'push_front_regex_info' instead of
          // 'push_back_regex_info'.
          mp_last_created_node_during_parsing->push_front_regex_info(
            /* start node */ mp_last_created_node_during_parsing,
            /* end node */ mp_last_created_node_during_parsing,
            /* regex_OR_info */ 0,
            /* type */ REGEX_TYPE_ZERO_OR_MORE);
        }
        else if (0 == str->compare(L"+"))
        {
          update_rule_node_attribute_when_seeing_a_regex(this);
          
          assert(mp_last_created_node_during_parsing != 0);
          assert(mp_last_created_node_during_parsing->name().size() != 0);
          
          // This should be the inner most regex_info among
          // regexes which is started by this node. So that
          // I use 'push_front_regex_info' instead of
          // 'push_back_regex_info'.
          mp_last_created_node_during_parsing->push_front_regex_info(
            /* start node */ mp_last_created_node_during_parsing,
            /* end node */ mp_last_created_node_during_parsing,
            /* regex_OR_info */ 0,
            /* type */ REGEX_TYPE_ONE_OR_MORE);
        }
        else if (0 == str->compare(L"?"))
        {
          if (false == m_using_pure_BNF)
          {
            update_rule_node_attribute_when_seeing_a_regex(this);
            
            assert(mp_last_created_node_during_parsing != 0);
            assert(mp_last_created_node_during_parsing->name().size() != 0);
            
            // This should be the inner most regex_info among
            // regexes which is started by this node. So that
            // I use 'push_front_regex_info' instead of
            // 'push_back_regex_info'.
            mp_last_created_node_during_parsing->push_front_regex_info(
              /* start node */ mp_last_created_node_during_parsing,
              /* end node */ mp_last_created_node_during_parsing,
              /* regex_OR_info */ 0,
              /* type */ REGEX_TYPE_ZERO_OR_ONE);
          }
          else
          {
            // In pure BNF environment, '?' means
            // 'optional'.
            mp_last_created_node_during_parsing->set_optional(true);
          }
        }
        else if (0 == str->compare(L"\r"))
        {
        }
        else
        {
          throw ga_exception_t();
        }
      }
      break;
      
    case PARSING_STATE_OPTION:
      {
        bool option_value_start = false;
        PARSING_OPTION_CMD_ENUM option_cmd = PARSING_OPTION_CMD_NONE;
        
        for (;;)
        {
          boost::shared_ptr<std::wstring> option_str;
          
          try
          {
            option_str = lexer_get_grammar_string(option_state_delimiter);
            assert(option_str->size() != 0);
          }
          catch (Wcl::Lexerlib::EndOfSourceException &)
          {
            assert(0);
          }
          
          if (0 == option_str->compare(L"}"))
          {
            // Finish option parsing, check integrity.
            if (false == m_using_pure_BNF)
            {
              m_use_paull_algo_to_remove_left_recursion = false;
              m_enable_left_factor = false;
            }
            
            state = PARSING_STATE_NORMAL;
            break;
          }
          else if (0 == option_str->compare(L"k"))
          {
            option_cmd = PARSING_OPTION_CMD_K;
          }
          else if (0 == option_str->compare(L"use_paull_algo"))
          {
            option_cmd = PARSING_OPTION_CMD_USE_PAULL_ALGO;
          }
          else if (0 == option_str->compare(L"enable_left_factor"))
          {
            option_cmd = PARSING_OPTION_CMD_ENABLE_LEFT_FACTOR;
          }
          else if (0 == option_str->compare(L"using_pure_BNF"))
          {
            option_cmd = PARSING_OPTION_CMD_USING_PURE_BNF;
          }
          else if (0 == option_str->compare(L";"))
          {
            option_value_start = false;
            option_cmd = PARSING_OPTION_CMD_NONE;
          }
          else if (0 == option_str->compare(L"="))
          {
            assert(option_cmd != PARSING_OPTION_CMD_NONE);
            option_value_start = true;
          }
          else
          {
            assert(true == option_value_start);
            
            switch (option_cmd)
            {
            case PARSING_OPTION_CMD_K:
              try
              {
                m_max_lookahead_searching_depth = boost::lexical_cast<unsigned int>(*(option_str.get()));
              }
              catch (boost::bad_lexical_cast &e)
              {
                std::cout << e.what() << std::endl;
                exit(-1);
              }
              break;
              
            case PARSING_OPTION_CMD_USE_PAULL_ALGO:
              if (0 == option_str->compare(L"yes"))
              {
                m_use_paull_algo_to_remove_left_recursion = true;
              }
              else if (0 == option_str->compare(L"no"))
              {
                m_use_paull_algo_to_remove_left_recursion = false;
              }
              else
              {
                assert(0);
              }
              break;
              
            case PARSING_OPTION_CMD_ENABLE_LEFT_FACTOR:
              if (0 == option_str->compare(L"yes"))
              {
                m_enable_left_factor = true;
              }
              else if (0 == option_str->compare(L"no"))
              {
                m_enable_left_factor = false;
              }
              else
              {
                assert(0);
              }
              break;
              
            case PARSING_OPTION_CMD_USING_PURE_BNF:
              if (0 == option_str->compare(L"yes"))
              {
                m_using_pure_BNF = true;
              }
              else if (0 == option_str->compare(L"no"))
              {
                m_using_pure_BNF = false;
              }
              else
              {
                assert(0);
              }
              break;
              
            default:
              assert(0);
              break;
            }
          }
        }
      }
      break;
      
    default:
      assert(0);
      break;
    }
  } while (true == repeat);
  
  if (true == find_same_alternative_in_one_rule(last_rule_node()->next_nodes().back()))
  {
    log(L"\n<INFO>: duplicate production found.\n");
    throw ga_exception_t();
  }
  
  if (false == m_using_pure_BNF)
  {
    assert(false == m_next_token_is_regex_OR_start_node);
    
    // If 'm_using_pure_BNR' is 'false', then every rule
    // will be treated as a regex rule, so that I have to
    // update its attribute.
    update_rule_node_attribute_when_seeing_a_regex(this);      
    assert(last_rule_node()->main_regex_alternative() != 0);
    
    // I have to make sure all the regex_info for the last
    // rule head is closed.
    //
    // Ex:
    //
    // S
    //  : a b c
    //  | d e
    //
    // The regex_info in node 'a' is not closing at this
    // point, so I have to close it now.
    if (regex_stack.size() != 0)
    {
      assert(1 == regex_stack.size());
      
      assert(regex_stack.front().m_regex_OR_range.size() != 0);
      assert(0 == regex_stack.front().m_regex_OR_range.back().mp_end_node);
      
      regex_stack.front().m_regex_OR_range.back().mp_end_node = mp_last_created_node_during_parsing;
      
      regex_stack.front().mp_node->push_back_regex_info(
        regex_stack.front().m_regex_OR_range,
        REGEX_TYPE_OR);
      
      assert(last_rule_node()->main_regex_alternative() != 0);        
      
      regex_stack.front().mp_node->push_back_regex_info(
        last_rule_node()->main_regex_alternative(),
        mp_last_created_node_during_parsing,
        0,
        REGEX_TYPE_ONE);
      
      regex_stack.pop_back();
    }
  }
  
  if (false == m_using_pure_BNF)
  {
    // for the last alternative.
    build_new_alternative_for_regex_nodes(last_rule_node()->next_nodes().back());
    
    remove_same_alternative_for_one_rule(this, last_rule_node());
  }
  
  {
    std::vector<check_node_func> check_func;
    
    check_func.push_back(find_same_alternative_in_one_rule);
    
    if (true == m_using_pure_BNF)
    {
      build_new_alternative_for_optional_nodes(
        last_rule_node()->next_nodes().back(),
        check_func,
        false);
    }
  }
  
  assert(0 == regex_stack.size());
}

namespace
{
  bool
  set_node_non_optional(node_t * const node)
  {
    assert(node != 0);
    
    node->set_optional(false);
    
    return true;
  }
}

void
analyser_environment_t::check_and_fork_alternative_for_optional_node(
  node_t * const start_node,
  std::vector<check_node_func> const &check_func,
  bool const after_link_nonterminal)
{
  node_t *node = start_node;
  node_t *rule_node = start_node->rule_node();
  
#if defined(_DEBUG)
  if (node != rule_node->rule_end_node())
  {
    assert(1 == node->prev_nodes().size());
  }
#endif
  
  assert((1 == node->next_nodes().size()) ||
         (0 == node->next_nodes().size()));
  
  while (node != rule_node->rule_end_node())
  {
    if (true == node->is_optional())
    {
      std::list<node_t *> discard_nodes;
      
      discard_nodes.push_back(node);
      
      ::fork_alternative(start_node,
                         start_node->rule_node(),
                         start_node->rule_node()->rule_end_node(),
                         discard_nodes,
                         after_link_nonterminal);
      
      bool is_deleted = false;
      
      // The newly created production should be the last one.
      for (std::vector<check_node_func>::const_iterator iter = check_func.begin();
           iter != check_func.end();
           ++iter)
      {
        if (true == (*iter)(rule_node->next_nodes().back()))
        {
          delete_last_alternative(rule_node, after_link_nonterminal);
          
          is_deleted = true;
          
          break;
        }
      }
      
      if (false == is_deleted)
      {
        rule_node->next_nodes().back()->alternative_length() = 
          count_alternative_length(rule_node->next_nodes().back());
      }
    }
    
    assert(1 == node->next_nodes().size());
    node = node->next_nodes().front();
  }
  
  // mark all nodes of this production non optional.
  if (start_node->name().size() != 0)
  {
    (void)node_for_each(start_node,
                        0,
                        set_node_non_optional);
  }
}

void
analyser_environment_t::build_new_alternative_for_optional_nodes(
  node_t * const alternative_start,
  std::vector<check_node_func> const &check_func,
  bool const after_link_nonterminal)
{
  node_t * const rule_node = alternative_start->rule_node();
  std::list<node_t *>::const_iterator iter = rule_node->next_nodes().end();
  --iter;
  
  check_and_fork_alternative_for_optional_node(
    alternative_start,
    check_func,
    after_link_nonterminal);
  
  ++iter;
  
  // check newly created productions.
  for (; iter != rule_node->next_nodes().end(); ++iter)
  {
    check_and_fork_alternative_for_optional_node(*iter,
                                                 check_func,
                                                 after_link_nonterminal);
  }
}

void
analyser_environment_t::delete_last_alternative(
  node_t * const rule_node,
  bool const after_link_nonterminal)
{
  node_t * const start_node = rule_node->next_nodes().back();
  assert(start_node != 0);
  
  rule_node->break_append_node(start_node);
  
  if (start_node == rule_node->rule_end_node())
  {
    start_node->break_prepend_node(rule_node);
  }
  else
  {
    node_t *node = start_node;
    node_t *next_node = node->next_nodes().front();
    assert(next_node != 0);
    
    while (next_node != rule_node->rule_end_node())
    {
      assert(1 == node->next_nodes().size());
      assert(1 == node->prev_nodes().size());
      
      if (false == node->is_terminal())
      {
        if (true == after_link_nonterminal)
        {
          assert(node->nonterminal_rule_node() != 0);
          node->nonterminal_rule_node()->remove_refer_to_me_node(node);
        }
        else
        {
          assert(0 == node->nonterminal_rule_node());
        }
      }
      
      boost::checked_delete(node);
      
      node = next_node;
      next_node = node->next_nodes().front();
    }
    
    rule_node->rule_end_node()->break_prepend_node(node);
    if (false == node->is_terminal())
    {
      if (true == after_link_nonterminal)
      {
        assert(node->nonterminal_rule_node() != 0);
        node->nonterminal_rule_node()->remove_refer_to_me_node(node);
      }
      else
      {
        assert(0 == node->nonterminal_rule_node());
      }
    }
    boost::checked_delete(node);
  }
}

/// Apply \p traverse_node_func (if not 0) on each node,
/// including rule node, rule end node, and every nodes in
/// each alternatives, and then apply \p traverse_rule_func
/// (if not 0) to each rule node.
///
/// \param traverse_node_func 
/// \param traverse_rule_func 
/// \param param 
///
/// \return
/// - true if all nodes are applied and without failures
/// - fail if one failure is happened at some node, and this
/// function returns immediately.
///
bool
analyser_environment_t::traverse_all_nodes(
  bool (*traverse_node_func)(analyser_environment_t const * const,
                             node_t * const,
                             void *param),
  bool (*traverse_rule_func)(analyser_environment_t const * const,
                             node_t * const,
                             void *param),
  void * const param) const
{
  BOOST_FOREACH(node_t * const rule_node, m_top_level_nodes)
  {
    if (traverse_node_func != 0)
    {
      // traverse rule node & rule end node.
      if (false == traverse_node_func(this, rule_node, param))
      {
        return false;
      }
      
      if (false == traverse_node_func(this, rule_node->rule_end_node(), param))
      {
        return false;
      }
    }
    
    // travese nodes in each alternative
    BOOST_FOREACH(node_t * const alternative_start, rule_node->next_nodes())
    {
      node_t *node = alternative_start;
      
      while (node != rule_node->rule_end_node())
      {
        assert(1 == node->prev_nodes().size());
        assert(1 == node->next_nodes().size());
        
        if (traverse_node_func != 0)
        {
          if (false == traverse_node_func(this, node, param))
          {
            return false;
          }
        }
        
        node = node->next_nodes().front();
      }
    }
    
    // Apply traverse_rule_func on rule node.
    if (traverse_rule_func != 0)
    {
      if (false == traverse_rule_func(this, rule_node, param))
      {
        return false;
      }
    }
  }
  
  return true;
}

void
analyser_environment_t::close_output_file()
{
  if (mp_output_file != 0)
  {
    mp_output_file->close();
    boost::checked_delete(mp_output_file);
  }
}

void
analyser_environment_t::log(wchar_t const * const fmt, ...) const
{
  va_list ap;
  boost::shared_ptr<wchar_t> str;
  
  va_start(ap, fmt);
  str.reset(fmtstr_new_valist(fmt, &ap), fmtstr_delete);
  va_end(ap);
  
  if (0 == mp_output_file)
  {
    std::wcerr << str.get() << std::flush;
  }
  else
  {
    assert(true == mp_output_file->is_open());
    (*mp_output_file) << str.get() << std::flush;
    
    if ((mp_output_file->tellp() * sizeof(wchar_t)) > 50 * 1024 * 1024)
    {
      // log file too large, create a new one.
      mp_output_file->close();
      ++m_curr_output_filename_idx;
      wchar_t *  new_filename = fmtstr_new(L"%s%d",
                                                m_output_filename.c_str(),
                                                m_curr_output_filename_idx);
      mp_output_file->open(new_filename, std::ios_base::out | std::ios_base::binary);
      fmtstr_delete(new_filename);
      if (false == mp_output_file->is_open())
      {
        assert(0);
        return;
      }
    }
  }
}

namespace
{
  bool
  mark_number_for_one_node(
    analyser_environment_t const * const /* ae */,
    node_t * const node,
    void * const param)
  {
    unsigned int *idx = reinterpret_cast<unsigned int *>(param);
    
    node->overall_idx() = (*idx);
    ++(*idx);
    
    return true;
  }
}

void
analyser_environment_t::mark_number_for_all_nodes() const
{
  unsigned int idx = 0;
  
  traverse_all_nodes(mark_number_for_one_node, 0, &idx);
}

void
analyser_environment_t::remove_duplicated_alternatives_for_all_rule()
{
  //mark_number_for_all_nodes();
  
  for (std::list<node_t *>::iterator iter = m_top_level_nodes.begin();
       iter != m_top_level_nodes.end();
       ++iter)
  {
    ::remove_duplicated_alternatives_for_one_rule(this, *iter);
  }
}

namespace
{
  bool
  check_nonterminal_rule_node_field_is_set(
    analyser_environment_t const * const /* ae */,
    node_t * const node,
    void * const /* param */)
  {
    assert(node != 0);
    
    if ((false == node->is_rule_head()) &&
        (node->name().size() != 0) &&
        (false == node->is_terminal()))
    {
      assert(node->nonterminal_rule_node() != 0);
      assert(0 == node->name().compare(node->nonterminal_rule_node()->name()));
    }
    
    return true;
  }
}

void
analyser_environment_t::check_nonterminal_linking() const
{
#if defined(_DEBUG)
  for (std::list<node_t *>::const_iterator iter = m_top_level_nodes.begin();
       iter != m_top_level_nodes.end();
       ++iter)
  {
    if (0 == (*iter)->refer_to_me_nodes().size())
    {
      if (false == (*iter)->is_starting_rule())
      {
        log(L"<WARN>: %s has no nonterminal refer to it.\n",
            (*iter)->name().c_str());
      }
    }
    else
    {
      for (std::list<node_t *>::const_iterator iter_i =
             (*iter)->refer_to_me_nodes().begin();
           iter_i != (*iter)->refer_to_me_nodes().end();
           ++iter_i)
      {
        assert((*iter_i)->nonterminal_rule_node() == (*iter));
        assert(0 == (*iter_i)->name().compare((*iter)->name()));
      }
    }
  }
  
  assert(true == traverse_all_nodes(
           check_nonterminal_rule_node_field_is_set,
           0,
           0));
#endif
}

void
analyser_environment_t::determine_node_position()
{
  for (std::list<node_t *>::iterator iter = m_top_level_nodes.begin();
       iter != m_top_level_nodes.end();
       ++iter)
  {
    for (std::list<node_t *>::iterator iter2 = (*iter)->next_nodes().begin();
         iter2 != (*iter)->next_nodes().end();
         ++iter2)
    {
      if ((*iter2)->name().size() != 0)
      {
        unsigned int count_from_rule_head = 1;
        unsigned int count_to_rule_end_node =
          (*iter2)->count_node_number_between_this_node_and_rule_end_node();
        
        (*iter2)->set_distance_from_rule_head(count_from_rule_head);
        (*iter2)->set_distance_to_rule_end_node(count_to_rule_end_node);
        
        assert(1 == (*iter2)->next_nodes().size());
        
        node_t *curr_node = (*iter2)->next_nodes().front();
        
        while (curr_node->name().size() != 0)
        {
          curr_node->set_distance_from_rule_head(++count_from_rule_head);
          curr_node->set_distance_to_rule_end_node(--count_to_rule_end_node);
          
          assert(1 == curr_node->next_nodes().size());
          curr_node = curr_node->next_nodes().front();
        }
      }
    }
  }
}

void
analyser_environment_t::find_eof() const
{
  for (std::list<node_t *>::const_iterator iter = m_top_level_nodes.begin();
       iter != m_top_level_nodes.end();
       ++iter)
  {
    assert((*iter)->rule_end_node() != 0);
    
    std::list<node_t *> last_symbol_chain;
    /* To indicate that I am in a situation that I want to
     * find what symbols can connect after this one.
     */
    last_symbol_chain.push_back(*iter);
    
    if (true == check_eof_condition((*iter)->rule_end_node(),
                                    last_symbol_chain))
    {
      (*iter)->rule_end_node()->set_eof();
    }
  }
}

bool
analyser_environment_t::check_eof_condition(
  node_t const * const node,
  std::list<node_t *> &last_symbol_chain) const
{
  for (std::list<node_t *>::const_iterator iter_refer_to_me =
         node->rule_node()->refer_to_me_nodes().begin();
       iter_refer_to_me !=
         node->rule_node()->refer_to_me_nodes().end();
       ++iter_refer_to_me)
  {
    assert(1 == (*iter_refer_to_me)->next_nodes().size());
    
    if ((*iter_refer_to_me)->next_nodes().front()->name().size() != 0)
    {
      /* There exist at least one terminal after this
       * nonterminal, thus it can not be an EOF condition.
       */
      return false;
    }
    else
    {
      /* This 'refer_to_me_node' is still the last symbol
       * in his alternative, thus I have the chance to
       * determine whether it is an EOF situation or not.
       */
      bool find_in_last_symbol_chain = false;
      
      for (std::list<node_t *>::const_iterator iter =
             last_symbol_chain.begin();
           iter != last_symbol_chain.end();
           ++iter)
      {
        assert(true == (*iter)->is_rule_head());
        
        if ((*iter) == (*iter_refer_to_me)->rule_node())
        {
          find_in_last_symbol_chain = true;
          break;
        }
      }
      
      if (true == find_in_last_symbol_chain)
      {
        /* If I have already in the situation of finding
         * what can connect to
         * (*iter_refer_to_me)->rule_node(), then I will
         * skip this this time.
         */
        continue;
      }
      else
      {
        last_symbol_chain.push_back((*iter_refer_to_me)->rule_node());
        
        if (false == check_eof_condition(*iter_refer_to_me, last_symbol_chain))
        {
          return false;
        }
      }
    }
  }
  
  return true;
}
