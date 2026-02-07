#include "stdafx.h"
#include "OverlayItem.h"


void COverlayItem::UpdateLayout() {
   //translate content to LT corner ->(0,0)
   const int32_t nDX = - m_Box.Left();
   const int32_t nDY = - m_Box.Top();
   m_pContent->MoveBy(nDX, nDY);
   /* not needed as overlays are defined relative to container box!
   for (SPolyLine& line : m_vOverlays) {
      for (SPoint& pt : line.vPoints) {
         pt.x += nDX;
         pt.y += nDY;
      }
   }*/
   m_Box.MoveTo(0, 0);
   //respect original ink-box bearings 
   m_Box.nLBearing = m_pContent->Box().Left() + m_pContent->Box().nLBearing;
   m_Box.nRBearing = m_pContent->Box().Right() - m_pContent->Box().Right() + m_pContent->Box().nRBearing;
   //keep baseline/ascent
   m_Box.nAscent = m_pContent->Box().BaselineY();
}
void COverlayItem::Draw(SPointF ptfAnchor, IDocRenderer& docr) {
   _ASSERT_RET(m_pContent, ); //snbh!
   float fFontSizePts = m_Doc.DefaultFontSizePts();
   SPointF ptfLT{
      ptfAnchor.fX + EM2DIPS(fFontSizePts, m_Box.Left()),
      ptfAnchor.fY + EM2DIPS(fFontSizePts, m_Box.Top())
   };
   if (m_argbFill || m_argbFrame) {
      SRectF rcf{ ptfLT.fX, ptfLT.fY,
         ptfLT.fX + EM2DIPS(fFontSizePts, m_Box.Width()),
         ptfLT.fY + EM2DIPS(fFontSizePts, m_Box.Height()) };
      if (m_argbFill)
         docr.FillRect(rcf, m_argbFill);
      if (m_argbFrame)
         docr.DrawRect(rcf, elsSolid, m_fBoxRule, m_argbFrame);
   }
   m_pContent->Draw(ptfLT, docr);
   for (const SPolyLine& line : m_vOverlays) {
      DrawPolyLine_(ptfLT, line, docr);
   }
}

void COverlayItem::DrawPolyLine_(SPointF ptfLT, const SPolyLine& line, IDocRenderer& docr) {
   float fFontSizePts = m_Doc.DefaultFontSizePts();
   // Draw lines connecting consecutive points
   for (int nIdx = 1; nIdx < line.vPoints.size(); ++nIdx) {
      SPointF ptF1 {
         ptfLT.fX + EM2DIPS(fFontSizePts, line.vPoints[nIdx - 1].x),
         ptfLT.fY + EM2DIPS(fFontSizePts, line.vPoints[nIdx - 1].y)
      };
      SPointF ptF2 {
         ptfLT.fX + EM2DIPS(fFontSizePts, line.vPoints[nIdx].x),
         ptfLT.fY + EM2DIPS(fFontSizePts, line.vPoints[nIdx].y)
      };
      docr.DrawLine(ptF1, ptF2,elsSolid, line.fWidth, 0);     // argb (0 = use default color)
   }

   // Close the polyline if needed
   if (line.bClosed && line.vPoints.size() > 2) {
      SPointF ptF1 {
         ptfLT.fX + EM2DIPS(fFontSizePts, line.vPoints.back().x),
         ptfLT.fY + EM2DIPS(fFontSizePts, line.vPoints.back().y)
      };
      SPointF ptF2{
         ptfLT.fX + EM2DIPS(fFontSizePts, line.vPoints.front().x),
         ptfLT.fY + EM2DIPS(fFontSizePts, line.vPoints.front().y)
      };

      docr.DrawLine(ptF1, ptF2, elsSolid, line.fWidth, 0);     // argb (0 = use default color)
   }
}
