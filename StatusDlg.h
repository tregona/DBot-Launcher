// Outlook64detectDlg.h: interface for the CStatusDlg class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_OUTLOOK64DETECTDLG_H__FDE671B0_E751_42F6_AF01_D6B390146B94__INCLUDED_)
#define AFX_OUTLOOK64DETECTDLG_H__FDE671B0_E751_42F6_AF01_D6B390146B94__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define DLG_WNDCLASS_NAME    _T("Cyr27Win32Dlg")
#define DLG_FONT_NAME        _T("Arial")
#define DLG_FONT_SIZE        16
#define DLG_WIDTH            420
#define DLG_HEIGHT           240
#define DLG_BTNCANCEL_TITLE  _T("Cancel")
#define DLG_BTNCANCEL_WIDTH  80
#define DLG_BTNCANCEL_HEIGHT 30
#define DLG_BTNRETRY_TITLE   _T("Retry")
#define DLG_BTNRETRY_WIDTH   80
#define DLG_BTNRETRY_HEIGHT  30
#define DLG_STATIC_HEIGHT    20
#define DLG_PROGRESS_HEIGHT  15
#define DLG_LASTMESSAGE_BUFFER 250 

class CStatusDlg  
{
public:
	BOOL IsReady();
   BOOL CheckForBreak() { return m_bNeedToBreak; }
   BOOL CheckForRetry() { return m_bNeedToRetry; }
	static void SetStatus(LPCTSTR lpszNewStatus, int nProgressPos = -1);
   BOOL DoModal(LPCTSTR lpszCaption);
   void GoToExit();
   void PrepareForWorkMode();
   void PrepareForRepeatMode();
   CStatusDlg(HWND hWndParent);
   virtual ~CStatusDlg();
protected:
   static HWND  m_hWndThisDlg;
   static HWND  m_hWndParent;
   static HWND  m_hWndCancel;
   static HWND  m_hWndProgress;
   static HWND  m_hWndRetry;
   static HFONT m_hFont;
   static HBRUSH m_hBrush;
   static HINSTANCE m_hInst;
   static BOOL m_bNeedToBreak;
   static BOOL m_bNeedToRetry;
   
   static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
   static void WndProcHelper_Create(HWND hWnd);
   static void WndProcHelper_Paint(HWND hWnd, HDC hDC);
};

#endif // !defined(AFX_OUTLOOK64DETECTDLG_H__FDE671B0_E751_42F6_AF01_D6B390146B94__INCLUDED_)
