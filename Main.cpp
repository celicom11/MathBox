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
            //BuildRadicals_();
            //BuildIndexed_("af");
            //BuildTexts_();
            //BuildMathFonts_();
            BuildAccentItems_();
            //BuildVBoxes_();
            //BuildOpenClose_();
            //BuildLargeOps_();
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
          false,
          m_fFontSizePt,
          m_d2d.pRenderTarget,
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
   // various $$Text\ 0+1+2+3$$
   void BuildTexts_() {
      CMathStyle style(m_MainBox.GetStyle());
      PCSTR szTestText = "Text 0+1+2+3";
      int32_t nY = 100, nInterLine = 1500;
      m_MainBox.AddBox(CWordItemBuilder::BuildText("text",       szTestText, style, 1.0f),100, nY); 
      nY += nInterLine;
      m_MainBox.AddBox(CWordItemBuilder::BuildText("textit", szTestText, style, 1.0f),100, nY);
      nY += nInterLine;
      m_MainBox.AddBox(CWordItemBuilder::BuildText("emph", szTestText, style, 1.0f), 100, nY);
      nY += nInterLine;
      m_MainBox.AddBox(CWordItemBuilder::BuildText("textsl", szTestText, style, 1.0f), 100, nY);
      nY += nInterLine;
      szTestText = "Textrm 0+1+2+3";
      m_MainBox.AddBox(CWordItemBuilder::BuildText("textrm", szTestText, style, 1.0f), 100, nY);
      nY += nInterLine;
      m_MainBox.AddBox(CWordItemBuilder::BuildText("textup", szTestText, style, 1.0f), 100, nY);
      nY += nInterLine;
      m_MainBox.AddBox(CWordItemBuilder::BuildText("textnormal", szTestText, style, 1.0f), 100, nY);
      nY += nInterLine;
      m_MainBox.AddBox(CWordItemBuilder::BuildText("textmd", szTestText, style, 1.0f), 100, nY);
      nY += nInterLine;
      szTestText = "Textsf 0+1+2+3";
      m_MainBox.AddBox(CWordItemBuilder::BuildText("textsf", szTestText, style, 1.0f), 100, nY);
      nY += nInterLine;
      szTestText = "Texttt 0+1+2+3";
      m_MainBox.AddBox(CWordItemBuilder::BuildText("texttt", szTestText, style, 1.0f), 100, nY);
      nY += nInterLine;
      szTestText = "Textbf 0+1+2+3";
      m_MainBox.AddBox(CWordItemBuilder::BuildText("textbf", szTestText, style, 1.0f), 100, nY);
      nY += nInterLine;
      szTestText = "Textsc 0+1+2+3";
      m_MainBox.AddBox(CWordItemBuilder::BuildText("textsc", szTestText, style, 1.0f), 100, nY);
      nY += nInterLine;
   }
   // various $$3x^2 \in R \subset Q$$
   void BuildMathFonts_() {
      CMathStyle style(m_MainBox.GetStyle());
      CMathStyle styleSuper(m_MainBox.GetStyle());
      styleSuper.Decrease();
      if (styleSuper.Style() == etsText)
         styleSuper.Decrease();
      PCSTR aMathFonts[]{ "mathnormal", "it", "mathit","mathrm","mathbf","mathbfup","mathbfit","mathscr",
                        "mathbfscr","mathcal","mathbfcal","mathsf","mathsfup","mathsfit","mathbfsfup","mathtt",
                        "mathbb","mathbbit","mathfrak","mathbffrak" };
      int32_t nY = 100, nInterLine = 1750;
      for (PCSTR szFont : aMathFonts) {
         CHBoxItem* pHBox = new CHBoxItem(style);
         //build parts
         CMathItem* pNum = CWordItemBuilder::BuildMathWord(szFont, "3",true, style);
         pHBox->AddItem(pNum);
         CMathItem* pBase = CWordItemBuilder::BuildMathWord(szFont, "x", false, style);
         _ASSERT(pBase);
         CMathItem* pIndex = CWordItemBuilder::BuildMathWord(szFont, "2", true, styleSuper);
         _ASSERT(pIndex);
         pHBox->AddItem(CIndexedBuilder::BuildIndexed(style, 1.0f, pBase, pIndex, nullptr));
         bool bOk = CWordItemBuilder::BuildMathText(szFont, "\\in R \\subset Q", style, *pHBox);
          _ASSERT(bOk);
          pHBox->Update();
         m_MainBox.AddBox(pHBox, 100, nY);
         nY += nInterLine;
      }
   }
   // $$\textit{\fontsize{##pt}{##pt}\selectfont {base}}_1^2$$
   void BuildIndexed_(PCSTR szBase) {
      CMathStyle style(m_MainBox.GetStyle());
      const float aFontSize[] = { 9, 12 , 14, 18 , 21, 24, 30, 40, 50};
      //styles for indexed Numerator
      CMathStyle styleSuper(style);
      styleSuper.Decrease();
      if(styleSuper.Style() == etsText)
         styleSuper.Decrease();
      CMathStyle styleSubs(styleSuper);
      styleSubs.SetCramped(true);
      CHBoxItem* pHBox = new CHBoxItem(style);
/*      for (float fSizePt : aFontSize) {
         CMathItem* pBase = CWordItemBuilder::BuildText("textit", szBase, style, fSizePt / m_fFontSizePt);
         _ASSERT_RET(pBase, );
         CMathItem* pSuperS = CWordItemBuilder::BuildMathWord("","2", true, styleSuper, 1.0f);
         _ASSERT_RET(pSuperS, );
         CMathItem* pSubS = CWordItemBuilder::BuildMathWord("", "1", true, styleSubs, 1.0f);
         _ASSERT_RET(pSubS, );
         CMathItem* pIndexed = CIndexedBuilder::BuildIndexed(style, 1.0f, pBase, pSuperS, pSubS);
         pHBox->AddItem(pIndexed);
         CMathItem* pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
         pHBox->AddItem(pComma);
      }*/
      CUnderOverBuilder builder;
      //\overparen{ABCDEF}^{n\,times}
      CMathItem* pBase = CWordItemBuilder::BuildMathWord("", "ABCDE", false, style);
      CMathItem* pSupers = CWordItemBuilder::BuildMathWord("", "n times", false, styleSubs);
      SLaTexCmdArgValue arg1{ elcatItem, {0} }; arg1.uVal.pMathItem = pBase;
      CMathItem* pVBox = builder.BuildItem("\\overparen", style, 1.0f, { arg1 });
      CMathItem* pIndexed = CIndexedBuilder::BuildIndexed(style, 1.0f, pVBox, pSupers, nullptr);
      pHBox->AddItem(pIndexed);

      CMathItem* pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
      pHBox->AddItem(pComma);
      //\overbrace{ABCDE}^{n\,times}
      pBase = CWordItemBuilder::BuildMathWord("", "ABCDE", false, style);
      pSupers = CWordItemBuilder::BuildMathWord("", "n times", false, styleSubs);
      arg1.uVal.pMathItem = pBase;
      pVBox = builder.BuildItem("\\overbrace", style, 1.0f, { arg1 });
      pIndexed = CIndexedBuilder::BuildIndexed(style, 1.0f, pVBox, pSupers, nullptr);
      pHBox->AddItem(pIndexed);
      pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
      pHBox->AddItem(pComma);

      //\underbracket{ABCDE}_{n\,times}
      pBase = CWordItemBuilder::BuildMathWord("", "ABCDE", false, style);
      pSupers = CWordItemBuilder::BuildMathWord("", "n times", false, styleSubs);
      arg1.uVal.pMathItem = pBase;
      pVBox = builder.BuildItem("\\underbracket", style, 1.0f, { arg1 });
      pIndexed = CIndexedBuilder::BuildIndexed(style, 1.0f, pVBox, nullptr, pSupers);
      pHBox->AddItem(pIndexed);
      pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
      pHBox->AddItem(pComma);

      pHBox->Update();
      m_MainBox.AddBox(pHBox, 0, 330);
   }
   // $$\int_{a}^{b} x^2 \,dx, \int_{a}\limits^{b} x^2 \,dx,$$
   // $\int_{a}^{b} x^2 \,dx, \int_{a}\limits^{b} x^2 \,dx,$
   void BuildLargeOps_() {
      CMathStyle styleD(etsDisplay);
      CMathStyle styleT(etsText);
      //styles for indexed Numerator
      CMathStyle styleSuper(etsScript);
      CMathStyle styleSubs(etsScript);
      styleSubs.SetCramped(true);
      CHBoxItem* pHBox = new CHBoxItem(styleD);
      CLOpBuilder builder;
      //\int_{a}^{b} x^2 \,dx
      CMathItem* pBase = CWordItemBuilder::BuildMathWord("", "x", false, styleD);
      CMathItem* pSupers = CWordItemBuilder::BuildMathWord("", "2", true, styleSuper);
      CMathItem* pLopArg = CIndexedBuilder::BuildIndexed(styleD, 1.0f, pBase, pSupers, nullptr);
      SLaTexCmdArgValue arg1{ elcatLimits, {0} }; arg1.uVal.nVal= 0;
      CMathItem* pLop = builder.BuildItem("\\int", styleD, 1.0f, { arg1 });
      CMathItem* pLimBtm = CWordItemBuilder::BuildMathWord("", "a", false, styleSuper);
      CMathItem* pLimTop = CWordItemBuilder::BuildMathWord("", "b", false, styleSuper);

      CMathItem* pLopLims = CIndexedBuilder::BuildIndexed(styleD, 1.0f, pLop, pLimTop, pLimBtm);
      pHBox->AddItem(pLopLims);
      pHBox->AddItem(pLopArg);
 
      CMathItem* pGlue = CWordItemBuilder::BuildTeXSymbol("", "\\,", styleD);
      pHBox->AddItem(pGlue);
      CMathItem* pDX = CWordItemBuilder::BuildMathWord("", "dx", false, styleD);
      pHBox->AddItem(pDX);
      CMathItem* pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", styleD);
      pHBox->AddItem(pComma);
      //\int\limits_{a}^{b} x^2 \,dx
      pBase = CWordItemBuilder::BuildMathWord("", "x", false, styleD);
      pSupers = CWordItemBuilder::BuildMathWord("", "2", true, styleSuper);
      pLopArg = CIndexedBuilder::BuildIndexed(styleD, 1.0f, pBase, pSupers, nullptr);
      arg1.uVal.nVal = 1;
      pLop = builder.BuildItem("\\int", styleD, 1.0f, { arg1 });
      pLimBtm = CWordItemBuilder::BuildMathWord("", "a", false, styleSuper);
      pLimTop = CWordItemBuilder::BuildMathWord("", "b", false, styleSuper);

      pLopLims = CIndexedBuilder::BuildIndexed(styleD, 1.0f, pLop, pLimTop, pLimBtm);
      pHBox->AddItem(pLopLims);
      pHBox->AddItem(pLopArg);

      pGlue = CWordItemBuilder::BuildTeXSymbol("", "\\,", styleD);
      pHBox->AddItem(pGlue);
      pDX = CWordItemBuilder::BuildMathWord("", "dx", false, styleD);
      pHBox->AddItem(pDX);
      pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", styleD);
      pHBox->AddItem(pComma);
      pHBox->Update();
      m_MainBox.AddBox(pHBox, 0, 330);
      //Text Mode
      pHBox = new CHBoxItem(styleT);
      //\int_{a}^{b} x^2 \,dx
      pBase = CWordItemBuilder::BuildMathWord("", "x", false, styleT);
      pSupers = CWordItemBuilder::BuildMathWord("", "2", true, styleSuper);
      pLopArg = CIndexedBuilder::BuildIndexed(styleT, 1.0f, pBase, pSupers, nullptr);
      arg1.uVal.nVal = 0;
      pLop = builder.BuildItem("\\int", styleT, 1.0f, { arg1 });
      pLimBtm = CWordItemBuilder::BuildMathWord("", "a", false, styleSuper);
      pLimTop = CWordItemBuilder::BuildMathWord("", "b", false, styleSuper);

      pLopLims = CIndexedBuilder::BuildIndexed(styleT, 1.0f, pLop, pLimTop, pLimBtm);
      pHBox->AddItem(pLopLims);
      pHBox->AddItem(pLopArg);

      pGlue = CWordItemBuilder::BuildTeXSymbol("", "\\,", styleT);
      pHBox->AddItem(pGlue);
      pDX = CWordItemBuilder::BuildMathWord("", "dx", false, styleT);
      pHBox->AddItem(pDX);
      pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", styleT);
      pHBox->AddItem(pComma);

      //\int\limits_{a}^{b} x^2 \,dx
      pBase = CWordItemBuilder::BuildMathWord("", "x", false, styleT);
      pSupers = CWordItemBuilder::BuildMathWord("", "2", true, styleSuper);
      pLopArg = CIndexedBuilder::BuildIndexed(styleT, 1.0f, pBase, pSupers, nullptr);
      arg1.uVal.nVal = 1;
      pLop = builder.BuildItem("\\int", styleT, 1.0f, { arg1 });
      pLimBtm = CWordItemBuilder::BuildMathWord("", "a", false, styleSuper);
      pLimTop = CWordItemBuilder::BuildMathWord("", "b", false, styleSuper);

      pLopLims = CIndexedBuilder::BuildIndexed(styleT, 1.0f, pLop, pLimTop, pLimBtm);
      pHBox->AddItem(pLopLims);
      pHBox->AddItem(pLopArg);

      pGlue = CWordItemBuilder::BuildTeXSymbol("", "\\,", styleT);
      pHBox->AddItem(pGlue);
      pDX = CWordItemBuilder::BuildMathWord("", "dx", false, styleT);
      pHBox->AddItem(pDX);
      pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", styleT);
      pHBox->AddItem(pComma);

      pHBox->Update();
      m_MainBox.AddBox(pHBox, 0, 4530);
   }

   // \sqrt{\sqrt[\textit{ABC}]{\frac{\textit{\fontsize{##pt}{??pt}\selectfont x}_1^2}{2}}}
   void BuildRadicals_() {
      CMathStyle style(m_MainBox.GetStyle());
      const float aFontSize[] = { 9, 12 , 14, 18 , 21, 24, 30, 40, 50 };
      uint32_t nLeftEm = 0;
      CMathStyle styleNumerator(m_MainBox.GetStyle());
      styleNumerator.Decrease();
      CMathStyle styleDenom(styleNumerator);
      styleDenom.SetCramped(true);
      //styles for indexed Numerator
      CMathStyle styleSuper(m_MainBox.GetStyle());
      styleSuper.Decrease();
      if (styleSuper.Style() == etsText)
         styleSuper.Decrease();
      CMathStyle styleSubs(styleSuper);
      styleSubs.SetCramped(true);

      const PCSTR szBase = "x";
      for (float fSizePt : aFontSize) {
         //{base}_1^2
         CMathItem* pBase = CWordItemBuilder::BuildText("textit", szBase, styleNumerator, fSizePt / m_fFontSizePt);
         _ASSERT_RET(pBase, );
         CMathItem* pSuperS = CWordItemBuilder::BuildMathWord("", "2", true, styleSuper, 1.0f);
         _ASSERT_RET(pSuperS, );
         CMathItem* pSubS = CWordItemBuilder::BuildMathWord("", "1", true, styleSubs, 1.0f);
         _ASSERT_RET(pSubS, );
         CMathItem* pNum = CIndexedBuilder::BuildIndexed(styleNumerator, 1.0f, pBase, pSuperS, pSubS);
         //
         CMathItem* pDenom = CWordItemBuilder::BuildMathWord("", "2", true, styleDenom, 1.0f);
         _ASSERT_RET(pDenom, );
         CMathItem* pRadicand = CFractionBuilder::_BuildFraction(m_MainBox.GetStyle(), 1.0f, pNum, pDenom);
         //Degree/Index
         CMathItem* pRadDegree = CWordItemBuilder::BuildText("textit", "ABC", etsScriptScript, 1.0f);
         _ASSERT_RET(pRadDegree, );

         CMathItem* pRadical0 = CRadicalBuilder::_BuildRadical(m_MainBox.GetStyle(), 1.0f, pRadicand, pRadDegree);
         if (!pRadical0) {
            _ASSERT(0);
            delete pRadicand;
            delete pRadDegree;
            return;
         }
         CMathItem* pRadical = CRadicalBuilder::_BuildRadical(m_MainBox.GetStyle(), 1.0f, pRadical0);
         if (!pRadical) {
            _ASSERT(0);
            delete pRadicand;
            delete pRadDegree;
            return;
         }
         int32_t nNextY = 0;
         if (!m_MainBox.Box().IsEmpty())//keep baselines aligned
            nNextY = m_MainBox.Box().BaselineY() - pRadical->Box().Ascent();
         m_MainBox.AddBox(pRadical, nLeftEm, nNextY);
         nLeftEm += pRadical->Box().Width() + 833;
      }
      m_MainBox.NormalizeOrigin(0, 0);
   }
   // \ocirc{a},\hat{b},\tilde{c},\dot{d},\ddot{e},\bar{f},\check{g},\acute{h},\grave{x},\breve{y},\vec{z},...
   CHBoxItem* BuildAccentItems1_() {
      CMathStyle style(m_MainBox.GetStyle());
      const PCSTR aAccents[]{ "\\ocirc","\\hat","\\tilde","\\dot","\\ddot","\\bar","\\check","\\acute",
         "\\grave","\\breve","\\vec","\\threeunderdot","\\underleftarrow","\\underrightarrow"};
      const PCSTR aBase[]{ "a","b","c","d","e","f","g","h","x","y","z","x","x","x"};
      CHBoxItem* pHBox = new CHBoxItem(style);
      for(int nIdx=0; nIdx < (int)_countof(aAccents); ++nIdx) {
         //build base
         CMathItem* pBase = CWordItemBuilder::BuildMathWord("", aBase[nIdx], false, style);
         _ASSERT(pBase);
         CMathItem* pAccentItem = CAccentBuilder::BuildAccented(style, 1.0f, pBase, aAccents[nIdx]);
         _ASSERT(pAccentItem);
         pHBox->AddItem(pAccentItem);
         CMathItem* pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
         pHBox->AddItem(pComma);
      }
      pHBox->Update();
      return pHBox;
   }
   //\hat{\dot{x}},\tilde{\bar{A}},\acute{\grave{e}}
   CHBoxItem* BuildAccentItems2_() {
      CMathStyle style(m_MainBox.GetStyle());
      CHBoxItem* pHBox = new CHBoxItem(style);
      //\hat{\dot{x}}
      CMathItem* pBase = CWordItemBuilder::BuildMathWord("", "x", false, style);
      _ASSERT(pBase);
      CMathItem* pAccentItem = CAccentBuilder::BuildAccented(style, 1.0f, pBase, "\\dot");
      _ASSERT(pAccentItem);
      CMathItem* pAccentItem2= CAccentBuilder::BuildAccented(style, 1.0f, pAccentItem, "\\hat");
      _ASSERT(pAccentItem2);
      pHBox->AddItem(pAccentItem2);
      CMathItem* pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
      pHBox->AddItem(pComma);
      //\tilde{\bar{A}},
      pBase = CWordItemBuilder::BuildMathWord("", "A", false, style);
      _ASSERT(pBase);
      pAccentItem = CAccentBuilder::BuildAccented(style, 1.0f, pBase, "\\bar");
      _ASSERT(pAccentItem);
      pAccentItem2 = CAccentBuilder::BuildAccented(style, 1.0f, pAccentItem, "\\tilde");
      _ASSERT(pAccentItem2);
      pHBox->AddItem(pAccentItem2);
      pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
      pHBox->AddItem(pComma);
      //\acute{\grave{e}}
      pBase = CWordItemBuilder::BuildMathWord("", "e", false, style);
      _ASSERT(pBase);
      pAccentItem = CAccentBuilder::BuildAccented(style, 1.0f, pBase, "\\grave");
      _ASSERT(pAccentItem);
      pAccentItem2 = CAccentBuilder::BuildAccented(style, 1.0f, pAccentItem, "\\acute");
      _ASSERT(pAccentItem2);
      pHBox->AddItem(pAccentItem2);

      pHBox->Update();
      return pHBox;
   }
   CHBoxItem* BuildOverItems_() {
      CMathStyle style(m_MainBox.GetStyle());
      CUnderOverBuilder builder;
      CHBoxItem* pHBox = new CHBoxItem(style);
      //\widehat{ABC}
      CMathItem* pBase = CWordItemBuilder::BuildMathWord("", "ABC", false, style);
      _ASSERT(pBase);
      SLaTexCmdArgValue arg1{ elcatItem, {0} }; arg1.uVal.pMathItem = pBase;
      CMathItem* pVBox = builder.BuildItem("\\widehat", style, 1.0f, { arg1 });
      pHBox->AddItem(pVBox);
      CMathItem* pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
      pHBox->AddItem(pComma);
      //\widecheck{ABC}
      pBase = CWordItemBuilder::BuildMathWord("", "ABC", false, style);
      arg1.uVal.pMathItem = pBase;
      pVBox = builder.BuildItem("\\widecheck", style, 1.0f, { arg1 });
      pHBox->AddItem(pVBox);
      pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
      pHBox->AddItem(pComma);
      //\widetilde{ABC}
      pBase = CWordItemBuilder::BuildMathWord("", "ABC", false, style);
      arg1.uVal.pMathItem = pBase;
      pVBox = builder.BuildItem("\\widetilde", style, 1.0f, { arg1 });
      pHBox->AddItem(pVBox);
      pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
      pHBox->AddItem(pComma);
      //\overline{ABC}
      pBase = CWordItemBuilder::BuildMathWord("", "ABC", false, style);
      arg1.uVal.pMathItem = pBase;
      pVBox = builder.BuildItem("\\overline", style, 1.0f, { arg1 });
      pHBox->AddItem(pVBox);
      pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
      pHBox->AddItem(pComma);
      //\overbracket{ABC}
      pBase = CWordItemBuilder::BuildMathWord("", "ABC", false, style);
      arg1.uVal.pMathItem = pBase;
      pVBox = builder.BuildItem("\\overbracket", style, 1.0f, { arg1 });
      pHBox->AddItem(pVBox);
      pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
      pHBox->AddItem(pComma);
      //\overparen{ABC}
      pBase = CWordItemBuilder::BuildMathWord("", "ABC", false, style);
      arg1.uVal.pMathItem = pBase;
      pVBox = builder.BuildItem("\\overparen", style, 1.0f, { arg1 });
      pHBox->AddItem(pVBox);
      pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
      pHBox->AddItem(pComma);
      //\overbrace{ABC}
      pBase = CWordItemBuilder::BuildMathWord("", "ABC", false, style);
      arg1.uVal.pMathItem = pBase;
      pVBox = builder.BuildItem("\\overbrace", style, 1.0f, { arg1 });
      pHBox->AddItem(pVBox);
      pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
      pHBox->AddItem(pComma);
      //\obrbrak{ABC}
      pBase = CWordItemBuilder::BuildMathWord("", "ABC", false, style);
      arg1.uVal.pMathItem = pBase;
      pVBox = builder.BuildItem("\\obrbrak", style, 1.0f, { arg1 });
      pHBox->AddItem(pVBox);
      pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
      pHBox->AddItem(pComma);

      pHBox->Update();
      return pHBox;
   }
   CHBoxItem* BuildOverItems2_() {
      CMathStyle style(m_MainBox.GetStyle());
      CUnderOverBuilder builder;
      CHBoxItem* pHBox = new CHBoxItem(style);
      //\overleftarrow{ABCDE}
      CMathItem* pBase = CWordItemBuilder::BuildMathWord("", "ABCDE", false, style);
      _ASSERT(pBase);
      SLaTexCmdArgValue arg1{ elcatItem, {0} }; arg1.uVal.pMathItem = pBase;
      CMathItem* pVBox = builder.BuildItem("\\overleftarrow", style, 1.0f, { arg1 });
      pHBox->AddItem(pVBox);
      CMathItem* pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
      pHBox->AddItem(pComma);
      //\overrightarrow{ABCDE}
      pBase = CWordItemBuilder::BuildMathWord("", "ABCDE", false, style);
      arg1.uVal.pMathItem = pBase;
      pVBox = builder.BuildItem("\\overrightarrow", style, 1.0f, { arg1 });
      pHBox->AddItem(pVBox);
      pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
      pHBox->AddItem(pComma);
      //\overleftrightarrow{ABCDE}
      pBase = CWordItemBuilder::BuildMathWord("", "ABCDE", false, style);
      arg1.uVal.pMathItem = pBase;
      pVBox = builder.BuildItem("\\overleftrightarrow", style, 1.0f, { arg1 });
      pHBox->AddItem(pVBox);
      pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
      pHBox->AddItem(pComma);
      //\Overrightarrow{ABCDE}
      pBase = CWordItemBuilder::BuildMathWord("", "ABCDE", false, style);
      arg1.uVal.pMathItem = pBase;
      pVBox = builder.BuildItem("\\Overrightarrow", style, 1.0f, { arg1 });
      pHBox->AddItem(pVBox);
      pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
      pHBox->AddItem(pComma);

      pHBox->Update();
      return pHBox;
   }
   CHBoxItem* BuildUnderItems_() {
      CMathStyle style(m_MainBox.GetStyle());
      CUnderOverBuilder builder;
      CHBoxItem* pHBox = new CHBoxItem(style);
      //\utilde{ABC}
      CMathItem* pBase = CWordItemBuilder::BuildMathWord("", "ABC", false, style);
      _ASSERT(pBase);
      SLaTexCmdArgValue arg1{ elcatItem, {0} }; arg1.uVal.pMathItem = pBase;
      CMathItem* pVBox = builder.BuildItem("\\utilde", style, 1.0f, { arg1 });
      pHBox->AddItem(pVBox);
      CMathItem* pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
      pHBox->AddItem(pComma);
      //\underline{ABC}
      pBase = CWordItemBuilder::BuildMathWord("", "ABC", false, style);
      arg1.uVal.pMathItem = pBase;
      pVBox = builder.BuildItem("\\underline", style, 1.0f, { arg1 });
      pHBox->AddItem(pVBox);
      pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
      pHBox->AddItem(pComma);
      //\underbracket{ABCDE}
      pBase = CWordItemBuilder::BuildMathWord("", "ABCDE", false, style);
      arg1.uVal.pMathItem = pBase;
      pVBox = builder.BuildItem("\\underbracket", style, 1.0f, { arg1 });
      pHBox->AddItem(pVBox);
      pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
      pHBox->AddItem(pComma);
      //\underparen{ABCDE}
      pBase = CWordItemBuilder::BuildMathWord("", "ABCDE", false, style);
      arg1.uVal.pMathItem = pBase;
      pVBox = builder.BuildItem("\\underparen", style, 1.0f, { arg1 });
      pHBox->AddItem(pVBox);
      pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
      pHBox->AddItem(pComma);
      //\underbrace{ABCDE}
      pBase = CWordItemBuilder::BuildMathWord("", "ABCDE", false, style);
      arg1.uVal.pMathItem = pBase;
      pVBox = builder.BuildItem("\\underbrace", style, 1.0f, { arg1 });
      pHBox->AddItem(pVBox);
      pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
      pHBox->AddItem(pComma);
      //\ubrbrak{ABC}
      pBase = CWordItemBuilder::BuildMathWord("", "ABC", false, style);
      arg1.uVal.pMathItem = pBase;
      pVBox = builder.BuildItem("\\ubrbrak", style, 1.0f, { arg1 });
      pHBox->AddItem(pVBox);
      pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
      pHBox->AddItem(pComma);

      pHBox->Update();
      return pHBox;
   }
   void BuildAccentItems_() {
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
      m_MainBox.AddBox(BuildVBoxes1_(), 100, nY);
      //nY += nInterLine;
      //m_MainBox.AddBox(BuildAccentItems2_(), 100, nY);
   }
   void BuildOpenClose_() {
      CMathStyle style(m_MainBox.GetStyle());
      int32_t nY = 100, nInterLine = 3200;

      m_MainBox.AddBox(BuildDelimiters_("(", ")", "[", "]"), 100, nY);
      nY += nInterLine;
      m_MainBox.AddBox(BuildDelimiters_("\\{", "\\}","\\backslash", "/"), 100, nY);
      nY += nInterLine;
      m_MainBox.AddBox(BuildDelimiters_("\\langle", "\\rangle", "\\lAngle", "\\rAngle"), 100, nY);
      nY += nInterLine;
      m_MainBox.AddBox(BuildDelimiters_("\\uparrow", "\\downarrow", "\\Uparrow", "\\Downarrow"), 100, nY);
      nY += nInterLine;
      m_MainBox.AddBox(BuildDelimiters_("\\updownarrow", "\\Updownarrow", "\\|", "|"), 100, nY);
      nY += nInterLine;
      m_MainBox.AddBox(BuildDelimiters_("\\lceil", "\\rceil", "\\lfloor", "\\rfloor"), 100, nY);
      nY += nInterLine;
   }
   //_a^b,^a_b,\underset{a}{b},\overset{a}{b},
   // \substack{b\\a},\substack{a\\b},\substack{\displaystyle b\\\displaystyle a},
   // \substack{\displaystyle a\\\displaystyle b},
   // \stackrel{\scriptscriptstyle >}{<},\stackrel{\scriptscriptstyle ?}{=},
   // \stackunder[0em]{a}{b},\stackunder[0.1em]{a}{b},\stackunder[0.2em]{a}{b},\stackunder[0.3em]{a}{b},...
   CHBoxItem* BuildVBoxes1_() {
      CMathStyle style(m_MainBox.GetStyle());
      CHBoxItem* pHBox = new CHBoxItem(style);
      CMathStyle styleSuper(style);
      styleSuper.Decrease();
      if (styleSuper.Style() == etsText)
         styleSuper.Decrease();

      //_a^b
      CMathItem* pFirst = CWordItemBuilder::BuildMathWord("", "a", false, styleSuper);
      SLaTexCmdArgValue arg1{ elcatItem, {0} }; arg1.uVal.pMathItem = pFirst;
      CMathItem* pSecond = CWordItemBuilder::BuildMathWord("", "b", false, styleSuper);
      SLaTexCmdArgValue arg2{ elcatItem, {0} }; arg2.uVal.pMathItem = pSecond;
      CVBoxBuilder builder;
      CMathItem* pVBox = builder.BuildItem("_^", style, 1.0f, { arg1, arg2 });
      pHBox->AddItem(pVBox);
      CMathItem* pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
      pHBox->AddItem(pComma);
      //^a_b
      pFirst = CWordItemBuilder::BuildMathWord("", "a", false, styleSuper);
      arg1.uVal.pMathItem = pFirst;
      pSecond = CWordItemBuilder::BuildMathWord("", "b", false, styleSuper);
      arg2.uVal.pMathItem = pSecond;
      pVBox = builder.BuildItem("^_", style, 1.0f, { arg1, arg2 });
      pHBox->AddItem(pVBox);
      pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
      pHBox->AddItem(pComma);
      //underset{a}{b}
      pFirst = CWordItemBuilder::BuildMathWord("", "a", false, styleSuper);
      arg1.uVal.pMathItem = pFirst;
      pSecond = CWordItemBuilder::BuildMathWord("", "b", false, style);
      arg2.uVal.pMathItem = pSecond;
      pVBox = builder.BuildItem("\\underset", style, 1.0f, { arg1, arg2 });
      pHBox->AddItem(pVBox);
      pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
      pHBox->AddItem(pComma);
      //overset{a}{b}
      pFirst = CWordItemBuilder::BuildMathWord("", "a", false, styleSuper);
      arg1.uVal.pMathItem = pFirst;
      pSecond = CWordItemBuilder::BuildMathWord("", "b", false, style);
      arg2.uVal.pMathItem = pSecond;
      pVBox = builder.BuildItem("\\overset", style, 1.0f, { arg1, arg2 });
      pHBox->AddItem(pVBox);
      pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
      pHBox->AddItem(pComma);
      //substack{b\\a}
      pFirst = CWordItemBuilder::BuildMathWord("", "b", false, styleSuper);
      arg1.uVal.pMathItem = pFirst;
      pSecond = CWordItemBuilder::BuildMathWord("", "a", false, styleSuper);
      arg2.uVal.pMathItem = pSecond;
      pVBox = builder.BuildItem("\\substack", style, 1.0f, { arg1, arg2 });
      pHBox->AddItem(pVBox);
      pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
      pHBox->AddItem(pComma);
      //substack{a\\b}
      pFirst = CWordItemBuilder::BuildMathWord("", "a", false, styleSuper);
      arg1.uVal.pMathItem = pFirst;
      pSecond = CWordItemBuilder::BuildMathWord("", "b", false, styleSuper);
      arg2.uVal.pMathItem = pSecond;
      pVBox = builder.BuildItem("\\substack", style, 1.0f, { arg1, arg2 });
      pHBox->AddItem(pVBox);
      pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
      pHBox->AddItem(pComma);
      //substack{\displaystyle b\\\displaystyle a}
      pFirst = CWordItemBuilder::BuildMathWord("", "b", false, style);
      arg1.uVal.pMathItem = pFirst;
      pSecond = CWordItemBuilder::BuildMathWord("", "a", false, style);
      arg2.uVal.pMathItem = pSecond;
      pVBox = builder.BuildItem("\\substack", style, 1.0f, { arg1, arg2 });
      pHBox->AddItem(pVBox);
      pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
      pHBox->AddItem(pComma);
      //substack{\displaystyle a\\\displaystyle b}
      pFirst = CWordItemBuilder::BuildMathWord("", "a", false, style);
      arg1.uVal.pMathItem = pFirst;
      pSecond = CWordItemBuilder::BuildMathWord("", "b", false, style);
      arg2.uVal.pMathItem = pSecond;
      pVBox = builder.BuildItem("\\substack", style, 1.0f, { arg1, arg2 });
      pHBox->AddItem(pVBox);
      pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
      pHBox->AddItem(pComma);
      //\stackrel{\scriptscriptstyle >}{<}
      CMathStyle styleSS(styleSuper);
      styleSS.Decrease();
      pFirst = CWordItemBuilder::BuildTeXSymbol("", "\\greater", styleSS);
      arg1.uVal.pMathItem = pFirst;
      pSecond = CWordItemBuilder::BuildTeXSymbol("", "\\less", style);
      arg2.uVal.pMathItem = pSecond;
      pVBox = builder.BuildItem("\\stackrel", style, 1.0f, { arg1, arg2 });
      pHBox->AddItem(pVBox);
      pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
      pHBox->AddItem(pComma);
      //\stackrel{\scriptscriptstyle ?}{=}
      pFirst = CWordItemBuilder::BuildTeXSymbol("", "\\mathquestion", styleSS);
      arg1.uVal.pMathItem = pFirst;
      pSecond = CWordItemBuilder::BuildTeXSymbol("", "\\equal", style);
      arg2.uVal.pMathItem = pSecond;
      pVBox = builder.BuildItem("\\stackrel", style, 1.0f, { arg1, arg2 });
      pHBox->AddItem(pVBox);
      pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
      pHBox->AddItem(pComma);
      //stackunder[0em]{a}{b}
      SLaTexCmdArgValue arg0{ elcatDim, {0} }; arg0.uVal.nVal= 0;
      pFirst = CWordItemBuilder::BuildMathWord("", "a", false, style);
      arg1.uVal.pMathItem = pFirst;
      pSecond = CWordItemBuilder::BuildMathWord("", "b", false, style);
      arg2.uVal.pMathItem = pSecond;
      pVBox = builder.BuildItem("\\stackunder", style, 1.0f, { arg0, arg1, arg2 });
      pHBox->AddItem(pVBox);
      pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
      pHBox->AddItem(pComma);
      //stackunder[0.1em]{a}{b}
      arg0.uVal.nVal = 100;
      pFirst = CWordItemBuilder::BuildMathWord("", "a", false, style);
      arg1.uVal.pMathItem = pFirst;
      pSecond = CWordItemBuilder::BuildMathWord("", "b", false, style);
      arg2.uVal.pMathItem = pSecond;
      pVBox = builder.BuildItem("\\stackunder", style, 1.0f, { arg0, arg1, arg2 });
      pHBox->AddItem(pVBox);
      pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
      pHBox->AddItem(pComma);
      //stackunder[0.2em]{a}{b}
      arg0.uVal.nVal = 200;
      pFirst = CWordItemBuilder::BuildMathWord("", "a", false, style);
      arg1.uVal.pMathItem = pFirst;
      pSecond = CWordItemBuilder::BuildMathWord("", "b", false, style);
      arg2.uVal.pMathItem = pSecond;
      pVBox = builder.BuildItem("\\stackunder", style, 1.0f, { arg0, arg1, arg2 });
      pHBox->AddItem(pVBox);
      pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
      pHBox->AddItem(pComma);
      //stackunder[0.3em]{a}{b}
      arg0.uVal.nVal = 300;
      pFirst = CWordItemBuilder::BuildMathWord("", "a", false, style,1.0f);
      arg1.uVal.pMathItem = pFirst;
      pSecond = CWordItemBuilder::BuildMathWord("", "b", false, style,1.0f);
      arg2.uVal.pMathItem = pSecond;
      pVBox = builder.BuildItem("\\stackunder", style, 1.0f, { arg0, arg1, arg2 });
      pHBox->AddItem(pVBox);
      pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
      pHBox->AddItem(pComma);
      //binom{a}{b}
      pFirst = CWordItemBuilder::BuildMathWord("", "a", false, styleSuper, 1.0f);
      arg1.uVal.pMathItem = pFirst;
      pSecond = CWordItemBuilder::BuildMathWord("", "b", false, styleSuper, 1.0f);
      arg2.uVal.pMathItem = pSecond;
      pVBox = builder.BuildItem("\\binom", style, 1.0f, { arg1, arg2 });
      pHBox->AddItem(pVBox);
      pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
      pHBox->AddItem(pComma);


      pHBox->Update();
      return pHBox;
   }
   // \big(\Big(\bigg(\Bigg(
   CHBoxItem* BuildDelimiters_(PCSTR szDelim1, PCSTR szDelim2, PCSTR szDelim3, PCSTR szDelim4) {
      CMathStyle style(m_MainBox.GetStyle());
      CHBoxItem* pHBox = new CHBoxItem(style);
      string sText = string(szDelim1) + "\\big" + szDelim1 + "\\Big" + szDelim1 + "\\bigg" + szDelim1 + "\\Bigg" + szDelim1;
      sText += string("\\Bigg") + szDelim2 + "\\bigg" + szDelim2 + "\\Big" + szDelim2 + "\\big" + szDelim2 + szDelim2;
      PCSTR szNext;
      SLMMDelimiter lmmDelimiter;
      bool bOk = COpenCloseBuilder::ConsumeDelimiter(sText.c_str(), style, szNext, lmmDelimiter);
      _ASSERT_RET(bOk, nullptr);
      while (bOk) {
         CMathItem* pDelim = COpenCloseBuilder::BuildDelimiter(lmmDelimiter, style);
         _ASSERT_RET(pDelim, nullptr);
         pHBox->AddItem(pDelim);
         if (!*szNext)
            break;//done!
         bOk = COpenCloseBuilder::ConsumeDelimiter(szNext, style, szNext, lmmDelimiter);
         _ASSERT_RET(bOk, nullptr);
      }
      CMathItem* pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
      pHBox->AddItem(pComma);
      //same for 3/4
      sText = string(szDelim3) + "\\big" + szDelim3 + "\\Big" + szDelim3 + "\\bigg" + szDelim3 + "\\Bigg" + szDelim3;
      sText += string("\\Bigg") + szDelim4 + "\\bigg" + szDelim4 + "\\Big" + szDelim4 + "\\big" + szDelim4 + szDelim4;
      bOk = COpenCloseBuilder::ConsumeDelimiter(sText.c_str(), style, szNext, lmmDelimiter);
      _ASSERT_RET(bOk, nullptr);
      while (bOk) {
         CMathItem* pDelim = COpenCloseBuilder::BuildDelimiter(lmmDelimiter, style);
         _ASSERT_RET(pDelim, nullptr);
         pHBox->AddItem(pDelim);
         if (!*szNext)
            break;//done!
         bOk = COpenCloseBuilder::ConsumeDelimiter(szNext, style, szNext, lmmDelimiter);
         _ASSERT_RET(bOk, nullptr);
      }

      pHBox->Update();
      return pHBox;
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

