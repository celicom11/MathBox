#pragma once
#include "AGL.h"
#include "LMMConsts.h"

//abstract TexBox. all coordinates are in EM/font design units!!! 
struct STexBox {
   int32_t nX{ 0 }, nY{ 0 };                 // left,INK-top!
   int32_t nShiftX{ 0 }, nShiftY{ 0 };       // NEW: shift for rendering!
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
   eacUNK = -1,         // unknown OR non selectable Word item
   eacWORD,             // word item: variable's name, number, operator, punctuation,etc or text
   eacHBOX,             // HBox container, e.g. \left..\right subformula, any line with items, etc.
   eacOVERUNDER,        // Ord: container item with an over/under-brace child
   eacACCENT,           // Acc: container item with an accent child (\bar, \hat, \vec, \tilde, etc.)
   eacINDEXED,          // [all]:container item with subscript/superscript indexes; its default in TeX, but not here!
   eacRADICAL,          // Ord: radical/root item with base and argument
   eacVBOX,             // Ord: fraction or stacked items: \frac, \binom, \stackrel, \substack, \atop, \genfrac, \overset, etc.
   eacBIGOP,            // OP(large),word item: integrals,sum, prod, etc. + optional \limits
   eacMATHOP,           // OP(small),word item: math op + optional \limits .Used for (\lim, \liminf, \min, \max, \gcd, \sin etc.)
   eacGLUE,             // invisible spacing item of variable width
   eacVDELIM,           // Ord: container item with vertical delimiter + parts
   eacHDELIM,           // Ord: container item with horizontal bracket + parts
   eacTABLE,            // Ord: special container item for matrices and arrays
   eacLINES,            // container item with multiple lines/HBOX-es
   eacRULE,             // rule/rectangle filler item
   eacOVERLAY,          // container item with a lines over an item
   eacTEXTLINES,        // container item with multiple TEXT lines/HBOX-es which could be un-boxed/merged to parent Text box if needed
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
   bool IsDisplay() const { return m_eTexStyle == etsDisplay; }
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
   void ToSubscriptStyle() {
      ToSuperscriptStyle();
      m_bCramped = true; // subscripts are always cramped
   }
   void ToSuperscriptStyle() {
      if (m_eTexStyle == etsScript)
         m_eTexStyle = etsScriptScript;
      else
         m_eTexStyle = etsScript;
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
   bool                 m_bSelected{ false };      //selection support
   float                m_fUserScale{ 1.0f };      //user scaling factor
   EnumIndexPlacement   m_eIndexPlacement{ eipStd };
   EnumTexAtom          m_eAtom{ etaORD };         //TeX atom class, for inter-item spacing rules
   EnumMathItemType     m_eType{ eacUNK };         //~TeX atom type
   IDocParams&          m_Doc;                     //doc params ref  
   CMathStyle           m_Style;                   //Tex style info   
   STexBox              m_Box;
public:
   //CTOR/DTOR
   CMathItem(IDocParams& doc, EnumMathItemType eType, const CMathStyle& style, float fUserScale = 1.0f) :
      m_Doc(doc),m_eType(eType), m_Style(style), m_fUserScale(fUserScale) {
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
   float UserScale() const { return m_fUserScale; }
   IDocParams& Doc() { return m_Doc; }
   float EffectiveScale() const {
      return m_fUserScale * m_Style.StyleScale();
   }
   int32_t GetShiftX() const { return m_Box.nShiftX; }
   int32_t GetShiftY() const { return m_Box.nShiftY; }
   //METHODS
   void DenominateBinRel() {
      //make it ordinary atom
      if (m_eAtom == etaBIN || m_eAtom == etaREL)
         m_eAtom = etaORD;
   }
   void ShiftUp(int32_t nShiftY) {
      m_Box.nShiftY -= nShiftY;
   }
   void ShiftLeft(int32_t nShiftX) {
      m_Box.nShiftX -= nShiftX;
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
   virtual void Draw(SPointF ptfAnchor, IDocRenderer& docr) = 0; //PURE
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
//PARSER DECLS
enum EnumTokenType {
   ettUndef = 0,
   ettSPACE,                           // space, tab, newline
   ettALNUM,                           // sequence of letters/digits
   ettNonALPHA,                        // non-AlphaNum: .,+-/*?:=@#"'<> etc.
   ettCOMMAND,                         // \command or \symbol
   ettFB_OPEN,                         // {
   ettFB_CLOSE,                        // }
   ettSB_OPEN,                         // [
   ettSB_CLOSE,                        // ]
   ettCOMMENT,                         // % to end of line
   ett$,                               // $..$
   ett$$,                              // $$..$$
   ettAMP,                             // &, tabular alignment
   ettSUBS,                            // _, subscript
   ettSUPERS,                          // ^, superscript
   ettMARG1 = 100,                     // macro parameter 1
   ettMARG2,                           // macro parameter 2
   ettMARG3,                           // macro parameter 3
   ettMARG4,                           // macro parameter 4
   ettMARG5,                           // macro parameter 5
   ettMARG6,                           // macro parameter 6
   ettMARG7,                           // macro parameter 7
   ettMARG8,                           // macro parameter 8
   ettMARG9                            // macro parameter 9
};
struct STexToken {
   int16_t        nType{ ettUndef };   // EnumTokenType
   uint16_t       nLen{ 0 };           // length in chars
   int32_t        nPos{ 0 };           // position in source/macro-expanded text
   uint32_t       nTkIdxEnd{ 0 };      // 0 OR index of the end-of-group token!
   // 1-based original src token index for macro-expanded tokens
   // -1 means not yet expanded!
   int32_t        nRefIdx{ 0 };
};
enum EnumParsingStage { epsUnk = 0, epsTOKENIZINGMACROS, epsGROUPINGMACROS, epsBUILDINGMACROS,
                                    epsTOKENIZING, epsGROUPING, epsMACROEXP, epsBUILDING };
//parser error info
struct ParserError {
   EnumParsingStage  eStage{ epsUnk };
   uint32_t          nStartPos{ 0 };   //in the input text
   uint32_t          nEndPos{ 0 };
   string            sError;
   void SetError(EnumParsingStage eStage_, const STexToken& tkn, const string& sErr) {
      eStage = eStage_;
      nStartPos = tkn.nPos;
      nEndPos = tkn.nPos + tkn.nLen;
      sError = sErr.empty() ? "Unknown error" : sErr;
   }
   void SetError(const STexToken& tkn, const string& sErr) {
      nStartPos = tkn.nPos;
      nEndPos = tkn.nPos + tkn.nLen;
      sError = sErr.empty() ? "Unknown error" : sErr;
   }
};
enum EnumLatexCmdArgType {
   elcatNull = -1,// empty, no argument
   elcatItem = 0, // ~any, CMathItem*
   elcatGlyph,    // single glyph/symbol is expected
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
enum EnumColAlignment { ecaLeft=0, ecaCenter, ecaRight };
struct SParserContext {
   bool        bTextMode{ false };            // TEXT/MATH mode
   bool        bInLeftRight{ false };         // inside \left...\right construct
   bool        bInSubscript{ false };         // building atom's subscript
   bool        bInSuperscript{ false };       // building atom's superscript
   bool        bDisplayFormula{ false };      // centered on a separate line
   bool        bNoNewLines{ false };          // ignore \\,\newline,\cr commands
   CMathStyle  currentStyle;                  // MATH mode style
   float       fUserScale{ 1.0f };            // User scaling factor
   float       fFontScale{ 1.0f };            // current font scaling factor, need in ApplyFontScale!
   string      sFontCmd;                      // Current font (for both Math/Text modes!)
 //METHODS
   float EffectiveScale() const {
      return fUserScale * currentStyle.StyleScale();
   }
   //rescale
   void ApplyFontScale(float fNewFontScale) {
      fUserScale = fUserScale/fFontScale* fNewFontScale;
      fFontScale = fNewFontScale;
   }
   void CopyBasics(const SParserContext& other) {
      this->bTextMode = other.bTextMode;
      this->currentStyle = other.currentStyle;
      this->fUserScale = other.fUserScale;
      this->sFontCmd = other.sFontCmd;
      this->bNoNewLines = other.bNoNewLines;
   }
   //helpers
   void SetInSubscript() {
      _ASSERT(!this->bInSubscript);
      this->currentStyle.ToSubscriptStyle();
      this->bInSubscript = true;
      this->bInLeftRight = false; //cannot handle \\middle in subscript
   }
   void SetInSuperscript() {
      _ASSERT(!this->bInSuperscript);
      this->currentStyle.ToSuperscriptStyle();
      this->bInSuperscript = true;
      this->bInLeftRight = false; //cannot handle \\middle in superscript
   }
};

DECLARE_INTERFACE(IParserAdapter) {
   virtual CMathItem* ConsumeItem(EnumLCATParenthesis eParens,  const SParserContext& ctx) = 0;
   virtual bool ConsumeDimension(EnumLCATParenthesis eParens, OUT float& fPts, OUT string & sOrigUnits) = 0;
   virtual bool ConsumeInteger(EnumLCATParenthesis eParens, OUT int& nVal) = 0;
   virtual bool ConsumeHSkipGlue(OUT STexGlue& glue) = 0;
   virtual uint32_t ConsumeColor(EnumLCATParenthesis eParens) = 0;
   //macros/variables
   virtual bool GetLength(const string& sVar, OUT float& fPts) const = 0;
   virtual void SetLength(const string& sVar, float fPts) = 0;
   // raw access
   virtual EnumTokenType GetTokenData(OUT string& sText) = 0;
   virtual void SkipToken() = 0;
   // context info
   virtual const SParserContext& GetContext() const = 0;
   virtual IDocParams& Doc() = 0;
   // error info/setting
   virtual bool HasError() const = 0;
   virtual void SetError(const string& sMessage) = 0;
};

DECLARE_INTERFACE(IMathItemBuilder) {
   virtual ~IMathItemBuilder() {}
   virtual bool CanTakeCommand(PCSTR szCmd) const = 0;
   virtual CMathItem* BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) = 0;
};
