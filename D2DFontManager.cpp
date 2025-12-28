#include "stdafx.h"
#include "D2DFontManager.h"
#include "MathBoxHostDefs.h"

CD2DFontManager::~CD2DFontManager() {
   for (IDWriteFontFace* pFontFace : m_vFontFaces) {
      if(pFontFace)
         pFontFace->Release();
   }
   m_vFontFaces.clear();
}
HRESULT CD2DFontManager::Init(const wstring& sFontDir, IDWriteFactory* pDWriteFactory) {
   HRESULT hRes;
   m_sFontDir = sFontDir;
   for (PCWSTR szFontFile : _aLMFonts) {
      IDWriteFontFace* pFontFace = nullptr;
      hRes = LoadLatinModernFont_(sFontDir + szFontFile, pDWriteFactory, &pFontFace);
      CHECK_HR(hRes);
      m_vFontFaces.push_back(pFontFace);
   }
   return S_OK;
}
bool CD2DFontManager::GetFontIndices(int32_t nFontIdx, uint32_t nCount, const uint32_t* pUnicodes,
                                       OUT uint16_t* pIndices) {
   _ASSERT_RET(nFontIdx >= 0 && nFontIdx < m_vFontFaces.size(), false);
   _ASSERT_RET(pUnicodes && pIndices && nCount > 0, false);
   HRESULT hRes = m_vFontFaces[nFontIdx]->GetGlyphIndicesW(pUnicodes, nCount, pIndices);
   return SUCCEEDED(hRes);
}
bool CD2DFontManager::GetGlyphRunMetrics(int32_t nFontIdx, uint32_t nCount, const uint16_t* pIndices,
                                          OUT MB_GlyphMetrics* pGlyphMetrics, OUT MB_Bounds& bounds) {
   _ASSERT_RET(nFontIdx >= 0 && nFontIdx < m_vFontFaces.size(), false);
   _ASSERT_RET(pGlyphMetrics && pIndices && nCount > 0, false);
   vector< DWRITE_GLYPH_METRICS> vMetrics(nCount);
   IDWriteFontFace* pFontFace = m_vFontFaces[nFontIdx];
   _ASSERT_RET(pFontFace, false);
   HRESULT hr = pFontFace->GetDesignGlyphMetrics(pIndices, nCount, vMetrics.data());
   if (FAILED(hr))
      return false;
   //copy metrics
   for (uint32_t nIdx = 0; nIdx < nCount; ++nIdx) {
      pGlyphMetrics[nIdx].nLSB = vMetrics[nIdx].leftSideBearing;
      pGlyphMetrics[nIdx].nRSB = vMetrics[nIdx].rightSideBearing;
      pGlyphMetrics[nIdx].nTSB = vMetrics[nIdx].topSideBearing;
      pGlyphMetrics[nIdx].nBSB = vMetrics[nIdx].bottomSideBearing;
      pGlyphMetrics[nIdx].nAdvanceWidth = vMetrics[nIdx].advanceWidth;
      pGlyphMetrics[nIdx].nAdvanceHeight = vMetrics[nIdx].advanceHeight;
   }

   //update bounding box dimensions
   //recalc ink box 	
   m_GeomSink.ResetBounds();
   const float fFontPts = 64.0f; //arbitrary, only relative values matter
   hr = pFontFace->GetGlyphRunOutline(PTS2DIPS(fFontPts), pIndices, nullptr, nullptr,
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


