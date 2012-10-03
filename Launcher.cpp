// Launcher.cpp: implementation of the CLauncher class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <shellapi.h>
#include <shlobj.h>
#include <time.h>
#include <urlmon.h>
#include <msi.h>
#include "Launcher.h"
#include "common/md5.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CLauncher::CLauncher()
{
   m_pTalker = NULL;
   m_pLogFile = NULL;
   m_pServerFile = NULL;
   m_strSingleFileName[0] = _T('\0');
}

CLauncher::~CLauncher()
{
   if (m_pLogFile!=NULL)
      fclose(m_pLogFile);
}

DWORD WINAPI CLauncher::LauncherThreadFunc(LPVOID lpData)
{
   CLauncher* pLauncher = (CLauncher*)lpData;
   if (pLauncher==NULL) return 1;

  #ifdef TEST_LAUNCHER_THREAD
   return pLauncher->tester_LauncherThreadFunc();
  #endif

   return pLauncher->LauncherProcessFunc();
}

DWORD CLauncher::LauncherProcessFunc()
{
   if (!WaitWhileTalkerBecomeReady(3000)) { // 3 seconds for creating window :)
      MessageBox(NULL, _T("LauncherProcessFunc() error: no talker class\n"),
         _T("Error"), MB_OK);
      return 2;
   }

   TCHAR buffLocalFolder[MAX_PATH] = {0};
   GetLocalFolder(buffLocalFolder);
   OpenLogFile(buffLocalFolder);

   TCHAR buffServer[URL_MAX_STRLEN] = {0};
   TCHAR buffRequest[URL_MAX_STRLEN] = {0};
   GetDownloadURLParts(buffServer, buffRequest);
   TCHAR buffLocalFile[MAX_PATH] = {0};
   _tcscpy(buffLocalFile, buffLocalFolder);
   _tcscat(buffLocalFile, _T("/"));
   _tcscat(buffLocalFile, m_strSingleFileName);

   BOOL bRepeat = FALSE, bResult = FALSE;
   UINT uiExitMessageID = 0;
   do {
      bRepeat = bResult = FALSE;
      BOOL bNeedDownload = TRUE;

      if (IsLocalFileExists(buffLocalFile))
      {
         TCHAR buffAllHeaders[HEADER_BUFFER_SIZE] = {0};
         bResult = GetServerHeaders(buffServer, buffRequest, buffAllHeaders, HEADER_BUFFER_SIZE);
         if (!bResult) {
            if (AskForRepeatLoop(IDS_STR_ACCESS_SITE_ERROR)) {
               bRepeat = TRUE; continue;
            }
            else {
               uiExitMessageID = IDS_STR_EXIT_BY_USER;
               break;
            }
         }
         if (m_pLogFile!=NULL) {
            fwrite(_T("Server headers = {\r\n"), sizeof(TCHAR), _tcslen(_T("Server headers = {\r\n")), m_pLogFile);
            fwrite(buffAllHeaders, sizeof(TCHAR), _tcslen(buffAllHeaders), m_pLogFile);
            fwrite(_T(" }\r\n"), sizeof(TCHAR), _tcslen(_T(" }\r\n")), m_pLogFile);
         }
         if (CheckForBreak()) {
            bResult = FALSE; break; }

         char bMD5Local[URL_MAX_STRLEN] = {0};
         GetLocalChecksum(buffLocalFile, bMD5Local);
         if (CheckForBreak()) {
            bResult = FALSE; break; }
         TCHAR* pLocalMD5 = NULL;
        #ifdef _UNICODE
         WCHAR buff[URL_MAX_STRLEN];
         MultiByteToWideChar(CP_ACP, 0, bMD5Local, -1, buff, URL_MAX_STRLEN);
         pLocalMD5 = buff;
        #else
         TCHAR* pLocalMD5 = bMD5Local;
        #endif
         if (m_pLogFile!=NULL) {
            fwrite(_T("local file MD5 sum = "), sizeof(TCHAR), _tcslen(_T("local file MD5 sum = ")), m_pLogFile);
            fwrite(pLocalMD5, sizeof(TCHAR), _tcslen(pLocalMD5), m_pLogFile);
            fwrite(_T("\r\n"), sizeof(TCHAR), _tcslen(_T("\r\n")), m_pLogFile);
         }
         if ( (_tcslen(pLocalMD5)>0) && (_tcsstr(buffAllHeaders, pLocalMD5)!=NULL) )
         {
            TalkMessage(IDS_STR_LOCAL_FILE_IS_VALID, -1);
            bNeedDownload = FALSE;
         }
         else
            TalkMessage(IDS_STR_LOCAL_FILE_IS_NOT_VALID, -1);
      }

      if (bNeedDownload)
      {
         bResult = DownloadTool(buffServer, buffRequest, buffLocalFile);
         if (CheckForBreak()) {
            bResult = FALSE; break; }

         if (!bResult) {
            if (AskForRepeatLoop(IDS_STR_DOWNLOAD_ERROR)) {
               bRepeat = TRUE; continue;
            }
            else {
               uiExitMessageID = IDS_STR_EXIT_BY_USER;
               break;
            }
         }
      }

      if (CheckForBreak()) {
         bResult = FALSE; break; }
      bResult = RunTool(buffLocalFile);
      if (!bResult) {
         if (AskForRepeatLoop(IDS_STR_START_FILE_ERROR)) {
            bRepeat = TRUE; continue;
         }
         else {
            uiExitMessageID = IDS_STR_EXIT_BY_USER;
            break;
         }
      }
      uiExitMessageID = IDS_STR_EXIT;
   } while(bRepeat);

   TalkMessage(uiExitMessageID, -1);
   Cleanup(NULL, bResult);
   if (m_pTalker!=NULL)
      m_pTalker->GoToExit();

   return RETCODE_SUCCESS;
}

void CLauncher::TalkMessage(LPCTSTR lpszMessage, int nProgressPos)
{
   if (m_pTalker==NULL) return;
   m_pTalker->TalkMessage(lpszMessage, nProgressPos);

   if ( (lpszMessage!=NULL) && (m_pLogFile!=NULL) ) {
      fwrite(lpszMessage, sizeof(TCHAR), _tcslen(lpszMessage), m_pLogFile);
      fwrite(_T("\r\n"), sizeof(TCHAR), _tcslen(_T("\r\n")), m_pLogFile);
   }
}

void CLauncher::TalkMessage(UINT uiMessageResourceID, int nProgressPos)
{
   if (uiMessageResourceID==0) {
      TalkMessage((LPCTSTR)NULL, nProgressPos);
      return;
   }
   TCHAR buffStr[URL_MAX_STRLEN];
   if ( !LoadString(NULL, uiMessageResourceID, buffStr, URL_MAX_STRLEN) )
      return;
   TalkMessage(buffStr, nProgressPos);
}

BOOL CLauncher::WaitWhileTalkerBecomeReady(int nMSecs)
{
   int nSleepPeriod = 50; // milliseconds
   int nNumTries = nMSecs / nSleepPeriod;
   for(int i=0; i<nNumTries; i++)
   {
      if ( (m_pTalker!=NULL) && (m_pTalker->IsReady()) )
         return TRUE;
      Sleep(nSleepPeriod);
   }
   return FALSE;
}

BOOL CLauncher::CheckForBreak()
{
   if (m_pTalker==NULL) return FALSE;
   BOOL bRet = m_pTalker->CheckForBreak();
   if ( bRet && (m_pLogFile!=NULL) )
      fwrite(_T("Interrupt by user\r\n"), sizeof(TCHAR),
         _tcslen(_T("Interrupt by user\r\n")), m_pLogFile);
   return bRet;
}

BOOL CLauncher::AskForRepeatLoop(LPCTSTR lpszError)
{
   if (m_pTalker==NULL) return FALSE;

   TCHAR buffer[200] = {0};
   _tcscpy(buffer, lpszError);
   _tcscat(buffer, _T(". "));
   TCHAR buffStr[URL_MAX_STRLEN];
   if ( LoadString(NULL, IDS_STR_RETRY_QUESTION, buffStr, URL_MAX_STRLEN) )
      _tcscat(buffer, buffStr);
   TalkMessage(buffer, -1);

   BOOL bRet = m_pTalker->AskForRepeat();
   if (m_pLogFile!=NULL) {
      if (bRet)
         fwrite(_T("User's answer: Retry\r\n"), sizeof(TCHAR),
            _tcslen(_T("User's answer: Retry\r\n")), m_pLogFile);
      else
         fwrite(_T("User's answer: Cancel\r\n"), sizeof(TCHAR),
            _tcslen(_T("User's answer: Cancel\r\n")), m_pLogFile);
   }

   return bRet;
}

BOOL CLauncher::AskForRepeatLoop(UINT uiMessageResourceID)
{
   if (uiMessageResourceID==0) return FALSE;
   TCHAR buffStr[URL_MAX_STRLEN];
   if ( !LoadString(NULL, uiMessageResourceID, buffStr, URL_MAX_STRLEN) )
      return FALSE;
   return AskForRepeatLoop(buffStr);
}

void CLauncher::Cleanup(LPCTSTR lpszSwanSong, BOOL bSuccess)
{
   TalkMessage(lpszSwanSong, -1);
   // Additional cleanup ...
}

BOOL CLauncher::IsOutlook64bitInstalled()
{
   TCHAR pszaOutlookQualifiedComponents[][MAX_PATH] = {
      TEXT("{1E77DE88-BCAB-4C37-B9E5-073AF52DFD7A}"), // Outlook 2010
      TEXT("{24AAE126-0911-478F-A019-07B875EB9996}"), // Outlook 2007
      TEXT("{BC174BAD-2F53-4855-A1D5-0D575C19B1EA}")  // Outlook 2003
   };
   LPCTSTR lpszQualifier = TEXT("outlook.x64.exe");

   int nOutlookQualifiedComponents
      = sizeof(pszaOutlookQualifiedComponents) / (MAX_PATH*sizeof(TCHAR));
   int i = 0;
   DWORD dwValueBuf = 0;
   UINT ret = ERROR_NOT_FOUND;
   for (i = 0; i < nOutlookQualifiedComponents; i++)
   {
      ret = MsiProvideQualifiedComponent(
         pszaOutlookQualifiedComponents[i],
         lpszQualifier,
         0,//(DWORD)INSTALLMODE_DEFAULT,
         NULL,
         &dwValueBuf);
      if (ERROR_SUCCESS == ret) return TRUE;
   }

   return FALSE;
}

BOOL CLauncher::GetDownloadURLParts(LPTSTR lpServer, LPTSTR lpRequest)
{
   BOOL bHaveOutlook64 = FALSE;
   TalkMessage(IDS_STR_LOOKING_FOR_OUTLOOK64, -1);
  #ifdef _DEBUG
   if (_tcscmp(m_buffTestInstructions, _T("Outlook64 = TRUE"))==0)
      bHaveOutlook64 = TRUE;
   else if (_tcscmp(m_buffTestInstructions, _T("Outlook64 = FALSE"))==0)
      bHaveOutlook64 = FALSE;
   else
  #endif
      bHaveOutlook64 = IsOutlook64bitInstalled();

   _tcscpy(lpServer, URL_SITE);
   if (bHaveOutlook64) {
      TalkMessage(IDS_STR_OUTLOOK64_FOUND, -1);
      _tcscpy(m_strSingleFileName, URL_SUFFIX_64_BIT);
   }
   else {
      TalkMessage(IDS_STR_OUTLOOK64_NOT_FOUND, -1);
      _tcscpy(m_strSingleFileName, URL_SUFFIX_32_BIT);
   }
   *lpRequest = _T('\0');
   _tcscat(lpRequest, URL_ALLSUBFOLDERS);
   _tcscat(lpRequest, m_strSingleFileName);
   return TRUE;
}

BOOL CLauncher::GetLocalFolder(LPTSTR lpBuffer, BOOL bCreateFoldersIfNotExist)
{
//   GetCurrentDirectory(MAX_PATH, lpBuffer);
   HRESULT hRes = SHGetFolderPath( NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE,
      NULL, SHGFP_TYPE_CURRENT, lpBuffer);
   if (hRes!=S_OK)
      return FALSE;
   else
   {
      _tcscat(lpBuffer, _T("/"));
      _tcscat(lpBuffer, FIRMFOLDER);
      if (!IsLocalFileExists(lpBuffer))
         return CreateDirectory(lpBuffer, NULL);
      return TRUE;
   }
}

BOOL CLauncher::IsLocalFileExists(LPCTSTR lpszFile)
{
   TalkMessage(IDS_STR_LOOKING_FOR_LOCAL_FILE, -1);
   BOOL bIsToolExist = FALSE;

   WIN32_FIND_DATA wfd;
   HANDLE hFind = FindFirstFile(lpszFile, &wfd);
   if (hFind==INVALID_HANDLE_VALUE) {
      TalkMessage(IDS_STR_LOCAL_FILE_NOT_FOUND, -1);
      bIsToolExist = FALSE;
   }
   else
   {
      FindClose(hFind);
      TalkMessage(IDS_STR_LOCAL_FILE_FOUND, -1);
      bIsToolExist = TRUE;
   }

   return bIsToolExist;
}

BOOL CLauncher::GetServerHeaders(LPCTSTR lpszServer, LPCTSTR lpszReqResource, LPTSTR lpBufferForHeaders, DWORD dwBuffHeaderSize)
{
   if (m_pServerFile==NULL) return FALSE;
   TalkMessage(IDS_STR_ACCESSING_TO_SITE, -1);
   BOOL bRes = m_pServerFile->GetServerHeaders(lpszServer,
      lpszReqResource, lpBufferForHeaders, dwBuffHeaderSize);
   return bRes;
}

BOOL CLauncher::GetLocalChecksum(LPTSTR lpFile, char* lpBufferMD5)
{
   TalkMessage(IDS_STR_EXAMINING_LOCAL_FILE, -1);
   MD5 md5; // this library works with (char*)
   char* pRes = md5.digestFile(lpFile);
   if ( (pRes==NULL) || (strlen(pRes)<=0) ) {
      return FALSE;
   }
   strcpy(lpBufferMD5, pRes);
   return TRUE;
}

BOOL CLauncher::DownloadTool(LPCTSTR lpszServer, LPCTSTR lpszReqResource, LPCTSTR lpszResFile)
{
   //HRESULT hr = URLDownloadToFile( NULL, linkBuffer, lpszResFile, 0, NULL );
   //return (FAILED(hr))? FALSE : TRUE;
   if (m_pServerFile==NULL) return FALSE;
   TalkMessage(IDS_STR_DOWNLOAD_FILE, -1);
   BOOL bRes = m_pServerFile->DownloadFile(lpszServer,
      lpszReqResource, lpszResFile);
   TalkMessage(IDS_STR_DOWNLOAD_SUCCESS, -1);
   return bRes;
}

BOOL CLauncher::RunTool(LPCTSTR pathBuffer)
{
   TalkMessage(IDS_STR_START_FILE, -1);
   HINSTANCE hResult = ShellExecute( NULL, NULL,
      pathBuffer, NULL, NULL, SW_SHOWNORMAL);
   DWORD dwResult = (DWORD)hResult;

   if ( dwResult > 32) {
      TalkMessage(IDS_STR_START_FILE_SUCCESS, -1);
      return TRUE;
   } else {
      return FALSE;
   }
}

BOOL CLauncher::OpenLogFile(LPCTSTR lpszFolder)
{
   if (lpszFolder==NULL) return FALSE;
   if (m_pLogFile!=NULL)
      fclose(m_pLogFile);

   TCHAR bufferFileName[MAX_PATH];
   _tcscpy(bufferFileName, lpszFolder); _tcscat(bufferFileName, _T("/"));

   time_t ltime; time(&ltime);
   struct tm *today = localtime(&ltime);
   TCHAR buffTimeStamp[50];
   _tcsftime(buffTimeStamp, 50, _T("%Y%m%dT%H%M%S_Diag.log"), today);
   _tcscat(bufferFileName, buffTimeStamp);

  #ifdef _DEBUG
   if (_tcscmp(m_buffTestInstructions, _T("Check file name"))==0)
      _tcscpy(m_buffTestInstructions, bufferFileName);
   else
  #endif
      m_pLogFile = _tfopen(bufferFileName, _T("wb"));
   return (m_pLogFile==NULL)? FALSE : TRUE;
}


//////////////////////////////////////////////////////////////////////
// Tests
//////////////////////////////////////////////////////////////////////

#ifdef TEST_LAUNCHER_THREAD
DWORD CLauncher::tester_LauncherThreadFunc()
{
   if (!WaitWhileTalkerBecomeReady(3000)) { // 3 seconds for creating window :)
      MessageBox(NULL, _T("LauncherProcessFunc() error: no talker class\n"),
         _T("Error"), MB_OK);
      return 2;
   }
   TCHAR buff[100] = {0};

   int nRetCode = RETCODE_OTHER_ERRORS;
   BOOL bRepeat = FALSE, bResult = FALSE;
   BOOL bFirst = TRUE; // test: first start will fail, but second will success
   do {
      bRepeat = bResult = FALSE; BOOL bBreak = FALSE;
      for(int i=0; i<100; i++)
      {
         if (CheckForBreak()) {
            Cleanup(_T("Interrupted by user"), FALSE);
            m_pTalker->GoToExit();
            return RETCODE_INTERRUPT_BY_USER;
         }
         Sleep(100);
         _stprintf(buff, _T("Testing threads cooperation: %ld / %ld"), i, 100);
         TalkMessage(buff, i*10);

         if ( bFirst && (i==20) )// model of error :)
         {
            bFirst = FALSE; bResult = FALSE;
            if ( AskForRepeatLoop(_T("Error while <doing something>")) )
               bRepeat = TRUE;
            else {
               nRetCode = RETCODE_INTERRUPT_BY_USER;
               bBreak = TRUE;
            }
            break;
         }
      }
      if (bBreak) break;
      bResult = TRUE;
   } while(bRepeat);

   Cleanup(_T("Exiting ..."), bResult);
   if (m_pTalker!=NULL)
      m_pTalker->GoToExit();
   return (bResult)? RETCODE_SUCCESS : nRetCode;
}
#endif

#ifdef _DEBUG

CLauncherTest::CLauncherTest()
{
   m_pLauncher = NULL;
   m_pFile = NULL;
}

CLauncherTest::~CLauncherTest()
{
   if (m_pLauncher!=NULL) delete m_pLauncher;
}

BOOL CLauncherTest::OpenResultLog(LPCTSTR lpszStartMessage)
{
   if (m_pFile!=NULL) {
      fclose(m_pFile); delete m_pFile; }

   TCHAR buffFileName[MAX_PATH] = {0};
   GetCurrentDirectory(MAX_PATH, buffFileName);
   _tcscat(buffFileName, _T("/"));
   _tcscat(buffFileName, _T("tests.log"));
   m_pFile = _tfopen(buffFileName, _T("wb"));
   if (m_pFile==NULL) {
      MessageBox(NULL, _T("Error while creating log file with test results"), _T("Error"), MB_OK);
      return FALSE;
   }

   TCHAR buffMessage[tester_MESSAGE_BUFFER_SIZE] = {0};

   // current date
   _tstrdate(buffMessage); _tcscat(buffMessage, _T(" "));
   OutputMessage(buffMessage);
   // current time
   _tstrtime(buffMessage); _tcscat(buffMessage, _T(" "));
   OutputMessage(buffMessage);
   // title
   _tcscpy(buffMessage, lpszStartMessage); _tcscat(buffMessage, _T("\r\n"));
   OutputMessage(buffMessage);

   return TRUE;
}

BOOL CLauncherTest::CloseResultLog(LPCTSTR lpszLastMessage)
{
   if (m_pFile==NULL) return FALSE;
   if (lpszLastMessage!=NULL)
   {
      TCHAR buffMessage[tester_MESSAGE_BUFFER_SIZE] = {0};
      _tcscpy(buffMessage, lpszLastMessage); _tcscat(buffMessage, _T("\r\n"));
      OutputMessage(buffMessage);
   }
   fclose(m_pFile); m_pFile = NULL;
   return TRUE;
}

BOOL CLauncherTest::OutputMessage(LPCTSTR lpszMessage)
{
   if (m_pFile==NULL) return FALSE;

  #ifdef OUTPUT_IN_UNICODE
     #ifdef _UNICODE
      fwrite(lpszMessage, sizeof(TCHAR), _tcslen(lpszMessage), m_pFile); 
     #else
      WCHAR buff[tester_MESSAGE_BUFFER_SIZE_convert];
      MultiByteToWideChar(CP_ACP, 0, lpszMessage, -1, buff, tester_MESSAGE_BUFFER_SIZE_convert);
      fwrite(buff, sizeof(WCHAR), strlen(buff), m_pFile); 
     #endif
  #else
     #ifdef _UNICODE
      char buff[tester_MESSAGE_BUFFER_SIZE_convert];
      WideCharToMultiByte(CP_ACP, 0, lpszMessage, -1, buff, tester_MESSAGE_BUFFER_SIZE_convert, NULL, NULL);
      fwrite(buff, sizeof(char), strlen(buff), m_pFile); 
     #else
      fwrite(lpszMessage, sizeof(char), strlen(lpszMessage), m_pFile); 
     #endif
  #endif

   return TRUE;
}

void CLauncherTest::tester_RunAllTests()
{
   if ( !OpenResultLog(_T("Starting tests")) )
      return;

   m_pLauncher = new CLauncher;
   TCHAR buffMessage[tester_MESSAGE_BUFFER_SIZE] = {0};

   _tcscpy(buffMessage, _T("Checking Outlook64 on this machine ... "));
   BOOL bHaveOL64 = m_pLauncher->IsOutlook64bitInstalled();
   _tcscat(buffMessage, (bHaveOL64)? _T(" FOUND\r\n") : _T(" NOT FOUND\r\n"));
   OutputMessage(buffMessage);

   tester_GetDownloadURLParts();
   tester_IsLocalFileExists();
   tester_GetLocalChecksum();
   tester_OpenLogFile();

   if (m_pLauncher!=NULL) {
      delete m_pLauncher; m_pLauncher = NULL;
   }

   CloseResultLog(_T("All tests are done."));
}

void CLauncherTest::tester_GetDownloadURLParts()
{
   BOOL bSingleTest = (m_pLauncher==NULL)? TRUE : FALSE;
   TCHAR buffMessage[tester_MESSAGE_BUFFER_SIZE] = {0};
   if (bSingleTest)
   {
      CloseResultLog();
      if ( !OpenResultLog(_T("Starting test of GetDownloadURLParts")) )
         return;
      m_pLauncher = new CLauncher;
   }
   else
   {
      _tcscpy(buffMessage, _T("test: GetDownloadURLParts\r\n"));
      OutputMessage(buffMessage);
   }

   TCHAR buffer[URL_MAX_STRLEN] = {0};
   TCHAR bufferR[URL_MAX_STRLEN] = {0};

   _tcscpy(m_pLauncher->m_buffTestInstructions, _T("Outlook64 = TRUE"));
   _tcscpy(buffMessage, m_pLauncher->m_buffTestInstructions);
   m_pLauncher->GetDownloadURLParts(buffer, bufferR);
   _tcscat(buffMessage, _T(", result = http://"));
   _tcscat(buffMessage, buffer); _tcscat(buffMessage, _T("/"));
   _tcscat(buffMessage, bufferR); _tcscat(buffMessage, _T("\r\n"));
   OutputMessage(buffMessage);

   _tcscpy(m_pLauncher->m_buffTestInstructions, _T("Outlook64 = FALSE"));
   _tcscpy(buffMessage, m_pLauncher->m_buffTestInstructions);
   m_pLauncher->GetDownloadURLParts(buffer, bufferR);
   _tcscat(buffMessage, _T(", result = http://"));
   _tcscat(buffMessage, buffer); _tcscat(buffMessage, _T("/"));
   _tcscat(buffMessage, bufferR); _tcscat(buffMessage, _T("\r\n"));
   OutputMessage(buffMessage);

   if (bSingleTest)
   {
      CloseResultLog(_T("Testing GetDownloadURLParts has finished."));
      delete m_pLauncher; m_pLauncher = NULL;
   }
}

void CLauncherTest::tester_IsLocalFileExists()
{
   BOOL bSingleTest = (m_pLauncher==NULL)? TRUE : FALSE;
   TCHAR buffMessage[tester_MESSAGE_BUFFER_SIZE] = {0};
   if (bSingleTest)
   {
      CloseResultLog();
      if ( !OpenResultLog(_T("Starting test of IsLocalFileExists\r\n")) )
         return;
      m_pLauncher = new CLauncher;
   }
   else
   {
      _tcscpy(buffMessage, _T("test: IsLocalFileExists\r\n"));
      OutputMessage(buffMessage);
   }

   TCHAR buffer[URL_MAX_STRLEN] = {0};

   _tcscpy(buffMessage, _T("lpszFile = <current directory>, result = "));
   GetCurrentDirectory(URL_MAX_STRLEN, buffer);
   BOOL bRes = m_pLauncher->IsLocalFileExists(buffer);
   if (bRes) _tcscat(buffMessage, _T("true (ok)\r\n"));
   else _tcscat(buffMessage, _T("false (ERROR!!!)\r\n"));
   OutputMessage(buffMessage);

   _tcscpy(buffMessage, _T("lpszFile = <bad directory>, result = "));
   _tcscat(buffer, _T("\x07\x07"));
   bRes = m_pLauncher->IsLocalFileExists(buffer);
   if (bRes) _tcscat(buffMessage, _T("true (ERROR!!!)\r\n"));
   else _tcscat(buffMessage, _T("false (ok)\r\n"));
   OutputMessage(buffMessage);

   if (bSingleTest)
   {
      CloseResultLog(_T("Testing IsLocalFileExists has finished."));
      delete m_pLauncher; m_pLauncher = NULL;
   }
}

void CLauncherTest::tester_GetLocalFolder()
{
   BOOL bSingleTest = (m_pLauncher==NULL)? TRUE : FALSE;
   TCHAR buffMessage[tester_MESSAGE_BUFFER_SIZE] = {0};
   if (bSingleTest)
   {
      CloseResultLog();
      if ( !OpenResultLog(_T("Starting test of GetLocalFolder\r\n")) )
         return;
      m_pLauncher = new CLauncher;
   }
   else
   {
      _tcscpy(buffMessage, _T("test: IsLocalFileExists\r\n"));
      OutputMessage(buffMessage);
   }

   TCHAR buffer[URL_MAX_STRLEN] = {0};

   _tcscpy(buffMessage, _T("lpszFile = <current directory>, result = "));
   GetCurrentDirectory(URL_MAX_STRLEN, buffer);
   BOOL bRes = m_pLauncher->IsLocalFileExists(buffer);
   if (bRes) _tcscat(buffMessage, _T("true (ok)\r\n"));
   else _tcscat(buffMessage, _T("false (ERROR!!!)\r\n"));
   OutputMessage(buffMessage);

   _tcscpy(buffMessage, _T("lpszFile = <bad directory>, result = "));
   _tcscat(buffer, _T("\x07\x07"));
   bRes = m_pLauncher->IsLocalFileExists(buffer);
   if (bRes) _tcscat(buffMessage, _T("true (ERROR!!!)\r\n"));
   else _tcscat(buffMessage, _T("false (ok)\r\n"));
   OutputMessage(buffMessage);

   if (bSingleTest)
   {
      CloseResultLog(_T("Testing IsLocalFileExists has finished."));
      delete m_pLauncher; m_pLauncher = NULL;
   }
}

void CLauncherTest::tester_OpenLogFile()
{
   BOOL bSingleTest = (m_pLauncher==NULL)? TRUE : FALSE;
   TCHAR buffMessage[tester_MESSAGE_BUFFER_SIZE] = {0};
   if (bSingleTest)
   {
      CloseResultLog();
      if ( !OpenResultLog(_T("Starting test of OpenLogFile (checking filename)\r\n")) )
         return;
      m_pLauncher = new CLauncher;
   }
   else
   {
      _tcscpy(buffMessage, _T("test: OpenLogFile (checking filename)\r\n"));
      OutputMessage(buffMessage);
   }

   TCHAR buffer[MAX_PATH] = {0};
   GetCurrentDirectory(URL_MAX_STRLEN, buffer);
   _tcscpy(buffMessage, _T("<current directory> = "));
   _tcscat(buffMessage, buffer); _tcscat(buffMessage, _T("\r\n"));
   OutputMessage(buffMessage);

   _tcscpy(m_pLauncher->m_buffTestInstructions, _T("Check file name"));
   m_pLauncher->OpenLogFile(buffer);
   _tcscpy(buffMessage, _T("result log filename = "));
   _tcscat(buffMessage, m_pLauncher->m_buffTestInstructions);
   _tcscat(buffMessage, _T("\r\n"));
   OutputMessage(buffMessage);

   if (bSingleTest)
   {
      CloseResultLog(_T("Testing OpenLogFile has finished."));
      delete m_pLauncher; m_pLauncher = NULL;
   }
}

void CLauncherTest::tester_GetLocalChecksum()
{
   BOOL bSingleTest = (m_pLauncher==NULL)? TRUE : FALSE;
   TCHAR buffMessage[tester_MESSAGE_BUFFER_SIZE] = {0};
   if (bSingleTest)
   {
      CloseResultLog();
      if ( !OpenResultLog(_T("Starting test of GetLocalChecksum\r\n")) )
         return;
      m_pLauncher = new CLauncher;
   }
   else
   {
      _tcscpy(buffMessage, _T("test: GetLocalChecksum\r\n"));
      OutputMessage(buffMessage);
   }

// MD5-sum for '1234567890' is 'e807f1fcf82d132f9bb018ca6738a19f'
// thanks http://md5-hash-online.waraxe.us/
   TCHAR buffer[MAX_PATH] = {0};
   GetCurrentDirectory(URL_MAX_STRLEN, buffer);
   _tcscat(buffer, _T("/testmd5"));
   FILE* pFile = _tfopen(buffer, _T("wb"));
   if (pFile==NULL)
   {
      _tcscpy(buffMessage, _T("ERROR CREATING TEST FILE\r\n"));
      OutputMessage(buffMessage);
   }
   else
   {
      fwrite("1234567890", 1, 10, pFile);
      fclose(pFile);
      char bMD5Local[100];
      m_pLauncher->GetLocalChecksum(buffer, bMD5Local);
         TCHAR* pLocalMD5 = NULL;
        #ifdef _UNICODE
         WCHAR buff[URL_MAX_STRLEN];
         MultiByteToWideChar(CP_ACP, 0, bMD5Local, -1, buff, URL_MAX_STRLEN);
         pLocalMD5 = buff;
        #else
         TCHAR* pLocalMD5 = bMD5Local;
        #endif
      _tcscpy(buffMessage, _T("GetLocalChecksum(1234567890) = "));
      _tcscat(buffMessage, pLocalMD5);
      if ( _tcscmp(pLocalMD5, _T("e807f1fcf82d132f9bb018ca6738a19f"))==0 )
         _tcscat(buffMessage, _T(" (ok)\r\n"));
      else _tcscat(buffMessage, _T(" (ERROR!!!)\r\n"));
      OutputMessage(buffMessage);
      ::DeleteFile(buffer);
   }

   if (bSingleTest)
   {
      CloseResultLog(_T("Testing OpenLogFile has finished."));
      delete m_pLauncher; m_pLauncher = NULL;
   }
}

#endif