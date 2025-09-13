#pragma once
#include "MathItem.h"

//generic item container
class CContainerItem : public CMathItem {
protected:
   vector<CMathItem*>		m_vItems;		//may be empty!
public:
   //CTOR/DTOR
   CContainerItem(EnumMathItemType eType, const CMathStyle& style) :
      CMathItem(eType, style) {
   }
   ~CContainerItem() { Clear(); }
   //ATTS
   const vector<CMathItem*>& Items() const { return m_vItems; }
   bool IsEmpty() const { return m_vItems.empty(); }
   //METHODS
   void Clear() {
      for (CMathItem* pTBox : m_vItems)
         delete pTBox;
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
   void SetAscent(int32_t nAscent) {
      m_Box.nAscent = nAscent;
   }
   void SetAxisHeight(int32_t nAxisHeight) {
      m_Box.nAxisHeight = nAxisHeight;
   }
   //VFUNC implementation
   void Draw(D2D1_POINT_2F ptAnchor, const SDWRenderInfo& dwri) override {
      D2D1_POINT_2F ptMyAnchor{
        ptAnchor.x + EM2DIPS(dwri.fFontSizePts, m_Box.Left()),
        ptAnchor.y + EM2DIPS(dwri.fFontSizePts, m_Box.Top())
      };
      for (CMathItem* pTBox : m_vItems) {
         pTBox->Draw(ptMyAnchor, dwri);
      }
   }
};
