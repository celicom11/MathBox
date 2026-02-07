#include "stdafx.h"
#include "D2DRenderer.h"
#include "D2DFontManager.h"
#include "CfgReader.h"
#include "MathBoxHost.h"
#include "Utf8Utils.h"

// Custom HINST_THISCOMPONENT for module handle
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)

// --- Main Application Class ---
class DemoApp : public IFontProvider_ {
   HWND              m_hwnd{ nullptr };
   //MathBox object and resources/callbacks
   CD2DRenderer      m_d2dr;
   CD2DFontManager   m_D2DFontManager;
   CMathBoxHost      m_MathBox;

public:
   DemoApp() : m_d2dr(*this), m_MathBox(m_D2DFontManager, m_d2dr){}
   ~DemoApp() {
      m_MathBox.CleanUp();
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
      if(!Utf8Utils::_readFile2Utf8(sDocPath, sDocUtf8)) {
         wstring sErr = L"Could not open/read document file [" + sDocPath + L"]";
         MessageBox(nullptr, sErr.c_str(), L"Error", MB_OK | MB_ICONERROR);
         return E_FAIL;
      }
      //macros, test
      wstring wsMacrosPath = L"Macros.mth";
      string sMacrosFile = "Macros.mth";
      string sMacros;
      if (!Utf8Utils::_readFile2Utf8(wsMacrosPath, sMacros)) {
         wstring sErr = L"Could not open/read document file [" + wsMacrosPath + L"]";
         MessageBox(nullptr, sErr.c_str(), L"Error", MB_OK | MB_ICONERROR);
         return E_FAIL;
      }
      // Apply cfg:
      int32_t nMaxWidthFDU{ 0 };
      cfgr.GetNVal(L"MaxWidthFDU", nMaxWidthFDU);
      m_MathBox.SetMaxWidth(nMaxWidthFDU);

      uint32_t clrText{ 0x004F00 };
      cfgr.GetHVal(L"ColorText", clrText);
      m_MathBox.SetClrText(clrText);
      m_d2dr.SetTextColor(clrText);

      uint32_t clrBkg{ 0xFFFFFF };
      cfgr.GetHVal(L"ColorBkg", clrBkg);
      m_MathBox.SetClrBkg(clrBkg);

      uint32_t clrSel{ 0xFF4FFF };
      cfgr.GetHVal(L"ColorSelect", clrSel);
      m_MathBox.SetClrSel(clrBkg);

      float fFontSizePts{ 24.0f };
      cfgr.GetFVal(L"FontSizePts", fFontSizePts);
      m_MathBox.SetFontSizePts(fFontSizePts);


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
      //convert to utf8
      wstring wsFontsDir(wszDir);
      wsFontsDir += L"\\LatinModernFonts\\";
      hr = m_D2DFontManager.Init(wsFontsDir, m_d2dr.DWFactory());
      CHECK_HR(hr);
      if (!m_MathBox.LoadMathBoxLib()) {
         wstring sErr = L"Could not load MathBoxLib";
         MessageBox(nullptr, sErr.c_str(), L"Error", MB_OK | MB_ICONERROR);
         return E_FAIL;
      }
      bool bOk;
      //add macros
      bOk = m_MathBox.AddMacros(sMacros, sMacrosFile);
      //parse document
      if(bOk)
         bOk = m_MathBox.Parse(sDocUtf8);
      _ASSERT(bOk);
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
 //IFontProvider_
   uint32_t FontCount() const override { return m_D2DFontManager.FontCount(); }
   IDWriteFontFace* GetDWFont(int32_t nIdx) const override {
      return m_D2DFontManager.GetDWFont(nIdx); 
   }
   float GetFontSizePts() const override { return m_MathBox.FontSizePts(); }

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
      m_d2dr.EraseBkgnd(m_MathBox.ClrBkg());
      m_MathBox.Draw(10.0f, 10.0f);
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
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
   _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);// | _CRTDBG_CHECK_ALWAYS_DF |_CRTDBG_DELAY_FREE_MEM_DF);
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

