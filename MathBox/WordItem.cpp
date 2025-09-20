#include "stdafx.h"
#include "WordItem.h"
#include "LMFontManager.h"

extern CLMFontManager g_LMFManager;

CWordItem::CWordItem(int nFontIdx, const CMathStyle& style, EnumMathItemType eType, float fUserScale) :
   CMathItem(eType, style, fUserScale), m_GlyphRun(nFontIdx)
{
   m_eType = eType;
   m_fUserScale = fUserScale;
}
bool CWordItem::SetText(const vector<UINT32>& vUniCodePoints) {
   HRESULT hr = m_GlyphRun.SetGlyphs(vUniCodePoints);
   if (FAILED(hr))
      return false;
   OnInit_();
   return true;
}
bool CWordItem::SetGlyphIndexes(const vector<UINT16>& vGIndexes) {
   HRESULT hr = m_GlyphRun.SetGlyphIndices(vGIndexes, vector<UINT32>(vGIndexes.size(), 0));
   if (FAILED(hr))
      return false;
   OnInit_();
   return true;
}
//CMathItem Implementation
void CWordItem::Draw(D2D1_POINT_2F ptAnchor, const SDWRenderInfo& dwri) {
   float fScale = m_fUserScale * m_Style.StyleScale();
   //draw at the box position, baseline aligned
   D2D1_POINT_2F ptfBaseOrigin = {
      ptAnchor.x + EM2DIPS(dwri.fFontSizePts, m_Box.Left()),
      ptAnchor.y + EM2DIPS(dwri.fFontSizePts, m_Box.BaselineY())
   };
   m_GlyphRun.Draw(dwri, ptfBaseOrigin, fScale);
}

void CWordItem::OnInit_() {
   float fScale = m_fUserScale * m_Style.StyleScale();
   m_Box.nHeight = F2NEAREST((m_GlyphRun.Bounds().nMaxY - m_GlyphRun.Bounds().nMinY) * fScale);
   m_Box.nAscent = -F2NEAREST(m_GlyphRun.Bounds().nMinY * fScale);
   UINT32 nAdvWidth = 0;
   for (const SDWGlyph& glyph : m_GlyphRun.Glyphs())
      nAdvWidth += glyph.metrics.advanceWidth;
   m_Box.nAdvWidth = F2NEAREST(nAdvWidth * fScale);
   m_Box.nLBearing = F2NEAREST(m_GlyphRun.Glyphs().front().metrics.leftSideBearing * fScale);
   m_Box.nRBearing = F2NEAREST(m_GlyphRun.Glyphs().back().metrics.rightSideBearing * fScale);

   //TODO: move to WordItemBuilder!
   //set atom type
   if (m_GlyphRun.Glyphs().size() == 1 && m_GlyphRun.GetFontIdx() == 0) {
      UINT32 nGlyphIdx = m_GlyphRun.Glyphs().front().index;
      const SLMMGlyph* pLmmGlyph = g_LMFManager.GetLMMGlyphByIdx(0, nGlyphIdx);
      if (pLmmGlyph) {
         switch (pLmmGlyph->eClass) {
         case egcLOP:   m_eAtom = etaOP; break;
         case egcBin:   m_eAtom = etaBIN; break;
         case egcRel:   m_eAtom = etaREL; break;
         case egcOpen:  m_eAtom = etaOPEN; break;
         case egcClose: m_eAtom = etaCLOSE; break;
         case egcPunct: m_eAtom = etaPUNCT; break;
            //case egcAccent:m_eAtom = ; break;
            //case egcOver:  m_eAtom = ; break;
            //case egcUnder: m_eAtom = ; break;
         default:       m_eAtom = etaORD;
         }
      }
   }
}
