#include "stdafx.h"
#include "GlueItem.h"
#include "MockMathItem.h"

namespace GlueItemTests
{
   TEST_CLASS(GlueItemTests) {
      CMockDocParams m_Doc;
      TEST_METHOD(TestResizeByRatio_Order0) {
         STexGlue glueDef{ 0,0, 900.0f, 100.f, 100.f, 100.0f };  //finite glue [0,100,1000]
         CGlueItem gFinite(m_Doc, glueDef, CMathStyle()); //finite glue [0,100,1000]
         Assert::AreEqual(0, gFinite.Box().Top());
         Assert::AreEqual(0, gFinite.Box().Bottom());
         Assert::AreEqual(0, gFinite.Box().BaselineY());
         Assert::AreEqual(0, gFinite.Box().Left());
         Assert::AreEqual(100, gFinite.Box().Width());
         //half stretch
         gFinite.ResizeByRatio(0, 0.5f); 
         Assert::AreEqual(550, gFinite.Box().Width());
         gFinite.ResizeByRatio(1, 100.0f);//no effect
         Assert::AreEqual(550, gFinite.Box().Width());
         //half shrink
         gFinite.ResizeByRatio(0, -0.5f);
         Assert::AreEqual(50, gFinite.Box().Width());
         gFinite.ResizeByRatio(2, -110.5f); //no effect
         Assert::AreEqual(50, gFinite.Box().Width());
         //reset to normal
         gFinite.ResizeByRatio(0, 0.0f); 
         Assert::AreEqual(100, gFinite.Box().Width());
      }
      TEST_METHOD(TestResizeByRatio_Order1) {
         STexGlue glueDef{ 1,1, 3.0f, 2.f, 100.f, 100.0f };  //100 plus 3fil minus 2fil
         CGlueItem gFinite(m_Doc, glueDef, CMathStyle()); //finite glue [0,100,1000]
         Assert::AreEqual(0, gFinite.Box().Top());
         Assert::AreEqual(0, gFinite.Box().Bottom());
         Assert::AreEqual(0, gFinite.Box().BaselineY());
         Assert::AreEqual(0, gFinite.Box().Left());
         Assert::AreEqual(100, gFinite.Box().Width());
         //half stretch
         gFinite.ResizeByRatio(1, 10.0f);
         Assert::AreEqual(130, gFinite.Box().Width());
         gFinite.ResizeByRatio(0, 101.0f);//no effect
         Assert::AreEqual(130, gFinite.Box().Width());
         //half shrink
         gFinite.ResizeByRatio(1, -60.0f);
         Assert::AreEqual(-20, gFinite.Box().Width());
         gFinite.ResizeByRatio(2, -110.5f); //no effect
         Assert::AreEqual(-20, gFinite.Box().Width());
         //reset to normal
         gFinite.ResizeByRatio(0, 0.0f);
         Assert::AreEqual(100, gFinite.Box().Width());
      }
   };
}
