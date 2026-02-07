#include "stdafx.h"
#include "LMMFont.h"
#include "MathItem.h"

namespace {
   inline bool _IsCRLF(char wc) { return wc == '\n' || wc == '\r'; }
   inline bool _IsGAP(char wc) { return wc == ' ' || wc == '\t'; }
   inline bool _IsSpace(char wc) { return _IsGAP(wc) || _IsCRLF(wc); }
   inline PCSTR _SkipSpaces(PCSTR szText) {
      while (_IsSpace(*szText)) {
         ++szText;
      }
      return szText;
   }
   void _SplitString(PCSTR szText, vector<string>& vOut) {
      vOut.clear();
      if (!szText || !*szText)
         return; //ntd
      PCSTR szStr1 = szText, szStr2;
      while (*szStr1 && (szStr2 = strchr(szStr1, ','))) {
         szStr1 = _SkipSpaces(szStr1);
         vOut.push_back(string(szStr1, szStr2 - szStr1));
         szStr1 = szStr2 + 1;
      }
      if (*szStr1) //last string
         vOut.push_back(szStr1);
   }
   inline PCSTR _ReplaceAlias(PCSTR szCmd) {
      for (auto aCmdAliases : _aLatexAliases) {
         if (0 == strcmp(szCmd, aCmdAliases[0]))
            return aCmdAliases[1];
      }
      return szCmd;
   }
}

//nCodePoint,sName,sLaTex,eGlyphClass,nTopAccentX,nItalicCorr
bool CLMMFont::Init(const wstring& sFontsDir) {
   ifstream ifile(sFontsDir+L"LatinModernMathGlyphs.csv");
   string sRow;
   vector<string> vRow;
   bool bFirst = true;
   while (!ifile.eof()) {
      std::getline(ifile, sRow);
      if (ifile.bad() || ifile.fail() || sRow.empty())
         break;
      _SplitString(sRow.c_str(), vRow);
      _ASSERT_RET(vRow.size() == 6, false);
      if (bFirst) {
         bFirst = false;
         continue;//skip header
      }
      m_vGlyphInfo.push_back(new SLMMGlyph);
      m_vGlyphInfo.back()->nIndex = (uint16_t)m_vGlyphInfo.size() - 1;
      PCSTR szUnicode = vRow[0].c_str();
      _ASSERT_RET(szUnicode && (*szUnicode == '0' || *szUnicode == 'x'), false);
      if (*szUnicode == 'x') {
         char* szEnd = nullptr;
         m_vGlyphInfo.back()->nUnicode = strtol(szUnicode + 1, &szEnd, 16);
         _ASSERT_RET(*szEnd == 0, false);
      }
      m_vGlyphInfo.back()->sName = vRow[1];
      m_vGlyphInfo.back()->sLaTexCmd = vRow[2];
      m_vGlyphInfo.back()->eClass = atoi(vRow[3].c_str());
      m_vGlyphInfo.back()->nTopAccentX = atoi(vRow[4].c_str());
      m_vGlyphInfo.back()->nItalCorrection = atoi(vRow[5].c_str());
      _ASSERT_RET(m_vGlyphInfo.back()->nUnicode || !m_vGlyphInfo.back()->sName.empty(), false);
   }
   return m_vGlyphInfo.size() >= 4800;//q&d
}
//symbol glyphs \Pi and or operators \mathplus, etc.
const SLMMGlyph* CLMMFont::GetLMMGlyphByCmd(PCSTR szCmd)  {
   static SLMMGlyph _lmmg;
   _ASSERT_RET(szCmd && *szCmd, nullptr);
   if (*szCmd == '\\')
      ++szCmd; //skip \\
   //special aliases/legacy
   string sCmd(szCmd); //save orig cmd
   szCmd = _ReplaceAlias(szCmd);
   if (*szCmd == '0' && szCmd[1] == 'x') {//search by unicode
      long lUni = strtol(szCmd, nullptr, 16);
      _ASSERT_RET(lUni > 0 && lUni < 0xFFFFF, nullptr);
      return GetLMMGlyph(FONT_LMM, (uint32_t)lUni);
   }
   // LMM font - search by LaTex cmd!
   const auto itGI = std::find_if(m_vGlyphInfo.begin(), m_vGlyphInfo.end(),
      [szCmd](const SLMMGlyph* pG) {
         return pG->sLaTexCmd == szCmd;
      });
   if (itGI == m_vGlyphInfo.end())
      return nullptr;
   const SLMMGlyph* pRet = (*itGI);
   //atom type adjustments
   //NOTE: dotso stays mathord
   if (sCmd == "dots" || sCmd == "ldots" || sCmd == "dotsc" ||
      sCmd == "colon" || sCmd == "ldotp") {
      _lmmg = *pRet;
      _lmmg.eClass = etaPUNCT;
      return &_lmmg;
   }
   if (sCmd == "cdots" || sCmd == "dotsb" || sCmd == "dotsm" || sCmd == "dotsi" ) {
      _lmmg = *pRet;
      _lmmg.eClass = etaINNER;
      return &_lmmg;
   }
   if (sCmd == "centerdot" || sCmd == "sdot") {
      _lmmg = *pRet;
      _lmmg.eClass = etaBIN;
      return &_lmmg;
   }
   if (sCmd == "coloncolon" || sCmd == "dblcolon" || sCmd == "ratio" || sCmd == "vcentcolon") {
      _lmmg = *pRet;
      _lmmg.eClass = etaREL;
      return &_lmmg;
   }
   return pRet;
}
//
const SLMMGlyph* CLMMFont::GetLMMGlyph(int16_t nFontIdx, uint32_t nUnicode)  {
   static SLMMGlyph _LMMG;

   if (nFontIdx != 0) {
      // For non-LMM fonts, create basic glyph info
      _LMMG.eClass = egcOrd;
      _LMMG.nUnicode = nUnicode;
      _LMMG.nIndex = 0;
      _LMMG.nTopAccentX = 0;

      // Set italic correction for italic fonts from OS/2, ySuperscriptXOffset
      switch (nFontIdx) {
      case FONT_ROMAN_ITALIC:
      case FONT_ROMAN_BOLDITALIC:   _LMMG.nItalCorrection = 87;    break;
      case FONT_SANS_OBLIQUE:
      case FONT_SANS_BOLDOBLIQUE:   _LMMG.nItalCorrection = 74;    break;
      case FONT_MONO_ITALIC:        _LMMG.nItalCorrection = 87;    break;
      default: _LMMG.nItalCorrection = 0;
      }

      return &_LMMG;
   }
   if(nUnicode == '-')
      nUnicode = 0x2212; //use proper minus sign in LMM
   // LMM font - search in loaded glyph info
   const auto itGI = std::find_if(m_vGlyphInfo.begin(), m_vGlyphInfo.end(),
      [nUnicode](const SLMMGlyph* pG) {
         return pG->nUnicode == nUnicode;
      });

   return itGI == m_vGlyphInfo.end() ? nullptr : (*itGI);
}
