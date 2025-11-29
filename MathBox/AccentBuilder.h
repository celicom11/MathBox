#pragma once
#include "MathItem.h"

//Factory for a Accented item
class CAccentBuilder : public IMathItemBuilder {
public:
   //IMathItemBuilder
   bool CanTakeCommand(PCSTR szCmd) const override;
   CMathItem* BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) override;
   static CMathItem* _BuildAccented(const CMathStyle& style, float fUserScale, CMathItem* pBase, 
      const string& sLatexAccentCmd);
};