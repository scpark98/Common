#include "ListBoxEx.h"

#include "../../colors.h"
#include "../../MemoryDC.h"

CListBoxEx::CListBoxEx()
{
	m_index = 0;
	m_selected = -1;
	m_use_hover = false;

	m_margin = CPoint(4, 0);

	set_color_theme(color_theme_default, false);

	memset(&m_lf, 0, sizeof(LOGFONT));
}



CListBoxEx::~CListBoxEx()
{
}


BOOL CListBoxEx::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Add your specialized code here and/or call the base class

	return CStatic::PreCreateWindow(cs);
}


void CListBoxEx::PreSubclassWindow()
{
	// TODO: Add your specialized code here and/or call the base class
	CFont* font = GetParent()->GetFont();

	if ( font != NULL )
		font->GetObject(sizeof(m_lf), &m_lf);
	else
		GetObject(GetStockObject(SYSTEM_FONT), sizeof(m_lf), &m_lf);

	reconstruct_font();

	//CStatic::PreSubclassWindow();
}


BOOL CListBoxEx::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_UP :
			scroll(true);
			break;
		case VK_DOWN :
			scroll(false);
			break;
		case VK_PRIOR :
			scroll(true, 10);
			break;
		case VK_NEXT :
			scroll(false, 10);
			break;
		}
	}
	else if (pMsg->message == WM_LBUTTONDBLCLK)
	{
		return false;
	}

	return CStatic::PreTranslateMessage(pMsg);
}

BEGIN_MESSAGE_MAP(CListBoxEx, CStatic)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_WINDOWPOSCHANGED()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	//ON_WM_LBUTTONDBLCLK()
	ON_WM_MOUSEMOVE()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()


void CListBoxEx::OnPaint()
{
	CPaintDC dc1(this); // device context for painting
					   // TODO: Add your message handler code here
					   // Do not call CStatic::OnPaint() for painting messages
	int i;
	CRect rc, rect, rText;

	GetClientRect(rc);

	CMemoryDC dc(&dc1, &rc, true);
	dc.FillSolidRect(rc, m_crBack);

	dc.SetBkMode(TRANSPARENT);
	CFont *pOldFont = (CFont*)dc.SelectObject(&m_font);

	rect = CRect(rc.left, m_margin.y, rc.right, m_margin.y + m_line_height);

	for (i = m_index; i < m_list.size(); i++)
	{
		if (i == m_highlighted)
		{
			dc.SetTextColor(m_crTextHighlighted);
			//dc.FillSolidRect(rect, m_crBackSelected);
		}
		else if (i == m_selected)
		{
			dc.SetTextColor(m_crTextSelected);
			dc.FillSolidRect(rect, m_crBackSelected);
		}
		else if (m_use_hover && (i == m_hover))
		{
			dc.SetTextColor(m_crTextHover);
			dc.FillSolidRect(rect, m_crBackHover);
		}
		else
		{
			dc.FillSolidRect(rect, m_list[i].crBack);

			if (m_list[i].crText == list_unused_color)
				dc.SetTextColor(m_crText);
			else
				dc.SetTextColor(m_list[i].crText);
		}

		rText = rect;
		rText.left += 4;
		dc.DrawText(m_list[i].text, rText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

		rect.OffsetRect(0, m_line_height);
		if (rect.top > rc.bottom)
			break;
	}

	dc.SelectObject(pOldFont);
	//dc.SetTextColor(oldTextColor);
}


BOOL CListBoxEx::OnEraseBkgnd(CDC* pDC)
{
	// TODO: Add your message handler code here and/or call default
	return false;
	return CStatic::OnEraseBkgnd(pDC);
}


void CListBoxEx::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CStatic::OnWindowPosChanged(lpwndpos);

	// TODO: Add your message handler code here
	Invalidate();
}


void CListBoxEx::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	point.y -= m_margin.y;
	m_selected = m_index + point.y / m_line_height;
	if (m_selected < 0 || m_selected >= m_list.size())
		m_selected = -1;
	Invalidate();

	CStatic::OnLButtonDown(nFlags, point);
}


void CListBoxEx::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	CStatic::OnLButtonUp(nFlags, point);
}


void CListBoxEx::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	MessageBeep(0);
	CStatic::OnLButtonDblClk(nFlags, point);
}


void CListBoxEx::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	if (m_use_hover)
	{
		point.y -= m_margin.y;
		m_hover = m_index + point.y / m_line_height;
		Invalidate();
	}

	CStatic::OnMouseMove(nFlags, point);
}


void CListBoxEx::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	CStatic::OnRButtonDown(nFlags, point);
}


void CListBoxEx::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	CStatic::OnRButtonUp(nFlags, point);
}


void CListBoxEx::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	CStatic::OnRButtonDblClk(nFlags, point);
}


//이 함수에서는 m_lf 정보를 이용해서 폰트를 재생성한다.
//즉, m_lf.lfHeight 값을 이용해서 폰트가 만들어지므로
//m_font_size 멤버 변수의 값이 변경되었다면
//공식을 이용해 이를 m_lf.lfHeight 값으로 변경한 후 이 함수가 호출되어야 한다.
//m_lf.lfHeight값의 절대값이 MM_TEXT모드의 현재 DC에서의 실제 픽셀크기가 된다.
//따라서 스크롤 크기 등을 계산할때는 m_font_size를 이용하는게 아니라
//m_lf.lfHeight값을 이용해야 정확한 스크롤 크기가 계산된다.
//m_font_size는 단지 사용자에게 일반적인 폰트 크기 설정 수치로 쓰이는 직관적인 수치이다.
void CListBoxEx::reconstruct_font()
{
	m_font.DeleteObject();
	BOOL bCreated = m_font.CreateFontIndirect(&m_lf);
	SetFont(&m_font, true);

	m_line_height = -m_lf.lfHeight + 8;
	m_font_size = get_font_size();

	ASSERT(bCreated);
}

int CListBoxEx::get_font_size()
{
	m_font_size = -MulDiv(m_lf.lfHeight, 72, GetDeviceCaps(::GetDC(GetParent()->GetSafeHwnd()), LOGPIXELSY));
	return m_font_size;
}

//예를 들어 폰트 크기를 10으로 설정하면
void CListBoxEx::set_font_size( int font_size )
{
	if (font_size == 0)
		return;

	m_font_size = font_size;
	//For the MM_TEXT mapping mode,
	//you can use the following formula to specify 
	//a height for a font with a specified point size:
	m_lf.lfHeight = -MulDiv(m_font_size, GetDeviceCaps(::GetDC(GetParent()->GetSafeHwnd()), LOGPIXELSY), 72);
	reconstruct_font();
}

void CListBoxEx::enlarge_font_size(bool enlarge)
{
	m_font_size = get_font_size();
	enlarge ? m_font_size++ : m_font_size--;
	m_lf.lfHeight = -MulDiv(m_font_size, GetDeviceCaps(::GetDC(GetParent()->GetSafeHwnd()), LOGPIXELSY), 72);
	reconstruct_font();
}

void CListBoxEx::set_font_name(LPCTSTR sFontname, BYTE byCharSet)
{
	if (sFontname == _T(""))
		return;
	m_lf.lfCharSet = byCharSet;
	_tcscpy( m_lf.lfFaceName, sFontname);
	reconstruct_font();
}

void CListBoxEx::set_font_bold( bool bBold )
{
	m_lf.lfWeight = ( bBold ? FW_BOLD : FW_NORMAL );
	reconstruct_font();
}

void CListBoxEx::set_font_italic(bool italic)
{
	m_lf.lfItalic = italic;
	reconstruct_font();
}

void CListBoxEx::set_log_font(LOGFONT lf)
{
	memcpy(&m_lf, &lf, sizeof(LOGFONT));
	reconstruct_font();
	Invalidate();
}

void CListBoxEx::set_color_theme(int theme, bool apply_now)
{
	switch ( theme )
	{
	case color_theme_default :
		m_crText				= ::GetSysColor( COLOR_BTNTEXT );
		m_crTextSelected		= ::GetSysColor( COLOR_HIGHLIGHTTEXT );
		m_crTextSelectedInactive= ::GetSysColor( COLOR_INACTIVECAPTIONTEXT );
		m_crBack				= ::GetSysColor( COLOR_WINDOW );
		m_crBackSelected		= ::GetSysColor( COLOR_HIGHLIGHT );
		m_crBackSelectedInactive= ::GetSysColor( COLOR_HIGHLIGHT );
		break;
	case color_theme_light_blue :
		m_crText				= ::GetSysColor( COLOR_BTNTEXT );
		m_crTextSelected		= RGB(  65, 102, 146 );
		m_crTextSelectedInactive= RGB(  65, 102, 146 );
		m_crBack				= RGB( 193, 219, 252 );
		m_crBackSelected		= get_color(m_crBack, -32);
		m_crBackSelectedInactive= RGB( 193, 219, 252 );
		break;
	case color_theme_navy_blue :
		m_crText				= RGB( 204, 216, 225 );
		m_crTextSelected		= RGB( 234, 246, 255 );
		m_crTextSelectedInactive= RGB( 105, 142, 186 );
		m_crBack				= RGB(  74,  94, 127 );
		m_crBackSelected		= RGB(  15,  36,  41 );
		m_crBackSelectedInactive= RGB(  15,  36,  41 );
		break; 
	case color_theme_dark_blue :
		m_crText				= RGB(  16, 177, 224 );
		m_crTextSelected		= RGB( 224, 180,  59 );
		m_crTextSelectedInactive= RGB( 105, 142, 186 );
		m_crBack				= RGB(   2,  21,  36 );
		m_crBackSelected		= RGB(   3,  42,  59 );
		m_crBackSelectedInactive= RGB(  15,  36,  41 );
		break; 
	case color_theme_dark_gray :
		m_crText				= RGB( 164, 164, 164);
		m_crTextHover			= get_color(m_crBack, -32);
		m_crTextSelected		= RGB( 241, 241, 241 );
		m_crTextHighlighted		= RGB( 241, 141, 141 );
		m_crTextSelectedInactive= get_color(m_crTextSelected, -36);
		m_crBack				= RGB(  64,  64,  64 );
		m_crBackHover			= get_color(m_crBack, 32);
		m_crBackSelected		= get_color(m_crBack, -16);
		m_crBackSelectedInactive= get_color(m_crBack, -16);
		break;
	}

	if (apply_now)
	{
		Invalidate();
	}
}


COLORREF CListBoxEx::get_text_color(int item)
{
	//item이 -1이면 해당 셀의 색상이 아니라 기본 텍스트 색상값을 원하는 것이다.
	if (item < 0)
		return m_crText;

	COLORREF cr = m_list[item].crText;
	if (cr == list_unused_color)
		return m_crText;
	return cr;
}

COLORREF CListBoxEx::get_back_color(int item)
{
	COLORREF cr = m_list[item].crBack;
	if (cr == list_unused_color)
		return m_crBack;
	return cr;
}

void CListBoxEx::set_text_color(int item, COLORREF crText)
{
	if (item < 0)
	{
		for (int i = 0; i < m_list.size(); i++)
			m_list[i].crText = crText;
	}
	else
	{
		m_list[item].crText = crText;
	}

	Invalidate();
}

void CListBoxEx::set_back_color(int item, COLORREF crBack)
{
	if (item < 0)
	{
		for (int i = 0; i < m_list.size(); i++)
			m_list[i].crBack = crBack;
	}
	else
	{
		m_list[item].crBack = crBack;
	}

	Invalidate();
}

void CListBoxEx::set_item_color(int item, COLORREF crText, COLORREF crBack)
{
	if (item < 0)
	{
		for (int i = 0; i < m_list.size(); i++)
		{
			m_list[i].crText = crText;
			m_list[i].crBack = crBack;
		}
	}
	else
	{
		m_list[item].crText = crText;
		m_list[item].crBack = crBack;
	}

	Invalidate();
}

void CListBoxEx::set_default_item_color(COLORREF crText, COLORREF crBack)
{
	Invalidate();
}

int CListBoxEx::add_string(CString text, COLORREF crText, COLORREF crBack )
{
	m_list.push_back(CListBoxData(text, crText, crBack));
	Invalidate();
	return m_list.size() - 1;
}

void CListBoxEx::set_alt_text(int index, CString _alt_text)
{
	m_list[index].alt_text = _alt_text;
}

DWORD CListBoxEx::get_data(int index)
{
	return m_list[index].dwData;
}

void CListBoxEx::set_data(int index, DWORD dwData)
{
	m_list[index].dwData = dwData;
}

void CListBoxEx::delete_all_items()
{
	m_list.clear();
	m_selected = -1;
	m_hover = -1;
	Invalidate();
}

void CListBoxEx::set_highlighted(int index)
{
	m_highlighted = index;
	Invalidate();
}

CString	CListBoxEx::get_text(int index, bool alt_text)
{
	if (index >= m_list.size())
		return CString();

	return (alt_text ? m_list[index].alt_text : m_list[index].text);
}

int CListBoxEx::find_string(CString text, bool compare_alt_text)
{
	for (int i = 0; i < m_list.size(); i++)
	{
		if (compare_alt_text)
		{
			if (m_list[i].alt_text == text)
				return i;
		}
		else
		{
			if (m_list[i].text == text)
				return i;
		}
	}

	return -1;
}

void CListBoxEx::scroll(bool up, int interval)
{
	m_index += (up ? -interval : interval);
	Clamp(m_index, 0, (int)(m_list.size() - 1));
	Invalidate();
}


BOOL CListBoxEx::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: Add your message handler code here and/or call default
	scroll(zDelta > 0, 4);
	return CStatic::OnMouseWheel(nFlags, zDelta, pt);
}
