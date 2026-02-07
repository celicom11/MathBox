#include "stdafx.h"
#include "FractionBuilder.h"
#include "ContainerItem.h"
#include "RuleItem.h"

bool CFractionBuilder::CanTakeCommand(PCSTR szCmd) const {
   _ASSERT_RET(szCmd && *szCmd == '\\', false);
   ++szCmd; // Skip backslash
   return 0 == strcmp(szCmd, "frac") || 0 == strcmp(szCmd, "tfrac") || 0 == strcmp(szCmd, "dfrac");
}
CMathItem* CFractionBuilder::BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) {
   _ASSERT_RET(szCmd && pParser, nullptr);
   _ASSERT_RET(CanTakeCommand(szCmd), nullptr);
   ++szCmd; //skip '\'
   SParserContext ctx(pParser->GetContext());
   SParserContext ctxParts(ctx);
   // Numerator style
   if (szCmd[0] == 't') {//tfrac
      ctx.currentStyle = etsText;
      ctxParts.currentStyle = etsScript;
   }
   else if (szCmd[0] == 'd') { //dfrac
      ctx.currentStyle = ctxParts.currentStyle = etsDisplay;
   }
   else //frac
      ctxParts.currentStyle.Decrease();
   // 1. Consume numerator {expr}
   CMathItem* pNumerator = pParser->ConsumeItem(elcapFig, ctxParts);
   if (!pNumerator) {
      if (!pParser->HasError())
         pParser->SetError("Missing numerator for \\frac");
      return nullptr;
   }

   // 2. Consume denominator {expr}
   // Denominator style: same as numerator but CRAMPED
   ctxParts.currentStyle.SetCramped(true);
   CMathItem* pDenominator = pParser->ConsumeItem(elcapFig, ctxParts);
   if (!pDenominator) {
      delete pNumerator;
      if (!pParser->HasError())
         pParser->SetError("Missing denominator for \\frac");
      return nullptr;
   }

   // 3. Build fraction item
   return _BuildFraction(pParser->Doc(), ctx, pNumerator, pDenominator);
}

CMathItem* CFractionBuilder::_BuildFraction(IDocParams& doc, const SParserContext& ctx,
                                             CMathItem* pNum, CMathItem* pDenom) {
   float fScale = ctx.EffectiveScale();
   bool bDisplayStyle = (ctx.currentStyle.Style() == etsDisplay);
   CContainerItem* pRetBox = new CContainerItem(doc, eacVBOX, ctx.currentStyle.Style(), etaORD);
   //1. Build fraction rule - it is an anchor!
   CRuleItem* pFracRule =
      new CRuleItem(doc, max(pNum->Box().Width(), pDenom->Box().Width()), F2NEAREST(otfFractionRuleThickness * fScale));
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
