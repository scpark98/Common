// ComboBoxPathCtrl.cpp: 구현 파일
//

#include "PathCtrl.h"
#include "../../MemoryDC.h"
#include "../../CEdit/SCEdit/SCEdit.h"
#include "../../Functions.h"	//get_monitor_index / get_monitor_rect / g_monitors — 서브폴더 팝업 모니터 보정용.

// CPathCtrl

//편집모드 바깥 클릭을 마우스 훅에서 감지하면, 훅 프로시저 안에서 직접 edit_end 를 부르지 않고(set_path/
//SendMessage 재진입 위험) 이 메시지를 자기 자신에게 post 해 message loop 에서 종료 처리한다. CStatic 은
//WM_USER 대역을 쓰지 않으므로 자기 window 로의 self-post 에 한해 충돌 없음.
#define WM_PATHCTRL_END_EDIT	(WM_USER + 0x171)

CPathCtrl* CPathCtrl::s_editing_ctrl = NULL;

IMPLEMENT_DYNAMIC(CPathCtrl, CStatic)

CPathCtrl::CPathCtrl()
{
	m_pEdit = NULL;

	//CSCColorTheme 의 default 테마는 CStatic 계열에는 cr_back = COLOR_BTNFACE 를 부여하지만
	//PathCtrl 은 Win 탐색기 주소표시줄과 동일한 흰 배경이 의도된 외관이므로 명시 override.
	m_theme.cr_back = RGB2gpColor(::GetSysColor(COLOR_WINDOW));
}

CPathCtrl::~CPathCtrl()
{
	remove_edit_mouse_hook();

	if (m_pEdit)
	{
		m_pEdit->DestroyWindow();
		delete m_pEdit;
	}
}

//#define IDC_LIST_FOLDERS 1999

BEGIN_MESSAGE_MAP(CPathCtrl, CStatic)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_TIMER()
	//ON_NOTIFY(LBN_SELCHANGE, IDC_LIST_FOLDERS, &CPathCtrl::OnLbnSelchange)
	ON_REGISTERED_MESSAGE(Message_CSCListBox, &CPathCtrl::on_message_CSCListBox)
	ON_REGISTERED_MESSAGE(Message_CSCEdit, &CPathCtrl::on_message_CSCEdit)
	ON_MESSAGE(WM_PATHCTRL_END_EDIT, &CPathCtrl::on_end_edit_posted)
	ON_WM_SIZE()
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()



// CPathCtrl 메시지 처리기

void CPathCtrl::PreSubclassWindow()
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

	//CStatic은 SS_NOTIFY 속성을 줘야 마우스 이벤트가 처리된다.
	ModifyStyle(0, SS_NOTIFY);

	//resource editor 의 Border / Client Edge / Static Edge / Sunken 중 무엇을 켰든 시그널로 받아 m_draw_border 로 옮김.
	//이 플래그들이 그대로 남으면 시스템이 sunken 또는 etched 외곽을 강제로 그려 (구식 XP 룩) 테마와 어긋남.
	//CSCEdit/CSCListBox 등 다른 Common 컨트롤들과 동일한 패턴 — 플래그는 "사용자가 border 원함" 신호로만 쓰고,
	//실제 그리기는 OnPaint 가 cr_border_inactive/active 로 직접 처리.
	if ((GetStyle() & WS_BORDER) ||
		(GetStyle() & SS_SUNKEN) ||
		(GetExStyle() & WS_EX_CLIENTEDGE) ||
		(GetExStyle() & WS_EX_STATICEDGE))
	{
		m_draw_border = true;
		ModifyStyle(WS_BORDER | SS_SUNKEN, 0);
		ModifyStyleEx(WS_EX_CLIENTEDGE | WS_EX_STATICEDGE, 0);
		SetWindowPos(NULL, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
	}

	//Resource Editor 에서 이 컨트롤을 사용하는 dlg 에 적용된 폰트를 기본으로 사용해야 한다.
	//단, 동적으로 생성된 클래스에서 이 클래스를 사용하거나 아직 MainWnd 가 생성되지 않은 상태에서도
	//이 코드를 만날 수 있으므로 parent 가 NULL 일 수 있다.
	//이전 패턴 (AfxGetMainWnd()->GetFont() + SYSTEM_FONT 폴백) 은 ① MainWnd NULL dereference 위험 ②
	//SYSTEM_FONT 가 비트맵 fixed font 라 stroke 가 두꺼워 bold 처럼 보이는 부작용 → CSCTreeCtrl / CSCListBox
	//와 동일한 4단계 폴백 (GetFont → parent->GetFont → NONCLIENTMETRICS.lfMessageFont → DEFAULT_GUI_FONT) 으로 교체.
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

	//Common 규약: 모든 기본 폰트는 Segoe UI 강제 (parent 에 다른 face 가 명시되어 있어도 일관성 우선).
	_tcscpy_s(m_lf.lfFaceName, _countof(m_lf.lfFaceName), _T("Segoe UI"));
	m_lf.lfCharSet = DEFAULT_CHARSET;

	ReconstructFont();

	//WS_BORDER 와 WS_EX_WINDOWEDGE 는 시스템이 NC 안쪽에 default 색으로 1-2px frame 을 그려 dark theme 에서
	//흰 띠로 보임. 둘 다 제거하고 시각적 border 는 DWM (DWMWA_WINDOW_CORNER_PREFERENCE + DWMWA_BORDER_COLOR) 가 담당.
	//LBS_NOINTEGRALHEIGHT — 이게 없으면 listbox 가 "부분 항목 미표시" 위해 window 높이를 item_height 배수로
	//자동 스냅하는데, 그 계산이 우리 커스텀 NC padding(2*m_popup_padding) 을 무시해 window 가 padding 만큼 줄어들고
	//client 가 한 줄 모자라져 (visible = N-1) 불필요한 scrollbar + 하단 빈 띠가 생긴다. 자동 스냅을 끄고 우리가
	//calc_popup_height_for_lines 로 정확한 높이를 직접 지정한다.
	DWORD dwStyle = WS_POPUP | LBS_NOTIFY | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | WS_VSCROLL | LBS_NOINTEGRALHEIGHT;

	m_list_folder.CreateEx(0, _T("listbox"),
							_T("listbox"), dwStyle, CRect(0, 0, m_sz_list_folder.cx, m_sz_list_folder.cy), this, 0);//IDC_LIST_FOLDERS);

	m_list_folder.set_as_folder_list();
	m_list_folder.set_font(m_lf);
	//m_list_folder.set_color_theme(CSCColorTheme::color_theme_popup_folder_list);// popup_folder_list);
	m_list_folder.set_draw_border();

	//OnNotify, OnSelChange등의 이벤트 핸들러를 추가해봤으나 되지 않아서 선택시에 SendMessage로 처리함.
	m_list_folder.set_parent(m_hWnd);


	CRect rc;
	GetClientRect(rc);
	//TRACE(_T("rc = %s\n"), get_rect_info_str(rc, 2));

	rc.DeflateRect(1, 1);
	rc.left = (m_icon_left - 2) + ROOT_WIDTH;
	dwStyle = WS_CHILD | ES_AUTOHSCROLL | ES_MULTILINE;
	m_pEdit = new CSCEdit();// (this, item, subItem, GetItemText(item, subItem));
	m_pEdit->Create(dwStyle, rc, this, 0);
	m_pEdit->SetFont(&m_font, true);
	((CSCEdit*)m_pEdit)->set_line_align(DT_VCENTER);
	//((CSCEdit*)m_pEdit)->set_draw_border(false);

	repos_edit();

	CStatic::PreSubclassWindow();
}

//resize를 하면 여백이 리셋되므로 다시 여백 설정
void CPathCtrl::repos_edit()
{
	CRect rc;

	GetClientRect(rc);
	rc.DeflateRect(1, 1);
	//path 가 설정되어 있을 때는 루트 아이콘 셀(chain origin + ROOT_WIDTH)만큼 좌측을 비워둬야 OnPaint 가 그
	//자리에 루트 아이콘(내 PC / 문서 / 바탕화면 선택용) 을 그릴 수 있다. origin 은 m_icon_left 한 knob 에서 파생.
	//path 미설정 상태에서는 아이콘이 안 그려지므로 edit 의 left 는 0 (deflate 후) 부터 시작 — 사용자가
	//"빈 상태에서는 왼쪽 들여쓰기가 어색하다" 라고 지적.
	if (m_path.size() > 0)
		rc.left = (m_icon_left - 2) + ROOT_WIDTH;
	m_pEdit->MoveWindow(rc);
	/*
	CRect margin = rc;
	CSize szText;
	CClientDC dc(m_pEdit);

	szText = dc.GetTextExtent(_T("A"));	//height만 필요함
	//rc.DeflateRect(2, (rc.Height() - szText.cy) / 2);
	margin.left = 0;
	margin.top += (margin.Height() - szText.cy) / 2 - 1;
	margin.bottom -= (margin.Height() - szText.cy) / 2;

	m_pEdit->SetRect(&margin);
	TRACE(_T("rc = %s\n"), get_rect_info_string(margin, 2));
	*/
}

void CPathCtrl::ReconstructFont()
{
	m_font.DeleteObject();
	BOOL bCreated = m_font.CreateFontIndirect(&m_lf);

	ASSERT(bCreated);
}

BOOL CPathCtrl::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	if (pMsg->message == WM_KEYDOWN)
	{
		bool	is_exist = false;
		CString text;

		switch (pMsg->wParam)
		{
		case VK_RETURN :
			edit_end();
			return true;

		case VK_ESCAPE:
			if (!m_pEdit->IsWindowVisible() || (GetFocus() != m_pEdit))
				break;

			TRACE(_T("escape\n"));
			m_pEdit->ShowWindow(SW_HIDE);
			SetWindowText(m_edit_old_text);
			return true;
		}
	}

	return CStatic::PreTranslateMessage(pMsg);
}


void CPathCtrl::OnPaint()
{
	CPaintDC dc1(this); // device context for painting
					   // TODO: 여기에 메시지 처리기 코드를 추가합니다.
					   // 그리기 메시지에 대해서는 CStatic::OnPaint()을(를) 호출하지 마십시오.
	CRect rc;
	CRect rt;
	CString text;

	GetClientRect(rc);

	CMemoryDC dc(&dc1, &rc);

	dc.FillSolidRect(rc, m_theme.cr_back.ToCOLORREF());

	//path 가 비어 있을 때도 border 는 그려야 한다 — 기존 early return 이 이 함수 끝의 else if (m_draw_border) 분기
	//도달을 막아 resource editor 의 Border=true 가 효과 없어보이는 원인이 되었다. 여기서 한 번 그리고 빠진 뒤,
	//path 가 채워진 경우는 함수 끝에서 한 번 더 그리게 된다 (drawing 비용 무시할 만하고, edit 활성 시
	//cr_border_active 로 덮어쓰는 경로도 유지).
	if (m_path.size() == 0)
	{
		if (m_draw_border)
			draw_rect(&dc, rc, m_theme.cr_border_inactive);
		return;
	}

	int		i;
	CFont*	pOldFont = dc.SelectObject(&m_font);
	//down/hover 색상 매핑 — m_crDown/m_crOver 시절의 RGB 값과 동일한 theme 필드 사용:
	//  m_crOver     (229,243,255) == cr_back_hover
	//  m_crOverBorder(204,232,255) == cr_back_selected (light blue fill 과 동일 톤)
	//  m_crDown     (204,232,255) == cr_back_selected
	//  m_crDownBorder(153,209,255) == cr_selected_border
	COLORREF cr_fill   = (m_down ? m_theme.cr_back_selected : m_theme.cr_back_hover).ToCOLORREF();
	COLORREF cr_border = (m_down ? m_theme.cr_selected_border : m_theme.cr_back_selected).ToCOLORREF();
	CPen	pen(PS_SOLID, 1, cr_border);
	CPen*	pOldPen = (CPen*)dc.SelectObject(&pen);
	CBrush	brush(cr_fill);
	CBrush* pOldBrush = (CBrush*)dc.SelectObject(&brush);

	dc.SetBkMode(TRANSPARENT);
	dc.SetTextColor(m_theme.cr_text.ToCOLORREF());
	CRect rArrow = rc;

	//하위폴더 pulldown 의 ">" (접힘) / "v" (펼침) chevron — CSCTreeCtrl 의 chevron 과 동일한 모양·두께로 그린다
	//(두께 1.6f, ">" = 3×7px / "v" = 7×3px 대칭, 둥근 끝점/꼭짓점, AA + PixelOffsetModeHalf). 색은 호출처에서
	//트리와 같이 get_weak_color(텍스트색, 48) 로 전달한다. (cx,cy) 는 arrow 영역 중심.
	auto draw_chevron = [&](int cx, int cy, bool down, Gdiplus::Color cr)
	{
		Gdiplus::Graphics g(dc.GetSafeHdc());
		g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
		g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);

		Gdiplus::Pen pen(cr, 1.6f);
		pen.SetStartCap(Gdiplus::LineCapRound);
		pen.SetEndCap(Gdiplus::LineCapRound);
		pen.SetLineJoin(Gdiplus::LineJoinRound);

		Gdiplus::PointF pts[3];
		if (down)
		{
			//v
			pts[0] = Gdiplus::PointF((float)(cx - 3.5f), (float)(cy - 1.5f));
			pts[1] = Gdiplus::PointF((float)(cx + 0.0f), (float)(cy + 1.5f));
			pts[2] = Gdiplus::PointF((float)(cx + 3.5f), (float)(cy - 1.5f));
		}
		else
		{
			//>
			pts[0] = Gdiplus::PointF((float)(cx - 1.5f), (float)(cy - 3.5f));
			pts[1] = Gdiplus::PointF((float)(cx + 1.5f), (float)(cy + 0.0f));
			pts[2] = Gdiplus::PointF((float)(cx - 1.5f), (float)(cy + 3.5f));
		}
		g.DrawLines(&pen, pts, 3);
	};

	//항상 표시되는 항목
	if (m_pShellImageList)
	{
		m_pShellImageList->m_imagelist_small.Draw(&dc, m_pShellImageList->GetSystemImageListIcon(!m_is_local, get_path(m_path.size() - 1), true),
			CPoint(rc.left + m_icon_left, rc.CenterPoint().y - 8), ILD_TRANSPARENT);
	}

	//0번은 무조건 표시하고 나머지는 m_start_index 이상부터 표시한다.
	for (i = 0; i < m_path.size(); i++)
	{
		if (i > 0 && i < m_start_index)
			continue;

		rt = m_path[i].r;

		rArrow = rt;

		if (i == m_index)
		{
			//현재 over 또는 down 상태인 셀을 특정색의 사각형으로 표시.
			//단, 0번의 경우는 화살표 영역만 사각형을 그려준다.
			if (i == 0)
			{
				rt.left = rt.right - m_arrow_area_width;
				dc.Rectangle(rt);
			}
			else
			{
				dc.Rectangle(rt);
			}

			if (i < m_path.size() - 1 || m_has_subfolder)
			{
				rArrow.left = rt.right - m_arrow_area_width;
				dc.Rectangle(rArrow);
			}
		}

		if (i < m_path.size() - 1 || m_has_subfolder)
		{
			rt.right -= m_arrow_area_width;
			rArrow.left = rArrow.right - m_arrow_area_width;
		}

		rt.left += 5;

		//활성 세그먼트(i==m_index)는 강조 배경 위에 그려지므로 그 상태색으로 — down=cr_text_selected, hover=cr_text_hover (CSCTreeCtrl 와 동일 규칙).
		dc.SetTextColor((i == m_index ? (m_down ? m_theme.cr_text_selected : m_theme.cr_text_hover) : m_theme.cr_text).ToCOLORREF());

		if (m_path[i].ellipsis)
		{
			dc.DrawText(m_path[i].label, rt, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
		}
		else
		{
			//0번 항목은 label은 존재하지만 텍스트를 출력하진 않는다.
			if (i > 0)
			{
				dc.DrawText(m_path[i].label, rt, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
			}
		}

		if (i < m_path.size() - 1 || m_has_subfolder)
		{
			//chevron 색을 CSCTreeCtrl 과 동일하게 — 항목 텍스트색(활성 세그먼트는 강조 배경 위라 상태색,
			//비활성은 본래 cr_text)을 흐리게. 원색은 너무 진해 탐색기 chevron 의 옅은 톤과 다름.
			//get_weak_color(절대 offset) 대신 get_color(glyph, cr_back, ratio) (대비 비례) — 저대비 테마에서 묻힘 방지. ratio 0.2 ≒ offset 48.
			Gdiplus::Color cr_glyph = (i == m_index ? (m_down ? m_theme.cr_text_selected : m_theme.cr_text_hover) : m_theme.cr_text);
			Gdiplus::Color cr_arrow = get_color(cr_glyph, m_theme.cr_back, 0.2);

			//pathctrl의 width가 좁아서 일부 노드만 표시할 경우 생략되었음을 나타내는 << 기호를 표시
			if (i == 0 && m_start_index > 1)
			{
				draw_line(&dc, rArrow.CenterPoint().x + 3, rArrow.CenterPoint().y - 2, rArrow.CenterPoint().x + 1, rArrow.CenterPoint().y, cr_arrow.ToCOLORREF(), 1);
				draw_line(&dc, rArrow.CenterPoint().x + 3, rArrow.CenterPoint().y + 2, rArrow.CenterPoint().x + 1, rArrow.CenterPoint().y, cr_arrow.ToCOLORREF(), 1);
				draw_line(&dc, rArrow.CenterPoint().x - 0, rArrow.CenterPoint().y - 2, rArrow.CenterPoint().x - 2, rArrow.CenterPoint().y, cr_arrow.ToCOLORREF(), 1);
				draw_line(&dc, rArrow.CenterPoint().x - 0, rArrow.CenterPoint().y + 2, rArrow.CenterPoint().x - 2, rArrow.CenterPoint().y, cr_arrow.ToCOLORREF(), 1);
			}
			else
			{
				//m_down 이고 현재 항목이면 펼침 "v", 아니면 접힘 ">".
				draw_chevron(rArrow.CenterPoint().x, rArrow.CenterPoint().y, (m_down && i == m_index), cr_arrow);
			}
		}
	}

	if (m_pEdit->IsWindowVisible())
	{
		//edit 활성 시 포커스 indicator. cr_border_active 가 default 테마에서 CornflowerBlue 로
		//Win Explorer 의 진한 accent blue 와 의미가 같음 (강한 active focus border).
		draw_rect(&dc, rc, m_theme.cr_border_active);
	}
	else if (m_draw_border)
	{
		//편집중이 아니어도 resource editor 에서 Border 를 켰다면 inactive 색으로 항상 외곽선.
		//기존엔 CSCEdit 가 보일 때만 border 가 그려져, edit 표시 ↔ 숨김 시 외곽선이 깜박이듯 사라졌다.
		draw_rect(&dc, rc, m_theme.cr_border_inactive);
	}

	dc.SelectObject(pOldFont);
	dc.SelectObject(pOldPen);
	dc.SelectObject(pOldBrush);
}

//로컬인지 원격인지 세팅
void CPathCtrl::set_is_local_device(bool is_local)
{
	m_is_local = is_local;
}

//원격일 경우 드라이브 볼륨 리스트를 얻어와서 이 함수를 통해 미리 넣어줘야 한다.
//({drive0 letter, drive0 volume}, {drive1 letter, drive1 volume}, ...)
/*
void CPathCtrl::set_remote_drive_volume(std::map<TCHAR, CString>* remote_drive_volume)
{
	if (!remote_drive_volume)
		return;

	m_remote_drive_volume.clear();
	m_remote_drive_volume.insert(remote_drive_volume->begin(), remote_drive_volume->end());
}

//한번에 map에 넣기 힘든 구조라면 아래 함수를 통해 하나씩 추가해도 된다.(add_remote_drive_volume(_T("로컬 디스크 (C:)"));
void CPathCtrl::add_remote_drive_volume(CString remote_drive_volume)
{
	//"C:\\" (볼륨 레이블을 구하지 못하는 경우)
	if (remote_drive_volume.Right(2) == _T(":\\"))
	{
		m_remote_drive_volume.insert(std::pair<TCHAR, CString>(remote_drive_volume[0], GetUserDefaultUILanguage() == 1042 ? _T("로컬 디스크") : _T("Local Disk")));
	}
	//"로컬 디스크 (C:)" (볼륨 레이블까지 포함된 경우)
	else
	{
		CString real_path = convert_special_folder_to_real_path(remote_drive_volume, m_pShellImageList->get_csidl_map());
		//CString drive_volume = remote_drive_volume.Left(remote_drive_volume.Find(real_path.Left(2)) - 2);
		m_remote_drive_volume.insert(std::pair<TCHAR, CString>(real_path[0], remote_drive_volume));
	}
}
*/
void CPathCtrl::SetWindowText(CString path, std::deque<CString>* sub_folders)
{
	set_path(path, sub_folders);
}

//경로는 그대로인데 세그먼트 표시 이름만 바뀐 경우(드라이브 볼륨 레이블 변경)용. get_path() 는 레이블의 " (X:)" 에서
//드라이브 문자를 뽑으므로 레이블이 옛 이름이어도 경로 자체는 정확하다. m_path 를 비워 set_path 의 조기 리턴을 지나가게 한 뒤
//같은 경로로 다시 구성하면 레이블이 새 볼륨 캐시에서 다시 만들어진다.
void CPathCtrl::refresh_path()
{
	CString path = get_path();

	m_path.clear();
	set_path(path);
}

void CPathCtrl::set_path(CString path, std::deque<CString>* sub_folders)
{
	if (m_pShellImageList == NULL)
		return;

	if (path.IsEmpty())
		path = m_pShellImageList->m_volume[!m_is_local].get_label(CSIDL_DRIVES);

	if (path == get_path())
		return;

	m_path.clear();

	path = m_pShellImageList->convert_special_folder_to_real_path(!m_is_local, path);

	CString folder;

	if (m_is_local)
	{
		//"내 PC"는 파일시스템 경로가 아니라 셸 가상 폴더라 FindFirstFile("내 PC\\*")가 열 수 없다.
		//즉 has_sub_folders()는 이 경우 항상 false 를 낸다. 내 PC의 하위 폴더는 드라이브이고 드라이브가 없는 PC 는 없으므로 true.
		if (path == m_pShellImageList->m_volume[!m_is_local].get_label(CSIDL_DRIVES))
		{
			m_has_subfolder = true;
		}
		else
		{
			m_has_subfolder = has_sub_folders(path);	//마지막 폴더명 아래 서브폴더가 존재하는지
		}
	}
	else
	{
		if (sub_folders)
		{
			m_has_subfolder = (sub_folders->size() > 0);
			m_remote_sub_folders.clear();
			m_remote_sub_folders.assign(sub_folders->begin(), sub_folders->end());
		}
		else
		{
			//::SendMessage(GetParent()->m_hWnd, MESSAGE_PATHCTRL, (WPARAM)&CPathCtrlMessage(this, message_pathctrl_path_changed), (LPARAM)&path);
			//::SendMessage(GetParent()->m_hWnd, MESSAGE_PATHCTRL, (WPARAM)&CPathCtrlMessage(this, message_pathctrl_request_remote_subfolders, path), (LPARAM)&m_remote_sub_folders);

			m_has_subfolder = (m_remote_sub_folders.size() > 0);
		}
	}

	if (path.IsEmpty())// || path == _T("내 PC"))
	{
		m_has_subfolder = true;
	}

	//path를 각 토큰별로 분리하고 앞에 기본 토큰을 넣어준다(ex. 내 PC)
	int i = 0;

	//path 토큰 분리
	for (folder = path.Tokenize(_T("\\"), i); i >= 0; folder = path.Tokenize(_T("\\"), i))
	{
		m_path.push_back(CPathElement(folder));
	}

	//드라이브 명을 레이블로 변경
	if (path.Mid(1, 2) == _T(":\\"))
	{
		CString drive_volume = m_pShellImageList->m_volume[!m_is_local].get_drive_volume(path);
		
		/*
		if (m_is_local)
		{
			drive_volume = m_pShellImageList->m_volume[!m_is_local].get_drive_volume(path[0]);
		}
		else
		{
			drive_volume = m_pShellImageList->m_volume[!m_is_local].get_drive_volume(path);
		}
		*/

		m_path[0].label.Format(_T("%s"), drive_volume);
	}

	//기본 토큰 추가
	if ((m_path.size() == 0) ||
		(m_path.size() > 0 && m_path[0].label != m_pShellImageList->m_volume[!m_is_local].get_label(CSIDL_DRIVES)) ||
		(m_path.size() > 1 && m_path[1].label != m_pShellImageList->m_volume[!m_is_local].get_label(CSIDL_DRIVES)))
		m_path.push_front(CPathElement(m_pShellImageList->m_volume[!m_is_local].get_label(CSIDL_DRIVES)));

	//항상 표시되는 최상위 항목 추가
	if (!m_path[0].label.IsEmpty())
		m_path.push_front(CPathElement(_T("")));

	//recalc_path_width();
	recalc_path_position();
}

BOOL CPathCtrl::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	return FALSE;
	return CStatic::OnEraseBkgnd(pDC);
}

void CPathCtrl::show_sub_folder_list(bool show)
{
	TRACE(_T("%s, show = %d\n"), __function__, show);
	CPoint pt(m_path[m_index].r.right - 32, m_path[m_index].r.bottom);
	ClientToScreen(&pt);

	//현재 선택된 폴더의 하위 폴더들을 리스트에 표시하는데
	//현재 폴더의 하위 폴더가 m_path에 존재하면 해당 항목을 선택된 항목으로(또는 볼드체로) 표시해준다.
	int total_lines = 0;
	CString full_path = get_path(m_index);

	if (show)
	{
		//리스트가 나타날 때 깜빡임을 없애려고 숨겼으나 여전히 나타날 때 깜빡인다.
		m_list_folder.ShowWindow(SW_HIDE);

		//shrink 상태라면 shrink된 항목들을 차례대로 넣어준다.
		if (m_index == 0 && m_start_index > 1)
		{
			std::deque<CString> lists;
			for (int i = m_start_index - 1; i >= 1; i--)
			{
				lists.push_back(m_path[i].label);
			}

			lists.push_back(m_pShellImageList->m_volume[!m_is_local].get_label(CSIDL_DESKTOP));//  _T("바탕 화면"));
			total_lines = m_list_folder.set_folder_list(&lists, m_path[m_start_index].label);
		}
		else
		{
			//맨 끝 폴더가 아니면 하위 폴더 목록에서 다음 폴더 항목이 선택된 상태로 표시되게 한다.
			if (m_index < m_path.size() - 1)
			{
				if (m_is_local)
				{
					total_lines = m_list_folder.set_path(full_path, m_path[m_index + 1].label);
				}
				else
				{
					::SendMessage(GetParent()->m_hWnd, Message_CPathCtrl, (WPARAM)&CPathCtrlMessage(this, message_pathctrl_request_remote_subfolders, full_path), (LPARAM)&m_remote_sub_folders);
					total_lines = m_list_folder.set_folder_list(&m_remote_sub_folders, m_path[m_start_index].label);
				}
			}
			else
			{
				if (m_is_local)
				{
					total_lines = m_list_folder.set_path(full_path);
				}
				else
				{
					::SendMessage(GetParent()->m_hWnd, Message_CPathCtrl, (WPARAM)&CPathCtrlMessage(this, message_pathctrl_request_remote_subfolders, full_path), (LPARAM)&m_remote_sub_folders);
					total_lines = m_list_folder.set_folder_list(&m_remote_sub_folders);
				}
			}
		}
	}
	
	if (total_lines == 0)
		show = false;

	//항목이 15개 이하면 항목 수만큼의 높이로 딱 맞게(스크롤바 없음), 15개를 초과하면 15줄 높이로 고정하고
	//나머지는 스크롤바로 본다. calc_popup_height_for_lines 는 NC padding 포함한 전체 window height 반환.
	const int max_popup_lines = 15;
	int display_lines = min(total_lines, max_popup_lines);
	int popup_w = m_sz_list_folder.cx;
	int popup_h = m_list_folder.calc_popup_height_for_lines(display_lines);

	//팝업이 모니터(작업영역) 밖으로 나가면 메뉴처럼 안쪽으로 보정한다. pt 는 이미 screen 좌표.
	if (g_monitors.empty())
		enum_display_monitors();
	int mon = get_monitor_index(pt.x, pt.y);
	CRect rc_mon = (mon >= 0 && mon < (int)g_monitors.size()) ? g_monitors[mon].rWork : get_monitor_rect(-1);

	if (pt.x + popup_w > rc_mon.right)
		pt.x = rc_mon.right - popup_w;
	if (pt.x < rc_mon.left)
		pt.x = rc_mon.left;

	if (pt.y + popup_h > rc_mon.bottom)
	{
		//아래로 넘치면 경로 항목 위쪽으로 flip(메뉴 방식). 위로도 넘치면 상단 경계에 맞춘다.
		int anchor_top = pt.y - m_path[m_index].r.Height();
		pt.y = anchor_top - popup_h;
		if (pt.y < rc_mon.top)
			pt.y = rc_mon.top;
	}

	m_list_folder.SetWindowPos(NULL, pt.x, pt.y,
		popup_w, popup_h,
		SWP_NOZORDER | (show ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));

	//WS_POPUP 라 표시만으로는 포커스가 안 옴 → WM_MOUSEWHEEL 이 포커스 창으로만 가는 특성상 휠이 커서가
	//팝업 위에 있을 때(Win10 hover-scroll)만 듣고, 위로 flip 되어 뜨면 안 먹었다. 팝업은 OnKillFocus 에서
	//숨겨지는 *포커스 보유* 설계이므로 표시 시 포커스를 줘 휠이 위치와 무관하게 항상 동작하게 한다.
	if (show)
		m_list_folder.SetFocus();
}


CString CPathCtrl::get_path(int index)
{
	CString fullpath;

	//전체 path크기가 1이면 최상위 루트이고
	if (m_path.size() == 1)
		return _T("");

	//index가 0보다 작으면 현재 선택된 항목까지의 fullpath를 만들어서 리턴하고
	if (index < 0)
		index = m_path.size() - 1;

	if (index <= 0)
		return _T("");
	else if (index == 1)
		return m_pShellImageList->m_volume[!m_is_local].get_label(CSIDL_DRIVES);

	//special folder들인 경우(바탕 화면 등등)
	if (m_path[2].label.Find(_T(":\\")) < 0 && m_path[2].label.Find(_T(":)")) < 0)
	{
		fullpath = m_pShellImageList->convert_special_folder_to_real_path(!m_is_local, m_path[2].label);
		return fullpath;
	}

	//"로컬 디스크 (C:)" => "C:\\"
	CString drive = m_path[2].label.Mid(m_path[2].label.GetLength() - 3, 1) + _T(":\\");


	if (index == 2)
		return drive;

	fullpath = drive;

	for (int i = 3; i <= index; i++)
	{
		fullpath = fullpath + m_path[i].label + (i == index ? _T("") : _T("\\"));
	}

	return fullpath;
}

void CPathCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	TRACE(_T("m_index = %d\n"), m_index);

	if (m_index >= (int)m_path.size())
		return;

	//항목 이외의 빈 공간이 눌려지면 edit을 표시해서 직접 편집가능하게 한다.
	if (m_use_edit && m_index < 0)
	{
		repos_edit();

		m_edit_old_text = get_path(-1);

		m_pEdit->ShowWindow(SW_SHOW);
		m_pEdit->SetWindowText(m_edit_old_text);

		m_pEdit->SetSel(0, -1);
		m_pEdit->SetFocus();

		//편집 시작 — 바깥 클릭 감지용 마우스 훅 설치.
		install_edit_mouse_hook();

		Invalidate();
		return;
	}

	//클릭된 위치가 폴더명인지 pulldown영역인지 구분.
	CRect r = m_path[m_index].r;
	int count = 0;
	CString full_path = get_path(m_index);

	if (m_is_local)
	{
		count = get_sub_folders(full_path);
	}
	else
	{
		if (m_index == 0 && full_path.IsEmpty())
		{
			m_remote_sub_folders.clear();
			m_remote_sub_folders.push_back(m_pShellImageList->m_volume[!m_is_local].get_label(CSIDL_DRIVES));
			m_remote_sub_folders.push_back(m_pShellImageList->m_volume[!m_is_local].get_label(CSIDL_MYDOCUMENTS));
			m_remote_sub_folders.push_back(m_pShellImageList->m_volume[!m_is_local].get_label(CSIDL_DESKTOP));
		}
		else
		{
			::SendMessage(GetParent()->m_hWnd, Message_CPathCtrl, (WPARAM)&CPathCtrlMessage(this, message_pathctrl_request_remote_subfolders, full_path), (LPARAM)&m_remote_sub_folders);
		}
		count = m_remote_sub_folders.size();
	}

	if (m_index == 0)
	{
		//최상위 항목의 아이콘이 눌린 경우는 아무런 처리를 하지 않는다.
		if (point.x < m_path[0].r.right - m_arrow_area_width)
			return;

		count = 1;
	}

	m_down = !m_down;

	r.left = r.right - m_arrow_area_width;
	if (count > 0 && r.PtInRect(point))
	{
		show_sub_folder_list(m_down);
	}
	else
	{
		//폴더 항목을 누른거라면 해당 폴더로 변경하고 parent에게도 통보한다.
		set_path(full_path);
		::SendMessage(GetParent()->m_hWnd, Message_CPathCtrl, (WPARAM)&CPathCtrlMessage(this, message_pathctrl_path_changed, full_path), (LPARAM)0);
	}

	Invalidate();

	CStatic::OnLButtonDown(nFlags, point);
}


void CPathCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	CStatic::OnLButtonUp(nFlags, point);
}


void CPathCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	for (int i = 0; i < m_path.size(); i++)
	{
		if (m_path[i].r.PtInRect(point))
		{
			if (i != m_index)
			{
				m_index = i;
				//TRACE(_T("m_index = %d\n"), m_index);
				Invalidate();
				if (m_down)
					show_sub_folder_list(m_down);

				//커서가 밖으로 나가면 오버 효과를 사라지도록. TrackMouse로 변경 필요.
				SetTimer(timer_mouse_over, 500, NULL);
			}

			return;
		}
	}

	if (m_index != -1 &&!m_list_folder.IsWindowVisible())
	{
		m_index = -1;
		Invalidate();
	}

	CStatic::OnMouseMove(nFlags, point);
}


void CPathCtrl::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	CStatic::OnRButtonDown(nFlags, point);
}


void CPathCtrl::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	CStatic::OnRButtonUp(nFlags, point);
}


void CPathCtrl::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (nIDEvent == timer_mouse_over)
	{
		CRect rw;
		CPoint pt;

		GetWindowRect(rw);
		GetCursorPos(&pt);

		if (!rw.PtInRect(pt) && !m_list_folder.IsWindowVisible())
		{
			KillTimer(timer_mouse_over);
			m_index = -1;
			m_down = false;
			m_list_folder.ShowWindow(SW_HIDE);
			Invalidate();
		}
	}

	CStatic::OnTimer(nIDEvent);
}

void CPathCtrl::OnLbnSelchange(NMHDR* pNMHDR, LRESULT* pResult)
{
	int index = m_list_folder.GetCurSel();

	if (index < 0 || index >= m_list_folder.GetCount())
		return;

	CString text = _T("lksjdflj");
	//text = m_list_folder.m_folder_list[index];

	TRACE(_T("CPathCtrl::selected = %s\n"), text);
	return;
}

LRESULT CPathCtrl::on_message_CSCListBox(WPARAM wParam, LPARAM lParam)
{
	CSCListBoxMessage* pMsg = (CSCListBoxMessage*)wParam;
	CString path = *(CString*)lParam;

	if (pMsg->pThis != &m_list_folder)
		return 0;

	m_down = false;

	set_path(path);
	::SendMessage(GetParent()->m_hWnd, Message_CPathCtrl, (WPARAM)&CPathCtrlMessage(this, message_pathctrl_path_changed, path), (LPARAM)0);

	return 0;
}


void CPathCtrl::OnSize(UINT nType, int cx, int cy)
{
	CStatic::OnSize(nType, cx, cy);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	if (m_pEdit->IsWindowVisible())
	{
		repos_edit();
	}
	else
	{
		recalc_path_position();
	}
}

void CPathCtrl::recalc_path_width()
{
	CRect	rc;
	CRect	rt;
	CClientDC dc(this);
	GetClientRect(rc);

	CFont* pOldFont = dc.SelectObject(&m_font);

	//각 토큰마다 필요한 영역을 계산하여 변경한다.
	for (int i = 0; i < m_path.size(); i++)
	{
		rt = rc;

		//다음 항목과의 기본 간격
		if (i > 0)
			rt.left = m_path[i - 1].r.right + 2;

		//20260709 by claude. DT_NOPREFIX — 폭 측정도 '&' 를 리터럴로 계산해야 한다. 없으면 "R&D" 가 mnemonic 으로 "RD" 폭으로 측정돼
		//세그먼트가 좁게 잡히고 실제 표시(NOPREFIX)와 폭이 어긋난다.
		dc.DrawText(m_path[i].label, &rt, DT_CALCRECT | DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);

		//맨 끝 항목은 rc.right전까지 영역을 허용한다.
		if ((i == m_path.size() - 1) && (rt.right + (m_has_subfolder ? m_arrow_area_width : 0) > rc.right - 10))
		{
			rt.right = rc.right - 10;
		}
		//if (rt.Width() >= m_max_width)
		//{
		//	rt.right = rt.left + m_max_width;
		//	//m_path[i].ellipsis = true;
		//	//m_path[i].ellipsis_pos = get_ellipsis_pos(&dc, m_path[i].label, m_max_width);
		//}

		//10은 항목 좌우 기본 여백크기
		if (i < m_path.size() - 1 || m_has_subfolder)
			rt.right += (10 + m_arrow_area_width);
		else
			rt.right += 10;

		rt.top = rc.top;
		rt.bottom = rc.bottom;
		m_path[i].r = rt;
	}

	dc.SelectObject(pOldFont);

	m_start_index = 1;
	recalc_path_position();

	//Invalidate();
}

void CPathCtrl::recalc_path_position()
{
	if (m_path.size() == 0)
		return;

	//전체 패스를 표시하기에는 rc width가 작은 경우
	//뒤에서부터 어느 항목까지 표시 가능한지 계산한다.
	//각 항목의 좌표값은 동적으로 조정됨.
	//단, 0번 항목은 항상 표시하므로 조정 대상에서 제외함.
	int i;
	CRect	rc;
	CRect	rt;
	CClientDC dc(this);

	GetClientRect(rc);
	//맨 오른쪽의 최소한의 마진을 둔다. 추후 history combobox가 들어갈...
	rc.right = (rc.right > 40 ? rc.right - 40 : rc.right);


	CFont* pOldFont = dc.SelectObject(&m_font);

	//각 항목의 표시 너비를 계산하고
	for (i = m_path.size() - 1; i >= 0; i--)
	{
		//label이 출력될 width를 구하고 (DT_NOPREFIX — '&' 를 리터럴로 계산. 20260709 by claude.)
		dc.DrawText(m_path[i].label, &rt, DT_CALCRECT | DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);

		if (i == 0)
			rt.right = rt.left + ROOT_WIDTH;

		//좌우 여백을 추가하고
		rt.right += m_width_margin * 2;

		//해당 폴더 아래 하위 폴더가 있다면 드롭다운 영역도 추가.
		//마지막보다 앞의 세그먼트는 자식이 있다는 게 자명하고, 마지막 세그먼트의 자식 유무는 set_path 가 이미
		//m_has_subfolder 에 판정해 뒀다. 여기서 다시 판정하면 OnPaint 의 그리기 조건과 어긋날 수 있는데,
		//어긋나면 그리기는 화살표 자리를 빼면서 폭은 안 더해져 레이블이 잘린다("내 PC" → "내..."). 조건을 OnPaint 와 동일하게 맞춘다.
		rt.right += (i < m_path.size() - 1 || m_has_subfolder ? m_arrow_area_width : 0);

		rt.top = rc.top;
		rt.bottom = rc.bottom;

		m_path[i].r = rt;
	}

	dc.SelectObject(pOldFont);

	//경로 체인의 가로 origin — 루트 아이콘과 동일 knob 에서 파생(chain origin = m_icon_left - 2, 2 = 아이콘 inset).
	//이후 모든 항목이 OffsetRect(앞 항목.right) 로 이 origin 에서 줄줄이 파생되므로, 여기 한 줄이 아이콘·'>'·전체
	//세그먼트의 좌측 시작점을 동시에 결정한다. (이전엔 width 루프의 carried rt.left 에 암묵 의존해 불안정했음.)
	m_path[0].r.left = rc.left + m_icon_left - 2;

	//최상위 항목의 너비는 21로 고정된다.
	m_path[0].r.right = m_path[0].r.left + ROOT_WIDTH + m_arrow_area_width;


	//각 항목의 크기가 결정됐다면 순차적으로 배치시켜준다.
	for (i = 1; i < m_path.size(); i++)
	{
		m_path[i].r.OffsetRect(m_path[i - 1].r.right, 0);
	}


	//rc.width를 넘치지 않는 m_start_index를 끝에서부터 찾는다.
	int total_width = m_path[0].r.Width();
	m_start_index = 1;

	for (i = m_path.size() - 1; i >= 1; i--)
	{
		total_width += m_path[i].r.Width();
		if (total_width >= rc.Width())
		{
			m_start_index = i + 1;
			
			//맨 끝 항목 하나만 표시하기에도 rc.width가 작은 경우
			if (m_start_index >= m_path.size())
				m_start_index = m_path.size() - 1;

			break;
		}
	}

	//아무리 rc.width가 좁아도 항상 끝에서 2개의 항목은 표시된다.
	if (m_path.size() >= 3 && m_start_index == m_path.size() - 1)
		m_start_index = m_path.size() - 2;
	//TRACE(_T("start_index = %d\n"), m_start_index);

	//옮겨진 start_index의 r.left만큼 모두 빼준다.
	if (m_path.size() > 1 && m_start_index >= 1)
	{
		int new_offset = -(m_path[m_start_index].r.left - m_path[0].r.right);
		//if (m_start_index > 0)
		{
			for (i = 1; i < m_path.size(); i++)
				m_path[i].r.OffsetRect(new_offset, 0);
		}
	}

	//아무리 rc.width가 좁아도 항상 끝에서 2개의 항목은 표시된다.
	//그럴 경우 end-2 항목부터 최소한으로 축소시켜주고 그래도 rc.width를 넘어간다면
	//end-1항목의 너비를 축소시킨다.
	int cnt = m_path.size();
	if (m_path.size() == 2)
	{

	}
	else if (m_path.size() > 2)
	{
		int rootw = m_path[0].r.Width();
		int p2w = m_path[cnt - 2].r.Width();
		int p1w = m_path[cnt - 1].r.Width();
		int min_width = 100;

		//두 항목을 표시하기에 좁다면
		if ((rootw + p2w + p1w) >= rc.right)
		{
			//p2w가 최소너비보다 크면 줄인다.
			if (p2w > min_width)
			{
				m_path[cnt - 2].r.right = m_path[cnt - 2].r.left + (rc.right - rootw - m_path[cnt - 1].r.Width());

				//줄였는데 최소너비라면 최소너비로 고정하고 끝 항목을 줄인다.
				if (m_path[cnt - 2].r.Width() < min_width)
				{
					m_path[cnt - 2].r.right = m_path[cnt - 2].r.left + min_width;
					m_path[cnt - 1].r.left = m_path[cnt - 2].r.right;
					m_path[cnt - 1].r.right = rc.right;
				}
				//줄어든 end-2 항목에 맞춰 end-1 항목도 조정해주고
				else
				{
					m_path[cnt - 1].r.left = m_path[cnt - 2].r.right;
					m_path[cnt - 1].r.right = m_path[cnt - 1].r.left + p1w;
				}
			}
			else
			{
				m_path[cnt - 1].r.right = rc.right;
				if (p1w > min_width && m_path[cnt - 1].r.Width() < min_width)
					m_path[cnt - 1].r.right = m_path[cnt - 1].r.left + min_width;
			}
		}
	}


	Invalidate();
}

void CPathCtrl::OnKillFocus(CWnd* pNewWnd)
{
	CStatic::OnKillFocus(pNewWnd);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	//editbox가 표시된 상태에서는 이 KillFocus가 발생할 수 없으므로 아래 코드는 처리되지 않는다.
	//실제 editbox에서 KillFocus()되면 이를 메시지로 받아서 처리해야 한다.
	//if (m_pEdit && m_pEdit->IsWindowVisible())
	//{
	//	m_pEdit->ShowWindow(SW_HIDE);
	//}
}

LRESULT CPathCtrl::on_message_CSCEdit(WPARAM wParam, LPARAM lParam)
{
	CSCEditMessage* msg = (CSCEditMessage*)wParam;

	if (msg->pThis->IsWindowVisible() == FALSE)
		return 0;

	TRACE(_T("message(%d) from CSCEdit(%p)\n"), (int)msg->message, msg->pThis);
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

void CPathCtrl::edit_end(bool valid)
{
	//편집 종료 경로(Enter/Esc/KILLFOCUS/바깥클릭) 가 어디든 훅은 반드시 해제. IsWindowVisible 가드보다 먼저.
	remove_edit_mouse_hook();

	if (!m_pEdit->IsWindowVisible())// || (GetFocus() != m_pEdit))
		return;

	TRACE(_T("edit_end(%d)\n"), valid);

	m_pEdit->ShowWindow(SW_HIDE);

	if (valid)
		m_pEdit->GetWindowText(m_edit_new_text);

	//입력한 경로를 실제 폴더경로와 동일한 대소문자로 보정해줘야 한다.
	//ex. "c:\windows"를 입력하면 "C:\Windows"와 같이 실제 파일시스템에 저장된 경로명으로 표시해줘야 한다.
	//GetFullPathName(), _tfullpath(), PathCanonicalize(), SHGetFileInfo(), GetLongPathName() 모두 사용해봤으나
	//GetShortPathName()으로 변환한 후 GetLongPathName()을 호출하면 된다.
	//if (!PathFileExists(m_edit_new_text))
	//	m_edit_new_text = _T("");
	//else
		m_edit_new_text = get_original_path(m_edit_new_text);

	TRACE(_T("m_edit_new_text = %s\n"), m_edit_new_text);

	if (m_edit_old_text == m_edit_new_text)
	{
		Invalidate();
		return;
	}

	bool res = false;

	//입력한 경로가 실제로 존재하는지 확인한 후 경로를 변경해줘야 한다.
	::SendMessage(GetParent()->m_hWnd, Message_CPathCtrl, (WPARAM)&CPathCtrlMessage(this, message_pathctrl_path_changed, m_edit_new_text), (LPARAM)&res);

	if (!res)
		set_path(m_edit_old_text);
	else
		set_path(m_edit_new_text);

	Invalidate();
}

void CPathCtrl::install_edit_mouse_hook()
{
	if (m_mouse_hook != NULL)
		return;

	//WH_MOUSE 는 자기 스레드 메시지 큐만 감시 → 다른 앱 클릭은 보지 않는다(그쪽은 WM_KILLFOCUS 가 처리).
	//이 훅의 목적은 "같은 스레드(같은 dlg) 의 edit 바깥 클릭" 감지뿐이다.
	s_editing_ctrl = this;
	m_mouse_hook = ::SetWindowsHookEx(WH_MOUSE, edit_mouse_hook_proc, NULL, ::GetCurrentThreadId());
}

void CPathCtrl::remove_edit_mouse_hook()
{
	if (m_mouse_hook != NULL)
	{
		::UnhookWindowsHookEx(m_mouse_hook);
		m_mouse_hook = NULL;
	}
	if (s_editing_ctrl == this)
		s_editing_ctrl = NULL;
}

LRESULT CALLBACK CPathCtrl::edit_mouse_hook_proc(int code, WPARAM wParam, LPARAM lParam)
{
	if (code == HC_ACTION && s_editing_ctrl != NULL && s_editing_ctrl->m_pEdit != NULL)
	{
		if (wParam == WM_LBUTTONDOWN || wParam == WM_RBUTTONDOWN || wParam == WM_MBUTTONDOWN ||
			wParam == WM_NCLBUTTONDOWN || wParam == WM_NCRBUTTONDOWN || wParam == WM_NCMBUTTONDOWN)
		{
			MOUSEHOOKSTRUCT* mh = (MOUSEHOOKSTRUCT*)lParam;
			CRect rc_edit;
			s_editing_ctrl->m_pEdit->GetWindowRect(&rc_edit);

			//edit 내부 클릭은 캐럿 이동이므로 종료 금지. 바깥이면 종료를 post(클릭 자체는 그대로 통과시킴).
			if (!rc_edit.PtInRect(mh->pt))
				s_editing_ctrl->PostMessage(WM_PATHCTRL_END_EDIT, 0, 0);
		}
	}

	return ::CallNextHookEx(NULL, code, wParam, lParam);
}

LRESULT CPathCtrl::on_end_edit_posted(WPARAM wParam, LPARAM lParam)
{
	edit_end(true);
	return 0;
}

void CPathCtrl::set_color_theme(int theme, bool invalidate)
{
	m_theme.set_color_theme(theme);

	//default/windows 테마는 control-kind 분기로 CStatic 계열에 COLOR_BTNFACE(회색) 를 주지만, PathCtrl 은
	//Win 탐색기 주소표시줄과 동일한 COLOR_WINDOW(흰) 배경이 의도된 외관이므로 cr_back 만 명시 override.
	if (theme == CSCColorTheme::color_theme_default || theme == CSCColorTheme::color_theme_windows)
		m_theme.cr_back = get_sys_color(COLOR_WINDOW);

	//popup folder 리스트도 같은 theme 으로 propagate — CSCTreeCtrl/SCComboBox 와 동일한 패턴.
	if (::IsWindow(m_list_folder.m_hWnd))
		m_list_folder.set_color_theme(theme, invalidate);

	if (invalidate && m_hWnd)
		Invalidate();
}

void CPathCtrl::set_color_theme(const CSCColorTheme& theme, bool invalidate)
{
	//src 가 default/windows(둘 다 control-kind 분기를 타는 시스템색 추종 테마) 라면 set_color_theme(int)
	//경로로 단락 — 그 안에 PathCtrl 전용 "cr_back = COLOR_WINDOW" override (탐색기 주소표시줄 흰 배경) 가
	//들어있어, 그걸 거치지 않으면 CStatic 계열이 colors.cpp 의 해당 분기에서 COLOR_BTNFACE 회색을 받게 된다.
	//copy_colors_from 도 default/windows src 에 set_color_theme(int) 을 재호출하지만 그건 CSCColorTheme
	//객체 자체의 메서드라 PathCtrl 의 override 는 안 거침.
	int src_theme = theme.get_color_theme();
	if (src_theme == CSCColorTheme::color_theme_default || src_theme == CSCColorTheme::color_theme_windows)
	{
		set_color_theme(src_theme, invalidate);
		return;
	}

	m_theme.copy_colors_from(theme);

	if (::IsWindow(m_list_folder.m_hWnd))
		m_list_folder.set_color_theme(m_theme, invalidate);

	if (invalidate && m_hWnd)
		Invalidate();
}
