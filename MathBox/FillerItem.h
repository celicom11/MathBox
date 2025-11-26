#pragma once
#include "MathItem.h"

class CFillerItem : public CMathItem {
public:
   //CTOR/DTOR
   CFillerItem(int32_t nWidth, int32_t nHeight) : CMathItem(eacFILLER, CMathStyle())
   {
      m_Box.nAdvWidth = nWidth; m_Box.nHeight = nHeight;
      m_Box.nAscent = nHeight / 2;//default baseline at center
   }
   //CMathItem implementation
   void Draw(D2D1_POINT_2F ptAnchor, const SDWRenderInfo& dwri) override {
      D2D1_POINT_2F ptLT{
         ptAnchor.x + EM2DIPS(dwri.fFontSizePts, m_Box.Left()),
         ptAnchor.y + EM2DIPS(dwri.fFontSizePts, m_Box.Top())
      };
      D2D1_RECT_F drcRect{ ptLT.x,ptLT.y,
                      ptLT.x + EM2DIPS(dwri.fFontSizePts, m_Box.Width()),
                      ptLT.y + EM2DIPS(dwri.fFontSizePts, m_Box.Height()) };
      dwri.pRT->FillRectangle(drcRect, dwri.pBrush);
   }
};
