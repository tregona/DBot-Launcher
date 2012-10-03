// Outlook64detectDlg.cpp: implementation of the CStatusDlg class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "StatusDlg.h"

HINSTANCE CStatusDlg::m_hInst = NULL;
HWND  CStatusDlg::m_hWndThisDlg = NULL;
HWND  CStatusDlg::m_hWndParent = NULL;
HWND  CStatusDlg::m_hWndCancel = NULL;
HWND  CStatusDlg::m_hWndProgress = NULL;
HWND  CStatusDlg::m_hWndRetry = NULL;
HFONT CStatusDlg::m_hFont = NULL;
HBRUSH CStatusDlg::m_hBrush = NULL;
BOOL CStatusDlg::m_bNeedToBreak = FALSE;
BOOL CStatusDlg::m_bNeedToRetry = FALSE;
TCHAR m_strLastMessage[DLG_LASTMESSAGE_BUFFER];

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CStatusDlg::CStatusDlg(HWND hWndParent)
{
   m_strLastMessage[0] = _T('\0');
   HINSTANCE hInst = GetModuleHandle(NULL);
   WNDCLASSEX wcex;

   m_hBrush = CreateSolidBrush(RGB(255,255,255));

   if (!GetClassInfoEx(hInst, DLG_WNDCLASS_NAME, &wcex))
   {
      wcex.cbSize        = sizeof(WNDCLASSEX); 
      wcex.style         = CS_HREDRAW | CS_VREDRAW;
      wcex.lpfnWndProc   = (WNDPROC)WndProc;
      wcex.cbClsExtra    = 0;
      wcex.cbWndExtra    = 0;
      wcex.hInstance     = hInst;
      wcex.hIcon         = LoadIcon(hInst, MAKEINTRESOURCE(IDI_MAINICON));
      wcex.hCursor       = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
      wcex.hbrBackground = m_hBrush; //(HBRUSH)(COLOR_BTNFACE);//COLOR_WINDOW);
      wcex.lpszMenuName  = NULL;
      wcex.lpszClassName = DLG_WNDCLASS_NAME;
      wcex.hIconSm       = NULL;

      if (RegisterClassEx(&wcex) == 0)
         MessageBox(NULL, _T("Can't create dialog!"), _T("Error"), MB_OK);
   }
   m_hWndThisDlg = hWndParent;
}

CStatusDlg::~CStatusDlg()
{
}

LRESULT CALLBACK CStatusDlg::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   switch (message) 
   {
      case WM_CREATE:
         WndProcHelper_Create(hWnd);
         SetFocus(m_hWndCancel);
         break;

      case WM_PAINT:
         WndProcHelper_Paint(hWnd, (HDC)wParam);
         SetStatus(m_strLastMessage, -10);
         break;

      case WM_DESTROY:
         if (m_hFont!=NULL) {
            DeleteObject(m_hFont); m_hFont = NULL; }
         if (m_hBrush!=NULL) {
            DeleteObject(m_hBrush); m_hBrush = NULL; }
         if (m_hWndParent!=NULL) {
            EnableWindow(m_hWndParent, TRUE);
            SetForegroundWindow(m_hWndParent);
         }
         DestroyWindow(hWnd);
         PostQuitMessage(0);
         break;

      case WM_ACTIVATE:
         if (wParam) {
            if ( IsWindowVisible(m_hWndRetry) )
               SetFocus(m_hWndRetry);
            else SetFocus(m_hWndCancel);
         }
         break;
      case WM_COMMAND:
         switch (HIWORD(wParam))
         {
            case BN_CLICKED:
               if ((HWND)lParam == m_hWndRetry)
                  m_bNeedToRetry = TRUE;
               else if ((HWND)lParam == m_hWndCancel)
               {
                  m_bNeedToBreak = TRUE;
                  //PostMessage(m_hWndThisDlg, WM_KEYDOWN, VK_ESCAPE, 0);
               }
               break;
         }
         break;

      default:
         return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}

void CStatusDlg::WndProcHelper_Create(HWND hWnd)
{
   // font
   LOGFONT lfont;
   memset(&lfont, 0, sizeof(lfont));
   lstrcpy(lfont.lfFaceName, DLG_FONT_NAME);
   lfont.lfHeight = DLG_FONT_SIZE;
   lfont.lfWeight = FW_NORMAL;//FW_BOLD;
   lfont.lfItalic = FALSE;
   lfont.lfCharSet = DEFAULT_CHARSET; // RUSSIAN_CHARSET
   lfont.lfOutPrecision = OUT_DEFAULT_PRECIS;
   lfont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
   lfont.lfQuality = DEFAULT_QUALITY;
   lfont.lfPitchAndFamily = DEFAULT_PITCH;
   m_hFont = CreateFontIndirect(&lfont);

   m_hInst = GetModuleHandle(NULL);

   // button Cancel
   int nCancelTop = DLG_HEIGHT - DLG_BTNCANCEL_HEIGHT - ::GetSystemMetrics(SM_CYCAPTION) - 10;
   m_hWndCancel = CreateWindowEx(WS_EX_STATICEDGE,
      _T("Button"), DLG_BTNCANCEL_TITLE,
      WS_VISIBLE | WS_CHILD | WS_TABSTOP, 
      (DLG_WIDTH-DLG_BTNCANCEL_WIDTH)/2, nCancelTop,
      DLG_BTNCANCEL_WIDTH, DLG_BTNCANCEL_HEIGHT, 
      hWnd, 
      NULL, 
      m_hInst, 
      NULL); 
   // setting font
   SendMessage(m_hWndCancel, WM_SETFONT, (WPARAM)m_hFont, 0);

   // button Retry
   m_hWndRetry = CreateWindowEx(WS_EX_STATICEDGE,
      _T("Button"), DLG_BTNRETRY_TITLE,
      ( WS_CHILD | WS_TABSTOP ) & (~WS_VISIBLE), 
      DLG_WIDTH/2 - DLG_BTNRETRY_WIDTH - 10, nCancelTop,
      DLG_BTNRETRY_WIDTH, DLG_BTNRETRY_HEIGHT, 
      hWnd, 
      NULL, 
      m_hInst, 
      NULL); 
   // setting font
   SendMessage(m_hWndRetry, WM_SETFONT, (WPARAM)m_hFont, 0);

   // progress bar
   m_hWndProgress = CreateWindowEx(0, //WS_EX_STATICEDGE,
      PROGRESS_CLASS, _T(""),
      (PBS_SMOOTH | WS_CHILD) & (~WS_VISIBLE), 
      5, nCancelTop - DLG_PROGRESS_HEIGHT - 5, DLG_WIDTH-15, DLG_PROGRESS_HEIGHT, 
      hWnd, 
      NULL, 
      m_hInst, 
      NULL); 
}

BOOL CStatusDlg::DoModal(LPCTSTR lpszCaption)
{
   RECT r;
   GetWindowRect(GetDesktopWindow(), &r);

   m_hWndThisDlg = CreateWindowEx(WS_EX_APPWINDOW, //WS_EX_TOOLWINDOW, 
      DLG_WNDCLASS_NAME,
      lpszCaption,
      (WS_POPUPWINDOW | WS_CAPTION | WS_TABSTOP | WS_MINIMIZEBOX) & (~WS_MAXIMIZEBOX) , 
      (r.right-DLG_WIDTH) / 2, (r.bottom-DLG_HEIGHT) / 2,
      DLG_WIDTH, DLG_HEIGHT,
      m_hWndParent,
      NULL,
      m_hInst,
      NULL);
   if (m_hWndThisDlg == NULL)
      return FALSE;

   SetForegroundWindow(m_hWndThisDlg);
   if (m_hWndParent!=NULL)
      EnableWindow(m_hWndParent, FALSE);
   ShowWindow(m_hWndThisDlg, SW_SHOW); 
   UpdateWindow(m_hWndThisDlg);

   BOOL ret = 0;
   MSG msg;
   while(GetMessage(&msg, NULL, 0, 0)) 
   {       
      if (msg.message == WM_KEYUP) 
      {
         if (msg.wParam == VK_ESCAPE)
         {
            m_bNeedToBreak = TRUE;
            //SendMessage(m_hWndThisDlg, WM_DESTROY, 0, 0);
            //ret = 0;
         }
         else if (msg.wParam == VK_SPACE)
         {
            HWND hWndFocused = GetFocus();
            if (hWndFocused==m_hWndCancel)
               m_bNeedToBreak = TRUE;
            else if (hWndFocused==m_hWndRetry)
               m_bNeedToRetry = TRUE;
         }
         else if (msg.wParam == VK_TAB)
         {
            HWND hWndFocused = GetFocus();
            if (hWndFocused == m_hWndRetry) SetFocus(m_hWndCancel);
            else if ( (hWndFocused == m_hWndCancel)
               && IsWindowVisible(m_hWndRetry) )
                  SetFocus(m_hWndRetry);
         }
      }
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

   return ret;
}

void CStatusDlg::WndProcHelper_Paint(HWND hWnd, HDC hDC0)
{
   if ( (hWnd==NULL) || (!IsWindow(hWnd)) )
      return;
   if (hWnd!=m_hWndThisDlg)
      return;

	HBITMAP hBmp = (HBITMAP)LoadImage(GetModuleHandle(NULL),
      MAKEINTRESOURCE(IDB_BACKGROUND), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
   BITMAP bmp;
   GetObject(hBmp, sizeof(BITMAP), &bmp);
   RECT r; GetWindowRect(m_hWndThisDlg, &r);
   int nLeft = ( (r.right - r.left) - bmp.bmWidth) / 2;

   PAINTSTRUCT ps;
	HDC hDC = BeginPaint(hWnd, &ps);
	HDC MemDC = CreateCompatibleDC(hDC);
	SelectObject(MemDC, hBmp);
	BitBlt(hDC, nLeft, 0, bmp.bmWidth, bmp.bmHeight, MemDC, 0, 0, SRCCOPY);
	DeleteDC(MemDC);
	DeleteObject(hBmp);
	EndPaint(hWnd, &ps);
}

void CStatusDlg::SetStatus(LPCTSTR lpszNewStatus, int nProgressPos)
{
   if ( (lpszNewStatus!=NULL) && (m_strLastMessage!=lpszNewStatus) )
      _tcscpy(m_strLastMessage, lpszNewStatus);
   HDC hDC = GetDC(m_hWndThisDlg);

   if (lpszNewStatus)
   {
      RECT r;
      GetWindowRect(m_hWndProgress, &r);
      POINT ptTL; ptTL.x = r.left; ptTL.y = r.top;
      ScreenToClient(m_hWndThisDlg, &ptTL);
      POINT ptBR; ptBR.x = r.right; ptBR.y = r.bottom;
      ScreenToClient(m_hWndThisDlg, &ptBR);
      SetRect(&r, ptTL.x, ptTL.y - DLG_STATIC_HEIGHT - 5, ptBR.x, ptTL.y - 5);

      // repainting the background
      COLORREF oldcr = SetBkColor(hDC, RGB(255,255,255));
      ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &r, _T(""), 0, 0);
      SetBkColor(hDC, oldcr);

      // writing new status
      HGDIOBJ hOld = SelectObject(hDC, m_hFont);
      DrawText(hDC, lpszNewStatus, -1, &r, DT_CENTER | DT_VCENTER);
      SelectObject(hDC, hOld);
   }

   if (nProgressPos<0) {
      if (nProgressPos!=-10)
         ShowWindow(m_hWndProgress, SW_HIDE);
   }
   else {
      BOOL bWasVisible = ShowWindow(m_hWndProgress, SW_SHOW);
      if (!bWasVisible)
         SendMessage(m_hWndProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 1000));
      SendMessage(m_hWndProgress, PBM_SETPOS, nProgressPos, 0L);
   }

   ReleaseDC(m_hWndThisDlg, hDC);
}

BOOL CStatusDlg::IsReady()
{
   if (m_hWndThisDlg==NULL) return FALSE;
   return IsWindow(m_hWndThisDlg);
}

void CStatusDlg::GoToExit()
{
   if ( (m_hWndThisDlg!=NULL) && IsWindow(m_hWndThisDlg) )
      PostMessage(m_hWndThisDlg, WM_CLOSE, 0, 0);
}

void CStatusDlg::PrepareForRepeatMode()
{
   int nCancelTop = DLG_HEIGHT - DLG_BTNCANCEL_HEIGHT - ::GetSystemMetrics(SM_CYCAPTION) - 10;
   MoveWindow(m_hWndCancel, DLG_WIDTH/2 + 10, nCancelTop,
      DLG_BTNCANCEL_WIDTH, DLG_BTNCANCEL_HEIGHT, TRUE);
   ShowWindow(m_hWndRetry, SW_SHOW);
   PostMessage(m_hWndThisDlg, WM_KEYUP, VK_TAB, 0L);
}

void CStatusDlg::PrepareForWorkMode()
{
   ShowWindow(m_hWndRetry, SW_HIDE);
   m_bNeedToRetry = FALSE;
   int nCancelTop = DLG_HEIGHT - DLG_BTNCANCEL_HEIGHT - ::GetSystemMetrics(SM_CYCAPTION) - 10;
   PostMessage(m_hWndThisDlg, WM_KEYUP, VK_TAB, 0L);
   MoveWindow(m_hWndCancel, (DLG_WIDTH-DLG_BTNCANCEL_WIDTH)/2, nCancelTop,
      DLG_BTNCANCEL_WIDTH, DLG_BTNCANCEL_HEIGHT, TRUE);
}
