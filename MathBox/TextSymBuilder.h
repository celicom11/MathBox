#pragma once
#include "MathItem.h"

//math mode operators and symbols (\mathord) builder
class CTextSymBuilder : public IMathItemBuilder {
public:
   static uint32_t _GetTextSymbolUni(PCSTR szCmd);
   //IMathItemBuilder implementation
   bool CanTakeCommand(PCSTR szCmd) const override {
      return _GetTextSymbolUni(szCmd) != 0;
   }
   CMathItem* BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) override;
};