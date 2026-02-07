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
         CTokenizer tokenizer("abc123+\\alpha{}[]%comment whatever");
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
         Assert::AreEqual((uint32_t)11, err.nStartPos, L"Error position should be at backslash");
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
         CTokenizer tokenizer("\\{\\}\\%\\&");
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
         CTokenizer tokenizer("before% this is a comment\nafter");
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
         Assert::AreEqual(string("% this is a comment\n"), tokenizer.TokenText(tokens[1]), L"Comment should include entire line");

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
         Assert::AreEqual((size_t)5, tokens.size(), L"Should have 5 tokens");

         // Verify positions account for skipped spaces
         Assert::AreEqual(0, tokens[0].nPos, L"First token should start at position 0");
         Assert::AreEqual((uint16_t)3, tokens[0].nLen, L"First token should have length 3");

         Assert::AreEqual(5, tokens[2].nPos, L"Second token should start at position 5 (after 2 spaces)");
         Assert::AreEqual((uint16_t)3, tokens[2].nLen, L"Second token should have length 3");

         Assert::AreEqual(11, tokens[4].nPos, L"Third token should start at position 11 (after 3 spaces)");
         Assert::AreEqual((uint16_t)5, tokens[4].nLen, L"Third token should have length 5");
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
         CTokenizer tokenizer("$&_^{}[\t]");
         vector<STexToken> tokens;
         ParserError err;

         // Act
         bool result = tokenizer.Tokenize(tokens, err);

         // Assert
         Assert::IsTrue(result, L"Special character parsing should succeed");
         Assert::AreEqual((size_t)9, tokens.size(), L"Should have 9 special character tokens");

         Assert::AreEqual((int)ett$, (int)tokens[0].nType, L"Should recognize dollar");
         Assert::AreEqual((int)ettAMP, (int)tokens[1].nType, L"Should recognize ampersand");
         Assert::AreEqual((int)ettSUBS, (int)tokens[2].nType, L"Should recognize underscore");
         Assert::AreEqual((int)ettSUPERS, (int)tokens[3].nType, L"Should recognize caret");
         Assert::AreEqual((int)ettFB_OPEN, (int)tokens[4].nType, L"Should recognize group open");
         Assert::AreEqual((int)ettFB_CLOSE, (int)tokens[5].nType, L"Should recognize group close");
         Assert::AreEqual((int)ettSB_OPEN, (int)tokens[6].nType, L"Should recognize square bracket open");
         Assert::AreEqual((int)ettSPACE, (int)tokens[7].nType, L"Should recognize space");
         Assert::AreEqual((int)ettSB_CLOSE, (int)tokens[8].nType, L"Should recognize square bracket close");
      }

      TEST_METHOD(Tokenize_CommandSequences_ParseCorrectly)
      {
         // Arrange
         CTokenizer tokenizer(" \\alpha\\beta\\Gamma\\Delta");
         vector<STexToken> tokens;
         ParserError err;

         // Act
         bool result = tokenizer.Tokenize(tokens, err);

         // Assert
         Assert::IsTrue(result, L"Command sequence parsing should succeed");
         Assert::AreEqual((size_t)5, tokens.size(), L"Should have 5 tokens");
         Assert::AreEqual((int)ettSPACE, (int)tokens[0].nType,L"Token 0 should be ettSPACE");

         for (size_t i = 1; i < tokens.size(); i++) {
            Assert::AreEqual((int)ettCOMMAND, (int)tokens[i].nType,
               (L"Token " + to_wstring(i) + L" should be COMMAND").c_str());
         }

         Assert::AreEqual(string("\\alpha"), tokenizer.TokenText(tokens[1]), L"First command should be '\\alpha'");
         Assert::AreEqual(string("\\beta"), tokenizer.TokenText(tokens[2]), L"Second command should be '\\beta'");
         Assert::AreEqual(string("\\Gamma"), tokenizer.TokenText(tokens[3]), L"Third command should be '\\Gamma'");
         Assert::AreEqual(string("\\Delta"), tokenizer.TokenText(tokens[4]), L"Fourth command should be '\\Delta'");
      }

      
      TEST_METHOD(Tokenize_WhitespaceHandling_Correctly)
      {
         // Arrange
         CTokenizer tokenizer("  \t  a   \r\n  b  \t  ");
         vector<STexToken> tokens;
         ParserError err;

         // Act
         bool result = tokenizer.Tokenize(tokens, err);

         // Assert
         Assert::IsTrue(result, L"Whitespace handling should succeed");
         Assert::AreEqual((size_t)4, tokens.size(), L"Should have 4 tokens (whitespace skipped)");

         Assert::AreEqual(string("a"), tokenizer.TokenText(tokens[1]), L"Second token should be 'a'");
         Assert::AreEqual(string("b"), tokenizer.TokenText(tokens[3]), L"Fourth token should be 'b'");
      }

      // ========================================
      // MACRO TEXT MODE TESTS
      // ========================================

      TEST_METHOD(MacroMode_AtSymbolInCommands_AllowedAndParsedCorrectly)
      {
         // Arrange - @ should be part of command name in macro mode
         CTokenizer tokenizer("\\my@helper \\if@internal \\set@length", true); // true = macro mode
         vector<STexToken> tokens;
         ParserError err;

         // Act
         bool result = tokenizer.Tokenize(tokens, err);

         // Assert
         Assert::IsTrue(result, L"Macro mode tokenization with @ should succeed");
         Assert::AreEqual((size_t)5, tokens.size(), L"Should have 3 command and two space tokens");

         // Verify all tokens are commands
         Assert::AreEqual((int)ettCOMMAND, (int)tokens[0].nType, L"First token should be COMMAND");
         Assert::AreEqual((int)ettCOMMAND, (int)tokens[2].nType, L"Second token should be COMMAND");
         Assert::AreEqual((int)ettCOMMAND, (int)tokens[4].nType, L"Third token should be COMMAND");

         // Verify @ is included in command names
         Assert::AreEqual(string("\\my@helper"), tokenizer.TokenText(tokens[0]), 
            L"Command should include @ character");
         Assert::AreEqual(string("\\if@internal"), tokenizer.TokenText(tokens[2]), 
            L"Command should include @ character");
         Assert::AreEqual(string("\\set@length"), tokenizer.TokenText(tokens[4]), 
            L"Command should include @ character");

         // Verify all tokens are marked as macro tokens (nRefIdx = -1)
         Assert::AreEqual(-1, tokens[0].nRefIdx, L"Macro token should have nRefIdx = -1");
         Assert::AreEqual(-1, tokens[2].nRefIdx, L"Macro token should have nRefIdx = -1");
         Assert::AreEqual(-1, tokens[4].nRefIdx, L"Macro token should have nRefIdx = -1");
      }

      TEST_METHOD(MacroMode_SingleParameter_TokenizesAsMARG1)
      {
         // Arrange - #1 should create ettMARG1 token
         CTokenizer tokenizer("#1", true); // true = macro mode
         vector<STexToken> tokens;
         ParserError err;

         // Act
         bool result = tokenizer.Tokenize(tokens, err);

         // Assert
         Assert::IsTrue(result, L"Single parameter tokenization should succeed");
         Assert::AreEqual((size_t)1, tokens.size(), L"Should have 1 token");
         
         Assert::AreEqual((int)ettMARG1, (int)tokens[0].nType, L"Token should be ettMARG1");
         Assert::AreEqual(0, tokens[0].nPos, L"Token should start at position 0");
         Assert::AreEqual((uint16_t)2, tokens[0].nLen, L"Token length should be 2 (#1)");
         Assert::AreEqual(-1, tokens[0].nRefIdx, L"Macro token should have nRefIdx = -1");
         
         Assert::AreEqual(string("#1"), tokenizer.TokenText(tokens[0]), L"Token text should be '#1'");
      }

      TEST_METHOD(MacroMode_AllSingleParameters_TokenizeCorrectly)
      {
         // Arrange - Test all single parameters #1 through #9
         CTokenizer tokenizer("#1 #2 #3 #4 #5 #6 #7 #8 #9", true);
         vector<STexToken> tokens;
         ParserError err;

         // Act
         bool result = tokenizer.Tokenize(tokens, err);

         // Assert
         Assert::IsTrue(result, L"All single parameters should tokenize successfully");
         Assert::AreEqual((size_t)17, tokens.size(), L"Should have 17 tokens (9 params + 8 spaces)");

         // Verify each parameter token
         for (int i = 0; i < 9; i++) {
            int tokenIdx = i * 2; // Skip space tokens
            int expectedType = ettMARG1 + i;
            
            Assert::AreEqual(expectedType, (int)tokens[tokenIdx].nType, 
               (L"Token " + to_wstring(i) + L" should be ettMARG" + to_wstring(i+1)).c_str());
            Assert::AreEqual((uint16_t)2, tokens[tokenIdx].nLen, 
               L"Parameter token should have length 2");
            Assert::AreEqual(-1, tokens[tokenIdx].nRefIdx, 
               L"Parameter token should have nRefIdx = -1");
         }
      }

      TEST_METHOD(MacroMode_DoubleHash_ReturnsError)
      {
         CTokenizer tokenizer("##1", true);
         vector<STexToken> tokens;
         ParserError err;

         // Act
         bool result = tokenizer.Tokenize(tokens, err);

         // Assert
         Assert::IsFalse(result, L"Double hash is prohibited");
      }

      TEST_METHOD(MacroMode_MacroDefinitionExample_TokenizesCorrectly)
      {
         // Arrange - Real macro definition: \newcommand{\mysum}[2]{#1 + #2}
         CTokenizer tokenizer("\\newcommand{\\mysum}[2]{#1 + #2}", true);
         vector<STexToken> tokens;
         ParserError err;

         // Act
         bool result = tokenizer.Tokenize(tokens, err);

         // Assert
         Assert::IsTrue(result, L"Macro definition should tokenize successfully");
         
         // Find parameter tokens
         bool foundParam1 = false, foundParam2 = false;
         for (const auto& token : tokens) {
            if (token.nType == ettMARG1) {
               foundParam1 = true;
               Assert::AreEqual((uint16_t)2, token.nLen, L"#1 should have length 2");
            }
            if (token.nType == ettMARG2) {
               foundParam2 = true;
               Assert::AreEqual((uint16_t)2, token.nLen, L"#2 should have length 2");
            }
            // All tokens should be marked as macro tokens
            Assert::AreEqual(-1, token.nRefIdx, L"All tokens should have nRefIdx = -1 in macro mode");
         }
         
         Assert::IsTrue(foundParam1, L"Should find #1 parameter");
         Assert::IsTrue(foundParam2, L"Should find #2 parameter");
      }

      TEST_METHOD(MacroMode_InvalidParameterNumber_ReturnsError)
      {
         // Arrange - #0 and #a are invalid
         CTokenizer tokenizer1("#0", true);
         CTokenizer tokenizer2("#a", true);
         vector<STexToken> tokens;
         ParserError err1, err2;

         // Act
         bool result1 = tokenizer1.Tokenize(tokens, err1);
         bool result2 = tokenizer2.Tokenize(tokens, err2);

         // Assert
         Assert::IsFalse(result1, L"#0 should fail tokenization");
         Assert::AreEqual((int)epsTOKENIZING, (int)err1.eStage, 
            L"Error should be in tokenization stage");
         Assert::IsTrue(err1.sError.find("Invalid macro-arg number") != string::npos, 
            L"Error message should mention invalid macro-arg");
         
         Assert::IsFalse(result2, L"#a should fail tokenization");
         Assert::AreEqual((int)epsTOKENIZING, (int)err2.eStage, 
            L"Error should be in tokenization stage");
      }

      TEST_METHOD(MacroMode_MixedParametersAndText_TokenizesCorrectly)
      {
         // Arrange - Complex macro body with mixed content
         CTokenizer tokenizer("A=#1 and B=#2 plus #4", true);
         vector<STexToken> tokens;
         ParserError err;

         // Act
         bool result = tokenizer.Tokenize(tokens, err);

         // Assert
         Assert::IsTrue(result, L"Mixed content should tokenize successfully");
         
         // Verify we have correct parameter tokens
         int param1Count = 0, param2Count = 0, param4Count = 0;
         for (const auto& token : tokens) {
            if (token.nType == ettMARG1) param1Count++;
            if (token.nType == ettMARG2) param2Count++;
            if (token.nType == ettMARG4) param4Count++;
         }
         
         Assert::AreEqual(1, param1Count, L"Should have one #1 parameter");
         Assert::AreEqual(1, param2Count, L"Should have one #2 parameter");
         Assert::AreEqual(1, param4Count, L"Should have one #4 unexpanded parameter");
      }

      TEST_METHOD(MacroMode_ParameterInCommand_TokenizesSeparately)
      {
         // Arrange - Parameter followed by command: #1\alpha
         CTokenizer tokenizer("#1\\alpha", true);
         vector<STexToken> tokens;
         ParserError err;

         // Act
         bool result = tokenizer.Tokenize(tokens, err);

         // Assert
         Assert::IsTrue(result, L"Parameter followed by command should tokenize");
         Assert::AreEqual((size_t)2, tokens.size(), L"Should have 2 tokens");
         
         Assert::AreEqual((int)ettMARG1, (int)tokens[0].nType, 
            L"First token should be parameter");
         Assert::AreEqual((int)ettCOMMAND, (int)tokens[1].nType, 
            L"Second token should be command");
         
         Assert::AreEqual(string("#1"), tokenizer.TokenText(tokens[0]), 
            L"First token should be '#1'");
         Assert::AreEqual(string("\\alpha"), tokenizer.TokenText(tokens[1]), 
            L"Second token should be '\\alpha'");
      }

      TEST_METHOD(MacroMode_TokenRefIdxMarking_AllTokensMarkedCorrectly)
      {
         // Arrange - Verify all tokens in macro mode have nRefIdx = -1
         CTokenizer tokenizer("\\cmd{arg}[opt]#1#2", true);
         vector<STexToken> tokens;
         ParserError err;

         // Act
         bool result = tokenizer.Tokenize(tokens, err);

         // Assert
         Assert::IsTrue(result, L"Mixed macro content should tokenize");
         
         // Verify ALL tokens have nRefIdx = -1
         for (size_t i = 0; i < tokens.size(); i++) {
            Assert::AreEqual(-1, tokens[i].nRefIdx, 
               (L"Token " + to_wstring(i) + L" should have nRefIdx = -1").c_str());
         }
      }

      TEST_METHOD(RegularMode_AtSymbolInCommand_NotIncluded)
      {
         // Arrange - @ should NOT be part of command in regular mode
         CTokenizer tokenizer("\\my@helper", false); // false = regular mode
         vector<STexToken> tokens;
         ParserError err;

         // Act
         bool result = tokenizer.Tokenize(tokens, err);

         // Assert
         Assert::IsTrue(result, L"Regular mode tokenization should succeed");
         
         // @ should be separate token, not part of command
         bool foundCommand = false;
         for (const auto& token : tokens) {
            if (token.nType == ettCOMMAND) {
               string cmdText = tokenizer.TokenText(token);
               Assert::AreEqual(string("\\my"), cmdText, 
                  L"Command should stop before @ in regular mode");
               foundCommand = true;
            }
         }
         
         Assert::IsTrue(foundCommand, L"Should find command token");
      }

      TEST_METHOD(RegularMode_TokenRefIdx_DefaultsToZero)
      {
         // Arrange - Regular mode tokens should have nRefIdx = 0
         CTokenizer tokenizer("\\alpha{test}", false); // false = regular mode
         vector<STexToken> tokens;
         ParserError err;

         // Act
         bool result = tokenizer.Tokenize(tokens, err);

         // Assert
         Assert::IsTrue(result, L"Regular mode tokenization should succeed");
         
         // Verify all tokens have nRefIdx = 0 (default)
         for (size_t i = 0; i < tokens.size(); i++) {
            Assert::AreEqual(0, tokens[i].nRefIdx, 
               (L"Regular mode token " + to_wstring(i) + L" should have nRefIdx = 0").c_str());
         }
      }
   };
}

