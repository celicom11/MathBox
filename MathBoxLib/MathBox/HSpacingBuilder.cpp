#include "stdafx.h"
#include "HSpacingBuilder.h"
#include "GlueItem.h"

bool CHSpacingBuilder::CanTakeCommand(PCSTR szCmd) const {
   _ASSERT_RET(szCmd && *szCmd, false);
   if (*szCmd == '\\')
      ++szCmd;
   if (*szCmd == ' ' || *szCmd == ',' || *szCmd == ':' || *szCmd == ';' || *szCmd == '!' ||
      0 == strcmp(szCmd, "qquad") || 0 == strcmp(szCmd, "quad") || 0 == strcmp(szCmd, "kern") ||
      0 == strcmp(szCmd, "hskip"))
      return true;
   return false;
}
CMathItem* CHSpacingBuilder::BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) {
   _ASSERT_RET(szCmd && pParser, nullptr);
   const SParserContext& ctx = pParser->GetContext();
   _ASSERT_RET(CanTakeCommand(szCmd), nullptr);
   string sCmd(szCmd);
   
   STexGlue glue;
   if (sCmd[1] == ' ' || sCmd == "\\quad") //1em
      glue.fNorm = glue.fActual = MU2EM(18);
   else if (sCmd[1] == ',')//3mu
      glue.fNorm = glue.fActual = MU2EM(3);
   else if (sCmd[1] == ':')//4mu
      glue.fNorm = glue.fActual = MU2EM(4);
   else if (sCmd[1] == ';')//5mu
      glue.fNorm = glue.fActual = MU2EM(5);
   else if (sCmd[1] == '!')//-3mu
      glue.fNorm = glue.fActual = MU2EM(-3);
   else if (sCmd == "\\qquad")//2 em
      glue.fNorm = glue.fActual = MU2EM(36);
   else if (sCmd == "\\kern") {
      float fPts = 0.0f;
      string sOrigUnits;
      if (!pParser->ConsumeDimension(elcapAny, fPts, sOrigUnits)) {
         if (!pParser->HasError())
            pParser->SetError("Failed to parse \\kern arguments");
         return nullptr;
      }
      if(sOrigUnits.empty()) {
         pParser->SetError("Units are required in \\kern command");
         return nullptr;
      }
      //convert to EM/FDU!
      glue.fNorm = otfUnitsPerEm * fPts / pParser->Doc().DefaultFontSizePts();
   }
   else if (sCmd == "\\hskip") {
      if (!pParser->ConsumeHSkipGlue(glue)) {
         if (!pParser->HasError())
            pParser->SetError("Failed to parse \\hskip arguments");
         return nullptr;
      }
   }
   return new CGlueItem(pParser->Doc(), glue, ctx.currentStyle, ctx.fUserScale);
}

