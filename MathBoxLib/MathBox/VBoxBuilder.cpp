#include "stdafx.h"
#include "VBoxBuilder.h"
#include "ExtGlyphBuilder.h"
#include "HBoxItem.h"

namespace {
   CContainerItem* _BuildVBox(IN OUT vector<CMathItem*>& vItems, int32_t nVKern) {
      _ASSERT_RET(!vItems.empty(), nullptr);
      CContainerItem* pRetBox = new CContainerItem(vItems.front()->Doc(), eacVBOX, CMathStyle(), etaORD);
      int32_t nWidth = 0;
      for (const CMathItem* pItem : vItems) {
         nWidth = max(nWidth, pItem->Box().Width());
      }
      _ASSERT_RET(nWidth > 0, nullptr);//snbh!
      CMathItem* pLastItem = nullptr;
      for (CMathItem* pItem : vItems) {
         pRetBox->AddBox(pItem, (nWidth - pItem->Box().Width())/2, (pLastItem ? pLastItem->Box().Bottom() + nVKern : 0));
         pLastItem = pItem;
      }
      pRetBox->NormalizeOrigin(0, 0);
      return pRetBox;
   }
}

//IMathItemBuilder 
bool CVBoxBuilder::CanTakeCommand(PCSTR szCmd) const {
   _ASSERT_RET(szCmd && *szCmd, false);
   if( szCmd[0] == '\\' )
      ++szCmd;
   if (!*szCmd)
      return false;
   string sCmd(szCmd);
   const bool bIsBinomCmd = sCmd == "binom" || sCmd == "tbinom" || sCmd == "dbinom";
   return sCmd == "underset" || sCmd == "overset" || sCmd == "stackrel" ||
      bIsBinomCmd || sCmd == "substack" || sCmd == "stackunder";
}
CMathItem* CVBoxBuilder::BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) {
   _ASSERT_RET(szCmd && *szCmd && pParser, nullptr);
   const SParserContext& ctx = pParser->GetContext();
   _ASSERT_RET(CanTakeCommand(szCmd), nullptr);
   string sCmd(szCmd+1);
   // Get current context
   CMathItem *pTop, *pBottom, *pRet = nullptr;
   int16_t nAnchor;
   int32_t nVKern = 0;
   EnumTexAtom eAtom = etaORD;
   const bool bIsBinomCmd = sCmd == "binom" || sCmd == "tbinom" || sCmd == "dbinom";
   if (sCmd == "underset" || sCmd == "overset" || sCmd == "stackrel" || bIsBinomCmd) {
      SParserContext ctxArg(ctx);
      if (sCmd == "dbinom") {
         ctxArg.currentStyle.SetStyle(etsDisplay);
      }
      else if (sCmd == "binom" && ctxArg.currentStyle.Style() == etsDisplay)
         ; //keep display style
      else
         ctxArg.currentStyle.ToSuperscriptStyle();
      //2 arg items : {}{}
      CMathItem* pArg1 = pParser->ConsumeItem(elcapFig, ctxArg);
      if (!pArg1) {
         if (!pParser->HasError())
            pParser->SetError("Missing {arg1} for command '" + sCmd + "'");
         return nullptr;
      }
      CMathItem* pArg2 = pParser->ConsumeItem(elcapFig, 
         ((sCmd == "binom"|| sCmd == "tbinom"|| sCmd == "dbinom") ? ctxArg : ctx));
      if (!pArg2) {
         if (!pParser->HasError())
            pParser->SetError("Missing {arg2} for command '" + sCmd + "'");
         return nullptr;
      }
      if (sCmd == "underset") {
         pTop = pArg2;
         pBottom = pArg1;
         nAnchor = 0; //top is a base
         nVKern = F2NEAREST(5 * otfFractionRuleThickness * ctx.currentStyle.StyleScale() * ctx.fUserScale);

      }
      else if (sCmd == "overset") {
         pTop = pArg1;
         pBottom = pArg2;
         nAnchor = 1; //bottom is a base
         nVKern = F2NEAREST(5 * otfFractionRuleThickness * ctx.currentStyle.StyleScale() * ctx.fUserScale);

      }
      else if (sCmd == "stackrel") {
         pTop = pArg1;
         pBottom = pArg2;  //todo: check if eacWord with 1 etaREL glyph?
         nAnchor = 1;      //bottom is a base
         eAtom = etaREL;   //relation
         nVKern = F2NEAREST((ctx.currentStyle.Style() == etsDisplay ? 180 : otfStackGapMin) * ctx.fUserScale);
      }
      else { //binom
         pTop = pArg1;
         pBottom = pArg2;
         nAnchor = 2;      //axis!
         nVKern = F2NEAREST(6 * otfFractionRuleThickness * ctx.currentStyle.StyleScale() * ctx.fUserScale);//q&d
      }
      pRet = BuildBox_(pTop, pBottom, nAnchor, nVKern, eAtom);
      if (bIsBinomCmd) {//add brackets
         CHBoxItem* pHBox = new CHBoxItem(pParser->Doc(), ctx.currentStyle);
         int32_t nAxisY = pRet->Box().AxisY();
         int32_t nSize = 2 * max(nAxisY - pRet->Box().Top(), pRet->Box().Bottom() - nAxisY);
         CExtGlyphBuilder egBuilder(pParser->Doc());
         CMathItem* pOpen = egBuilder.BuildVerticalGlyph('(', ctx.currentStyle, nSize, ctx.fUserScale);
         _ASSERT_RET(pOpen, nullptr);
         CMathItem* pClose = egBuilder.BuildVerticalGlyph(')', ctx.currentStyle, nSize, ctx.fUserScale);
         _ASSERT_RET(pClose, nullptr);
         pHBox->AddItem(pOpen);
         pHBox->AddItem(pRet);
         pHBox->AddItem(pClose);
         pHBox->Update();
         pRet = pHBox;
      }
   }
   else if (sCmd == "substack") {
      // ~= subarray{c}
      SParserContext ctxArg(ctx);
      ctxArg.currentStyle.ToSuperscriptStyle();

      CMathItem* pArg = pParser->ConsumeItem(elcapFig, ctxArg);
      if (!pArg) {
         if (!pParser->HasError())
            pParser->SetError("Missing {arg} for command '\\substack'");
         return nullptr;
      }
      if (pArg->Type() != eacLINES) {
         _ASSERT(0); //snbh 
         if (!pParser->HasError())
            pParser->SetError("Unexpected type of the {arg} for command '\\substack'");
         delete pArg;
         return nullptr;
      }
      CContainerItem* pCnt = static_cast<CContainerItem*>(pArg); 
      if (pCnt->Items().empty()) {
         delete pArg;
         return nullptr;//ntd
      }
      vector<CMathItem*> vItems;
      for (CMathItem* pItem : pCnt->Items()) {
         vItems.push_back(pItem);
      }
      pCnt->Clear(true);//dnd moved items!
      nVKern = F2NEAREST((ctx.currentStyle.Style() == etsDisplay ? 180 : otfStackGapMin) * ctx.fUserScale);
      CContainerItem* pVBox = _BuildVBox(vItems, nVKern);
      pVBox->SetMathAxis(pVBox->Box().Height() / 2); // subarray axis?
      pRet = pVBox;
      delete pArg; //items moved to vItems
   }
   else if (sCmd == "stackunder") {
      //[dim]{arg1}{arg2}
      float fKern;
      string sOrigUnits;
      if (!pParser->ConsumeDimension(elcapSquare, fKern, sOrigUnits)) {
         // Read from macro variable
         if (pParser->GetLength("stackundergap", fKern))
            nVKern = F2NEAREST(fKern * ctx.fUserScale);
         else
            nVKern = F2NEAREST(290 * ctx.fUserScale); // default
      } else {
         nVKern = F2NEAREST(otfUnitsPerEm * fKern * ctx.fUserScale /pParser->Doc().DefaultFontSizePts());
      }
      pTop = pParser->ConsumeItem(elcapFig, ctx);
      if (!pTop) {
         if (!pParser->HasError())
            pParser->SetError("Missing {arg1} for command '\\stackunder'");
         return nullptr;
      }
      pBottom = pParser->ConsumeItem(elcapFig, ctx);
      if (!pBottom) {
         if (!pParser->HasError())
            pParser->SetError("Missing {arg2} for command '\\stackunder'");
         return nullptr;
      }
      nAnchor = 0;      //top is a base
      pRet = BuildBox_(pTop, pBottom, nAnchor, nVKern, eAtom);
   }
   else
      _ASSERT(0);//snbh!
   return pRet;
}
//helpers
CMathItem* CVBoxBuilder::BuildBox_(CMathItem* pTop, CMathItem* pBottom,
   int16_t nAnchor, int32_t nVKern, EnumTexAtom eAtom) {
   _ASSERT_RET(pTop && pBottom, nullptr);
   _ASSERT_RET(nAnchor >= 0 && nAnchor <= 2, nullptr);
   CContainerItem* pRetBox;
   int32_t nWidth = max(pTop->Box().Width(), pBottom->Box().Width());
   if (pTop->Type() == eacVBOX)
      pRetBox = static_cast<CContainerItem*>(pTop);
   else {
      pRetBox = new CContainerItem(pTop->Doc(), eacVBOX, CMathStyle(), eAtom);
      pRetBox->AddBox(pTop, (nWidth - pTop->Box().Width()) / 2, 0);
   }
   int32_t nBottomY = pTop->Box().Height() + nVKern;
   pRetBox->AddBox(pBottom, (nWidth - pBottom->Box().Width()) / 2, nBottomY);
   pRetBox->NormalizeOrigin(0, 0);
   //set VBox Baseline
   int32_t nAscent = 0;
   if (nAnchor == 0) //top
      pRetBox->SetAscent(pTop->Box().BaselineY());
   else if (nAnchor == 1) //bottom
      pRetBox->SetAscent(pBottom->Box().BaselineY());
   else //Gap's center is a Math Axis!
      pRetBox->SetMathAxis((pTop->Box().Bottom() + pBottom->Box().Top()) / 2);

   return pRetBox;

}

CMathItem* CVBoxBuilder::_BuildGenFraction(const CMathStyle& style, float fUserScale, 
                                             CMathItem* pNumerator, CMathItem* pDenominator) {

   _ASSERT_RET(pNumerator && pDenominator, nullptr);
   int32_t nVKern = F2NEAREST(5 * otfFractionRuleThickness * style.StyleScale() * fUserScale);
   return BuildBox_(pNumerator, pDenominator, 2, nVKern, etaORD);
}
