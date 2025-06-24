#pragma once

/*
* �⺻ CIPAddressCtrl������ �Է� �� IPN_FIELDCHANGED �̺�Ʈ�� ���������� return Ű�� �������� �����Ƿ�
* �Է� �� return Ű �̺�Ʈ�� �ٷ� ������ ó���� �� ����.
* �Ǵ� KillFocus������ ó���� �� �ִ� �̺�Ʈ�� ������ �������� �����Ƿ� customize �ϱ� ���� ������.
*/

#include <afxcmn.h>

static const UINT Message_CSCIPAddressCtrl = ::RegisterWindowMessage(_T("MessageString_CSCIPAddressCtrl"));


class CSCIPAddressCtrl : public CIPAddressCtrl
{
	DECLARE_DYNAMIC(CSCIPAddressCtrl)

public:
	CSCIPAddressCtrl();
	virtual ~CSCIPAddressCtrl();

protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnIpnFieldchanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
};


