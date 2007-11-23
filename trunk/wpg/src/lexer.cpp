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

wchar_t
analyser_environment_t::lexer_get_grammar_char()
{
  wchar_t ch;
  
  /* read the first character. */
  bool const result = lexerlib_read_ch(m_grammar_file, &ch);
  if (false == result)
  {
    /* if read failed, return 0. */
    throw ga_exception_t();
  }
  
  return ch;
}

void
analyser_environment_t::lexer_put_grammar_string(
  boost::shared_ptr<std::wstring> grammar_string)
{
  mp_saved_grammar_string = grammar_string;
}

boost::shared_ptr<std::wstring>
analyser_environment_t::lexer_get_grammar_string(
  std::list<wchar_t> const &delimiter)
{
  if (mp_saved_grammar_string.get() != 0)
  {
    boost::shared_ptr<std::wstring> result(mp_saved_grammar_string);
    
    mp_saved_grammar_string.reset();
    
    return result;
  }
  else
  {
    wchar_t ch;
  
    ch = lexer_get_grammar_char();
    if (WEOF == ch)
    {
      return boost::shared_ptr<std::wstring>(new std::wstring(L"EOF"));
    }
  
    // read success
  
    while ((L' ' == ch) || (L'\n' == ch) || (L'\r' == ch))
    {
      // If I first see a SPACE, NL, or CR,
      // then I have to read further to see the first normal
      // character.
      ch = lexer_get_grammar_char();
      if (WEOF == ch)
      {
        return boost::shared_ptr<std::wstring>(new std::wstring(L"EOF"));
      }
    }
  
    // If the first normal character is a delimiter, then return it
    // immediately.
    BOOST_FOREACH(wchar_t const &del_ch, delimiter)
    {
      if (del_ch == ch)
      {
        return boost::shared_ptr<std::wstring>(new std::wstring(1, ch));
      }
    }
  
    bool see_delimiter;
    boost::shared_ptr<std::wstring> token_str(new std::wstring);
  
    do
    {
      see_delimiter = false;
    
      (*token_str) += ch;
    
      ch = lexer_get_grammar_char();
    
      if ((L' ' == ch) ||
          (L'\n' == ch) ||
          (L'\r' == ch) ||
          (WEOF == ch))
      {
        return token_str;
      }
      
      BOOST_FOREACH(wchar_t const &del_ch, delimiter)
      {
        if (del_ch == ch)
        {
          see_delimiter = true;
          break;
        }
      }
      
      if (true == see_delimiter)
      {
        lexerlib_put_back(m_grammar_file, ch);
        
        return token_str;
      }
    } while (1);
  }
}
