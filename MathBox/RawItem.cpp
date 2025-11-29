#include "stdafx.h"
#include "RawItem.h"
#include "OpenCloseBuilder.h"
#include "WordItem.h"
#include "HBoxItem.h"
#include "IndexedBuilder.h"
#include "VBoxBuilder.h"

bool CRawItem::AddSubScript(CMathItem* pSubScript) {
   if (!pSubScript || m_pSubScript)
      return false; //double subscript
   m_pSubScript = pSubScript;
   return true;
}
bool CRawItem::AddSuperScript(CMathItem* pSuperScript) {
   if (!pSuperScript || m_pSuperScript)
      return false; //double subscript
   m_pSuperScript = pSuperScript;
   return true;
}
bool CRawItem::AddPrime() {
   if (m_pSuperScript)
      return false; //double subscript
   ++m_nPrimes;
   return true;
}

bool CRawItem::InitDelimiter(const string& sDelim, EnumDelimType edt) {
   _ASSERT_RET(!m_pBase && edt != edtAny, false);//snbh!
   SLMMDelimiter lmmDelimiter;
   if (!COpenCloseBuilder::_GetDelimiter(sDelim.c_str(), etsDisplay, lmmDelimiter))
      return false;
   m_edt = edt;
   m_nUnicode = lmmDelimiter.nUni;
   return IsDelimiter();
}
CMathItem* CRawItem::BuildItem(const CMathStyle& style, float fUserScale, int32_t nSize) const {
   CMathItem* pRet = m_pBase; //default
   if (IsDelimiter() && m_nUnicode != '.') {
      pRet = COpenCloseBuilder::_BuildDelimiter(m_nUnicode, m_edt, nSize, style, fUserScale);
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
         CWordItem* pPrimes = new CWordItem(FONT_LMM, styleSuper, eacUNK, fUserScale);
         vector<UINT32> vUniCodePoints(m_nPrimes, 0x2032);
         pPrimes->SetText(vUniCodePoints);
         if (pSuperScript) {
            //pack together in hbox
            CHBoxItem* pHBox = new CHBoxItem(styleSuper);
            pHBox->AddItem(pPrimes);
            pHBox->AddItem(m_pSuperScript);
            pSuperScript = pHBox;
         }
         else
            pSuperScript = pPrimes;
      }
      if (pRet && (pSuperScript || m_pSubScript))
         pRet = CIndexedBuilder::BuildIndexed(style, fUserScale, pRet, pSuperScript, m_pSubScript);
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