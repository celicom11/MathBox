#pragma once
#include "MathItem.h"


//Factory for a VBOX container
class CUnderOverBuilder : public IMathItemBuilder {
public:
   //IMathItemBuilder implementation
   bool CanTakeCommand(PCSTR szCmd) const override;
   CMathItem* BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) override;
   //internal usage
   static CMathItem* _BuildUnderOverItem(IN CMathItem* pBase, PCSTR szCmd, const SParserContext& ctx);
};