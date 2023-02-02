/*******************************************************************************
File:        SysTrayIcon.h

Description: This file contains the module for creating and manipulating 
			 a system tray icon	 
             
Created: Dec 5, 2002
Author:  Prateek Kaul
e-mail:  kaulpr@yahoo.com

Compiler with version number : Visual C++ 6.0/7.0 (.NET)
********************************************************************************/

/*사용법
- SysTrayIcon.h와 cpp를 프로젝트에 추가하고 h파일에 다음을 추가.
#include "../Common/SysTrayIcon.h"
	CSysTrayIcon	m_SysTray;
	LRESULT			OnSysTrayMessage( WPARAM, LPARAM );

- main dlg에서 다음과 같이 사용한다.
	m_SysTray.SetParent( m_hWnd );							//마우스 이벤트를 처리할 parent hwnd 설정
    HICON hIcon = ::AfxGetApp()->LoadIcon(IDR_MAINFRAME);
    m_SysTray.CreateIcon(hIcon, 1, _T("ProcessKiller"));	//트레이 아이콘과 툴팁 설정
	m_SysTray.ShowIcon( 1 );								//아이콘 표시
- 팝업메뉴든 show, hide든 main dlg에서 메시지를 받아서 처리한다.
*/

#ifndef SYSTRAYICON_H
#define SYSTRAYICON_H


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <afxwin.h>
#include <afxtempl.h>
#include <shellapi.h>

class CSysTrayIcon;

#define WM_SYSTRAYMSG  (WM_USER + 1)


/*---------------------------------------------------------------------------
 class CSysTrayIconWnd

 Created: Dec 5, 2002
 Author:  Prateek Kaul
 e-mail:  kaulpr@yahoo.com

 Abstract : Helper invisbile window to route messages to the CSysTrayIcon class

 Revisions :none.
----------------------------------------------------------------------------*/

class CSysTrayWnd : public CWnd  
{
    DECLARE_MESSAGE_MAP()
public:
	CSysTrayWnd();
	virtual ~CSysTrayWnd();
	
	BOOL CreateWnd(
	         CSysTrayIcon* pstiSysTrayIcon,
	         LPCTSTR lpszClassName,
             LPCTSTR lpszWindowName,
             DWORD dwStyle,
             int x,
             int y,
             int nWidth,
             int nHeight,
             HWND hWndParent,
             HMENU nIDorHMenu,
             LPVOID lpParam = NULL 
         );

protected:
    afx_msg LRESULT OnSysTrayMsg(WPARAM wParam, LPARAM lParam);
    CSysTrayIcon* m_pstiSysTrayIcon;
    CPoint GetMouseScreenPt(void);
	void OnMenuExit();
};


/*---------------------------------------------------------------------------
 class CSysTrayIcon

 Created: Dec 5, 2002
 Author:  Prateek Kaul
 e-mail:  kaulpr@yahoo.com

 Abstract : For creating and manipluating a Sys tray icon

 Revisions :none.
----------------------------------------------------------------------------*/

typedef CTypedPtrList<CPtrList, NOTIFYICONDATA*> NOTIFYICONDATAList;

class CSysTrayIcon
{
    friend class CSysTrayWnd; // The CSysTrayWnd is a friend so that it call call the protected mouse
                              // overridable methods for message routing

public:
    CSysTrayIcon();   
    virtual ~CSysTrayIcon();

	//parent에게 SW_MINIMIZE등의 메시지 전송
	void SetParent( HWND hParent ) { m_hParentWnd = hParent; }
	HWND GetParent() { return m_hParentWnd; }
    BOOL CreateIcon(HICON hIcon, UINT nIconID, LPCTSTR szTip);
    BOOL ShowIcon(UINT nIconID); // Show an icon i     
    BOOL HideIcon(UINT nIconID);
	BOOL DeleteIcon(UINT nIconID); // Remove an icon from the Sys tray
	BOOL ChangeIconTip(UINT nIconID, CString strTip);


	BOOL ShowBalloon(
	         UINT  nIconID,        // nIconID
			LPCTSTR szBalloonTitle, // Balloon title
			LPCTSTR szBalloonMsg,   // Balloon message
             DWORD dwIcon = NIIF_NONE, // Type of icon in the message ICON_ERROR, ICON_INFO,ICON_WARNING
             UINT nTimeOut = 10        // Time(secs)for the balloon to be shown, shoudn't be less than 10 sec.
         );

            
protected:

    virtual void OnLButtonDown(UINT nIconID, CPoint ptMouse);
    virtual void OnRButtonDown(UINT nIconID, CPoint ptMouse);
    virtual void OnMButtonDown(UINT nIconID, CPoint ptMouse);
    virtual void OnLButtonUp(UINT nIconID, CPoint ptMouse);
    virtual void OnRButtonUp(UINT nIconID, CPoint ptMouse);
    virtual void OnMButtonUp(UINT nIconID, CPoint ptMouse);
    virtual void OnLButtonDblClk(UINT nIconID, CPoint ptMouse);
    virtual void OnRButtonDblClk(UINT nIconID, CPoint ptMouse);
    virtual void OnMButtonDblClk(UINT nIconID, CPoint ptMouse);
    virtual void OnMouseMove(UINT nIconID, CPoint ptMouse);
                                                
private:
	HWND m_hParentWnd;
	BOOL CreateTheSysTrayMsgReceiverWnd();
    CSysTrayWnd m_wndTrayMsgReceiver;     // Helper invisible window, that will receive
                                          // the tray icon messages
    NOTIFYICONDATAList m_NotifyIconDataList;  // List of NOTIFIYICONDATA structs
    BOOL CheckIfIconIDExists(UINT nIconID);
    BOOL DeleteNotifyIconDataFromList(NOTIFYICONDATA* pnidIconInfoToBeDeleted);
    NOTIFYICONDATA* GetNotifyIconDataFromID(int nIconID);
};


#endif // SYSTRAYICON_H