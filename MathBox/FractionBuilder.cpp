#include "stdafx.h"
#include "FractionBuilder.h"
#include "ContainerItem.h"
#include "FillerItem.h"

bool CFractionBuilder::CanTakeCommand(PCSTR szCmd) const {
   _ASSERT_RET(szCmd && *szCmd == '\\', false);
   ++szCmd; // Skip backslash
   return 0 == strcmp(szCmd, "frac");
}
CMathItem* CFractionBuilder::BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) {
   _ASSERT_RET(szCmd && pParser, nullptr);
   SParserContext ctx(pParser->GetContext());
   _ASSERT_RET(CanTakeCommand(szCmd), nullptr);

   // Numerator style
   ctx.currentStyle.Decrease();
   // 1. Consume numerator {expr}
   CMathItem* pNumerator = pParser->ConsumeItem(elcapFig, ctx);
   if (!pNumerator) {
      if (!pParser->HasError())
         pParser->SetError("Missing numerator for \\frac");
      return nullptr;
   }

   // 2. Consume denominator {expr}
   // Denominator style: same as numerator but CRAMPED
   ctx.currentStyle.SetCramped(true);
   CMathItem* pDenominator = pParser->ConsumeItem(elcapFig, ctx);
   if (!pDenominator) {
      delete pNumerator;
      if (!pParser->HasError())
         pParser->SetError("Missing denominator for \\frac");
      return nullptr;
   }

   // 3. Build fraction item
   ctx = pParser->GetContext(); //reset
   return _BuildFraction(ctx.currentStyle, ctx.fUserScale, pNumerator, pDenominator);
}

CMathItem* CFractionBuilder::_BuildFraction(const CMathStyle& style, float fUserScale, CMathItem* pNum, CMathItem* pDenom) {
   float fScale = fUserScale * style.StyleScale();
   bool bDisplayStyle = (style.Style() == etsDisplay);
   CContainerItem* pRetBox = new CContainerItem(eacVBOX, style, etaORD);
   //1. Build fraction rule - it is an anchor!
   CFillerItem* pFracRule =
      new CFillerItem(max(pNum->Box().Width(), pDenom->Box().Width()), F2NEAREST(otfFractionRuleThickness * fScale));
   pRetBox->AddBox(pFracRule, 0, 0);
   //2. Params to calculate the required gaps
   const int32_t nMinNumGap = bDisplayStyle ? otfFractionNumDisplayStyleGapMin : otfFractionNumeratorGapMin;
   const int32_t nMinDenomGap = bDisplayStyle ? otfFractionDenomDisplayStyleGapMin : otfFractionDenominatorGapMin;
   const int32_t nNumBaseToBase = bDisplayStyle ? otfFractionNumeratorDisplayStyleShiftUp : otfFractionNumeratorShiftUp;
   const int32_t nBaseToDenomBase = bDisplayStyle ? otfFractionDenominatorDisplayStyleShiftDown : otfFractionDenominatorShiftDown;
   //3. Numerator, above the rule
   int32_t nNumLeft = 0;
   if (pNum->Box().Width() < pDenom->Box().Width())
      nNumLeft = (pDenom->Box().Width() - pNum->Box().Width()) / 2;
   //distance between Num bottom and rule's top(==0!):
   int32_t nNumShiftUp = 
      max(F2NEAREST((nNumBaseToBase - otfFractionRuleThickness/2 - otfAxisHeight) * fScale - pNum->Box().Depth()),
          F2NEAREST(nMinNumGap * fScale));
   pRetBox->AddBox(pNum, nNumLeft, -nNumShiftUp - pNum->Box().Height());
   //4. Denominator, below the rule
   int32_t nDenomLeft = 0;
   if (pDenom->Box().Width() < pNum->Box().Width())
      nDenomLeft = (pNum->Box().Width() - pDenom->Box().Width()) / 2;
   int32_t nDenomY =
      max(F2NEAREST((otfFractionRuleThickness + nMinDenomGap) * fScale),
          F2NEAREST((otfFractionRuleThickness/2 + otfAxisHeight + nBaseToDenomBase) * fScale) - 
            pDenom->Box().Ascent());
   pRetBox->AddBox(pDenom, nDenomLeft, nDenomY );
   pRetBox->NormalizeOrigin(0, 0);
   pRetBox->SetMathAxis((pFracRule->Box().Top() + pFracRule->Box().Bottom())/2);
   return pRetBox;
}
