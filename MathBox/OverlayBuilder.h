#pragma once
#include "MathItem.h"


//Factory for item's poly-line overlay/decorations
class COverlayBuilder : public IMathItemBuilder {
public:
   //IMathItemBuilder implementation
   bool CanTakeCommand(PCSTR szCmd) const override;
   CMathItem* BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) override; 
};