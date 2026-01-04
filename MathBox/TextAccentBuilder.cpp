#include "stdafx.h"
#include "TextAccentBuilder.h"
#include "WordItem.h"
#include "FontStyleHelper.h"

namespace {
   struct SAccMap {
      char cCmd{ 0 };
      char cChar{ 0 };
      uint32_t    nUnicode{ 0 };
   };
   vector<SAccMap> _vAccents{
      // Grave \`
      {'`', 'a', 0x00E0}, {'`', 'A', 0x00C0},
      {'`', 'e', 0x00E8}, {'`', 'E', 0x00C8},
      {'`', 'i', 0x00EC}, {'`', 'I', 0x00CC},
      {'`', 'o', 0x00F2}, {'`', 'O', 0x00D2},
      {'`', 'u', 0x00F9}, {'`', 'U', 0x00D9},

      // Acute \'
      {'\'', 'a', 0x00E1}, {'\'', 'A', 0x00C1},
      {'\'', 'e', 0x00E9}, {'\'', 'E', 0x00C9},
      {'\'', 'i', 0x00ED}, {'\'', 'I', 0x00CD},
      {'\'', 'o', 0x00F3}, {'\'', 'O', 0x00D3},
      {'\'', 'u', 0x00FA}, {'\'', 'U', 0x00DA},
      {'\'', 'y', 0x00FD}, {'\'', 'Y', 0x00DD},
      {'\'', 'c', 0x0107}, {'\'', 'C', 0x0106},
      {'\'', 'n', 0x0144}, {'\'', 'N', 0x0143},
      {'\'', 's', 0x015B}, {'\'', 'S', 0x015A},
      {'\'', 'z', 0x017A}, {'\'', 'Z', 0x0179},
      {'\'', 'l', 0x013A}, {'\'', 'L', 0x0139},
      {'\'', 'r', 0x0155}, {'\'', 'R', 0x0154},

      // Circumflex \^
      {'^', 'a', 0x00E2}, {'^', 'A', 0x00C2},
      {'^', 'e', 0x00EA}, {'^', 'E', 0x00CA},
      {'^', 'i', 0x00EE}, {'^', 'I', 0x00CE},
      {'^', 'o', 0x00F4}, {'^', 'O', 0x00D4},
      {'^', 'u', 0x00FB}, {'^', 'U', 0x00DB},
      {'^', 'w', 0x0175}, {'^', 'W', 0x0174},
      {'^', 'y', 0x0177}, {'^', 'Y', 0x0176},

      // Umlaut/Dieresis \"
      {'"', 'a', 0x00E4}, {'"', 'A', 0x00C4},
      {'"', 'e', 0x00EB}, {'"', 'E', 0x00CB},
      {'"', 'i', 0x00EF}, {'"', 'I', 0x00CF},
      {'"', 'o', 0x00F6}, {'"', 'O', 0x00D6},
      {'"', 'u', 0x00FC}, {'"', 'U', 0x00DC},
      {'"', 'y', 0x00FF}, {'"', 'Y', 0x0178},

      // Tilde \~
      {'~', 'a', 0x00E3}, {'~', 'A', 0x00C3},
      {'~', 'n', 0x00F1}, {'~', 'N', 0x00D1},
      {'~', 'o', 0x00F5}, {'~', 'O', 0x00D5},
      {'~', 'i', 0x0129}, {'~', 'I', 0x0128},
      {'~', 'u', 0x0169}, {'~', 'U', 0x0168},

      // Macron \=
      {'=', 'a', 0x0101}, {'=', 'A', 0x0100},
      {'=', 'e', 0x0113}, {'=', 'E', 0x0112},
      {'=', 'i', 0x012B}, {'=', 'I', 0x012A},
      {'=', 'o', 0x014D}, {'=', 'O', 0x014C},
      {'=', 'u', 0x016B}, {'=', 'U', 0x016A},

      // Dot over \.
      {'.', 'c', 0x010B}, {'.', 'C', 0x010A},
      {'.', 'e', 0x0117}, {'.', 'E', 0x0116},
      {'.', 'g', 0x0121}, {'.', 'G', 0x0120},
      {'.', 'I', 0x0130}, // Capital I with dot
      {'.', 'z', 0x017C}, {'.', 'Z', 0x017B},

      // Breve \u
      {'u', 'a', 0x0103}, {'u', 'A', 0x0102},
      {'u', 'e', 0x0115}, {'u', 'E', 0x0114},
      {'u', 'g', 0x011F}, {'u', 'G', 0x011E},
      {'u', 'i', 0x012D}, {'u', 'I', 0x012C},
      {'u', 'o', 0x014F}, {'u', 'O', 0x014E},
      {'u', 'u', 0x016D}, {'u', 'U', 0x016C},

      // Caron/Hacek \v
      {'v', 'c', 0x010D}, {'v', 'C', 0x010C},
      {'v', 'd', 0x010F}, {'v', 'D', 0x010E},
      {'v', 'e', 0x011B}, {'v', 'E', 0x011A},
      {'v', 'l', 0x013E}, {'v', 'L', 0x013D},
      {'v', 'n', 0x0148}, {'v', 'N', 0x0147},
      {'v', 'r', 0x0159}, {'v', 'R', 0x0158},
      {'v', 's', 0x0161}, {'v', 'S', 0x0160},
      {'v', 't', 0x0165}, {'v', 'T', 0x0164},
      {'v', 'z', 0x017E}, {'v', 'Z', 0x017D},

      // Double Acute \H
      {'H', 'o', 0x0151}, {'H', 'O', 0x0150},
      {'H', 'u', 0x0171}, {'H', 'U', 0x0170},

      // Cedilla \c
      {'c', 'c', 0x00E7}, {'c', 'C', 0x00C7},
      {'c', 's', 0x015F}, {'c', 'S', 0x015E},
      {'c', 't', 0x0163}, {'c', 'T', 0x0162},
      {'c', 'n', 0x0146}, {'c', 'N', 0x0145},
      {'c', 'g', 0x0123}, {'c', 'G', 0x0122},
      {'c', 'k', 0x0137}, {'c', 'K', 0x0136},
      {'c', 'l', 0x013C}, {'c', 'L', 0x013B},
      {'c', 'r', 0x0157}, {'c', 'R', 0x0156},

      // Ogonek \k
      {'k', 'a', 0x0105}, {'k', 'A', 0x0104},
      {'k', 'e', 0x0119}, {'k', 'E', 0x0118},
      {'k', 'i', 0x012F}, {'k', 'I', 0x012E},
      {'k', 'u', 0x0173}, {'k', 'U', 0x0172},

      // Ring over \r
      {'r', 'a', 0x00E5}, {'r', 'A', 0x00C5},
      {'r', 'u', 0x016F}, {'r', 'U', 0x016E},

      // Dot under \d
      {'d', 'a', 0x1EA1}, {'d', 'A', 0x1EA0},
      {'d', 'e', 0x1EB9}, {'d', 'E', 0x1EB8},
      {'d', 'i', 0x1ECB}, {'d', 'I', 0x1ECA},
      {'d', 'o', 0x1ECD}, {'d', 'O', 0x1ECC},
      {'d', 'u', 0x1EE5}, {'d', 'U', 0x1EE4},
      {'d', 'y', 0x1EF5}, {'d', 'Y', 0x1EF4}
   };
}

bool CTextAccentBuilder::CanTakeCommand(PCSTR szCmd) const {
   // text mode + font commands: \text,\textit,etc.
   if (szCmd[0] != '\\')
      return false;
   szCmd++; //skip '\'
   char cCmd = *szCmd;
   if (!cCmd)
      return false;
   if (szCmd[1])
      return false;//single accent char is expected
   constexpr PCSTR _szAccents = "`'^\"~=.uvHckrdt";
   return strchr(_szAccents, cCmd) != nullptr;
}
CMathItem* CTextAccentBuilder::BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) {
   _ASSERT_RET(szCmd && pParser, nullptr);
   const SParserContext& ctx = pParser->GetContext();
   _ASSERT_RET(CanTakeCommand(szCmd), nullptr);
   ++szCmd; //skip '\'
   //build unicode vector
   vector<uint32_t> vUnicode;
   string sTkText;
   EnumTokenType ettNext = pParser->GetTokenData(sTkText);
   if (ettNext != ettFB_OPEN) {
      pParser->SetError("Missing {arg} for '\\" + string(szCmd) + "' command");
      return nullptr;
   }
   pParser->SkipToken();
   ettNext = pParser->GetTokenData(sTkText);

   if (*szCmd == 't') {
      //tie after accent
      if (ettNext != ettALNUM || (sTkText.size() != 1 && sTkText.size() != 2)) {
         pParser->SetError("Expected one or two ASCII chars as arguments for '\\t{..}' command");
         return nullptr;
      }
      vUnicode.push_back(sTkText[0]);
      vUnicode.push_back(0x0311); //NO cmb double inverted breve, 0x0361 in LMR!
      if(sTkText.size() == 2)
         vUnicode.push_back(sTkText[1]);
      pParser->SkipToken(); //arg + }
      pParser->SkipToken();
   }
   else {
      if (ettNext != ettALNUM || sTkText.size() != 1) {
         pParser->SetError("Expected one char as argument for '\\" + string(szCmd) + "{.}' command");
         return nullptr;
      }
      //find accent
      for (const SAccMap& accmap : _vAccents) {
         if (accmap.cCmd == *szCmd && accmap.cChar == sTkText[0]) {
            vUnicode.push_back(accmap.nUnicode);
            break;//found
         }
      }
      if (vUnicode.empty()) {
         //use combining glyphs if possible
         if (*szCmd == 'H') {
            vUnicode.push_back(sTkText[0]);
            vUnicode.push_back(0x30B); // COMBINING DOUBLE ACUTE ACCENT
         }
         else if (*szCmd == '.') {
            vUnicode.push_back(sTkText[0]);
            vUnicode.push_back(0x307); // COMBINING DOT ABOVE
         }
         else if (*szCmd == 'v') {
            vUnicode.push_back(sTkText[0]);
            vUnicode.push_back(0x30C); // COMBINING CARON
         }
         else {
            pParser->SetError("Unknown accent command: '\\" + string(szCmd) + "{" + sTkText + "}'");
            return nullptr;
         }
      }
      pParser->SkipToken(); //arg + }
      pParser->SkipToken();
   }
   if (vUnicode.empty())
      return nullptr;
   STextFontStyle tfStyle;
   _ASSERT_RET(FontStyleHelper::_GetTextFontStyle(ctx.sFontCmd, tfStyle), nullptr);
   int16_t nFontIdx = tfStyle.nLetterDigitsFont == FONT_DOC ? FONT_ROMAN_REGULAR : tfStyle.nLetterDigitsFont;
   CWordItem* pWord = new CWordItem(pParser->Doc(), nFontIdx, ctx.currentStyle, eacWORD, ctx.fUserScale);
   pWord->SetText(vUnicode);
   return pWord;
}
