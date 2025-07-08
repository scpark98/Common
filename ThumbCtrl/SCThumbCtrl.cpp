// CSCThumbCtrl.cpp: 구현 파일
//

#include "SCThumbCtrl.h"
#include "../MemoryDC.h"
#include "../Functions.h"
#include "../messagebox/Win32InputBox/Win32InputBox.h"

// CSCThumbCtrl 대화 상자

IMPLEMENT_DYNAMIC(CSCThumbCtrl, CDialogEx)

CSCThumbCtrl::CSCThumbCtrl()
{
	memset(&m_lf, 0, sizeof(LOGFONT));

	m_max_thumbs = AfxGetApp()->GetProfileInt(_T("setting\\thumbctrl"), _T("max thumbs limit"), 0);
}

CSCThumbCtrl::~CSCThumbCtrl()
{
	//로딩중에 프로그램을 종료시킬 경우 로딩 쓰레드들을 정상 중지시켜준 후 종료시켜야 한다.
	release_thumb(-1);

	if (m_pEdit)
	{
		m_pEdit->DestroyWindow();
		delete m_pEdit;
	}
}

void CSCThumbCtrl::release_thumb(int index)
{
	int i;
	int start = 0;
	int end = m_thumb.size();

	if (index >= (int)m_thumb.size())
	{
		return;
	}
	else if (index >= 0)
	{
		start = index;
		end = index + 1;
	}

	for (i = start; i < end; i++)
	{
		if (m_thumb[i].img != NULL)
		{
			//delete만 호출해도 CGdiplusBitmap의 소멸자에서 이미 release를 하므로 별도로 img->release()를 호출할 필요가 없다.
			//m_thumb[i].img->release();

			//여기서 m_thumb.erase(m_thumb.begin() + i);를 호출하면 이는 release()가 아닌 remove가 되므로 정리가 필요하다.
			delete m_thumb[i].img;
			m_thumb[i].img = NULL;
		}

		SAFE_DELETE_ARRAY(m_thumb[i].feature);
	}

	if (index < 0)
		m_thumb.clear();
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
	ON_REGISTERED_MESSAGE(Message_CSCEdit, &CSCThumbCtrl::on_message_CSCEdit)
	ON_WM_CONTEXTMENU()
	ON_COMMAND_RANGE(idTotalCount, idProperty, on_context_menu)
END_MESSAGE_MAP()


// CSCThumbCtrl 메시지 처리기

//타일 크기, 마진, 간격, 스크롤, 컨트롤 크기조정 등에 따라 각 썸네일이 표시되는 r이 재계산된다.
//이러한 변화에 대해 재계산하고 Invalidate()까지 수행하므로 OnPaint()에서는 r에 표시만 하면 된다.
void CSCThumbCtrl::recalc_tile_rect()
{
	if (m_thumb.size() == 0)
	{
		m_scroll_pos = 0;
		m_scroll_total = 0;
		return;
	}

	int		i;
	CRect	rc;
	CRect	rTile;

	GetClientRect(rc);

	//thumb크기에 따라 tile 크기는 자동 계산된다.
	m_sz_tile.cx = m_r_inner.left + m_sz_thumb.cx + m_r_inner.right;
	m_sz_tile.cy = m_r_inner.top + m_sz_thumb.cy + m_thumb_title_gap + m_title_height + m_r_inner.bottom;

	//맨 첫번째 항목의 위치를 잡고 인덱스가 증가하면 그 위치를 조정해준다.
	while (true)
	{
		rTile = make_rect(m_sz_margin.cx, m_sz_margin.cy + m_scroll_pos, m_sz_tile.cx, m_sz_tile.cy);

		m_per_line = -1;

		for (i = 0; i < m_thumb.size(); i++)
		{
			m_thumb[i].r = rTile;
			m_thumb[i].thumb_bottom = rTile.top + m_r_inner.top + m_sz_thumb.cy;

			rTile.OffsetRect(rTile.Width() + m_sz_gap.cx, 0);

			//다음 thumb를 표시할 위치가 rc.right를 벗어난다면 다음줄로 내려준다.
			//단, i가 마지막 항목일 경우는 이 체크를 하지 않아야 한다.
			if (i < m_thumb.size() - 1 && (rTile.right + m_sz_margin.cx > rc.right))
			{
				if (m_per_line == -1)
					m_per_line = i + 1;

				rTile.left = m_sz_margin.cx;
				rTile.top = rTile.bottom + m_sz_gap.cy;
				rTile.right = rTile.left + m_sz_tile.cx;
				rTile.bottom = rTile.top + m_sz_tile.cy;
			}
		}

		m_scroll_total = rTile.bottom - m_scroll_pos + m_sz_margin.cy;

		//맨 아래로 스크롤한 후 창 크기를 늘리거나 썸네일 크기를 줄이거나 썸네일을 제거하면
		//하단에 공백이 생기므로 m_scroll_pos를 보정한 후 다시 계산해줘야 한다.
		if ((m_scroll_pos != 0) && (m_scroll_pos < -m_scroll_total + rc.Height()))
		{
			m_scroll_pos = -m_scroll_total + rc.Height();
			if (m_scroll_pos > 0)
				m_scroll_pos = 0;
		}
		else
		{
			break;
		}
	}
	/*
	//여러 thumb들을 삭제할 경우 현재의 m_scroll_pos는 m_scroll_total보다 불필요하게 큰 값인 경우가 있다.
	//이럴 경우는 m_scroll_pos를 0으로 리셋한 후 다시 계산해줘야 한다.
	if (m_scroll_total > rc.Height() || m_scroll_total < abs(m_scroll_pos))
	{
		Clamp(m_scroll_pos, -m_scroll_total + rc.Height(), 0);

		//맨 첫번째 항목의 위치를 잡고 인덱스가 증가하면 그 위치를 조정해준다.
		rTile = make_rect(m_sz_margin.cx, m_sz_margin.cy + m_scroll_pos, m_sz_tile.cx, m_sz_tile.cy);

		m_per_line = -1;

		for (i = 0; i < m_thumb.size(); i++)
		{
			m_thumb[i].r = rTile;
			m_thumb[i].thumb_bottom = rTile.top + m_r_inner.top + m_sz_thumb.cy;

			rTile.OffsetRect(rTile.Width() + m_sz_gap.cx, 0);

			//다음 thumb를 표시할 위치가 rc.right를 벗어난다면 다음줄로 내려준다.
			//단, i가 마지막 항목일 경우는 이 체크를 하지 않아야 한다.
			if (i < m_thumb.size() - 1 && (rTile.right + m_sz_margin.cx > rc.right))
			{
				if (m_per_line == -1)
					m_per_line = i + 1;

				rTile.left = m_sz_margin.cx;
				rTile.top = rTile.bottom + m_sz_gap.cy;
				rTile.right = rTile.left + m_sz_tile.cx;
				rTile.bottom = rTile.top + m_sz_tile.cy;
			}
		}
	}
	*/


	//TRACE(_T("m_scroll_total = %d, m_scroll_pos = %d\n"), m_scroll_total, m_scroll_pos);

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
	dc.SetTextColor(m_theme.cr_text.ToCOLORREF());
	dc.FillSolidRect(rc, m_theme.cr_back.ToCOLORREF());

	CFont* pOldFont = (CFont*)dc.SelectObject(&m_font);

	/*if (m_files.size() > 0 && !m_loading_completed)
	{
		TRACE(_T("loading...\n"));
		CRect r = make_center_rect(rc.CenterPoint().x, rc.CenterPoint().y, rc.Width() / 2, 16);
		draw_sunken_rect(&dc, r, true, Gdiplus::Color::DimGray, Gdiplus::Color::DarkGray);
		r.DeflateRect(1, 1);
		double width = r.Width();
		dc.FillSolidRect(r, green);
		return;
	}
	else */if (m_loading_completed && m_thumb.size() == 0)
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
			//draw_rectangle(g, rThumb, Gdiplus::Color(153, 209, 255), Gdiplus::Color(128, 204, 232, 255));
			draw_rectangle(g, rThumb, m_theme.cr_selected_border, m_theme.cr_back_selected);

		rThumb.DeflateRect(m_r_inner);
		rThumb.bottom -= (m_thumb_title_gap + m_title_height);

		if (m_thumb[i].load_completed == false)
		{
			DrawShadowText(dc.GetSafeHdc(), _T("loading..."), -1, rThumb,
				DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP, m_theme.cr_text.ToCOLORREF(), 0, 2, 1);
		}
		else if (m_thumb[i].img == NULL || m_thumb[i].img->is_empty())
		{
			DrawShadowText(dc.GetSafeHdc(), _T("X"), -1, rThumb,
				DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP, RGB(255, 0, 0), 0, 2, 1);
		}
		else
		{
			CRect rImage = get_ratio_rect(rThumb, m_thumb[i].img->width, m_thumb[i].img->height, DT_BOTTOM);
			g.DrawImage(*m_thumb[i].img, CRect2GpRect(rImage));
		}

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

#include <mutex>
std::mutex mtx;
CSCThumbCtrl* pThisWnd;	//add_files() 함수에서 할당됨

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
		if (pThisWnd->m_stop_loading)
			break;

		pThisWnd->insert(i, pThisWnd->m_files[i], get_part(pThisWnd->m_files[i], fn_name), false, false);
		trace(i);
	}

	if ((end - start) == 1)
		str.Format(_T("job completed : %d"), start);
	else
		str.Format(_T("job completed : %d ~ %d"), start, end - 1);
	TRACE(_T("%s\n"), str);

	mtx.lock();
	pThisWnd->m_thread.set_thread_completed(idx);
	mtx.unlock();
}

void CSCThumbCtrl::loading_completed_callback()
{
	pThisWnd->on_loading_completed();
}

void CSCThumbCtrl::on_loading_completed()
{
	m_tloading_end = clock();
	TRACE(_T("total loading completed. elapsed = %ld ms\n"), m_tloading_end - m_tloading_start);

	m_loading_completed = true;
	recalc_tile_rect();

	::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCThumbCtrl,
		(WPARAM)&CSCThumbCtrlMessage(this, CSCThumbCtrl::message_thumb_loading_completed, 0), 0);
}

void CSCThumbCtrl::set_path(CString path)
{
	stop_loading();

	m_files.clear();
	m_files = find_all_files(path, _T(""), FILE_EXTENSION_IMAGE, _T(""), false);

	add_files(m_files);
}

void CSCThumbCtrl::add_files(std::deque<CString> files, bool reset)
{
	pThisWnd = this;

	m_tloading_start = clock();

	if (reset)
	{
		release_thumb(-1);

		m_files.clear();
		m_selected.clear();
		m_files.assign(files.begin(), files.end());
	}
	else
	{
		m_files.insert(m_files.end(), files.begin(), files.end());
	}

	if (files.size() == 0)
	{
		m_loading_completed = true;
		::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCThumbCtrl,
			(WPARAM)&CSCThumbCtrlMessage(this, message_thumb_loading_completed, 0), 0);
		Invalidate();
		return;
	}

	m_thumb.resize(m_files.size());

	m_thread.job(m_files.size(), loading_function, loading_completed_callback);

	Invalidate();
}

int CSCThumbCtrl::insert(int index, CString full_path, CString title, bool key_thumb, bool invalidate)
{
	m_thumb[index].img = new CGdiplusBitmap();
	m_thumb[index].img->load(full_path);
	m_thumb[index].width = m_thumb[index].img->width;
	m_thumb[index].height = m_thumb[index].img->height;
	m_thumb[index].channel = m_thumb[index].img->channel;

	//현재 m_sz_thumb가 작을 경우 이 크기로 resize해서 불러오면 나중에 m_sz_thumb를 키웠을 때 흐릿해진다.
	//MAX_TILE_SIZE 크기로 축소해야 한다.
	CRect r = get_ratio_rect(CRect(0, 0, MAX_TILE_SIZE, MAX_TILE_SIZE), m_thumb[index].img->width, m_thumb[index].img->height);

	m_thumb[index].img->resize(r.Width(), r.Height());
	m_thumb[index].title = title;
	m_thumb[index].full_path = full_path;
	m_thumb[index].score = 0.0;
	m_thumb[index].feature = NULL;

	m_thumb[index].load_completed = true;

	if (invalidate)
	{
		recalc_tile_rect();
		//Invalidate();
	}

	return index;
}

BOOL CSCThumbCtrl::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	switch (pMsg->message)
	{
		case WM_SYSKEYDOWN:
		case WM_MBUTTONDOWN:
			return FALSE;
	}

	if (pMsg->message == WM_KEYDOWN)
	{
		//if (!IsWindowVisible())
		//	return false;

		switch (pMsg->wParam)
		{
			case VK_RETURN:
			{
				if (m_in_editing)
				{
					edit_end(true);
					return TRUE;
				}
				return false;
			}
			case VK_ESCAPE:
			{
				if (m_in_editing)
				{
					edit_end(false);
					return TRUE;
				}
				return false;
			}
			case VK_F2:
				if (m_in_editing)
					return true;
				edit_begin(-1);
				return true;
			case VK_LEFT:
			case VK_RIGHT:
			case VK_UP:
			case VK_DOWN:
			case VK_HOME:
			case VK_END:
			case VK_PRIOR:
			case VK_NEXT:
				if (m_in_editing)
					return false;
				//{
				//	::SendMessage(m_pEdit->m_hWnd, pMsg->message, pMsg->wParam, pMsg->lParam);
				//	return true;
				//}
				else
					on_key_down(pMsg->wParam);
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

	if (is_editing())
		edit_end();

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
	if (is_editing())
		edit_end();

	if (IsCtrlPressed())
	{
		enlarge_size(zDelta > 0);
		return TRUE;
	}
	else
	{
		scroll_up(zDelta > 0);
		return TRUE;
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
	pos += (up ? 1 : -1) * (m_sz_tile.cy + m_sz_gap.cy) / 4.0;

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

	m_sz_thumb.cx = AfxGetApp()->GetProfileInt(_T("setting\\thumbctrl"), _T("thumb size cx"), 100);
	m_sz_thumb.cy = AfxGetApp()->GetProfileInt(_T("setting\\thumbctrl"), _T("thumb size cy"), 120);

	CDialogEx::PreSubclassWindow();
}

void CSCThumbCtrl::reconstruct_font()
{
	m_lf.lfQuality = CLEARTYPE_NATURAL_QUALITY;

	m_font.DeleteObject();
	m_font.CreateFontIndirect(&m_lf);

	SetFont(&m_font, true);

	CClientDC dc(this);
	CFont* pOldFont = dc.SelectObject(&m_font);
	CRect textRect;

	dc.DrawText(_T("Title Sample"), textRect, DT_CALCRECT/*| DT_CENTER | DT_WORDBREAK | DT_EDITCONTROL*/);
	m_title_height = textRect.Height();
	recalc_tile_rect();
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
							(WPARAM)&CSCThumbCtrlMessage(this, message_thumb_lbutton_unselected, selected_index), 0);
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
								(WPARAM)&CSCThumbCtrlMessage(this, message_thumb_lbutton_selected, i), 0);
						}
					}
					else
					{
						m_selected.push_back(i);
						::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCThumbCtrl,
							(WPARAM)&CSCThumbCtrlMessage(this, message_thumb_lbutton_selected, i), 0);
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
						(WPARAM)&CSCThumbCtrlMessage(this, message_thumb_lbutton_unselected, selected_index), 0);
				}
				else
				{
					if (find_index(m_selected, i) < 0)
						m_selected.push_back(i);
					::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCThumbCtrl,
						(WPARAM)&CSCThumbCtrlMessage(this, message_thumb_lbutton_selected, i), 0);
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
	if (m_selected.size())
	{
		//dbclick 했을때의 액션은 parent에서 처리한다.
		::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCThumbCtrl,
			(WPARAM)&CSCThumbCtrlMessage(this, message_thumb_lbutton_dbclicked, m_selected[0]), 0);
	}

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
	r.top = m_thumb[index].thumb_bottom + m_thumb_title_gap;
	r.bottom = r.top + m_title_height;

	if (m_pEdit == NULL)
	{
		m_pEdit = new CSCEdit();
		m_pEdit->Create(dwStyle, r, this, IDC_EDIT_CELL);
		m_pEdit->SetFont(&m_font);
		m_pEdit->set_line_align(DT_TOP);
	}

	CClientDC dc(this);
	CFont* pOldFont = dc.SelectObject(&m_font);
	CRect textRect = r;

	//1 line이라 가정하고 height만 구한다.
	dc.DrawText(_T("for height"), textRect, DT_CALCRECT);// | DT_CENTER | DT_WORDBREAK | DT_EDITCONTROL);
	dc.SelectObject(&pOldFont);

	m_in_editing = true;
	m_editing_index = index;
	m_old_title = m_thumb[index].title;

	m_pEdit->SetWindowText(m_thumb[index].title);
	int lc = m_pEdit->GetLineCount();
	r.bottom = r.top + lc * textRect.Height() + 4;

	m_pEdit->set_text_color(m_theme.cr_text);
	m_pEdit->set_back_color(m_theme.cr_back);
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
			(WPARAM)&CSCThumbCtrlMessage(this, message_thumb_rename, m_editing_index), 0);
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

//선택된 항목들은 이미 m_selected에 저장되어 있으므로 이를 dqSelected로 복사한다.
//dqSelected가 null이면 그냥 선택 갯수를 리턴한다.
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

void CSCThumbCtrl::select_item(CString fullpath)
{
	int i;

	for (i = 0; i < m_thumb.size(); i++)
	{
		if (find(m_thumb[i].full_path, fullpath) >= 0)
		{
			select_item(i);
			return;
		}
	}
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
		int res = CWin32InputBox::InputBox(_T("검색어 입력"), msg, buf, 200, false);

		if (res == IDCANCEL)
			return dqFound;

		text = buf;
	}


	for (i = 0; i < m_thumb.size(); i++)
	{
		if (find(m_thumb[i].title, text) >= 0)
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

void CSCThumbCtrl::set_font_name(LPCTSTR sFontname, BYTE byCharSet)
{
	m_lf.lfCharSet = byCharSet;
	_tcscpy_s(m_lf.lfFaceName, _countof(m_lf.lfFaceName), sFontname);
	reconstruct_font();
}

void CSCThumbCtrl::set_font_size(int nSize)
{
	//For the MM_TEXT mapping mode,
	//you can use the following formula to specify 
	//a height for a font with a specified point size:
	//m_lf.lfHeight = -MulDiv(nSize, GetDeviceCaps(::GetDC(GetParent()->GetSafeHwnd()), LOGPIXELSY), 72);
	m_lf.lfHeight = get_pixel_size_from_font_size(m_hWnd, nSize);
	reconstruct_font();
}

void CSCThumbCtrl::set_font_bold(bool bBold)
{
	m_lf.lfWeight = (bBold ? FW_BOLD : FW_NORMAL);
	reconstruct_font();
}

LRESULT CSCThumbCtrl::on_message_CSCEdit(WPARAM wParam, LPARAM lParam)
{
	CSCEdit* pEdit = (CSCEdit*)wParam;
	int	msg = (int)lParam;

	if (!pEdit->IsWindowVisible())
		return 0;

	TRACE(_T("message(%d) from CSCEdit(%p)\n"), (int)lParam, pEdit);
	if (msg == WM_KILLFOCUS)
		edit_end();

	Invalidate();

	return 0;
}

void CSCThumbCtrl::set_title(int index, CString title)
{
	if (index < 0 || index > m_thumb.size() - 1)
		return;
	m_thumb[index].title = title;
	Invalidate();
}

void CSCThumbCtrl::sort_by_title()
{
	std::sort(m_thumb.begin(), m_thumb.end(),
		[](CThumbImage& a, CThumbImage& b)
		{
			//CThumbImage의 img가 참조생성자에 의해 a로 생성되고 사용된 후
			//CGdiplusBitmap()의 소멸자에서 release()에 의해 메모리에서 제거되려 하므로 오류가 발생한다.
			//참조에 의해 생성된 a의 img 역시 참조임을 명시하고 release()시키지 않아야 한다.
			a.img->m_referenced_variable = true;
			b.img->m_referenced_variable = true;

			CString str0 = a.title;
			CString str1 = b.title;
			return !is_greater_with_numeric(str0, str1);
		}
	);

	//sort되면 각 thumb.r도 변경되므로 recalc_tile_rect()해야 한다.
	recalc_tile_rect();
}

//info text를 기준으로 리스트를 정렬시킨다.
void CSCThumbCtrl::sort_by_info(int idx)
{
	std::sort(m_thumb.begin(), m_thumb.end(),
		[idx](CThumbImage& a, CThumbImage& b)
		{
			//CThumbImage의 img가 참조생성자에 의해 a로 생성되고 사용된 후
			//CGdiplusBitmap()의 소멸자에서 release()에 의해 메모리에서 제거되려 하므로 오류가 발생한다.
			//참조에 의해 생성된 a의 img 역시 참조임을 명시하고 release()시키지 않아야 한다.
			a.img->m_referenced_variable = true;
			b.img->m_referenced_variable = true;
			return (a.info[idx] > b.info[idx]);
		}
	);

	//sort되면 각 thumb.r도 변경되므로 recalc_tile_rect()해야 한다.
	recalc_tile_rect();
}

void CSCThumbCtrl::sort_by_score()
{
	std::sort(m_thumb.begin(), m_thumb.end(),
		[](CThumbImage& a, CThumbImage& b)
		{
			//CThumbImage의 img가 참조생성자에 의해 a로 생성되고 사용된 후
			//CGdiplusBitmap()의 소멸자에서 release()에 의해 메모리에서 제거되려 하므로 오류가 발생한다.
			//참조에 의해 생성된 a의 img 역시 참조임을 명시하고 release()시키지 않아야 한다.
			a.img->m_referenced_variable = true;
			b.img->m_referenced_variable = true;
			return (a.score > b.score);
		}
	);

	//sort되면 각 thumb.r도 변경되므로 recalc_tile_rect()해야 한다.
	recalc_tile_rect();
}

int	 CSCThumbCtrl::find_by_title(CString title, bool bWholeWord)
{
	if (title.IsEmpty())
		return -1;

	for (int i = 0; i < m_thumb.size(); i++)
	{
		if (bWholeWord)
		{
			if (m_thumb[i].title == title)
				return i;
		}
		else
		{
			if (m_thumb[i].title.Find(title) >= 0)
				return i;
		}
	}

	return -1;
}

void CSCThumbCtrl::set_thumb_size(CSize sz_thumb)
{
	m_sz_thumb = sz_thumb;
	recalc_tile_rect();

	AfxGetApp()->WriteProfileInt(_T("setting\\thumbctrl"), _T("thumb size cx"), m_sz_thumb.cx);
	AfxGetApp()->WriteProfileInt(_T("setting\\thumbctrl"), _T("thumb size cy"), m_sz_thumb.cy);
}

void CSCThumbCtrl::on_key_down(int key)
{
	if (m_thumb.size() == 0)
		return;

	int selected = get_selected_item();
	if (selected < 0)
	{
		select_item(0);
		return;
	}

	if (key == VK_LEFT && selected > 0)
	{
		selected--;
	}
	else if (key == VK_RIGHT && selected < m_thumb.size() - 1)
	{
		selected++;
	}
	else if (key == VK_UP && selected >= m_per_line)
	{
		selected -= m_per_line;
	}
	else if (key == VK_DOWN && selected <= m_thumb.size() - 1 - m_per_line)
	{
		selected += m_per_line;
		//if (selected > m_dqThumb.size() - 1)
		//	selected = m_dqThumb.size() - 1;
	}
	else if (key == VK_HOME && selected > 0)
	{
		selected = 0;
	}
	else if (key == VK_END && selected < m_thumb.size() - 1)
	{
		selected = m_thumb.size() - 1;
	}
	else if (key == VK_PRIOR && selected >= m_per_line * 3)
	{
		selected -= m_per_line * 3;
		//if (selected < 0)
		//	selected = 0;
	}
	else if (key == VK_NEXT && selected <= m_thumb.size() - 1 - m_per_line * 3)
	{
		selected += m_per_line * 3;
		//if (selected > m_dqThumb.size() - 1)
		//	selected = m_dqThumb.size() - 1;
	}

	select_item(selected);
}

void CSCThumbCtrl::set_info_text(int thumb_index, int idx, CString info, bool refresh)
{
	if (thumb_index < 0 || thumb_index >= m_thumb.size())
		return;

	if (idx < 0 || idx >= 4)
		return;

	m_show_info_text[idx] = true;
	m_thumb[thumb_index].info[idx] = info;

	if (refresh)
		Invalidate();
}

//point는 screen coord.
void CSCThumbCtrl::OnContextMenu(CWnd* pWnd, CPoint point)
{
	if (get_selected_items() <= 1)
	{
		CPoint pt = point;
		ScreenToClient(&pt);

		int index = get_index_from_point(pt);
		if (index >= 0)
			select_item(index);
	}

	int selected_count = get_selected_items();

	CMenu	menu;
	menu.CreatePopupMenu();

	menu.AppendMenu(MF_STRING, idTotalCount, _T("총 ") + i2S(m_thumb.size()) + _T("개의 썸네일"));
	menu.EnableMenuItem(idTotalCount, MF_GRAYED);
	menu.AppendMenu(MF_SEPARATOR);

	menu.AppendMenu(MF_STRING, idFind, _T("찾기...(&F)"));
	menu.AppendMenu(MF_STRING, idReload, _T("새로 고침(&R)"));
	menu.AppendMenu(MF_STRING, idReloadSelected, _T("선택 항목만 새로 고침"));
	menu.AppendMenu(MF_STRING, idSortByTitle, _T("파일이름으로 정렬(&S)"));
	menu.AppendMenu(MF_SEPARATOR);

	menu.AppendMenu(MF_STRING, idCopyToClipboard, _T("복사(&C)"));
	menu.AppendMenu(MF_SEPARATOR);

	menu.AppendMenu(MF_STRING, idToggleIndex, _T("인덱스 표시"));
	menu.AppendMenu(MF_STRING, idToggleTitle, _T("파일이름 표시"));
	//menu.AppendMenu(MF_STRING, idToggleResolution, _T("해상도 정보 표시"));
	//menu.AppendMenu(MF_STRING, idPromptMaxThumb, _T("Set max thumbnails(&A)(0 : no limits)..."));
	menu.AppendMenu(MF_SEPARATOR);

	menu.AppendMenu(MF_STRING, idDeleteThumb, _T("선택 항목을 리스트에서 삭제(&D)"));
	//menu.AppendMenu(MF_SEPARATOR);
	//menu.AppendMenu(MF_STRING, idRemoveAll, _T("모든 썸네일 삭제...(&R)"));
	menu.AppendMenu(MF_SEPARATOR);
	menu.AppendMenu(MF_STRING, idProperty, _T("속성(&R)"));

	menu.EnableMenuItem(idFind, m_thumb.size() ? MF_ENABLED : MF_DISABLED);
	menu.EnableMenuItem(idReloadSelected, selected_count ? MF_ENABLED : MF_DISABLED);
	menu.EnableMenuItem(idSortByTitle, m_thumb.size() ? MF_ENABLED : MF_DISABLED);
	menu.EnableMenuItem(idCopyToClipboard, selected_count ? MF_ENABLED : MF_DISABLED);
	menu.EnableMenuItem(idDeleteThumb, selected_count ? MF_ENABLED : MF_DISABLED);
	menu.EnableMenuItem(idProperty, selected_count ? MF_ENABLED : MF_DISABLED);

	menu.CheckMenuItem(idToggleIndex, m_show_index ? MF_CHECKED : MF_UNCHECKED);
	menu.CheckMenuItem(idToggleTitle, m_show_title ? MF_CHECKED : MF_UNCHECKED);
	menu.CheckMenuItem(idToggleResolution, m_show_resolution ? MF_CHECKED : MF_UNCHECKED);

	menu.TrackPopupMenu(TPM_LEFTALIGN, point.x, point.y, this);

	menu.DestroyMenu();
}

void CSCThumbCtrl::on_context_menu(UINT nMenuID)
{
	switch (nMenuID)
	{
		case idToggleIndex:
			m_show_index = !m_show_index;
			AfxGetApp()->WriteProfileInt(_T("setting\\thumbctrl"), _T("show index"), m_show_index);
			Invalidate();
			break;

		case idToggleTitle:
			m_show_title = !m_show_title;
			AfxGetApp()->WriteProfileInt(_T("setting\\thumbctrl"), _T("show title"), m_show_title);
			Invalidate();
			break;

		case idToggleResolution:
			m_show_resolution = !m_show_resolution;
			AfxGetApp()->WriteProfileInt(_T("setting\\thumbctrl"), _T("show resolution"), m_show_resolution);
			Invalidate();
			break;

		case idFind:
			find_text(true, true);
			break;

		//loading은 main에서 호출하므로 메인에게 메시지만 전달한다.
		case idReload:
			::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCThumbCtrl,
				(WPARAM)&CSCThumbCtrlMessage(this, message_thumb_reload, 0), 0);
			break;
		case idReloadSelected:
		{
			int index = get_selected_item();
			if (index < 0 || index >= m_thumb.size())
				return;
			m_thumb[index].reload();
			InvalidateRect(m_thumb[index].r);
			::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCThumbCtrl,
				(WPARAM)&CSCThumbCtrlMessage(this, message_thumb_reload_selected, 0), 0);
		}
		break;

		case idSortByTitle:
			sort_by_title();
			break;
		case idCopyToClipboard:
			if (m_thumb.size() == 0)
				break;
			{
				int index = get_selected_item();
				if (index >= 0)
				{
					CGdiplusBitmap img(m_thumb[index].full_path);
					img.copy_to_clipbard();
				}
			}
			break;
		case idDeleteThumb:
			on_menu_delete();
			break;

		case idPromptMaxThumb:
		{
			TCHAR buf[200] = { 0 };
			TCHAR msg[200] = { 0 };

			_stprintf_s(msg, _countof(msg), _T("표시할 썸네일의 최대 개수를 입력하세요(0:제한없음).\n현재 설정값 = %d"), m_max_thumbs);
			int res = CWin32InputBox::InputBox(_T("썸네일 최대 개수"), msg, buf, 200, false);

			if (res == IDCANCEL)
				return;

			int n = _ttoi(buf);

			if (n <= 0)
				return;

			m_max_thumbs = n;

			//썸네일에 표시할 최대 개수가 넘으면 맨 마지막 썸네일을 지우고 추가해준다.
			if (m_max_thumbs && (m_thumb.size() >= m_max_thumbs))
			{
				while (m_thumb.size() > m_max_thumbs)
					m_thumb.pop_front();

				Invalidate();
			}

			AfxGetApp()->WriteProfileInt(_T("setting\\thumbctrl"), _T("max thumbs limit"), m_max_thumbs);
		}
		break;
		case idProperty:
			show_property_window(std::deque<CString> { m_thumb[get_selected_item()].full_path });
			break;
		break;
	}
}

void CSCThumbCtrl::on_menu_delete()
{
	int count = get_selected_items();
	CString str;

	if (count == 1)
		str.Format(_T("선택된 항목을 삭제합니다."));
	else if (count > 1)
		str.Format(_T("선택된 %d개의 썸네일을 삭제합니다."), count);

	if (AfxMessageBox(str, MB_OKCANCEL) == IDCANCEL)
		return;

	remove_selected(true);
}

//index == -1이면 전체 삭제
//이 함수는 파일삭제인듯한데 뭔가 코드 미완성된듯하므로 우선 스킵한다.
void CSCThumbCtrl::remove(int index, bool refresh)
{
	if (index < 0)
	{
		//실제적인 삭제는 여기서 이루어지지만 삭제를 묻는 질문은 메인에서 해야 한다.
		int res = 0;
		::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCThumbCtrl,
			(WPARAM)&CSCThumbCtrlMessage(this, message_thumb_remove, index), (LPARAM)&res);

		if (res == 0)
			return;

		release_thumb(-1);
		m_files.clear();
		m_selected.clear();
	}
	else
	{
		//if (m_thumb[index].full_path.IsEmpty() == false)
		//	DeleteFile(m_thumb[index].full_path);
		release_thumb(index);
		m_thumb.erase(m_thumb.begin() + index);
		m_files.erase(m_files.begin() + index);
	}


	if (refresh)
		recalc_tile_rect();
}

void CSCThumbCtrl::remove_selected(bool refresh)
{
	int i;
	std::deque<int> selected;

	//get_selected_items(&selected);
	//if (selected.size() == 0)
	//	return;

	////실제적인 삭제는 여기서 이루어지지만 삭제를 묻는 질문은 메인에서 해야 한다.
	//int res = 0;
	//::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCThumbCtrl,
	//	(WPARAM)&CSCThumbCtrlMessage(this, message_thumb_remove_selected, selected.size()), (LPARAM)&res);

	//if (res == 0)
	//	return;

	for (i = m_selected.size() - 1; i >= 0; i--)
	{
		remove(m_selected[i], false);
	}

	m_selected.clear();
	recalc_tile_rect();
}

CGdiplusBitmap* CSCThumbCtrl::get_img(int index)
{
	if (index < 0 || index >= m_thumb.size())
		return NULL;

	return m_thumb[index].img;
}

void CSCThumbCtrl::stop_loading()
{
	//if (!m_loading_completed)
	{
		m_stop_loading = true;

		while (!m_thread.is_all_thread_completed())
		{
			Wait(10);
		}

		m_stop_loading = false;
	}
}
