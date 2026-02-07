#pragma once
#include "MathItem.h"
class CHBoxItem;
//Factory for a Indexed (subscript/superscript) item
class CWordItemBuilder {
public:
   static bool _IsNumber(PCSTR szText);
   static CMathItem* _BuildText(IDocParams& doc, const string& sText, const SParserContext& ctx);
   static CMathItem* _BuildMathWord(IDocParams& doc, const string& sWord, bool bNumber, const SParserContext& ctx);
   //static CMathItem* BuildTeXSymbol(const string& sFontCmd, const string& sSym, const CMathStyle& style, float fUserScale = 1.0f);
   //static bool BuildMathText(const string& sFontCmd, const string& sText, const CMathStyle& style, OUT CHBoxItem& hbox, float fUserScale = 1.0f);
};