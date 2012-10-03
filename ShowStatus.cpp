// ShowStatus.cpp: implementation of the CShowStatus class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ShowStatus.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CShowStatus::CShowStatus()
{

}

CShowStatus::~CShowStatus()
{

}


//////////////////////////////////////////////////////////////////////
// CShowStatus_StatusDlg
//////////////////////////////////////////////////////////////////////

CShowStatus_StatusDlg::CShowStatus_StatusDlg()
{
   m_pDlg = NULL;
}

void CShowStatus_StatusDlg::TalkMessage(LPCTSTR lpszMessage, int nProgressPos)
{
   if (!IsReady()) return;
   m_pDlg->SetStatus(lpszMessage, nProgressPos);
}

BOOL CShowStatus_StatusDlg::IsReady()
{
   if ( m_pDlg == NULL )
      return FALSE;
   return m_pDlg->IsReady();
}

BOOL CShowStatus_StatusDlg::CheckForBreak()
{
   if ( m_pDlg == NULL )
      return FALSE;
   return m_pDlg->CheckForBreak();
}

void CShowStatus_StatusDlg::GoToExit()
{
   if ( m_pDlg == NULL ) return;
   m_pDlg->GoToExit();
}

BOOL CShowStatus_StatusDlg::AskForRepeat()
{
   if ( m_pDlg == NULL ) return FALSE;
   m_pDlg->PrepareForRepeatMode();
   while(TRUE)
   {
      if (m_pDlg->CheckForBreak())
         return FALSE;
      if (m_pDlg->CheckForRetry()) {
         m_pDlg->PrepareForWorkMode();
         return TRUE;
      }
      Sleep(50);
   }
}
