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

#ifndef __ga_exception_hpp__
#define __ga_exception_hpp__

class ga_exception_t : public std::exception
{
public:

  virtual ~ga_exception_t()
  { }
};
typedef class ga_exception_t ga_exception_t;

class ga_exception_meet_ambiguity_t : public ga_exception_t
{
};
typedef ga_exception_meet_ambiguity_t ga_exception_meet_ambiguity_t;

#endif
