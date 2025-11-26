#include "stdafx.h"
#include "TexParser.h"
#include "RawItem.h"
#include "Tokenizer.h"
#include "ParserAdapter.h"
#include "LMFontManager.h"
#include "HBoxItem.h"
#include "ExtGlyphBuilder.h"
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


//extern CLMFontManager g_LMFManager;
namespace { //static helpers
   bool _isTextCommand(const string& sCmd) {
      // text mode + font commands: \text,\textit,etc.
      PCSTR szCmd = sCmd.c_str();
      if (szCmd[0] != '\\')
         return false;
      szCmd++; //skip '\'
      if (!*szCmd)
         return false;
      STextFontStyle tfStyle; //dummy
      return CLMFontManager::_GetTextFontStyle(szCmd, tfStyle);
   }
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
   bool _IsOpenClose(const string& sOpen, const string& sClose) {
      //check matching open/close delimiters
      if (sOpen == "(" && sClose == ")")
         return true;
      if (sOpen == "[" && sClose == "]")
         return true;
      if (sOpen == "{" && sClose == "}")
         return true;
      if (sOpen == "\\[" && sClose == "\\]")
         return true;
      if (sOpen == "\\left" && sClose == "\\right")
         return true;
      if (sOpen == "\\begin" && sClose == "\\end") //environments
         return true;
      return false;
   }
   bool _IsKnownEnvironment(const string& sEnv) {
      return sEnv == "array" || sEnv == "matrix"; //q&d
   }
}
// CTexParser
CTexParser::CTexParser() {
   RegisterBuilder(new CAccentBuilder);
   RegisterBuilder(new CFractionBuilder);
   RegisterBuilder(new CHSpacingBuilder);
   RegisterBuilder(new CLOpBuilder);
   RegisterBuilder(new CMathFontCmdBuilder);
   RegisterBuilder(new CMathSymBuilder);
   RegisterBuilder(new CRadicalBuilder);
   RegisterBuilder(new CScaleCmdBuilder);
   RegisterBuilder(new CUnderOverBuilder);
   RegisterBuilder(new CVBoxBuilder);
   RegisterBuilder(new COpenCloseBuilder);
}
CTexParser::~CTexParser() {
   delete m_pTokenizer;
   for (IMathItemBuilder* pBuilder : m_vBuilders) {
      delete pBuilder;
   }
}
CMathItem* CTexParser::Parse(const string& sText) {
   if (m_pTokenizer)
      delete m_pTokenizer;
   m_pTokenizer = new CTokenizer(sText);
   if (!m_pTokenizer->Tokenize(m_vTokens, m_Error))
      return nullptr; //tokenization error
   if (!BuildGroups_())
      return nullptr;      // grouping error
   m_Error.eStage = epsBUILDING; //fwd set
   SParserContext ctx;     // todo: initial context
   ctx.bTextMode = true;   // start in text mode
   int nStart = -1;
   return ProcessGroup(nStart, ctx);
}
// TOKEN PROCESSING
// Used by CParserAdapter as well!
CMathItem* CTexParser::ProcessItemToken(IN OUT int& nIdx, const SParserContext& ctx) {
   if((uint32_t)nIdx >= m_vTokens.size())
      return nullptr; //no more tokens
   STexToken tkItem = m_vTokens[nIdx];//copy
   CMathItem* pItem = nullptr;
   m_Error.nPosition = tkItem.nPos;
   if (tkItem.nTkIdxEnd > 0)
      pItem = ProcessGroup(nIdx, ctx);
   else {
      switch (tkItem.nType) {
      case ettALNUM:     pItem = ProcessAlnum_(nIdx, ctx); break;
      case ettNonALPHA:
         if(ctx.bTextMode || TokenText(nIdx) != "'")
            pItem = ProcessNonAlnum_(nIdx, ctx);
         //else, superscript '= ^\prime, to be processed in the group
         break;
      case ettCOMMAND: { // command or symbol!
         string sCmd = TokenText(nIdx);
         if (_isMathStyleCommand(sCmd) || _isFontSizeCommand(sCmd) || _isNewlineCommand(sCmd))
            return nullptr; // not an Item Token!
         if (sCmd == "\\middle")
            return nullptr; //group should handle this command!
         if (_isTextCommand(sCmd)) //both text/math mode
            pItem = ProcessTextCmd_(nIdx, ctx);
         else
            pItem = ProcessItemBuilderCmd_(nIdx, ctx);
         }
         break;
      }
   }
   return pItem;
}

CMathItem* CTexParser::ProcessGroup(IN OUT int& nIdx, const SParserContext& ctx) {
   int nIdxStart, nIdxEnd;
   bool bCanBeEmpty = true;
   SParserContext ctxG(ctx);
   vector<CRawItem> vGroupItems;

   if (nIdx == -1) {
      //root group
      ctxG = ctx; //copy all settings
      nIdxStart = 0;
      nIdx = nIdxEnd = (int)m_vTokens.size();
   }
   else {
      const STexToken& tkOpen = m_vTokens[nIdx];
      _ASSERT_RET((int)tkOpen.nTkIdxEnd > nIdx && tkOpen.nTkIdxEnd <= m_vTokens.size(), nullptr);//snbh!
      if(!OnGroupOpen_(tkOpen, ctxG, bCanBeEmpty))
         return nullptr; //error in group open token
      nIdxEnd = tkOpen.nTkIdxEnd; //group_close token
      if (ctxG.bInLeftRight) {
         //try to add opening delimiter
         vGroupItems.emplace_back();
         const STexToken* pNext = GetToken(nIdx+1);
         if (!pNext || (pNext->nType != ettNonALPHA && pNext->nType != ettCOMMAND) ||
            !vGroupItems.back().InitDelimiter(TokenText(nIdx + 1), edtOpen)
            ) {
            m_Error.nPosition = m_vTokens[nIdx+1].nPos;
            m_Error.sError = "Missing or bad delimiter after \\left";
            return nullptr;
         }
         nIdxStart = nIdx + 2;            //skip opening delimiter token
         nIdx = tkOpen.nTkIdxEnd + 2;     //skip closing delimiter token
      }
      else if (!ctxG.sEnv.empty()) {
         nIdxStart = nIdx + 4;            //skip: \begin,{,env,}
         nIdx = tkOpen.nTkIdxEnd + 4;     //skip: \end,{,env,}
      }
      else {
         nIdxStart = nIdx + 1;            //skip group_open token
         nIdx = tkOpen.nTkIdxEnd + 1;     //skip group_close token
      }
   }
   if (nIdxEnd <= nIdxStart) {
      if(!bCanBeEmpty) {
         m_Error.nPosition = m_vTokens[nIdxStart].nPos;
         m_Error.sError = "Empty group is not allowed here!";
      }
      //else, empty group is OK 
      return nullptr;
   }
   _ASSERT_RET(nIdxStart >=0 && nIdxEnd > nIdxStart && nIdxEnd <= m_vTokens.size() , nullptr);
   for (int nIdxG = nIdxStart; nIdxG < nIdxEnd;) {
      CMathItem* pItem = nullptr;
      const STexToken& tkCurrent = m_vTokens[nIdxG];
      m_Error.nPosition = tkCurrent.nPos;
      pItem = ProcessItemToken(nIdxG, ctxG);
      if(!m_Error.sError.empty())
         return nullptr; //error in sub-group
      if(!pItem) {
         switch (tkCurrent.nType) {
         case ettNonALPHA: 
            if (!ctx.bTextMode && TokenText(nIdxG) == "'") {
               if(vGroupItems.empty()) //cannot add prime as superscript->add a prime glyph as an Item
                  pItem = ProcessNonAlnum_(nIdx, ctx);
               else {// superscript!
                  if (!vGroupItems.back().AddPrime())
                     m_Error.sError = "' as a double superscript is not allowed";
                  else
                     pItem = nullptr; // already consumed
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
               vGroupItems.emplace_back();
               vGroupItems.back().InitNewLine();
            }
            else if (sCmd == "\\middle" && ctxG.bInLeftRight) {
               vGroupItems.emplace_back();
               ++nIdxG;//skip to next
               const STexToken* pNext = GetToken(nIdxG);
               if (!pNext || (pNext->nType != ettNonALPHA && pNext->nType != ettCOMMAND) ||
                  !vGroupItems.back().InitDelimiter(TokenText(nIdxG), edtFence)
                  )
                  m_Error.sError = "Missing or bad delimiter after \\middle ";
            }
            else
               m_Error.sError = "Unexpected command '" + sCmd +"'";
         }
            break;
         case ettCOMMENT:break;    // ignore comment tokens
         case ettAMP: 
            if (ctxG.sEnv.empty())
               m_Error.sError = "& is allowed only in environments";
            else {
               vGroupItems.emplace_back();
               vGroupItems.back().InitAmp();
            }
            break;
         case ettSUBS:
         case ettSUPERS: {
               SParserContext ctxGS(ctxG); //sub/super context
               if (tkCurrent.nType == ettSUBS)
                  ctxGS.SetInSubscript();
               else
                  ctxGS.SetInSuperscript();
               pItem = ProcessItemToken(++nIdxG, ctxGS);
               if (pItem) {
                  if (vGroupItems.empty())
                     vGroupItems.emplace_back();//new raw item
                  bool bOk = false;
                  if (tkCurrent.nType == ettSUBS)
                     bOk = vGroupItems.back().AddSubScript(pItem);
                  else
                     bOk = vGroupItems.back().AddSuperScript(pItem);
                  if (!bOk) {
                     m_Error.nPosition = tkCurrent.nPos;
                     m_Error.sError = "Double subcsript/superscript is not allowed!";
                  }
                  else
                     pItem = nullptr; // already consumed
               }
               else
                  m_Error.sError = "Missing item after _/^!";

            }
            break;
         default:
            m_Error.sError = "Unexpected token in ProcessGroup!";
         }
      }
      if (!m_Error.sError.empty()) {
         // TODO: cleanup lines!
         return nullptr;//error in sub-group
      }
      if (pItem)
         vGroupItems.emplace_back(pItem);
   } //main for
   if (ctxG.bInLeftRight) {
      //try to add closing delimiter
      vGroupItems.emplace_back();
      const STexToken* pNext = GetToken(nIdxEnd + 1);
      if (!pNext || (pNext->nType != ettNonALPHA && pNext->nType != ettCOMMAND) ||
         !vGroupItems.back().InitDelimiter(TokenText(nIdxEnd + 1), edtClose)
         ) {
         m_Error.nPosition = m_vTokens[nIdx + 1].nPos;
         m_Error.sError = "Missing or bad delimiter after \\right";
         return nullptr;
      }
   }
   return PackGroupItems_(nIdxStart,vGroupItems,ctxG);
}
// HELPERS
bool CTexParser::OnGroupOpen_(const STexToken& tkOpen, IN OUT SParserContext& ctxG, OUT bool& bCanBeEmpty) {
   if (!ctxG.bTextMode && (tkOpen.nType == ett$ || tkOpen.nType == ett$$)) {
      m_Error.nPosition = tkOpen.nPos;
      m_Error.sError = "Inner $..$/$$...$$ are not allowed in math mode!";
      return false;
   }
   // reset external context flags
   switch (tkOpen.nType) {
   case ettSB_OPEN: bCanBeEmpty = false; break;
   case ettFB_OPEN:bCanBeEmpty = true; break;
   case ett$$:
      ctxG.currentStyle = etsDisplay;
      ctxG.bTextMode = false; //math display mode
      bCanBeEmpty = false;
      break;
   case ett$:
      ctxG.currentStyle = etsText;
      ctxG.bTextMode = false; //math inline mode
      bCanBeEmpty = false;
      break;
   case ettCOMMAND: {
      string sCmd = m_pTokenizer->TokenText(tkOpen);
      if (sCmd == "\\[") {
         if (!ctxG.bTextMode) {
            m_Error.nPosition = tkOpen.nPos;
            m_Error.sError = "Inner \\[...\\] are not allowed in math mode!";
            return false;
         }
         ctxG.currentStyle = etsDisplay;
         ctxG.bTextMode = false; //math mode
         bCanBeEmpty = false;
      }
      else if (sCmd == "\\left") {
         if (ctxG.bTextMode) {
            m_Error.nPosition = tkOpen.nPos;
            m_Error.sError = "\\left can only be used in math mode!";
            return false;
         }
         bCanBeEmpty = true;
         ctxG.bInLeftRight = true;
      }
      else 
         _ASSERT_RET(0, false); //snbh!
   }
      break;
   default:
      _ASSERT_RET(0, false); //snbh!
   }
   return true;
}
CMathItem* CTexParser::PackGroupItems_(int nIdxStart, vector<CRawItem>& vGroupItems,
                                        const SParserContext& ctx) {
   bool bOneLine = !ctx.bTextMode && ctx.currentStyle.Style() == etsDisplay &&
                     ctx.sEnv.empty(); //EOL/breaks are ignored;
   if (ctx.bInLeftRight) {
      bOneLine = true; //"\left and \right must appear in the same "math list"(c)
      _ASSERT(vGroupItems.size() >= 2 && vGroupItems.front().IsDelimiter() &&
         vGroupItems.back().IsDelimiter()); //snbh!
   }
   //remove trailing EOLs - needed?
   //while (!vGroupItems.empty()) {
   //   if (vGroupItems.back().IsNewLine())
   //      vGroupItems.pop_back();
   //}
   const int nRealItems = (int)std::count_if(vGroupItems.begin(), vGroupItems.end(), [](const CRawItem& ritem) {
      return ritem.HasMathItem();
      });
   if (!nRealItems)
      return nullptr; //ntd?

   if (ctx.bInLeftRight) { //one line, no columns
      //process raw_items
      vector<CMathItem*> vItems;
      for (CRawItem& ritem : vGroupItems) {
         if (ritem.IsDelimiter()) {
            vItems.push_back(nullptr); //delimiter marker
            continue;
         }
         if (!ritem.HasMathItem())
            continue; //ignore AMP or NewLine
         CMathItem* pItem = ritem.BuildItem(ctx.currentStyle, ctx.fUserScale);
         if (!pItem) {
            _ASSERT(0); //snbh!
            m_Error.sError = "Failed to build item!"; //todo!
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
      CHBoxItem* pHBox = new CHBoxItem(ctx.currentStyle);
      // pack to one line + build missed delimiters
      int nItemIdx = 0;
      for (CRawItem& ritem : vGroupItems) {
         if (ritem.IsDelimiter())
            vItems[nItemIdx] = ritem.BuildItem(ctx.currentStyle, ctx.fUserScale, nSize);
         else if (!ritem.HasMathItem())
            continue; //ignore AMP or NewLine
         _ASSERT(vItems[nItemIdx]);
         pHBox->AddItem(vItems[nItemIdx]);
         ++nItemIdx;
      }
      pHBox->Update();
      pHBox->NormalizeOrigin(0, 0);
      pHBox->SetAtom(etaINNER);
      return pHBox;
   }
   //else
   //prepare lines/columns of MathItems
   //const int nLineBreaks = (int)std::count_if(vGroupItems.begin(), vGroupItems.end(), [](const CRawItem& ritem) {
   //   return ritem.IsNewLine(); });
   vector<vector<CMathItem*>> vvColumns(1);
   vector<vector<CMathItem*>> vvLines(1);
   for (CRawItem& ritem : vGroupItems) {
      if (ritem.IsNewLine()) {
         if(!bOneLine)
            vvLines.emplace_back();    // goto new line
      }
      else if (ritem.IsAmp())
         vvColumns.emplace_back();  // goto new column
      else {
         _ASSERT(ritem.HasMathItem());
         CMathItem* pItem = ritem.BuildItem(ctx.currentStyle, ctx.fUserScale);
         if (!pItem) {
            _ASSERT(0);
            m_Error.sError = "Failed to build item!"; //todo!
            return nullptr;
         }
         vvLines.back().push_back(pItem);
         vvColumns.back().push_back(pItem);
      }
   } //for
   if (vvColumns.size() > 1) {
      //TODO: insert fixed size GlueItems to position items in the columns
      _ASSERT_RET(0,nullptr);
   }
   //else 
   if (vvLines.size() == 1) {
      if (vvLines[0].size() == 1)
         return vvLines[0][0];//just 1 item
      //else, pack items to HBox
      CHBoxItem* pHBox = new CHBoxItem(ctx.currentStyle);
      for (CMathItem* pItem : vvLines[0]) {
         pHBox->AddItem(pItem);
      }
      pHBox->Update();
      return pHBox;
   }
   //else //multiline
   CContainerItem* pRet = new CContainerItem(eacVBOX, ctx.currentStyle);
   for (const vector<CMathItem*>& vLine : vvLines) {
      if (vLine.empty())
         continue; //todo!
      CHBoxItem* pHBox = new CHBoxItem(ctx.currentStyle);
      for (CMathItem* pItem : vLine) {
         pHBox->AddItem(pItem);
      }
      pHBox->Update();
      pRet->AddBox(pHBox, 0, pRet->Box().Bottom());// TODO: inter-line spacing
   }
   return pRet;
}
bool CTexParser::BuildGroups_() {
   m_Error.eStage = epsGROUPING;
   vector<int> vGroupStarts; // indexes of group opening tokens
   for (int nIdx = 0; nIdx < (int)m_vTokens.size(); ++nIdx) {
      STexToken& tkn = m_vTokens[nIdx];
      switch (tkn.nType)
      {
      case ettFB_OPEN:
      case ettSB_OPEN: vGroupStarts.push_back(nIdx); break;
      case ett$:
      case ett$$:     //check if closing the group!
         if (!vGroupStarts.empty() && m_vTokens[vGroupStarts.back()].nType == tkn.nType) {
            m_vTokens[vGroupStarts.back()].nTkIdxEnd = nIdx;
            vGroupStarts.pop_back();
         }
         else {
            //check if we already have opening $/$$ token:
            // "${$"    means '{'  is not closed!
            for(int nGSIdx: vGroupStarts) {
               STexToken& tknGS = m_vTokens[nGSIdx];
               if (tknGS.nType == tkn.nType) {
                  m_Error.nPosition = m_vTokens[vGroupStarts.back()].nPos;
                  m_Error.sError = "Unclosed group '" + TokenText(vGroupStarts.back()) +"'";
                  return false;
               }
            }
            //else , we are OK
            vGroupStarts.push_back(nIdx);
         }
         break;
      case ettCOMMAND: {
         string sCmd = TokenText(nIdx);
         if (sCmd == "\\left" || sCmd == "\\[" || sCmd == "\\begin") {

            if (sCmd == "\\begin") {
               const STexToken* pTknFBOpen = GetToken(nIdx+1);
               const STexToken* pTknEnv = GetToken(nIdx + 2);
               const STexToken* pTknFBClose = GetToken(nIdx + 3);
               if (!pTknFBOpen || pTknFBOpen->nType!= ettFB_OPEN || 
                  !pTknEnv || pTknEnv->nType != ettALNUM ||
                  !pTknFBClose || pTknFBClose->nType != ettFB_CLOSE) {
                  m_Error.nPosition = nIdx;
                  m_Error.sError = "Missing {environment} argument after \\begin";
                  return false;
               }
               if (!_IsKnownEnvironment(TokenText(nIdx + 2))) {
                  m_Error.nPosition = nIdx+2;
                  m_Error.sError = "Unknown environment '" + TokenText(nIdx + 2) + "'";
                  return false;
               }
            }
            vGroupStarts.push_back(nIdx);
         }
         else if (sCmd == "\\right" || sCmd == "\\]" || sCmd == "\\end") {
            if (vGroupStarts.empty() || !_IsOpenClose(TokenText(vGroupStarts.back()), sCmd)) {
               m_Error.nPosition = tkn.nPos;
               m_Error.sError = "Unmatched '" + sCmd + "' !";
               return false;
            }
            int nGroupEnd = nIdx;
            if (sCmd == "\\end") {
               const STexToken* pTknFBOpen = GetToken(nIdx + 1);
               const STexToken* pTknEnv = GetToken(nIdx + 2);
               const STexToken* pTknFBClose = GetToken(nIdx + 3);
               if (!pTknFBOpen || pTknFBOpen->nType != ettFB_OPEN ||
                  !pTknEnv || pTknEnv->nType != ettALNUM ||
                  !pTknFBClose || pTknFBClose->nType != ettFB_CLOSE) {
                  m_Error.nPosition = nIdx;
                  m_Error.sError = "Missing {environment} argument after \\end";
                  return false;
               }
               //const STexToken* pTknOpenEnv = GetToken(vGroupStarts.back() + 2);
               if (TokenText(vGroupStarts.back() + 2) != TokenText(nIdx + 2)) {
                  m_Error.nPosition = nIdx + 2;
                  m_Error.sError = "Unmatched environment name'" + TokenText(nIdx + 2) + "' afetr \\end";
                  return false;

               }
            }
            m_vTokens[vGroupStarts.back()].nTkIdxEnd = nGroupEnd;
            vGroupStarts.pop_back();
         }
      }
         break;
      case ettFB_CLOSE:
      case ettSB_CLOSE: {
         string sClosing = (tkn.nType == ettFB_CLOSE) ? "}" : "]";
         if (vGroupStarts.empty() || !_IsOpenClose(TokenText(vGroupStarts.back()), sClosing)) {
            //change token to nonAlpha
            tkn.nType = ettNonALPHA;
            continue;
         }
         m_vTokens[vGroupStarts.back()].nTkIdxEnd = nIdx;
         vGroupStarts.pop_back();
      }
         break;
      }//switch
   } //for
   if(!vGroupStarts.empty()) {
      const STexToken& tkUnmatched = m_vTokens[vGroupStarts.back()];
      m_Error.nPosition = tkUnmatched.nPos;
      m_Error.sError = "Unclosed opening token '" + TokenText(vGroupStarts.back()) + "' !";
      return false;
   }
   return true;
}
string CTexParser::TokenText(int nIdx) const {
   const STexToken* pToken = (nIdx >= 0 && nIdx < (int)m_vTokens.size()) ? &m_vTokens[nIdx] : nullptr;
   return pToken ? m_pTokenizer->TokenText(*pToken) : string();
}

// CONCRETE TOKEN PROCESSORS
bool CTexParser::ProcessMathStyleCmd_(IN OUT int& nIdx, IN OUT SParserContext& ctx) {
   if(ctx.bTextMode) {
      m_Error.nPosition = m_vTokens[nIdx].nPos;
      m_Error.sError = "Math style commands can only be used in math mode!";
      return false;
   }
   string sCmd = TokenText(nIdx);
   ++nIdx; // next token
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
      m_Error.nPosition = m_vTokens[nIdx - 1].nPos;
      m_Error.sError = "Unknown math style command: " + sCmd;
      return false;
   }
   return true;
}
bool CTexParser::ProcessFontSizeCmd_(IN OUT int& nIdx, IN OUT SParserContext& ctx) {
   string sCmd = TokenText(nIdx);
   ++nIdx; // next token
   PCSTR szCmd = sCmd.c_str();
   if (szCmd[0] != '\\')
      return false;
   szCmd++; //skip '\'
   for (const SFontSizeVariant& fsVar : _aFontSizeVariants) {
      if (0 == strcmp(szCmd, fsVar.szFontSizeCmd)) {
         ctx.fUserScale *= fsVar.fSizeFactor; //apply font-size scaling
         break;
      }
   }
   return true;
}
CMathItem* CTexParser::ProcessAlnum_(IN OUT int& nIdx, const SParserContext& ctx) {
   string sText = TokenText(nIdx);
   nIdx = nIdx + 1; //next token
   CMathItem* pRet = nullptr;
   if (ctx.bTextMode)
      pRet = CWordItemBuilder::BuildText(ctx.sFontCmd, sText, ctx.currentStyle, ctx.fUserScale);
   else { // math mode
      if (CWordItemBuilder::_IsNumber(sText.c_str()))
         pRet = CWordItemBuilder::BuildMathWord(ctx.sFontCmd, sText, true, ctx.currentStyle, ctx.fUserScale);
      else if(sText.size() > 1) {
         // process each char to a WordItem to have wider spacing
         CHBoxItem *pHBox = new CHBoxItem(ctx.currentStyle);
         for (char cChar : sText) {
            CMathItem* pWord = CWordItemBuilder::BuildMathWord(ctx.sFontCmd,
               string(1, cChar), false, ctx.currentStyle, ctx.fUserScale);
            pHBox->AddItem(pWord);
         }
         pHBox->Update();
         pRet = pHBox;
      }
      else //single-char word
         pRet = CWordItemBuilder::BuildMathWord(ctx.sFontCmd, sText, false, ctx.currentStyle, ctx.fUserScale);
   }
   if(!pRet) {
      m_Error.nPosition = m_vTokens[nIdx].nPos;
      m_Error.sError = "Failed to build alphanumeric item!";
   }
   return pRet;
}
CMathItem* CTexParser::ProcessNonAlnum_(IN OUT int& nIdx, const SParserContext& ctx) {
   CMathItem* pRet = nullptr;
   string sText = TokenText(nIdx);
   nIdx = nIdx + 1; //next token
   _ASSERT(sText.size() == 1 || sText.size() == 2);
   if (sText.size() == 2) {
      //escape char
      _ASSERT(sText[0] == '\\');
      sText = sText[1];
   }
   if (ctx.bTextMode)
      pRet = CWordItemBuilder::BuildText(ctx.sFontCmd, sText, ctx.currentStyle, ctx.fUserScale);
   else
      pRet = CWordItemBuilder::BuildMathWord(ctx.sFontCmd, sText, false, ctx.currentStyle, ctx.fUserScale);
   if (!pRet) {
      m_Error.nPosition = m_vTokens[nIdx].nPos;
      m_Error.sError = "Failed to process non-alphanumeric character!";
   }
   return pRet;
}
CMathItem* CTexParser::ProcessTextCmd_(IN OUT int& nIdx, const SParserContext& ctx) {
   //we require an argument in {}
   _ASSERT_RET(nIdx + 1 < m_vTokens.size(), nullptr);//snbh!
   if(m_vTokens[nIdx + 1].nTkIdxEnd == 0 || m_vTokens[nIdx + 1].nType != ettFB_OPEN) {
      m_Error.nPosition = m_vTokens[nIdx].nPos;
      m_Error.sError = "Text command requires argument in curly braces {}!";
      return nullptr;
   }
   SParserContext ctxText(ctx);
   ctxText.bTextMode = true; //text mode
   ctxText.sFontCmd = TokenText(nIdx);
   CMathItem* pArg = ProcessGroup(++nIdx, ctxText); //move to argument group
   if(!pArg) {
      //error in argument processing
      if (m_Error.sError.empty()) {
         m_Error.nPosition = m_vTokens[nIdx].nPos;
         m_Error.sError = "Failed to process argument of text command!";
      }
      return nullptr;
   }
   return pArg;
}
CMathItem* CTexParser::ProcessItemBuilderCmd_(IN OUT int& nIdx, const SParserContext& ctx) {
   string sCmd = TokenText(nIdx);
   IMathItemBuilder* pBuilder = nullptr;
   for (IMathItemBuilder* pBldr : m_vBuilders) {
      if (!pBldr->CanTakeCommand(sCmd.c_str(), ctx.bTextMode))
         continue;
      pBuilder = pBldr;
      break;
   }
   if (!pBuilder) {
      m_Error.nPosition = m_vTokens[nIdx].nPos;
      m_Error.sError = "Unkown command '"+ sCmd + "' !";
      return nullptr;
   }
   CParserAdapter parserAdapter(*this, ++nIdx, ctx);
   CMathItem* pItem = pBuilder->BuildFromParser(sCmd.c_str(), &parserAdapter);
   nIdx = parserAdapter.CurrentTokenIdx();  // Sync position
   return pItem;
}
