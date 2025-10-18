#pragma once
#include "MathItem.h"

class CExtGlyphBuilder {
public:
   static uint16_t GetGlyphIndexBySizeIdx(uint32_t nUnicode, uint16_t nVariantIdx);
   static CMathItem* BuildVerticalGlyph(uint32_t nUnicode, const CMathStyle& style, uint32_t nSize, float fUserScale=1.0f);
   static CMathItem* BuildHorizontalGlyph(uint32_t nUnicode, const CMathStyle& style, uint32_t nSize, float fUserScale);
};