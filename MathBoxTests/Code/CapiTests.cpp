#include "stdafx.h"
#include "../../MathBoxLib/LibCode/MathBox_CAPI.h"
#include "../../MathBoxLib/LibCode/DocParamWrap.h"
#include <fstream>

namespace {
   class CCallbackRecorder {
   public:
      wstring sFontsDir;
      uint32_t nDirectoryCalls{ 0 }, nIndexCalls{ 0 }, nMetricCalls{ 0 };
      uint32_t nLines{ 0 }, nRects{ 0 }, nFills{ 0 }, nRuns{ 0 };
      uint16_t nGlyph{ 17 };
      int32_t nLastFont{ -1 }, nLastDrawFont{ -1 };
      float fLastDrawScale{ 0 };
      bool bFailIndices{ false }, bFailMetrics{ false };
      vector<uint32_t> vUnicodes;
      vector<int32_t> vFonts, vDrawFonts;
      vector<float> vDrawY;
      vector<float> vDrawX, vDrawScales;
      bool bCheckGlyph{ true };

      static bool _Directory(void* pContext, uint32_t* pLength, wchar_t* pBuffer) {
         CCallbackRecorder& recorder = *static_cast<CCallbackRecorder*>(pContext);
         ++recorder.nDirectoryCalls;
         uint32_t nRequired = (uint32_t)recorder.sFontsDir.size() + 1;
         if (pBuffer) {
            if (*pLength < nRequired)
               return false;
            memcpy(pBuffer, recorder.sFontsDir.c_str(), nRequired * sizeof(wchar_t));
         }
         *pLength = nRequired;
         return true;
      }
      static bool _Indices(void* pContext, int32_t nFontIdx, uint32_t nCount,
                           const uint32_t* pUnicodes, uint16_t* pIndices) {
         CCallbackRecorder& recorder = *static_cast<CCallbackRecorder*>(pContext);
         ++recorder.nIndexCalls;
         recorder.nLastFont = nFontIdx;
         recorder.vFonts.push_back(nFontIdx);
         recorder.vUnicodes.insert(recorder.vUnicodes.end(), pUnicodes, pUnicodes + nCount);
         if (recorder.bFailIndices)
            return false;
         for (uint32_t nIdx = 0; nIdx < nCount; ++nIdx)
            pIndices[nIdx] = recorder.nGlyph;
         return true;
      }
      static bool _Metrics(void* pContext, int32_t, uint32_t nCount,
                           const uint16_t*, MB_GlyphMetrics* pMetrics, MB_Bounds* pBounds) {
         CCallbackRecorder& recorder = *static_cast<CCallbackRecorder*>(pContext);
         ++recorder.nMetricCalls;
         if (recorder.bFailMetrics)
            return false;
         for (uint32_t nIdx = 0; nIdx < nCount; ++nIdx) {
            Assert::AreEqual((uint32_t)sizeof(MB_GlyphMetrics), pMetrics[nIdx].size_bytes);
            pMetrics[nIdx].nAdvanceWidth = 500;
            pMetrics[nIdx].nAdvanceHeight = 1000;
         }
         *pBounds = { 0, -700, (int32_t)nCount * 500, 200 };
         return true;
      }
      static void _Line(void* pContext, float, float, float, float, uint32_t, float, uint32_t) {
         ++static_cast<CCallbackRecorder*>(pContext)->nLines;
      }
      static void _Rect(void* pContext, float, float, float, float, uint32_t, float, uint32_t) {
         ++static_cast<CCallbackRecorder*>(pContext)->nRects;
      }
      static void _Fill(void* pContext, float, float, float, float, uint32_t) {
         ++static_cast<CCallbackRecorder*>(pContext)->nFills;
      }
      static void _Run(void* pContext, int32_t nFontIdx, uint32_t nCount, const uint16_t* pIndices,
                       float fX, float fY, float fScale, uint32_t) {
         CCallbackRecorder& recorder = *static_cast<CCallbackRecorder*>(pContext);
         Assert::IsTrue(nCount > 0);
         if (recorder.bCheckGlyph)
            Assert::AreEqual(recorder.nGlyph, pIndices[0]);
         ++recorder.nRuns;
         recorder.nLastDrawFont = nFontIdx;
         recorder.fLastDrawScale = fScale;
         recorder.vDrawFonts.push_back(nFontIdx);
         recorder.vDrawY.push_back(fY);
         recorder.vDrawX.push_back(fX);
         recorder.vDrawScales.push_back(fScale);
      }
      MBI_DocRenderer Renderer() {
         return { sizeof(MBI_DocRenderer), this, _Line, _Rect, _Fill, _Run };
      }
   };

   class CApiFixture {
      HMODULE m_hLibrary{ nullptr };
   public:
      const MBI_API* pApi{ nullptr };
      MB_Engine engine{ nullptr };
      MB_MathItem item{ nullptr };
      CCallbackRecorder fonts;
      MB_DocParams params{};

      CApiFixture() {
         // Locate runtime artifacts relative to this test DLL, independent of runner cwd.
         HMODULE hTests = nullptr;
         Assert::IsTrue(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&CCallbackRecorder::_Directory), &hTests) != FALSE);
         wchar_t wszModule[32768]{};
         DWORD nLength = GetModuleFileNameW(hTests, wszModule, _countof(wszModule));
         Assert::IsTrue(nLength > 0 && nLength < _countof(wszModule));
         wstring sOutput(wszModule);
         sOutput.resize(sOutput.find_last_of(L"\\/") + 1);
         fonts.sFontsDir = sOutput + L"..\\..\\MathBoxLib\\LatinModernFonts\\";
         m_hLibrary = LoadLibraryW((sOutput + L"MathBoxLib.dll").c_str());
         Assert::IsNotNull(m_hLibrary);
         auto pGetApi = reinterpret_cast<const MBI_API* (*)()>(GetProcAddress(m_hLibrary, "MB_GetApi"));
         Assert::IsTrue(pGetApi != nullptr);
         pApi = pGetApi();
         params.size_bytes = sizeof(params);
         params.font_size_pts = 12;
         params.font_mgr = { sizeof(MBI_FontManager), 14, &fonts,
            CCallbackRecorder::_Directory, CCallbackRecorder::_Indices, CCallbackRecorder::_Metrics };
      }
      ~CApiFixture() {
         if (pApi) {
            pApi->destroyMathItem(item);
            pApi->destroyEngine(engine);
         }
         if (m_hLibrary)
            FreeLibrary(m_hLibrary);
      }
      void Create() {
         Assert::AreEqual((MB_RET)MBOK, pApi->createEngine(&params, &engine));
         Assert::IsTrue(engine != nullptr);
      }
      void Parse(const char* szText = "abc") {
         Assert::AreEqual((MB_RET)MBOK, pApi->parseLatex(engine, szText, &item));
         Assert::IsNotNull(item);
      }
      void LoadDemoMacros() {
         const wstring sPath = fonts.sFontsDir + L"..\\..\\MathBoxDemo\\Macros.mth";
         std::ifstream file(sPath.c_str(), std::ios::binary);
         Assert::IsTrue(file.is_open());
         const string sMacros((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
         Assert::AreEqual((MB_RET)MBOK, pApi->addMacros(engine, sMacros.c_str(), "Macros.mth"));
      }
   };

   bool _NullContextDirectory(void* pContext, uint32_t*, wchar_t*) {
      Assert::IsNull(pContext);
      return false; // Deliberate host failure before font loading.
   }
   string _MeasuredText(const CCallbackRecorder& fonts) {
      string sText;
      for (uint32_t nUnicode : fonts.vUnicodes)
         sText += (char)nUnicode;
      return sText;
   }
   void _NullContextFill(void* pContext, float, float, float, float, uint32_t) {
      Assert::IsNull(pContext);
   }
}

namespace CapiTests {
   TEST_CLASS(CapiTests) {
   public:
      TEST_METHOD(ApiTable_ReportsAbi2AndRequiredEntries) {
         CApiFixture fixture;
         Assert::AreEqual((uint32_t)2, fixture.pApi->abi_version);
         Assert::IsTrue(fixture.pApi->size_bytes >= sizeof(MBI_API));
         Assert::IsTrue(fixture.pApi->createEngine && fixture.pApi->destroyEngine &&
            fixture.pApi->destroyMathItem && fixture.pApi->getLastError && fixture.pApi->addMacros &&
            fixture.pApi->parseLatex && fixture.pApi->mathItemLineCount && fixture.pApi->mathItemDraw &&
            fixture.pApi->mathItemDrawLines && fixture.pApi->mathItemSelect &&
            fixture.pApi->mathItemGetBox && fixture.pApi->mathItemGetLineBox);
      }
      TEST_METHOD(ApiVersion_HeaderReportsFinalizedAbi2Layout) {
         CApiFixture fixture;
         Assert::AreEqual((size_t)8, offsetof(MBI_API, minor_version));
         Assert::AreEqual(offsetof(MBI_API, minor_version) + sizeof(uint32_t), offsetof(MBI_API, patch_version));
         Assert::AreEqual((size_t)16, offsetof(MBI_API, createEngine));
         Assert::IsTrue(fixture.pApi->size_bytes >= offsetof(MBI_API, patch_version) + sizeof(uint32_t));
         Assert::AreEqual((uint32_t)MB_VERSION_MINOR, fixture.pApi->minor_version);
         Assert::AreEqual((uint32_t)MB_VERSION_PATCH, fixture.pApi->patch_version);
      }
      TEST_METHOD(Callbacks_TwoEnginesKeepIndependentFontAndRendererContexts) {
         CApiFixture first;
         CApiFixture second;
         second.fonts.nGlyph = 23;
         first.Create();
         second.Create();
         Assert::AreEqual((uint32_t)2, first.fonts.nDirectoryCalls);
         Assert::AreEqual((uint32_t)2, second.fonts.nDirectoryCalls);
         first.Parse();
         Assert::IsTrue(first.fonts.nIndexCalls > 0 && first.fonts.nMetricCalls > 0);
         Assert::AreEqual((uint32_t)0, second.fonts.nIndexCalls);
         second.Parse();
         MBI_DocRenderer rendererFirst = first.fonts.Renderer();
         MBI_DocRenderer rendererSecond = second.fonts.Renderer();
         Assert::AreEqual((MB_RET)MBOK, first.pApi->mathItemDraw(first.item, 10, 20, &rendererFirst));
         Assert::IsTrue(first.fonts.nRuns > 0);
         Assert::AreEqual((uint32_t)0, second.fonts.nRuns);
         Assert::AreEqual((MB_RET)MBOK, second.pApi->mathItemDraw(second.item, 30, 40, &rendererSecond));
         Assert::IsTrue(second.fonts.nRuns > 0);
      }
      TEST_METHOD(RendererAdapter_ForwardsAllFourContexts) {
         CCallbackRecorder recorder;
         MBI_DocRenderer renderer = recorder.Renderer();
         SDocRendererWrap wrapper(&renderer);
         wrapper.DrawLine({}, {}, elsSolid, 1, 0);
         wrapper.DrawRect({}, elsSolid, 1, 0);
         wrapper.FillRect({}, 0);
         wrapper.DrawGlyphRun(1, 1, &recorder.nGlyph, {}, 1, 0);
         Assert::AreEqual((uint32_t)1, recorder.nLines);
         Assert::AreEqual((uint32_t)1, recorder.nRects);
         Assert::AreEqual((uint32_t)1, recorder.nFills);
         Assert::AreEqual((uint32_t)1, recorder.nRuns);
      }
      TEST_METHOD(Callbacks_AllowNullContextForStatelessHosts) {
         CApiFixture fixture;
         fixture.params.font_mgr.context = nullptr;
         fixture.params.font_mgr.getFontsDir = _NullContextDirectory;
         Assert::AreEqual((MB_RET)MB_ERR, fixture.pApi->createEngine(&fixture.params, &fixture.engine));
         Assert::IsTrue(fixture.engine == nullptr);
         MBI_DocRenderer renderer{};
         renderer.fillRect = _NullContextFill;
         SDocRendererWrap wrapper(&renderer);
         wrapper.FillRect({}, 0);
      }
      TEST_METHOD(CreateEngine_RejectsIncompatibleStructureSizesBeforeCallbacks) {
         CApiFixture fixture;
         --fixture.params.size_bytes;
         Assert::AreEqual((MB_RET)MB_BADVERSION, fixture.pApi->createEngine(&fixture.params, &fixture.engine));
         fixture.params.size_bytes = sizeof(MB_DocParams);
         --fixture.params.font_mgr.size_bytes;
         Assert::AreEqual((MB_RET)MB_BADVERSION, fixture.pApi->createEngine(&fixture.params, &fixture.engine));
         Assert::AreEqual((uint32_t)0, fixture.fonts.nDirectoryCalls);
         Assert::IsTrue(fixture.engine == nullptr);
      }
      TEST_METHOD(Draw_RejectsIncompatibleRendererSizeBeforeCallbacks) {
         CApiFixture fixture;
         fixture.Create();
         fixture.Parse();
         MBI_DocRenderer renderer = fixture.fonts.Renderer();
         --renderer.size_bytes;
         Assert::AreEqual((MB_RET)MB_BADVERSION, fixture.pApi->mathItemDraw(fixture.item, 0, 0, &renderer));
         Assert::AreEqual((uint32_t)0, fixture.fonts.nRuns);
      }
      TEST_METHOD(Parse_ErrorThenSuccessClearsError) {
         CApiFixture fixture;
         fixture.Create();
         Assert::IsTrue(fixture.pApi->parseLatex(fixture.engine, "\\undefinedcommand", &fixture.item) != MBOK);
         Assert::IsNull(fixture.item);
         uint32_t nStart = 0, nEnd = 0;
         Assert::IsTrue(*fixture.pApi->getLastError(fixture.engine, &nStart, &nEnd) != 0);
         Assert::IsTrue(nEnd > nStart);
         fixture.Parse();
         Assert::AreEqual(string(), string(fixture.pApi->getLastError(fixture.engine, &nStart, &nEnd)));
      }
      TEST_METHOD(Lifetime_ItemCanBeDestroyedAfterEngine) {
         CApiFixture fixture;
         fixture.Create();
         fixture.Parse();
         fixture.pApi->destroyEngine(fixture.engine);
         fixture.engine = nullptr;
         fixture.pApi->destroyMathItem(fixture.item);
         fixture.item = nullptr;
      }
      TEST_METHOD(Utf8_TextUsesUnicodeScalarsAndHostRegularFont) {
         CApiFixture fixture;
         fixture.params.font_mgr.fontCount = 19;
         fixture.Create();
         fixture.Parse("\xD0\x96\xE4\xB8\xAD\xF0\x9F\x98\x80");
         Assert::AreEqual((int32_t)14, fixture.fonts.nLastFont);
         Assert::AreEqual((size_t)3, fixture.fonts.vUnicodes.size());
         Assert::AreEqual((uint32_t)0x416, fixture.fonts.vUnicodes[0]);
         Assert::AreEqual((uint32_t)0x4E2D, fixture.fonts.vUnicodes[1]);
         Assert::AreEqual((uint32_t)0x1F600, fixture.fonts.vUnicodes[2]);
         float fRight = 0;
         Assert::AreEqual((MB_RET)MBOK, fixture.pApi->mathItemGetBox(fixture.item, nullptr, nullptr, &fRight, nullptr));
         Assert::AreEqual(24.0f, fRight, 0.01f);
      }
      TEST_METHOD(Utf8_TextStylesSelectAllFourHostSlots) {
         const char* asCommands[] = { "text", "textbf", "textit", "textbfit" };
         const int32_t anSlots[] = { 14, 15, 16, 17 };
         for (size_t nIdx = 0; nIdx < 4; ++nIdx) {
            CApiFixture fixture;
            fixture.params.font_mgr.fontCount = 19;
            fixture.Create();
            string sInput = string("\\") + asCommands[nIdx] + "{\xD0\x96}";
            fixture.Parse(sInput.c_str());
            Assert::AreEqual(anSlots[nIdx], fixture.fonts.nLastFont);
            MBI_DocRenderer renderer = fixture.fonts.Renderer();
            Assert::AreEqual((MB_RET)MBOK, fixture.pApi->mathItemDraw(fixture.item, 0, 0, &renderer));
            Assert::AreEqual(anSlots[nIdx], fixture.fonts.nLastDrawFont);
         }
      }
      TEST_METHOD(Utf8_MixedTextPreservesAsciiPrefixAndMultilineContent) {
         CApiFixture fixture;
         fixture.params.font_mgr.fontCount = 19;
         fixture.Create();
         fixture.Parse("Environments; \xD0\x96\\newline\xE4\xB8\xAD");
         Assert::AreEqual((uint32_t)2, fixture.pApi->mathItemLineCount(fixture.item));
         Assert::AreEqual((uint32_t)'E', fixture.fonts.vUnicodes[0]);
         Assert::IsTrue(std::find(fixture.fonts.vUnicodes.begin(), fixture.fonts.vUnicodes.end(), 0x416) != fixture.fonts.vUnicodes.end());
      }
      TEST_METHOD(Utf8_MacroArgumentsAndDefaultPreserveUnicode) {
         CApiFixture fixture;
         fixture.params.font_mgr.fontCount = 19;
         fixture.Create();
         Assert::AreEqual((MB_RET)MBOK, fixture.pApi->addMacros(fixture.engine,
            "\\newcommand{\\label}[1][\xD0\x96]{\\textbf{#1}}", "unicode.mth"));
         fixture.Parse("\\label");
         Assert::AreEqual((int32_t)15, fixture.fonts.nLastFont);
         Assert::AreEqual((uint32_t)0x416, fixture.fonts.vUnicodes.back());
      }
      TEST_METHOD(Utf8_MissingFontAndHostFailureRejectIncompleteItems) {
         for (uint32_t nCase = 0; nCase < 3; ++nCase) {
            CApiFixture fixture;
            fixture.params.font_mgr.fontCount = nCase ? 19 : 14;
            fixture.fonts.bFailIndices = nCase == 1;
            fixture.fonts.bFailMetrics = nCase == 2;
            fixture.Create();
            Assert::IsTrue(fixture.pApi->parseLatex(fixture.engine, "\xD0\x96", &fixture.item) != MBOK);
            Assert::IsNull(fixture.item);
         }
      }
      TEST_METHOD(Emoji_NamedAndAliasUseSymbolFontInTextAndMath) {
         const char* asInputs[] = { "\\emoji{grinning-face}", "\\emoji{grinning}", "$\\emoji{grinning-face}$" };
         for (const char* szInput : asInputs) {
            CApiFixture fixture;
            fixture.params.font_mgr.fontCount = 19;
            fixture.Create();
            fixture.Parse(szInput);
            Assert::AreEqual((int32_t)18, fixture.fonts.nLastFont);
            Assert::AreEqual((uint32_t)0x1F600, fixture.fonts.vUnicodes.back());
            MBI_DocRenderer renderer = fixture.fonts.Renderer();
            Assert::AreEqual((MB_RET)MBOK, fixture.pApi->mathItemDraw(fixture.item, 0, 0, &renderer));
            Assert::AreEqual((int32_t)18, fixture.fonts.nLastDrawFont);
         }
      }
      TEST_METHOD(Emoji_InvalidNamesAndArgumentsAreErrors) {
         const char* asInputs[] = { "\\emoji{unknown}", "\\emoji{}", "\\emoji grinning", "\\emoji{{grinning}}" };
         for (const char* szInput : asInputs) {
            CApiFixture fixture;
            fixture.params.font_mgr.fontCount = 19;
            fixture.Create();
            Assert::IsTrue(fixture.pApi->parseLatex(fixture.engine, szInput, &fixture.item) != MBOK);
            Assert::IsNull(fixture.item);
         }
      }
      TEST_METHOD(Emoji_MissingSymbolFontProducesUsefulError) {
         CApiFixture fixture;
         fixture.Create();
         Assert::IsTrue(fixture.pApi->parseLatex(fixture.engine, "\\emoji{warning}", &fixture.item) != MBOK);
         Assert::IsNull(fixture.item);
         Assert::IsTrue(string(fixture.pApi->getLastError(fixture.engine, nullptr, nullptr)).find("symbol font") != string::npos);
      }
      TEST_METHOD(Utf8_ErrorOffsetsAfterUnicodeRemainByteOffsets) {
         CApiFixture fixture;
         fixture.params.font_mgr.fontCount = 19;
         fixture.Create();
         Assert::IsTrue(fixture.pApi->parseLatex(fixture.engine, "\xD0\x96\\unknown", &fixture.item) != MBOK);
         uint32_t nStart = 0, nEnd = 0;
         fixture.pApi->getLastError(fixture.engine, &nStart, &nEnd);
         Assert::AreEqual((uint32_t)2, nStart);
         Assert::AreEqual((uint32_t)10, nEnd);
      }
      TEST_METHOD(Emoji_ItemScaleIsPreservedAndMetricsFailureIsReported) {
         CApiFixture fixture;
         fixture.params.font_mgr.fontCount = 19;
         fixture.Create();
         fixture.Parse("\\Large \\emoji{warning}");
         MBI_DocRenderer renderer = fixture.fonts.Renderer();
         Assert::AreEqual((MB_RET)MBOK, fixture.pApi->mathItemDraw(fixture.item, 0, 0, &renderer));
         Assert::IsTrue(fixture.fonts.fLastDrawScale > 1.0f);
         fixture.pApi->destroyMathItem(fixture.item);
         fixture.item = nullptr;
         fixture.fonts.bFailMetrics = true;
         Assert::IsTrue(fixture.pApi->parseLatex(fixture.engine, "\\emoji{warning}", &fixture.item) != MBOK);
         Assert::IsNull(fixture.item);
         Assert::IsTrue(string(fixture.pApi->getLastError(fixture.engine, nullptr, nullptr)).find("measure emoji") != string::npos);
      }
      TEST_METHOD(Tabular_TextCommandsInMathContentUseTextFonts) {
         CApiFixture fixture;
         fixture.Create();
         fixture.Parse("$\\begin{tabular}{@{}l@{}}"
            "\\scriptsize\\textbackslash begin\\{matrix\\}\\\\"
            "\\scriptsize\\quad a \\& b \\textbackslash\\textbackslash"
            "\\end{tabular}$");
         MBI_DocRenderer renderer = fixture.fonts.Renderer();
         Assert::AreEqual((MB_RET)MBOK, fixture.pApi->mathItemDraw(fixture.item, 10, 20, &renderer));
         Assert::IsTrue(fixture.fonts.nRuns > 0);
         // Text symbols may use the LMM symbol font; ordinary words use Roman.
         Assert::IsTrue(std::find(fixture.fonts.vDrawFonts.begin(), fixture.fonts.vDrawFonts.end(), 1) !=
            fixture.fonts.vDrawFonts.end());
         Assert::IsTrue(std::find(fixture.fonts.vUnicodes.begin(), fixture.fonts.vUnicodes.end(), (uint32_t)'&') !=
            fixture.fonts.vUnicodes.end());
         Assert::IsTrue(fixture.fonts.fLastDrawScale < 1.0f);
         Assert::AreEqual((uint32_t)1, fixture.pApi->mathItemLineCount(fixture.item));
      }
      TEST_METHOD(Tabular_CellNewlinesStayInsideRowsAndStylesReset) {
         CApiFixture fixture;
         fixture.Create();
         fixture.Parse("$$\\begin{tabular}{ll}A\\newline B & \\textbf{C}\\\\D & E\\end{tabular}$$");
         MBI_DocRenderer renderer = fixture.fonts.Renderer();
         Assert::AreEqual((MB_RET)MBOK, fixture.pApi->mathItemDraw(fixture.item, 0, 0, &renderer));
         Assert::AreEqual((size_t)5, fixture.fonts.vDrawFonts.size());
         Assert::AreEqual((int32_t)2, fixture.fonts.vDrawFonts[2]);
         Assert::AreEqual((int32_t)1, fixture.fonts.vDrawFonts[3]);
         Assert::IsTrue(fixture.fonts.vDrawY[1] > fixture.fonts.vDrawY[0]);
         Assert::IsTrue(fixture.fonts.vDrawY[3] > fixture.fonts.vDrawY[1]);
      }
      TEST_METHOD(Tabular_InnerMathWorksUnderInlineAndDisplayMath) {
         const char* asDelimiters[] = { "$", "$$" };
         for (const char* szDelimiter : asDelimiters) {
            CApiFixture fixture;
            fixture.Create();
            string sInput = string(szDelimiter) + "\\begin{tabular}{l}Name $x^2$\\end{tabular}" + szDelimiter;
            fixture.Parse(sInput.c_str());
            Assert::IsTrue(std::find(fixture.fonts.vFonts.begin(), fixture.fonts.vFonts.end(), 1) != fixture.fonts.vFonts.end());
            Assert::IsTrue(fixture.fonts.nMetricCalls > 1);
         }
      }
      TEST_METHOD(Tabular_NestedMakecellMacroPreservesInnerRows) {
         CApiFixture fixture;
         fixture.Create();
         Assert::AreEqual((MB_RET)MBOK, fixture.pApi->addMacros(fixture.engine,
            "\\newcommand{\\makecell}[2][c]{\\begin{tabular}{@{}#1@{}}#2\\end{tabular}}", "cells.mth"));
         fixture.Parse("$$\\begin{tabular}{ll}\\makecell[l]{A\\\\B} & C\\\\D & E\\end{tabular}$$");
         MBI_DocRenderer renderer = fixture.fonts.Renderer();
         Assert::AreEqual((MB_RET)MBOK, fixture.pApi->mathItemDraw(fixture.item, 0, 0, &renderer));
         Assert::AreEqual((size_t)5, fixture.fonts.vDrawY.size());
         Assert::IsTrue(fixture.fonts.vDrawY[1] > fixture.fonts.vDrawY[0]);
         Assert::IsTrue(fixture.fonts.vDrawY[3] > fixture.fonts.vDrawY[1]);
      }
      TEST_METHOD(Makecell_DemoMacroAlignsUnequalLines) {
         const char* asAlignment[] = { "", "[l]", "[c]", "[r]" };
         const float afOffset[] = { 12, 0, 12, 24 };
         for (int nCase = 0; nCase < 4; ++nCase) {
            CApiFixture fixture;
            fixture.Create();
            fixture.LoadDemoMacros();
            string sInput = string("$\\makecell") + asAlignment[nCase] + "{A\\\\BBBB}$";
            fixture.Parse(sInput.c_str());
            MBI_DocRenderer renderer = fixture.fonts.Renderer();
            Assert::AreEqual((MB_RET)MBOK, fixture.pApi->mathItemDraw(fixture.item, 0, 0, &renderer));
            Assert::AreEqual((size_t)2, fixture.fonts.vDrawX.size());
            Assert::AreEqual(afOffset[nCase], fixture.fonts.vDrawX[0] - fixture.fonts.vDrawX[1], 0.03f);
            Assert::IsTrue(fixture.fonts.vDrawY[1] > fixture.fonts.vDrawY[0]);
         }
      }
      TEST_METHOD(Makecell_DemoMacroNestsWithoutSplittingOuterRows) {
         CApiFixture fixture;
         fixture.Create();
         fixture.LoadDemoMacros();
         fixture.Parse("$\\begin{tabular}{ll}\\makecell[l]{\\makecell[r]{A\\\\BB}\\\\C}&D\\\\E&F\\end{tabular}$");
         MBI_DocRenderer renderer = fixture.fonts.Renderer();
         Assert::AreEqual((MB_RET)MBOK, fixture.pApi->mathItemDraw(fixture.item, 0, 0, &renderer));
         Assert::AreEqual((size_t)6, fixture.fonts.vDrawY.size());
         Assert::IsTrue(fixture.fonts.vDrawX[0] > fixture.fonts.vDrawX[1]);
         Assert::IsTrue(fixture.fonts.vDrawY[1] > fixture.fonts.vDrawY[0]);
         Assert::IsTrue(fixture.fonts.vDrawY[2] > fixture.fonts.vDrawY[1]);
         Assert::IsTrue(fixture.fonts.vDrawY[4] > fixture.fonts.vDrawY[2]);
      }
      TEST_METHOD(Makecell_DemoMacroScopedSizeScalesRowsAndRestoresFont) {
         float afSpacing[2]{};
         for (int nSize = 0; nSize < 2; ++nSize) {
            CApiFixture fixture;
            fixture.Create();
            fixture.LoadDemoMacros();
            string sInput = string("$\\begin{tabular}{ll}{") + (nSize ? "\\scriptsize" : "") +
               "\\makecell[l]{A\\\\B}}&C\\end{tabular}$";
            fixture.Parse(sInput.c_str());
            MBI_DocRenderer renderer = fixture.fonts.Renderer();
            Assert::AreEqual((MB_RET)MBOK, fixture.pApi->mathItemDraw(fixture.item, 0, 0, &renderer));
            Assert::AreEqual((size_t)3, fixture.fonts.vDrawScales.size());
            Assert::AreEqual(nSize ? 0.7f : 1.0f, fixture.fonts.vDrawScales[0]);
            Assert::AreEqual(1.0f, fixture.fonts.vDrawScales[2]);
            afSpacing[nSize] = fixture.fonts.vDrawY[1] - fixture.fonts.vDrawY[0];
         }
         Assert::AreEqual(0.7f * afSpacing[0], afSpacing[1], 0.03f);
      }
      TEST_METHOD(Makecell_DemoMacroSupportsMathCellsAndArrayMatrixHosts) {
         const char* asInputs[] = {
            "$\\begin{tabular}{l}\\makecell[l]{Name $x^2$\\\\Value}\\end{tabular}$",
            "$\\begin{array}{ll}\\makecell{A\\\\B}&x\\end{array}$",
            "$\\begin{matrix}\\makecell{A\\\\B}&x\\end{matrix}$"
         };
         for (const char* szInput : asInputs) {
            CApiFixture fixture;
            fixture.Create();
            fixture.LoadDemoMacros();
            fixture.fonts.bCheckGlyph = false; // math atoms also use native LMM glyph indices
            fixture.Parse(szInput);
            MBI_DocRenderer renderer = fixture.fonts.Renderer();
            Assert::AreEqual((MB_RET)MBOK, fixture.pApi->mathItemDraw(fixture.item, 0, 0, &renderer));
            Assert::AreEqual((uint32_t)1, fixture.pApi->mathItemLineCount(fixture.item));
         }
      }
      TEST_METHOD(Makecell_DemoMacroHandlesBlankLinesAndRejectsUnsupportedArguments) {
         const char* asBodies[] = { "", "A\\\\\\\\B", "A\\newline B" };
         for (const char* szBody : asBodies) {
            CApiFixture fixture;
            fixture.Create();
            fixture.LoadDemoMacros();
            string sInput = string("$\\makecell{") + szBody + "}$";
            fixture.Parse(sInput.c_str());
         }
         const char* asInvalid[] = { "$\\makecell*{A}$", "$\\makecell[t]{A}$", "$\\makecell[]{A}$", "$\\makecell[l]$" };
         for (const char* szInput : asInvalid) {
            CApiFixture fixture;
            fixture.Create();
            fixture.LoadDemoMacros();
            Assert::IsTrue(fixture.pApi->parseLatex(fixture.engine, szInput, &fixture.item) != MBOK);
            Assert::IsNull(fixture.item);
            Assert::IsTrue(*fixture.pApi->getLastError(fixture.engine, nullptr, nullptr) != 0);
         }
      }
      TEST_METHOD(LengthScopes_NestedGroupsRestoreRepeatedSetAndAddAssignments) {
         CApiFixture fixture;
         fixture.Create();
         Assert::AreEqual((MB_RET)MBOK, fixture.pApi->addMacros(fixture.engine,
            "\\setlength{\\x}{10pt}", "lengths.mth"));
         fixture.Parse("\\the\\x{\\setlength{\\x}{19pt}\\setlength{\\x}{20pt}\\the\\x"
            "{\\addtolength{\\x}{3pt}\\the\\x}\\the\\x}\\the\\x");
         Assert::AreEqual(string("10.00pt20.00pt23.00pt20.00pt10.00pt"), _MeasuredText(fixture.fonts));
      }
      TEST_METHOD(LengthScopes_MathGroupsAndMacroGroupsRestoreOuterValues) {
         CApiFixture fixture;
         fixture.Create();
         Assert::AreEqual((MB_RET)MBOK, fixture.pApi->addMacros(fixture.engine,
            "\\setlength{\\x}{10pt}"
            "\\newcommand{\\local}{\\begingroup\\setlength{\\x}{7pt}\\the\\x\\endgroup}", "lengths.mth"));
         fixture.Parse("\\the\\x$\\setlength{\\x}{20pt}\\the\\x{\\addtolength{\\x}{2pt}\\the\\x}\\the\\x$"
            "\\the\\x\\local\\the\\x");
         Assert::AreEqual(string("10.00pt20.00pt22.00pt20.00pt10.00pt7.00pt10.00pt"), _MeasuredText(fixture.fonts));
      }
      TEST_METHOD(LengthScopes_TableCellsRestoreAtColumnsAndRows) {
         const char* asEnv[] = { "tabular", "array", "matrix" };
         for (const char* szEnv : asEnv) {
            CApiFixture fixture;
            fixture.Create();
            Assert::AreEqual((MB_RET)MBOK, fixture.pApi->addMacros(fixture.engine,
               "\\setlength{\\x}{10pt}", "lengths.mth"));
            string sInput = string("$\\begin{") + szEnv + "}" + (string(szEnv) == "matrix" ? "" : "{ll}") +
               "\\setlength{\\x}{20pt}\\the\\x&\\the\\x\\cr"
               "\\addtolength{\\x}{5pt}\\the\\x&\\the\\x\\end{" + szEnv + "}$\\the\\x";
            fixture.Parse(sInput.c_str());
            Assert::AreEqual(string("20.00pt10.00pt15.00pt10.00pt10.00pt"), _MeasuredText(fixture.fonts));
         }
      }
      TEST_METHOD(LengthScopes_RootAssignmentsDoNotPersistAcrossParses) {
         CApiFixture fixture;
         fixture.Create();
         Assert::AreEqual((MB_RET)MBOK, fixture.pApi->addMacros(fixture.engine,
            "\\setlength{\\x}{10pt}", "lengths.mth"));
         fixture.Parse("\\setlength{\\x}{25pt}\\the\\x");
         Assert::AreEqual(string("25.00pt"), _MeasuredText(fixture.fonts));
         fixture.pApi->destroyMathItem(fixture.item);
         fixture.item = nullptr;
         fixture.fonts.vUnicodes.clear();
         fixture.Parse("\\the\\x");
         Assert::AreEqual(string("10.00pt"), _MeasuredText(fixture.fonts));
      }
      TEST_METHOD(LengthScopes_NewVariablesDisappearAndFailedParsesRestoreDefaults) {
         const char* asInputs[] = {
            "{\\setlength{\\local}{3pt}}\\the\\local",
            "\\setlength{\\x}{99pt}{\\setlength{\\local}{3pt}\\unknown}",
            "$\\begin{tabular}{l}\\setlength{\\x}{99pt}\\unknown\\end{tabular}$",
            "$\\begin{array}{l}\\setlength{\\x}{99pt}\\unknown\\end{array}$"
         };
         for (const char* szInput : asInputs) {
            CApiFixture fixture;
            fixture.Create();
            Assert::AreEqual((MB_RET)MBOK, fixture.pApi->addMacros(fixture.engine,
               "\\setlength{\\x}{10pt}", "lengths.mth"));
            Assert::IsTrue(fixture.pApi->parseLatex(fixture.engine, szInput, &fixture.item) != MBOK);
            Assert::IsNull(fixture.item);
            fixture.fonts.vUnicodes.clear();
            fixture.Parse("\\the\\x");
            Assert::AreEqual(string("10.00pt"), _MeasuredText(fixture.fonts));
         }
      }
      TEST_METHOD(LengthScopes_NestedTableInheritsCellLengthsWithoutLeaking) {
         CApiFixture fixture;
         fixture.Create();
         Assert::AreEqual((MB_RET)MBOK, fixture.pApi->addMacros(fixture.engine,
            "\\setlength{\\x}{10pt}", "lengths.mth"));
         fixture.Parse("$\\begin{tabular}{ll}\\setlength{\\x}{20pt}"
            "\\begin{tabular}{ll}\\the\\x&\\addtolength{\\x}{3pt}\\the\\x\\end{tabular}"
            "\\the\\x&\\the\\x\\end{tabular}$");
         Assert::AreEqual(string("20.00pt23.00pt20.00pt10.00pt"), _MeasuredText(fixture.fonts));
      }
      TEST_METHOD(LengthScopes_GroupPaddingAffectsOnlyItsOwnTable) {
         CApiFixture fixture;
         fixture.Create();
         fixture.LoadDemoMacros();
         fixture.Parse("{\\setlength{\\tabcolsep}{0pt}$\\begin{tabular}{ll}A&B\\end{tabular}$}"
            "$\\begin{tabular}{ll}C&D\\end{tabular}$");
         MBI_DocRenderer renderer = fixture.fonts.Renderer();
         Assert::AreEqual((MB_RET)MBOK, fixture.pApi->mathItemDraw(fixture.item, 0, 0, &renderer));
         Assert::AreEqual((size_t)4, fixture.fonts.vDrawX.size());
         Assert::AreEqual(8.0f, fixture.fonts.vDrawX[1] - fixture.fonts.vDrawX[0], 0.03f);
         Assert::AreEqual(24.0f, fixture.fonts.vDrawX[3] - fixture.fonts.vDrawX[2], 0.03f);
      }
      TEST_METHOD(LengthScopes_CellAssignmentsDoNotChangeEnclosingTablePadding) {
         const char* asEnv[] = { "tabular", "array" };
         for (const char* szEnv : asEnv) {
            CApiFixture fixture;
            fixture.Create();
            fixture.LoadDemoMacros();
            fixture.fonts.bCheckGlyph = false; // array cells use native LMM glyph indices
            string sInput = string("$\\begin{") + szEnv + "}{ll}\\setlength{\\" +
               (string(szEnv) == "tabular" ? "tabcolsep" : "arraycolsep") + "}{0pt}A&B\\end{" + szEnv + "}$";
            fixture.Parse(sInput.c_str());
            MBI_DocRenderer renderer = fixture.fonts.Renderer();
            Assert::AreEqual((MB_RET)MBOK, fixture.pApi->mathItemDraw(fixture.item, 0, 0, &renderer));
            Assert::AreEqual((size_t)2, fixture.fonts.vDrawX.size());
            Assert::AreEqual(24.0f, fixture.fonts.vDrawX[1] - fixture.fonts.vDrawX[0], 0.03f);
         }
      }
      TEST_METHOD(LengthScopes_CellNewlineKeepsAssignmentsUntilActualRowEnd) {
         CApiFixture fixture;
         fixture.Create();
         Assert::AreEqual((MB_RET)MBOK, fixture.pApi->addMacros(fixture.engine,
            "\\setlength{\\x}{10pt}", "lengths.mth"));
         fixture.Parse("$\\begin{tabular}{l}\\setlength{\\x}{20pt}\\the\\x\\newline\\the\\x\\\\\\the\\x\\end{tabular}$");
         Assert::AreEqual(string("20.00pt20.00pt10.00pt"), _MeasuredText(fixture.fonts));
      }
      TEST_METHOD(GroupErrors_UnclosedBraceBeforeEnvironmentPointsToOpening) {
         CApiFixture fixture;
         fixture.Create();
         fixture.LoadDemoMacros();
         const string sInput = "$\\begin{tabular}{l}{\\scriptsize\\makecell[l]{A\\\\B}\\end{tabular}$";
         Assert::IsTrue(fixture.pApi->parseLatex(fixture.engine, sInput.c_str(), &fixture.item) != MBOK);
         uint32_t nStart = 0, nEnd = 0;
         const string sError = fixture.pApi->getLastError(fixture.engine, &nStart, &nEnd);
         Assert::AreEqual((uint32_t)sInput.find("{\\scriptsize"), nStart);
         Assert::AreEqual(nStart + 1, nEnd);
         Assert::IsTrue(sError.find("Unclosed opening token '{'") != string::npos);
         Assert::IsTrue(sError.find("\\end{tabular}") != string::npos);
         fixture.Parse("$\\makecell{A\\\\B}$"); // recover after grouping failure
      }
      TEST_METHOD(GroupErrors_UnmatchedClosersReturnErrorsWithoutCrashing) {
         const char* asInputs[] = { "}", "[}", "abc}", "\\end{tabular}",
            "$\\begin{tabular}{l}A\\end{matrix}$", "$\\left({x\\right)$" };
         for (const char* szInput : asInputs) {
            CApiFixture fixture;
            fixture.Create();
            Assert::IsTrue(fixture.pApi->parseLatex(fixture.engine, szInput, &fixture.item) != MBOK);
            Assert::IsNull(fixture.item);
            uint32_t nStart = 0, nEnd = 0;
            const string sError = fixture.pApi->getLastError(fixture.engine, &nStart, &nEnd);
            Assert::IsFalse(sError.empty());
            Assert::IsTrue(nEnd > nStart);
            fixture.Parse("abc");
         }
      }
      TEST_METHOD(Tabular_ScopedFontSizeScalesNestedRowSpacing) {
         auto RowSpacing = [](const char* szSize, const char* szBody) {
            CApiFixture fixture;
            fixture.Create();
            string sInput = string("$\\begin{tabular}{ll}{") + szSize +
               "\\begin{tabular}{@{}l@{}}" + szBody + "\\end{tabular}}&C\\end{tabular}$";
            fixture.Parse(sInput.c_str());
            MBI_DocRenderer renderer = fixture.fonts.Renderer();
            Assert::AreEqual((MB_RET)MBOK, fixture.pApi->mathItemDraw(fixture.item, 0, 0, &renderer));
            Assert::AreEqual((size_t)3, fixture.fonts.vDrawY.size());
            Assert::AreEqual(1.0f, fixture.fonts.fLastDrawScale); // C is outside the size group
            return fixture.fonts.vDrawY[1] - fixture.fonts.vDrawY[0];
         };
         const float fNormal = RowSpacing("", "A\\\\B");
         Assert::AreEqual(0.7f * fNormal, RowSpacing("\\scriptsize", "A\\\\B"), 0.03f);
         Assert::AreEqual(fNormal, RowSpacing("", "\\scriptsize A\\\\\\scriptsize B"), 0.03f);
      }
      TEST_METHOD(Tabular_ScopedFontSizeScalesEmptyRowsAndCellNewlines) {
         const char* asBodies[] = { "A\\\\\\\\B", "A\\newline B" };
         for (const char* szBody : asBodies) {
            float afSpacing[2]{};
            for (int nSize = 0; nSize < 2; ++nSize) {
               CApiFixture fixture;
               fixture.Create();
               string sInput = string("$\\begin{tabular}{l}{") + (nSize ? "\\scriptsize" : "") +
                  "\\begin{tabular}{l}" + szBody + "\\end{tabular}}\\end{tabular}$";
               fixture.Parse(sInput.c_str());
               MBI_DocRenderer renderer = fixture.fonts.Renderer();
               Assert::AreEqual((MB_RET)MBOK, fixture.pApi->mathItemDraw(fixture.item, 0, 0, &renderer));
               Assert::AreEqual((size_t)2, fixture.fonts.vDrawY.size());
               afSpacing[nSize] = fixture.fonts.vDrawY[1] - fixture.fonts.vDrawY[0];
            }
            Assert::AreEqual(0.7f * afSpacing[0], afSpacing[1], 0.03f);
         }
      }
      TEST_METHOD(Tabular_EmptyCellsRulesAndTrailingRowTerminatorParse) {
         const char* asBodies[] = { "", "&B&", "A&&C", "A&B\\\\", "\\hline A&B\\\\\\hline" };
         for (const char* szBody : asBodies) {
            CApiFixture fixture;
            fixture.Create();
            string sInput = string("$$\\begin{tabular}{lll}") + szBody + "\\end{tabular}$$";
            fixture.Parse(sInput.c_str());
         }
      }
      TEST_METHOD(Tabular_ExcessColumnsAndInvalidTextReturnErrors) {
         const char* asInputs[] = { "$$\\begin{tabular}{l}A&B\\end{tabular}$$",
            "$$\\begin{tabular}{l}\\frac{1}{2}\\end{tabular}$$",
            "$$\\begin{tabular}{l}A\\hline B\\end{tabular}$$" };
         for (const char* szInput : asInputs) {
            CApiFixture fixture;
            fixture.Create();
            Assert::IsTrue(fixture.pApi->parseLatex(fixture.engine, szInput, &fixture.item) != MBOK);
            Assert::IsNull(fixture.item);
            fixture.Parse("abc"); // parsing can recover after a partially built table
         }
      }
      TEST_METHOD(Tabular_UnicodeAndEmojiUseHostFontsInsideCells) {
         CApiFixture fixture;
         fixture.params.font_mgr.fontCount = 19;
         fixture.Create();
         fixture.Parse("$$\\begin{tabular}{ll}\\textbf{\xD0\x96} & \\emoji{warning}\\end{tabular}$$");
         MBI_DocRenderer renderer = fixture.fonts.Renderer();
         Assert::AreEqual((MB_RET)MBOK, fixture.pApi->mathItemDraw(fixture.item, 0, 0, &renderer));
         Assert::AreEqual((int32_t)15, fixture.fonts.vDrawFonts[0]);
         Assert::AreEqual((int32_t)18, fixture.fonts.vDrawFonts[1]);
      }
   };
}
