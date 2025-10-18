#pragma once
#include "MathItem.h"

//Factory for a Accented item
class CAccentBuilder { //TODO: public IMathItemBuilder
public:
   static CMathItem* BuildAccented(const CMathStyle& style, float fUserScale, CMathItem* pBase, 
      const string& sLatexAccentCmd);
};