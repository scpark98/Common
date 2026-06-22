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

	//info text default 를 theme.cr_text 로 — light/dark theme 자동 적응.
	for (int i = 0; i < INFO_TEXT_COUNT; i++)
	{
		m_show_info_text[i] = false;
		m_cr_info_text[i] = get_weak_color(m_theme.cr_text, 64);
	}
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
			//delete만 호출해도 CSCGdiplusBitmap의 소멸자에서 이미 release를 하므로 별도로 img->release()를 호출할 필요가 없다.
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
	//WS_CLIPCHILDREN — 자식 (m_scrollbar) 영역 제외하고 부모 paint. 안 하면 부모 FillSolidRect 위에
	//자식이 다시 paint 해 같은 영역에 2 회 그리기 → flicker.
	DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN;

	WNDCLASS wc = {};
	::GetClassInfo(AfxGetInstanceHandle(), _T("#32770"), &wc);
	wc.lpszClassName = _T("CSCThumbCtrl");
	//#32770 (dialog) 의 default class style 에는 CS_DBLCLKS 가 없어 추가 안 하면 WM_LBUTTONDBLCLK 가 dispatch 안 됨 → OnLButtonDblClk 호출 X.
	wc.style |= CS_DBLCLKS;
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

	//자체 스크롤바 — m_scroll_total > viewport 일 때만 가시화. 기본 hide 로 시작.
	m_scrollbar.create(this, CSCScrollbar::vertical, rc.right - m_scrollbar.get_width(), 0, m_scrollbar.get_width(), rc.Height());
	m_scrollbar.set_color_theme(m_theme, false);
	m_scrollbar.ShowWindow(SW_HIDE);

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
	ON_REGISTERED_MESSAGE(Message_CSCScrollbar, &CSCThumbCtrl::on_message_CSCScrollbar)
	ON_WM_CONTEXTMENU()
	ON_COMMAND_RANGE(idTotalCount, idProperty, on_context_menu)
	ON_WM_NCPAINT()
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
		if (::IsWindow(m_scrollbar.m_hWnd))
		{
			m_scrollbar.ShowWindow(SW_HIDE);
			m_scrollbar_visible = false;
		}

		Invalidate();
		return;
	}

	int		i;
	CRect	rc;
	CRect	rTile;

	GetClientRect(rc);

	//thumb크기에 따라 tile 크기는 자동 계산된다.
	m_sz_tile.cx = m_r_inner.left + m_sz_thumb.cx + m_r_inner.right;
	m_sz_tile.cy = m_r_inner.top + m_sz_thumb.cy + m_thumb_title_gap + m_title_height + m_r_inner.bottom;

	//스크롤바 visibility 가 column wrap 경계를 바꾸므로 visibility 변동 감지 시 1 회 더 iterate.
	//iter cap 으로 무한 루프 방지.
	int max_iter = 4;

	while (max_iter-- > 0)
	{
		int reserve = m_scrollbar_visible ? m_scrollbar.get_width() : 0;
		int rc_right = rc.right - reserve;

		rTile = make_rect(m_sz_margin.cx, m_sz_margin.cy + m_scroll_pos, m_sz_tile.cx, m_sz_tile.cy);

		m_per_line = -1;

		for (i = 0; i < m_thumb.size(); i++)
		{
			m_thumb[i].r = rTile;
			m_thumb[i].thumb_bottom = rTile.top + m_r_inner.top + m_sz_thumb.cy;

			rTile.OffsetRect(rTile.Width() + m_sz_gap.cx, 0);

			//다음 thumb를 표시할 위치가 가용 영역 (rc_right) 을 벗어난다면 다음줄로 내려준다.
			//단, i가 마지막 항목일 경우는 이 체크를 하지 않아야 한다.
			if (i < m_thumb.size() - 1 && (rTile.right + m_sz_margin.cx > rc_right))
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

		//visibility 갱신 — total > viewport 면 visible.
		bool need_visible = (m_scroll_total > rc.Height());
		if (need_visible != m_scrollbar_visible)
		{
			m_scrollbar_visible = need_visible;
			continue;	//width 가 변했으니 layout 다시.
		}

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

	//스크롤바 위치/크기 + 모델 sync. m_scroll_pos 는 0 또는 음수 (top=0) — scrollbar 모델은 양수 (top=0) 라 부호 반전.
	//각 setter 가 변동 없으면 no-op. 두께(width)는 scrollbar 가 hover 확장 자체 관리 — host 는 우측 edge 정렬 + 수직 영역만 갱신.
	if (::IsWindow(m_scrollbar.m_hWnd))
	{
		CRect rCur;
		m_scrollbar.GetWindowRect(rCur);
		ScreenToClient(rCur);
		int cur_width = rCur.Width();
		CRect rTarget(rc.right - cur_width, 0, rc.right, rc.Height());
		if (rCur != rTarget)
			m_scrollbar.MoveWindow(rTarget);

		m_scrollbar.set_range(0, m_scroll_total);
		m_scrollbar.set_page(rc.Height());
		m_scrollbar.set_line(m_sz_tile.cy / 4 + m_sz_gap.cy);
		m_scrollbar.set_pos(-m_scroll_pos);

		bool cur_visible = m_scrollbar.IsWindowVisible() != FALSE;
		if (cur_visible != m_scrollbar_visible)
			m_scrollbar.ShowWindow(m_scrollbar_visible ? SW_SHOW : SW_HIDE);
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
	//g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

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
		draw_rect(&dc, rc, m_draw_border ? m_theme.cr_border_inactive : Gdiplus::Color::Transparent, m_theme.cr_back);

		CString str = _T("표시할 이미지가 없습니다.");

		if (m_path.IsEmpty() == false)
			str.Format(_T("%s\n위 경로에는 %s"), m_path, str);

		dc.SetTextColor(m_theme.cr_text.ToCOLORREF());
		draw_center_text(&dc, str, rc);
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
			//draw_rect(g, rThumb, Gdiplus::Color(153, 209, 255), Gdiplus::Color(128, 204, 232, 255));
			draw_rect(g, rThumb, m_theme.cr_selected_border, m_theme.cr_back_selected);

		rThumb.DeflateRect(m_r_inner);
		rThumb.bottom -= (m_thumb_title_gap + m_title_height);

		if (m_thumb[i].load_completed == false)
		{
			DrawShadowText(dc.GetSafeHdc(), CStringW("loading..."), -1, rThumb,
				DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP, m_theme.cr_text.ToCOLORREF(), 0, 2, 1);
		}
		else if (m_thumb[i].img == NULL || m_thumb[i].img->is_empty())
		{
			DrawShadowText(dc.GetSafeHdc(), CStringW("X"), -1, rThumb,
				DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP, RGB(255, 0, 0), 0, 2, 1);
		}
		else
		{
			CRect rImage = get_ratio_rect(rThumb, m_thumb[i].img->width, m_thumb[i].img->height, DT_BOTTOM);
			g.DrawImage(*m_thumb[i].img, CRect_to_gpRect(rImage));
		}

		//info text 9 슬롯 (3x3) — rThumb (이미지 영역) 안에 위치 고정 표시.
		{
			static const UINT halign[3] = { DT_LEFT, DT_CENTER, DT_RIGHT };
			static const UINT valign[3] = { DT_TOP, DT_VCENTER, DT_BOTTOM };

			CRect rInfo = rThumb;
			rInfo.DeflateRect(8, 8);

			for (int idx = 0; idx < INFO_TEXT_COUNT; idx++)
			{
				if (!m_show_info_text[idx] || m_thumb[i].info[idx].IsEmpty())
					continue;

				//per-thumb cr_info 직접 사용 — insert 에서 m_cr_info_text 로부터 복사받음.
				UINT flags = halign[idx % 3] | valign[idx / 3] | DT_SINGLELINE | DT_NOCLIP;
				DrawShadowText(dc.GetSafeHdc(), CStringW(m_thumb[i].info[idx]), -1, rInfo,
					flags, m_thumb[i].cr_info[idx].ToCOLORREF(), 0, 2, 1);
			}
		}

		CRect rTitle = rThumb;
		rTitle.top = rThumb.bottom + m_thumb_title_gap;
		rTitle.bottom = rTitle.top + m_title_height;

		CString title = (m_show_extension ? m_thumb[i].title : get_part(m_thumb[i].title, fn_title));
		dc.DrawText(title, rTitle, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
		//draw_rect(g, rTitle, Gdiplus::Color::Blue);

		//항목 인덱스 표시
		if (m_show_index)
		{
			CRect rIndex = m_thumb[i].r;
			rIndex.top += 4;
			//멀티바이트 환경에서는 DrawShadowText를 쓰기위해서는 manifest를 추가해야 하는 등
			//번거로워 일단 DrawTextShadow로 대체한다.
			DrawShadowText(dc.GetSafeHdc(), CStringW(i2S(i + m_start_index)), -1, rIndex,
							DT_CENTER | DT_TOP | DT_NOCLIP, RGB(255, 255, 255), 0, 2, 1);
		}
	}

	dc.SelectObject(pOldFont);
}

BOOL CSCThumbCtrl::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	return FALSE;
	return CDialogEx::OnEraseBkgnd(pDC);
}

void CSCThumbCtrl::on_loading_completed()
{
	m_tloading_end = clock();
	//TRACE(_T("total loading completed. elapsed = %ld ms\n"), m_tloading_end - m_tloading_start);

	m_loading_completed = true;
	recalc_tile_rect();

	::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCThumbCtrl,
		(WPARAM)&CSCThumbCtrlMessage(this, CSCThumbCtrl::message_thumb_loading_completed, 0), 0);
}

void CSCThumbCtrl::set_path(CString path)
{
	m_path = path;

	stop_loading();

	m_files.clear();
	m_files = find_all_files(path, _T(""), FILE_EXTENSION_IMAGE, _T(""), false);
	//find_all_files(path, &m_files, FILE_EXTENSION_IMAGE);

	add_files(m_files);
}

void CSCThumbCtrl::add_files(std::deque<CString> files, bool reset)
{
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

	m_loading_completed = false;
	m_loading_completed_count.store(0);

	m_thread.start((int)m_files.size(),
		[this](int /*worker_idx*/, int start, int end, CSCThread& th)
		{
			for (int i = start; i < end; i++)
			{
				if (th.stop_requested())
					break;

				insert(i, m_files[i], get_part(m_files[i], fn_name), false, false);
				m_loading_completed_count.fetch_add(1, std::memory_order_acq_rel);
			}
		},
		[this]()
		{
			on_loading_completed();
		});

	Invalidate();
}

int CSCThumbCtrl::insert(int index, CString full_path, CString title, bool key_thumb, bool invalidate)
{
	m_thumb[index].img = new CSCGdiplusBitmap();

	if (get_part(full_path, fn_ext).CompareNoCase(_T("webp")) == 0)
	{
		if (m_thumb[index].img->load_webp(full_path) == false)
			return -1;
	}
	else
	{
		if (m_thumb[index].img->load(full_path) == false)
			return -1;
	}

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

	//cr_info 를 default (m_cr_info_text) 로 초기화 — Gdiplus::Color() 의 default ctor 가 사실 opaque black 이라 init 안 하면 모든 info text 가 검정.
	for (int j = 0; j < INFO_TEXT_COUNT; j++)
		m_thumb[index].cr_info[j] = m_cr_info_text[j];

	m_thumb[index].load_completed = true;

	if (invalidate)
	{
		recalc_tile_rect();
		//Invalidate();
	}

	return index;
}

int CSCThumbCtrl::insert(int index, CSCGdiplusBitmap* src_thumb, CString title, bool key_thumb, bool invalidate)
{
	if (index < 0 || index >= (int)m_thumb.size())
		return -1;

	//img 가 유효하면 deep_copy 해서 보관. null / invalid 면 m_thumb[index].img 는 그대로 (NULL 또는 기존 값).
	//왜 분기: 호출자가 thumbnail 유실된 항목 (예: 캡처 실패 / deserialize 실패) 을 그래도 표시하려는 경우,
	//title / info 는 보이고 image 영역은 OnPaint 의 "X" placeholder 가 그려져야 한다. 이전엔 img null 이면
	//early-return 으로 title 까지 set 안 돼 cell 전체가 빈 채로 남던 우회 (BookmarkEditDlg 의 set_title 명시 호출 등) 가 필요했음.
	if (src_thumb && src_thumb->m_pBitmap)
	{
		if (m_thumb[index].img)
			delete m_thumb[index].img;

		m_thumb[index].img = new CSCGdiplusBitmap();
		src_thumb->deep_copy(m_thumb[index].img);

		m_thumb[index].width = m_thumb[index].img->width;
		m_thumb[index].height = m_thumb[index].img->height;
		m_thumb[index].channel = m_thumb[index].img->channel;

		//file-path 버전과 동일 — MAX_TILE_SIZE 로 미리 축소해야 m_sz_thumb 변경 시 품질 유지.
		CRect r = get_ratio_rect(CRect(0, 0, MAX_TILE_SIZE, MAX_TILE_SIZE), m_thumb[index].img->width, m_thumb[index].img->height);
		m_thumb[index].img->resize(r.Width(), r.Height());
	}

	//title / metadata 는 img 유무와 무관하게 항상 set.
	m_thumb[index].title = title;
	m_thumb[index].full_path.Empty();
	m_thumb[index].score = 0.0;
	m_thumb[index].feature = NULL;

	//cr_info 를 default (m_cr_info_text) 로 복사.
	for (int j = 0; j < INFO_TEXT_COUNT; j++)
		m_thumb[index].cr_info[j] = m_cr_info_text[j];

	m_thumb[index].load_completed = true;

	//bitmap 직접 insert 는 동기 — 비동기 loader 가 set 할 m_loading_completed 가 영원히 false 로 남아
	//select_item 가드를 통과 못 하는 증상 방지. 여기서 호출자에게 "이 항목은 로드 완료" 신호.
	m_loading_completed = true;

	if (invalidate)
		recalc_tile_rect();

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
				else
				{
					//thumb가 선택된 상태에서 enter키를 누르면 parent에서 해당 이미지를 보여주는 등의 액션을 처리한다.
					//이미지를 보여주든 다른 액션을 취하든 그 액션은 parent에서 처리하는 것이 맞다.
					//enter or dbclick은 동일한 동작으로 처리한다.
					if (m_selected.size())
					{
						//dbclick 했을때의 액션은 parent에서 처리한다.
						::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCThumbCtrl,
							(WPARAM)&CSCThumbCtrlMessage(this, message_thumb_lbutton_dbclicked, m_selected[0]), 0);
						return true;
					}
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
	//trace(m_scroll_pos);
	//TRACE(_T("current = %d / %d\n"), m_scroll_pos, m_scroll_total);
	m_scrollbar_trans = 0.0;
	recalc_tile_rect();

	//CPoint pt;
	//GetCursorPos(&pt);
	//ScreenToClient(&pt);

	//커서가 스크롤바 영역 밖이라면 스크롤바를 숨겨준다.
	//if ((rc.PtInRect(pt) == false) || !m_r_scrollbar.PtInRect(pt))
	//	SetTimer(timer_scroll_bar_disappear, 2000, NULL);
}

void CSCThumbCtrl::scroll_up(bool up, int offset)
{
	CRect rc;
	GetClientRect(rc);

	if ((rc.Height() >= m_scroll_total))// || (offset == 0))
		return;

	int pos = m_scroll_pos;

	//trace(offset);
	if (offset == 0)
		pos += (up ? 1 : -1) * (m_sz_tile.cy + m_sz_gap.cy) / 4.0;
	else
		pos += offset;

	set_scroll_pos(pos);
}

void CSCThumbCtrl::PreSubclassWindow()
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

	//resource editor 의 border 속성 자동 감지 — WS_BORDER 또는 WS_EX_CLIENTEDGE 있으면 우리가 직접 테두리 그림.
	//분명 WS_BORDER 가 켜져 있어도 (GetStyle() & WS_BORDER) 가 false 로 나오는 경우가 있어 exStyle 까지 함께 검사.
	if ((GetStyle() & WS_BORDER) || (GetExStyle() & WS_EX_CLIENTEDGE))
	{
		m_draw_border = true;

		//기본 border 제거 — default OnNcPaint 가 덮어쓰지 않도록.
		ModifyStyle(WS_BORDER, 0);
		ModifyStyleEx(WS_EX_CLIENTEDGE | WS_EX_STATICEDGE, 0);
		SetWindowPos(NULL, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
	}

	//Resource Editor 에서 이 컨트롤을 사용하는 dlg 에 적용된 폰트를 기본으로 사용해야 한다.
	//단, 동적으로 생성된 클래스에서 이 클래스를 사용하거나
	//아직 MainWnd 가 생성되지 않은 상태에서도 이 코드를 만날 수 있으므로 parent 가 NULL 일 수 있다.
	CWnd*  parent = GetParent();
	CFont* font   = GetFont();
	if (font == NULL && parent != nullptr)
		font = parent->GetFont();

	if (font != NULL)
	{
		font->GetObject(sizeof(m_lf), &m_lf);
	}
	else
	{
		NONCLIENTMETRICS ncm = {};
		ncm.cbSize = sizeof(ncm);
		BOOL ok = ::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
#if (WINVER >= 0x0600)
		if (!ok)
		{
			ncm.cbSize = sizeof(ncm) - sizeof(ncm.iPaddedBorderWidth);
			ok = ::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
		}
#endif
		if (ok)
			m_lf = ncm.lfMessageFont;
		else
			GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(m_lf), &m_lf);
	}

	reconstruct_font();

	m_sz_thumb.cx = AfxGetApp()->GetProfileInt(_T("setting\\thumbctrl"), _T("thumb size cx"), 100);
	m_sz_thumb.cy = AfxGetApp()->GetProfileInt(_T("setting\\thumbctrl"), _T("thumb size cy"), 120);

	CDialogEx::PreSubclassWindow();
}

void CSCThumbCtrl::OnNcPaint()
{
	CDialogEx::OnNcPaint();

	if (!m_draw_border)
		return;

	CWindowDC dc(this);
	CRect rc;
	GetWindowRect(&rc);
	rc.OffsetRect(-rc.TopLeft());
	draw_rect(&dc, rc, m_theme.cr_border_inactive);
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

	if (m_r_scrollbar.PtInRect(point))
	{
		m_scrollbar_drag = true;
		SetCapture();
		CRect rc;
		GetClientRect(rc);
		set_scroll_pos(-(double)(point.y - m_scrollbar_grip_size / 2) / (double)(rc.Height() - m_scrollbar_grip_size) * (double)(m_scroll_total - rc.Height()));
		return;
	}

	for (i = 0; i < m_thumb.size(); i++)
	{
		if (m_thumb[i].r.PtInRect(point))
		{
			//Windows Explorer 식 멀티 선택:
			// - modifier 없음     : 단일 선택 (clear + add), anchor = i.
			// - Ctrl              : i 토글 (다른 선택 보존), anchor = i.
			// - Shift             : clear 후 [anchor, i] 범위 선택. anchor 유지.
			// - Ctrl+Shift        : [anchor, i] 범위 추가 (기존 선택 보존). anchor 유지.
			//m_use_multi_selection 가 false 여도 Ctrl/Shift 가 눌리면 다중 선택 가능.
			bool ctrl  = IsCtrlPressed();
			bool shift = IsShiftPressed();
			int  selected_index = find_index(m_selected, i);
			int  anchor = (m_anchor_index >= 0) ? m_anchor_index : i;

			if (ctrl && shift)
			{
				//Ctrl+Shift: [anchor, i] 범위 추가. 이미 포함된 인덱스는 skip.
				for (j = MIN(i, anchor); j <= MAX(i, anchor); j++)
				{
					if (find_index(m_selected, j) < 0)
					{
						m_selected.push_back(j);
						::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCThumbCtrl,
							(WPARAM)&CSCThumbCtrlMessage(this, message_thumb_lbutton_selected, j), 0);
					}
				}
				//anchor 유지.
			}
			else if (shift)
			{
				//Shift: clear 후 [anchor, i] 범위 선택. anchor 유지.
				m_selected.clear();
				for (j = MIN(i, anchor); j <= MAX(i, anchor); j++)
				{
					m_selected.push_back(j);
					::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCThumbCtrl,
						(WPARAM)&CSCThumbCtrlMessage(this, message_thumb_lbutton_selected, j), 0);
				}
			}
			else if (ctrl)
			{
				//Ctrl: 토글. 다른 선택은 그대로.
				if (selected_index >= 0)
				{
					m_selected.erase(m_selected.begin() + selected_index);
					::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCThumbCtrl,
						(WPARAM)&CSCThumbCtrlMessage(this, message_thumb_lbutton_unselected, i), 0);
				}
				else
				{
					m_selected.push_back(i);
					::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCThumbCtrl,
						(WPARAM)&CSCThumbCtrlMessage(this, message_thumb_lbutton_selected, i), 0);
				}
				m_anchor_index = i;
			}
			else
			{
				//modifier 없음.
				//click-rename state 의 다른 항목 click 차단은 OnLButtonUp 의 m_last_clicked_index 가드가 처리 — 여기서 별도 reset 불필요.
				if (m_use_multi_selection)
				{
					//multi 모드의 plain click — toggle 동작 보존 (기존 동작 호환).
					if (selected_index >= 0)
					{
						m_selected.erase(m_selected.begin() + selected_index);
						::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCThumbCtrl,
							(WPARAM)&CSCThumbCtrlMessage(this, message_thumb_lbutton_unselected, i), 0);
					}
					else
					{
						m_selected.push_back(i);
						::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCThumbCtrl,
							(WPARAM)&CSCThumbCtrlMessage(this, message_thumb_lbutton_selected, i), 0);
					}
				}
				else
				{
					//단일 선택: clear + add.
					m_selected.clear();
					m_selected.push_back(i);
					::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCThumbCtrl,
						(WPARAM)&CSCThumbCtrlMessage(this, message_thumb_lbutton_selected, i), 0);
				}
				m_anchor_index = i;
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
			break;
		}
	}

	//클릭된 곳이 썸네일 영역이 아니면 선택을 해제시켜준다.
	if (!m_use_multi_selection && (i == m_thumb.size()))
	{
		m_selected.clear();
		Invalidate();
	}

	m_lbutton_down = true;
	m_pt_old = point;
	SetCapture();

	CDialogEx::OnLButtonDown(nFlags, point);
}

void CSCThumbCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	m_lbutton_down = false;
	ReleaseCapture();

	if (m_scrollbar_drag)
	{
		m_scrollbar_drag = false;
		CRect rc;
		GetClientRect(rc);
		set_scroll_pos(-(double)(point.y - m_scrollbar_grip_size / 2) / (double)(rc.Height() - m_scrollbar_grip_size) * (double)(m_scroll_total - rc.Height()));
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
	long delta = t1 - m_last_clicked;
	//TRACE(_T("clicked = %d\n"), delta);

	//Windows 탐색기 식 click-rename — 같은 index 의 name 영역을 두 번 클릭 (double-click 보다 김, 너무 늦지 않음) 시 편집모드.
	// (a) m_last_clicked_index == index — 직전 클릭과 같은 항목. 다른 항목 거치면 자동 차단.
	// (b) m_last_clicked != 0 — 최초 클릭 이후만.
	// (c) delta > GetDoubleClickTime() — double-click (즉시 두 번 클릭) 회피, OnLButtonDblClk 으로 라우팅.
	// (d) delta < click_rename_max_ms — 너무 오래 전 클릭 (별개 의도) 이 rename 으로 잡히는 사고 회피. 탐색기는 무한이지만 5초 정도면 의도성 클릭 안 놓침.
	const long click_rename_max_ms = 5000;
	const long dbl_time = (long)::GetDoubleClickTime();
	if (m_last_clicked_index == index && m_last_clicked != 0
		&& delta > dbl_time && delta < click_rename_max_ms)
		edit_begin(-1);

	m_last_clicked = t1;
	m_last_clicked_index = index;

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
	if (m_lbutton_down)
	{
		//m_lbutton_down으로 스크롤 기능 구현
		//TRACE(_T("point.y = %d, m_pt_old.y = %d\n"), point.y, m_pt_old.y);
		if (point.y != m_pt_old.y)
		{
			scroll_up(true, point.y - m_pt_old.y);
			m_pt_old.y = point.y;
		}
	}

	CDialogEx::OnMouseMove(nFlags, point);
}

void CSCThumbCtrl::OnRButtonDown(UINT nFlags, CPoint point)
{
	if (m_in_editing)
		edit_end();

	SetFocus();

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
	if (lc == 1)
		r.bottom = m_thumb[index].r.bottom;
	else
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

	//편집모드 종료 시 click-rename state 도 초기화 — 그래야 종료 직후 같은 썸네일 click 이
	//"첫 번째 클릭" 으로 인식돼 두 번째 click 까지 가야 다시 편집모드 진입.
	//(reset 안 하면 편집 직전의 m_last_clicked 가 남아 있어 종료 직후 single click 으로 즉시 재진입.)
	m_last_clicked = 0;
	m_last_clicked_index = -1;

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

void CSCThumbCtrl::set_color_theme(int theme, bool invalidate)
{
	m_theme.set_color_theme(theme);
	apply_theme_to_info_text();
	if (::IsWindow(m_scrollbar.m_hWnd))
		m_scrollbar.set_color_theme(m_theme, invalidate);
	if (invalidate && m_hWnd)
		Invalidate();
}

void CSCThumbCtrl::set_color_theme(const CSCColorTheme& theme, bool invalidate)
{
	m_theme.copy_colors_from(theme);
	apply_theme_to_info_text();
	if (::IsWindow(m_scrollbar.m_hWnd))
		m_scrollbar.set_color_theme(m_theme, invalidate);
	if (invalidate && m_hWnd)
		Invalidate();
}

//theme 변경 시 info text default + 모든 썸네일의 cr_info 를 theme.cr_text 의 weak 색 (살짝 톤 다운) 으로 동기화.
//사용자가 set_info_text_color 로 customize 한 색은 함께 reset 됨 — 필요시 theme 변경 후 재호출.
void CSCThumbCtrl::apply_theme_to_info_text()
{
	Gdiplus::Color cr_default = get_weak_color(m_theme.cr_text, 64);

	for (int i = 0; i < INFO_TEXT_COUNT; i++)
		m_cr_info_text[i] = cr_default;

	for (auto& t : m_thumb)
		for (int i = 0; i < INFO_TEXT_COUNT; i++)
			t.cr_info[i] = cr_default;
}

LRESULT CSCThumbCtrl::on_message_CSCScrollbar(WPARAM wParam, LPARAM lParam)
{
	CSCScrollbarMsg* msg = (CSCScrollbarMsg*)wParam;
	if (msg == nullptr || msg->pThis != &m_scrollbar)
		return 0;

	if (msg->msg == CSCScrollbarMsg::msg_scrollbar_pos_changed)
	{
		//scrollbar pos (양수, top=0) → thumb 의 m_scroll_pos (음수 또는 0).
		set_scroll_pos(-msg->pos);
	}
	return 0;
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
	CSCEditMessage* msg = (CSCEditMessage*)wParam;

	if (!msg->pThis->IsWindowVisible())
		return 0;
	
	TRACE(_T("message(%d) from CSCEdit(%p)\n"), (int)lParam, msg->pThis);
	if (msg->message == WM_KILLFOCUS)
	{
		edit_end();
	}
	else if (msg->message == WM_KEYDOWN)
	{
		switch ((int)lParam)
		{
		case VK_RETURN:
			edit_end();
			break;
		case VK_ESCAPE:
			edit_end(false);
			break;
		}
	}

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
			//CSCGdiplusBitmap()의 소멸자에서 release()에 의해 메모리에서 제거되려 하므로 오류가 발생한다.
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
			//CSCGdiplusBitmap()의 소멸자에서 release()에 의해 메모리에서 제거되려 하므로 오류가 발생한다.
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
			//CSCGdiplusBitmap()의 소멸자에서 release()에 의해 메모리에서 제거되려 하므로 오류가 발생한다.
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
	}
	else if (key == VK_NEXT && selected <= m_thumb.size() - 1 - m_per_line * 3)
	{
		selected += m_per_line * 3;
	}

	if (selected < 0 || selected >= m_thumb.size())
		return;

	select_item(selected);
}

void CSCThumbCtrl::set_info_text(int thumb_index, int idx, CString info, bool refresh)
{
	if (thumb_index < 0 || thumb_index >= (int)m_thumb.size())
		return;

	if (idx < 0 || idx >= INFO_TEXT_COUNT)
		return;

	m_show_info_text[idx] = true;
	m_thumb[thumb_index].info[idx] = info;

	if (refresh)
		Invalidate();
}


//특정 썸네일의 특정 info text의 색상을 변경할 수 있다.
//thumb_index < 0이면 모든 썸네일에 대해 모두 적용하고
//idx < 0이면 9개의 모든 인덱스에 적용된다.
//기본값 m_cr_info_text[idx] 는 insert 시점에 각 썸네일의 cr_info[idx] 로 복사됨.
//따라서 default 변경 (thumb_index<0) 시 m_cr_info_text + 모든 기존 썸네일의 cr_info 를 같이 갱신해야 일관성 유지.
void CSCThumbCtrl::set_info_text_color(int thumb_index, int idx, Gdiplus::Color cr_info_text)
{
	auto apply_range = [&](Gdiplus::Color* arr)
	{
		if (idx < 0)
		{
			for (int i = 0; i < INFO_TEXT_COUNT; i++)
				arr[i] = cr_info_text;
		}
		else if (idx < INFO_TEXT_COUNT)
		{
			arr[idx] = cr_info_text;
		}
	};

	if (thumb_index < 0)
	{
		//default 갱신 + 모든 썸네일의 per-thumb override 도 동일 색으로 force.
		apply_range(m_cr_info_text);
		for (auto& t : m_thumb)
			apply_range(t.cr_info);
	}
	else if (thumb_index < (int)m_thumb.size())
	{
		apply_range(m_thumb[thumb_index].cr_info);
	}

	Invalidate();
}

//point는 screen coord.
void CSCThumbCtrl::OnContextMenu(CWnd* pWnd, CPoint point)
{
	//우클릭 위치의 썸네일을 선택 — 메뉴 표시 (자체 popup 이든 parent 위임이든) 의 대상이 시각·m_selected 양쪽 모두로
	//일관되도록. multi-select 가 이미 2 개 이상이면 그 선택을 보존.
	if (get_selected_items() <= 1)
	{
		CPoint pt = point;
		ScreenToClient(&pt);

		int index = get_index_from_point(pt);
		if (index >= 0)
			select_item(index);
	}

	if (!m_use_context_menu)
	{
		CWnd* parent = GetParent();
		if (parent)
			parent->SendMessage(WM_CONTEXTMENU, (WPARAM)(pWnd ? pWnd->GetSafeHwnd() : GetSafeHwnd()), MAKELPARAM(point.x, point.y));
		return;
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
			if (index < 0 || index >= (int)m_thumb.size())
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
					CSCGdiplusBitmap img(m_thumb[index].full_path);
					img.copy_to_clipboard();
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

CSCGdiplusBitmap* CSCThumbCtrl::get_img(int index)
{
	if (index < 0 || index >= m_thumb.size())
		return NULL;

	return m_thumb[index].img;
}

void CSCThumbCtrl::stop_loading()
{
	//CSCThreadGroup::stop()이 모든 워커에 stop 신호 전송 후 join 까지 대기.
	//각 워커는 th.stop_requested()를 매 반복마다 체크해서 즉시 빠져나옴.
	m_thread.stop();
}
