#pragma once
#include "MathItem.h"

//math mode font commands: \mathnormal,\mathit,etc. {arg}
class CMathFontCmdBuilder : public IMathItemBuilder {
public:
   //IMathItemBuilder implementation
   bool CanTakeCommand(PCSTR szCmd) const override;
   CMathItem* BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) override;
};