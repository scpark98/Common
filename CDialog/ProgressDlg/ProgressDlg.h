#pragma once
#include "afxwin.h"
#include "afxcmn.h"

#include "../Common/CSliderCtrl/MacProgressCtrl.h"
#include "../Common/CStatic/StaticEx.h"

// CProgressDlg dialog
/*
1.Resource Editor���� IDD_PROGRESS�� �߰��ϰ� Ŭ���� �̸��� CProgressDlg�� �����Ѵ�.
2.�޽����� ǥ���ϴ� static�� progress ��Ʈ���� �߰��ϰ� �Ʒ��� ���� id�� �������ش�.
	IDC_STATIC_MESSAGE
	IDC_PROGRESS
3.���� ProgressDlg.cpp�� h ������ �����.
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
