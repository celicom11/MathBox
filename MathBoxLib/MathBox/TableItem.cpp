#include "stdafx.h"
#include "TableItem.h"

void CTableItem::Draw(SPointF ptfAnchor, IDocRenderer& docr) {
   float fFontSizePts = m_Doc.DefaultFontSizePts();
   SPointF ptfMyAnchor{
      ptfAnchor.fX + EM2DIPS(fFontSizePts, m_Box.Left()),
      ptfAnchor.fY + EM2DIPS(fFontSizePts, m_Box.Top())
   };
   //
   for (vector<CMathItem*>& vRow: m_vvCells) {
      for (CMathItem* pCell : vRow) {
         if(pCell)
            pCell->Draw(ptfMyAnchor, docr);
      }
   }
   //draw vert lines in array environment
   if (!m_vVLines.empty()) {
      const float fYBottom = ptfAnchor.fY + EM2DIPS(fFontSizePts, m_Box.Bottom());
      for (int nIdx = 0; nIdx < (int)m_vVLines.size(); ++nIdx) {
         if (m_vVLines[nIdx]) {
            float fX = ptfMyAnchor.fX + EM2DIPS(fFontSizePts, m_vRuleX[nIdx]);
            docr.DrawLine({ fX, ptfMyAnchor.fY }, { fX, fYBottom }, 
               (m_vVLines[nIdx] == ':' ? elsDash : elsSolid));
            if (m_vVLines[nIdx] == 'd') {
               fX += EM2DIPS(fFontSizePts, m_nDoubleRuleSep);
               docr.DrawLine({ fX, ptfMyAnchor.fY }, { fX, fYBottom }, elsSolid);
            }
         }
      }
   }
   //draw horz lines
   const float fXRight = ptfAnchor.fX + EM2DIPS(fFontSizePts, m_Box.Right());
   for (int nIdx = 0; nIdx < (int)m_vHLines.size(); ++nIdx) {
      if (m_vHLines[nIdx]) {
         int32_t nY = m_vRowLayouts.back().Bottom(); //default
         if (nIdx < (int)m_vRowLayouts.size())
            nY = nIdx?m_vRowLayouts[nIdx].nTop : 0;
         float fY = ptfMyAnchor.fY + EM2DIPS(fFontSizePts, nY);
         docr.DrawLine({ ptfMyAnchor.fX, fY }, { fXRight, fY }, elsSolid);
      }
   }
   //DrawFrame(ptAnchor, dwri);
}

bool CTableItem::Init(const vector<EnumColAlignment>& vColAligns, const vector<char>& vVLines, const SEnvTable& table) {
   if (!m_vvCells.empty() || table.nCols <= 0 || table.vRows.empty() || vColAligns.empty() ||
       vColAligns.size() > (size_t)table.nCols ||
       (!vVLines.empty() && vVLines.size() != vColAligns.size() + 1) ||
       (!table.vColSpacing.empty() && table.vColSpacing.size() != (size_t)table.nCols + 1))
      return false;
   for (const SEnvTableRow& row : table.vRows) {
      if (row.vCells.size() > (size_t)table.nCols)
         return false;
   }
   m_vColAligns = vColAligns;
   m_vVLines = vVLines;
   bool bArrayEnv = !m_vVLines.empty();
   float fSpacingScale = 1.0f;
   if (m_Style.Style() >= etsScript){
      float fScale = bArrayEnv? m_Style.StyleScale(): 0.45f; //test aggressive scaling
      fSpacingScale = fScale;
      m_nMinAscent = F2NEAREST(m_nMinAscent * fScale);
      m_nMinDescent = F2NEAREST(m_nMinDescent * fScale);
      m_nColSep = F2NEAREST(m_nColSep * fScale);
      m_nRowSep = 40;//test // F2NEAREST(m_nRowSep * fScale);
   }

   //adjust columns in envh
   while (m_vColAligns.size() < (size_t)table.nCols) {
      m_vColAligns.push_back(ecaCenter);
      if(bArrayEnv)
         m_vVLines.push_back(0); //no vline by default
   }

   m_vHLines.resize(table.vRows.size(), 0);
   m_vHLines.reserve(table.vRows.size() + 1);
   m_vColWidths.resize(table.nCols, 0);
   m_vRowLayouts.resize(table.vRows.size());
   m_vColStarts.resize(table.nCols);
   m_vRuleX.resize((size_t)table.nCols + 1);
   m_vvCells.resize(table.vRows.size());
   for (auto& vRow : m_vvCells)
      vRow.resize(table.nCols, nullptr);
   // All allocations precede taking ownership of the caller's cells.
   for (int nIdx = 0; nIdx < (int)table.vRows.size(); ++nIdx) {
      const SEnvTableRow& row = table.vRows[nIdx];
      _ASSERT_RET(row.vCells.size() <= (size_t)table.nCols, false); //snbh!
      m_vHLines[nIdx] = row.cHLine;
      std::copy(row.vCells.begin(), row.vCells.end(), m_vvCells[nIdx].begin());
      CalcRowGeometry_(nIdx);
   }
   //update table box
   m_Box.nHeight = m_vRowLayouts.back().Bottom();
   m_Box.SetMathAxis(m_Box.nHeight/2);
   // Resolve all boundary spacing once for width, cells and rule drawing.
   int32_t nX = 0;
   for (int nBoundary = 0; nBoundary <= table.nCols; ++nBoundary) {
      int32_t nPadding = m_nColSep;
      if (nBoundary == 0)
         nPadding = bArrayEnv ? m_nColSep / 2 : 0;
      else if (nBoundary == table.nCols)
         nPadding = bArrayEnv ? m_nColSep - m_nColSep / 2 : 0;
      if (!table.vColSpacing.empty() && table.vColSpacing[nBoundary].bOverride)
         nPadding = F2NEAREST(table.vColSpacing[nBoundary].nWidthFDU * fSpacingScale);
      int32_t nRuleOffset = nPadding / 2;
      if (nBoundary == 0)
         nRuleOffset = 0;
      else if (nBoundary == table.nCols)
         nRuleOffset = nPadding;
      m_vRuleX[nBoundary] = nX + nRuleOffset;
      nX += nPadding;
      if (nBoundary < (int)m_vVLines.size() && m_vVLines[nBoundary] == 'd')
         nX += m_nDoubleRuleSep;
      if (nBoundary < table.nCols) {
         m_vColStarts[nBoundary] = nX;
         nX += m_vColWidths[nBoundary];
      }
   }
   m_Box.nAdvWidth = nX;
   //PASS2 - needed as column widths known only now
   for (int nIdx = 0; nIdx < (int)table.vRows.size(); ++nIdx) {
      LayoutRowItems_(nIdx);
   }
   if(table.cHLineBottom)
      m_vHLines.push_back(table.cHLineBottom);

   return true;
}
//updates m_vColWidths and m_vRowLayouts
void CTableItem::CalcRowGeometry_(int nRowIdx) {
   //insert 3*otfFractionRuleThickness vertical gap/margin for top row
   m_vRowLayouts[nRowIdx].nTop = nRowIdx ? m_vRowLayouts[nRowIdx - 1].Bottom(): 3*otfFractionRuleThickness;
   //enforce strut
   int32_t nMaxAscent = m_nMinAscent, nMaxDescent = m_nMinDescent;
   const vector<CMathItem*>& vRow = m_vvCells[nRowIdx];
   for (int nCol = 0; nCol < (int)m_vColWidths.size(); ++nCol) {
      CMathItem* pCell = vRow[nCol];
      if (!pCell) 
         continue;
      //else
      const STexBox& box = pCell->Box();
      //update col width
      if (box.Width() > m_vColWidths[nCol])
         m_vColWidths[nCol] = box.Width();
      //update row ascent/descent
      if (box.Ascent() > nMaxAscent)
         nMaxAscent = box.Ascent();
      int32_t nDescent = box.Height() - box.Ascent();
      if (nDescent > nMaxDescent)
         nMaxDescent = nDescent;
   }
   m_vRowLayouts[nRowIdx].nAscent = nMaxAscent + m_nRowSep / 2;
   m_vRowLayouts[nRowIdx].nDescent = nMaxDescent + m_nRowSep / 2;
}
// position MathItems in the nRowIdx row relative to table's origin (0,0) and
// according to column alignment,m_nColSep, m_nRowSep and input nRowHeight,nRowAscent
void CTableItem::LayoutRowItems_(int nRowIdx) {
   const vector<CMathItem*>& vRow = m_vvCells[nRowIdx];
   for (int nCol = 0; nCol < (int)m_vColWidths.size(); ++nCol) {
      CMathItem* pCell = vRow[nCol];
      if (pCell) {
         //horizontal alignment
         int nX = m_vColStarts[nCol];
         int32_t nExcess = m_vColWidths[nCol] - pCell->Box().Width();
         _ASSERT(nExcess >= 0);
         if (nExcess > 1 && m_vColAligns[nCol]) {
            //align within the column
            if(m_vColAligns[nCol] == ecaCenter)
               nX += nExcess / 2;
            else
               nX += nExcess;
         }
         //vertical alignment: cell baseline aligns with row baseline
         int nY = m_vRowLayouts[nRowIdx].Baseline() - pCell->Box().nAscent;
         pCell->MoveTo(nX, nY);
      }
   }
}

