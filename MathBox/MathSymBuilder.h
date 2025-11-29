#pragma once
#include "MathItem.h"

//math mode operators and symbols (\mathord) builder
class CMathSymBuilder : public IMathItemBuilder {
public:
   //IMathItemBuilder implementation
   bool CanTakeCommand(PCSTR szCmd) const override;
   CMathItem* BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) override;
private:
   static CMathItem* BuildTeXSymbol_(const string& sFontCmd, const string& sSym, const CMathStyle& style, float fUserScale);
};