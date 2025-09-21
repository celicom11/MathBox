#include "stdafx.h"
#include "LMFontManager.h"

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
}
bool CLMFontManager::_IsTexFontCommand(PCSTR szCmd){
   _ASSERT_RET(szCmd && *szCmd == '\\', false);
   for (const SLatexFontCmd& lfCmd  : _aTexFontCmds) {
      if (0 == strcmp(szCmd, lfCmd.szLatexCmd))
         return true;
   }
   return false;
}
//symbol glyps \Pi and or operators \mathplus, etc.
const SLMMGlyph* CLMFontManager::GetLMMGlyphByCmd(PCSTR szCmd) const {
   if (!szCmd || szCmd[0] != '\\')
      return nullptr;
   ++szCmd; //skip \\
   // LMM font - search by LaTex cmd!
   const auto itGI = std::find_if(m_vGlyphInfo.begin(), m_vGlyphInfo.end(),
      [szCmd](const SLMMGlyph* pG) {
         return pG->sLaTexCmd== szCmd;
      });

   return itGI == m_vGlyphInfo.end() ? nullptr : (*itGI);

}
bool CLMFontManager::_GetLaTexFontCmdInfo(const string& sFontCmd, OUT SLatexFontCmd& lfCmdOut) {
   _ASSERT_RET(sFontCmd.size()>1 && sFontCmd[0] == '\\', false);
   for (const SLatexFontCmd& lfCmd : _aTexFontCmds) {
      if (sFontCmd == lfCmd.szLatexCmd) {
         lfCmdOut = lfCmd;
         return true;
      }
   }
   return false;
}
//
HRESULT CLMFontManager::Init(const wstring& sAppDir, IDWriteFactory* pDWriteFactory) {
   HRESULT hRes = LoadLatinModernFonts_(sAppDir, pDWriteFactory);
   CHECK_HR(hRes);
   // Font metrics for baseline calculations (design units)
   // m_vFontFaces[0]->GetMetrics(&m_fm);
   //_ASSERT(m_fm.designUnitsPerEm == otfUnitsPerEm);
   
   bool bOk = LoadLMMGlyphInfo_(sAppDir + L"\\LMMGlyphInfo\\LatinModernMathGlyphs.csv");
   _ASSERT_RET(bOk, E_FAIL);
   return S_OK;

}
const SLMMGlyph* CLMFontManager::GetLMMGlyph(int16_t nFontIdx, uint32_t nUnicode) const {
   static SLMMGlyph _LMMG;

   if (nFontIdx != 0) {
      // For non-LMM fonts, create basic glyph info
      _LMMG.eClass = egcOrd;
      _LMMG.nUnicode = nUnicode;
      _LMMG.nIndex = 0;
      _LMMG.nItalCorrection = 0;
      _LMMG.nTopAccentX = 0;

      // Set italic correction for italic fonts
      if (nFontIdx == FONT_ROMAN_ITALIC || nFontIdx == FONT_ROMAN_BOLDITALIC ||
         nFontIdx == FONT_SANS_OBLIQUE || nFontIdx == FONT_SANS_BOLDOBLIQUE) {
         //TODO!
         //_LMMG.nItalCorrection = GetEstimatedItalicCorrection(nUnicode);
      }

      return &_LMMG;
   }

   // LMM font - search in loaded glyph info
   const auto itGI = std::find_if(m_vGlyphInfo.begin(), m_vGlyphInfo.end(),
      [nUnicode](const SLMMGlyph* pG) {
         return pG->nUnicode == nUnicode;
      });

   return itGI == m_vGlyphInfo.end() ? nullptr : (*itGI);
}

///
HRESULT CLMFontManager::LoadLatinModernFonts_(const wstring& sAppDir, IDWriteFactory* pDWriteFactory) {
   HRESULT hRes;
   for (PCWSTR szFontFile : _aLMFonts) {
      IDWriteFontFace* pFontFace = nullptr;
      hRes = LoadLatinModernFont_(sAppDir+ L"\\LatinModernFonts\\" +szFontFile, pDWriteFactory, &pFontFace);
      CHECK_HR(hRes);
      m_vFontFaces.push_back(pFontFace);
   }
   return hRes;
}
HRESULT CLMFontManager::LoadLatinModernFont_(const wstring& sFontPath, IDWriteFactory* pDWriteFactory, 
                                            OUT IDWriteFontFace** ppFontFace) {
   //wstring sFontPath = sDir + L"\\LatinModernFonts\\" + szFontFile;
   IDWriteFontFile* pFontFile = nullptr;
   HRESULT hr = pDWriteFactory->CreateFontFileReference(sFontPath.c_str(), nullptr, &pFontFile);
   CHECK_HR(hr);
   BOOL isSupported;
   DWRITE_FONT_FILE_TYPE fileType;
   DWRITE_FONT_FACE_TYPE faceType;
   UINT32 numberOfFonts;
   pFontFile->Analyze(&isSupported, &fileType, &faceType, &numberOfFonts);

   IDWriteFontFile* fontFileArray[] = { pFontFile };
   hr = pDWriteFactory->CreateFontFace(
      faceType, //DWRITE_FONT_FACE_TYPE_CFF,
      1, // file count
      fontFileArray,
      0, // face index
      DWRITE_FONT_SIMULATIONS_NONE,
      ppFontFace
   );
   pFontFile->Release();
   return hr;
}

//nCodePoint,sName,sLaTex,eGlyphClass,nTopAccentX,nItalicCorr
bool CLMFontManager::LoadLMMGlyphInfo_(const wstring& sPathCSV) {
   ifstream ifile(sPathCSV);
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

