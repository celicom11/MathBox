#include "stdafx.h"
#include "TextSymBuilder.h"
#include "FontStyleHelper.h"
#include "WordItem.h"

//handles:  char{"}###,
//          textunderscore, textendash, textemdash, textasciitilde, textasciicircum, textellipsis,
//          textbackslash, textbraceleft, textbraceright, textdagger, textdaggerdbl, textquoteleft,
//          textquoteright, textdegree, textregistered, copyright, checkmark, backslash,
//          textquotedblleft, textquotedblright, textbullet
namespace {
   struct SCmd2Sym {
      PCSTR    szCmd;
      uint32_t nUnicode;
   };
   const SCmd2Sym _aTextSym[] = {
      { "backslash", 0x005C },
      { "aa",              0xE5 },
      { "AA",              0xC5 },
      { "ae",              0xE6 },
      { "AE",              0xC6 },
      { "oe",              0x0153 },
      { "OE",              0x0152 },
      { "ss",              0xDF },
      { "i",               0x0131 }, //dotless i
      { "j",               0x0237 }, //dotless j
      { "$",               '$' },
      { "o",               0x00F8 },
      { "O",               0x00D8 },
      { "l",               0x019A },
      { "L",               0x023D },
      { "euro",            0x20AC},
      { "yen",             0xA5 },
      { "pounds",          0xA3 },

      { "textsterling",    0xA3 },
      { "S",               0xA7 },
      { "sect",            0xA7 },
      { "P",               0xB6 },
      { "textunderscore",  0x005F },
      { "textendash",      0x2013 },
      { "textemdash",      0x2014 },
      { "textasciitilde",  0x007E },
      { "textasciicircum", 0x005E },
      { "textellipsis",    0x2026 },
      { "textbackslash",   0x005C },
      { "textbraceleft",   0x007B },
      { "textbraceright",  0x007D },
      { "textdagger",      0x2020 },
      { "textdaggerdbl",   0x2021 },
      { "textbar",         0x7C },
      { "textbardbl",      0x2016 },
      { "textquoteleft",   0x2018 },
      { "textquoteright",  0x2019 },
      { "textdegree",      0x00B0 },
      { "textregistered",  0x00AE },
      { "copyright",       0x00A9 },
      { "checkmark",       0x2713 },
      { "textquotedblleft",  0x201C },
      { "textquotedblright", 0x201D },
      { "textbullet",        0x2022 },
   };
}
uint32_t CTextSymBuilder::_GetTextSymbolUni(PCSTR szCmd) {
   _ASSERT_RET(szCmd, 0);
   if (*szCmd == '\\')
      ++szCmd;
   _ASSERT_RET(*szCmd, 0);
   string sCmdStr(szCmd);
   if (sCmdStr.substr(0, 4) == "char") {
      uint32_t nRet = 0;
      if (sCmdStr[4] == '"') {
         //parse hex
         PCSTR szHex = sCmdStr.c_str() + 5;
         nRet = std::stoul(szHex, nullptr, 16);
      }
      else {
         //parse decimal
         PCSTR szDec = sCmdStr.c_str() + 4;
         nRet = std::stoul(szDec);
      }
      return nRet;
   }
   //else
   for (const SCmd2Sym cmd2sym : _aTextSym) {
      if (sCmdStr == cmd2sym.szCmd)
         return cmd2sym.nUnicode;
   }
   return 0;
}

CMathItem* CTextSymBuilder::BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) {
   _ASSERT_RET(szCmd && pParser, nullptr);
   uint32_t nUnicode = _GetTextSymbolUni(szCmd);
   _ASSERT_RET(nUnicode, nullptr);
   const SParserContext& ctx(pParser->GetContext());
   STextFontStyle tfStyle;
   if (!ctx.sFontCmd.empty() && !FontStyleHelper::_GetTextFontStyle(ctx.sFontCmd, tfStyle)) {
      if (!pParser->HasError())
         pParser->SetError("Unknown font '" + ctx.sFontCmd + "'");
      return nullptr;
   }
   int16_t nFontIdx = tfStyle.nLetterDigitsFont == FONT_DOC ? FONT_ROMAN_REGULAR : tfStyle.nLetterDigitsFont;
   CWordItem* pWord = new CWordItem(pParser->Doc(), nFontIdx, ctx.currentStyle, eacWORD, ctx.fUserScale);
   pWord->SetText({ nUnicode });

   return pWord;
}

