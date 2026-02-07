#include "stdafx.h"
#include "HBoxItem.h"

bool CHBoxItem::AddItem(CMathItem* pItem) {
   _ASSERT_RET(pItem, false);
   const STexGlue* pItemGlue = pItem->GetGlue();
   if (pItemGlue) { 
      pItem->ResizeByRatio(0, 0.0f); //reset to normal
      //update virtual glue
      if (pItemGlue->nStretchOrder == m_Glue.nStretchOrder)
         m_Glue.fStretchCapacity += pItemGlue->fStretchCapacity;
      else if (pItemGlue->nStretchOrder > m_Glue.nStretchOrder) {
         m_Glue.nStretchOrder = pItemGlue->nStretchOrder;
         m_Glue.fStretchCapacity = pItemGlue->fStretchCapacity;
      }//else ignore smaller order
      if (pItemGlue->nShrinkOrder == m_Glue.nShrinkOrder)
         m_Glue.fShrinkCapacity += pItemGlue->fShrinkCapacity;
      else if (pItemGlue->nShrinkOrder > m_Glue.nShrinkOrder) {
         m_Glue.nShrinkOrder = pItemGlue->nShrinkOrder;
         m_Glue.fShrinkCapacity = pItemGlue->fShrinkCapacity;
      }//else ignore smaller order
   }
   if (!m_vItems.empty()) {
      //add inter-item glue
      m_Glue.fNorm += pItemGlue ? pItemGlue->fNorm : pItem->Box().Width();
      CMathItem* pPrev = m_vItems.back();
      CGlueItem* pGlue = pItem->Type() == eacGLUE ? nullptr: CGlueItem::_Create(pPrev, pItem, m_Style, m_fUserScale);
      const STexGlue* pInterGlue = pGlue ? pGlue->GetGlue() : nullptr;
      if (pInterGlue) {
         //update virtual glue
         _ASSERT(pInterGlue->nShrinkOrder == 0 && pInterGlue->nStretchOrder == 0);
         m_Glue.fNorm += pInterGlue->fNorm;
         if (m_Glue.nStretchOrder == 0)
            m_Glue.fStretchCapacity += pInterGlue->fStretchCapacity;
         if (m_Glue.nShrinkOrder == 0)
            m_Glue.fShrinkCapacity += pInterGlue->fShrinkCapacity;
      }
      m_vGlues.push_back(pGlue); //nullptr is ok
   }
   else {//first item: 
      pItem->DenominateBinRel();
      m_Glue.fNorm = pItemGlue ? pItemGlue->fNorm : pItem->Box().Width();
      //initialize
      m_Box = pItem->Box();
   }
   m_vItems.push_back(pItem);
   m_Glue.fActual = m_Glue.fNorm; //initially
   //check if new item fits in
   return IsAuto() || m_Glue.nShrinkOrder || m_Glue.fNorm - m_Glue.fShrinkCapacity <= m_fFixedWidth;
}
void CHBoxItem::Update() {
   if (IsAuto() || m_fFixedWidth == m_Glue.fNorm)
      UpdateLayout_();//we are good already
   else if (m_Glue.fNorm > m_fFixedWidth && m_Glue.fShrinkCapacity) //shrink
      ResizeByRatio(m_Glue.nShrinkOrder, (m_fFixedWidth - m_Glue.fNorm) / m_Glue.fShrinkCapacity);
   else if (m_Glue.fNorm < m_fFixedWidth && m_Glue.fStretchCapacity) //stretch
      ResizeByRatio(m_Glue.nStretchOrder, (m_fFixedWidth - m_Glue.fNorm) / m_Glue.fStretchCapacity);
}
// align items both vertically and horizontally without any re-sizing!
void CHBoxItem::UpdateLayout_() {
   //1. adust items vertically by the baseline of the SBox AND update box top/bottom
   const int32_t nBaseline = m_Box.BaselineY();
   for (CMathItem* pItem : m_vItems) {
      int32_t nItemBaseline = pItem->Box().BaselineY() - pItem->GetShiftY();
      int32_t nDY = nBaseline - nItemBaseline;
      pItem->MoveBy(0, nDY);
      m_Box.Union(pItem->Box());     //only top/bottom are valid now!
   }
   //2. position items horizontally
   for (int32_t nIdx = 1; nIdx < (int32_t)m_vItems.size(); ++nIdx) {
      CMathItem* pItem = m_vItems[nIdx];
      CMathItem* pPrevItem = m_vItems[nIdx - 1];
      float fX = (float)pPrevItem->Box().Right();
      CGlueItem* pGlue = m_vGlues[nIdx - 1];
      if (pGlue) 
         fX += pGlue->GetGlue()->fActual;//may be < 0!
      pItem->MoveTo(F2NEAREST(m_Box.Left() + fX), pItem->Box().Top());
   }
   m_Box.nAdvWidth = m_vItems.back()->Box().Right() - m_Box.Left();
   m_Box.nRBearing = m_vItems.back()->Box().RBearing();
}
//adjust resizable glues to fit box with min Badness and UpdateLayout
void CHBoxItem::ResizeByRatio(uint16_t nOrder, float fRatio) {
   if ( (fRatio >= 0 && (nOrder != m_Glue.nStretchOrder || m_Glue.fStretchCapacity <= 0)) ||
        (fRatio < 0 && (nOrder != m_Glue.nShrinkOrder  || m_Glue.fShrinkCapacity <= 0))
      )
      return; //n/a
   //else
   float fWidth = 0; //calc actual width
   for (int32_t nIdx = 1; nIdx < (int32_t)m_vItems.size(); ++nIdx) {
      CMathItem* pItem = m_vItems[nIdx];
      CMathItem* pPrevItem = m_vItems[nIdx - 1];
      pPrevItem->ResizeByRatio(nOrder, fRatio);
      fWidth += pPrevItem->GetGlue()? pPrevItem->GetGlue()->fActual : pPrevItem->Box().Width();
      CGlueItem* pGlue = m_vGlues[nIdx - 1];
      if (pGlue) {
         pGlue->ResizeByRatio(nOrder, fRatio);
         fWidth += pGlue->GetGlue()->fActual;//may be <0!
      }
   }
   m_vItems.back()->ResizeByRatio(nOrder, fRatio);
   //4. update width
   if (IsAuto()) {
      m_Glue.fActual = fWidth + m_vItems.back()->Box().Width();
      m_Box.nAdvWidth = F2NEAREST(m_Glue.fActual); //why fWidth?
   }
   else {
      //fixed width? -> reset m_Glue 
      m_Glue.nStretchOrder = m_Glue.nShrinkOrder = 0;
      m_Glue.fStretchCapacity = m_Glue.fShrinkCapacity = 0.0f;
      m_Glue.fActual = m_Glue.fNorm = m_fFixedWidth;
      m_Box.nAdvWidth = F2NEAREST(m_fFixedWidth);
   }
   UpdateLayout_();
}
float CHBoxItem::CalcBadness() const {
   return 0;//TODO!
}

