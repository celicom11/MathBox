#pragma once
#include "MathItem.h"


//Factory for \{textcmd}{...text...}
class CTextCmdBuilder : public IMathItemBuilder {
public:
   //IMathItemBuilder implementation
   bool CanTakeCommand(PCSTR szCmd) const override;
   CMathItem* BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) override; 
};