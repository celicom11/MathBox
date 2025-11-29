#pragma once
#include "MathItem.h"


//Factory for a VBOX container
class CUnderOverBuilder : public IMathItemBuilder {
public:
   //IMathItemBuilder implementation
   bool CanTakeCommand(PCSTR szCmd) const override;
   CMathItem* BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) override;
   //Legacy API!
   static CMathItem* _BuildItem(PCSTR szCmd, CMathItem* pBase, const CMathStyle& style, float fUserScale);
                        
private:

};