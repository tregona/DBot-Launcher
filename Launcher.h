// Launcher.h: interface for the CLauncher class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_LAUNCHER_H__174B4442_05F1_4DD9_92C9_2E5837496972__INCLUDED_)
#define AFX_LAUNCHER_H__174B4442_05F1_4DD9_92C9_2E5837496972__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ShowStatus.h"
#include "ServerFile.h"

#define RETCODE_SUCCESS 0
#define RETCODE_INTERRUPT_BY_USER 10
#define RETCODE_OTHER_ERRORS 1

#define URL_MAX_STRLEN    4096
#define HEADER_BUFFER_SIZE  URL_MAX_STRLEN
#define URL_SUFFIX_32_BIT _T("DiscoveryBOT.exe")
#define URL_SUFFIX_64_BIT _T("DiscoveryBOT_64.exe")
#define URL_SITE          _T("download.totaldiscovery.com")
#define URL_ALLSUBFOLDERS _T("")
#define FIRMFOLDER        _T("BIA")

class CLauncherTest;

class CLauncher  
{
   friend CLauncherTest;
public:
   static DWORD WINAPI LauncherThreadFunc(LPVOID lpData);
   void SetTalker(CShowStatus* pTalker) { m_pTalker = pTalker; }
   void SetServerFile(CServerFile* pSFile) { m_pServerFile = pSFile; }
	CLauncher();
	virtual ~CLauncher();

   // unit tests support
  #ifdef TEST_LAUNCHER_THREAD
   DWORD tester_LauncherThreadFunc();
  #endif
  #ifdef _DEBUG
   TCHAR m_buffTestInstructions[100];
  #endif

protected:
   DWORD LauncherProcessFunc();
   BOOL IsOutlook64bitInstalled(); // no tests - wrapper for MsiProvideQualifiedComponent
	BOOL GetDownloadURLParts(LPTSTR lpServer, LPTSTR lpRequest);
   BOOL GetLocalFolder(LPTSTR lpBuffer, BOOL bCreateFoldersIfNotExist = TRUE);
   BOOL IsLocalFileExists(LPCTSTR lpszFile);
   BOOL GetLocalChecksum(LPTSTR lpFile, char* lpBufferMD5); // no tests - wrapper for md5.digestFile
   BOOL GetServerHeaders(LPCTSTR lpszServer, LPCTSTR lpszRequest, LPTSTR lpBufferMD5, DWORD dwBuffHeaderSize); // see ServerFile.*
   BOOL DownloadTool(LPCTSTR lpszServer, LPCTSTR lpszReqResource, LPCTSTR lpszResFile); // see ServerFile.*
   BOOL RunTool(LPCTSTR lpszFile); // no tests - wrapper for ShellExecute
   BOOL OpenLogFile(LPCTSTR lpszFolder);

   BOOL WaitWhileTalkerBecomeReady(int nMSecs); // no tests - wrapper for UI
	BOOL CheckForBreak(); // no tests - wrapper for UI
   BOOL AskForRepeatLoop(LPCTSTR lpszError); // no tests - wrapper for UI
   BOOL AskForRepeatLoop(UINT uiMessageResourceID); // no tests - wrapper for UI
   void TalkMessage(LPCTSTR lpszMessage, int nProgressPos); // no tests - wrapper for UI
   void TalkMessage(UINT uiMessageResourceID, int nProgressPos); // no tests - wrapper for UI
   void Cleanup(LPCTSTR lpszSwanSong, BOOL bSuccess); // no tests - cleaning objects

   CShowStatus* m_pTalker;
   CServerFile* m_pServerFile;
   FILE* m_pLogFile;
   TCHAR m_strSingleFileName[MAX_PATH];
};

#ifdef _DEBUG

//#define OUTPUT_IN_UNICODE
#define tester_MESSAGE_BUFFER_SIZE 100
#define tester_MESSAGE_BUFFER_SIZE_convert tester_MESSAGE_BUFFER_SIZE*4
class CLauncherTest
{
public:
   void tester_RunAllTests();
   void tester_GetDownloadURLParts();
   void tester_IsLocalFileExists();
   void tester_GetLocalFolder();
   void tester_GetLocalChecksum();
   void tester_OpenLogFile();
   CLauncherTest();
   virtual ~CLauncherTest();
protected:
   BOOL OpenResultLog(LPCTSTR lpszStartMessage);
   BOOL CloseResultLog(LPCTSTR lpszLastMessage = NULL);
   BOOL OutputMessage(LPCTSTR lpszMessage);
   CLauncher* m_pLauncher;
   FILE* m_pFile;
};
#endif

#endif // !defined(AFX_LAUNCHER_H__174B4442_05F1_4DD9_92C9_2E5837496972__INCLUDED_)
