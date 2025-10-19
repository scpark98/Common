////////////////////////////////////////////////////////////////////////////////////
#include "PropGridSlider.h"
////////////////////////////////////////////////////////////////////////////////////


BEGIN_MESSAGE_MAP(CPropSlider, CSliderCtrl)
	ON_NOTIFY_REFLECT(NM_RELEASEDCAPTURE, &CPropSlider::OnNMReleasedcapture)
	ON_WM_SETCURSOR()
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()


void CPropSlider::OnNMReleasedcapture(NMHDR *pNMHDR, LRESULT *pResult)
{
	OnUpdateValue();
	*pResult = 0;
}

BOOL CPropSlider::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	switch (pMsg->message) {
		case WM_MOUSEMOVE:
			if (pMsg->wParam == 1) OnUpdateValue();
			break;
		case WM_MOUSEWHEEL:
			OnUpdateValue();
			break;
		case WM_KEYDOWN:
			OnUpdateValue();
			break;
	}

	return CSliderCtrl::PreTranslateMessage(pMsg);
}

void CPropSlider::OnUpdateValue()
{
	int pos = GetPos();
	if (pos != m_iPrevPos) {
		m_iPrevPos = pos;
		m_pParentProp->OnSliderPosChanged();
	}
}

void CPropGridSlider::OnSliderPosChanged()
{
	int pos = m_pSlider->GetPos();
	float ratio = ((float)pos / float(m_step));
	float value = (m_fMax - m_fMin) * ratio + m_fMin;
	SetValue((float)value);
	m_pWndList->OnPropertyChanged(this);
}

void CPropGridSlider::SetSliderPos()
{
	if (m_fMax == m_fMin)
	{
		m_pSlider->SetPos(0);
		return;
	}

	COleVariant value = GetValue();
	float fval = value.fltVal;
	float ratio = (fval - m_fMin) / (m_fMax - m_fMin);
	int pos = (int)(ratio * float(m_step));

	m_pSlider->SetPos(pos);
}

BOOL CPropGridSlider::OnEndEdit() 
{
	SetSliderPos();
	return CMFCPropertyGridProperty::OnEndEdit();
}

void CPropGridSlider::OnDrawValue(CDC* pDC, CRect rect)
{
	CRect rt = rect;
	int w = rect.Width();

	rect.right = rect.left 
		+ (m_iEditCtrlWidth < 0 ? w / (-m_iEditCtrlWidth) : m_iEditCtrlWidth);
	rt.left = rect.right;

	if (!m_pSlider) {
		m_pSlider = new CPropSlider(this);
		//원래는 TBS_BOTH였으나 약간 위로 밀리고 thumb도 작게 표시되어 TBS_TOP으로 변경함.
		DWORD style = WS_VISIBLE | WS_CHILD | TBS_TOP | TBS_NOTICKS;
		m_pSlider->Create(style, rt, m_pWndList, 5);
		m_pSlider->SetRange(0, m_step);
		m_pSlider->ShowWindow(SW_SHOW);
		SetSliderPos();
	}
	
	m_pSlider->MoveWindow(&rt);
	CMFCPropertyGridProperty::OnDrawValue(pDC, rect);
}

CWnd* CPropGridSlider::CreateInPlaceEdit(CRect rectEdit, BOOL& bDefaultFormat)
{
	int w = rectEdit.Width();
	rectEdit.right = rectEdit.left 
		+ (m_iEditCtrlWidth < 0 ? w / (-m_iEditCtrlWidth) : m_iEditCtrlWidth);

	return CMFCPropertyGridProperty::CreateInPlaceEdit(rectEdit, bDefaultFormat);
}


BOOL CPropSlider::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	::SetCursor(::LoadCursor(NULL, IDC_ARROW));
	return TRUE;
	//return CSliderCtrl::OnSetCursor(pWnd, nHitTest, message);
}


//void CPropSlider::OnPaint()
//{
//	CPaintDC dc(this); // device context for painting
//	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
//	// 그리기 메시지에 대해서는 CSliderCtrl::OnPaint()을(를) 호출하지 마십시오.
//	RedrawWindow();
//}


HBRUSH CPropSlider::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CSliderCtrl::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO:  여기서 DC의 특성을 변경합니다.

	// TODO:  기본값이 적당하지 않으면 다른 브러시를 반환합니다.
	pDC->SetBkColor(RGB(255, 0, 0));
	static CBrush br(RGB(255, 0, 0));

	return (HBRUSH)br;
}
