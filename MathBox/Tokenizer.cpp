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
      static const string _sSpecial{ "$%&_{}" };
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
}
bool CTokenizer::Tokenize(OUT vector<STexToken>& vTokens, OUT ParserError& err) {
   vTokens.clear();
   PCSTR szPos = m_sText.c_str();
   while (1) {
      STexToken token;
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
         //newline, glue and isplay mode start/end commands
         else if (_IsCmdNonAlphaChar(*szPos))
            ++szPos; //skip special 1-non-alpha-cmd-char
         else if (_IsCharCmd(szPos)) {
            //\char{"}### unicode char command
            szPos = _ConsumeCharCmd(szPos);
         }
         else {
            //command\sym - sequence of letters\digits
            while (_IsAlNum(*szPos)) {
               ++szPos;
            }
            //todo: allow adding '*' at the end of the command?
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