#ifndef __precompiled_header_hpp__
#define __precompiled_header_hpp__

#include <cstdio>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <climits>
#include <cctype>

#include <list>
#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <exception>
#include <algorithm>
#include <functional>

#include <windows.h>

#include <boost/smart_ptr.hpp>
#include <boost/foreach.hpp>

#if !defined(NDEBUG)
#define BOOST_MULTI_INDEX_ENABLE_INVARIANT_CHECKING
#define BOOST_MULTI_INDEX_ENABLE_SAFE_MODE
#endif

#pragma warning(disable:4819)
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>

#include <boost/utility.hpp>
#include <boost/lexical_cast.hpp>

#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>

#include "wcl_fmtstr/fmtstr.h"
#include "wcl_memory_debugger/memory_debugger.h"
#include "wcl_fslib/filename.h"
#include "wcl_lexerlib/lexerlib.h"

#endif
