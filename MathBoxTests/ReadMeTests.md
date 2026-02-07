# MathBoxTest - Unit Test Suite

Comprehensive MSTest-based test suite for MathBox library, covering parser, tokenizer, builders, and macro system.

## Overview

**MathBoxTest** provides automated testing for MathBox core functionality with approximately **70% code coverage**.

## Test Coverage

### Test Statistics

- **Total Test Methods**: ~80+
- **Test Classes**: 7 major test classes
- **Lines of Test Code**: ~2,000+
- **Estimated Code Coverage**: ~70%

**Coverage by Component:**
| Component | Coverage | Notes |
|-----------|----------|-------|
| Parser pipeline | ~75% | Tokenization, grouping, building |
| Macro system | ~80% | Definition, expansion, nesting |
| Builders | ~65% | Fraction, radical, accent, spacing |
| Tokenizer | ~70% | Token types, position tracking |
| Dimension parsing | ~90% | Units, spaces, error cases |
| Error handling | ~60% | All parsing stages |

## Test Classes

### 1. TexParserTests (`TexParserTests.cpp`)

Tests core parser functionality including subscripts, superscripts, and error cases.

**Test Scenarios:**

**Indexed Items:**
- ? `SimpleIndexed_Subscript_Display` - Test `x_a` parsing
- ? `SimpleIndexed_Superscript_Display` - Test `x^2` parsing
- ? `Indexed_SubSuperscript_Inline` - Test `x_a^b` parsing
- ? `Indexed_GroupedSubscript_Display` - Test `x_{ab}` parsing
- ? `Error_DoubleSubSuperscript` - Test `x_a_b` error handling
- ? `Indexed_GeneralizedFraction_Display` - Test `_a^b` (no base)
- ? `Indexed_ComplexChaining_Display` - Test `{X_Y^Z}_A` nesting

**Style Verification:**
- Verify `etsDisplay` / `etsText` / `etsScript` style propagation
- Verify cramped vs. non-cramped styles
- Verify subscripts are cramped, superscripts are not

**Grouping & Optimization:**
- ? `NestedGroups_OptimizedToSingleItem` - Test `{{a}}` unwrapping
- ? `Brackets_TextMode_Literals` - Test `[a]` in text mode
- ? `Brackets_MathMode_Literals` - Test `[a]` in math mode
- ? `Brackets_As_OptionalArg_Radical` - Test `\sqrt[3]{x}`
- ? `Brackets_Ignored_For_NonOpt_Command` - Test `\mathbf[a]`

**Math in Text:**
- ? `MathInText_Display` - Test mixed text and math

**Multiple Terms:**
- ? `MultipleTerms_Display` - Test `a+b-c` parsing

**Error Cases:**
- ? `Error_MissingSubscriptArgument_EndOfInput` - Test `x_` error
- ? `Error_MissedSubSuperscript` - Test `x_^y` error
- ? `Error_MissedSuperSubscript` - Test `x^_y` error
- ? `Error_EmptyGroups_NotAllowed` - Test empty group errors
- ? `Error_BuildGroups_Unclosed` - Test unclosed brace errors
- ? `Error_NestedMathModes` - Test nested `$...$` errors

### 2. MathBuilderTests (`TexParserTests.cpp`)

Tests specialized builders for fractions, radicals, accents, and spacing.

**Fraction Builder:**
- ? `FractionBuilder_Basic_Display` - Test `\frac{a}{b}`
  - Verify VBox structure (3 items: numerator, bar, denominator)
  - Verify style changes (Display ? Text)
  - Verify denominator cramping
- ? `FractionBuilder_Nested_Display` - Test `\frac{\frac{a}{b}}{c}`
- ? `FractionBuilder_WithSuperscript_Display` - Test `\frac{a}{b}^2`

**Radical Builder:**
- ? `RadicalBuilder_Basic_Display` - Test `\sqrt{x}`
  - Verify RADICAL container structure
  - Verify radicand cramping
- ? `RadicalBuilder_Degree_Display` - Test `\sqrt[3]{A}`
  - Verify degree positioning and ScriptScript style
  - Verify 4 items: RadGlyph, Degree, Overbar, Radicand
- ? `RadicalBuilder_RecursiveDegree_Inline` - Test `\sqrt[\sqrt{2}]{x}`
  - Verify nested radical in degree
  - Verify style cascade

**Accent Builder:**
- ? `AccentBuilder_Basic_Display` - Test `\ocirc{a}`
  - Verify 2-item structure: Base + Accent

**HSpacing Builder:**
- ? `HSpacingBuilder_HSkip_Fixed` - Test `\hskip 10pt`
- ? `HSpacingBuilder_HSkip_EmUnit` - Test `\hskip 2em`
- ? `HSpacingBuilder_HSkip_ExUnit` - Test `\hskip 3ex`

### 3. DimensionParserTests (`TexParserTests.cpp`)

Tests dimension parsing with various units and edge cases.

**Basic Parsing:**
- ? `ParseDimensionString_BasicPoints` - Test `12pt`
- ? `ParseDimensionString_DecimalPoints` - Test `12.5pt`
- ? `ParseDimensionString_NegativeValue` - Test `-5pt`

**Unit Conversions:**
- ? `ParseDimensionString_Centimeters` - Test `2.54cm` ? 72pt
- ? `ParseDimensionString_EmUnits` - Test `2em` at 12pt font
- ? `ParseDimensionString_ExUnits` - Test `3ex` at 12pt font
- ? `ParseDimensionString_Millimeters` - Test `25.4mm` ? 72pt
- ? `ParseDimensionString_Inches` - Test `1in` ? 72pt

**Space Handling:**
- ? `HSkip_WithSpacesAroundNumber` - Test `\hskip 12 pt`
- ? `HSkip_WithSpacesInBraces` - Test `\hskip{ 10 pt }`
- ? `HSkip_DecimalWithSpaces` - Test `\hskip 12.5 pt`
- ? `HSkip_NegativeWithSpaceAfterSign` - Test `\hskip - 5 pt`
- ? `HSkip_TrailingSpacesInBraces` - Test `\hskip{ 8pt   }`
- ? `HSkip_LeadingSpacesInBraces` - Test `\hskip{   7pt}`
- ? `HSkip_MultipleSpaces` - Test `\hskip   9   pt`

**Variable Support:**
- ? `SetLength_WithSpaces` - Test variable storage
- ? `SetLength_DecimalWithSpaces` - Test decimal variable
- ? `HSkip_WithVariableAndSpaces` - Test `\hskip{ 2.5 \mylen }`

**Error Cases:**
- ? `ParseDimensionString_InvalidNoUnit` - Test `12` without unit
- ? `ParseDimensionString_InvalidUnit` - Test `12xyz`
- ? `ParseDimensionString_NoSpacesAllowed` - Test `12 pt` in string parser
- ? `ParseDimensionString_InvalidSpaceInNumber` - Test `1 2pt`
- ? `Error_SpaceInsideDecimalNumber` - Test `12. 5pt`

### 4. MacroProcessorTests (`TexParserTests.cpp`)

Tests macro definition, expansion, and error handling.

**Macro Definitions:**
- ? `InitMacros_SetLength_Basic` - Test `\setlength{\mylen}{10pt}`
- ? `InitMacros_SetLength_WithBraces` - Test `\setlength{\parindent}{0pt}`
- ? `InitMacros_SetLength_NoBraces` - Test `\setlength\mylen{5em}`
- ? `InitMacros_AddToLength_Basic` - Test `\addtolength{\x}{5pt}`
- ? `InitMacros_Let_AliasMacro` - Test `\let\alias\mycmd`
- ? `InitMacros_Let_WithEquals` - Test `\let\copy = \test`
- ? `InitMacros_Let_ForwardReference` - Test `\let\myalias\undefined`
- ? `InitMacros_NewCommand_NoParams` - Test `\newcommand{\hello}{world}`
- ? `InitMacros_NewCommand_WithParams` - Test `\newcommand{\mysum}[2]{#1 + #2}`
- ? `InitMacros_NewCommand_OptionalArg_SingleToken` - Test `\newcommand{\test}[2][+]{#1 #2}`
- ? `InitMacros_NewCommand_OptionalArg_MultipleTokens` - Test optional arg groups
- ? `InitMacros_NewCommand_NestedDefinition` - Test nested `\newcommand`
- ? `InitMacros_MultipleMacros` - Test multiple macro definitions
- ? `InitMacros_RenewCommand_ReplacesExisting` - Test `\renewcommand`

**Macro Expansion:**
- ? `MacroExpansion_NoParams_Simple` - Test simple expansion
- ? `MacroExpansion_NoParams_MultipleUses` - Test reuse
- ? `MacroExpansion_WithParams_OneArg` - Test `\twice{a}`
- ? `MacroExpansion_WithParams_TwoArgs` - Test `\add{a}{b}`
- ? `MacroExpansion_WithParams_GroupedArg` - Test `\twice{x^2}`
- ? `MacroExpansion_OptionalArg_Provided` - Test `\test[-]{x}`
- ? `MacroExpansion_OptionalArg_Default` - Test `\test{x}` with default
- ? `MacroExpansion_Nested_TwoLevels` - Test nested expansion
- ? `MacroExpansion_Nested_WithArguments` - Test `\outer{a}` ? `\inner{a}{a}`

**Parameter Substitution:**
- ? `MacroExpansion_ParameterSubstitution_SimpleCase` - Test `#1` replacement
- ? `MacroExpansion_MultipleParameters_AllReplaced` - Test `#1`, `#2`, `#3`

**Special Tokens:**
- ? `MacroExpansion_BeginGroupEndGroup` - Test `\begingroup`/`\endgroup` replacement

**With Delimiters:**
- ? `MacroExpansion_WithLeftRight_Norm` - Test `\norm{x}` with `\left\|...\right\|`
- ? `MacroExpansion_WithLeftRight_Absolute` - Test `\abs{-5}` with `\left|...\right|`
- ? `MacroExpansion_WithLeftRight_Brackets` - Test `\floor{3.7}`
- ? `MacroExpansion_WithLeftRight_Nested` - Test nested delimiters
- ? `MacroExpansion_WithLeftRight_InArgument` - Test delimiter in argument

**With Environments:**
- ? `MacroExpansion_WithEnvironment_Matrix` - Test `\smallmatrix` with matrix
- ? `MacroExpansion_WithEnvironment_Cases` - Test `\signum` with cases
- ? `MacroExpansion_WithEnvironment_Array` - Test `\myarray` with array
- ? `MacroExpansion_WithEnvironment_NestedInArgument` - Test environment as arg
- ? `MacroExpansion_WithEnvironment_MultipleMacros` - Test multiple env macros

**Real-World Macros:**
- ? `MacroExpansion_RealWorld_SetBuilder` - Test `\set{x}{x>0}`
- ? `MacroExpansion_RealWorld_Binom` - Test `\binom{n}{k}`
- ? `MacroExpansion_RealWorld_PiecewiseFunction` - Test piecewise function

**Token Tracking:**
- ? `MacroExpansion_RefIdx_FirstExpansion` - Test `nRefIdx` tracking
- ? `MacroExpansion_RefIdx_NestedExpansion` - Test nested `nRefIdx`
- ? `MacroExpansion_CompareOriginalVsExpanded` - Test token comparison
- ? `InitMacros_TokensHaveNegativeRefIdx` - Test macro token `nRefIdx = -1`

**Error Cases:**
- ? `InitMacros_Error_InvalidCommand` - Test unknown command
- ? `InitMacros_Error_AddToLength_Undefined` - Test undefined variable
- ? `InitMacros_Error_NewCommand_MissingName` - Test empty name
- ? `InitMacros_Error_NewCommand_InvalidParamCount` - Test invalid count
- ? `InitMacros_Error_NewCommand_UnclosedBody` - Test unclosed body
- ? `InitMacros_Error_Let_MissingNewName` - Test missing name
- ? `InitMacros_Error_SetLength_InvalidDimension` - Test invalid dimension
- ? `MacroExpansion_Error_MissingArgument` - Test missing argument
- ? `MacroExpansion_Error_EndOfInput` - Test end of input
- ? `MacroExpansion_Error_UnclosedArgument` - Test unclosed argument
- ? `MacroExpansion_Error_UnmatchedLeftRight` - Test unmatched delimiter
- ? `MacroExpansion_Error_WrongEnvironment` - Test mismatched environment

### 5. TokenizerTests (`TokenizerTests.cpp`)

Tests tokenization of LaTeX input.

**Token Types:**
- ? Basic alphanumeric tokens
- ? Non-alpha characters
- ? Commands (`\command`)
- ? Numbers (integers and decimals)
- ? Braces and brackets
- ? Special characters (`$`, `&`, `\\`)

**Comment Handling:**
- ? Single-line comments (`%...`)
- ? Comments at end of line
- ? Empty comments

**Position Tracking:**
- ? Token position in input
- ? Token length
- ? Multi-line position tracking

**Escape Sequences:**
- ? `\{`, `\}`, `\%`, `\$`, etc.
- ? Control space (`\ `)

### 6. GlueFactoryTests & GlueItemTests

Tests glue creation and behavior.

**Glue Types:**
- Fixed spacing
- Stretchable spacing (fil, fill, filll)
- Shrinkable spacing

**Operations:**
- Addition of glues
- Multiplication by scalar
- Solving for actual width

### 7. HBoxItemTests & RawItemTests

Tests horizontal box and raw item functionality.

**HBox:**
- Item addition and removal
- Baseline alignment
- Width calculation
- Glue distribution

**Raw Items:**
- ? `TryAppendWord_ReturnTrue` - Test word concatenation
- Text mode accent handling

## Running Tests

### From Visual Studio

1. **Build Test Project**
   ```
   Build ? Build MathBoxTest (or Build Solution)
   ```

2. **Open Test Explorer**
   ```
   Test ? Test Explorer (Ctrl+E, T)
   ```

3. **Run Tests**
   - **All Tests**: Click "Run All" button or Ctrl+R, A
   - **Specific Class**: Right-click test class ? Run
   - **Specific Test**: Right-click test method ? Run
   - **Failed Tests**: Click "Run Failed Tests" button

4. **View Results**
   - ? Green check = Passed
   - ? Red X = Failed
   - ? Yellow warning = Skipped
   - Click test to see details, output, and stack trace

### From Command Line

```bash
# Run all tests
vstest.console.exe x64\Debug\MathBoxTest.dll

# Run specific test
vstest.console.exe x64\Debug\MathBoxTest.dll /Tests:SimpleIndexed_Subscript_Display

# Run tests with logger
vstest.console.exe x64\Debug\MathBoxTest.dll /Logger:trx
```

### Test Output

**Console Output:**
```
Test run for D:\...\x64\Debug\MathBoxTest.dll (.NETFramework,Version=v4.8)
Microsoft (R) Test Execution Command Line Tool Version 17.0.0

Starting test execution, please wait...
A total of 1 test files matched the specified pattern.

Passed   SimpleIndexed_Subscript_Display [123 ms]
Passed   SimpleIndexed_Superscript_Display [45 ms]
...

Test Run Successful.
Total tests: 80
     Passed: 80
 Total time: 12.3456 Seconds
```

## Test Assertions

**Common Assertions:**
```cpp
// Pointer checks
Assert::IsNotNull(pRet, L"Parser failed!");
Assert::IsNull(pRet, L"Should fail");

// Equality checks
Assert::AreEqual(expected, actual, L"Message");
Assert::AreNotEqual(unexpected, actual, L"Message");

// Boolean checks
Assert::IsTrue(condition, L"Message");
Assert::IsFalse(condition, L"Message");

// Type checks
CContainerItem* pCont = dynamic_cast<CContainerItem*>(pRet);
Assert::IsNotNull(pCont, L"Type check failed");

// Collection size
Assert::AreEqual((size_t)3, pHBox->Items().size(), L"Expected 3 items");

// String checks
Assert::AreEqual(string("expected"), actual, L"Message");
```

## Adding New Tests

### 1. Create Test Method

```cpp
TEST_METHOD(YourTestName) {
   // Arrange
   CTexParser parser(g_Doc);
   
   // Act
   CMathItem* pRet = parser.Parse("$$...$$");
   SMemGuard mg{ pRet };  // Auto-cleanup
   
   // Assert
   Assert::IsNotNull(pRet, L"Parse should succeed");
   Assert::IsTrue(parser.LastError().sError.empty(), L"No errors");
   
   // Verify structure
   Assert::AreEqual((int)eacEXPECTED_TYPE, (int)pRet->Type());
   
   // ... more assertions ...
}
```

### 2. Test Error Cases

```cpp
TEST_METHOD(Error_YourErrorCase) {
   CTexParser parser(g_Doc);
   CMathItem* pRet = parser.Parse("$$invalid$$");
   SMemGuard mg{ pRet };
   
   Assert::IsNull(pRet, L"Parse should fail");
   
   const ParserError& err = parser.LastError();
   Assert::IsFalse(err.sError.empty(), L"Should have error");
   Assert::IsTrue(err.sError.find("expected text") != string::npos,
                 L"Error should contain expected text");
   Assert::AreEqual((int)epsEXPECTED_STAGE, (int)err.eStage);
}
```

### 3. Memory Management

Always use `SMemGuard` to ensure cleanup:
```cpp
struct SMemGuard {
   CMathItem* pItem;
   ~SMemGuard() { delete pItem; }
};

TEST_METHOD(YourTest) {
   CMathItem* pRet = parser.Parse("...");
   SMemGuard mg{ pRet };  // Auto-delete on scope exit
   
   // ... test code ...
   // No need to manually delete pRet
}
```

## Coverage Gaps

**Areas Needing More Tests:**
- [ ] Error recovery and partial parsing
- [ ] Large document stress tests
- [ ] Unicode edge cases
- [ ] Font fallback scenarios
- [ ] Rendering output validation
- [ ] Performance benchmarks
- [ ] Memory leak detection
- [ ] Thread safety (if applicable)

## Continuous Integration

**Recommended CI Setup:**
```yaml
# Azure Pipelines example
- task: VSTest@2
  inputs:
    testAssemblyVer2: '**\*Test*.dll'
    platform: 'x64'
    configuration: 'Release'
    codeCoverageEnabled: true
```

## License

See main README.md for license information.
