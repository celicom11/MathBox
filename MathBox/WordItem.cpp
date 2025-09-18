#include "stdafx.h"
#include "WordItem.h"

namespace {
   bool _IsIn(UINT32 nCP, const UINT32* pCP0, const UINT32* pCP1) {
      for (const UINT32* pCP = pCP0; pCP < pCP1; ++pCP) {
         if (*pCP == nCP)
            return true;
      }
      return false;
   }
   bool _IsBin(UINT32 nCP) {
      return _IsIn(nCP, _aUniBIN, _aUniBIN + _countof(_aUniBIN));
   }
   bool _IsRel(UINT32 nCP) {
      return _IsIn(nCP, _aUniREL, _aUniREL + _countof(_aUniREL)) ||
         (nCP >= _rngUniREL1[0] && nCP <= _rngUniREL1[1]) ||
         (nCP >= _rngUniREL2[0] && nCP <= _rngUniREL2[1]) ||
         (nCP >= _rngUniREL3[0] && nCP <= _rngUniREL3[1]);
         
   }
   bool _IsDelim(UINT32 nCP) {
      return _IsIn(nCP, _aUniDELIM, _aUniDELIM + _countof(_aUniDELIM));
   }
   bool _IsPunct(UINT32 nCP) {
      return _IsIn(nCP, _aUniPUNCT, _aUniPUNCT + _countof(_aUniPUNCT));
   }
   bool _IsAccent(UINT32 nCP) {
      return _IsIn(nCP, _aUniACCENT, _aUniACCENT + _countof(_aUniACCENT));
   }
   bool _IsBigOp(UINT32 nCP) {
      return _IsIn(nCP, _aUniBigOP, _aUniBigOP + _countof(_aUniBigOP));
   }
   bool _IsArrow(UINT32 nCP) {
      return (nCP >= _rngUniArrows[0] && nCP <= _rngUniArrows[1]);
   }
}
CWordItem::CWordItem(IDWriteFontFace* pFontFace, const CMathStyle& style, EnumMathItemType eType, float fUserScale) :
   CMathItem(eType, style, fUserScale), m_GlyphRun(pFontFace)
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

   //set atom type
   if (m_GlyphRun.Glyphs().size() == 1) {
      UINT32 nCP = m_GlyphRun.Glyphs().front().codepoint;
      if(_IsPunct(nCP))
         m_eAtom = etaPUNCT;
      else if(_IsBin(nCP))
         m_eAtom = etaBIN;
      else if (_IsRel(nCP) || _IsArrow(nCP)) //treat arrows as REL
         m_eAtom = etaREL;
      else if(_IsDelim(nCP))
         m_eAtom = etaOPEN; //assume open, will be fixed in CMathRow
      else if(_IsBigOp(nCP))
         m_eAtom = etaOP;
      else
         m_eAtom = etaORD;
   }
}
