#pragma once
#include "LMMConsts.h"

#define PTS2DIPS(x) ((x)*96.0f/72.0f)
#define DIPS2PTS(x) ((x)*72.0f/96.0f)
#define F2NEAREST(x) ((int32_t)((x)<0?(x)-0.5f:(x)+0.5f))
#define EM2DIPS(fFontHPts, x) ((x)*PTS2DIPS(fFontHPts)/otfUnitsPerEm)
#define DIPS2EM(fFontHPts, x) F2NEAREST((x)*otfUnitsPerEm/PTS2DIPS(fFontHPts))

//
//forward declarations
struct ID2D1RenderTarget;
struct ID2D1Brush;

//abstract TexBox. all coordinates are in EM/font design units!!! 
struct STexBox {
   int32_t nX{ 0 }, nY{ 0 };                 // left,INK-top!
   int32_t nAdvWidth{ 0 };                   // Ink-Box width = nAdvWidth - side bearings!
   int32_t nHeight{ 0 };                     // nAscent+Descent = BoundsMaxY-BoundsMinY
   int32_t nAscent{ 0 };                     // distance from INKTOP to baseline, may be negative!
   int32_t nLBearing{ 0 }, nRBearing{ 0 };   // for glyph boxes!
   //ATTS
   bool IsEmpty() const { return nAdvWidth == 0 && nHeight == 0; }
   int32_t Ascent() const { return nAscent; }
   int32_t Depth() const { return nHeight - nAscent; }
   int32_t Height() const { return nHeight; }
   int32_t Width() const { return nAdvWidth; }
   int32_t LBearing() const { return nLBearing; }
   int32_t RBearing() const { return nRBearing; }
   //abs positions
   int32_t Left() const { return nX; }
   int32_t Right() const { return nX + nAdvWidth; }
   int32_t Top() const { return nY; }  //==Ink-Top
   int32_t Bottom() const { return nY + nHeight; }
   int32_t BaselineY() const { return nY + nAscent; }
   int32_t AxisY() const { return BaselineY() - otfAxisHeight; } //absolute Y of math axis
   int32_t InkLeft() const { return Left() + nLBearing; }
   int32_t InkRight() const { return Right() - nRBearing; }
   int32_t InkWidth() const { return InkRight() - InkLeft(); }
   // OPERATIONS
   void MoveTo(int32_t x, int32_t y) { nX = x; nY = y; }
   void MoveBy(int32_t dx, int32_t dy) { nX += dx; nY += dy; }
   void Union(const STexBox& b) {
      if (IsEmpty()) { *this = b; return; }
      if (b.IsEmpty()) return;
      int32_t x0 = min(nX, b.nX);
      int32_t y0 = min(nY, b.nY);
      int32_t x1 = max(Right(), b.Right());
      int32_t y1 = max(Bottom(), b.Bottom());
      // keep current baseline/ascent
      if (b.nY < nY)
         nAscent += (nY - b.nY);
      nX = x0; nY = y0;
      nHeight = y1 - y0;
      nAdvWidth = x1 - x0;
   }
   void SetMathAxis(int32_t nAxisY) {
      //nY + nAscent - otfAxisHeight = nAxisY =>
      nAscent = nAxisY + otfAxisHeight - nY;
   }
   void Copy(const STexBox& b) {
      nX = b.nX; nY = b.nY;
      nAdvWidth = b.nAdvWidth; nHeight = b.nHeight;
      nAscent = b.nAscent;
      nLBearing = b.nLBearing; nRBearing = b.nRBearing;
   }
};
struct SDWRenderInfo {
   bool                 bDrawBoxes{ false };       //debugging aid
   float                fFontSizePts{ 0.0f };      //document font size, in points
   ID2D1RenderTarget*   pRT{ nullptr };
   ID2D1Brush*          pBrush{ nullptr };
   ID2D1Brush*          pSelBrush{ nullptr };      //selection support
};
enum EnumTexStyle {
   etsDisplay = 0,   // display mode
   etsText,          // inline/compact mode
   etsScript,
   etsScriptScript
};
// TeX atom's basic types, for inter-spacing rules
enum EnumTexAtom {
   etaORD=0,            // Ord: variable name or text
   etaOP,               // Op: Large AND math operator, e.g. integral, sum, gcd, etc.
   etaBIN,              // Bin: binary operation, e.g. +,-,*,etc.
   etaREL,              // Rel: relation, e.g. =,<,>,etc.
   etaOPEN,             // Open: left delimiters, e.g. (,[,{,|,etc.
   etaCLOSE,            // Close: right delimiters, e.g. |,),],},etc.
   etaPUNCT,            // Punct: punctuation, e.g. comma,semicolon,etc.
   etaINNER             // Inner: boxed subformula, NOTE: fraction is Ord now!
};
// Math item types
enum EnumMathItemType {
   eacUNK = -1,         // exention glyphs/fillers and other non selectable items
   eacWORD,             // variable's name, number, operator, punctuation,etc or text
   eacHBOX,             // HBox or Inner \left..\right subformula
   eacOVERUNDER,        // Ord: item with an over/under-brace child
   eacACCENT,           // Acc: item with an accent child (\bar, \hat, \vec, \tilde, etc.)
   eacINDEXED,          // [all]: item with subscript/superscript indexes; its default in TeX, but not here!
   eacRADICAL,          // Ord: radical/root item with base and argument
   eacVBOX,             // Ord: fraction or stacked items: \frac, \binom, \stackrel, \substack, \atop, \genfrac, \overset, etc.
   eacBIGOP,            // OP(large): integrals,sum, prod, etc. + optional \limits
   eacMATHOP,           // OP(small): math op + optional \limits .Used for (\lim, \liminf, \min, \max, \gcd, \sin etc.)
   //eacCANCEL,         // item with a cancel line (diagonal cross-out)
   //eacNOT,            // item with a slash (negation)
   eacGLUE,             // -: invisible spacing item of variable width
};
enum EnumIndexPlacement {
   eipStd = 0,           //default
   eipOverscript,        //overbrace^{cmnt}
   eipUnderscript,       //underbrace^{cmnt}
   eipOverUnderscript    //LargOp in D mode
};
//Tex style helper class
class CMathStyle {
   bool              m_bCramped{ false };      //cramped/compact style (for scripts, radicand,)
   EnumTexStyle      m_eTexStyle{ etsDisplay };
public:
   //CTOR
   CMathStyle(EnumTexStyle eTexStyle = etsDisplay, bool bCramped = false) :
      m_eTexStyle(eTexStyle), m_bCramped(bCramped) {
   }
   //ATTS
   bool IsCramped() const { return m_bCramped; }
   void SetCramped(bool bCramped = true) { m_bCramped = bCramped; }
   EnumTexStyle Style() const { return   m_eTexStyle; }
   void SetStyle(EnumTexStyle eTexStyle) { m_eTexStyle = eTexStyle; }
   float StyleScale() const {
      if (m_eTexStyle == etsScript)
         return otfScriptPercentScaleDown;
      else if (m_eTexStyle == etsScriptScript)
         return otfScriptScriptPercentScaleDown;
      return 1.0f; //display
   }
   //METHODS
   void Decrease() {
      if (m_eTexStyle < etsScriptScript)
         m_eTexStyle = EnumTexStyle(m_eTexStyle + 1);
   }
};
//Tex Glue info
struct STexGlue {
   uint16_t nStretchOrder{ 0 };      //0=normal, 1=fil, 2=fill, 3=filll
   uint16_t nShrinkOrder{ 0 };       //0=normal, 1=fil, 2=fill, 3=filll
   float    fStretchCapacity{ 0 };   //in EM or in fill units if InfStretchOrder>0
   float    fShrinkCapacity{ 0 };    //in EM or in fill units if InfShrinkOrder>0
   float    fNorm{ 0 };              //in EM 
   float    fActual{ 0 };            //in EM 
   void ResizeByRatio(float fRatio) {
      if (fRatio >= 0.0f)
         fActual = fNorm + fStretchCapacity*fRatio;
      else
         fActual = fNorm + fShrinkCapacity*fRatio;
   }
};
//base class for math items, abstract!
class CMathItem {
protected:
   bool                 m_bSelected{ false };    //selection support
   float                m_fUserScale{ 1.0f };    //user scaling factor
   EnumIndexPlacement   m_eIndexPlacement{ eipStd };
   EnumTexAtom          m_eAtom{ etaORD };       //TeX atom class, for inter-item spacing rules
   EnumMathItemType     m_eType{ eacUNK };       //~TeX atom type
   CMathStyle           m_Style;                 //Tex style info   
   STexBox              m_Box;
public:
   //CTOR/DTOR
   CMathItem(EnumMathItemType eType, const CMathStyle& style, float fUserScale = 1.0f) :
      m_eType(eType), m_Style(style), m_fUserScale(fUserScale) {
   }
   virtual ~CMathItem() {}
   //ATTS
   bool IsSelected() const { return m_bSelected; }
   EnumIndexPlacement IndexPlacement() const { return m_eIndexPlacement; }
   void SetIdxPlacement(EnumIndexPlacement eIndexPlacement) { m_eIndexPlacement = eIndexPlacement; }
   EnumTexAtom AtomType() const { return m_eAtom; }
   void SetAtom(EnumTexAtom eAtom) { m_eAtom = eAtom; } //sometimes has to be re set !
   const CMathStyle& GetStyle() { return m_Style; }
   const STexBox& Box() const { return m_Box; }
   EnumMathItemType Type() const { return m_eType; }
   //METHODS
   void DenominateBinRel() {
      //make it ordinary atom
      if (m_eAtom == etaBIN || m_eAtom == etaREL)
         m_eAtom = etaORD;
   }
   void MoveTo(int32_t nX, int32_t nY) {
      m_Box.MoveTo(nX, nY);
   }
   void MoveBy(int32_t nDX, int32_t nDY) {
      m_Box.MoveTo(m_Box.Left() + nDX, m_Box.Top() + nDY);
   }
   //VIRTUALS
   virtual const STexGlue* GetGlue() const { return nullptr; } //default: not resizable!
   //resize to Norm + fRatio*StretchCapacity() - for all Glue orders! fRatio<0 means shrink!
   virtual void ResizeByRatio(uint16_t nOrder, float fRatio) {} //default: not resizable!
   virtual void Select(bool bSelect = true) { m_bSelected = bSelect; } //default
   virtual void Draw(D2D1_POINT_2F ptAnchor, const SDWRenderInfo& dwri) = 0;
};
//
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
enum EnumDelimType {
   edtNotDelim = 0,
   edtAny,     //valid, but undefined
   edtOpen,    //left
   edtClose,   //right
   edtFence,   //middle
};
struct SLMMDelimiter {
   EnumDelimType  edt{ edtNotDelim };
   int16_t        nVariantIdx{ 0 };    // index of the variant-size : -1(dynamic),0,1,2,3,4
   uint32_t       nUni{ 0 };           // base Unicode, may be 0 for invisible open/close delimiters!
};
//PARSER DEFS - TODO: move to parser's decls
enum EnumLatexCmdArgType {
   elcatNull = -1,// empty, no argument
   elcatItem = 0, // ~any, CMathItem*
   elcatGlyph,   // single glyph/symbol is expected
   elcatDim,      // size: #{pt|em}
   elcatTexStyle, // nVal = 0,1,2,3
   elcatLimits,   // nVal = 0(nolimits),1(limits)
   elcatMultiLine // multi-liner arguments, split by "\\\\" (e.g. \substack)
};
enum EnumLCATParenthesis {
   elcapAny = 0,  // optional {} brackets
   elcapFig,      // non-optional {} brackets
   elcapSquare,   // non-optional [] brackets
};
struct SLaTexCmdArgInfo {
   bool                 bOpt{ false };
   bool                 bBase{ false };
   bool                 bScript{ false };          // decreased style, of the Base OR the cmd input style if no base!
   int16_t              nArgPos{ 0 };              // -1 = BEFORE command,0 -inside command (e.g. "_{}^{}), >0 after command
   EnumLatexCmdArgType  eLCAT{ elcatItem };
   EnumLCATParenthesis  eParenthesis{ elcapAny };
};
struct SLaTexCmdInfo {
   bool                       bHasLimits{ false }; //command supports \limits
   vector<SLaTexCmdArgInfo>   vArgInfo;
};
struct SLaTexCmdArgValue {
   EnumLatexCmdArgType  eLCAT;
   union {
      int16_t     nVal;
      float       fVal;
      CMathItem* pMathItem{ nullptr };
   }       uVal;
};
DECLARE_INTERFACE(IMathItemBuilder) {
   virtual ~IMathItemBuilder() {}
   virtual bool CanTakeCommand(PCSTR szCmd) const = 0;
   virtual bool GetCommandInfo(PCSTR szCmd, OUT SLaTexCmdInfo& cmdInfo) const = 0;
   virtual CMathItem* BuildItem(PCSTR szCmd, const CMathStyle& style, float fUserScale, 
                                 const vector<SLaTexCmdArgValue>& vArgValues) const = 0;
};
