#include "stdafx.h"
#include "TexParser.h"
#include "Tokenizer.h"
#include "MathModeProcessor.h"
#include "TextModeProcessor.h"
#include "EnvHelper.h"


namespace { //static helpers
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
}
// CTexParser
CTexParser::CTexParser(IDocParams& doc):
   m_Doc(doc), m_MacroProcessor(*this) {
   m_pMathProcessor = new CMathModeProcessor(*this);
   m_pTextProcessor = new CTextModeProcessor(*this);
}
CTexParser::~CTexParser() {
   delete m_pTokenizer;
   delete m_pTextProcessor;
   delete m_pMathProcessor;
}
// In TexParser.cpp
void CTexParser::SetError(int nTkIdx, const string& sError) {
   const STexToken* pToken = GetToken(nTkIdx);
   m_Error.sError = sError;
   m_Error.nStartPos = 0;
   m_Error.nEndPos = 0;

   if (pToken) {
      m_Error.nStartPos = pToken->nPos;
      m_Error.nEndPos = pToken->nPos + pToken->nLen;
   }

   if (!pToken || pToken->nRefIdx == 0)
      return; // Simple error - no macro expansion involved

   // Build detailed error for macro-expanded tokens
   string sDetailedError;

   if (m_Error.eStage <= epsBUILDINGMACROS) {
      // Building macros stage - tokens are in CMacroProcessor
      string sMacroFile;
      int nPosInFile = 0;
      
      if (m_MacrosMgr.getTokenSource(*pToken, nPosInFile, sMacroFile)) {
         sDetailedError += "\n  from macro file: " + sMacroFile;
      }
   }
   else {
      // Main document parsing - show the actual expanded text
      const int nRefIdx = pToken->nRefIdx;
      
      // Map error position from expanded tokens to original user input
      if (!m_vTokensOrig.empty() && nRefIdx > 0 && nRefIdx <= (int)m_vTokensOrig.size()) {
         const STexToken& tkOrig = m_vTokensOrig[nRefIdx - 1];
         m_Error.nStartPos = tkOrig.nPos;
         m_Error.nEndPos = tkOrig.nPos + tkOrig.nLen;
      }
      
      // Find first (TkStart) and last (TkEnd) tokens with this nRefIdx in m_vTokens
      int nTkStart = -1;
      int nTkEnd = -1;
      
      for (int i = 0; i < (int)m_vTokens.size(); ++i) {
         if (m_vTokens[i].nRefIdx == nRefIdx) {
            if (nTkStart == -1) nTkStart = i;
            nTkEnd = i;
         }
      }
      
      // Build expanded text from ALL tokens between TkStart and TkEnd (inclusive)
      // This includes both macro-expanded tokens (nRefIdx > 0) and 
      // user-input tokens (nRefIdx == 0) like "1^2" in the example
      if (nTkStart >= 0 && nTkEnd >= nTkStart) {
         string sExpandedText;
         for (int i = nTkStart; i <= nTkEnd; ++i) {
            sExpandedText += TokenText(i);
         }
         
         if (!sExpandedText.empty()) {
            sDetailedError += "\n  Expanded macro: " + sExpandedText;
         }
      }
      
      // Collect unique source files for all tokens with this nRefIdx
      set<string> setFiles;
      for (const STexToken& tk : m_vTokens) {
         if (tk.nRefIdx == nRefIdx) {
            string sFile;
            int nPos;
            if (m_MacrosMgr.getTokenSource(tk, nPos, sFile)) {
               setFiles.insert(sFile);
            }
         }
      }
      
      // Format macro file(s) line
      if (!setFiles.empty()) {
         sDetailedError += "\n  from macro file(s): ";
         bool bFirst = true;
         for (const string& sFile : setFiles) {
            if (!bFirst) sDetailedError += ", ";
            sDetailedError += sFile;
            bFirst = false;
         }
      }
   }
   
   m_Error.sError += sDetailedError;
}
CMathItem* CTexParser::Parse(const string& sText) {
   ClearError();
   if(!m_MacroProcessor.InitMacros())
      return nullptr;
   if (m_pTokenizer)
      delete m_pTokenizer;
   m_pTokenizer = new CTokenizer(sText);
   SetStage(epsTOKENIZING);
   if (!m_pTokenizer->Tokenize(m_vTokens, m_Error))
      return nullptr; //tokenization error
   SetStage(epsGROUPING);
   if (!BuildGroups())
      return nullptr;      // grouping error
   if (!m_MacrosMgr.isEmpty()) {
      m_vTokensOrig = m_vTokens; //save original tokens for error reporting
      bool bExpanded = true;
      int nExpansionCount = 0;
      vector<STexToken> vExpanded, vTokens2;
      while (bExpanded) {
         SetStage(epsMACROEXP);
         bExpanded = false;
         for (int nIdx = 0; nIdx < (int)m_vTokens.size(); ) {
            if (m_MacroProcessor.ExpandToken(m_vTokens, nIdx, vExpanded)) {
               bExpanded = true;
               for (STexToken& tkExpanded : vExpanded) {
                  tkExpanded.nTkIdxEnd = 0;        //reset any previous grouping info
                  vTokens2.push_back(tkExpanded);
               }
            }
            else if(HasError())
               return nullptr; //macro expansion error
            else {
               m_vTokens[nIdx].nTkIdxEnd = 0;      //reset any previous grouping info
               vTokens2.push_back(m_vTokens[nIdx]);
               ++nIdx;
            }
         }
         if (bExpanded) {
            if (++nExpansionCount > 11) {
               SetError(-1, "Too many expansions, possible cycled definition");
               return nullptr;
            }
         }
         //move 
         m_vTokens = vTokens2;
         vTokens2.clear();
         SetStage(epsGROUPING);
         if (!BuildGroups())
            return nullptr;      // grouping error
      } // while
   }
   SetStage(epsBUILDING);
   SParserContext ctx;
   ctx.bTextMode = true;         // start in text mode
   //more settings in initial context?

   int nIdx = -1; //root text group
   return m_pTextProcessor->ProcessGroup(nIdx, ctx);
}
CMathItem* CTexParser::ProcessItemToken(IN OUT int& nIdx, const SParserContext& ctx) {
   return ctx.bTextMode ? m_pTextProcessor->ProcessItemToken(nIdx, ctx) :
      m_pMathProcessor->ProcessItemToken(nIdx, ctx);
}
CMathItem* CTexParser::ProcessGroup(IN OUT int& nIdx, const SParserContext& ctx) {
   bool bCanBeEmpty = true;
   SParserContext ctxG;
   ctxG.CopyBasics(ctx);

   STexToken& tkOpen = m_vTokens[nIdx];
   _ASSERT_RET((int)tkOpen.nTkIdxEnd > nIdx && tkOpen.nTkIdxEnd <= m_vTokens.size(), nullptr);//snbh!
   CEnvHelper envh;
   const int nIdx0 = nIdx;
   if (!OnGroupOpen_(nIdx, ctxG, bCanBeEmpty, envh))
      return nullptr; //error in group open token
   CMathItem* pRet = ctxG.bTextMode ? m_pTextProcessor->ProcessGroup(nIdx, ctxG) :
                                      m_pMathProcessor->ProcessGroup(nIdx, ctxG, envh);
   if (!pRet && m_Error.sError.empty() && !bCanBeEmpty) {
      m_Error.nStartPos = tkOpen.nPos;
      const STexToken* ptkClose = GetToken(tkOpen.nTkIdxEnd);
      m_Error.nEndPos = ptkClose ? ptkClose->nPos + ptkClose->nLen : tkOpen.nPos + tkOpen.nLen;
      m_Error.sError = "Unexpected empty group";
   }
   return pRet;
}

// HELPERS
bool CTexParser::OnGroupOpen_(int nTkIdx, IN OUT SParserContext& ctxG,
                              OUT bool& bCanBeEmpty, IN OUT CEnvHelper& envh) {
   const STexToken& tkOpen = m_vTokens[nTkIdx];
   if (!ctxG.bTextMode && (tkOpen.nType == ett$ || tkOpen.nType == ett$$)) {
      SetError(nTkIdx,"Inner $..$/$$...$$ are not allowed in math mode");
      return false;
   }
   // reset external context flags
   switch (tkOpen.nType) {
   case ettSB_OPEN: bCanBeEmpty = false; break;
   case ettFB_OPEN:bCanBeEmpty = true; break;
   case ett$$:
      ctxG.currentStyle = etsDisplay;
      ctxG.bDisplayFormula = true;
      ctxG.bTextMode = false; //math display mode
      bCanBeEmpty = false;
      break;
   case ett$:
      ctxG.currentStyle = etsText;
      ctxG.bTextMode = false; //math inline mode
      bCanBeEmpty = false;
      break;
   case ettCOMMAND: {
      string sCmd = TokenText(&tkOpen);
      if (sCmd == "\\[") {
         if (!ctxG.bTextMode) {
            SetError(nTkIdx, "Inner \\[...\\] are not allowed in math mode");
            return false;
         }
         ctxG.currentStyle = etsDisplay;
         ctxG.bDisplayFormula = true;
         ctxG.bTextMode = false; //math mode
         bCanBeEmpty = false;
      }
      else if (sCmd == "\\left") {
         if (ctxG.bTextMode) {
            SetError(nTkIdx, "\\left can only be used in math mode");
            return false;
         }
         bCanBeEmpty = true;
         ctxG.bInLeftRight = true;
      }
      else if (sCmd == "\\begin") {
         if (!envh.Init(*this, nTkIdx, ctxG)) {
            if (m_Error.sError.empty()) {
               _ASSERT(0);//snbh!
               SetError(nTkIdx, "Failed to init environment");
            }
            return false;
         }
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
bool CTexParser::BuildGroups() {
   vector<STexToken>& vTokens = (m_Error.eStage <= epsBUILDINGMACROS ? m_MacroProcessor.GetTokens() : m_vTokens);
   vector<int> vGroupStarts; // indexes of group opening tokens
   for (int nIdx = 0; nIdx < (int)vTokens.size(); ++nIdx) {
      STexToken& tkn = vTokens[nIdx];
      switch (tkn.nType)
      {
      case ettFB_OPEN:
      case ettSB_OPEN: vGroupStarts.push_back(nIdx); break;
      case ett$:
      case ett$$:     //check if closing the group!
         if (!vGroupStarts.empty() && vTokens[vGroupStarts.back()].nType == tkn.nType) {
            vTokens[vGroupStarts.back()].nTkIdxEnd = nIdx;
            vGroupStarts.pop_back();
         }
         else {
            //check if we already have opening $/$$ token:
            // "${$"    means '{'  is not closed!
            for(int nGSIdx: vGroupStarts) {
               STexToken& tknGS = vTokens[nGSIdx];
               if (tknGS.nType == tkn.nType) {
                  int nOpenIdx = vGroupStarts.back();
                  SetError(nOpenIdx, "Unclosed group '" + TokenText(nOpenIdx) + "'");
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
                  SetError(nIdx, "Missing {environment} argument after \\begin");
                  return false;
               }
            }
            else if (sCmd == "\\left") {
               //denominate next [ to non-alpha if any
               const STexToken* pNext = GetToken(nIdx + 1);
               if (pNext && pNext->nType == ettSB_OPEN)
                  vTokens[nIdx + 1].nType = ettNonALPHA;
            }
            vGroupStarts.push_back(nIdx);
         }
         else if (sCmd == "\\right" || sCmd == "\\]" || sCmd == "\\end") {
            if (vGroupStarts.empty() || !_IsOpenClose(TokenText(vGroupStarts.back()), sCmd)) {
               SetError(nIdx, "Unmatched '" + sCmd + "' ");
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
                  SetError(nIdx, "Missing {environment} argument after \\end");
                  return false;
               }
               //const STexToken* pTknOpenEnv = GetToken(vGroupStarts.back() + 2);
               if (TokenText(vGroupStarts.back() + 2) != TokenText(nIdx + 2)) {
                  SetError(nIdx + 2, "Unmatched environment name'" + TokenText(nIdx + 2) + "' afetr \\end");
                  return false;
               }
            }
            vTokens[vGroupStarts.back()].nTkIdxEnd = nGroupEnd;
            vGroupStarts.pop_back();
         }
      }
         break;
      case ettFB_CLOSE: { //non-optional!
         bool bOk = false;
            while(!vGroupStarts.empty() ) {
               STexToken& tkOpen = vTokens[vGroupStarts.back()];
               if (tkOpen.nType == ettSB_OPEN) {
                  tkOpen.nType = ettNonALPHA;
                  vGroupStarts.pop_back();
                  continue;
               }
               //else
               if (tkOpen.nType == ettFB_OPEN) {
                  bOk = true;
                  break;
               }
               //else
               SetError(nIdx, "Unmatched closing '}' bracket is not allowed");
               return false;
            }
            vTokens[vGroupStarts.back()].nTkIdxEnd = nIdx;
            vGroupStarts.pop_back();
         }
         break;
      case ettSB_CLOSE:
         if (vGroupStarts.empty() || vTokens[vGroupStarts.back()].nType != ettSB_OPEN) {
            //change token to nonAlpha
            tkn.nType = ettNonALPHA;
            continue;
         }
         //else
         vTokens[vGroupStarts.back()].nTkIdxEnd = nIdx;
         vGroupStarts.pop_back();
         break;
      }//switch
   } //for
   while(!vGroupStarts.empty()) {
      STexToken& tkUnmatched = vTokens[vGroupStarts.back()];
      if (tkUnmatched.nType == ettSB_OPEN) {
         //change token to nonAlpha
         tkUnmatched.nType = ettNonALPHA;
         vGroupStarts.pop_back();
         continue;
      }
      //else
      SetError(vGroupStarts.back(), "Unclosed opening token '" + TokenText(&tkUnmatched) + "'");
      return false;
   }
   return true;
}
const STexToken* CTexParser::GetToken(int nIdx) const {
   const vector<STexToken>& vTokens = GetTokens();
   if ((uint32_t)nIdx >= vTokens.size())
      return nullptr; //caller must set error!
   return &vTokens[nIdx];
}
string CTexParser::TokenText(int nIdx) const {
   const vector<STexToken>& vTokens = GetTokens();
   const STexToken* pToken = (nIdx >= 0 && nIdx < (int)vTokens.size()) ? &vTokens[nIdx] : nullptr;
   return TokenText(pToken);
}

string CTexParser::TokenText(const STexToken* pToken) const {
   string sRet;
   if (!pToken)
      return sRet;
   if (pToken->nRefIdx != 0) {
      m_MacrosMgr.getTokenText(*pToken, sRet);
      return sRet;
   }
   return pToken ? m_pTokenizer->TokenText(*pToken) : sRet;
}





