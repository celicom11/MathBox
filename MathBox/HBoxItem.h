#pragma once
#include "ContainerItem.h"
#include "GlueItem.h"

class CHBoxItem : public CContainerItem {
   //DATA
   const float        m_fFixedWidth{ 0 }; //fixed width, \hbox to XXX
   STexGlue           m_Glue;             //virtual glue!
   vector<CGlueItem*> m_vGlues;           //inter-item glues, may contain nullptrs
public:
   //CTOR/DTOR
   CHBoxItem() = delete;
   CHBoxItem(IDocParams& doc, const CMathStyle& style, float fWidth = 0.0f ) :
      CContainerItem(doc, eacHBOX, style), m_fFixedWidth(fWidth) {
      m_Glue.fActual = m_Glue.fNorm = fWidth;
      m_Box.nAdvWidth = F2NEAREST(m_fFixedWidth);
   }
   ~CHBoxItem() {
      Clear();
   }
   //ATTS
   float FixedWidth() const { return m_fFixedWidth; }
   bool IsAuto() const { return m_fFixedWidth <= 0.0f; }
   void ContentInfo(OUT uint32_t& nItems, uint32_t& nActGlues) const { //for debugging/testing only
      nItems = (uint32_t)m_vItems.size();
      nActGlues = 0;
      for (const CGlueItem* pItem : m_vGlues) {
         if (pItem)
            ++nActGlues;
      }
   }
   float CalcBadness() const; //>0 -undefill, 0-perfect, <0-overfill
   //METHODS
   void Clear(bool bDoNotDeleteTtems = false) {
      CContainerItem::Clear(bDoNotDeleteTtems);
      for (CGlueItem* pGlue : m_vGlues)
         delete pGlue;
      m_vGlues.clear();
      m_Glue = STexGlue();
   }
   bool AddItem(CMathItem* pItem);
   void Update(); //MUST be called after last AddItem!
   //CMathItem implementation
   const STexGlue* GetGlue() const override { return &m_Glue; }
   void ResizeByRatio(uint16_t nOrder, float fRatio) override;
private:
   //adjusts items both vertically and horizontally to fit in m_fFixedWidth with min Badness
   void UpdateLayout_();
};