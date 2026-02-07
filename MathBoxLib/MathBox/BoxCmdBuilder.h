#pragma once
#include "MathItem.h"
#include "OverlayItem.h"


//OverlayItem factory for boxes
class CBoxCmdBuilder : public IMathItemBuilder {
public:
   //IMathItemBuilder implementation
   bool CanTakeCommand(PCSTR szCmd) const override;
   CMathItem* BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) override; 
private:
   CMathItem* GetContent_(const string& sCmd, IParserAdapter* pParser,
      OUT SRect& rcBox, OUT float& fBoxRule,
      OUT uint32_t& argbFrame, OUT uint32_t& argbFill);
};