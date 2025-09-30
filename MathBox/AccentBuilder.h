#pragma once
#include "MathItem.h"

//Factory for a Accented item
class CAccentBuilder {
public:
   static CMathItem* BuildAccented(const CMathStyle& style, float fUserScale, CMathItem* pBase, const string& sLatexAccentCmd);
   //static bool IsValidLatexAccentCmd(const string& sLatexAccentCmd);
};