#include "stdafx.h"
#include "GlueItem.h"
#include "HBoxItem.h"

namespace { //static helpers
   // TeX atom spacing matrix in D/T mode; values in muskip units/MuSkipType
   constexpr int16_t ATOM_SPACING_MATRIX[8][8] = {
      //L\R      Ord  Op   Bin  Rel  Open Close Punct Inner
      /*Ord*/  { 0,   1,   2,   3,   0,    0,    0,    1 },   // Ordinary
      /*Op*/   { 1,   1,   0,   3,   0,    0,    0,    1 },   // Math operator  
      /*Bin*/  { 2,   2,   0,   0,   2,    0,    0,    2 },   // Binary operator
      /*Rel*/  { 3,   3,   0,   0,   3,    0,    0,    3 },   // Relation
      /*Open*/ { 0,   0,   0,   0,   0,    0,    0,    0 },   // Opening delimiter
      /*Close*/{ 0,   1,   2,   3,   0,    0,    0,    1 },   // Closing delimiter
      /*Punct*/{ 1,   1,   1,   1,   1,    1,    1,    1 },   // Punctuation
      /*Inner*/{ 1,   1,   2,   3,   1,    0,    1,    1 }    // Inner (fractions, etc.)
   };
   const STexGlue _THINMUSKIP =  { 0,0, 0.0f,     0.0f,     MU2EM(3),MU2EM(3) }; //3mu, kern
   const STexGlue _MEDMUSKIP =   { 0,0, MU2EM(2), MU2EM(4), MU2EM(4),MU2EM(4) }; //4mu plus 2mu minus 4mu
   const STexGlue _THICKMUSKIP = { 0,0, MU2EM(5), 0.0f,     MU2EM(5),MU2EM(5) }; //5mu plus 5mu
}

CGlueItem* CGlueItem::_Create(CMathItem* pPrev, CMathItem* pNext, const CMathStyle& style, float fUserScale) {
   _ASSERT_RET(pPrev != nullptr && pNext != nullptr, nullptr);//snbh!
   MuSkipType eMSType = _GetMuSkipType_(style, pPrev, pNext);
   switch (eMSType) {
   case emstThinMuSkip:
      return new CGlueItem(pPrev->Doc(), _THINMUSKIP, style, fUserScale);
   case emstMedMuSkip:
      return new CGlueItem(pPrev->Doc(), _MEDMUSKIP, style, fUserScale);
   case emstThickMuSkip:
      return new CGlueItem(pPrev->Doc(), _THICKMUSKIP, style, fUserScale);
   }
   return nullptr; //no glue needed!
}
CGlueItem::MuSkipType CGlueItem::_GetMuSkipType_(const CMathStyle& style, CMathItem* pPrev, CMathItem* pNext) {
   if(pPrev->Type() == eacGLUE || pNext->Type() == eacGLUE)
      return emstNoSpace;//no extra space between glues!
   //map prev/next types to matrix indices 0=Ord, 1=Op, 2=Bin, 3=Rel, 4=Open, 5=Close, 6=Punct, 7=Inner
   int nPrevClass = pPrev->AtomType(); 
   //in case of inner formula (HBOX+Inner), get atom type of the last item
   // WHY?
   //if (pPrev->Type() == eacHBOX && nPrevClass == etaINNER)
   //   nPrevClass = ((CHBoxItem*)pPrev)->Items().back()->AtomType();
   int nNextClass = pNext->AtomType();
   //in case of inner formula (HBOX+Inner), get atom type of the first item
   // WHY?
   //if (pNext->Type() == eacHBOX && nNextClass == etaINNER)
   //   nNextClass = ((CHBoxItem*)pNext)->Items().front()->AtomType();

   if (style.Style() <= etsText)
      return (MuSkipType)ATOM_SPACING_MATRIX[nPrevClass][nNextClass];
   //else
   switch (nPrevClass) {
   case etaOP: if (etaORD == nNextClass || etaOP == nNextClass) return emstThinMuSkip;
      break;
   case etaORD: 
   case etaCLOSE: 
   case etaINNER: if (etaOP == nNextClass) return emstThinMuSkip;
      break;
   }
   return emstNoSpace; //no space in other cases in D/T mode
}
//only width changes, height/ascent remain the same
void CGlueItem::UpdateBox_() {
   float fScale = m_fUserScale * m_Style.StyleScale();
   int32_t nWidth = F2NEAREST(m_Glue.fActual * fScale);
   m_Box.nAdvWidth = nWidth; //may be negative!
}