#pragma once
#include "MathItem.h"
//forward decls
class CTexParser;
class CRawItem;
class CEnvHelper;

class CMathModeProcessor {
 //DATA
   CTexParser&                m_Parser;      //owner
   vector<IMathItemBuilder*>  m_vItemBuilders;
public:
//CTOR/DTOR/INIT
   CMathModeProcessor()  = delete;
   CMathModeProcessor(CTexParser& parser);
   ~CMathModeProcessor();
//ATTS
   const STexToken* GetToken(int nIdx) const;
   string TokenText(int nIdx) const;
   //processing methods
   CMathItem* ProcessGroup(IN OUT int& nIdx, const SParserContext& ctx, IN OUT CEnvHelper& envh);
   CMathItem* ProcessItemToken(IN OUT int& nIdx, const SParserContext& ctx);
   void RegisterBuilder(IMathItemBuilder* pBuilder) {
      m_vItemBuilders.push_back(pBuilder);
   }
private:
   CMathItem* ProcessAlnum_(IN OUT int& nIdx, const SParserContext& ctx);
   CMathItem* ProcessNonAlnum_(IN OUT int& nIdx, const SParserContext& ctx);
   CMathItem* BuildItemCmd_(IN OUT int& nIdx, const SParserContext& ctx);
   //context-modifying commands
   bool ProcessMathStyleCmd_(IN OUT int& nIdx, IN OUT SParserContext& ctx);
   bool ProcessFontSizeCmd_(IN OUT int& nIdx, IN OUT SParserContext& ctx);
   //
   //group's output raw item packers
   CMathItem* PackGroupItems_(vector<CRawItem>& vGroupItems, const SParserContext& ctx);
   CMathItem* PackGroupItemsLeftRight_(vector<CRawItem>& vGroupItems, const SParserContext& ctx);
   CMathItem* PackGroupItemsTabularEnv_(vector<CRawItem>& vGroupItems, const SParserContext& ctx,
                                          const CEnvHelper& envh);
};