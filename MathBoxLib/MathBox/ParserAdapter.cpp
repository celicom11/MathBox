#include "stdafx.h"
#include "ParserAdapter.h"
#include "DimensionParser.h"

// static helpers
namespace {
   map<string, uint32_t> _mapColors{
      {"aqua",       0xFF00FFFF},
      {"black",      0xFF000000},
      {"blue",       0xFF0000FF},
      {"brown",      0xFFA52A2A},
      {"cyan",       0xFF00FFFF}, // alias of aqua
      {"darkblue",   0xFF00008B},
      {"darkcyan",   0xFF008B8B},
      {"darkgray",   0xFFA9A9A9},
      {"darkgreen",  0xFF006400},
      {"darkred",    0xFF8B0000},
      {"fuchsia",    0xFFFF00FF},
      {"gray",       0xFF808080},
      {"green",      0xFF008000},
      {"lightgray",  0xFFD3D3D3},
      {"lime",       0xFF00FF00},
      {"magenta",    0xFFFF00FF},
      {"maroon",     0xFF800000},
      {"navy",       0xFF000080},
      {"olive",      0xFF808000},
      {"orange",     0xFFFFA500},
      {"pink",       0xFFFFC0CB},
      {"purple",     0xFF800080},
      {"red",        0xFFFF0000},
      {"silver",     0xFFC0C0C0},
      {"teal",       0xFF008080},
      {"violet",     0xFFEE82EE},
      {"white",      0xFFFFFFFF},
      {"yellow",     0xFFFFFF00},
   };
   uint32_t _GetColorByName(const string& sColor) {
      _ASSERT_RET(!sColor.empty(), 0);
      if('#' == sColor[0]) {
         //hex color #RRGGBB or #AARRGGBB
         size_t nLen = sColor.length();
         if (nLen != 7 && nLen != 9)
            return 0; //invalid
         try {
            uint32_t nColor = std::stoul(sColor.substr(1), nullptr, 16);
            if (nLen == 7) //add full alpha
               nColor |= 0xFF000000;
            return nColor;
         }
         catch (...) {
            return 0; //invalid
         }
      }
      //else, named color
      auto it = _mapColors.find(sColor);
      if (it != _mapColors.end())
         return it->second;
      return 0; //not found
   }
   /*bool _IsDigit(char cC) { return (cC >= '0' && cC <= '9'); }
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
   }*/
}
bool CParserAdapter::_CanConsumeToken(EnumLCATParenthesis eParens) const {
   const STexToken* pTkNext = GetToken(m_nTkIdx);
   //ignore space token
   if (pTkNext && pTkNext->nType == ettSPACE)
      pTkNext = GetToken(m_nTkIdx+1);
   if (!pTkNext)
      return false;
   if (eParens == elcapSquare)
      return (pTkNext->nType == ettSB_OPEN && pTkNext->nTkIdxEnd > (uint32_t)m_nTkIdx); //double check
   if (eParens == elcapFig)
      return (pTkNext->nType == ettFB_OPEN);
   //elcapAny
   return (pTkNext->nType > ettUndef && pTkNext->nType != ettFB_CLOSE && 
      pTkNext->nType != ettCOMMENT && pTkNext->nType != ettAMP);
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
      //SParserContext ctxCmd; 
      //ctxCmd.CopyBasics(ctx);
      pRet = m_TexParser.ProcessItemToken(nTkIdx, ctx);
   }
   if(pRet && !m_TexParser.HasError())
      m_nTkIdx = nTkIdx; //move to next token on success
   return pRet;
}
bool CParserAdapter::ConsumeDimension(EnumLCATParenthesis eParens, OUT float& fPts, OUT string& sOrigUnits) {
   if (!_CanConsumeToken(eParens))
      return false; //caller must set error!
   string sError;
   int nNextIdx = CDimensionParser::_ConsumeDimension( m_TexParser, m_nTkIdx, eParens,
                                    fPts,sOrigUnits,sError);

   if (nNextIdx < 0) {
      m_TexParser.SetError(m_nTkIdx, sError);
      return false;
   }

   m_nTkIdx = nNextIdx;
   return true;
}
bool CParserAdapter::ConsumeInteger(EnumLCATParenthesis eParens, OUT int& nVal) {
   if (!_CanConsumeToken(eParens))
      return false; //caller must set error!
   const STexToken* pTkNext = GetToken(m_nTkIdx);
   string sText;
   int nTkStart = m_nTkIdx;
   if (eParens != elcapAny) {//group
      ++nTkStart; //skip opening
      pTkNext = GetToken(nTkStart);
   } 
   int nTkEnd = nTkStart+1;
   //check for leading '-'
   if(pTkNext->nType == ettNonALPHA && m_TexParser.TokenText(pTkNext) == "-")
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
      pTkNext = GetToken(m_nTkIdx);
      if(nTkEnd != pTkNext->nTkIdxEnd)
         return false; //not at closing, some extra tokens in the group?
      m_nTkIdx = pTkNext->nTkIdxEnd + 1; //move to next token on success
   }
   else
      m_nTkIdx = nTkEnd + 1; //move to next token on success

   return true;
}
EnumTokenType CParserAdapter::GetTokenData(OUT string& sText) {
   const STexToken* pTkNext = GetToken(m_nTkIdx);
   if (!pTkNext)
      return ettUndef; //error
   sText = m_TexParser.TokenText(pTkNext);
   return (EnumTokenType)pTkNext->nType;
}
bool CParserAdapter::ConsumeHSkipGlue(OUT STexGlue& glue) {
   // Initialize glue to defaults
   glue = STexGlue();

   // 1. Parse base dimension (required)
   float fSize = 0.0f;
   string sOrigUnits;
   if (!ConsumeDimension(elcapAny, fSize, sOrigUnits)) {
      m_TexParser.SetError(m_nTkIdx, "Expected dimension for \\hskip");
      return false;
   }
   if(sOrigUnits.empty()) {
      m_TexParser.SetError(m_nTkIdx, "Units are required in \\hskip command");
      return false;
   }

   // Convert to EM units (your glue uses EM)
   float fFontSizePts = m_TexParser.Doc().DefaultFontSizePts();
   float fSizeEM = otfUnitsPerEm * fSize/fFontSizePts;
   glue.fNorm = fSizeEM;
   glue.fActual = fSizeEM;

   // 2. Check for "plus" stretch component
   const STexToken* pTkNext = GetToken(m_nTkIdx);
   if (m_TexParser.TokenText(pTkNext) == "plus") {
      ++m_nTkIdx;
      if (!_ConsumeGlueComponent(glue.fStretchCapacity, glue.nStretchOrder)) {
         m_TexParser.SetError(m_nTkIdx, "Invalid stretch component after 'plus'");
         return false;
      }
      pTkNext = GetToken(m_nTkIdx);
   }

   // 3. Check for "minus" shrink component
   if (m_TexParser.TokenText(pTkNext) == "minus") {
      ++m_nTkIdx;
      if (!_ConsumeGlueComponent(glue.fShrinkCapacity, glue.nShrinkOrder)) {
         m_TexParser.SetError(m_nTkIdx, "Invalid shrink component after 'minus'");
         return false;
      }
   }

   return true;
}
uint32_t CParserAdapter::ConsumeColor(EnumLCATParenthesis eParens) {
   if (!_CanConsumeToken(eParens))
      return 0; //caller must set error!
   const STexToken* pTkNext = GetToken(m_nTkIdx);
   int nTkStart = m_nTkIdx;
   if (eParens != elcapAny) {//group
      ++nTkStart; //skip opening
      pTkNext = GetToken(nTkStart);
   }
   int nTkEnd = nTkStart + 1;
   string sText;
   if (!_GetText(nTkStart, nTkEnd, sText))
      return 0; //error, caller must set error!
   uint32_t nARGB = _GetColorByName(sText);
   if (nARGB == 0) {
      m_TexParser.SetError(m_nTkIdx, "Unknown color '" + sText + "' !");
      return 0;
   }
   if (eParens != elcapAny) {
      //check we are at closing!
      pTkNext = GetToken(m_nTkIdx);
      if (nTkEnd != pTkNext->nTkIdxEnd)
         return 0; //not at closing, some extra tokens in the group?
      m_nTkIdx = pTkNext->nTkIdxEnd+1; //move to next token on success
   }
   else
      m_nTkIdx = nTkEnd; //move to next token on success
   return nARGB;
}
// Helper: Parse stretch/shrink component (can be dimension or fil/fill/filll)
bool CParserAdapter::_ConsumeGlueComponent(OUT float& fValue, OUT uint16_t& nOrder) {
   fValue = 0.0f;
   nOrder = 0;

   const STexToken* pTok = GetToken(m_nTkIdx);
   if (!pTok) return false;

   // Try to consume dimension
   float fDim = 0.0f;
   string sOrigUnits;
   if (!ConsumeDimension(elcapAny, fDim, sOrigUnits) || sOrigUnits.empty())
      return false;
   // Check if followed by fil/fill/filll
   pTok = GetToken(m_nTkIdx);
   if (pTok && pTok->nType == ettALNUM) {
      string sUnit = m_TexParser.TokenText(pTok);

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
   float fFontSizePts = m_TexParser.Doc().DefaultFontSizePts();
   fValue = otfUnitsPerEm*PTS2DIPS(fDim)/fFontSizePts;
   return true;
}
