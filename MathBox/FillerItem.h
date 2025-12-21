#pragma once
#include "MathItem.h"

class CFillerItem : public CMathItem {
public:
   //CTOR/DTOR
   CFillerItem(IDocParams& doc, int32_t nWidth, int32_t nHeight) : CMathItem(doc, eacFILLER, CMathStyle())
   {
      m_Box.nAdvWidth = nWidth; m_Box.nHeight = nHeight;
      m_Box.nAscent = nHeight / 2;//default baseline at center
   }
   //CMathItem implementation
   void Draw(SPointF ptfAnchor, IDocRenderer& docr) override {
      float fFontSizePts = m_Doc.DefaultFontSizePts();
      SPointF ptfLT{
         ptfAnchor.fX + EM2DIPS(fFontSizePts, m_Box.Left()),
         ptfAnchor.fY + EM2DIPS(fFontSizePts, m_Box.Top())
      };
      SRectF rcf{ ptfLT.fX, ptfLT.fY,
                  ptfLT.fX + EM2DIPS(fFontSizePts, m_Box.Width()),
                  ptfLT.fY + EM2DIPS(fFontSizePts, m_Box.Height()) };
      docr.FillRect(rcf, 0); //use default brush
   }
};
