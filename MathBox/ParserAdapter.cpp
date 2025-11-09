#include "stdafx.h"
#include "ParserAdapter.h"

// static helpers
namespace {
   bool _IsNaturalNum(const string& sText) {
      for (char c : sText) {
         if (c < '0' || c > '9')
            return false;
      }
      return true;
   }
   bool _IsKnownUnit(const string& sUnit) {
      static const vector<string> _vKnownUnits{
         "pt","cm","mm","in"
      };
      for (const string& sKU : _vKnownUnits) {
         if (sUnit == sKU)
            return true;
      }
      return false;
   }
   bool _ConvertToPts(IN OUT float& fPts, const string& sUnit) {
      if (sUnit == "pt")
         ; //ntd
      else if (sUnit == "cm")
         fPts *= 72.0f / 2.54f;
      else if (sUnit == "mm")
         fPts *= 72.0f / 25.4f;
      else if (sUnit == "in")
         fPts *= 72.0f;
      else
         return false; //unknown unit
      return true;
   }
}
bool CParserAdapter::_CanConsumeToken(EnumLCATParenthesis eParens) const {
   const STexToken* pTkNext = m_TexParser.GetToken(m_nTkIdx);
   if (!pTkNext)
      return false;
   if (eParens == elcapSquare)
      return (pTkNext->nType == ettSB_OPEN);
   if (eParens == elcapFig)
      return (pTkNext->nType == ettFB_OPEN);
   //elcapAny
   return (pTkNext->nType> ettUndef && pTkNext->nType <= ettFB_OPEN );
}
bool CParserAdapter::_GetText(int nTkStart, int nTkEnd, OUT string& sText) const {
   sText.clear();
   for (int nIdx = nTkStart; nIdx < nTkEnd; ++nIdx) {
      const STexToken* pTk = m_TexParser.GetToken(nIdx);
      if (!pTk || !(pTk->nType == ettALNUM || pTk->nType == ettNonALPHA))
         return false; //we take only alnum/non-alnum tokens!
      sText += m_TexParser.TokenText_(nIdx);
   }
   return true;
}
CMathItem* CParserAdapter::ConsumeItem(EnumLCATParenthesis eParens, CMathStyle* pStyle) {
   if(!_CanConsumeToken(eParens))
      return nullptr; //caller must set error!
   // Create context with custom style if provided
   SParserContext ctxCmd(m_ctx);
   if (pStyle)
      ctxCmd.currentStyle = *pStyle;
   CMathItem* pRet = nullptr;
   int nTkIdx = m_nTkIdx;
   if (eParens != elcapAny) //process group
      pRet = m_TexParser.ProcessGroup(nTkIdx, ctxCmd); //move to opening
   else { //process single token, without sub/superscripts if any
      ctxCmd.bInCmdArg = true; //we are in command argument
      pRet = m_TexParser.ProcessItemToken(nTkIdx, ctxCmd);
   }
   if(pRet && m_TexParser.LastError().sError.empty())
      m_nTkIdx = nTkIdx; //move to next token on success
   return pRet;
}
// [ettNonAlNum]ettAlNum[{ettNonAlNum}{ettAlNum}][ettAlNum] = [-]{#...#[.#...#][unit]}. default unit = "pt" 
bool CParserAdapter::ConsumeDimension(EnumLCATParenthesis eParens, OUT float& fPts) {
   if (!_CanConsumeToken(eParens))
      return false; //caller must set error!
   const STexToken* pTkNext = m_TexParser.GetToken(m_nTkIdx);
   int nTkStart = m_nTkIdx;
   if (eParens != elcapAny) {//group
      ++nTkStart; //skip opening
      pTkNext = m_TexParser.GetToken(nTkStart);
   }
   int nTkEnd = nTkStart + 1;
   //check for leading '-'
   bool bNegative = false;
   if (pTkNext->nType == ettNonALPHA && m_TexParser.TokenText_(nTkStart) == "-") {
      ++nTkEnd;
      bNegative = true;
   }
   pTkNext = m_TexParser.GetToken(nTkEnd - 1);
   string sNum, sN2;
   //next must be integer part
   if(pTkNext->nType != ettALNUM)
      return false; // not a number, caller must set error!
   sNum = m_TexParser.TokenText_(nTkEnd - 1);
   if(!_IsNaturalNum(sNum))
      return false; // not an integer number, caller must set error!
   ++nTkEnd; pTkNext = m_TexParser.GetToken(nTkEnd - 1);
   if (pTkNext->nType == ettNonALPHA && m_TexParser.TokenText_(nTkEnd - 1) == ".") {
      string sN2;   //decimal part
      ++nTkEnd; pTkNext = m_TexParser.GetToken(nTkEnd - 1);
      if (pTkNext->nType != ettALNUM)
         return false; // not a number, caller must set error!
      sN2 = m_TexParser.TokenText_(nTkEnd - 1);
      if(!_IsNaturalNum(sN2))
         return false; // not an integer number, caller must set error!
      sNum += "." + sN2;// combine
      ++nTkEnd; pTkNext = m_TexParser.GetToken(nTkEnd - 1);
   }
   if(bNegative)
      sNum = "-" + sNum;
   //parse float
   try {
      fPts = stof(sNum);
   }
   catch (...) {
      //SetError("Failed to parse FP value from '" + sN1 + "' !");
      return false;
   }
   //check/convert units
   if (pTkNext->nType == ettALNUM && _IsKnownUnit(m_TexParser.TokenText_(nTkEnd - 1))) {
      if(!_ConvertToPts(fPts , m_TexParser.TokenText_(nTkEnd - 1)))
         _ASSERT_RET(0, false); //snbh!
      ++nTkEnd; pTkNext = m_TexParser.GetToken(nTkEnd - 1);
   }
   if (eParens != elcapAny) {
      //check we are at closing!
      pTkNext = m_TexParser.GetToken(m_nTkIdx);
      if (nTkEnd != pTkNext->nTkIdxEnd - 1)
         return false; //not at closing, some extra tokens in the group?
      m_nTkIdx = pTkNext->nTkIdxEnd; //move to next token on success
   }
   else
      m_nTkIdx = nTkEnd; //move to next token on success

   return true;
}
bool CParserAdapter::ConsumeInteger(EnumLCATParenthesis eParens, OUT int& nVal) {
   if (!_CanConsumeToken(eParens))
      return false; //caller must set error!
   const STexToken* pTkNext = m_TexParser.GetToken(m_nTkIdx);
   string sText;
   int nTkStart = m_nTkIdx;
   if (eParens != elcapAny) {//group
      ++nTkStart; //skip opening
      pTkNext = m_TexParser.GetToken(nTkStart);
   } 
   int nTkEnd = nTkStart+1;
   //check for leading '-'
   if(pTkNext->nType == ettNonALPHA && m_TexParser.TokenText_(nTkStart) == "-")
      ++nTkEnd;
   //get text
   if(!_GetText(nTkStart, nTkEnd, sText))
      return false; //error, caller must set error!
   //parse integer
   try {
      nVal = stoi(sText);
   }
   catch (...) {
      //SetError("Failed to parse integer value from '" + sText + "' !");
      return false;
   }
   if (eParens != elcapAny) {
      //check we are at closing!
      pTkNext = m_TexParser.GetToken(m_nTkIdx);
      if(nTkEnd != pTkNext->nTkIdxEnd - 1)
         return false; //not at closing, some extra tokens in the group?
      m_nTkIdx = pTkNext->nTkIdxEnd; //move to next token on success
   }
   else
      m_nTkIdx = nTkEnd; //move to next token on success

   return true;
}
bool CParserAdapter::ConsumeKeyword(PCSTR szKeyword) {
   const STexToken* pTkNext = m_TexParser.GetToken(m_nTkIdx);
   if (!pTkNext || ettCOMMAND != pTkNext->nType)
      return false; //caller must set error!
   string sCmd = m_TexParser.TokenText_(m_nTkIdx);
   if(sCmd != szKeyword)
      return false;
   // consume it
   ++m_nTkIdx;
   return true;
}
