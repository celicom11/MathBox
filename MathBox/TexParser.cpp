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
CTexParser::CTexParser(IDocParams& doc):m_Doc(doc) {
   m_pMathProcessor = new CMathModeProcessor(*this);
   m_pTextProcessor = new CTextModeProcessor(*this);
}
CTexParser::~CTexParser() {
   delete m_pTokenizer;
   delete m_pTextProcessor;
   delete m_pMathProcessor;
}
CMathItem* CTexParser::Parse(const string& sText) {
   ClearError();
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

   STexToken& tkOpen = m_vTokens[nIdx];
   _ASSERT_RET((int)tkOpen.nTkIdxEnd > nIdx && tkOpen.nTkIdxEnd <= m_vTokens.size(), nullptr);//snbh!
   CEnvHelper envh;
   const int nIdx0 = nIdx;
   if (!OnGroupOpen_(nIdx, ctxG, bCanBeEmpty, envh))
      return nullptr; //error in group open token
   CMathItem* pRet = ctxG.bTextMode ? m_pTextProcessor->ProcessGroup(nIdx, ctxG) :
                                      m_pMathProcessor->ProcessGroup(nIdx, ctxG, envh);
   if (!pRet && m_Error.sError.empty() && !bCanBeEmpty) {
      if (tkOpen.nType == ettSB_OPEN) {
         //exceptional: empty [...] is allowed but group is disqualified!
         STexToken& tkClose = m_vTokens[tkOpen.nTkIdxEnd];
         tkOpen.nType = tkClose.nType = ettNonALPHA; //convert to normal token
         tkOpen.nTkIdxEnd = 0;
         nIdx = nIdx0; //reset index to re-process as normal tokens
      }
      else {
         m_Error.nStartPos = tkOpen.nPos;
         const STexToken* ptkClose = GetToken(tkOpen.nTkIdxEnd);
         m_Error.nEndPos = ptkClose ? ptkClose->nPos + ptkClose->nLen : tkOpen.nPos + tkOpen.nLen;
         m_Error.sError = "Unexpected empty group";
      }
   }
   return pRet;
}
// HELPERS
bool CTexParser::OnGroupOpen_(int nTkIdx, IN OUT SParserContext& ctxG,
                              OUT bool& bCanBeEmpty, IN OUT CEnvHelper& envh) {
   const STexToken& tkOpen = m_vTokens[nTkIdx];
   if (!ctxG.bTextMode && (tkOpen.nType == ett$ || tkOpen.nType == ett$$)) {
      m_Error.SetError(tkOpen ,"Inner $..$/$$...$$ are not allowed in math mode");
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
            m_Error.SetError(tkOpen, "Inner \\[...\\] are not allowed in math mode");
            return false;
         }
         ctxG.currentStyle = etsDisplay;
         ctxG.bDisplayFormula = true;
         ctxG.bTextMode = false; //math mode
         bCanBeEmpty = false;
      }
      else if (sCmd == "\\left") {
         if (ctxG.bTextMode) {
            m_Error.SetError(tkOpen, "\\left can only be used in math mode");
            return false;
         }
         bCanBeEmpty = true;
         ctxG.bInLeftRight = true;
      }
      else if (sCmd == "\\begin") {
         if (!envh.Init(*this, nTkIdx, ctxG)) {
            if (m_Error.sError.empty()) {
               _ASSERT(0);//snbh!
               m_Error.SetError(tkOpen, "Failed to init environment");
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
                  const STexToken& tknOpen = m_vTokens[vGroupStarts.back()];
                  m_Error.SetError(tknOpen, "Unclosed group '" + TokenText(vGroupStarts.back()) + "'");
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
                  const STexToken& tknError = m_vTokens[nIdx];
                  m_Error.SetError(tknError, "Missing {environment} argument after \\begin");
                  return false;
               }
            }
            vGroupStarts.push_back(nIdx);
         }
         else if (sCmd == "\\right" || sCmd == "\\]" || sCmd == "\\end") {
            if (vGroupStarts.empty() || !_IsOpenClose(TokenText(vGroupStarts.back()), sCmd)) {
               m_Error.nStartPos = tkn.nPos;
               m_Error.nEndPos = tkn.nPos + tkn.nLen;
               m_Error.sError = "Unmatched '" + sCmd + "' ";
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
                  const STexToken& tknError = m_vTokens[nIdx];
                  m_Error.SetError(tknError, "Missing {environment} argument after \\end");
                  return false;
               }
               //const STexToken* pTknOpenEnv = GetToken(vGroupStarts.back() + 2);
               if (TokenText(vGroupStarts.back() + 2) != TokenText(nIdx + 2)) {
                  m_Error.SetError(*pTknEnv, 
                     "Unmatched environment name'" + TokenText(nIdx + 2) + "' afetr \\end");
                  return false;
               }
            }
            m_vTokens[vGroupStarts.back()].nTkIdxEnd = nGroupEnd;
            vGroupStarts.pop_back();
         }
      }
         break;
      case ettFB_CLOSE: { //non-optional!
         bool bOk = false;
            while(!vGroupStarts.empty() ) {
               STexToken& tkOpen = m_vTokens[vGroupStarts.back()];
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
               m_Error.SetError(tkn, "Unmatched closing '}' bracket is not allowed");
               return false;
            }
            m_vTokens[vGroupStarts.back()].nTkIdxEnd = nIdx;
            vGroupStarts.pop_back();
         }
         break;
      case ettSB_CLOSE:
         if (vGroupStarts.empty() || m_vTokens[vGroupStarts.back()].nType != ettSB_OPEN) {
            //change token to nonAlpha
            tkn.nType = ettNonALPHA;
            continue;
         }
         //else
         m_vTokens[vGroupStarts.back()].nTkIdxEnd = nIdx;
         vGroupStarts.pop_back();
         break;
      }//switch
   } //for
   while(!vGroupStarts.empty()) {
      STexToken& tkUnmatched = m_vTokens[vGroupStarts.back()];
      if (tkUnmatched.nType == ettSB_OPEN) {
         //change token to nonAlpha
         tkUnmatched.nType = ettNonALPHA;
         vGroupStarts.pop_back();
         continue;
      }
      //else
      m_Error.SetError(tkUnmatched, "Unclosed opening token '" + TokenText(vGroupStarts.back()) + "'");
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


