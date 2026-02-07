#include "stdafx.h"
#include "BoxCmdBuilder.h"

namespace {
   bool _IsFrameCmd(const string& sCmd) {
      return (sCmd == "boxed" || sCmd == "framebox" || sCmd == "fbox" || sCmd == "fcolorbox");
   }
   bool _IsColorCmd(const string& sCmd) {
      return (sCmd == "colorbox" || sCmd == "fcolorbox");
   }
}
bool CBoxCmdBuilder::CanTakeCommand(PCSTR szCmd) const {
   _ASSERT_RET(szCmd && *szCmd, false);
   if (*szCmd == '\\')
      ++szCmd;
   string sCmd(szCmd);
   return (sCmd == "boxed" || sCmd == "mbox" || sCmd == "hbox" ||
      sCmd == "makebox" || sCmd == "framebox" ||
      sCmd == "fbox" || sCmd == "colorbox" || sCmd == "fcolorbox");
}
CMathItem* CBoxCmdBuilder::BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) {
   _ASSERT_RET(szCmd && *szCmd && pParser, nullptr);
   _ASSERT_RET(CanTakeCommand(szCmd), nullptr);
   string sCmd(szCmd + 1);
   SRect rcBox{ 0,0,0,0 };
   float fBoxRule = 0.0f;
   uint32_t argbFrame = 0, argbFill = 0;
   CMathItem* pContent = GetContent_(sCmd, pParser, rcBox, fBoxRule, argbFrame, argbFill);
   if (!pContent)
      return nullptr;
   //check empty boxing - not needed in MathBox
   const STexBox& boxContent = pContent->Box();
   if(rcBox.left == boxContent.Left() && rcBox.top == boxContent.Top() &&
      rcBox.right == boxContent.Right() && rcBox.bottom == boxContent.Bottom() &&
      argbFrame == 0 && argbFill == 0)
      return pContent; //no boxing/overlays needed!

   COverlayItem* pRet = new COverlayItem(pContent);
   if(rcBox.right> rcBox.left)
      pRet->SetBox(rcBox, fBoxRule, argbFrame, argbFill);
   pRet->UpdateLayout();
   return pRet;
}
CMathItem* CBoxCmdBuilder::GetContent_(const string& sCmd, IParserAdapter* pParser,
                                       OUT SRect& rcBox, OUT float& fBoxRule,
                                       OUT uint32_t& argbFrame, OUT uint32_t& argbFill) {
   char cPos = 'c'; //default: center
   fBoxRule = 0.0f;
   float fBoxWidth = -1.0f;
   string sOrigUnits;
   if (sCmd == "makebox" || sCmd == "framebox") {
      //read two optional params:[width][pos]
      if (pParser->ConsumeDimension(elcapSquare, fBoxWidth, sOrigUnits)) {
         if(sOrigUnits.empty() ) {
            pParser->SetError("Missed units for width in '" + sCmd + "' command");
            return nullptr;
         }
         string sText;
         EnumTokenType ettNext = pParser->GetTokenData(sText);
         if (sText == "[") {
            pParser->SkipToken(); //skip '['
            ettNext = pParser->GetTokenData(sText);
            if (sText != "c" && sText != "l" && sText != "r") {
               //'s' - not supported!
               pParser->SetError("Invalid position specifier in '" + sCmd +
                  "' command. Expected one of [c],[l],[r]");
               return nullptr;
            }
            cPos = sText[0];
            pParser->SkipToken(); //skip pos char
            pParser->SkipToken(); //skip ']'
         }
      }
   }
   else if(_IsColorCmd(sCmd)) {
      argbFill = pParser->ConsumeColor(elcapFig);
      if (argbFill == 0) {
         pParser->SetError("Invalid color specification in '" + sCmd + "' command");
         return nullptr;
      }
      if(sCmd == "fcolorbox") {
         argbFrame = argbFill; //default frame color = fill color
         //read optional frame color
         argbFill = pParser->ConsumeColor(elcapFig);
         if (argbFill == 0) {
            pParser->SetError("Invalid background color specification in '" + sCmd + "' command");
            return nullptr;
         }
      }
   }
   if(_IsFrameCmd(sCmd)) {
      //get optional frame width
      if(!pParser->GetLength("fboxrule", fBoxRule)) 
         fBoxRule = PTS2DIPS(0.4f); //DIPs, default
      if(argbFrame == 0)
         argbFrame = pParser->Doc().ColorText(); //frame color == text color
   }
   //read content
   SParserContext ctxArg = pParser->GetContext();
   if(sCmd != "boxed")
      ctxArg.bTextMode = true; //boxing always in TEXT mode!
   CMathItem* pContent = pParser->ConsumeItem(elcapFig, ctxArg);
   if (!pContent) {
      if (!pParser->HasError())
         pParser->SetError("Missing {arg} for '" + sCmd + "' command");
      return nullptr;
   }
   //determine box
   float fBoxSep = 4.0f;//should be 3Pts but it looks too narrow compare to KaTeX/LuaLaTex!
   pParser->GetLength("fboxsep", fBoxSep);
   fBoxSep = PTS2DIPS(fBoxSep);
   const uint32_t nBoxSepFDU = DIPS2EM(pParser->Doc().DefaultFontSizePts(), fBoxSep);

   const STexBox& boxContent = pContent->Box();
   rcBox.left = boxContent.Left();
   rcBox.top = boxContent.Top();
   rcBox.right = boxContent.Right();
   rcBox.bottom = boxContent.Bottom();
   //adjust width if needed
   if (sCmd != "mbox" && sCmd != "hbox") {
      //position box around content
      rcBox.top -= nBoxSepFDU;
      rcBox.bottom += nBoxSepFDU;
      if (fBoxWidth < 0.0f) {
         //default: add box sep on L/R sides
         rcBox.left -= nBoxSepFDU;
         rcBox.right += nBoxSepFDU;
      }
      else {
         const int32_t nBoxWidthFDU = DIPS2EM(pParser->Doc().DefaultFontSizePts(), fBoxWidth);
         switch (cPos) {
         case 'l': //left+margin
            rcBox.left = pContent->Box().Left() - nBoxSepFDU;
            rcBox.right = rcBox.left + nBoxWidthFDU;
            break;
         case 'r': //right
            rcBox.right = pContent->Box().Right() + nBoxSepFDU;
            rcBox.left = rcBox.right - nBoxWidthFDU;
            break;
         case 'c': //center
         default: {
            int32_t nContentWidth = rcBox.right - rcBox.left;
            rcBox.left -= (nBoxWidthFDU - nContentWidth) / 2;
            rcBox.right = rcBox.left + nBoxWidthFDU;
            }
            break;
         }
      }
   }
   return pContent;
}
