#include "stdafx.h"
#include "EnvHelper.h"
#include "TexParser.h"
//#include "HBoxItem.h"
//#include "FillerItem.h"

namespace {
   inline bool _IsValidColAlignment(const string& sAlign) {
      for(char cC : sAlign) {
         if (cC != 'l' && cC != 'r' && cC != 'c')
            return false;
      }
      return true;
   }
}

bool CEnvHelper::Init(CTexParser& parser, int nTkIdx, IN OUT SParserContext& ctx) {
   m_nStartTkIdx = 0;
   m_sEnv.clear();
   m_vColAlignments.clear();
   m_vVLines.clear();
   m_vColSpacing.clear();
   m_cLeftDelim = m_cRightDelim = 0;
   const STexToken* pToken = parser.GetToken(nTkIdx);
   //nTkIdx must be ettCOMMAND, "\\begin"
   _ASSERT_RET(pToken && pToken->nType == ettCOMMAND, false);
   _ASSERT_RET(parser.TokenText(nTkIdx) == "\\begin", false);
   //get environment name
   const STexToken* pTokenFBOpen = parser.GetToken(nTkIdx + 1);
   _ASSERT_RET(pTokenFBOpen && pTokenFBOpen->nType == ettFB_OPEN, false);//snbh!
   const STexToken* pTokenFBClose = parser.GetToken(nTkIdx + 3); 
   _ASSERT_RET(pTokenFBClose && pTokenFBClose->nType == ettFB_CLOSE, false);//snbh!
   m_sEnv = parser.TokenText(nTkIdx + 2);
   if (TextCells() && ctx.bTextMode && !ctx.bInTabularCell) {
      parser.SetError(nTkIdx, "tabular environment requires math content");
      return false;
   }
   string sError;
   if (!InitEnvDelimiters_(sError)) {
      parser.SetError(nTkIdx + 2, sError);
      return false;
   }
   m_nStartTkIdx = nTkIdx + 4; //default: next token after \begin{env}
   if (m_sEnv == "array" || m_sEnv == "darray" || m_sEnv == "subarray" || TextCells()) {
      int nIdxColSpec = nTkIdx + 4;
      const STexToken* pColSpec = parser.GetToken(nIdxColSpec);
      while (pColSpec && (pColSpec->nType == ettSPACE || pColSpec->nType == ettCOMMENT)) {
         pColSpec = parser.GetToken(++nIdxColSpec);
      }
      if (!InitArrayColumnSpecs_(parser, nIdxColSpec))
         return false; //error set by InitArrayColumnSpecs_
      if (m_sEnv == "subarray") {
         //must be single column, no vlines
         if (m_vColAlignments.size() != 1) {
            parser.SetError(nTkIdx + 4, "subarray environment must have a single column!");
            return false;
         }
         m_vVLines.clear(); //ignore vlines
      }
   }
   else if (m_sEnv == "cases" || m_sEnv == "rcases" || m_sEnv == "dcases" || m_sEnv == "drcases") {
      m_vColAlignments.push_back(ecaLeft);
      m_vColAlignments.push_back(ecaLeft);
   }
   else
      m_vColAlignments.push_back(ecaCenter); //default matrix alignment
   //set context stles
   if (TextCells()) {
      if (!ctx.bTextMode)
         ctx.sFontCmd.clear(); // do not carry math-font commands into text cells
      ctx.bTextMode = true;
      ctx.bDisplayFormula = false;
      ctx.bInLeftRight = false;
   }
   else if (m_sEnv == "array" || m_sEnv == "cases" || m_sEnv == "rcases") {
      if (ctx.currentStyle.Style() == etsDisplay)
         ctx.currentStyle.SetStyle(etsText); //cases use text style even in display math
   }
   else if (m_sEnv == "darray" || m_sEnv == "dcases" || m_sEnv == "drcases") {
      ctx.currentStyle.SetStyle(etsDisplay); //force display math
   }
   else if (m_sEnv == "smallmatrix" || m_sEnv == "subarray") {
      if(ctx.currentStyle.Style() < etsScript)
         ctx.currentStyle.ToSubscriptStyle();
   }
   ctx.bNoNewLines = false; //we process new-lines in the environments!
   return true;
}
bool CEnvHelper::InitArrayColumnSpecs_(CTexParser & parser, int nIdxColSpec) {
   const STexToken* pTokenColSpecOpen = parser.GetToken(nIdxColSpec);
   if (!pTokenColSpecOpen || pTokenColSpecOpen->nType != ettFB_OPEN || (int)pTokenColSpecOpen->nTkIdxEnd <= nIdxColSpec + 1) {
      parser.SetError(nIdxColSpec, "Missing {column specifications} argument after \\begin{" + m_sEnv + "}");
      return false;
   }
   //check for optional column alignments : {[|,||,:][l,c,r]+[|,||,:]}
   // default is 'c' for each column
   bool bLastWasSep = false; //to avoid two seps in a row
   m_vVLines.push_back(0); // boundary before the first column
   m_vColSpacing.emplace_back();
   for (int nIdx = nIdxColSpec + 1; nIdx < (int)pTokenColSpecOpen->nTkIdxEnd;) {
      const STexToken* pTkNext = parser.GetToken(nIdx);
      _ASSERT_RET(pTkNext, false);//snbh!
      string sTkText = parser.TokenText(nIdx);
      if (pTkNext->nType == ettSPACE || pTkNext->nType == ettCOMMENT) {
         ++nIdx;
         continue;
      }
      if (pTkNext->nType == ettNonALPHA && sTkText == "@") {
         int nIdxGroup = nIdx + 1;
         const STexToken* pGroup = parser.GetToken(nIdxGroup);
         while (pGroup && (pGroup->nType == ettSPACE || pGroup->nType == ettCOMMENT))
            pGroup = parser.GetToken(++nIdxGroup);
         if (!pGroup || pGroup->nType != ettFB_OPEN || !pGroup->nTkIdxEnd) {
            parser.SetError(nIdx, "Expected {replacement spacing} after @ in column specifications");
            return false;
         }
         for (int nContent = nIdxGroup + 1; nContent < (int)pGroup->nTkIdxEnd; ++nContent) {
            const STexToken* pContent = parser.GetToken(nContent);
            if (pContent->nType != ettSPACE && pContent->nType != ettCOMMENT) {
               parser.SetError(nContent, "Only empty @{} replacement spacing is currently supported");
               return false;
            }
         }
         m_vColSpacing.back().bOverride = true;
         nIdx = pGroup->nTkIdxEnd + 1;
      }
      else if (pTkNext->nType == ettNonALPHA &&
         (sTkText=="|" || sTkText == ":")) { //|,||,:
         if(bLastWasSep) {
            parser.SetError(nIdx, "Two consecutive column separators in array column specifications");
            return false;
         }
         char cSep = 0;
         if (sTkText == "|") {
            const STexToken* pTkNext2 = parser.GetToken(nIdx + 1);
            if (pTkNext2 && pTkNext2->nType == ettNonALPHA && parser.TokenText(nIdx + 1) == "|") {
               cSep = 'd'; //double V line
               ++nIdx;
            }
            else
               cSep = '|';
         }
         else
            cSep = ':';
         m_vVLines.back() = cSep;
         ++nIdx;
         bLastWasSep = true;
      }
      else if (pTkNext->nType == ettALNUM && _IsValidColAlignment(sTkText)) { 
         //l,c,r
         for (char cAlign : sTkText) {
            if (cAlign == 'l')
               m_vColAlignments.push_back(ecaLeft);
            else if (cAlign == 'r')
               m_vColAlignments.push_back(ecaRight);
            else //c
               m_vColAlignments.push_back(ecaCenter);
            m_vVLines.push_back(0); //no separator after this column yet
            m_vColSpacing.emplace_back();
         }
         ++nIdx; // one token can contain several alignment letters
         bLastWasSep = false;
      }
      else {
         parser.SetError(nIdx, "Invalid column specifier in array environment!");
         return false;
      }
   }//for
   if (m_vColAlignments.empty()) {
      parser.SetError(nIdxColSpec, "Column specifications must declare at least one column");
      return false;
   }
   _ASSERT(m_vColAlignments.size()+1 == m_vVLines.size());
   m_nStartTkIdx = pTokenColSpecOpen->nTkIdxEnd + 1;//next token after \begin{env}{colspec}
   return true;
}
bool CEnvHelper::InitEnvDelimiters_(OUT string& sError) {
   
   if (m_sEnv == "darray" || m_sEnv == "array" || m_sEnv == "matrix" ||
      m_sEnv=="smallmatrix" || m_sEnv == "subarray" || TextCells())
      return true;//no delimiters, no checks
   if (m_sEnv == "pmatrix" )
      m_cLeftDelim = '(', m_cRightDelim = ')';
   else if (m_sEnv == "bmatrix")
      m_cLeftDelim = '[', m_cRightDelim = ']';
   else if (m_sEnv == "Bmatrix")
      m_cLeftDelim = '{', m_cRightDelim = '}';
   else if (m_sEnv == "vmatrix")
      m_cLeftDelim = m_cRightDelim = '|';
   else if (m_sEnv == "Vmatrix")
      m_cLeftDelim = m_cRightDelim = 'd'; //double vertical line
   else if (m_sEnv == "cases" || m_sEnv == "dcases")
      m_cLeftDelim = '{', m_cRightDelim = 0;
   else if (m_sEnv == "rcases" || m_sEnv == "drcases")
      m_cLeftDelim = 0, m_cRightDelim = '}';
   else {
      sError = "Unsupported environment '" + m_sEnv + "'";
      return false;
   }
   return true;
}
