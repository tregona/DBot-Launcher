// ServerFile.h: interface for the CServerFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SERVERFILE_H__E0DE2459_E834_4D0B_8A0C_66F4393E188A__INCLUDED_)
#define AFX_SERVERFILE_H__E0DE2459_E834_4D0B_8A0C_66F4393E188A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <wininet.h>
#include "ShowStatus.h"

#define LASTERROR_BUFFER_LENGTH 250
#define SERVER_BUFFER_LENGTH 120

class CServerFile  
{
public:
   virtual BOOL GetServerHeaders(LPCTSTR lpszServer, LPCTSTR lpszReqResource, LPTSTR lpBufferForHeaders, DWORD dwBufferLen) = 0;
   virtual BOOL DownloadFile(LPCTSTR lpszServer, LPCTSTR lpszReqResource, LPCTSTR lpszResFile) = 0;
   void SetTalker(CShowStatus* pTalker) { m_pTalker = pTalker; }
	CServerFile();
	virtual ~CServerFile();
protected:
   TCHAR m_strLastError[LASTERROR_BUFFER_LENGTH];
   TCHAR m_strServerName[SERVER_BUFFER_LENGTH];
   CShowStatus* m_pTalker;
};

class CServerFile_Test : public CServerFile
{
public:
   virtual BOOL GetServerHeaders(LPCTSTR lpszServer, LPCTSTR lpszReqResource, LPTSTR lpBufferForHeaders, DWORD dwBufferLen) { return FALSE; }
   virtual BOOL DownloadFile(LPCTSTR lpszServer, LPCTSTR lpszReqResource, LPCTSTR lpszResFile) { return FALSE; }
   CServerFile_Test() { }
   virtual ~CServerFile_Test() { }
};


class CServerFile_WinInet : public CServerFile
{
public:
   virtual BOOL GetServerHeaders(LPCTSTR lpszServer, LPCTSTR lpszReqResource, LPTSTR lpBufferForHeaders, DWORD dwBufferLen);
   virtual BOOL DownloadFile(LPCTSTR lpszServer, LPCTSTR lpszReqResource, LPCTSTR lpszResFile);
   CServerFile_WinInet();
   virtual ~CServerFile_WinInet();
protected:
   HINTERNET m_hInternet;
   HINTERNET m_hConnect;
   BOOL OpenHTTPConnection(LPCTSTR lpszServer);
   BOOL CloseHTTPConnection();

   // trace file instead of unit tests
  #ifdef _DEBUG
   void OpenTraceFile(LPCTSTR lpszFileName);
   void OutputToTraceFile(LPCTSTR lpszMessage);
   void CloseTraceFile();
   FILE* m_pTraceFile;
  #endif
};

#endif // !defined(AFX_SERVERFILE_H__E0DE2459_E834_4D0B_8A0C_66F4393E188A__INCLUDED_)
