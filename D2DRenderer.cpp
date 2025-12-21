#include "stdafx.h"
#include "D2DRenderer.h"
#include "MathBox\LMMConsts.h"

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
      D2D1::HwndRenderTargetProperties(hwnd, size), &m_pRenderTarget);
   CHECK_HR(hr);
   m_pRenderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);

   hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &m_pSolidBrush);
   CHECK_HR(hr);
   //add stroke styles
   //elsDash,elsDot,elsDashDot
   m_vStrokeStyles.resize(3, nullptr);
   static const float _aDashes[] = { 4.0f, 2.0f };
   ID2D1StrokeStyle* pStrokeStyle = nullptr;
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
void CD2DRenderer::DrawLine(SPointF pt1, SPointF pt2, EnumLineStyles eStyle, float fWidth, uint32_t nARGB) {
   m_pSolidBrush->SetColor(D2D1::ColorF(nARGB ? nARGB : m_argbText));
   ID2D1StrokeStyle* pSStyle =
      eStyle > elsSolid && eStyle <= elsDashDot ? m_vStrokeStyles[(int)eStyle - 1] : nullptr;
   m_pRenderTarget->DrawLine({ pt1.fX,pt1.fY }, { pt2.fX, pt2.fY }, m_pSolidBrush, fWidth, pSStyle);
}
void CD2DRenderer::DrawRect(SRectF rcf, EnumLineStyles eStyle, float fWidth, uint32_t nARGB) {
   m_pSolidBrush->SetColor(D2D1::ColorF(nARGB ? nARGB : m_argbText));
   ID2D1StrokeStyle* pSStyle =
      eStyle > elsSolid && eStyle <= elsDashDot ? m_vStrokeStyles[(int)eStyle - 1] : nullptr;
   D2D1_RECT_F rcd2d{ rcf.fLeft,rcf.fTop,rcf.fRight,rcf.fBottom };
   m_pRenderTarget->DrawRectangle(rcd2d, m_pSolidBrush, fWidth, pSStyle);
}
void CD2DRenderer::FillRect(SRectF rcf, uint32_t nARGB) {
   m_pSolidBrush->SetColor(D2D1::ColorF(nARGB ? nARGB : m_argbText));
   D2D1_RECT_F rcd2d{ rcf.fLeft,rcf.fTop,rcf.fRight,rcf.fBottom };
   m_pRenderTarget->FillRectangle(rcd2d, m_pSolidBrush);
}
void CD2DRenderer::DrawGlyphRun(int32_t nFontIdx, uint32_t nCount, const uint16_t* pIndices,
                                SPointF ptfBaseOrigin, float fScale, uint32_t nARGB) {
   _ASSERT_RET(nCount && pIndices, );
   IDWriteFontFace* pFontFace = m_FontProvider.GetDWFont(nFontIdx);
   _ASSERT_RET(pFontFace, );
   m_pSolidBrush->SetColor(D2D1::ColorF(nARGB ? nARGB : m_argbText));

   DWRITE_GLYPH_RUN glyphRun = {};
   glyphRun.fontFace = pFontFace;
   glyphRun.fontEmSize = PTS2DIPS(m_FontProvider.GetFontSizePts() * fScale);
   glyphRun.glyphCount = nCount;
   glyphRun.glyphIndices = pIndices;
   glyphRun.glyphAdvances = nullptr; //use font advances
   glyphRun.glyphOffsets = nullptr; //offsets, not used
   glyphRun.isSideways = FALSE;
   glyphRun.bidiLevel = 0;
   m_pRenderTarget->DrawGlyphRun({ ptfBaseOrigin.fX,ptfBaseOrigin.fY}, &glyphRun, m_pSolidBrush);
}
