#pragma once
#include "MathItem.h"

class CEmojiCmdBuilder : public IMathItemBuilder {
public:
   bool CanTakeCommand(PCSTR szCmd) const override;
   CMathItem* BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) override;
};
