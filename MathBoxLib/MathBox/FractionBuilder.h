#pragma once
#include "MathItem.h"

//Factory for a Fraction (with a bar) Item
class CFractionBuilder : public IMathItemBuilder {
public:
   //IMathItemBuilder implementation
   bool CanTakeCommand(PCSTR szCmd) const override;
   CMathItem* BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) override;
private:
   static CMathItem* _BuildFraction(IDocParams& doc, const SParserContext& ctx,
                                    CMathItem* pNum, CMathItem* pDenom);
};