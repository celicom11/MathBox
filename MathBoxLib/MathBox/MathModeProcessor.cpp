#include "stdafx.h"
#include "MathModeProcessor.h"
#include "TexParser.h"
#include "RawItem.h"
#include "Tokenizer.h"
#include "ParserAdapter.h"
#include "HBoxItem.h"
#include "ExtGlyphBuilder.h"
#include "EnvHelper.h"
#include "TableItem.h"
// MathItem Builders
#include "WordItemBuilder.h"
#include "VBoxBuilder.h"
#include "ScaleCmdBuilder.h"
#include "AccentBuilder.h"
#include "UnderOverBuilder.h"
#include "OpenCloseBuilder.h"
#include "LOpBuilder.h"
#include "HSpacingBuilder.h"
#include "RadicalBuilder.h"
#include "FractionBuilder.h"
#include "MathFontCmdBuilder.h"
#include "MathSymBuilder.h"
#include "TextCmdBuilder.h"
#include "ShiftBuilder.h"
#include "MathClassBuilder.h"
#include "BoxCmdBuilder.h"
#include "OverlayBuilder.h"
#include "MathStyleBuilder.h"



namespace { //static helpers
   bool _isMathStyleCommand(const string& sCmd) {
      // \displaystyle, \textstyle, etc
      return (sCmd == "\\displaystyle" || sCmd == "\\textstyle" ||
         sCmd == "\\scriptstyle" || sCmd == "\\scriptscriptstyle");
   }
   bool _isFontSizeCommand(const string& sCmd) {
      PCSTR szCmd = sCmd.c_str();
      if (szCmd[0] != '\\')
         return false;
      szCmd++; //skip '\'
      for (const SFontSizeVariant& fsVar : _aFontSizeVariants) {
         if (0 == strcmp(szCmd, fsVar.szFontSizeCmd))
            return true;
      }
      //check for 
      return false;
   }
   bool _isNewlineCommand(const string& sCmd) {
      // \\, \newline, \cr
      return (sCmd == "\\\\" || sCmd == "\\newline" || sCmd == "\\cr");
   }
}
// CMathModeProcessor
CMathModeProcessor::CMathModeProcessor(CTexParser& parser):m_Parser(parser) {
   //MATH MODE COMMAND PROCESSORS/BUILDERS
   RegisterBuilder(new CAccentBuilder);
   RegisterBuilder(new CFractionBuilder);
   RegisterBuilder(new CHSpacingBuilder);
   RegisterBuilder(new CLOpBuilder);
   RegisterBuilder(new CMathFontCmdBuilder);
   RegisterBuilder(new CRadicalBuilder);
   RegisterBuilder(new CUnderOverBuilder);
   RegisterBuilder(new CVBoxBuilder);
   RegisterBuilder(new COpenCloseBuilder);
   RegisterBuilder(new CScaleCmdBuilder);
   RegisterBuilder(new CTextCmdBuilder);
   RegisterBuilder(new CShiftBuilder);
   RegisterBuilder(new CMathClassBuilder);
   RegisterBuilder(new CBoxCmdBuilder);
   RegisterBuilder(new COverlayBuilder);
   RegisterBuilder(new CMathStyleBuilder);
   RegisterBuilder(new CMathSymBuilder(m_Parser.Doc()));//MUST BE LAST!
}
CMathModeProcessor::~CMathModeProcessor() {
   for (IMathItemBuilder* pBuilder : m_vItemBuilders) {
      delete pBuilder;
   }
}
// MATH MODE PROCESSING
CMathItem* CMathModeProcessor::ProcessItemToken(IN OUT int& nIdx, const SParserContext& ctx) {
   const STexToken* pCurToken = GetToken(nIdx);
   if(pCurToken && pCurToken->nType == ettSPACE) {
      pCurToken = GetToken(++nIdx); // ignore space token
   }
   if (!pCurToken)
      return nullptr;//ntd
   if (pCurToken->nType != ettSB_OPEN && pCurToken->nTkIdxEnd > 0)
      return m_Parser.ProcessGroup(nIdx, ctx);
   //else
   STexToken tkItem = *pCurToken;//copy
   CMathItem* pItem = nullptr;
   switch (tkItem.nType) {
   case ettSB_OPEN:
   case ettSB_CLOSE:
   case ettALNUM:     pItem = ProcessAlnum_(nIdx, ctx); break;
   case ettNonALPHA:
      if (TokenText(nIdx) != "'")
         pItem = ProcessNonAlnum_(nIdx, ctx);
      //else, superscript '= ^\prime, to be processed in the group
      break;
   case ettCOMMAND: { // command or symbol!
         string sCmd = TokenText(nIdx);
         if (_isMathStyleCommand(sCmd) || _isFontSizeCommand(sCmd) || _isNewlineCommand(sCmd))
            return nullptr; // not an Item Token!
         if (sCmd == "\\middle" || sCmd == "\\hline")
            return nullptr; //group should handle this command!
         pItem = BuildItemCmd_(nIdx, ctx);
      }
      break;
   }
   return pItem;
}
CMathItem* CMathModeProcessor::ProcessGroup(IN OUT int& nIdx, const SParserContext& ctx,
                                             IN OUT CEnvHelper& envh) {
   int nIdxStart, nIdxEnd;
   SParserContext ctxG(ctx);
   vector<CRawItem> vGroupItems;
   const STexToken* pTokenOpen = GetToken(nIdx);
   _ASSERT_RET(pTokenOpen,nullptr);//snbh!
   const STexToken& tkOpen = *pTokenOpen;
   _ASSERT_RET((int)tkOpen.nTkIdxEnd > nIdx, nullptr);//snbh!
   nIdxEnd = tkOpen.nTkIdxEnd;
   if (ctxG.bInLeftRight) {
      //try to add opening delimiter
      vGroupItems.emplace_back(nIdx + 1, nIdx + 1);
      const STexToken* pNext = GetToken(nIdx + 1);
      if (!pNext || (pNext->nType != ettNonALPHA && pNext->nType != ettCOMMAND) ||
         !vGroupItems.back().InitDelimiter(m_Parser.Doc().LMFManager(), TokenText(nIdx + 1), edtOpen)
         ) {
         m_Parser.SetError(nIdx + 1, "Missing or bad delimiter after \\left");
         return nullptr;
      }
      nIdxStart = nIdx + 2;            //skip opening delimiter token
      nIdx = tkOpen.nTkIdxEnd + 2;     //skip closing delimiter token
   }
   else if (envh.Enabled()) {
      nIdxStart = envh.GetStartTkIdx();
      nIdx = tkOpen.nTkIdxEnd + 4;     //skip: \end,{,env,}
   }
   else {
      nIdxStart = nIdx + 1;            //skip group_open token
      nIdx = tkOpen.nTkIdxEnd + 1;     //skip group_close token
   }
   if (nIdxStart >= nIdxEnd)
      return nullptr; //ntd
   for (int nIdxG = nIdxStart; nIdxG < nIdxEnd;) {
      CMathItem* pItem = nullptr;
      const STexToken* pCurToken = GetToken(nIdxG);
      if (pCurToken && pCurToken->nType == ettSPACE) {
         ++nIdxG;
         continue; //ignore spaces
      }
      _ASSERT_RET(pCurToken, nullptr);//snbh!
      int nCurTokenIdx = nIdxG;
      pItem = ProcessItemToken(nIdxG, ctxG);
      if (!m_Parser.HasError() && !pItem && !pCurToken->nTkIdxEnd) { //post-process non-group tokens
         switch (pCurToken->nType) {
         case ettNonALPHA:
            if (TokenText(nIdxG) == "'") {
               if (vGroupItems.empty()) //cannot add prime as superscript->add a prime glyph as an Item
                  pItem = ProcessNonAlnum_(nIdx, ctx);
               else {// superscript!
                  if (!vGroupItems.back().AddPrime(nCurTokenIdx))
                     m_Parser.SetError(nCurTokenIdx, "' is a double superscript");
                  else {
                     pItem = nullptr; // already consumed
                     ++nIdxG;
                  }
               }
            }
            break;
         case ettCOMMAND: { // command or symbol!
               string sCmd = TokenText(nIdxG);
               if (_isMathStyleCommand(sCmd))
                  ProcessMathStyleCmd_(nIdxG, ctxG);
               else if (_isFontSizeCommand(sCmd))
                  ProcessFontSizeCmd_(nIdxG, ctxG);
               else if (_isNewlineCommand(sCmd)) {
                  ++nIdxG;
                  if (!ctxG.bDisplayFormula) {
                     vGroupItems.emplace_back(nCurTokenIdx, nCurTokenIdx);
                     vGroupItems.back().InitNewLine();
                  }
               }
               else if (sCmd == "\\middle" && ctxG.bInLeftRight) {
                  nCurTokenIdx = ++nIdxG;//skip to delimiter
                  vGroupItems.emplace_back(nCurTokenIdx, nCurTokenIdx);
                  const STexToken* pNext = GetToken(nIdxG);
                  if (!pNext || (pNext->nType != ettNonALPHA && pNext->nType != ettCOMMAND) ||
                     !vGroupItems.back().InitDelimiter(m_Parser.Doc().LMFManager(), TokenText(nIdxG), edtFence)
                     )
                     m_Parser.SetError(nIdxG, "Missing or bad delimiter after '\\middle'");
                  ++nIdxG; // skip delimiter
               }
               else if(sCmd == "\\hline"){
                  if (!envh.Enabled())
                     m_Parser.SetError(nCurTokenIdx, "\\hline is allowed only in environments");
                  else {
                     ++nIdxG;
                     vGroupItems.emplace_back(nCurTokenIdx, nCurTokenIdx);
                     vGroupItems.back().InitHLine();
                  }
               }
               else
                  m_Parser.SetError(nCurTokenIdx, "Unexpected command '" + sCmd + "'");
            }
            break;
         case ettCOMMENT:++nIdxG;  break;    // ignore comment tokens
         case ettAMP:
            if (!envh.Enabled())
               m_Parser.SetError(nCurTokenIdx, "& is allowed only in environments");
            else {
               ++nIdxG;
               vGroupItems.emplace_back(nCurTokenIdx, nCurTokenIdx);
               vGroupItems.back().InitAmp();
            }
            break;
         case ettSUBS:
         case ettSUPERS: {
               SParserContext ctxGS(ctxG); //sub/super context
               if (pCurToken->nType == ettSUBS)
                  ctxGS.SetInSubscript();
               else
                  ctxGS.SetInSuperscript();
               nCurTokenIdx = ++nIdxG;
               if (pCurToken->nType == ettSUPERS && !vGroupItems.empty()) {
                  const STexToken* pSubSuperToken = GetToken(nIdxG);
                  if (pSubSuperToken && pSubSuperToken->nType == ettCOMMAND &&
                     "\\prime" == TokenText(nIdxG)) {
                     if (!vGroupItems.back().AddPrime(nCurTokenIdx))
                        m_Parser.SetError(nCurTokenIdx, "\\prime is a double superscript");
                     else {
                        ++nIdxG;//skip \\prime
                        break;
                     }

                  }
               }
               pItem = ProcessItemToken(nIdxG, ctxGS);
               if (pItem) {
                  if (vGroupItems.empty() || 
                     (!vGroupItems.back().HasMathItem() && !vGroupItems.back().IsDelimiter()))
                     vGroupItems.emplace_back(nCurTokenIdx, nIdxG - 1);//gen fraction base
                  bool bOk = false;
                  if (pCurToken->nType == ettSUBS)
                     bOk = vGroupItems.back().AddSubScript(pItem, nCurTokenIdx, nIdxG - 1);
                  else
                     bOk = vGroupItems.back().AddSuperScript(pItem, nCurTokenIdx, nIdxG - 1);
                  if (!bOk)
                     m_Parser.SetError(nCurTokenIdx - 1, "Double subcsript/superscript is not allowed");
                  else
                     pItem = nullptr; // already consumed
               }
               else
                  m_Parser.SetError(nCurTokenIdx, "Missing item after _/^!");
            }
               break;
         default:
            m_Parser.SetError(nCurTokenIdx, "Unexpected token in ProcessGroup");
         }
      }
      if (m_Parser.HasError()) {
         for (auto& rawitem : vGroupItems) {
            rawitem.Delete(); //cleanup!
         }
         return nullptr;
      }
      if (pItem)
         vGroupItems.emplace_back(nCurTokenIdx, nIdxG-1, pItem);
   } //main for
   if (ctxG.bInLeftRight) {
      //try to add closing delimiter
      vGroupItems.emplace_back(nIdxEnd + 1, nIdxEnd + 1);
      const STexToken* pNext = GetToken(nIdxEnd + 1);
      if (!pNext || (pNext->nType != ettNonALPHA && pNext->nType != ettCOMMAND) ||
         !vGroupItems.back().InitDelimiter(m_Parser.Doc().LMFManager(), TokenText(nIdxEnd + 1), edtClose)
         ) {
         m_Parser.SetError(nIdxEnd + 1, "Missing or bad delimiter after \\right");
         return nullptr;
      }
   }
   //packing dispatch
   if (ctxG.bInLeftRight)
      return PackGroupItemsLeftRight_(vGroupItems, ctxG);
   if (envh.Enabled())
      return PackGroupItemsTabularEnv_(vGroupItems, ctxG, envh);
   //else, default
   return PackGroupItems_(vGroupItems, ctxG);
}
CMathItem* CMathModeProcessor::BuildItemCmd_(IN OUT int& nIdx, const SParserContext& ctx) {
   string sCmd = TokenText(nIdx);
   IMathItemBuilder* pBuilder = nullptr;
   for (IMathItemBuilder* pBldr : m_vItemBuilders) {
      if (!pBldr->CanTakeCommand(sCmd.c_str()))
         continue;
      pBuilder = pBldr;
      break;
   }
   if (!pBuilder) {
      m_Parser.SetError(nIdx, "Unknown command '" + sCmd + "'");
      return nullptr;
   }
   CParserAdapter parserAdapter(m_Parser, ++nIdx, ctx);
   CMathItem* pItem = pBuilder->BuildFromParser(sCmd.c_str(), &parserAdapter);
   nIdx = parserAdapter.CurrentTokenIdx();  // Sync position
   return pItem;
}

// HELPERS
const STexToken* CMathModeProcessor::GetToken(int nIdx) const {
   return m_Parser.GetToken(nIdx);
}
string CMathModeProcessor::TokenText(int nIdx) const {
   return m_Parser.TokenText(nIdx);
}

bool CMathModeProcessor::ProcessMathStyleCmd_(IN OUT int& nIdx, IN OUT SParserContext& ctx) {
   string sCmd = TokenText(nIdx);
   if (sCmd == "\\displaystyle")
      ctx.currentStyle = etsDisplay;
   else if (sCmd == "\\textstyle")
      ctx.currentStyle = etsText;
   else if (sCmd == "\\scriptstyle")
      ctx.currentStyle = etsScript;
   else if (sCmd == "\\scriptscriptstyle")
      ctx.currentStyle = etsScriptScript;
   else {
      _ASSERT(0); //snbh!
      m_Parser.SetError(nIdx, "Unknown math style command: " + sCmd);
      return false;
   }
   ++nIdx; // next token
   return true;
}
bool CMathModeProcessor::ProcessFontSizeCmd_(IN OUT int& nIdx, IN OUT SParserContext& ctx) {
   string sCmd = TokenText(nIdx);
   PCSTR szCmd = sCmd.c_str();
   _ASSERT_RET(*szCmd == '\\', false);//snbh!
   szCmd++; //skip '\\'
   for (const SFontSizeVariant& fsVar : _aFontSizeVariants) {
      if (0 == strcmp(szCmd, fsVar.szFontSizeCmd)) {
         ctx.ApplyFontScale(fsVar.fSizeFactor); //apply font-size scaling
         ++nIdx; // next token
         return true;
      }
   }
   _ASSERT_RET(0, false);//snbh!
}

CMathItem* CMathModeProcessor::ProcessAlnum_(IN OUT int& nIdx, const SParserContext& ctx) {
   string sText = TokenText(nIdx);
   CMathItem* pRet = nullptr;
   if (CWordItemBuilder::_IsNumber(sText.c_str()))
      pRet = CWordItemBuilder::_BuildMathWord(m_Parser.Doc(), sText, true, ctx);
   else if(sText.size() > 1) {
      // word to be split to chars in Math Mode!
      CHBoxItem *pHBox = new CHBoxItem(m_Parser.Doc(), ctx.currentStyle);
      for (char cChar : sText) {
         CMathItem* pWord = CWordItemBuilder::_BuildMathWord(m_Parser.Doc(), string(1, cChar), false, ctx);
         pHBox->AddItem(pWord);
      }
      pHBox->Update();
      pHBox->NormalizeOrigin(0, 0);
      pRet = pHBox;
   }
   else //single-char word
      pRet = CWordItemBuilder::_BuildMathWord(m_Parser.Doc(), sText, false, ctx);
   if(!pRet)
      m_Parser.SetError(nIdx, "Failed to build alphanumeric item");
   else
      ++nIdx;
   return pRet;
}
CMathItem* CMathModeProcessor::ProcessNonAlnum_(IN OUT int& nIdx, const SParserContext& ctx) {
   CMathItem* pRet = nullptr;
   string sText = TokenText(nIdx);
   _ASSERT(sText.size() == 1 || sText.size() == 2);
   if (sText.size() == 2) {
      //escape char
      _ASSERT(sText[0] == '\\');
      sText = sText[1];
   }
   pRet = CWordItemBuilder::_BuildMathWord(m_Parser.Doc(), sText, false, ctx);
   if (!pRet)
      m_Parser.SetError(nIdx, "Failed to build non-alphanumeric character");
   else
      ++nIdx;
   return pRet;
}
//packers
CMathItem* CMathModeProcessor::PackGroupItems_(vector<CRawItem>& vGroupItems, const SParserContext& ctx) {
   const int nRealItems = (int)std::count_if(vGroupItems.begin(), vGroupItems.end(), [](const CRawItem& ritem) {
      return ritem.HasMathItem();
      });
   if (!nRealItems)
      return nullptr; //ntd?
   vector<vector<CMathItem*>> vvLines(1);
   for (CRawItem& ritem : vGroupItems) {
      if (ritem.IsNewLine())
         vvLines.emplace_back();    // goto new line
      else {
         _ASSERT(ritem.HasMathItem());
         CMathItem* pItem = ritem.BuildItem(m_Parser.Doc(), ctx.currentStyle, ctx.fUserScale);
         if (!pItem) {
            _ASSERT(0);
            m_Parser.SetError(ritem.TkIdxStart(), "RawItem failed to build");
            return nullptr;
         }
         vvLines.back().push_back(pItem);
      }
   } //for
   //pack lines to HBoxes
   if (vvLines.size() == 1) {
      if (vvLines[0].size() == 1)
         return vvLines[0][0];//just 1 item
      //else, pack items to HBox
      CHBoxItem* pHBox = new CHBoxItem(m_Parser.Doc(), ctx.currentStyle);
      for (CMathItem* pItem : vvLines[0]) {
         pHBox->AddItem(pItem);
      }
      pHBox->Update();
      pHBox->NormalizeOrigin(0, 0);
      return pHBox;
   }
   //else //multiline
   CContainerItem* pRet = new CContainerItem(m_Parser.Doc(), eacLINES, ctx.currentStyle);
   for (const vector<CMathItem*>& vLine : vvLines) {
      if (vLine.empty())
         continue; //todo!
      if(vLine.size() == 1)
         pRet->AddBox(vLine[0], 0, pRet->Box().Bottom());
      else {
         CHBoxItem* pHBox = new CHBoxItem(m_Parser.Doc(), ctx.currentStyle);
         for (CMathItem* pItem : vLine) {
            pHBox->AddItem(pItem);
         }
         pHBox->Update();
         pHBox->NormalizeOrigin(0, 0);
         pRet->AddBox(pHBox, 0, pRet->Box().Bottom());// TODO: inter-line spacing?
      }
   }
   return pRet;
}
CMathItem* CMathModeProcessor::PackGroupItemsLeftRight_( vector<CRawItem>& vGroupItems, 
                                                         const SParserContext& ctx) {
   _ASSERT(vGroupItems.size() >= 2 && vGroupItems.front().IsDelimiter() &&
      vGroupItems.back().IsDelimiter()); //snbh!
   //process raw_items
   vector<CMathItem*> vItems;
   for (CRawItem& ritem : vGroupItems) {
      if (ritem.IsDelimiter()) {
         vItems.push_back(nullptr); //delimiter marker
         continue;
      }
      if (!ritem.HasMathItem())
         continue; //ignore AMP or NewLine
      CMathItem* pItem = ritem.BuildItem(m_Parser.Doc(), ctx.currentStyle, ctx.fUserScale);
      if (!pItem) {
         _ASSERT(0); //snbh!
         m_Parser.SetError(ritem.TkIdxStart(), "RawItem failed to build");
         return nullptr;
      }
      vItems.push_back(pItem);
   }
   // calc min_top/max_bottom
   STexBox box;
   int32_t nAscent = 0;
   for (CMathItem* pItem : vItems) {
      if (!pItem)
         continue; //skip delimiter
      if (box.IsEmpty()) {
         box = pItem->Box();
         nAscent = pItem->Box().Ascent();
      }
      else {
         int32_t nItemAscent = pItem->Box().Ascent();
         int32_t nDY = nAscent - nItemAscent;
         pItem->MoveBy(0, nDY);
         box.Union(pItem->Box());     //only top/bottom are needed!
         pItem->MoveBy(0, -nDY); //restore
      }
   }
   const int32_t nSize = box.Height();
   CHBoxItem* pHBox = new CHBoxItem(m_Parser.Doc(), ctx.currentStyle);
   // pack to one line + build missed delimiters
   int nItemIdx = 0;
   for (CRawItem& ritem : vGroupItems) {
      if (ritem.IsDelimiter())
         vItems[nItemIdx] = ritem.BuildItem(m_Parser.Doc(), ctx.currentStyle, ctx.fUserScale, nSize);
      else if (!ritem.HasMathItem())
         continue; //ignore AMP or NewLine
      if(vItems[nItemIdx])
         pHBox->AddItem(vItems[nItemIdx]);
      ++nItemIdx;
   }
   pHBox->Update();
   pHBox->NormalizeOrigin(0, 0);
   pHBox->SetAtom(etaINNER);
   return pHBox;
}
CMathItem* CMathModeProcessor::PackGroupItemsTabularEnv_(vector<CRawItem>& vGroupItems,
                                 const SParserContext& ctx, const CEnvHelper& envh) {
   SEnvTable table;
   //build a table/grid of CRawItemindexes
   vector<CMathItem*> vCell; //current cell
   table.vRows.emplace_back(); //first row
   for (int nIdx = 0; nIdx < (int)vGroupItems.size(); ++nIdx) {
      const CRawItem& ritem = vGroupItems[nIdx];
      if (ritem.IsNewLine() || ritem.IsAmp()) {
         //pack last cell
         if (!vCell.empty()) {
            if (vCell.size() == 1)
               table.vRows.back().vCells.push_back(vCell[0]);
            else { //pack items to hbox
               CHBoxItem* pHBox = new CHBoxItem(m_Parser.Doc(), ctx.currentStyle);
               for (CMathItem* pItem : vCell) {
                  pHBox->AddItem(pItem);
               }
               pHBox->Update();
               pHBox->NormalizeOrigin(0, 0);
               table.vRows.back().vCells.push_back(pHBox);
            }
            vCell.clear();
         }
         if (ritem.IsNewLine()) {
            if(table.nCols < (int16_t)table.vRows.back().vCells.size())
               table.nCols = (int16_t)table.vRows.back().vCells.size();
            table.vRows.emplace_back();
         }
      }
      else if (ritem.IsHLine()) {
         if (!vCell.empty() || !table.vRows.back().vCells.empty()) {
            m_Parser.SetError(ritem.TkIdxStart(), "Unexpected \\hline");
            return nullptr;
         }
         if (table.vRows.back().cHLine == '=') {
            m_Parser.SetError(ritem.TkIdxStart(), "Tripple \\hline is not allowed");
            return nullptr;
         }
         else if (table.vRows.back().cHLine == '-')
            table.vRows.back().cHLine = '=';
         else
            table.vRows.back().cHLine = '-';
      }
      else {
         _ASSERT(ritem.HasMathItem());
         CMathItem* pItem = ritem.BuildItem(m_Parser.Doc(), ctx.currentStyle, ctx.fUserScale);
         if (!pItem) {
            _ASSERT(0);
            m_Parser.SetError(ritem.TkIdxStart(), "RawItem failed to build");
            return nullptr;
         }
         vCell.push_back(pItem);
      }
   } //for
   //pack last cell
   if (!vCell.empty()) {
      if (vCell.size() == 1)
         table.vRows.back().vCells.push_back(vCell[0]);
      else { //pack items to hbox
         CHBoxItem* pHBox = new CHBoxItem(m_Parser.Doc(), ctx.currentStyle);
         for (CMathItem* pItem : vCell) {
            pHBox->AddItem(pItem);
         }
         pHBox->Update();
         pHBox->NormalizeOrigin(0, 0);
         table.vRows.back().vCells.push_back(pHBox);
      }
      vCell.clear();
   }
   //remove move last row if empty
   if (table.vRows.back().vCells.empty()) {
      table.cHLineBottom = table.vRows.back().cHLine;
      table.vRows.pop_back();
   }
   //set columns for 1-row table
   if (table.nCols < (int16_t)table.vRows.back().vCells.size())
      table.nCols = (int16_t)table.vRows.back().vCells.size();

   vector<char> vHLines(table.vRows.size(),'\0');
   //build Tabel Item
   int nIdx = vGroupItems.front().TkIdxStart(); //TODO: better index?
   CParserAdapter parserAdapter(m_Parser, nIdx, ctx);
   CTableItem* pTable = new CTableItem(parserAdapter, ctx.currentStyle);
   if( !pTable->Init(envh.ColAlignments(), envh.VertLines(), table) ) {
      _ASSERT(0);//snbh!
      delete pTable;
      return nullptr;
   }
   //pack to left/right delimiters
   if(envh.LeftDelim() || envh.RightDelim() ) {
      vector<CRawItem> vLRItems;
      string sDelim;
      //left delim
      vLRItems.emplace_back(0, 0);
      if (!envh.LeftDelim())
         sDelim = ".";
      else if(envh.LeftDelim() == 'd')
         sDelim = "\\|"; //double pipe
      else         
         sDelim.append(1,envh.LeftDelim());
      vLRItems.back().InitDelimiter(m_Parser.Doc().LMFManager(), sDelim, edtOpen);
      //table item
      vLRItems.emplace_back(0, 0, pTable);
      //right delim
      vLRItems.emplace_back(0, 0);
      if (!envh.RightDelim())
         sDelim = ".";
      else if (envh.RightDelim() == 'd')
         sDelim = "\\|"; //double pipe
      else
         sDelim = string(1, envh.RightDelim());
      vLRItems.back().InitDelimiter(m_Parser.Doc().LMFManager(), sDelim, edtClose);
      return PackGroupItemsLeftRight_(vLRItems, ctx);
   }
   //else
   return pTable;
}




