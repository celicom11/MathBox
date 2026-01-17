#pragma once
#include "MathItem.h"


//Set shifts for theargument item
class CShiftBuilder : public IMathItemBuilder {
public:
//IMathItemBuilder implementation
   bool CanTakeCommand(PCSTR szCmd) const override;
   CMathItem* BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) override; 
};