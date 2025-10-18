#include "stdafx.h"
#include "VBoxBuilder.h"
#include "ContainerItem.h"
#include "OpenCloseBuilder.h"

namespace {
   static const map<string, vector<SLaTexCmdArgInfo>> _mapMathModeCmdInfo {
      {"_^",{        {false, false, true,0,elcatItem,elcapAny},
                     {false, false, true,1,elcatItem,elcapAny}
            }
      },
      {"^_",{        {false, false, true,0,elcatItem,elcapAny},
                     {false, false, true,1,elcatItem,elcapAny}
            }
      },
      {"overset",{   {false, false, true,1,elcatItem,elcapFig},
                     {false, true, false,2,elcatItem,elcapFig}
            }
      },
      {"underset",{  {false, false, true,1,elcatItem,elcapFig},
                     {false, true, false,2,elcatItem,elcapFig}
            }
      },
      {"substack",{  {false, false, true,1,elcatMultiLine,elcapFig}}},
      {"stackunder",{
                     {true, false, false,1,elcatDim,elcapSquare},
                     {false, true, false,2,elcatItem,elcapFig},
                     {false, false, false,3,elcatItem,elcapFig},
            }
      },
      {"stackrel",{  
                     {true, false, true,1,elcatItem,elcapSquare},
                     {false, false, true,2,elcatItem,elcapFig},
                     {false, true, false,3,elcatGlyph,elcapFig},

            }
      },
      {"binom", { 
                     {false, false, false,1,elcatItem,elcapFig},
                     {false, false, false,2,elcatItem,elcapFig},
            }
      },
      {"genfrac",{ // need ParenthesisBuider for delim args!
                     {false, false, false,1,elcatGlyph,elcapFig},
                     {false, false, false,2,elcatGlyph,elcapFig},
                     {false, false, false,3,elcatDim,elcapFig},
                     {false, false, false,4,elcatTexStyle,elcapFig},
                     {false, false, false,5,elcatItem,elcapFig},
                     {false, false, false,6,elcatItem,elcapFig},
            }
      }
   };
   CContainerItem* _BuildVBox(vector<CMathItem*>& vItems, int32_t nVKern) {
      CContainerItem* pRetBox = new CContainerItem(eacVBOX, CMathStyle(), etaORD);
      int32_t nWidth = 0;
      for (const CMathItem* pItem : vItems) {
         nWidth = max(nWidth, pItem->Box().Width());
      }
      _ASSERT_RET(nWidth > 0, nullptr);//snbh!
      CMathItem* pLastItem = nullptr;
      for (CMathItem* pItem : vItems) {
         pRetBox->AddBox(pItem, (nWidth - pItem->Box().Width())/2, (pLastItem ? pLastItem->Box().Bottom() + nVKern : 0));
         pLastItem = pItem;
      }
      pRetBox->NormalizeOrigin(0, 0);
      return pRetBox;
   }
}

CMathItem* CVBoxBuilder::BuildBox_(CMathItem* pTop, CMathItem* pBottom,
                                 int16_t nAnchor, int32_t nVKern, EnumTexAtom eAtom) {
   _ASSERT_RET(pTop && pBottom, nullptr);
   _ASSERT_RET(nAnchor>=0 && nAnchor<=2, nullptr);
   CContainerItem* pRetBox;
   int32_t nWidth = max(pTop->Box().Width(), pBottom->Box().Width());
   if(pTop->Type() == eacVBOX)
      pRetBox = static_cast<CContainerItem*>(pTop);
   else {
      pRetBox = new CContainerItem(eacVBOX, CMathStyle(), eAtom);
      pRetBox->AddBox(pTop, (nWidth - pTop->Box().Width()) / 2, 0);
   }
   int32_t nBottomY = pTop->Box().Height() + nVKern;
   pRetBox->AddBox(pBottom, (nWidth - pBottom->Box().Width()) / 2, nBottomY);
   pRetBox->NormalizeOrigin(0, 0);
   //set VBox Baseline
   int32_t nAscent = 0;
   if (nAnchor == 0) //top
      pRetBox->SetAscent( pTop->Box().BaselineY());
   else if (nAnchor == 1) //bottom
      pRetBox->SetAscent(pBottom->Box().BaselineY());
   else //Gap's center is a Math Axis!
      pRetBox->SetMathAxis((pTop->Box().Bottom() + pBottom->Box().Top()) / 2);
   
   return pRetBox;

}
//IMathItemBuilder 
bool CVBoxBuilder::CanTakeCommand(PCSTR szCmd) const {
   _ASSERT_RET(szCmd && *szCmd, false);
   if( szCmd[0] == '\\' )
      ++szCmd;
   return (_mapMathModeCmdInfo.find(szCmd) != _mapMathModeCmdInfo.end());
}
bool CVBoxBuilder::GetCommandInfo(PCSTR szCmd, OUT SLaTexCmdInfo& cmdInfo) const {
   _ASSERT_RET(szCmd && *szCmd, false);
   auto itPair = _mapMathModeCmdInfo.find(szCmd);
   if (itPair == _mapMathModeCmdInfo.end())
      return false;
   cmdInfo.vArgInfo = itPair->second;//copy vector
   return true;
}
CMathItem* CVBoxBuilder::BuildItem(PCSTR szCmd, const CMathStyle& style, float fUserScale,
                                 const vector<SLaTexCmdArgValue>& vArgValues) const {
   if (szCmd[0] == '\\')
      ++szCmd;
   auto itPair = _mapMathModeCmdInfo.find(szCmd);
   _ASSERT_RET(itPair != _mapMathModeCmdInfo.end(), nullptr);//snbh!
   //verify\build arguments for the BuildBox_
   CMathItem* pTop, * pBottom;
   int16_t nAnchor;
   int32_t nVKern = 0; 
   EnumTexAtom eAtom = etaORD;
   if (itPair->first == "_^" || itPair->first == "^_") {
      _ASSERT_RET(vArgValues.size() == 2, nullptr);
      _ASSERT_RET(vArgValues[0].eLCAT == elcatItem, nullptr);
      _ASSERT_RET(vArgValues[1].eLCAT == elcatItem, nullptr);
      pTop = vArgValues[itPair->first == "_^"?1:0].uVal.pMathItem;
      pBottom = vArgValues[itPair->first == "_^" ? 0 : 1].uVal.pMathItem;
      nAnchor = 2; //axis
      nVKern = F2NEAREST(5 * otfFractionRuleThickness* style.StyleScale()* fUserScale);
   }
   else if (itPair->first == "underset" || itPair->first == "overset") {
      _ASSERT_RET(vArgValues.size() == 2, nullptr);
      _ASSERT_RET(vArgValues[0].eLCAT == elcatItem, nullptr);
      _ASSERT_RET(vArgValues[1].eLCAT == elcatItem, nullptr);
      if (itPair->first == "underset") {
         pTop = vArgValues[1].uVal.pMathItem;
         pBottom = vArgValues[0].uVal.pMathItem;
         nAnchor = 0; //top is a base
      }
      else {
         pTop = vArgValues[0].uVal.pMathItem;
         pBottom = vArgValues[1].uVal.pMathItem;
         nAnchor = 1; //bottom is a base
      }
      nVKern = F2NEAREST(5 * otfFractionRuleThickness * style.StyleScale() * fUserScale);
   }
   else if (itPair->first == "substack") {
      _ASSERT_RET(!vArgValues.empty(), nullptr);
      if (vArgValues.size() == 1) {
         _ASSERT_RET(vArgValues[0].eLCAT == elcatItem, nullptr);
         return vArgValues[0].uVal.pMathItem; //no box needed!
      }
      vector<CMathItem*> vItems;
      for (const SLaTexCmdArgValue& val : vArgValues) {
         _ASSERT_RET(val.eLCAT == elcatItem && val.uVal.pMathItem, nullptr);
         vItems.push_back(val.uVal.pMathItem);
      }
      nVKern = F2NEAREST((style.Style() == etsDisplay? 180 : otfStackGapMin) * fUserScale);
      //nVKern = F2NEAREST(180 * fUserScale);
      CContainerItem* pVBox = _BuildVBox(vItems, nVKern);
      pVBox->SetMathAxis(pVBox->Box().Height() / 2); //q&d
      return pVBox;
   }
   else if (itPair->first == "stackrel") {
      _ASSERT_RET(vArgValues.size() == 2, nullptr);
      _ASSERT_RET(vArgValues[0].eLCAT == elcatItem, nullptr);
      _ASSERT_RET(vArgValues[1].eLCAT == elcatItem, nullptr);
      pTop = vArgValues[0].uVal.pMathItem;
      pBottom = vArgValues[1].uVal.pMathItem;
      nAnchor = 1;      //bottom is a base
      eAtom = etaREL;   //relation
      nVKern = F2NEAREST((style.Style() == etsDisplay ? 180 : otfStackGapMin) * fUserScale);
   }
   else if (itPair->first == "stackunder") {
      _ASSERT_RET(vArgValues.size() == 3, nullptr);
      _ASSERT_RET(vArgValues[0].eLCAT == elcatDim, nullptr);
      _ASSERT_RET(vArgValues[1].eLCAT == elcatItem, nullptr);
      _ASSERT_RET(vArgValues[2].eLCAT == elcatItem, nullptr);
      pTop = vArgValues[1].uVal.pMathItem;
      pBottom = vArgValues[2].uVal.pMathItem;
      nAnchor = 0;      //top is a base
      nVKern = F2NEAREST(vArgValues[0].uVal.nVal * fUserScale);
   }
   else if (itPair->first == "binom") {
      _ASSERT_RET(vArgValues.size() == 2, nullptr);
      _ASSERT_RET(vArgValues[0].eLCAT == elcatItem, nullptr);
      _ASSERT_RET(vArgValues[1].eLCAT == elcatItem, nullptr);
      pTop = vArgValues[0].uVal.pMathItem;
      pBottom = vArgValues[1].uVal.pMathItem;
      nAnchor = 2;      //axis!
      nVKern = F2NEAREST(6 * otfFractionRuleThickness * style.StyleScale() * fUserScale);//q&d
      CMathItem* pGenFrac = BuildBox_(pTop, pBottom, nAnchor, nVKern, eAtom);
      _ASSERT_RET(pGenFrac, nullptr);
      return COpenCloseBuilder::BuildOpenClose(L'(', L')', pGenFrac, style, fUserScale);
   }
   else
      return nullptr;//snbh!
   return BuildBox_(pTop, pBottom, nAnchor, nVKern, eAtom);
}
