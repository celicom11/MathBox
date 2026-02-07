#pragma once
#include "MathItem.h"

class CRuleItem : public CMathItem {
public:
   //CTOR/DTOR
   CRuleItem(IDocParams& doc, int32_t nWidthFDU, int32_t nHeightFDU, int32_t nAscentFDU,
             const CMathStyle& style, float fUserScale = 1.0f) : CMathItem(doc, eacRULE, style, fUserScale)
   {
      float fScale = EffectiveScale();
      m_Box.nAdvWidth = F2NEAREST(nWidthFDU * fScale);
      m_Box.nHeight = F2NEAREST(nHeightFDU * fScale);
      m_Box.nAscent = F2NEAREST(nAscentFDU * fScale);
   }
   //already scaled = default filler!
   CRuleItem(IDocParams& doc, int32_t nWidthFDU, int32_t nHeightFDU):
      CMathItem(doc, eacRULE, CMathStyle()) {
      m_Box.nAdvWidth = nWidthFDU; m_Box.nHeight = nHeightFDU;
      m_Box.nAscent = nHeightFDU / 2;//default baseline at center
   }
   //CMathItem implementation
   void Draw(SPointF ptfAnchor, IDocRenderer& docr) override {
      if (!m_Box.Width() || !m_Box.Height())
         return; //ntd
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
