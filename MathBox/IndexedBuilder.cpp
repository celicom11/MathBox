#include "stdafx.h"
#include "IndexedBuilder.h"
#include "ContainerItem.h"
#include "WordItem.h"
#include "LMFontManager.h"

extern CLMFontManager g_LMFManager;

CMathItem* CIndexedBuilder::BuildIndexed(const CMathStyle& style, float fUserScale,
                                          CMathItem* pBase, CMathItem* pSupers, CMathItem* pSubs) {
   _ASSERT_RET(pBase && (pSupers || pSubs), nullptr);
   float fScale = fUserScale * style.StyleScale();
   CContainerItem* pRetBox = new CContainerItem(eacINDEXED, style);
   pRetBox->AddBox(pBase, 0, 0);
   uint16_t nItalicCorrection = 0;
   //bool bBaseIsSym = false; //not in use?
   if (pBase->Type() == eacWORD ) {
      const CGlyphRun& glyphRun = ((CWordItem*)pBase)->GlyphRun();
      if (glyphRun.GetFontIdx() == FONT_LMM) {
         const SDWGlyph& dwgLast = glyphRun.Glyphs().back();
         const SLMMGlyph* pLMMGlyph = g_LMFManager.GetLMMGlyphByIdx(FONT_LMM, dwgLast.index);
         if (pLMMGlyph)
            nItalicCorrection = F2NEAREST(pLMMGlyph->nItalCorrection * fScale);
      }
   }
   //position superscript
   int32_t nSuperShiftUp = F2NEAREST((style.IsCramped() ? otfSuperscriptShiftUpCramped : otfSuperscriptShiftUp)*fScale);
   if (pSupers) {
      nSuperShiftUp = max(nSuperShiftUp, F2NEAREST(otfSuperscriptBottomMin*fScale) + pSupers->Box().Depth());
      nSuperShiftUp = max(nSuperShiftUp, pBase->Box().Ascent() - F2NEAREST(otfSuperscriptBaselineDropMax*fScale));
      //nSuperShiftUp may be increased below in pSubs placement!
   }
   if (pSubs) {
      int32_t nSubShiftDown = F2NEAREST(otfSubscriptShiftDown*fScale);
      nSubShiftDown = max(nSubShiftDown, pSubs->Box().Ascent() - F2NEAREST(otfSubscriptTopMax*fScale));
      nSubShiftDown = max(nSubShiftDown, F2NEAREST(otfSubscriptBaselineDropMin*fScale) + pBase->Box().Depth());
      if (pSupers) {
         int32_t nSubSuperGap = (nSuperShiftUp - pSupers->Box().Depth()) + (nSubShiftDown - pSubs->Box().Ascent());
         if (nSubSuperGap < F2NEAREST(otfSubSuperscriptGapMin*fScale)) {
            int32_t nD = F2NEAREST(otfSuperscriptBottomMaxWithSubscript*fScale) - (nSuperShiftUp - pSupers->Box().Depth());
            if (nD > 0) {
               nD = min(nD, F2NEAREST(otfSubSuperscriptGapMin*fScale) - nSubSuperGap);
               nSubSuperGap += nD;
               nSuperShiftUp += nD;
            }
            nD = F2NEAREST(otfSubSuperscriptGapMin*fScale) - nSubSuperGap;
            if (nD > 0)
               nSubShiftDown += nD;
         }
      }
      pRetBox->AddBox(pSubs, pBase->Box().Right(), pBase->Box().BaselineY() + nSubShiftDown - pSubs->Box().Ascent());
   }
   if (pSupers)
      pRetBox->AddBox(pSupers, pBase->Box().Right() + nItalicCorrection,
                               pBase->Box().BaselineY() - nSuperShiftUp - pSupers->Box().Ascent());
   pRetBox->AddWidth(F2NEAREST(otfSpaceAfterScript*fScale));
   pRetBox->NormalizeOrigin(0, 0);
   return pRetBox;
}
