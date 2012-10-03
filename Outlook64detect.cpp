// Outlook64detect.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include <commctrl.h>

#include "StatusDlg.h"
#include "Launcher.h"

#ifdef _UNICODE
 #pragma message("Build type: UNICODE")
#endif

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
  #ifdef _DEBUG
   CLauncherTest launcher_tester;
   launcher_tester.tester_RunAllTests();
  #endif

   InitCommonControls();

   CStatusDlg oDlg(NULL);
   CShowStatus_StatusDlg oStatus;
   oStatus.SetDlg(&oDlg);
   CServerFile_WinInet oSFile;
   oSFile.SetTalker(&oStatus);

   CLauncher launcher;
   launcher.SetServerFile(&oSFile);
   launcher.SetTalker(&oStatus);
   DWORD dwThreadId = 0;
   HANDLE hLauncherInst = CreateThread( NULL, 0,
      CLauncher::LauncherThreadFunc, &launcher, 0, &dwThreadId);

   TCHAR buffTitle[100];
   LoadString(NULL, IDS_STR_WINDOW_TITLE, buffTitle, 100);
   oDlg.DoModal(buffTitle);

	return 0;
}

