#include "stdafx.h"
#include "MathFontCmdBuilder.h"
#include "FontStyleHelper.h"

// \fontsize{size pt}{h_skip}\selectfont{arg} 
// \scalefnt{factor}{arg}
// \relscale{factor}{arg}
bool CMathFontCmdBuilder::CanTakeCommand(PCSTR szCmd) const {
   _ASSERT_RET(szCmd && *szCmd, false);
   if (*szCmd == '\\')
      ++szCmd;
   SMathFontStyle mfStyle; //dummy
   return FontStyleHelper::_GetMathFontStyle(szCmd, mfStyle);
}

CMathItem* CMathFontCmdBuilder::BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) {
   _ASSERT_RET(szCmd && pParser, nullptr);
   _ASSERT_RET(CanTakeCommand(szCmd), nullptr);
   if (*szCmd == '\\')
      ++szCmd;
   string sCmd(szCmd);
   SParserContext ctxMath(pParser->GetContext());
   ctxMath.bTextMode = false; //math mode
   ctxMath.sFontCmd = szCmd;
   //arg
   CMathItem* pRet = pParser->ConsumeItem(elcapAny, ctxMath);
   if (!pRet) {
      if (!pParser->HasError())
         pParser->SetError("Failed to parse argumend for math font command '\\" + sCmd + "'");
      return nullptr;
   }

   return pRet;
}

