#include "stdafx.h"
#include "OpenCloseBuilder.h"
#include "ExtGlyphBuilder.h"
#include "HBoxItem.h"
#include "LMFontManager.h"
#include "WordItem.h"

extern CLMFontManager g_LMFManager;

namespace {
   inline bool _IsAlpha(char cChar) { return (cChar >= 'a' && cChar <= 'z') || (cChar >= 'A' && cChar <= 'Z'); }
   uint32_t _GetDelimiterUnicode(PCSTR szText, OUT PCSTR& szNext, OUT EnumDelimType& edtNative) {
      szNext = szText + 1;
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
         const SLMMGlyph* pGlyph = g_LMFManager.GetLMMGlyphByCmd(sCmd.c_str());
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
         if (*szText == '[' || *szText == '(')
            edtNative = edtOpen;
         else if (*szText == ']' || *szText == ')')
            edtNative = edtClose;
         else if (*szText == '|')
            edtNative = edtAny;
         else if (*szText == '/')
            edtNative = edtAny;
         else
            return 0;
         return uint32_t(*szText);
      }
      return 0; //not found
   }
}
bool COpenCloseBuilder::ConsumeDelimiter(PCSTR szCmd, const CMathStyle& style, OUT PCSTR& szNext, 
                                          OUT SLMMDelimiter& lmmDelimiter) {
   _ASSERT_RET(szCmd && *szCmd, false);
   lmmDelimiter.nVariantIdx = 0; //default
   lmmDelimiter.nUni = _GetDelimiterUnicode(szCmd, szNext, lmmDelimiter.edt);
   if (lmmDelimiter.nUni)
      return true;
   lmmDelimiter.edt = edtNotDelim;
   //else - check delim-size OR virtual prefix 
   szNext = szCmd;
   if (szCmd[0] != '\\')
      return false;
   ++szNext;
   if (szNext[0] == 'l' && szNext[1] == 'e' && szNext[2] == 'f' && szNext[3] == 't') {
      szNext += 4;
      lmmDelimiter.edt = edtOpen;
      if (*szNext == '.') {
         ++szNext;
         lmmDelimiter.nUni = 0; //hidden
         return true;
      }
      lmmDelimiter.nVariantIdx = -1;
   }
   else if (szNext[0] == 'r' && szNext[1] == 'i' && szNext[2] == 'g' && szNext[3] == 'h' && szNext[4] == 't') {
      szNext += 5;
      lmmDelimiter.edt = edtClose;
      if (*szNext == '.') {
         ++szNext;
         lmmDelimiter.nUni = 0; //hidden
         return true;
      }
      lmmDelimiter.nVariantIdx = -1;
   }
   else if ((szNext[0] == 'b' || szNext[0] == 'B') && szNext[1] == 'i' && szNext[2] == 'g') {
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
   if (style.Style() != etsDisplay && lmmDelimiter.nVariantIdx)
      --lmmDelimiter.nVariantIdx;//decrease size for compact mode
   PCSTR szText = szNext;
   EnumDelimType edtNative{ edtNotDelim };
   lmmDelimiter.nUni = _GetDelimiterUnicode(szText, szNext, edtNative);
   if (lmmDelimiter.nUni && lmmDelimiter.edt == edtNotDelim)
      lmmDelimiter.edt = edtNative;//determine type of the delimiter by the glyph's class
   return lmmDelimiter.nUni != 0;
}
CMathItem* COpenCloseBuilder::BuildDelimiter(const SLMMDelimiter& lmmDelimiter, const CMathStyle& style, float fUserScale) {
   _ASSERT_RET(lmmDelimiter.edt != edtNotDelim, nullptr); 
   if (!lmmDelimiter.nUni) {
      //TODO: virt/empty MathItem for invisible open/close items!
      _ASSERT_RET(0, nullptr);
   }
   CWordItem* pWordItem = new CWordItem(FONT_LMM, style, eacWORD, fUserScale);
   CMathItem* pRet = pWordItem;

   if (lmmDelimiter.nVariantIdx <= 0) //just a basic glyph
      pWordItem->SetText({ lmmDelimiter.nUni });
   else { //lmmDelimiter.nVariantIdx >= 1
      uint16_t nGIdx = CExtGlyphBuilder::GetGlyphIndexBySizeIdx(lmmDelimiter.nUni, lmmDelimiter.nVariantIdx);
      if(nGIdx)
         pWordItem->SetGlyphIndexes({ nGIdx });
      else {
         pWordItem->SetText({ lmmDelimiter.nUni });//base size
         uint32_t nSize = pWordItem->Box().Height();
         if (lmmDelimiter.nVariantIdx == 1)
            nSize = nSize * 11 / 10;
         else if (lmmDelimiter.nVariantIdx == 2)
            nSize = nSize * 33 / 20;
         else if (lmmDelimiter.nVariantIdx == 3)
            nSize = nSize * 22 / 10;
         else if (lmmDelimiter.nVariantIdx == 4)
            nSize = nSize * 55 / 20;
         delete pWordItem;
         pRet = CExtGlyphBuilder::BuildVerticalGlyph(lmmDelimiter.nUni, style, nSize, fUserScale);
      }
   }
   switch (lmmDelimiter.edt) {
   case edtOpen: pRet->SetAtom(etaOPEN); break;
   case edtClose: pRet->SetAtom(etaCLOSE); break;
   case edtFence: pRet->SetAtom(etaINNER); break;
   default: pRet->SetAtom(etaORD);
   }
   return pRet;
}
CMathItem* COpenCloseBuilder::BuildOpenClose(uint32_t nUniOpen, uint32_t nUniClose, CMathItem* pItem,
   const CMathStyle& style, float fUserScale) {
   _ASSERT_RET(nUniOpen || nUniClose, nullptr);
   float fScale = style.StyleScale() * fUserScale;
   CHBoxItem* pHBox = new CHBoxItem(style);
   int32_t nSize = otfAxisHeight;//dummy/default
   int32_t nAxisY = 0;
   if (pItem) {
      nAxisY = pItem->Box().AxisY();
      nSize = 2*max(nAxisY - pItem->Box().Top(), pItem->Box().Bottom() - nAxisY);
   }
   if (nUniOpen) {
      CMathItem* pOpen = CExtGlyphBuilder::BuildVerticalGlyph(nUniOpen, style, nSize, fUserScale);
      _ASSERT_RET(pOpen, nullptr);
      pHBox->AddItem(pOpen);
   }
   if(pItem)
      pHBox->AddItem(pItem);
   if (nUniClose) {
      CMathItem* pClose = CExtGlyphBuilder::BuildVerticalGlyph(nUniClose, style, nSize, fUserScale);
      _ASSERT_RET(pClose, nullptr);
      pHBox->AddItem(pClose);
   }
   pHBox->Update();
   return pHBox;
}


