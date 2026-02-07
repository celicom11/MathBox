#pragma once
#include "AGL.h"

//LatinModernMath font OpenType data wrapper
class CLMMFont : public ILMFManager {
   vector<SLMMGlyph*>   m_vGlyphInfo;
public:
 //CTOR/DTOR/Init
   CLMMFont() = default;
   ~CLMMFont() {
      Clear();
   }
   bool Init(const wstring& sFontsDir); //Unicode
 //METHODS
   void Clear() {
      for (SLMMGlyph* pG : m_vGlyphInfo) {
         delete pG;
      }
      m_vGlyphInfo.clear();
   }
 //ILMFManager
   const SLMMGlyph* GetLMMGlyph(int16_t nFontIdx, uint32_t nUnicode) override;
   const SLMMGlyph* GetLMMGlyphByIdx(int16_t nFontIdx, uint16_t nIndex) override {
      _ASSERT_RET(nFontIdx == 0, nullptr);//only by Unicode if nFOntIdx>0! 
      return nIndex < m_vGlyphInfo.size() ? m_vGlyphInfo[nIndex] : nullptr;
   }
   const SLMMGlyph* GetLMMGlyphByCmd(PCSTR szCmd) override;
};