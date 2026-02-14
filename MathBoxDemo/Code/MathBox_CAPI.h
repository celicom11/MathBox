// MathBox C-ABI, v1.0-beta
#pragma once
#include <stdint.h>

// Version information (MAJOR.MINOR format)
#define MB_VERSION_MAJOR 1
#define MB_VERSION_MINOR 1
#define MB_VERSION_PRERELEASE "beta"  // Empty string for final releases
#define MB_VERSION_STRING "1.1-beta"

// Helper macro for version comparisons
#define MB_MAKE_VERSION(major, minor) ((major) * 100 + (minor))
#define MB_VERSION MB_MAKE_VERSION(MB_VERSION_MAJOR, MB_VERSION_MINOR)

//RETURN CODES
#define MBOK                  0
#define MB_BADPARAM           1     //bad parameter
#define MB_BADVERSION         2     //bad ABI version
#define MB_HOSTERR            4     //host has error
#define MB_ERR                5     //generic error
#define MB_PARSER             10    //parsing: generic/unknown error
#define MB_PARSER_BADTOKEN    11    //parsing: bad token
#define MB_PARSER_BADGROUP    12    //parsing: bad group
#define MB_PARSER_BUILDING    13    //parsing: building items error
typedef uint32_t MB_RET;

//By default all units are FDUs = font design units
typedef struct {
   //FIXED/WILL NEVER CHANGE!
   int32_t nMinX, nMinY, nMaxX, nMaxY;
} MB_Bounds;

typedef struct {
   uint32_t size_bytes;
   int32_t  nLSB;                // leftSideBearing
   uint32_t nAdvanceWidth;
   int32_t  nRSB;                // rightSideBearing
   int32_t  nTSB;                // topSideBearing
   uint32_t nAdvanceHeight;
   int32_t  nBSB;                // bottomSideBearing
} MB_GlyphMetrics;

// INTERFACES //

// owns LMM fonts, measures and renders glyphs
typedef struct {
   uint32_t size_bytes;
   uint32_t fontCount;

   bool (*getFontsDir)(/*in out*/ uint32_t* buf_len, /*out*/ wchar_t* out_dir); //Unicode!
   bool (*getFontIndices)(int32_t font_idx, uint32_t count, const uint32_t* unicodes, /*out*/ uint16_t* out_indices);
   bool (*getGlyphRunMetrics)(int32_t font_idx, uint32_t count, const uint16_t* indices,
      /*out*/ MB_GlyphMetrics* out_glyph_metrics, /*out*/ MB_Bounds* out_bounds);
} MBI_FontManager;

//document registers/params and fonts
typedef struct {
   uint32_t           size_bytes;

   float              font_size_pts;
   int32_t            max_width_fdu;
   uint32_t           color_bkg_argb;
   uint32_t           color_selection_argb;
   uint32_t           color_text_argb;
   MBI_FontManager    font_mgr;
} MB_DocParams;

//abstract renderer, all units are in DIPs
typedef struct {
   uint32_t size_bytes;

   void (*drawLine)(float x1, float y1, float x2, float y2, uint32_t style, float width, uint32_t argb);
   void (*drawRect)(float l, float t, float r, float b, uint32_t style, float width, uint32_t argb);
   void (*fillRect)(float l, float t, float r, float b, uint32_t argb);
   void (*drawGlyphRun)(int32_t font_idx, uint32_t count, const uint16_t* indices,
                        float base_x, float base_y, float scale, uint32_t argb);
} MBI_DocRenderer;

//Opaque Handlers
typedef struct MB_Engine_t* MB_Engine;
typedef void* MB_MathItem;
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
   //add more macros; optional
   MB_RET(*addMacros)(MB_Engine engine, const char* szMacros, const char* szFileName);
   //Caller must use/destroy returned MB_MathItem NOTE: can be destroyed after engine!
   MB_RET (*parseLatex)(MB_Engine engine, const char* szText, /*out*/ MB_MathItem* out_item);
   //all "float" units are in DIPs
   uint32_t(*mathItemLineCount)(MB_MathItem item);
   MB_RET (*mathItemDraw)(MB_MathItem item, float x, float y, const MBI_DocRenderer* renderer);
   MB_RET (*mathItemDrawLines)(MB_MathItem item, float x, float y, int32_t line_start, int32_t line_end, 
                               const MBI_DocRenderer* renderer);
   MB_RET (*mathItemSelect)(MB_MathItem item, float left, float top, float right, float bottom, uint32_t flags);
   MB_RET (*mathItemGetBox)(MB_MathItem item, /*out*/ float* left, /*out*/ float* top, 
                                              /*out*/ float* right, /*out*/ float* bottom);
   MB_RET(*mathItemGetLineBox)(MB_MathItem item, int32_t line,
                              /*out*/ float* left, /*out*/ float* top, /*out*/ float* right, /*out*/ float* bottom);
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