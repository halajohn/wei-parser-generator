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

#include "wcl_memory_debugger\memory_debugger.h"

namespace
{
  std::wstring
  create_dot_name(node_t const * const node)
  {
    std::wstringstream ss;
    std::wstring str;
  
    ss.clear();
    ss.str(L"");
  
    if (node->rule_node()->rule_end_node() == node)
    {
      ss << node->rule_node()->name();
      ss << L"END";
    }
    else
    {
      ss << node->name();
    }
    ss << node->overall_idx();
  
    ss >> str;
  
    return str;
  }
  
  void
  create_dot_node_line(std::wfstream &fp,
                       node_t * const node)
  {
    assert(false == node->create_dot_node_line());
    std::wstring const str = create_dot_name(node);
  
    if (true == node->is_terminal())
    {
      /// terminal
      std::wstring fmt_str =
        L"%s [label=\"%s\", fillcolor=lightseagreen, style=filled";
      if (node->is_ambigious())
      {
        fmt_str.append(L", shape=tripleoctagon");
      }
      fmt_str.append(L"];\n");
      ::report(&fp, fmt_str.c_str(),
               str.c_str(), node->name().c_str());
    }
    else
    {
      std::wstring fmt_str = L"%s [label=\"%s\"";
      if (node->is_rule_head())
      {
        /// rule node
        fmt_str.append(L", shape=doublecircle");
        if (node->is_nullable())
        {
          /// nullable rule node
          fmt_str.append(L", fillcolor=red");
        }
        else
        {
          /// non-nullable rule node
          fmt_str.append(L", fillcolor=lightblue");
        }
        if (node->is_cyclic())
        {
          /// cyclic rule node
          fmt_str.append(L", color=orange");
        }
        fmt_str.append(L", style=filled");
      }
      else if (node == node->rule_node()->rule_end_node())
      {
        /// rule end node
        fmt_str.append(L", shape=doublecircle");
        fmt_str.append(L", fillcolor=blue, style=filled");
      }
      else
      {
        /// normal non-terminal
        if (node->is_ambigious())
        {
          fmt_str.append(L", shape=tripleoctagon");
        }
        else
        {
          fmt_str.append(L", shape=box");
        }
        fmt_str.append(L", fillcolor=green, style=filled");
      }
      fmt_str.append(L"];\n");
      ::report(&fp, fmt_str.c_str(), str.c_str(), node->name().c_str());
    }
    node->set_create_dot_node_line(true);
    node->rule_node()->add_same_rule_node(node);
  }

  void
  create_dot_relation_line(std::wfstream &fp,
                           node_t const * const node1,
                           node_t const * const node2,
                           node_rel_t const rel)
  {
    assert(true == node1->create_dot_node_line());
    assert(true == node2->create_dot_node_line());
  
    std::wstring fmt_str = L"%s -> %s";
    fmt_str.append(L" [");
    switch (rel)
    {
    case NODE_REL_NORMAL:
      fmt_str.append(L"color=purple");
      break;
    case NODE_REL_POINT_TO_RULE_HEAD:
      assert(node1->nonterminal_rule_node() == node2);
      fmt_str.append(L"style=dashed color=red");
      break;
    case NODE_REL_RULE_HEAD_REFER_TO_NODE:
      assert(node1->is_refer_to_me_node(node2));
      fmt_str.append(L"style=dashed color=blue");
      break;
    default:
      assert(0);
      break;
    }
    fmt_str.append(L"];\n");
    ::report(&fp, fmt_str.c_str(),
             create_dot_name(node1).c_str(),
             create_dot_name(node2).c_str());
  }
  
  bool
  dump_nodes(analyser_environment_t const * const /* ae */,
             node_t * const node,
             void * const param)
  {
    std::wfstream &fp = *(reinterpret_cast<std::wfstream *>(param));
  
    if (false == node->create_dot_node_line())
    {
      create_dot_node_line(fp, node);
    }
  
    for (std::list<node_t *>::const_iterator iter = node->next_nodes().begin();
         iter != node->next_nodes().end();
         ++iter)
    {
      if (false == (*iter)->create_dot_node_line())
      {
        create_dot_node_line(fp, *iter);
      }
      create_dot_relation_line(fp, node, *iter, NODE_REL_NORMAL);
      ::report(&fp, L"\n");
    }
  
    if (node->is_rule_head())
    {
      for (std::list<node_t *>::const_iterator iter = node->refer_to_me_nodes().begin();
           iter != node->refer_to_me_nodes().end();
           ++iter)
      {
        if (false == (*iter)->create_dot_node_line())
        {
          create_dot_node_line(fp, *iter);
        }
        create_dot_relation_line(fp, node, *iter,
                                 NODE_REL_RULE_HEAD_REFER_TO_NODE);
      }
    }
    else
    {
      if (node->nonterminal_rule_node() != NULL)
      {
        if (false == node->nonterminal_rule_node()->create_dot_node_line())
        {
          create_dot_node_line(fp, node->nonterminal_rule_node());
        }
        create_dot_relation_line(fp, node,
                                 node->nonterminal_rule_node(),
                                 NODE_REL_POINT_TO_RULE_HEAD);
      }
    }
    return true;
  }

  bool
  dump_rules(analyser_environment_t const * const /* ae */,
             node_t * const rule_node,
             void * const param)
  {
    std::wfstream &fp = *(reinterpret_cast<std::wfstream *>(param));
  
    assert(rule_node->is_rule_head());
  
    std::wstringstream ss;
    std::wstring overall_idx_str;
    ss.clear();
    ss.str(L"");
  
    ss << rule_node->overall_idx();
    ss >> overall_idx_str;
  
    ::report(&fp, L"subgraph cluster%s {\n", overall_idx_str.c_str());
  
    for (std::list<node_t *>::const_iterator iter = rule_node->same_rule_nodes().begin();
         iter != rule_node->same_rule_nodes().end();
         ++iter)
    {
      assert((*iter)->rule_node() == rule_node);
      ::report(&fp, L"\"%s\";\n", create_dot_name(*iter).c_str());
    }
    ::report(&fp, L"}\n");
    return true;
  }
}

void
analyser_environment_t::dump_tree(std::wstring const &filename)
{
  assert(false == filename.empty());
  
  std::wfstream fp;
  fp.open(filename.c_str(), std::ios_base::out | std::ios_base::binary);
  assert(true == fp.is_open());
  
  mark_number_for_all_nodes();
  
  ::report(&fp, L"digraph grammar_tree\n");
  ::report(&fp, L"{\n");
  ::report(&fp, L"rankdir=LR;\n");
  
  traverse_all_nodes(clear_node_state, 0, 0);
  traverse_all_nodes(dump_nodes, dump_rules, &fp);
  
  ::report(&fp, L"{\n");
  ::report(&fp, L"rank=same;\n");
  for (std::list<node_t *>::const_iterator iter = m_top_level_nodes.begin();
       iter != m_top_level_nodes.end();
       ++iter)
  {
    ::report(&fp, L"\"%s\";", create_dot_name(*iter).c_str());
  }
  ::report(&fp, L"}\n");
  ::report(&fp, L"}\n");
  
  fp.close();
  
  if (false == filename.empty())
  {
    STARTUPINFO startupInfo;
    PROCESS_INFORMATION procInfo;
    
    FillMemory(&startupInfo, sizeof(startupInfo), 0);
    startupInfo.cb = sizeof(startupInfo);
    ZeroMemory(&procInfo, sizeof(PROCESS_INFORMATION));
    
    std::wstring png_filename = filename;
    std::wstring::size_type dot_substring_idx = png_filename.rfind(L".dot", png_filename.size());
    png_filename.replace(dot_substring_idx, 4, L".png");
    wchar_t *command_line = fmtstr_new(L"dot -q1 -Tpng -o %s %s",
                                       png_filename.c_str(),
                                       filename.c_str());
    BOOL const success = CreateProcess(NULL, (LPWSTR)command_line, NULL, NULL, FALSE,
                                       NORMAL_PRIORITY_CLASS,
                                       NULL, NULL, &startupInfo, &procInfo);
    fmtstr_delete(command_line);
    if (FALSE == success)
    {
      LPVOID lpMsgBuf;
      FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                    0,
                    GetLastError(),
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /// Default language
                    (LPTSTR)&lpMsgBuf,
                    0,
                    0
                    );
      MessageBox(NULL, (LPCTSTR)lpMsgBuf, 0, MB_OK | MB_ICONERROR);
      LocalFree(lpMsgBuf);
      CloseHandle(procInfo.hProcess);
      CloseHandle(procInfo.hThread);
      exit(1);
    }
    
    DWORD dwExitCode;
    
    /// wait for the 'dot' finished.
    WaitForSingleObject(procInfo.hProcess, INFINITE);
    GetExitCodeProcess(procInfo.hProcess, &dwExitCode);
  }
  
  traverse_all_nodes(clear_node_state, 0, 0);
  for (std::list<node_t *>::const_iterator iter = m_top_level_nodes.begin();
       iter != m_top_level_nodes.end();
       ++iter)
  {
    if ((false == (*iter)->traversed()) &&
        (true == (*iter)->is_cyclic()))
    {
      log(L"<INFO>: cyclic set: ");
      for (std::list<node_t *>::const_iterator iter2 = (*iter)->cyclic_set().begin();
           iter2 != (*iter)->cyclic_set().end();
           ++iter2)
      {
        assert((*iter2)->is_rule_head());
        (*iter2)->set_traversed(true);
        log(L"%s ",
               (*iter2)->name().c_str());
      }
      log(L"\n");
    }
  }
  
  fp.close();
}

namespace
{
  void
  construct_path_and_dump(std::wfstream &fp,
                          std::vector<node_t *> &path,
                          std::list<lookahead_set_t> const &lookahead_set)
  {
#if defined(_DEBUG)
    std::vector<node_t *>::size_type const level = path.size();
#endif
    
    for (std::list<lookahead_set_t>::const_iterator iter = lookahead_set.begin();
         iter != lookahead_set.end();
         ++iter)
    {
      path.push_back((*iter).mp_node);
      if ((*iter).m_next_level.size() != 0)
      {
        construct_path_and_dump(fp, path, (*iter).m_next_level);
      }
      else
      {
        bool first = true;
        
        /// This is one path with depth 2:
        ///   a,b
        ///
        /// This is one path with depth 3:
        ///   a,b,c
        ///
        /// This is two paths with depth 3:
        ///   a,b,c;a,b,d
        ///
        for (std::vector<node_t *>::const_iterator iter2 = path.begin();
             iter2 != path.end();
             ++iter2)
        {
          if (true == first)
          {
            first = false;
          }
          else
          {
            ::report(&fp, L",");
          }
          
          if ((*iter2)->name().size() != 0)
          {
            ::report(&fp, L"%s", (*iter2)->name().c_str());
          }
          else
          {
            ::report(&fp, L"%s", L"EOF");
          }
        }
        ::report(&fp, L";");
      }
      assert(path.back() == (*iter).mp_node);
      path.pop_back();
    }
    
#if defined(_DEBUG)
    assert(level == path.size());
#endif
  }
  
  void
  dump_lookahead_set(std::wfstream &fp, node_t const * const node)
  {
    for (std::list<node_t *>::const_iterator iter = node->ambigious_set().begin();
         iter != node->ambigious_set().end();
         ++iter)
    {
      std::wstring str;
      
      if ((*iter)->name().size() != 0)
      {
        str = (*iter)->name();
      }
      else
      {
        assert((*iter)->rule_node() != 0);
        str = (*iter)->rule_node()->name();
        str += L"_EOF";
      }
      ::report(&fp, L"[ambigious to %s[%d]]",
               str.c_str(),
               (*iter)->overall_idx());
    }
    
    if (node->lookahead_set().m_next_level.size() != 0)
    {
      ::report(&fp, L"[");
      std::vector<node_t *> path;
      construct_path_and_dump(fp, path, node->lookahead_set().m_next_level);
      ::report(&fp, L"]");
    }
  }
}

void
analyser_environment_t::dump_grammar_alternative(std::wfstream &fp,
                                                 node_t * const node) const
{
  if (node->name().size() != 0)
  {
    BOOST_FOREACH(regex_info_t const &range,
                  node->regex_info())
    {
      if (range.m_ranges.front().mp_start_node == node)
      {
        ::report(&fp, L"(");
      }
    }
    
    ::report(&fp, L"%s[%d]", node->name().c_str(), node->overall_idx());
    
    if (true == node->is_in_regex_OR_group())
    {
      ::report(&fp, L".");
    }
    
    BOOST_FOREACH(regex_info_t const &range,
                  node->regex_info())
    {
      if (range.m_ranges.front().mp_end_node == node)
      {
        ::report(&fp, L")");
        
        switch (range.m_type)
        {
        case REGEX_TYPE_ZERO_OR_MORE:
          ::report(&fp, L"*");
          break;
          
        case REGEX_TYPE_ZERO_OR_ONE:
          ::report(&fp, L"?");
          break;
          
        case REGEX_TYPE_ONE_OR_MORE:
          ::report(&fp, L"+");
          break;
          
        case REGEX_TYPE_ONE:
          break;
          
        default:
          assert(0);
          break;
        }
      }
    }
  }
  else
  {
    assert(0 == node->regex_info().size());
    assert(0 == node->tmp_regex_info().size());
    
    return;
  }
  
  dump_lookahead_set(fp, node);
  
  ::report(&fp, L" ");
  if (node->name().size() != 0)
  {
    assert(1 == node->next_nodes().size());
    dump_grammar_alternative(fp, node->next_nodes().front());
  }
  else
  {
    assert(0 == node->next_nodes().size());
  }
}

void
analyser_environment_t::dump_grammar_rule(std::wfstream &fp, node_t * const node) const
{
  assert(node->is_rule_head());
  
  std::map<node_t *, int> alternative_start_map;
  int i = 0;
  
  BOOST_FOREACH(node_t *alternative_start, node->next_nodes())
  {
    alternative_start_map[alternative_start] = i;
    
    ++i;
  }

  for (std::list<node_t *>::const_iterator iter = node->next_nodes().begin();
       iter != node->next_nodes().end();
       ++iter)
  {
    ::report(&fp, L"  %d) ",
             alternative_start_map[(*iter)]);
    
    if (true == (*iter)->name().empty())
    {
      ::report(&fp, L"...epsilon...");
    }
    else
    {
      dump_grammar_alternative(fp, *iter);
    }
    ::report(&fp, L"\n");
  }
}

void
analyser_environment_t::dump_grammar(std::wstring const &filename) const
{
  assert(false == filename.empty());
  
  std::wfstream fp;
  fp.open(filename.c_str(), std::ios_base::out | std::ios_base::binary);
  assert(true == fp.is_open());
  
  mark_number_for_all_nodes();
  
  for (std::list<node_t *>::const_iterator iter = m_top_level_nodes.begin();
       iter != m_top_level_nodes.end();
       ++iter)
  {
    ::report(&fp, L"%s[%d] ",
             (*iter)->name().c_str(),
             (*iter)->overall_idx());
    ::report(&fp, L"%s_END[%d]",
             (*iter)->name().c_str(),
             (*iter)->rule_end_node()->overall_idx());
    dump_lookahead_set(fp, (*iter)->rule_end_node());
    ::report(&fp, L"\n");
    dump_grammar_rule(fp, *iter);
    ::report(&fp, L"\n");
  }
  fp.close();
}
