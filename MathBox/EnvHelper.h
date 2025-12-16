#pragma once
#include "MathItem.h"
//forward decls
class CTexParser;

class CEnvHelper {
 //DATA
   int                        m_nStartTkIdx{ 0 }; //first token to process
   string                     m_sEnv;
   // Grid Configuration
   vector<EnumColAlignment>   m_vColAlignments;
   // inter-column separators/vert lines: 0, '|', ':', or 'd' for double '||'
   vector<char>               m_vVLines;       // Size = m_vColAlignments.size() + 1
   // Outer Delimiters unicodes (e.g. for pmatrix)
   char                       m_cLeftDelim { 0 };   // 0 if none
   char                       m_cRightDelim { 0 };  // 0 if none
public:
//CTOR/DTOR/INIT
   CEnvHelper() = default;
   ~CEnvHelper() = default;
//ATTS
   int GetStartTkIdx() const { return m_nStartTkIdx; }
   const string& GetEnvName() const { return m_sEnv; }
   const vector<EnumColAlignment>& ColAlignments() const { return m_vColAlignments; }
   const vector<char>& VertLines() const { return m_vVLines; }
   char LeftDelim() const { return m_cLeftDelim; }
   char RightDelim() const { return m_cRightDelim; }

   bool Enabled() const { return !m_sEnv.empty();}
   bool WantsScriptStyle() const {
      return m_sEnv == "smallmatrix" || m_sEnv == "subarray";
   }
//METHODS
   bool Init(CTexParser& parser, int nTkIdx); //nTkIdx must be ettCOMMAND, "\\begin"
   //CMathItem* PackItems(const SEnvTable& envTable, const CMathStyle& style, float fUserScale);
private:
   bool InitArrayColumnSpecs_(CTexParser& parser, int nIdxColSpec);
   bool InitEnvDelimiters_(OUT string& sError); //also, checks env validity
};