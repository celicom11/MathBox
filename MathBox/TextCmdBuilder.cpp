#include "stdafx.h"
#include "TextCmdBuilder.h"
#include "LMFontManager.h"

bool CTextCmdBuilder::CanTakeCommand(PCSTR szCmd) const {
   // text mode + font commands: \text,\textit,etc.
   if (szCmd[0] != '\\')
      return false;
   szCmd++; //skip '\'
   if (!*szCmd)
      return false;
   STextFontStyle tfStyle; //dummy
   return CLMFontManager::_GetTextFontStyle(szCmd, tfStyle);
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
   CMathItem* pItem = pParser->ConsumeItem(elcapFig, ctxText);
   if (!pItem) {
      if (!pParser->HasError())
         pParser->SetError("Missing {arg} for '\\"+ string(szCmd) +"' command");
   }
   return pItem;
}
