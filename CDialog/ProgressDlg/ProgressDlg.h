#pragma once
#include "afxwin.h"
#include "afxcmn.h"

#include "../Common/CSliderCtrl/MacProgressCtrl.h"
#include "../Common/CStatic/StaticEx.h"

// CProgressDlg dialog
/*
1.Resource Editor에서 IDD_PROGRESS를 추가하고 클래스 이름은 CProgressDlg로 생성한다.
2.메시지를 표시하는 static과 progress 컨트롤을 추가하고 아래와 같이 id를 변경해준다.
	IDC_STATIC_MESSAGE
	IDC_PROGRESS
3.기존 ProgressDlg.cpp와 h 파일을 덮어쓴다.
*/

class CProgressDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CProgressDlg)

public:
	CProgressDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CProgressDlg();

	void		SetIndeterminate(bool bIndeterminate = true);
	void		SetRange(int min, int max);
	void		SetPos(int pos) { m_Progress.SetPos(pos); };

	void		SetMessageText(CString str) { m_Static_Message.SetText(str); m_Static_Message.Invalidate(); }
// Dialog Data
	enum { IDD = IDD_PROGRESS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	CStaticEx m_Static_Message;
	CMacProgressCtrl m_Progress;
	virtual BOOL OnInitDialog();
};
