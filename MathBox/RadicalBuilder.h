#pragma once
#include "MathItem.h"

//Radical's Factory
class CRadicalBuilder {
public:
   //CTOR
   CRadicalBuilder() = default;
   static CMathItem* BuildRadical(const CMathStyle& style, float fUserScale, CMathItem* pRadicand, CMathItem* pRadDegree = nullptr);
private:
   static CMathItem* BuildSimpleRadical_(const CMathStyle& style, float fUserScale, CMathItem* pRadicand,
                                  UINT16 nRadicalVariant, CMathItem* pRadDegree);
   static CMathItem* AssembleRadical_(const CMathStyle& style, float fUserScale, CMathItem* pRadicand, CMathItem* pRadDegree);
};