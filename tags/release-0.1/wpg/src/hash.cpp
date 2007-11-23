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
#include "hash.hpp"
#include "node.hpp"
#include "ga_exception.hpp"

#include "wcl_memory_debugger\memory_debugger.h"

node_t *
analyser_environment_t::nonterminal_rule_node(std::wstring const &str) const
{
  typedef rule_head_hash_table_t::index<node_name>::type node_by_name;
  
  node_by_name::iterator iter = m_rule_head_hash_table.get<node_name>().find(str);
  if (iter == m_rule_head_hash_table.get<node_name>().end())
  {
    return 0;
  }
  else
  {
    return (*iter);
  }
}

void
analyser_environment_t::hash_rule_head(node_t * const node)
{
  assert(node != 0);
  assert(node->name().size() != 0);
  
  typedef rule_head_hash_table_t::index<node_name>::type node_by_name;
  
  std::pair<node_by_name::iterator, bool> result = 
    m_rule_head_hash_table.insert(node);
  assert(true == result.second);
}

void
analyser_environment_t::hash_answer(node_t * const node)
{
  assert(node != 0);
  
  typedef answer_node_hash_table_t::index<node_overall_idx>::type node_by_overall_idx;
  
  std::pair<node_by_overall_idx::iterator, bool> result =
    m_answer_node_hash_table.insert(node);
  assert(true == result.second);
}

node_t *
analyser_environment_t::find_answer(node_t * const node) const
{
  typedef answer_node_hash_table_t::index<node_overall_idx>::type
    node_by_overall_idx;
  
  node_by_overall_idx::iterator iter =
    m_answer_node_hash_table.get<node_overall_idx>().find(node->overall_idx());
  if (iter == m_answer_node_hash_table.get<node_overall_idx>().end())
  {
    return 0;
  }
  else
  {
    return (*iter);
  }
}

void
analyser_environment_t::remove_rule_head(node_t * const node)
{
  assert(node != 0);
  assert(node->name().size() != 0);
  
  typedef rule_head_hash_table_t::index<node_name>::type node_by_name;
  
  node_by_name::size_type result = 
    m_rule_head_hash_table.erase(node->name());
  assert(1 == result);
}

bool
analyser_environment_t::is_terminal(std::wstring const &str) const
{
  typedef terminal_hash_table_t::index<terminal_name>::type terminal_by_name;
  
  terminal_by_name::iterator iter =
    m_terminal_hash_table.get<terminal_name>().find(str);
  
  if (iter == m_terminal_hash_table.get<terminal_name>().end())
  {
    return false;
  }
  else
  {
    return true;
  }
}

void
analyser_environment_t::hash_terminal(std::wstring const &str)
{
  typedef terminal_hash_table_t::index<terminal_name>::type terminal_by_name;
  
  std::pair<terminal_by_name::iterator, bool> result =
    m_terminal_hash_table.insert(str);
  
  if (false == result.second)
  {
    log(L"<ERROR>: duplicate terminal symbol: %s\n", str.c_str());
    throw ga_exception_t();
  }
}
