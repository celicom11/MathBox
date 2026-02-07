#pragma once
#include "MacrosMgr.h"
#include "MacroProcessor.h"
//forward decls
class CTokenizer;
class CRawItem;
class CMathModeProcessor;
class CTextModeProcessor;
class CEnvHelper;

class CTexParser {
 //DATA
   IDocParams&          m_Doc;
   CTokenizer*          m_pTokenizer{ nullptr };   
   CMathModeProcessor*  m_pMathProcessor{nullptr};
   CTextModeProcessor*  m_pTextProcessor{nullptr};
   CMacrosMgr           m_MacrosMgr;
   CMacroProcessor      m_MacroProcessor;
   ParserError          m_Error;                   // last error
   vector<STexToken>    m_vTokensOrig;             // tokens of first tokenization (for error reporting)
   vector<STexToken>    m_vTokens;                 // current tokens
   map<string, float>   m_mapLengths;              // macros/user dimensions/lengths
public:
//CTOR/DTOR/INIT
   CTexParser(IDocParams& doc);
   ~CTexParser();
//ATTS
   IDocParams& Doc() { return m_Doc; }
   const ParserError& LastError() const { return m_Error; }
   //error handling
   bool HasError() const { return !m_Error.sError.empty(); }
   void SetError(int nTkIdx, const string& sError);
   void ClearError() {
      m_Error.nStartPos = m_Error.nEndPos = 0;
      m_Error.sError.clear();
   }
   //parsing stage
   void SetStage(EnumParsingStage eStage) {
      m_Error.eStage = eStage;
   }
   EnumParsingStage Stage() const { return m_Error.eStage; }

   bool HasMacros() const { return !m_MacrosMgr.isEmpty(); }
   //safe token getters
   const STexToken* GetToken(int nIdx) const;
   string TokenText(int nIdx) const;
   string TokenText(const STexToken* pToken) const;
   const vector<STexToken>& GetTokens() const {
      return m_Error.eStage <= epsBUILDINGMACROS ? m_MacroProcessor.GetTokens() : m_vTokens;
   }
   //unit-test getters
   const CMacrosMgr& GetMacrosMgr() const {return m_MacrosMgr;}
   const string& GetMacrosText() const { return m_MacrosMgr.getMacrosText(); }
   const vector<STexToken>& GetTokensOrig() const { return m_vTokensOrig; }
   const vector<STexToken>& GetMacroTokens() const { return m_MacroProcessor.GetTokens(); }
   uint32_t MacroCount() const { return m_MacroProcessor.MacroCount(); }
   const CMacroProcessor::SMacroDef* GetMacroDef(const string& name) const { 
      return m_MacroProcessor.GetMacroDef(name); 
   }
   //METHODS
   void AddMacros(PCSTR szMacros, PCSTR szFileName) {
      m_MacrosMgr.addMacros(szMacros, szFileName);
   }
   CMathItem* Parse(const string& sText);
   //public processing methods
   CMathItem* ProcessGroup(IN OUT int& nIdx, const SParserContext& ctx);
   CMathItem* ProcessItemToken(IN OUT int& nIdx, const SParserContext& ctx);
   bool GetLength(const string& sVar, OUT float& fPts) const {
      PCSTR szVar = sVar.c_str();
      if(szVar[0] == '\\')
         ++szVar; //skip leading backslash
      auto it = m_mapLengths.find(szVar);
      if (it == m_mapLengths.end())
         return false;
      fPts = it->second;
      return true;
   }
   void SetLength(const string& sVar, float fPts) {
      PCSTR szVar = sVar.c_str();
      if (szVar[0] == '\\')
         ++szVar; //skip leading backslash
      m_mapLengths[szVar] = fPts;
   }
   //internal grouping builder
   bool BuildGroups();

private:
   bool OnGroupOpen_(int nTkIdx, IN OUT SParserContext& ctxG, 
                     OUT bool& bCanBeEmpty, IN OUT CEnvHelper& envh);
};