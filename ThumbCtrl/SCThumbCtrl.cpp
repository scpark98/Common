// CSCThumbCtrl.cpp: 구현 파일
//

#include "SCThumbCtrl.h"
#include "../MemoryDC.h"
#include "../Functions.h"

// CSCThumbCtrl 대화 상자

IMPLEMENT_DYNAMIC(CSCThumbCtrl, CDialogEx)

CSCThumbCtrl::CSCThumbCtrl()
{
	memset(&m_lf, 0, sizeof(LOGFONT));
}

CSCThumbCtrl::~CSCThumbCtrl()
{
}

void CSCThumbCtrl::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

bool CSCThumbCtrl::create(CWnd* parent, int left, int top, int right, int bottom)
{
	m_parent = parent;

	//DWORD dwStyle = WS_POPUP;
	DWORD dwStyle = WS_CHILD | WS_VISIBLE;

	WNDCLASS wc = {};
	::GetClassInfo(AfxGetInstanceHandle(), _T("#32770"), &wc);
	wc.lpszClassName = _T("CSCThumbCtrl");
	AfxRegisterClass(&wc);

	if (right <= left)
		right = left + 100;
	if (bottom <= top)
		bottom = top + 100;

	bool res = CreateEx(NULL, wc.lpszClassName, _T("CSCThumbCtrl"), dwStyle, CRect(left, top, right, bottom), parent, 0);

	CRect rc;
	GetClientRect(rc);
	//TRACE(_T("rc = %s\n"), get_rect_info_string(rc));

	dwStyle = GetWindowLongPtr(m_hWnd, GWL_STYLE);
	dwStyle &= ~(WS_CAPTION);
	SetWindowLongPtr(m_hWnd, GWL_STYLE, dwStyle);

	//dwStyle = GetWindowLong(m_hWnd, GWL_EXSTYLE) | WS_EX_LAYERED;
	//SetWindowLongPtr(m_hWnd, GWL_EXSTYLE, dwStyle);

	//GetClientRect(rc);
	//TRACE(_T("rc = %s\n"), get_rect_info_string(rc));

	return res;
}

BEGIN_MESSAGE_MAP(CSCThumbCtrl, CDialogEx)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_HSCROLL()
	ON_WM_MOUSEWHEEL()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_MOUSEMOVE()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_RBUTTONDBLCLK()
END_MESSAGE_MAP()


// CSCThumbCtrl 메시지 처리기

//타일 크기, 마진, 간격, 스크롤, 컨트롤 크기조정 등에 따라 각 썸네일이 표시되는 r이 재계산된다.
//이러한 변화에 대해 재계산하고 OnPaint()에서는 r에 표시만 하면 된다.
void CSCThumbCtrl::recalc_tile_rect()
{
	int		i;
	CRect	rc;
	CRect	rTile;

	GetClientRect(rc);

	//thumb크기에 따라 tile 크기는 자동 계산된다.
	m_sz_tile.cx = m_r_inner.left + m_sz_thumb.cx + m_r_inner.right;
	m_sz_tile.cy = m_r_inner.top + m_sz_thumb.cy + m_thumb_title_gap + m_title_height + m_r_inner.bottom;

	//맨 첫번째 항목의 위치를 잡고 인덱스가 증가하면 그 위치를 조정해준다.
	rTile = make_rect(m_sz_margin.cx, m_sz_margin.cy + m_scroll_pos, m_sz_tile.cx, m_sz_tile.cy);

	for (i = 0; i < m_thumb.size(); i++)
	{
		m_thumb[i].r = rTile;
		m_thumb[i].thumb_bottom = rTile.top + m_r_inner.top + m_sz_thumb.cy;

		rTile.OffsetRect(rTile.Width() + m_sz_gap.cx, 0);

		if ((rTile.right + m_sz_margin.cx > rc.right))
		{
			rTile.left = m_sz_margin.cx;
			rTile.top = rTile.bottom + m_sz_gap.cy;
			rTile.right = rTile.left + m_sz_tile.cx;
			rTile.bottom = rTile.top + m_sz_tile.cy;
		}
	}

	m_scroll_total = rTile.bottom - m_scroll_pos + m_sz_margin.cy;
	TRACE(_T("m_scroll_total = %d\n"), m_scroll_total);

	//맨 아래로 스크롤한 후 창 크기를 늘리거나 썸네일 크기를 줄이면 하단에 공백이 생기므로 m_scroll_pos를 보정해줘야 한다.
	Clamp(m_scroll_pos, -m_scroll_total + rc.Height(), 0);

	Invalidate();
}

void CSCThumbCtrl::OnPaint()
{
	CPaintDC dc1(this); // device context for painting

	int i;
	CRect rc;

	GetClientRect(rc);

	CMemoryDC dc(&dc1, &rc);
	Gdiplus::Graphics g(dc);

	g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
	g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
	g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

	dc.SetBkMode(TRANSPARENT);
	dc.FillSolidRect(rc, m_theme.cr_back.ToCOLORREF());

	CFont* pOldFont = (CFont*)dc.SelectObject(&m_font);

	if (m_files.size() > 0 && !m_loading_completed)
	{
		CRect r = make_center_rect(rc.CenterPoint().x, rc.CenterPoint().y, rc.Width() / 2, 16);
		draw_sunken_rect(&dc, r, true, Gdiplus::Color::DimGray, Gdiplus::Color::DarkGray);
		r.DeflateRect(1, 1);
		double width = r.Width();
		dc.FillSolidRect(r, green);
		return;
	}
	else if (m_thumb.size() == 0)
	{
		CString str = _T("표시할 이미지가 없습니다.");
		dc.DrawText(str, rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		//DrawTextShadow(&dc, str, rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE, GRAY64, red);
		//DrawShadowText(dc.GetSafeHdc(), str, str.GetLength(), rc,
		//	DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP, 0, red, 2, 4);
		draw_rectangle(&dc, rc, GRAY192);
		dc.SelectObject(pOldFont);
		return;
	}

	for (i = 0; i < m_thumb.size(); i++)
	{
		//현재 표시할 썸네일이 rc 영역 밖이라면 그려줄 필요가 없다.
		if (get_intersect_rect(rc, m_thumb[i].r).IsRectEmpty())
			continue;

		CRect rThumb = m_thumb[i].r;

		//선택된 항목이면
		if (find_index(m_selected, i) >= 0)
			draw_rectangle(g, rThumb, Gdiplus::Color(153, 209, 255), Gdiplus::Color(128, 204, 232, 255));

		rThumb.DeflateRect(m_r_inner);
		rThumb.bottom -= (m_thumb_title_gap + m_title_height);
		CRect rImage = get_ratio_rect(rThumb, m_thumb[i].img.width, m_thumb[i].img.height, DT_BOTTOM);

		if (m_thumb[i].img.is_empty())
		{
			DrawShadowText(dc.GetSafeHdc(), _T("X"), -1, rThumb,
				DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP, RGB(255, 0, 0), 0, 2, 1);
		}

		g.DrawImage(m_thumb[i].img, CRect2GpRect(rImage));

		CRect rTitle = rThumb;
		rTitle.top = rThumb.bottom + m_thumb_title_gap;
		rTitle.bottom = rTitle.top + m_title_height;

		CString title = (m_show_extension ? m_thumb[i].title : get_part(m_thumb[i].title, fn_title));
		dc.DrawText(title, rTitle, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
		//draw_rectangle(g, rTitle, Gdiplus::Color::Blue);

		//항목 인덱스 표시
		if (m_show_index)
		{
			CRect rIndex = m_thumb[i].r;
			rIndex.top += 4;
			//멀티바이트 환경에서는 DrawShadowText를 쓰기위해서는 manifest를 추가해야 하는 등
			//번거로워 일단 DrawTextShadow로 대체한다.
			DrawShadowText(dc.GetSafeHdc(), i2S(i), -1, rIndex,
							DT_CENTER | DT_TOP | DT_NOCLIP, RGB(255, 255, 255), 0, 2, 1);
		}
	}

	dc.SelectObject(pOldFont);


	/*
	//선택된 항목에 특수한 표시를 해야 할 경우는 이 블럭을 주석해제하여 수정한다. (m_img_selection_mark or m_use_circle_number)
	Gdiplus::Color cr_text(128, 255, 24, 16);
	Gdiplus::Font font(L"Spoqa Han Sans Neo", 32, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
	Gdiplus::FontFamily fontFamily(L"맑은 고딕");
	Gdiplus::SolidBrush br_text(cr_text);
	Gdiplus::Pen pen(Gdiplus::Color(64, 0, 0, 255), 8);

	Gdiplus::StringFormat format;
	format.SetAlignment(Gdiplus::StringAlignmentCenter);
	format.SetLineAlignment(Gdiplus::StringAlignmentCenter);

	Gdiplus::GraphicsPath path;

	for (i = 0; i < m_selected.size(); i++)
	{
		CRect r = m_thumb[m_selected[i]].r;
		//r.top -= 4;
		r.InflateRect(4, 2, 4, 2);

		if (m_img_selection_mark.is_valid())
		{
			r = make_center_rect(r.CenterPoint().x, r.CenterPoint().y, m_img_selection_mark.width, m_img_selection_mark.height);
			m_img_selection_mark.draw(g, r.left, r.top);
		}
		else if (m_use_circle_number)
		{
			//g.DrawString(CStringW(i2S(i)), -1, &font, CRect2GpRectF(r), &format, &br_text);

			r = make_center_rect(r.CenterPoint().x, r.CenterPoint().y, 60, 60);

			g.DrawEllipse(&pen, CRect2GpRectF(r));
			path.AddString(CStringW(i2S(i + 1)), -1, &fontFamily, Gdiplus::FontStyleBold, 32, CRect2GpRectF(r), &format);
			for (int j = 0; j < 4; ++j)
			{
				Gdiplus::Pen pen(Gdiplus::Color(128, 64, 64, 64), j);		//윤곽선
				pen.SetLineJoin(Gdiplus::LineJoinRound);
				g.DrawPath(&pen, &path);
			}
			Gdiplus::SolidBrush brush(Gdiplus::Color(128, 255, 128, 255));	//글자색
			g.FillPath(&brush, &path);
			path.Reset();
		}
		else
		{
			draw_rectangle(g, r, Gdiplus::Color(153, 209, 255), Gdiplus::Color(128, 204, 232, 255));
		}
	}
	*/




	/*
	CRect	rItem(m_sz_margin.cx, m_sz_margin.cy + m_scroll_pos, m_sz_margin.cx + m_sz_tile.cx, m_sz_margin.cy + m_sz_tile.cy + m_scroll_pos);

	for (i = 0; i < m_thumb.size(); i++)
	{
		if ((rItem.top > rc.bottom) || ((rItem.bottom + 80) < rc.top))
		{
			
		}

		CRect rTile = rItem;
		CRect rFit;

		if (m_thumb[i].img.is_valid() == false)
		{
			dc.SetTextColor(deeppink);
			dc.DrawText(_T("Ｘ"), rItem, DT_SINGLELINE | DT_VCENTER | DT_CENTER);
		}
		else
		{
			rFit = get_ratio_rect(rItem, (double)m_thumb[i].img.width / (double)m_thumb[i].img.height);
			m_thumb[i].img.draw(g, rFit.left, rItem.bottom - rFit.Height(), rFit.Width(), rFit.Height());
		}

		m_thumb[i].r = rItem;
		//m_thumb[i].r.bottom = rItem.bottom + height + 2;
		m_thumb[i].r.InflateRect(2, 2);
		m_thumb[i].thumb_bottom = rItem.bottom;
		//m_thumb[i].line_index = nLineCount;

		rItem.left += (m_sz_gap.cx + m_sz_tile.cx);
		rItem.right = rItem.left + m_sz_tile.cx;

		if ((rItem.right + m_sz_margin.cx > rc.right + 2))
		{
			//m_per_line = dqOneLineHeight.size();

			//std::deque<int>::iterator result = std::max_element(dqOneLineHeight.begin(), dqOneLineHeight.end());
			rItem.left = m_sz_margin.cx;
			rItem.top = rItem.bottom + m_sz_gap.cy;// +result[0];
			rItem.right = rItem.left + m_sz_tile.cx;
			rItem.bottom = rItem.top + m_sz_tile.cy;

			//m_line_height.push_back(result[0]);
			//dqOneLineHeight.clear();
			//nLineCount++;
		}
	}

	m_scroll_total = rItem.bottom - m_scroll_pos + m_sz_margin.cy;
	TRACE(_T("m_scroll_total = %d\n"), m_scroll_total);
	*/
}

BOOL CSCThumbCtrl::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	return FALSE;
	return CDialogEx::OnEraseBkgnd(pDC);
}

void CSCThumbCtrl::set_path(CString path)
{
	m_files.clear();

	m_files = find_all_files(path, _T(""), FILE_EXTENSION_IMAGE, _T(""), false);

	add_files(m_files);
}

CSCThumbCtrl* pThisWnd;
void CSCThumbCtrl::add_files(std::deque<CString> files, bool reset)
{
	pThisWnd = this;

	if (reset)
	{
		m_selected.clear();
		m_thumb.clear();
	}

	if (files.size() == 0)
	{
		m_loading_completed = true;
		::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCThumbCtrl,
			(WPARAM)&CSCThumbCtrlMsg(GetDlgCtrlID(), CSCThumbCtrlMsg::message_thumb_loading_completed, 0), 0);
		Invalidate();
		return;
	}

	m_thumb.resize(files.size());
	m_thread.job(files.size(), loading_function, loading_completed_callback);

	//for (int i = 0; i < files.size(); i++)
	//{
	//	m_thumb[i].img.load(files[i]);
	//	m_thumb[i].img.resize(100, 100);
	//	m_thumb[i].title = get_part(files[i], fn_name);
	//}



	Invalidate();
}

int CSCThumbCtrl::insert(int index, CString full_path, CString title, bool key_thumb, bool invalidate)
{
	m_thumb[index].img.load(full_path);
	m_thumb[index].width = m_thumb[index].img.width;
	m_thumb[index].height = m_thumb[index].img.height;
	m_thumb[index].channel = m_thumb[index].img.channel;

	CRect r = get_ratio_rect(CRect(0, 0, m_sz_thumb.cx, m_sz_thumb.cy), m_thumb[index].img.width, m_thumb[index].img.height);
	m_thumb[index].img.resize(r.Width(), r.Height());
	m_thumb[index].title = title;
	m_thumb[index].full_path = full_path;
	m_thumb[index].score = 0.0;
	m_thumb[index].feature = NULL;

	if (invalidate)
	{
		//recalculate_scroll_size();
		Invalidate();
	}

	return index;
}

#include <mutex>
std::mutex mtx;

void CSCThumbCtrl::loading_function(int idx, int start, int end)
{
	int i;
	CString str;

	if (end - start == 1)
		str.Format(_T("%d job started..."), start);
	else
		str.Format(_T("%d ~ %d job started..."), start, end - 1);
	TRACE(_T("%s\n"), str);

	for (i = start; i < end; i++)
	{
		if (pThisWnd->m_files[i].Right(3).MakeLower() == _T("gif"))
		{
			//CGifEx gif;
			//gif.Load(m_loading_files[i], m_hWnd, true);
			//mat = HBITMAP2Mat(gif.get_hBitmap());
		}
		else
		{
			//img->Load(m_loading_files[i]);
		}

		pThisWnd->insert(i, pThisWnd->m_files[i], get_part(pThisWnd->m_files[i], fn_name), false, false);
	}

	if ((end - start) == 1)
		str.Format(_T("job completed : %d"), start);
	else
		str.Format(_T("job completed : %d ~ %d"), start, end - 1);
	TRACE(_T("%s\n"), str);

	mtx.lock();
	pThisWnd->m_thread.thread_ended.push_back(idx);
	mtx.unlock();
}

void CSCThumbCtrl::loading_completed_callback()
{
	pThisWnd->on_loading_completed();
	//pWnd->m_loading_completed = true;
	//if (pWnd->m_index_select_after_loading >= 0)
	//	pWnd->m_selected.push_back(pWnd->m_index_select_after_loading);

	//pWnd->recalculate_scroll_size();
}

void CSCThumbCtrl::on_loading_completed()
{
	m_loading_completed = true;
	recalc_tile_rect();
	//if (m_index_select_after_loading >= 0)
	//	m_selected.push_back(m_index_select_after_loading);

	//recalculate_scroll_size();

	//::SendMessage(GetParent()->GetSafeHwnd(), Message_CThumbCtrl,
	//	(WPARAM)&CThumbCtrlMsg(GetDlgCtrlID(), CThumbCtrlMsg::message_thumb_loading_completed, 0), 0);
}

BOOL CSCThumbCtrl::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	if (pMsg->message == WM_KEYDOWN)
	{
		if (!IsWindowVisible())
			return false;

		switch (pMsg->wParam)
		{
			case VK_F2:
				if (m_in_editing)
					return true;
				edit_begin(-1);
				return true;
			default :
				return FALSE;
		}
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

void CSCThumbCtrl::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	if (!m_hWnd)
		return;

	recalc_tile_rect();
}


void CSCThumbCtrl::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);
}

BOOL CSCThumbCtrl::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	//if (is_editing())
	//	edit_end();

	if (IsCtrlPressed())
	{
		enlarge_size(zDelta > 0);
		return TRUE;
	}
	else
	{
		scroll_up(zDelta > 0);
	}

	return CDialogEx::OnMouseWheel(nFlags, zDelta, pt);
}

void CSCThumbCtrl::set_scroll_pos(int pos)
{
	CRect rc;
	GetClientRect(rc);

	if (m_scroll_total > rc.Height())
		Clamp(pos, -m_scroll_total + rc.Height(), 0);
	else
		pos = 0;

	m_scroll_pos = pos;
	//TRACE(_T("current = %d / %d\n"), m_scroll_pos, m_scroll_total);
	m_scroll_trans = 0.0;
	recalc_tile_rect();

	CPoint pt;
	GetCursorPos(&pt);
	ScreenToClient(&pt);

	//커서가 스크롤바 영역 밖이라면 스크롤바를 숨겨준다.
	//if ((rc.PtInRect(pt) == false) || !m_rScroll.PtInRect(pt))
	//	SetTimer(timer_scroll_bar_disappear, 2000, NULL);
}

void CSCThumbCtrl::scroll_up(bool up)
{
	CRect rc;
	GetClientRect(rc);

	if (rc.Height() >= m_scroll_total)
		return;

	int pos = m_scroll_pos;
	pos += (up ? 1 : -1) * (m_sz_tile.cy + m_sz_gap.cy);

	/*
	if (up)
		pos += ((m_sz_tile.cy + 20 + m_sz_gap.cy) >> 2) * 3;
	else
		pos -= ((m_sz_tile.cy + 20 + m_sz_gap.cy) >> 2) * 3;
	*/

	set_scroll_pos(pos);
}

void CSCThumbCtrl::PreSubclassWindow()
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	//자기 자신에게 부여된 폰트가 없다면 null이 리턴된다.
	//dlg의 parent의 font를 얻어와야 한다.
	CFont* font = AfxGetApp()->GetMainWnd()->GetFont();

	if (font != NULL)
		font->GetObject(sizeof(m_lf), &m_lf);
	else
		GetObject(GetStockObject(SYSTEM_FONT), sizeof(m_lf), &m_lf);

	reconstruct_font();

	CDialogEx::PreSubclassWindow();
}

void CSCThumbCtrl::reconstruct_font()
{
	m_lf.lfQuality = CLEARTYPE_NATURAL_QUALITY;

	m_font.DeleteObject();
	m_font.CreateFontIndirect(&m_lf);

	SetFont(&m_font, true);
}

//m_sz_thumb의 크기를 기준으로 tile의 크기가 자동 계산되므로 thumb의 크기를 변경해준다.
void CSCThumbCtrl::enlarge_size(bool enlarge)
{
	if (enlarge && (m_sz_tile.cx >= MAX_TILE_SIZE))
		return;
	if (!enlarge && (m_sz_tile.cx <= MIN_TILE_SIZE))
		return;

	m_sz_thumb.cx = m_sz_thumb.cx + (enlarge ? 4 : -4);
	m_sz_thumb.cy = m_sz_thumb.cy + (enlarge ? 4 : -4);

	AfxGetApp()->WriteProfileInt(_T("setting\\thumbctrl"), _T("thumb size cx"), m_sz_thumb.cx);
	AfxGetApp()->WriteProfileInt(_T("setting\\thumbctrl"), _T("thumb size cy"), m_sz_thumb.cy);

	recalc_tile_rect();
}


void CSCThumbCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (m_in_editing)
		edit_end();

	SetFocus();

	int i, j;
	CRect rc;
	GetClientRect(rc);

	if (m_rScroll.PtInRect(point))
	{
		m_scroll_drag = true;
		SetCapture();
		CRect rc;
		GetClientRect(rc);
		set_scroll_pos(-(double)(point.y - m_scroll_grip_size / 2) / (double)(rc.Height() - m_scroll_grip_size) * (double)(m_scroll_total - rc.Height()));
		return;
	}

	for (i = 0; i < m_thumb.size(); i++)
	{
		if (m_thumb[i].r.PtInRect(point))
		{
			int selected_index = find_index(m_selected, i);

			//컨트롤키를 누르면 m_use_multi_selection이 false라고 해도 다중선택이 가능하다.
			if (IsCtrlPressed())
			{
				int last_selected = 0;
				if (m_selected.size() > 0)
					last_selected = m_selected[m_selected.size() - 1];

				//해당 항목이 이미 선택된 상태라면 선택 해제시켜주고
				if (selected_index >= 0)
				{
					//shift까지 눌렸다면 마지막 선택 ~ 현재 항목까지 모두 해제.
					if (IsShiftPressed())
					{
						//마지막 선택 ~ 현재 항목중에서 선택된 항목이 있다면 삭제시킨다.
						if (m_selected.size())
						{
							for (j = m_selected.size() - 1; j >= 0; j--)
							{
								if ((m_selected[j] >= last_selected && m_selected[j] <= i) ||
									(m_selected[j] <= last_selected && m_selected[j] >= i))
									m_selected.erase(m_selected.begin() + j);
							}
						}
					}
					else
					{
						m_selected.erase(m_selected.begin() + selected_index);
						::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCThumbCtrl,
							(WPARAM)&CSCThumbCtrlMsg(GetDlgCtrlID(), CSCThumbCtrlMsg::message_thumb_lbutton_unselected, selected_index), 0);
					}
				}
				//선택되지 않은 항목이라면 선택리스트에 추가한다.
				else
				{
					//shift까지 눌렸다면 마지막 선택 ~ 현재 항목까지 모두 선택
					if (IsShiftPressed())
					{
						//마지막 선택 ~ 현재 항목중에서 선택된 항목이 있다면 삭제하고 추가해야 한다.
						if (m_selected.size())
						{
							for (j = m_selected.size() - 1; j >= 0; j--)
							{
								if ((m_selected[j] >= last_selected && m_selected[j] <= i) ||
									(m_selected[j] <= last_selected && m_selected[j] >= i))
									m_selected.erase(m_selected.begin() + j);
							}
						}

						for (j = MIN(i, last_selected); j <= MAX(i, last_selected); j++)
						{
							m_selected.push_back(j);
							::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCThumbCtrl,
								(WPARAM)&CSCThumbCtrlMsg(GetDlgCtrlID(), CSCThumbCtrlMsg::message_thumb_lbutton_selected, i), 0);
						}
					}
					else
					{
						m_selected.push_back(i);
						::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCThumbCtrl,
							(WPARAM)&CSCThumbCtrlMsg(GetDlgCtrlID(), CSCThumbCtrlMsg::message_thumb_lbutton_selected, i), 0);
					}
				}
			}
			else
			{
				//전에 선택항 항목과 새로 선택한 항목이 다르면 m_last_clicked를 초기화시켜줘야
				//원클릭에 의한 타이틀 변경이 올바르게 동작한다.
				if (m_selected.size() && (i != m_selected[0]))
					m_last_clicked = 0;

				//단일 선택이면 기존 선택리스트를 초기화하고 선택리스트에 새로 넣어준다.
				if (!m_use_multi_selection)
					m_selected.clear();

				if (selected_index >= 0 && m_selected.size() && find_index(m_selected, m_selected[selected_index]) >= 0)
				{
					m_selected.erase(m_selected.begin() + selected_index);
					::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCThumbCtrl,
						(WPARAM)&CSCThumbCtrlMsg(GetDlgCtrlID(), CSCThumbCtrlMsg::message_thumb_lbutton_unselected, selected_index), 0);
				}
				else
				{
					if (find_index(m_selected, i) < 0)
						m_selected.push_back(i);
					::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCThumbCtrl,
						(WPARAM)&CSCThumbCtrlMsg(GetDlgCtrlID(), CSCThumbCtrlMsg::message_thumb_lbutton_selected, i), 0);
				}
			}

			//선택된 항목이 rc안에 다 안들어오고 일부만 보이면 다 보이도록 스크롤 시켜준다.
			if (rect_in_rect(rc, m_thumb[i].r) == false)
			{
				//int max_height = m_line_height[m_dqThumb[i].line_index];
				//SetScrollPos(SB_VERT, m_dqThumb[i].rect.CenterPoint().y - m_dqThumb[0].rect.CenterPoint().y - rc.Height() / 2);
				ensure_visible(i);
			}

#ifdef _DEBUG
			for (int j = 0; j < m_selected.size(); j++)
				Trace(_T("%d "), m_selected[j]);
			Trace(_T("\n"));
#endif
			Invalidate();
			return;
		}
	}

	//클릭된 곳이 썸네일 영역이 아니면 선택을 해제시켜준다.
	if (!m_use_multi_selection)
		m_selected.clear();

	Invalidate();

	CDialogEx::OnLButtonDown(nFlags, point);
}

void CSCThumbCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (m_scroll_drag)
	{
		ReleaseCapture();
		m_scroll_drag = false;
		CRect rc;
		GetClientRect(rc);
		set_scroll_pos(-(double)(point.y - m_scroll_grip_size / 2) / (double)(rc.Height() - m_scroll_grip_size) * (double)(m_scroll_total - rc.Height()));
		return;
	}

	int index = get_selected_item();
	if (index < 0)
		return;

	//name영역이 눌려지고
	CRect r = m_thumb[index].r;
	r.top = m_thumb[index].thumb_bottom;
	if (r.PtInRect(point) == false)
		return;

	long t1 = getClock();
	//TRACE(_T("clicked = %d\n"), t1 - m_last_clicked);

	//name영역을 클릭한 후 1~2초 사이에 다시 클릭하면 name 편집모드로 전환된다.
	if ((abs(t1 - m_last_clicked) >= 1000) && (abs(t1 - m_last_clicked) < 2000))
		edit_begin(-1);

	m_last_clicked = t1;

	CDialogEx::OnLButtonUp(nFlags, point);
}

void CSCThumbCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	CDialogEx::OnLButtonDblClk(nFlags, point);
}

void CSCThumbCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	CDialogEx::OnMouseMove(nFlags, point);
}

void CSCThumbCtrl::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	CDialogEx::OnRButtonDown(nFlags, point);
}

void CSCThumbCtrl::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	CDialogEx::OnRButtonUp(nFlags, point);
}

void CSCThumbCtrl::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	CDialogEx::OnRButtonDblClk(nFlags, point);
}

void CSCThumbCtrl::ensure_visible(int index)
{
	if (index < 0 || index >= m_thumb.size())
		return;

	CRect rc;
	GetClientRect(rc);

	//TRACE(_T("thumb rect = %s\n"), GetRectInfoString(m_dqThumb[index].rect, 1));
	//썸네일의 rect가 rc 영역에 들어와있지 않으면 스크롤시킨다.
	if (rect_in_rect(rc, m_thumb[index].r) == false)
	{
		set_scroll_pos(-(m_thumb[index].thumb_bottom - m_thumb[0].thumb_bottom - rc.Height() / 2 + m_thumb[index].r.Height() / 2));
	}
}

#define IDC_EDIT_CELL 29832
void CSCThumbCtrl::edit_begin(int index)
{
	if (!m_show_title)
		return;

	if (index < 0)
	{
		if (m_selected.size() > 0)
		{
			//멀티선택사용일 경우는 맨 마지막 선택된 항목을
			if (m_use_multi_selection)
				index = m_selected[m_selected.size() - 1];
			//그렇지 않으면 선택된 항목을 편집모드로 한다.
			else
				index = get_selected_item();
		}
		else
		{
			return;
		}
	}

	DWORD dwStyle = ES_CENTER | WS_BORDER | WS_CHILD | WS_VISIBLE /*| ES_AUTOHSCROLL */ | ES_AUTOVSCROLL | ES_MULTILINE;
	CRect r = m_thumb[index].r;
	//r.right--;
	//r.InflateRect(5, 5);
	r.top = m_thumb[index].thumb_bottom + 10;
	r.bottom = r.top + 32;

	if (m_pEdit == NULL)
	{
		m_pEdit = new CSCEdit();
		m_pEdit->Create(dwStyle, r, this, IDC_EDIT_CELL);
		m_pEdit->SetFont(&m_font);
		//DWORD margin = m_pEdit->GetMargins();
		m_pEdit->SetMargins(0, 0);
		m_pEdit->set_line_align(DT_VCENTER);
	}

	CDC* pDC = m_pEdit->GetDC();
	CRect textRect = r;

	pDC->DrawText(m_thumb[index].title, textRect, DT_CALCRECT/*| DT_CENTER | DT_WORDBREAK | DT_EDITCONTROL*/);

	m_in_editing = true;
	m_editing_index = index;
	m_old_title = m_thumb[index].title;

	m_pEdit->SetWindowText(m_thumb[index].title);

	//텍스트 출력 높이를 위에서 계산해서 해봤으나 DrawText와 CEdit의 텍스트가 그려지는 모양이 다르다. 그냥 이방법이 정확하다.
	//다만 폰트의 높이에 따라 다를 수 있으나 아직 확인하진 않았다.
	int lc = m_pEdit->GetLineCount();
	r.bottom = r.top + (textRect.Height() + (lc == 1 ? 8 : 8)) * lc;

	//if (textRect.Width() > r.Width())
	//	r.InflateRect((textRect.Width() - r.Width()) / 2, 0);

	if (m_show_resolution)
		r.OffsetRect(0, textRect.Height() - 6);
	else
		r.OffsetRect(0, -9);
	m_pEdit->MoveWindow(r);
	m_pEdit->ShowWindow(SW_SHOW);
	m_pEdit->SetFocus();
	m_pEdit->SetSel(0, (m_show_extension ? m_thumb[index].title.ReverseFind('.') : -1));
}

void CSCThumbCtrl::edit_end(bool valid)
{
	if (!m_in_editing)
		return;

	m_in_editing = false;
	m_pEdit->ShowWindow(SW_HIDE);

	CString sText;

	if (valid)
	{
		m_pEdit->GetWindowText(sText);

		if (sText == m_old_title)
			return;

		TRACE(_T("editted text = %s\n"), sText);
		sText.Trim();

		if (sText.IsEmpty())
			return;

		m_thumb[m_editing_index].title = sText;
		Invalidate();

		//썸네일의 이름을 변경하는 것은 파일명인지 무엇인지를 이 컨트롤에서 처리하면 안된다.
		//용도에 맞게 메인에서 어떤 변경인지에 따라 처리한다.
		::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCThumbCtrl,
			(WPARAM)&CSCThumbCtrlMsg(GetDlgCtrlID(), CSCThumbCtrlMsg::message_thumb_rename, m_editing_index), 0);
	}
}

int CSCThumbCtrl::get_selected_item()
{
	if (m_selected.size())
		return m_selected[0];
	return -1;
}

int	CSCThumbCtrl::get_index_from_point(CPoint pt)
{
	for (int i = 0; i < m_thumb.size(); i++)
	{
		if (m_thumb[i].r.PtInRect(pt))
			return i;
	}
	return -1;
}

//dqSelected를 NULL로 주고 선택 개수만 리턴받아 쓰기도 한다.
int CSCThumbCtrl::get_selected_items(std::deque<int>* dqSelected)
{
	if (dqSelected)
	{
		dqSelected->clear();
		dqSelected->assign(m_selected.begin(), m_selected.end());
	}
	return m_selected.size();
}

//index = -1 : 전체선택
void CSCThumbCtrl::select_item(int index, bool select, bool make_ensure_visible)
{
	if (!m_loading_completed)
	{
		//m_index_select_after_loading = index;
		//SetTimer(timer_select_after_loading, 1000, NULL);
		return;
	}

	int i;

	if (index == -1)
	{
		if (select)
		{
			if (m_selected.size() < m_thumb.size())
				m_selected.resize(m_thumb.size());

			m_selected.clear();

			for (i = 0; i < m_thumb.size(); i++)
				m_selected.push_back(i);
		}
		else
		{
			m_selected.clear();
			make_ensure_visible = false;
			make_ensure_visible = false;
		}
	}
	else
	{
		if (IsShiftPressed())
		{
			if (m_selected.size() > 0)
			{
				int last = m_selected[m_selected.size() - 1];
				int curr = index;

				while (true)
				{
					if (find_index(m_selected, curr) < 0)
						m_selected.push_front(curr);

					if (curr < last)
						curr++;
					else if (curr > last)
						curr--;
					else
						break;
				}
			}
		}
		else
		{
			m_selected.clear();
			m_selected.push_back(index);
		}
	}

	if (index >= 0 && make_ensure_visible)
		ensure_visible(index);

	Invalidate();
}

std::deque<int> CSCThumbCtrl::find_text(bool show_inputbox, bool select)
{
	return find_text(true, _T(""), select);
}

std::deque<int> CSCThumbCtrl::find_text(bool show_inputbox, CString text, bool select)
{
	TCHAR buf[200] = { 0 };
	TCHAR msg[200] = { 0 };
	int i;
	std::deque<int> dqFound;

	if (show_inputbox)
	{
		_stprintf_s(msg, _countof(msg), _T("검색어를 입력하세요."));
		int res = 0;// CWin32InputBox::InputBox(_T("검색어 입력"), msg, buf, 200, false);

		if (res == IDCANCEL)
			return dqFound;

		text = buf;
	}


	for (i = 0; i < m_thumb.size(); i++)
	{
		if (m_thumb[i].full_path.Find(text) >= 0)
			dqFound.push_back(i);
	}

	if (select)
	{
		m_selected.clear();
		m_selected.assign(dqFound.begin(), dqFound.end());

		if (dqFound.size() > 0)
			ensure_visible(dqFound[0]);
	}

	Invalidate();
	MessageBeep(0);

	return dqFound;
}

void CSCThumbCtrl::set_color_theme(int theme)
{
	m_theme.set_color_theme(theme);
	Invalidate();
}
