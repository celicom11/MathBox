#include "stdafx.h"
#include "AccentBuilder.h"
#include "WordItem.h"
#include "ContainerItem.h"

namespace {
   uint32_t _GetAccentUnicode(PCSTR szCmd,OUT bool& bBelow) {
      _ASSERT_RET(szCmd && *szCmd, 0);
      if (szCmd[0] == '\\')
         ++szCmd; // skip '\\'
      static map<string, uint32_t> _mapCmd2UniTop = {
        {"grave",             0x0300},  // GRAVE ACCENT
        {"acute",             0x0301},  // ACUTE ACCENT
        {"hat",               0x0302},  // CIRCUMFLEX ACCENT
        {"tilde",             0x0303},  // TILDE  
        {"bar",               0x0304},  // MACRON
        {"overbar",           0x0305},  // overbar embellishment
        {"breve",             0x0306},  // breve
        {"dot",               0x0307},  // DOT ABOVE
        {"ddot",              0x0308},  // DIAERESIS
        {"ovhook",            0x0309},  // combining hook above
        {"ocirc",             0x030A},  // RING ABOVE
        {"mathring",          0x030A},  // RING ABOVE 2
        {"check",             0x030C},  // CARON
        {"leftharpoonaccent", 0x20D0},  // 
        {"rightharpoonaccent",0x20D1},  //
        {"vertoverlay",       0x20D2},  //
        {"vec",               0x20D7},  //
        {"dddot",             0x20DB},  //
        {"ddddot",            0x20DC},  // 
        {"widebridgeabove",   0x20E9},  // 
        {"asteraccent",       0x20F0},  //

        // ... add more as needed
      };
      static map<string, uint32_t> _mapCmd2UniBottom = {
         {"threeunderdot",    0x20E8},  //
         //{"underleftarrow",   0x20EE},  // moved to CUnderOverBuilder
         //{"underrightarrow",  0x20EF},  //
         
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
   CMathItem* _getParentBase(CMathItem* pItem) {
      while (pItem->Type() == eacACCENT) {
         CContainerItem* pCont = static_cast<CContainerItem*>(pItem);
         pItem = pCont->Items().front();
      }
      return pItem;
   }
}
bool CAccentBuilder::CanTakeCommand(PCSTR szCmd) const {
   bool bBelow;
   uint32_t nAccentUni = _GetAccentUnicode(szCmd, bBelow);
   return nAccentUni != 0;
}
CMathItem* CAccentBuilder::BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) {
   _ASSERT_RET(szCmd && pParser, nullptr);
   const SParserContext& ctx = pParser->GetContext();
   _ASSERT_RET(CanTakeCommand(szCmd), nullptr);
   CMathItem* pBase = pParser->ConsumeItem(elcapFig, ctx);
   if (!pBase) {
      if (!pParser->HasError())
         pParser->SetError("Missing {arg} for accent command");
      return nullptr;
   }
   return BuildAccented_(pParser, pBase, szCmd);
}

CMathItem* CAccentBuilder::BuildAccented_(IParserAdapter* pParser, CMathItem* pBase,const string& sLatexAccentCmd) {
   _ASSERT_RET(pBase, nullptr);
   const SParserContext& ctx = pParser->GetContext();
   bool bBelow;
   uint32_t nAccentUni = _GetAccentUnicode(sLatexAccentCmd.c_str(), bBelow);
   _ASSERT_RET(nAccentUni, nullptr);
   const SLMMGlyph* pLmmGlyph = pParser->Doc().LMFManager().GetLMMGlyph(FONT_LMM, nAccentUni);
   _ASSERT_RET(pLmmGlyph, nullptr);
   int32_t nAccentX = pLmmGlyph->nTopAccentX;
   CContainerItem* pRet = new CContainerItem(pParser->Doc(), eacACCENT, ctx.currentStyle);
   //Accent vertical position
   CMathItem* pAccentBase = _getParentBase(pBase);
   int32_t nBaseAccentX = pAccentBase->Box().Width()/2; //default
   if (!bBelow && pAccentBase->Type() == eacWORD) {
      CWordItem* pWordItem = static_cast<CWordItem*>(pAccentBase);
      if (pWordItem->GetFontIdx() == FONT_LMM && pWordItem->GlyphCount() == 1) {
         //get glyph's info
         const SLMMGlyph* pLmmGlyph =
            pParser->Doc().LMFManager().GetLMMGlyphByIdx(FONT_LMM, pWordItem->GetIndexAt(0));
         if (pLmmGlyph)
            nBaseAccentX = pLmmGlyph->nTopAccentX;
      }
   }
   pRet->AddBox(pBase, 0, 0);
   CWordItem* pAccent = new CWordItem(pParser->Doc(), 0, ctx.currentStyle, eacUNK, ctx.fUserScale);
   pAccent->SetText({ nAccentUni });
   //position accent
   int32_t nAccentY = 0;
   if (bBelow)
      nAccentY = pBase->Box().Bottom() - pAccent->Box().Ascent();
   else
      nAccentY = min(otfAccentBaseHeight, pBase->Box().Height()) - pAccent->Box().Ascent();
   pRet->AddBox(pAccent, nBaseAccentX - nAccentX, //align attachment points of the base and the accent!
                         nAccentY);
   pRet->NormalizeOrigin(0, 0);
   //restore pAccentBase's box horizontal metrics
   pRet->Box().nAdvWidth = pAccentBase->Box().Width();
   pRet->Box().nLBearing = pAccentBase->Box().nLBearing;
   pRet->Box().nRBearing = pAccentBase->Box().nRBearing;
   return pRet;
}
