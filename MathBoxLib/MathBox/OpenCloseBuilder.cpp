#include "stdafx.h"
#include "OpenCloseBuilder.h"
#include "ExtGlyphBuilder.h"
#include "HBoxItem.h"
#include "WordItem.h"

namespace {
   inline bool _IsAlpha(char cChar) { return (cChar >= 'a' && cChar <= 'z') || (cChar >= 'A' && cChar <= 'Z'); }
   uint32_t _GetDelimiterUnicode(ILMFManager& lmfManager, PCSTR szText, OUT EnumDelimType& edtNative) {
      PCSTR szNext = szText+1;
      if (*szText == '\\') {
         if (*szNext == '{') { // \lbrace
            ++szNext;
            edtNative = edtOpen;
            return '{';
         }
         if (*szNext == '}') { // \rbrace
            ++szNext;
            edtNative = edtClose;
            return '}';
         }
         if (*szNext == '|') { // \Vert
            ++szNext;
            edtNative = edtAny;
            return 0x2016;
         }
         //else
         while (_IsAlpha(*szNext)) {
            ++szNext;
         }
         string sCmd(szText, szNext - szText);
         if(sCmd == "\\lvert" || sCmd == "\\rvert") {
            edtNative = sCmd == "\\lvert"? edtOpen : edtClose;
            return '|';
         }
         if (sCmd == "\\lVert" || sCmd == "\\rVert") {
            edtNative = sCmd == "\\lVert" ? edtOpen : edtClose;
            return 0x2016;
         }
         const SLMMGlyph* pGlyph = lmfManager.GetLMMGlyphByCmd(sCmd.c_str());
         if (!pGlyph)
            return 0;
         if (pGlyph->eClass == egcOpen || pGlyph->eClass == egcClose) {
            edtNative = pGlyph->eClass == egcOpen ? edtOpen : edtClose;
            return pGlyph->nUnicode;
         }
         if (pGlyph->nUnicode == '\\' || pGlyph->nUnicode == '/') {
            edtNative = edtAny; //actually Ord, not a delimiter!
            return pGlyph->nUnicode;
         }
         //else
         if (pGlyph->eClass == egcRel) {
            edtNative = edtFence;
            //legacy\dual relation | fence delimiter-like glyphs
            if (pGlyph->nUnicode == 0x2191 || pGlyph->nUnicode == 0x2193 || pGlyph->nUnicode == 0x2195 ||
               pGlyph->nUnicode == 0x21D1 || pGlyph->nUnicode == 0x21D3 || pGlyph->nUnicode == 0x21D5 || 
               pGlyph->nUnicode == 0x2223)
               return pGlyph->nUnicode;
         }
      }
      else {
         if (*szText == '[' || *szText == '(' || *szText == '{')
            edtNative = edtOpen;
         else if (*szText == ']' || *szText == ')' || *szText == '}')
            edtNative = edtClose;
         else if (*szText == '|' || *szText == '/' || *szText == '.')
            edtNative = edtAny;
         else
            return 0;
         return uint32_t(*szText);
      }
      return 0; //not found
   }
}
bool COpenCloseBuilder::CanTakeCommand(PCSTR szCmd) const {
   _ASSERT_RET(szCmd && *szCmd, false);
   if (*szCmd == '\\')
      ++szCmd;
   vector<string> vCmds{
      "left","right","middle","lvert","rvert","lVert","rVert",
      "big","bigl","bigm","bigr",
      "Big","Bigl","Bigm","Bigr",
      "bigg","biggl","biggm","biggr",
      "Bigg","Biggl","Biggm","Biggr",
   };
   return std::find(vCmds.begin(), vCmds.end(), szCmd) != vCmds.end();
}
CMathItem* COpenCloseBuilder::BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) {
   _ASSERT_RET(szCmd && pParser, nullptr);
   const SParserContext& ctx = pParser->GetContext();
   _ASSERT_RET(CanTakeCommand(szCmd), nullptr);
   string sCmd(szCmd),sToken;
   if (sCmd != "\\lvert" && sCmd != "\\rvert" && sCmd != "\\lVert" && sCmd != "\\rVert") {
      //check/consume/ next token
      EnumTokenType ettNext = pParser->GetTokenData(sToken);
      if (ettNext != ettCOMMAND && ettNext != ettNonALPHA) {
         if (!pParser->HasError())
            pParser->SetError("Missing delimiter after '\\" + sCmd + "'");
         return nullptr;
      }
      pParser->SkipToken();
      _ASSERT_RET(sToken != ".", nullptr); //not sure if can be here
   }
   string sCmdFull = szCmd + sToken;
   SLMMDelimiter lmmDelimiter;
   if (!_GetDelimiter(pParser->Doc().LMFManager(), sCmdFull.c_str(), ctx.currentStyle, lmmDelimiter) ||
      lmmDelimiter.edt == edtNotDelim || 0 == lmmDelimiter.nUni) {
      if (!pParser->HasError())
         pParser->SetError("Unknown delimiter '" + sToken + "' after '" + sCmd + "'");
      return nullptr;
   }
   CMathItem* pRet = BuildDelimiter_(pParser->Doc(), lmmDelimiter, ctx.currentStyle, ctx.fUserScale);
   if (!pRet) {
      _ASSERT(0); //snbh
      if (!pParser->HasError())
         pParser->SetError("Failed to build delimiter '" + sToken + "' after '\\" + sCmd + "'");
   }
   return pRet;
}

bool COpenCloseBuilder::_GetDelimiter(ILMFManager& lmfManager, PCSTR szCmd, const CMathStyle& style, OUT SLMMDelimiter& lmmDelimiter) {
   _ASSERT_RET(szCmd && *szCmd, false);
   lmmDelimiter.nVariantIdx = 0; //default
   lmmDelimiter.nUni = _GetDelimiterUnicode(lmfManager, szCmd, lmmDelimiter.edt);
   if (lmmDelimiter.nUni)
      return true;
   lmmDelimiter.edt = edtNotDelim;
   //else - check delim-size OR virtual prefix 
   if (szCmd[0] != '\\')
      return false;
   PCSTR szNext = szCmd + 1;
   if ((szNext[0] == 'b' || szNext[0] == 'B') && szNext[1] == 'i' && szNext[2] == 'g') {
      szNext += 3;
      if (*szNext == 'g') {
         ++szNext;
         lmmDelimiter.nVariantIdx = szCmd[1] == 'B' ? 4 : 3;
      }
      else
         lmmDelimiter.nVariantIdx = szCmd[1] == 'B' ? 2 : 1;
      if (*szNext == 'l') {
         ++szNext;
         lmmDelimiter.edt = edtOpen;
      }
      else if (*szNext == 'r') {
         ++szNext;
         lmmDelimiter.edt = edtClose;
      }
      else if (*szNext == 'm') {
         ++szNext;
         lmmDelimiter.edt = edtFence; //FENCE gets only only with m!
      }
      else
         lmmDelimiter.edt = edtAny;
   }
   else
      return false;//unkn
   if (style.Style() != etsDisplay && lmmDelimiter.nVariantIdx > 0)
      --lmmDelimiter.nVariantIdx;//decrease size for compact mode
   EnumDelimType edtNative{ edtNotDelim };
   lmmDelimiter.nUni = _GetDelimiterUnicode(lmfManager, szNext, edtNative);
   //if (lmmDelimiter.nUni && lmmDelimiter.edt == edtNotDelim)
   //   lmmDelimiter.edt = edtNative;//determine type of the delimiter by the glyph's class
   return lmmDelimiter.nUni != 0;
}
//build by size Idx 0-4
CMathItem* COpenCloseBuilder::BuildDelimiter_(IDocParams& doc, const SLMMDelimiter& lmmDelimiter,
                                                const CMathStyle& style, float fUserScale) {
   _ASSERT_RET(lmmDelimiter.edt != edtNotDelim, nullptr); 
   _ASSERT_RET(lmmDelimiter.nUni, nullptr);

   uint32_t nSize = 0;//default

   if (lmmDelimiter.nVariantIdx > 0) {
      //get base size
      CWordItem* pWordItem = new CWordItem(doc, FONT_LMM, style, eacWORD, fUserScale);
      pWordItem->SetText({ lmmDelimiter.nUni });
      nSize = pWordItem->Box().Height();
      delete pWordItem;
      //calc target size
      if (lmmDelimiter.nVariantIdx == 1)
         nSize = nSize * 11 / 10;
      else if (lmmDelimiter.nVariantIdx == 2)
         nSize = nSize * 33 / 20;
      else if (lmmDelimiter.nVariantIdx == 3)
         nSize = nSize * 22 / 10;
      else if (lmmDelimiter.nVariantIdx == 4)
         nSize = nSize * 55 / 20;
   }
   return _BuildDelimiter(doc, lmmDelimiter.nUni, lmmDelimiter.edt, nSize, style, fUserScale);
}
CMathItem* COpenCloseBuilder::_BuildDelimiter(IDocParams& doc, uint32_t nUni, EnumDelimType edt, 
                                             uint32_t nSize, const CMathStyle& style, float fUserScale) {
   _ASSERT_RET(nUni, nullptr);
   CMathItem* pRet;
   if (nSize <= 0) {
      CWordItem* pWordItem = new CWordItem(doc, FONT_LMM, style, eacWORD, fUserScale);
      pWordItem->SetText({ nUni });
      pRet = pWordItem;
   }
   else {
      CExtGlyphBuilder egBuilder(doc);
      pRet = egBuilder.BuildVerticalGlyph(nUni, style, nSize, fUserScale);
   }
   switch (edt) {
   case edtOpen: pRet->SetAtom(etaOPEN); break;
   case edtClose: pRet->SetAtom(etaCLOSE); break;
   case edtFence: pRet->SetAtom(etaINNER); break;
   default: pRet->SetAtom(etaORD);
   }
   return pRet;
}

