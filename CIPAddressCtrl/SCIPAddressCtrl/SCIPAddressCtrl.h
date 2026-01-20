#pragma once

/*
* 기본 CIPAddressCtrl에서는 입력 시 IPN_FIELDCHANGED 이벤트는 지원하지만 return 키는 지원하지 않으므로
* 입력 후 return 키 이벤트로 바로 뭔가를 처리할 수 없다.
* 또는 KillFocus에서도 처리할 수 있는 이벤트를 별도로 제공하지 않으므로 customize 하기 위해 생성함.
*/

#include <afxcmn.h>

static const UINT Message_CSCIPAddressCtrl = ::RegisterWindowMessage(_T("MessageString_CSCIPAddressCtrl"));


class CSCIPAddressCtrl : public CIPAddressCtrl
{
	DECLARE_DYNAMIC(CSCIPAddressCtrl)

public:
	CSCIPAddressCtrl();
	virtual ~CSCIPAddressCtrl();

	CString		get_text();

protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnIpnFieldchanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
};


