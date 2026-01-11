#include "stdafx.h"
#include "MathSymBuilder.h"
#include "FontStyleHelper.h"
#include "WordItem.h"
#include "HBoxItem.h"
#include "UnderOverBuilder.h"

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
   bool _IsMathOpLim(PCSTR szText) {
      for (PCSTR szOp : _aMathOpsLim) {
         if (0 == strcmp(szText, szOp))
            return true;
      }
      return false;
   }
   struct SymCmdPair {
      PCSTR szCmd;
      uint32_t nUni1;
      uint32_t nUni2;
   };
   static const SymCmdPair _aTwoSymCmds[] = {
      {"Eqqcolon",            '=',0x2237},
      {"approxcolon",         0x2248,0x2236},
      {"approxcoloncolon",    0x2248,0x2237},
      {"colonapprox",         0x2236,0x2248},
      {"Colonapprox",         0x2237,0x2248},
      
   };
   const SymCmdPair* _GetSymCmdPair(PCSTR szText) {
      for (const SymCmdPair& entry: _aTwoSymCmds) {
         if (0 == strcmp(szText, entry.szCmd))
            return &entry;
      }
      return nullptr;
   }
}

bool CMathSymBuilder::CanTakeCommand(PCSTR szCmd) const {
   _ASSERT_RET(szCmd && *szCmd, false);
   if (*szCmd == '\\')
      ++szCmd;
   return _IsSpecialChar(*szCmd) || _IsMathOp(szCmd) || _IsMathOpLim(szCmd) ||
      m_Doc.LMFManager().GetLMMGlyphByCmd(szCmd) != nullptr;
}

CMathItem* CMathSymBuilder::BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) {
   _ASSERT_RET(szCmd && pParser, nullptr);
   _ASSERT_RET(CanTakeCommand(szCmd), nullptr);
   if (*szCmd == '\\')
      ++szCmd;
   const SParserContext& ctx(pParser->GetContext());
   CMathItem* pRet = nullptr;
   SMathFontStyle mfStyle;
   if (!ctx.sFontCmd.empty() && !FontStyleHelper::_GetMathFontStyle(ctx.sFontCmd, mfStyle)) {
      if (!pParser->HasError())
         pParser->SetError("Unknown font '" + ctx.sFontCmd + "'");
      return nullptr;

   }
   if (_IsSpecialChar(*szCmd)) {
      CWordItem* pRet = new CWordItem(m_Doc, mfStyle.nLetterDigitsFont, ctx.currentStyle, eacWORD, ctx.fUserScale);
      pRet->SetText({ uint32_t(*szCmd) });
   }
   else if (_IsMathOp(szCmd)) 
      pRet = BuildMathOperator_(szCmd, pParser);
   else if (_IsMathOpLim(szCmd))
      pRet = BuildMathOperatorLim_(szCmd, pParser);
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
         pLmmGlyph = m_Doc.LMFManager().GetLMMGlyphByCmd(sArg.c_str());
      }
      if (!pLmmGlyph) // try sym w/o font decoration
         pLmmGlyph = m_Doc.LMFManager().GetLMMGlyphByCmd(szCmd);
      _ASSERT_RET(pLmmGlyph && pLmmGlyph->nIndex, nullptr);//snbh!
      pRet = BuildLMMSymbol_(pLmmGlyph, ctx.currentStyle, ctx.fUserScale);

   }

   return pRet;
}
// BUILDERS
CMathItem* CMathSymBuilder::BuildMathOperator_(PCSTR szOp, IParserAdapter* pParser) {
   const SParserContext& ctx(pParser->GetContext());

   const CMathStyle& style = ctx.currentStyle;
   float fUserScale = ctx.fUserScale;
   CWordItem* pRet = new CWordItem(m_Doc, FONT_ROMAN_REGULAR, style, eacMATHOP, fUserScale);
   string sOpStr(szOp);
   if (sOpStr == "mod")
      sOpStr = "  mod ";      //q&d
   if (sOpStr == "bmod") {
      sOpStr = "mod";
      pRet->SetAtom(etaBIN);
   }
   else if (sOpStr == "pmod") {
      //need to tak next item to build (mod ?)
      sOpStr = " (mod ";
      pRet->SetTextA(sOpStr.c_str());
      CMathItem* pNextItem = pParser->ConsumeItem(elcapAny, ctx);
      if(!pNextItem) {
         delete pRet;
         if (!pParser->HasError())
            pParser->SetError("Missing argument for 'pmod'");
         return nullptr;
      }
      CWordItem* pRightBrk = new CWordItem(m_Doc, FONT_ROMAN_REGULAR, style, eacMATHOP, fUserScale);
      pRightBrk->SetTextA(")");
      CHBoxItem* pHBox = new CHBoxItem(m_Doc, style);
      pHBox->AddItem(pRet);
      pHBox->AddItem(pNextItem);
      pHBox->AddItem(pRightBrk);
      pHBox->Update();
      return pHBox;
   }
   else if (sOpStr == "operatorname") {
      string sTemp;
      //need next token
      EnumTokenType ettNext = pParser->GetTokenData(sTemp);
      if (ettNext != ettFB_OPEN) {
         if (!pParser->HasError())
            pParser->SetError("Missed { for '\\operatorname'");
      }
      if (!pParser->HasError()) {
         pParser->SkipToken(); //skip {
         ettNext = pParser->GetTokenData(sOpStr);
         if (ettNext != ettALNUM) {
            if (!pParser->HasError())
               pParser->SetError("Bad argument for '\\operatorname'");
         }
      }
      if (!pParser->HasError()) {
         pParser->SkipToken(); //skip arg
         ettNext = pParser->GetTokenData(sTemp);
         if (ettNext != ettFB_CLOSE) {
            if (!pParser->HasError())
               pParser->SetError("Missed } for '\\operatorname'");
         }
      }
      if (pParser->HasError()) {
         delete pRet;
         return nullptr;
      }
      //else 
      pParser->SkipToken(); //skip {
      // build operatorname
      pRet->SetTextA(sOpStr.c_str());
   }
   pRet->SetTextA(sOpStr.c_str());
   return pRet;
}
CMathItem* CMathSymBuilder::BuildMathOperatorLim_(PCSTR szOp, IParserAdapter* pParser) {
   const SParserContext& ctx(pParser->GetContext());

   const CMathStyle& style = ctx.currentStyle;
   float fUserScale = ctx.fUserScale;
   CWordItem* pRet = new CWordItem(m_Doc, FONT_ROMAN_REGULAR, style, eacMATHOP, fUserScale);
   string sOpStr(szOp),sUnderOver;

   if (sOpStr == "varinjlim")
      sUnderOver = "underrightarrow";
   else if (sOpStr == "varliminf")
      sUnderOver = "underline";
   else if (sOpStr == "varlimsup")
      sUnderOver = "overline";
   else if(sOpStr == "varprojlim")
      sUnderOver = "underleftarrow";

   //command -> operator name
   if(!sUnderOver.empty())
      sOpStr = "lim";
   else if (sOpStr == "argmax")
      sOpStr = "arg max";
   else if (sOpStr == "argmin")
      sOpStr = "arg min";
   else if (sOpStr == "injlim")
      sOpStr = "inj lim";
   else if (sOpStr == "liminf")
      sOpStr = "lim inf";
   else if (sOpStr == "limsup")
      sOpStr = "lim sup";
   else if (sOpStr == "projlim")
      sOpStr = "proj lim";

   pRet->SetTextA(sOpStr.c_str());
   if (!sUnderOver.empty()) {
      //need to build under/overbrace item
      CMathItem* pUnderOver = CUnderOverBuilder::_BuildUnderOverItem(pRet, sUnderOver.c_str(), ctx);
      if(!pUnderOver) {
         delete pRet;
         if (!pParser->HasError())
            pParser->SetError("Failed to build operator with limits for '" + string(szOp) + "'");
         return nullptr;
      }
      //correct index placement
      pUnderOver->SetIdxPlacement(sUnderOver == "overline" ? eipOverscript : eipUnderscript);
      return pUnderOver;
   }
   return pRet;
}
CWordItem* CMathSymBuilder::BuildLMMSymbol_(const SLMMGlyph* pLmmGlyph, const CMathStyle& style, float fUserScale) {
   CWordItem* pRet = new CWordItem(m_Doc, FONT_LMM, style, eacWORD, fUserScale);
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


