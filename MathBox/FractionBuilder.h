#pragma once
#include "MathItem.h"

//Factory for a Fraction (with a bar) Item
class CFractionBuilder : public IMathItemBuilder {
public:
   //IMathItemBuilder implementation
   bool CanTakeCommand(PCSTR szCmd) const override;
   CMathItem* BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) override;
   //Legacy API!
   static CMathItem* _BuildFraction(const CMathStyle& style, float fUserScale, CMathItem* pNum, CMathItem* pDenom);
};