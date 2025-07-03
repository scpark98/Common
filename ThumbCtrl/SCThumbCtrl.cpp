// CSCThumbCtrl.cpp: 구현 파일
//

#include "SCThumbCtrl.h"
#include "../MemoryDC.h"
#include "../Functions.h"

// CSCThumbCtrl 대화 상자

IMPLEMENT_DYNAMIC(CSCThumbCtrl, CDialogEx)

CSCThumbCtrl::CSCThumbCtrl()
{

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
END_MESSAGE_MAP()


// CSCThumbCtrl 메시지 처리기

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

	dc.FillSolidRect(rc, white);

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

		return;
	}

	CRect	rItem(m_szMargin.cx, m_szMargin.cy + m_scroll_pos, m_szMargin.cx + m_szTile.cx, m_szMargin.cy + m_szTile.cy + m_scroll_pos);

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

		rItem.left += (m_szGap.cx + m_szTile.cx);
		rItem.right = rItem.left + m_szTile.cx;

		if ((rItem.right + m_szMargin.cx > rc.right + 2) /* || (nLineCount >= m_per_line)*/)
		{
			//m_per_line = dqOneLineHeight.size();

			//std::deque<int>::iterator result = std::max_element(dqOneLineHeight.begin(), dqOneLineHeight.end());
			rItem.left = m_szMargin.cx;
			rItem.top = rItem.bottom + m_szGap.cy;// +result[0];
			rItem.right = rItem.left + m_szTile.cx;
			rItem.bottom = rItem.top + m_szTile.cy;

			//m_line_height.push_back(result[0]);
			//dqOneLineHeight.clear();
			//nLineCount++;
		}
	}

	m_scroll_total = rItem.bottom + 20;
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

	m_thumb[index].img.resize(100, 100);
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

		pThisWnd->insert(i, pThisWnd->m_files[i], get_part(pThisWnd->m_files[i], fn_title), false, false);
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
		return FALSE;

	return CDialogEx::PreTranslateMessage(pMsg);
}

void CSCThumbCtrl::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	if (!m_hWnd)
		return;

	Invalidate();
}


void CSCThumbCtrl::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);
}

BOOL CSCThumbCtrl::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	scroll_up(zDelta > 0);

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
	//m_cr_scroll = m_cr_title;
	Invalidate(false);

	CPoint pt;
	GetCursorPos(&pt);
	ScreenToClient(&pt);

	//커서가 스크롤바 영역 밖이라면 스크롤바를 숨겨준다.
	//if ((rc.PtInRect(pt) == false) || !m_rScroll.PtInRect(pt))
	//	SetTimer(timer_scroll_bar_disappear, 2000, NULL);
}

void CSCThumbCtrl::recalculate_scroll_size()
{
	TRACE(_T("%s\n"), __function__);

	CRect	rc;
	GetClientRect(rc);

	long t0 = clock();
	//draw_function(NULL, false);
	TRACE(_T("draw_function(NULL, false) = %ld\n"), clock() - t0);

	if (m_thumb.size() == 0)
		return;

	GetClientRect(rc);
	m_rScroll = CRect(rc.right - ::GetSystemMetrics(SM_CXVSCROLL), rc.top, rc.right + 1, rc.bottom);

	//맨 아래로 스크롤한 후 썸네일 크기를 줄이면 스크롤바가 잘못 표시되는 버그 있음!!
	Clamp(m_scroll_pos, -m_scroll_total + rc.Height(), 0);


	/*
	//Gap의 너비는 균등하게 자동 조절하자.
	if (m_per_line > 1)
	{
		while (true)
		{
			int rest = (rc.Width() - m_szMargin.cx * 2 - m_per_line * m_szTile.cx - m_szGap.cx * (m_per_line - 1));
			TRACE(_T("rest = %d, m_szGap.cx = %d\n"), rest, m_szGap.cx);
			if ((m_szGap.cx * (m_per_line - 1) > m_szTile.cx) || (rest > m_szTile.cx))
			{
				m_per_line++;
				m_szGap.cx = (rc.Width() - m_szMargin.cx * 2 - m_per_line * m_szTile.cx) / (m_per_line - 1);
				if (m_szGap.cx <= 20)
				{
					m_per_line--;
					break;
				}
			}
			else if (m_szGap.cx <= 20)
			{
				m_per_line--;
				if (m_per_line == 1)
					break;
			}
			else
			{
				break;
			}
		}
	}

	if (m_per_line > 1)
		m_szGap.cx = (rc.Width() - m_szMargin.cx * 2 - m_per_line * m_szTile.cx) / (m_per_line - 1);
	TRACE( _T("tile.cx = %d, perLine = %d, maxLine = %d\n"), m_szTile.cx, m_per_line, m_max_line );
	*/
	//최근 윈도우의 스크롤바는 그 범위 크기에 따라 thumb의 크기가 자동 변경되는데
	//그래야만 thumb이 크게 표시되어 드래그하기 편해지기 떄문이다.
	//이렇게 구현하기 위해서는 nPage의 값을 적정값으로 고정하고
	//nMax = realMax + si.nPage - 1;로 주면 원하는 범위로 동작된다.
	//물론 정밀한 이동이 필요한 경우는 그 값을 작게 줘야 정밀하게 스크롤할 수 있다.
	/*
	SCROLLINFO	si;
	si.cbSize = sizeof( SCROLLINFO );

	memset( &si, 0, sizeof(SCROLLINFO) );
	GetScrollInfo( SB_VERT, &si );
	si.fMask = SIF_RANGE | SIF_PAGE;
	si.nPage = 1080 + 1;
	*/
	int i;
	m_scroll_total = m_szMargin.cy;

	for (i = 0; i < m_line_height.size(); i++)
		m_scroll_total = m_scroll_total + (m_szTile.cy + m_line_height[i] + m_szGap.cy);
	/*
	if ( totalScrollHeight > rc.Height() )
		//SetScrollRange( SB_VERT, 0, totalScrollHeight - rc.Height() );
		si.nMax = totalScrollHeight - rc.Height() + si.nPage - 1;
	else
		//SetScrollRange( SB_VERT, 0, 0 );
		si.nMax = 0;

	SetScrollInfo( SB_VERT, &si );
	*/
}

void CSCThumbCtrl::scroll_up(bool up)
{
	CRect rc;
	GetClientRect(rc);

	if (rc.Height() >= m_scroll_total)
		return;

	int pos = m_scroll_pos;

	if (up)
		pos += (m_szTile.cy + m_szGap.cy);
	else
		pos -= (m_szTile.cy + m_szGap.cy);

	/*
	if (up)
		pos += ((m_szTile.cy + 20 + m_szGap.cy) >> 2) * 3;
	else
		pos -= ((m_szTile.cy + 20 + m_szGap.cy) >> 2) * 3;
	*/

	set_scroll_pos(pos);
}
