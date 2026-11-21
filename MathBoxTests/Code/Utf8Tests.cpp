#include "stdafx.h"
#include "Tokenizer.h"

namespace Utf8Tests {
   TEST_CLASS(Utf8Tests) {
   public:
      TEST_METHOD(Utf8_ContiguousCharactersHaveByteSpansAndAsciiBoundaries) {
         const string sWord = "\xD0\x96\xE4\xB8\xAD\xF0\x9F\x98\x80";
         CTokenizer tokenizer("abc" + sWord + ",x");
         vector<STexToken> vTokens;
         ParserError error;
         Assert::IsTrue(tokenizer.Tokenize(vTokens, error));
         Assert::AreEqual((size_t)4, vTokens.size());
         Assert::AreEqual((int)ettALNUM, (int)vTokens[0].nType);
         Assert::AreEqual((int)ettUTF8, (int)vTokens[1].nType);
         Assert::AreEqual((int32_t)3, vTokens[1].nPos);
         Assert::AreEqual((uint16_t)9, vTokens[1].nLen);
         Assert::AreEqual(sWord, tokenizer.TokenText(vTokens[1]));
         Assert::AreEqual((int)ettNonALPHA, (int)vTokens[2].nType);
         Assert::AreEqual((int32_t)12, vTokens[2].nPos);
      }
      TEST_METHOD(Utf8_RejectsInvalidScalarsAndTruncatedSequences) {
         const vector<string> vInvalid = { "\x80", "\xC0\xAF", "\xC2", "\xE4\xB8",
            "\xED\xA0\x80", "\xF4\x90\x80\x80", "\xF0\x80\x80\x80", "\xE2!\xA1" };
         for (const string& sInvalid : vInvalid) {
            CTokenizer tokenizer("ab" + sInvalid);
            vector<STexToken> vTokens;
            ParserError error;
            Assert::IsFalse(tokenizer.Tokenize(vTokens, error));
            Assert::AreEqual((uint32_t)2, error.nStartPos);
            Assert::AreEqual((uint32_t)3, error.nEndPos);
            Assert::AreEqual((int)epsTOKENIZING, (int)error.eStage);
         }
      }
      TEST_METHOD(Utf8_CombiningCharactersAndJoinersStayInSequence) {
         const string sWord = "\xD0\x96\xCC\x81\xE2\x80\x8D\xF0\x9F\x98\x80";
         CTokenizer tokenizer("{" + sWord + "} $x$");
         vector<STexToken> vTokens;
         ParserError error;
         Assert::IsTrue(tokenizer.Tokenize(vTokens, error));
         Assert::AreEqual((int)ettFB_OPEN, (int)vTokens[0].nType);
         Assert::AreEqual((int)ettUTF8, (int)vTokens[1].nType);
         Assert::AreEqual(sWord, tokenizer.TokenText(vTokens[1]));
         Assert::AreEqual((int)ettFB_CLOSE, (int)vTokens[2].nType);
      }
      TEST_METHOD(Utf8_SourceBomAndMacroReferencesPreserveBytePositions) {
         CTokenizer tokenizer("\xEF\xBB\xBF\xD0\x96", true);
         vector<STexToken> vTokens;
         ParserError error;
         Assert::IsTrue(tokenizer.Tokenize(vTokens, error));
         Assert::AreEqual((size_t)1, vTokens.size());
         Assert::AreEqual((int32_t)3, vTokens[0].nPos);
         Assert::AreEqual((int32_t)-1, vTokens[0].nRefIdx);
      }
      TEST_METHOD(Utf8_OversizeWordFailsWithoutLengthTruncation) {
         string sWord;
         for (size_t nIdx = 0; nIdx < 32768; ++nIdx)
            sWord += "\xD0\x96";
         CTokenizer tokenizer(sWord);
         vector<STexToken> vTokens;
         ParserError error;
         Assert::IsFalse(tokenizer.Tokenize(vTokens, error));
         Assert::IsTrue(error.sError.find("maximum byte length") != string::npos);
      }
      TEST_METHOD(Utf8_ChineseJapaneseKoreanAndAsciiSyntaxHaveSeparateTokens) {
         CTokenizer tokenizer("\xE4\xB8\xAD \xE6\x97\xA5 \xED\x95\x9C\\text{x}");
         vector<STexToken> vTokens;
         ParserError error;
         Assert::IsTrue(tokenizer.Tokenize(vTokens, error));
         Assert::AreEqual((int)ettUTF8, (int)vTokens[0].nType);
         Assert::AreEqual((int)ettUTF8, (int)vTokens[2].nType);
         Assert::AreEqual((int)ettUTF8, (int)vTokens[4].nType);
         Assert::AreEqual((int)ettCOMMAND, (int)vTokens[5].nType);
         Assert::AreEqual(string("\\text"), tokenizer.TokenText(vTokens[5]));
      }
   };
}
