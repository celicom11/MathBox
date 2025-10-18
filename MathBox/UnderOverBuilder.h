#pragma once
#include "MathItem.h"


//Factory for a VBOX container
class CUnderOverBuilder : public IMathItemBuilder {
public:
   //IMathItemBuilder implementation
   bool CanTakeCommand(PCSTR szCmd) const override;
   bool GetCommandInfo(PCSTR szCmd, OUT SLaTexCmdInfo& cmdInfo) const override;
   CMathItem* BuildItem(PCSTR szCmd, const CMathStyle& style, float fUserScale,
                        const vector<SLaTexCmdArgValue>& vArgValues) const override;
private:

};