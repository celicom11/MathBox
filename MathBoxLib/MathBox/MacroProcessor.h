#pragma once
#include "MathItem.h"

class CTexParser;

class CMacroProcessor {
public: //for unit-tests!
//TYPES
   struct SMacroDef {
      int32_t nNameTkIdx{ 0 };            // Macro name token index in m_vTokens
      int32_t nNumArgs{ 0 };              // Number of required arguments (0-9)
      int32_t nBodyTkIdx{ 0 };            // Macro Body token index (one or group!)
      int32_t nDefArgTkIdx{ 0 };          // Default arg token index for optional arg (0 if none)
   };
private:
//DATA
   CTexParser&             m_Parser;      // owner
   vector<STexToken>       m_vTokens;     // tokenized/grouped ALL macros text
   map<string, SMacroDef>  m_mapMacros;   // defined macros
public:
//CTOR/DTOR
   CMacroProcessor(CTexParser& parser) : m_Parser(parser) {}
//ATTS
   bool IsInitialized() const;
   const STexToken* GetToken(int nIdx) const {
      return (nIdx >= 0 && nIdx < m_vTokens.size()) ? &m_vTokens[nIdx] : nullptr;
   }
   // public only for testing /debugging!
   uint32_t MacroCount() const { return (uint32_t)m_mapMacros.size(); }
   const SMacroDef* GetMacroDef(const string& name) const { 
      auto itPair = m_mapMacros.find(name);
      return itPair== m_mapMacros.end()? nullptr : &itPair->second;
   }
   const vector<STexToken>& GetTokens() const { return m_vTokens; }
   vector<STexToken>& GetTokens() { return m_vTokens; }
//METHODS
   bool InitMacros();
   bool ExpandToken(const vector<STexToken>& vMainTokens, IN OUT int& nTkIdx, OUT vector<STexToken>& vExpanded);
private:
   bool ParseMacroDefinitions_();
   bool ParseSetLength_(IN OUT int& nIdx);
   bool ParseLet_(IN OUT int& nIdx);
   // \newcommand{\cmdname}[N][[opt_arg]]{body}
   bool ParseNewCommand_(IN OUT int& nIdx);
   bool ExtractArguments_(int nRefIdx, const SMacroDef& macroDef, const vector<STexToken>& vMainTokens,
                          IN OUT int& nTkIdx, OUT vector<vector<STexToken>>& vArgs);
};
