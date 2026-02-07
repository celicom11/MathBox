#include "stdafx.h"
#include "HBoxItem.h"
#include "MockMathItem.h"

namespace GlueFactoryTests
{
   //static CGlueItem* CGlueItem::_Create is tested here
   TEST_CLASS(GlueFactoryTests) {
      CMockDocParams m_Doc;
      CMathStyle     m_ms;//default
      CMockMathItem* m_pMIOrd{ nullptr };
      CMockMathItem* m_pMIOp{ nullptr };
      CMockMathItem* m_pMIBin{ nullptr };
      CMockMathItem* m_pMIRel{ nullptr };
      CMockMathItem* m_pMIOpen{ nullptr };
      CMockMathItem* m_pMIClose{ nullptr };
      CMockMathItem* m_pMIPunct{ nullptr };
      CMockMathItem* m_pMIInner{ nullptr };
      CHBoxItem*     m_pMIHBox{ nullptr };
      CGlueItem*     m_pFinGlue{ nullptr };
      TEST_METHOD_INITIALIZE(Init) {
         m_pMIOrd = new CMockMathItem(m_Doc, etaORD, m_ms);
         m_pMIOp = new CMockMathItem(m_Doc, etaOP, m_ms);
         m_pMIBin = new CMockMathItem(m_Doc, etaBIN, m_ms);
         m_pMIRel = new CMockMathItem(m_Doc, etaREL, m_ms);
         m_pMIOpen = new CMockMathItem(m_Doc, etaOPEN, m_ms);
         m_pMIClose = new CMockMathItem(m_Doc, etaCLOSE, m_ms);
         m_pMIPunct = new CMockMathItem(m_Doc, etaPUNCT, m_ms);
         m_pMIInner = new CMockMathItem(m_Doc, etaINNER, m_ms);
         m_pMIHBox = new CHBoxItem(m_Doc, m_ms);
         STexGlue glueDef{ 0,0, 900.0f, 100.f, 100.f, 100.0f };  //finite glue [0,100,1000]
         m_pFinGlue = new CGlueItem(m_Doc, glueDef, m_ms);
      }
      TEST_METHOD_CLEANUP(DeInit) {
         delete m_pMIOrd; delete m_pMIOp; delete m_pMIBin; delete m_pMIRel;
         delete m_pMIOpen; delete m_pMIClose; delete m_pMIPunct; delete m_pMIInner;
         delete m_pMIHBox; delete m_pFinGlue;
      }
      TEST_METHOD(Test_DisplayMode_Glue) {
         // Verify CGlueItem::_Create() returns correct MuSkipType
         // Test all 8×8 combinations from ATOM_SPACING_MATRIX
         // Compare: Ord-Op, Op-Bin, Rel-Rel, etc.
         CGlueItem* pGlue = CGlueItem::_Create(m_pMIOrd, m_pMIOp, m_ms);
         Assert::IsNotNull(pGlue); //Thinmuskip
         Assert::AreEqual(MU2EM(THINMUSKIP), pGlue->GetGlue()->fNorm);
         Assert::AreEqual(0.0f, pGlue->GetGlue()->fShrinkCapacity);
         Assert::AreEqual(0.0f, pGlue->GetGlue()->fStretchCapacity);
         delete pGlue;
         pGlue = CGlueItem::_Create(m_pMIBin, m_pMIInner, m_ms);
         Assert::IsNotNull(pGlue); //Thinmuskip
         Assert::AreEqual(MU2EM(MEDMUSKIP), pGlue->GetGlue()->fNorm);
         Assert::AreEqual(MU2EM(MEDMUSKIP_SHRINK), pGlue->GetGlue()->fShrinkCapacity);
         Assert::AreEqual(MU2EM(MEDMUSKIP_STRETCH), pGlue->GetGlue()->fStretchCapacity);
         delete pGlue;
         pGlue = CGlueItem::_Create(m_pMIClose, m_pMIRel, m_ms);
         Assert::IsNotNull(pGlue); //Thinmuskip
         Assert::AreEqual(MU2EM(THICKMUSKIP), pGlue->GetGlue()->fNorm);
         Assert::AreEqual(0.0f, pGlue->GetGlue()->fShrinkCapacity);
         Assert::AreEqual(MU2EM(THICKMUSKIP_STRETCH), pGlue->GetGlue()->fStretchCapacity);
         delete pGlue;
      }
      TEST_METHOD(Test_DisplayMode_NoGlue) {
         CGlueItem* pGlue = CGlueItem::_Create(m_pMIOp, m_pMIOpen, m_ms);
         Assert::IsNull(pGlue); 
         pGlue = CGlueItem::_Create(m_pMIBin, m_pMIBin, m_ms);
         Assert::IsNull(pGlue);
         pGlue = CGlueItem::_Create(m_pMIRel, m_pMIPunct, m_ms);
         Assert::IsNull(pGlue);
         pGlue = CGlueItem::_Create(m_pMIRel, m_pMIRel, m_ms);
         Assert::IsNull(pGlue);
         pGlue = CGlueItem::_Create(m_pMIOpen, m_pMIOrd, m_ms);
         Assert::IsNull(pGlue);
         pGlue = CGlueItem::_Create(m_pFinGlue, m_pMIBin, m_ms);
         Assert::IsNull(pGlue);
         pGlue = CGlueItem::_Create(m_pMIOp, m_pMIOpen, m_ms);
         Assert::IsNull(pGlue);
      }
      //Thinmuskip only!
      TEST_METHOD(Test_NonDisplayMode_Glue) {
         CMathStyle msS(etsScript, false);
         CGlueItem* pGlue = CGlueItem::_Create(m_pMIOrd, m_pMIOp, msS);
         Assert::IsNotNull(pGlue); 
         Assert::AreEqual(MU2EM(THINMUSKIP), pGlue->GetGlue()->fNorm);
         Assert::AreEqual(0.0f, pGlue->GetGlue()->fShrinkCapacity);
         Assert::AreEqual(0.0f, pGlue->GetGlue()->fStretchCapacity);
         delete pGlue;
         pGlue = CGlueItem::_Create(m_pMIOp, m_pMIOp, msS);
         Assert::IsNotNull(pGlue); //Thinmuskip
         Assert::AreEqual(MU2EM(THINMUSKIP), pGlue->GetGlue()->fNorm);
         Assert::AreEqual(0.0f, pGlue->GetGlue()->fShrinkCapacity);
         Assert::AreEqual(0.0f, pGlue->GetGlue()->fStretchCapacity);
         delete pGlue;
         pGlue = CGlueItem::_Create(m_pMIOp, m_pMIOrd, msS);
         Assert::IsNotNull(pGlue); //Thinmuskip
         Assert::AreEqual(MU2EM(THINMUSKIP), pGlue->GetGlue()->fNorm);
         Assert::AreEqual(0.0f, pGlue->GetGlue()->fShrinkCapacity);
         Assert::AreEqual(0.0f, pGlue->GetGlue()->fStretchCapacity);
         delete pGlue;
         pGlue = CGlueItem::_Create(m_pMIInner, m_pMIOp, msS);
         Assert::IsNotNull(pGlue); //Thinmuskip
         Assert::AreEqual(MU2EM(THINMUSKIP), pGlue->GetGlue()->fNorm);
         Assert::AreEqual(0.0f, pGlue->GetGlue()->fShrinkCapacity);
         Assert::AreEqual(0.0f, pGlue->GetGlue()->fStretchCapacity);
         delete pGlue;
      }
      TEST_METHOD(Test_NonDisplayMode_NoGlue) {
         CMathStyle msS(etsScript, false);
         CGlueItem* pGlue = CGlueItem::_Create(m_pMIOrd, m_pMIBin, msS);
         Assert::IsNull(pGlue);
         pGlue = CGlueItem::_Create(m_pMIOp, m_pMIRel, msS);
         Assert::IsNull(pGlue);
         pGlue = CGlueItem::_Create(m_pMIBin, m_pMIOp, msS);
         Assert::IsNull(pGlue);
         pGlue = CGlueItem::_Create(m_pMIRel, m_pMIOrd, msS);
         Assert::IsNull(pGlue);
         pGlue = CGlueItem::_Create(m_pMIClose, m_pMIBin, msS);
         Assert::IsNull(pGlue);
         pGlue = CGlueItem::_Create(m_pMIPunct, m_pMIRel, msS);
         Assert::IsNull(pGlue);
         pGlue = CGlueItem::_Create(m_pMIOp, m_pMIInner, msS);
         Assert::IsNull(pGlue);
         pGlue = CGlueItem::_Create(m_pMIInner, m_pMIOrd, msS);
         Assert::IsNull(pGlue);
      }

   };
}
