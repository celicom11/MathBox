#pragma once
#include "MathItem.h"
class CRawItem {
//DATA
   int            m_nTkIdxStart{ -1 };
   int            m_nTkIdxEnd{ -1 };
   EnumDelimType  m_edt{ edtNotDelim };
   uint32_t       m_nPrimes{ 0 };
   uint32_t       m_nUnicode{0};              //delimiter{including '.'!), \\(0xA) or &(0x26)
   CMathItem*     m_pBase{ nullptr };         //tmp holding
   CMathItem*     m_pSubScript{ nullptr };    //tmp holding
   CMathItem*     m_pSuperScript{ nullptr };  //tmp holding
public:
//CTOR/DTOR/INITS
   CRawItem(int nTkIdxStart, int nTkIdxEnd, CMathItem* pBase = nullptr):
      m_nTkIdxStart(nTkIdxStart), m_nTkIdxEnd(nTkIdxEnd), m_pBase(pBase) {};
   ~CRawItem() = default;
   bool InitDelimiter(ILMFManager& lmfm, const string& sDelim, EnumDelimType edt);
   void InitAmp() { m_nUnicode = 0x26; }
   void InitNewLine() { m_nUnicode = 0xA; }
   void InitHLine() { m_nUnicode = '-'; }
//ATTS
   bool IsDelimiter() const { return m_edt != edtNotDelim; }
   bool IsNewLine() const { return m_nUnicode == 0xA; }
   bool IsAmp() const { return m_nUnicode == 0x26; }
   bool IsHLine() const { return m_nUnicode == '-'; }
   bool HasMathItem() const {
      return m_pBase || m_pSuperScript || m_pSubScript;
   }
   int TkIdxStart() const { return m_nTkIdxStart; }
   int TkIdxEnd() const { return m_nTkIdxEnd; }
   uint32_t Unicode() const { return m_nUnicode; }
   CMathItem* Base() const { return m_pBase; }
//METHODS
   void Delete() {//cleanup!
      delete m_pBase; m_pBase = nullptr;
      delete m_pSubScript; m_pSubScript = nullptr;
      delete m_pSuperScript; m_pSuperScript = nullptr;
   }
   bool AddSubScript(CMathItem* pSubScript, int nTkIdxStart, int nTkIdxEnd);
   bool AddPrime(int nTkIdxStart); //"^\\prime", can be multiple!
   bool AddSuperScript(CMathItem* pSuperScript, int nTkIdxStart, int nTkIdxEnd);
   bool TryAppendWord(CMathItem* pWordItem, int nTkIdxStart, int nTkIdxEnd);
   CMathItem* BuildItem(IDocParams& doc, const CMathStyle& style, float fUserScale, int32_t nSize = 0) const;
};