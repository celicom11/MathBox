#include "stdafx.h"
#include "Tokenizer.h"

namespace { //static helpers
   inline bool _IsCRLF(char wc) { return wc == '\n' || wc == '\r'; }
   inline bool _IsGAP(char wc) { return wc == ' ' || wc == '\t'; }
   inline bool _IsSpace(char wc) { return _IsGAP(wc) || _IsCRLF(wc); }
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
   inline PCSTR _SkipSpaces(PCSTR szText) {
      while (_IsSpace(*szText)) {
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
      //moved to TextProcessor
      //if(*szPos && _IsSpace(*szPos))
      //   ++szPos; //post space must be eaten!
      return szPos;
   }
}
bool CTokenizer::Tokenize(OUT vector<STexToken>& vTokens, OUT ParserError& err) {
   vTokens.clear();
   PCSTR szPos = m_sText.c_str();
   while (1) {
      szPos = _SkipSpaces(szPos);
      if(!*szPos)
         break; //done!
      STexToken token;
      token.nPos = (uint32_t)(szPos - m_sText.c_str());
      if (*szPos == '\\') {
         ++szPos; //skip '\'
         if (!*szPos) {//lone '\' is not valid!
            err.eStage = epsTOKENIZING;
            err.nPosition = token.nPos;
            err.sError = "Orphan backslash '\\' at end of input.";
            return false;
         }
         if (_IsEscapedChar(*szPos)) {//skip '\\',leave single-non-alpha char token, 
            token.nType = ettNonALPHA;
            token.nLen = 2;
            //++token.nPos; // skip '\\'
            vTokens.push_back(token);
            ++szPos;      // next char
            continue;
         }
         //newline, glue and isplay mode start/end commands
         if (_IsCmdNonAlphaChar(*szPos))
            ++szPos; //skip special 1-non-alpha-cmd-char
         else if (_IsCharCmd(szPos)) {
            //\char{"}### unicode char command
            szPos = _ConsumeCharCmd(szPos);
         }
         else {
            //command\sym - sequence of letters
            while (_IsAlpha(*szPos)) {
               ++szPos;
            }
         }
         token.nType = ettCOMMAND;
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
         while (*szPos && !_IsCRLF(*szPos)) {
            ++szPos;
         }
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
      token.nLen = (uint16_t)((szPos - m_sText.c_str()) - token.nPos);
      vTokens.push_back(token);
   }//while
   return true;
}