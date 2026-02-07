// OverlayItem.h
#pragma once
#include "MathItem.h"

class COverlayItem : public CMathItem {

public:
   struct SPolyLine {
      bool              bClosed{ false };    // true = connect last to first
      uint32_t          argb{ 0 };           // use default document text color if 0
      float             fWidth{ 0 };         // ABS line width, DIPs!
      vector<SPoint>    vPoints;             // Points relative to content box, FDU!
   };

private:
   float             m_fBoxRule{ 0 };     // frame width , DIPs
   uint32_t          m_argbFrame{ 0 };    // frame color, ignored if 0
   uint32_t          m_argbFill{ 0 };     // box fill color, ignored if 0
   CMathItem*        m_pContent{nullptr}; // we OWN this!
   vector<SPolyLine> m_vOverlays;         // Lines to draw

public:
//CTOR
   COverlayItem(CMathItem* pContent) : 
         CMathItem(pContent->Doc(), eacOVERLAY, pContent->GetStyle()),
         m_pContent(pContent) {
      _ASSERT(pContent);
      m_Box = pContent->Box();  // Start with content's box
      m_eIndexPlacement = pContent->IndexPlacement();
      m_eAtom = pContent->AtomType();
   }

   ~COverlayItem() {
      delete m_pContent;
   }

   // Add overlay lines (called by builder)
   void AddOverlay(const SPolyLine& line) {
      _ASSERT_RET(!line.vPoints.empty(), );
      m_vOverlays.push_back(line);
   }
   void SetBox(SRect rcBox, float fBoxRule = 0.0f, uint32_t argbFrame=0, uint32_t argbFill=0) {
      m_Box.nX = rcBox.left;
      m_Box.nY = rcBox.top;
      m_Box.nAdvWidth = rcBox.right - rcBox.left;
      m_Box.nHeight = rcBox.bottom - rcBox.top;
      m_fBoxRule = fBoxRule;
      m_argbFrame = argbFrame;
      m_argbFill = argbFill;
   }
   //caller must call after SetBox+all overlays added!
   void UpdateLayout();
   // CMathItem implementation
   void Draw(SPointF ptfAnchor, IDocRenderer& docr) override;
private:
   void DrawPolyLine_(SPointF ptfLT, const SPolyLine& line, IDocRenderer& docr);
};
