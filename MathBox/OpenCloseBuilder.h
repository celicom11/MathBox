#pragma once
#include "MathItem.h"

class COpenCloseBuilder : public IMathItemBuilder {
public:
   //IMathItemBuilder implementation
   bool CanTakeCommand(PCSTR szCmd, bool bTextMode) const override;
   CMathItem* BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) override;
   //helper
   static bool _GetDelimiter(PCSTR szCmd, const CMathStyle& style, OUT SLMMDelimiter& lmmDelimiter);
   static CMathItem* _BuildDelimiter(uint32_t nUni, EnumDelimType edt, uint32_t nSize, 
                                    const CMathStyle& style, float fUserScale);
private:
   //fixed size delimiter
   static CMathItem* BuildDelimiter_(const SLMMDelimiter& lmmDelimiter, const CMathStyle& style,
      float fUserScale = 1.0f);

};