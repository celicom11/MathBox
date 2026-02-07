#pragma once
struct SBoundsF {
   float fMinX, fMinY, fMaxX, fMaxY;
};
class CD2DGeomSink : public IDWriteGeometrySink {
//DATA
   D2D1_POINT_2F                 m_ptfCur{ 0,0 };
   SBoundsF                      m_BoundsF{ 0,0,0,0 };
public:
//ATTS
   const SBoundsF& GetBounds() const { return m_BoundsF; }
   void ResetBounds() {
      m_BoundsF = { FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX };
   }
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
   void UpdateInkBox_(const D2D1_POINT_2F& ptf) {
      m_BoundsF.fMinX = min(m_BoundsF.fMinX, ptf.x);
      m_BoundsF.fMinY = min(m_BoundsF.fMinY, ptf.y);
      m_BoundsF.fMaxX = max(m_BoundsF.fMaxX, ptf.x);
      m_BoundsF.fMaxY = max(m_BoundsF.fMaxY, ptf.y);
   }
   void GetCubicBezierMinMax_(float fP0, float fP1, float fP2, float fP3, OUT float& fMin, OUT float& fMax);
};