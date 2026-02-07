#pragma once
#include "MathBox_CAPI.h"
#if MB_VERSION_MAJOR < 1
#error "MathBox version 1.0 or higher required"
#endif

#include "MathBoxHostDefs.h"

class CD2DRenderer;
class CD2DFontManager;
class CLMMFont;
//cpp front-end for MBI_API/parsed MB_MathItem
class CMathBoxHost {
//DATA
   HMODULE                 m_hMathBoxLib{ nullptr };
   MB_DocParams            m_DocParams;
   MBI_DocRenderer         m_DocRenderer;
   const MBI_API*          m_pMBAPI{ nullptr }; //external/dnd!
   MB_Engine               m_engine{ nullptr };
   MB_MathItem             m_pMBItem{ nullptr };
   SMathBoxError           m_Error;

public:
//CTOR/DTOR/INIT
   CMathBoxHost() = delete;
   CMathBoxHost(CD2DFontManager& d2dFontManager, CD2DRenderer& d2dr);
   ~CMathBoxHost() { CleanUp(); }
   bool LoadMathBoxLib();
   void CleanUp();
//ATTS
   void SetMaxWidth(int32_t nMaxWidthFDU) { m_DocParams.max_width_fdu = nMaxWidthFDU; }
   void SetClrText(uint32_t clrText) { m_DocParams.color_text_argb = clrText; }
   uint32_t ClrBkg() const { return m_DocParams.color_bkg_argb; }
   void SetClrBkg(uint32_t clrBkg) { m_DocParams.color_bkg_argb = clrBkg; }
   void SetClrSel(uint32_t clrSel) { m_DocParams.color_selection_argb = clrSel; }

   float FontSizePts() const { return m_DocParams.font_size_pts; }
   void SetFontSizePts(float fFontSizePts) { m_DocParams.font_size_pts = fFontSizePts; }
   const SMathBoxError& LastError() const { return m_Error;}
//METHODS
   bool AddMacros(const string& sMacros, const string& sFile);
   bool Parse(const string& sTeX);
   void Draw(float fX, float fY);
   //C-API/callbacks
   static bool _getFontsDir(IN OUT uint32_t* buf_len, OUT wchar_t* out_dir);
   static bool _getFontIndices(int32_t font_idx, uint32_t count, const uint32_t* unicodes, OUT uint16_t* out_indices);
   static bool _getGlyphRunMetrics(int32_t font_idx, uint32_t count, const uint16_t* indices,
                                     OUT MB_GlyphMetrics* out_glyph_metrics, OUT MB_Bounds* out_bounds);
   //
   static void _drawLine(float x1, float y1, float x2, float y2, uint32_t style, float width, uint32_t argb);
   static void _drawRect(float l, float t, float r, float b, uint32_t style, float width, uint32_t argb);
   static void _fillRect(float l, float t, float r, float b, uint32_t argb);
   static void _drawGlyphRun(int32_t font_idx, uint32_t count, const uint16_t* indices,
                              float base_x, float base_y, float scale, uint32_t argb);
   //TODO: static void _traceMsg(int nLevel,const char* szMsg);

};