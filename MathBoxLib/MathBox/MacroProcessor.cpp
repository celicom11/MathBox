#include "stdafx.h"
#include "TexParser.h"
#include "Tokenizer.h"
#include "ParserAdapter.h"
#include "VarCmdBuilder.h"

bool CMacroProcessor::IsInitialized() const { 
   return !m_Parser.HasMacros() || !m_vTokens.empty(); 
}

bool CMacroProcessor::InitMacros() {
   if(IsInitialized())
      return true; //already initialized!
   //tokenize/group ALL macros text
   m_vTokens.clear();
   m_Parser.SetStage(epsTOKENIZINGMACROS);
   CTokenizer tokenizer(m_Parser.GetMacrosText(), true);
   ParserError parser_err;
   if (!tokenizer.Tokenize(m_vTokens, parser_err))
      return false; //tokenization error
   m_Parser.SetStage(epsGROUPINGMACROS);
   if (!m_Parser.BuildGroups())
      return false;      // grouping error
   return ParseMacroDefinitions_();
}
bool CMacroProcessor::ParseMacroDefinitions_() {
   m_mapMacros.clear();
   m_Parser.SetStage(epsBUILDINGMACROS);
   for (int nIdx = 0; nIdx < (int)m_vTokens.size(); ) {
      const STexToken& tkn = m_vTokens[nIdx];
      //skip spaces/comments
      if(tkn.nType == ettSPACE || tkn.nType == ettCOMMENT) {
         ++nIdx;
         continue;
      }
      if (tkn.nType != ettCOMMAND) {
         m_Parser.SetError(nIdx, "Expected macro definition command");
         return false;
      }
      string sCmd = m_Parser.TokenText(&tkn);
      // Dispatch to specialized handlers
      if (sCmd == "\\setlength" || sCmd == "\\addtolength") {
         if (!ParseSetLength_(nIdx))
            return false;
      }
      else if (sCmd == "\\let") {
         if (!ParseLet_(nIdx))
            return false;
      }
      else if (sCmd == "\\newcommand" || sCmd == "\\renewcommand") {
         if (!ParseNewCommand_(nIdx))
            return false;
      }
      else {
         m_Parser.SetError(nIdx, "Unknown macro command: " + sCmd);
         return false;
      }
   }

   return true;
}
// Input: \setlength{\var}{length}  or  \addtolength{\var}{length}
bool CMacroProcessor::ParseSetLength_(IN OUT int& nIdx) {
   // Use CParserAdapter+VarCmdBuilder set length
   SParserContext dummyCtx;  // Dummy context (not used for dimension parsing)
   const STexToken& tkn = m_vTokens[nIdx];
   CParserAdapter adapter(m_Parser, ++nIdx, dummyCtx);//
   CVarCmdBuilder varBuilder;
   varBuilder.BuildFromParser(m_Parser.TokenText(&tkn).c_str(), &adapter);
   nIdx = adapter.CurrentTokenIdx();
   return !adapter.HasError();
}
// Input: \let\newcmd\existingcmd  or  \let\newcmd = \existingcmd
bool CMacroProcessor::ParseLet_(IN OUT int& nIdx) {
   ++nIdx;  // Skip \let
   const STexToken* pTkCurrent = GetToken(nIdx);
   // Skip optional space
   if (pTkCurrent && (pTkCurrent->nType == ettSPACE || pTkCurrent->nType == ettCOMMENT))
      pTkCurrent = GetToken(++nIdx);

   // Get new command name
   if (!pTkCurrent || pTkCurrent->nType != ettCOMMAND) {
      m_Parser.SetError(nIdx, "Expected new command name after \\let");
      return false;
   }
   int32_t nTkIdxNewCmd = nIdx;
   string sNewCmd = m_Parser.TokenText(pTkCurrent);
   pTkCurrent = GetToken(++nIdx);
   // Skip spaces and optional '='
   if (pTkCurrent && (pTkCurrent->nType == ettSPACE || pTkCurrent->nType == ettCOMMENT))
      pTkCurrent = GetToken(++nIdx);
   if (pTkCurrent && pTkCurrent->nType == ettNonALPHA && m_Parser.TokenText(pTkCurrent) == "=") {
      pTkCurrent = GetToken(++nIdx);
      if (pTkCurrent && (pTkCurrent->nType == ettSPACE || pTkCurrent->nType == ettCOMMENT))
         pTkCurrent = GetToken(++nIdx);
   }

   // Get existing command name
   if (!pTkCurrent || pTkCurrent->nType != ettCOMMAND) {
      m_Parser.SetError(nIdx, "Expected a command name after \\let");
      return false;
   }
   int32_t nBodyTkIdx = nIdx;
   string sExistingCmd = m_Parser.TokenText(pTkCurrent);
   ++nIdx;

   // Copy/alias macro definition
   auto it = m_mapMacros.find(sExistingCmd);
   if (it != m_mapMacros.end()) {
      m_mapMacros[sNewCmd] = it->second;  // COPY BODY, but name_idx
      m_mapMacros[sNewCmd].nNameTkIdx = nTkIdxNewCmd;
   }
   else {
      SMacroDef& mdef = m_mapMacros[sNewCmd];
      mdef.nNameTkIdx = nTkIdxNewCmd;
      mdef.nNumArgs = 0;
      mdef.nBodyTkIdx = nBodyTkIdx; // unknown yet command
   }
   return true;
}
// \newcommand[{]\cmdname[}][N][opt_arg]{body}
bool CMacroProcessor::ParseNewCommand_(IN OUT int& nIdx) {
   // Skip \newcommand or \renewcommand
   const STexToken* pTkCurrent = GetToken(++nIdx);

   SMacroDef macroDef;

   // 1. Parse command name: {\cmdname}
   // Skip optional space
   while (pTkCurrent && (pTkCurrent->nType == ettSPACE || pTkCurrent->nType == ettCOMMENT)) {
      pTkCurrent = GetToken(++nIdx);
   }

   // Expect { 
   if (!pTkCurrent || pTkCurrent->nType != ettFB_OPEN) {
      m_Parser.SetError(nIdx, "Expected { before command name");
      return false;
   }
   pTkCurrent = GetToken(++nIdx);

   // Skip space after {
   while (pTkCurrent && (pTkCurrent->nType == ettSPACE || pTkCurrent->nType == ettCOMMENT)) {
      pTkCurrent = GetToken(++nIdx);
   }

   // Get command name
   if (!pTkCurrent || pTkCurrent->nType != ettCOMMAND) {
      m_Parser.SetError(nIdx, "Expected command name");
      return false;
   }

   string sMacroName = m_Parser.TokenText(pTkCurrent);
   macroDef.nNameTkIdx = nIdx;
   pTkCurrent = GetToken(++nIdx);//next tk

   // Skip space before }
   while (pTkCurrent && (pTkCurrent->nType == ettSPACE || pTkCurrent->nType == ettCOMMENT)) {
      pTkCurrent = GetToken(++nIdx);
   }

   // Expect }
   if (!pTkCurrent || pTkCurrent->nType != ettFB_CLOSE) {
      m_Parser.SetError(nIdx, "Expected } after command name");
      return false;
   }
   pTkCurrent = GetToken(++nIdx);

   // 2. Parse optional argument count: [N]
   macroDef.nNumArgs = 0;

   // Skip space
   if (pTkCurrent && (pTkCurrent->nType == ettSPACE || pTkCurrent->nType == ettCOMMENT))
      pTkCurrent = GetToken(++nIdx);

   if (pTkCurrent && pTkCurrent->nType == ettSB_OPEN) {
      // Use CParserAdapter to consume integer
      SParserContext dummyCtx;
      CParserAdapter adapter(m_Parser, nIdx, dummyCtx);

      int nNumArgs = 0;
      if (!adapter.ConsumeInteger(elcapSquare, nNumArgs))
         return false;  // Error already set

      if (nNumArgs < 0 || nNumArgs > 9) {
         m_Parser.SetError(nIdx, "Argument count must be 0-9");
         return false;
      }

      macroDef.nNumArgs = nNumArgs;
      nIdx = adapter.CurrentTokenIdx();
      pTkCurrent = GetToken(nIdx);
   }

   // 3. Parse optional default argument: [opt_arg]
   macroDef.nDefArgTkIdx = 0;  // Default: no optional arg

   // Skip space
   if (pTkCurrent && (pTkCurrent->nType == ettSPACE || pTkCurrent->nType == ettCOMMENT))
      pTkCurrent = GetToken(++nIdx);

   if (pTkCurrent && pTkCurrent->nType == ettSB_OPEN) {
      // Store the opening [ token index (or content if single token)
      const int nOptArgStart = nIdx;

      if (pTkCurrent->nTkIdxEnd == 0) {
         m_Parser.SetError(nIdx, "Unclosed [ in optional argument");
         return false;
      }

      int nOptArgEnd = pTkCurrent->nTkIdxEnd;

      // Check if single token or group
      int nContentStart = nOptArgStart + 1;
      int nContentEnd = nOptArgEnd;

      // Skip leading space
      const STexToken* pContentTk = GetToken(nContentStart);
      if (pContentTk && pContentTk->nType == ettSPACE) {
         ++nContentStart;
         pContentTk = GetToken(nContentStart);
      }

      // Skip trailing space
      const STexToken* pLastTk = GetToken(nContentEnd - 1);
      if (pLastTk && pLastTk->nType == ettSPACE)
         --nContentEnd;

      if (nContentEnd - nContentStart == 1) 
         // Single token - store its index
         macroDef.nDefArgTkIdx = nContentStart;
      else // Multiple tokens - store opening [ index
         macroDef.nDefArgTkIdx = nOptArgStart;

      nIdx = nOptArgEnd + 1;  // Move past ]
      pTkCurrent = GetToken(nIdx);
   }

   // 4. Parse body: {body}
   // Skip space
   if (pTkCurrent && (pTkCurrent->nType == ettSPACE || pTkCurrent->nType == ettCOMMENT))
      pTkCurrent = GetToken(++nIdx);

   // Expect {
   if (!pTkCurrent || pTkCurrent->nType != ettFB_OPEN) {
      m_Parser.SetError(nIdx, "Expected { before macro body");
      return false;
   }
   _ASSERT(pTkCurrent->nTkIdxEnd > (uint32_t)nIdx); //snbh!
   macroDef.nBodyTkIdx = nIdx;  // Store opening { index
   nIdx = pTkCurrent->nTkIdxEnd + 1;  // Move past }
   // 5. Store macro definition
   m_mapMacros[sMacroName] = macroDef;

   return true;
}
//MACROS-EXPANSION
bool CMacroProcessor::ExpandToken(const vector<STexToken>& vMainTokens, 
                                    IN OUT int& nTkIdx, OUT vector<STexToken>& vExpanded) {
   vExpanded.clear();

   const STexToken& tkCmd = vMainTokens[nTkIdx];
   if(tkCmd.nType != ettCOMMAND)
      return false; //not a macro!
   _ASSERT(tkCmd.nRefIdx != -1); //macro token must have a valid reference here!
   const int nRefIdx = tkCmd.nRefIdx > 0 ? tkCmd.nRefIdx : nTkIdx + 1;
   string sCmdName = m_Parser.TokenText(&tkCmd);
   //0. nested nor in-document-macros not supported yet!
   if (sCmdName == "\\newcommand") {
      m_Parser.SetError(nRefIdx - 1, "Nested or in-place macro definitions are not supported");
      return false;
   }
   // 1. Find macro definition
   auto it = m_mapMacros.find(sCmdName);
   if (it == m_mapMacros.end())
      return false; // Not a macro
   const SMacroDef& macroDef = it->second;
   // 2. Extract arguments (if any)
   vector<vector<STexToken>> vArgs;
   if (macroDef.nNumArgs > 0) {
      if (!ExtractArguments_(nRefIdx, macroDef, vMainTokens, nTkIdx, vArgs))
         return false; // Error: missing/invalid arguments
   }
   else
      ++nTkIdx; //move to next token!
   // 3. Get body token range
   int nBodyStart = macroDef.nBodyTkIdx, nBodyEnd = macroDef.nBodyTkIdx + 1;
   const STexToken* pBodyTk = GetToken(macroDef.nBodyTkIdx);
   _ASSERT_RET(pBodyTk, false); //snbh!
   if (pBodyTk->nTkIdxEnd > (uint32_t)nBodyStart) {
      nBodyEnd = pBodyTk->nTkIdxEnd; //end of group
      ++nBodyStart; //skip {
   }
   // 4. Clone and process body tokens
   for (int nIdx = nBodyStart; nIdx < nBodyEnd; ++nIdx) {
      STexToken tk = m_vTokens[nIdx]; // Clone
      tk.nRefIdx = nRefIdx; //1-based original token index
      // A. Replace \begingroup -> {
      if (tk.nType == ettCOMMAND) {
         string sCmd = m_Parser.TokenText(&tk);
         if (sCmd == "\\begingroup") {
            tk.nType = ettFB_OPEN;
            vExpanded.push_back(tk);
            continue;
         }
         if (sCmd == "\\endgroup") {
            tk.nType = ettFB_CLOSE;
            vExpanded.push_back(tk);
            continue;
         }
      }
      // B. Replace ettMARG1-9 with argument tokens
      if (tk.nType >= ettMARG1 && tk.nType <= ettMARG9) {
         int nArgIdx = tk.nType - ettMARG1;
         if (nArgIdx < vArgs.size()) {
            // Insert all tokens from this argument
            for (const STexToken& argTk : vArgs[nArgIdx]) {
               vExpanded.push_back(argTk);
               //set ref for orig token in macro-tokens (only)
               if(vExpanded.back().nRefIdx == -1)
                  vExpanded.back().nRefIdx = nRefIdx;
            }
            continue;
         }
      }
      // C. Keep other tokens unchanged
      vExpanded.push_back(tk);
   } //for body tokens
   return true;
}
bool CMacroProcessor::ExtractArguments_(int nRefIdx,const SMacroDef& macroDef,
                                        const vector<STexToken>& vMainTokens, 
                                 IN OUT int& nTkIdx, OUT vector<vector<STexToken>>& vArgs) {
   vArgs.clear();
   if(!macroDef.nNumArgs)
      return true; //no args!
   ++nTkIdx;// move to next token after \macro
   //check first optional arg: must be in []!
   int nArgIdx = 0;
   if (macroDef.nDefArgTkIdx) {
      vArgs.emplace_back();
      vector<STexToken>& vArgTokens = vArgs.back();
      const STexToken& tkCurrent = vMainTokens[nTkIdx]; //token after macro name
      if (tkCurrent.nType == ettSB_OPEN) {
         for (uint32_t nIdx = nTkIdx + 1; nIdx < tkCurrent.nTkIdxEnd; ++nIdx) {
            //copy tokens
            vArgTokens.push_back(vMainTokens[nIdx]);
         }
         nTkIdx = tkCurrent.nTkIdxEnd + 1;
      }
      else {
         //copy default arg tokens
         const STexToken& tkDefArg = m_vTokens[macroDef.nDefArgTkIdx];
         if (tkDefArg.nType == ettSB_OPEN) {
            for (uint32_t nIdx = macroDef.nDefArgTkIdx + 1; nIdx < tkDefArg.nTkIdxEnd; ++nIdx) {
               //copy MACRO tokens
               vArgTokens.push_back(m_vTokens[nIdx]);
            }
         }
         else //single macro token
            vArgTokens.push_back(tkDefArg);
      }
      ++nArgIdx;
   }
   //continue with non-optional args
   for (; nArgIdx < macroDef.nNumArgs; ++nArgIdx) {
      if (nTkIdx >= vMainTokens.size()) {
         m_Parser.SetError(nRefIdx - 1, "Unexpected end of input while extracting macro arguments");
         return false;
      }
      const STexToken* pTkCurrent = &vMainTokens[nTkIdx];
      if (pTkCurrent->nType != ettFB_OPEN) {
         m_Parser.SetError(nRefIdx - 1, "Expected {macro argument}");
         return false;
      }
      vArgs.emplace_back();
      vector<STexToken>& vArgTokens = vArgs.back();
      for (uint32_t nIdx = nTkIdx+1; nIdx < pTkCurrent->nTkIdxEnd; ++nIdx) {
         //copy tokens
         vArgTokens.push_back(vMainTokens[nIdx]);
      }
      nTkIdx = pTkCurrent->nTkIdxEnd + 1;//skip }
   }
   return true;
}


