#pragma once
#include "MathItem.h"

class CMockDocParams : public IDocParams {
public:
   IFontManager& FontManager() override {
      IFontManager* pNull = nullptr;
      return *pNull;
   }
   ILMFManager& LMFManager() override {
      ILMFManager* pNull = nullptr;
      return *pNull;
   }
   float DefaultFontSizePts() override {
      return 12.0f;
   }
   int32_t MaxWidthFDU() override {
      return 0;
   }
   uint32_t ColorBkg() override { return 0; }
   uint32_t ColorSelection() override { return 0; }
   //Text color ~= Foreground color!
   uint32_t ColorText() override { return 0; }
};
class CMockMathItem : public CMathItem {
   public:
   CMockMathItem() = delete;
   CMockMathItem(IDocParams& doc, EnumTexAtom eAtomType, const CMathStyle& style) :
      CMathItem(doc, eacUNK, style) {
      //deduce fake MathItem type
      m_eAtom = eAtomType;
      switch (m_eAtom) {
         case etaOP:
            m_eType = eacMATHOP;//or eacBIGOP
            break;
         case etaINNER:
            m_eType = eacVBOX;
            break;
         default: m_eType = eacWORD;
      }
   }
   //ATTS
   STexBox& Box() { return m_Box; } //r/w access to TBox for testing
   //METHODS
   void Draw(SPointF ptfAnchor, IDocRenderer& docr) override {}
};
