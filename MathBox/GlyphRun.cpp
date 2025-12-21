#include "stdafx.h"
#include "GlyphRun.h"
#include "LMFontManager.h"

extern CLMFontManager g_LMFManager;

HRESULT CGlyphRun::SetGlyphs(vector<UINT32> vUnicode) {
   vector<UINT16> vIndices(vUnicode.size());
   IDWriteFontFace* pFontFace = g_LMFManager.GetFont(m_nFontIdx);
   _ASSERT_RET(pFontFace, E_FAIL);
   HRESULT hr = pFontFace->GetGlyphIndicesW(vUnicode.data(), (UINT32)vUnicode.size(), vIndices.data());
   if (FAILED(hr))
      return hr;
   return SetGlyphIndices(vIndices, vUnicode);
}
HRESULT CGlyphRun::SetGlyphIndices(vector<UINT16> vIndices, vector<UINT32> vUnicode) {
   vector< DWRITE_GLYPH_METRICS> vMetrics(vIndices.size());
   IDWriteFontFace* pFontFace = g_LMFManager.GetFont(m_nFontIdx);
   _ASSERT_RET(pFontFace, E_FAIL);
   HRESULT hr = pFontFace->GetDesignGlyphMetrics(vIndices.data(), (UINT32)vIndices.size(), vMetrics.data());
   if (FAILED(hr))
      return hr;
   //add new SWGlyphs
   for (UINT32 nIdx = 0; nIdx < vIndices.size(); ++nIdx) {
      SDWGlyph glyph;
      glyph.index = vIndices[nIdx];
      glyph.codepoint = nIdx < vUnicode.size() ? vUnicode[nIdx] : 0;
      glyph.metrics = vMetrics[nIdx];
      m_vGlyphs.push_back(glyph);
   }

   //update bounding box dimensions
   //recalc ink box 	
   m_BoundsF = { FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX };
   const float fFontPts = 64.0f; //arbitrary, only relative values matter
   hr = pFontFace->GetGlyphRunOutline(PTS2DIPS(fFontPts), vIndices.data(), nullptr, nullptr,
      (UINT32)vIndices.size(), FALSE, FALSE, this);
   if (FAILED(hr))
      return hr;
   m_Bounds.nMinX = DIPS2EM(fFontPts, m_BoundsF.fMinX);
   m_Bounds.nMinY = DIPS2EM(fFontPts, m_BoundsF.fMinY);
   m_Bounds.nMaxX = DIPS2EM(fFontPts, m_BoundsF.fMaxX);
   m_Bounds.nMaxY = DIPS2EM(fFontPts, m_BoundsF.fMaxY);
   return hr;
}

void CGlyphRun::Draw(const SDWRenderInfo& dwri, D2D1_POINT_2F ptfBaseOrigin, float fScale) {
   if (IsEmpty())
      return;
   //prepare glyph run
   DWRITE_GLYPH_RUN glyphRun = {};
   vector<UINT16> vGlyphIndices(m_vGlyphs.size());
   for (UINT32 i = 0; i < m_vGlyphs.size(); ++i) {
      vGlyphIndices[i] = m_vGlyphs[i].index;
   }
   IDWriteFontFace* pFontFace = g_LMFManager.GetFont(m_nFontIdx);
   _ASSERT_RET(pFontFace, );

   glyphRun.fontFace = pFontFace;
   glyphRun.fontEmSize = PTS2DIPS(dwri.fFontSizePts * fScale);
   glyphRun.glyphCount = (UINT32)m_vGlyphs.size();
   glyphRun.glyphIndices = vGlyphIndices.data();
   glyphRun.glyphAdvances = nullptr; //use font advances
   glyphRun.glyphOffsets = nullptr; //offsets, not used
   glyphRun.isSideways = FALSE;
   glyphRun.bidiLevel = 0;
   dwri.pRT->DrawGlyphRun(ptfBaseOrigin, &glyphRun, dwri.pBrush);
}
void CGlyphRun::UpdateInkBox_(const D2D1_POINT_2F& ptf) {
   m_BoundsF.fMinX = min(m_BoundsF.fMinX, ptf.x);
   m_BoundsF.fMinY = min(m_BoundsF.fMinY, ptf.y);
   m_BoundsF.fMaxX = max(m_BoundsF.fMaxX, ptf.x);
   m_BoundsF.fMaxY = max(m_BoundsF.fMaxY, ptf.y);
}
void CGlyphRun::GetCubicBezierMinMax_(float fP0, float fP1, float fP2, float fP3, OUT float& fMin, OUT float& fMax) {
   //defaults
   fMin = min(fP0, fP3);
   fMax = max(fP0, fP3);
   //p(t) = (1-t)^3⋅p0 + 3⋅(1-t)^2⋅t⋅p1 + 3⋅(1-t)⋅t^2⋅p2 + t^3⋅p3
   float fA = -fP0 + 3 * fP1 - 3 * fP2 + fP3;
   float fB = fP0 - 2 * fP1 + fP2;
   float fC = fP0 + fP1;
   //fA*t^2 + 2*fB*t + fC = 0
   //t = -fB +/- sqrt(fB*fB-fA*fC)/fA
   float fDiscr = fB * fB - fA * fC;
   if (fA < 0.000001f && fA > -0.000001f) {
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
