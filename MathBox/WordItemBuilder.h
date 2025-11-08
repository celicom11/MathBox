#pragma once
#include "MathItem.h"
class CHBoxItem;
//Factory for a Indexed (subscript/superscript) item
class CWordItemBuilder {
public:
   //sCmd: word/text with optional text-font command: [\{text|textit|mathnormal|...}]word
   static CMathItem* BuildText(const string& sFontCmd, const string& sText, const CMathStyle& style, float fUserScale = 1.0f);
   static CMathItem* BuildMathWord(const string& sFontCmd, const string& sWord, bool bNumber, const CMathStyle& style, float fUserScale = 1.0f);
   static CMathItem* BuildTeXSymbol(const string& sFontCmd, const string& sSym, const CMathStyle& style, float fUserScale = 1.0f);
   static bool BuildMathText(const string& sFontCmd, const string& sText, const CMathStyle& style,
      OUT CHBoxItem& hbox, float fUserScale = 1.0f);
   static bool _IsNumber(PCSTR szText);
};