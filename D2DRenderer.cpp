#include "stdafx.h"
#include "D2DRenderer.h"

HRESULT CD2DRenderer::Initialize(HWND hwnd) {
   HRESULT hr = ::D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);
   CHECK_HR(hr);
   hr = ::DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), 
                              reinterpret_cast<IUnknown**>(&m_pDWriteFactory));
   CHECK_HR(hr);
   //WCHAR wszDir[MAX_PATH] = { 0 };
   //GetCurrentDirectoryW(_countof(wszDir), wszDir);
   //hr = g_LMFManager.Init(wszDir, pDWriteFactory);
   //CHECK_HR(hr);
   RECT rc;
   GetClientRect(hwnd, &rc);
   D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
   hr = m_pD2DFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(),
                                              D2D1::HwndRenderTargetProperties(hwnd, size), 
                                              &m_pRenderTarget);
   CHECK_HR(hr);
   m_pRenderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);

   hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &m_pSolidBrush);
   CHECK_HR(hr);
   //add stroke styles
   //elsDash,elsDot,elsDashDot
   m_vStrokeStyles.resize(3, nullptr);
   static const float _aDashes[] = { 4.0f, 2.0f };
   D2D1_STROKE_STYLE_PROPERTIES dssp;
   dssp = D2D1::StrokeStyleProperties(
      D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT,
      D2D1_CAP_STYLE_FLAT, D2D1_LINE_JOIN_MITER,
      10.0f, D2D1_DASH_STYLE_CUSTOM, 0.0f);
   hr = m_pD2DFactory->CreateStrokeStyle(dssp, _aDashes, _countof(_aDashes), &m_vStrokeStyles[0]);
   CHECK_HR(hr);
   dssp.dashStyle = D2D1_DASH_STYLE_DOT;
   hr = m_pD2DFactory->CreateStrokeStyle(dssp, nullptr, 0, &m_vStrokeStyles[1]);
   CHECK_HR(hr);
   dssp.dashStyle = D2D1_DASH_STYLE_DASH_DOT;
   hr = m_pD2DFactory->CreateStrokeStyle(dssp, nullptr, 0, &m_vStrokeStyles[2]);
   CHECK_HR(hr);

   return S_OK;
}
void CD2DRenderer::DrawLine(float x1, float y1, float x2, float y2, EnumLineStyles style, float width, 
                            uint32_t argb) {
   m_pSolidBrush->SetColor(D2D1::ColorF(argb ? argb : m_argbText));
   ID2D1StrokeStyle* pSStyle =
      style > elsSolid && style <= elsDashDot ? m_vStrokeStyles[style - 1] : nullptr;
   m_pRenderTarget->DrawLine({x1,y1}, { x2, y2 }, m_pSolidBrush, width, pSStyle);
}
void CD2DRenderer::DrawRect(float left, float top, float right, float bottom, EnumLineStyles style,
                            float width, uint32_t argb) {
   m_pSolidBrush->SetColor(D2D1::ColorF(argb ? argb : m_argbText));
   ID2D1StrokeStyle* pSStyle =
      style > elsSolid && style <= elsDashDot ? m_vStrokeStyles[style - 1] : nullptr;
   D2D1_RECT_F rcd2d{ left, top, right,bottom};
   m_pRenderTarget->DrawRectangle(rcd2d, m_pSolidBrush, width, pSStyle);
}
void CD2DRenderer::FillRect(float left, float top, float right, float bottom, uint32_t argb) {
   m_pSolidBrush->SetColor(D2D1::ColorF(argb ? argb : m_argbText));
   D2D1_RECT_F rcd2d{ left, top, right,bottom };
   m_pRenderTarget->FillRectangle(rcd2d, m_pSolidBrush);
}
void CD2DRenderer::DrawGlyphRun(int32_t font_idx, uint32_t count, const uint16_t* indices,
                                 float base_x, float base_y, float scale, uint32_t argb) {
   _ASSERT_RET(count && indices, );
   IDWriteFontFace* pFontFace = m_FontProvider.GetDWFont(font_idx);
   _ASSERT_RET(pFontFace, );
   m_pSolidBrush->SetColor(D2D1::ColorF(argb ? argb : m_argbText));

   DWRITE_GLYPH_RUN glyphRun = {};
   glyphRun.fontFace = pFontFace;
   glyphRun.fontEmSize = PTS2DIPS(m_FontProvider.GetFontSizePts() * scale);
   glyphRun.glyphCount = count;
   glyphRun.glyphIndices = indices;
   glyphRun.glyphAdvances = nullptr; //use font advances
   glyphRun.glyphOffsets = nullptr; //offsets, not used
   glyphRun.isSideways = FALSE;
   glyphRun.bidiLevel = 0;
   m_pRenderTarget->DrawGlyphRun({ base_x,base_y }, &glyphRun, m_pSolidBrush);
}
