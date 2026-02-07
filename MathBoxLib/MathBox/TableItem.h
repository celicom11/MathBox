#pragma once
#include "MathItem.h"

struct SEnvTableRow {
   char                       cHLine{ 0 };      //0, '-' or '='
   vector<CMathItem*>         vCells;           //whole row or a cell may be empty!
};
struct SEnvTable {
   char                       cHLineBottom{ 0 };   //bottom hline: 0, '-' or '='
   int16_t                    nCols{ 0 };       //number of columns
   vector<SEnvTableRow>       vRows;            //each row is either empty OR has same number of cells!
};

/*array / table container
* |l|r| row sample:
 .Col1_width.                     .Col2_width.
|{cell1}{pad}[colsep/2]|[colsep/2]{pad}{cell2}[colsep/2]|
*/
class CTableItem : public CMathItem {
//TYPES
   struct SRowLayout {
      int16_t nAscent{ 0 };
      int16_t nDescent{ 0 };
      int32_t nTop{ 0 };
      int32_t Height() const { return nAscent + nDescent; }
      int32_t Top() const { return nTop; }
      int32_t Bottom() const { return nTop + nAscent + nDescent; }
      int32_t Baseline() const { return nTop + nAscent; }
   };
//DATA
   int32_t                    m_nMinAscent{ 700 };    // 0.7 of the \strut
   int32_t                    m_nMinDescent{ 300 };   // 0.3 of the \strut
   int32_t                    m_nColSep{ 1000 };      // \arraycolsep, >half the width between columns
   int32_t                    m_nRowSep{ 200 };       // \extrarowheight in FDU
   int32_t                    m_nDoubleRuleSep{ 200 }; // \doublerulesep in FDU
   vector<EnumColAlignment>   m_vColAligns;
   vector<char>               m_vVLines;              // |, :, d, 0
   vector<char>               m_vHLines;              // Row separators (from \hline)

   vector<vector<CMathItem*>> m_vvCells;              // table items, nullptrs allowed!
   //calculated sizes
   vector<int32_t>            m_vColWidths;           // Calculated in CalcRowGeometry_
   vector<SRowLayout>         m_vRowLayouts;          // Calculated in CalcRowGeometry_
public:
//CTOR/INIT
   CTableItem() = delete;
   CTableItem(IParserAdapter& parser, const CMathStyle& style) :
      CMathItem(parser.Doc(), eacTABLE, style) {
      m_eAtom = etaORD;
      float fLength;
      if (parser.GetLength("arraystrutheight", fLength))
         m_nMinAscent = PTS2EM(parser.Doc().DefaultFontSizePts(), fLength);
      if (parser.GetLength("arraystrutdepth", fLength))
         m_nMinDescent = PTS2EM(parser.Doc().DefaultFontSizePts(), fLength);
      if(parser.GetLength("arraycolsep", fLength))
         m_nColSep = PTS2EM(parser.Doc().DefaultFontSizePts(),fLength);
      if (parser.GetLength("extrarowheight", fLength))
         m_nRowSep = PTS2EM(parser.Doc().DefaultFontSizePts(), fLength);
      if (parser.GetLength("doublerulesep", fLength))
         m_nDoubleRuleSep = PTS2EM(parser.Doc().DefaultFontSizePts(), fLength);
   }
   ~CTableItem() {
      for (auto& vRow : m_vvCells) {
         for (CMathItem* pItem : vRow) {
            delete pItem;
         }
      }
      m_vvCells.clear();
   }
//ATTS
//METHODS
   bool Init(const vector<EnumColAlignment>& vColAligns, const vector<char>& vVLines, const SEnvTable& table);
//CMathItem Implementation
   void Draw(SPointF ptfAnchor, IDocRenderer& docr) override;
private:
   void CalcRowGeometry_(int nRowIdx);
   void LayoutRowItems_(int nRowIdx);
};