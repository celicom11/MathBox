#include "stdafx.h"
#include "OverlayBuilder.h"

bool COverlayBuilder::CanTakeCommand(PCSTR szCmd) const {
   _ASSERT_RET(szCmd && *szCmd, false);
   static const vector<string> _vCmds{
      "underline","cancel","bcancel","xcancel","angl","phase","boxed","sout",
   };
   if (*szCmd == '\\')
      ++szCmd;
   return *szCmd && std::find(_vCmds.begin(), _vCmds.end(), szCmd) != _vCmds.end();
}
//CMathItem* BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) override;
