#pragma once
#include "MathBox\AGL.h"
#include "D2DGeomSink.h"

class CD2DFontManager : public IFontManager {
//DATA
   CD2DGeomSink               m_GeomSink;
   vector<IDWriteFontFace*>   m_vFontFaces; //11 lm fonts, 0 - LMM!
public:
//CTOR/DTOR/INIT
   CD2DFontManager() = default;
   ~CD2DFontManager();
   HRESULT Init(const wstring& sAppDir, IDWriteFactory* pDWriteFactory);
//ATTS
   IDWriteFontFace* GetDWFont(int32_t nIdx) const {
      _ASSERT_RET(nIdx >= 0 && nIdx < (int32_t)m_vFontFaces.size(), nullptr);
      return m_vFontFaces[nIdx];
   }
//IFontManager
   uint32_t FontCount() const override {
      return (uint32_t)m_vFontFaces.size();
   }
   bool GetFontIndices(int32_t nFontIdx, uint32_t nCount, const uint32_t* pUnicodes,
                        OUT uint16_t* pIndices) override;
   bool GetGlyphRunMetrics(int32_t nFontIdx, uint32_t nCount, const uint16_t* pIndices,
                           OUT SGlyphMetrics* pGlyphMetrics, OUT SBounds& bounds) override;
private:
   static HRESULT LoadLatinModernFont_(const wstring& sFontPath, IDWriteFactory* pDWriteFactory, OUT IDWriteFontFace** ppFontFace);
};
