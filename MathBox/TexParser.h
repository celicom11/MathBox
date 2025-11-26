#pragma once
#include "MathItem.h"
//forward decls
class CTokenizer;
class CRawItem;

class CTexParser {
 //DATA
   float                      m_fDocFontSizePts{ 12.0f };
   CTokenizer*                m_pTokenizer{ nullptr };   
   ParserError                m_Error;                   // last error
   vector<STexToken>          m_vTokens;                 // tokens of the current parsing
   vector<IMathItemBuilder*>  m_vBuilders;               // registered builders
public:
//CTOR/DTOR/INIT
   CTexParser();
   ~CTexParser();
//ATTS
   float DocumentFontSizePts() const { return m_fDocFontSizePts; }
   void  SetDocumentFontSizePts(float fDocFontSizePts) { m_fDocFontSizePts = fDocFontSizePts; }
   const ParserError& LastError() const { return m_Error; }
   void SetError(const ParserError& error) { 
      if (m_Error.sError.empty()) //set only if no previous error
         m_Error = error; 
   }
   //safe token getters
   const STexToken* GetToken(int nIdx) const { 
      if ((uint32_t)nIdx >= m_vTokens.size())
         return nullptr; //caller must set error!
      return &m_vTokens[nIdx]; 
   }
   string TokenText(int nIdx) const;
   //METHODS
   CMathItem* Parse(const string& sText);
   //processing methods
   CMathItem* ProcessGroup(IN OUT int& nIdx, const SParserContext& ctx);
   CMathItem* ProcessItemToken(IN OUT int& nIdx, const SParserContext& ctx);
protected:
   void RegisterBuilder(IMathItemBuilder* pBuilder) {
      m_vBuilders.push_back(pBuilder);
   }

private:
   bool BuildGroups_();

   CMathItem* ProcessAlnum_(IN OUT int& nIdx, const SParserContext& ctx);
   CMathItem* ProcessNonAlnum_(IN OUT int& nIdx, const SParserContext& ctx);
   CMathItem* ProcessTextCmd_(IN OUT int& nIdx, const SParserContext& ctx);
   CMathItem* ProcessItemBuilderCmd_(IN OUT int& nIdx, const SParserContext& ctx);
   //context-modifying commands
   bool ProcessMathStyleCmd_(IN OUT int& nIdx, IN OUT SParserContext& ctx);
   bool ProcessFontSizeCmd_(IN OUT int& nIdx, IN OUT SParserContext& ctx);
   //
   bool OnGroupOpen_(const STexToken& tkOpen, IN OUT SParserContext& ctxG, OUT bool& bCanBeEmpty);
   CMathItem* PackGroupItems_(int nIdxStart, vector<CRawItem>& vGroupItems, const SParserContext& ctx);
};