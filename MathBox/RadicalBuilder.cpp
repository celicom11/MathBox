#include "stdafx.h"
#include "RadicalBuilder.h"
#include "WordItem.h"
#include "FillerItem.h"
#include "ContainerItem.h"
#include "LMFontManager.h"

extern CLMFontManager g_LMFManager;
//static helpers
namespace {
   // LMM radical variant indexes
   const UINT16 lmmRadical    = 3077;     // base glyph, 0x221A
   const UINT16 lmmRadical_btm =3078;     // radical assembly btm, 0x23b7
   const UINT16 lmmRadical_ex = 3079;     // vert extender, not used!
   const UINT16 lmmRadical_tp = 3080;     // extender's top
   const UINT16 lmmRadical_v1 = 3081;
   const UINT16 lmmRadical_v2 = 3082;
   const UINT16 lmmRadical_v3 = 3083;
   const UINT16 lmmRadical_v4 = 3084;
   // LMM radical dimensions 
   const int32_t lmmRadicalH_Em = 1000;
   const int32_t lmmRadicalH_v1Em = 1200;
   const int32_t lmmRadicalH_v2Em = 1800;
   const int32_t lmmRadicalH_v3Em = 2400;
   const int32_t lmmRadicalH_v4Em = 3000;
   const int32_t lmmRadicalBtmH_Em = 1820;
   const int32_t lmmRadicalTopH_Em = 620;   // Note: lmmRadicalBtmH_Em+lmmRadicalTopH_Em << lmmRadicalH_v4Em!
   const int32_t lmmRadicalBtmL_Em = 702;
   const int32_t lmmRadicalBtmR_Em = 742;
   //

   //returns radical's variant glyph index
   UINT16 _getRadicalVariant(int32_t nTargetHeightEM) {
      if (nTargetHeightEM <= lmmRadicalH_Em)
         return lmmRadical; //default
      if (nTargetHeightEM <= lmmRadicalH_v1Em)
         return lmmRadical_v1;
      if (nTargetHeightEM <= lmmRadicalH_v2Em)
         return lmmRadical_v2;
      if (nTargetHeightEM <= lmmRadicalH_v3Em)
         return lmmRadical_v3;
      if (nTargetHeightEM <= lmmRadicalH_v4Em)
         return lmmRadical_v4;
      //assembly is needed
      return 0;
   }

}
CMathItem* CRadicalBuilder::BuildRadical(const CMathStyle& style, float fUserScale, CMathItem* pRadicand, CMathItem* pRadDegree) {
   float fScale = fUserScale * style.StyleScale();
   const int32_t nRadicandHEm = pRadicand->Box().Height();
   const int32_t nTargetEM = F2NEAREST(nRadicandHEm / fScale) + 2 * otfRadicalRuleThickness +
      (style.Style() == etsDisplay ? otfRadicalDisplayStyleVerticalGap : otfRadicalVerticalGap);
   //Radical choice
   UINT16 nRadicalVariant = _getRadicalVariant(nTargetEM);
   if (nRadicalVariant)
      return BuildSimpleRadical_(style, fUserScale, pRadicand, nRadicalVariant, pRadDegree);
   //else
   return AssembleRadical_(style, fUserScale, pRadicand, pRadDegree);
}
CMathItem* CRadicalBuilder::BuildSimpleRadical_(const CMathStyle& style, float fUserScale, CMathItem* pRadicand,
   UINT16 nRadicalVariant, CMathItem* pRadDegree) {
   float fScale = fUserScale * style.StyleScale();

   // Build GlyphRun/RadicalSignBox with the lmmRadical
   CWordItem* pRSign = new CWordItem(0, style, eacWORD, fUserScale);
   pRSign->SetGlyphIndexes({ nRadicalVariant });
   //Build overbar line
   CFillerItem* pRadicalOverbar =
      new CFillerItem(pRadicand->Box().InkRight() + F2NEAREST(60 * fScale),
         F2NEAREST(otfRadicalRuleThickness * fScale));
   //4. Build return CContainerItem container with RadicalSignBox,RadicandBox and RadicalOverbar properly positioned
   CContainerItem* pRetBox = new CContainerItem(eacRADICAL, style);
   pRetBox->AddBox(pRSign, 0, 0);
   if (pRadDegree) {
      //Position Degree box
      int32_t nDegreeX = -F2NEAREST(otfRadicalKernAfterDegree * fScale + pRadDegree->Box().Width() + pRSign->Box().LBearing());
      int32_t nDegreeY = pRSign->Box().Height() * (100 - otfRadicalDegreeBottomRaisePercent) / 100 - pRadDegree->Box().Height();
      pRetBox->AddBox(pRadDegree, nDegreeX, nDegreeY);
   }
   pRetBox->AddBox(pRadicalOverbar, pRSign->Box().Width(), -F2NEAREST(otfRadicalRuleThickness * fScale));
   int32_t nRadcandTop = 2 * otfRadicalRuleThickness +
      (style.Style() == etsDisplay ? otfRadicalDisplayStyleVerticalGap : otfRadicalVerticalGap);
   //scale it!
   nRadcandTop = F2NEAREST(nRadcandTop * fScale);
   nRadcandTop = max(nRadcandTop, (pRSign->Box().Height() - pRadicand->Box().Height()) / 2);

   pRetBox->AddBox(pRadicand, pRSign->Box().Width() + pRSign->Box().RBearing(), nRadcandTop, true);
   pRetBox->NormalizeOrigin(0, 0);
   return pRetBox;//stub
}
CMathItem* CRadicalBuilder::AssembleRadical_(const CMathStyle& style, float fUserScale, CMathItem* pRadicand, CMathItem* pRadDegree) {
   const UINT32 nRadicalBottomCode = 0x23B7; // ⎷ bottom piece
   float fScale = fUserScale * style.StyleScale();
   bool bDisplayStyle = (style.Style() == etsDisplay);
   //1. Deduce optimal glyph variant of DEFFONTPTSIZE font for root sign to be slightly taller than Radicand
   const int32_t nTopGapUnscaled = 2 * otfRadicalRuleThickness +
      (bDisplayStyle ? otfRadicalDisplayStyleVerticalGap + 4 * otfRadicalRuleThickness : otfRadicalVerticalGap);
   int32_t nTargetHEM = pRadicand->Box().Height() + F2NEAREST(nTopGapUnscaled * fScale);
   if (bDisplayStyle)
      nTargetHEM += F2NEAREST(6 * otfRadicalRuleThickness * fScale);//extra gap at the bottom
   //2. Build bottom, and top glyphs
   CWordItem* pRSignBtm = new CWordItem(0, style, eacWORD, fUserScale);
   pRSignBtm->SetText({ nRadicalBottomCode });
   CWordItem* pRSignTop = new CWordItem(0, style, eacUNK, fUserScale);
   pRSignTop->SetGlyphIndexes({ lmmRadical_tp });
   //3. Build vertical line extender (test: do not use lmmRadical_ex!)
   int32_t nExtenderHEM = nTargetHEM - F2NEAREST(fScale * (lmmRadicalBtmH_Em + lmmRadicalTopH_Em));
   CFillerItem* pVLine = nullptr;
   if (nExtenderHEM > 0)
      pVLine = new CFillerItem(F2NEAREST(fScale * (lmmRadicalBtmR_Em - lmmRadicalBtmL_Em + 20)),
         nExtenderHEM);//+ F2NEAREST(fScale * 60)
   //4. Build overbar line
   CFillerItem* pRadicalOverbar = new CFillerItem(pRadicand->Box().InkRight() + F2NEAREST(fScale * 60),
      F2NEAREST(fScale * otfRadicalRuleThickness));
   //5. Build return Box
   CContainerItem* pRetBox = new CContainerItem(eacRADICAL, style);
   pRetBox->AddBox(pRSignTop, 0, 0);
   pRetBox->AddBox(pRadicalOverbar, pRSignTop->Box().Width(), 0);
   int32_t nTop = pRSignTop->Box().Height();
   if (pVLine) {
      pRetBox->AddBox(pVLine, F2NEAREST(pRSignTop->Box().LBearing()), nTop);
      nTop += nExtenderHEM;
   }
   pRetBox->AddBox(pRSignBtm, 0, nTop);
   nTop += pRSignBtm->Box().Height();
   if (pRadDegree) {
      //Position Degree box
      int32_t nDegreeX = -F2NEAREST(otfRadicalKernAfterDegree * fScale + pRadDegree->Box().Width() + pRSignBtm->Box().LBearing());
      int32_t nDegreeY = pRSignBtm->Box().Height() + pRSignTop->Box().Height();
      if (pVLine)
         nDegreeY += pVLine->Box().Height() / 2;
      pRetBox->AddBox(pRadDegree, nDegreeX,
         nTop - nDegreeY * otfRadicalDegreeBottomRaisePercent / 100 - pRadDegree->Box().Height());
   }

   //pRetBox->AddBox(pRadicand, pRSignBtm->Box().Width() - pRSignBtm->Box().RBearing() / 2, F2NEAREST(fScale* nTopGapUnscaled), true);
   pRetBox->AddBox(pRadicand, pRSignBtm->Box().Width(), F2NEAREST(fScale * nTopGapUnscaled), true);
   pRetBox->NormalizeOrigin(0, 0);
   return pRetBox;//stub
}
