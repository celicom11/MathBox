#include "stdafx.h"
#include "FractionBuilder.h"
#include "ContainerItem.h"
#include "FillerItem.h"

bool CFractionBuilder::CanTakeCommand(PCSTR szCmd) const {
   _ASSERT_RET(szCmd && *szCmd == '\\', false);
   ++szCmd; // Skip backslash
   return (strcmp(szCmd, "frac") == 0);
}
CMathItem* CFractionBuilder::BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) {
   _ASSERT_RET(szCmd && pParser, nullptr);
   _ASSERT_RET(CanTakeCommand(szCmd), nullptr);

   // Get current context
   CMathStyle styleBase = pParser->GetCurrentStyle();
   float fUserScale = pParser->GetUserScale();
   CMathStyle styleNumerator(styleBase);
   styleNumerator.Decrease();
   CMathStyle styleDenom(styleNumerator);
   styleDenom.SetCramped(true);
   // 1. Consume numerator {expr}

   CMathItem* pNumerator = pParser->ConsumeItem(elcapFig, &styleNumerator);
   if (!pNumerator) {
      if (!pParser->HasError())
         pParser->SetError("Missing numerator for \\frac");
      return nullptr;
   }

   // 2. Consume denominator {expr}
   //    Denominator style: same as numerator but CRAMPED
   CMathItem* pDenominator = pParser->ConsumeItem(elcapFig, &styleDenom);
   if (!pDenominator) {
      delete pNumerator;
      if (!pParser->HasError())
         pParser->SetError("Missing denominator for \\frac");
      return nullptr;
   }

   // 3. Build fraction item
   return _BuildFraction(styleBase, fUserScale, pNumerator, pDenominator);
}

CMathItem* CFractionBuilder::_BuildFraction(const CMathStyle& style, float fUserScale, CMathItem* pNum, CMathItem* pDenom) {
   float fScale = fUserScale * style.StyleScale();
   bool bDisplayStyle = (style.Style() == etsDisplay);
   CContainerItem* pRetBox = new CContainerItem(eacVBOX, style, etaORD);
   //1. Build fraction rule - it is an anchor!
   CFillerItem* pFracRule =
      new CFillerItem(max(pNum->Box().Width(), pDenom->Box().Width()), F2NEAREST((2 * otfFractionRuleThickness) * fScale));
   pRetBox->AddBox(pFracRule, 0, 0);
   //2. Params to calculate the required gaps
   const int32_t nMinNumGap = bDisplayStyle ? otfFractionNumDisplayStyleGapMin : otfFractionNumeratorGapMin;
   const int32_t nMinDenomGap = bDisplayStyle ? otfFractionDenomDisplayStyleGapMin : otfFractionDenominatorGapMin;
   const int32_t nNumBaseToBase = bDisplayStyle ? otfFractionNumeratorDisplayStyleShiftUp : otfFractionNumeratorShiftUp;
   const int32_t nBaseToDenomBase = bDisplayStyle ? otfFractionDenominatorDisplayStyleShiftDown : otfFractionDenominatorShiftDown;
   //3. place Numerator above the rule
   int32_t nNumLeft = 0;
   if (pNum->Box().Width() < pDenom->Box().Width())
      nNumLeft = (pDenom->Box().Width() - pNum->Box().Width()) / 2;
   int32_t nNumShiftUp = max(pRetBox->Box().BaselineY() + F2NEAREST(nMinNumGap * fScale) + pNum->Box().Depth(),
      F2NEAREST(nNumBaseToBase * fScale));
   pRetBox->AddBox(pNum, nNumLeft, pRetBox->Box().BaselineY() - nNumShiftUp - pNum->Box().Ascent());
   //4. place Denominator below the rule
   int32_t nDenomLeft = 0;
   if (pDenom->Box().Width() < pNum->Box().Width())
      nDenomLeft = (pNum->Box().Width() - pDenom->Box().Width()) / 2;
   int32_t nDenomY =
      max(F2NEAREST((3 * otfFractionRuleThickness + nMinDenomGap) * fScale),
         pRetBox->Box().BaselineY() + F2NEAREST(nBaseToDenomBase * fScale) - pDenom->Box().Ascent());
   pRetBox->AddBox(pDenom, nDenomLeft, nDenomY);
   pRetBox->NormalizeOrigin(0, 0);
   pRetBox->SetMathAxis((pFracRule->Box().Top() + pFracRule->Box().Bottom())/2);
   return pRetBox;
}
