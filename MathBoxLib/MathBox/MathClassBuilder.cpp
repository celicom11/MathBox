#include "stdafx.h"
#include "MathClassBuilder.h"
//\mathord, \mathop, \mathbin, \mathrel, \mathopen, \mathclose, \mathpunct
bool CMathClassBuilder::CanTakeCommand(PCSTR szCmd) const {
   _ASSERT_RET(szCmd && *szCmd, false);
   if (szCmd[0] == '\\')
      ++szCmd;
   if (!*szCmd)
      return false;
   string sCmd(szCmd);
   return (sCmd == "mathord" || sCmd == "mathop" || sCmd == "mathbin" ||
      sCmd == "mathrel" || sCmd == "mathopen" || sCmd == "mathclose" ||
      sCmd == "mathpunct" || sCmd == "ensuremath");

}
CMathItem* CMathClassBuilder::BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) {
   _ASSERT_RET(szCmd && pParser, nullptr);
   _ASSERT_RET(CanTakeCommand(szCmd), nullptr);
   if (*szCmd == '\\')
      ++szCmd; //skip '\'
   EnumTexAtom eClass = etaORD;
   string sCmdStr(szCmd);
   if (sCmdStr == "mathord")
      eClass = etaORD;
   else if (sCmdStr == "mathop")
      eClass = etaOP;
   else if (sCmdStr == "mathbin")
      eClass = etaBIN;
   else if (sCmdStr == "mathrel")
      eClass = etaREL;
   else if (sCmdStr == "mathopen")
      eClass = etaOPEN;
   else if (sCmdStr == "mathclose")
      eClass = etaCLOSE;
   else if (sCmdStr == "mathpunct")
      eClass = etaPUNCT;
   else {
      pParser->SetError("Unknown class command: \\" + sCmdStr);
      return nullptr;
   }
   CMathItem* pItem = pParser->ConsumeItem(elcapFig, pParser->GetContext());
   if (!pItem) {
      if (!pParser->HasError())
         pParser->SetError("Missing {arg} for '\\" + sCmdStr + "' command");
      return nullptr;
   }
   //set class
   pItem->SetAtom(eClass);
   return pItem;
}
