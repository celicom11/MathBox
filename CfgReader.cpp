#include "StdAfx.h"
#include "CfgReader.h"

namespace {
   void _TrimString(IN OUT wstring& wsStr) {
      while (!wsStr.empty() && iswspace(wsStr.front()))
         wsStr.erase(wsStr.begin());
      while (!wsStr.empty() && iswspace(wsStr.back()))
         wsStr.pop_back();
   }
};
bool CCfgReader::Init(PCWSTR wszPath) {
   m_mapDict.clear();
   wifstream inFile;
   inFile.open(wszPath, wifstream::in);
   if (!inFile)
      return false;
   wstring wsLine, wsKey, wsVal;
   while (std::getline(inFile, wsLine)) {
      if (wsLine.front() == L';' || wsLine.find('=') == wstring::npos) //skip comments/garbage lines
         continue;
      uint32_t nEqPos = (uint32_t)wsLine.find(L'=');
      wsKey = wsLine.substr(0, nEqPos);
      _TrimString(wsKey);
      wsVal = wsLine.substr(nEqPos + 1);
      nEqPos = (uint32_t)wsVal.find(L';');
      if (nEqPos != wstring::npos) 
         wsVal = wsVal.substr(0, nEqPos);
      _TrimString(wsVal);
      if (wsKey.empty() || wsVal.empty() || m_mapDict.find(wsKey) != m_mapDict.end()) {  //duplicates are not allowed
         _ASSERT(0);
         return false;
      }
      m_mapDict[wsKey] = wsVal;
   }
   return true;
}
//METHODS
bool CCfgReader::GetSVal(PCWSTR wszKey, IN OUT wstring& wsVal) {
   if (!wszKey || m_mapDict.find(wszKey) == m_mapDict.end())
      return false;
   wsVal = m_mapDict[wszKey];
   return true;
}
bool CCfgReader::GetNVal(PCWSTR wszKey, IN OUT int32_t& nVal) {
   if (!wszKey || m_mapDict.find(wszKey) == m_mapDict.end())
      return false;
   nVal = std::stoi(m_mapDict[wszKey]);
   return true;
}
bool CCfgReader::GetNVal(PCWSTR wszKey, IN OUT uint32_t& nVal) {
   if (!wszKey || m_mapDict.find(wszKey) == m_mapDict.end())
      return false;
   nVal = std::stoul(m_mapDict[wszKey]);
   return true;
}
bool CCfgReader::GetHVal(PCWSTR wszKey, IN OUT uint32_t& nVal) {
   if (!wszKey || m_mapDict.find(wszKey) == m_mapDict.end())
      return false;
   nVal = std::stoul(m_mapDict[wszKey], nullptr, 16);
   return true;
}

bool CCfgReader::GetBVal(PCWSTR wszKey, IN OUT bool& bVal) {
   if (!wszKey || m_mapDict.find(wszKey) == m_mapDict.end())
      return false;
   wstring wsVal = m_mapDict[wszKey];
   bVal = wsVal.front() == L'1' || wsVal.front() == L'y' || wsVal.front() == L'Y'; //todo: overflow check
   return true;
}
bool CCfgReader::GetFVal(PCWSTR wszKey, IN OUT float& fVal) {
   if (!wszKey || m_mapDict.find(wszKey) == m_mapDict.end())
      return false;
   fVal = std::stof(m_mapDict[wszKey]);
   return true;
}

