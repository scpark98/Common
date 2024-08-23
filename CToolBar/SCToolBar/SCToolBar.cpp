// SCToolBar.cpp: 구현 파일
//

#include "SCToolBar.h"
#include "../../Functions.h"

// CSCToolBar 대화 상자

IMPLEMENT_DYNAMIC(CSCToolBar, CDialogEx)

CSCToolBar::CSCToolBar()
{

}

CSCToolBar::~CSCToolBar()
{
	for (int i = 0; i < m_toolbar.size(); i++)
	{
		delete m_toolbar[i];
	}
}

void CSCToolBar::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CSCToolBar, CDialogEx)
	ON_REGISTERED_MESSAGE(Message_CGdiButton, &CSCToolBar::on_message_GdiButton)
END_MESSAGE_MAP()

bool CSCToolBar::create()
{
	DWORD dwStyle = WS_CHILD | WS_VISIBLE;

	bool res = CreateEx(0, NULL, _T("CSCToolBar"), dwStyle, CRect(m_sx, m_sy, m_sx + m_width, m_sy + m_height), m_parent, 0);
	ShowWindow(SW_SHOW);
	//LONG_PTR wStyle = GetWindowLongPtr(m_hWnd, GWL_STYLE); // Get the current style
	//wStyle &= ~WS_BORDER;   // Here, we mask out the style bit(s) we want to remove
	//SetWindowLongPtr(m_hWnd, GWL_STYLE, wStyle);

	/*
	if (res)
	{
		CFont* font = GetFont();

		if (font == NULL)
			font = AfxGetMainWnd()->GetFont();

		if (font != NULL)
			font->GetObject(sizeof(m_lf), &m_lf);
		else
			GetObject(GetStockObject(SYSTEM_FONT), sizeof(m_lf), &m_lf);

		reconstruct_font();
	}
	*/

	return res;
}

void CSCToolBar::init(CWnd* parent, int x, int y, int width, int height)
{
	m_parent = parent;
	m_sx = x;
	m_sy = y;
	m_width = MAX(width, m_width);
	m_height = MAX(height, m_height);

	create();
}

void CSCToolBar::add(CString caption, UINT resource_id, int button_type)
{
	insert(-1, caption, resource_id, button_type);
}

//resourceID와 type(BS_PUSHBUTTON or BS_CHECKBOX or BS_RADIOBUTTON...)만 주면 버튼 추가
void CSCToolBar::insert(int index, CString caption, UINT resource_id, int button_type)
{
	int sz_separator = 20;
	CRect r = CRect(0, 0, m_width, m_height);
	CGdiButton* btn = new CGdiButton;
	
	btn->create(caption, WS_CHILD | WS_VISIBLE | BS_PUSHLIKE | BS_FLAT, r, this, SCTOOLBAR_BUTTON_ID);
	btn->set_tooltip_text(caption);

	//빈 버튼 영역 또는 분리기호인 경우
	if (caption.IsEmpty() || resource_id <= 0 || button_type < 0)
	{
		if (button_type < 0)
		{
			btn->SetWindowPos(NULL, 0, 0, sz_separator, r.Height(), SWP_NOMOVE | SWP_NOZORDER);
			btn->GetWindowRect(r);
		}

		btn->set_transparent(true);

		//btn->back_color(m_cr_back, m_cr_back, m_cr_back, m_cr_back);
		//btn->EnableWindow(FALSE);
		//btn->use_normal_image_on_disabled();
	}
	else
	{
		btn->SetButtonStyle(button_type);

		CGdiplusBitmap img(_T("PNG"), resource_id), img1;
		img.canvas_size(m_width, m_height);

		img.deep_copy(&img1);
		img1.replace_color(Gdiplus::Color(255, 45, 51, 51));

		btn->back_color(m_cr_back);
		btn->fit_to_image(false);

		btn->add_image(&img);

		if (button_type == BS_CHECKBOX)
			btn->add_image(&img1);
	}

	if (index < 0)
		m_toolbar.push_back(btn);
	else
		m_toolbar.insert(m_toolbar.begin() + index, btn);

	rearrange_toolbar();
}

void CSCToolBar::remove(int index)
{
	if (index < 0 || index >= m_toolbar.size())
		return;

	//동적으로 생성하여 추가한 항목을 제거할때는 반드시 해당 항목을 delete한 후에
	//deque에서 erase해줘야 한다.
	delete m_toolbar[index];
	m_toolbar.erase(m_toolbar.begin() + index);

	rearrange_toolbar();
}

void CSCToolBar::rearrange_toolbar()
{
	int x = 0;

	CRect r, rc;
	CSize total(0, 0);

	for (int i = 0; i < m_toolbar.size(); i++)
	{
		m_toolbar[i]->GetClientRect(rc);

		r = rc;

		r.left = x;
		r.right = r.left + rc.Width();
		total.cx += rc.Width();

		m_toolbar[i]->MoveWindow(r);
		x = r.right;
	}

	SetWindowPos(NULL, m_sx, m_sy, total.cx, m_height, SWP_NOZORDER);
}

void CSCToolBar::set_back_color(COLORREF cr_back)
{
	m_cr_back = cr_back;
}

// CSCToolBar 메시지 처리기
LRESULT	CSCToolBar::on_message_GdiButton(WPARAM wParam, LPARAM lParam)
{
	CGdiButtonMessage* msg = (CGdiButtonMessage*)wParam;
	TRACE(_T("btn = %p, msg = %d\n"), msg->m_pWnd, msg->m_message);

	if (msg->m_message == WM_LBUTTONUP)
	{
		CString caption;
		msg->m_pWnd->GetWindowText(caption);
		TRACE(_T("toolbar id = %d, caption = %s\n"), msg->m_ctrl_id, caption);
	}
	//main dlg에 그림이 깔려있는 상태에서 투명 GdiButton이 있는 경우라면
	//배경 그림과 관계없이 투명 GdiButton이 잘 표시되고 leave시에도 잘 갱신되지만
	//main dlg안에 SCToolbar가 있고 그 안에 투명 GdiButton이 있다면
	//GdiButton에서 hover, leave시에 화면을 갱신하는 것은 GdiButton 내부에서의 처리로만은 해결되지 않는다.
	//GdiButton의 parent인 SCToolBar에서 그 메시지를 받아서 main dlg의 해당 영역을 invalidate()해줘야 한다.
	else if (msg->m_message == WM_MOUSEHOVER)
	{
		CRect rc;
		msg->m_pWnd->GetWindowRect(rc);
		GetParent()->ScreenToClient(rc);
		GetParent()->InvalidateRect(rc, TRUE);
	}
	else if (msg->m_message == WM_MOUSELEAVE)
	{
		CRect rc;
		msg->m_pWnd->GetWindowRect(rc);
		GetParent()->ScreenToClient(rc);
		GetParent()->InvalidateRect(rc, TRUE);
	}

	return 0;
}


BOOL CSCToolBar::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	if (pMsg->message == WM_KEYDOWN)
	{
		return false;
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}
