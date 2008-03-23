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
#include "ga_exception.hpp"
#include "lexer.hpp"

#include "wcl_memory_debugger\memory_debugger.h"

boost::shared_ptr<std::wstring>
analyser_environment_t::lexer_get_grammar_string(
  std::list<wchar_t> const &delimiter)
{
  std::list<wchar_t> skip;
  boost::shared_ptr<std::wstring> result_str(::new std::wstring);
  
  skip.push_back(L' ');
  skip.push_back(L'\n');
  skip.push_back(L'\r');
  
  try
  {
    Wcl::Lexerlib::read_string(m_grammar_file, delimiter, skip, *(result_str.get()));
  }
  catch (Wcl::Lexerlib::SourceIsBrokenException &)
  {
    throw ga_exception_t();
  }
  
  return result_str;
}
