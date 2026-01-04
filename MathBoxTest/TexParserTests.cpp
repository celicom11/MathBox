#include "stdafx.h"
#include "TexParser.h"
#include "ContainerItem.h"
#include "WordItem.h"
#include "RawItem.h"
#include "D2DFontManager.h"
#include "LMMFont.h"

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
   int32_t LineSkipFDU() override {
      return 200;
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
      wsFontDir += L"\\LatinModernFonts\\";
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
   TEST_CLASS(TexParserTests)
   {
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
         CMathItem* pRet = parser.Parse("$${[a]}$$");
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
         Assert::AreEqual("Unexpected empty group", err.sError.c_str());
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
         Assert::AreEqual(err.sError.c_str(), "Inner $..$/$$...$$ are not allowed in math mode!");
         pRet = parser.Parse("$a $$b$$ c$");
         Assert::IsNull(pRet, L"Parse should return nullptr for empty group");
         err = parser.LastError();
         Assert::AreEqual(err.sError.c_str(), "Inner $..$/$$...$$ are not allowed in math mode!");
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
         Assert::IsTrue(pBar->Type() == eacFILLER, L"Rule item should be UNK");

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
         Assert::IsTrue(pOverbar->Type() == eacFILLER, L"Overbar should be FILLER");

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
};
