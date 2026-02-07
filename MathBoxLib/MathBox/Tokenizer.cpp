#include "stdafx.h"
#include "Tokenizer.h"

namespace { //static helpers
   inline bool _IsCRLF(char wc) { return wc == '\n' || wc == '\r'; }
   inline bool _IsGAP(char wc) { return wc == ' ' || wc == '\t'; }
   inline bool _IsDigit(char wc) { return wc >= '0' && wc <= '9'; }
   inline bool _IsHexDigit(char wc) { 
      return _IsDigit(wc) || (wc >= 'a' && wc <= 'f') || (wc >= 'A' && wc <= 'F');
   }
   inline bool _IsAlpha(char wc) { return (wc >= 'a' && wc <= 'z') || (wc >= 'A' && wc <= 'Z'); }
   inline bool _IsAlNum(char wc) { return _IsAlpha(wc) || _IsDigit(wc); }

   inline bool _IsEscapedChar(char cChar) {
      static const string _sSpecial{ "#$%&_{}" };
      return _sSpecial.find(cChar) != string::npos;
   }
   inline bool _IsCmdNonAlphaChar(char cChar) {
      static const string _sSpecial{ " ,:;!>.^'~`=\"[]\\|" };
      return _sSpecial.find(cChar) != string::npos;
   }
   inline PCSTR _SkipSpaces(PCSTR szText, OUT int& nSpaces) {
      nSpaces = 0;
      while (_IsGAP(*szText) || _IsCRLF(*szText)) {
         if(*szText != 'r') // sorry OLD-MAC users :/
            ++nSpaces;
         ++szText;
      }
      return szText;
   }
   //char{"}###
   inline bool _IsCharCmd(PCSTR szCmd) {
      if(szCmd[0] != 'c' || szCmd[1] != 'h' || szCmd[2] != 'a' || szCmd[3] != 'r')
         return false;
      szCmd += 4;
      if (*szCmd == '"') {
         ++szCmd;
         //verify next is hex digit
         if (!_IsHexDigit(*szCmd))
            return false;
      }
      else if (!_IsDigit(*szCmd))
         return false;
      return true;
   }
   //presume _IsCharCmd == TRUE!
   PCSTR _ConsumeCharCmd(PCSTR szCmd) {
      PCSTR szPos = szCmd + 4; //skip "char"
      if (*szPos == '"') {
         ++szPos; //skip '"'
         while (_IsHexDigit(*szPos)) {
            ++szPos;
         }
      }
      else {
         while (_IsDigit(*szPos)) {
            ++szPos;
         }
      }
      return szPos;
   }
   //\verb<delim>...<delim>
   inline bool _IsVerbCmd(PCSTR szCmd) {
      if (szCmd[0] != 'v' || szCmd[1] != 'e' || szCmd[2] != 'r' || szCmd[3] != 'b')
         return false;
      szCmd += 4;
      if (_IsAlNum(*szCmd))
         return false; //delimiter cannot be alnum
      return true;
   }

}
bool CTokenizer::Tokenize(OUT vector<STexToken>& vTokens, OUT ParserError& err) {
   vTokens.clear();
   PCSTR szPos = m_sText.c_str();
   while (1) {
      STexToken token;
      if(m_bIsMacroText)
         token.nRefIdx = -1; //mark as macro text token
      token.nPos = (uint32_t)(szPos - m_sText.c_str());
      int nSpaces = 0;
      szPos = _SkipSpaces(szPos, nSpaces);
      if(!*szPos)
         break; //done, ignore trailing spaces
      if(nSpaces) {
         token.nType = ettSPACE;
         token.nLen = (uint16_t)nSpaces;
         vTokens.push_back(token);
         continue;
      }
      if (*szPos == '\\') {
         token.nType = ettCOMMAND;
         ++szPos; //skip '\'
         if (!*szPos) {//lone '\' is not valid!
            err.SetError(epsTOKENIZING, token, "Orphan backslash '\\' at end of input.");
            return false;
         }
         if (_IsEscapedChar(*szPos)) {
            //\{special char}
            token.nType = ettNonALPHA; //override type
            ++szPos;      // next char
         }
         else if (_IsCmdNonAlphaChar(*szPos)) //newline, glue and isplay mode start/end commands
            ++szPos; //skip special 1-non-alpha-cmd-char
         else if (_IsCharCmd(szPos))  //\char{"}### unicode char command
            szPos = _ConsumeCharCmd(szPos);
         else if (_IsVerbCmd(szPos)) {
            token.nLen = 5;
            char cDelim = *(szPos + 4); //delimiter char
            szPos += 5; //skip 'verb' + delim
            if (*szPos == cDelim) {
               //skip empty \verb?? item completely
               ++szPos; //skip closing delim
               continue;
            }
            while (*szPos && *szPos != cDelim) {
               if(_IsCRLF(*szPos)) {
                  err.SetError(epsTOKENIZING, token, "\\verb command cannot span multiple lines.");
                  return false;
               }
               ++szPos;
            }
            if(!*szPos) {
               err.SetError(epsTOKENIZING, token, "\\verb command missing closing delimiter.");
               return false;
            }
            //create two tokens and continue
            vTokens.push_back(token);
            token.nType = ettALNUM; //single word token
            token.nPos += 6;
            token.nLen = (uint16_t)(szPos - (m_sText.c_str() + token.nPos));
            vTokens.push_back(token);
            ++szPos; //skip closing delim
            continue;
         }
         else {
            //command\sym - sequence of letters\digits
            while (_IsAlNum(*szPos) || (m_bIsMacroText && *szPos == '@')) {
               ++szPos;
            }
         }
      }
      else if (*szPos == '{') {
         token.nType = ettFB_OPEN;
         ++szPos;
      }
      else if (*szPos == '}') {
         token.nType = ettFB_CLOSE;
         ++szPos;
      }
      else if (*szPos == '[') {
         token.nType = ettSB_OPEN;
         ++szPos;
      }
      else if (*szPos == ']') {
         token.nType = ettSB_CLOSE;
         ++szPos;
      }
      else if (*szPos == '%') {
         //comment to EOL
         while (*szPos && *szPos != '\n') { // sorry OLD-MAC users 2 :/
            ++szPos;
         }
         if (szPos && *szPos == '\n')
            ++szPos;
         token.nType = ettCOMMENT;
      }
      else if (*szPos == '$') {
         token.nType = ett$;
         ++szPos;
         if(*szPos == '$') {
            token.nType = ett$$;
            ++szPos;
         }
      }
      else if (*szPos == '&') {
         token.nType = ettAMP;
         ++szPos;
      }
      else if (*szPos == '_') {
         token.nType = ettSUBS;
         ++szPos;
      }
      else if (*szPos == '^') {
         token.nType = ettSUPERS;
         ++szPos;
      }
      else if (_IsAlNum(*szPos)) {
         //sequence of letters/digits
         while (_IsAlNum(*szPos)) {
            ++szPos;
         }
         token.nType = ettALNUM;
      }
      else if(m_bIsMacroText && *szPos == '#') {
         //macro parameter in macro text
         ++szPos; //skip '#'
         if (*szPos < '1' || *szPos > '9') {
            err.SetError(epsTOKENIZING, token, "Invalid macro-arg number after '#'.");
            return false;
         }
         int nParamIdx = *szPos - '0';
         token.nType = (int16_t)(ettMARG1 + nParamIdx - 1);
         ++szPos;//skip arg number
      }
      else {//All others: no checks; presume punctuations, operators, etc. - single char per token
         token.nType = ettNonALPHA;
         ++szPos;
      }
      //adjust post command spacing if needed
      if (token.nType == ettALNUM && vTokens.size() >= 2) {
         STexToken& tkPrev = vTokens.back();
         const STexToken& tkPrevPrev = vTokens[vTokens.size()-2];
         if (tkPrev.nType == ettSPACE && tkPrevPrev.nType == ettCOMMAND) {
            --tkPrev.nLen;
            if (tkPrev.nLen < 1)
               vTokens.pop_back(); //remove empty token
         }
      }
      if (!token.nLen) //could be set already, e.g.: for SPACE tokens
         token.nLen = (uint16_t)((szPos - m_sText.c_str()) - token.nPos);
      vTokens.push_back(token);
   }//while
   return true;
}