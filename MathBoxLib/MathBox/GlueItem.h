#pragma once
#include "MathItem.h"
#define GLUE_MAX 10000000.0f //arbitrary large value for infinite resize

//TeX \hskip glue wrapper: spacing+penalty functionality
class CGlueItem : public CMathItem {
//TYPES
   enum MuSkipType { emstNoSpace = 0, emstThinMuSkip = 1, emstMedMuSkip = 2, emstThickMuSkip = 3 };
//DATA
   STexGlue m_Glue;            //TeX glue info
public:
//CTOR/DTOR
   CGlueItem(IDocParams& doc, const STexGlue& glue, const CMathStyle& style, float fUserScale = 1.0f) :
      CMathItem(doc, eacGLUE, style, fUserScale), m_Glue(glue) {
      float fScale = m_fUserScale * m_Style.StyleScale();
      m_Box.nHeight = 0; //F2NEAREST(otfUnitsPerEm * fScale); //fontsize in em = otfUnitsPerEm units
      m_Box.nAscent = 0; //F2NEAREST(otfAscent * fScale);
      UpdateBox_();
   }
   //factory for intermediate glue 
   static CGlueItem* _Create(CMathItem* pPrev, CMathItem* pNext, const CMathStyle& style, float fUserScale = 1.0);
//CMathItem implementation
   const STexGlue* GetGlue() const override { return &m_Glue; }
   void ResizeByRatio(uint16_t nOrder, float fRatio) override {
      if (nOrder == 0 && fRatio == 0.0f) //reset to normal
         m_Glue.fActual = m_Glue.fNorm;
      else 
      if ((fRatio >= 0 && nOrder != m_Glue.nStretchOrder) ||
          (fRatio < 0 && nOrder != m_Glue.nShrinkOrder))
         return; //ntd!
      m_Glue.ResizeByRatio(fRatio);
      UpdateBox_();
   }
   void Select(bool bSelect) override {} //ntd
   void Draw(SPointF ptfAnchor, IDocRenderer& docr) override {} //ntd
private:
   static MuSkipType _GetMuSkipType_(const CMathStyle& style, CMathItem* pPrev, CMathItem* pNext);
   void UpdateBox_();
};