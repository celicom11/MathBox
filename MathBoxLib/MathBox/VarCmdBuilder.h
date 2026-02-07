#pragma once
#include "MathItem.h"

//setlength and other variable commands builder
class CVarCmdBuilder : public IMathItemBuilder {
public:
   //IMathItemBuilder implementation
   bool CanTakeCommand(PCSTR szCmd) const override;
   CMathItem* BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) override; 
private:
   CMathItem* BuildSetLength_(IParserAdapter* pParser, bool bAdd);
   CMathItem* BuildThe_(IParserAdapter* pParser);
};