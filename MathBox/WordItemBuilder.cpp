#include "stdafx.h"
#include "WordItemBuilder.h"
#include "WordItem.h"
#include "LMFontManager.h"
#include "HBoxItem.h"

extern CLMFontManager g_LMFManager;

namespace {
   wstring _A2W(const string& sText, bool bEatSpaces = false) {
      wstring wsRet;
      for (char cChar : sText) {
         if (bEatSpaces && (' ' == cChar || '\t' == cChar))
            continue;
         wsRet.push_back(WCHAR(cChar));//q&d
      }
      return wsRet;
   }
   bool _IsMathOp(PCSTR szText) {
      for (PCSTR szOp : _aMathOps) {
         if (0 == strcmp(szText, szOp))
            return true;
      }
      return false;
   }
   // BUILDERS
   CWordItem* _BuildMathOperator(PCSTR szOp, const CMathStyle& style, float fUserScale) {
      CWordItem* pRet = new CWordItem(FONT_LMM, style, eacMATHOP, fUserScale);
      //TODO:liminf->lim inf,limsup->lim sup
      pRet->SetTextA(szOp);
      return pRet;
   }
   CWordItem* _BuildMathSymbol(const SLMMGlyph* pLmmGlyph, const CMathStyle& style, float fUserScale) {
      CWordItem* pRet = new CWordItem(FONT_LMM, style, eacWORD, fUserScale);
      EnumTexAtom eAtom = etaORD;//default
      switch (pLmmGlyph->eClass) {
      case egcLOP:   eAtom = etaOP; break;
      case egcBin:   eAtom = etaBIN; break;
      case egcRel:   eAtom = etaREL; break;
      case egcOpen:  eAtom = etaOPEN; break;
      case egcClose: eAtom = etaCLOSE; break;
      case egcPunct: eAtom = etaPUNCT; break;
      }
      pRet->SetAtom(eAtom);
      return pRet;
   }
   CMathItem* _BuildMathModeText(int16_t nLetterDigitsFont, const wstring& wsArg, const CMathStyle& style, float fUserScale) {
      CHBoxItem* pHBox = new CHBoxItem(style);
      if (FONT_LMM == nLetterDigitsFont) {
         //split arg to single symbols
         for (WCHAR wc : wsArg) {
            const SLMMGlyph* pLmmGlyph = g_LMFManager.GetLMMGlyph(FONT_LMM, wc);
            _ASSERT_RET(pLmmGlyph, nullptr);//snbh!
            pHBox->AddItem(_BuildMathSymbol(pLmmGlyph, style, fUserScale));
         }
      }
      else {
         //mix of letter_digits and others
         wstring wsLetterDigits;
         for (WCHAR wc : wsArg) {
            if (iswalnum(wc))
               wsLetterDigits.push_back(wc);
            else
            {
               if (!wsLetterDigits.empty()) {
                  CWordItem* pWord = new CWordItem(nLetterDigitsFont, style, eacWORD, fUserScale);
                  pWord->SetText(wsLetterDigits.c_str());
                  pHBox->AddItem(pWord);
                  wsLetterDigits.clear();
               }
               const SLMMGlyph* pLmmGlyph = g_LMFManager.GetLMMGlyph(FONT_LMM, wc);
               _ASSERT_RET(pLmmGlyph, nullptr);//snbh!
               pHBox->AddItem(_BuildMathSymbol(pLmmGlyph, style, fUserScale));
            }
         }
      }
      return pHBox;
   }
   CMathItem* _BuildFontCmdWord(const SLatexFontCmd& lfCmd, const string& sArg, const CMathStyle& style, float fUserScale) {
      int16_t nFontIdx = lfCmd.nLetterDigitsFont == FONT_DOC ? FONT_ROMAN_REGULAR : lfCmd.nLetterDigitsFont; //TODO!
      bool bHasSpaces = sArg.find(' ') != string::npos;
      wstring wsArg = _A2W(sArg, !lfCmd.bPreserveTextSpacing);
      if (lfCmd.bUseLMM)
         return _BuildMathModeText(lfCmd.nLetterDigitsFont, wsArg, style, fUserScale);
      //else, single glyphrun
      CWordItem* pRet = new CWordItem(lfCmd.nLetterDigitsFont, style, eacWORD, fUserScale);
      pRet->SetText(wsArg.c_str());
      return pRet;
   }
}
CMathItem* CWordItemBuilder::BuildWordItem(const string& sCmd, const string& sArg, 
                                                   const CMathStyle& style, float fUserScale) {
   _ASSERT_RET(sCmd.empty() || sCmd[0] == '\\', nullptr);
   _ASSERT_RET(sArg.empty() || sArg[0] != '\\', nullptr); //inner command not supported here!
   CWordItem* pRet = nullptr;
   if (!sCmd.empty() ) {
      SLatexFontCmd lfCmd;
      if (!sArg.empty() && g_LMFManager._GetLaTexFontCmdInfo(sCmd.c_str(), lfCmd))
         return _BuildFontCmdWord(lfCmd, sArg, style, fUserScale);
      if (sArg.empty()) {
         if (_IsMathOp(sCmd.c_str() + 1))
            pRet = _BuildMathOperator(sCmd.c_str() + 1, style, fUserScale);
         else {
            const SLMMGlyph* pLmmGlyph = g_LMFManager.GetLMMGlyphByCmd(sCmd.c_str());
            if (pLmmGlyph)
               pRet = _BuildMathSymbol(pLmmGlyph, style, fUserScale);
         }
      }
      //else - cannot handle this cmd!
   }
   else { //just a "math mode" variable/exprs, spaces gets eaten!
      wstring wsText = _A2W(sArg, true);
      pRet = new CWordItem(FONT_LMM, style, eacWORD, fUserScale);
      pRet->SetAtom(etaORD);
      pRet->SetText(wsText.c_str());
   }
   return pRet;
}
