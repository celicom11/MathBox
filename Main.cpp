#include "stdafx.h"
#include "D2DRenderer.h"
#include "D2DFontManager.h"
#include "LMMFont.h"
#include "MathBox\TexParser.h"
#include "CfgReader.h"

// Custom HINST_THISCOMPONENT for module handle
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)

//conversion helpers
static bool _W2UA(const std::wstring& wsUtf16, OUT std::string& sUtf8, bool bUTF8) {
   // Special case of empty input string
   if (wsUtf16.empty()) {
      sUtf8.clear();
      return true;
   }

   // Get length (in chars) of resulting UTF-8 string
   const int utf8_length = ::WideCharToMultiByte(bUTF8 ? CP_UTF8 : CP_ACP,	// convert to ANSI/UTF-8
      0,                        // default flags
      wsUtf16.data(),           // source UTF-16 string
      (int)wsUtf16.length(),         // source string length, in wchar_t's,
      NULL,                     // unused - no conversion required in this step
      0,                        // request buffer size
      NULL, NULL                // unused
   );
   if (utf8_length <= 0)
      return false;

   // Allocate destination buffer for UTF-8 string
   sUtf8.resize(utf8_length);

   // Do the conversion from UTF-16 to UTF-8
   return !!::WideCharToMultiByte(bUTF8 ? CP_UTF8 : CP_ACP,	// convert from ANSI/UTF-8
      0,                        // default flags
      wsUtf16.data(),           // source UTF-16 string
      (int)wsUtf16.length(),         // source string length, in wchar_t's,
      &sUtf8[0],                // destination buffer
      utf8_length,              // destination buffer size, in chars
      NULL, NULL                // unused
   );
}
static bool _W2U(const wchar_t* wszIn, int nLen, OUT std::string& sUtf8) {
   std::wstring wsIn(wszIn, nLen >= 0 ? nLen : wcslen(wszIn));
   return _W2UA(wsIn, sUtf8, true);
}
//read ANY file to Utf8 string
static bool _readFile2Utf8(const wstring& sDocPath, OUT string& sUtf8) {
      //read file to tmp buffer
      FILE* fp = _wfsopen(sDocPath.c_str(), L"rb", _SH_DENYNO);
      if (!fp)
         return false;
      //move on
      fseek(fp, 0, SEEK_END);
      long length = ftell(fp);
      fseek(fp, 0, SEEK_SET);
      if (length <= 0) {
         fclose(fp);
         return true; //empty file
      }
      // allocate memory:
      vector<char> vBuf(length + 2);
      char* szBuf = vBuf.data();
      // read data as a block:
      size_t nread = fread(szBuf, 1, length, fp);
      szBuf[nread] = szBuf[nread + 1] = 0;
      fclose(fp);
      //check for Utf16LE
      if (szBuf[0] == 0xFF && szBuf[1] == 0xFE) {
         szBuf += 2;//skip BOM
         nread -= 2;
         //convert UTF16->UTF8
         if (!_W2U((PCWSTR)szBuf, (int)nread, sUtf8)) {
            //TRACE("Failed to convert UTF16 to UTF8!\n");
            return false;
         }
      }
      else {
         //treat it as UTF8
         if (szBuf[0] == 0xEF && szBuf[1] == 0xBB && szBuf[2] == 0xBF) {
            szBuf += 3;
            nread -= 3;
         }
         sUtf8.assign(szBuf, nread);
      }
      return true;
}
// --- Main Application Class ---
class DemoApp : public IDocParams, 
                public IFontProvider_ {
   HWND              m_hwnd{ nullptr };
   CD2DRenderer      m_d2dr;
   CD2DFontManager   m_D2DFontManager;
   CLMMFont          m_LMMFont;
   CMathItem*        m_pMainBox{nullptr};
   //single/current document params
   int32_t           m_nLineSkipFDU{ 200 };
   uint32_t          m_clrText{ 0x004F00 };
   uint32_t          m_clrBkg{ 0xFFFFFF };
   uint32_t          m_clrSel{ 0xFF4FFF };
   float             m_fFontSizePts{ 24.0f };
public:
   DemoApp() : m_d2dr(*this) {}
   ~DemoApp() {
      delete m_pMainBox;
   }

   HRESULT Initialize() {
      //read config
      CCfgReader cfgr;
      if(!cfgr.Init(L"MathBoxDemo.cfg")) {
         MessageBox(nullptr, L"Could not read MathBoxDemo.cfg", L"Error", MB_OK | MB_ICONERROR);
         return E_FAIL;
      }
      wstring sDocPath;
      if (!cfgr.GetSVal(L"DocPath", sDocPath)) {
         wstring sErr = L"Could not read [" + sDocPath + L"]";
         MessageBox(nullptr, sErr.c_str(), L"Error", MB_OK | MB_ICONERROR);
         return E_FAIL;
      }
      string sDocUtf8;
      if(!_readFile2Utf8(sDocPath, sDocUtf8)) {
         wstring sErr = L"Could not open/read document file [" + sDocPath + L"]";
         MessageBox(nullptr, sErr.c_str(), L"Error", MB_OK | MB_ICONERROR);
         return E_FAIL;
      }
      // Apply cfg:
      cfgr.GetFVal(L"FontSizePts", m_fFontSizePts);
      cfgr.GetHVal(L"ColorText", m_clrText); m_d2dr.SetTextColor(m_clrText);
      cfgr.GetHVal(L"ColorBkg", m_clrBkg);
      cfgr.GetHVal(L"ColorSelect", m_clrSel);

      //create window
      WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
      wcex.style = CS_HREDRAW | CS_VREDRAW;
      wcex.lpfnWndProc = DemoApp::WndProc;
      wcex.cbClsExtra = 0;
      wcex.cbWndExtra = sizeof(LONG_PTR);
      wcex.hInstance = HINST_THISCOMPONENT;
      wcex.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
      wcex.lpszClassName = L"MathBoxDemoApp";

      RegisterClassEx(&wcex);

      m_hwnd = CreateWindow(
         L"MathBoxDemoApp",
         L"MathBox Demo",
         WS_OVERLAPPEDWINDOW,
         CW_USEDEFAULT, CW_USEDEFAULT,
         1000, 800,
         nullptr, nullptr, HINST_THISCOMPONENT, this
      );
      _ASSERT_RET(m_hwnd, E_FAIL);
      HRESULT hr = m_d2dr.Initialize(m_hwnd);
      CHECK_HR(hr);
      WCHAR wszDir[MAX_PATH] = { 0 };
      GetCurrentDirectoryW(_countof(wszDir), wszDir);
      hr = m_D2DFontManager.Init(wszDir, m_d2dr.DWFactory());
      CHECK_HR(hr);
      m_LMMFont.Init(wszDir);
      //parse document
      CTexParser parser(*this);
      m_pMainBox = parser.Parse(sDocUtf8);
      _ASSERT(m_pMainBox);
      //m_pMainBox->MoveTo(0, 330);

      //ArrayTest_();
      //LeftRightTest_();
      //BuildIndexed_("af");
      //BuildTexts_();
      //BuildRadicals_();
      //BuildMathFonts_();
      //BuildAccentItems_();
      //BuildVBoxes_();
      //BuildLargeOps_();
      //BuildOpenClose_();
      ShowWindow(m_hwnd, SW_SHOWNORMAL);
      UpdateWindow(m_hwnd);
      return S_OK;
   }
   void RunMessageLoop() {
      MSG msg;
      while (GetMessage(&msg, nullptr, 0, 0)) {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }
   }
//IDocParams
   IFontManager& FontManager() override { return m_D2DFontManager; }
   ILMFManager& LMFManager() override { return m_LMMFont; }
   float DefaultFontSizePts() override { return m_fFontSizePts; }
   int32_t DefaultLineSkipEm() override { return m_nLineSkipFDU; }
   uint32_t ColorText() override { return m_clrText; }
   uint32_t ColorBkg() override { return m_clrBkg;}
   uint32_t ColorSelection() override {return m_clrSel;}
   void SetColorText(uint32_t clrText) override { 
      m_clrText = clrText; 
      m_d2dr.SetTextColor(clrText);
   }
 //IFontProvider_
   uint32_t FontCount() const override { return m_D2DFontManager.FontCount(); }
   IDWriteFontFace* GetDWFont(int32_t nIdx) const { 
      return m_D2DFontManager.GetDWFont(nIdx); 
   }
   float GetFontSizePts() const { return m_fFontSizePts; }

private:
   static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
      if (message == WM_CREATE) {
         LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
         DemoApp* pApp = (DemoApp*)pcs->lpCreateParams;
         SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pApp);
         return 1;
      }

      DemoApp* pApp = reinterpret_cast<DemoApp*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
      if (pApp) {
         switch (message) {
         case WM_ERASEBKGND: return TRUE;//do nothing!
         case WM_PAINT:
            pApp->OnPaint();
            return 0;
         case WM_SIZE:
            pApp->OnSize(lParam);
            ::InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
         case WM_DESTROY:
            ::PostQuitMessage(0);
            return 1;
         }
      }
      return DefWindowProc(hwnd, message, wParam, lParam);
   }

   void OnPaint() {
      m_d2dr.BeginDraw();
      m_d2dr.EraseBkgnd(m_clrBkg);
      if(m_pMainBox)
         m_pMainBox->Draw({ 10.0f,30.0f }, m_d2dr);

      HRESULT hr = m_d2dr.EndDraw();
      if (FAILED(hr)) {
         wchar_t errorMsg[256];
         swprintf_s(errorMsg, L"EndDraw failed with HRESULT: 0x%08X", hr);
         MessageBox(m_hwnd, errorMsg, L"Render Error", MB_OK);
      }
      ::ValidateRect(m_hwnd, nullptr);
   }
   void OnSize(LPARAM lParam) {
      m_d2dr.OnSize(lParam);
   }
   /*
   //$$S_n = \left(a\right)$$
   void LeftRightTest_() {
      string sTeX("$$S_n = \\left(\\frac{X_1}{A^2}\\middle|\\frac{X_1^2}{A^2}\\middle\\| \\right)$$");
      CTexParser parser;
      parser.SetDocumentFontSizePts(m_dwri.fFontSizePts); //12pt
      CMathItem* pRet = parser.Parse(sTeX);
      _ASSERT(pRet);
      m_MainBox.AddBox(pRet, 0, 330);
   }
   void ArrayTest_() {
      string sTeX = "$$x=\\begin{dcases} \\frac{1}{1+x^2} & \\text{if } x>0\\\\ \\sum_{i=1}^n i^2 & \
 \\text{if } x\\le 0 \\end{dcases}$$";
      CTexParser parser;
      parser.SetDocumentFontSizePts(m_dwri.fFontSizePts); //12pt
      CMathItem* pRet = parser.Parse(sTeX);
      _ASSERT(pRet);
      m_MainBox.AddBox(pRet, 0, 330);
   }
   // various "Text* 0+1+2+3"in Text Mode
   void BuildTexts_() {
      CTexParser parser;
      parser.SetDocumentFontSizePts(m_dwri.fFontSizePts);
      string sText1 = "\\text{A }\\text{B }\\text{C }\\\\\\text{A B C }";
      string sText = "Text 0+1+2+3\\\\\n"
         "\\text{Text 0+1+2+3}\\\\\n"
         "\\textit{Textit 0+1+2+3}\\\\\n"
         "\\emph{Emph 0+1+2+3}\\\\\n"
         "\\textsl{Textsl 0+1+2+3 }\\\\\n"
         "\\textrm{Textrm 0+1+2+3 }\\\\\n"
         "\\textup{Textup 0+1+2+3 }\\\\\n"
         "\\textnormal{textnormal 0+1+2+3 }\\\\\n"
         "\\textmd{Textmd 0+1+2+3 }\\\\\n"
         "\\textsf{Textsf 0+1+2+3 }\\\\\n"
         "\\texttt{Texttt 0+1+2+3 }\\\\\n"
         "\\textbf{Textbf 0+1+2+3 }\\\\\n"
         "\\textsc{Textsc 0+1+2+3 }";
      CMathItem* pRet = parser.Parse(sText1);
      _ASSERT(pRet);
      if(pRet)
         m_MainBox.AddBox(pRet, 0, 330);
   }
   // various $$3x^2 \in R \subset Q$$
   void BuildMathFonts_() {
      PCSTR aMathFonts[]{ "mathnormal", "mathit","mathrm","mathbf","mathbfup","mathbfit","mathscr",
                        "mathbfscr","mathcal","mathbfcal","mathsf","mathsfup","mathsfit","mathbfsfup","mathtt",
                        "mathbb","mathbbit","mathfrak","mathbffrak" };
      CTexParser parser;
      parser.SetDocumentFontSizePts(m_dwri.fFontSizePts);
      string sText("$");
      for (PCSTR szFont : aMathFonts) {
         sText += "\\" + string(szFont) + "{3x^2 \\in R \\subset Q}\\\\\n";
      }
      sText.pop_back();
      sText.pop_back();
      sText.pop_back();
      sText += "$";
      CMathItem* pHBox = parser.Parse(sText);
      _ASSERT_RET(pHBox, );
      m_MainBox.AddBox(pHBox, 100, 100);
   }
   // $$\textit{\fontsize{##pt}{##pt}\selectfont {base}}_1^2$$
   void BuildIndexed_(PCSTR szBase) {
      CMathStyle style(m_MainBox.GetStyle());
      int aFontSize[] = { 9, 12 , 14, 18 , 21, 24, 30, 40, 50 };
      string sTeX("$$");
      for (int nPts : aFontSize) {
         sTeX += "\\textit{\\fontsize{" + std::to_string(nPts) + 
                 "pt}{1pt}\\selectfont{" + szBase + "}}_1^2,\n";
      }
      sTeX.pop_back();
      sTeX += "$$";
      CTexParser parser;
      parser.SetDocumentFontSizePts(m_dwri.fFontSizePts); //12pt
      CMathItem* pRet = parser.Parse(sTeX);
      _ASSERT(pRet);
      m_MainBox.AddBox(pRet, 0, 330);
   }
   // $$\int_{a}^{b} x^2 \,dx, \int_{a}\limits^{b} x^2 \,dx,$$
   // $\int_{a}^{b} x^2 \,dx, \int_{a}\limits^{b} x^2 \,dx,$
   void BuildLargeOps_() {
      CTexParser parser;
      parser.SetDocumentFontSizePts(m_dwri.fFontSizePts);
      CMathItem* pRet = parser.Parse("$$\\int_{a}^{b} x^2 \\,dx, \\int\\limits_{a}^{b} x^2 \\,dx,$$\
$\\int_{a}^{b} x^2 \\,dx, \\int\\limits_{a}^{b} x^2 \\,dx,$");
      m_MainBox.AddBox(pRet, 0, 330);
   }
   //$$\sqrt{\sqrt[\textit{ABC}]{\frac{\textit{\fontsize{##pt}{??pt}\selectfont x}_1^2}{2}}}
   //\quad...$$
   void BuildRadicals_() {
      PCSTR aFontSize[] = { "9", "12", "14", "18", "21", "24", "30", "40", "50" };
      string sTex = "$$";
      for (PCSTR szPts : aFontSize) {
         string sPts(szPts);
         sTex += "\\sqrt{\\sqrt[\\textit{ABC}]{\\frac{\\textit{\\fontsize{" + sPts +
            "pt}{1pt}\\selectfont x}_1^2}{2}}}";
         if (sPts != "50")
            sTex += "\\quad";
      }
      sTex += "$$";
      CTexParser parser;
      parser.SetDocumentFontSizePts(m_dwri.fFontSizePts);
      CMathItem* pRet = parser.Parse(sTex);
      _ASSERT_RET(pRet, );
      m_MainBox.AddBox(pRet, 0, 330);
   }
   // \ocirc{a},\hat{b},\tilde{c},\dot{d},\ddot{e},\bar{f},\check{g},\acute{h},\grave{x},\breve{y},
   // \vec{z},\threeunderdot{x},\underleftarrow{x},\underrightarrow{x}
   CMathItem* BuildAccentItems1_() {
      //test parser
      CTexParser parser;
      parser.SetDocumentFontSizePts(m_dwri.fFontSizePts);
      return parser.Parse("$\\ocirc{a},\\hat{b},\\tilde{c},\\dot{d},\\ddot{e},\\bar{f},\\check{g},\\acute{h},\\grave{x},\\breve{y},\
         \\vec{ z }, \\threeunderdot{ x }, \\underleftarrow{ x }, \\underrightarrow{ x }$");
   }
   //\hat{\dot{x}},\tilde{\bar{A}},\acute{\grave{e}}
   CMathItem* BuildAccentItems2_() {
      //test parser
      CTexParser parser;
      parser.SetDocumentFontSizePts(m_dwri.fFontSizePts);
      return parser.Parse("$\\hat{\\dot{x}},\\tilde{\\bar{A}},\\acute{\\grave{e}}$");
   }
   //$\widehat{ABC},\widecheck{ABC},\widetilde{ABC},\overline{ABC},\overbracket{ABC},\overparen{ABC},
   // \overbrace{ABC},obrbrak{ABC},$
   CMathItem* BuildOverItems_() {
      CTexParser parser;
      parser.SetDocumentFontSizePts(m_dwri.fFontSizePts);
      return parser.Parse("$\\widehat{ABC},\\widecheck{ABC},\\widetilde{ABC},\\overline{ABC},\
         \\overbracket{ABC},\\overparen{ABC},\\overbrace{ABC},\\obrbrak{ABC},$");
   }
   //$\overleftarrow{ABCDE},\overrightarrow{ABCDE},\overleftrightarrow{ABCDE},\Overrightarrow{ABCDE},$
   CMathItem* BuildOverItems2_() {
      //test parser
      CTexParser parser;
      parser.SetDocumentFontSizePts(m_dwri.fFontSizePts);
      return parser.Parse(
         "$\\overleftarrow{ABCDE},\\overrightarrow{ABCDE},\\overleftrightarrow{ABCDE},\\Overrightarrow{ABCDE},$");
   }
   //$\utilde{ABC},\underline{ABC},\underbracket{ABCDE}_{below},\underparen{ABCDE}_{below},\underbrace{ABCDE}_{below},$
   CMathItem* BuildUnderItems_() {
      //test parser
      CTexParser parser;
      parser.SetDocumentFontSizePts(m_dwri.fFontSizePts);
      return parser.Parse("$\\utilde{ABC},\\underline{ABC},\\underbracket{ABCDE}_{below},\
         \\underparen{ABCDE}_{below},\\underbrace{ABCDE}_{below},$");
   }
   void BuildAccentItems_() {
      CTexParser parser;
      parser.SetDocumentFontSizePts(m_dwri.fFontSizePts);
      CMathItem* pRet = parser.Parse("$\\overline{ABC},\\overbracket{ABC}$");
      _ASSERT(pRet);
      if(pRet)
         m_MainBox.AddBox(pRet, 100, 100);
      
      CMathStyle style(m_MainBox.GetStyle());
      int32_t nY = 100, nInterLine = 1600;
      m_MainBox.AddBox(BuildAccentItems1_(), 100, nY);
      nY += nInterLine;
      m_MainBox.AddBox(BuildAccentItems2_(), 100, nY);
      nY += nInterLine;
      m_MainBox.AddBox(BuildOverItems_(), 100, nY);
      nY += nInterLine;
      m_MainBox.AddBox(BuildOverItems2_(), 100, nY);
      nY += nInterLine;
      m_MainBox.AddBox(BuildUnderItems_(), 100, nY);
   }
   void BuildVBoxes_() {
      CMathStyle style(m_MainBox.GetStyle());
      int32_t nY = 100, nInterLine = 1600;
      CTexParser parser;
      parser.SetDocumentFontSizePts(m_dwri.fFontSizePts);
      CMathItem* pRet = parser.Parse("${_a^b}, {^a_b}, \\underset{a}{b},\\overset{a}{b},\
\\substack{b\\\\a},\\substack{a\\\\b},\\substack{\\displaystyle b\\\\\\displaystyle a},\
\\substack{\\displaystyle a\\\\\\displaystyle b},\\stackrel{\\scriptscriptstyle >}{<},\
\\stackrel{\\scriptscriptstyle ?}{=},\
\\stackunder{a}{b},\\stackunder[0.1em]{a}{b},\\stackunder[0.2em]{a}{b},\\stackunder[0.3em]{a}{b},\\binom{ a }{b}$");
      m_MainBox.AddBox(pRet, 100, nY);
   }
   void BuildOpenClose_() {
      CMathStyle style(m_MainBox.GetStyle());
      int32_t nY = 100, nInterLine = 3200;
      CMathItem* pHBox = BuildDelimitersParser_("(", ")", "\\lbrack", "]");
      if(pHBox)
         m_MainBox.AddBox(pHBox, 100, nY);
      nY += nInterLine;
      pHBox = BuildDelimitersParser_("\\{", "\\}", "\\backslash", "/");
      if (pHBox)
         m_MainBox.AddBox(pHBox, 100, nY);
      nY += nInterLine;
      pHBox = BuildDelimitersParser_("\\langle", "\\rangle", "\\lAngle", "\\rAngle");
      if (pHBox)
         m_MainBox.AddBox(pHBox, 100, nY);
      nY += nInterLine;
      pHBox = BuildDelimitersParser_("\\uparrow", "\\downarrow", "\\Uparrow", "\\Downarrow");
      if (pHBox)
         m_MainBox.AddBox(pHBox, 100, nY);
      nY += nInterLine;
      pHBox = BuildDelimitersParser_("\\updownarrow", "\\Updownarrow", "\\|", "|");
      if (pHBox)
         m_MainBox.AddBox(pHBox, 100, nY);
      nY += nInterLine;
      pHBox = BuildDelimitersParser_("\\lceil", "\\rceil", "\\lfloor", "\\rfloor");
      if (pHBox)
         m_MainBox.AddBox(pHBox, 100, nY);
      nY += nInterLine;
      pHBox = BuildDelimitersParser_("\\lBrack", "\\rBrack", "\\lgroup", "\\rgroup");
      if (pHBox)
         m_MainBox.AddBox(pHBox, 100, nY);
   }
   CMathItem* BuildDelimitersParser_(PCSTR szDelim1, PCSTR szDelim2, PCSTR szDelim3, PCSTR szDelim4) {
      CMathStyle style(m_MainBox.GetStyle());
      CTexParser parser;
      parser.SetDocumentFontSizePts(m_dwri.fFontSizePts);
      string sText("$$");
      //1/2
      sText += string(szDelim1) + "\\big" + szDelim1 + "\\Big" + szDelim1 + "\\bigg" + szDelim1 + 
                                 "\\Bigg" + szDelim1;
      sText += string("\\Bigg") + szDelim2 + "\\bigg" + szDelim2 + "\\Big" + szDelim2 + "\\big" + szDelim2 + 
         szDelim2;
      sText += ",";
      //3/4
      sText += string(szDelim3) + "\\big" + szDelim3 + "\\Big" + szDelim3 + "\\bigg" + szDelim3 + "\\Bigg" + szDelim3;
      sText += string("\\Bigg") + szDelim4 + "\\bigg" + szDelim4 + "\\Big" + szDelim4 + "\\big" + szDelim4 + szDelim4;
      sText += "$$";
      CMathItem* pRet = parser.Parse(sText);
      _ASSERT(pRet);
      return pRet;
   }
   */
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
   HeapSetInformation(nullptr, HeapEnableTerminationOnCorruption, nullptr, 0);
   if (SUCCEEDED(CoInitialize(nullptr))) {
      DemoApp app;
      if (SUCCEEDED(app.Initialize())) {
         app.RunMessageLoop();
      }
      CoUninitialize();
   }
   return 0;
}

