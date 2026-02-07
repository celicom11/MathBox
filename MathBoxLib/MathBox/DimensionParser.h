#pragma once
#include "MathItem.h"

class CTexParser;

// Static/stateless helper for parsing length/dimension from tokens:
// [{][-]#...#[.#...#]{unit | \varname}[}] 
class CDimensionParser {
public:
   // Returns next token index on success, -1 on failure
   // Handles optional parentheses (eParens) and both literal and variable-based dimensions
   static int _ConsumeDimension( CTexParser& parser, int nStartTkIdx, EnumLCATParenthesis eParens,
      OUT float& fPts, OUT string& sOrigUnits, OUT string& sError);

   // Parse dimension from string value: [-]#...#[.#...#]{unit | \varname}
   // Used for stored variable values (e.g., from \setlength)
   static bool _ParseDimensionString( const string& sValue, float fDocFontSizePts, OUT float& fPts,
      OUT string& sOrigUnits, OUT string& sError);
};