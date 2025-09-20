#pragma once
#include "MathItem.h"

//Radical's Factory
class CRadicalBuilder {
public:
   //CTOR
   CRadicalBuilder() = default;
   CMathItem* BuildRadical(const CMathStyle& style, float fUserScale, CMathItem* pRadicand, CMathItem* pRadDegree = nullptr);
private:
   CMathItem* BuildSimpleRadical_(const CMathStyle& style, float fUserScale, CMathItem* pRadicand,
                                  UINT16 nRadicalVariant, CMathItem* pRadDegree);
   CMathItem* AssembleRadical_(const CMathStyle& style, float fUserScale, CMathItem* pRadicand, CMathItem* pRadDegree);
};