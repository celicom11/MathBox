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
   return ProcessGroup_(nStart, ctx);
}
// TOKEN PROCESSING
CMathItem* CTexParser::ProcessGroup_(IN OUT int& nIdx, const SParserContext& ctx) {
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
   CMathItem* pSuper = nullptr;
   CMathItem* pSub = nullptr;
   SParserContext ctxGCur; //backup of the current context
   bool bInSubs = false, bInSupers = false; // additional context
   vector<vector<CMathItem*>> vvLines(1); //one empty line
   for (int nIdxG = nIdxStart; nIdxG < nIdxEnd;) {
      CMathItem* pItem = nullptr;
      bool bNewLine = false;
      const STexToken& tkCurrent = m_vTokens[nIdxG];
      m_Error.eStage = epsBUILDING;
      m_Error.nPosition = tkCurrent.nPos;
      if (tkCurrent.nTkIdxEnd > 0)
         pItem = ProcessGroup_(nIdxG, ctxG);
      else {
         switch (tkCurrent.nType) {
         case ettALNUM:     pItem = ProcessAlnum_(nIdxG, ctxG); break;
         case ettNonALPHA:  pItem = ProcessNonAlnum_(nIdxG, ctxG); break;
         case ettCOMMAND: { // command or symbol!
            string sCmd = m_pTokenizer->TokenText(tkCurrent);
            if (_isMathStyleCommand(sCmd))
               ProcessMathStyleCmd_(nIdxG, ctxG);
            else if (_isFontSizeCommand(sCmd))
               ProcessFontSizeCmd_(nIdxG, ctxG);
            else if (_isTextCommand(sCmd))
               pItem = ProcessTextCmd_(nIdxG, ctxG);
            else if (_isMathFontCommand(sCmd))
               pItem = ProcessMathFontCmd_(nIdxG, ctxG);
            else if (_isScalingCommand(sCmd))
               pItem = ProcessScaleCmd_(nIdxG, ctxG);
            else if (_isNewlineCommand(sCmd)) {
               // like in \begin{align*} env.? : TODO: review
               bNewLine = true;
               ++nIdxG; //next token
            }
            else
               pItem = ProcessItemBuilderCmd_(nIdxG, ctxG);
         }
         case ettCOMMENT:   ++nIdxG;  break;    // ignore comments at this level
         case ettAMP:       _ASSERT(0);         // TODO: handle tabular alignment
            break;
         case ettSUBS:// subscript
            if(bInSubs) {
               m_Error.sError = "Double subscript is not allowed!";
               break;
            }
            if(bInSupers) {
               m_Error.sError = "Subscript after superscript is not allowed!";
               break;
            }
            ctxGCur = ctxG; //backup current context
            ctxG.currentStyle.Decrease();
            if (ctxG.currentStyle.Style() == etsText)
               ctxG.currentStyle.Decrease();
            ctxG.currentStyle.SetCramped(); //subscripts are cramped
            bInSubs = true;
            ++nIdxG;    // move past _ token
            continue;   // next token
            break;
         case ettSUPERS:// superscript
            if (bInSupers) {
               m_Error.sError = "Double supescript is not allowed!";
               break;
            }
            if (bInSubs) {
               m_Error.sError = "Superscript after subscript is not allowed!";
               break;
            }
            ctxGCur = ctxG; //backup current context
            ctxG.currentStyle.Decrease();
            if (ctxG.currentStyle.Style() == etsText)
               ctxG.currentStyle.Decrease();
            bInSupers = true;
            ++nIdxG; //move past ^ token
            continue; //next token
            break;
         default:
            m_Error.sError = "Unexpected token in ProcessGroup_!";
         }
      }
      if (!m_Error.sError.empty()) {
         // TODO: cleanup lines!
         return nullptr;//error in sub-group
      }
      if (bNewLine) {
         if (bInSubs || bInSupers) {
            m_Error.sError = "Newline is not allowed after subscript/superscript!";
            return nullptr;
         }
         vvLines.emplace_back();
      }
      // check for pending sub/superscript
      if(pSub || pSuper) {
         //we must build IndexedItem if 
         if ((pSub && pSuper) ||          //both
            (!bInSubs && !bInSupers) ||   //next token is not _ or ^
            (bInSubs && pSub) ||          //X_Y_Z== {X_Y}_Z
            (bInSupers && pSuper)) {      //X^Y^Z== {X^Y}^Z
            CMathItem* pAtomBase = vvLines.back().empty() ? nullptr : vvLines.back().back();
            if (pAtomBase) {
               vvLines.back().pop_back(); //remove last item
               pAtomBase = CIndexedBuilder::BuildIndexed(ctxG.currentStyle, ctxG.fUserScale, pAtomBase, pSuper, pSub);
            }
            else {
               //no base atom!
               if(!pSuper || !pSub) {
                  m_Error.sError = "Alone subscript/superscript without base is not allowed!";
                  return nullptr;
               }
               pAtomBase = CVBoxBuilder::_BuildGenFraction(ctxG.currentStyle, ctxG.fUserScale, pSuper, pSub);
            }
            if(!pAtomBase) {
               m_Error.sError = "Error during building Indexed"; //snbh!
               return nullptr;
            }
            pSuper = pSub = nullptr; //reset
            vvLines.back().push_back(pAtomBase);
         }
      }
      if(bInSubs) {
         if (!pItem) {
            m_Error.sError = "Missing subscript argument!";
            return nullptr;
         }
         pSub = pItem;
         bInSubs = false;
         ctxG = ctxGCur;   // restore context
         continue;         // next token
      }
      if(bInSupers) {
         if (!pItem) {
            m_Error.sError = "Missing superscript argument!";
            return nullptr;
         }
         pSuper = pItem;
         bInSupers = false;
         ctxG = ctxGCur; //restore context
         continue; //next token
      }
      //else
      if (pItem)
         vvLines.back().push_back(pItem);
   } //main for

   // EPILOGUE: build Indexed,Pack lines, process \left...\right
   //check for pending sub/superscript
   if(bInSubs || bInSupers) {
      m_Error.sError = "Missing subscript/superscript argument at the end of group!";
      return nullptr;
   }
   if (pSub || pSuper) {
      CMathItem* pAtomBase = vvLines.back().empty() ? nullptr : vvLines.back().back();
      if (pAtomBase) {
         vvLines.back().pop_back(); //remove last item
         pAtomBase = CIndexedBuilder::BuildIndexed(ctxG.currentStyle, ctxG.fUserScale, pAtomBase, pSuper, pSub);
      }
      else {
         //no base atom!
         if (!pSuper || !pSub) {
            m_Error.sError = "Alone subscript/superscript without base is not allowed!";
            return nullptr;
         }
         pAtomBase = CVBoxBuilder::_BuildGenFraction(ctxG.currentStyle, ctxG.fUserScale, pSuper, pSub);
      }
      if (!pAtomBase) {
         m_Error.sError = "Error during building Indexed"; //snbh!
         return nullptr;
      }
      //return pAtomBase back!
      vvLines.back().push_back(pAtomBase);
   }
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
         else
            vGroupStarts.push_back(nIdx);
         break;
      case ettCOMMAND: {
         string sCmd = m_pTokenizer->TokenText(tkn);
         if (sCmd == "\\left" || sCmd == "\\[")
            vGroupStarts.push_back(nIdx);
         else if (sCmd == "\\right" || sCmd == "\\]") {
            if (vGroupStarts.empty() || _IsOpenClose(TokenText_(vGroupStarts.back()), sCmd)) {
               m_Error.eStage = epsGROUPING;
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
            m_Error.eStage = epsGROUPING;
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
      m_Error.eStage = epsGROUPING;
      m_Error.nPosition = tkUnmatched.nPos;
      m_Error.sError = "Unmatched opening token '" + TokenText_(vGroupStarts.back()) + "' !";
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
   CMathItem* pArg = ProcessGroup_(++nIdx, ctxText); //move to argument group
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
   CMathItem* pArg = ProcessGroup_(++nIdx, ctxMath); //move to argument group
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
   CMathItem* pArg = ProcessGroup_(nIdxTextArg, ctxScaled); //process argument with the new scale
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

