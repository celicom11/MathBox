#pragma once
#include "MathItem.h"

class CMockMathItem : public CMathItem {
   public:
   CMockMathItem() = delete;
   CMockMathItem(EnumTexAtom eAtomType, const CMathStyle& style) :
      CMathItem(eacUNK, style) {
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
   void Draw(D2D1_POINT_2F ptAnchor, const SDWRenderInfo& dwri) override {}
};
