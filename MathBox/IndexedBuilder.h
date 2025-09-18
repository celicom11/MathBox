#pragma once
#include "MathItem.h"

//Factory for a Indexed (subscript/superscript) item
class CIndexedBuilder {
public:
   static CMathItem* BuildIndexed(ILMMInfo* pLMMInfo, const CMathStyle& style, float fUserScale,
                                 CMathItem* pBase, CMathItem* pSupers, CMathItem* pSubs);
};