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

#ifndef __lexer_hpp__
#define __lexer_hpp__

#include "ga_exception.hpp"

enum GA_TOKEN_TYPE_ENUM
{
  GA_TOKEN_TYPE_NORMAL,
  GA_TOKEN_TYPE_CONTROL
};

/// 
/// @brief
/// Read a string from the file specified from the first
/// parameter.
///
/// This function will return a string without space and a
/// cariage return at each time.
///
/// @param file 
/// @param delimiter 
///
/// @return 
///
template<typename T>
boost::shared_ptr<std::wstring>
lexer_get_answer_string(
  T &file,
  std::list<wchar_t> const &delimiter)
{
  wchar_t ch;
  bool result;
  
  /* read the first character. */
  result = lexerlib_read_ch(file, &ch);
  if (false == result)
  {
    /* if read failed, return 0. */
    throw ga_exception_t();
  }
  else
  {
    if (WEOF == ch)
    {
      return boost::shared_ptr<std::wstring>(new std::wstring(L"EOF"));
    }
  }
  
  /* read success */
  
  if (L' ' == ch)
  {
    /* If I first read a space, then I have to read further
     * to see the first normal character.
     */
    result = lexerlib_read_ch(file, &ch);
    if (false == result)
    {
      /* read failed. */
      throw ga_exception_t();
    }
    else
    {
      if (WEOF == ch)
      {
        return boost::shared_ptr<std::wstring>(new std::wstring(L"EOF"));
      }
    }
  }
  
  /* If the first normal character is a delimiter, then return it
   * immediately.
   */
  for (std::list<wchar_t>::const_iterator iter = delimiter.begin();
       iter != delimiter.end();
       ++iter)
  {
    if ((*iter) == ch)
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
    
    result = lexerlib_read_ch(file, &ch);
    if (false == result)
    {
      throw ga_exception_t();
    }
    
    if ((L' ' == ch) || (WEOF == ch))
    {
      return token_str;
    }
    
    for (std::list<wchar_t>::const_iterator iter = delimiter.begin();
         iter != delimiter.end();
         ++iter)
    {
      if ((*iter) == ch)
      {
        see_delimiter = true;
        break;
      }
    }
    
    if (true == see_delimiter)
    {
      lexerlib_put_back(file, ch);
      
      return token_str;
    }
  } while (1);
}

#endif
