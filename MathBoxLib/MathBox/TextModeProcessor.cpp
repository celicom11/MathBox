#include "stdafx.h"
#include "TextModeProcessor.h"
#include "TexParser.h"
#include "RawItem.h"
#include "Tokenizer.h"
#include "ParserAdapter.h"
#include "HBoxItem.h"
#include "WordItemBuilder.h"
#include "RuleItem.h"
// TextMode MathItem Builders
#include "ScaleCmdBuilder.h"
#include "HSpacingBuilder.h"
#include "TextCmdBuilder.h"
#include "TextAccentBuilder.h"
#include "TextSymBuilder.h"
#include "ShiftBuilder.h"
#include "BoxCmdBuilder.h"
#include "OverlayBuilder.h"
#include "VarCmdBuilder.h"
#include "MathStyleBuilder.h"



namespace { //static helpers
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
   inline bool _IsDigit(char wc) { return wc >= '0' && wc <= '9'; }
   inline bool _IsAlpha(char wc) { return (wc >= 'a' && wc <= 'z') || (wc >= 'A' && wc <= 'Z'); }
   inline bool _IsAlNum(char wc) { return _IsAlpha(wc) || _IsDigit(wc); }

}
// CTextModeProcessor
CTextModeProcessor::CTextModeProcessor(CTexParser& parser):m_Parser(parser) {
   //TEXT MODE COMMAND PROCESSORS/BUILDERS
   RegisterBuilder(new CHSpacingBuilder);
   RegisterBuilder(new CShiftBuilder);
   RegisterBuilder(new CScaleCmdBuilder);
   RegisterBuilder(new CTextCmdBuilder);
   RegisterBuilder(new CTextAccentBuilder);
   RegisterBuilder(new CTextSymBuilder);
   RegisterBuilder(new CBoxCmdBuilder);
   RegisterBuilder(new COverlayBuilder);
   RegisterBuilder(new CVarCmdBuilder);
   RegisterBuilder(new CMathStyleBuilder);
}
CTextModeProcessor::~CTextModeProcessor() {
   for (IMathItemBuilder* pBuilder : m_vItemBuilders) {
      delete pBuilder;
   }
}
// TEXT MODE PROCESSING
CMathItem* CTextModeProcessor::ProcessItemToken(IN OUT int& nIdx, const SParserContext& ctx) {
   const STexToken* pCurToken = GetToken(nIdx);
   if (!pCurToken)
      return nullptr;//ntd
   STexToken tkItem = *pCurToken;//copy
   CMathItem* pItem = nullptr;
   if (tkItem.nType != ettSB_OPEN && tkItem.nTkIdxEnd > 0)
      pItem = m_Parser.ProcessGroup(nIdx, ctx);
   else if(tkItem.nType == ettSPACE) {
      _ASSERT_RET(tkItem.nLen, nullptr);//snbh!
      STexGlue glueSpace;
      glueSpace.fNorm = glueSpace.fActual = tkItem.nLen * 1000.0f / 3;
      glueSpace.fStretchCapacity = glueSpace.fNorm / 2;
      glueSpace.fShrinkCapacity = glueSpace.fNorm / 3;
      pItem = new CGlueItem(m_Parser.Doc(), glueSpace, ctx.currentStyle, ctx.fUserScale);
      ++nIdx;
   }
   else if( tkItem.nType == ettALNUM || tkItem.nType == ettNonALPHA || 
            tkItem.nType == ettSB_OPEN || tkItem.nType == ettSB_CLOSE)
      pItem = BuildItemWordToken_(nIdx, ctx);
   else if (tkItem.nType == ettCOMMAND) { 
      // command or symbol!
      string sCmd = TokenText(nIdx);
      if (_isFontSizeCommand(sCmd) || _isNewlineCommand(sCmd))
         return nullptr; // not processed here
      pItem = BuildItemCmd_(nIdx, ctx);
   }
   return pItem;
}
CMathItem* CTextModeProcessor::ProcessGroup(IN OUT int& nIdx, const SParserContext& ctx) {
   vector<CRawItem> vGroupItems;
   SParserContext ctxG(ctx); //may be changed during processing!
   int nIdxStart, nIdxEnd;
   if (nIdx == -1) {
      //root group
      nIdxStart = 0;
      nIdx = nIdxEnd = (int)m_Parser.GetTokens().size();
   }
   else {
      //we must be in {} group
      const STexToken* pTokenOpen = GetToken(nIdx);
      _ASSERT_RET(pTokenOpen && pTokenOpen->nType == ettFB_OPEN, nullptr); //snbh!
      nIdxStart = nIdx + 1;   //skip group_open token
      //skip group_close token
      nIdxEnd = pTokenOpen->nTkIdxEnd;
      nIdx = pTokenOpen->nTkIdxEnd + 1;
   }
   for (int nIdxG = nIdxStart; nIdxG < nIdxEnd;) {
      CMathItem* pItem = nullptr;
      const STexToken* pCurToken = GetToken(nIdxG);
      _ASSERT_RET(pCurToken,nullptr);//snbh!
      const int nCurTokenIdx = nIdxG;
      pItem = ProcessItemToken(nIdxG, ctxG);
      if(!m_Parser.HasError() && !pItem && !pCurToken->nTkIdxEnd) { //post-process non-group tokens
         switch (pCurToken->nType) {
         case ettCOMMAND: { // command or symbol!
            string sCmd = TokenText(nIdxG);
            if (_isFontSizeCommand(sCmd))
               ProcessFontSizeCmd_(nIdxG, ctxG);
            else if (_isNewlineCommand(sCmd)) {
               ++nIdxG;
               if (!ctxG.bNoNewLines) {
                  vGroupItems.emplace_back(nCurTokenIdx, nCurTokenIdx);
                  vGroupItems.back().InitNewLine();
               }
               //else TraceLog("ignoring NewLine token!");
            }
            else
               m_Parser.SetError(nCurTokenIdx, "Unexpected command '" + sCmd +"'");
         }
            break;
         case ettCOMMENT:++nIdxG;  break;    // ignore comment tokens
         case ettAMP: m_Parser.SetError(nCurTokenIdx, "& is not allowed in text mode"); break;
         case ettSUBS: m_Parser.SetError(nCurTokenIdx, "_ must be in math mode"); break;
         case ettSUPERS: m_Parser.SetError(nCurTokenIdx, "^ must be in math mode"); break;
         default:
            _ASSERT(0);//snbh!
            m_Parser.SetError(nCurTokenIdx, "Unexpected token in ProcessGroup!");
         }
      }
      if (m_Parser.HasError()) {
         for (auto& rawitem : vGroupItems) {
            rawitem.Delete(); //cleanup!
         }
         return nullptr;
      }
      if (pItem) {
         if (pCurToken->nTkIdxEnd && (pCurToken->nType == ett$$ || 
            (pCurToken->nType == ettCOMMAND && TokenText(nCurTokenIdx)=="\\[")) ) {
            //prepend new line
            if (!vGroupItems.empty() && !vGroupItems.back().IsNewLine()) {
               vGroupItems.emplace_back(nCurTokenIdx, nCurTokenIdx);
               vGroupItems.back().InitNewLine();
            }
            //todo: add hfill glues for centering formula
            vGroupItems.emplace_back(nCurTokenIdx, nIdxG-1, pItem);
            if (nIdxG < nIdxEnd) {
               //append post line
               vGroupItems.emplace_back(nIdxG - 1, nIdxG - 1);
               vGroupItems.back().InitNewLine();
            }
         }
         else
            vGroupItems.emplace_back(nCurTokenIdx, nIdxG - 1, pItem);
      }
   } //main for
   return PackGroupItems_(vGroupItems, ctxG);
}
CMathItem* CTextModeProcessor::BuildItemCmd_(IN OUT int& nIdx, const SParserContext& ctx) {
   string sCmd = TokenText(nIdx);
   IMathItemBuilder* pBuilder = nullptr;
   for (IMathItemBuilder* pBldr : m_vItemBuilders) {
      if (!pBldr->CanTakeCommand(sCmd.c_str()))
         continue;
      pBuilder = pBldr;
      break; //found
   }
   if (!pBuilder) {
      m_Parser.SetError(nIdx, "Unkown command '" + sCmd + "'");
      return nullptr;
   }
   CParserAdapter parserAdapter(m_Parser, ++nIdx, ctx);
   CMathItem* pItem = pBuilder->BuildFromParser(sCmd.c_str(), &parserAdapter);
   nIdx = parserAdapter.CurrentTokenIdx();  // Sync position
   return pItem;
}
// HELPERS
const STexToken* CTextModeProcessor::GetToken(int nIdx) const {
   return m_Parser.GetToken(nIdx);
}
string CTextModeProcessor::TokenText(int nIdx) const {
   return m_Parser.TokenText(nIdx);
}

bool CTextModeProcessor::ProcessFontSizeCmd_(IN OUT int& nIdx, IN OUT SParserContext& ctx) {
   string sCmd = TokenText(nIdx);
   PCSTR szCmd = sCmd.c_str();
   if (szCmd[0] != '\\')
      return false;
   szCmd++; //skip '\'
   for (const SFontSizeVariant& fsVar : _aFontSizeVariants) {
      if (0 == strcmp(szCmd, fsVar.szFontSizeCmd)) {
         ctx.ApplyFontScale(fsVar.fSizeFactor); //apply font-size scaling
         ++nIdx; // next token
         return true;
      }
   }
   _ASSERT_RET(0, false);//snbh!
}
CMathItem* CTextModeProcessor::BuildItemWordToken_(IN OUT int& nIdx, const SParserContext& ctx) {
   const STexToken* pCurToken = GetToken(nIdx);
   _ASSERT_RET(pCurToken, nullptr);
   _ASSERT_RET(pCurToken->nType == ettALNUM || pCurToken->nType == ettNonALPHA || 
      pCurToken->nType == ettSB_OPEN || pCurToken->nType == ettSB_CLOSE, nullptr);
   string sText = TokenText(nIdx);
   //awkward solution for : single escape char
   //consider emit single ettNonALPHA char in tokenozer!
   if (sText[0] == '\\' && pCurToken->nType == ettNonALPHA && sText.size() == 2)
      sText = sText[1];
   CMathItem* pRet = CWordItemBuilder::_BuildText(m_Parser.Doc(), sText, ctx);
   if(!pRet)
      m_Parser.SetError(nIdx, "Failed to build word item");
   ++nIdx; //next token
   return pRet;
}
CMathItem* CTextModeProcessor::PackGroupItems_(vector<CRawItem>& vGroupItems, const SParserContext& ctx) {
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
         if (!ctx.bNoNewLines && pItem->Type() == eacTEXTLINES) {
            //UNBOX/MOVE LINES TO OUR CONTAINER
            CContainerItem* pCnt = static_cast<CContainerItem*>(pItem);
            _ASSERT(pCnt && !pCnt->Items().empty()); //snbh!
            
            //append first line 
            vvLines.back().push_back(pCnt->Items().front());
            for (int nLineIdx = 1; nLineIdx < (int)pCnt->Items().size(); ++nLineIdx) {
               vvLines.emplace_back();             // goto new line
               vvLines.back().push_back(pCnt->Items()[nLineIdx]);
            }
            pCnt->Clear(true); //do not delete child items!
            delete pCnt;       //not needed: all items moved out
         }
         else
            vvLines.back().push_back(pItem);
      }
   } //for
   //pack lines to HBoxes
   if (vvLines.size() == 1) {
      if (vvLines[0].size() == 1)
         return vvLines[0][0];
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
   int nDummy = 0;
   CParserAdapter parserAdapter(m_Parser, nDummy, ctx);
   float fLineskipRatio = 1.2f, fMinLineskipRatio = 0.1f;//default scaling values 
   parserAdapter.GetLength("LineskipRatio", fLineskipRatio);
   parserAdapter.GetLength("MinLineskipRatio", fMinLineskipRatio);
   const int32_t nBaselineskip = F2NEAREST(fLineskipRatio * ctx.EffectiveScale() * otfUnitsPerEm);
   const int32_t nMinLineskip = F2NEAREST(fMinLineskipRatio * ctx.EffectiveScale() * otfUnitsPerEm );
   CContainerItem* pRet = new CContainerItem(m_Parser.Doc(), (ctx.bNoNewLines ? eacLINES : eacTEXTLINES), ctx.currentStyle);
   for (const vector<CMathItem*>& vLine : vvLines) {
      if (vLine.empty()) {
         CMathItem* pStrut = new CRuleItem(m_Parser.Doc(), 0, otfUnitsPerEm, 3*otfUnitsPerEm/4, 
                                             ctx.currentStyle, ctx.fUserScale);
         int32_t nNextLineTop = nMinLineskip;
         if (!pRet->IsEmpty()) {
            const STexBox& boxLastLine = pRet->Items().back()->Box();
            nNextLineTop = boxLastLine.BaselineY() + nBaselineskip - pStrut->Box().Ascent();
            if (nNextLineTop - boxLastLine.Bottom() < nMinLineskip)
               nNextLineTop = boxLastLine.Bottom() + nMinLineskip;
         }
         pRet->AddBox(pStrut, 0, nNextLineTop);
      }
      else if (vLine.size() == 1) {
         int32_t nNextLineTop = nMinLineskip;
         if (!pRet->IsEmpty()) {
            const STexBox& boxLastLine = pRet->Items().back()->Box();
            nNextLineTop = boxLastLine.BaselineY() + nBaselineskip - vLine[0]->Box().Ascent();
            if (nNextLineTop - boxLastLine.Bottom() < nMinLineskip)
               nNextLineTop = boxLastLine.Bottom() + nMinLineskip;
         }
         pRet->AddBox(vLine[0], 0, nNextLineTop);
      }
      else { //pack line items
         CHBoxItem* pHBox = new CHBoxItem(m_Parser.Doc(), ctx.currentStyle);
         for (CMathItem* pItem : vLine) {
            pHBox->AddItem(pItem);
         }
         pHBox->Update();
         pHBox->NormalizeOrigin(0, 0);
         int32_t nNextLineTop = nMinLineskip;
         if (!pRet->IsEmpty()) {
            const STexBox& boxLastLine = pRet->Items().back()->Box();
            nNextLineTop = boxLastLine.BaselineY() + nBaselineskip - pHBox->Box().Ascent();
            if (nNextLineTop - boxLastLine.Bottom() < nMinLineskip)
               nNextLineTop = boxLastLine.Bottom() + nMinLineskip;
         }
         pRet->AddBox(pHBox, 0, nNextLineTop);
      }
   }
   pRet->NormalizeOrigin(0, 0);
   pRet->SetMathAxis(pRet->Box().Height() / 2); //keep axis in the middle
   return pRet;
}


