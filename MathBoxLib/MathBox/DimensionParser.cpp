#include "stdafx.h"
#include "DimensionParser.h"
#include "TexParser.h"

namespace { // static helpers
   bool _IsDigit(char cC) {
      return (cC >= '0' && cC <= '9');
   }

   bool _IsKnownUnit(const string& sUnit) {
      static const vector<string> s_vKnownUnits{
         "pt", "cm", "mm", "in", "em", "ex", "mu"
      };
      for (const string& sKU : s_vKnownUnits) {
         if (sUnit == sKU)
            return true;
      }
      return false;
   }

   bool _ConvertToPts(IN OUT float& fPts, const string& sUnit, float fDocFontSizePts) {
      if (sUnit == "pt")
         ; // already in points
      else if (sUnit == "cm")
         fPts *= 72.0f / 2.54f;
      else if (sUnit == "mm")
         fPts *= 72.0f / 25.4f;
      else if (sUnit == "in")
         fPts *= 72.0f;
      else if (sUnit == "em")
         fPts *= fDocFontSizePts;
      else if (sUnit == "ex")
         fPts *= fDocFontSizePts / 2.0f;
      else if (sUnit == "mu")
         fPts *= fDocFontSizePts / 18.0f;  // 1mu = 1/18 em
      else
         return false; // unknown unit
      return true;
   }

   // Extract number and optional unit from a single token text
   // e.g., "12.5pt" -> fNum=12.5, sUnit="pt"
   // e.g., "1000" -> fNum=1000, sUnit="" (unitless)
   bool _GetDimen(const string& sText, OUT float& fNum, OUT string& sUnit) {
      _ASSERT_RET(!sText.empty(), false);
      bool bHasDot = false;
      bool bHasSign = false;
      PCSTR szPos = sText.c_str();

      // Handle optional leading sign
      if (*szPos == '-' || *szPos == '+') {
         bHasSign = true;
         ++szPos;
      }

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
         sUnit = sText.substr(szPos - sText.c_str());
      else
         sUnit.clear(); // No unit found - will default to em

      string sFNum = sText.substr(0, szPos - sText.c_str());
      if (sFNum.empty() || (bHasSign && sFNum.length() == 1))
         return false;

      try {
         fNum = stof(sFNum);
      }
      catch (...) {
         return false;
      }
      return true;
   }

   bool _CanConsumeToken(const CTexParser& parser, int nIdx, EnumLCATParenthesis eParens) {
      const STexToken* pTkNext = parser.GetToken(nIdx);

      // Skip space token
      if (pTkNext && pTkNext->nType == ettSPACE)
         pTkNext = parser.GetToken(nIdx + 1);

      if (!pTkNext)
         return false;

      if (eParens == elcapSquare)
         return (pTkNext->nType == ettSB_OPEN && pTkNext->nTkIdxEnd > (uint32_t)nIdx);

      if (eParens == elcapFig)
         return (pTkNext->nType == ettFB_OPEN && pTkNext->nTkIdxEnd > (uint32_t)nIdx);

      // elcapAny
      return (pTkNext->nType > ettUndef && pTkNext->nType != ettFB_CLOSE &&
         pTkNext->nType != ettCOMMENT && pTkNext->nType != ettAMP);
   }

   // Skip leading spaces and group opening
   // Returns: adjusted token index and group end position (-1 if no group)
   bool _SkipLeadingTokens(const CTexParser& parser, IN OUT int& nTkIdx,
      OUT int& nTkGroupEnd) {
      nTkGroupEnd = -1;
      const STexToken* pTkNext = parser.GetToken(nTkIdx);

      // Skip leading space token
      if (pTkNext && pTkNext->nType == ettSPACE)
         pTkNext = parser.GetToken(++nTkIdx);

      // Handle optional group opening
      if (pTkNext && pTkNext->nTkIdxEnd > 0) { // it's a group
         nTkGroupEnd = pTkNext->nTkIdxEnd;
         // skip opening bracket
         pTkNext = parser.GetToken(++nTkIdx);

         // Skip space after opening
         if (pTkNext && pTkNext->nType == ettSPACE)
            pTkNext = parser.GetToken(++nTkIdx);
      }

      return (pTkNext != nullptr);
   }

   // Parse optional sign token (+ or -)
   // Returns: adjusted token index and whether value should be negative
   bool _ParseSign(CTexParser& parser, IN OUT int& nTkIdx, OUT bool& bNegative) {
      bNegative = false;
      const STexToken* pTkNext = parser.GetToken(nTkIdx);

      if (pTkNext && pTkNext->nType == ettNonALPHA) {
         string sSign = parser.TokenText(nTkIdx);
         if (sSign == "-") {
            bNegative = true;
            // Skip space token after sign (if any)
            pTkNext = parser.GetToken(++nTkIdx);
            if (pTkNext && pTkNext->nType == ettSPACE)
               ++nTkIdx;
            return true;
         }
         else if (sSign == "+") {
            // Skip space token after sign (if any)
            pTkNext = parser.GetToken(++nTkIdx);
            if (pTkNext && pTkNext->nType == ettSPACE)
               ++nTkIdx;
            return true;
         }
      }
      return true;
   }

   // Parse decimal number from tokens: # or #.#
   // Returns: next token index or -1 on error
   int _ParseDecimalNumber(CTexParser& parser,  int nTkIdx, OUT float& fValue, OUT string& sUnit, OUT string& sError) {
      const STexToken* pTkNext = parser.GetToken(nTkIdx);
      if (!pTkNext || pTkNext->nType != ettALNUM) {
         sError = "Expected numeric value";
         return -1;
      }

      string sNum = parser.TokenText(nTkIdx);
      if (!_GetDimen(sNum, fValue, sUnit)) {
         sError = "Invalid numeric value";
         return -1;
      }

      ++nTkIdx; // consume number token

      // If no unit found in the token, check for decimal point
      if (sUnit.empty()) {
         pTkNext = parser.GetToken(nTkIdx);

         if (pTkNext && pTkNext->nType == ettNonALPHA &&
            parser.TokenText(nTkIdx) == ".") {
            pTkNext = parser.GetToken(++nTkIdx);

            if (!pTkNext || pTkNext->nType != ettALNUM) {
               sError = "Expected fractional part after '.'";
               return -1;
            }

            sNum = sNum + "." + parser.TokenText(nTkIdx);
            if (!_GetDimen(sNum, fValue, sUnit)) {
               sError = "Invalid numeric value";
               return -1;
            }

            ++nTkIdx;
         }
      }

      // Skip space token after number (if any)
      pTkNext = parser.GetToken(nTkIdx);
      if (pTkNext && pTkNext->nType == ettSPACE)
         ++nTkIdx;
      return nTkIdx;
   }

   // Parse unit token if present (handles spaces before unit)
   // Returns: next token index
   int _ParseUnitToken(CTexParser& parser, int nTkIdx, IN OUT string& sUnit) {
      if (sUnit.empty()) {
         const STexToken* pTkNext = parser.GetToken(nTkIdx);
         if (pTkNext && pTkNext->nType == ettALNUM) {
            string sPotentialUnit = parser.TokenText(nTkIdx);
            // Only consume as unit if it's a known unit
            if (_IsKnownUnit(sPotentialUnit)) {
               sUnit = sPotentialUnit;
               // Skip space token after unit (if any)
               pTkNext = parser.GetToken(++nTkIdx);
               if (pTkNext && pTkNext->nType == ettSPACE)
                  ++nTkIdx;
            }
         }
      }
      return nTkIdx;
   }

   // Validate and convert unit to points
   // If sUnit is empty, value is unitless (no conversion)
   bool _ValidateAndConvertUnit(const string& sUnit, float fDocFontSizePts, IN OUT float& fValue, OUT string& sError) {
      if (sUnit.empty()) {
         // No unit - keep raw numeric value (unitless)
         return true;
      }

      if (!_IsKnownUnit(sUnit)) {
         sError = "Unknown unit: " + sUnit;
         return false;
      }

      if (!_ConvertToPts(fValue, sUnit, fDocFontSizePts)) {
         _ASSERT_RET(0, false); // snbh
         sError = "Failed to convert unit to points";
         return false;
      }

      return true;
   }

   // Parse dimension from string value: [-]#...#[.#...#][{unit}]
   // e.g., "12.5pt", "-3em", "1000" (unitless)
   bool _ParseDimensionFromString(const string& sVarValue, float fDocFontSizePts, 
                                  OUT float& fPts, OUT string& sOrigUnits, OUT string& sError) {
      fPts = 0.0f;
      sOrigUnits.clear();
      
      if (sVarValue.empty()) {
         sError = "Empty dimension value";
         return false;
      }

      string sValue = sVarValue;
      bool bNegative = false;

      // Check for leading sign
      if (sValue[0] == '-') {
         bNegative = true;
         sValue = sValue.substr(1);
      }
      else if (sValue[0] == '+') {
         sValue = sValue.substr(1);
      }

      if (sValue.empty()) {
         sError = "Invalid dimension value";
         return false;
      }

      // Extract number and unit
      string sUnit;
      if (!_GetDimen(sValue, fPts, sUnit)) {
         sError = "Invalid numeric value in dimension";
         return false;
      }

      // Store original unit (empty if unitless)
      sOrigUnits = sUnit;

      // Convert to points - if no unit, value is unitless (no conversion)
      if (!sUnit.empty()) {
         if (!_IsKnownUnit(sUnit)) {
            sError = "Unknown unit: " + sUnit;
            return false;
         }

         if (!_ConvertToPts(fPts, sUnit, fDocFontSizePts)) {
            sError = "Failed to convert unit to points";
            return false;
         }
      }
      // else: unitless - keep raw numeric value

      if (bNegative)
         fPts = -fPts;

      return true;
   }
}

bool CDimensionParser::_ParseDimensionString( const string& sValue, float fDocFontSizePts, 
                                       OUT float& fPts, OUT string& sOrigUnits, OUT string& sError) {
   return _ParseDimensionFromString(sValue, fDocFontSizePts, fPts, sOrigUnits, sError);
}

int CDimensionParser::_ConsumeDimension( CTexParser& parser, int nStartTkIdx, EnumLCATParenthesis eParens,
                                      OUT float& fPts, OUT string& sOrigUnits, OUT string& sError) {
fPts = 0.0f;
sOrigUnits.clear();
sError.clear();
if (!_CanConsumeToken(parser, nStartTkIdx, eParens)) {
   sError = "Expected dimension value";
   return -1;
}

int nTkIdx = nStartTkIdx;
int nTkGroupEnd = -1;

// Skip leading spaces and handle group opening
if (!_SkipLeadingTokens(parser, nTkIdx, nTkGroupEnd)) {
   sError = "Unexpected end of tokens";
   return -1;
}

// Parse optional sign
bool bNegative = false;
if (!_ParseSign(parser, nTkIdx, bNegative)) {
   sError = "Invalid sign token";
   return -1;
}

const STexToken* pTkNext = parser.GetToken(nTkIdx);
if (!pTkNext) {
   sError = "Expected number after sign";
   return -1;
}

// Case 1: Direct variable reference (e.g., \parindent)
// Variables store float values directly (in points)
if (pTkNext->nType == ettCOMMAND) {
   string sVarName = parser.TokenText(nTkIdx);
   ++nTkIdx;

   // Skip space token after variable name (if any)
   pTkNext = parser.GetToken(nTkIdx);
   if (pTkNext && pTkNext->nType == ettSPACE) {
      ++nTkIdx;
   }

   // Get variable value (stored as float in points)
   float fVarPts = 0.0f;
   if (!parser.GetLength(sVarName, fVarPts)) {
      sError = "Unknown length variable: " + sVarName;
      return -1;
   }

   fPts = fVarPts;
   if (bNegative)
      fPts = -fPts;

   // Variables have implicit units (stored in points), so set sOrigUnits
   sOrigUnits = "pt";

   // Skip trailing space token before closing bracket (if any)
   pTkNext = parser.GetToken(nTkIdx);
   if (pTkNext && pTkNext->nType == ettSPACE) {
      ++nTkIdx;
   }

   // Move past closing bracket if in group
   if (nTkGroupEnd > 0) {
      if (nTkIdx != nTkGroupEnd) {
         sError = "Extra tokens in dimension group";
         return -1;
      }
      ++nTkIdx;
   }

   return nTkIdx;
}

   // Case 2: Numeric value (with optional unit or followed by variable)
   if (pTkNext->nType != ettALNUM) {
      sError = "Expected number or length variable";
      return -1;
   }

   // Parse decimal number
   float fMultiplier = 1.0f;
   string sUnit;
   nTkIdx = _ParseDecimalNumber(parser, nTkIdx, fMultiplier, sUnit, sError);
   if (nTkIdx < 0)
      return -1;

   // Parse unit token if not embedded in number
   nTkIdx = _ParseUnitToken(parser, nTkIdx, sUnit);

   // Skip space token after unit (if any)
   pTkNext = parser.GetToken(nTkIdx);
   if (pTkNext && pTkNext->nType == ettSPACE) 
      pTkNext = parser.GetToken(++nTkIdx);

   // Check if followed by variable reference (e.g., 2.5\baselineskip)
   if (sUnit.empty() && pTkNext && pTkNext->nType == ettCOMMAND) {
      string sVarName = parser.TokenText(nTkIdx);
      ++nTkIdx;

      // Skip space token after variable name (if any)
      pTkNext = parser.GetToken(nTkIdx);
      if (pTkNext && pTkNext->nType == ettSPACE) {
         ++nTkIdx;
      }

      // Get variable value (stored as float in points)
      float fVarPts = 0.0f;
      if (!parser.GetLength(sVarName, fVarPts)) {
         sError = "Unknown length variable: " + sVarName;
         return -1;
      }

      fPts = fMultiplier * fVarPts;
      if (bNegative)
         fPts = -fPts;

      // Multiplier with variable has implicit units
      sOrigUnits = "pt";

      // Skip trailing space token before closing bracket (if any)
      pTkNext = parser.GetToken(nTkIdx);
      if (pTkNext && pTkNext->nType == ettSPACE) {
         ++nTkIdx;
      }

      // Move past closing bracket if in group
      if (nTkGroupEnd > 0) {
         if (nTkIdx != nTkGroupEnd) {
            sError = "Extra tokens in dimension group";
            return -1;
         }
         ++nTkIdx;
      }

      return nTkIdx;
   }

   // Validate and convert unit to points (or keep unitless if no unit)
   float fDocFontSizePts = parser.Doc().DefaultFontSizePts();
   if (!_ValidateAndConvertUnit(sUnit, fDocFontSizePts, fMultiplier, sError))
      return -1;

   // Store original unit (empty if unitless)
   sOrigUnits = sUnit;

   fPts = fMultiplier;
   if (bNegative)
      fPts = -fPts;

   // Skip trailing space token before closing bracket (if any)
   pTkNext = parser.GetToken(nTkIdx);
   if (pTkNext && pTkNext->nType == ettSPACE)
      ++nTkIdx;

   // Validate we're at the expected end position
   if (nTkGroupEnd > 0) {
      if (nTkIdx != nTkGroupEnd) {
         sError = "Extra tokens in dimension group";
         return -1;
      }
      ++nTkIdx; // move past closing bracket
   }

   return nTkIdx;
}