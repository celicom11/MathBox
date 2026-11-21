#include "stdafx.h"
#include "D2DFontManager.h"
#include "MathBoxHostDefs.h"
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

CD2DFontManager::~CD2DFontManager() {
   for (IDWriteFontFace* pFontFace : m_vFontFaces) {
      if(pFontFace)
         pFontFace->Release();
   }
   m_vFontFaces.clear();
   SafeRelease(&m_pDWriteFactory);
}
HRESULT CD2DFontManager::Init(const wstring& sFontDir, IDWriteFactory* pDWriteFactory) {
   if (!pDWriteFactory || !m_vFontFaces.empty())
      return E_INVALIDARG;
   m_pDWriteFactory = pDWriteFactory;
   m_pDWriteFactory->AddRef();
   HRESULT hRes;
   m_sFontDir = sFontDir;
   for (PCWSTR szFontFile : _aLMFonts) {
      IDWriteFontFace* pFontFace = nullptr;
      hRes = LoadLatinModernFont_(sFontDir + szFontFile, pDWriteFactory, &pFontFace);
      CHECK_HR(hRes);
      m_vFontFaces.push_back(pFontFace);
      m_vFontScales.push_back(1.0f);
   }
   return S_OK;
}
bool CD2DFontManager::AddUtf8Font(PCWSTR szFontName, float fScale) {
   return AddSystemFonts_(szFontName, FONT_UTF8_REGULAR, 4, fScale);
}
bool CD2DFontManager::AddSymbolFont(PCWSTR szFontName, float fScale) {
   return AddSystemFonts_(szFontName, FONT_SYMBOL, 1, fScale);
}
bool CD2DFontManager::AddSystemFonts_(PCWSTR szFontName, int32_t nFirstSlot,
                                     uint32_t nCount, float fScale) {
   if (!m_pDWriteFactory || m_vFontFaces.size() < _countof(_aLMFonts) ||
       !szFontName || !*szFontName || !std::isfinite(fScale) || fScale <= 0)
      return false;
   ComPtr<IDWriteFontCollection> collection;
   if (FAILED(m_pDWriteFactory->GetSystemFontCollection(collection.GetAddressOf())))
      return false;
   UINT32 nFamilyIdx = 0;
   BOOL bExists = FALSE;
   if (FAILED(collection->FindFamilyName(szFontName, &nFamilyIdx, &bExists)) || !bExists)
      return false;
   ComPtr<IDWriteFontFamily> family;
   if (FAILED(collection->GetFontFamily(nFamilyIdx, family.GetAddressOf())))
      return false;
   vector<ComPtr<IDWriteFontFace>> vFaces(nCount);
   for (uint32_t nIdx = 0; nIdx < nCount; ++nIdx) {
      ComPtr<IDWriteFont> font;
      DWRITE_FONT_WEIGHT eWeight = (nIdx == 1 || nIdx == 3) ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL;
      DWRITE_FONT_STYLE eStyle = nIdx >= 2 ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL;
      // DirectWrite chooses the closest available face for missing family styles.
      if (FAILED(family->GetFirstMatchingFont(eWeight, DWRITE_FONT_STRETCH_NORMAL,
                                            eStyle, font.GetAddressOf())) ||
          FAILED(font->CreateFontFace(vFaces[nIdx].GetAddressOf())))
         return false;
   }
   // Reserve both arrays before replacing any existing faces.
   size_t nRequired = (size_t)nFirstSlot + nCount;
   m_vFontFaces.reserve(nRequired);
   m_vFontScales.reserve(nRequired);
   if (m_vFontFaces.size() < nRequired) {
      m_vFontFaces.resize(nRequired, nullptr);
      m_vFontScales.resize(nRequired, 1.0f);
   }
   for (uint32_t nIdx = 0; nIdx < nCount; ++nIdx) {
      SafeRelease(&m_vFontFaces[nFirstSlot + nIdx]);
      m_vFontFaces[nFirstSlot + nIdx] = vFaces[nIdx].Detach();
      m_vFontScales[nFirstSlot + nIdx] = fScale;
   }
   return true;
}
bool CD2DFontManager::GetFontIndices(int32_t nFontIdx, uint32_t nCount, const uint32_t* pUnicodes,
                                       OUT uint16_t* pIndices) {
   _ASSERT_RET(nFontIdx >= 0 && nFontIdx < m_vFontFaces.size(), false);
   _ASSERT_RET(pUnicodes && pIndices && nCount > 0, false);
   if (!m_vFontFaces[nFontIdx])
      return false;
   HRESULT hRes = m_vFontFaces[nFontIdx]->GetGlyphIndicesW(pUnicodes, nCount, pIndices);
   return SUCCEEDED(hRes);
}
bool CD2DFontManager::GetGlyphRunMetrics(int32_t nFontIdx, uint32_t nCount, const uint16_t* pIndices,
                                          OUT MB_GlyphMetrics* pGlyphMetrics, OUT MB_Bounds& bounds) {
   _ASSERT_RET(nFontIdx >= 0 && nFontIdx < m_vFontFaces.size(), false);
   _ASSERT_RET(pGlyphMetrics && pIndices && nCount > 0, false);
   vector< DWRITE_GLYPH_METRICS> vMetrics(nCount);
   IDWriteFontFace* pFontFace = m_vFontFaces[nFontIdx];
   if (!pFontFace)
      return false;
   HRESULT hr = pFontFace->GetDesignGlyphMetrics(pIndices, nCount, vMetrics.data());
   if (FAILED(hr))
      return false;
   DWRITE_FONT_METRICS fontMetrics{};
   pFontFace->GetMetrics(&fontMetrics);
   if (!fontMetrics.designUnitsPerEm)
      return false;
   const float fMetricScale = GetFontScale(nFontIdx) * otfUnitsPerEm / fontMetrics.designUnitsPerEm;
   //copy metrics
   for (uint32_t nIdx = 0; nIdx < nCount; ++nIdx) {
      pGlyphMetrics[nIdx].nLSB = F2NEAREST(vMetrics[nIdx].leftSideBearing * fMetricScale);
      pGlyphMetrics[nIdx].nRSB = F2NEAREST(vMetrics[nIdx].rightSideBearing * fMetricScale);
      pGlyphMetrics[nIdx].nTSB = F2NEAREST(vMetrics[nIdx].topSideBearing * fMetricScale);
      pGlyphMetrics[nIdx].nBSB = F2NEAREST(vMetrics[nIdx].bottomSideBearing * fMetricScale);
      pGlyphMetrics[nIdx].nAdvanceWidth = (uint32_t)F2NEAREST(vMetrics[nIdx].advanceWidth * fMetricScale);
      pGlyphMetrics[nIdx].nAdvanceHeight = (uint32_t)F2NEAREST(vMetrics[nIdx].advanceHeight * fMetricScale);
   }

   //update bounding box dimensions
   //recalc ink box 	
   m_GeomSink.ResetBounds();
   const float fFontPts = 64.0f; //arbitrary, only relative values matter
   hr = pFontFace->GetGlyphRunOutline(PTS2DIPS(fFontPts) * GetFontScale(nFontIdx), pIndices, nullptr, nullptr,
                                       nCount, FALSE, FALSE, &m_GeomSink);
   if (FAILED(hr))
      return false;
   const SBoundsF& boundsF = m_GeomSink.GetBounds();
   bounds.nMinX = DIPS2EM(fFontPts, boundsF.fMinX);
   bounds.nMinY = DIPS2EM(fFontPts, boundsF.fMinY);
   bounds.nMaxX = DIPS2EM(fFontPts, boundsF.fMaxX);
   bounds.nMaxY = DIPS2EM(fFontPts, boundsF.fMaxY);

   return true;

}

HRESULT CD2DFontManager::LoadLatinModernFont_(const wstring& sFontPath, IDWriteFactory* pDWriteFactory,
                                                OUT IDWriteFontFace** ppFontFace) {
   IDWriteFontFile* pFontFile = nullptr;
   HRESULT hr = pDWriteFactory->CreateFontFileReference(sFontPath.c_str(), nullptr, &pFontFile);
   CHECK_HR(hr);
   BOOL isSupported;
   DWRITE_FONT_FILE_TYPE fileType;
   DWRITE_FONT_FACE_TYPE faceType;
   UINT32 numberOfFonts;
   pFontFile->Analyze(&isSupported, &fileType, &faceType, &numberOfFonts);

   IDWriteFontFile* fontFileArray[] = { pFontFile };
   hr = pDWriteFactory->CreateFontFace(
      faceType, //DWRITE_FONT_FACE_TYPE_CFF,
      1, // file count
      fontFileArray,
      0, // face index
      DWRITE_FONT_SIMULATIONS_NONE,
      ppFontFace
   );
   pFontFile->Release();
   return hr;
}
void CD2DGeomSink::GetCubicBezierMinMax_(float fP0, float fP1, float fP2, float fP3, OUT float& fMin,
   OUT float& fMax) {
   //defaults
   fMin = min(fP0, fP3);
   fMax = max(fP0, fP3);
   //p(t) = (1-t)^3p0 + 3(1-t)^2tp1 + 3(1-t)t^2p2 + t^3p3
   float fA = -fP0 + 3 * fP1 - 3 * fP2 + fP3;
   float fB = fP0 - 2 * fP1 + fP2;
   float fC = -fP0 + fP1;
   //fA*t^2 + 2*fB*t + fC = 0
   //t = -fB +/- sqrt(fB*fB-fA*fC)/fA
   float fDiscr = fB * fB - fA * fC;
   if (fabs(fA) < 0.000001f) {
      if (fabs(fB) < 1e-6f) 
         return;     // derivative constant => no interior extrema
      float fTe = -fC / (2 * fB);
      float fOneTe = 1.0f - fTe;
      if (fTe >= 0 && fTe <= 1.0f) {
         fMin = fMax = fOneTe * fOneTe * fOneTe * fP0 + 3 * fOneTe * fOneTe * fTe * fP1 + 3 * fOneTe * fTe * fTe * fP2 + fTe * fTe * fTe * fP3;
      }
   }
   else if (fDiscr >= 0.0f) {
      float fTe = (-fB + sqrt(fDiscr)) / fA;
      if (fTe >= 0 && fTe <= 1.0f) {
         float fOneTe = 1.0f - fTe;
         float fVal = fOneTe * fOneTe * fOneTe * fP0 + 3 * fOneTe * fOneTe * fTe * fP1 + 3 * fOneTe * fTe * fTe * fP2 + fTe * fTe * fTe * fP3;
         fMin = min(fMin, fVal);
         fMax = max(fMax, fVal);
      }
      fTe = (-fB - sqrt(fDiscr)) / fA;
      if (fTe >= 0 && fTe <= 1.0f) {
         float fOneTe = 1.0f - fTe;
         float fVal = fOneTe * fOneTe * fOneTe * fP0 + 3 * fOneTe * fOneTe * fTe * fP1 + 3 * fOneTe * fTe * fTe * fP2 + fTe * fTe * fTe * fP3;
         fMin = min(fMin, fVal);
         fMax = max(fMax, fVal);
      }
   }
}


