// Change these values to use different versions
#define WINVER        _WIN32_WINNT_WIN10
#define _WIN32_WINNT  _WIN32_WINNT_WIN10
#define _WIN32_IE     _WIN32_IE_IE110

#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
//STL
#include <vector>
#include <cmath>
#include <string>
#include <fstream>
#include <unordered_map>
using namespace std;

#define _ASSERT_RET(expr,retval) {if(!(expr)){_ASSERT(0);return retval;}}
#define CHECK_HR(hr) {if(FAILED(hr)){_ASSERT(0);return hr;}}
//linker
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma once
