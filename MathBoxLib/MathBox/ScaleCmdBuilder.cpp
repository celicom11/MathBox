#include "stdafx.h"
#include "ScaleCmdBuilder.h"

// \fontsize{size pt}{h_skip}\selectfont{arg} 
// \scalefnt{factor}{arg}
// \relscale{factor}{arg}
bool CScaleCmdBuilder::CanTakeCommand(PCSTR szCmd) const {
   _ASSERT_RET(szCmd && *szCmd, false);
   if (*szCmd == '\\')
      ++szCmd;
   string sCmd(szCmd);
   return sCmd == "fontsize" || sCmd == "scalefnt" || sCmd == "relscale";
}

CMathItem* CScaleCmdBuilder::BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) {
   _ASSERT_RET(szCmd && pParser, nullptr);
   const SParserContext& ctx = pParser->GetContext();
   _ASSERT_RET(CanTakeCommand(szCmd), nullptr);
   if (*szCmd == '\\')
      ++szCmd;
   string sCmd(szCmd);
   SParserContext ctxCmd;
   ctxCmd.CopyBasics(pParser->GetContext());
   float fScale;
   string sOrigUnits; //ok if empty
   if(!pParser->ConsumeDimension(elcapFig, fScale, sOrigUnits)) {
      if (!pParser->HasError())
         pParser->SetError("Failed to parse scaling factor for command '\\" + sCmd + "'");
      return nullptr;
   }
   if (sCmd == "fontsize") {
      fScale = fScale / pParser->Doc().DefaultFontSizePts();
      float fInterLine; //not used
      if (!pParser->ConsumeDimension(elcapFig, fInterLine, sOrigUnits)) {
         if (!pParser->HasError())
            pParser->SetError("Failed to parse interline spacing factor for command '\\fontsize'");
         return nullptr;
      }
      string sToken;
      EnumTokenType ettNext = pParser->GetTokenData(sToken);
      if (ettNext != ettCOMMAND || sToken != "\\selectfont") {
         if (!pParser->HasError())
            pParser->SetError("Missing \\selectfont after '\\fontsize'");
         return nullptr;
      }
      pParser->SkipToken();
   }
   ctxCmd.fUserScale *= fScale; //apply scale
   //arg
   CMathItem* pRet = pParser->ConsumeItem(elcapAny, ctxCmd);
   if (!pRet) {
      if (!pParser->HasError())
         pParser->SetError("Failed to parse argumend for scale command '\\" + sCmd + "'");
      return nullptr;
   }

   return pRet;
}

