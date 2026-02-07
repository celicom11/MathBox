#pragma once
// ABSTRACT GUI LAYER
struct SPointF {
   float fX{ 0 }, fY{ 0 };
};
struct SRectF {
   float fLeft{ 0 }, fTop{ 0 }, fRight{ 0 }, fBottom{ 0 };
};
struct SBoundsF {
   float fMinX, fMinY, fMaxX, fMaxY;
};
//all integer coordinates are in FDU!
struct SPoint {
   int32_t  x{ 0 }, y{ 0 };
};
struct SRect {
   int32_t  left{ 0 }, top{ 0 }, right{ 0 }, bottom{ 0 };
};

struct SBounds { 
   int32_t nMinX{ 0 }, nMinY{ 0 }, nMaxX{ 0 }, nMaxY{ 0 };
};
//units are FDUs = unitsPerEm
struct SGlyphMetrics {
   int32_t  nLSB{0};                // leftSideBearing
   uint32_t nAdvanceWidth{0};
   int32_t  nRSB{0};                // rightSideBearing
   int32_t  nTSB{0};                // topSideBearing
   uint32_t nAdvanceHeight {0};
   int32_t  nBSB{0};                // bottomSideBearing
};
enum EnumLineStyles {elsSolid=0,elsDash,elsDot,elsDashDot};
//LMM glyph MATH tables + other info
enum EnumGlyphClass {
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
};
struct SLMMGlyph {
   uint32_t nUnicode{ 0 };
   uint16_t nIndex{ 0 };
   uint16_t eClass{ 0 };               //EnumGlyphClass
   int16_t  nTopAccentX{ 0 };          //MATH
   uint16_t nItalCorrection{ 0 };      //MATH
   string   sName;                     //cmap glyph name
   string   sLaTexCmd;                 //latex-unicode.json
};
// INTERFACES //
// Manages specific to LMM fonts OpenType info/mappings/ etc.

DECLARE_INTERFACE(ILMFManager) {
   virtual ~ILMFManager() {}
   virtual const SLMMGlyph* GetLMMGlyph(int16_t nFontIdx, uint32_t nUnicode) = 0;
   virtual const SLMMGlyph* GetLMMGlyphByIdx(int16_t nFontIdx, uint16_t nIndex) = 0;
   virtual const SLMMGlyph* GetLMMGlyphByCmd(PCSTR szCmd) = 0;
};
// owns LMM fonts, measures and renders glyphs
DECLARE_INTERFACE(IFontManager) {
   virtual ~IFontManager() {}
   virtual uint32_t FontCount() const = 0;
   virtual bool GetFontIndices(int32_t nFontIdx, uint32_t nCount, const uint32_t* pUnicodes,
                               OUT uint16_t* pIndices) = 0;
   virtual bool GetGlyphRunMetrics(int32_t nFontIdx, uint32_t nCount, const uint16_t* pIndices,
                                    OUT SGlyphMetrics* pGlyphMetrics, OUT SBounds& bounds) = 0;
};
//document register/params/resources: DefaultFontSize, clrBackground, clrText, clrSelection
DECLARE_INTERFACE(IDocParams) {
   virtual ~IDocParams() {}
   virtual IFontManager& FontManager() = 0;
   virtual ILMFManager& LMFManager() = 0;
   virtual float DefaultFontSizePts() = 0;
   virtual int32_t MaxWidthFDU() = 0;
   virtual uint32_t ColorBkg() = 0;
   virtual uint32_t ColorSelection() = 0;
   //Text color ~= Foreground color!
   virtual uint32_t ColorText() = 0;
   //virtual void SetColorText(uint32_t clrText) = 0;
};
//abstract renderer interface
DECLARE_INTERFACE(IDocRenderer) {
   virtual ~IDocRenderer() {}
   virtual void DrawLine(SPointF pt1, SPointF pt2, EnumLineStyles eStyle = elsSolid,
                         float fWidth = 1.0f, uint32_t nARGB = 0) = 0;
   virtual void DrawRect(SRectF rcf, EnumLineStyles eStyle = elsSolid, 
                         float fWidth = 1.0f, uint32_t nARGB = 0) = 0;
   virtual void FillRect(SRectF rcf, uint32_t nARGB = 0) = 0;
   virtual void DrawGlyphRun(int32_t nFontIdx, uint32_t nCount, const uint16_t* pIndices,
                             SPointF ptfBaseOrigin, float fScale, uint32_t nARGB = 0) = 0;
};