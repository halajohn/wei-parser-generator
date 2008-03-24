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
  std::list<wchar_t> skip;
  boost::shared_ptr<std::wstring> result_str(::new std::wstring);
  
  skip.push_back(L' ');
  
  try
  {
    Wcl::Lexerlib::read_string(file, delimiter, skip, *(result_str.get()));
  }
  catch (Wcl::Lexerlib::SourceIsBrokenException &)
  {
    /* if read failed, return 0. */
    throw ga_exception_t();
  }
  
  return result_str;
}

#endif
