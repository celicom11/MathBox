#pragma once
#include "MathItem.h"


class CWordItem : public CMathItem {
   int16_t                 m_nFontIdx{ 0 };  // 0-LMM!
   SBounds                 m_Bounds;         //bounds around base_origin in design units
   vector<uint16_t>        m_vGIndexes;
   vector<uint32_t>        m_vGUnicode;
   vector<SGlyphMetrics>   m_vGMetrics;

public:
//CTOR/INIT
   CWordItem() = delete;
   CWordItem(IDocParams& doc, int16_t nFontIdx, const CMathStyle& style, 
             EnumMathItemType eType = eacWORD, float fUserScale = 1.0f);
   static CWordItem* _MergeWords(CWordItem* pWord1, CWordItem* pWord2);
//ATTS
   int16_t GetFontIdx() const { return m_nFontIdx; }
   bool IsEmpty() const { return m_vGIndexes.empty(); }
   const SBounds& Bounds() const {
      return m_Bounds;
   }
   uint32_t GlyphCount() const {
      return (uint32_t)m_vGIndexes.size();
   }
   uint32_t GetUnicodeAt(uint32_t nIdx) const {
      if (nIdx < m_vGUnicode.size())
         return m_vGUnicode[nIdx];
      return 0;
   }
   uint16_t GetIndexAt(uint32_t nIdx) const {
      if (nIdx < m_vGIndexes.size())
         return m_vGIndexes[nIdx];
      return 0;
   }
//METHODS  
   bool SetText(PCWSTR szText) {
      vector<uint32_t> vUniCodePoints;
      while (*szText) {
         vUniCodePoints.push_back((uint32_t)*szText);
         ++szText;
      }
      return SetText(vUniCodePoints);
   }
   bool SetTextA(PCSTR szText) {
      vector<uint32_t> vUniCodePoints;
      while (*szText) {
         vUniCodePoints.push_back((uint32_t)*szText);
         ++szText;
      }
      return SetText(vUniCodePoints);
   }
   bool SetText(const vector<uint32_t>& vUniCodePoints);
   bool SetGlyphIndexes(const vector<uint16_t>& vGIndexes);
//CMathItem Implementation
   void Draw(SPointF ptfAnchor, IDocRenderer& docr) override;

};