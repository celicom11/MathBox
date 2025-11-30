#include "stdafx.h"
#include "TexParser.h"
#include "Tokenizer.h"
#include "MathModeProcessor.h"
#include "TextModeProcessor.h"


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
   bool _IsKnownEnvironment(const string& sEnv) {
      return sEnv == "array" || sEnv == "matrix"; //q&d
   }
}
// CTexParser
CTexParser::CTexParser() {
   m_pMathProcessor = new CMathModeProcessor(*this);
   m_pTextProcessor = new CTextModeProcessor(*this);
}
CTexParser::~CTexParser() {
   delete m_pTokenizer;
   delete m_pTextProcessor;
   delete m_pMathProcessor;
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

   const STexToken& tkOpen = m_vTokens[nIdx];
   _ASSERT_RET((int)tkOpen.nTkIdxEnd > nIdx && tkOpen.nTkIdxEnd <= m_vTokens.size(), nullptr);//snbh!
   if (!OnGroupOpen_(tkOpen, ctxG, bCanBeEmpty))
      return nullptr; //error in group open token
   CMathItem* pRet = ctxG.bTextMode ? m_pTextProcessor->ProcessGroup(nIdx, ctxG) :
      m_pMathProcessor->ProcessGroup(nIdx, ctxG);
   if (!pRet && m_Error.sError.empty() && !bCanBeEmpty) {
      m_Error.nPosition = tkOpen.nPos;
      m_Error.sError = "Unexpected empty group";
   }
   return pRet;
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
      string sCmd = m_pTokenizer->TokenText(tkOpen);
      if (sCmd == "\\[") {
         if (!ctxG.bTextMode) {
            m_Error.nPosition = tkOpen.nPos;
            m_Error.sError = "Inner \\[...\\] are not allowed in math mode!";
            return false;
         }
         ctxG.currentStyle = etsDisplay;
         ctxG.bDisplayFormula = true;
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
const STexToken* CTexParser::GetToken(int nIdx) const {
   if ((uint32_t)nIdx >= m_vTokens.size())
      return nullptr; //caller must set error!
   return &m_vTokens[nIdx];
}
string CTexParser::TokenText(int nIdx) const {
   const STexToken* pToken = (nIdx >= 0 && nIdx < (int)m_vTokens.size()) ? &m_vTokens[nIdx] : nullptr;
   return pToken ? m_pTokenizer->TokenText(*pToken) : string();
}


