#pragma once
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
using namespace std;

#define _ASSERT_RET(expr,retval) {if(!(expr)){_ASSERT(0);return retval;}}


#include <CppUnitTest.h>
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
