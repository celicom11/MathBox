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
   const STexToken* GetToken(int nIdx) const {
      return m_TexParser.GetToken(nIdx);
   }
   const STexToken* GetCurrentToken() const {
      return m_TexParser.GetToken(m_nTkIdx);
   }
// IParserAdapter
   CMathItem* ConsumeItem(EnumLCATParenthesis eParens, const SParserContext& ctx) override;
   bool ConsumeDimension(EnumLCATParenthesis eParens, OUT float& fPts, OUT string& sOrigUnits) override;
   bool ConsumeInteger(EnumLCATParenthesis eParens, OUT int& nVal) override;
   EnumTokenType GetTokenData(OUT string& sText) override;
   void SkipToken() override { ++m_nTkIdx; } //unsafe?
   bool ConsumeHSkipGlue(OUT STexGlue& glue) override;
   uint32_t ConsumeColor(EnumLCATParenthesis eParens) override;
   //macros/variables
   bool GetLength(const string& sVar, OUT float& fPts) const override {
      return m_TexParser.GetLength(sVar, fPts);
   }
   void SetLength(const string& sVar, float fPts) override {
      m_TexParser.SetLength(sVar, fPts);
   }
   //context info
   const SParserContext& GetContext() const override { return m_ctx; }
   IDocParams& Doc() override { return m_TexParser.Doc(); }
   //error info/setting
   bool HasError() const override { return !m_TexParser.LastError().sError.empty(); }
   void SetError(const string& sMessage) override {
      m_TexParser.SetError(m_nTkIdx, sMessage);
   }
private:
   bool _CanConsumeToken(EnumLCATParenthesis eParens) const;
   bool _GetText(int nTkStart, int nTkEnd, OUT string& sText) const;
   bool _ConsumeGlueComponent(OUT float& fValue, OUT uint16_t& nOrder);
};