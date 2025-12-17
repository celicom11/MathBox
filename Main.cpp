#include "stdafx.h"
#include "MathBox\RadicalBuilder.h"
#include "MathBox\FractionBuilder.h"
#include "MathBox\IndexedBuilder.h"
#include "MathBox\LMFontManager.h"
#include "MathBox\WordItemBuilder.h"
#include "MathBox\HBoxItem.h"
#include "MathBox\AccentBuilder.h"
#include "MathBox\VBoxBuilder.h"
#include "MathBox\UnderOverBuilder.h"
#include "MathBox\OpenCloseBuilder.h"
#include "MathBox\LOpBuilder.h"
#include "MathBox\HSpacingBuilder.h"
#include "MathBox\TexParser.h"

//globals
CLMFontManager g_LMFManager;

// Custom HINST_THISCOMPONENT for module handle
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
template <class T> void SafeRelease(T** ppT) {
   if (*ppT) {
      (*ppT)->Release();
      *ppT = nullptr;
   }
}

// Helper class to manage DirectX resources
struct D2DResources {

   ID2D1Factory*              pD2DFactory = nullptr;
   IDWriteFactory*            pDWriteFactory = nullptr;
   ID2D1HwndRenderTarget*     pRenderTarget = nullptr;
   ID2D1SolidColorBrush*      pBlackBrush = nullptr;
   //DWRITE_FONT_METRICS        m_fm;
   ~D2DResources() {
      SafeRelease(&pD2DFactory);
      SafeRelease(&pDWriteFactory);
      SafeRelease(&pRenderTarget);
      SafeRelease(&pBlackBrush);
      g_LMFManager.Clear();
   }

   HRESULT Initialize(HWND hwnd) {
      HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2DFactory);
      CHECK_HR(hr);
      hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
         reinterpret_cast<IUnknown**>(&pDWriteFactory));
      CHECK_HR(hr);
      WCHAR wszDir[MAX_PATH] = { 0 };
      GetCurrentDirectoryW(_countof(wszDir), wszDir);
      hr = g_LMFManager.Init(wszDir, pDWriteFactory);
      CHECK_HR(hr);
      RECT rc;
      GetClientRect(hwnd, &rc);
      D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
      hr = pD2DFactory->CreateHwndRenderTarget(
         D2D1::RenderTargetProperties(),
         D2D1::HwndRenderTargetProperties(hwnd, size),
         &pRenderTarget
      );
      pRenderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
      CHECK_HR(hr);
      hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &pBlackBrush);
      return hr;
   }
};

// --- Main Application Class ---

class DemoApp {
   float               m_fFontSizePt{ 24.0f }; //document font size, in points
   HWND                m_hwnd;
   D2DResources        m_d2d;
   CContainerItem      m_MainBox;
   //CHBoxItem           m_MainBox;
public:
   DemoApp() : m_hwnd(nullptr), m_MainBox(eacHBOX, etsDisplay) {}
   ~DemoApp() {}

   HRESULT Initialize() {
      WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
      wcex.style = CS_HREDRAW | CS_VREDRAW;
      wcex.lpfnWndProc = DemoApp::WndProc;
      wcex.cbClsExtra = 0;
      wcex.cbWndExtra = sizeof(LONG_PTR);
      wcex.hInstance = HINST_THISCOMPONENT;
      wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
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

      HRESULT hr = m_hwnd ? S_OK : E_FAIL;
      if (SUCCEEDED(hr)) {
         hr = m_d2d.Initialize(m_hwnd);
         if (SUCCEEDED(hr)) {
            ArrayTest_();
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
         }
      }
      return hr;
   }

   void RunMessageLoop() {
      MSG msg;
      while (GetMessage(&msg, nullptr, 0, 0)) {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }
   }

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
         case WM_PAINT:
            pApp->OnPaint();
            return 0;
         case WM_SIZE:
            if (pApp->m_d2d.pRenderTarget) {
               D2D1_SIZE_U size = D2D1::SizeU(LOWORD(lParam), HIWORD(lParam));
               pApp->m_d2d.pRenderTarget->Resize(size);
               InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
         case WM_DESTROY:
            PostQuitMessage(0);
            return 1;
         }
      }
      return DefWindowProc(hwnd, message, wParam, lParam);
   }

   void OnPaint() {
      m_d2d.pRenderTarget->BeginDraw();
      m_d2d.pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));
      SDWRenderInfo dwri{
//          false,
          m_fFontSizePt,
          m_d2d.pRenderTarget,
          m_d2d.pD2DFactory,
          m_d2d.pBlackBrush,
          nullptr
      };
      m_MainBox.Draw({ 10.0f,30.0f }, dwri);

      HRESULT hr = m_d2d.pRenderTarget->EndDraw();
      if (FAILED(hr)) {
         wchar_t errorMsg[256];
         swprintf_s(errorMsg, L"EndDraw failed with HRESULT: 0x%08X", hr);
         MessageBox(m_hwnd, errorMsg, L"Render Error", MB_OK);
      }
      ValidateRect(m_hwnd, nullptr);
   }
   //$$S_n = \left(a\right)$$
   void LeftRightTest_() {
      string sTeX("$$S_n = \\left(\\frac{X_1}{A^2}\\middle|\\frac{X_1^2}{A^2}\\middle\\| \\right)$$");
      CTexParser parser;
      parser.SetDocumentFontSizePts(m_fFontSizePt); //12pt
      CMathItem* pRet = parser.Parse(sTeX);
      _ASSERT(pRet);
      m_MainBox.AddBox(pRet, 0, 330);
   }
   void ArrayTest_() {
      string sTeX = "$$x=\\begin{cases} \\frac{1}{1+x^2} & \\text{if } x>0\\\\ \\sum_{i=1}^n i^2 & \
 \\text{if } x\\le 0 \\end{cases}$$";
      CTexParser parser;
      parser.SetDocumentFontSizePts(m_fFontSizePt); //12pt
      CMathItem* pRet = parser.Parse(sTeX);
      _ASSERT(pRet);
      m_MainBox.AddBox(pRet, 0, 330);
   }

   // various "Text* 0+1+2+3"in Text Mode
   void BuildTexts_() {
      CTexParser parser;
      parser.SetDocumentFontSizePts(m_fFontSizePt);
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
      parser.SetDocumentFontSizePts(m_fFontSizePt);
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
      parser.SetDocumentFontSizePts(m_fFontSizePt); //12pt
      CMathItem* pRet = parser.Parse(sTeX);
      _ASSERT(pRet);
      m_MainBox.AddBox(pRet, 0, 330);
   }
   // $$\int_{a}^{b} x^2 \,dx, \int_{a}\limits^{b} x^2 \,dx,$$
   // $\int_{a}^{b} x^2 \,dx, \int_{a}\limits^{b} x^2 \,dx,$
   void BuildLargeOps_() {
      CTexParser parser;
      parser.SetDocumentFontSizePts(m_fFontSizePt);
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
      parser.SetDocumentFontSizePts(m_fFontSizePt);
      CMathItem* pRet = parser.Parse(sTex);
      _ASSERT_RET(pRet, );
      m_MainBox.AddBox(pRet, 0, 330);
   }
   // \ocirc{a},\hat{b},\tilde{c},\dot{d},\ddot{e},\bar{f},\check{g},\acute{h},\grave{x},\breve{y},
   // \vec{z},\threeunderdot{x},\underleftarrow{x},\underrightarrow{x}
   CMathItem* BuildAccentItems1_() {
      //test parser
      CTexParser parser;
      parser.SetDocumentFontSizePts(m_fFontSizePt);
      return parser.Parse("$\\ocirc{a},\\hat{b},\\tilde{c},\\dot{d},\\ddot{e},\\bar{f},\\check{g},\\acute{h},\\grave{x},\\breve{y},\
         \\vec{ z }, \\threeunderdot{ x }, \\underleftarrow{ x }, \\underrightarrow{ x }$");
   }
   //\hat{\dot{x}},\tilde{\bar{A}},\acute{\grave{e}}
   CMathItem* BuildAccentItems2_() {
      //test parser
      CTexParser parser;
      parser.SetDocumentFontSizePts(m_fFontSizePt);
      return parser.Parse("$\\hat{\\dot{x}},\\tilde{\\bar{A}},\\acute{\\grave{e}}$");
   }
   //$\widehat{ABC},\widecheck{ABC},\widetilde{ABC},\overline{ABC},\overbracket{ABC},\overparen{ABC},
   // \overbrace{ABC},obrbrak{ABC},$
   CMathItem* BuildOverItems_() {
      CTexParser parser;
      parser.SetDocumentFontSizePts(m_fFontSizePt);
      return parser.Parse("$\\widehat{ABC},\\widecheck{ABC},\\widetilde{ABC},\\overline{ABC},\
         \\overbracket{ABC},\\overparen{ABC},\\overbrace{ABC},\\obrbrak{ABC},$");
   }
   //$\overleftarrow{ABCDE},\overrightarrow{ABCDE},\overleftrightarrow{ABCDE},\Overrightarrow{ABCDE},$
   CMathItem* BuildOverItems2_() {
      //test parser
      CTexParser parser;
      parser.SetDocumentFontSizePts(m_fFontSizePt);
      return parser.Parse(
         "$\\overleftarrow{ABCDE},\\overrightarrow{ABCDE},\\overleftrightarrow{ABCDE},\\Overrightarrow{ABCDE},$");
   }
   //$\utilde{ABC},\underline{ABC},\underbracket{ABCDE}_{below},\underparen{ABCDE}_{below},\underbrace{ABCDE}_{below},$
   CMathItem* BuildUnderItems_() {
      //test parser
      CTexParser parser;
      parser.SetDocumentFontSizePts(m_fFontSizePt);
      return parser.Parse("$\\utilde{ABC},\\underline{ABC},\\underbracket{ABCDE}_{below},\
         \\underparen{ABCDE}_{below},\\underbrace{ABCDE}_{below},$");
   }
   void BuildAccentItems_() {
      /*CTexParser parser;
      parser.SetDocumentFontSizePts(m_fFontSizePt);
      CMathItem* pRet = parser.Parse("$\\overline{ABC},\\overbracket{ABC}$");
      _ASSERT(pRet);
      if(pRet)
         m_MainBox.AddBox(pRet, 100, 100);*/
      
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
      parser.SetDocumentFontSizePts(m_fFontSizePt);
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
      parser.SetDocumentFontSizePts(m_fFontSizePt);
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

