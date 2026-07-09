// SkinHeaderCtrl.cpp : implementation file
//

#include "SCHeaderCtrl.h"

#include "../../Functions.h"
#include "../../MemoryDC.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSCHeaderCtrl

CSCHeaderCtrl::CSCHeaderCtrl()
{
	m_cr_separator = get_weak_color(m_cr_back, 32);
	m_header_is_clicked = false;
	m_header_clicked_index = -1;
	m_header_height = 16;
}

CSCHeaderCtrl::~CSCHeaderCtrl()
{
}


BEGIN_MESSAGE_MAP(CSCHeaderCtrl, CHeaderCtrl)
	//{{AFX_MSG_MAP(CSCHeaderCtrl)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_MESSAGE(HDM_LAYOUT, &CSCHeaderCtrl::OnLayout)
	ON_WM_LBUTTONDBLCLK()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSCHeaderCtrl message handlers
/*
void CSCHeaderCtrl::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{
}
*/

LRESULT CSCHeaderCtrl::OnLayout(WPARAM, LPARAM lParam)
{
	LPHDLAYOUT pHL = reinterpret_cast<LPHDLAYOUT>(lParam);
	if (pHL->prc == NULL || pHL->prc == INVALID_HANDLE_VALUE ||
		pHL->pwpos == NULL || pHL->pwpos == INVALID_HANDLE_VALUE)
		return 0;

	memcpy(&m_HDLayout, pHL, sizeof(HDLAYOUT));

	//*** The table list rectangle
	RECT* pRect = pHL->prc;

	//*** The table header rectangle
	WINDOWPOS* pWPos = pHL->pwpos;

	//*** Sends HDM_LAYOUT message to the base class
	int nRet = CHeaderCtrl::DefWindowProc(HDM_LAYOUT, 0, lParam);

	//아래 x, cy값은 큰 의미는 없는듯하다. 오히려 cy값을 주면 스크롤시 잔상이 생긴다.
	//*** Moves the table header to the right
	pWPos->x += 0;
	//pWPos->x += m_nHdrWeightDefect;

	//*** New table header height
	pWPos->cy = m_header_height + 0;
	//pWPos->cy = m_nHdrHeight + m_nHdrHeightDefect;

	//*** Decreases the table list height on the table header height
	pRect->top = m_header_height;// m_nHdrHeight;

	return nRet;
}

void CSCHeaderCtrl::OnPaint() 
{
	CPaintDC dc1(this);
	CRect rc, rItem;

	GetClientRect(rc);

	CMemoryDC dc(&dc1, &rc);
	Gdiplus::Graphics g(dc);

	dc.SelectObject(GetFont());
	dc.FillSolidRect(rc, GetParent()->IsWindowEnabled() ? m_cr_back.ToCOLORREF() : GRAY(164));
	dc.SetBkMode(TRANSPARENT);
	dc.SetTextColor(GetParent()->IsWindowEnabled() ? m_cr_text.ToCOLORREF() : ::GetSysColor(COLOR_GRAYTEXT));

	Gdiplus::Color crSunkenLight = get_color(m_cr_back, 48);
	Gdiplus::Color crSunkenDark  = get_color(m_cr_back, -48);

	CFont* pOldFont = (CFont*)dc.SelectObject(&m_font);

	int header_count = GetItemCount();

	m_r_header_text.resize(header_count);
	m_r_header_text.clear();

	for (int i = 0; i < GetItemCount(); i++)
	{
		GetItemRect(i, rItem);
		//rItem.top -= 1;
		rItem.bottom -= 1;

		if (i == m_header_clicked_index)
		{
			draw_sunken_rect(&dc, rItem, true, crSunkenDark, crSunkenLight);
			rItem.OffsetRect(1, 1);
		}
		else
		{
			if (m_flat_style)
			{
				//separator를 그릴 때 가로좌표를 rItem.right로 그려주면 그 위치는 맨 우측으로 스크롤했을때 화면에 그려지지 않는다.
				//-2를 해줘야 딱 맞게 표시된다. 물론 이 보정을 해줌으로써 커서변경, 실제 항목들의 우측 잘림 위치 등 약간의 오차는 있지만 거의 구분되지 않는다.
				//오히려 맨 우측으로 스크롤 했는데도 분리기호가 화면에 표시되지 않는 불편함이 더 크다.
				//그리고 더블클릭 시 자동 너비 조정기능이 간혹 반응이 없을때가 있는데 이 조정 후 덜한듯한 느낌이다. 이는 좀 더 테스트해봐야 한다.
				if (m_use_header_separator)
					draw_line(g, rItem.right - 2, rItem.top + 2, rItem.right - 2, rItem.bottom - 2, crSunkenDark);
			}
			else
			{
				draw_sunken_rect(&dc, rItem, false, crSunkenDark, crSunkenLight);
			}
		}

		DWORD dwAlign = m_header_text_align[i];
		DWORD dwFormat = DT_VCENTER | DT_SINGLELINE;
		if (dwAlign == HDF_LEFT)
			dwFormat |= DT_LEFT;
		else if (dwAlign == HDF_CENTER)
			dwFormat |= DT_CENTER;
		else//if (dwAlign == HDF_LEFT)
			dwFormat |= DT_RIGHT;

		rItem.DeflateRect(6, 0);

		CRect rcalc = rItem;
		dc.DrawText(get_header_text(i), rcalc, dwFormat | DT_CALCRECT);
		dc.DrawText(get_header_text(i), rItem, dwFormat);

		m_r_header_text.push_back(rcalc);

		//7x4 크기의 화살표를 그린다.
		if (i == m_cur_sort_column)
		{
			CRect rArrow = rItem;
			rArrow.left = rItem.CenterPoint().x - 4;
			rArrow.right = rItem.CenterPoint().x + 3;
			rArrow.top = rItem.top + 1;
			rArrow.bottom = rArrow.top + 4;

			if (m_cur_sort_asc)
			{
				draw_line(g, rArrow.left, rArrow.top + 1, rArrow.CenterPoint().x, rArrow.bottom, m_cr_sort_arrow);
				draw_line(g, rArrow.CenterPoint().x, rArrow.bottom, rArrow.right - 1, rArrow.top + 1, m_cr_sort_arrow);
			}
			else
			{
				draw_line(g, rArrow.left, rArrow.bottom - 1, rArrow.CenterPoint().x, rArrow.top, m_cr_sort_arrow);
				draw_line(g, rArrow.CenterPoint().x, rArrow.top, rArrow.right - 1, rArrow.bottom - 1, m_cr_sort_arrow);
			}
		}
	}

	dc.SelectObject(pOldFont);
}

BOOL CSCHeaderCtrl::OnEraseBkgnd(CDC* pDC) 
{
	return FALSE;	
	//return CHeaderCtrl::OnEraseBkgnd(pDC);
}

CString	CSCHeaderCtrl::get_header_text(int column)
{
	TCHAR				szText[256];
	HDITEM				hdItem;

	hdItem.mask			= HDI_TEXT;
	hdItem.pszText		= szText;
	hdItem.cchTextMax	= 255;

	GetItem(column, &hdItem);

	return szText;
}

void CSCHeaderCtrl::set_header_text(int column, CString sText)
{
	TCHAR				szText[256];
	HDITEM				hdItem;

	hdItem.mask			= HDI_TEXT;
	hdItem.pszText		= szText;
	hdItem.cchTextMax	= 255;

	GetItem(column, &hdItem);

	_stprintf_s(hdItem.pszText, _countof(szText), _T("%s"), sText);
	SetItem(column, &hdItem);
}

int	CSCHeaderCtrl::get_header_text_align(int column)
{
	return m_header_text_align[column];
}

void CSCHeaderCtrl::set_header_text_align(int column, int format)
{
	if (GetItemCount() <= 0)
		return;

	HDITEM	hdItem;

	//scpark 2010-3-25 10:01:15
	//아래 memset을 안해주면 heap error난다.
	memset(&hdItem, 0, sizeof(hdItem));
	
	GetItem(column, &hdItem);
	hdItem.mask	= HDI_FORMAT;
	hdItem.fmt	= format;
	SetItem(column, &hdItem);

	m_header_text_align[column] = format;
}

int CSCHeaderCtrl::get_clicked_header(CPoint point)
{
	CRect	rItem;

	for (int i = 0; i < GetItemCount(); i++)
	{
		GetItemRect(i, rItem);
		//separator와 구분하기 위해 줄여서 체크.

		rItem.DeflateRect(6, 0);
		if (rItem.PtInRect(point))
		{
			m_header_clicked_index = i;
			return m_header_clicked_index;
		}
	}

	m_header_clicked_index = -1;
	return m_header_clicked_index;
}

void CSCHeaderCtrl::set_sort_arrow(int column, bool sort_asc, Gdiplus::Color cr_sort_arrow)
{
	m_cur_sort_column = column;
	m_cur_sort_asc = sort_asc;

	if (cr_sort_arrow.GetValue() != Gdiplus::Color::Transparent)
		m_cr_sort_arrow = cr_sort_arrow;

	Invalidate();
}

//pt가 separator 위치인지 판별
int CSCHeaderCtrl::is_separator(CPoint pt)
{
	int margin = 4;
	int count = GetItemCount();
	CRect rItem;

	for (int i = 0; i < count; i++)
	{
		GetItemRect(i, rItem);
		rItem.left = rItem.right;
		//마지막 컬럼은 우측 edge 가 header client 의 우측 boundary 와 일치/근접 — 우측 4px 영역이
		//header 밖이라 더블클릭이 header window 에 도달 못 함. 좌측만 inflate.
		//다른 컬럼의 좌우 대칭 hit area (총 폭 2*margin) 와 동일 총 폭 유지를 위해 좌측 2*margin 확장.
		if (i == count - 1)
			rItem.left -= margin * 2;
		else
			rItem.InflateRect(margin, 0);
		if (rItem.PtInRect(pt))
		{
			return i;
		}
	}

	return -1;
}

void CSCHeaderCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	if (!m_allow_sort && is_separator(point) < 0)
		return;

	m_header_is_clicked = true;
	m_header_clicked_index = get_clicked_header(point);
	Invalidate();
	CHeaderCtrl::OnLButtonDown(nFlags, point);
}


void CSCHeaderCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
	if ((!m_allow_sort || !m_header_is_clicked) && is_separator(point) < 0)
		return;

	// TODO: Add your message handler code here and/or call default
	m_header_is_clicked = false;
	m_header_clicked_index = -1;
	Invalidate();
	CHeaderCtrl::OnLButtonUp(nFlags, point);
}

void CSCHeaderCtrl::set_color(Gdiplus::Color cr_text, Gdiplus::Color cr_back, Gdiplus::Color cr_separator)
{
	if (cr_text.GetValue() != Gdiplus::Color::Transparent)
		m_cr_text = cr_text;

	if (cr_back.GetValue() != Gdiplus::Color::Transparent)
		m_cr_back = cr_back;

	if (m_cr_separator.GetA() == 0)
	{
		if (get_gray_value(m_cr_back) < 128)
			m_cr_separator = get_color(m_cr_back, 32);
		else
			m_cr_separator = get_color(m_cr_back, -32);
	}

	if (!m_hWnd)
		return;

	Invalidate();
}

void CSCHeaderCtrl::set_text_color(Gdiplus::Color cr_text)
{
	m_cr_text = cr_text;

	if (!m_hWnd)
		return;
	Invalidate();
}

void CSCHeaderCtrl::set_back_color(Gdiplus::Color crBack)
{
	m_cr_back = crBack;

	if (!m_hWnd)
		return;
	Invalidate();
}

int CSCHeaderCtrl::get_header_height()
{
	CRect r;

	GetItemRect(0, r);
	
	return r.Height();
}

void CSCHeaderCtrl::set_header_height(int height)
{
	m_header_height = height;
	Invalidate();
	//OnLayout이 호출되야 실제 높이가 적용되는데
	//아래 화면갱신 함수로는 적용되지 않는다.
	//SetFont / MoveWindow / SetWindowPos 등이 필요.
	//Invalidate();
	//RedrawWindow();
	//UpdateWindow();
	//SendMessage(HDM_LAYOUT, 0, (LPARAM)&m_HDLayout);
	/*
	//=> CFont를 생성하여 SetFont()하면 된다?
	CDC* pDC = GetDC();
	int real_height = -MulDiv(height, pDC->GetDeviceCaps(LOGPIXELSY), 72);
	ReleaseDC(pDC);

	CFont NewFont;
	NewFont.CreateFont(
		real_height,               // nHeight 
		0,                         // nWidth 
		0,                         // nEscapement 
		0,                         // nOrientation 
		FW_NORMAL,                 // nWeight 
		FALSE,                     // bItalic 
		FALSE,                     // bUnderline 
		0,                         // cStrikeOut 
		ANSI_CHARSET,              // nCharSet 
		OUT_DEFAULT_PRECIS,        // nOutPrecision 
		CLIP_DEFAULT_PRECIS,       // nClipPrecision 
		DEFAULT_QUALITY,           // nQuality 
		DEFAULT_PITCH | FF_SWISS,  // nPitchAndFamily 
		_T("Arial"));             // lpszFacename   

	SetFont(&NewFont);
	*/
}

void CSCHeaderCtrl::set_header_flat_style(bool flat)
{
	m_flat_style = flat;
	Invalidate();
}

//폰트 관련
void CSCHeaderCtrl::set_font(LOGFONT* lf)
{
	memcpy(&m_lf, lf, sizeof(LOGFONT));
	reconstruct_font();
}

void CSCHeaderCtrl::reconstruct_font()
{
	m_font.DeleteObject();
	BOOL bCreated = m_font.CreateFontIndirect(&m_lf);
	SetFont(&m_font, false);
	//20260709 by claude. owner-draw 헤더는 m_lf 로 직접 그리므로 폰트 변경 후 재그리기가 필요하다(SetFont bRedraw=FALSE 로는 갱신 안 됨 → 헤더가 옛 폰트 유지).
	if (::IsWindow(m_hWnd))
		Invalidate();
}

void CSCHeaderCtrl::set_font_name(LPCTSTR sFontname, BYTE byCharSet)
{
	if (sFontname == _T(""))
		return;
	m_lf.lfCharSet = byCharSet;
	_tcscpy_s(m_lf.lfFaceName, _countof(m_lf.lfFaceName), sFontname);
	reconstruct_font();
}

void CSCHeaderCtrl::set_font_bold(int weight)
{
	m_lf.lfWeight = weight;
	reconstruct_font();
}

void CSCHeaderCtrl::set_font_italic(bool italic)
{
	m_lf.lfItalic = italic;
	reconstruct_font();
}

//보통 header의 separator를 더블클릭하면 width를 최소 크기로 조정해주는데
//header text를 고려하지 않거나 checkbox가 있는 등의 경우에는 올바르게 동작하지 않는다.
//따라서 이를 수동으로 조정해주도록 한다.
void CSCHeaderCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	//separator를 dbclick하면 width를 자동 조정해줘야 하는데 오류가 있다. 이를 메인에서 계산하여 조정하도록 함.
	int index = is_separator(point);

	if (index < 0 || index >= GetItemCount())
	{
		//CHeaderCtrl::OnLButtonDblClk(nFlags, point);
		return;
	}

	::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCHeaderCtrl, index, m_r_header_text[index].Width());

	//기본 핸들러를 호출하지 않도록 한다.
	//CHeaderCtrl::OnLButtonDblClk(nFlags, point);
}
