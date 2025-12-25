#pragma once
#include <stdint.h>
// MathBox Cross Platform API/ABI
// enum EnumLineStyles {elsSolid=0,elsDash,elsDot,elsDashDot};
//LMM glyph MATH tables + other info
/* enum EnumGlyphClass {
   egcOrd = 0, //mathordand others/default
   egcLOP,     //largeop
   egcBin,     //mathbin
   egcRel,     //mathrel, including arrows
   egcOpen,    //mathopen
   egcClose,   //mathclose
   egcPunct,   //mathpunct
   egcAccent,  //mathaccent, mathbotaccent
   egcOver,    //mathover
   egcUnder,   //mathunder
};*/
typedef uint32_t MB_RET; //0-OK, see documentation for non-0/errors
//By default all units are FDUs = font design units
typedef struct MB_Bounds {
   //FIXED/WILL NEVER CHANGE!
   int32_t nMinX, nMinY, nMaxX, nMaxY;
} MB_Bounds;

typedef struct MB_GlyphMetrics {
   uint32_t size_bytes;
   int32_t  nLSB;                // leftSideBearing
   uint32_t nAdvanceWidth;
   int32_t  nRSB;                // rightSideBearing
   int32_t  nTSB;                // topSideBearing
   uint32_t nAdvanceHeight ;
   int32_t  nBSB;                // bottomSideBearing
} MB_GlyphMetrics;

typedef struct MB_LMMGlyph {
   uint32_t size_bytes;
   uint32_t nUnicode;
   uint16_t nIndex;
   uint16_t eClass;               //EnumGlyphClass
   int16_t  nTopAccentX;          //MATH
   uint16_t nItalCorrection;      //MATH
   char     szName[64];                //cmap glyph name
   char     szLaTexCmd[64];            //latex-unicode.json
} MB_LMMGlyph;

// INTERFACES //
typedef struct MBI_LMFManager {
   uint32_t size_bytes;
   
   MB_RET (*getLMMGlyph)(int16_t font_idx, uint32_t unicode, /* out */ MB_LMMGlyph* out_lmmg);
   MB_RET (*getLMMGlyphByIdx)(int16_t font_idx, uint16_t index, /* out */ MB_LMMGlyph* out_lmmg);
   MB_RET (*getLMMGlyphByCmd)(const char* cmd, /* out */ MB_LMMGlyph* out_lmmg);
} MBI_LMFManager;

// owns LMM fonts, measures and renders glyphs
typedef struct MBI_FontManager {
  uint32_t size_bytes;
  
  MB_RET (*fontCount)();
  MB_RET (*getFontIndices)( int32_t font_idx, uint32_t count, const uint32_t* unicodes, /* out */ uint16_t* out_indices);
  MB_RET (*getGlyphRunMetrics)( int32_t font_idx, uint32_t count, const uint16_t* indices,
                                 /* out */ MB_GlyphMetrics* out_glyph_metrics, /* out */ MB_Bounds* out_bounds);
} MBI_FontManager;

//document register/params/resources: DefaultFontSize, clrBackground, clrText, clrSelection
typedef struct MB_DocParams {
  uint32_t size_bytes;

  float     default_font_size_pts;
  int32_t   default_line_skip_em;
  uint32_t  color_bkg_argb;
  uint32_t  color_selection_argb;
  uint32_t  color_text_argb;

  MBI_FontManager font_mgr;
  MBI_LMFManager  lmf_mgr;
} MB_DocParams;

//abstract renderer, all units are in DIPs
typedef struct MBI_DocRenderer {
  uint32_t size_bytes;

  void (*drawLine)(float x1, float y1, float x2, float y2,
                   uint32_t style, float width, uint32_t argb);
  void (*drawRect)(float l, float t, float r, float b,
                   uint32_t style, float width, uint32_t argb);
  void (*fillRect)(float l, float t, float r, float b, uint32_t argb);
  void (*drawGlyphRun)(int32_t font_idx, uint32_t count, const uint16_t* indices,
                       float base_x, float base_y, float scale, uint32_t argb);
} MBI_DocRenderer;

//Opaque Handlers
typedef struct MB_Engine_t* MB_Engine;
typedef struct MB_MathItem_t* MB_MathItem;
typedef struct MBI_API {
   uint32_t size_bytes;
   uint32_t abi_version;
   //Caller must use/destroy returned MB_Engine 
   MB_RET (*createEngine)(const MB_DocParams* doc, /*out*/ MB_Engine* out_engine);
   void   (*destroyEngine)(MB_Engine engine);
   void   (*destroyMathItem)(MB_MathItem item);
   //@ret: do not free! Per-engine pointer to 0-end ASCII string is valid 
   // until next API call (except this one) or until engine destroyed!
   const char* (*getLastError)(MB_Engine engine, /*out*/ uint32_t* startPos, /*out*/ uint32_t* endPos);
   //Caller must use/destroy returned MB_MathItem 
   MB_RET (*parseLatex)(MB_Engine engine, const char* text, uint32_t len, /*out*/ MB_MathItem* out_item);
   //all "float" units are in DIPs
   MB_RET (*mathItemDraw)(MB_MathItem item, float x, float y, const MBI_DocRenderer* renderer);
   MB_RET (*mathItemSelect)(MB_MathItem item, float left, float top, float right, float bottom, uint32_t flags);
   MB_RET (*mathItemGetBox)(MB_MathItem item, /*out*/ float* left, /*out*/ float* top, /*out*/ float* right, /*out*/ float* bottom);

} MBI_API;

//MathBox Engine API
#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#ifdef MB_EXPORTS
#define MB_API __declspec(dllexport)
#else
#define MB_API __declspec(dllimport)
#endif
#else
#define MB_API __attribute__((visibility("default")))
#endif

MB_API const MBI_API* MB_GetApi();

#ifdef __cplusplus
} /* extern "C" */
#endif