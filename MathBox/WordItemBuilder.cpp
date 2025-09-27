#include "stdafx.h"
#include "WordItemBuilder.h"
#include "WordItem.h"
#include "LMFontManager.h"
#include "HBoxItem.h"

extern CLMFontManager g_LMFManager;

namespace {
   inline bool _IsCRLF(char wc) { return wc == '\n' || wc == '\r'; }
   inline bool _IsGAP(char wc) { return wc == ' ' || wc == '\t'; }
   inline bool _IsSpace(char wc) { return _IsGAP(wc) || _IsCRLF(wc); }
   inline bool _IsDigit(char wc) { return wc>='0' && wc<='9'; }

   inline bool _IsSpecialChar(char cChar) {
      static const string _sSpecial{ "$%&~_^\\{}" };
      return _sSpecial.find(cChar) != string::npos;
   }
   inline bool _IsNumber(PCSTR szNum) {
      _ASSERT_RET(szNum && *szNum, false);
      bool bHasFP = false;
      while (*szNum) {
         if ('.' == *szNum) {
            if (bHasFP)
               return false;
            bHasFP = true;
         }
         else if (!_IsDigit(*szNum))
            return false;
         ++szNum;
      }
      return true;
   }
   inline PCSTR _SkipSpaces(PCSTR szText) {
      while (_IsSpace(*szText)) {
         ++szText;
      }
      return szText;
   }
   //preservs "\ " as a space!
   string _EatSpaces(const string& sText) {
      string sRet;
      bool bEscape = false;
      for (char cChar : sText) {
         if ('\\' == cChar) {
            if (bEscape)
               bEscape = false;
            else
               continue;//skip escape
         }
         else if ((' ' == cChar || '\t' == cChar)) {
            if (bEscape)
               bEscape = false;
            else
               continue; //skip space
         }
         sRet.push_back(cChar);
      }
      return sRet;
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
   CWordItem* _BuildLMMSymbol(const SLMMGlyph* pLmmGlyph, const CMathStyle& style, float fUserScale) {
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
      pRet->SetGlyphIndexes({ pLmmGlyph->nIndex });
      pRet->SetAtom(eAtom);
      return pRet;
   }
   bool _GetLMMIndexes(const string& sLatexUnicodeCmd, const string& sText, OUT vector<UINT16>& vGIdx) {
      for (char cChar : sText) {
         string sArg = "\\" + sLatexUnicodeCmd + '{' + cChar + '}';
         PCSTR szNextBracket = sLatexUnicodeCmd.c_str();
         while (szNextBracket = strchr(szNextBracket, '{')) {
            sArg.push_back('}');
            ++szNextBracket;
         }
         const SLMMGlyph* pLmmGlyph = g_LMFManager.GetLMMGlyphByCmd(sArg.c_str());
         if (!pLmmGlyph && sLatexUnicodeCmd != "mathit") {
            //try default mathit
            sArg = string("\\mathit{") + cChar + '}';
            pLmmGlyph = g_LMFManager.GetLMMGlyphByCmd(sArg.c_str());
         }
         if (!pLmmGlyph)
            pLmmGlyph = g_LMFManager.GetLMMGlyph(FONT_LMM, UINT32(cChar));
         _ASSERT_RET(pLmmGlyph && pLmmGlyph->nIndex, false);//unkown character?
         vGIdx.push_back(pLmmGlyph->nIndex);
      }
      return true;
   }
}

//used for a letter OR numbers (integer or float)
//@sWord: already processed, no spaces, no special chars
CMathItem* CWordItemBuilder::BuildMathWord(const string& sFontCmd, const string& sWord, bool bIsNumber,
                                           const CMathStyle& style, float fUserScale) {
   _ASSERT_RET(!sWord.empty(), nullptr); //ntd?
   SMathFontStyle mfStyle;
   _ASSERT_RET(g_LMFManager._GetMathFontStyle((sFontCmd.empty() ? "mathnormal" : sFontCmd), mfStyle), nullptr);
   if(bIsNumber && (sFontCmd == "mathbfit" || sFontCmd == "mathsfit" || sFontCmd == "mathssit"))
      mfStyle.nLetterDigitsFont = FONT_LMM; //use upright digits with these math fonts!
   CWordItem* pRet = new CWordItem(mfStyle.nLetterDigitsFont, style, eacWORD, fUserScale);
   if (mfStyle.nLetterDigitsFont != FONT_LMM)
      pRet->SetTextA(sWord.c_str());
   else {
      //we may need to map glyphs
      vector<UINT16> vGIdx;
      _ASSERT_RET(_GetLMMIndexes(mfStyle.szLatexUnicodeCmd, sWord, vGIdx), nullptr);
      pRet->SetGlyphIndexes(vGIdx);
   }
   return pRet;
}

//text mode!
CMathItem* CWordItemBuilder::BuildText(const string& sFontCmd, const string& sText, const CMathStyle& style,
                                       float fUserScale) {
   _ASSERT_RET(!sFontCmd.empty(), nullptr); //ntd?
   STextFontStyle tfStyle;
   _ASSERT_RET(g_LMFManager._GetTextFontStyle(sFontCmd, tfStyle), nullptr);
   int16_t nFontIdx = tfStyle.nLetterDigitsFont == FONT_DOC ? FONT_ROMAN_REGULAR : tfStyle.nLetterDigitsFont; //TODO!
   if (!tfStyle.bUseLMM || nFontIdx == FONT_LMM) {
      //uniform font for all chars
      CWordItem* pWord = new CWordItem(nFontIdx, style, eacWORD, fUserScale);
      pWord->SetTextA(sText.c_str());
      return pWord;
   }
   //else, mixed LMM/LM fonts
   CHBoxItem* pHBox = new CHBoxItem(style);
   string sLettersDigits;
   string sLMM;
   for (char cChar : sText) {
      if (' ' == cChar || isalnum(cChar)) {
         if (!sLMM.empty()) {
            CWordItem* pWord = new CWordItem(FONT_LMM, style, eacWORD, fUserScale);
            pWord->SetTextA(sLMM.c_str());
            pHBox->AddItem(pWord);
            sLMM.clear();
         }
         sLettersDigits.push_back(cChar);
      }
      else {
         if (!sLettersDigits.empty()) {
            CWordItem* pWord = new CWordItem(nFontIdx, style, eacWORD, fUserScale);
            pWord->SetTextA(sLettersDigits.c_str());
            pHBox->AddItem(pWord);
            sLettersDigits.clear();
         }
         sLMM.push_back(cChar);
      }
   }
   //EPILOG
   if (!sLettersDigits.empty()) {
      CWordItem* pWord = new CWordItem(nFontIdx, style, eacWORD, fUserScale);
      pWord->SetTextA(sLettersDigits.c_str());
      pHBox->AddItem(pWord);
   }
   else if (!sLMM.empty()) {
      CWordItem* pWord = new CWordItem(FONT_LMM, style, eacWORD, fUserScale);
      pWord->SetTextA(sLMM.c_str());
      pHBox->AddItem(pWord);
   }
   pHBox->Update();
   return pHBox;
}
//@sSym: escape+special, LMM symbol, MathOperator or Space\Kern
CMathItem* CWordItemBuilder::BuildTeXSymbol(const string& sFontCmd, const string& sSym, const CMathStyle& style,
                                          float fUserScale) {
   _ASSERT_RET(sSym.size() > 1 && sSym[0] == '\\', nullptr); //ntd?
   SMathFontStyle mfStyle;
   _ASSERT_RET(g_LMFManager._GetMathFontStyle((sFontCmd.empty()?"mathnormal":sFontCmd), mfStyle), nullptr);
   if (_IsSpecialChar(sSym[1])) {
      CWordItem* pRet = new CWordItem(mfStyle.nLetterDigitsFont, style, eacWORD, fUserScale);
      pRet->SetText({ UINT32(sSym[1]) });
      return pRet;
   }
   //2. MathOp
   if(_IsMathOp(sSym.c_str() + 1))
      return _BuildMathOperator(sSym.c_str() + 1, style, fUserScale);
   //3. Spacing
   if (sSym[1] == ' ' || sSym=="\\quad")
      return new CGlueItem({ 0,0,0.0f,0.0f,MU2EM(18),MU2EM(18) }, style, fUserScale);
   if (sSym[1] == ',')//3mu
      return new CGlueItem({ 0,0,0.0f,0.0f,MU2EM(3),MU2EM(3) }, style, fUserScale);
   if (sSym[1] == ':')//4mu
      return new CGlueItem({ 0,0,0.0f,0.0f,MU2EM(4),MU2EM(4) }, style, fUserScale);
   if (sSym[1] == ';')//5mu
      return new CGlueItem({ 0,0,0.0f,0.0f,MU2EM(5),MU2EM(5) }, style, fUserScale);
   if (sSym[1] == '!')//-3mu
      return new CGlueItem({ 0,0,0.0f,0.0f,MU2EM(-3),MU2EM(-3) }, style, fUserScale);
   if (sSym == "\\qquad")//2 em
      return new CGlueItem({ 0,0,0.0f,0.0f,MU2EM(36),MU2EM(36) }, style, fUserScale);
   //4. map LMMSym WITH font_cmd first
   const SLMMGlyph* pLmmGlyph = nullptr;
   if (mfStyle.szLatexUnicodeCmd) {
      string sArg = "\\" + string(mfStyle.szLatexUnicodeCmd) + '{' + sSym + '}';
      PCSTR szNextBracket = mfStyle.szLatexUnicodeCmd;
      while (szNextBracket = strchr(szNextBracket, '{')) {
         sArg.push_back('}');
         ++szNextBracket;
      }
      pLmmGlyph = g_LMFManager.GetLMMGlyphByCmd(sArg.c_str());
   }
   if (!pLmmGlyph) {
      if (sSym[0] == '\\')  //try sym w/o font decoration
         pLmmGlyph = g_LMFManager.GetLMMGlyphByCmd(sSym.c_str());
      else //single char
         pLmmGlyph = g_LMFManager.GetLMMGlyph(mfStyle.nLetterDigitsFont, UINT32(sSym[0]));
   }
   _ASSERT_RET(pLmmGlyph && pLmmGlyph->nIndex, nullptr);//unkown symbol/cmd!
   return _BuildLMMSymbol(pLmmGlyph, style, fUserScale);
}

bool CWordItemBuilder::BuildMathText(const string& sFontCmd, const string& sText, const CMathStyle& style, 
                             OUT CHBoxItem& hbox, float fUserScale) {
   CMathItem* pItem = nullptr;
   PCSTR szPos = _SkipSpaces(sText.c_str());
   _ASSERT_RET(*szPos, false);//snbh
   string sTerm;
   if (*szPos == '\\') {
      sTerm = "\\";
      ++szPos; //skip escaper!
   }
   while (1) {
      if (isalnum(*szPos) || *szPos=='.') { //floating point
         sTerm.push_back(*szPos);
         ++szPos;//eat next char
         continue;
      }
      if (*szPos == ' ' && (sTerm.empty() || sTerm.front() != '\\')) {
         ++szPos;//eat space in math mode!
         continue;
      }
      //we have special symbol or end-of Tex command
      if (!sTerm.empty()) {
         if (sTerm.front() == '\\') {
            if (sTerm.size() == 1 && _IsSpecialChar(*szPos)) {
               sTerm.push_back(*szPos);
               ++szPos;
            }
            pItem = BuildTeXSymbol(sFontCmd, sTerm, style, fUserScale);
            _ASSERT_RET(pItem, false);
            hbox.AddItem(pItem);
         }
         else if (_IsNumber(sTerm.c_str())) {
            pItem = BuildMathWord(sFontCmd, sTerm, true, style, fUserScale);
            _ASSERT_RET(pItem, false);
            hbox.AddItem(pItem);
         }
         else { //sTerm is a word (letters+digits)
            //TEST: process each char to a WordItem to have wider spacing
            for (char cChar : sTerm) {
               CMathItem* pWord = BuildMathWord(sFontCmd, string(1, cChar), false, style, fUserScale);
               hbox.AddItem(pWord);
            }
         }
         sTerm.clear();
      }
      szPos = _SkipSpaces(szPos);//skip next spaces if any
      if (*szPos == '\\') {
         sTerm = "\\";
         ++szPos; //skip escaper!
      }
      else if (0 == *szPos)
         break;
   }
   hbox.Update();
   return !hbox.IsEmpty();
}
