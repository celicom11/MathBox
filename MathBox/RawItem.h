#pragma once
#include "MathItem.h"
class CRawItem {
//DATA
   int            m_nTokenIdx{ -1 };
   EnumDelimType  m_edt{ edtNotDelim };
   uint32_t       m_nPrimes{ 0 };
   uint32_t       m_nUnicode{0};              //delimiter{including '.'!), \\(0xA) or &(0x26)
   CMathItem*     m_pBase{ nullptr };         //external!
   CMathItem*     m_pSubScript{ nullptr };    //external!
   CMathItem*     m_pSuperScript{ nullptr };  //external!
public:
//CTOR/DTOR/INITS
   CRawItem(int nTokenIdx, CMathItem* pBase = nullptr):
      m_nTokenIdx(nTokenIdx),m_pBase(pBase){};
   ~CRawItem() = default;
   bool InitDelimiter(const string& sDelim, EnumDelimType edt);
   void InitAmp() { m_nUnicode = 0x26; }
   void InitNewLine() { m_nUnicode = 0xA; }
//ATTS
   bool IsDelimiter() const { return m_edt != edtNotDelim; }
   bool IsNewLine() const { return m_nUnicode == 0xA; }
   bool IsAmp() const { return m_nUnicode == 0x26; }
   bool HasMathItem() const {
      return m_pBase || m_pSuperScript || m_pSubScript;
   }
   int TokenIdx() const { return m_nTokenIdx; }
   uint32_t Unicode() const { return m_nUnicode; }
   CMathItem* Base() const { return m_pBase; }
//METHODS
   void Delete() {//cleanup!
      delete m_pBase; m_pBase = nullptr;
      delete m_pSubScript; m_pSubScript = nullptr;
      delete m_pSuperScript; m_pSuperScript = nullptr;
   }
   bool AddSubScript(CMathItem* pSubScript);
   bool AddPrime(); //"^\\prime", can be multiple!
   bool AddSuperScript(CMathItem* pSuperScript);
   CMathItem* BuildItem(const CMathStyle& style, float fUserScale, int32_t nSize = 0) const;
};