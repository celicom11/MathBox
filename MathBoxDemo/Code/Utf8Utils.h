#pragma once
//static
namespace Utf8Utils {
   //conversion helpers
   static bool _W2UA(const std::wstring& wsUtf16, OUT std::string& sUtf8, bool bUTF8) {
      // Special case of empty input string
      if (wsUtf16.empty()) {
         sUtf8.clear();
         return true;
      }

      // Get length (in chars) of resulting UTF-8 string
      const int utf8_length = ::WideCharToMultiByte(bUTF8 ? CP_UTF8 : CP_ACP,	// convert to ANSI/UTF-8
         0,                        // default flags
         wsUtf16.data(),           // source UTF-16 string
         (int)wsUtf16.length(),         // source string length, in wchar_t's,
         NULL,                     // unused - no conversion required in this step
         0,                        // request buffer size
         NULL, NULL                // unused
      );
      if (utf8_length <= 0)
         return false;

      // Allocate destination buffer for UTF-8 string
      sUtf8.resize(utf8_length);

      // Do the conversion from UTF-16 to UTF-8
      return !!::WideCharToMultiByte(bUTF8 ? CP_UTF8 : CP_ACP,	// convert from ANSI/UTF-8
         0,                        // default flags
         wsUtf16.data(),           // source UTF-16 string
         (int)wsUtf16.length(),         // source string length, in wchar_t's,
         &sUtf8[0],                // destination buffer
         utf8_length,              // destination buffer size, in chars
         NULL, NULL                // unused
      );
   }
   static bool _W2U(const wchar_t* wszIn, int nLen, OUT std::string& sUtf8) {
      std::wstring wsIn(wszIn, nLen >= 0 ? nLen : wcslen(wszIn));
      return _W2UA(wsIn, sUtf8, true);
   }
   //read ANY file to Utf8 string
   static bool _readFile2Utf8(const wstring& sDocPath, OUT string& sUtf8) {
      //read file to tmp buffer
      FILE* fp = _wfsopen(sDocPath.c_str(), L"rb", _SH_DENYNO);
      if (!fp)
         return false;
      //move on
      fseek(fp, 0, SEEK_END);
      long length = ftell(fp);
      fseek(fp, 0, SEEK_SET);
      if (length <= 0) {
         fclose(fp);
         return true; //empty file
      }
      // allocate memory:
      vector<char> vBuf(length + 2);
      char* szBuf = vBuf.data();
      // read data as a block:
      size_t nread = fread(szBuf, 1, length, fp);
      szBuf[nread] = szBuf[nread + 1] = 0;
      fclose(fp);
      //check for Utf16LE
      if (szBuf[0] == 0xFF && szBuf[1] == 0xFE) {
         szBuf += 2;//skip BOM
         nread -= 2;
         //convert UTF16->UTF8
         if (!_W2U((PCWSTR)szBuf, (int)nread, sUtf8)) {
            //TRACE("Failed to convert UTF16 to UTF8!\n");
            return false;
         }
      }
      else {
         //treat it as UTF8
         if (szBuf[0] == 0xEF && szBuf[1] == 0xBB && szBuf[2] == 0xBF) {
            szBuf += 3;
            nread -= 3;
         }
         sUtf8.assign(szBuf, nread);
      }
      return true;
   }

};