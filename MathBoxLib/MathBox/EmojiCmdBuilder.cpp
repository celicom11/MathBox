#include "stdafx.h"
#include "EmojiCmdBuilder.h"
#include "WordItem.h"

namespace {
   struct SEmojiName {
      PCSTR szName;
      uint32_t nUnicode;
   };
   const SEmojiName _aEmojiNames[] = {
      { "grinning-face", 0x1F600 }, { "grinning", 0x1F600 },
      { "smile", 0x1F604 }, { "grinning-face-with-smiling-eyes", 0x1F604 },
      { "slightly-smiling-face", 0x1F642 }, { "wink", 0x1F609 },
      { "winking-face", 0x1F609 }, { "thinking-face", 0x1F914 },
      { "warning", 0x26A0 }, { "check-mark", 0x2713 }, { "checkmark", 0x2713 },
      { "cross-mark", 0x274C }, { "red-heart", 0x2764 }, { "heart", 0x2764 },
      { "thumbs-up", 0x1F44D }, { "thumbs-down", 0x1F44E },
      { "light-bulb", 0x1F4A1 }, { "star", 0x2B50 },
   };
}
bool CEmojiCmdBuilder::CanTakeCommand(PCSTR szCmd) const {
   return szCmd && (strcmp(szCmd, "emoji") == 0 || strcmp(szCmd, "\\emoji") == 0);
}
CMathItem* CEmojiCmdBuilder::BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) {
   _ASSERT_RET(CanTakeCommand(szCmd) && pParser, nullptr);
   string sText, sName;
   EnumTokenType eType = pParser->GetTokenData(sText);
   while (eType == ettSPACE || eType == ettCOMMENT) {
      pParser->SkipToken();
      eType = pParser->GetTokenData(sText);
   }
   if (eType != ettFB_OPEN) {
      pParser->SetError("Expected {name} after \\emoji");
      return nullptr;
   }
   pParser->SkipToken();
   eType = pParser->GetTokenData(sText);
   while (eType == ettALNUM || (eType == ettNonALPHA && sText == "-")) {
      sName += sText;
      pParser->SkipToken();
      eType = pParser->GetTokenData(sText);
   }
   if (eType != ettFB_CLOSE || sName.empty()) {
      pParser->SetError("Expected an emoji name inside {braces}");
      return nullptr;
   }
   uint32_t nUnicode = 0;
   for (const SEmojiName& emoji : _aEmojiNames) {
      if (sName == emoji.szName) {
         nUnicode = emoji.nUnicode;
         break;
      }
   }
   if (!nUnicode) {
      pParser->SetError("Unknown emoji name '" + sName + "'");
      return nullptr;
   }
   if (pParser->Doc().FontManager().FontCount() <= FONT_SYMBOL) {
      pParser->SetError("Host symbol font is unavailable for \\emoji");
      return nullptr;
   }
   const SParserContext& ctx = pParser->GetContext();
   CWordItem* pWord = new CWordItem(pParser->Doc(), FONT_SYMBOL, ctx.currentStyle, eacWORD, ctx.fUserScale);
   if (!pWord->SetText({ nUnicode })) {
      delete pWord;
      pParser->SetError("Failed to measure emoji '" + sName + "'");
      return nullptr;
   }
   pParser->SkipToken(); // closing brace
   return pWord;
}
