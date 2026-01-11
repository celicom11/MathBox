#include "stdafx.h"
#include "LOpBuilder.h"
#include "WordItem.h"

namespace {
   struct SLOpGlyphInfo {
      uint16_t nIdx{ 0 }, nIdxD{ 0 };
      int16_t nItCor{ 0 }, nItCorD{ 0 };
   };
   //copy from LMMGlyphs/CLMFontManager
   static const vector<pair<string, SLOpGlyphInfo> > _vCmd2GInfo{
      {"int",              {3049,3063,332,591}},      //0x222B
      {"iint",             {3050,3064,332,591}},      //0x222C
      {"iiint",            {3051,3065,332,591}},      //0x222D
      {"iiiint",           {3052,3066,332,591}},      //0x2A0C
      {"oint",             {3053,3067,332,591}},      //0x222E
      {"oiint",            {3054,3068,332,591}},      //0x222F
      {"oiiint",           {3055,3069,332,591}},      //0x2230
      {"intclockwise",     {3056,3069,361,591}},      //0x2231
      {"awint",            {3057,3069,361,591}},      //0x2A11
      {"varointclockwise", {3058,3069,350,591}},      //0x2231
      {"ointctrclockwise", {3059,3069,350,591}},      //0x2231
      //OverUnderD = true below!
      {"sum",              {3060,3074}},              //0x2211
      {"prod",             {3061,3075}},              //0x220F
      {"coprod",           {3062,3076}},              //0x2210
      {"bigwedge",         {2771,2772}},              //0x22C0
      {"bigvee",           {2773,2774}},              //0x22C1
      {"bigcap",           {2765,2766}},              //0x22C2
      {"bigcup",           {2767,2768}},              //0x22C3

      {"bigoplus",         {2806,2807}},              //0x2A01
      {"bigotimes",        {2811,2812}},              //0x2A02
      {"bigcupdot",        {2796,2797}},              //0x2A03
      {"biguplus",         {2799,2800}},              //0x2A04
      {"bigsqcap",         {2790,2791}},              //0x2A05
      {"bigsqcup",         {2792,2793}},              //0x2A06
      {"bigtimes",         {2639,2640}},              //0x2A09
   };
   const SLOpGlyphInfo* _FindCmd(PCSTR szCmd,  OUT bool& bOverUnderD) {
      _ASSERT_RET(szCmd && *szCmd, nullptr);
      if (szCmd[0] == '\\')
         ++szCmd;
      if (!*szCmd)
         return nullptr;
      string sCmd(szCmd);
      if (sCmd.size() < 3)
         return nullptr; //"int"
      bOverUnderD = false;
      //else, check "op" suffix
      if (sCmd.back() == 'p' && sCmd[sCmd.size() - 2] == 'o') {
         bOverUnderD = true;
         sCmd.pop_back();
         sCmd.pop_back();
      }
      int nIdx = 0;
      for (; nIdx < (int)_vCmd2GInfo.size(); ++nIdx) {
         if (sCmd == _vCmd2GInfo[nIdx].first) {
            if(!bOverUnderD)
               bOverUnderD = nIdx > 10; //>ointctrclockwise
            return &_vCmd2GInfo[nIdx].second; //copy
         }
      }
      return nullptr;
   }
}
//IMathItemBuilder 
bool CLOpBuilder::CanTakeCommand(PCSTR szCmd) const {
   bool bOverUnderD;
   return _FindCmd(szCmd, bOverUnderD) != nullptr;
}
CMathItem* CLOpBuilder::BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) {
   _ASSERT_RET(szCmd && *szCmd && pParser, nullptr);
   bool bOverUnderD;
   const SLOpGlyphInfo* pGInfo = _FindCmd(szCmd, bOverUnderD);
   _ASSERT_RET(pGInfo, nullptr); //snbh!

   // Get current context
   CMathStyle style = pParser->GetContext().currentStyle;
   float fUserScale = pParser->GetContext().fUserScale;
   CWordItem* pRet = new CWordItem(pParser->Doc(), FONT_LMM, style, eacBIGOP, fUserScale);
   pRet->SetAtom(etaOP);
   pRet->SetGlyphIndexes({ style.Style() == etsDisplay ? pGInfo->nIdxD : pGInfo->nIdx });
   bool bLimits = false, bNoLimits = false;
   string sToken;
   EnumTokenType ettNext = pParser->GetTokenData(sToken);
   if (ettNext == ettCOMMAND && (sToken == "\\nolimits" || sToken == "\\limits")) {
      bLimits = sToken == "\\limits";
      bNoLimits = sToken == "\\nolimits";
      pParser->SkipToken(); //consumed
   }
   if (bLimits || (bNoLimits == false && style.Style() == etsDisplay && bOverUnderD))
      pRet->SetIdxPlacement(eipOverUnderscript);
   return pRet;
}
