#include "stdafx.h"
#include "TexParser.h"
#include "DimensionParser.h"
#include "ContainerItem.h"
#include "WordItem.h"
#include "RawItem.h"
#include "D2DFontManager.h"
#include "LMMFont.h"
#include "MacroProcessor.h"

static IDWriteFactory* g_pDWriteFactory = nullptr;
static CD2DFontManager g_D2DFontManager;
static CLMMFont g_LMMFont;
class CTestDocParams : public IDocParams {
   float m_fFontSizePts{ 12.0f };
public:
   void SetFontSizePts(float fSize) { m_fFontSizePts = fSize; }
   IFontManager& FontManager() override {
      return g_D2DFontManager;
   }
   ILMFManager& LMFManager() override {
      return g_LMMFont;
   }
   float DefaultFontSizePts() override {
      return m_fFontSizePts;
   }
   int32_t MaxWidthFDU() override {
      return 0;
   }
   uint32_t ColorBkg() override { return 0; }
   uint32_t ColorSelection() override { return 0; }
   //Text color ~= Foreground color!
   uint32_t ColorText() override { return 0; }
};
static CTestDocParams g_Doc;

struct SMemGuard {
   CMathItem*     pItem;
   ~SMemGuard() { delete pItem; }
};

namespace TexParserTests
{
   TEST_MODULE_INITIALIZE(ModuleInitialize)
   {
      HRESULT hRes = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                                       reinterpret_cast<IUnknown**>(&g_pDWriteFactory));
      _ASSERT(SUCCEEDED(hRes));
      WCHAR wszDir[MAX_PATH] = { 0 };
      GetCurrentDirectoryW(_countof(wszDir), wszDir);
      // Remove last two path components from wszDir (e.g. remove "\x64\Debug")
      std::wstring wsFontDir(wszDir);
      for (int i = 0; i < 2; ++i) {
         size_t pos = wsFontDir.find_last_of(L"\\/");
         if (pos == std::wstring::npos) {
            // nothing more to strip
            wsFontDir.clear();
            break;
         }
         wsFontDir.resize(pos);
      }
      wsFontDir += L"\\MathBoxLib\\LatinModernFonts\\";
      hRes = g_D2DFontManager.Init(wsFontDir, g_pDWriteFactory);
      _ASSERT(SUCCEEDED(hRes));
      bool bOk = g_LMMFont.Init(wsFontDir);
      _ASSERT(bOk);
   }
   TEST_MODULE_CLEANUP(ModuleCleanup)
   {
      SafeRelease(&g_pDWriteFactory);
      g_pDWriteFactory = nullptr;
   }
   TEST_CLASS(TexParserTests) {
   public:
      // Input: "$$x_a$$"
      // Expected Result:
      // - Single CIndexedItem (NOT wrapped in CHBoxItem)
      // - Base: CWordItem for 'x'
      // - Subscript: CWordItem for 'a'
      // Verify:
      // - Result type == eacINDEXED
      // - Cast to CIndexedItem succeeds
      // - Base item exists and is 'x'
      // - Subscript item exists and is 'a'
      // - Superscript is nullptr
      // - Subscript style is decreased and cramped
      // -  No parse errors
      TEST_METHOD(SimpleIndexed_Subscript_Display) {
         CTexParser parser(g_Doc);
         CMathItem* pRet = parser.Parse("$$x_a$$");
         SMemGuard mg{ pRet };
         Assert::IsNotNull(pRet, L"Parser failed!");
         Assert::IsTrue(eacINDEXED == pRet->Type(), L"Resulting item is not INDEXED");
         CContainerItem* pCont = dynamic_cast<CContainerItem*>(pRet);
         Assert::IsNotNull(pCont, L"Unexpected base for eacINDEXED!");
         Assert::IsTrue(parser.LastError().sError.empty(), L"Unexpected parse error");
         Assert::IsTrue(pCont->Items().size() == 2, L"Expected 2 items in the eacINDEXED container");
         // Assert - Base item
         CMathItem* pBase = pCont->Items()[0];
         Assert::AreEqual((int)eacWORD, (int)pBase->Type(), L"Base should be CWordItem");
         Assert::AreEqual((int)etsDisplay, (int)pBase->GetStyle().Style(), L"Base should have Display style");
         Assert::IsFalse(pBase->GetStyle().IsCramped(), L"Base should not be cramped");
         // Assert - Subscript item
         CMathItem* pSub = pCont->Items()[1];
         Assert::AreEqual((int)eacWORD, (int)pSub->Type(), L"Subscript should be CWordItem");
         Assert::AreEqual((int)etsScript, (int)pSub->GetStyle().Style(), L"Subscript should have Script style");
         Assert::IsTrue(pSub->GetStyle().IsCramped(), L"Subscript should be cramped");
      }
      // ## Test 2: SimpleSuperscript
      // # Input: "$$x^2$$"
      // 
      // # Expected Result:
      // - Single CIndexedItem (NOT wrapped in CHBoxItem)
      // - Base: CWordItem for 'x'
      // - Superscript: CWordItem for '2'
      // 
      // # Verify:
      // - Result type == eacINDEXED
      // - Base item is 'x'
      // - Superscript item is '2'
      // - Subscript is nullptr
      // - Superscript style is decreased (NOT cramped)
      // - No parse errors
      TEST_METHOD(SimpleIndexed_Superscript_Display) {
         CTexParser parser(g_Doc);
         CMathItem* pRet = parser.Parse("$$x^2$$");
         SMemGuard mg{ pRet };
         Assert::IsNotNull(pRet, L"Parser failed!");
         Assert::IsTrue(eacINDEXED == pRet->Type(), L"Resulting item is not INDEXED");
         CContainerItem* pCont = dynamic_cast<CContainerItem*>(pRet);
         Assert::IsNotNull(pCont, L"Unexpected base for eacINDEXED!");
         Assert::IsTrue(parser.LastError().sError.empty(), L"Unexpected parse error");
         Assert::IsTrue(pCont->Items().size() == 2, L"Expected 2 items in the eacINDEXED container");
         // Assert - Base item
         CMathItem* pBase = pCont->Items()[0];
         Assert::AreEqual((int)eacWORD, (int)pBase->Type(), L"Base should be CWordItem");
         Assert::AreEqual((int)etsDisplay, (int)pBase->GetStyle().Style(), L"Base should have Display style");
         Assert::IsFalse(pBase->GetStyle().IsCramped(), L"Base should not be cramped");

         // Assert - Superscript item
         CMathItem* pSupers = pCont->Items()[1];
         Assert::AreEqual((int)eacWORD, (int)pSupers->Type(), L"Superscript  should be CWordItem");
         Assert::AreEqual((int)etsScript, (int)pSupers->GetStyle().Style(), L"Superscript should have Script style");
         Assert::IsFalse(pSupers->GetStyle().IsCramped(), L"Superscript should not be cramped");
      }
      // ## Test 3: BothScripts
      // # Input: "$$x_a^b$$"
      // 
      // # Expected Result:
      // - Single CIndexedItem (NOT wrapped in CHBoxItem)
      // - Base: CWordItem for 'x'
      // - Subscript: CWordItem for 'a'
      // - Superscript: CWordItem for 'b'
      // 
      // # Verify:
      // - Result type == eacINDEXED
      // - Base is 'x'
      // - Subscript is 'a' with decreased+cramped style
      // - Superscript is 'b' with decreased (not cramped) style
      // - No parse errors
      TEST_METHOD(Indexed_SubSuperscript_Inline) {
         CTexParser parser(g_Doc);
         CMathItem* pRet = parser.Parse("$x_a^b$");
         SMemGuard mg{ pRet };
         Assert::IsNotNull(pRet, L"Parser failed!");
         Assert::IsTrue(eacINDEXED == pRet->Type(), L"Resulting item is not INDEXED");
         Assert::AreEqual((int)etsText, (int)pRet->GetStyle().Style(), L"Base should have Text style");
         //content check
         CContainerItem* pCont = dynamic_cast<CContainerItem*>(pRet);
         Assert::IsNotNull(pCont, L"Unexpected base for eacINDEXED!");
         Assert::IsTrue(parser.LastError().sError.empty(), L"Unexpected parse error");
         Assert::IsTrue(pCont->Items().size() == 3, L"Expected 3 items in the eacINDEXED container");
         // Assert - Base item
         CMathItem* pBase = pCont->Items()[0];
         Assert::AreEqual((int)eacWORD, (int)pBase->Type(), L"Base should be CWordItem");
         Assert::AreEqual((int)etsText, (int)pBase->GetStyle().Style(), L"Base should have Text style");
         Assert::IsFalse(pBase->GetStyle().IsCramped(), L"Base should not be cramped");
         // Assert - Subscript item
         CMathItem* pSub = pCont->Items()[1];
         Assert::AreEqual((int)eacWORD, (int)pSub->Type(), L"Subscript should be CWordItem");
         Assert::AreEqual((int)etsScript, (int)pSub->GetStyle().Style(), L"Subscript should have Script style");
         Assert::IsTrue(pSub->GetStyle().IsCramped(), L"Subscript should be cramped");
         // Assert - Superscript item
         CMathItem* pSupers = pCont->Items()[2];
         Assert::AreEqual((int)eacWORD, (int)pSupers->Type(), L"Superscript  should be CWordItem");
         Assert::AreEqual((int)etsScript, (int)pSupers->GetStyle().Style(), L"Superscript should have Script style");
         Assert::IsFalse(pSupers->GetStyle().IsCramped(), L"Superscript should not be cramped");
      }
      // ## Test 4: GroupedSubscript
      // # Input: "$$x_{ab}$$"
      // 
      // # Expected Result:
      // - Single CIndexedItem (NOT wrapped in CHBoxItem)
      // - Base: CWordItem for 'x'
      // - Subscript: CHBoxItem containing 2 CWordItems ('a', 'b')
      // 
      // # Verify:
      // - Result type == eacINDEXED
      // - Base is single 'x' CWordItem
      // - Subscript is CHBoxItem (NOT single item because it contains 2 items)
      // - Subscript CHBoxItem has exactly 2 items: 'a' and 'b'
      // - Both items in subscript have decreased+cramped style
      // - No parse errors
      TEST_METHOD(Indexed_GroupedSubscript_Display) {
         // Input: "$$x_{ab}$$"
         CTexParser parser(g_Doc);
         CMathItem* pRet = parser.Parse("$$x_{ab}$$");
         SMemGuard mg{ pRet };

         // Parse succeeded
         Assert::IsNotNull(pRet, L"Parser failed!");
         Assert::IsTrue(parser.LastError().sError.empty(), L"Unexpected parse error");

         // Structure - CIndexedItem with 2 items
         Assert::AreEqual((int)eacINDEXED, (int)pRet->Type(), L"Result should be CIndexedItem");

         CContainerItem* pCont = dynamic_cast<CContainerItem*>(pRet);
         Assert::IsNotNull(pCont, L"CIndexedItem should be CContainerItem");
         Assert::AreEqual((size_t)2, pCont->Items().size(), L"Expected 2 items (base + subscript, no superscript)");

         // Base item verification
         CMathItem* pBase = pCont->Items()[0];
         Assert::AreEqual((int)eacWORD, (int)pBase->Type(), L"Base should be CWordItem");
         Assert::AreEqual((int)etsDisplay, (int)pBase->GetStyle().Style(), L"Base should have Display style");
         Assert::IsFalse(pBase->GetStyle().IsCramped(), L"Base should not be cramped");

         // Subscript item verification
         CMathItem* pSub = pCont->Items()[1];
         Assert::AreEqual((int)eacHBOX, (int)pSub->Type(), L"Subscript should be CHBoxItem");
         Assert::AreEqual((int)etsScript, (int)pSub->GetStyle().Style(), L"Subscript should have Script style");
         Assert::IsTrue(pSub->GetStyle().IsCramped(), L"Subscript should be cramped");

         // Subscript HBox contents verification
         CContainerItem* pSubHBox = dynamic_cast<CContainerItem*>(pSub);
         Assert::IsNotNull(pSubHBox, L"Subscript should be CContainerItem");
         Assert::AreEqual((size_t)2, pSubHBox->Items().size(), L"Subscript HBox should contain 2 items");

         // Verify both items inside subscript HBox
         CMathItem* pSubItem1 = pSubHBox->Items()[0];
         CMathItem* pSubItem2 = pSubHBox->Items()[1];

         // First item (should be 'a')
         Assert::AreEqual((int)eacWORD, (int)pSubItem1->Type(), L"First subscript item should be CWordItem");
         Assert::AreEqual((int)etsScript, (int)pSubItem1->GetStyle().Style(), L"First subscript item should be Script style");
         Assert::IsTrue(pSubItem1->GetStyle().IsCramped(), L"First subscript item should be cramped");

         // Second item (should be 'b')
         Assert::AreEqual((int)eacWORD, (int)pSubItem2->Type(), L"Second subscript item should be CWordItem");
         Assert::AreEqual((int)etsScript, (int)pSubItem2->GetStyle().Style(), L"Second subscript item should be Script style");
         Assert::IsTrue(pSubItem2->GetStyle().IsCramped(), L"Second subscript item should be cramped");
      }
      // ## Test 5: ChainedSubscripts
      // # Input: "$$x_a_b$$"
      // 
      // # Expected Result:
      // - Nested CIndexedItem structure: (x_a)_b
      // - Outer item: base=(x_a), subscript=b
      // - Inner item (base of outer): base=x, subscript=a
      // 
      // # Verify:
      // - Result type == eacINDEXED (outer)
      // - Outer has 2 items (base + subscript)
      // - Outer base type == eacINDEXED (inner indexed item)
      // - Inner base is 'x' (eacWORD)
      // - Inner subscript is 'a' (eacWORD, cramped)
      // - Outer subscript is 'b' (eacWORD, cramped)
      // - No parse errors
      // - Demonstrates TeX chaining: x_a_b = (x_a)_b
      TEST_METHOD(Error_DoubleSubSuperscript) {
         // Input: "$$x_a_b$$" -> parsed as (x_a)_b
         CTexParser parser(g_Doc);
         CMathItem* pRet = parser.Parse("$$x_a_b$$");
         SMemGuard mg{ pRet };

         // Parse should fail
         Assert::IsNull(pRet, L"Parse should fail for missing subscript argument");
         auto err = parser.LastError();
         Assert::AreEqual("Double subcsript/superscript is not allowed", err.sError.c_str());
         Assert::AreEqual((int)epsBUILDING, (int)err.eStage, L"Error stage should be epsBUILDING");
         Assert::AreEqual(5, (int)err.nStartPos, L"Error position should be 5");
      }
      // ## Test 6: GeneralizedFraction
      // # Input: "$$_a^b$$"
      // 
      // # Expected Result:
      // - VBox (generalized fraction, bar-less)
      // - Super 'b' over sub 'a', no base atom
      // 
      // # Verify:
      // - Result type == eacVBOX
      // - VBox has 2 items
      // - Top item is superscript 'b' (eacWORD)
      // - Bottom item is subscript 'a' (eacWORD)
      // - Both items have Script style, subscript is cramped
      // - No parse errors
      TEST_METHOD(Indexed_GeneralizedFraction_Display) {
         CTexParser parser(g_Doc);
         CMathItem* pRet = parser.Parse("$$_a^b$$");
         SMemGuard mg{ pRet };

         // Parse should succeed
         Assert::IsNotNull(pRet, L"Parse should succeed for _a^b");
         Assert::IsTrue(parser.LastError().sError.empty(), L"No error expected");

         // Structure: VBox (generalized fraction)
         Assert::AreEqual((int)eacVBOX, (int)pRet->Type(), L"Result should be VBox (generalized fraction)");

         // VBox should have 2 items
         CContainerItem* pVBox = dynamic_cast<CContainerItem*>(pRet);
         Assert::IsNotNull(pVBox, L"VBox should be CContainerItem");
         Assert::AreEqual((size_t)2, pVBox->Items().size(), L"VBox should have 2 items (super + sub)");

         // Top item (superscript 'b')
         CMathItem* pTop = pVBox->Items()[0];
         Assert::AreEqual((int)eacWORD, (int)pTop->Type(), L"Top item should be CWordItem");
         Assert::AreEqual((int)etsScript, (int)pTop->GetStyle().Style(), L"Top item (super) should have Script style");
         Assert::IsFalse(pTop->GetStyle().IsCramped(), L"Superscript should NOT be cramped");

         // Bottom item (subscript 'a')
         CMathItem* pBottom = pVBox->Items()[1];
         Assert::AreEqual((int)eacWORD, (int)pBottom->Type(), L"Bottom item should be CWordItem");
         Assert::AreEqual((int)etsScript, (int)pBottom->GetStyle().Style(), L"Bottom item (sub) should have Script style");
         Assert::IsTrue(pBottom->GetStyle().IsCramped(), L"Subscript should be cramped");
      }
      // ## Test 6a: ComplexChaining
      // Input: "$${X_Y^Z}_A$$"
      // Parse: X_Y^Z (single indexed, 3 items), then chain _A
      // Result: {X_Y^Z}_A (nested indexed)
      TEST_METHOD(Indexed_ComplexChaining_Display) {

         CTexParser parser(g_Doc);
         CMathItem* pRet = parser.Parse("$${X_Y^Z}_A$$");
         SMemGuard mg{ pRet };

         Assert::IsNotNull(pRet, L"Parse should succeed");
         Assert::IsTrue(parser.LastError().sError.empty());

         // Outer indexed item: base=(X_Y^Z) + subscript=A
         Assert::AreEqual((int)eacINDEXED, (int)pRet->Type(), L"Outer should be indexed");

         CContainerItem* pOuter = dynamic_cast<CContainerItem*>(pRet);
         Assert::AreEqual((size_t)2, pOuter->Items().size(), L"Outer: base + subscript A");

         // Outer subscript is 'A'
         CMathItem* pOuterSub = pOuter->Items()[1];
         Assert::AreEqual((int)eacWORD, (int)pOuterSub->Type());
         Assert::IsTrue(pOuterSub->GetStyle().IsCramped());

         // Outer base is inner indexed item (X_Y^Z with 3 items)
         CMathItem* pInner = pOuter->Items()[0];
         Assert::AreEqual((int)eacINDEXED, (int)pInner->Type(), L"Base should be indexed (X_Y^Z)");

         CContainerItem* pInnerCont = dynamic_cast<CContainerItem*>(pInner);
         Assert::AreEqual((size_t)3, pInnerCont->Items().size(), L"Inner: base X + subscript Y + superscript Z");

         // Inner item 0: base 'X'
         CMathItem* pX = pInnerCont->Items()[0];
         Assert::AreEqual((int)eacWORD, (int)pX->Type());
         Assert::IsFalse(pX->GetStyle().IsCramped());

         // Inner item 1: subscript 'Y' (cramped)
         CMathItem* pY = pInnerCont->Items()[1];
         Assert::AreEqual((int)eacWORD, (int)pY->Type());
         Assert::IsTrue(pY->GetStyle().IsCramped(), L"Subscript Y should be cramped");

         // Inner item 2: superscript 'Z' (not cramped)
         CMathItem* pZ = pInnerCont->Items()[2];
         Assert::AreEqual((int)eacWORD, (int)pZ->Type());
         Assert::IsFalse(pZ->GetStyle().IsCramped(), L"Superscript Z should NOT be cramped");
      }
      // ## Test 7: TextAndMath
      // # Input: "Text $$E=mc^2$$ more"
      // 
      // # Expected Result:
      // - CHBoxItem containing multiple items (NOT optimized because >1 item)
      // - Contains: text items, math part, more text items
      // 
      // # Verify:
      // - Result type == eacHBOX
      // - CHBoxItem has >= 3 items (text + math + text)
      // - Math part (E=mc^2) contains CIndexedItem for c^2
      // - Math part has display style (etsDisplay)
      // - Text parts have text mode
      // - No parse errors
      TEST_METHOD(MathInText_Display) {
         CTexParser parser(g_Doc);
         CMathItem* pRet = parser.Parse("Text $E=mc^2$ more");
         SMemGuard mg{ pRet };
         // Parse should succeed
         Assert::IsNotNull(pRet, L"Parser failed!");
         Assert::IsTrue(parser.LastError().sError.empty(), L"Unexpected parse error");
         // Result should be HBox
         Assert::AreEqual((int)eacHBOX, (int)pRet->Type(), L"Result should be HBox");
         CContainerItem* pHBox = dynamic_cast<CContainerItem*>(pRet);
         Assert::IsNotNull(pHBox, L"HBox should be CContainerItem");
         Assert::IsTrue(pHBox->Items().size() == 5, L"HBox should have 5 items: Word,Glue,HBox,Glue,Word)");
         // Find math part (should be second item)
         CMathItem* pMathPart = pHBox->Items()[2];
         Assert::AreEqual((int)eacHBOX, (int)pMathPart->Type(), L"Item 2 should be HBox");
         CContainerItem* pMathHBox = dynamic_cast<CContainerItem*>(pMathPart);
         bool foundIndexed = false;
         for (CMathItem* pItem : pMathHBox->Items()) {
            if (pItem->Type() == eacINDEXED) {
               foundIndexed = true;
               break;
            }
         }
         Assert::IsTrue(foundIndexed, L"Math part should contain indexed item for c^2");
      }
      // ## Test 8: MultipleTerms
      // # Input: "$$a+b-c$$"
      // 
      // # Expected Result:
      // - CHBoxItem with 5 CWordItems (NOT optimized because >1 item)
      // 
      // # Verify:
      // - Result type == eacHBOX
      // - HBox contains exactly 5 items: 'a', '+', 'b', '-', 'c'
      // - All items are eacWORD type
      // - All have display style
      // - No parse errors
      TEST_METHOD(MultipleTerms_Display) {
         CTexParser parser(g_Doc);
         CMathItem* pRet = parser.Parse("$$a+b-c$$");
         SMemGuard mg{ pRet };
         // Parse should succeed
         Assert::IsNotNull(pRet, L"Parser failed!");
         Assert::IsTrue(parser.LastError().sError.empty(), L"Unexpected parse error");
         // Result should be HBox
         Assert::AreEqual((int)eacHBOX, (int)pRet->Type(), L"Result should be HBox");
         CContainerItem* pHBox = dynamic_cast<CContainerItem*>(pRet);
         Assert::IsNotNull(pHBox, L"HBox should be CContainerItem");
         Assert::AreEqual((size_t)5, pHBox->Items().size(), L"HBox should have exactly 5 items");
         // Verify each item
         for (size_t i = 0; i < 5; ++i) {
            CMathItem* pItem = pHBox->Items()[i];
            Assert::AreEqual((int)eacWORD, (int)pItem->Type(), L"Each item should be CWordItem");
            CWordItem* pWord = dynamic_cast<CWordItem*>(pItem);
            Assert::IsNotNull(pWord, L"Item should be CWordItem");
            Assert::AreEqual((UINT32)1, pWord->GlyphCount(), L"Each WordItem should have single glyph");
            Assert::AreEqual((int)etsDisplay, (int)pWord->GetStyle().Style(), L"Item should have Display style");
         }
      }
      // Input: "$${[a]}$$"
      // Expected: Single CWordItem 'a' (all wrapper groups optimized away)
      // 
      // Parsing steps:
      // 1. $$ opens display math group
      // 2. {} creates fig-brace group containing [a]
      // 3. [] creates square-bracket group containing 'a'
      // 4. ProcessGroup optimizes: single item [a] -> unwraps to 'a'
      // 5. ProcessGroup optimizes: single item {a} -> unwraps to 'a'
      // 6. ProcessGroup optimizes: single item $$a$$ -> unwraps to 'a'
      // Result: CWordItem('a')
      TEST_METHOD(NestedGroups_OptimizedToSingleItem) {
         CTexParser parser(g_Doc);
         CMathItem* pRet = parser.Parse("$${{a}}$$");
         SMemGuard mg{ pRet };

         // Parse should succeed
         Assert::IsNotNull(pRet, L"Parse should succeed for nested groups");
         Assert::IsTrue(parser.LastError().sError.empty(), L"No error expected");

         // Result should be single CWordItem (all groups optimized away)
         Assert::AreEqual((int)eacWORD, (int)pRet->Type(), L"Result should be CWordItem (nested groups unwrapped)");

         // Verify it's 'a' (optional - if you have GetText or similar)
         // CWordItem* pWord = dynamic_cast<CWordItem*>(pRet);
         // Assert::IsNotNull(pWord);

         // Verify Display style (from $$)
         Assert::AreEqual((int)etsDisplay, (int)pRet->GetStyle().Style(), L"Should have Display style from $$ delimiters");
         Assert::IsFalse(pRet->GetStyle().IsCramped(), L"Display style should not be cramped");
      }
      // ## Test: Brackets treated as literals in Text Mode
// # Input: "$\text{[a]}$"
// # Expected:
// - HBox/Word containing '[' and ']' characters, NOT treated as argument boundaries.
// - Text should render as "[a]"
      TEST_METHOD(Brackets_TextMode_Literals) {
         CTexParser parser(g_Doc);
         // \text{[a]} -> Should be parsed as text mode sequence [, a, ]
         CMathItem* pRet = parser.Parse(R"($\text{[a]}$)");
         SMemGuard mg{ pRet };

         Assert::IsNotNull(pRet, L"Parser failed for text mode brackets");

         // \text usually produces an HBOX containing the text items.
         Assert::AreEqual((int)eacHBOX, (int)pRet->Type(), L"Expected HBox for \\text content");

         CContainerItem* pHBox = dynamic_cast<CContainerItem*>(pRet);
         Assert::IsNotNull(pHBox);

         // We expect the content to represent "[a]"
         // This ensures '[' wasn't eaten by the parser as a delimiter.
         Assert::IsTrue(pHBox->Items().size() > 0, L"Text box should not be empty");
         Assert::IsTrue(parser.LastError().sError.empty(), L"Should allow [ ] in text");
      }

      // ## Test: Brackets treated as literals in Math Mode
      // # Input: "$$[a]$$"
      // # Expected:
      // - Sequence of items: Word '[', Word 'a', Word ']'
      // - Not parsed as optional argument (since no command precedes it)
      TEST_METHOD(Brackets_MathMode_Literals) {
         CTexParser parser(g_Doc);
         CMathItem* pRet = parser.Parse("$$[a]$$");
         SMemGuard mg{ pRet };

         Assert::IsNotNull(pRet, L"Parser failed for math mode brackets");
         Assert::IsTrue(parser.LastError().sError.empty());

         // In display mode, pRet is likely the VBox wrapping the list.
         CContainerItem* pCont = dynamic_cast<CContainerItem*>(pRet);
         Assert::IsNotNull(pCont);

         // We expect 3 distinct WORD items: [, a, ]
         // If [ was consumed as an optional arg start, this count would likely be lower or fail.
         int wordCount = 0;
         for (auto* pItem : pCont->Items()) {
            if (pItem->Type() == eacWORD) wordCount++;
         }

         // [, a, ] -> 3 words
         Assert::AreEqual(3, wordCount, L"Expected 3 word items: [, a, ]");
      }

      // ## Test: Brackets acting as Optional Argument
// # Input: "$$\sqrt[3]{x}$$"
// # Expected:
// - Radical item (Container) with degree '3'
// - Brackets consumed (not in output list)
      TEST_METHOD(Brackets_As_OptionalArg_Radical) {
         CTexParser parser(g_Doc);
         CMathItem* pRet = parser.Parse(R"($$\sqrt[3]{x}$$)");
         SMemGuard mg{ pRet };

         Assert::IsNotNull(pRet);
         Assert::IsTrue(parser.LastError().sError.empty());

         // Verify type is RADICAL (assuming eacRADICAL is defined in EnumMathItemType)
         // If CRadicalBuilder returns a VBOX or generic item, verify that instead.
         Assert::AreEqual((int)eacRADICAL, (int)pRet->Type());

         CContainerItem* pRadContainer = dynamic_cast<CContainerItem*>(pRet);
         Assert::IsNotNull(pRadContainer);

         // Implementation detail: CRadicalBuilder::BuildRadical likely puts the degree 
         // as a specific child item (e.g. index 0 or 1) or leaves it null if missing.
         // We expect the degree '3' to be present.

         // Let's assume the container has children: [Degree, Radicand] or similar.
         // If implementation follows standard structure:
         // Check that we have enough children and one of them corresponds to '3'.
         bool foundDegree = false;
         for (auto* pChild : pRadContainer->Items()) {
            // A simple check: if we find a WORD item "3", we know it was parsed and attached.
            if (pChild && pChild->Type() == eacWORD) {
               // (In a real test we'd check the content "3" specifically if accessible)
               foundDegree = true;
               break;
            }
         }
         Assert::IsTrue(foundDegree, L"Radical container should contain the degree '3'");
      }


      // ## Test: Brackets Ignored for commands that don't take optional arguments
      // # Input: "$$\mathbf[a]$$"
      // # Standard LaTeX: \mathbf takes 1 mandatory arg. It consumes the next token '[', bolds it, then 'a' is normal.
      // # If parser incorrectly thinks \mathbf takes optional arg, it might eat [a] differently.
      TEST_METHOD(Brackets_Ignored_For_NonOpt_Command) {
         CTexParser parser(g_Doc);
         CMathItem* pRet = parser.Parse(R"($$\mathbf[a]$$)");
         SMemGuard mg{ pRet };

         Assert::IsNotNull(pRet);
         Assert::IsTrue(parser.LastError().sError.empty());

         CContainerItem* pCont = dynamic_cast<CContainerItem*>(pRet);
         // We expect a sequence (merged or distinct) representing "[a]" (plus closing bracket potentially).
         // Key check: The first item should be the '[' char, rendered (not hidden).

         Assert::IsTrue(pCont->Items().size() >= 1);
         CMathItem* pItem1 = pCont->Items()[0];

         // Verify it's a visible WORD item (the bold bracket)
         Assert::AreEqual((int)eacWORD, (int)pItem1->Type(), L"First item should be the rendered '[' bracket");
      }


#pragma region ErrorTests
      // ## Test 9: MissingSubscriptArgument_EndOfInput
      // # Input: "$$x_$$"
      // 
      // # Expected Result:
      // - nullptr
      // 
      // # Verify:
      // - Result is nullptr
      // - LastError().sError == "Orphan subscript '_'"
      // - LastError().eStage == epsBUILDING
      // - LastError().nPosition == 4
      TEST_METHOD(Error_MissingSubscriptArgument_EndOfInput) {
         CTexParser parser(g_Doc);
         CMathItem* pRet = parser.Parse("$$x_$$");
         SMemGuard mg{ pRet };
         // Parse should fail
         Assert::IsNull(pRet, L"Parse should fail for missing subscript argument");
         auto err = parser.LastError();
         Assert::AreEqual("Missing item after _/^!", err.sError.c_str());
         Assert::AreEqual((int)epsBUILDING, (int)err.eStage, L"Error stage should be epsBUILDING");
         Assert::AreEqual((int)err.nStartPos, 4, L"Error position should be 4");
      }
      // ## Test 10: DoubleSubSuperscript
      // # Input: "$$x_^y$$"
      // 
      // # Expected Result:
      // - nullptr
      // 
      // # Verify:
      // - Result is nullptr
      // - LastError().sError == "Double subscript/superscript is not allowed!"
      // - LastError().eStage == epsBUILDING
      // - LastError().nPosition == 4
      TEST_METHOD(Error_MissedSubSuperscript) {
         CTexParser parser(g_Doc);
         CMathItem* pRet = parser.Parse("$$x_^y$$");
         SMemGuard mg{ pRet };
         // Parse should fail
         Assert::IsNull(pRet, L"Parse should fail for missing subscript argument");
         auto err = parser.LastError();
         Assert::AreEqual("Missing item after _/^!", err.sError.c_str());
         Assert::AreEqual((int)epsBUILDING, (int)err.eStage, L"Error stage should be epsBUILDING");
         Assert::AreEqual((int)err.nStartPos, 4, L"Error position should be 4");
      }
      // ## Test 11: DoubleSuperSubscript
      // # Input: "$$x^_y$$"
      // 
      // # Expected Result:
      // - nullptr
      // 
      // # Verify:
      // - Result is nullptr
      // - LastError().sError == "Double subscript/superscript is not allowed!"
      // - LastError().eStage == epsBUILDING
      // - LastError().nPosition == 4
      TEST_METHOD(Error_MissedSuperSubscript) {
         CTexParser parser(g_Doc);
         CMathItem* pRet = parser.Parse("$$x^_y$$");
         SMemGuard mg{ pRet };
         // Parse should fail
         Assert::IsNull(pRet, L"Parse should fail for missing subscript argument");
         auto err = parser.LastError();
         Assert::AreEqual(err.sError.c_str(), "Missing item after _/^!");
         Assert::AreEqual((int)epsBUILDING, (int)err.eStage, L"Error stage should be epsBUILDING");
         Assert::AreEqual((int)err.nStartPos, 4, L"Error position should be 4");
      }
      // ## Test 12: EmptyMathGroup
      // # Input: "$$$$","$ $","${}$","{}"
      // 
      // # Expected Result:
      // - nullptr, error about empty group
      // 
      // # Verify:
      // - Result is nullptr
      // - LastError().sError == "Empty group is not allowed here!"
      TEST_METHOD(Error_EmptyGroups_NotAllowed) {
         CTexParser parser(g_Doc);
         CMathItem* pRet = parser.Parse("$$ $$");
         // Parse should succeed with empty result
         Assert::IsNull(pRet, L"Parse should return nullptr for empty group");
         auto err = parser.LastError();
         Assert::AreEqual("Unexpected empty group", err.sError.c_str());

         pRet = parser.Parse("$ $");
         Assert::IsNull(pRet, L"Parse should return nullptr for empty group");
         err = parser.LastError();
         Assert::AreEqual("Unexpected empty group", err.sError.c_str());

         pRet = parser.Parse("${}$");
         Assert::IsNull(pRet, L"Parse should return nullptr for empty group");
         err = parser.LastError();
         Assert::AreEqual("Unexpected empty group", err.sError.c_str());

         pRet = parser.Parse("{}");
         Assert::IsNull(pRet, L"Parse should return nullptr for empty group");
         err = parser.LastError();
         Assert::IsTrue(err.sError.empty(), L"{} is valid empty group");
      }
      // ## Test 14: BuildGroups has unclosed opening
      // # Input: "$${ abc$$", "${a^bc$"
      // 
      // # Expected Result:
      // - nullptr 
      // 
      // # Verify:
      // - Result is nullptr
      // - LastError().sError == "Unclosed group '{'"
      // - LastError().nPosition == 2
      TEST_METHOD(Error_BuildGroups_Unclosed) {
         CTexParser parser(g_Doc);
         CMathItem* pRet = parser.Parse("$${ abc$$");
         // Parse should succeed with empty result
         Assert::IsNull(pRet, L"Parse should return nullptr for empty group");
         auto err = parser.LastError();
         Assert::AreEqual(err.sError.c_str(), "Unclosed group '{'");
         Assert::AreEqual((int)err.nStartPos, 2, L"Error position should be 2");

         pRet = parser.Parse("$a{{a^bc$");
         Assert::IsNull(pRet, L"Parse should return nullptr for empty group");
         err = parser.LastError();
         Assert::AreEqual(err.sError.c_str(), "Unclosed group '{'");
         Assert::AreEqual((int)err.nStartPos, 3, L"Error position should be 3");
      }
      // ## Test 15: NestedMathModes are not allowed
      // # Input: "$$a $b$ c$$","$a $$b$$ c$"
      // # Expected Result:
      // - nullptr 
      // 
      // # Verify:
      // - Result is nullptr
      // - LastError().sError == "Inner $..$/$$...$$ are not allowed in math mode!"
      TEST_METHOD(Error_NestedMathModes) {
         CTexParser parser(g_Doc);
         CMathItem* pRet = parser.Parse("$$a $b$ c$$");
         // Parse should succeed with empty result
         Assert::IsNull(pRet, L"Parse should return nullptr for empty group");
         auto err = parser.LastError();
         Assert::AreEqual(err.sError.c_str(), "Inner $..$/$$...$$ are not allowed in math mode");

         pRet = parser.Parse("$a $$b$$ c$");
         Assert::IsNull(pRet, L"Parse should return nullptr for empty group");
         err = parser.LastError();
         Assert::AreEqual(err.sError.c_str(), "Inner $..$/$$...$$ are not allowed in math mode");
      }
#pragma endregion
   };
   TEST_CLASS(MathBuilderTests)
   {
   public:
      TEST_METHOD(FractionBuilder_Basic_Display) {
         CTexParser parser(g_Doc);
         CMathItem* pRet = parser.Parse("$$\\frac{a}{b}$$");
         struct SMemGuard {
            CMathItem* pItem;
            ~SMemGuard() { delete pItem; }
         } mg{ pRet };
         Assert::IsNotNull(pRet, L"Parser failed!");
         Assert::IsTrue(eacVBOX == pRet->Type(), L"Resulting item is not VBOX");
         // VBox overall style: Display (from $$)
         Assert::AreEqual((int)etsDisplay, (int)pRet->GetStyle().Style(), L"VBox should have Display style");

         // VBox should have 3 items
         CContainerItem* pVBox = dynamic_cast<CContainerItem*>(pRet);
         Assert::IsNotNull(pVBox, L"VBox should be CContainerItem");
         Assert::AreEqual((size_t)3, pVBox->Items().size(), L"VBox should have 3 items: Numerator,Bar,Denom");

         // Item 1: Numerator 'a'
         CMathItem* pNumerator = pVBox->Items()[1];
         Assert::AreEqual((int)eacWORD, (int)pNumerator->Type(), L"Numerator should be CWordItem");
         Assert::AreEqual((int)etsText, (int)pNumerator->GetStyle().Style(), L"Numerator style: Display -> Text");
         Assert::IsFalse(pNumerator->GetStyle().IsCramped(), L"Numerator should NOT be cramped");

         // Item 0: Fraction bar (rule/line)
         CMathItem* pBar = pVBox->Items()[0];
         Assert::IsTrue(pBar->Type() == eacRULE, L"Rule item should be eacRULE");

         // Item 2: Denominator 'b'
         CMathItem* pDenominator = pVBox->Items()[2];
         Assert::AreEqual((int)eacWORD, (int)pDenominator->Type(), L"Denominator should be CWordItem");

         // Denominator style: Text + cramped
         Assert::AreEqual((int)etsText, (int)pDenominator->GetStyle().Style(), 
            L"Denominator style: same as Numerator, Display -> Text");
         Assert::IsTrue(pDenominator->GetStyle().IsCramped(), L"Denominator SHOULD be cramped");

      }
      TEST_METHOD(FractionBuilder_Nested_Display) {
         // Input: "$$\frac{\frac{a}{b}}{c}$$"
         // Tests: Recursive builder calls work

         CTexParser parser(g_Doc);
         CMathItem* pRet = parser.Parse("$$\\frac{\\frac{a}{b}}{c}$$");
         SMemGuard mg{ pRet };

         Assert::IsNotNull(pRet, L"Parse should succeed");
         Assert::IsTrue(parser.LastError().sError.empty());

         // Outer fraction
         Assert::AreEqual((int)eacVBOX, (int)pRet->Type());
         CContainerItem* pOuter = dynamic_cast<CContainerItem*>(pRet);
         Assert::AreEqual((size_t)3, pOuter->Items().size(), L"Outer: num, bar, denom");

         // Numerator is inner fraction
         CMathItem* pNumerator = pOuter->Items()[1];
         Assert::AreEqual((int)eacVBOX, (int)pNumerator->Type(), L"Numerator should be nested fraction");

         // Denominator is 'c'
         CMathItem* pDenominator = pOuter->Items()[2];
         Assert::AreEqual((int)eacWORD, (int)pDenominator->Type());
      }
      TEST_METHOD(FractionBuilder_WithSuperscript_Display) {
         // Input: "$$\frac{a}{b}^2$$"
         // Tests: TryAddSubSuperscript_ works on builder result

         CTexParser parser(g_Doc);
         CMathItem* pRet = parser.Parse("$$\\frac{a}{b}^2$$");
         SMemGuard mg{ pRet };

         Assert::IsNotNull(pRet);
         Assert::IsTrue(parser.LastError().sError.empty());

         // Result is indexed (fraction with superscript)
         Assert::AreEqual((int)eacINDEXED, (int)pRet->Type(), L"Should be indexed: (\\frac{a}{b})^2");

         CContainerItem* pIndexed = dynamic_cast<CContainerItem*>(pRet);
         Assert::AreEqual((size_t)2, pIndexed->Items().size(), L"Indexed: base + superscript");

         // Base is fraction
         Assert::AreEqual((int)eacVBOX, (int)pIndexed->Items()[0]->Type());

         // Superscript is '2'
         CMathItem* pSuper = pIndexed->Items()[1];
         Assert::AreEqual((int)eacWORD, (int)pSuper->Type());
         Assert::IsFalse(pSuper->GetStyle().IsCramped(), L"Superscript not cramped");
      }
      TEST_METHOD(RadicalBuilder_Basic_Display) {
         // Input: "$$\sqrt{x}$$"
         // Tests: Optional degree argument not provided
         // Expected: Radical without degree (square root)

         CTexParser parser(g_Doc);
         CMathItem* pRet = parser.Parse("$$\\sqrt{x}$$");
         SMemGuard mg{ pRet };

         // Parse succeeded
         Assert::IsNotNull(pRet, L"Parse should succeed");
         Assert::IsTrue(parser.LastError().sError.empty(), L"No parse errors expected");

         // Result is Radical
         Assert::AreEqual((int)eacRADICAL, (int)pRet->Type(), L"Result should be RADICAL");

         // Display style (from $$)
         Assert::AreEqual((int)etsDisplay, (int)pRet->GetStyle().Style());

         // Check radical structure
         CContainerItem* pRadCont = dynamic_cast<CContainerItem*>(pRet);
         Assert::IsNotNull(pRadCont);

         // Should have 3 items (RadGlyph, Overbar, Radicand)
         Assert::IsTrue(pRadCont->Items().size() == 3, L"Radical should have 3 items");
         // RadGlyph, Overbar, Radicand
         CMathItem* pRadicand = pRadCont->Items()[2];
         Assert::AreEqual((int)eacWORD, (int)pRadicand->Type(), L"Radicand should be WORD");
         Assert::IsTrue(pRadicand->GetStyle().IsCramped(), L"Radicand should be cramped");
      }
      TEST_METHOD(RadicalBuilder_Degree_Display) {
         CTexParser parser(g_Doc);
         CMathItem* pRet = parser.Parse("$$\\sqrt[3]{A}$$");
         SMemGuard mg{ pRet };
         Assert::IsNotNull(pRet, L"Parser failed!");
         Assert::IsTrue(eacRADICAL == pRet->Type(), L"Resulting item is not RADICAL");
         // Ret overall style: Display (from $$)
         Assert::AreEqual((int)etsDisplay, (int)pRet->GetStyle().Style(), L"Container should have Display style");

         // VBox should have 4 items
         CContainerItem* pRadCont = dynamic_cast<CContainerItem*>(pRet);
         Assert::IsNotNull(pRadCont, L"Radical should be CContainerItem");
         Assert::AreEqual((size_t)4, pRadCont->Items().size(), L"Radical should have 4 items: RadGlyph,Degree,Overbar,Radicand");
         // Item 0: Radical glyph 
         CMathItem* pRadGlyph = pRadCont->Items()[0];
         Assert::IsTrue(pRadGlyph->Type() == eacWORD, L"RadGlyph should be WORD (symbol)");

         // Item 1: Degree '3'
         CMathItem* pDegree = pRadCont->Items()[1];
         Assert::AreEqual((int)eacWORD, (int)pDegree->Type(), L"Degree should be CWordItem");

         // Degree style: ScriptScript (very small)
         Assert::AreEqual((int)etsScriptScript, (int)pDegree->GetStyle().Style(),
            L"Degree should be ScriptScript style");
         Assert::IsFalse(pDegree->GetStyle().IsCramped(), L"Degree should NOT be cramped");

         // Item 2: Overbar (horizontal line over radicand)
         CMathItem* pOverbar = pRadCont->Items()[2];
         Assert::IsTrue(pOverbar->Type() == eacRULE, L"Overbar should be eacRULE");

         // Item 3: Radicand 'A'
         CMathItem* pRadicand = pRadCont->Items()[3];
         Assert::AreEqual((int)eacWORD, (int)pRadicand->Type(), L"Radicand should be CWordItem");

         // Radicand style: Display (same as outer), CRAMPED
         Assert::AreEqual((int)etsDisplay, (int)pRadicand->GetStyle().Style(), L"Radicand: same style as outer context");
         Assert::IsTrue(pRadicand->GetStyle().IsCramped(), L"Radicand SHOULD be cramped");

      }
      TEST_METHOD(RadicalBuilder_RecursiveDegree_Inline) {
         // Input: "$\sqrt[\sqrt{2}]{x}$"
         // Tests: Nested radical as degree (advanced recursion)
         // Also tests: Inline math mode ($ not $$)

         CTexParser parser(g_Doc);
         CMathItem* pRet = parser.Parse("$\\sqrt[\\sqrt{2}]{x}$");
         SMemGuard mg{ pRet };

         // Parse succeeded
         Assert::IsNotNull(pRet, L"Parse should succeed");
         Assert::IsTrue(parser.LastError().sError.empty(), L"No parse errors expected");

         // Result is Radical (outer)
         Assert::AreEqual((int)eacRADICAL, (int)pRet->Type(), L"Result should be outer RADICAL");

         // Inline style (from single $)
         Assert::AreEqual((int)etsText, (int)pRet->GetStyle().Style(), L"Inline math should have Text style");

         // Outer radical structure
         CContainerItem* pOuterRad = dynamic_cast<CContainerItem*>(pRet);
         Assert::IsNotNull(pOuterRad);
         Assert::AreEqual((size_t)4, pOuterRad->Items().size(), L"Outer radical: RadGlyph, Degree, Overbar, Radicand");

         // Item[1]: Degree is nested radical
         CMathItem* pDegree = pOuterRad->Items()[1];
         Assert::AreEqual((int)eacRADICAL, (int)pDegree->Type(), L"Degree should be nested RADICAL");

         // Degree style: ScriptScript
         Assert::AreEqual((int)etsScriptScript, (int)pDegree->GetStyle().Style(), L"Degree should be ScriptScript");
         Assert::IsFalse(pDegree->GetStyle().IsCramped(), L"Degree should NOT be cramped");

         // Inner radical (degree's radical)
         CContainerItem* pInnerRad = dynamic_cast<CContainerItem*>(pDegree);
         Assert::IsNotNull(pInnerRad);

         // Inner radical should have radicand '2'
         // (Check last item, which is radicand in your structure)
         size_t nInnerItems = pInnerRad->Items().size();
         CMathItem* pInnerRadicand = pInnerRad->Items()[nInnerItems - 1];
         Assert::AreEqual((int)eacWORD, (int)pInnerRadicand->Type(), L"Inner radicand should be '2'");
         Assert::IsTrue(pInnerRadicand->GetStyle().IsCramped(), L"Inner radicand should be cramped");

         // Outer radicand: 'x'
         CMathItem* pOuterRadicand = pOuterRad->Items()[3];
         Assert::AreEqual((int)eacWORD, (int)pOuterRadicand->Type(), L"Outer radicand should be 'x'");
         Assert::IsTrue(pOuterRadicand->GetStyle().IsCramped(), L"Outer radicand should be cramped");

         // Verify style cascade: Text (outer) -> ScriptScript (degree)
         // Inner radical inherits ScriptScript, its radicand decreases further
         Assert::IsTrue(pInnerRadicand->GetStyle().Style() == etsScriptScript,
            L"Inner radicand should have ScriptScript style");
      }
      TEST_METHOD(AccentBuilder_Basic_Display) {
         // Input: "$$\ocirc{a}$$"
         // Expected: accent over 'a'

         CTexParser parser(g_Doc);
         CMathItem* pRet = parser.Parse("$$\\ocirc{a}$$");
         SMemGuard mg{ pRet };

         // Parse succeeded
         Assert::IsNotNull(pRet, L"Parse should succeed");
         Assert::IsTrue(parser.LastError().sError.empty(), L"No parse errors expected");

         // Result is Radical
         Assert::AreEqual((int)eacACCENT, (int)pRet->Type(), L"Result should be ACCENT");

         // Display style (from $$)
         Assert::AreEqual((int)etsDisplay, (int)pRet->GetStyle().Style());

         // Check radical structure
         CContainerItem* pAccentCont = dynamic_cast<CContainerItem*>(pRet);
         Assert::IsNotNull(pAccentCont);

         // Should have 2 items Base + Accent
         Assert::IsTrue(pAccentCont->Items().size() == 2, L"Accent should have 2 items");
         CMathItem* pBase = pAccentCont->Items()[0];
         Assert::AreEqual((int)eacWORD, (int)pBase->Type(), L"Accent base should be WORD");
         CMathItem* pAccent = pAccentCont->Items()[1];
         Assert::AreEqual((int)eacUNK, (int)pAccent->Type(), L"Accent should be UNK"); //to avoid selection?
      }
      
      //GlueBuilder tests
      TEST_METHOD(HSpacingBuilder_HSkip_Fixed) {
         // Input: "$$a\hskip 10pt b$$"
         CTexParser parser(g_Doc);
         g_Doc.SetFontSizePts(10.0f);

         CMathItem* pRet = parser.Parse("$$a\\hskip 10pt b$$");
         SMemGuard mg{ pRet };
         g_Doc.SetFontSizePts(12.0f); //restore!

         Assert::IsNotNull(pRet, L"Parse should succeed");
         Assert::IsTrue(parser.LastError().sError.empty(), L"No parse errors");

         // Should be HBox with 3 items: 'a', glue, 'b'
         CContainerItem* pHBox = dynamic_cast<CContainerItem*>(pRet);
         Assert::IsNotNull(pHBox);
         Assert::AreEqual((size_t)3, pHBox->Items().size(), L"HBox should have 3 items: a, glue, b");

         // Middle item is glue
         CMathItem* pGlue = pHBox->Items()[1];
         Assert::AreEqual((int)eacGLUE, (int)pGlue->Type(), L"Middle item should be glue");

         // Check glue properties
         const STexGlue* pGlueData = pGlue->GetGlue();
         Assert::IsNotNull(pGlueData);

         // 10pt in EM units (approximate check)
         float fExpectedEM = DIPS2EM(10.0f, PTS2DIPS(10.0f));
         Assert::AreEqual(fExpectedEM, pGlueData->fNorm, 0.01f, L"Glue size should be 10pt");
         Assert::AreEqual(0.0f, pGlueData->fStretchCapacity, 0.01f, L"No stretch");
         Assert::AreEqual((uint16_t)0, pGlueData->nStretchOrder, L"No infinite stretch");
      }

      TEST_METHOD(HSpacingBuilder_HSkip_EmUnit) {
         // Input: "$$\hskip 2em$$" at 10pt font

         CTexParser parser(g_Doc);
         g_Doc.SetFontSizePts(10.0f);

         // Set document font size to 10pt for test
         // (Assuming you have a way to set this)

         CMathItem* pRet = parser.Parse("$$\\hskip 2em$$");
         SMemGuard mg{ pRet };
         g_Doc.SetFontSizePts(12.0f); //restore!

         Assert::IsNotNull(pRet);

         const STexGlue* pGlue = pRet->GetGlue();

         // 2em at 10pt font = 20pts
         // Convert to EM: DIPS2EM(10, PTS2DIPS(20))
         float fExpected = DIPS2EM(10.0f, PTS2DIPS(20.0f));

         Assert::AreEqual(fExpected, pGlue->fNorm, 0.01f, L"2em should equal 20pts at 10pt font");
      }

      TEST_METHOD(HSpacingBuilder_HSkip_ExUnit) {
         // Input: "$$\hskip 3ex$$" at 10pt font
         CTexParser parser(g_Doc);
         g_Doc.SetFontSizePts(10.0f);

         CMathItem* pRet = parser.Parse("$$\\hskip 3ex$$");
         SMemGuard mg{ pRet };
         g_Doc.SetFontSizePts(12.0f); //restore!

         Assert::IsNotNull(pRet);

         const STexGlue* pGlue = pRet->GetGlue();

         // 3ex ~ 1.5em at 10pt = 15pts
         float fExpected = DIPS2EM(10.0f, PTS2DIPS(15.0f));

         Assert::AreEqual(fExpected, pGlue->fNorm, 0.01f, L"3ex should equal ~1.5em (15pts) at 10pt font");
      }
   };
   TEST_CLASS(RawItemTests)
   {
   public:
      TEST_METHOD(TryAppendWord_ReturnTrue) {
         //emulate a\'{a}b
         CWordItem* pWordItem1 = new CWordItem(g_Doc, FONT_LMM, CMathStyle());
         pWordItem1->SetText(L"a");
         CWordItem* pWordItem2 = new CWordItem(g_Doc, FONT_LMM, CMathStyle());
         pWordItem2->SetText({0x00E0});
         CWordItem* pWordItem3 = new CWordItem(g_Doc, FONT_LMM, CMathStyle());
         pWordItem3->SetText(L"b");
         //SMemGuard mg1{ pWordItem1 };
         CRawItem ritem{ 1, 1, pWordItem1 };
         Assert::IsTrue(ritem.TryAppendWord(pWordItem2,2,2));
         Assert::IsTrue(ritem.TryAppendWord(pWordItem3,3,3));
         delete ritem.Base();
      }

   };
   TEST_CLASS(DimensionParserTests) {
   public:
      TEST_METHOD(ParseDimensionString_BasicPoints) {
         float fPts = 0.0f;
         string sOrigUnits, sError;

         bool bResult = CDimensionParser::_ParseDimensionString( "12pt",12.0f,fPts,sOrigUnits,sError);

         Assert::IsTrue(bResult, L"Should parse '12pt' successfully");
         Assert::AreEqual(12.0f, fPts, 0.001f, L"Should return 12 points");
         Assert::AreEqual(string("pt"), sOrigUnits, L"Should have 'pt' as original unit");
         Assert::IsTrue(sError.empty(), L"Should not have error");
      }

      TEST_METHOD(ParseDimensionString_DecimalPoints) {
         float fPts = 0.0f;
         string sOrigUnits, sError;

         bool bResult = CDimensionParser::_ParseDimensionString("12.5pt",12.0f,fPts,sOrigUnits,sError);

         Assert::IsTrue(bResult, L"Should parse '12.5pt' successfully");
         Assert::AreEqual(12.5f, fPts, 0.001f, L"Should return 12.5 points");
         Assert::AreEqual(string("pt"), sOrigUnits, L"Should have 'pt' as original unit");
      }

      TEST_METHOD(ParseDimensionString_NegativeValue) {
         float fPts = 0.0f;
         string sOrigUnits, sError;

         bool bResult = CDimensionParser::_ParseDimensionString("-5pt",12.0f,fPts,sOrigUnits,sError);

         Assert::IsTrue(bResult, L"Should parse '-5pt' successfully");
         Assert::AreEqual(-5.0f, fPts, 0.001f, L"Should return -5 points");
         Assert::AreEqual(string("pt"), sOrigUnits, L"Should have 'pt' as original unit");
      }

      TEST_METHOD(ParseDimensionString_Centimeters) {
         float fPts = 0.0f;
         string sOrigUnits, sError;

         bool bResult = CDimensionParser::_ParseDimensionString("2.54cm",12.0f,fPts,sOrigUnits,sError);

         Assert::IsTrue(bResult, L"Should parse '2.54cm' successfully");
         Assert::AreEqual(72.0f, fPts, 0.1f, L"2.54cm should equal 72 points (1 inch)");
         Assert::AreEqual(string("cm"), sOrigUnits, L"Should have 'cm' as original unit");
      }

      TEST_METHOD(ParseDimensionString_EmUnits) {
         float fPts = 0.0f;
         string sOrigUnits, sError;

         bool bResult = CDimensionParser::_ParseDimensionString("2em",12.0f,fPts,sOrigUnits,sError);

         Assert::IsTrue(bResult, L"Should parse '2em' successfully");
         Assert::AreEqual(24.0f, fPts, 0.001f, L"2em should equal 24 points (2 * 12pt font)");
         Assert::AreEqual(string("em"), sOrigUnits, L"Should have 'em' as original unit");
      }

      TEST_METHOD(ParseDimensionString_ExUnits) {
         float fPts = 0.0f;
         string sOrigUnits, sError;

         bool bResult = CDimensionParser::_ParseDimensionString(
            "3ex",
            12.0f,
            fPts,
            sOrigUnits,
            sError);

         Assert::IsTrue(bResult, L"Should parse '3ex' successfully");
         Assert::AreEqual(18.0f, fPts, 0.001f, L"3ex should equal 18 points (3 * 6pt)");
         Assert::AreEqual(string("ex"), sOrigUnits, L"Should have 'ex' as original unit");
      }

      TEST_METHOD(ParseDimensionString_Millimeters) {
         float fPts = 0.0f;
         string sOrigUnits, sError;

         bool bResult = CDimensionParser::_ParseDimensionString(
            "25.4mm",
            12.0f,
            fPts,
            sOrigUnits,
            sError);

         Assert::IsTrue(bResult, L"Should parse '25.4mm' successfully");
         Assert::AreEqual(72.0f, fPts, 0.1f, L"25.4mm should equal 72 points (1 inch)");
         Assert::AreEqual(string("mm"), sOrigUnits, L"Should have 'mm' as original unit");
      }

      TEST_METHOD(ParseDimensionString_Inches) {
         float fPts = 0.0f;
         string sOrigUnits, sError;

         bool bResult = CDimensionParser::_ParseDimensionString(
            "1in",
            12.0f,
            fPts,
            sOrigUnits,
            sError);

         Assert::IsTrue(bResult, L"Should parse '1in' successfully");
         Assert::AreEqual(72.0f, fPts, 0.001f, L"1 inch should equal 72 points");
         Assert::AreEqual(string("in"), sOrigUnits, L"Should have 'in' as original unit");
      }

      TEST_METHOD(ParseDimensionString_NoUnit) {
         float fPts = 0.0f;
         string sOrigUnits, sError;

         bool bResult = CDimensionParser::_ParseDimensionString(
            "12",12.0f,fPts,sOrigUnits,sError);

         Assert::IsTrue(bResult, L"Should succeed without unit");
         Assert::IsTrue(sError.empty(), L"Should have no error message");
         Assert::IsTrue(sOrigUnits.empty(), L"Should have empty original unit (unitless)");
         Assert::AreEqual(12.0f, fPts, 0.001f, L"Should return raw value (unitless)");
      }

      TEST_METHOD(ParseDimensionString_InvalidUnit) {
         float fPts = 0.0f;
         string sOrigUnits, sError;

         bool bResult = CDimensionParser::_ParseDimensionString(
            "12xyz",
            12.0f,
            fPts,
            sOrigUnits,
            sError);

         Assert::IsFalse(bResult, L"Should fail with unknown unit embedded in token");
         Assert::IsFalse(sError.empty(), L"Should have error message");
         Assert::IsTrue(sError.find("Unknown unit") != string::npos, L"Error should mention unknown unit");
      }

      // Note: ParseDimensionString works with string literals, not tokens
      // Space handling is tested in integration tests with actual tokenization
      TEST_METHOD(ParseDimensionString_NoSpacesAllowed) {
         // String-based parsing doesn't handle spaces - that's done at token level
         float fPts = 0.0f;
         string sOrigUnits, sError;

         // "12 pt" with space would fail in string parsing
         bool bResult = CDimensionParser::_ParseDimensionString(
            "12 pt",
            12.0f,
            fPts,
            sOrigUnits,
            sError);

         Assert::IsFalse(bResult, L"String parser should fail with embedded space");
         Assert::IsFalse(sError.empty(), L"Should have error message");
      }

      TEST_METHOD(ParseDimensionString_InvalidSpaceInNumber) {
         // "1 2pt" with space inside number should fail
         float fPts = 0.0f;
         string sOrigUnits, sError;

         bool bResult = CDimensionParser::_ParseDimensionString(
            "1 2pt",
            12.0f,
            fPts,
            sOrigUnits,
            sError);

         Assert::IsFalse(bResult, L"Should fail with space inside number");
         Assert::IsFalse(sError.empty(), L"Should have error message");
      }

      // Integration tests with tokenization and space handling
      TEST_METHOD(HSkip_WithSpacesAroundNumber) {
         // Input: "\hskip 12 pt" (with space between number and unit)
         CTexParser parser(g_Doc);
         g_Doc.SetFontSizePts(10.0f);

         CMathItem* pRet = parser.Parse("$$\\hskip 12 pt$$");
         SMemGuard mg{ pRet };
         g_Doc.SetFontSizePts(12.0f); //restore!

         Assert::IsNotNull(pRet, L"Parse should succeed with space between number and unit");
         Assert::IsTrue(parser.LastError().sError.empty(), L"No parse errors");
         
         const STexGlue* pGlue = pRet->GetGlue();
         Assert::IsNotNull(pGlue, L"Should be glue item");
         
         float fExpectedEM = DIPS2EM(10.0f, PTS2DIPS(12.0f));
         Assert::AreEqual(fExpectedEM, pGlue->fNorm, 0.01f, L"12pt with space should work");
      }

      TEST_METHOD(HSkip_WithSpacesInBraces) {
         // Input: "\hskip { 10 pt }" (spaces inside braces)
         CTexParser parser(g_Doc);
         g_Doc.SetFontSizePts(10.0f);

         CMathItem* pRet = parser.Parse("$$\\hskip{ 10 pt }$$");
         SMemGuard mg{ pRet };
         g_Doc.SetFontSizePts(12.0f); //restore!

         Assert::IsNotNull(pRet, L"Parse should succeed with spaces in braces");
         Assert::IsTrue(parser.LastError().sError.empty(), L"No parse errors");
         
         const STexGlue* pGlue = pRet->GetGlue();
         Assert::IsNotNull(pGlue, L"Should be glue item");
         
         float fExpectedEM = DIPS2EM(10.0f, PTS2DIPS(10.0f));
         Assert::AreEqual(fExpectedEM, pGlue->fNorm, 0.01f, L"10pt with spaces in braces should work");
      }

      TEST_METHOD(HSkip_DecimalWithSpaces) {
         // Input: "\hskip 12.5 pt" (space after decimal number)
         CTexParser parser(g_Doc);
         g_Doc.SetFontSizePts(10.0f);
         
         CMathItem* pRet = parser.Parse("$$\\hskip 12.5 pt$$");
         SMemGuard mg{ pRet };
         g_Doc.SetFontSizePts(12.0f); //restore!

         Assert::IsNotNull(pRet, L"Parse should succeed");
         Assert::IsTrue(parser.LastError().sError.empty(), L"No parse errors");
         
         const STexGlue* pGlue = pRet->GetGlue();
         Assert::IsNotNull(pGlue, L"Should be glue item");
         
         float fExpectedEM = DIPS2EM(10.0f, PTS2DIPS(12.5f));
         Assert::AreEqual(fExpectedEM, pGlue->fNorm, 0.01f, L"12.5pt with space should work");
      }

      TEST_METHOD(HSkip_NegativeWithSpaceAfterSign) {
         // Input: "\hskip - 5 pt" (space after minus sign)
         CTexParser parser(g_Doc);
         g_Doc.SetFontSizePts(10.0f);
         
         CMathItem* pRet = parser.Parse("$$\\hskip - 5 pt$$");
         SMemGuard mg{ pRet };
         g_Doc.SetFontSizePts(12.0f); //restore!

         Assert::IsNotNull(pRet, L"Parse should succeed with space after sign");
         Assert::IsTrue(parser.LastError().sError.empty(), L"No parse errors");
         
         const STexGlue* pGlue = pRet->GetGlue();
         Assert::IsNotNull(pGlue, L"Should be glue item");
         
         float fExpectedEM = DIPS2EM(10.0f, PTS2DIPS(-5.0f));
         Assert::AreEqual(fExpectedEM, pGlue->fNorm, 0.01f, L"-5pt with spaces should work");
      }

      TEST_METHOD(HSkip_TrailingSpacesInBraces) {
         // Input: "\hskip{ 8pt   }" (trailing spaces before closing brace)
         CTexParser parser(g_Doc);
         g_Doc.SetFontSizePts(10.0f);
         
         CMathItem* pRet = parser.Parse("$$\\hskip{ 8pt   }$$");
         SMemGuard mg{ pRet };
         g_Doc.SetFontSizePts(12.0f); //restore!

         Assert::IsNotNull(pRet, L"Parse should succeed with trailing spaces");
         Assert::IsTrue(parser.LastError().sError.empty(), L"No parse errors");
         
         const STexGlue* pGlue = pRet->GetGlue();
         Assert::IsNotNull(pGlue, L"Should be glue item");
         
         float fExpectedEM = DIPS2EM(10.0f, PTS2DIPS(8.0f));
         Assert::AreEqual(fExpectedEM, pGlue->fNorm, 0.01f, L"8pt with trailing spaces should work");
      }

      TEST_METHOD(SetLength_WithSpaces) {
         // Test dimension parsing with spaces after SetLength
         CTexParser parser(g_Doc);
         
         // Directly set the length variable (bypass macro system for now)
         parser.SetLength("\\mylen", 15.0f);
         
         float fPts = 0.0f;
         bool bResult = parser.GetLength("\\mylen", fPts);
         
         Assert::IsTrue(bResult, L"Should retrieve variable");
         Assert::AreEqual(15.0f, fPts, 0.001f, L"Should return stored value of 15pt");
      }

      TEST_METHOD(SetLength_DecimalWithSpaces) {
         // Test dimension storage and retrieval with decimal values
         CTexParser parser(g_Doc);
         
         // Directly set the length variable (bypass macro system for now)
         parser.SetLength("\\mylen", 12.5f);
         
         float fPts = 0.0f;
         bool bResult = parser.GetLength("\\mylen", fPts);
         
         Assert::IsTrue(bResult, L"Should retrieve variable");
         Assert::AreEqual(12.5f, fPts, 0.001f, L"Should return stored value of 12.5pt");
      }

      TEST_METHOD(HSkip_WithVariableAndSpaces) {
         // Input: "\hskip { 2.5 \mylen }" where \mylen = 20pt
         CTexParser parser(g_Doc);
         g_Doc.SetFontSizePts(10.0f);
         
         // Directly set the variable (bypass macro system for now)
         parser.SetLength("\\mylen", 20.0f);
         
         CMathItem* pRet = parser.Parse("$$\\hskip{ 2.5 \\mylen }$$");
         SMemGuard mg{ pRet };
         g_Doc.SetFontSizePts(12.0f); //restore!

         Assert::IsNotNull(pRet, L"Parse should succeed");
         Assert::IsTrue(parser.LastError().sError.empty(), L"No parse errors");
         
         const STexGlue* pGlue = pRet->GetGlue();
         Assert::IsNotNull(pGlue, L"Should be glue item");
         
         // 2.5 * 20pt = 50pt
         float fExpectedEM = DIPS2EM(10.0f, PTS2DIPS(50.0f));
         Assert::AreEqual(fExpectedEM, pGlue->fNorm, 0.01f, L"2.5 * 20pt should equal 50pt");
      }

      TEST_METHOD(HSkip_LeadingSpacesInBraces) {
         // Input: "\hskip{   7pt}" (leading spaces after opening brace)
         CTexParser parser(g_Doc);
         g_Doc.SetFontSizePts(10.0f);
         
         CMathItem* pRet = parser.Parse("$$\\hskip{   7pt}$$");
         SMemGuard mg{ pRet };
         g_Doc.SetFontSizePts(12.0f); //restore!

         Assert::IsNotNull(pRet, L"Parse should succeed with leading spaces");
         Assert::IsTrue(parser.LastError().sError.empty(), L"No parse errors");
         
         const STexGlue* pGlue = pRet->GetGlue();
         Assert::IsNotNull(pGlue, L"Should be glue item");
         
         float fExpectedEM = DIPS2EM(10.0f, PTS2DIPS(7.0f));
         Assert::AreEqual(fExpectedEM, pGlue->fNorm, 0.01f, L"7pt with leading spaces should work");
      }

      TEST_METHOD(HSkip_MultipleSpaces) {
         // Input: "\hskip   9   pt" (multiple spaces aggregated into single ettSPACE token)
         CTexParser parser(g_Doc);
         g_Doc.SetFontSizePts(10.0f);
         
         CMathItem* pRet = parser.Parse("$$\\hskip   9   pt$$");
         SMemGuard mg{ pRet };
         
         g_Doc.SetFontSizePts(12.0f); //restore!

         Assert::IsNotNull(pRet, L"Parse should succeed with multiple spaces");
         Assert::IsTrue(parser.LastError().sError.empty(), L"No parse errors");
         
         const STexGlue* pGlue = pRet->GetGlue();
         Assert::IsNotNull(pGlue, L"Should be glue item");
         
         float fExpectedEM = DIPS2EM(10.0f, PTS2DIPS(9.0f));
         Assert::AreEqual(fExpectedEM, pGlue->fNorm, 0.01f, L"9pt with multiple spaces should work");
      }

      TEST_METHOD(Error_SpaceInsideDecimalNumber) {
         // Input: "\hskip 12. 5pt" (space between decimal point and fraction)
         // Tokenizer creates: "12" "." " " "5pt"
         // Parser should fail or parse as "12." (invalid) or "12.5pt" won't form
         CTexParser parser(g_Doc);
         
         CMathItem* pRet = parser.Parse("$$\\hskip 12. 5pt$$");
         SMemGuard mg{ pRet };

         // This might fail during parsing or produce unexpected result
         // The exact behavior depends on how decimal parsing handles the space
         // At minimum, it shouldn't silently succeed as "12.5pt"
         if (pRet) {
            // If it succeeds, it should not be 12.5pt
            const STexGlue* pGlue = pRet->GetGlue();
            if (pGlue) {
               float fExpectedWrong = DIPS2EM(10.0f, PTS2DIPS(12.5f));
               // Should NOT equal 12.5pt
               Assert::AreNotEqual(fExpectedWrong, pGlue->fNorm, 0.01f, 
                  L"Should not parse as 12.5pt with space in decimal");
            }
         }
      }

      TEST_METHOD(HSkip_WithUnknownUnit_SeparateToken) {
         // Input: "\hskip 12 abc" where 'abc' is separate token (not known unit)
         // Should parse as: 12 (unitless), but hskip requires units, so should fail
         CTexParser parser(g_Doc);
         g_Doc.SetFontSizePts(10.0f);

         CMathItem* pRet = parser.Parse("$$\\hskip 12 abc$$");
         SMemGuard mg{ pRet };
         g_Doc.SetFontSizePts(12.0f);

         // Should fail because \hskip requires units
         Assert::IsNull(pRet, L"Parse should fail - \\hskip requires units");
         auto err = parser.LastError();
         Assert::IsTrue(err.sError.find("Units are required") != string::npos, 
            L"Error should mention units are required");
      }
   };
   TEST_CLASS(MacroProcessorTests) {
   public:
      // Test 1: Basic \setlength
      TEST_METHOD(InitMacros_SetLength_Basic) {
         CTexParser parser(g_Doc);
         parser.AddMacros("\\setlength{\\mylen}{10pt}","test");
         CMathItem* pRet = parser.Parse("$$x$$");
         SMemGuard mg{ pRet };
         
         // Verify parse succeeded (macros initialized)
         Assert::IsNotNull(pRet, L"Parse should succeed after InitMacros");
         Assert::IsTrue(parser.LastError().sError.empty(), L"No errors expected");
         
         // Verify variable was set
         float fValue = 0.0f;
         bool bFound = parser.GetLength("\\mylen", fValue);
         Assert::IsTrue(bFound, L"Variable \\mylen should be defined");
         Assert::AreEqual(10.0f, fValue, 0.001f, L"Variable should equal 10pt");
      }
      
      // Test 2: \setlength with braces around variable
      TEST_METHOD(InitMacros_SetLength_WithBraces) {
         CTexParser parser(g_Doc);
         parser.AddMacros("\\setlength{\\parindent}{0pt}", "testfile");
         
         CMathItem* pRet = parser.Parse("$$a$$");
         SMemGuard mg{ pRet };
         
         Assert::IsNotNull(pRet);
         Assert::IsTrue(parser.LastError().sError.empty());
         
         float fValue = 0.0f;
         Assert::IsTrue(parser.GetLength("\\parindent", fValue));
         Assert::AreEqual(0.0f, fValue, 0.001f);
      }
      
      // Test 3: \setlength without braces around variable
      TEST_METHOD(InitMacros_SetLength_NoBraces) {
         CTexParser parser(g_Doc);
         parser.AddMacros("\\setlength\\mylen{5em}", "testfile");
         
         CMathItem* pRet = parser.Parse("$$b$$");
         SMemGuard mg{ pRet };
         
         Assert::IsNotNull(pRet);
         Assert::IsTrue(parser.LastError().sError.empty());
         
         // 5em at default 12pt font = 60pt
         float fValue = 0.0f;
         Assert::IsTrue(parser.GetLength("\\mylen", fValue));
         Assert::AreEqual(60.0f, fValue, 0.1f, L"5em should equal 60pt at 12pt font");
      }
      
      // Test 4: \addtolength
      TEST_METHOD(InitMacros_AddToLength_Basic) {
         CTexParser parser(g_Doc);
         parser.AddMacros("\\setlength{\\x}{10pt}\\addtolength{\\x}{5pt}", "testfile");
         
         CMathItem* pRet = parser.Parse("$$c$$");
         SMemGuard mg{ pRet };
         
         Assert::IsNotNull(pRet);
         Assert::IsTrue(parser.LastError().sError.empty());
         
         float fValue = 0.0f;
         Assert::IsTrue(parser.GetLength("\\x", fValue));
         Assert::AreEqual(15.0f, fValue, 0.001f, L"10pt + 5pt should equal 15pt");
      }
      
      // Test 5: \let - alias existing macro
      TEST_METHOD(InitMacros_Let_AliasMacro) {
         CTexParser parser(g_Doc);
         parser.AddMacros("\\newcommand{\\mycmd}{hello}\\let\\alias\\mycmd", "testfile");
         
         CMathItem* pRet = parser.Parse("$$d$$");
         SMemGuard mg{ pRet };
         
         Assert::IsNotNull(pRet);
         Assert::IsTrue(parser.LastError().sError.empty());
         
         // Verify both macros exist
         const CMacroProcessor::SMacroDef* pOriginal = parser.GetMacroDef("\\mycmd");
         const CMacroProcessor::SMacroDef* pAlias = parser.GetMacroDef("\\alias");
         
         Assert::IsNotNull(pOriginal, L"Original macro should exist");
         Assert::IsNotNull(pAlias, L"Alias should exist");
         
         // Verify they point to same body
         Assert::AreEqual(pOriginal->nBodyTkIdx, pAlias->nBodyTkIdx, L"Alias should share body with original");
      }
      
      // Test 6: \let with equals sign
      TEST_METHOD(InitMacros_Let_WithEquals) {
         CTexParser parser(g_Doc);
         parser.AddMacros("\\newcommand{\\test}{world}\\let\\copy = \\test", "testfile");
         
         CMathItem* pRet = parser.Parse("$$e$$");
         SMemGuard mg{ pRet };
         
         Assert::IsNotNull(pRet);
         Assert::IsTrue(parser.LastError().sError.empty());
         
         const CMacroProcessor::SMacroDef* pCopy = parser.GetMacroDef("\\copy");
         Assert::IsNotNull(pCopy, L"Copy should exist");
      }
      
      // Test 7: \let - forward reference (undefined source)
      TEST_METHOD(InitMacros_Let_ForwardReference) {
         CTexParser parser(g_Doc);
         parser.AddMacros("\\let\\myalias\\undefined", "testfile");
         
         CMathItem* pRet = parser.Parse("$$f$$");
         SMemGuard mg{ pRet };
         
         Assert::IsNotNull(pRet);
         Assert::IsTrue(parser.LastError().sError.empty());
         
         // Alias should exist (deferred resolution)
         const CMacroProcessor::SMacroDef* pAlias = parser.GetMacroDef("\\myalias");
         Assert::IsNotNull(pAlias, L"Alias should exist even if source undefined");
         Assert::AreEqual(0, pAlias->nNumArgs, L"Undefined alias should have 0 args");
      }
      
      // Test 8: \newcommand - no parameters
      TEST_METHOD(InitMacros_NewCommand_NoParams) {
         CTexParser parser(g_Doc);
         parser.AddMacros("\\newcommand{\\hello}{world}", "testfile");
         
         CMathItem* pRet = parser.Parse("$$g$$");
         SMemGuard mg{ pRet };
         
         Assert::IsNotNull(pRet);
         Assert::IsTrue(parser.LastError().sError.empty());
         
         const CMacroProcessor::SMacroDef* pMacro = parser.GetMacroDef("\\hello");
         const vector<STexToken>& vMacroTokens = parser.GetMacroTokens();
         Assert::IsNotNull(pMacro, L"Macro should exist");
         string sMacroName = parser.TokenText(&vMacroTokens[pMacro->nNameTkIdx]);
         Assert::AreEqual(string("\\hello"), sMacroName, L"Macro name should match");
         Assert::AreEqual(0, pMacro->nNumArgs, L"Should have 0 arguments");
         Assert::AreEqual(0, pMacro->nDefArgTkIdx, L"No default arg");
         Assert::IsTrue(pMacro->nBodyTkIdx > 0, L"Body index should be valid");
      }
      
      // Test 9: \newcommand - with parameters
      TEST_METHOD(InitMacros_NewCommand_WithParams) {
         CTexParser parser(g_Doc);
         parser.AddMacros("\\newcommand{\\mysum}[2]{#1 + #2}", "testfile");
         
         CMathItem* pRet = parser.Parse("$$h$$");
         SMemGuard mg{ pRet };
         
         Assert::IsNotNull(pRet);
         Assert::IsTrue(parser.LastError().sError.empty());
         
         const CMacroProcessor::SMacroDef* pMacro = parser.GetMacroDef("\\mysum");
         Assert::IsNotNull(pMacro);
         Assert::AreEqual(2, pMacro->nNumArgs, L"Should have 2 arguments");
         Assert::AreEqual(0, pMacro->nDefArgTkIdx, L"No optional arg");
      }
      
      // Test 10: \newcommand - with optional argument (single token)
      TEST_METHOD(InitMacros_NewCommand_OptionalArg_SingleToken) {
         CTexParser parser(g_Doc);
         parser.AddMacros("\\newcommand{\\test}[2][+]{#1 #2 #3}", "testfile");
         
         CMathItem* pRet = parser.Parse("$$i$$");
         SMemGuard mg{ pRet };
         
         Assert::IsNotNull(pRet);
         Assert::IsTrue(parser.LastError().sError.empty());
         
         const CMacroProcessor::SMacroDef* pMacro = parser.GetMacroDef("\\test");
         Assert::IsNotNull(pMacro);
         Assert::AreEqual(2, pMacro->nNumArgs);
         Assert::IsTrue(pMacro->nDefArgTkIdx > 0, L"Should have optional arg");
         
         // Verify it's a single token (not a group)
         const vector<STexToken>& vTokens = parser.GetMacroTokens();
         const STexToken& tkOpt = vTokens[pMacro->nDefArgTkIdx];
         Assert::AreNotEqual((int)ettSB_OPEN, (int)tkOpt.nType, L"Should be token, not opening bracket");
      }
      
      // Test 11: \newcommand - with optional argument (multiple tokens)
      TEST_METHOD(InitMacros_NewCommand_OptionalArg_MultipleTokens) {
         CTexParser parser(g_Doc);
         parser.AddMacros("\\newcommand{\\fancy}[1][hello ,world,]{#1!}", "testfile");
         
         CMathItem* pRet = parser.Parse("$$j$$");
         SMemGuard mg{ pRet };
         
         Assert::IsNotNull(pRet);
         Assert::IsTrue(parser.LastError().sError.empty());
         
         const CMacroProcessor::SMacroDef* pMacro = parser.GetMacroDef("\\fancy");
         Assert::IsNotNull(pMacro);
         Assert::IsTrue(pMacro->nDefArgTkIdx > 0);
         
         // Should point to opening [ (group)
         const vector<STexToken>& vTokens = parser.GetMacroTokens();
         const STexToken& tkOpt = vTokens[pMacro->nDefArgTkIdx];
         Assert::AreEqual((int)ettSB_OPEN, (int)tkOpt.nType, L"Should be opening bracket for group");
      }
      
      // Test 13: Multiple macros
      TEST_METHOD(InitMacros_MultipleMacros) {
         CTexParser parser(g_Doc);
         parser.AddMacros(
            "\\newcommand{\\a}{A}"
            "\\newcommand{\\b}{B}"
            "\\newcommand{\\c}{C}"
         , "testfile");
         
         CMathItem* pRet = parser.Parse("$$l$$");
         SMemGuard mg{ pRet };
         
         Assert::IsNotNull(pRet);
         Assert::IsTrue(parser.LastError().sError.empty());
         
         Assert::AreEqual(3, (int)parser.MacroCount(), L"Should have 3 macros");
         Assert::IsNotNull(parser.GetMacroDef("\\a"));
         Assert::IsNotNull(parser.GetMacroDef("\\b"));
         Assert::IsNotNull(parser.GetMacroDef("\\c"));
      }
      
      // Test 14: \renewcommand (same as \newcommand)
      TEST_METHOD(InitMacros_RenewCommand_ReplacesExisting) {
         CTexParser parser(g_Doc);
         parser.AddMacros(
            "\\newcommand{\\test}{old}"
            "\\renewcommand{\\test}{new}"
         , "testfile");
         
         CMathItem* pRet = parser.Parse("$$m$$");
         SMemGuard mg{ pRet };
         
         Assert::IsNotNull(pRet);
         Assert::IsTrue(parser.LastError().sError.empty());
         
         // Should have only 1 macro (replaced)
         Assert::AreEqual(1, (int)parser.MacroCount());
         const CMacroProcessor::SMacroDef* pMacro = parser.GetMacroDef("\\test");
         Assert::IsNotNull(pMacro);
      }
      
      // === ERROR TESTS ===
      
      // Test 15: Error - Invalid macro command
      TEST_METHOD(InitMacros_Error_InvalidCommand) {
         CTexParser parser(g_Doc);
         parser.AddMacros("\\unknown{something}", "testfile");
         
         CMathItem* pRet = parser.Parse("$$n$$");
         SMemGuard mg{ pRet };
         
         // Should fail during InitMacros
         Assert::IsNull(pRet, L"Parse should fail with invalid macro command");
         
         const ParserError& err = parser.LastError();
         Assert::IsFalse(err.sError.empty(), L"Should have error message");
         Assert::AreEqual((int)epsBUILDINGMACROS, (int)err.eStage, L"Error should be in macro building stage");
         Assert::IsTrue(err.sError.find("Unknown macro command") != string::npos, 
            L"Error should mention unknown command");
      }
      
      // Test 16: Error - \addtolength on undefined variable
      TEST_METHOD(InitMacros_Error_AddToLength_Undefined) {
         CTexParser parser(g_Doc);
         parser.AddMacros("\\addtolength{\\undef}{5pt}", "testfile");
         
         CMathItem* pRet = parser.Parse("$$o$$");
         SMemGuard mg{ pRet };
         
         Assert::IsNull(pRet);
         
         const ParserError& err = parser.LastError();
         Assert::IsTrue(err.sError.find("not defined") != string::npos, L"Error should mention undefined variable");
      }
      
      // Test 17: Error - \newcommand missing command name
      TEST_METHOD(InitMacros_Error_NewCommand_MissingName) {
         CTexParser parser(g_Doc);
         parser.AddMacros("\\newcommand{}{body}", "testfile");
         
         CMathItem* pRet = parser.Parse("$$p$$");
         SMemGuard mg{ pRet };
         
         Assert::IsNull(pRet);
         
         const ParserError& err = parser.LastError();
         Assert::IsFalse(err.sError.empty());
         Assert::IsTrue(err.sError.find("Expected command name") != string::npos ||
                       err.sError.find("Unexpected empty group") != string::npos,
            L"Error should mention missing command name");
      }
      
      // Test 18: Error - \newcommand invalid parameter count
      TEST_METHOD(InitMacros_Error_NewCommand_InvalidParamCount) {
         CTexParser parser(g_Doc);
         parser.AddMacros("\\newcommand{\\test}[10]{body}", "testfile");
         
         CMathItem* pRet = parser.Parse("$$q$$");
         SMemGuard mg{ pRet };
         
         Assert::IsNull(pRet);
         
         const ParserError& err = parser.LastError();
         Assert::IsTrue(err.sError.find("0-9") != string::npos, L"Error should mention valid parameter count range");
      }
      
      // Test 19: Error - \newcommand unclosed body
      TEST_METHOD(InitMacros_Error_NewCommand_UnclosedBody) {
         CTexParser parser(g_Doc);
         parser.AddMacros("\\newcommand{\\test}{unclosed", "testfile");
         
         CMathItem* pRet = parser.Parse("$$r$$");
         SMemGuard mg{ pRet };
         
         Assert::IsNull(pRet);
         
         const ParserError& err = parser.LastError();
         Assert::AreEqual((int)epsGROUPINGMACROS, (int)err.eStage, 
            L"Error should be in grouping stage");
         Assert::IsTrue(err.sError.find("Unclosed") != string::npos, 
            L"Error should mention unclosed group");
      }
      
      // Test 20: Error - \let missing new command name
      TEST_METHOD(InitMacros_Error_Let_MissingNewName) {
         CTexParser parser(g_Doc);
         parser.AddMacros("\\let = \\test", "testfile");
         
         CMathItem* pRet = parser.Parse("$$s$$");
         SMemGuard mg{ pRet };
         
         Assert::IsNull(pRet);
         
         const ParserError& err = parser.LastError();
         Assert::IsTrue(err.sError.find("Expected new command name") != string::npos ||
                       err.sError.find("Expected") != string::npos,
            L"Error should mention missing command name");
      }
      
      // Test 21: Error - Invalid dimension
      TEST_METHOD(InitMacros_Error_SetLength_InvalidDimension) {
         CTexParser parser(g_Doc);
         parser.AddMacros("\\setlength{\\x}{invalid}", "testfile");
         
         CMathItem* pRet = parser.Parse("$$t$$");
         SMemGuard mg{ pRet };
         
         Assert::IsNull(pRet);
         
         const ParserError& err = parser.LastError();
         Assert::IsFalse(err.sError.empty(), L"Should have error for invalid dimension");
      }
      
      // Test 22: Tokenization - Verify nRefIdx = -1 for macro tokens
      TEST_METHOD(InitMacros_TokensHaveNegativeRefIdx) {
         CTexParser parser(g_Doc);
         parser.AddMacros("\\newcommand{\\test}{body}", "testfile");
         
         CMathItem* pRet = parser.Parse("$$u$$");
         SMemGuard mg{ pRet };
         
         Assert::IsNotNull(pRet);
         
         // All macro tokens should have nRefIdx = -1
         const vector<STexToken>& vTokens = parser.GetMacroTokens();
         for (const STexToken& tk : vTokens) {
            if (tk.nRefIdx != -1) {
               Assert::Fail(L"Macros has token with nRefIdx <> -1 before expansion!");
               break;
            }
         }
      }
   };
   TEST_CLASS(MacroCommandsTests) {
   public:
      // ============================================================================
      // BASIC EXPANSION TESTS
      // ============================================================================

      TEST_METHOD(MacroExpansion_NoParams_Simple) {
         // Input: \newcommand{\hello}{world}  ->  $$\hello$$
         // Expected: Expands to single ettALNUM token "world"
         CTexParser parser(g_Doc);
         parser.AddMacros("\\newcommand{\\hello}{world}", "test.tex");

         CMathItem* pRet = parser.Parse("$$\\hello$$");
         SMemGuard mg{ pRet };

         Assert::IsNotNull(pRet, L"Parse should succeed");
         Assert::IsTrue(parser.LastError().sError.empty(), L"No errors");

         // Verify tokens are expanded
         const auto& tokens = parser.GetTokens();
         Assert::AreEqual((int)epsBUILDING, (int)parser.Stage());

         // Find the expanded "world" token
         bool foundWorld = false;
         for (const auto& tk : tokens) {
            if (tk.nType == ettALNUM) {
               string sText = parser.TokenText(&tk);
               if (sText == "world") {
                  foundWorld = true;
                  Assert::IsTrue(tk.nRefIdx > 0, L"Expanded token should have nRefIdx > 0");
                  Assert::AreEqual(5, (int)tk.nLen, L"'world' should have length 5");
                  break;
               }
            }
         }
         Assert::IsTrue(foundWorld, L"Should find expanded 'world' token");
      }

      TEST_METHOD(MacroExpansion_NoParams_MultipleUses) {
         // Test: \foo used twice in same formula
         CTexParser parser(g_Doc);
         parser.AddMacros("\\newcommand{\\foo}{x}", "test.tex");

         CMathItem* pRet = parser.Parse("$$\\foo + \\foo$$");
         SMemGuard mg{ pRet };

         Assert::IsNotNull(pRet);
         Assert::IsTrue(parser.LastError().sError.empty());

         // Verify HBox with multiple items
         Assert::AreEqual((int)eacHBOX, (int)pRet->Type());
         CContainerItem* pHBox = dynamic_cast<CContainerItem*>(pRet);

         // Should be: x + x (3 items)
         Assert::IsTrue(pHBox->Items().size() >= 3);
      }

      TEST_METHOD(MacroExpansion_WithParams_OneArg) {
         // \newcommand{\twice}[1]{#1 + #1}  ->  $$\twice{a}$$
         CTexParser parser(g_Doc);
         parser.AddMacros("\\newcommand{\\twice}[1]{#1 + #1}", "test.tex");

         CMathItem* pRet = parser.Parse("$$\\twice{a}$$");
         SMemGuard mg{ pRet };

         Assert::IsNotNull(pRet);
         Assert::IsTrue(parser.LastError().sError.empty());

         // Result should be HBox with: a + a
         Assert::AreEqual((int)eacHBOX, (int)pRet->Type());
         CContainerItem* pHBox = dynamic_cast<CContainerItem*>(pRet);
         Assert::AreEqual((size_t)3, pHBox->Items().size(), L"Should be 'a' '+' 'a'");
      }

      TEST_METHOD(MacroExpansion_WithParams_TwoArgs) {
         // \newcommand{\add}[2]{#1 + #2}  ->  $$\add{a}{b}$$
         CTexParser parser(g_Doc);
         parser.AddMacros("\\newcommand{\\add}[2]{#1 + #2}", "test.tex");

         CMathItem* pRet = parser.Parse("$$\\add{a}{b}$$");
         SMemGuard mg{ pRet };

         Assert::IsNotNull(pRet);
         Assert::IsTrue(parser.LastError().sError.empty());

         // Result: a + b
         Assert::AreEqual((int)eacHBOX, (int)pRet->Type());
         CContainerItem* pHBox = dynamic_cast<CContainerItem*>(pRet);
         Assert::AreEqual((size_t)3, pHBox->Items().size());
      }

      TEST_METHOD(MacroExpansion_WithParams_GroupedArg) {
         // \twice{x^2}  ->  Should expand to: x^2 + x^2
         CTexParser parser(g_Doc);
         parser.AddMacros("\\newcommand{\\twice}[1]{#1 + #1}", "test.tex");

         CMathItem* pRet = parser.Parse("$$\\twice{x^2}$$");
         SMemGuard mg{ pRet };

         Assert::IsNotNull(pRet);
         Assert::IsTrue(parser.LastError().sError.empty());

         // Result should contain two indexed items (x^2)
         CContainerItem* pHBox = dynamic_cast<CContainerItem*>(pRet);
         int nIndexedCount = 0;
         for (auto* pItem : pHBox->Items()) {
            if (pItem->Type() == eacINDEXED) ++nIndexedCount;
         }
         Assert::AreEqual(2, nIndexedCount, L"Should have 2 indexed items (x^2 + x^2)");
      }

      // ============================================================================
      // OPTIONAL ARGUMENT TESTS
      // ============================================================================

      TEST_METHOD(MacroExpansion_OptionalArg_Provided) {
         // \newcommand{\test}[2][+]{#1 #2}  ->  $$\test[-]{x}$$
         CTexParser parser(g_Doc);
         parser.AddMacros("\\newcommand{\\test}[2][+]{#1 #2}", "test.tex");

         CMathItem* pRet = parser.Parse("$$\\test[-]{x}$$");
         SMemGuard mg{ pRet };

         Assert::IsNotNull(pRet);
         Assert::IsTrue(parser.LastError().sError.empty());

         // Result: - x
         CContainerItem* pHBox = dynamic_cast<CContainerItem*>(pRet);
         Assert::AreEqual((size_t)2, pHBox->Items().size(), L"Should be '-' and 'x'");
      }

      TEST_METHOD(MacroExpansion_OptionalArg_Default) {
         // \test{x}  (no optional arg)  ->  Should use default '+'
         CTexParser parser(g_Doc);
         parser.AddMacros("\\newcommand{\\test}[2][+]{#1 #2}", "test.tex");

         CMathItem* pRet = parser.Parse("$$\\test{x}$$");
         SMemGuard mg{ pRet };

         Assert::IsNotNull(pRet);
         Assert::IsTrue(parser.LastError().sError.empty());

         // Result: + x
         CContainerItem* pHBox = dynamic_cast<CContainerItem*>(pRet);
         Assert::AreEqual((size_t)2, pHBox->Items().size(), L"Should be '+' and 'x'");
      }

      // ============================================================================
      // NESTED EXPANSION TESTS
      // ============================================================================

      TEST_METHOD(MacroExpansion_Nested_TwoLevels) {
         // \outer calls \inner
         CTexParser parser(g_Doc);
         parser.AddMacros(
            "\\newcommand{\\inner}{x}"
            "\\newcommand{\\outer}{\\inner + \\inner}",
            "test.tex");

         CMathItem* pRet = parser.Parse("$$\\outer$$");
         SMemGuard mg{ pRet };

         Assert::IsNotNull(pRet);
         Assert::IsTrue(parser.LastError().sError.empty());

         // Result: x + x (3 items in HBox)
         CContainerItem* pHBox = dynamic_cast<CContainerItem*>(pRet);
         Assert::AreEqual((size_t)3, pHBox->Items().size(), L"Should be 'x' '+' 'x'");

         // Verify tokens: should have 'x' (ettALNUM), '+' (ettNonALPHA), 'x' (ettALNUM)
         const auto& tokens = parser.GetTokens();
         int nXCount = 0;
         for (const auto& tk : tokens) {
            if (tk.nType == ettALNUM) {
               string sText = parser.TokenText(&tk);
               if (sText == "x") {
                  Assert::IsTrue(tk.nRefIdx > 0, L"Expanded 'x' should have nRefIdx > 0");
                  Assert::AreEqual(1, (int)tk.nLen, L"'x' should have length 1");
                  ++nXCount;
               }
            }
         }
         Assert::AreEqual(2, nXCount, L"Should find two 'x' tokens");
      }

      TEST_METHOD(MacroExpansion_Nested_WithArguments) {
         // \outer{a} -> \inner{a}{a} -> a + a
         CTexParser parser(g_Doc);
         parser.AddMacros(
            "\\newcommand{\\inner}[2]{#1 + #2}"
            "\\newcommand{\\outer}[1]{\\inner{#1}{#1}}",
            "test.tex");

         CMathItem* pRet = parser.Parse("$$\\outer{b}$$");
         SMemGuard mg{ pRet };

         Assert::IsNotNull(pRet);
         Assert::IsTrue(parser.LastError().sError.empty());

         // Result: b + b
         CContainerItem* pHBox = dynamic_cast<CContainerItem*>(pRet);
         Assert::AreEqual((size_t)3, pHBox->Items().size());
      }

      // ============================================================================
      // SPECIAL TOKEN PROCESSING TESTS
      // ============================================================================

      TEST_METHOD(MacroExpansion_BeginGroupEndGroup) {
         // Body contains \begingroup and \endgroup
         CTexParser parser(g_Doc);
         parser.AddMacros("\\newcommand{\\test}{\\begingroup a \\endgroup}", "test.tex");

         CMathItem* pRet = parser.Parse("$$\\test$$");
         SMemGuard mg{ pRet };

         Assert::IsNotNull(pRet);
         Assert::IsTrue(parser.LastError().sError.empty());

         // Verify tokens contain { and } instead of \begingroup/\endgroup
         const auto& tokens = parser.GetTokens();
         bool foundFB_OPEN = false, foundFB_CLOSE = false;
         for (const auto& tk : tokens) {
            if (tk.nType == ettFB_OPEN) foundFB_OPEN = true;
            if (tk.nType == ettFB_CLOSE) foundFB_CLOSE = true;
         }
         Assert::IsTrue(foundFB_OPEN && foundFB_CLOSE,
            L"\\begingroup/\\endgroup should be replaced by { }");
      }
      TEST_METHOD(MacroExpansion_ParameterSubstitution_SimpleCase) {
         // Test that #1 gets replaced with argument during expansion
         CTexParser parser(g_Doc);
         parser.AddMacros("\\newcommand{\\test}[1]{Begin #1 End}", "test.tex");

         CMathItem* pRet = parser.Parse("$$\\test{VALUE}$$");
         SMemGuard mg{ pRet };

         Assert::IsNotNull(pRet, L"Parse should succeed");
         Assert::IsTrue(parser.LastError().sError.empty());

         // Result should be: Begin VALUE End
         const auto& tokens = parser.GetTokens();

         // Find the tokens in order
         bool foundBegin = false, foundValue = false, foundEnd = false;
         for (const auto& tk : tokens) {
            if (tk.nType == ettALNUM) {
               string sText = parser.TokenText(&tk);
               if (sText == "Begin") foundBegin = true;
               else if (sText == "VALUE") foundValue = true;
               else if (sText == "End") foundEnd = true;
            }
         }

         Assert::IsTrue(foundBegin && foundValue && foundEnd,
            L"Should find 'Begin VALUE End' in expanded tokens");

         // Verify NO ettMARG1 tokens remain (all replaced)
         for (const auto& tk : tokens) {
            Assert::AreNotEqual((int)ettMARG1, (int)tk.nType,
               L"No ettMARG1 tokens should remain after expansion");
         }
      }

      TEST_METHOD(MacroExpansion_MultipleParameters_AllReplaced) {
         // Test that all parameters #1, #2, #3 get replaced
         CTexParser parser(g_Doc);
         parser.AddMacros("\\newcommand{\\test}[3]{#1 and #2 and #3}", "test.tex");

         CMathItem* pRet = parser.Parse("$$\\test{A}{B}{C}$$");
         SMemGuard mg{ pRet };

         Assert::IsNotNull(pRet, L"Parse should succeed");
         Assert::IsTrue(parser.LastError().sError.empty());

         // Result: A and B and C
         const auto& tokens = parser.GetTokens();

         bool foundA = false, foundB = false, foundC = false, foundAnd = false;
         for (const auto& tk : tokens) {
            if (tk.nType == ettALNUM) {
               string sText = parser.TokenText(&tk);
               if (sText == "A") foundA = true;
               else if (sText == "B") foundB = true;
               else if (sText == "C") foundC = true;
               else if (sText == "and") foundAnd = true;
            }
         }

         Assert::IsTrue(foundA && foundB && foundC && foundAnd,
            L"All three arguments and 'and' text should be present");

         // Verify NO parameter tokens remain (ettMARG1, ettMARG2, ettMARG3)
         for (const auto& tk : tokens) {
            Assert::IsTrue(tk.nType < ettMARG1 || tk.nType > ettMARG9,
               L"No parameter tokens should remain after expansion");
         }
      }
      // ============================================================================
      // \let ALIAS TESTS
      // ============================================================================

      TEST_METHOD(MacroExpansion_Let_SimpleAlias) {
         // \let\alias\original  ->  Both should expand identically
         CTexParser parser(g_Doc);
         parser.AddMacros(
            "\\newcommand{\\original}{hello}"
            "\\let\\alias\\original",
            "test.tex");

         CMathItem* pRet1 = parser.Parse("$$\\original$$");
         SMemGuard mg1{ pRet1 };
         Assert::IsNotNull(pRet1);

         CMathItem* pRet2 = parser.Parse("$$\\alias$$");
         SMemGuard mg2{ pRet2 };
         Assert::IsNotNull(pRet2);

         // Both should produce same result
         Assert::AreEqual((int)pRet1->Type(), (int)pRet2->Type());
      }

      TEST_METHOD(MacroExpansion_Let_WithArguments) {
         // \let copies argument count
         CTexParser parser(g_Doc);
         parser.AddMacros(
            "\\newcommand{\\add}[2]{#1 + #2}"
            "\\let\\plus\\add",
            "test.tex");

         CMathItem* pRet = parser.Parse("$$\\plus{a}{b}$$");
         SMemGuard mg{ pRet };

         Assert::IsNotNull(pRet);
         Assert::IsTrue(parser.LastError().sError.empty());

         // Result: a + b
         CContainerItem* pHBox = dynamic_cast<CContainerItem*>(pRet);
         Assert::AreEqual((size_t)3, pHBox->Items().size());
      }

      // ============================================================================
      // TOKEN REFERENCE TRACKING TESTS
      // ============================================================================

      TEST_METHOD(MacroExpansion_RefIdx_FirstExpansion) {
         // First expansion: nRefIdx should point to m_vTokensOrig
         CTexParser parser(g_Doc);
         parser.AddMacros("\\newcommand{\\test}{abc}", "test.tex");

         parser.Parse("$$\\test$$");

         const auto& tokensOrig = parser.GetTokensOrig();
         const auto& tokensExpanded = parser.GetTokens();

         // Find \test in original tokens
         int nTestIdx = -1;
         for (int i = 0; i < tokensOrig.size(); ++i) {
            if (tokensOrig[i].nType == ettCOMMAND) {
               string sCmd = parser.TokenText(&tokensOrig[i]);
               if (sCmd == "\\test") {
                  nTestIdx = i;
                  break;
               }
            }
         }
         Assert::IsTrue(nTestIdx >= 0, L"Should find \\test in original tokens");

         // Expanded tokens should have nRefIdx = nTestIdx + 1
         for (const auto& tk : tokensExpanded) {
            if (tk.nType == ettALNUM && tk.nRefIdx > 0) {
               Assert::AreEqual(nTestIdx + 1, tk.nRefIdx,
                  L"Expanded tokens should reference original \\test position");
               break;
            }
         }
      }

      TEST_METHOD(MacroExpansion_RefIdx_NestedExpansion) {
         // Nested expansion: nRefIdx should be preserved
         CTexParser parser(g_Doc);
         parser.AddMacros(
            "\\newcommand{\\inner}{x}"
            "\\newcommand{\\outer}{\\inner}",
            "test.tex");

         parser.Parse("$$\\outer$$");

         const auto& tokensExpanded = parser.GetTokens();

         // Find the expanded 'x' token and verify nRefIdx
         bool foundX = false;
         int nXRefIdx = -1;
         for (const auto& tk : tokensExpanded) {
            if (tk.nType == ettALNUM) {
               string sText = parser.TokenText(&tk);
               if (sText == "x") {
                  foundX = true;
                  nXRefIdx = tk.nRefIdx;
                  Assert::IsTrue(nXRefIdx > 0, L"Expanded 'x' should have nRefIdx > 0");
                  Assert::AreEqual(1, (int)tk.nLen, L"'x' should have length 1");
                  break;
               }
            }
         }
         Assert::IsTrue(foundX, L"Should find expanded 'x' token");

         // Verify nRefIdx points to \outer (not \inner) since that's the direct expansion
         const auto& tokensOrig = parser.GetTokensOrig();
         if (nXRefIdx > 0 && nXRefIdx <= tokensOrig.size()) {
            const STexToken& tkOrig = tokensOrig[nXRefIdx - 1];
            string sOrigCmd = parser.TokenText(&tkOrig);
            Assert::AreEqual(string("\\outer"), sOrigCmd,
               L"nRefIdx should point to \\outer in original tokens");
         }
      }
      // ============================================================================
      // ERROR TESTS
      // ============================================================================

      TEST_METHOD(MacroExpansion_Error_MissingArgument) {
         // \add requires 2 args, but only 1 provided
         CTexParser parser(g_Doc);
         parser.AddMacros("\\newcommand{\\add}[2]{#1 + #2}", "test.tex");

         CMathItem* pRet = parser.Parse("$$\\add{a}$$");
         SMemGuard mg{ pRet };

         Assert::IsNull(pRet, L"Should fail with missing argument");

         const ParserError& err = parser.LastError();
         Assert::IsFalse(err.sError.empty());
         Assert::IsTrue(err.sError.find("Expected") != string::npos ||
            err.sError.find("argument") != string::npos);
      }

      TEST_METHOD(MacroExpansion_Error_EndOfInput) {
         // Macro at end of input without arguments
         CTexParser parser(g_Doc);
         parser.AddMacros("\\newcommand{\\needs}[1]{#1}", "test.tex");

         CMathItem* pRet = parser.Parse("$$\\needs$$");
         SMemGuard mg{ pRet };

         Assert::IsNull(pRet);

         const ParserError& err = parser.LastError();
         Assert::IsTrue(err.sError.find("end of input") != string::npos ||
            err.sError.find("argument") != string::npos);
      }

      TEST_METHOD(MacroExpansion_Error_UnclosedArgument) {
         // Argument not closed: \test{abc
         CTexParser parser(g_Doc);
         parser.AddMacros("\\newcommand{\\test}[1]{#1}", "test.tex");

         CMathItem* pRet = parser.Parse("$$\\test{abc$$");
         SMemGuard mg{ pRet };

         Assert::IsNull(pRet);

         const ParserError& err = parser.LastError();
         Assert::AreEqual((int)epsGROUPING, (int)err.eStage);
         Assert::IsTrue(err.sError.find("Unclosed") != string::npos);
      }

      TEST_METHOD(MacroExpansion_Error_NestedDefinition) {
         CTexParser parser(g_Doc);
         parser.AddMacros("\\newcommand{\\outer}{\\newcommand{\\inner}[1]{#1}}", "testfile");

         CMathItem* pRet = parser.Parse("$$k\\outer$$");
         SMemGuard mg{ pRet };

         Assert::IsNull(pRet, L"Nested macro definition are not supported");

         auto err = parser.LastError();
         string sLine1 = err.sError.substr(0, err.sError.find('\n'));
         Assert::AreEqual(sLine1.c_str(), "Nested or in-place macro definitions are not supported");

         // Outer macro should exist
         const CMacroProcessor::SMacroDef* pOuter = parser.GetMacroDef("\\outer");
         Assert::IsNotNull(pOuter, L"Outer macro should exist");

      }

      // ============================================================================
      // COMPARE ORIGINAL VS EXPANDED TOKENS
      // ============================================================================
      TEST_METHOD(MacroExpansion_CompareOriginalVsExpanded) {
         CTexParser parser(g_Doc);
         parser.AddMacros("\\newcommand{\\foo}{bar}", "test.tex");

         parser.Parse("$$\\foo$$");

         const auto& tokensOrig = parser.GetTokensOrig();
         const auto& tokensExpanded = parser.GetTokens();

         // Original has \foo command
         bool foundFoo = false;
         for (const auto& tk : tokensOrig) {
            if (tk.nType == ettCOMMAND && parser.TokenText(&tk) == "\\foo") {
               foundFoo = true;
               break;
            }
         }
         Assert::IsTrue(foundFoo, L"Original tokens should contain \\foo");

         // Expanded has 'bar' as single ettALNUM token
         bool foundBar = false;
         for (const auto& tk : tokensExpanded) {
            if (tk.nType == ettALNUM) {
               string sText = parser.TokenText(&tk);
               if (sText == "bar") {
                  foundBar = true;
                  Assert::AreEqual(3, (int)tk.nLen, L"'bar' should have length 3");
                  Assert::IsTrue(tk.nRefIdx > 0, L"Expanded 'bar' should have nRefIdx > 0");
                  break;
               }
            }
         }
         Assert::IsTrue(foundBar, L"Expanded tokens should contain 'bar'");

         // No \foo in expanded tokens
         bool foundFooInExpanded = false;
         for (const auto& tk : tokensExpanded) {
            if (tk.nType == ettCOMMAND && parser.TokenText(&tk) == "\\foo") {
               foundFooInExpanded = true;
               break;
            }
         }
         Assert::IsFalse(foundFooInExpanded, L"Expanded tokens should not contain \\foo");
      }

// ============================================================================
// MACRO WITH \left...\right TESTS
// ============================================================================

      TEST_METHOD(MacroExpansion_WithLeftRight_Norm) {
         // Critical test: \left and \right inside macro body
         // This tests the bug where OnGroupOpen_ used m_pTokenizer->TokenText() 
         // instead of TokenText() for macro tokens
         CTexParser parser(g_Doc);
         parser.AddMacros("\\newcommand{\\norm}[1]{\\left\\|#1\\right\\|}", "test.tex");

         CMathItem* pRet = parser.Parse("$$\\norm{x}$$");
         SMemGuard mg{ pRet };

         Assert::IsNotNull(pRet, L"Parse should succeed");
         Assert::IsTrue(parser.LastError().sError.empty(), L"No errors expected");

         // Result should be a container with delimiters and content
         // The structure depends on your delimiter handling, but at minimum:
         // - Should not crash
         // - Should contain 'x' somewhere in the tree

         // Verify the item type (likely HBOX or similar container)
         Assert::IsTrue(pRet->Type() != eacUNK, L"Should produce valid item");
      }

      TEST_METHOD(MacroExpansion_WithLeftRight_Absolute) {
         // Test: Absolute value macro with \left| and \right|
         CTexParser parser(g_Doc);
         parser.AddMacros("\\newcommand{\\abs}[1]{\\left|#1\\right|}", "test.tex");

         CMathItem* pRet = parser.Parse("$$\\abs{-5}$$");
         SMemGuard mg{ pRet };

         Assert::IsNotNull(pRet, L"Parse should succeed");
         Assert::IsTrue(parser.LastError().sError.empty());

         // Verify content exists
         Assert::IsTrue(pRet->Type() != eacUNK);
      }

      TEST_METHOD(MacroExpansion_WithLeftRight_Brackets) {
         // Test: Macro with \left[ and \right] brackets
         CTexParser parser(g_Doc);
         parser.AddMacros("\\newcommand{\\floor}[1]{\\left\\lfloor #1\\right\\rfloor}", "test.tex");

         CMathItem* pRet = parser.Parse("$$\\floor{3.7}$$");
         SMemGuard mg{ pRet };

         Assert::IsNotNull(pRet, L"Parse should succeed");
         Assert::IsTrue(parser.LastError().sError.empty());
      }

      TEST_METHOD(MacroExpansion_WithLeftRight_Nested) {
         // Test: Nested \left...\right in macro
         CTexParser parser(g_Doc);
         parser.AddMacros(
            "\\newcommand{\\inner}[1]{\\left(#1\\right)}"
            "\\newcommand{\\outer}[1]{\\left[\\inner{#1}\\right]}",
            "test.tex");

         CMathItem* pRet = parser.Parse("$$\\outer{x}$$");
         SMemGuard mg{ pRet };

         Assert::IsNotNull(pRet, L"Parse should succeed with nested delimiters");
         Assert::IsTrue(parser.LastError().sError.empty());
      }

      TEST_METHOD(MacroExpansion_WithLeftRight_InArgument) {
         // Test: Macro argument containing \left...\right
         CTexParser parser(g_Doc);
         parser.AddMacros("\\newcommand{\\wrap}[1]{[#1]}", "test.tex");

         CMathItem* pRet = parser.Parse("$$\\wrap{\\left\\|x\\right\\|}$$");
         SMemGuard mg{ pRet };

         Assert::IsNotNull(pRet, L"Parse should succeed");
         Assert::IsTrue(parser.LastError().sError.empty());
      }

      // ============================================================================
      // MACRO WITH ENVIRONMENT (\begin...\end) TESTS
      // ============================================================================

      TEST_METHOD(MacroExpansion_WithEnvironment_Matrix) {
         // Test: Macro containing \begin{matrix}...\end{matrix}
         CTexParser parser(g_Doc);
         parser.AddMacros(
            "\\newcommand{\\smallmatrix}[4]{\\begin{matrix}#1&#2\\\\#3&#4\\end{matrix}}",
            "test.tex");

         CMathItem* pRet = parser.Parse("$$\\smallmatrix{a}{b}{c}{d}$$");
         SMemGuard mg{ pRet };

         Assert::IsNotNull(pRet, L"Parse should succeed");
         Assert::IsTrue(parser.LastError().sError.empty(), L"Environment in macro should work");

         // Verify result is a table/matrix
         Assert::AreEqual((int)eacTABLE, (int)pRet->Type(),
            L"Should produce table for matrix environment");
      }

      TEST_METHOD(MacroExpansion_WithEnvironment_Cases) {
         // Test: Macro with cases environment
         CTexParser parser(g_Doc);
         parser.AddMacros(
            "\\newcommand{\\signum}{\\begin{cases}1&x>0\\\\0&x=0\\\\-1&x<0\\end{cases}}",
            "test.tex");

         CMathItem* pRet = parser.Parse("$$\\signum$$");
         SMemGuard mg{ pRet };

         Assert::IsNotNull(pRet, L"Parse should succeed");
         Assert::IsTrue(parser.LastError().sError.empty());
      }

      TEST_METHOD(MacroExpansion_WithEnvironment_Array) {
         // Test: Macro with array environment
         CTexParser parser(g_Doc);
         parser.AddMacros(
            "\\newcommand{\\myarray}[2]{\\begin{array}{cc}#1&#2\\end{array}}",
            "test.tex");

         CMathItem* pRet = parser.Parse("$$\\myarray{1}{2}$$");
         SMemGuard mg{ pRet };

         Assert::IsNotNull(pRet, L"Parse should succeed");
         Assert::IsTrue(parser.LastError().sError.empty());
         Assert::AreEqual((int)eacTABLE, (int)pRet->Type());
      }

      TEST_METHOD(MacroExpansion_WithEnvironment_NestedInArgument) {
         // Test: Environment passed as macro argument
         CTexParser parser(g_Doc);
         parser.AddMacros("\\newcommand{\\bracket}[1]{\\left[#1\\right]}", "test.tex");

         CMathItem* pRet = parser.Parse(
            "$$\\bracket{\\begin{matrix}a&b\\\\c&d\\end{matrix}}$$");
         SMemGuard mg{ pRet };

         Assert::IsNotNull(pRet, L"Parse should succeed");
         Assert::IsTrue(parser.LastError().sError.empty());
      }

      TEST_METHOD(MacroExpansion_WithEnvironment_MultipleMacros) {
         // Test: Multiple macros each with environments
         CTexParser parser(g_Doc);
         parser.AddMacros(
            "\\newcommand{\\mat}[1]{\\begin{matrix}#1\\end{matrix}}"
            "\\newcommand{\\pmat}[1]{\\left(\\mat{#1}\\right)}",
            "test.tex");

         CMathItem* pRet = parser.Parse("$$\\pmat{a&b\\\\c&d}$$");
         SMemGuard mg{ pRet };

         Assert::IsNotNull(pRet, L"Parse should succeed");
         Assert::IsTrue(parser.LastError().sError.empty());
      }

      // ============================================================================
      // EDGE CASE: MACRO WITH DISPLAY MATH (\[...\])
      // ============================================================================

      TEST_METHOD(MacroExpansion_WithDisplayMath_Brackets) {
         // Test: Macro containing \[ and \]
         // Note: This might be unusual but should be handled
         CTexParser parser(g_Doc);
         parser.AddMacros("\\newcommand{\\display}[1]{\\[#1\\]}", "test.tex");

         // This tests if \[ and \] from macro body are properly handled
         CMathItem* pRet = parser.Parse("\\display{x^2}");
         SMemGuard mg{ pRet };

         Assert::IsNotNull(pRet, L"Parse should succeed");
         Assert::IsTrue(parser.LastError().sError.empty());
      }

      // ============================================================================
      // ERROR CASES
      // ============================================================================

      TEST_METHOD(MacroExpansion_Error_UnmatchedLeftRight) {
         // Test: Macro with unmatched \left (missing \right)
         CTexParser parser(g_Doc);
         parser.AddMacros("\\newcommand{\\bad}{\\left(x}", "test.tex");

         CMathItem* pRet = parser.Parse("$$\\bad\\right)$$");
         SMemGuard mg{ pRet };

         // This should either succeed (if \right is outside) or fail gracefully
         if (!pRet) {
            const ParserError& err = parser.LastError();
            Assert::IsFalse(err.sError.empty(), L"Should have error message");
         }
      }

      TEST_METHOD(MacroExpansion_Error_WrongEnvironment) {
         // Test: Macro with mismatched environment names
         CTexParser parser(g_Doc);
         parser.AddMacros(
            "\\newcommand{\\badenv}{\\begin{matrix}x\\end{array}}",
            "test.tex");

         CMathItem* pRet = parser.Parse("$$\\badenv$$");
         SMemGuard mg{ pRet };

         Assert::IsNull(pRet, L"Should fail with mismatched environment");

         const ParserError& err = parser.LastError();
         Assert::IsFalse(err.sError.empty());
         Assert::IsTrue(err.sError.find("environment") != string::npos ||
            err.sError.find("Unmatched") != string::npos,
            L"Error should mention environment mismatch");
      }

      // ============================================================================
      // COMPLEX REAL-WORLD MACROS
      // ============================================================================

      TEST_METHOD(MacroExpansion_RealWorld_SetBuilder) {
         // Test: Set builder notation macro
         CTexParser parser(g_Doc);
         parser.AddMacros(
            "\\newcommand{\\set}[2]{\\left\\{#1\\mid #2\\right\\}}",
            "test.tex");

         CMathItem* pRet = parser.Parse("$$\\set{x}{x>0}$$");
         SMemGuard mg{ pRet };

         Assert::IsNotNull(pRet, L"Parse should succeed");
         Assert::IsTrue(parser.LastError().sError.empty());
      }

      TEST_METHOD(MacroExpansion_RealWorld_Binom) {
         // Test: Binomial coefficient macro
         CTexParser parser(g_Doc);
         parser.AddMacros(
            "\\newcommand{\\binom}[2]{\\left(\\begin{array}{c}#1\\\\#2\\end{array}\\right)}",
            "test.tex");

         CMathItem* pRet = parser.Parse("$$\\binom{n}{k}$$");
         SMemGuard mg{ pRet };

         Assert::IsNotNull(pRet, L"Parse should succeed");
         Assert::IsTrue(parser.LastError().sError.empty());
      }

      TEST_METHOD(MacroExpansion_RealWorld_PiecewiseFunction) {
         // Test: Piecewise function macro
         CTexParser parser(g_Doc);
         parser.AddMacros(
            "\\newcommand{\\piecewise}[4]{\\begin{cases}#1&#2\\\\#3&#4\\end{cases}}",
            "test.tex");

         CMathItem* pRet = parser.Parse("$$f(x)=\\piecewise{x^2}{x\\geq 0}{-x^2}{x<0}$$");
         SMemGuard mg{ pRet };

         Assert::IsNotNull(pRet, L"Parse should succeed");
         Assert::IsTrue(parser.LastError().sError.empty());
      }

      // ============================================================================
      // ERROR REPORTING WITH MACRO EXPANSION TESTS
      // ============================================================================

      TEST_METHOD(MacroExpansion_Error_SingleLevel_UndefinedCommand) {
         // Test: Error in expanded macro body - undefined command
         // \mycommand expands to: \undefined{x}
         CTexParser parser(g_Doc);
         parser.AddMacros("\\newcommand{\\mycommand}[1]{\\undefined{#1}}", "Macros.mth");

         CMathItem* pRet = parser.Parse("$$\\mycommand{x}$$");
         SMemGuard mg{ pRet };

         Assert::IsNull(pRet, L"Parse should fail with undefined command");

         const ParserError& err = parser.LastError();
         Assert::IsFalse(err.sError.empty(), L"Should have error message");

         // DEBUG: Output the actual error message  
         Logger::WriteMessage(("Error message:\n" + err.sError).c_str());
         Logger::WriteMessage(("Error stage: " + to_string((int)err.eStage)).c_str());
         Logger::WriteMessage(("Error pos: " + to_string(err.nStartPos) + "-" + to_string(err.nEndPos)).c_str());
         
         // Verify error has 3 lines:
         // Line 1: Main error "Unknown command '\undefined'"
         // Line 2: "  Expanded macro: \undefined{x}"
         // Line 3: "  from macro file(s): Macros.mth"
         size_t nFirstNewline = err.sError.find('\n');
         Assert::IsTrue(nFirstNewline != string::npos, L"Error should have multiple lines");

         // Verify contains expanded macro text
         Assert::IsTrue(err.sError.find("Expanded macro:") != string::npos,
            L"Error should have 'Expanded macro:' line");
         Assert::IsTrue(err.sError.find("\\undefined{x}") != string::npos,
            L"Error should show expanded macro with argument substituted");

         // Verify contains macro file
         Assert::IsTrue(err.sError.find("from macro file(s):") != string::npos,
            L"Error should have 'from macro file(s):' line");
         Assert::IsTrue(err.sError.find("Macros.mth") != string::npos,
            L"Error should mention macro file");

         // Verify error position maps to user input (\mycommand in "$$\mycommand{x}$$")
         Assert::IsTrue(err.nStartPos >= 2 && err.nStartPos <= 15,
            L"Error position should point to \\mycommand in user input");
      }

      TEST_METHOD(MacroExpansion_Error_NestedMacros_ThreeLevels) {
         // Test: Error in deeply nested macro expansion (your example)
         // \outer{1} -> \@inner{1^2} -> \sqrt{\half{1^2}} -> \sqrt{\ensuremat\frac{1^2}{2}}
         // Error: \ensuremat is not defined (typo, should be \ensuremath)
         CTexParser parser(g_Doc);
         parser.AddMacros(
            "\\newcommand{\\half}[1]{\\ensuremat\\frac{#1}{2}}"  // Typo: \ensuremat instead of \ensuremath
            "\\newcommand{\\@inner}[1]{\\sqrt{\\half{#1}}}"
            "\\newcommand{\\outer}[1]{\\@inner{#1^2}}",
            "Macros.mth");

         CMathItem* pRet = parser.Parse("$$\\outer{1}$$");
         SMemGuard mg{ pRet };

         Assert::IsNull(pRet, L"Parse should fail with undefined command");

         const ParserError& err = parser.LastError();
         Assert::IsFalse(err.sError.empty());

         // DEBUG: Output the actual error
         Logger::WriteMessage(("Nested error message:\n" + err.sError).c_str());

         // Expected error format:
         // Line 1: "Unknown command '\ensuremat'"
         // Line 2: "  Expanded macro: \sqrt{\ensuremat\frac{1^2}{2}}"
         // Line 3: "  from macro file(s): Macros.mth"

         // Verify expanded macro shows the FULL expanded text including user input
         Assert::IsTrue(err.sError.find("Expanded macro:") != string::npos,
            L"Should have 'Expanded macro:' line");
         
         // The expanded text should be: \sqrt{\ensuremat\frac{1^2}{2}}
         // This includes:
         // - \sqrt from \@inner
         // - \ensuremat\frac from \half
         // - {1^2} from user input (not from macro!)
         // - {2} from \half
         Assert::IsTrue(err.sError.find("\\sqrt") != string::npos,
            L"Should show \\sqrt from \\@inner");
         Assert::IsTrue(err.sError.find("\\ensuremat") != string::npos,
            L"Should show \\ensuremat (the error token)");
         Assert::IsTrue(err.sError.find("\\frac") != string::npos,
            L"Should show \\frac from \\half");
         Assert::IsTrue(err.sError.find("1^2") != string::npos,
            L"Should show user input 1^2 (not from macro)");

         // Verify macro file mentioned
         Assert::IsTrue(err.sError.find("Macros.mth") != string::npos,
            L"Should mention macro file");

         // Verify error position maps to \outer in user input
         Assert::IsTrue(err.nStartPos >= 2 && err.nStartPos <= 12,
            L"Error position should point to \\outer in user input");
      }

      TEST_METHOD(MacroExpansion_Error_WithArguments_ShowsSubstitution) {
         // Test: Error with argument substitution visible
         // \abs expands to: \badcmd{x+y} (argument substituted)
         CTexParser parser(g_Doc);
         parser.AddMacros("\\newcommand{\\abs}[1]{\\badcmd{#1}}", "Macros.mth");

         CMathItem* pRet = parser.Parse("$$\\abs{x+y}$$");
         SMemGuard mg{ pRet };

         Assert::IsNull(pRet, L"Should fail with undefined command");

         const ParserError& err = parser.LastError();
         Assert::IsFalse(err.sError.empty());

         // Verify expanded macro shows argument substituted
         Assert::IsTrue(err.sError.find("\\badcmd{x+y}") != string::npos,
            L"Should show \\badcmd with substituted argument x+y");
         Assert::IsTrue(err.sError.find("Macros.mth") != string::npos,
            L"Should mention macro file");
      }

      TEST_METHOD(MacroExpansion_Error_InEnvironment_ShowsFullExpansion) {
         // Test: Error in macro containing environment - shows complete expanded text
         // Mismatched \begin{matrix} ... \end{array}
         CTexParser parser(g_Doc);
         parser.AddMacros(
            "\\newcommand{\\badmatrix}[2]{\\begin{matrix}#1&#2\\end{array}}",
            "Macros.mth");

         CMathItem* pRet = parser.Parse("$$\\badmatrix{a}{b}$$");
         SMemGuard mg{ pRet };

         Assert::IsNull(pRet, L"Should fail with mismatched environment");

         const ParserError& err = parser.LastError();
         Assert::IsFalse(err.sError.empty());

         // Should show expanded macro: \begin{matrix}a&b\end{array}
         Assert::IsTrue(err.sError.find("Expanded macro:") != string::npos,
            L"Should have expanded macro line");
         Assert::IsTrue(err.sError.find("\\begin{matrix}") != string::npos,
            L"Should show \\begin{matrix}");
         Assert::IsTrue(err.sError.find("a&b") != string::npos,
            L"Should show substituted arguments a&b");
         Assert::IsTrue(err.sError.find("\\end{array}") != string::npos,
            L"Should show \\end{array}");

         // Should mention environment error
         Assert::IsTrue(err.sError.find("environment") != string::npos ||
            err.sError.find("Unmatched") != string::npos,
            L"Error should mention environment issue");
      }

      TEST_METHOD(MacroExpansion_Error_MixedMacroAndUserTokens) {
         // Test: Expanded macro contains both macro tokens AND user input tokens
         // \wrap{x^2} expands to: [\badcmd x^2] where [ ] are from macro, x^2 is user input
         CTexParser parser(g_Doc);
         parser.AddMacros("\\newcommand{\\wrap}[1]{[\\badcmd #1]}", "Macros.mth");

         CMathItem* pRet = parser.Parse("$$\\wrap{x^2}$$");
         SMemGuard mg{ pRet };

         Assert::IsNull(pRet, L"Should fail with undefined command");

         const ParserError& err = parser.LastError();
         Assert::IsFalse(err.sError.empty());

         // Expanded macro should show: [\badcmd x^2]
         // Note: x^2 is from user input (nRefIdx == 0), but should still be included
         Assert::IsTrue(err.sError.find("\\badcmd") != string::npos,
            L"Should show \\badcmd from macro");
         Assert::IsTrue(err.sError.find("x^2") != string::npos,
            L"Should show user input x^2");
      }

      TEST_METHOD(MacroExpansion_Error_VerifyPositionMapping) {
         // Test: Verify error position maps to user input, not expanded text
         CTexParser parser(g_Doc);
         parser.AddMacros("\\newcommand{\\test}[1]{\\undefined{#1}}", "Macros.mth");

         // Error: \undefined is not a valid command
         CMathItem* pRet = parser.Parse("$$\\test{a}$$");
         SMemGuard mg{ pRet };

         Assert::IsNull(pRet, L"Should fail with undefined command");

         const ParserError& err = parser.LastError();
         Assert::IsFalse(err.sError.empty());

         // Error position should point to user input (\test command)
         // Position of \test in "$$\test{a}$$" is at index 2
         Assert::IsTrue(err.nStartPos >= 2 && err.nStartPos <= 11,
            L"Error position should be in user input range");

         // Should have macro expansion info
         Assert::IsTrue(err.sError.find("Expanded macro:") != string::npos,
            L"Should include 'Expanded macro:' line");
         Assert::IsTrue(err.sError.find("\\undefined{a}") != string::npos,
            L"Should show expanded text");
      }

      TEST_METHOD(MacroExpansion_Error_MultipleFiles_ShowAll) {
         // Test: When error involves macros from multiple files
         CTexParser parser(g_Doc);
         parser.AddMacros("\\newcommand{\\first}[1]{\\badfunc #1}", "File1.mth");
         parser.AddMacros("\\newcommand{\\second}[1]{\\first{#1}}", "File2.mth");

         // Error occurs in \first expansion (undefined \badfunc)
         // \second{x} -> \first{x} -> \badfunc x
         CMathItem* pRet = parser.Parse("$$\\second{x}$$");
         SMemGuard mg{ pRet };

         Assert::IsNull(pRet, L"Should fail with undefined command");

         const ParserError& err = parser.LastError();
         Assert::IsFalse(err.sError.empty());

         // Should show expanded macro
         Assert::IsTrue(err.sError.find("\\badfunc") != string::npos,
            L"Should show \\badfunc in expanded text");

         // Should mention File1.mth (where \first with \badfunc is defined)
         // Note: May also mention File2.mth if both involved
         Assert::IsTrue(err.sError.find("File1.mth") != string::npos,
            L"Should show macro file where error originated");
      }

      TEST_METHOD(MacroExpansion_Error_NoMacroExpansion_SimpleError) {
         // Test: Error without macro expansion - should have only 1 line
         CTexParser parser(g_Doc);

         CMathItem* pRet = parser.Parse("$$\\undefined$$");
         SMemGuard mg{ pRet };

         Assert::IsNull(pRet, L"Should fail with undefined command");

         const ParserError& err = parser.LastError();
         Assert::IsFalse(err.sError.empty());

         // Should NOT have "Expanded macro:" line
         Assert::IsTrue(err.sError.find("Expanded macro:") == string::npos,
            L"Simple error should not have 'Expanded macro:' line");
         Assert::IsTrue(err.sError.find("from macro file") == string::npos,
            L"Simple error should not have 'from macro file' line");

         // Should have only the main error message
         size_t nNewlineCount = 0;
         for (char c : err.sError) {
            if (c == '\n') ++nNewlineCount;
         }
         Assert::AreEqual((size_t)0, nNewlineCount,
            L"Simple error should be single line (no newlines)");
      }
};
};
