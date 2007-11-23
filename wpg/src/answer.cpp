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

#if defined(_DEBUG)

#include "ae.hpp"
#include "node.hpp"
#include "lexer.hpp"

#include "wcl_memory_debugger\memory_debugger.h"

enum parse_answer_state_t
{
  PARSE_ANSWER_STATE_NONTERMINAL,
  PARSE_ANSWER_STATE_LOOKAHEAD
};
typedef enum parse_answer_state_t parse_answer_state_t;

void
analyser_environment_t::parse_answer_lookahead(node_t * const node,
                                               std::wstring const &src_str)
{
  assert(src_str.size() != 0);
  
  std::list<wchar_t> delimiter;
  
  delimiter.push_back(L';');
  delimiter.push_back(L',');
  
  lookahead_set_t *curr_lookahead_set;
  lookahead_set_t *result_lookahead_set = 0;
  
  curr_lookahead_set = &(node->lookahead_set());
  assert(curr_lookahead_set != 0);
  
  std::wstring src_str_i = src_str;
  
  do
  {
    boost::shared_ptr<std::wstring> str(
      ::lexer_get_answer_string<std::wstring>(src_str_i, delimiter));
    
    assert(str->size() != 0);
    if (0 == str->compare(L"EOF"))
    {
      if (src_str_i.size() != 0)
      {
        log(L"<ERROR>: answer file error, incorrect lookahead format: %s\n",
            src_str.c_str());
        throw ga_exception_t();
      }
	  break;
    }
    
    if (0 == str->compare(L","))
    {
      continue;
    }
    else if (0 == str->compare(L";"))
    {
      assert(result_lookahead_set != 0);
      curr_lookahead_set = result_lookahead_set;
      continue;
    }
    else
    {
      node_t *node;
      
      if (0 == str->compare(L"...EOF..."))
      {
        /// use rule end node to represent EOF.
        node = new node_t(this, 0);
      }
      else
      {
        node = new node_t(this, 0, *(str.get()));
      }
      
      assert(node != 0);
      m_answer_nodes.push_back(node);
      result_lookahead_set = curr_lookahead_set->find_lookahead(node);
      
      if (0 == result_lookahead_set)
      {
        result_lookahead_set =
          curr_lookahead_set->insert_lookahead(node);
      }
    }
  } while(1);
}

/// 
///
/// @param str 
/// @param node 
/// @param parse_answer_state
/// @param latest_char_is_linefeed
///
/// @return 
/// - true: The value stored in the second parameter (i.e. *node) is the new
/// nonterminal node.
/// - false: Otherwise.
///
bool
analyser_environment_t::parse_answer(std::wstring const &str,
                                     node_t **node,
                                     parse_answer_state_t *parse_answer_state,
                                     bool *latest_char_is_linefeed)
{
  assert(str.size() != 0);
  
  switch (*parse_answer_state)
  {
  case PARSE_ANSWER_STATE_NONTERMINAL:
    if (0 == str.compare(L"\n"))
    {
      *latest_char_is_linefeed = true;
      *parse_answer_state = PARSE_ANSWER_STATE_LOOKAHEAD;
    }
    else
	{
      *latest_char_is_linefeed = false;
      assert(str.at(str.size() - 1) == L':');
      assert(str.at(str.size() - 2) == L']');
      std::wstring::size_type const pos = str.rfind(L'[', str.size() - 1);
      assert(pos != std::wstring::npos);
      
      node_t *new_node;
      
      if (0 == pos)
      {
        ///
        /// a[4] means a normal node whose dot number is 4.
        /// [4] means a rule end node whose dot number is 4.
        ///
        new_node = new node_t(this, 0);
      }
      else
      {
        new_node = new node_t(this, 0, str.substr(0, pos));
      }
      assert(new_node != 0);
      m_answer_nodes.push_back(new_node);
      
      std::wstringstream ss;
      unsigned int overall_idx;
      ss.clear();
      ss.str(L"");
      ss << str.substr(pos + 1, str.size() - pos - 2);
      ss >> overall_idx;
      
      new_node->overall_idx() = overall_idx;
      *node = new_node;
      
      return true;
	}
    break;
    
  case PARSE_ANSWER_STATE_LOOKAHEAD:
    assert(node != 0);
    assert((*node) != 0);
    if (0 == str.compare(L"\n"))
    {
      if (true == *latest_char_is_linefeed)
      {
        *parse_answer_state = PARSE_ANSWER_STATE_NONTERMINAL;
      }
      else
      {
        *latest_char_is_linefeed = true;
      }
    }
    else
    {
      *latest_char_is_linefeed = false;
      parse_answer_lookahead(*node, str);
    }
    break;
    
  default:
    assert(0);
    break;
  }
  
  return false;
}

bool
analyser_environment_t::parse_answer_file(std::wfstream * const answer_file)
{
  assert(answer_file != 0);
  assert(true == answer_file->is_open());
  
  bool repeat;
  node_t *node;
  parse_answer_state_t state = PARSE_ANSWER_STATE_NONTERMINAL;
  bool latest_char_is_linefeed;
  
  std::list<wchar_t> delimiter;
  
  delimiter.push_back(L'\n');
  delimiter.push_back(L'\r');
  
  do
  {
    boost::shared_ptr<std::wstring> str(
      ::lexer_get_answer_string<std::wfstream>(*answer_file, delimiter));
    
    assert(str->size() != 0);
    if (0 == str->compare(L"EOF"))
    {
      repeat = false;
    }
    else
    {
      if (str->compare(L"\r") != 0)
      {
        if (true == parse_answer(*(str.get()),
                                 &node,
                                 &state,
                                 &latest_char_is_linefeed))
        {
          hash_answer(node);
        }
      }
      repeat = true;
    }
  } while (true == repeat);
  
  return true;
}

bool
analyser_environment_t::read_answer_file()
{
  std::wstring::size_type loc;
  
  loc = m_grammar_file_name.rfind(L".gra", m_grammar_file_name.size());
  assert(loc != std::wstring::npos);
  
  m_grammar_file_name.replace(loc, 4, L".ans");
  
  boost::scoped_ptr<std::wfstream> mp_answer_file(
    new std::wfstream(m_grammar_file_name.c_str(),
                      std::ios_base::in |
                      std::ios_base::binary));
  
  if (false == mp_answer_file->is_open())
  {
    return false;
  }
  
  try
  {
    parse_answer_file(mp_answer_file.get());
  }
  catch (ga_exception_t const &)
  {
    return false;
  }
  
  return true;
}

bool
analyser_environment_t::perform_answer_comparison_for_each_node(
  node_t * const node_a,
  node_t * const node_b) const
{
  node_t * const answer_node_a = find_answer(node_a);
  if (0 == answer_node_a)
  {
    return false;
  }
  node_t * const answer_node_b = find_answer(node_b);
  if (0 == answer_node_b)
  {
    return false;
  }
  
  if (false == node_a->compare_lookahead_set(answer_node_a))
  {
    log(L"<ERROR>: lookahead set for %s[%d] is wrong.\n",
        node_a->name().c_str(),
        node_a->overall_idx());
    return false;
  }
  if (false == node_b->compare_lookahead_set(answer_node_b))
  {
    log(L"<ERROR>: lookahead set for %s[%d] is wrong.\n",
        node_b->name().c_str(),
        node_b->overall_idx());
    return false;
  }
  
  return true;
}

bool
analyser_environment_t::perform_answer_comparison() const
{
  for (std::list<node_t *>::const_iterator iter = m_top_level_nodes.begin();
       iter != m_top_level_nodes.end();
       ++iter)
  {
    ///
    /// If there are more than 1 alternative node of this rule node,
    /// then I will look for lookahead terminals.
    /// Otherwise, I don't need to do this because there is only one way to
    /// derive.
    ///
    if ((*iter)->next_nodes().size() > 1)
    {
      std::list<todo_nodes_t> todo_nodes_set;
      
      (*iter)->calculate_lookahead_waiting_pool_for_pure_BNF_rule(todo_nodes_set);
      mark_number_for_all_nodes();
      
      while (todo_nodes_set.size() != 0)
      {
        /// I will find lookahead terminals for these 2 nodes now.
        todo_nodes_t &todo_nodes = todo_nodes_set.front();
        
        bool result = perform_answer_comparison_for_each_node(
          todo_nodes.mp_node_a,
          todo_nodes.mp_node_b);
        
        if (false == result)
        {
          return false;
        }
        
        todo_nodes_set.pop_front();
      }
    }
  }
  return true;
}

#endif // defined(_DEBUG)
