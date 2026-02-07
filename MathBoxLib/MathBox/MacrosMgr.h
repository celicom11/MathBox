#pragma once
#include "MathItem.h"

class CMacrosMgr {
//TYPES
struct SMacrosEntry {
   uint32_t    nLen{ 0 };
   string      sFileName;
};
//DATA
   string                  m_sMacros;     // all macros, combined
   vector<SMacrosEntry>    m_vCatalog;
public:
//CTOR/DTOR
   CMacrosMgr() = default;
//ATTS
   bool isEmpty() const { return m_sMacros.empty(); }
   const string& getMacrosText() const { return m_sMacros; }
//METHODS
   bool getTokenText(const STexToken& token, OUT string& sText) const {
      _ASSERT_RET(token.nPos <= m_sMacros.size() - token.nLen, false);
      sText = string(m_sMacros.c_str() + token.nPos, token.nLen);
      return true;
   }
   bool getTokenSource(const STexToken& token, OUT int32_t& nPos, OUT string& sFileName) const {
      _ASSERT_RET(token.nPos < m_sMacros.size() - token.nLen, false);
      int nFileIdx = 0;
      nPos = 0;
      while (nFileIdx < m_vCatalog.size()) {
         const int32_t nPosMNext = nPos + m_vCatalog[nFileIdx].nLen;
         if (token.nPos >= nPos && token.nPos < nPosMNext) {
            _ASSERT_RET(token.nPos + token.nLen < nPosMNext, false);//snbh!
            sFileName = m_vCatalog[nFileIdx].sFileName;
            nPos = token.nPos - nPos; //pos relative file start
            return true;//done
         }
         nPos += m_vCatalog[nFileIdx].nLen;
         ++nFileIdx;
      }
      _ASSERT_RET(0, false);//snbh!
   }
   void addMacros(PCSTR szMacros, PCSTR szFileName) {
      _ASSERT_RET(szMacros && szFileName, ); //snbh!
      m_vCatalog.push_back({(uint32_t)strlen(szMacros), string(szFileName) });
      m_sMacros += szMacros;
   }
};
