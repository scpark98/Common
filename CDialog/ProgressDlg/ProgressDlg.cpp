// ProgressDlg.cpp : implementation file
//

#include "stdafx.h"
#include "VerasysPlayer.h"
#include "ProgressDlg.h"
#include "afxdialogex.h"


// CProgressDlg dialog

IMPLEMENT_DYNAMIC(CProgressDlg, CDialogEx)

CProgressDlg::CProgressDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CProgressDlg::IDD, pParent)
{

}

CProgressDlg::~CProgressDlg()
{
}

void CProgressDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_MESSAGE, m_Static_Message);
	DDX_Control(pDX, IDC_PROGRESS, m_Progress);
}


BEGIN_MESSAGE_MAP(CProgressDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CProgressDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CProgressDlg::OnBnClickedCancel)
END_MESSAGE_MAP()


// CProgressDlg message handlers

BOOL CProgressDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  Add extra initialization here
	//m_Progress.SetColor(  RGB( 128, 128, 255 ), ::GetSysColor( COLOR_3DSHADOW ) );
	//m_Static_Message.SetTextColor( 0 );
	//m_Static_Message.SetBackColor( ::GetSysColor( COLOR_3DFACE ) );
	m_Static_Message.SetTransparent( false );
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


void CProgressDlg::OnBnClickedOk()
{
}


void CProgressDlg::OnBnClickedCancel()
{
	ShowWindow( SW_HIDE );
}

void CProgressDlg::SetIndeterminate(bool bIndeterminate)
{
	m_Progress.SetIndeterminate( bIndeterminate );
}

void CProgressDlg::SetRange(int min, int max)
{
	m_Progress.SetRange32(min, max);
	m_Progress.Invalidate();
}