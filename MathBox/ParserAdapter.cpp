#include "stdafx.h"
#include "ParserAdapter.h"

// static helpers
namespace {
   bool _IsDigit(char cC) { return (cC >= '0' && cC <= '9'); }
   bool _IsKnownUnit(const string& sUnit) {
      static const vector<string> _vKnownUnits{
         "pt","cm","mm","in","em", "ex"
      };
      for (const string& sKU : _vKnownUnits) {
         if (sUnit == sKU)
            return true;
      }
      return false;
   }
   bool _ConvertToPts(IN OUT float& fPts, const string& sUnit, float fDocFontSizePts) {
      if (sUnit == "pt")
         ; //ntd
      else if (sUnit == "cm")
         fPts *= 72.0f / 2.54f;
      else if (sUnit == "mm")
         fPts *= 72.0f / 25.4f;
      else if (sUnit == "in")
         fPts *= 72.0f;
      else if (sUnit == "em") 
         fPts *= fDocFontSizePts; 
      else if (sUnit == "ex")
         fPts *= fDocFontSizePts/2;
      else
         return false; //unknown unit
      return true;
   }
   bool _GetDimen(const string& sNum, OUT float& fNum, OUT string& sUnit) {
      _ASSERT_RET(!sNum.empty(), false);
      bool bHasDot = false;
      PCSTR szPos = sNum.c_str();
      while (*szPos) {
         if (!_IsDigit(*szPos)) {
            if (!bHasDot && '.' == *szPos) {
               bHasDot = true;
               ++szPos;
               continue;
            }
            break;
         }
         ++szPos;
      }
      if (*szPos)
         sUnit = sNum.substr(szPos - sNum.c_str(), -1);
      string sFNum = sNum.substr(0, szPos - sNum.c_str());
      try{
         fNum = stof(sFNum);
      }
      catch (...) {
         return false;
      }
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
      sText += m_TexParser.TokenText(nIdx);
   }
   return true;
}
CMathItem* CParserAdapter::ConsumeItem(EnumLCATParenthesis eParens, const SParserContext& ctx) {
   if(!_CanConsumeToken(eParens))
      return nullptr; //caller must set error!
   // Create context with custom style if provided
   CMathItem* pRet = nullptr;
   int nTkIdx = m_nTkIdx;
   if (eParens != elcapAny) //process group
      pRet = m_TexParser.ProcessGroup(nTkIdx, ctx); //move to opening
   else { //process single token, without sub/superscripts if any
      SParserContext ctxCmd(ctx);
      pRet = m_TexParser.ProcessItemToken(nTkIdx, ctxCmd);
   }
   if(pRet && !m_TexParser.HasError())
      m_nTkIdx = nTkIdx; //move to next token on success
   return pRet;
}
// [ettNonAlNum]ettAlNum[{ettNonAlNum}{ettAlNum}][ettAlNum] = [-]{#...#[.#...#][unit]}. default unit = "pt" 
bool CParserAdapter::ConsumeDimension(EnumLCATParenthesis eParens, OUT float& fPts) {
   if (!_CanConsumeToken(eParens))
      return false; //caller must set error!
   const STexToken* pTkNext = m_TexParser.GetToken(m_nTkIdx);
   int nTkStart = m_nTkIdx;
   if (pTkNext->nTkIdxEnd) {//group
      ++nTkStart; //skip opening
      pTkNext = m_TexParser.GetToken(nTkStart);
   }
   int nTkPos = nTkStart;
   //check for leading '-'
   bool bNegative = false;
   if (pTkNext->nType == ettNonALPHA && m_TexParser.TokenText(nTkStart) == "-") {
      ++nTkPos;
      bNegative = true;
      pTkNext = m_TexParser.GetToken(nTkPos);
   }
   string sNum, sUnit;
   //next must be number
   if(pTkNext->nType != ettALNUM)
      return false; // not a number, caller must set error!
   sNum = m_TexParser.TokenText(nTkPos);
   if(! _GetDimen(sNum, fPts, sUnit))
      return false; // not an integer number, caller must set error!
   ++nTkPos; //consume good #{unit} token
   if (sUnit.empty()) {
      pTkNext = m_TexParser.GetToken(nTkPos);
      if (pTkNext->nType == ettNonALPHA && m_TexParser.TokenText(nTkPos) == ".") {
         ++nTkPos; 
         pTkNext = m_TexParser.GetToken(nTkPos);
         if (pTkNext->nType != ettALNUM)
            return false; // not a number, caller must set error!
         sNum = sNum + "." + m_TexParser.TokenText(nTkPos);
         if (!_GetDimen(sNum, fPts, sUnit))
            return false; // not an integer number, caller must set error!
         // move to next token
         ++nTkPos; 
         pTkNext = m_TexParser.GetToken(nTkPos);
      }
   }
   if (sUnit.empty() && pTkNext->nType == ettALNUM) {
      sUnit = m_TexParser.TokenText(nTkPos);
      ++nTkPos;
   }
   if (!sUnit.empty()){
      if (!_IsKnownUnit(sUnit))
         return false; //bad units
      if (!_ConvertToPts(fPts, sUnit, m_TexParser.DocumentFontSizePts()))
         _ASSERT_RET(0, false); //snbh!
      //check we are at closing!
      //++nTkPos; //good units!
   }
   pTkNext = m_TexParser.GetToken(m_nTkIdx);//start
   if (pTkNext->nTkIdxEnd && nTkPos + 1 != pTkNext->nTkIdxEnd)
      return false; //not at closing, some extra tokens in the group?
   m_nTkIdx = nTkPos; //move to next token on success
   if (bNegative)
      fPts = -fPts;
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
   if(pTkNext->nType == ettNonALPHA && m_TexParser.TokenText(nTkStart) == "-")
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
EnumTokenType CParserAdapter::GetTokenData(OUT string& sText) {
   const STexToken* pTkNext = m_TexParser.GetToken(m_nTkIdx);
   if (!pTkNext)
      return ettUndef; //error
   sText = m_TexParser.TokenText(m_nTkIdx);
   return (EnumTokenType)pTkNext->nType;
}
bool CParserAdapter::ConsumeHSkipGlue(OUT STexGlue& glue) {
   // Initialize glue to defaults
   glue = STexGlue();

   // 1. Parse base dimension (required)
   float fSize = 0.0f;
   if (!ConsumeDimension(elcapAny, fSize)) {
      m_TexParser.SetError(m_nTkIdx, "Expected dimension for \\hskip");
      return false;
   }

   // Convert to EM units (your glue uses EM)
   float fFontSizePts = m_TexParser.DocumentFontSizePts();
   float fSizeEM = otfUnitsPerEm * fSize/fFontSizePts;
   glue.fNorm = fSizeEM;
   glue.fActual = fSizeEM;

   // 2. Check for "plus" stretch component
   if (m_TexParser.TokenText(m_nTkIdx) == "plus") {
      ++m_nTkIdx;
      if (!_ConsumeGlueComponent(glue.fStretchCapacity, glue.nStretchOrder)) {
         m_TexParser.SetError(m_nTkIdx, "Invalid stretch component after 'plus'");
         return false;
      }
   }

   // 3. Check for "minus" shrink component
   if (m_TexParser.TokenText(m_nTkIdx) == "minus") {
      ++m_nTkIdx;
      if (!_ConsumeGlueComponent(glue.fShrinkCapacity, glue.nShrinkOrder)) {
         m_TexParser.SetError(m_nTkIdx, "Invalid shrink component after 'minus'");
         return false;
      }
   }

   return true;
}

// Helper: Parse stretch/shrink component (can be dimension or fil/fill/filll)
bool CParserAdapter::_ConsumeGlueComponent(OUT float& fValue, OUT uint16_t& nOrder) {
   fValue = 0.0f;
   nOrder = 0;

   const STexToken* pTok = m_TexParser.GetToken(m_nTkIdx);
   if (!pTok) return false;

   // Try to consume dimension
   float fDim = 0.0f;
   if (ConsumeDimension(elcapAny, fDim)) {
      // Check if followed by fil/fill/filll
      pTok = m_TexParser.GetToken(m_nTkIdx);
      if (pTok && pTok->nType == ettALNUM) {
         string sUnit = m_TexParser.TokenText(m_nTkIdx);

         if (sUnit == "fil") {
            nOrder = 1;
            fValue = fDim;
            ++m_nTkIdx;
            return true;
         }
         else if (sUnit == "fill") {
            nOrder = 2;
            fValue = fDim;
            ++m_nTkIdx;
            return true;
         }
         else if (sUnit == "filll") {
            nOrder = 3;
            fValue = fDim;
            ++m_nTkIdx;
            return true;
         }
      }

      // Regular dimension (convert to EM)
      float fFontSizePts = m_TexParser.DocumentFontSizePts();
      fValue = otfUnitsPerEm*PTS2DIPS(fDim)/fFontSizePts;
      return true;
   }

   return false;
}
