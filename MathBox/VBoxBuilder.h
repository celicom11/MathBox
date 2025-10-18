#pragma once
#include "MathItem.h"


//Factory for a VBOX container
class CVBoxBuilder : public IMathItemBuilder {
public:
   //IMathItemBuilder implementation
   bool CanTakeCommand(PCSTR szCmd) const override;
   bool GetCommandInfo(PCSTR szCmd, OUT SLaTexCmdInfo& cmdInfo) const override;
   CMathItem* BuildItem(PCSTR szCmd, const CMathStyle& style, float fUserScale,
                        const vector<SLaTexCmdArgValue>& vArgValues) const override;
private:
   //NOTE:nVKern must be scaled by style/UserScale before this call!
   static CMathItem* BuildBox_(CMathItem* pTop, CMathItem* pBottom,
                              int16_t nAnchor,              // index of the item for Baseline positioning
                              int32_t nVKern = 0,           // Custom vertical spacing in EMs
                              EnumTexAtom eAtom = etaORD);  // output Atom type

};