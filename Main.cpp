#include "stdafx.h"
#include "MathBox\RadicalBuilder.h"
#include "MathBox\FractionBuilder.h"
#include "MathBox\IndexedBuilder.h"
#include "MathBox\LMFontManager.h"
#include "MathBox\WordItemBuilder.h"
#include "MathBox\HBoxItem.h"
#include "MathBox\AccentBuilder.h"

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
            //BuildIndexed_("f");
            //BuildTexts_();
            //BuildMathFonts_();
            BuildAccentItems_();
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
   // build various $$Text\ 0+1+2+3$$
   void BuildTexts_() {
      CMathStyle style(m_MainBox.GetStyle());
      const STexGlue glueSpace{ 0,0,0,0,833,833 };
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
   // build various $$3x^2 \in R \subset Q$$
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
   // build $$\textit{\fontsize{##pt}{##pt}\selectfont {base}}_1^2$$
   void BuildIndexed_(PCSTR szBase) {
      CMathStyle style(m_MainBox.GetStyle());
      const float aFontSize[] = { 9, 12 , 14, 18 , 21, 24, 30, 40, 50};
      uint32_t nLeftEm = 0;
      //styles for indexed Numerator
      CMathStyle styleSuper(style);
      styleSuper.Decrease();
      if(styleSuper.Style() == etsText)
         styleSuper.Decrease();
      CMathStyle styleSubs(styleSuper);
      styleSubs.SetCramped(true);

      for (float fSizePt : aFontSize) {
         CMathItem* pBase = CWordItemBuilder::BuildText("textit", szBase, style, fSizePt / m_fFontSizePt);
         _ASSERT_RET(pBase, );
         CMathItem* pSuperS = CWordItemBuilder::BuildText("","2", styleSuper, 1.0f);
         _ASSERT_RET(pSuperS, );
         CMathItem* pSubS = CWordItemBuilder::BuildText("", "1", styleSubs, 1.0f);
         _ASSERT_RET(pSubS, );
         CMathItem* pIndexed = CIndexedBuilder::BuildIndexed(style, 1.0f, pBase, pSuperS, pSubS);
         int32_t nNextY = 0;
         if (!m_MainBox.Box().IsEmpty())//keep baselines aligned
            nNextY = m_MainBox.Box().BaselineY() - pIndexed->Box().Ascent();
         m_MainBox.AddBox(pIndexed, nLeftEm, nNextY);
         nLeftEm += pIndexed->Box().Width() + 1600;
      }
      m_MainBox.NormalizeOrigin(0, 0);
   }
   // builds \sqrt{\sqrt[\textit{ABC}]{\frac{\textit{\fontsize{##pt}{??pt}\selectfont x}_1^2}{2}}}
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

      CMathStyle styleDegree(etsScriptScript);
      const PCSTR szBase = "x";
      for (float fSizePt : aFontSize) {
         //{base}_1^2
         CMathItem* pBase = CWordItemBuilder::BuildText("\\textit", szBase, styleNumerator, fSizePt / m_fFontSizePt);
         _ASSERT_RET(pBase, );
         CMathItem* pSuperS = CWordItemBuilder::BuildText("", "2", styleSuper, 1.0f);
         _ASSERT_RET(pSuperS, );
         CMathItem* pSubS = CWordItemBuilder::BuildText("", "1", styleSubs, 1.0f);
         _ASSERT_RET(pSubS, );
         CMathItem* pNum = CIndexedBuilder::BuildIndexed(styleNumerator, 1.0f, pBase, pSuperS, pSubS);
         //
         CMathItem* pDenom = CWordItemBuilder::BuildText("", "2", styleDenom, 1.0f);
         _ASSERT_RET(pDenom, );
         CMathItem* pRadicand = CFractionBuilder::BuildFraction(m_MainBox.GetStyle(), 1.0f, pNum, pDenom);
         //Degree/Index
         CMathItem* pRadDegree = CWordItemBuilder::BuildText("\\textit", "ABC", styleDegree, 1.0f);
         _ASSERT_RET(pRadDegree, );

         CMathItem* pRadical0 = CRadicalBuilder::BuildRadical(m_MainBox.GetStyle(), 1.0f, pRadicand, pRadDegree);
         if (!pRadical0) {
            _ASSERT(0);
            delete pRadicand;
            delete pRadDegree;
            return;
         }
         CMathItem* pRadical = CRadicalBuilder::BuildRadical(m_MainBox.GetStyle(), 1.0f, pRadical0);
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
   // build \ocirc{a},\hat{b},\tilde{c},\dot{d},\ddot{e},\bar{f},\check{g},\acute{h},\grave{x},\breve{y},\vec{z},...
   CHBoxItem* BuildAccentItems1_() {
      CMathStyle style(m_MainBox.GetStyle());
      const PCSTR aAccents[]{ "\\ocirc","\\hat","\\tilde","\\dot","\\ddot","\\bar","\\check","\\acute",
         "\\grave","\\breve","\\vec","\\threeunderdot","\\underleftarrow","\\underrightarrow"};
      const PCSTR aBase[]{ "a","b","c","d","e","f","g","h","x","y","z","x","x","x"};
      CHBoxItem* pHBox = new CHBoxItem(style);
      for(size_t i=0; i<_countof(aAccents); ++i) {
         //build base
         CMathItem* pBase = CWordItemBuilder::BuildMathWord("", aBase[i], false, style);
         _ASSERT(pBase);
         CMathItem* pAccentItem = CAccentBuilder::BuildAccented(style, 1.0f, pBase, aAccents[i]);
         _ASSERT(pAccentItem);
         pHBox->AddItem(pAccentItem);
         CMathItem* pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
         pHBox->AddItem(pComma);
      }
      pHBox->Update();
      return pHBox;
   }
   CHBoxItem* BuildAccentItems2_() {
      CMathStyle style(m_MainBox.GetStyle());
      const PCSTR aAccents[]{ 
         "\\ocirc","\\hat","\\tilde","\\dot","\\ddot","\\bar","\\check","\\acute",
         "\\grave","\\breve","\\vec","\\threeunderdot","\\underleftarrow","\\underrightarrow" };
      const PCSTR aBase[]{ "a","b","c","d","e","f","g","h","x","y","z","x","x","x" };
      CHBoxItem* pHBox = new CHBoxItem(style);
      for (size_t i = 0; i < _countof(aAccents); ++i) {
         //build base
         CMathItem* pBase = CWordItemBuilder::BuildMathWord("", aBase[i], false, style);
         _ASSERT(pBase);
         CMathItem* pAccentItem = CAccentBuilder::BuildAccented(style, 1.0f, pBase, aAccents[i]);
         _ASSERT(pAccentItem);
         CMathItem* pAccentItem2= CAccentBuilder::BuildAccented(style, 1.0f, pAccentItem, aAccents[i?i-1:11]);
         _ASSERT(pAccentItem2);
         pHBox->AddItem(pAccentItem2);
         CMathItem* pComma = CWordItemBuilder::BuildTeXSymbol("", "\\mathcomma", style);
         pHBox->AddItem(pComma);
      }
      pHBox->Update();
      return pHBox;
   }
   void BuildAccentItems_() {
      CMathStyle style(m_MainBox.GetStyle());
      int32_t nY = 100, nInterLine = 1750;
      m_MainBox.AddBox(BuildAccentItems1_(), 100, nY);
      nY += nInterLine;
      m_MainBox.AddBox(BuildAccentItems2_(), 100, nY);
      //}
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

