#include "stdafx.h"
#include "ShiftBuilder.h"

// \raise <dimen> {arg}          TeX vertical
// \lower <dimen> {arg}          TeX vertical
// \moveleft <dimen> {arg}       TeX horizontal
// \moveright <dimen> {arg}      TeX horizontal
// \raisebox<dimen>{arg}         LaTeX vertical
bool CShiftBuilder::CanTakeCommand(PCSTR szCmd) const {
   _ASSERT_RET(szCmd && *szCmd, false);
   if (szCmd[0] == '\\')
      ++szCmd;
   if (!*szCmd)
      return false;
   string sCmd(szCmd);
   return (sCmd == "raise" || sCmd == "lower" || sCmd == "raisebox" || //horiz-mode!
           sCmd == "moveleft" || sCmd == "moveright");   //vert mode!
}
CMathItem* CShiftBuilder::BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) {
   _ASSERT_RET(szCmd && *szCmd && pParser, nullptr);
   _ASSERT_RET(CanTakeCommand(szCmd), nullptr);
   string sCmd(szCmd + 1);
   SParserContext ctxArg = pParser->GetContext();
   float fShift1Pts = 0.0f;
   string sOrigUnits;
   if(!pParser->ConsumeDimension(elcapAny, fShift1Pts, sOrigUnits)) {
      if (!pParser->HasError())
         pParser->SetError("Missing/bad dimension argument for \\" + sCmd + "command");
      return nullptr;
   }
   if(sOrigUnits.empty()) {
      pParser->SetError("Units are required in \\" + sCmd + " command");
      return nullptr;
   }
   if(!ctxArg.bTextMode && sCmd == "raisebox") {
      //switch to text mode for arg
      ctxArg.bTextMode = true;
   }
   CMathItem* pArg = pParser->ConsumeItem(elcapAny, ctxArg);
   if(!pArg) {
      if (!pParser->HasError())
         pParser->SetError("Missing {arg} for \\" + sCmd + " command");
      return nullptr;
   }
   int32_t nShift1 = F2NEAREST(otfUnitsPerEm * fShift1Pts / pParser->Doc().DefaultFontSizePts());

   //apply shifts
   if(sCmd == "raise" || sCmd == "raisebox")
      pArg->ShiftUp(nShift1);
   else if (sCmd == "lower")
      pArg->ShiftUp(-nShift1);
   else if (sCmd == "moveleft")
      pArg->ShiftLeft(nShift1);
   else if (sCmd == "moveright")
      pArg->ShiftLeft(-nShift1);
   return pArg;
}
