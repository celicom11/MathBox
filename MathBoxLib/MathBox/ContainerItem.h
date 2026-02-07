#pragma once
#include "MathItem.h"

//generic item container
class CContainerItem : public CMathItem {
protected:
   vector<CMathItem*>		m_vItems;		//may be empty!
public:
 //CTOR/DTOR
   CContainerItem(IDocParams& doc, EnumMathItemType eType, const CMathStyle& style, EnumTexAtom eAtom = etaORD) :
      CMathItem(doc, eType, style) {
      m_eAtom = eAtom;
   }
   ~CContainerItem() { Clear(); }
 //ATTS
   const vector<CMathItem*>& Items() const { return m_vItems; }
   bool IsEmpty() const { return m_vItems.empty(); }
   STexBox& Box() { return m_Box; } //needed for builder's adjustments
 //METHODS
   void Clear(bool bDoNotDeleteTtems = false) {
      if (!bDoNotDeleteTtems) {
         for (CMathItem* pTBox : m_vItems)
            delete pTBox;
      }
      m_vItems.clear();
      m_Box = STexBox();
   }
   void AddBox(CMathItem* pTBox, int32_t nX, int32_t nY, bool bAnchor = false) {
      if (pTBox) {
         pTBox->MoveTo(nX, nY);
         m_vItems.push_back(pTBox);
         m_Box.Union(pTBox->Box());
         if (bAnchor)
            m_Box.nAscent = pTBox->Box().BaselineY() - m_Box.Top();
      }
   }
   void NormalizeOrigin(int nXOrig, int nYOrig) {
      if (m_Box.IsEmpty() || nXOrig == m_Box.Left() && nYOrig == m_Box.Top())
         return;//ntd
      const int32_t nDX = nXOrig - m_Box.Left();
      const int32_t nDY = nYOrig - m_Box.Top();
      for (CMathItem* pTBox : m_vItems) {
         pTBox->MoveBy(nDX, nDY);
      }
      m_Box.MoveTo(nXOrig, nYOrig);
   }
   void SetAscent(int32_t nAscent) { m_Box.nAscent = nAscent; }
   void SetMathAxis(int32_t nAxisY) { m_Box.SetMathAxis(nAxisY); }
   void AddWidth(int32_t nSX) { m_Box.nAdvWidth += nSX; } //needed for some item on construction
 //CMathItem implementation
   void Draw(SPointF ptfAnchor, IDocRenderer& docr) override {
      float fFontSizePts = m_Doc.DefaultFontSizePts();
      SPointF ptfMyAnchor{
         ptfAnchor.fX + EM2DIPS(fFontSizePts, m_Box.Left()),
         ptfAnchor.fY + EM2DIPS(fFontSizePts, m_Box.Top())
      };
      for (CMathItem* pTBox : m_vItems) {
         pTBox->Draw(ptfMyAnchor, docr);
      }
   }
};
