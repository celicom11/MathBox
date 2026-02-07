#include "stdafx.h"
#include "OverlayBuilder.h"

namespace {
}
bool COverlayBuilder::CanTakeCommand(PCSTR szCmd) const {
   _ASSERT_RET(szCmd && *szCmd, false);
   static const vector<string> _vCmds{
      // Cancellation
      "cancel", "bcancel", "xcancel", "not","sout",
      // Annotation Lines
      "overline", "underline","underbar",
      "overlinesegment", "underlinesegment",
      // Angles (just straight lines!)
      "angl", "phase",
   };
   if (*szCmd == '\\')
      ++szCmd;
   return std::find(_vCmds.begin(), _vCmds.end(), szCmd) != _vCmds.end();
}
CMathItem* COverlayBuilder::BuildFromParser(PCSTR szCmd, IParserAdapter* pParser) {
   _ASSERT_RET(szCmd && *szCmd && pParser, nullptr);
   _ASSERT_RET(CanTakeCommand(szCmd), nullptr);
   string sCmd(szCmd + 1);
   vector< COverlayItem::SPolyLine> vOverlays;
   SRect rcBox{ 0,0,0,0 };
   CMathItem* pContent = GetContentWithOverlays_(sCmd, pParser, vOverlays, rcBox);
   if (!pContent)
      return nullptr;
   COverlayItem* pRet = new COverlayItem(pContent);
   if(rcBox.right > rcBox.left)
      pRet->SetBox(rcBox);
   for (const COverlayItem::SPolyLine& line : vOverlays) {
      pRet->AddOverlay(line);
   }
   pRet->UpdateLayout();
   return pRet;
}
CMathItem* COverlayBuilder::GetContentWithOverlays_(const string& sCmd, IParserAdapter* pParser,
                                       OUT vector< COverlayItem::SPolyLine>& vOverlays, 
                                       OUT SRect& rcBox) {
   const SParserContext& ctx = pParser->GetContext();
   //read content
   CMathItem* pContent = pParser->ConsumeItem(elcapAny, ctx);
   if (!pContent) {
      if (!pParser->HasError())
         pParser->SetError("Missing {arg} for '" + sCmd + "' command");
      return nullptr;
   }
   //default box = content box
   const STexBox& boxContent = pContent->Box();
   rcBox.left = boxContent.Left();
   rcBox.top = boxContent.Top();
   rcBox.right = boxContent.Right();
   rcBox.bottom = boxContent.Bottom();
   //define box and overlay lines relative to content box!
   COverlayItem::SPolyLine line;
   if (sCmd == "cancel" || sCmd == "bcancel" || sCmd == "xcancel") {
      const int32_t nExtFDU = DIPS2EM(pParser->Doc().DefaultFontSizePts(), PTS2DIPS(2.0f));
      if(boxContent.Width() > boxContent.Height()) 
         rcBox.right += nExtFDU, rcBox.left -= nExtFDU;
      else if (boxContent.Width() < boxContent.Height())
         rcBox.bottom += nExtFDU, rcBox.top -= nExtFDU;
      line.fWidth = PTS2DIPS(0.4f)* pParser->Doc().DefaultFontSizePts()/10.0f;
      if (sCmd == "cancel" ) {
         //left-bottom to right-top 
         line.vPoints.push_back({ 0, rcBox.bottom-rcBox.top});
         line.vPoints.push_back({ rcBox.right - rcBox.left, 0 });
         vOverlays.push_back(line);
      }
      else if (sCmd == "bcancel") {
         //left-top to right-bottom
         line.vPoints.push_back({ 0, 0});
         line.vPoints.push_back({ rcBox.right - rcBox.left, rcBox.bottom - rcBox.top });
         vOverlays.push_back(line);
      }
      else if (sCmd == "xcancel") {
         //cross
         line.vPoints.push_back({ 0, 0 });
         line.vPoints.push_back({ rcBox.right - rcBox.left, rcBox.bottom - rcBox.top});
         vOverlays.push_back(line);
         line.vPoints[0] = { 0, rcBox.bottom - rcBox.top};
         line.vPoints[1] = { rcBox.right - rcBox.left, 0};
         vOverlays.push_back(line);
      }
   }
   else if (sCmd == "sout") {
      line.fWidth = PTS2DIPS(0.6f) * pParser->Doc().DefaultFontSizePts() / 10.0f;
      //use math axis
      int32_t nAxisY = boxContent.AxisY() - rcBox.top;
      line.vPoints.push_back({ 0, nAxisY });
      line.vPoints.push_back({ rcBox.right - rcBox.left, nAxisY });
      vOverlays.push_back(line);
   }
   else if (sCmd == "not") {
      line.fWidth = PTS2DIPS(0.4f) * pParser->Doc().DefaultFontSizePts() / 10.0f;
      //overlay reverse solidus (56,250)-(406,-750) rel.baseline pt.->
      const int32_t nCenterX = (rcBox.left + rcBox.right) / 2;

      line.vPoints.push_back({ nCenterX - 350 / 2, boxContent.Ascent() + 250});
      line.vPoints.push_back({ nCenterX + 350 / 2, boxContent.Ascent() - 750 });
      vOverlays.push_back(line);
      pContent->SetAtom(etaREL); // make relation
   }
   else if (sCmd == "overline" || sCmd == "underline" || sCmd == "underbar") {
      line.fWidth = PTS2DIPS(0.4f) * pParser->Doc().DefaultFontSizePts() / 10.0f;
      //single line, different Y-positions
      int32_t nY = 0;
      if (sCmd == "overline")
         nY = -20 - otfOverbarVerticalGap;//q&d
      else if (sCmd == "underbar")
         nY = boxContent.BaselineY() - rcBox.top + 20 + otfUnderbarVerticalGap;
      else //if (sCmd == "underbar")
         nY = rcBox.bottom - rcBox.top + 20 + otfUnderbarVerticalGap;
      line.vPoints.push_back({ 0, nY });
      line.vPoints.push_back({ rcBox.right - rcBox.left, nY });
      vOverlays.push_back(line);
   }
   else if (sCmd == "overlinesegment" || sCmd == "underlinesegment") {
      line.fWidth = PTS2DIPS(0.4f) * pParser->Doc().DefaultFontSizePts() / 10.0f;
      //single line, different Y-positions
      const int32_t nSegmSideFDU = 300; //TODO:tune!
      int32_t nY = 0;
      if (sCmd == "overlinesegment") {
         nY = nSegmSideFDU/2;
         rcBox.top -= nSegmSideFDU + otfOverbarVerticalGap;
      }
      else {//sCmd == "underlinesegment"
         nY = rcBox.bottom - rcBox.top + nSegmSideFDU / 2 + otfUnderbarVerticalGap;
         rcBox.bottom += nSegmSideFDU + otfUnderbarVerticalGap;
      }
      //main bar
      line.vPoints.push_back({ 0, nY });
      line.vPoints.push_back({ rcBox.right - rcBox.left, nY });
      vOverlays.push_back(line);
      //left segment
      line.vPoints[0] = { 0, nY - nSegmSideFDU / 2 };
      line.vPoints[1] = { 0, nY + nSegmSideFDU / 2 };
      vOverlays.push_back(line);
      //right segment
      line.vPoints[0] = { rcBox.right - rcBox.left, nY - nSegmSideFDU / 2 };
      line.vPoints[1] = { rcBox.right - rcBox.left, nY + nSegmSideFDU / 2 };
      vOverlays.push_back(line);
   }
   else if (sCmd == "angl") {
      //add angle brackets - just lines!
      line.fWidth = PTS2DIPS(0.4f) * pParser->Doc().DefaultFontSizePts() / 10.0f;
      const int32_t nGap = otfUnderbarVerticalGap; //TODO:tune!
      //top bar
      line.vPoints.push_back({ rcBox.left, rcBox.top - nGap});
      line.vPoints.push_back({ rcBox.right + nGap/2, rcBox.top - nGap });
      vOverlays.push_back(line);
      //right bar
      line.vPoints[0] = { rcBox.right + nGap/2, rcBox.top - nGap };
      line.vPoints[1] = { rcBox.right + nGap/2, rcBox.bottom };
      vOverlays.push_back(line);

   }
   else if (sCmd == "phase") {
      line.fWidth = PTS2DIPS(0.4f) * pParser->Doc().DefaultFontSizePts() / 10.0f;
      // /____ angle 
      rcBox.bottom += otfUnderbarVerticalGap;//make some space
      const int32_t nHeight = rcBox.bottom - rcBox.top;
      const int32_t nDeltaX = nHeight * 577 / 1000;//~60 degree angle
      rcBox.left -= nDeltaX;
      //top-right to bottom-left
      line.vPoints.push_back({ nDeltaX, 0 });
      line.vPoints.push_back({ 0, nHeight });
      vOverlays.push_back(line);
      //bottom bar
      line.vPoints[0] = { 0, nHeight };
      line.vPoints[1] = { rcBox.right - rcBox.left, nHeight };
      vOverlays.push_back(line);
   }
   return pContent;
}

