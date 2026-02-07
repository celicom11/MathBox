#include "stdafx.h"
#include "MathStyleBuilder.h"

bool CMathStyleBuilder::CanTakeCommand(PCSTR szCmd) const {
   _ASSERT_RET(szCmd && *szCmd, false);
   if (szCmd[0] == '\\')
      ++szCmd;
   if (!*szCmd)
      return false;
   string sCmd(szCmd);
   return sCmd == "ensuremath";

}
CMathItem* CMathStyleBuilder::BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) {
   _ASSERT_RET(szCmd && pParser, nullptr);
   _ASSERT_RET(CanTakeCommand(szCmd), nullptr);
   if (*szCmd == '\\')
      ++szCmd; //skip '\'
   string sCmdStr(szCmd);
   if (sCmdStr != "ensuremath") {
      pParser->SetError("Unknown class command: \\" + sCmdStr);
      return nullptr;
   }
   SParserContext ctx = pParser->GetContext();
   if(ctx.bTextMode)
      ctx.bTextMode = false; //switch to MATH mode!
   CMathItem* pItem = pParser->ConsumeItem(elcapFig, ctx);
   if (!pItem) {
      if (!pParser->HasError())
         pParser->SetError("Missing {arg} for '\\" + sCmdStr + "' command");
      return nullptr;
   }
   return pItem;
}
