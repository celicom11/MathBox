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
      int32_t nX = m_Box.Left();
      const float fYBottom = ptfAnchor.fY + EM2DIPS(fFontSizePts, m_Box.Bottom());
      for (int nIdx = 0; nIdx < (int)m_vVLines.size(); ++nIdx) {
         if (m_vVLines[nIdx]) {
            float fX = ptfAnchor.fX + EM2DIPS(fFontSizePts, nX);
            docr.DrawLine({ fX, ptfMyAnchor.fY }, { fX, fYBottom }, 
               (m_vVLines[nIdx] == ':' ? elsDash : elsSolid));
            if (m_vVLines[nIdx] == 'd') {
               nX += m_nDoubleRuleSep; //double line extra space
               fX = ptfAnchor.fX + EM2DIPS(fFontSizePts, nX);
               docr.DrawLine({ fX, ptfMyAnchor.fY }, { fX, fYBottom }, elsSolid);
            }
         }
         if(nIdx < (int)m_vColWidths.size()) //go to start of the next column
            nX += m_vColWidths[nIdx] + m_nColSep;
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
   _ASSERT_RET(vVLines.empty() || vVLines.size() == vColAligns.size()+1, false);
   m_vColAligns = vColAligns;
   m_vVLines = vVLines;
   bool bArrayEnv = !m_vVLines.empty();
   if (m_Style.Style() >= etsScript){
      float fScale = bArrayEnv? m_Style.StyleScale(): 0.45f; //test aggressive scaling
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
   m_vColWidths.resize(table.nCols, 0);
   m_vRowLayouts.resize(table.vRows.size());
   for (int nIdx = 0; nIdx < (int)table.vRows.size(); ++nIdx) {
      const SEnvTableRow& row = table.vRows[nIdx];
      _ASSERT_RET(row.vCells.size() <= (size_t)table.nCols, false); //snbh!
      m_vHLines[nIdx] = row.cHLine;
      m_vvCells.emplace_back(row.vCells);
      while (m_vvCells.back().size() < table.nCols) {
         m_vvCells.back().emplace_back(nullptr);
      }
      CalcRowGeometry_(nIdx);
   }
   //update table box
   m_Box.nHeight = m_vRowLayouts.back().Bottom();
   m_Box.SetMathAxis(m_Box.nHeight/2);
   for (int nCol = 0; nCol < (int)m_vColWidths.size(); ++nCol) {
      m_Box.nAdvWidth += m_vColWidths[nCol] + m_nColSep;
      if (nCol < (int)m_vVLines.size() && m_vVLines[nCol] == 'd')
         m_Box.nAdvWidth += m_nDoubleRuleSep;
   }
   if(!bArrayEnv)
      m_Box.nAdvWidth -= m_nColSep; //no half-sep in first/last columns
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
   bool bArrayEnv = !m_vVLines.empty();
   int32_t nCellX = bArrayEnv ? m_nColSep / 2 : 0;
   const vector<CMathItem*>& vRow = m_vvCells[nRowIdx];
   for (int nCol = 0; nCol < (int)m_vColWidths.size(); ++nCol) {
      CMathItem* pCell = vRow[nCol];
      if (pCell) {
         //horizontal alignment
         int nX = nCellX;
         int32_t nExcess = m_vColWidths[nCol] - pCell->Box().Width();
         _ASSERT(nExcess >= 0);
         if (nExcess > 1 && m_vColAligns[nCol]) {
            //align within the column
            if(m_vColAligns[nCol] == ecaCenter)
               nX += nExcess / 2;
            else
               nX = nCellX + nExcess;
         }
         //vertical alignment: cell baseline aligns with row baseline
         int nY = m_vRowLayouts[nRowIdx].Baseline() - pCell->Box().nAscent;
         pCell->MoveTo(nX, nY);
      }
      if (nCol + 1 < (int)m_vColWidths.size()) {
         nCellX += m_vColWidths[nCol] + m_nColSep;
         if (bArrayEnv && m_vVLines[nCol] == 'd')
            nCellX += m_nDoubleRuleSep; //double line extra space
      }
   }
}

