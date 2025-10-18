#include "stdafx.h"
#include "IndexedBuilder.h"
#include "ContainerItem.h"
#include "WordItem.h"
#include "LMFontManager.h"

extern CLMFontManager g_LMFManager;
namespace {
   void _GetSuperscriptRelPos(CMathItem* pBase, bool bCramped, float fScale, const STexBox& boxSuper, OUT int32_t& nDY) {
      //position superscript
      int32_t nSuperShiftUp = F2NEAREST((bCramped ? otfSuperscriptShiftUpCramped : otfSuperscriptShiftUp) * fScale);
      nSuperShiftUp = max(nSuperShiftUp, F2NEAREST(otfSuperscriptBottomMin * fScale) + boxSuper.Depth());
      nSuperShiftUp = max(nSuperShiftUp, pBase->Box().Ascent() - F2NEAREST(otfSuperscriptBaselineDropMax * fScale));
      nDY = nSuperShiftUp + boxSuper.Ascent();
   }
   void _GetSubscriptRelPos(CMathItem* pBase, float fScale, const STexBox& boxSubs, const STexBox* pBoxSuper,
                              OUT int32_t& nDY, IN OUT int32_t& nSuperShiftUp) {
      int32_t nSubShiftDown = F2NEAREST(otfSubscriptShiftDown * fScale);
      nSubShiftDown = max(nSubShiftDown, boxSubs.Ascent() - F2NEAREST(otfSubscriptTopMax * fScale));
      nSubShiftDown = max(nSubShiftDown, F2NEAREST(otfSubscriptBaselineDropMin * fScale) + pBase->Box().Depth());
      if (pBoxSuper) {
         int32_t nSubSuperGap = (nSuperShiftUp - pBoxSuper->Depth()) + (nSubShiftDown - boxSubs.Ascent());
         if (nSubSuperGap < F2NEAREST(otfSubSuperscriptGapMin * fScale)) {
            int32_t nD = F2NEAREST(otfSuperscriptBottomMaxWithSubscript * fScale) - (nSuperShiftUp - pBoxSuper->Depth());
            if (nD > 0) {
               nD = min(nD, F2NEAREST(otfSubSuperscriptGapMin * fScale) - nSubSuperGap);
               nSubSuperGap += nD;
               nSuperShiftUp += nD;
            }
            nD = F2NEAREST(otfSubSuperscriptGapMin * fScale) - nSubSuperGap;
            if (nD > 0)
               nSubShiftDown += nD;
         }
      }
      nDY = nSubShiftDown - boxSubs.Ascent();
   }
}

CMathItem* CIndexedBuilder::BuildIndexed(const CMathStyle& style, float fUserScale, CMathItem* pBase, 
                                          CMathItem* pSupers, CMathItem* pSubs) {
   _ASSERT_RET(pBase && (pSupers || pSubs), nullptr);
   float fScale = fUserScale * style.StyleScale();
   CContainerItem* pRetBox = new CContainerItem(eacINDEXED, style);
   pRetBox->AddBox(pBase, 0, 0);
   uint16_t nItalicCorrection = 0;
   //bool bBaseIsSym = false; //not in use?
   if (pBase->Type() == eacWORD || pBase->Type() == eacBIGOP) {
      const CGlyphRun& glyphRun = static_cast<CWordItem*>(pBase)->GlyphRun();
      const SDWGlyph& dwgLast = glyphRun.Glyphs().back();
      const SLMMGlyph* pLMMGlyph = glyphRun.GetFontIdx() == FONT_LMM ?
         g_LMFManager.GetLMMGlyphByIdx(FONT_LMM, dwgLast.index) :
         g_LMFManager.GetLMMGlyph(glyphRun.GetFontIdx(), dwgLast.codepoint);
      if (pLMMGlyph)
         nItalicCorrection = F2NEAREST(pLMMGlyph->nItalCorrection * fScale);
   }
   int32_t nSuperShiftUp = 0;
   int32_t nSuperDX, nSuperDY;
   if (pSupers) {
      if (pBase->IndexPlacement() == eipOverscript || pBase->IndexPlacement() == eipOverUnderscript) {
         //Overscript/UpperLimit
         nSuperDX = (nItalicCorrection + pBase->Box().Width() - pSupers->Box().Width())/2;
         nSuperDY = pBase->Box().BaselineY() + F2NEAREST(otfUpperLimitGapMin * fScale) + 
            pSupers->Box().Height(); //q&d
      }
      else {
         nSuperDX = pBase->Box().Right();
         _GetSuperscriptRelPos(pBase, style.IsCramped(), fScale, pSupers->Box(), nSuperDY);
         nSuperShiftUp = nSuperDY - pSupers->Box().Ascent();
      }
   }
   if (pSubs) {
      int32_t nSubDX, nSubDY;
      if (pBase->IndexPlacement() == eipUnderscript || pBase->IndexPlacement() == eipOverUnderscript) {
         //Underscript/BottomLimit
         nSubDX = (-nItalicCorrection + pBase->Box().Width() - pSubs->Box().Width()) / 2;;
         nSubDY = pBase->Box().Bottom() - pBase->Box().BaselineY() + F2NEAREST(otfLowerLimitGapMin * fScale); //q&d
      }
      else {
         nSubDX = pBase->Box().Right() - nItalicCorrection;
         _GetSubscriptRelPos(pBase, fScale, pSubs->Box(), pSupers ? &pSupers->Box() : nullptr,
                              nSubDY, nSuperShiftUp);
         if (pSupers)
            nSuperDY = nSuperShiftUp + pSupers->Box().Ascent();//corrected
      }
      pRetBox->AddBox(pSubs, nSubDX, pBase->Box().BaselineY() + nSubDY);
   }
   if (pSupers)
      pRetBox->AddBox(pSupers, nSuperDX, pBase->Box().BaselineY() - nSuperDY);
   bool bAddSpace = pSupers && pBase->IndexPlacement() != eipOverscript && pBase->IndexPlacement() != eipOverUnderscript;
   if(!bAddSpace)
      bAddSpace = pSubs && pBase->IndexPlacement() != eipUnderscript && pBase->IndexPlacement() != eipOverUnderscript;
   if(bAddSpace)
      pRetBox->AddWidth(F2NEAREST(otfSpaceAfterScript * fScale));
   pRetBox->NormalizeOrigin(0, 0);
   return pRetBox;
}
