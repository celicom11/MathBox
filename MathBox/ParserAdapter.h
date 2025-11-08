#pragma once
#include "TexParser.h"

class CParserAdapter : public IParserAdapter {
// DATA
   int                     m_nTkIdx;
   CTexParser&             m_TexParser;   // external parser
   const SParserContext&   m_ctx;         // external parser context
public:
// CTOR
   CParserAdapter() = delete;
   CParserAdapter(CTexParser& texParser, int nTkIdx, const SParserContext& ctx) :
      m_TexParser(texParser),m_nTkIdx(nTkIdx), m_ctx(ctx) {}
// ATTS
   int CurrentTokenIdx() const { return m_nTkIdx; }

// IParserAdapter
   CMathItem* ConsumeItem(EnumLCATParenthesis eParens, CMathStyle* pStyle = nullptr) override;
   bool ConsumeDimension(EnumLCATParenthesis eParens, OUT float& fPts) override;
   bool ConsumeInteger(EnumLCATParenthesis eParens, OUT int& nVal) override;
   bool ConsumeKeyword(PCSTR szKeyword) override;

   //context info
   CMathStyle GetCurrentStyle() const override { return m_ctx.currentStyle; }
   float GetUserScale() const override { return m_ctx.fUserScale; }
   //error info/setting
   bool HasError() const override { return !m_TexParser.LastError().sError.empty(); }
   void SetError(const string& sMessage) override {
      ParserError error;
      error.eStage = epsBUILDING;
      error.nPosition = m_TexParser.GetToken(m_nTkIdx)->nPos;
      error.sError = sMessage;
      m_TexParser.SetError(error);
   }
private:
   bool _CanConsumeToken(EnumLCATParenthesis eParens) const;
   bool _GetText(int nTkStart, int nTkEnd, OUT string& sText) const;
};