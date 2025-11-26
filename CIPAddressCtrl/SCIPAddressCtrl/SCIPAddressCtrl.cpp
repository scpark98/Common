// SCIPAddressCtrl.cpp: 구현 파일
//

#include "SCIPAddressCtrl.h"


// CSCIPAddressCtrl

IMPLEMENT_DYNAMIC(CSCIPAddressCtrl, CIPAddressCtrl)

CSCIPAddressCtrl::CSCIPAddressCtrl()
{

}

CSCIPAddressCtrl::~CSCIPAddressCtrl()
{
}


BEGIN_MESSAGE_MAP(CSCIPAddressCtrl, CIPAddressCtrl)
	ON_NOTIFY_REFLECT(IPN_FIELDCHANGED, &CSCIPAddressCtrl::OnIpnFieldchanged)
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()



// CSCIPAddressCtrl 메시지 처리기



BOOL CSCIPAddressCtrl::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
			case VK_RETURN :
				TRACE(_T("return on CSCIPAddressCtrl::PreTranslateMessage\n"));
				::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCIPAddressCtrl, WM_KEYDOWN, pMsg->wParam);
				return TRUE;
		}
	}
	return CIPAddressCtrl::PreTranslateMessage(pMsg);
}

void CSCIPAddressCtrl::OnIpnFieldchanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMIPADDRESS pIPAddr = reinterpret_cast<LPNMIPADDRESS>(pNMHDR);
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCIPAddressCtrl, IPN_FIELDCHANGED, 0);

	*pResult = 0;
}

void CSCIPAddressCtrl::OnKillFocus(CWnd* pNewWnd)
{
	CIPAddressCtrl::OnKillFocus(pNewWnd);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCIPAddressCtrl, WM_KILLFOCUS, 0);
}
