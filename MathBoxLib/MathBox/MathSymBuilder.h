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
   CMathItem* BuildMathOperator_(PCSTR szOp, IParserAdapter* pParser);
   CMathItem* BuildMathOperatorLim_(PCSTR szOp, IParserAdapter* pParser);
   CWordItem* BuildLMMSymbol_(const SLMMGlyph* pLmmGlyph, const CMathStyle& style, float fUserScale);

};