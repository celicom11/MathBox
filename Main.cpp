#include "stdafx.h"
#include "MathBox\WordItem.h"
#include "MathBox\ContainerItem.h"
#include "MathBox\RadicalBuilder.h"
#include "MathBox\FractionBuilder.h"
#include "MathBox\IndexedBuilder.h"
#include "MathBox\LMFontManager.h"

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
   float               m_fFontSizePt{ 12.0f }; //document font size, in points
   HWND                m_hwnd;
   D2DResources        m_d2d;
   CContainerItem      m_MainBox;//TODO: HBOX
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
            BuildMainBox_();
            //BuildMainBoxTest_();
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

   // The core logic to render the growing radical
   void BuildMainBoxTest_() {
      HRESULT hRes;

      CMathStyle style(m_MainBox.GetStyle());
      const float aFontSize[] = { 9, 12 , 14, 18 , 21, 24, 30, 40, 50};
      //const UINT32 uRad1 = 0x01D453; //'𝘧' (MATHEMATICAL ITALIC SMALL F)
      //const UINT32 uRad2 = L'P';
      uint32_t nLeftEm = 0;
      //styles for indexed Numerator
      CMathStyle styleSuper(style);
      styleSuper.Decrease();
      if(styleSuper.Style() == etsText)
         styleSuper.Decrease();
      CMathStyle styleSubs(styleSuper);
      styleSubs.SetCramped(true);

      for (float fSizePt : aFontSize) {
         // build \sqrt\sqrt[ABC]{\frac{x_1^2}{2}}
         //x_1^2
         CWordItem* pBase = new CWordItem(3, style, eacWORD, fSizePt / m_fFontSizePt);
         //hRes = pX->SetText({ 0x1D465 }); //\mathit{x}
         hRes = pBase->SetText({ L'f'}); //\mathit{f}
         if (FAILED(hRes))
            return;//ERROR
         CWordItem* pSuperS = new CWordItem(0, styleSuper, eacWORD);
         hRes = pSuperS->SetText({ (UINT32)L'2' });
         if (FAILED(hRes))
            return;//ERROR
         CWordItem* pSubS = new CWordItem(0, styleSubs, eacWORD);
         hRes = pSubS->SetText({ (UINT32)L'1'});
         if (FAILED(hRes))
            return;//ERROR
         CMathItem* pIndexed = CIndexedBuilder::BuildIndexed(style, 1.0f, pBase, pSuperS, pSubS);
         int32_t nNextY = 0;
         if (!m_MainBox.Box().IsEmpty())//keep baselines aligned
            nNextY = m_MainBox.Box().BaselineY() - pIndexed->Box().Ascent();
         m_MainBox.AddBox(pIndexed, nLeftEm, nNextY);
         nLeftEm += pIndexed->Box().Width() + 1600;
      }
      m_MainBox.NormalizeOrigin(0, 0);
   }
   void BuildMainBox_() {
      HRESULT hRes;
      CRadicalBuilder radicalBuilder;

      CMathStyle style(m_MainBox.GetStyle());
      const float aFontSize[] = { 9, 12 , 14, 18 , 21, 24, 30, 40, 50 };
      //const UINT32 uRad1 = 0x01D453; //'𝘧' (MATHEMATICAL ITALIC SMALL F)
      //const UINT32 uRad2 = L'P';
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
      for (float fSizePt : aFontSize) {
         // build \sqrt\sqrt[ABC]{\frac{x_1^2}{2}}
         //x_1^2
         CWordItem* pX = new CWordItem(3, styleNumerator, eacWORD, fSizePt / m_fFontSizePt);
         //hRes = pX->SetText({ 0x1D465 }); //\mathit{x}
         hRes = pX->SetText({ L'x'}); //\mathit{f}
         if (FAILED(hRes))
            return;//ERROR
         CWordItem* pSuperS = new CWordItem(0, styleSuper, eacWORD);
         hRes = pSuperS->SetText({ (UINT32)L'2' });
         if (FAILED(hRes))
            return;//ERROR
         CWordItem* pSubS = new CWordItem(0, styleSubs, eacWORD);
         hRes = pSubS->SetText({ (UINT32)L'1' });
         if (FAILED(hRes))
            return;//ERROR
         CMathItem* pNum = CIndexedBuilder::BuildIndexed(styleNumerator, 1.0f, pX, pSuperS, pSubS);
         //
         CWordItem* pDenom = new CWordItem(0, styleDenom, eacUNK);
         hRes = pDenom->SetText({ (UINT32)L'2' });
         CMathItem* pRadicand = CFractionBuilder::BuildFraction(m_MainBox.GetStyle(), 1.0f, pNum, pDenom);
         //Degree/Index
         CWordItem* pRadDegree = new CWordItem(3, styleDegree, eacUNK);
         //mathit ABC
         hRes = pRadDegree->SetText(L"ABC");
         if (FAILED(hRes))
            return;//ERROR

         CMathItem* pRadical0 = radicalBuilder.BuildRadical(m_MainBox.GetStyle(), 1.0f, pRadicand, pRadDegree);
         if (!pRadical0) {
            _ASSERT(0);
            delete pRadicand;
            delete pRadDegree;
            return;
         }
         CMathItem* pRadical = radicalBuilder.BuildRadical(m_MainBox.GetStyle(), 1.0f, pRadical0);
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
         nLeftEm += pRadical->Box().Width() + 1090;
      }
      m_MainBox.NormalizeOrigin(0, 0);
   }
};

// --- WinMain Entry Point ---

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

