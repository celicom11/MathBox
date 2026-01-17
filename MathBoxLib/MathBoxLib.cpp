#include "stdafx.h"
#include "MathBox_CAPI.h"
#include "DocParamWrap.h"
#include "..\MathBox\TexParser.h"
#include "..\MathBox\LMMFont.h"
#include "..\MathBox\ContainerItem.h"

#define ABI_VERSION 1

static uint32_t _startPos = 0;
static uint32_t _endPos = 0;
static string _sGlobalError;
static string _sParserError;
// --- C API Implementations ---
// Opaque engine struct for ABI
struct MB_Engine_t {
   CDocParamWrap* pDocWrap{ nullptr };
   CTexParser*    pParser{ nullptr };
   CLMMFont*      pLmmFont{ nullptr };
};
static MB_RET createEngine_impl(const MB_DocParams* doc, MB_Engine* out_engine) {
   if (!doc) {
      _sGlobalError = "MB_DocParams* doc is NULL";
      return MB_BADPARAM;
   }
   if (doc->size_bytes != sizeof(MB_DocParams)) {
      _sGlobalError = "Unsupported version of MB_DocParams* doc";
      return MB_BADVERSION;
   }
   if (doc->font_mgr.size_bytes != sizeof(MBI_FontManager)) {
      _sGlobalError = "Unsupported version of MBI_FontManager* doc->font_mgr";
      return MB_BADVERSION;
   }
   if (!out_engine) {
      _sGlobalError = "MB_Engine* out_engine is NUll";
      return MB_BADPARAM;
   }
   if (*out_engine) {
      _sGlobalError = "MB_Engine* out_engine already has value";
      return MB_BADPARAM;
   }
   uint32_t buf_len = 0;
   if (!doc->font_mgr.getFontsDir(&buf_len, nullptr) || !buf_len) {
      _sGlobalError = "Failed to get LMM fonts directory size";
      return MB_ERR;
   }
   vector<wchar_t> vBuf(buf_len, 0);
   if (!doc->font_mgr.getFontsDir(&buf_len, vBuf.data()) || 0 == vBuf[0]) {
      _sGlobalError = "Failed to get LMM fonts directory";
      return MB_ERR;
   }
   //
   CLMMFont* pLmmFont = new CLMMFont;
   if (!pLmmFont->Init(vBuf.data())) {
      delete pLmmFont;
      _sGlobalError = "Failed to initialize LMM font manager";
      return MB_ERR;
   }
   try {
      MB_Engine_t* engine = new MB_Engine_t;
      engine->pDocWrap = new CDocParamWrap(doc, *pLmmFont);
      engine->pParser = new CTexParser(*engine->pDocWrap);
      engine->pLmmFont = pLmmFont;
      *out_engine = engine;
   }
   catch (...) {
      _sGlobalError = "Out of memory/unexpected failure in construction";
      return MB_ERR;
   }
   _sGlobalError.clear();
   return MBOK;
}
static void destroyEngine_impl(MB_Engine engine) {
   if (engine) {
      delete engine->pParser;
      delete engine->pDocWrap;
      delete engine->pLmmFont;
      delete engine;
   }
}
static void destroyMathItem_impl(MB_MathItem item) {
   if (item) {
      CMathItem* pItem = reinterpret_cast<CMathItem*>(item);
      delete pItem;
   }
}
//@ret: do not free! Per-engine pointer to 0-end ASCII string is valid 
// until next API call (except this one) or until engine destroyed!
static const char* getLastError_impl(MB_Engine engine, OUT uint32_t* startPos, OUT uint32_t* endPos) {
   if(startPos)
      *startPos = 0;
   if (endPos)
      *endPos = 0;
   if (!engine)
      return _sGlobalError.c_str();
   if (startPos)
      *startPos = _startPos;
   if (endPos)
      *endPos = _endPos;
   return _sParserError.c_str();
}
static MB_RET addMacros_impl(MB_Engine engine, const char* szMacros, const char* szFileName) {
   if (!engine || !engine->pDocWrap || !engine->pParser || !engine->pLmmFont) {
      _sGlobalError = "Corrupted engine";
      return MB_ERR;
   }
   if (!szMacros || !*szMacros) {
      _sGlobalError = "No input macros";
      return MB_BADPARAM;
   }
   if (!szFileName || !*szFileName) {
      _sGlobalError = "Macros filename is empty";
      return MB_BADPARAM;
   }
   CTexParser* pParser = engine->pParser;
   pParser->AddMacros(szMacros, szFileName);
   return MBOK;
}

//Caller must use/destroy returned MB_MathItem 
static MB_RET parseLatex_impl(MB_Engine engine, const char* szText, OUT MB_MathItem* out_item) {
   if (!engine || !engine->pDocWrap || !engine->pParser || !engine->pLmmFont) {
      _sGlobalError = "Corrupted engine";
      return MB_ERR;
   }
   if (!szText || !*szText) {
      _sGlobalError = "No input text";
      return MB_BADPARAM;
   }
   if (!out_item) {
      _sGlobalError = "MB_MathItem* out_item is NULL";
      return MB_BADPARAM;
   }
   CTexParser* pParser = engine->pParser;
   CMathItem* pItem = pParser->Parse(szText);
   *out_item = pItem;
   //copy last error info
   const ParserError& lasterror = pParser->LastError();
   _sParserError = lasterror.sError; //copy
   _startPos = lasterror.nStartPos;
   _endPos = lasterror.nEndPos;

   if(_sParserError.empty())
      return MBOK;
   return MB_PARSER + (MB_RET)lasterror.eStage;
}
static uint32_t mathItemLineCount_impl(MB_MathItem item) {
   if (!item) {
      _sGlobalError = "MB_MathItem item is NULL";
      return 0;
   }
   CMathItem* pItem = reinterpret_cast<CMathItem*>(item);
   uint32_t lines = 1;
   if (pItem->Type() == eacLINES) {
      CContainerItem* pVBOX = static_cast<CContainerItem*>(pItem);
      lines = (uint32_t)pVBOX->Items().size();
   }
   return lines;
}
//all "float" units are in DIPs
static MB_RET mathItemDraw_impl(MB_MathItem item, float x, float y, const MBI_DocRenderer* renderer) {
   if (!item) {
      _sGlobalError = "MB_MathItem item is NULL";
      return MB_BADPARAM;
   }
   if (!renderer) {
      _sGlobalError = "MBI_DocRenderer* renderer is NULL";
      return MB_BADPARAM;
   }
   if (renderer->size_bytes != sizeof(MBI_DocRenderer)) {
      _sGlobalError = "Unsupported version of MBI_DocRenderer* renderer";
      return MB_BADVERSION;
   }
   SDocRendererWrap dr(renderer);
   CMathItem* pItem = reinterpret_cast<CMathItem*>(item);
   pItem->Draw({ x,y }, dr);
   return MBOK;
}
static MB_RET mathItemDrawLines_impl(MB_MathItem item, float x, float y, 
                                     int32_t line_start, int32_t line_end, const MBI_DocRenderer* renderer) {
   if (!item) {
      _sGlobalError = "MB_MathItem item is NULL";
      return MB_BADPARAM;
   }
   if (line_end < line_start) {
      _sGlobalError = "line_end is less than item";
      return MB_BADPARAM;
   }
   CMathItem* pItem = reinterpret_cast<CMathItem*>(item);
   if (line_start >= 0 && pItem->Type() == eacLINES) {
      CContainerItem* pVBOX = static_cast<CContainerItem*>(pItem);
      if (line_end >= (int32_t)pVBOX->Items().size()) {
         _sGlobalError = "line_end is out of range";
         return MB_BADPARAM;
      }
      if (!renderer) {
         _sGlobalError = "MBI_DocRenderer* renderer is NULL";
         return MB_BADPARAM;
      }
      if (renderer->size_bytes != sizeof(MBI_DocRenderer)) {
         _sGlobalError = "Unsupported version of MBI_DocRenderer* renderer";
         return MB_BADVERSION;
      }
      SDocRendererWrap docr(renderer);
      float fFontSizePts = pItem->Doc().DefaultFontSizePts();
      //need to translate containerLT so that line_start is at (x,y)
      const STexBox& boxLineStart = pVBOX->Items()[line_start]->Box();
      SPointF ptfContainerLT{
         x - EM2DIPS(fFontSizePts, pVBOX->Box().Left() + boxLineStart.Left()),
         y - EM2DIPS(fFontSizePts, pVBOX->Box().Top() + boxLineStart.Top())
      };
      for (int32_t lineIdx = line_start; lineIdx <= line_end; ++lineIdx) {
         pVBOX->Items()[lineIdx]->Draw(ptfContainerLT, docr);
      }
      return MBOK;
   }
   else
      return mathItemDraw_impl(item, x, y, renderer);
}
static MB_RET mathItemSelect_impl(MB_MathItem item, float left, float top, float right, float bottom, 
                                  uint32_t flags) {
   if (!item ) {
      _sGlobalError = "MB_MathItem item is NULL";
      return MB_BADPARAM;
   }
   CMathItem* pItem = reinterpret_cast<CMathItem*>(item);

   _ASSERT(0);//TODO: select by marquee(DIPs)
   return MB_ERR;
}
static MB_RET mathItemGetBox_impl(MB_MathItem item, OUT float* left, OUT float* top, 
                                                    OUT float* right, OUT float* bottom) {
   if (!item ) {
      _sGlobalError = "MB_MathItem item is NULL";
      return MB_BADPARAM;
   }
   CMathItem* pItem = reinterpret_cast<CMathItem*>(item);
   const STexBox& box = pItem->Box();
   IDocParams& docp = pItem->Doc();
   if (left)
      *left = EM2DIPS(docp.DefaultFontSizePts(), box.Left());
   if (top)
      *top = EM2DIPS(docp.DefaultFontSizePts(), box.Top());
   if (right)
      *right = EM2DIPS(docp.DefaultFontSizePts(), box.Right());
   if (bottom)
      *bottom = EM2DIPS(docp.DefaultFontSizePts(), box.Bottom());

   return MBOK;
}
static MB_RET mathItemGetLineBox_impl(MB_MathItem item, int32_t line,
                                      OUT float* left, OUT float* top, OUT float* right, OUT float* bottom) {
   if (!item) {
      _sGlobalError = "MB_MathItem item is NULL";
      return MB_BADPARAM;
   }
   CMathItem* pItem = reinterpret_cast<CMathItem*>(item);
   STexBox box;
   if (line >= 0 && pItem->Type() == eacLINES) {
      CContainerItem* pVBOX = static_cast<CContainerItem*>(pItem);
      if (line >= (int32_t)pVBOX->Items().size()) {
         _sGlobalError = "Line index is out of range";
         return MB_BADPARAM;
      }
      box = pVBOX->Items()[line]->Box();
   }
   else {
      if (line > 0) {
         _sGlobalError = "Line index is out of range";
         return MB_BADPARAM;
      }
      box = pItem->Box();
   }

   IDocParams& docp = pItem->Doc();
   if (left)
      *left = EM2DIPS(docp.DefaultFontSizePts(), box.Left());
   if (top)
      *top = EM2DIPS(docp.DefaultFontSizePts(), box.Top());
   if (right)
      *right = EM2DIPS(docp.DefaultFontSizePts(), box.Right());
   if (bottom)
      *bottom = EM2DIPS(docp.DefaultFontSizePts(), box.Bottom());

   return MBOK;
}

//MAIN ENTRY
static const MBI_API g_Api = {
   sizeof(MBI_API),
   ABI_VERSION,
   createEngine_impl,
   destroyEngine_impl,
   destroyMathItem_impl,
   getLastError_impl,
   addMacros_impl,
   parseLatex_impl,
   mathItemLineCount_impl,
   mathItemDraw_impl,
   mathItemDrawLines_impl,
   mathItemSelect_impl,
   mathItemGetBox_impl,
   mathItemGetLineBox_impl
};
// --- Exported ABI Function ---
extern "C" MB_API const MBI_API* MB_GetApi() {
   return &g_Api;
}