#include "stdafx.h"
#include "TexParser.h"
#include "LMFontManager.h"
#include "ContainerItem.h"
#include "WordItem.h"
//globals
CLMFontManager g_LMFManager;
IDWriteFactory* g_pDWriteFactory = nullptr;


namespace TexParserTests
{
   TEST_MODULE_INITIALIZE(ModuleInitialize)
   {
      //HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2DFactory);
      //_ASSERT(SUCCEEDED(hr));
      HRESULT hRes = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
         reinterpret_cast<IUnknown**>(&g_pDWriteFactory));
      _ASSERT(SUCCEEDED(hRes));
      WCHAR wszDir[MAX_PATH] = { 0 };
      GetCurrentDirectoryW(_countof(wszDir), wszDir);
      // Remove last two path components from wszDir (e.g. remove "\x64\Debug")
      std::wstring sDir(wszDir);
      for (int i = 0; i < 2; ++i) {
         size_t pos = sDir.find_last_of(L"\\/");
         if (pos == std::wstring::npos) {
            // nothing more to strip
            sDir.clear();
            break;
         }
         sDir.resize(pos);
      }
      hRes = g_LMFManager.Init(sDir, g_pDWriteFactory);
      _ASSERT(SUCCEEDED(hRes));
   }
   TEST_MODULE_CLEANUP(ModuleCleanup)
   {
      if (g_pDWriteFactory) {
         g_pDWriteFactory->Release();
         g_pDWriteFactory = nullptr;
      }
      g_LMFManager.Clear();
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
         CTexParser parser;
         CMathItem* pRet = parser.Parse("$$x_a$$");
         struct SMemGuard {
            CMathItem* pItem;
            ~SMemGuard() { delete pItem; }
         } mg{ pRet };
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
         CTexParser parser;
         CMathItem* pRet = parser.Parse("$$x^2$$");
         struct SMemGuard {
            CMathItem* pItem;
            ~SMemGuard() { delete pItem; }
         } mg{ pRet };
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
         CTexParser parser;
         CMathItem* pRet = parser.Parse("$x_a^b$");
         struct SMemGuard {
            CMathItem* pItem;
            ~SMemGuard() { delete pItem; }
         } mg{ pRet };
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
         CTexParser parser;
         CMathItem* pRet = parser.Parse("$$x_{ab}$$");
         struct SMemGuard {
            CMathItem* pItem;
            ~SMemGuard() { delete pItem; }
         } mg{ pRet };

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
      TEST_METHOD(Indexed_ChainedSubscripts_Display) {
         // Input: "$$x_a_b$$" -> parsed as (x_a)_b
         CTexParser parser;
         CMathItem* pRet = parser.Parse("$$x_a_b$$");
         struct SMemGuard {
            CMathItem* pItem;
            ~SMemGuard() { delete pItem; }
         } mg{ pRet };

         // Should succeed
         Assert::IsNotNull(pRet, L"Parse should succeed for x_a_b");
         Assert::IsTrue(parser.LastError().sError.empty(), L"No error expected");
         // Structure: Outer indexed item (result)_b
         Assert::AreEqual((int)eacINDEXED, (int)pRet->Type(), L"Result should be CIndexedItem");
         CContainerItem* pOuter = dynamic_cast<CContainerItem*>(pRet);
         Assert::AreEqual((size_t)2, pOuter->Items().size(), L"Outer indexed item: base + subscript");
         // Base of outer item should be (x_a) - another indexed item
         CMathItem* pOuterBase = pOuter->Items()[0];
         Assert::AreEqual((int)eacINDEXED, (int)pOuterBase->Type(), L"Base should be indexed item (x_a)");
         // Inner indexed item: x_a
         CContainerItem* pInner = dynamic_cast<CContainerItem*>(pOuterBase);
         Assert::AreEqual((size_t)2, pInner->Items().size(), L"Inner indexed item: base + subscript");
         // Inner base: x
         Assert::AreEqual((int)eacWORD, (int)pInner->Items()[0]->Type(), L"Inner base should be 'x'");
         // Inner subscript: a (cramped)
         CMathItem* pInnerSub = pInner->Items()[1];
         Assert::AreEqual((int)eacWORD, (int)pInnerSub->Type(), L"Inner subscript should be 'a'");
         Assert::IsTrue(pInnerSub->GetStyle().IsCramped(), L"Inner subscript should be cramped");
         // Outer subscript: b (cramped)
         CMathItem* pOuterSub = pOuter->Items()[1];
         Assert::AreEqual((int)eacWORD, (int)pOuterSub->Type(), L"Outer subscript should be 'b'");
         Assert::IsTrue(pOuterSub->GetStyle().IsCramped(), L"Outer subscript should be cramped");
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
         CTexParser parser;
         CMathItem* pRet = parser.Parse("$$_a^b$$");
         struct SMemGuard {
            CMathItem* pItem;
            ~SMemGuard() { delete pItem; }
         } mg{ pRet };

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
   };
};
