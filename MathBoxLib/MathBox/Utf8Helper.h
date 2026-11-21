#pragma once
#include <stdint.h>
#include <string>

struct Utf8Helper {
   // Advance one Unicode scalar. On failure leave the original byte offset intact.
   static bool _DecodeNext(const std::string& sText, size_t& nPos, uint32_t& nUnicode) {
      if (nPos >= sText.size())
         return false;
      const uint8_t nLead = (uint8_t)sText[nPos];
      size_t nBytes = 1;
      uint32_t nMin = 0;
      if (nLead < 0x80)
         nUnicode = nLead;
      else if (nLead >= 0xC2 && nLead <= 0xDF) {
         nBytes = 2; nMin = 0x80; nUnicode = nLead & 0x1F;
      }
      else if (nLead >= 0xE0 && nLead <= 0xEF) {
         nBytes = 3; nMin = 0x800; nUnicode = nLead & 0x0F;
      }
      else if (nLead >= 0xF0 && nLead <= 0xF4) {
         nBytes = 4; nMin = 0x10000; nUnicode = nLead & 0x07;
      }
      else
         return false;
      if (nBytes > sText.size() - nPos)
         return false;
      for (size_t nIdx = 1; nIdx < nBytes; ++nIdx) {
         const uint8_t nByte = (uint8_t)sText[nPos + nIdx];
         if ((nByte & 0xC0) != 0x80)
            return false;
         nUnicode = (nUnicode << 6) | (nByte & 0x3F);
      }
      if (nUnicode < nMin || nUnicode > 0x10FFFF ||
          (nUnicode >= 0xD800 && nUnicode <= 0xDFFF))
         return false;
      nPos += nBytes;
      return true;
   }
};
