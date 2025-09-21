#pragma once
#include "MathItem.h"

//Factory for a Indexed (subscript/superscript) item
class CWordItemBuilder {
public:
   //sCmd: word/text with optional text-font command: [\{text|textit|mathnormal|...}]word
   static CMathItem* BuildWordItem(const string& sCmd, const string& sArg, const CMathStyle& style, float fUserScale);

};