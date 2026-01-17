#include "stdafx.h"
#include "ShiftBuilder.h"

// \raise <dimen> {arg}          TeX vertical
// \lower <dimen> {arg}          TeX vertical
// \moveleft <dimen> {arg}       TeX horizontal
// \moveright <dimen> {arg}      TeX horizontal
// \raisebox<dimen>{arg}         LaTeX vertical
// \movebox<dimen><dimen>{arg}   MathBox 2D
bool CShiftBuilder::CanTakeCommand(PCSTR szCmd) const {
   _ASSERT_RET(szCmd && *szCmd, false);
   if (szCmd[0] == '\\')
      ++szCmd;
   if (!*szCmd)
      return false;
   string sCmd(szCmd);
   return (sCmd == "raise" || sCmd == "lower" ||
           sCmd == "moveleft" || sCmd == "moveright" ||
      sCmd == "raisebox" || sCmd == "movebox");
}
CMathItem* CShiftBuilder::BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) {
   _ASSERT_RET(szCmd && *szCmd && pParser, nullptr);
   _ASSERT_RET(CanTakeCommand(szCmd), nullptr);
   string sCmd(szCmd + 1);
   SParserContext ctxArg = pParser->GetContext();
   float fShift1Pts = 0.0f;
   if(!pParser->ConsumeDimension(elcapAny, fShift1Pts)) {
      if (!pParser->HasError())
         pParser->SetError("Missing/bad dimension argument for \\" + sCmd + "command");
      return nullptr;
   }
   float fShift2Pts = 0.0f;
   if(sCmd == "movebox") {
      if (!pParser->ConsumeDimension(elcapAny, fShift2Pts)) {
         if (!pParser->HasError())
            pParser->SetError("Missing/bad 2nd dimension argument for \\movebox command");
         return nullptr;
      }
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
   else {//movebox
      int32_t nShift2 = F2NEAREST(otfUnitsPerEm * fShift2Pts / pParser->Doc().DefaultFontSizePts());
      pArg->ShiftLeft(-nShift1);
      pArg->ShiftUp(-nShift2);
   }
   return pArg;
}
