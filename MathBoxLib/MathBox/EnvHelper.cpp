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
   string sError;
   if (!InitEnvDelimiters_(sError)) {
      parser.SetError(nTkIdx + 2, sError);
      return false;
   }
   m_nStartTkIdx = nTkIdx + 4; //default: next token after \begin{env}
   if (m_sEnv == "array" || m_sEnv == "darray" || m_sEnv == "subarray") {
      if (!InitArrayColumnSpecs_(parser, nTkIdx + 4))
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
   if (m_sEnv == "array" || m_sEnv == "cases" || m_sEnv == "rcases") {
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
   if (pTokenColSpecOpen->nType != ettFB_OPEN || (int)pTokenColSpecOpen->nTkIdxEnd <= nIdxColSpec + 1) {
      parser.SetError(nIdxColSpec, "Missing {column specifications} argument after \\begin{" + m_sEnv + "}");
      return false;
   }
   //check for optional column alignments : {[|,||,:][l,c,r]+[|,||,:]}
   // default is 'c' for each column
   bool bLastWasSep = false; //to avoid two seps in a row
   for (int nIdx = nIdxColSpec + 1; nIdx < (int)pTokenColSpecOpen->nTkIdxEnd;) {
      const STexToken* pTkNext = parser.GetToken(nIdx);
      _ASSERT_RET(pTkNext, false);//snbh!
      string sTkText = parser.TokenText(nIdx);
      if (pTkNext->nType == ettNonALPHA && 
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
         m_vVLines.push_back(cSep);
         ++nIdx;
         bLastWasSep = true;
      }
      else if (pTkNext->nType == ettALNUM && _IsValidColAlignment(sTkText)) { 
         if(!bLastWasSep && m_vColAlignments.empty()) {
            //no separator before first column
            m_vVLines.push_back(0);
         }
         //l,c,r
         for (char cAlign : sTkText) {
            if (cAlign == 'l')
               m_vColAlignments.push_back(ecaLeft);
            else if (cAlign == 'r')
               m_vColAlignments.push_back(ecaRight);
            else //c
               m_vColAlignments.push_back(ecaCenter);
            ++nIdx;
            m_vVLines.push_back(0); //no separator after this column yet
         }
         m_vVLines.pop_back(); //remove extra 0 sep after last column
         bLastWasSep = false;
      }
      else {
         parser.SetError(nIdx, "Invalid column specifier in array environment!");
         return false;
      }
   }//for
   if (!bLastWasSep) {
      _ASSERT(m_vColAlignments.size() == m_vVLines.size());
      m_vVLines.push_back(0); //no separator after last column
   }
   else
      _ASSERT(m_vColAlignments.size()+1 == m_vVLines.size());
   m_nStartTkIdx = pTokenColSpecOpen->nTkIdxEnd + 1;//next token after \begin{env}{colspec}
   return true;
}
bool CEnvHelper::InitEnvDelimiters_(OUT string& sError) {
   
   if (m_sEnv == "darray" || m_sEnv == "array" || m_sEnv == "matrix" ||
      m_sEnv=="smallmatrix" || m_sEnv == "subarray")
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
