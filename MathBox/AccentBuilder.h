#pragma once
#include "MathItem.h"

//Factory for a Accented item
class CAccentBuilder : public IMathItemBuilder {

public:
   //IMathItemBuilder
   bool CanTakeCommand(PCSTR szCmd) const override;
   CMathItem* BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) override;
private:
   CMathItem* BuildAccented_(IParserAdapter* pParser, CMathItem* pBase, const string& sLatexAccentCmd);
};