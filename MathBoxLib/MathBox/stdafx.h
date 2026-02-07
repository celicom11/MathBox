#pragma once
#include <stdint.h>

#define OUT
#define IN
typedef const char* PCSTR;
typedef const wchar_t* PCWSTR;

#define DECLARE_INTERFACE(iface)   struct __declspec(novtable) iface
//STL
#include <vector>
#include <cmath>
#include <string>
#include <fstream>
#include <map>
#include <set>
#include <unordered_map>
#include <algorithm>
#include <numeric>
using namespace std;

#define _ASSERT_RET(expr,retval) {if(!(expr)){_ASSERT(0);return retval;}}
