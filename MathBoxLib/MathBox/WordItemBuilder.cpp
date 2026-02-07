#include "stdafx.h"
#include "WordItemBuilder.h"
#include "WordItem.h"
#include "FontStyleHelper.h"
#include "HBoxItem.h"

namespace {
   inline bool _IsCRLF(char wc) { return wc == '\n' || wc == '\r'; }
   inline bool _IsGAP(char wc) { return wc == ' ' || wc == '\t'; }
   inline bool _IsSpace(char wc) { return _IsGAP(wc) || _IsCRLF(wc); }
   inline bool _IsDigit(char wc) { return wc>='0' && wc<='9'; }

   inline bool _IsSpecialChar(char cChar) {
      static const string _sSpecial{ "$%&~_^\\{}" };
      return _sSpecial.find(cChar) != string::npos;
   }
   inline PCSTR _SkipSpaces(PCSTR szText) {
      while (_IsSpace(*szText)) {
         ++szText;
      }
      return szText;
   }
   bool _IsMathOp(PCSTR szText) {
      for (PCSTR szOp : _aMathOps) {
         if (0 == strcmp(szText, szOp))
            return true;
      }
      return false;
   }
   // BUILDERS
   CWordItem* _BuildMathOperator(IDocParams& doc, PCSTR szOp, const CMathStyle& style, float fUserScale) {
      CWordItem* pRet = new CWordItem(doc, FONT_LMM, style, eacMATHOP, fUserScale);
      //TODO:liminf->lim inf,limsup->lim sup
      pRet->SetTextA(szOp);
      return pRet;
   }
   CWordItem* _BuildLMMSymbol(IDocParams& doc, const SLMMGlyph* pLmmGlyph, const CMathStyle& style, float fUserScale) {
      CWordItem* pRet = new CWordItem(doc, FONT_LMM, style, eacWORD, fUserScale);
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
   bool _GetLMMIndexes(IDocParams& doc, const string& sLatexUnicodeCmd, const string& sText, OUT vector<uint16_t>& vGIdx) {
      for (char cChar : sText) {
         string sArg = "\\" + sLatexUnicodeCmd + '{' + cChar + '}';
         PCSTR szNextBracket = sLatexUnicodeCmd.c_str();
         while (szNextBracket = strchr(szNextBracket, '{')) {
            sArg.push_back('}');
            ++szNextBracket;
         }
         const SLMMGlyph* pLmmGlyph = doc.LMFManager().GetLMMGlyphByCmd(sArg.c_str());
         if (!pLmmGlyph && sLatexUnicodeCmd != "mathit") {
            //try default mathit
            sArg = string("\\mathit{") + cChar + '}';
            pLmmGlyph = doc.LMFManager().GetLMMGlyphByCmd(sArg.c_str());
         }
         if (!pLmmGlyph)
            pLmmGlyph = doc.LMFManager().GetLMMGlyph(FONT_LMM, uint32_t(cChar));
         _ASSERT_RET(pLmmGlyph && pLmmGlyph->nIndex, false);//unkown character?
         vGIdx.push_back(pLmmGlyph->nIndex);
      }
      return true;
   }
}
bool CWordItemBuilder::_IsNumber(PCSTR szNum) {
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

//used for a letter OR numbers (integer or float)
//@sWord: already processed, no spaces, no special chars
CMathItem* CWordItemBuilder::_BuildMathWord(IDocParams& doc, const string& sWord, bool bNumber, const SParserContext& ctx) {
   _ASSERT_RET(!sWord.empty(), nullptr); //ntd?

   SMathFontStyle mfStyle;
   PCSTR szFontCmd = (ctx.sFontCmd.empty() ? "mathnormal" : ctx.sFontCmd.c_str());
   _ASSERT_RET(FontStyleHelper::_GetMathFontStyle(szFontCmd, mfStyle), nullptr);
   if(bNumber && (ctx.sFontCmd == "mathbfit" || ctx.sFontCmd == "mathsfit" || ctx.sFontCmd == "mathssit"))
      mfStyle.nLetterDigitsFont = FONT_LMM; //use upright digits with these math fonts!
   if (sWord.size() == 1 && !isalnum(sWord[0]) && sWord[0] !='[' && sWord[0] != ']') {
      _ASSERT(!_IsSpace(sWord[0]));
      // Check atom type!
      const SLMMGlyph* pLmmGlyph = doc.LMFManager().GetLMMGlyph(mfStyle.nLetterDigitsFont, uint32_t(sWord[0]));
      _ASSERT_RET(pLmmGlyph && pLmmGlyph->nIndex, nullptr);//unkown unicode?
      return _BuildLMMSymbol(doc, pLmmGlyph, ctx.currentStyle, ctx.fUserScale);
   }
   CWordItem* pRet = new CWordItem(doc, mfStyle.nLetterDigitsFont, ctx.currentStyle, eacWORD, ctx.fUserScale);
   if (mfStyle.nLetterDigitsFont != FONT_LMM)
      pRet->SetTextA(sWord.c_str());
   else {
      //we may need to map glyphs
      vector<uint16_t> vGIdx;
      _ASSERT_RET(_GetLMMIndexes(doc, mfStyle.szLatexUnicodeCmd, sWord, vGIdx), nullptr);
      pRet->SetGlyphIndexes(vGIdx);
   }
   return pRet;
}
// text mode CWordItem builder
CMathItem* CWordItemBuilder::_BuildText(IDocParams& doc, const string& sText, const SParserContext& ctx) {
   STextFontStyle tfStyle;
   _ASSERT_RET(FontStyleHelper::_GetTextFontStyle(ctx.sFontCmd.c_str(), tfStyle), nullptr);
   int16_t nFontIdx = tfStyle.nLetterDigitsFont == FONT_DOC ? FONT_ROMAN_REGULAR : tfStyle.nLetterDigitsFont; //TODO!
   CWordItem* pWord = new CWordItem(doc, nFontIdx, ctx.currentStyle, eacWORD, ctx.fUserScale);
   pWord->SetTextA(sText.c_str());
   return pWord;
}
