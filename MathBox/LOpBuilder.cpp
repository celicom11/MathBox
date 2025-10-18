#include "stdafx.h"
#include "LOpBuilder.h"
#include "WordItem.h"

namespace {
   static const vector<SLaTexCmdArgInfo> _vCmdArgs{
   { true, false, false, 0, elcatLimits, elcapAny},
   };
   struct SLOpGlyphInfo {
      uint16_t nIdx{ 0 }, nIdxD{ 0 };
      int16_t nItCor{ 0 }, nItCorD{ 0 };
   };
   //copy from LMMGlyphs/CLMFontManager
   static const map<string, SLOpGlyphInfo> _mapCmd2GInfo{
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
}
//IMathItemBuilder 
bool CLOpBuilder::CanTakeCommand(PCSTR szCmd) const {
   _ASSERT_RET(szCmd && *szCmd, false);
   if (szCmd[0] == '\\')
      ++szCmd;
   return (_mapCmd2GInfo.find(szCmd) != _mapCmd2GInfo.end());
}
bool CLOpBuilder::GetCommandInfo(PCSTR szCmd, OUT SLaTexCmdInfo& cmdInfo) const {
   if(!CanTakeCommand(szCmd))
      return false;
   cmdInfo.bHasLimits = true;
   cmdInfo.vArgInfo = _vCmdArgs;//same for all LOps
   return true;
}
CMathItem* CLOpBuilder::BuildItem(PCSTR szCmd, const CMathStyle& style, float fUserScale,
                                    const vector<SLaTexCmdArgValue>& vArgValues) const {
   _ASSERT_RET(szCmd && *szCmd, nullptr);
   if (*szCmd == '\\')
      ++szCmd;
   auto itPair = _mapCmd2GInfo.find(szCmd);
   _ASSERT_RET(itPair != _mapCmd2GInfo.end(), nullptr);
   //return a syngle glyph of D/non-D size
   CWordItem* pRet = new CWordItem(FONT_LMM, style, eacBIGOP, fUserScale);
   pRet->SetGlyphIndexes({ style.Style() == etsDisplay? itPair->second.nIdxD: itPair->second.nIdx });
   if (vArgValues.size() == 1 && vArgValues[0].eLCAT == elcatLimits) {
      if (vArgValues[0].uVal.nVal == 1)
         pRet->SetIdxPlacement(eipOverUnderscript);
   }
   return pRet;
}
