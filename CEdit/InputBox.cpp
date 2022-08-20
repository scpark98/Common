//#include "stdafx.h"
#include "InputBox.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CInputBox, CFrameWnd)

CInputBox::CInputBox(CWnd *pParent)
{
	this->pParent=pParent;
}

CInputBox::~CInputBox()
{	
}

CInputBox::CInputBox()
{
}

BEGIN_MESSAGE_MAP(CInputBox, CFrameWnd)
	//{{AFX_MSG_MAP(CInputBox)
	ON_WM_CLOSE()
	ON_WM_CREATE()
	ON_BN_CLICKED(5000, OnOk)
	ON_BN_CLICKED(5050, OnCancel)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

int CInputBox::ShowInputBox(CString strPrompt,CString strTitle,CString strDefault)
{
	this->strDefault=strDefault;
	this->strPrompt=strPrompt;
	this->strTitle=strTitle;

	Create( NULL, strTitle, WS_CAPTION | WS_SYSMENU, CRect(0,0,0,0), pParent );
	pParent->EnableWindow( false );	

	RECT parentrect;
	pParent->GetWindowRect( &parentrect );    
	MoveWindow( parentrect.left + ( ( parentrect.right - parentrect.left ) / 2 ) - 150, 
		parentrect.top  + ( ( parentrect.bottom - parentrect.top ) / 2 ) - 65, 300, 150);

	ShowWindow( SW_SHOW );
	UpdateWindow();

	MSG msg;

	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);		

 		if ( msg.message == WM_USER + 1 )
 			break; 

		switch( msg.message ) 
		{
			//
/*
		case WM_DESTROY:
			break;

			//
		case WM_CLOSE:
			break;
*/

			//
			// lots of other cases, probably, and somewhere there is a
		case WM_CHAR:

			switch( msg.wParam )
			{
			case 0x0D:
				OnOk();
				break;

			case 0x1B:
				OnCancel();
				break;

			};

			break;

		};

	}	
	 
	ShowWindow(SW_HIDE);
	return retvalue; 

}

void CInputBox::OnClose() 
{
	pParent->SetForegroundWindow();
	pParent->EnableWindow(true);
	
	CFrameWnd::OnClose();
}


int CInputBox::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;	

	m_dummy.Create(_T(""),WS_VISIBLE,CRect(0,0,300,130),this);

	m_prompt.Create( strPrompt, WS_VISIBLE, CRect( 10, 5, 280, 40 ), this, 3000 );	
	m_edit.Create( WS_VISIBLE | WS_BORDER, CRect( 10, 40, 280, 65 ), this, 4000 );
	m_ok.Create(_T("OK"), WS_VISIBLE, CRect( 50, 80, 140, 110 ), this, 5000 );
	m_cancel.Create(_T("Cancel"), WS_VISIBLE, CRect( 150, 80, 240, 110 ), this, 5050 );
	
	m_edit.SetWindowText( strDefault );
	m_edit.SetFocus();
	
	return 0;
}

void CInputBox::OnCancel()
{
	retvalue  =IDCANCEL;
	InputText = "";
	PostMessage( WM_USER + 1 );
}


void CInputBox::OnOk()
{
	retvalue = IDOK;
	m_edit.GetWindowText( InputText );
	PostMessage( WM_USER + 1 );	
}

void CInputBox::CloseBox()
{
	PostMessage( WM_CLOSE );
}
