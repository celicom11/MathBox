#pragma once
#include "MathItem.h"
//forward decls
class CTokenizer; 
struct SParserContext {
   bool bTextMode{ false };            // TEXT/MATH mode
   bool bInLeftRight{ false };         // inside \left...\right construct
   CMathStyle currentStyle;            // MATH mode style
   float fUserScale{ 1.0f };           // User scaling factor
   string sFontCmd;                    // Current font (for both Math/Text modes!)
};

class CTexParser {
   friend class CParserAdapter; //allow adapter to inner processing
//DATA
   float                      m_fDocFontSizePts{ 12.0f };
   CTokenizer*                m_pTokenizer{ nullptr };   
   ParserError                m_Error;                   // last error
   vector<STexToken>          m_vTokens;                 // tokens of the current parsing
   vector<IMathItemBuilder*>  m_vBuilders;               // registered builders
public:
//CTOR/DTOR
   CTexParser() = default;
   ~CTexParser();
//ATTS
   float DocumentFontSizePts() const { return m_fDocFontSizePts; }
   const ParserError& LastError() const { return m_Error; }
   void SetError(const ParserError& error) { 
      if (m_Error.sError.empty()) //set only if no previous error
         m_Error = error; 
   }
   //safe token getter
   const STexToken* GetToken(int nIdx) const { 
      if ((uint32_t)nIdx >= m_vTokens.size())
         return nullptr; //caller must set error!
      return &m_vTokens[nIdx]; 
   }
   string TokenText_(int nIdx) const;
   //METHODS
   void RegisterBuilder(IMathItemBuilder* pBuilder) {
      m_vBuilders.push_back(pBuilder);
   }
   CMathItem* Parse(const string& sText);

private:
   bool BuildGroups_();
   //processing methods
   CMathItem* ProcessGroup_(IN OUT int& nIdx, const SParserContext& ctx);

   CMathItem* ProcessAlnum_(IN OUT int& nIdx, const SParserContext& ctx);
   CMathItem* ProcessNonAlnum_(IN OUT int& nIdx, const SParserContext& ctx);
   CMathItem* ProcessTextCmd_(IN OUT int& nIdx, const SParserContext& ctx);
   CMathItem* ProcessMathFontCmd_(IN OUT int& nIdx, const SParserContext& ctx);
   CMathItem* ProcessScaleCmd_(IN OUT int& nIdx, const SParserContext& ctx);
   CMathItem* ProcessItemBuilderCmd_(IN OUT int& nIdx, const SParserContext& ctx);
   //context-modifying commands
   bool ProcessMathStyleCmd_(IN OUT int& nIdx, IN OUT SParserContext& ctx);
   bool ProcessFontSizeCmd_(IN OUT int& nIdx, IN OUT SParserContext& ctx);
   //
   bool OnGroupOpen_(const STexToken& tkOpen, IN OUT SParserContext& ctxG, OUT bool& bCanBeEmpty);
   CMathItem* PackLines_(const vector<vector<CMathItem*>>& vvLines, const SParserContext& ctx);
};