#include "stdafx.h"
#include "TextCmdBuilder.h"
#include "FontStyleHelper.h"

bool CTextCmdBuilder::CanTakeCommand(PCSTR szCmd) const {
   // text mode + font commands: \text,\textit,etc.
   if (szCmd[0] != '\\')
      return false;
   szCmd++; //skip '\'
   if (!*szCmd)
      return false;
   STextFontStyle tfStyle; //dummy
   return FontStyleHelper::_GetTextFontStyle(szCmd, tfStyle);
}
CMathItem* CTextCmdBuilder::BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) {
   _ASSERT_RET(szCmd && pParser, nullptr);
   const SParserContext& ctx = pParser->GetContext();
   _ASSERT_RET(CanTakeCommand(szCmd), nullptr);
   if (*szCmd == '\\')
      ++szCmd; //skip \\

   SParserContext ctxText(ctx);
   ctxText.bTextMode = true; //text mode
   ctxText.sFontCmd = szCmd;
   if((ctx.sFontCmd == "textit" || ctx.sFontCmd == "emph") && ctxText.sFontCmd == "textbf")
      ctxText.sFontCmd = "textbfit"; //combined
   else if ((ctxText.sFontCmd == "textit" || ctxText.sFontCmd == "emph") && ctx.sFontCmd == "textbf")
      ctxText.sFontCmd = "textbfit"; //combined
   //todo: more replacements to combine fonts?
   CMathItem* pItem = nullptr; 
   if (ctxText.sFontCmd == "verb") {
      //verify next token is ettALNUM
      string sVerbArg;
      EnumTokenType ettNext = pParser->GetTokenData(sVerbArg);
      if (ettNext != ettALNUM) {
         _ASSERT(0);//snbh!
         pParser->SetError("Unexpected argument for \\verb command");
      }
      else
         pItem = pParser->ConsumeItem(elcapAny, ctxText);
   }
   else
      pItem = pParser->ConsumeItem(elcapFig, ctxText);
   if (!pItem) {
      if (!pParser->HasError())
         pParser->SetError("Missing {arg} for '\\"+ string(szCmd) +"' command");
   }
   return pItem;
}
