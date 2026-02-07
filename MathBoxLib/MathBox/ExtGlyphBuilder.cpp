#include "stdafx.h"
#include "ExtGlyphBuilder.h"
#include "WordItem.h"
#include "ContainerItem.h"
#include "RuleItem.h"

namespace //static helpers
{
   //TYPES
   struct SGlyphVariant {
      uint16_t nGlyphIdx{ 0 };            //glyph index 
      uint16_t nSize{ 0 };                //vertical or horizontal dimension in font units
   };
   struct SGlyphExtensionInfo {
      uint32_t nUnicode{ 0 };             // base glyph's unicode
      SGlyphVariant gvBottomLeftId;       // 
      SGlyphVariant gvMiddleId;           // optional middle part.If 0, use extender!
      SGlyphVariant gvTopRightId;         // 
      int16_t       nMaxOverlap{ 0 };     // max overlap of Top/Middle (or Tob/Bottom if no middle) parts
      // Extender/Filler info, two symmetric fillers if nMiddleId <> 0
      int16_t nExtenderLeft{ 0 };         //Left or Bottom (rel. to Baseline!);
                                          // filler boundaries, overlappi joint parts with otfMinConnectorOverlap
      int16_t nExtenderRight{ 0 };        //Right or Top;
      int16_t nExtender2Left{ 0 };        //2nd extender left/bottom;
      vector< SGlyphVariant> vVariants;   // usually non empty!
   };
   static const vector<SGlyphExtensionInfo> _vVerticalExtGlyphs {
      {0x0028, {2503,1495},{},{2505,1495},249,277,379,0, // (
         {{9, 997},{2367, 1095},{2389,1195},{2411,1445},{2433,1793},{2455,2093},{2477, 2393},{2499, 2991}}},
      {0x0029, {2508,1495},{},{2506,1495},249,496,598,0,  // )
         {{10, 997},{2368, 1095},{2390,1195},{2412,1445},{2434,1793},{2456,2093},{2478, 2393},{2500, 2991}}},
      {0x002F, {},{},{},0,0,0,0,  // /
         {{16, 1001},{2672, 1311},{2679,1717},{2686,2249},{2693,2945},{2700,3859},{2707, 5055},{2714, 6621}}},
      {0x005C, {},{},{},0,0,0,0,  // backslash
         {{61, 1001},{2673, 1311},{2680,1717},{2687,2249},{2694,2945},{2701,3859},{2708, 5055},{2715, 6621}}},
      {0x007C, {2721,1202},{},{2723,1202},601,100,178,0,  // | , divides , x2223
         {{93, 1001},{2677, 1203},{2684,1445},{2691,1735},{2698,2085},{2705,2503},{2712, 3005},{2719, 3607}}},

      {0x005B, {2527,1500},{},{2529,1500},500,321,381,0, // [
         {{60, 1001},{2373,1101},{2395,1201},{2417,1451},{2439,1801},{2461,2101},{2483,2401},{2525,3001}}},
      {0x005D, {2530,1500},{},{2532,1500},500,286,346,0, // ]
         {{62, 1001},{2374,1101},{2396,1201},{2418,1451},{2440,1801},{2462,2101},{2484,2401},{2526,3001}}},
      {0x007B, {2517,750},{2519,1500},{2520,750},374,400,502,0, // {
         {{92, 1001},{2371,1101},{2393,1201},{2415,1451},{2437,1801},{2459,2101},{2481,2401},{2515,3001}}},
      {0x007D, {2521,750},{2523,1500},{2524,750},374,400,502,0, // }
         {{94, 1001},{2372,1101},{2394,1201},{2416,1451},{2438,1801},{2460,2101},{2482,2401},{2516,3001}}},

      {0x2191, {1871,506},{},{1873,505},169,230,270,0,  // uparrow
         {{1867, 883},{1869, 1349}}},
      {0x2193, {1874,505},{},{1876,506},169,230,270,0,  // downarrow
         {{1868, 883},{1870, 1349}}},
      {0x2195, {1896,380},{},{1898,380},127,266,306,0,  // updownarrow
         {{1894, 1015},{1895, 1015}}}, //why same size??
      {0x21D1, {2113,505},{},{2115,504},168,209,243,403,  // Uparrow/arrowdblup, 2 fillers
         {{2109, 880},{2111, 1346}}},
      {0x21D3, {2116,504},{},{2118,505},168,209,243,403,  // Downarrow/arrowdbldown, 2 fillers
         {{2110, 880},{2112, 1346}}},
      {0x21D5, {2126,533},{},{2128,533},178,209,243,403,  // Updownarrow, 2 fillers
         {{2124, 880},{2125, 1346}}},

      {0x2016, {2724,1202},{},{2726,1202},601,57,336,290, // dblverticalbar(x2016), 2 fillers of 79 units!
         {{2671, 1001},{2678,1203},{2685,1445},{2692,1735},{2699,2085},{2706,2503},{2713,3005},{2720,3607}}},
      {0x2225, {2724,1202},{},{2726,1202},601,57,336,290, // parallel,              2 fillers of 79 units!
         {{2671, 1001},{2678,1203},{2685,1445},{2692,1735},{2699,2085},{2706,2503},{2713,3005},{2720,3607}}},

      {0x2308, {},{},{2529,1000},500,321,381,0, // lceil
         {{2353, 1001},{2375,1101},{2397,1201},{2419,1451},{2441,1801},{2463,2101},{2485,2401},{2533,3001}}},
      {0x2309, {},{},{2532,1000},500,286,346,0, // rceil
         {{2354, 1001},{2376,1101},{2398,1201},{2420,1451},{2442,1801},{2464,2101},{2486,2401},{2534,3001}}},
      {0x230A, {2527,1000},{},{},500,321,381,0, // lfloor
         {{2355, 1001},{2377,1101},{2399,1201},{2421,1451},{2443,1801},{2465,2101},{2487,2401},{2535,3001}}},
      {0x230B, {2530,1000},{},{},500,286,346,0, // rfloor
         {{2356, 1001},{2378,1101},{2400,1201},{2422,1451},{2444,1801},{2466,2101},{2488,2401},{2536,3001}}},

      {0x27E8, {},{},{},0,0,0,0, // LEFT ANGLE BRACKET
         {{2579, 1001},{2583,1101},{2587,1201},{2591,1451},{2595,1801},{2599,2101},{2603,2401},{2607,3001}}},
      {0x27E9, {},{},{},0,0,0,0, // RIGHT ANGLE BRACKET
         {{2580, 1001},{2584,1101},{2588,1201},{2592,1451},{2596,1801},{2600,2101},{2604,2401},{2608,3001}}},
      {0x27EA, {},{},{},0,0,0,0, // LEFT DOUBLE ANGLE BRACKET
         {{2581, 1001},{2585,1101},{2589,1201},{2593,1451},{2597,1801},{2601,2101},{2605,2401},{2609,3001}}},
      {0x27EB, {},{},{},0,0,0,0, // RIGHT DOUBLE ANGLE BRACKET
         {{2582, 1001},{2586,1101},{2590,1201},{2594,1451},{2598,1801},{2602,2101},{2606,2401},{2610,3001}}},
      {0x27EE, {2509,1526},{},{2511,1526},499,251,353,0, // lgroup
         {{2351, 1025},{2369,1127},{2391,1229},{2413,1483},{2435,1837},{2457,2141},{2479,2445},{2501,3053}}},
      {0x27EF, {2512,1526},{},{2514,1526},499,294,396,0, // rgroup
         {{2352, 1025},{2370,1127},{2392,1229},{2414,1483},{2436,1837},{2458,2141},{2480,2445},{2502,3053}}},
      {0x27E6, {2539,1526},{},{2541,1526},500,353,423,654, // lBrack, special case: 2 fillers of 70 units!
         {{2357, 1001},{2379,1101},{2401,1201},{2423,1451},{2445,1801},{2467,2101},{2489,2401},{2537,3001}}},
      {0x27E7, {2542,1526},{},{2544,1526},500,293,363,594, // rBrack, special case: 2 fillers of 70 units!
         {{2358, 1001},{2380,1101},{2402,1201},{24234,1451},{2446,1801},{2468,2101},{2490,2401},{2538,3001}}},
   };
   static const vector<SGlyphExtensionInfo> _vHorizontalExtGlyphs{
      {0x0305, {2257,189},{},{2259,189},95,630,670,0, // overlinecmb
         {{2246, 393},{2256, 569}}},
      {0x0332, {2249,189},{},{2251,189},95,-143,-103,0, // lowlinecmb
         {{2244, 393},{2248, 569}}},
      {0x030C, {},{},{},0,0,0,0,   // caroncmb
         {{2268, 365},{2278, 645},{2288, 769},{2298, 920},{2308, 1101},{2318, 1321},{2328, 1582},{2338, 1897}}},
      {0x0302, {},{},{},0,0,0,0,   // circumflexcmb
         {{2270, 365},{2280, 645},{2290, 769},{2300, 920},{2310, 1101},{2320, 1321},{2330, 1582},{2340, 1897}}},
      {0x032D, {},{},{},0,0,0,0,   // circumflexbelowcmb
         {{2271, 365},{2281, 645},{2291, 769},{2301, 920},{2311, 1101},{2321, 1321},{2331, 1582},{2341, 1897}}},
      {0x0303, {},{},{},0,0,0,0,   // tildecomb
         {{2272, 371},{2282, 653},{2292, 779},{2302, 932},{2312, 1116},{2322, 1336},{2332, 1600},{2342, 1916}}},
      {0x330, {},{},{},0,0,0,0,    // tildebelowcmb
         {{2273, 371},{2283, 653},{2293, 779},{2303, 932},{2313, 1116},{2323, 1336},{2333, 1600},{2343, 1916}}},
      
      {0x20D0, {1808,208},{},{1810,208},70,601,631,0, // overleftharpoon
         {{1804, 423},{1806, 556}}},
      {0x20D1, {1811,208},{},{1813,208},70,601,631,0, // overleftharpoon
         {{1805, 423},{1807, 556}}},

      {0x20D6, {1820,205},{},{1822,205},69,601,631,0, // overleftarrow
         {{1816, 417},{1818, 548}}},
      {0x20D7, {1823,205},{},{1825,205},69,601,631,0, // overrightarrow (aka vec) 
         {{1817, 417},{1819, 548}}},
      {0x20E1, {1828,226},{},{1830,226},76,601,631,0, // overleftrightarrow
         {{1826, 471},{1827, 604}}},
      {0x23DE, {2547,1002},{2549,2003},{2550,1001},497,622,724,0,   // overbrace
         {{2359, 493},{2381, 994},{2403, 1495},{2425, 1997},{2447, 2499},{2469, 3001},{2491, 3503},{2545, 4007},}},
      {0x23DF, {2551,1002},{2553,2003},{2554,1001},497,-294,-192,0, // underbrace
         {{2360, 493},{2382, 994},{2404, 1495},{2426, 1997},{2448, 2499},{2470, 3001},{2492, 3503},{2546, 4007},}},
      {0x23B4, {2565,1493},{},{2567,1493},498,712,772,0,            // overbracket
         {{2363, 361},{2385, 736},{2407, 1111},{2429,1486},{2451, 1861},{2473, 2236},{2495, 2611},{2563, 2986},}},
      {0x23B5, {2568,1493},{},{2570,1493},498,-342,-282,0,          // underbracket
         {{2364, 361},{2386, 736},{2408, 1111},{2430,1486},{2452, 1861},{2474, 2236},{2496, 2611},{2564, 2986},}},
      {0x23DC, {2557,2016},{},{2559,2016},497,694,796,0,            // overparen/overgroup
         {{2361, 505},{2383, 1007},{2405, 1509},{2427,2013},{2449, 2517},{2471, 3021},{2493, 3525},{2555, 4033},}},
      {0x23DD, {2560,2016},{},{2562,2016},497,-366,-264,0,          // underparen
         {{2362, 505},{2384, 1007},{2406, 1509},{2428,2013},{2450, 2517},{2472, 3021},{2494, 3525},{2556, 4033},}},
      {0x23E0, {2573,2041},{},{2575,2041},680,771,873,0,            // obrbrak
         {{2365, 547},{2387, 1049},{2409, 1551},{2431,2057},{2453, 2565},{2475, 3069},{2497, 3575},{2571, 4083},}},
      {0x23E1, {2576,2041},{},{2578,2041},680,-443,-341,0,          // ubrbrak
         {{2366, 547},{2388, 1049},{2410, 1551},{2432,2057},{2454, 2565},{2476, 3069},{2498, 3575},{2572, 4083},}},
      {0x21D2, {2106,533},{},{2108,533},178,173,213,327,             // arrowdblright, 2 fillers of 40units !
         {{2100, 880},{2102, 1346}}}
   };
   int _FIndGEIIndex(uint32_t nUnicode, const vector<SGlyphExtensionInfo>& vExtGlyphs) {
      for (int nIdx = 0; nIdx < vExtGlyphs.size(); ++nIdx) {
         if (vExtGlyphs[nIdx].nUnicode == nUnicode)
            return nIdx;
      }
      return -1;
   }
   CWordItem* _BuildSingleVerticalGlyph(IDocParams& doc, uint16_t nGlyphIdx,
                                       const CMathStyle& style, EnumTexAtom eAtom, float fUserScale) {
      CWordItem* pRet = new CWordItem(doc, FONT_LMM, style,eacWORD, fUserScale);
      pRet->SetAtom(eAtom);
      pRet->SetGlyphIndexes({ nGlyphIdx });
      return pRet;
   }
   CWordItem* _BuildSingleHorizontalGlyph(IDocParams& doc, uint16_t nGlyphIdx, const CMathStyle& style,
                                          float fUserScale) {
      CWordItem* pRet = new CWordItem(doc, FONT_LMM, style, eacUNK, fUserScale);
      pRet->SetGlyphIndexes({ nGlyphIdx });
      return pRet;
   }
   EnumTexAtom _GetAtom(EnumGlyphClass eClass) {
      EnumTexAtom eAtom = etaORD;//default
      switch (eClass) {
      case egcLOP: eAtom = etaOP; break;
      case egcBin: eAtom = etaBIN; break;
      case egcRel: eAtom = etaREL; break;
      case egcOpen: eAtom = etaOPEN; break;
      case egcClose: eAtom = etaCLOSE; break;
      case egcPunct: eAtom = etaPUNCT; break;
      }
      return eAtom;
   }
}
// CExtGlyphBuilder
//for vertical/delimiters only
uint16_t CExtGlyphBuilder::GlyphIndexByVariantIdx(uint32_t nUnicode, uint16_t nVariantIdx) {
   _ASSERT_RET(nUnicode && nVariantIdx >= 0 && nVariantIdx <= 4, 0);
   int nIdx = _FIndGEIIndex(nUnicode, _vVerticalExtGlyphs);
   if (nIdx < 0)
      return 0; //not found
   const SGlyphExtensionInfo& geInfo = _vVerticalExtGlyphs[nIdx];
   if (nVariantIdx >= geInfo.vVariants.size())
      return 0;//not enough variants
   if (nVariantIdx == 1)
      nIdx = 1;
   else {
      float fTargetSize;
      if (nVariantIdx == 2)
         //2 should be ~1.5 x size of nVariantIdx=1
         fTargetSize = 1.5f* geInfo.vVariants[1].nSize;
      else if (nVariantIdx == 3)
         //3: ~2 x nVariantIdx=1
         fTargetSize = 2.0f * geInfo.vVariants[0].nSize;
      else //nVariantIdx == 4!
         //4: ~2.5 x nVariantIdx=1
         fTargetSize = 2.5f * geInfo.vVariants[1].nSize;
      for (nIdx = 2; nIdx < geInfo.vVariants.size(); ++nIdx) {
         if ((float)geInfo.vVariants[nIdx].nSize >= fTargetSize)
            break;
      }
      if (nIdx >= geInfo.vVariants.size())
         return 0;
      //else
      //compare with previos candidate
      if (2 * fTargetSize < float(geInfo.vVariants[nIdx - 1].nSize + geInfo.vVariants[nIdx].nSize))
         --nIdx; //smaller size candidate is better
   }
   return geInfo.vVariants[nIdx].nGlyphIdx;
}
CMathItem* CExtGlyphBuilder::BuildVerticalGlyph(uint32_t nUnicode, const CMathStyle& style, uint32_t nSize, float fUserScale) {
   int nIdx = _FIndGEIIndex(nUnicode, _vVerticalExtGlyphs);
   _ASSERT_RET(nIdx >= 0, nullptr);
   const SGlyphExtensionInfo& geInfo = _vVerticalExtGlyphs[nIdx];
   _ASSERT_RET(!geInfo.vVariants.empty(), nullptr);
   const SLMMGlyph* pLMMGlyph = m_Doc.LMFManager().GetLMMGlyphByIdx(FONT_LMM, geInfo.vVariants.front().nGlyphIdx);
   _ASSERT_RET(pLMMGlyph, nullptr);//snbh!
   EnumTexAtom eAtom = _GetAtom((EnumGlyphClass)pLMMGlyph->eClass);
   //check if variant fits
   float fScale = style.StyleScale() * fUserScale;
   CContainerItem* pRet = new CContainerItem(m_Doc, eacVDELIM, CMathStyle(), eAtom);
   for (int nIdx = 0; nIdx< (int)geInfo.vVariants.size(); ++nIdx) {
      const SGlyphVariant* pGvar = &geInfo.vVariants[nIdx];
      if (F2NEAREST(pGvar->nSize * fScale) >= (int32_t)nSize) {
         //if (style.Style() != etsDisplay && nIdx) {
            //use prev. variant in compact styles
         //   pGvar = &geInfo.vVariants[nIdx - 1];
         //}
         pRet->AddBox(_BuildSingleVerticalGlyph(m_Doc, pGvar->nGlyphIdx, style, eAtom, fUserScale), 0, 0);
         pRet->SetMathAxis(pRet->Box().Height() / 2);
         return pRet;
      }
   }
   if (geInfo.gvBottomLeftId.nGlyphIdx == 0 && geInfo.gvTopRightId.nGlyphIdx == 0) {
      //no assembly/extension? return the largest
      pRet->AddBox(_BuildSingleVerticalGlyph(m_Doc, geInfo.vVariants.back().nGlyphIdx, style, eAtom, fUserScale), 0, 0);
      pRet->SetMathAxis(pRet->Box().Height() / 2);
      return pRet;
   }
   //else, assembly large vertical bracket
   CMathItem* pTop = nullptr;
   if (geInfo.gvTopRightId.nGlyphIdx) {
      pTop = _BuildSingleVerticalGlyph(m_Doc, geInfo.gvTopRightId.nGlyphIdx, style, etaORD, fUserScale);
      _ASSERT_RET(pTop, nullptr);
      pRet->AddBox(pTop, 0, 0);
   }
   CMathItem* pBottom = nullptr;
   if (geInfo.gvBottomLeftId.nGlyphIdx) {
      pBottom = _BuildSingleVerticalGlyph(m_Doc, geInfo.gvBottomLeftId.nGlyphIdx, style, etaORD, fUserScale);
      _ASSERT_RET(pBottom, nullptr);
   }
   if (geInfo.gvMiddleId.nGlyphIdx) {
      _ASSERT_RET(pTop && pBottom, nullptr);//need both with the middle!
      //btm-ext-mid-ext-top
      CMathItem* pMid = _BuildSingleVerticalGlyph(m_Doc, geInfo.gvMiddleId.nGlyphIdx, style, etaORD, fUserScale);
      const int32_t nExtSize2 = nSize - F2NEAREST((geInfo.gvBottomLeftId.nSize + geInfo.gvMiddleId.nSize +
         geInfo.gvTopRightId.nSize - 4 * otfMinConnectorOverlap) * fScale);
      if (nExtSize2 > 1) {
         //2 fillers
         CRuleItem* pFiller1 = new CRuleItem(m_Doc, F2NEAREST((geInfo.nExtenderRight - geInfo.nExtenderLeft+10) * fScale), nExtSize2/2);
         CRuleItem* pFiller2 = new CRuleItem(m_Doc, F2NEAREST((geInfo.nExtenderRight - geInfo.nExtenderLeft+10) * fScale), nExtSize2/2);
         pRet->AddBox(pFiller1, F2NEAREST(geInfo.nExtenderLeft * fScale),
                                pTop->Box().Bottom() - F2NEAREST(otfMinConnectorOverlap * fScale));
         pRet->AddBox(pMid, 0, pFiller1->Box().Bottom() - F2NEAREST(otfMinConnectorOverlap * fScale));
         pRet->AddBox(pFiller2, F2NEAREST(geInfo.nExtenderLeft * fScale),
                                pMid->Box().Bottom() - F2NEAREST(otfMinConnectorOverlap * fScale));
         pRet->AddBox(pBottom, 0, pFiller2->Box().Bottom() - F2NEAREST(otfMinConnectorOverlap * fScale));
      }
      else {
         //no filler; overlap must be between otfMinConnectorOverlap and geInfo.nMaxOverlap!
         int32_t nOverlap2 = nSize - F2NEAREST((geInfo.gvBottomLeftId.nSize + geInfo.gvMiddleId.nSize + 
                                                geInfo.gvTopRightId.nSize) * fScale);
         if ((float)nOverlap2 < 2*otfMinConnectorOverlap * fScale)
            nOverlap2 = F2NEAREST(2*otfMinConnectorOverlap * fScale);
         else if (nOverlap2 > F2NEAREST(2*geInfo.nMaxOverlap * fScale))
            nOverlap2 = F2NEAREST(2*geInfo.nMaxOverlap * fScale);//snbh?
         pRet->AddBox(pMid, 0, pTop->Box().Bottom() - nOverlap2 / 2);
         pRet->AddBox(pBottom, 0, pMid->Box().Bottom() - nOverlap2 / 2);
      }
   }
   else {
      if (pBottom && pTop) {
         //btm-ext-top
         const int32_t nExtSize = nSize - 
            F2NEAREST((geInfo.gvBottomLeftId.nSize + geInfo.gvTopRightId.nSize - 2 * otfMinConnectorOverlap) * fScale);
         int32_t nTop2 = pTop->Box().Bottom();
         if (nExtSize > 0) {
            int16_t nFillerSX = geInfo.nExtenderRight - geInfo.nExtenderLeft+20;
            if (geInfo.nExtender2Left) {
               //2 fillers 
               CRuleItem* pFiller1 = new CRuleItem(m_Doc, F2NEAREST(nFillerSX * fScale), nExtSize);
               pRet->AddBox(pFiller1, F2NEAREST(geInfo.nExtenderLeft * fScale),
                  pTop->Box().Bottom() - F2NEAREST(otfMinConnectorOverlap * fScale));
               CRuleItem* pFiller2 = new CRuleItem(m_Doc, F2NEAREST(nFillerSX * fScale), nExtSize);
               pRet->AddBox(pFiller2, F2NEAREST(geInfo.nExtender2Left * fScale),
                  pTop->Box().Bottom() - F2NEAREST(otfMinConnectorOverlap * fScale));
               nTop2 = pFiller1->Box().Bottom() - F2NEAREST(otfMinConnectorOverlap * fScale);
            }
            else {
               //1 filler
               CRuleItem* pFiller = new CRuleItem(m_Doc, F2NEAREST(nFillerSX * fScale), nExtSize);
               pRet->AddBox(pFiller, F2NEAREST(geInfo.nExtenderLeft * fScale),
                  pTop->Box().Bottom() - F2NEAREST(otfMinConnectorOverlap * fScale));
               nTop2 = pFiller->Box().Bottom() - F2NEAREST(otfMinConnectorOverlap * fScale);
            }
         }
         else {
            //no filler; overlap must be between otfMinConnectorOverlap and geInfo.nMaxOverlap!
            int32_t nOverlap = nSize - F2NEAREST((geInfo.gvBottomLeftId.nSize + geInfo.gvTopRightId.nSize) * fScale);
            if ((float)nOverlap < otfMinConnectorOverlap * fScale)
               nOverlap = F2NEAREST(otfMinConnectorOverlap * fScale);
            else if (nOverlap > F2NEAREST(geInfo.nMaxOverlap * fScale))
               nOverlap = F2NEAREST(geInfo.nMaxOverlap * fScale);//snbh?
            nTop2 = pTop->Box().Bottom() - F2NEAREST(nOverlap * fScale);
         }
         pRet->AddBox(pBottom, 0, nTop2);
      }
      else if (pTop) { //already in VBox!
         //ext-top
         const int32_t nExtSize = nSize - F2NEAREST(geInfo.gvTopRightId.nSize * fScale);
         if (nExtSize > 0) {
            const int16_t nFillerSX = geInfo.nExtenderRight - geInfo.nExtenderLeft;
            //1 filler
            CRuleItem* pFiller = new CRuleItem(m_Doc, F2NEAREST((nFillerSX + 10) * fScale),
                                                   nExtSize + F2NEAREST(otfMinConnectorOverlap * fScale));
            pRet->AddBox(pFiller, F2NEAREST(geInfo.nExtenderLeft * fScale),
                                 pTop->Box().Bottom() - F2NEAREST(otfMinConnectorOverlap * fScale));
         }
      }
      else {
         //btm-ext
         int32_t nTop2 = 0;
         const int32_t nExtSize = nSize - F2NEAREST(geInfo.gvBottomLeftId.nSize * fScale);
         if (nExtSize > 0) {
            const int16_t nFillerSX = geInfo.nExtenderRight - geInfo.nExtenderLeft ;
            //1 filler
            CRuleItem* pFiller = new CRuleItem(m_Doc, F2NEAREST((nFillerSX + 10) * fScale),
                                                   nExtSize + F2NEAREST(otfMinConnectorOverlap * fScale));
            pRet->AddBox(pFiller, F2NEAREST(geInfo.nExtenderLeft * fScale), 0);
            nTop2 = pFiller->Box().Bottom() - F2NEAREST(otfMinConnectorOverlap * fScale);
         }
         pRet->AddBox(pBottom, 0, nTop2);
      }
   }
   //Set Axis!
   pRet->SetMathAxis(pRet->Box().Height() / 2);
   return pRet;
}
CMathItem* CExtGlyphBuilder::BuildHorizontalGlyph(uint32_t nUnicode, const CMathStyle& style, uint32_t nSize, float fUserScale) {
   int nIdx = _FIndGEIIndex(nUnicode, _vHorizontalExtGlyphs);
   _ASSERT_RET(nIdx >= 0, nullptr);
   const SGlyphExtensionInfo& geInfo = _vHorizontalExtGlyphs[nIdx];
   //check if variant fits
   float fScale = style.StyleScale() * fUserScale;
   for (const SGlyphVariant& gvar : geInfo.vVariants) {
      if (F2NEAREST(gvar.nSize * fScale) >= (int32_t)nSize)
         return _BuildSingleHorizontalGlyph(m_Doc, gvar.nGlyphIdx, style, fUserScale);
   }
   if (geInfo.gvBottomLeftId.nGlyphIdx == 0) //no assembly/extension? return the largest
      return _BuildSingleHorizontalGlyph(m_Doc, geInfo.vVariants.back().nGlyphIdx, style, fUserScale);
   //else, assembly horizontal bracket
   CContainerItem* pRet = new CContainerItem(m_Doc, eacHDELIM, CMathStyle());
   CMathItem* pLeft = _BuildSingleHorizontalGlyph(m_Doc, geInfo.gvBottomLeftId.nGlyphIdx, style, fUserScale);
   CMathItem* pRight = _BuildSingleHorizontalGlyph(m_Doc, geInfo.gvTopRightId.nGlyphIdx, style, fUserScale);
   _ASSERT_RET(pLeft && pRight, nullptr);
   pRet->AddBox(pLeft, 0, 0);
   int32_t nRight1 = pLeft->Box().Right();
   if (geInfo.gvMiddleId.nGlyphIdx) {
      //left-ext-mid-ext-right
      CMathItem* pMid = _BuildSingleHorizontalGlyph(m_Doc, geInfo.gvMiddleId.nGlyphIdx, style, fUserScale);
      const int32_t nExtSize2 = nSize - F2NEAREST((geInfo.gvBottomLeftId.nSize + geInfo.gvMiddleId.nSize +
         geInfo.gvTopRightId.nSize - 4 * otfMinConnectorOverlap) * fScale);
      if (nExtSize2 > 1) {
         //2 simmetric fillers
         CRuleItem* pFiller1 = new CRuleItem(m_Doc, F2NEAREST((geInfo.nExtenderRight - geInfo.nExtenderLeft) * fScale), nExtSize2 / 2);
         CRuleItem* pFiller2 = new CRuleItem(m_Doc, F2NEAREST((geInfo.nExtenderRight - geInfo.nExtenderLeft) * fScale), nExtSize2 / 2);
         pRet->AddBox(pFiller1, F2NEAREST(geInfo.nExtenderLeft * fScale), 
                                nRight1 - F2NEAREST(otfMinConnectorOverlap * fScale));
         pRet->AddBox(pMid, 0, pFiller1->Box().Right() - F2NEAREST(otfMinConnectorOverlap * fScale));
         pRet->AddBox(pFiller2, F2NEAREST(geInfo.nExtenderLeft * fScale),
                                pMid->Box().Right() - F2NEAREST(otfMinConnectorOverlap * fScale));
         pRet->AddBox(pRight, 0, pFiller2->Box().Right() - F2NEAREST(otfMinConnectorOverlap * fScale));
      }
      else {
         //no filler; overlap must be between otfMinConnectorOverlap and geInfo.nMaxOverlap!
         int32_t nOverlap2 = nSize - F2NEAREST((geInfo.gvBottomLeftId.nSize + geInfo.gvMiddleId.nSize +
            geInfo.gvTopRightId.nSize) * fScale);
         if ((float)nOverlap2 < 2 * otfMinConnectorOverlap * fScale)
            nOverlap2 = F2NEAREST(2 * otfMinConnectorOverlap * fScale);
         else if (nOverlap2 > F2NEAREST(2 * geInfo.nMaxOverlap * fScale))
            nOverlap2 = F2NEAREST(2 * geInfo.nMaxOverlap * fScale);//snbh?
         pRet->AddBox(pMid, nRight1 - nOverlap2 / 2, 0);
         pRet->AddBox(pRight, pMid->Box().Right() - nOverlap2 / 2, 0);
      }
   }
   else {
      //left-ext-right
      const int32_t nExtSize = nSize - F2NEAREST((geInfo.gvBottomLeftId.nSize + geInfo.gvTopRightId.nSize -
                                       2 * otfMinConnectorOverlap) * fScale);
      int32_t nLeft2 = nRight1;
      if (nExtSize > 0) {
         int16_t nFillerSY = geInfo.nExtenderRight - geInfo.nExtenderLeft;
         _ASSERT(nFillerSY > 0);
         if (geInfo.nExtender2Left) {
            //2 fillers
            CRuleItem* pFiller1 = new CRuleItem(m_Doc, nExtSize, F2NEAREST(nFillerSY * fScale));
            pRet->AddBox(pFiller1, nRight1 - F2NEAREST(otfMinConnectorOverlap * fScale),
                                   pLeft->Box().Ascent() - F2NEAREST((geInfo.nExtenderLeft) * fScale));
            CRuleItem* pFiller2 = new CRuleItem(m_Doc, nExtSize, F2NEAREST(nFillerSY * fScale));
            pRet->AddBox(pFiller2, nRight1 - F2NEAREST(otfMinConnectorOverlap * fScale),
                                   pLeft->Box().Ascent() - F2NEAREST((geInfo.nExtender2Left+ nFillerSY) * fScale));
            nLeft2 = pFiller1->Box().Right() - F2NEAREST(otfMinConnectorOverlap * fScale);
         }
         else {
            //1 filler
            CRuleItem* pFiller = new CRuleItem(m_Doc, nExtSize, F2NEAREST((nFillerSY) * fScale));
            pRet->AddBox(pFiller, nRight1 - F2NEAREST(otfMinConnectorOverlap * fScale),
                                  pLeft->Box().Ascent() - F2NEAREST((geInfo.nExtenderRight)*fScale));
            nLeft2 = pFiller->Box().Right() - F2NEAREST(otfMinConnectorOverlap * fScale);
         }
      }
      else {
         //no filler; overlap must be between otfMinConnectorOverlap and geInfo.nMaxOverlap!
         int32_t nOverlap = nSize - F2NEAREST((geInfo.gvBottomLeftId.nSize + geInfo.gvTopRightId.nSize) * fScale);
         if ((float)nOverlap < otfMinConnectorOverlap * fScale)
            nOverlap = F2NEAREST(otfMinConnectorOverlap * fScale);
         else if (nOverlap > F2NEAREST(geInfo.nMaxOverlap * fScale))
            nOverlap = F2NEAREST(geInfo.nMaxOverlap * fScale);//snbh?
         nLeft2 = nRight1 - F2NEAREST(nOverlap * fScale);
      }
      pRet->AddBox(pRight, nLeft2, pLeft->Box().Ascent()- pRight->Box().Ascent());
   }
   pRet->NormalizeOrigin(0, 0);
   return pRet;
}
