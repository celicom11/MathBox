#include "stdafx.h"
#include "RawItem.h"
#include "OpenCloseBuilder.h"
#include "WordItem.h"
#include "HBoxItem.h"
#include "IndexedBuilder.h"
#include "VBoxBuilder.h"

bool CRawItem::AddSubScript(CMathItem* pSubScript, int nTkIdxStart, int nTkIdxEnd) {
   if (!pSubScript || m_pSubScript)
      return false; //double subscript
   _ASSERT(m_nTkIdxEnd < nTkIdxStart || (!HasMathItem() && m_nTkIdxEnd == nTkIdxStart));
   m_pSubScript = pSubScript;
   m_nTkIdxEnd = nTkIdxEnd;
   return true;
}
bool CRawItem::AddSuperScript(CMathItem* pSuperScript, int nTkIdxStart, int nTkIdxEnd) {
   if (!pSuperScript || m_pSuperScript)
      return false; //double subscript
   _ASSERT(m_nTkIdxEnd < nTkIdxStart || (!HasMathItem() && m_nTkIdxEnd == nTkIdxStart));
   m_pSuperScript = pSuperScript;
   m_nTkIdxEnd = nTkIdxEnd;
   return true;
}
bool CRawItem::AddPrime(int nTkIdx) {
   if (m_pSuperScript)
      return false; //double subscript
   ++m_nPrimes;
   _ASSERT(m_nTkIdxEnd < nTkIdx);
   m_nTkIdxEnd = nTkIdx;
   return true;
}
bool CRawItem::TryAppendWord(CMathItem* pWordItem, int nTkIdxStart, int nTkIdxEnd) {
   if (!m_pBase || m_pSubScript || m_pSuperScript || m_nUnicode)
      return false;
   if (m_pBase->Type() != eacWORD || pWordItem->Type() != eacWORD)
      return false;
   CWordItem* pBaseWord = static_cast<CWordItem*>(m_pBase);
   CWordItem* pWord2 = static_cast<CWordItem*>(pWordItem);
   CWordItem* pMerged = CWordItem::_MergeWords(pBaseWord, pWord2);
   if (!pMerged)
      return false;
   delete m_pBase;
   m_pBase = pMerged;
   _ASSERT(m_nTkIdxEnd < nTkIdxStart);
   m_nTkIdxEnd = nTkIdxEnd;
   return true;
}
bool CRawItem::InitDelimiter(ILMFManager& lmfm, const string& sDelim, EnumDelimType edt) {
   _ASSERT_RET(!m_pBase && edt != edtAny, false);//snbh!
   SLMMDelimiter lmmDelimiter;
   if (!COpenCloseBuilder::_GetDelimiter(lmfm, sDelim.c_str(), etsDisplay, lmmDelimiter))
      return false;
   m_edt = edt;
   m_nUnicode = lmmDelimiter.nUni;
   return IsDelimiter();
}
CMathItem* CRawItem::BuildItem(IDocParams& doc, const CMathStyle& style, float fUserScale, int32_t nSize) const {
   CMathItem* pRet = m_pBase; //default
   if (IsDelimiter() && m_nUnicode != '.') {
      pRet = COpenCloseBuilder::_BuildDelimiter(doc, m_nUnicode, m_edt, nSize, style, fUserScale);
      //fractions and \left...\right constructions are treated as “inner” subformulas (c)
      if(m_edt == edtOpen || m_edt == edtClose) 
         pRet->SetAtom(etaINNER);
      //else
      //   pRet->SetAtom(etaREL);
   }
   if (m_nPrimes || m_pSubScript || m_pSuperScript) { //build indexed item
      CMathItem* pSuperScript = m_pSuperScript;
      if (m_nPrimes) {//build prime's superscript
         CMathStyle styleSuper(style);
         styleSuper.ToSuperscriptStyle();
         CWordItem* pPrimes = new CWordItem(doc, FONT_LMM, styleSuper, eacUNK, fUserScale);
         vector<uint16_t> vIndexes(m_nPrimes, 2981); //minute.st
         pPrimes->SetGlyphIndexes(vIndexes);
         if (pSuperScript) {
            //pack together in hbox
            CHBoxItem* pHBox = new CHBoxItem(doc, styleSuper);
            pHBox->AddItem(pPrimes);
            pHBox->AddItem(m_pSuperScript);
            pSuperScript = pHBox;
         }
         else
            pSuperScript = pPrimes;
      }
      if (pRet && (pSuperScript || m_pSubScript))
         pRet = CIndexedBuilder::_BuildIndexed(style, fUserScale, pRet, pSuperScript, m_pSubScript);
      else if (!pRet && pSuperScript && m_pSubScript)
         pRet = CVBoxBuilder::_BuildGenFraction(style, fUserScale, pSuperScript, m_pSubScript);
      else { 
         if (!pRet)
            pRet = pSuperScript;
         if (!pRet)
            pRet = m_pSubScript;
      }
   }
   return pRet;
}