#pragma once
#include "LMMConsts.h"

struct FontStyleHelper {
   static bool _GetTextFontStyle(const string& sFontCmd, OUT STextFontStyle& tfStyle) {
      if (sFontCmd.empty()) {
         tfStyle = _aTexFontCmds[0]; //default
         return true;
      }
      // else 
      string sFCmd = sFontCmd[0] == '\\' ? &sFontCmd[1] : &sFontCmd[0];
      for (const STextFontStyle& tfCmd : _aTexFontCmds) {
         if (sFCmd == tfCmd.szTextFontStyle) {
            tfStyle = tfCmd;
            return true;
         }
      }
      return false;
   }
   static bool _GetMathFontStyle(const string& sFontCmd, OUT SMathFontStyle& mfStyle) {
      if (sFontCmd.size() < 2)
         return false;
      for (const SMathFontStyle& mfCmd : _aMathFontCmds) {
         if (sFontCmd == mfCmd.szMathFontStyle) {
            mfStyle = mfCmd;
            return true;
         }
      }
      return false;
   }
};