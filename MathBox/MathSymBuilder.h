#pragma once
#include "MathItem.h"
class CWordItem;
//math mode operators and symbols (\mathord) builder
class CMathSymBuilder : public IMathItemBuilder {
   IDocParams& m_Doc;                     //doc params ref  
public:
//CTOR/DTOR
   CMathSymBuilder(IDocParams& doc) : m_Doc(doc) {}
//IMathItemBuilder implementation
   bool CanTakeCommand(PCSTR szCmd) const override;
   CMathItem* BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) override;
private:
   // BUILDERS
   CWordItem* BuildMathOperator_(PCSTR szOp, const CMathStyle& style, float fUserScale);
   CWordItem* BuildLMMSymbol_(const SLMMGlyph* pLmmGlyph, const CMathStyle& style, float fUserScale);

};