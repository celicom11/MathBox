#pragma once
#include "MathItem.h"
//forward decls
class CTexParser;
class CRawItem;

class CTextModeProcessor {
 //DATA
   CTexParser&                m_Parser;      //owner
   vector<IMathItemBuilder*>  m_vItemBuilders;
public:
//CTOR/DTOR/INIT
   CTextModeProcessor()  = delete;
   CTextModeProcessor(CTexParser& parser);
   ~CTextModeProcessor();
//ATTS
   const STexToken* GetToken(int nIdx) const;
   string TokenText(int nIdx) const;
   //processing methods
   CMathItem* ProcessGroup(IN OUT int& nIdx, const SParserContext& ctx);
   CMathItem* ProcessItemToken(IN OUT int& nIdx, const SParserContext& ctx);
   void RegisterBuilder(IMathItemBuilder* pBuilder) {
      m_vItemBuilders.push_back(pBuilder);
   }
private:
   CMathItem* BuildItemWordToken_(IN OUT int& nIdx, const SParserContext& ctx);
   CMathItem* BuildItemCmd_(IN OUT int& nIdx, const SParserContext& ctx);
   //context-modifying commands
   bool ProcessFontSizeCmd_(IN OUT int& nIdx, IN OUT SParserContext& ctx);
   //
   //group's output raw item packers
   CMathItem* PackGroupItems_(vector<CRawItem>& vGroupItems, const SParserContext& ctx);
};