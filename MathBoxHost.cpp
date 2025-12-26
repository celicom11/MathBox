#include "stdafx.h"
#include "MathBoxHost.h"
#include "D2DFontManager.h"
#include "D2DRenderer.h"

namespace {
   CD2DFontManager* _pFontManager{ nullptr };
   CD2DRenderer*     _pD2DR{ nullptr };
}
CMathBoxHost::CMathBoxHost(CD2DFontManager& d2dFontManager, CD2DRenderer& d2dr) {
   _pFontManager = &d2dFontManager;
   _pD2DR = &d2dr;
   m_DocParams.size_bytes = sizeof(MB_DocParams);
   //D2DFontManager shim
   m_DocParams.font_mgr.size_bytes = sizeof(MBI_FontManager);
   m_DocParams.font_mgr.fontCount = _fontCount;
   m_DocParams.font_mgr.getFontIndices = _getFontIndices;
   m_DocParams.font_mgr.getGlyphRunMetrics = _getGlyphRunMetrics;
   //DocRenderer shim
   m_DocRenderer.size_bytes = sizeof(MBI_DocRenderer);
   m_DocRenderer.drawLine = _drawLine;
   m_DocRenderer.drawRect = _drawRect;
   m_DocRenderer.fillRect = _fillRect;
   m_DocRenderer.drawGlyphRun = _drawGlyphRun;
}
bool CMathBoxHost::LoadMathBoxLib() {
   _ASSERT_RET(!m_pMBAPI && !m_hMathBoxLib && !m_engine, false);//snbh!
   m_hMathBoxLib = ::LoadLibraryW(L"MathBoxLib.dll");
   _ASSERT_RET(m_hMathBoxLib, false);
   auto pfnMB_GetApi = (const MBI_API * (*)())GetProcAddress(m_hMathBoxLib, "MB_GetApi");
   _ASSERT_RET(pfnMB_GetApi, false);
   m_pMBAPI = pfnMB_GetApi();
   _ASSERT_RET(m_pMBAPI && m_pMBAPI->abi_version >= 1, false);
   MB_RET ret = m_pMBAPI->createEngine(&m_DocParams, &m_engine);
   if (!m_engine || ret != MBOK) {
      //todo: set error
      return false;
   }

   return true;
}
void CMathBoxHost::CleanUp() {
   if (m_pMBAPI) {
      if (m_pMBItem) {
         m_pMBAPI->destroyMathItem(m_pMBItem);
         m_pMBItem = nullptr;
      }
      if (m_engine) {
         m_pMBAPI->destroyEngine(m_engine);
         m_engine = nullptr;
      }
      m_pMBAPI = nullptr;
   }
   if (m_hMathBoxLib) {
      ::FreeModule(m_hMathBoxLib);
      m_hMathBoxLib = NULL;
   }
}
bool CMathBoxHost::Parse(const string& sTeX) {
   _ASSERT_RET(!!m_engine, false);
   if (m_pMBItem) {
      m_pMBAPI->destroyMathItem(m_pMBItem);
      m_pMBItem = nullptr;
   }
   MB_RET ret = m_pMBAPI->parseLatex(m_engine, sTeX.c_str(), &m_pMBItem);
   if (ret != MBOK) {
      //todo: set error
      return false;
   }
   return true;
}
void CMathBoxHost::Draw(float fX, float fY) {
   if (!!m_pMBAPI && !!m_pMBItem)
      m_pMBAPI->mathItemDraw(m_pMBItem, fX, fY, &m_DocRenderer);
}
//C-API/callbacks
uint32_t CMathBoxHost::_fontCount() {
   return _pFontManager->FontCount();
}
bool CMathBoxHost::_getFontIndices(int32_t font_idx, uint32_t count, const uint32_t* unicodes, OUT uint16_t* out_indices) {
   return _pFontManager->GetFontIndices(font_idx, count, unicodes, out_indices);
}
bool CMathBoxHost::_getGlyphRunMetrics(int32_t font_idx, uint32_t count,
                                          const uint16_t* indices, OUT MB_GlyphMetrics* out_glyph_metrics, 
                                          OUT MB_Bounds* out_bounds) {
   vector<SGlyphMetrics> vGlyphMetrics(count);
   SBounds bounds;
   if (!_pFontManager->GetGlyphRunMetrics(font_idx, count, indices, vGlyphMetrics.data(), bounds))
      return false;
   //copy output data
   if(out_bounds) {
      out_bounds->nMinX = bounds.nMinX;
      out_bounds->nMinY = bounds.nMinY;
      out_bounds->nMaxX = bounds.nMaxX;
      out_bounds->nMaxY = bounds.nMaxY;
   }
   if (out_glyph_metrics) {
      for (uint32_t nIdx = 0; nIdx < count; ++nIdx) {
         MB_GlyphMetrics& gmTarget = out_glyph_metrics[nIdx];
         const SGlyphMetrics& gmSrc = vGlyphMetrics[nIdx];
         gmTarget.nLSB = gmSrc.nLSB;
         gmTarget.nAdvanceWidth = gmSrc.nAdvanceWidth;
         gmTarget.nRSB = gmSrc.nRSB;
         gmTarget.nTSB = gmSrc.nTSB;
         gmTarget.nBSB = gmSrc.nBSB;
         gmTarget.nAdvanceHeight = gmSrc.nAdvanceHeight;
      }
   }
   return true;
}
//
void CMathBoxHost::_drawLine(float x1, float y1, float x2, float y2, uint32_t style, float width, uint32_t argb) {
   _pD2DR->DrawLine({ x1, y1 }, { x2, y2 }, (EnumLineStyles)style, width, argb);
}
void CMathBoxHost::_drawRect(float l, float t, float r, float b, uint32_t style, float width, uint32_t argb) {
   _pD2DR->DrawRect({ l, t, r, b }, (EnumLineStyles)style, width, argb);
}
void CMathBoxHost::_fillRect(float l, float t, float r, float b, uint32_t argb) {
   _pD2DR->FillRect({ l, t, r, b }, argb);
}
void CMathBoxHost::_drawGlyphRun(int32_t font_idx, uint32_t count, const uint16_t* indices,
                                 float base_x, float base_y, float scale, uint32_t argb) {
   _pD2DR->DrawGlyphRun(font_idx, count, indices, { base_x, base_y }, scale, argb);
}
