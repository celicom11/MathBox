#pragma once
#include "MathItem.h"

class CExtGlyphBuilder {
   IDocParams& m_Doc; //external!
public:
   CExtGlyphBuilder(IDocParams& docp):m_Doc(docp) {}
   uint16_t GlyphIndexByVariantIdx(uint32_t nUnicode, uint16_t nVariantIdx);
   CMathItem* BuildVerticalGlyph(uint32_t nUnicode, const CMathStyle& style, uint32_t nSize, 
                     float fUserScale = 1.0f);
   CMathItem* BuildHorizontalGlyph(uint32_t nUnicode, const CMathStyle& style, uint32_t nSize, float fUserScale);
};