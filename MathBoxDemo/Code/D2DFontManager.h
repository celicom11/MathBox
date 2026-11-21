#pragma once
#include "..\..\MathBoxLib\LibCode\MathBox_CAPI.h"
#include "D2DGeomSink.h"

class CD2DFontManager {
//DATA
   wstring                    m_sFontDir;
   CD2DGeomSink               m_GeomSink;
   vector<IDWriteFontFace*>   m_vFontFaces; //14 LM faces followed by optional host faces
   vector<float>              m_vFontScales;
   IDWriteFactory*            m_pDWriteFactory{ nullptr };
public:
//CTOR/DTOR/INIT
   CD2DFontManager() = default;
   ~CD2DFontManager();
   CD2DFontManager(const CD2DFontManager&) = delete;
   CD2DFontManager& operator=(const CD2DFontManager&) = delete;
   HRESULT Init(const wstring& sFontDir, IDWriteFactory* pDWriteFactory);
//ATTS
   const wstring& GetFontDir() const { return m_sFontDir; }
   float GetFontScale(int32_t nIdx) const {
      return nIdx >= 0 && nIdx < (int32_t)m_vFontScales.size() ? m_vFontScales[nIdx] : 1.0f;
   }
   IDWriteFontFace* GetDWFont(int32_t nIdx) const {
      _ASSERT_RET(nIdx >= 0 && nIdx < (int32_t)m_vFontFaces.size(), nullptr);
      return m_vFontFaces[nIdx];
   }
//IFontManager
   uint32_t FontCount() const { return (uint32_t)m_vFontFaces.size(); }
   bool GetFontIndices(int32_t nFontIdx, uint32_t nCount, const uint32_t* pUnicodes, OUT uint16_t* pIndices);
   bool GetGlyphRunMetrics(int32_t nFontIdx, uint32_t nCount, const uint16_t* pIndices,
                           OUT MB_GlyphMetrics* pGlyphMetrics, OUT MB_Bounds& bounds);
//METHODS
   bool AddUtf8Font(PCWSTR szFontName, float fScale = 1.0f);
   bool AddSymbolFont(PCWSTR szFontName, float fScale = 0.8f);
private:
   bool AddSystemFonts_(PCWSTR szFontName, int32_t nFirstSlot, uint32_t nCount, float fScale);
   static HRESULT LoadLatinModernFont_(const wstring& sFontPath, IDWriteFactory* pDWriteFactory, OUT IDWriteFontFace** ppFontFace);
};
