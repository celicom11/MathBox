#pragma once
#include "MathItem.h"

//Factory for Glue items
class CHSpacingBuilder : public IMathItemBuilder {
public:
   //IMathItemBuilder
   bool CanTakeCommand(PCSTR szCmd) const override;
   CMathItem* BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) override;
};