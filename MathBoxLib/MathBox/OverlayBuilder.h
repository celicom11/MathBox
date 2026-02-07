#pragma once
#include "MathItem.h"
#include "OverlayItem.h"


//OverlayItem factory with poly-line decorations
class COverlayBuilder : public IMathItemBuilder {
public:
   //IMathItemBuilder implementation
   bool CanTakeCommand(PCSTR szCmd) const override;
   CMathItem* BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) override; 
private:
   CMathItem* GetContentWithOverlays_(const string& sCmd, IParserAdapter* pParser,
                              OUT vector< COverlayItem::SPolyLine>& vOverlays, OUT SRect& rcBox);

};