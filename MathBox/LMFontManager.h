#pragma once
#include "MathItem.h"

//LatinModern fonts data wrapper
class CLMFontManager {
   vector<IDWriteFontFace*>   m_vFontFaces; //11 lm fonts, 0 - LMM!
   vector<SLMMGlyph*>         m_vGlyphInfo;
public:
//CTOR/DTOR/Init
   CLMFontManager() = default;
   ~CLMFontManager() {
      Clear();
   }
   HRESULT Init(const wstring& sAppDir, IDWriteFactory* pDWriteFactory);
//ATTS
   IDWriteFontFace* GetFont(int16_t nFontIdx) const {
      return nFontIdx >= 0 && nFontIdx < m_vFontFaces.size() ? m_vFontFaces[nFontIdx] : nullptr;
   }
 //METHODS
   void Clear() {
      for (SLMMGlyph* pG : m_vGlyphInfo) {
         delete pG;
      }
      m_vGlyphInfo.clear();
      for (IDWriteFontFace* pFontFace : m_vFontFaces) {
         if (pFontFace) {
            pFontFace->Release();
            pFontFace = nullptr;
         }
      }
      m_vFontFaces.clear();
   }
   const SLMMGlyph* GetLMMGlyph(int16_t nFontIdx, uint32_t nUnicode) const;
   const SLMMGlyph* GetLMMGlyphByIdx(int16_t nFontIdx, uint16_t nIndex) const {
      _ASSERT_RET(nFontIdx == 0, nullptr);//only by Unicode if nFOntIdx>0! 
      return nIndex < m_vGlyphInfo.size() ? m_vGlyphInfo[nIndex] : nullptr;
   }
   const SLMMGlyph* GetLMMGlyphByCmd(PCSTR szCmd) const;
   // STATIC
   static bool _GetTextFontStyle(const string& sFontCmd, OUT STextFontStyle& tfStyle);
   static bool _GetMathFontStyle(const string& sFontCmd, OUT SMathFontStyle& mfStyle);
private:
   bool LoadLMMGlyphInfo_(const wstring& sAppDir);
   HRESULT LoadLatinModernFonts_(const wstring& sAppDir, IDWriteFactory* pDWriteFactory);
   HRESULT LoadLatinModernFont_(const wstring& sFontPath, IDWriteFactory* pDWriteFactory, OUT IDWriteFontFace** ppFontFace);
};