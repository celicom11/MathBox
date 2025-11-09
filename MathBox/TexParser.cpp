#include "stdafx.h"
#include "TexParser.h"
#include "Tokenizer.h"
#include "HBoxItem.h"
#include "LMFontManager.h"
#include "WordItemBuilder.h"
#include "IndexedBuilder.h"
#include "VBoxBuilder.h"
#include "ParserAdapter.h"
//extern CLMFontManager g_LMFManager;
namespace { //static helpers
   bool _isTextCommand(const string& sCmd) {
      // text mode + font commands: \text,\textit,etc.
      PCSTR szCmd = sCmd.c_str();
      if (szCmd[0] != '\\')
         return false;
      szCmd++; //skip '\'
      STextFontStyle tfStyle; //dummy
      return CLMFontManager::_GetTextFontStyle(szCmd, tfStyle);
   }
   bool _isMathFontCommand(const string& sCmd) {
      // math mode font commands: \mathnormal,\mathit,etc.
      PCSTR szCmd = sCmd.c_str();
      if (szCmd[0] != '\\')
         return false;
      szCmd++; //skip '\'
      SMathFontStyle mfStyle; //dummy
      return CLMFontManager::_GetMathFontStyle(szCmd, mfStyle);
   }
   bool _isMathStyleCommand(const string& sCmd) {
      // \displaystyle, \textstyle, etc
      return (sCmd == "\\displaystyle" || sCmd == "\\textstyle" ||
         sCmd == "\\scriptstyle" || sCmd == "\\scriptscriptstyle");
   }
   bool _isScalingCommand(const string& sCmd) {
      // \fontsize,\scalefnt,\relscale,  etc
      return (sCmd == "\\fontsize" || sCmd == "\\scalefnt" || sCmd == "\\relscale");
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
      // \\, \newline, \break
      return (sCmd == "\\\\" || sCmd == "\\newline" || sCmd == "\\break");
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
      return false;
   }
   // { number } { "pt" }
   float _GetSizePts(const string& sNum, const string& sUnits) {
      if (sUnits != "pt")
         return 0.0f;//todo: parse/convert units to Pt
      float fSize = 0.0f;
      try {
         fSize = stof(sNum);
      }
      catch (...) {
         return 0.0f;
      }
      return fSize;
   }
}
// CTexParser
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
   const STexToken& tkItem = m_vTokens[nIdx];
   CMathItem* pItem = nullptr;
   m_Error.eStage = epsBUILDING;
   m_Error.nPosition = tkItem.nPos;
   if (tkItem.nTkIdxEnd > 0)
      pItem = ProcessGroup(nIdx, ctx);
   else {
      switch (tkItem.nType) {
      case ettALNUM:     pItem = ProcessAlnum_(nIdx, ctx); break;
      case ettNonALPHA:  pItem = ProcessNonAlnum_(nIdx, ctx); break;
      case ettCOMMAND: { // command or symbol!
         string sCmd = m_pTokenizer->TokenText(tkItem);
         if (_isMathStyleCommand(sCmd) || _isFontSizeCommand(sCmd))
            return nullptr; // not an Item Token!
         if (_isTextCommand(sCmd))
            pItem = ProcessTextCmd_(nIdx, ctx);
         else if (_isMathFontCommand(sCmd))
            pItem = ProcessMathFontCmd_(nIdx, ctx);
         else if (_isScalingCommand(sCmd))
            pItem = ProcessScaleCmd_(nIdx, ctx);
         else
            pItem = ProcessItemBuilderCmd_(nIdx, ctx);
         }
         break;
      case ettSUBS:
      case ettSUPERS:
         // we can be here only if there were no base item!
         pItem = BuildGenerilizedFraction_(nIdx, ctx); //error if failed!
         break;
      default:;
         // do not process non-item tokens at this level
      }
   }
   if (!ctx.bInCmdArg && !ctx.bInSubscript && !ctx.bInSuperscript) {
      //lookahead subscript/superscript building
      while (pItem && m_Error.sError.empty()) {
         CMathItem* pNewAtom = TryAddSubSuperscript_(pItem, nIdx, ctx);
         if (pNewAtom == pItem)
            break; //no sub/superscript added
         pItem = pNewAtom; //update
      }
   }
   return pItem;
}
CMathItem* CTexParser::BuildGenerilizedFraction_(IN OUT int& nIdx, const SParserContext& ctx) {
   _ASSERT_RET(m_vTokens[nIdx].nType == ettSUBS || m_vTokens[nIdx].nType == ettSUPERS, nullptr); //snbh!
   if (ctx.bInSubscript || ctx.bInSuperscript) {
      // we are already in sub/superscript context - do not process here!
      m_Error.sError = "Double subscript/superscript is not allowed!";
      return nullptr;
   }

   bool bSubscriptFirst = (m_vTokens[nIdx].nType == ettSUBS);
   ++nIdx; //move past _ or ^
   CMathItem* pNum = nullptr, *pDenom = nullptr;
   SParserContext ctxSS(ctx);
   if(bSubscriptFirst) {
      ctxSS.SetInSubscript();
      pDenom = ProcessItemToken(nIdx, ctxSS);
      if (!pDenom) {
         if(m_Error.sError.empty())
            m_Error.sError = "Orphan subscript '_'";
         return nullptr;
      }
      if(nIdx >= m_vTokens.size() || m_vTokens[nIdx].nType != ettSUPERS) {
         m_Error.sError = "Single subscript '_' is not supported";
         return nullptr;
      }
      ++nIdx; //move past ^
      ctxSS = ctx; //reset
      ctxSS.SetInSuperscript();
      pNum = ProcessItemToken(nIdx, ctxSS);
      if (!pNum) {
         if (m_Error.sError.empty())
            m_Error.sError = "Missed item after superscript '^'";
         return nullptr;
      }
   }
   else {
      ctxSS.SetInSuperscript();
      pNum = ProcessItemToken(nIdx, ctxSS);
      if (!pNum) {
         if (m_Error.sError.empty())
            m_Error.sError = "Orphan superscript '^'";
         return nullptr;
      }
      if (nIdx >= m_vTokens.size() || m_vTokens[nIdx].nType != ettSUBS) {
         m_Error.sError = "Single superscript '^' is not supported";
         return nullptr;
      }
      ++nIdx; //move past _
      ctxSS = ctx; //reset
      ctxSS.SetInSubscript();
      pDenom = ProcessItemToken(nIdx, ctxSS);
      if (!pDenom) {
         if (m_Error.sError.empty())
            m_Error.sError = "Missed item after subscript '_'";
         return nullptr;
      }
   }
   _ASSERT(pNum && pDenom); //snbh!
   //build CIndexedItem
   return CVBoxBuilder::_BuildGenFraction(ctx.currentStyle, ctx.fUserScale, pNum, pDenom); //never fails?
}
//Lookahead for sub/superscript after creating an atom
CMathItem* CTexParser::TryAddSubSuperscript_(CMathItem* pAtom, IN OUT int& nIdx, const SParserContext& ctx) {
   _ASSERT_RET(pAtom, pAtom); //snbh!
   if (ctx.bInSubscript || ctx.bInSuperscript)
      // we are already in sub/superscript context - do not process here!
      return pAtom; //no error
   if(nIdx >= m_vTokens.size() || 
      (m_vTokens[nIdx].nType != ettSUBS && m_vTokens[nIdx].nType != ettSUPERS))
      return pAtom; //no sub/superscript
   CMathItem* pSub = nullptr;
   CMathItem* pSuper = nullptr;
   if(m_vTokens[nIdx].nType == ettSUBS) {
      ++nIdx; //move past _
      SParserContext ctxSub(ctx);
      ctxSub.SetInSubscript();
      pSub = ProcessItemToken(nIdx, ctxSub);
      if (!pSub) {
         if (m_Error.sError.empty())
            m_Error.sError = "Orphan subscript '_'";
         return nullptr;
      }
      //check for superscript after subscript
      if (m_vTokens[nIdx].nType == ettSUPERS) {
         ++nIdx; //move past ^
         SParserContext ctxSuper(ctx);
         ctxSuper.SetInSuperscript();
         pSuper = ProcessItemToken(nIdx, ctxSuper);
         if (!pSuper) {
            if (m_Error.sError.empty())
               m_Error.sError = "Orphan superscript '^'";
            delete pSub;
            return nullptr;
         }
      }
   }
   else { //superscript first
      ++nIdx; //move past ^
      SParserContext ctxSuper(ctx);
      ctxSuper.SetInSuperscript();
      pSuper = ProcessItemToken(nIdx, ctxSuper);
      if (!pSuper) {
         if (m_Error.sError.empty())
            m_Error.sError = "Orphan superscript '^'";
         return nullptr;
      }
      //check for subscript after superscript
      if (m_vTokens[nIdx].nType == ettSUBS) {
         ++nIdx; //move past _
         SParserContext ctxSub(ctx);
         ctxSub.SetInSubscript();
         pSub = ProcessItemToken(nIdx, ctxSub);
         if (!pSub) {
            if (m_Error.sError.empty())
               m_Error.sError = "Orphan subscript '_'";
            delete pSuper;
            return nullptr;
         }
      }
   }
   _ASSERT(pSub || pSuper); //snbh!
   //build CIndexedItem
   return CIndexedBuilder::BuildIndexed(ctx.currentStyle, ctx.fUserScale, pAtom, pSuper, pSub);
}

CMathItem* CTexParser::ProcessGroup(IN OUT int& nIdx, const SParserContext& ctx) {
   int nIdxStart, nIdxEnd;
   bool bCanBeEmpty = true;
   SParserContext ctxG(ctx);
   if (nIdx == -1) {
      //root group
      nIdxStart = 0;
      nIdx = nIdxEnd = (int)m_vTokens.size();
   }
   else {
      const STexToken& tkOpen = m_vTokens[nIdx];
      _ASSERT_RET((int)tkOpen.nTkIdxEnd > nIdx && tkOpen.nTkIdxEnd <= m_vTokens.size(), nullptr);//snbh!
      if(!OnGroupOpen_(tkOpen, ctxG, bCanBeEmpty))
         return nullptr; //error in group open token
      nIdxStart = nIdx + 1;//move past the open token
      nIdxEnd = tkOpen.nTkIdxEnd - 1; //exclude close token
      nIdx = tkOpen.nTkIdxEnd;
   }
   if (nIdxEnd <= nIdxStart) {
      if(!bCanBeEmpty) {
         m_Error.eStage = epsBUILDING;
         m_Error.nPosition = m_vTokens[nIdxStart].nPos;
         m_Error.sError = "Empty group is not allowed here!";
      }
      //else, empty group is OK 
      return nullptr;
   }
   _ASSERT_RET(nIdxStart >=0 && nIdxEnd > nIdxStart && nIdxEnd <= m_vTokens.size() , nullptr);
   vector<vector<CMathItem*>> vvLines(1); //one empty line
   for (int nIdxG = nIdxStart; nIdxG < nIdxEnd;) {
      CMathItem* pItem = nullptr;
      bool bNewLine = false;
      const STexToken& tkCurrent = m_vTokens[nIdxG];
      m_Error.eStage = epsBUILDING;
      m_Error.nPosition = tkCurrent.nPos;
      pItem = ProcessItemToken(nIdxG, ctxG);
      if(!m_Error.sError.empty())
         return nullptr; //error in sub-group
      if(!pItem) {
         switch (tkCurrent.nType) {
         case ettCOMMAND: { // command or symbol!
            string sCmd = m_pTokenizer->TokenText(tkCurrent);
            if (_isMathStyleCommand(sCmd))
               ProcessMathStyleCmd_(nIdxG, ctxG);
            else if (_isFontSizeCommand(sCmd))
               ProcessFontSizeCmd_(nIdxG, ctxG);
            else if (_isNewlineCommand(sCmd)) {
               // like in \begin{align*} env.? : TODO: review
               bNewLine = true;
               ++nIdxG; //next token
            }
            else
               m_Error.sError = "Unexpected command in ProcessGroup!";
         }
            break;
         case ettCOMMENT:   ++nIdxG;  break;    // ignore comments at this level
         case ettAMP:       _ASSERT(0);         // TODO: handle tabular alignment
            break;
         default:
            m_Error.sError = "Unexpected token in ProcessGroup!";
         }
      }
      if (!m_Error.sError.empty()) {
         // TODO: cleanup lines!
         return nullptr;//error in sub-group
      }
      if (bNewLine)
         vvLines.emplace_back();
      if (pItem)
         vvLines.back().push_back(pItem);
   } //main for

   CMathItem* pRet = nullptr;
   // check group items
   if (vvLines.size() == 1) {
      //single line
      int nItemsCount = 0;
      for(CMathItem* pI:vvLines[0]) {
         nItemsCount += (pI ? 1 : 0);
      }
      if (nItemsCount == 0) {
         if (!bCanBeEmpty) {
            m_Error.eStage = epsBUILDING;
            m_Error.nPosition = m_vTokens[nIdxStart].nPos;
            m_Error.sError = "Empty group is not allowed here!";
         }
         return nullptr;
      }
      else if (nItemsCount == 1) {
         //return single MathItem!
         for (CMathItem* pI : vvLines[0]) {
            if (pI) {
               pRet = pI;
               break;
            }
         }
      }
      else
         pRet = PackLines_(vvLines, ctx);
   }
   else
      pRet = PackLines_(vvLines, ctx);
   if (ctxG.bInLeftRight) {
      //process dyynamic-sized open/close delimiters (todo: middle!)
      _ASSERT(0); //TODO!
   }
   return pRet;
}
// HELPERS
bool CTexParser::OnGroupOpen_(const STexToken& tkOpen, IN OUT SParserContext& ctxG, OUT bool& bCanBeEmpty) {
   if (!ctxG.bTextMode && (tkOpen.nType == ett$ || tkOpen.nType == ett$$)) {
      m_Error.eStage = epsBUILDING;
      m_Error.nPosition = tkOpen.nPos;
      m_Error.sError = "Inner $..$/$$...$$ are not allowed in math mode!";
      return false;
   }
   // reset external context flags
   ctxG.bInLeftRight = ctxG.bInSubscript = ctxG.bInSuperscript = ctxG.bInCmdArg = false;
   switch (tkOpen.nType) {
   case ettSB_OPEN: bCanBeEmpty = false; break;
   case ettFB_OPEN:bCanBeEmpty = true; break;
   case ett$$:
      ctxG.currentStyle = etsDisplay;
      ctxG.bTextMode = false; //math mode
      bCanBeEmpty = false;
      break;
   case ett$:
      ctxG.currentStyle = etsText;
      ctxG.bTextMode = false; //math mode
      bCanBeEmpty = false;
      break;
   case ettCOMMAND: {
      string sCmd = m_pTokenizer->TokenText(tkOpen);
      if (sCmd == "\\[") {
         if (!ctxG.bTextMode) {
            m_Error.eStage = epsBUILDING;
            m_Error.nPosition = tkOpen.nPos;
            m_Error.sError = "Inner \\[...\\] are not allowed in math mode!";
            return false;
         }
         ctxG.bTextMode = false; //math mode
         bCanBeEmpty = true;
      }
      else if (sCmd == "\\left") {
         if (ctxG.bTextMode) {
            m_Error.eStage = epsBUILDING;
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
CMathItem* CTexParser::PackLines_(const vector<vector<CMathItem*>>& vvLines, const SParserContext& ctx) {
   if (vvLines.size() == 1) {
      CHBoxItem* pHBox = new CHBoxItem(ctx.currentStyle);
      for (CMathItem* pI : vvLines[0]) {
         if (pI)
            pHBox->AddItem(pI);
      }
      return pHBox;
   }
   //else 
   CContainerItem* pRet = new CContainerItem(eacVBOX, ctx.currentStyle);
   for(const vector<CMathItem*>& vLine : vvLines) {
      CHBoxItem* pHBox = new CHBoxItem(ctx.currentStyle);
      for (CMathItem* pI : vLine) {
         if (pI)
            pHBox->AddItem(pI);
      }
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
            m_vTokens[vGroupStarts.back()].nTkIdxEnd = nIdx + 1;
            vGroupStarts.pop_back();
         }
         else {
            //check if we already have opening $/$$ token:
            // "${$"    means '{'  is not closed!
            for(int nGSIdx: vGroupStarts) {
               STexToken& tknGS = m_vTokens[nGSIdx];
               if (tknGS.nType == tkn.nType) {
                  m_Error.nPosition = m_vTokens[vGroupStarts.back()].nPos;
                  m_Error.sError = "Unclosed group '" + TokenText_(vGroupStarts.back()) +"'";
                  return false;
               }
            }
            //else , we are OK
            vGroupStarts.push_back(nIdx);
         }
         break;
      case ettCOMMAND: {
         string sCmd = m_pTokenizer->TokenText(tkn);
         if (sCmd == "\\left" || sCmd == "\\[")
            vGroupStarts.push_back(nIdx);
         else if (sCmd == "\\right" || sCmd == "\\]") {
            if (vGroupStarts.empty() || _IsOpenClose(TokenText_(vGroupStarts.back()), sCmd)) {
               m_Error.nPosition = tkn.nPos;
               m_Error.sError = "Unmatched '" + sCmd + "' !";
               return false;
            }
            int nGroupEnd = nIdx + 1;
            if (sCmd == "\\right")
               ++nGroupEnd;//include \right's argument
            m_vTokens[vGroupStarts.back()].nTkIdxEnd = nGroupEnd;
            vGroupStarts.pop_back();
         }
      }
                     break;
      case ettFB_CLOSE:
      case ettSB_CLOSE: {
         string sClosing = (tkn.nType == ettFB_CLOSE) ? "}" : "]";
         if (vGroupStarts.empty() || !_IsOpenClose(TokenText_(vGroupStarts.back()), sClosing)) {
            m_Error.nPosition = tkn.nPos;
            m_Error.sError = "Unmatched '" + sClosing + "' !";
            return false;
         }
         m_vTokens[vGroupStarts.back()].nTkIdxEnd = nIdx + 1;
         vGroupStarts.pop_back();
      }
                      break;
      }
   }
   if(!vGroupStarts.empty()) {
      const STexToken& tkUnmatched = m_vTokens[vGroupStarts.back()];
      m_Error.nPosition = tkUnmatched.nPos;
      m_Error.sError = "Unclosed opening token '" + TokenText_(vGroupStarts.back()) + "' !";
      return false;
   }
   return true;
}
string CTexParser::TokenText_(int nIdx) const {
   const STexToken* pToken = (nIdx >= 0 && nIdx < (int)m_vTokens.size()) ? &m_vTokens[nIdx] : nullptr;
   return pToken ? m_pTokenizer->TokenText(*pToken) : string();
}

// CONCRETE TOKEN PROCESSORS
bool CTexParser::ProcessMathStyleCmd_(IN OUT int& nIdx, IN OUT SParserContext& ctx) {
   if(ctx.bTextMode) {
      m_Error.eStage = epsBUILDING;
      m_Error.nPosition = m_vTokens[nIdx].nPos;
      m_Error.sError = "Math style commands can only be used in math mode!";
      return false;
   }
   string sCmd = TokenText_(nIdx);
   ++nIdx; // next token
   if (sCmd == "\\displaystyle")
      ctx.currentStyle = etsDisplay;
   else if (sCmd == "\\textstyle")
      ctx.currentStyle = etsText;
   else if (sCmd == "\\scriptstyle")
      ctx.currentStyle = etsScript;
   else if (sCmd == "\\scriptstyle")
      ctx.currentStyle = etsScriptScript;
   else {
      _ASSERT(0); //snbh!
      m_Error.eStage = epsBUILDING;
      m_Error.nPosition = m_vTokens[nIdx - 1].nPos;
      m_Error.sError = "Unknown math style command: " + sCmd;
      return false;
   }
   return true;
}
bool CTexParser::ProcessFontSizeCmd_(IN OUT int& nIdx, IN OUT SParserContext& ctx) {
   string sCmd = TokenText_(nIdx);
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
   string sText = m_pTokenizer->TokenText(m_vTokens[nIdx]);
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
         pRet = pHBox;
      }
      else //single-char word
         pRet = CWordItemBuilder::BuildMathWord(ctx.sFontCmd, sText, false, ctx.currentStyle, ctx.fUserScale);
   }
   if(!pRet) {
      m_Error.eStage = epsBUILDING;
      m_Error.nPosition = m_vTokens[nIdx].nPos;
      m_Error.sError = "Failed to build alphanumeric item!";
   }
   return pRet;
}
CMathItem* CTexParser::ProcessNonAlnum_(IN OUT int& nIdx, const SParserContext& ctx) {
   CMathItem* pRet = nullptr;
   string sText = m_pTokenizer->TokenText(m_vTokens[nIdx]);
   nIdx = nIdx + 1; //next token
   _ASSERT(sText.size() == 1);
   if (ctx.bTextMode)
      pRet = CWordItemBuilder::BuildText(ctx.sFontCmd, sText, ctx.currentStyle, ctx.fUserScale);
   else
      pRet = CWordItemBuilder::BuildMathWord(ctx.sFontCmd, sText, false, ctx.currentStyle, ctx.fUserScale);
   if (!pRet) {
      m_Error.eStage = epsBUILDING;
      m_Error.nPosition = m_vTokens[nIdx].nPos;
      m_Error.sError = "Failed to process non-alphanumeric character!";
   }
   return pRet;
}
CMathItem* CTexParser::ProcessTextCmd_(IN OUT int& nIdx, const SParserContext& ctx) {
   //we require an argument in {}
   _ASSERT_RET(nIdx + 1 < m_vTokens.size(), nullptr);//snbh!
   if(m_vTokens[nIdx + 1].nTkIdxEnd == 0 || m_vTokens[nIdx + 1].nType != ettFB_OPEN) {
      m_Error.eStage = epsBUILDING;
      m_Error.nPosition = m_vTokens[nIdx].nPos;
      m_Error.sError = "Text command requires argument in curly braces {}!";
      return nullptr;
   }
   SParserContext ctxText(ctx);
   ctxText.bTextMode = true; //text mode
   ctxText.sFontCmd = m_pTokenizer->TokenText(m_vTokens[nIdx]);
   CMathItem* pArg = ProcessGroup(++nIdx, ctxText); //move to argument group
   if(!pArg) {
      //error in argument processing
      if (m_Error.sError.empty()) {
         m_Error.eStage = epsBUILDING;
         m_Error.nPosition = m_vTokens[nIdx].nPos;
         m_Error.sError = "Failed to process argument of text command!";
      }
      return nullptr;
   }
   return pArg;
}
CMathItem* CTexParser::ProcessMathFontCmd_(IN OUT int& nIdx, const SParserContext& ctx) {
   //we require an argument in {}
   _ASSERT_RET(nIdx + 1 < m_vTokens.size(), nullptr);//snbh!
   if (m_vTokens[nIdx + 1].nTkIdxEnd == 0 || m_vTokens[nIdx + 1].nType != ettFB_OPEN) {
      m_Error.eStage = epsBUILDING;
      m_Error.nPosition = m_vTokens[nIdx].nPos;
      m_Error.sError = "Text command requires argument in curly braces {}!";
      return nullptr;
   }
   SParserContext ctxMath(ctx);
   ctxMath.bTextMode = false; //math mode
   ctxMath.sFontCmd = m_pTokenizer->TokenText(m_vTokens[nIdx]);
   CMathItem* pArg = ProcessGroup(++nIdx, ctxMath); //move to argument group
   if (!pArg) {
      //error in argument processing
      if (m_Error.sError.empty()) {
         m_Error.eStage = epsBUILDING;
         m_Error.nPosition = m_vTokens[nIdx].nPos;
         m_Error.sError = "Failed to process argument of text command!";
      }
      return nullptr;
   }
   return pArg;
}
CMathItem* CTexParser::ProcessScaleCmd_(IN OUT int& nIdx, const SParserContext& ctx) {
   // \fontsize{size pt}{h_skip}{arg}\selectfont 
   // \scalefnt{factor}{arg}
   // \relscale{factor}{arg}
   string sCmd = m_pTokenizer->TokenText(m_vTokens[nIdx]);
   //we require 2+ arguments in {}: size and argument/text
   _ASSERT_RET(nIdx + 1 < m_vTokens.size(), nullptr);//snbh!
   if (m_vTokens[nIdx + 1].nTkIdxEnd == 0 || m_vTokens[nIdx + 1].nType != ettFB_OPEN) {
      m_Error.eStage = epsBUILDING;
      m_Error.nPosition = m_vTokens[nIdx].nPos;
      m_Error.sError = "Scaling command '" + sCmd + "' requires size argument in curly braces{}!";
      return nullptr;
   }
   int nIdxSizeArg = nIdx + 2, nIdxTextArg = (sCmd == "\\fontsize" ? nIdx + 4 : nIdx + 3);
   float fScale = 1.0f;
   //process size argument
   if (sCmd == "\\fontsize") {
      // expect two AlphaNum tokens: {number}{"pt"}
      float fSizePts = _GetSizePts(TokenText_(nIdxSizeArg), TokenText_(nIdxSizeArg + 1));
      if (fSizePts <= 0.0f) {
         m_Error.eStage = epsBUILDING;
         m_Error.nPosition = m_vTokens[nIdxSizeArg].nPos;
         m_Error.sError = "Failed to process font size!";
         return nullptr;
      }
      fScale = fSizePts / m_fDocFontSizePts;
   }
   else {
      // \scalefnt or \relscale: expect one AlphaNum token: {number}
      string sFactor = TokenText_(nIdxSizeArg);
      try {
         fScale = stof(sFactor);
      }
      catch (...) {
         m_Error.eStage = epsBUILDING;
         m_Error.nPosition = m_vTokens[nIdxSizeArg].nPos;
         m_Error.sError = "Failed to process scaling factor!";
         return nullptr;
      }
   }
   //process argument
   _ASSERT_RET(nIdxTextArg < m_vTokens.size(), nullptr);//snbh!
   if (m_vTokens[nIdxTextArg].nTkIdxEnd == 0 || m_vTokens[nIdxTextArg].nType != ettFB_OPEN) {
      m_Error.eStage = epsBUILDING;
      m_Error.nPosition = m_vTokens[nIdxTextArg].nPos;
      m_Error.sError = "Scaling command'" + sCmd + "' requires argument in curly braces {}!";
      return nullptr;
   }
   SParserContext ctxScaled(ctx);   // mode can be any
   ctxScaled.fUserScale *= fScale;  // apply scaling
   CMathItem* pArg = ProcessGroup(nIdxTextArg, ctxScaled); //process argument with the new scale
   if (!pArg) {
      //error in argument processing
      if (m_Error.sError.empty()) {
         m_Error.eStage = epsBUILDING;
         m_Error.nPosition = m_vTokens[nIdx].nPos;
         m_Error.sError = "Failed to process argument of scaling command!";
      }
      return nullptr;
   }
   nIdx = nIdxTextArg;
   if (sCmd == "\\fontsize") {
      // skip \selectfont post command!
      if (nIdx < m_vTokens.size() && m_vTokens[nIdx].nType == ettCOMMAND && TokenText_(nIdx) == "\\selectfont")
         ++nIdx;
   }
   return pArg;
}
CMathItem* CTexParser::ProcessItemBuilderCmd_(IN OUT int& nIdx, const SParserContext& ctx) {
   string sCmd = TokenText_(nIdx);
   IMathItemBuilder* pBuilder = nullptr;
   for (IMathItemBuilder* pBldr : m_vBuilders) {
      if (!pBldr->CanTakeCommand(sCmd.c_str()))
         continue;
      pBuilder = pBldr;
      break;
   }
   if (!pBuilder) {
      m_Error.eStage = epsBUILDING;
      m_Error.nPosition = m_vTokens[nIdx].nPos;
      m_Error.sError = "Unkown command '"+ sCmd + "' !";
      return nullptr;
   }
   CParserAdapter parserAdapter(*this, ++nIdx, ctx);
   CMathItem* pItem = pBuilder->BuildFromParser(sCmd.c_str(), &parserAdapter);
   nIdx = parserAdapter.CurrentTokenIdx();  // Sync position
   return pItem;
}

