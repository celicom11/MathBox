#pragma once
#include "MathItem.h"

class COpenCloseBuilder {
public:
   //
   static bool ConsumeDelimiter(PCSTR szCmd, const CMathStyle& style, OUT PCSTR& szNext, OUT SLMMDelimiter& lmmDelimiter);
   //manual sized delimiter
   static CMathItem* BuildDelimiter(const SLMMDelimiter& lmmDelimiter, const CMathStyle& style, float fUserScale = 1.0f);
   //dynamic sized delimiter
   static CMathItem* BuildOpenClose(uint32_t nUniOpen, uint32_t nUniClose, CMathItem* pItem, const CMathStyle& style, float fUserScale=1.0f);
   //static CMathItem* BuildOpenCloseM(uint32_t nUniOpen, uint32_t nUniMiddle, uint32_t nUniClose,
   //                                  CMathItem* pItem1, CMathItem* pItem2, const CMathStyle& style, float fUserScale = 1.0f);
private:
};