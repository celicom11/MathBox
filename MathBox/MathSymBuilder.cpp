#include "stdafx.h"
#include "MathSymBuilder.h"
#include "LMFontManager.h"
#include "HBoxItem.h"
#include "WordItem.h"

extern CLMFontManager g_LMFManager;

namespace {
   inline bool _IsSpecialChar(char cChar) {
      static const string _sSpecial{ "$%&~_^\\{}" };
      return _sSpecial.find(cChar) != string::npos;
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
}

bool CMathSymBuilder::CanTakeCommand(PCSTR szCmd, bool bInTextMode) const {
   _ASSERT_RET(szCmd && *szCmd, false);
   if (*szCmd == '\\')
      ++szCmd;
   return !bInTextMode && (_IsSpecialChar(*szCmd) || _IsMathOp(szCmd) || 
      g_LMFManager.GetLMMGlyphByCmd(szCmd) != nullptr);
}

CMathItem* CMathSymBuilder::BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) {
   _ASSERT_RET(szCmd && pParser, nullptr);
   _ASSERT_RET(CanTakeCommand(szCmd, false), nullptr);
   if (*szCmd == '\\')
      ++szCmd;
   SParserContext ctx(pParser->GetContext());
   CMathItem* pRet = nullptr;
   SMathFontStyle mfStyle;
   if (!g_LMFManager._GetMathFontStyle((ctx.sFontCmd.empty() ? "mathnormal" : ctx.sFontCmd), mfStyle)) {
      if (!pParser->HasError())
         pParser->SetError("Unknown font '" + ctx.sFontCmd + "'");
      return nullptr;

   }
   if (_IsSpecialChar(*szCmd)) {
      CWordItem* pRet = new CWordItem(mfStyle.nLetterDigitsFont, ctx.currentStyle, eacWORD, ctx.fUserScale);
      pRet->SetText({ UINT32(*szCmd) });
   }
   else if (_IsMathOp(szCmd)) 
      pRet = _BuildMathOperator(szCmd, ctx.currentStyle, ctx.fUserScale);
   else {
      //map LMMSym WITH font_cmd first
      const SLMMGlyph* pLmmGlyph = nullptr;
      if (mfStyle.szLatexUnicodeCmd) {
         string sArg = "\\" + string(mfStyle.szLatexUnicodeCmd) + "{\\" + szCmd + '}';
         PCSTR szNextBracket = mfStyle.szLatexUnicodeCmd;
         while (szNextBracket = strchr(szNextBracket, '{')) {
            sArg.push_back('}');
            ++szNextBracket;
         }
         pLmmGlyph = g_LMFManager.GetLMMGlyphByCmd(sArg.c_str());
      }
      if (!pLmmGlyph) // try sym w/o font decoration
         pLmmGlyph = g_LMFManager.GetLMMGlyphByCmd(szCmd);
      _ASSERT_RET(pLmmGlyph && pLmmGlyph->nIndex, nullptr);//snbh!
      pRet = _BuildLMMSymbol(pLmmGlyph, ctx.currentStyle, ctx.fUserScale);

   }

   return pRet;
}

