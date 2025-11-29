#pragma once
#include "MathItem.h"


//Factory for font/scale commands
class CScaleCmdBuilder : public IMathItemBuilder {
public:
   //IMathItemBuilder implementation
   bool CanTakeCommand(PCSTR szCmd) const override;
   CMathItem* BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) override;
};