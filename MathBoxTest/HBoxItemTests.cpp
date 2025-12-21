#include "stdafx.h"
#include "HBoxItem.h"
#include "MockMathItem.h"
#include <algorithm>

namespace HBoxItemTests
{
   TEST_CLASS(HBoxItemTests) {
      CMockDocParams     m_Doc;
      CMathStyle         m_msD;     //display style
      vector<CMathItem*> m_vItems;  //dnd - HBox takes ownership!
      vector<uint8_t>    m_vGlueTypes;
      float m_fShrink{ 0.0f };
      float m_fGluesNorm{ 0.0f };
      float m_fStretch{ 0.0f };
      //
      TEST_METHOD_INITIALIZE(Init) {
         m_vItems.clear();
         //$$x+y=\max\{x,y\}+\min\{x,y\}$$ (TeX Book, p.170)
         const EnumTexAtom _aAtoms[17] = {
            etaORD, etaBIN, etaORD, etaREL, etaOP,
            etaOPEN, etaORD, etaPUNCT, etaORD, etaCLOSE, etaBIN,
            etaOP,
            etaOPEN, etaORD, etaPUNCT, etaORD, etaCLOSE
         };
         const int32_t _aAtomSX[17] = { //in EM!
            (572 + 49 + 25), (778 + 56 + 56), (490 + 29 + 0), (778 + 56 + 56), ((833 + 32 + 20) + (500 + 32 + 17) + (528 + 12 + 12)),
            (500 + 75 + 75), (572 + 49 + 25),(278 + 86 + 65), (490 + 29 + 0), (500 + 75 + 75), (778 + 56 + 56),
            ((833 + 32 + 20) + (278 + 33 + 31) + (556 + 32 + 21)),
            (500 + 75 + 75), (572 + 49 + 25), (278 + 86 + 65), (490 + 29 + 0), (500 + 75 + 75)
         };
         for (int nIdx = 0; nIdx < _countof(_aAtoms); ++nIdx) {
            CMockMathItem* pMI = new CMockMathItem(m_Doc, _aAtoms[nIdx], m_msD);
            pMI->Box().nHeight = otfUnitsPerEm;
            pMI->Box().nAscent = otfAscent;
            pMI->Box().nAdvWidth = _aAtomSX[nIdx];
            m_vItems.push_back(pMI);
         }
         //expected glues data
         m_vGlueTypes = { 2,2,3,3,0,0,0,1,0,2,2,0,0,0,1,0 };
         m_fGluesNorm = m_fShrink = m_fStretch = 0.0f;
         for (size_t i = 0; i < m_vGlueTypes.size(); ++i) {
            switch (m_vGlueTypes[i]) {
            case 1: m_fGluesNorm += THINMUSKIP; break;
            case 2: m_fGluesNorm += MEDMUSKIP; m_fShrink += MEDMUSKIP_SHRINK; m_fStretch += MEDMUSKIP_STRETCH;  break;
            case 3: m_fGluesNorm += THICKMUSKIP; m_fStretch += THICKMUSKIP_STRETCH;  break;
            }
         }
         m_fGluesNorm = MU2EM(m_fGluesNorm);
         m_fShrink = MU2EM(m_fShrink);
         m_fStretch = MU2EM(m_fStretch);
      }
      TEST_METHOD(TestAutoBox_NoInfGlues_RetTrue) {
         CHBoxItem hbox(m_Doc, m_msD); //auto width
         for (CMathItem* pMI : m_vItems) {
            bool bRes = hbox.AddItem(pMI);
            Assert::IsTrue(bRes);
         }
         // Test first item set m_Box correctly
         Assert::AreEqual(hbox.Box().Width(), m_vItems.front()->Box().Width());
         Assert::AreEqual(hbox.Box().Height(), m_vItems.front()->Box().Height());
         // check hbox content info
         uint32_t nItems, nActGlues;
         hbox.ContentInfo(nItems, nActGlues);
         Assert::AreEqual(nItems, (uint32_t)m_vItems.size());
         Assert::AreEqual(nActGlues, (uint32_t)std::count_if(m_vGlueTypes.begin(), m_vGlueTypes.end(), 
            [](uint8_t nT) {return nT == 0; }) );
         uint32_t nItemsSX = 0;
         for (CMathItem* pMI : m_vItems) {
            nItemsSX += pMI->Box().Width();
         }
         hbox.Update(); //auto-> Norm
         // Checks
         const STexGlue* pGlue = hbox.GetGlue();
         Assert::IsNotNull(pGlue);
         Assert::AreEqual(pGlue->nShrinkOrder, (uint16_t)0);
         Assert::AreEqual(pGlue->nShrinkOrder, (uint16_t)0);
         Assert::AreEqual(int32_t(pGlue->fActual*100), int32_t(100*(nItemsSX + m_fGluesNorm)));
         Assert::AreEqual(pGlue->fActual, pGlue->fNorm);
         Assert::AreEqual(pGlue->fShrinkCapacity, m_fShrink);
         Assert::AreEqual(pGlue->fStretchCapacity, m_fStretch);
      }
      TEST_METHOD(TestFixedBox_NoInfGlues_RetFalse) {
         // Add items until possible
         const float fWidth = 10000.0f; //box width
         CHBoxItem hbox(m_Doc, m_msD, fWidth);
         for (CMathItem* pMI : m_vItems) {
            bool bRes = hbox.AddItem(pMI);
            if (!bRes) {
               uint32_t nItems, nActGlues;
               hbox.ContentInfo(nItems, nActGlues);
               Assert::AreEqual(12u, nItems);
               const STexGlue* pGlue = hbox.GetGlue();
               Assert::IsTrue(pGlue->fNorm - pGlue->fShrinkCapacity > fWidth);
               return;
            }
         }
         Assert::Fail();
      }
      TEST_METHOD(TestFixedBox_NoInfGlues_Stretch) {
         //\hbox to 100pt {A\hskip 2pt plus 2pt{B}\hskip 2pt plus 3pt{B}}
         const float fWidth = 10000.0; //box width
         CHBoxItem hbox(m_Doc, m_msD, fWidth);
         //A w = 750+32+33 = 
         CMockMathItem* pLetterA = new CMockMathItem(m_Doc, etaORD, m_msD);
         pLetterA->Box().nHeight = otfUnitsPerEm;
         pLetterA->Box().nAscent = otfAscent;
         pLetterA->Box().nAdvWidth = 750;
         bool bRet = hbox.AddItem(pLetterA); //A
         Assert::IsTrue(bRet);
         //\hskip 2pt plus 2pt
         STexGlue glueDef = { 0,0, 200.0f, 0.0f, 200.0f, 200.0f };
         bRet = hbox.AddItem(new CGlueItem(m_Doc, glueDef, m_msD));
         Assert::IsTrue(bRet);
         //B w = 708+36+57
         CMockMathItem* pLetterB = new CMockMathItem(m_Doc, etaORD, m_msD);
         pLetterB->Box().nHeight = otfUnitsPerEm;
         pLetterB->Box().nAscent = otfAscent;
         pLetterB->Box().nAdvWidth = 708;
         bRet = hbox.AddItem(pLetterB); //B
         Assert::IsTrue(bRet);
         //\hskip 2pt plus 3pt
         glueDef = { 0,0, 300.0f, 0.0f, 200.0f, 200.0f };
         bRet = hbox.AddItem(new CGlueItem(m_Doc, glueDef, m_msD));
         Assert::IsTrue(bRet);
         //B2 
         CMockMathItem* pLetterB2 = new CMockMathItem(m_Doc, etaORD, m_msD);
         pLetterB2->Box().nHeight = otfUnitsPerEm;
         pLetterB2->Box().nAscent = otfAscent;
         pLetterB2->Box().nAdvWidth = 708;
         bRet = hbox.AddItem(pLetterB2); //second B
         Assert::IsTrue(bRet);

         hbox.Update(); //stretch to fWidth
         //check items
         const vector<CMathItem*>& vItems = hbox.Items();
         Assert::AreEqual(vItems.front()->Box().Left(), 0);
         Assert::AreEqual(vItems[2]->Box().Left(), 3924);
         Assert::AreEqual(vItems[4]->Box().Left(), 10000-708);
      }
      TEST_METHOD(TestFixedBox_InfStretch) {
         //\hbox to 30pt {A\hskip 2pt plus 1fil minus 20pt{B}}
         const float fWidth = 3000.0; //box width
         CHBoxItem hbox(m_Doc, m_msD, fWidth);
         //A w = 750/+32+33
         CMockMathItem* pLetterA = new CMockMathItem(m_Doc, etaORD, m_msD);
         pLetterA->Box().nHeight = otfUnitsPerEm;
         pLetterA->Box().nAscent = otfAscent;
         pLetterA->Box().nAdvWidth = 750;
         bool bRet = hbox.AddItem(pLetterA); //A
         Assert::IsTrue(bRet);
         //\hskip 2pt plus 1fil minus 20pt
         STexGlue glueDef{ 1,0, 1.0f, 2000.0f, 200.0f, 200.0f };
         bRet = hbox.AddItem(new CGlueItem(m_Doc, glueDef, m_msD));
         Assert::IsTrue(bRet);
         //B w = 708+36+50 = 794
         CMockMathItem* pLetterB = new CMockMathItem(m_Doc, etaORD, m_msD);
         pLetterB->Box().nHeight = otfUnitsPerEm;
         pLetterB->Box().nAscent = otfAscent;
         pLetterB->Box().nAdvWidth = 708;
         bRet = hbox.AddItem(pLetterB); //B
         Assert::IsTrue(bRet);
         hbox.Update(); //stretch to 3000
         // Checks
         const STexGlue* pGlue = hbox.GetGlue();
         Assert::IsNotNull(pGlue);
         Assert::AreEqual(pGlue->nShrinkOrder, (uint16_t)0);
         Assert::AreEqual(pGlue->nStretchOrder, (uint16_t)0);
         Assert::AreEqual(pGlue->fActual, fWidth);
         //check items
         const vector<CMathItem*>& vItems = hbox.Items();
         Assert::AreEqual(vItems.front()->Box().Left(), 0);
         Assert::AreEqual(vItems.back()->Box().Right(), (int32_t)fWidth);
      }
      TEST_METHOD(TestFixedBox_InfStretch1) {
         // \hbox to 100pt{\hfil{A}\hskip 2pt plus 1fil minus 20pt{B} }
         const float fWidth = 10000.0; //box width
         CHBoxItem hbox(m_Doc, m_msD, fWidth);
         //\hfil
         STexGlue glueDef{ 1,0, 1.0f, 0.0f, 0.0f, 0.0f };
         bool bRet = hbox.AddItem(new CGlueItem(m_Doc, glueDef, m_msD));
         Assert::IsTrue(bRet);
         //A w = 750+32+33 = 
         CMockMathItem* pLetterA = new CMockMathItem(m_Doc, etaORD, m_msD);
         pLetterA->Box().nHeight = otfUnitsPerEm;
         pLetterA->Box().nAscent = otfAscent;
         pLetterA->Box().nAdvWidth = 750; 
         bRet = hbox.AddItem(pLetterA); //A
         Assert::IsTrue(bRet);
         //\hskip 2pt plus 1fil minus 20pt
         glueDef = { 1,0, 1.0f, 2000.0f, 200.0f, 200.0f };
         bRet = hbox.AddItem(new CGlueItem(m_Doc, glueDef, m_msD));
         Assert::IsTrue(bRet);
         //B w = 708+36+57
         CMockMathItem* pLetterB = new CMockMathItem(m_Doc, etaORD, m_msD);
         pLetterB->Box().nHeight = otfUnitsPerEm;
         pLetterB->Box().nAscent = otfAscent;
         pLetterB->Box().nAdvWidth = 708;
         bRet = hbox.AddItem(pLetterB); //B
         Assert::IsTrue(bRet);
         const STexGlue* pGlue = hbox.GetGlue();
         Assert::IsNotNull(pGlue);
         Assert::AreEqual(pGlue->nStretchOrder, (uint16_t)1);
         Assert::AreEqual(pGlue->fStretchCapacity, 2.0f);
         hbox.Update();
         // Checks
         Assert::AreEqual(pGlue->nShrinkOrder, (uint16_t)0);
         Assert::AreEqual(pGlue->nStretchOrder, (uint16_t)0); //reset on Update!
         Assert::AreEqual(pGlue->fActual, fWidth);
         // Check items
         const vector<CMathItem*>& vItems = hbox.Items();
         Assert::AreEqual(vItems.front()->Box().Left(), 0);
         Assert::AreEqual(vItems.front()->Box().Right(), 4171);//=(10000-(750+200+708))/2
      }
      TEST_METHOD(TestFixedBox_InfShrink) {
         // \hbox to 12pt{A\hskip 100pt minus 1fil{B}}
         const float fWidth = 1200.0; //box width
         CHBoxItem hbox(m_Doc, m_msD, fWidth);
         // A w = 750
         CMockMathItem* pLetterA = new CMockMathItem(m_Doc, etaORD, m_msD);
         pLetterA->Box().nHeight = otfUnitsPerEm;
         pLetterA->Box().nAscent = otfAscent;
         pLetterA->Box().nAdvWidth = 750;
         bool bRet = hbox.AddItem(pLetterA); //A
         Assert::IsTrue(bRet);
         // \hskip 100pt minus 1fil
         STexGlue glueDef{ 0,1, 0.0f, 1.0f, 10000.0f, 10000.0f };
         bRet = hbox.AddItem(new CGlueItem(m_Doc, glueDef, m_msD));
         Assert::IsTrue(bRet);// inf shrink order!
         // B w = 708+36+57
         CMockMathItem* pLetterB = new CMockMathItem(m_Doc, etaORD, m_msD);
         pLetterB->Box().nHeight = otfUnitsPerEm;
         pLetterB->Box().nAscent = otfAscent;
         pLetterB->Box().nAdvWidth = 708;
         bRet = hbox.AddItem(pLetterB);
         Assert::IsTrue(bRet);
         hbox.Update();
         // Check items
         const vector<CMathItem*>& vItems = hbox.Items();
         Assert::AreEqual(vItems.front()->Box().Left(), 0);
         Assert::AreEqual(vItems[2]->Box().Left(), (int32_t)fWidth- 708);
      }
      TEST_METHOD(TestFixedBox_InfShrink1) {
         // \hbox to 23pt{A\hskip 20pt minus 2fil{B}\hskip 15pt minus 1fil{C}}
         // Only the 2fil glue shrinks, 1fil glue must stays at 15pt

         const float fWidth = 2300.0f; // 23pt box width
         CHBoxItem hbox(m_Doc, m_msD, fWidth);

         // A w = 750
         CMockMathItem* pLetterA = new CMockMathItem(m_Doc, etaORD, m_msD);
         pLetterA->Box().nHeight = otfUnitsPerEm;
         pLetterA->Box().nAscent = otfAscent;
         pLetterA->Box().nAdvWidth = 750;
         bool bRet = hbox.AddItem(pLetterA); // A
         Assert::IsTrue(bRet);

         // \hskip 20pt minus 2fil
         STexGlue glueDef{ 0,2, 0.0f, 1.0f, 2000.0f, 2000.0f };
         bRet = hbox.AddItem(new CGlueItem(m_Doc, glueDef, m_msD));
         Assert::IsTrue(bRet);

         // B w = 708
         CMockMathItem* pLetterB = new CMockMathItem(m_Doc, etaORD, m_msD);
         pLetterB->Box().nHeight = otfUnitsPerEm;
         pLetterB->Box().nAscent = otfAscent;
         pLetterB->Box().nAdvWidth = 708;
         bRet = hbox.AddItem(pLetterB); // B
         Assert::IsTrue(bRet);

         // \hskip 15pt minus 1fil 
         glueDef = { 0,1, 0.0f, 1.0f, 1500.0f, 1500.0f };
         bRet = hbox.AddItem(new CGlueItem(m_Doc, glueDef, m_msD));
         Assert::IsTrue(bRet);

         // C w = 708
         CMockMathItem* pLetterC = new CMockMathItem(m_Doc, etaORD, m_msD);
         pLetterC->Box().nHeight = otfUnitsPerEm;
         pLetterC->Box().nAscent = otfAscent;
         pLetterC->Box().nAdvWidth = 708;
         bRet = hbox.AddItem(pLetterC); // C
         Assert::IsTrue(bRet);

         hbox.Update(); // shrink to fWidth

         // Checks - only 2fil glue shrinks, 1fil stays at natural size
         const vector<CMathItem*>& vItems = hbox.Items();
         Assert::AreEqual(vItems[0]->Box().Left(), 0);        // A at left
         Assert::AreEqual(vItems[4]->Box().Right(), (int32_t)fWidth); // C at right edge

         // B position after 2fil glue shrinks
         Assert::AreEqual(vItems[2]->Box().Left(), 750 - 1366); // A_width + shrunk_glue1
      }

      TEST_METHOD(TestFixedBox_InfMix) {
         // \hbox to 15pt{A\hskip 10pt plus 1fil minus 2fil{B}\hskip 5pt plus 0.5fil minus 0.5fil{C}}

         const float fWidth = 1500.0f; // 15pt box width
         CHBoxItem hbox(m_Doc, m_msD, fWidth);

         // A w = 750
         CMockMathItem* pLetterA = new CMockMathItem(m_Doc, etaORD, m_msD);
         pLetterA->Box().nHeight = otfUnitsPerEm;
         pLetterA->Box().nAscent = otfAscent;
         pLetterA->Box().nAdvWidth = 750;
         bool bRet = hbox.AddItem(pLetterA); // A
         Assert::IsTrue(bRet);

         // \hskip 10pt plus 1fil minus 2fil
         STexGlue glueDef{ 1,2, 1.0f, 1.0f, 1000.0f, 1000.0f };
         bRet = hbox.AddItem(new CGlueItem(m_Doc, glueDef, m_msD));
         Assert::IsTrue(bRet);

         // B w = 708
         CMockMathItem* pLetterB = new CMockMathItem(m_Doc, etaORD, m_msD);
         pLetterB->Box().nHeight = otfUnitsPerEm;
         pLetterB->Box().nAscent = otfAscent;
         pLetterB->Box().nAdvWidth = 708;
         bRet = hbox.AddItem(pLetterB); // B
         Assert::IsTrue(bRet);

         // \hskip 5pt plus 0.5fil minus 0.5fil
         glueDef = { 1,1, 0.5f, 0.5f, 500.0f, 500.0f };
         bRet = hbox.AddItem(new CGlueItem(m_Doc, glueDef, m_msD));
         Assert::IsTrue(bRet);

         // C w = 722
         CMockMathItem* pLetterC = new CMockMathItem(m_Doc, etaORD, m_msD);
         pLetterC->Box().nHeight = otfUnitsPerEm;
         pLetterC->Box().nAscent = otfAscent;
         pLetterC->Box().nAdvWidth = 722;
         bRet = hbox.AddItem(pLetterC); // C
         Assert::IsTrue(bRet);

         hbox.Update(); // shrink to fWidth (box narrower than natural)

         // Checks - highest shrink order (2fil) dominates
         const vector<CMathItem*>& vItems = hbox.Items();
         Assert::AreEqual(vItems[0]->Box().Left(), 0);        // A at left
         Assert::AreEqual(vItems[4]->Box().Right(), (int32_t)fWidth); // C at right edge

         // B position after only first glue (2fil) shrinks
         Assert::AreEqual(vItems[2]->Box().Left(), 750 - 1180); // A_width + shrunk_glue1
      }

   };
}
