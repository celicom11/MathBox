#pragma once
#include "..\..\MathBoxLib\LibCode\MathBox_CAPI.h"
#include "MathBoxHostDefs.h"


//internal
struct IFontProvider_ {
   virtual ~IFontProvider_() {};
   virtual uint32_t FontCount() const = 0;
   virtual IDWriteFontFace* GetDWFont(int32_t nIdx) const = 0;
   virtual float GetFontSizePts() const = 0;
};

class CD2DRenderer {
//DATA
   uint32_t                   m_argbText{ 0 }; //and lines == foreground color
   ID2D1Factory*              m_pD2DFactory{nullptr};
   IDWriteFactory*            m_pDWriteFactory{ nullptr };
   ID2D1HwndRenderTarget*     m_pRenderTarget{ nullptr };
   ID2D1SolidColorBrush*      m_pSolidBrush{ nullptr };
   IFontProvider_&            m_FontProvider;
   vector<ID2D1StrokeStyle*>  m_vStrokeStyles;
public:
//CTOR/DTOR/INIT
   CD2DRenderer(IFontProvider_& pFontProvider) : m_FontProvider(pFontProvider) {};
   ~CD2DRenderer() {
      for (ID2D1StrokeStyle* pSStyle : m_vStrokeStyles) {
         SafeRelease(&pSStyle);
      }
      SafeRelease(&m_pD2DFactory);
      SafeRelease(&m_pDWriteFactory);
      SafeRelease(&m_pRenderTarget);
      SafeRelease(&m_pSolidBrush);
   }
   HRESULT Initialize(HWND hwnd);
//ATTS
   IDWriteFactory* DWFactory() const { return m_pDWriteFactory; }
   void SetTextColor(uint32_t nARGB) { m_argbText = nARGB; }
//METHODS
   void EraseBkgnd(uint32_t nARGB) {
      nARGB &= 0x00FFFFFF; //want rgb only
      if(m_pRenderTarget && nARGB)
         m_pRenderTarget->Clear(D2D1::ColorF(nARGB));
   }
   void OnSize(LPARAM lParam) {
      D2D1_SIZE_U size = D2D1::SizeU(LOWORD(lParam), HIWORD(lParam));
      if (m_pRenderTarget)
         m_pRenderTarget->Resize(size);
   }
   void BeginDraw() {
      if(m_pRenderTarget)
         m_pRenderTarget->BeginDraw();
   }
   HRESULT EndDraw() {
      HRESULT hr = S_OK;
      if(m_pRenderTarget)
         hr = m_pRenderTarget->EndDraw();
      return hr;
   }
   void SetTransform(const D2D1_MATRIX_3X2_F& mat) {
      if (m_pRenderTarget)
         m_pRenderTarget->SetTransform(mat);
   }
   void DrawLine(float x1, float y1, float x2, float y2, EnumLineStyles style, float width, uint32_t argb);
   void DrawRect(float left, float top, float right, float bottom, EnumLineStyles style, 
                  float width, uint32_t argb);
   void FillRect(float left, float top, float right, float bottom, uint32_t argb);
   void DrawGlyphRun(int32_t font_idx, uint32_t count, const uint16_t* indices,
                     float base_x, float base_y, float scale, uint32_t argb);

};