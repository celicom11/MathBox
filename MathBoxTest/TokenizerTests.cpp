#include "stdafx.h"
#include "Tokenizer.h"


namespace TokenizerTests
{
   TEST_CLASS(TokenizerTests)
   {
   public:

      TEST_METHOD(Tokenize_BasicTokenTypes_ReturnCorrectTypes)
      {
         // Arrange
         CTokenizer tokenizer("abc123 + \\alpha { } [ ] %comment whatever");
         vector<STexToken> tokens;
         ParserError err;

         // Act
         bool result = tokenizer.Tokenize(tokens, err);

         // Assert
         Assert::IsTrue(result, L"Tokenization should succeed");
         Assert::AreEqual((size_t)8, tokens.size(), L"Should have 8 tokens");

         Assert::AreEqual((int)ettALNUM, (int)tokens[0].nType, L"First token should be ALPHA");
         Assert::AreEqual(string("abc123"), tokenizer.TokenText(tokens[0]), L"First token text should be 'abc'");

         Assert::AreEqual((int)ettNonALPHA, (int)tokens[1].nType, L"Second token should be NonALPHA");
         Assert::AreEqual(string("+"), tokenizer.TokenText(tokens[1]), L"Second token text should be '123'");

         Assert::AreEqual((int)ettCOMMAND, (int)tokens[2].nType, L"Third token should be COMMAND");
         Assert::AreEqual(string("\\alpha"), tokenizer.TokenText(tokens[2]), L"Third token text should be '\\alpha'");

         Assert::AreEqual((int)ettFB_OPEN, (int)tokens[3].nType, L"Fourth token should be GROUP_OPEN");
         Assert::AreEqual((int)ettFB_CLOSE, (int)tokens[4].nType, L"Fifth token should be GROUP_CLOSE");
         Assert::AreEqual((int)ettSB_OPEN, (int)tokens[5].nType, L"Sixth token should be SB_OPEN");
         Assert::AreEqual((int)ettSB_CLOSE, (int)tokens[6].nType, L"Seventh token should be SB_CLOSE");
         Assert::AreEqual((int)ettCOMMENT, (int)tokens[7].nType, L"Last token should be COMMENT");
      }

      TEST_METHOD(Tokenize_OrphanBackslash_ReturnFalse)
      {
         // Arrange
         CTokenizer tokenizer("valid text \\");
         vector<STexToken> tokens;
         ParserError err;

         // Act
         bool result = tokenizer.Tokenize(tokens, err);

         // Assert
         Assert::IsFalse(result, L"Tokenization should fail for orphan backslash");
         Assert::AreEqual((int)epsTOKENIZING, (int)err.eStage, L"Error should be in tokenization stage");
         Assert::AreEqual((uint32_t)11, err.nPosition, L"Error position should be at backslash");
         Assert::IsTrue(err.sError.find("Orphan backslash") != string::npos, L"Error message should mention orphan backslash");
      }

      TEST_METHOD(Tokenize_EmptyInput_ReturnEmptyTokens)
      {
         // Arrange
         CTokenizer tokenizer("");
         vector<STexToken> tokens;
         ParserError err;

         // Act
         bool result = tokenizer.Tokenize(tokens, err);

         // Assert
         Assert::IsTrue(result, L"Empty input should tokenize successfully");
         Assert::AreEqual((size_t)0, tokens.size(), L"Should have no tokens");
      }

      TEST_METHOD(Tokenize_EscapeSequences_CreateCorrectTokens)
      {
         // Arrange
         CTokenizer tokenizer("\\{ \\} \\% \\&");
         vector<STexToken> tokens;
         ParserError err;

         // Act
         bool result = tokenizer.Tokenize(tokens, err);

         // Assert
         Assert::IsTrue(result, L"Escape sequence parsing should succeed");
         Assert::AreEqual((size_t)4, tokens.size(), L"Should have 4 escape tokens");

         for (size_t i = 0; i < tokens.size(); i++) {
            Assert::AreEqual((int)ettNonALPHA, (int)tokens[i].nType,
               (L"Escape token " + to_wstring(i) + L" should be NonALPHA").c_str());
            Assert::AreEqual((uint16_t)2, tokens[i].nLen,
               (L"Escape token " + to_wstring(i) + L" should have length 2").c_str());
         }

         Assert::AreEqual(string("\\{"), tokenizer.TokenText(tokens[0]), L"First escape should be '\\{'");
         Assert::AreEqual(string("\\}"), tokenizer.TokenText(tokens[1]), L"Second escape should be '\\}'");
         Assert::AreEqual(string("\\%"), tokenizer.TokenText(tokens[2]), L"Third escape should be '\\%'");
         Assert::AreEqual(string("\\&"), tokenizer.TokenText(tokens[3]), L"Fourth escape should be '\\&'");
      }

      TEST_METHOD(Tokenize_Comments_ParseCorrectly)
      {
         // Arrange
         CTokenizer tokenizer("before % this is a comment\nafter");
         vector<STexToken> tokens;
         ParserError err;

         // Act
         bool result = tokenizer.Tokenize(tokens, err);

         // Assert
         Assert::IsTrue(result, L"Comment parsing should succeed");
         Assert::AreEqual((size_t)3, tokens.size(), L"Should have 3 tokens");

         Assert::AreEqual((int)ettALNUM, (int)tokens[0].nType, L"First token should be ALPHA");
         Assert::AreEqual(string("before"), tokenizer.TokenText(tokens[0]), L"First token should be 'before'");

         Assert::AreEqual((int)ettCOMMENT, (int)tokens[1].nType, L"Second token should be COMMENT");
         Assert::AreEqual(string("% this is a comment"), tokenizer.TokenText(tokens[1]), L"Comment should include entire line");

         Assert::AreEqual((int)ettALNUM, (int)tokens[2].nType, L"Third token should be ALPHA");
         Assert::AreEqual(string("after"), tokenizer.TokenText(tokens[2]), L"Third token should be 'after'");
      }

      TEST_METHOD(Tokenize_PositionTracking_AccuratePositions)
      {
         // Arrange
         CTokenizer tokenizer("abc  123   \\frac");
         vector<STexToken> tokens;
         ParserError err;

         // Act
         bool result = tokenizer.Tokenize(tokens, err);

         // Assert
         Assert::IsTrue(result, L"Position tracking should work");
         Assert::AreEqual((size_t)3, tokens.size(), L"Should have 3 tokens");

         // Verify positions account for skipped spaces
         Assert::AreEqual((uint32_t)0, tokens[0].nPos, L"First token should start at position 0");
         Assert::AreEqual((uint16_t)3, tokens[0].nLen, L"First token should have length 3");

         Assert::AreEqual((uint32_t)5, tokens[1].nPos, L"Second token should start at position 5 (after 2 spaces)");
         Assert::AreEqual((uint16_t)3, tokens[1].nLen, L"Second token should have length 3");

         Assert::AreEqual((uint32_t)11, tokens[2].nPos, L"Third token should start at position 11 (after 3 spaces)");
         Assert::AreEqual((uint16_t)5, tokens[2].nLen, L"Third token should have length 5");
      }

      TEST_METHOD(Tokenize_MathExpressions_ParseAllOperators)
      {
         // Arrange
         CTokenizer tokenizer("x^2 + y_1 = $\\frac{a}{b}$");
         vector<STexToken> tokens;
         ParserError err;

         // Act
         bool result = tokenizer.Tokenize(tokens, err);

         // Assert
         Assert::IsTrue(result, L"Math expression parsing should succeed");

         // Verify mathematical operators are properly tokenized
         bool foundCaret = false, foundUnderscore = false, foundDollar = false;
         for (const auto& token : tokens) {
            if (token.nType == ettSUPERS) foundCaret = true;
            if (token.nType == ettSUBS) foundUnderscore = true;
            if (token.nType == ett$) foundDollar = true;
         }

         Assert::IsTrue(foundCaret, L"Should find caret operator");
         Assert::IsTrue(foundUnderscore, L"Should find underscore operator");
         Assert::IsTrue(foundDollar, L"Should find dollar math delimiter");
      }

      TEST_METHOD(Tokenize_SpecialCharacters_HandleAllTypes)
      {
         // Arrange
         CTokenizer tokenizer("$ & _ ^ { } [ ]");
         vector<STexToken> tokens;
         ParserError err;

         // Act
         bool result = tokenizer.Tokenize(tokens, err);

         // Assert
         Assert::IsTrue(result, L"Special character parsing should succeed");
         Assert::AreEqual((size_t)8, tokens.size(), L"Should have 8 special character tokens");

         Assert::AreEqual((int)ett$, (int)tokens[0].nType, L"Should recognize dollar");
         Assert::AreEqual((int)ettAMP, (int)tokens[1].nType, L"Should recognize ampersand");
         Assert::AreEqual((int)ettSUBS, (int)tokens[2].nType, L"Should recognize underscore");
         Assert::AreEqual((int)ettSUPERS, (int)tokens[3].nType, L"Should recognize caret");
         Assert::AreEqual((int)ettFB_OPEN, (int)tokens[4].nType, L"Should recognize group open");
         Assert::AreEqual((int)ettFB_CLOSE, (int)tokens[5].nType, L"Should recognize group close");
         Assert::AreEqual((int)ettSB_OPEN, (int)tokens[6].nType, L"Should recognize square bracket open");
         Assert::AreEqual((int)ettSB_CLOSE, (int)tokens[7].nType, L"Should recognize square bracket close");
      }

      TEST_METHOD(Tokenize_CommandSequences_ParseCorrectly)
      {
         // Arrange
         CTokenizer tokenizer("\\alpha \\beta \\Gamma \\Delta");
         vector<STexToken> tokens;
         ParserError err;

         // Act
         bool result = tokenizer.Tokenize(tokens, err);

         // Assert
         Assert::IsTrue(result, L"Command sequence parsing should succeed");
         Assert::AreEqual((size_t)4, tokens.size(), L"Should have 4 command tokens");

         for (size_t i = 0; i < tokens.size(); i++) {
            Assert::AreEqual((int)ettCOMMAND, (int)tokens[i].nType,
               (L"Token " + to_wstring(i) + L" should be COMMAND").c_str());
         }

         Assert::AreEqual(string("\\alpha"), tokenizer.TokenText(tokens[0]), L"First command should be '\\alpha'");
         Assert::AreEqual(string("\\beta"), tokenizer.TokenText(tokens[1]), L"Second command should be '\\beta'");
         Assert::AreEqual(string("\\Gamma"), tokenizer.TokenText(tokens[2]), L"Third command should be '\\Gamma'");
         Assert::AreEqual(string("\\Delta"), tokenizer.TokenText(tokens[3]), L"Fourth command should be '\\Delta'");
      }

      
      TEST_METHOD(Tokenize_WhitespaceHandling_SkipCorrectly)
      {
         // Arrange
         CTokenizer tokenizer("  \t  a   \t\t  b  \t  ");
         vector<STexToken> tokens;
         ParserError err;

         // Act
         bool result = tokenizer.Tokenize(tokens, err);

         // Assert
         Assert::IsTrue(result, L"Whitespace handling should succeed");
         Assert::AreEqual((size_t)2, tokens.size(), L"Should have 2 tokens (whitespace skipped)");

         Assert::AreEqual(string("a"), tokenizer.TokenText(tokens[0]), L"First token should be 'a'");
         Assert::AreEqual(string("b"), tokenizer.TokenText(tokens[1]), L"Second token should be 'b'");
      }
   };
}

