#pragma once
#include "MathItem.h"
//forward decls
class CTokenizer;
class CRawItem;
class CMathModeProcessor;
class CTextModeProcessor;
class CEnvHelper;

class CTexParser {
 //DATA
   float                m_fDocFontSizePts{ 12.0f };
   CTokenizer*          m_pTokenizer{ nullptr };   
   CMathModeProcessor*  m_pMathProcessor{nullptr};
   CTextModeProcessor*  m_pTextProcessor{nullptr};
   ParserError          m_Error;                   // last error
   vector<STexToken>    m_vTokens;                 // tokens of the current parsing
public:
//CTOR/DTOR/INIT
   CTexParser();
   ~CTexParser();
//ATTS
   float DocumentFontSizePts() const { return m_fDocFontSizePts; }
   void  SetDocumentFontSizePts(float fDocFontSizePts) { m_fDocFontSizePts = fDocFontSizePts; }
   const ParserError& LastError() const { return m_Error; }
   bool HasError() const { return !m_Error.sError.empty(); }
   void SetError(int nTkIdx, const string& sError) { 
      if (m_Error.sError.empty()) {//set only if no previous error
         const STexToken* pToken = GetToken(nTkIdx);
         m_Error.nPosition = pToken ? pToken->nPos : -1;
         m_Error.sError = sError;
      }
   }
   //safe token getters
   int TokenCount() const { return (int)m_vTokens.size(); }
   const STexToken* GetToken(int nIdx) const;
   string TokenText(int nIdx) const;
   //METHODS
   CMathItem* Parse(const string& sText);
   //public processing methods
   CMathItem* ProcessGroup(IN OUT int& nIdx, const SParserContext& ctx);
   CMathItem* ProcessItemToken(IN OUT int& nIdx, const SParserContext& ctx);
private:
   bool BuildGroups_();
   bool OnGroupOpen_(int nTkIdx, IN OUT SParserContext& ctxG, 
                     OUT bool& bCanBeEmpty, IN OUT CEnvHelper& envh);
};