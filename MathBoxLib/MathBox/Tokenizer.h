#pragma once
#include "MathItem.h"

class CTokenizer {
   const bool     m_bIsMacroText{ false }; // true = tokenizing macros text
   string         m_sText;
public:
//CTOR/DTOR
   CTokenizer() = delete;
   CTokenizer(const string& sText, bool bIsMacroText = false) :
      m_bIsMacroText(bIsMacroText), m_sText(sText) {}
//ATTS
   const string& Text() const { return m_sText; }
//METHODS
   string TokenText(const STexToken& token) const {
      return m_sText.substr(token.nPos, token.nLen);
   }
   bool Tokenize(OUT vector<STexToken>& vTokens, OUT ParserError& err);
};
