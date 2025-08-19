// ComboBoxPathCtrl.cpp: 구현 파일
//

#include "PathCtrl.h"
#include "../../MemoryDC.h"
#include "../../CEdit/SCEdit/SCEdit.h"

// CPathCtrl

IMPLEMENT_DYNAMIC(CPathCtrl, CStatic)

CPathCtrl::CPathCtrl()
{
	m_pEdit = NULL;

	m_cr_text.SetFromCOLORREF(::GetSysColor(COLOR_BTNTEXT));
	m_cr_back.SetFromCOLORREF(::GetSysColor(COLOR_WINDOW));
	m_crOver = Gdiplus::Color(255, 229, 243, 255);
	m_crOverBorder = Gdiplus::Color(255, 204, 232, 255);
	m_crDown = Gdiplus::Color(255, 204, 232, 255);
	m_crDownBorder = Gdiplus::Color(255, 153, 209, 255);
}

CPathCtrl::~CPathCtrl()
{
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
	ON_WM_SIZE()
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()



// CPathCtrl 메시지 처리기

void CPathCtrl::PreSubclassWindow()
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

	//CStatic은 SS_NOTIFY 속성을 줘야 마우스 이벤트가 처리된다.
	ModifyStyle(0, SS_NOTIFY);

	// Get Defalut Font 
	CFont* font = GetFont();

	if (font == NULL)
		font = AfxGetMainWnd()->GetFont();

	if (font != NULL)
		font->GetObject(sizeof(m_lf), &m_lf);
	else
		GetObject(GetStockObject(SYSTEM_FONT), sizeof(m_lf), &m_lf);

	ReconstructFont();

	DWORD dwStyle = WS_POPUP | LBS_NOTIFY | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | WS_VSCROLL | WS_BORDER;

	m_list_folder.CreateEx(WS_EX_WINDOWEDGE, _T("listbox"),
							_T("listbox"), dwStyle, CRect(0, 0, m_sz_list_folder.cx, m_sz_list_folder.cy), this, 0);//IDC_LIST_FOLDERS);

	m_list_folder.set_as_folder_list();
	m_list_folder.set_font(m_lf);
	m_list_folder.set_color_theme(CSCColorTheme::color_theme_popup_folder_list);// popup_folder_list);
	m_list_folder.set_draw_border();

	//OnNotify, OnSelChange등의 이벤트 핸들러를 추가해봤으나 되지 않아서 선택시에 SendMessage로 처리함.
	m_list_folder.set_parent(m_hWnd);


	CRect rc;
	GetClientRect(rc);
	TRACE(_T("rc = %s\n"), get_rect_info_string(rc, 2));

	rc.DeflateRect(1, 1);
	rc.left = ROOT_WIDTH;
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
	rc.left = ROOT_WIDTH;
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

	dc.FillSolidRect(rc, m_cr_back.ToCOLORREF());

	if (m_path.size() == 0)
		return;

	int		i;
	int		arrow_width = 2;
	CFont*	pOldFont = dc.SelectObject(&m_font);
	CPen	pen(PS_SOLID, 1, m_down ? m_crDownBorder.ToCOLORREF() : m_crOverBorder.ToCOLORREF());
	CPen*	pOldPen = (CPen*)dc.SelectObject(&pen);
	CBrush	brush(m_down ? m_crDown.ToCOLORREF() : m_crOver.ToCOLORREF());
	CBrush* pOldBrush = (CBrush*)dc.SelectObject(&brush);

	dc.SetBkMode(TRANSPARENT);
	CRect rArrow = rc;

	//항상 표시되는 항목
	if (m_pShellImageList)
	{
		m_pShellImageList->m_imagelist_small.Draw(&dc, m_pShellImageList->GetSystemImageListIcon(!m_is_local, get_path(m_path.size() - 1), true),
			CPoint(rc.left + 2, rc.CenterPoint().y - 8), ILD_TRANSPARENT);
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

		if (m_path[i].ellipsis)
		{
			//dc.DrawText(m_path[i].label.Left(m_path[i].ellipsis_pos) + _T("..."), rt, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
			dc.DrawText(m_path[i].label, rt, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
		}
		else
		{
			//0번 항목은 label은 존재하지만 텍스트를 출력하진 않는다.
			if (i > 0)
			{
			//	if (rt.right >= rc.right)
			//		dc.DrawText(m_path[i].label, rt, DT_RIGHT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
			//	else
					dc.DrawText(m_path[i].label, rt, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
			}
		}

		if (i < m_path.size() - 1 || m_has_subfolder)
		{
			//pathctrl의 width가 좁아서 일부 노드만 표시할 경우 생략되었음을 나타내는 << 기호를 표시
			if (i == 0 && m_start_index > 1)
			{
				draw_line(&dc, rArrow.CenterPoint().x + 3, rArrow.CenterPoint().y - 2, rArrow.CenterPoint().x + 1, rArrow.CenterPoint().y, gGRAY(192), 1);
				draw_line(&dc, rArrow.CenterPoint().x + 3, rArrow.CenterPoint().y + 2, rArrow.CenterPoint().x + 1, rArrow.CenterPoint().y, gGRAY(192), 1);
				draw_line(&dc, rArrow.CenterPoint().x - 0, rArrow.CenterPoint().y - 2, rArrow.CenterPoint().x - 2, rArrow.CenterPoint().y, gGRAY(192), 1);
				draw_line(&dc, rArrow.CenterPoint().x - 0, rArrow.CenterPoint().y + 2, rArrow.CenterPoint().x - 2, rArrow.CenterPoint().y, gGRAY(192), 1);
			}
			else
			{
				//아래로 향한 화살표 표시
				if (m_down && i == m_index)
				{
					draw_line(&dc, rArrow.CenterPoint().x - 4, rArrow.CenterPoint().y - 2, rArrow.CenterPoint().x + 1, rArrow.CenterPoint().y + 3, gGRAY(192), arrow_width);
					draw_line(&dc, rArrow.CenterPoint().x + 5, rArrow.CenterPoint().y - 2, rArrow.CenterPoint().x - 0, rArrow.CenterPoint().y + 3, gGRAY(192), arrow_width);
				}
				//일반상태의 > 화살표 표시
				else
				{
					draw_line(&dc, rArrow.CenterPoint().x - 2, rArrow.CenterPoint().y - 4, rArrow.CenterPoint().x + 3, rArrow.CenterPoint().y + 1, gGRAY(192), arrow_width);
					draw_line(&dc, rArrow.CenterPoint().x - 2, rArrow.CenterPoint().y + 4, rArrow.CenterPoint().x + 3, rArrow.CenterPoint().y - 1, gGRAY(192), arrow_width);
				}
			}
		}
	}

	if (m_pEdit->IsWindowVisible())
	{
		draw_rectangle(&dc, rc, Gdiplus::Color(0, 120, 215));
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

void CPathCtrl::set_path(CString path, std::deque<CString>* sub_folders)
{
	if (m_pShellImageList == NULL)
		return;

	if (path.IsEmpty())
		path = m_pShellImageList->m_volume[!m_is_local].get_label(CSIDL_DRIVES);

	m_path.clear();

	path = m_pShellImageList->convert_special_folder_to_real_path(!m_is_local, path);

	CString folder;

	if (m_is_local)
	{
		m_has_subfolder = has_sub_folders(path);	//마지막 폴더명 아래 서브폴더가 존재하는지
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

	//total_lines * m_list_folder.get_line_height()를 했음에도
	//1라인 적게 크기가 조정되는 현상이 있다. 왜 그럴까...우선 +1해서 처리한다.
	total_lines++;

	m_list_folder.SetWindowPos(NULL, pt.x, pt.y,
		m_sz_list_folder.cx, min(m_sz_list_folder.cy, total_lines * m_list_folder.get_line_height()),
		SWP_NOZORDER | (show ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));
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

	if (index == 0)
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

		dc.DrawText(m_path[i].label, &rt, DT_CALCRECT | DT_LEFT | DT_SINGLELINE | DT_VCENTER);

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
		//label이 출력될 width를 구하고
		dc.DrawText(m_path[i].label, &rt, DT_CALCRECT | DT_LEFT | DT_SINGLELINE | DT_VCENTER);

		if (i == 0)
			rt.right = rt.left + ROOT_WIDTH;

		//좌우 여백을 추가하고
		rt.right += m_width_margin * 2;

		//해당 폴더 아래 하위 폴더가 있다면 드롭다운 영역도 추가
		//remote인 경우는 하위 폴더목록을 request해서 받기 전까지는 구할 수 없다.
		//하지만 m_path.size() - 1보다 하위의 폴더라면 이미 하위폴더가 있다는 뜻이므로 우선 이렇게 처리한다.
		bool has_sub = (m_is_local ? has_sub_folders(m_pShellImageList->convert_special_folder_to_real_path(!m_is_local, get_path(i))) : true);
		rt.right += (has_sub || (i < m_path.size() - 1) ? m_arrow_area_width : 0);

		rt.top = rc.top;
		rt.bottom = rc.bottom;

		m_path[i].r = rt;
	}

	dc.SelectObject(pOldFont);

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

void CPathCtrl::edit_end(bool valid)
{
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
