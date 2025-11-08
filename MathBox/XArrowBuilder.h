#pragma once
#include "MathItem.h"


//Factory for extendable horizontal glyphs with Over/Under items
class CXArrowBuilder : public IMathItemBuilder {
public:
   //IMathItemBuilder implementation
   bool CanTakeCommand(PCSTR szCmd) const override;
   CMathItem* BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) override; 
private:
};