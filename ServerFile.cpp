// ServerFile.cpp: implementation of the CServerFile class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ServerFile.h"
//#include "common/Base64Coder.h"

#define INET_USER_AGENT _T("Opera/9.80 (Windows NT 5.1; U; ru) Presto/2.10.289 Version/12.01")

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CServerFile::CServerFile()
{
   m_strLastError[0] = _T('\0');
   m_strServerName[0] = _T('\0');
   m_pTalker = NULL;
}

CServerFile::~CServerFile()
{

}

CServerFile_WinInet::CServerFile_WinInet()
{
   m_hInternet = m_hConnect = NULL;
  #ifdef _DEBUG
   m_pTraceFile = NULL;
   OpenTraceFile(_T("trace_serverfile_wininet.log"));
  #endif
}

CServerFile_WinInet::~CServerFile_WinInet()
{
   CloseHTTPConnection();
  #ifdef _DEBUG
   CloseTraceFile();
  #endif
}

BOOL CServerFile_WinInet::OpenHTTPConnection(LPCTSTR lpszServer)
{
   m_strLastError[0] = _T('\0');
   if (m_hConnect!=NULL)
   {
      if (_tcscmp(m_strServerName,lpszServer)==0) {
        #ifdef _DEBUG
         OutputToTraceFile(_T("OpenHTTPConnection("));
         OutputToTraceFile(lpszServer);
         OutputToTraceFile(_T(") : use exist\r\n"));
        #endif
         return TRUE; // actual connection
      }
      else CloseHTTPConnection();
   }

   m_strServerName[0] = _T('\0');
   m_hConnect = NULL;
   m_hInternet = InternetOpen( INET_USER_AGENT,
      INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
   if (m_hInternet==NULL) {
      DWORD dwLastError = GetLastError();
      FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwLastError, 0,
         m_strLastError, LASTERROR_BUFFER_LENGTH, NULL);
     #ifdef _DEBUG
      OutputToTraceFile(_T("OpenHTTPConnection(")); OutputToTraceFile(lpszServer);
      OutputToTraceFile(_T(") : error in InternetOpen() : "));
      OutputToTraceFile(m_strLastError); OutputToTraceFile(_T("\r\n"));
     #endif
   return FALSE;
   }

   m_hConnect = InternetConnect( m_hInternet, lpszServer,
      INTERNET_DEFAULT_HTTP_PORT, NULL,NULL, INTERNET_SERVICE_HTTP, 0, 1u);
   if (m_hConnect==NULL) {
      DWORD dwLastError = GetLastError();
      FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwLastError, 0,
         m_strLastError, LASTERROR_BUFFER_LENGTH, NULL);
     #ifdef _DEBUG
      OutputToTraceFile(_T("OpenHTTPConnection(")); OutputToTraceFile(lpszServer);
      OutputToTraceFile(_T(") : error in InternetConnect() : "));
      OutputToTraceFile(m_strLastError); OutputToTraceFile(_T("\r\n"));
     #endif
      InternetCloseHandle(m_hInternet);
      return FALSE;
   }

   _tcscpy(m_strServerName, lpszServer);
  #ifdef _DEBUG
   OutputToTraceFile(_T("OpenHTTPConnection(")); OutputToTraceFile(lpszServer);
   OutputToTraceFile(_T(") : success connection\r\n"));
  #endif
   return TRUE;
}

BOOL CServerFile_WinInet::CloseHTTPConnection()
{
   if (m_hConnect!=NULL) {
      InternetCloseHandle(m_hConnect);
      m_hConnect = NULL;
   }
   if (m_hInternet!=NULL) {
      InternetCloseHandle(m_hInternet);
      m_hInternet = NULL;
   }
   m_strLastError[0] = _T('\0');
   m_strServerName[0] = _T('\0');
  #ifdef _DEBUG
   OutputToTraceFile(_T("CloseHTTPConnection()\r\n"));
  #endif
   return TRUE;
}

BOOL CServerFile_WinInet::GetServerHeaders(LPCTSTR lpszServer, LPCTSTR lpszReqResource, LPTSTR lpBufferForHeaders, DWORD dwBufferLen)
{
   if ( !OpenHTTPConnection(lpszServer) )
      return FALSE;

   HINTERNET hRequest = HttpOpenRequest( m_hConnect, _T("HEAD"),
      lpszReqResource, NULL, NULL, 0, INTERNET_FLAG_KEEP_CONNECTION, 1);
   if (hRequest==NULL)
   {
      DWORD dwLastError = GetLastError();
      FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwLastError, 0,
         m_strLastError, LASTERROR_BUFFER_LENGTH, NULL);
     #ifdef _DEBUG
      OutputToTraceFile(_T("GetServerHeaders(")); OutputToTraceFile(lpszServer);
      OutputToTraceFile(_T(",")); OutputToTraceFile(lpszReqResource);
      OutputToTraceFile(_T(",...) : error in HttpOpenRequest( m_hConnect, _T(\"HEAD\"), ...) : "));
      OutputToTraceFile(m_strLastError); OutputToTraceFile(_T("\r\n"));
     #endif
      CloseHTTPConnection();
      return FALSE;
   }
   
   BOOL bReturn = FALSE;
   BOOL bSend = HttpSendRequest(hRequest, NULL,0, NULL,0);
   if (!bSend) {
      DWORD dwLastError = GetLastError();
      DWORD dwRes = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE,
         NULL, dwLastError, 0, m_strLastError, LASTERROR_BUFFER_LENGTH, NULL);
     #ifdef _DEBUG
      OutputToTraceFile(_T("GetServerHeaders(")); OutputToTraceFile(lpszServer);
      OutputToTraceFile(_T(",")); OutputToTraceFile(lpszReqResource);
      OutputToTraceFile(_T(",...) : error in HttpSendRequest() : "));
      OutputToTraceFile(m_strLastError); OutputToTraceFile(_T("\r\n"));
     #endif
   }
   else
   {
      DWORD dwIndex = 0, dwBLen = dwBufferLen;
      *lpBufferForHeaders = _T('\0');
      BOOL bResQI = HttpQueryInfo(hRequest, HTTP_QUERY_STATUS_CODE,
         lpBufferForHeaders, &dwBLen, &dwIndex);
      if ( !bResQI ) {
         DWORD dwLastError = GetLastError();
         FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwLastError, 0,
            m_strLastError, LASTERROR_BUFFER_LENGTH, NULL);
        #ifdef _DEBUG
         OutputToTraceFile(_T("GetServerHeaders(")); OutputToTraceFile(lpszServer);
         OutputToTraceFile(_T(",")); OutputToTraceFile(lpszReqResource);
         OutputToTraceFile(_T(",...) : error in HttpQueryInfo(hRequest, HTTP_QUERY_STATUS_CODE, ...) : "));
         OutputToTraceFile(m_strLastError); OutputToTraceFile(_T("\r\n"));
        #endif
      } else if ( (*lpBufferForHeaders) != _T('2') ) { // HTTP codes 2XX
         _tcscpy(m_strLastError, _T("Server returns an error with code "));
         _tcscat(m_strLastError, lpBufferForHeaders);
        #ifdef _DEBUG
         OutputToTraceFile(_T("GetServerHeaders(")); OutputToTraceFile(lpszServer);
         OutputToTraceFile(_T(",")); OutputToTraceFile(lpszReqResource);
         OutputToTraceFile(_T(",...) : HTTP CODE ")); OutputToTraceFile(lpBufferForHeaders);
         OutputToTraceFile(m_strLastError); OutputToTraceFile(_T("\r\n"));
        #endif
      }
      else {
         dwIndex = 0; dwBLen = dwBufferLen;
         *lpBufferForHeaders = _T('\0');
         bResQI = HttpQueryInfo(hRequest, HTTP_QUERY_RAW_HEADERS_CRLF,
            lpBufferForHeaders, &dwBLen, &dwIndex);
         if ( !bResQI ) {
            DWORD dwLastError = GetLastError();
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwLastError, 0,
               m_strLastError, LASTERROR_BUFFER_LENGTH, NULL);
           #ifdef _DEBUG
            OutputToTraceFile(_T("GetServerHeaders(")); OutputToTraceFile(lpszServer);
            OutputToTraceFile(_T(",")); OutputToTraceFile(lpszReqResource);
            OutputToTraceFile(_T(",...) : error in HttpQueryInfo(hRequest, HTTP_QUERY_RAW_HEADERS_CRLF, ...) : "));
            OutputToTraceFile(m_strLastError); OutputToTraceFile(_T("\r\n"));
           #endif
         } else
            bReturn = TRUE;
      } 
   }
   InternetCloseHandle(hRequest);

  #ifdef _DEBUG
   if (bReturn)
   {
      OutputToTraceFile(_T("GetServerHeaders(")); OutputToTraceFile(lpszServer);
      OutputToTraceFile(_T(",")); OutputToTraceFile(lpszReqResource);
      OutputToTraceFile(_T(",...) : success\r\nHeaders = {\r\n"));
      OutputToTraceFile(lpBufferForHeaders); OutputToTraceFile(_T("}\r\n"));
   }
  #endif
   return bReturn;
}

BOOL CServerFile_WinInet::DownloadFile(LPCTSTR lpszServer, LPCTSTR lpszReqResource, LPCTSTR lpszResFile)
{
   if ( !OpenHTTPConnection(lpszServer) )
      return FALSE;

   HINTERNET hRequest = HttpOpenRequest( m_hConnect, _T("GET"),
      lpszReqResource, NULL, NULL, 0, INTERNET_FLAG_KEEP_CONNECTION, 1);
   if (hRequest==NULL)
   {
      DWORD dwLastError = GetLastError();
      FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwLastError, 0,
         m_strLastError, LASTERROR_BUFFER_LENGTH, NULL);
     #ifdef _DEBUG
      OutputToTraceFile(_T("DownloadFile(")); OutputToTraceFile(lpszServer);
      OutputToTraceFile(_T(",")); OutputToTraceFile(lpszReqResource);
      OutputToTraceFile(_T(",...) : error in HttpOpenRequest(m_hConnect, _T(\"GET\"),...) : "));
      OutputToTraceFile(m_strLastError); OutputToTraceFile(_T("\r\n"));
     #endif
      CloseHTTPConnection();
      return FALSE;
   }
   
   BOOL bReturn = FALSE;
   BOOL bSend = HttpSendRequest(hRequest, NULL,0, NULL,0);
   if (!bSend) {
      DWORD dwLastError = GetLastError();
      FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwLastError, 0,
         m_strLastError, LASTERROR_BUFFER_LENGTH, NULL);
     #ifdef _DEBUG
      OutputToTraceFile(_T("DownloadFile(")); OutputToTraceFile(lpszServer);
      OutputToTraceFile(_T(",")); OutputToTraceFile(lpszReqResource);
      OutputToTraceFile(_T(",...) : error in HttpSendRequest() : "));
      OutputToTraceFile(m_strLastError); OutputToTraceFile(_T("\r\n"));
     #endif
   }
   else
   {
      double dFileSize = 0.0;
      DWORD dwIndex = 0, dwBLen = 120;
      TCHAR buffHeader[120]; buffHeader[0] = _T('\0');
      BOOL bResQI = HttpQueryInfo(hRequest, HTTP_QUERY_CONTENT_LENGTH,
         buffHeader, &dwBLen, &dwIndex);
     #ifdef _DEBUG
      OutputToTraceFile(_T("DownloadFile(")); OutputToTraceFile(lpszServer);
      OutputToTraceFile(_T(",")); OutputToTraceFile(lpszReqResource);
      if (bResQI) {
         OutputToTraceFile(_T(",...) : ok content-length="));
         OutputToTraceFile(buffHeader);
      } else OutputToTraceFile(_T(",...) : ERROR"));
      OutputToTraceFile(_T("\r\n"));
     #endif
      if (bResQI)
         dFileSize = (double)_ttol(buffHeader);
      if (dFileSize<=0.0)
         dFileSize = 5000000.0; // if server can't say file size, so ...

      FILE* pFileRes = _tfopen(lpszResFile, _T("wb"));
      if (pFileRes==NULL)
      {
         DWORD dwLastError = GetLastError();
         FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE,
            NULL, dwLastError, 0, m_strLastError, LASTERROR_BUFFER_LENGTH, NULL);
        #ifdef _DEBUG
         OutputToTraceFile(_T("DownloadFile(")); OutputToTraceFile(lpszServer);
         OutputToTraceFile(_T(",")); OutputToTraceFile(lpszReqResource);
         OutputToTraceFile(_T(",...) : error in _tfopen() : "));
         OutputToTraceFile(m_strLastError); OutputToTraceFile(_T("\r\n"));
        #endif
      }
      else
      {
         double dTotalBytesRead = 0.0;
         do {
            BYTE szData[4096];
            DWORD dwBytesRead;
            bReturn = InternetReadFile( hRequest,
               szData, sizeof(szData), &dwBytesRead);
           #ifdef _DEBUG
            if (!bReturn) {
               OutputToTraceFile(_T("DownloadFile(")); OutputToTraceFile(lpszServer);
               OutputToTraceFile(_T(",")); OutputToTraceFile(lpszReqResource);
               OutputToTraceFile(_T(",...) : error in InternetReadFile() : "));
               OutputToTraceFile(m_strLastError); OutputToTraceFile(_T("\r\n"));
            }
           #endif
            dTotalBytesRead += (double)dwBytesRead;
            if ( (bReturn == FALSE) || (dwBytesRead == 0) )
               break;
            fwrite(szData, sizeof(BYTE), dwBytesRead, pFileRes);
            if (m_pTalker!=NULL)
            {
               m_pTalker->TalkMessage(NULL,
                  (int)(dTotalBytesRead / dFileSize * 1000.0) );
               if (m_pTalker->CheckForBreak()) {
                  bReturn = FALSE;
                  break;
               }
            }
         } while(TRUE);
         fclose(pFileRes);
      }
   }
   InternetCloseHandle(hRequest);

  #ifdef _DEBUG
   if (bReturn) {
      OutputToTraceFile(_T("DownloadFile(")); OutputToTraceFile(lpszServer);
      OutputToTraceFile(_T(",")); OutputToTraceFile(lpszReqResource);
      OutputToTraceFile(_T(",...) : success\r\n"));
   }
  #endif
   return bReturn;

}


#ifdef _DEBUG

void CServerFile_WinInet::OpenTraceFile(LPCTSTR lpszFileName)
{
   if (m_pTraceFile!=NULL)
      CloseTraceFile();
   m_pTraceFile = _tfopen(lpszFileName, _T("wb"));
}

void CServerFile_WinInet::OutputToTraceFile(LPCTSTR lpszMessage)
{
   if (m_pTraceFile==NULL) return;
   fwrite(lpszMessage, sizeof(TCHAR), _tcslen(lpszMessage), m_pTraceFile);
}

void CServerFile_WinInet::CloseTraceFile()
{
   if (m_pTraceFile==NULL) return;
   fclose(m_pTraceFile);
   m_pTraceFile = NULL;
}

#endif
