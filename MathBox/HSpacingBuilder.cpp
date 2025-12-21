#include "stdafx.h"
#include "HSpacingBuilder.h"
#include "GlueItem.h"

bool CHSpacingBuilder::CanTakeCommand(PCSTR szCmd) const {
   _ASSERT_RET(szCmd && *szCmd, false);
   if (*szCmd == '\\')
      ++szCmd;
   if (*szCmd == ' ' || *szCmd == ',' || *szCmd == ':' || *szCmd == ';' || *szCmd == '!' ||
      0 == strcmp(szCmd, "qquad") || 0 == strcmp(szCmd, "quad") || 0 == strcmp(szCmd, "hskip"))
      return true;
   return false;
}
CMathItem* CHSpacingBuilder::BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) {
   _ASSERT_RET(szCmd && pParser, nullptr);
   const SParserContext& ctx = pParser->GetContext();
   _ASSERT_RET(CanTakeCommand(szCmd), nullptr);
   string sCmd(szCmd);
   
   CMathItem* pRet = nullptr;
   if (sCmd[1] == ' ' || sCmd == "\\quad") //1em
      pRet = new CGlueItem(pParser->Doc(), {0,0,0.0f,0.0f,MU2EM(18),MU2EM(18)}, ctx.currentStyle, ctx.fUserScale);
   else if (sCmd[1] == ',')//3mu
      pRet = new CGlueItem(pParser->Doc(), { 0,0,0.0f,0.0f,MU2EM(3),MU2EM(3) }, ctx.currentStyle, ctx.fUserScale);
   else if (sCmd[1] == ':')//4mu
      pRet = new CGlueItem(pParser->Doc(), { 0,0,0.0f,0.0f,MU2EM(4),MU2EM(4) }, ctx.currentStyle, ctx.fUserScale);
   else if (sCmd[1] == ';')//5mu
      pRet = new CGlueItem(pParser->Doc(), { 0,0,0.0f,0.0f,MU2EM(5),MU2EM(5) }, ctx.currentStyle, ctx.fUserScale);
   else if (sCmd[1] == '!')//-3mu
      pRet = new CGlueItem(pParser->Doc(), { 0,0,0.0f,0.0f,MU2EM(-3),MU2EM(-3) }, ctx.currentStyle, ctx.fUserScale);
   else if (sCmd == "\\qquad")//2 em
      pRet = new CGlueItem(pParser->Doc(), { 0,0,0.0f,0.0f,MU2EM(36),MU2EM(36) }, ctx.currentStyle, ctx.fUserScale);
   else if (sCmd == "\\hskip") {
      STexGlue glue;
      if (!pParser->ConsumeHSkipGlue(glue)) {
         if (!pParser->HasError())
            pParser->SetError("Failed to parse \\hskip arguments");
         return nullptr;
      }
      pRet = new CGlueItem(pParser->Doc(), glue, ctx.currentStyle, ctx.fUserScale);
   }
   return pRet;
}

