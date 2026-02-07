#pragma once
#include "..\..\MathBoxLib\LibCode\MathBox_CAPI.h"

constexpr int32_t otfUnitsPerEm = 1000;
#define PTS2DIPS(x) ((x)*96.0f/72.0f)
#define F2NEAREST(x) ((int32_t)((x)<0?(x)-0.5f:(x)+0.5f))
#define DIPS2EM(fFontHPts, x) F2NEAREST((x)*otfUnitsPerEm/PTS2DIPS(fFontHPts))
static const PCWSTR _aLMFonts[]{
   L"latinmodern-math.otf",               // FONT_LMM
   L"lmroman10-regular.otf",              // FONT_ROMAN_REGULAR
   L"lmroman10-bold.otf",                 // FONT_ROMAN_BOLD
   L"lmroman10-italic.otf",               // FONT_ROMAN_ITALIC
   L"lmroman10-bolditalic.otf",           // FONT_ROMAN_BOLDITALIC
   L"lmsans10-regular.otf",               // FONT_SANS_REGULAR
   L"lmsans10-bold.otf",                  // FONT_SANS_BOLD
   L"lmsans10-oblique.otf",               // FONT_SANS_OBLIQUE
   L"lmsans10-boldoblique.otf",           // FONT_SANS_BOLDOBLIQUE
   L"lmmono10-regular.otf",               // FONT_MONO_REGULAR
   L"lmmono10-italic.otf",                // FONT_MONO_ITALIC
   L"lmromanslant10-regular.otf",         // FONT_ROMAN_SL_REGULAR
   L"lmromanslant10-bold.otf",            // FONT_ROMAN_SL_BOLD
   L"lmromancaps10-regular.otf",          // FONT_ROMAN_SMALLCAPS
};

struct SMathBoxError {
   MB_RET     nErrorCode{ MBOK };
   uint32_t   nErrorStartPos{ 0 };
   uint32_t   nErrorEndPos{ 0 };
   string     sError;
};
enum EnumLineStyles { elsSolid = 0, elsDash, elsDot, elsDashDot };

