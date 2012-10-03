// ShowStatus.h: interface for the CShowStatus class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SHOWSTATUS_H__FF0BEE42_4125_40CB_8B2A_90CCA0C41D8E__INCLUDED_)
#define AFX_SHOWSTATUS_H__FF0BEE42_4125_40CB_8B2A_90CCA0C41D8E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CShowStatus
{
public:
   virtual void TalkMessage(LPCTSTR lpszMessage, int nProgressPos = -1) = 0;
   virtual BOOL IsReady() = 0;
   virtual BOOL CheckForBreak() = 0;
   virtual void GoToExit() = 0;
   virtual BOOL AskForRepeat() = 0;
	CShowStatus();
	virtual ~CShowStatus();
};

// Hidden for reducing EXE size
/*
#include <iostream.h>
class CShowStatus_IOStream : public CShowStatus 
{
   virtual void TalkMessage(LPCTSTR lpszMessage, int nProgressPos = -1) {
      cout << lpszMessage << endl;
   }
   virtual BOOL IsReady() { return TRUE; }
   virtual BOOL CheckForBreak() { return FALSE; }
   virtual void GoToExit() { }
   virtual BOOL AskForRepeat() { return FALSE; }
};
*/
#include "StatusDlg.h"
class CShowStatus_StatusDlg : public CShowStatus
{
public:
   void SetDlg(CStatusDlg* pDlg) { m_pDlg = pDlg; }
   virtual void TalkMessage(LPCTSTR lpszMessage, int nProgressPos = -1);
   virtual BOOL IsReady();
   virtual BOOL CheckForBreak();
   virtual void GoToExit();
   virtual BOOL AskForRepeat();
   CShowStatus_StatusDlg();
protected:
   CStatusDlg* m_pDlg;
};

#endif // !defined(AFX_SHOWSTATUS_H__FF0BEE42_4125_40CB_8B2A_90CCA0C41D8E__INCLUDED_)
