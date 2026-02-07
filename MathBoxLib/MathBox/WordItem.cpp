#include "stdafx.h"
#include "WordItem.h"


CWordItem::CWordItem(IDocParams& doc, int16_t nFontIdx, const CMathStyle& style, EnumMathItemType eType, float fUserScale) :
   CMathItem(doc, eType, style, fUserScale), m_nFontIdx(nFontIdx)
{
   m_eType = eType;
   m_fUserScale = fUserScale;
}
bool CWordItem::SetText(const vector<uint32_t>& vUniCodePoints) {
   m_vGUnicode = vUniCodePoints;
   vector<uint16_t> vGIndexes(vUniCodePoints.size());
   if (!m_Doc.FontManager().GetFontIndices(m_nFontIdx, (int32_t)vUniCodePoints.size(), vUniCodePoints.data(),
                           vGIndexes.data()))
      return false;
   return SetGlyphIndexes(vGIndexes);
}
bool CWordItem::SetGlyphIndexes(const vector<uint16_t>& vGIndexes) {
   m_vGIndexes = vGIndexes;
   m_vGMetrics.resize(vGIndexes.size());
   if (!m_Doc.FontManager().GetGlyphRunMetrics(m_nFontIdx, (int32_t)vGIndexes.size(), vGIndexes.data(),
                                               m_vGMetrics.data(), m_Bounds))
      return false;
   //set Box
   float fScale = EffectiveScale();
   m_Box.nHeight = F2NEAREST((m_Bounds.nMaxY - m_Bounds.nMinY) * fScale);
   m_Box.nAscent = -F2NEAREST(m_Bounds.nMinY * fScale);

   uint32_t nAdvWidth = 0;
   for (const SGlyphMetrics& gmetric : m_vGMetrics)
      nAdvWidth += gmetric.nAdvanceWidth;
   m_Box.nAdvWidth = F2NEAREST(nAdvWidth * fScale);
   m_Box.nLBearing = F2NEAREST(m_vGMetrics.front().nLSB * fScale);
   m_Box.nRBearing = F2NEAREST(m_vGMetrics.back().nRSB * fScale);

   return true;
}
//CMathItem Implementation
void CWordItem::Draw(SPointF ptfAnchor, IDocRenderer& docr) {
   //draw at the box position, baseline aligned
   float fFontSizePts = m_Doc.DefaultFontSizePts();
   SPointF ptfBaseOrigin = {
      ptfAnchor.fX + EM2DIPS(fFontSizePts, m_Box.Left()),
      ptfAnchor.fY + EM2DIPS(fFontSizePts, m_Box.BaselineY())
   };
   docr.DrawGlyphRun(m_nFontIdx, (int32_t)m_vGIndexes.size(), m_vGIndexes.data(),
                     ptfBaseOrigin, EffectiveScale());
}
//static helper
CWordItem* CWordItem::_MergeWords(CWordItem* pWord1, CWordItem* pWord2) {
   if (!pWord1 || pWord1->AtomType() != etaORD || !pWord2 || pWord2->AtomType() != etaORD)
      return nullptr;
   if (pWord1->GetFontIdx() != pWord2->GetFontIdx() ||
      pWord1->UserScale() != pWord2->UserScale() ||
      pWord1->GetStyle().Style() != pWord2->GetStyle().Style())
      return nullptr;
   //good to merge
   CWordItem* pMerged = new CWordItem(pWord1->Doc(), pWord1->GetFontIdx(),
                                       pWord1->GetStyle(), eacWORD, pWord1->UserScale());
   vector<uint16_t> vGlyphIdx(pWord1->GlyphCount() + pWord2->GlyphCount());
   for (uint32_t nIdx = 0; nIdx < pWord1->GlyphCount(); ++nIdx) {
      vGlyphIdx[nIdx] = pWord1->GetIndexAt(nIdx);
   }
   for (uint32_t nIdx = 0; nIdx < pWord2->GlyphCount(); ++nIdx) {
      vGlyphIdx[pWord1->GlyphCount() + nIdx] = pWord2->GetIndexAt(nIdx);
   }
   pMerged->SetGlyphIndexes(vGlyphIdx);
   return pMerged;
}

