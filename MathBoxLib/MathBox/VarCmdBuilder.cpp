#include "stdafx.h"
#include "VarCmdBuilder.h"
#include "WordItem.h"

bool CVarCmdBuilder::CanTakeCommand(PCSTR szCmd) const {
   return (0 == strcmp(szCmd, "\\the") || 0 == strcmp(szCmd, "\\setlength") ||
            0 == strcmp(szCmd, "\\addtolength"));
}
CMathItem* CVarCmdBuilder::BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) {
   if (strcmp(szCmd, "\\setlength") == 0)
      return BuildSetLength_(pParser, false);
   if (strcmp(szCmd, "\\addtolength") == 0)
      return BuildSetLength_(pParser, true);
   if (strcmp(szCmd, "\\the") == 0)
      return BuildThe_(pParser);
   _ASSERT_RET(0,nullptr); //snbh!
}
CMathItem* CVarCmdBuilder::BuildSetLength_(IParserAdapter* pParser, bool bAdd) {
   
   string sText;
   EnumTokenType ett = pParser->GetTokenData(sText);
   // Skip optional space
   if (ett == ettSPACE) {
      pParser->SkipToken();
      ett = pParser->GetTokenData(sText);
   }
   // Check for optional braces around variable name: {\var} or \var
   bool bInBraces = false;
   if (ett == ettFB_OPEN) {
      bInBraces = true;
      pParser->SkipToken();
      ett = pParser->GetTokenData(sText);
      // Skip space after {
      if (ett == ettSPACE) {
         pParser->SkipToken();
         ett = pParser->GetTokenData(sText);
      }
   }
   // Get variable name (command)
   if (ett != ettCOMMAND) {
      pParser->SetError("Expected variable name");
      return nullptr;
   }
   string sVarName = sText;
   // Get current value if adding
   float fCurrentVal = 0.0f;
   if (bAdd) {
      if (!pParser->GetLength(sVarName, fCurrentVal)) {
         pParser->SetError("Variable " + sVarName + " not defined");
         return nullptr;
      }
   }

   pParser->SkipToken();
   ett = pParser->GetTokenData(sText);

   if (bInBraces) {
      // Skip space before }
      if (ett == ettSPACE) {
         pParser->SkipToken();
         ett = pParser->GetTokenData(sText);
      }
      // Expect closing brace
      if (ett != ettFB_CLOSE) {
         pParser->SetError("Expected } after variable name");
         return nullptr;
      }
      pParser->SkipToken();
      ett = pParser->GetTokenData(sText);
   }

   float fDimension = 0.0f;
   string sOrigUnits;//ok if empty - default pt will be applied in GetLength/SetLength
   if (!pParser->ConsumeDimension(elcapAny, fDimension, sOrigUnits))
      return nullptr;  // Error already set by adapter   
   // Store result
   pParser->SetLength(sVarName, fCurrentVal + fDimension);
   return nullptr;
}
CMathItem* CVarCmdBuilder::BuildThe_(IParserAdapter* pParser) {
   // Get next token (variable name)
   string sText;
   EnumTokenType ett = pParser->GetTokenData(sText);

   if (ett != ettCOMMAND) {
      pParser->SetError("Expected variable name after \\the");
      return nullptr;
   }

   string sVarName = sText;
   pParser->SkipToken();

   // Get variable value
   float fValue = 0.0f;
   if (!pParser->GetLength(sVarName, fValue)) {
      pParser->SetError("Variable " + sVarName + " not defined");
      return nullptr;
   }

   // Convert to text: "10.5pt"
   char szBuffer[64];
   sprintf_s(szBuffer, "%.2fpt", fValue);

   // Create word item with the value
   CWordItem* pItem = new CWordItem(pParser->Doc(), FONT_LMM, pParser->GetContext().currentStyle);
   pItem->SetTextA(szBuffer);
   return pItem;
}