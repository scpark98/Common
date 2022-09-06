// ColorDialogEx.cpp : implementation file
//

//#include "stdafx.h"
#include "ColorDialogEx.h"

#include "../../colors.h"

#ifndef _WIN32_WCE // CColorDialog is not supported for Windows CE.

// CColorDialogEx

IMPLEMENT_DYNAMIC(CColorDialogEx, CColorDialog)

CColorDialogEx::CColorDialogEx(COLORREF clrInit, DWORD dwFlags, CWnd* pParentWnd) :
	CColorDialog(clrInit, dwFlags, pParentWnd)
{
}

CColorDialogEx::~CColorDialogEx()
{
}


BEGIN_MESSAGE_MAP(CColorDialogEx, CColorDialog)
END_MESSAGE_MAP()



// CColorDialogEx message handlers


#endif // !_WIN32_WCE


BOOL CColorDialogEx::OnInitDialog()
{
	CColorDialog::OnInitDialog();

	// TODO:  Add extra initialization here

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}


INT_PTR CColorDialogEx::DoModal()
{
	// TODO: Add your specialized code here and/or call the base class
	CString str;

	//�÷��� ������ �������� ����ϴ� ������ ������ �� DoModal�� ȣ���Ѵ�.
	//m_cc.Flags |= CC_RGBINIT | CC_FULLOPEN;

	//���� ����ڰ� �߰��� ������ ������ �̸� ǥ���ϰ�
	//���ٸ� �⺻ ���ǵ� ���� 16���� ǥ���Ѵ�.
	if ((0x8000 == (GetKeyState(VK_SHIFT) & 0x8000)) ||
		AfxGetApp()->GetProfileInt(_T("setting\\custom colors"), _T("00"), -1) == -1)
	{
		m_cc.lpCustColors = g_default_color;
	}
	else
	{
		for (int i = 0; i < 16; i++)
		{
			str.Format(_T("%02d"), i);
			m_cc.lpCustColors[i] = AfxGetApp()->GetProfileInt(_T("setting\\custom colors"), str, 0xffffff);
		}
	}

	return CColorDialog::DoModal();
}


BOOL CColorDialogEx::OnColorOK()
{
	// TODO: Add your specialized code here and/or call the base class
	CString str;

	for (int i = 0; i < 16; i++)
	{
		str.Format(_T("%02d"), i);
		AfxGetApp()->WriteProfileInt(_T("setting\\custom colors"), str, m_cc.lpCustColors[i]);
	}

	return CColorDialog::OnColorOK();
}
