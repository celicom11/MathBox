#pragma once
#include "MathItem.h"
#include "GlyphRun.h"

class CWordItem : public CMathItem {
   CGlyphRun        m_GlyphRun;
public:
//CTOR/INIT
   CWordItem() = delete;
   CWordItem(int16_t nFontIdx, const CMathStyle& style, EnumMathItemType eType = eacWORD, float fUserScale = 1.0f);
   bool SetText(PCWSTR szText) {
      vector<UINT32> vUniCodePoints;
      while (*szText) {
         vUniCodePoints.push_back((UINT32)*szText);
         ++szText;
      }
      return SetText(vUniCodePoints);
   }
   bool SetTextA(PCSTR szText) {
      vector<UINT32> vUniCodePoints;
      while (*szText) {
         vUniCodePoints.push_back((UINT32)*szText);
         ++szText;
      }
      return SetText(vUniCodePoints);
   }
   bool SetText(const vector<UINT32>& vUniCodePoints);
   bool SetGlyphIndexes(const vector<UINT16>& vGIndexes);
//ATTS
   const CGlyphRun& GlyphRun() const { return m_GlyphRun; }
   void SetAtom(EnumTexAtom eAtom) { m_eAtom = eAtom; } //sometimes has to be set by the creator
//CMathItem Implementation
   void Draw(D2D1_POINT_2F ptAnchor, const SDWRenderInfo& dwri) override;
private:
   void OnInit_();
};