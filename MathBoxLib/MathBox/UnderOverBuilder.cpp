#include "stdafx.h"
#include "UnderOverBuilder.h"
#include "ExtGlyphBuilder.h"
#include "ContainerItem.h"


namespace {
   uint32_t _GetGlyphUnicode(PCSTR szCmd, OUT bool& bBelow) {
      _ASSERT_RET(szCmd && *szCmd, 0);
      if (szCmd[0] == '\\')
         ++szCmd; // skip '\\'
      static map<string, uint32_t> _mapCmd2UniTop = {
        {"widehat",              0x0302},  // circumflex accent
        {"widetilde",            0x0303},  // tildecomb
        {"widecheck",            0x030C},  // caroncmb
        {"overline",             0x0305},  // extendable overbar
        {"overbracket",          0x23B4},  // top square bracket
        {"overparen",            0x23DC},  // top parenthesis
        {"overbrace",            0x23DE},  // top curly bracket
        {"obrbrak",              0x23E0},  // top tortoise shell bracket
        //{"overlinesegment",      0xFFFF},// TODO: custom impl!
        {"overleftarrow",        0x20D6},  // 
        {"overrightarrow",       0x20D7},  // 
        {"overleftrightarrow",   0x20E1},  // COMBINING LEFT RIGHT ARROW ABOVE
        {"Overrightarrow",       0x21D2},  // arrowdblright, to be manually scaled/positioned in the VBox!
        {"overleftharpoon",      0x20D0},  // aka leftharpoonaccent
        {"overrightharpoon",     0x20D1},  // aka rightharpoonaccent
      };
      static map<string, uint32_t> _mapCmd2UniBottom = {
         {"utilde",              0x0330},  // Combining Tilde Below
         {"underline",           0x0332},  // lowlinecmb
         {"underleftarrow",      0x20D6},  //
         {"underrightarrow",     0x20D7},  // 
         {"underleftrightarrow",  0x20E1},
         {"underbracket",        0x23B5},  // bottom square bracket
         {"underparen",          0x23DD},  // bottom parenthesis 
         {"underbrace",          0x23DF},  // bottom curly bracket 
         {"ubrbrak",             0x23E1},  // bottom tortoise shell bracket
      };
      auto itCmdUni = _mapCmd2UniTop.find(szCmd);
      if (itCmdUni != _mapCmd2UniTop.end()) {
         bBelow = false;
         return itCmdUni->second;
      }
      auto itCmdUniB = _mapCmd2UniBottom.find(szCmd);
      if (itCmdUniB != _mapCmd2UniBottom.end()) {
         bBelow = true;
         return itCmdUniB->second;
      }
      return 0;//not found
   }
}
//CUnderOverBuilder
bool CUnderOverBuilder::CanTakeCommand(PCSTR szCmd) const {
   bool bBelow;
   uint32_t nUni = _GetGlyphUnicode(szCmd, bBelow);
   return nUni != 0;
}
CMathItem* CUnderOverBuilder::BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) {
   _ASSERT_RET(szCmd && pParser, nullptr);
   const SParserContext& ctx = pParser->GetContext();
   _ASSERT_RET(CanTakeCommand(szCmd), nullptr);
   // Get current context
   CMathItem* pBase = pParser->ConsumeItem(elcapFig, ctx);
   if (!pBase) {
      if (!pParser->HasError())
         pParser->SetError("Missing {arg} for accent command");
      return nullptr;
   }
   return _BuildUnderOverItem(pBase, szCmd, ctx);
}
CMathItem* CUnderOverBuilder::_BuildUnderOverItem(IN CMathItem* pBase, PCSTR szCmd, const SParserContext& ctx) {
   if (szCmd[0] == '\\')
      ++szCmd;
   bool bBelow;
   uint32_t nUni = _GetGlyphUnicode(szCmd, bBelow);
   _ASSERT_RET(nUni, nullptr);//snbh!
   _ASSERT_RET(pBase, nullptr);//snbh!
   CExtGlyphBuilder egBuilder(pBase->Doc());
   CMathItem* pDecor = egBuilder.BuildHorizontalGlyph(nUni, ctx.currentStyle, pBase->Box().Width(), ctx.fUserScale);
   _ASSERT_RET(pDecor, nullptr);//snbh!
   //build VBox
   float fScale = ctx.EffectiveScale();
   CContainerItem* pRet = new CContainerItem(pBase->Doc(), eacOVERUNDER, ctx.currentStyle);
   pRet->AddBox(pBase, 0, 0);
   int32_t nDecorLeft = (pBase->Box().Width() - pDecor->Box().Width()) / 2;
   if (bBelow)
      pRet->AddBox(pDecor, nDecorLeft, pBase->Box().Bottom() + F2NEAREST(fScale * otfUnderbarVerticalGap));
   else
      pRet->AddBox(pDecor, nDecorLeft,
         pBase->Box().Top() - pDecor->Box().Height() - F2NEAREST(fScale * otfOverbarVerticalGap));
   pRet->NormalizeOrigin(0, 0);
   //correct possible index placements
   if (0 == strcmp(szCmd, "overbracket") || 0 == strcmp(szCmd, "overbrace") || 0 == strcmp(szCmd, "overparen"))
      pRet->SetIdxPlacement(eipOverscript);
   else if (0 == strcmp(szCmd, "underbracket") || 0 == strcmp(szCmd, "underbrace") || 0 == strcmp(szCmd, "underparen"))
      pRet->SetIdxPlacement(eipUnderscript);
   return pRet;

}
