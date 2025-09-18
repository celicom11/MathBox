#pragma once
#include "MathItem.h"

//single glyph in a glyph run
struct SBoundsF {
   float fMinX, fMinY, fMaxX, fMaxY;
};
struct SBounds {
   INT32 nMinX, nMinY, nMaxX, nMaxY;
};
struct SDWGlyph {
   UINT16               index{ 0 };       //glyph index in the font
   UINT32               codepoint{ 0 };   //Unicode codepoint, may be 0!
   DWRITE_GLYPH_METRICS metrics;          //glyph metrics
};
class CGlyphRun : public IDWriteGeometrySink
{
   IDWriteFontFace*              m_pFontFace{NULL};      //external, dnd
   D2D1_POINT_2F                 m_ptfCur{ 0,0 };       //tmp. outline tracker
   SBounds                       m_Bounds{ 0,0,0,0 };   //bounds around base_origin in design units
   SBoundsF                      m_BoundsF{ 0,0,0,0 };  //tmp. outline box
   vector<SDWGlyph>              m_vGlyphs;             //glyphs in the run
   vector<DWRITE_GLYPH_OFFSET>   m_vOffsets;            //optional, glyph offsets for complex geometry
public:
   //CTORS
   CGlyphRun(IDWriteFontFace* pFontFace): 
      m_pFontFace(pFontFace) {};
   CGlyphRun(CGlyphRun&& other) noexcept :
      m_pFontFace(other.m_pFontFace),
      m_Bounds(other.m_Bounds), m_vGlyphs(std::move(other.m_vGlyphs)) {
   }
   //ATTS
   bool IsEmpty() const { return m_vGlyphs.empty(); }
   const SBounds& Bounds() const {
      //convert to int32_t bounds
      return m_Bounds;
   }
   const vector<SDWGlyph>& Glyphs() const { return m_vGlyphs; }
   //METHODS
   void Clear() {
      m_vGlyphs.clear();
      m_Bounds = { 0,0,0,0 };
   }
   HRESULT SetGlyphs(vector<UINT32> vUnicode) {
      vector<UINT16> vIndices(vUnicode.size());
      HRESULT hr = m_pFontFace->GetGlyphIndicesW(vUnicode.data(), (UINT32)vUnicode.size(), vIndices.data());
      if (FAILED(hr))
         return hr;
      return SetGlyphIndices(vIndices, vUnicode);
   }
   HRESULT SetGlyphIndices(vector<UINT16> vIndices, vector<UINT32> vUnicode);
   void Draw(const SDWRenderInfo& dwri, D2D1_POINT_2F ptfBaseOrigin, float fScale = 1.0f);
   //IUnknown stub
   STDMETHOD_(ULONG, AddRef)() override { return 1; }
   STDMETHOD_(ULONG, Release)() override { return 1; }
   STDMETHOD(QueryInterface)(REFIID riid, OUT void __RPC_FAR* __RPC_FAR* ppvObject) override {
      if (nullptr == ppvObject)
         return E_INVALIDARG;
      if (riid == __uuidof(IUnknown) || riid == __uuidof(IDWriteGeometrySink)) {
         *ppvObject = static_cast<IDWriteGeometrySink*>(this);
         return S_OK;
      }
      else {
         *ppvObject = nullptr;
         return E_NOINTERFACE;
      }
   }
   //IDWriteGeometrySink
   STDMETHOD_(void, BeginFigure)(D2D1_POINT_2F startPoint, D2D1_FIGURE_BEGIN) override {
      m_ptfCur = startPoint;
      UpdateInkBox_(startPoint);
   }
   STDMETHOD_(void, AddLines)(const D2D1_POINT_2F* points, UINT32 count) override {
      for (UINT32 i = 0; i < count; ++i) {
         UpdateInkBox_(points[i]);
      }
      m_ptfCur = points[count - 1];
   }
   STDMETHOD_(void, AddBeziers)(const D2D1_BEZIER_SEGMENT* beziers, UINT32 count) override {
      for (UINT32 i = 0; i < count; ++i) {
         float fXMin, fXMax;
         GetCubicBezierMinMax_(m_ptfCur.x, beziers[i].point1.x, beziers[i].point2.x, beziers[i].point3.x, fXMin, fXMax);
         float fYMin, fYMax;
         GetCubicBezierMinMax_(m_ptfCur.y, beziers[i].point1.y, beziers[i].point2.y, beziers[i].point3.y, fYMin, fYMax);
         UpdateInkBox_({ fXMin , fYMin });
         UpdateInkBox_({ fXMax , fYMax });
         UpdateInkBox_(beziers[i].point3);
         m_ptfCur = beziers[i].point3;
      }
   }
   STDMETHOD_(void, EndFigure)(D2D1_FIGURE_END) override {}
   STDMETHOD(Close)() override { return S_OK; }
   STDMETHOD_(void, SetFillMode)(D2D1_FILL_MODE) override {}
   STDMETHOD_(void, SetSegmentFlags)(D2D1_PATH_SEGMENT) override {}
private:
   void UpdateInkBox_(const D2D1_POINT_2F& ptf);
   void GetCubicBezierMinMax_(float fP0, float fP1, float fP2, float fP3, OUT float& fMin, OUT float& fMax);
};