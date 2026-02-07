#pragma once
#include "..\MathBox\MathItem.h"

// C++ IDocParams, MB_DocParams wrapper
class CDocParamWrap :public IDocParams,
                     public IFontManager
{
   const MB_DocParams*  m_pDP;
   ILMFManager&         m_lmfMgr; //external ref
public:
   CDocParamWrap(const MB_DocParams* doc, ILMFManager& lmfMgr) : 
      m_pDP(doc), m_lmfMgr(lmfMgr) {}
   ~CDocParamWrap() = default;
//IDocParams impl
   IFontManager& FontManager() override { return *this; }
   ILMFManager& LMFManager() override { return m_lmfMgr; }
   float DefaultFontSizePts() override { 
      return m_pDP->font_size_pts; 
   }
   int32_t MaxWidthFDU() override {
      return m_pDP->max_width_fdu;
   }
   uint32_t ColorBkg() override {
      return m_pDP->color_bkg_argb;
   }
   uint32_t ColorSelection() override {
      return m_pDP->color_selection_argb;
   }
   //Text color ~= Foreground color!
   uint32_t ColorText() override {
      return m_pDP->color_text_argb;
   }
//IFontManager 
   uint32_t FontCount() const override {
      return m_pDP->font_mgr.fontCount;
   }
   bool GetFontIndices(int32_t nFontIdx, uint32_t nCount, const uint32_t* pUnicodes,
                        OUT uint16_t* pIndices) override {
      return m_pDP->font_mgr.getFontIndices(nFontIdx, nCount, pUnicodes, pIndices);
   }
   bool GetGlyphRunMetrics(int32_t nFontIdx, uint32_t nCount, const uint16_t* pIndices,
                           OUT SGlyphMetrics* pGlyphMetrics, OUT SBounds& bounds) override {
      vector<MB_GlyphMetrics> vGMetrics(nCount,{sizeof(MB_GlyphMetrics)});
      MB_Bounds mbBounds;
      if (!m_pDP->font_mgr.getGlyphRunMetrics(nFontIdx, nCount, pIndices, vGMetrics.data(), &mbBounds))
         return false;
      //copy output data
      bounds.nMinX = mbBounds.nMinX;
      bounds.nMinY = mbBounds.nMinY;
      bounds.nMaxX = mbBounds.nMaxX;
      bounds.nMaxY = mbBounds.nMaxY;
      for (uint32_t nIdx = 0; nIdx < nCount; ++nIdx) {
         SGlyphMetrics& gmTarget = pGlyphMetrics[nIdx];
         const MB_GlyphMetrics& gmSrc = vGMetrics[nIdx];
         gmTarget.nLSB = gmSrc.nLSB;
         gmTarget.nAdvanceWidth = gmSrc.nAdvanceWidth;
         gmTarget.nRSB = gmSrc.nRSB;
         gmTarget.nTSB = gmSrc.nTSB;
         gmTarget.nAdvanceHeight = gmSrc.nAdvanceHeight;
         gmTarget.nBSB = gmSrc.nBSB;
      }
      return true;
   }
};

struct SDocRendererWrap : public IDocRenderer {
   const MBI_DocRenderer* m_pMBRenderer;
   SDocRendererWrap(const MBI_DocRenderer* pMBRenderer) : m_pMBRenderer(pMBRenderer) {}

   void DrawLine(SPointF pt1, SPointF pt2, EnumLineStyles eStyle,
                 float fWidth, uint32_t nARGB) override {
      m_pMBRenderer->drawLine(pt1.fX, pt1.fY, pt2.fX, pt2.fY, eStyle, fWidth, nARGB);
   }
   void DrawRect(SRectF rcf, EnumLineStyles eStyle,
                  float fWidth, uint32_t nARGB) override {
      m_pMBRenderer->drawRect(rcf.fLeft, rcf.fTop, rcf.fRight, rcf.fBottom, eStyle, fWidth, nARGB);
   }
   void FillRect(SRectF rcf, uint32_t nARGB) override {
      m_pMBRenderer->fillRect(rcf.fLeft, rcf.fTop, rcf.fRight, rcf.fBottom, nARGB);
   }
   void DrawGlyphRun(int32_t nFontIdx, uint32_t nCount, const uint16_t* pIndices,
                     SPointF ptfBaseOrigin, float fScale, uint32_t nARGB) override {
      m_pMBRenderer->drawGlyphRun(nFontIdx, nCount, pIndices, ptfBaseOrigin.fX, ptfBaseOrigin.fY, fScale, nARGB);
   
   }
};
