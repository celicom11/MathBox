#pragma once
#include "MathItem.h"

//Radical's Factory
class CRadicalBuilder : public IMathItemBuilder {
public:
   //IMathItemBuilder implementation
   bool CanTakeCommand(PCSTR szCmd) const override;
   CMathItem* BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) override;
   // static helpers
   static CMathItem* _BuildRadical(const CMathStyle& style, float fUserScale, CMathItem* pRadicand, CMathItem* pRadDegree = nullptr);
private:
   static CMathItem* BuildSimpleRadical_(const CMathStyle& style, float fUserScale, CMathItem* pRadicand,
                                  uint16_t nRadicalVariant, CMathItem* pRadDegree);
   static CMathItem* AssembleRadical_(const CMathStyle& style, float fUserScale, CMathItem* pRadicand, CMathItem* pRadDegree);
};