// TreeCtrlEx.cpp : implementation file
//

#include "stdafx.h"
#include "TreeCtrlEx.h"

#include "../Functions.h"
#include "../MemoryDC.h"
#include "../AutoFont.h"
//#include "../colors.h"

#define CTREECTRLEXWND_CLASSNAME _T("CTreeCtrlEx")

// CTreeCtrlEx

IMPLEMENT_DYNAMIC(CTreeCtrlEx, CWnd)

CTreeCtrlEx::CTreeCtrlEx()
{
	if( !RegisterWindowClass() )
		return;

	//m_unicode_file = true;

	m_use_keyboard = true;

	m_use_expand_button = true;
	m_expand_button_size = 16;	//default=16

	m_use_check_box = false;
	m_check_box_at_root = false;
	m_check_box_size = 12;		//default=12
	m_check_box_text_gap = 4;

	m_use_image_list = false;
	m_image_list_size = 16;

	m_always_show_selection = true;
	m_use_multiple_selection = false;

	m_margin = CSize(4, 4);
	m_indent_size = 16;			//default=16

	memset( &m_lf, 0, sizeof( LOGFONT ) );

	set_color_theme(color_theme_default, false);

	m_use_node_line = true;
	m_node_line_style = PS_DOT;
	m_node_line_color = GRAY(128);
	m_node_line_width = 1;

	m_use_scrollbar = true;
	m_scroll_total = 0;
	m_scroll_pos = 0;
	m_scroll_unit = 1;

	m_use_image_list = false;
	m_image_list_size = 16;

	m_auto_line_height = true;
	m_line_gap = 6;
	m_line_height = 16 + m_line_gap;
}

CTreeCtrlEx::~CTreeCtrlEx()
{
	m_tr.clear();
}


BEGIN_MESSAGE_MAP(CTreeCtrlEx, CWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_VSCROLL()
	ON_WM_MOUSEWHEEL()
	ON_WM_WINDOWPOSCHANGING()
	ON_WM_WINDOWPOSCHANGED()
	ON_WM_KEYDOWN()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_ACTIVATE()
END_MESSAGE_MAP()



// CTreeCtrlEx message handlers
BOOL CTreeCtrlEx::RegisterWindowClass()
{
	WNDCLASS wndcls;
	HINSTANCE hInst = AfxGetInstanceHandle();

	if (!(::GetClassInfo(hInst, CTREECTRLEXWND_CLASSNAME, &wndcls)))
	{
		// otherwise we need to register a new class
		CBrush	brush;
		brush.CreateSolidBrush( ::GetSysColor( COLOR_3DFACE ) );

		wndcls.style            = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW ;
		wndcls.lpfnWndProc      = ::DefWindowProc;
		wndcls.cbClsExtra       = wndcls.cbWndExtra = 0;
		wndcls.hInstance        = hInst;
		wndcls.hIcon            = NULL;
		wndcls.hCursor          = AfxGetApp()->LoadStandardCursor(IDC_ARROW);
		wndcls.hbrBackground    = (HBRUSH) brush.GetSafeHandle();
		wndcls.lpszMenuName     = NULL;
		wndcls.lpszClassName    = CTREECTRLEXWND_CLASSNAME;

		if (!AfxRegisterClass(&wndcls))
		{
			AfxThrowResourceException();
			return FALSE;
		}
	}

	return TRUE;
}

//목록의 갯수가 많지 않을 경우 탭으로 표시된 문자열에서도 로딩이 가능하다.
void CTreeCtrlEx::load_from_string(CString data)
{
	m_tr.clear();

	int i;
	CString sline;

	CTreeItem item(_T("top"));

	tree<CTreeItem>::iterator top = m_tr.set_head(item);
	tree<CTreeItem>::iterator it;
	int prev_indent, tabCount;

	for (i = 0; i < data.GetLength(); i++)
	{
		if (data[i] != '\n')
		{
			sline += data[i];
		}
		else
		{
			//맨 처음 노드이면 "top"으로 추가
			if (it == NULL)
			{
				item = CTreeItem(sline);
				it = m_tr.append_child(top, item);
				prev_indent = 0;
				sline.Empty();
				continue;
			}

			//읽어온 문자열이 몇 탭 들어간 항목인지 세고
			tabCount = 0;
			while (sline[tabCount] == '\t')
				tabCount++;

			sline.Trim();
			//prev_indent와 비교하여 레벨에 맞게 추가시킨다.

			item = CTreeItem(sline);

			//전보다 1개 많으면 child이고
			if (tabCount == prev_indent + 1)
			{
				it = m_tr.append_child(it, item);
				prev_indent = tabCount;
			}
			//전과 같으면 sibling이고
			else if (tabCount == prev_indent)
			{
				it = m_tr.parent(it);
				it = m_tr.append_child(it, item);
			}
			//전보다 적으면 (prev_index-tabCount) * parent의 sibling이다.
			else if (tabCount < prev_indent)
			{
				while (prev_indent >= tabCount)
				{
					it = m_tr.parent(it);
					prev_indent--;
				}

				if (it == NULL)
				{
					it = m_tr.insert(m_tr.begin(), item);
				}
				else
				{
					it = m_tr.append_child(it, item);
				}
				prev_indent++;
			}
			else
			{
				AfxMessageBox(sline + _T("\ntext 파일 입력 오류"));
				break;
			}
			sline.Empty();
		}
	}

	recalculate_scroll_size();
	Invalidate();
}

//Tab indentation을 이용한 로딩, 저장방식
//checkAtRoot:check tree일 경우 root에도 checkbox를 표시할 지
bool CTreeCtrlEx::load_from_file(bool load, CString sfile)
{
	//_wsetlocale(LC_ALL, L"korean"); 
	FILE* fp = NULL;
	CTreeItem item(_T("top"));

	if (load)
	{
		TCHAR	chLine[1000];
		CString sline;

		//encoding 방식을 읽어온다.
		int	text_encoding = get_text_encoding(sfile);
		if (text_encoding < 0)
			return false;

		//encoding 방식을 판별하여 read/write했으나
		//utf8만 다루도록 통일한다.
		/*
		if (text_encoding <= text_ansi)
		{
			fp = _tfopen(sfile, _T("rt"));
			m_unicode_file = false;
		}
		else
		{
		*/
			fp = _tfopen(sfile, _T("rt")CHARSET);
			//m_unicode_file = true;
		//}

		if (fp == NULL)
			return false;

		m_tr.clear();


		tree<CTreeItem>::iterator top = m_tr.set_head(item);
		tree<CTreeItem>::iterator it;
		int prev_indent, tabCount;

		while (_fgetts(chLine, 1000, fp) != NULL)
		{
			sline = chLine;
			sline.TrimRight();

			//빈 라인이 있을 경우는 패스
			if (sline.GetLength() == 0)
				continue;

			//맨 처음 노드이면 루트로 추가
			if (it == NULL)
			{
				item = CTreeItem(sline);
				it = m_tr.append_child(top, item);
				prev_indent = 0;
				continue;
			}

			//읽어온 문자열이 몇 탭 들어간 항목인지 세고
			tabCount = 0;
			while (sline[tabCount] == '\t')
				tabCount++;

			sline.Trim();
			//prev_indent와 비교하여 레벨에 맞게 추가시킨다.

			item = CTreeItem(sline);

			//전보다 1개 많으면 child이고
			if (tabCount == prev_indent + 1)
			{
				it = m_tr.append_child(it, item);
				prev_indent = tabCount;
			}
			//전과 같으면 sibling이고
			else if (tabCount == prev_indent)
			{
				it = m_tr.parent(it);
				it = m_tr.append_child(it, item);
			}
			//전보다 적으면 (prev_index-tabCount) * parent의 sibling이다.
			else if (tabCount < prev_indent)
			{
				while (prev_indent >= tabCount)
				{
					it = m_tr.parent(it);
					prev_indent--;
				}

				if (it == NULL)
				{
					it = m_tr.insert(m_tr.begin(), item);
				}
				else
				{
					it = m_tr.append_child(it, item);
				}
				prev_indent++;
			}
			else
			{
				AfxMessageBox(sline + _T("\ntext 파일 입력 오류"));
				break;
			}
		}

		int res = fclose(fp);
		recalculate_scroll_size();
		Invalidate();
	}
	//텍스트 파일로 저장할 경우
	else 
	{
		//if (m_unicode_file)
			fp = _tfopen(sfile, _T("wt")CHARSET);
		//else
			//fp = _tfopen(sfile, _T("wt"));

		if (!fp)
			return false;

		// storing code
		tree<CTreeItem>::pre_order_iterator it = m_tr.begin();
		tree<CTreeItem>::pre_order_iterator end = m_tr.end();
		
		if (!m_tr.is_valid(it))
			return false;

		int rootdepth = m_tr.depth(it);

		while (it != end)
		{
			for(int i = 0; i < m_tr.depth(it) - rootdepth - 1; i++)
				_ftprintf(fp, _T("\t"));
			if (m_tr.parent(it) != NULL)
			{
				item = *it;
				_ftprintf(fp, _T("%s\n"), item.text);
			}
			++it;
		}
		fclose(fp);
	}

	return true;
}


void CTreeCtrlEx::OnPaint()
{
	CPaintDC dc1(this); // device context for painting
					   // TODO: Add your message handler code here
					   // Do not call CWnd::OnPaint() for painting messages
	CRect	rc;
	GetClientRect(rc);

	CMemoryDC	dc( &dc1, &rc, false);

	dc.FillSolidRect(rc, m_crBack);

	CFont font;
	CFont *pOldFont;

	font.CreateFontIndirect(&m_lf);
	pOldFont = (CFont*)dc.SelectObject(&font);

	//CRect	rect(m_margin.cx, m_margin.cy - GetScrollPos(SB_VERT),
	//			 rc.Width(), m_margin.cy + m_line_height - GetScrollPos(SB_VERT));
	CRect	rect(m_margin.cx,
				 m_margin.cy - m_scroll_pos,
				 rc.Width(),
				 m_margin.cy + m_line_height - m_scroll_pos);

	//노드간의 라인은 바로 이전 노드와 그려주는 것이 아니고
	//현재 노드의 parent 노드와의 선을 그려주는 것이다.
	//다만 이전 노드가 현재 노드의 sibling이었다면
	//ㄴ자 선은 parent 노드부터 그려지지 않고 이전 노드부터 그려져야 한다.
	//실선으로 그린다면 parent로부터 그려도 관계없지만
	//점선으로 그릴 경우는 점선이 겹쳐져서 실선처럼 보이게 되는 현상이 있을 수 있다.
	CRect	line_rect;		//그려질 라인 좌표
	CRect	prev_rect;		//이전 항목 사각형
	int		prev_depth = -1;//이전 항목 깊이

	tree<CTreeItem>::pre_order_iterator it = m_tr.begin();
	tree<CTreeItem>::pre_order_iterator end = m_tr.end();
	CTreeItem item;
	CRect text_rect;
	
	
	if (!m_tr.is_valid(it))
		return;

	int oldBkMode = dc.SetBkMode(TRANSPARENT);

	while (it != end)
	{
		//최상위 루트이면 패스
		if (m_tr.parent(it) == NULL)
		{
			it++;
			continue;
		}

		//화면 밖을 벗어나는 아이템은 그릴 필요가 없다.
		//단, 라인을 표시할 경우는 위에서부터 내려오는 라인이 있고
		//아래로 내려가는 라인까지는 그려줘야 하므로 상단,하단에 한 라인의 마진은 주자.
		if ((rect.top > rc.bottom + m_line_height) ||
			(rect.bottom < rc.top - m_line_height))
		{
			//화면 아래를 벗어나는 아이템을 만나면 더 이상 진행할 필요 없다.
			if (rect.top > rc.bottom + m_line_height)
				break;

			//트리의 끝까지 도달했다면 진행을 종료한다.
			if (it == end)
				break;

			it = get_next_visible_item(it);
			if (it == NULL)
				break;

			rect.left = m_margin.cx;
			rect.OffsetRect(0, m_line_height);
			continue;
		}

		item = *it;

		rect.left = MAX(rect.left, rect.left + m_indent_size * (m_tr.depth(it) - 1));
		text_rect = rect;
		text_rect.left += (m_use_expand_button ? m_expand_button_size : 0);

		//draw checkbox
		if (m_use_check_box)
		{
			if (m_check_box_at_root || (m_tr.depth(it) >= 2))
			{
				CRect check_rect = CRect(rect.left, rect.CenterPoint().y - m_check_box_size/2,
									rect.left + m_check_box_size, rect.CenterPoint().y + m_check_box_size/2);
				text_rect.left += (m_check_box_size + m_check_box_text_gap); //4 px between checkbox and text

				if (m_use_expand_button)
					check_rect.OffsetRect(m_expand_button_size, 0);

				DrawRectangle(&dc, check_rect, m_crCheck);


				if (item.checked)
				{
					double div4 = (double)check_rect.Width() / 16.0;
					int sy = check_rect.bottom - div4*6.0;

					DrawLine(&dc, 
							check_rect.CenterPoint().x - div4 - div4*4.0, sy - div4*4.0,
							check_rect.CenterPoint().x - div4, sy, m_crCheck, m_check_box_size / 8, 0 );
					DrawLine(&dc, 
							check_rect.CenterPoint().x - div4, sy,
							check_rect.CenterPoint().x - div4 + div4*6.0, sy - div4*6.0, m_crCheck, m_check_box_size / 8, 0 );
				}
			}
		}

		if (m_use_image_list)
		{
			if (m_image_list.GetImageCount() &&
				(item.image_index >= 0) &&
				(item.image_index < m_image_list.GetImageCount()))
			{
				m_image_list.Draw(&dc, item.image_index, CPoint(text_rect.left, text_rect.CenterPoint().y - m_image_list_size/2), ILD_TRANSPARENT);
				text_rect.left += m_image_list_size;
			}
		}

		if (m_tr.parent(it) != NULL)
		{
			//폰트의 종류와 크기에 따라 세로 중앙에 표시되지 않는 폰트도 있다.
			//이를 보정해주려 했으나 font matrics를 알아야 할듯해서 일단 스킵한다.
			if (item.get_selected())
			{
				//italic체를 사용할 경우 텍스트 출력 영역의 right가 짧은 경우가 있다.
				//우선 아래 두 줄을 이용해서 마진을 준다.
				CSize szText = dc.GetTextExtent( item.text );
				text_rect.right = text_rect.left + szText.cx - m_lf.lfHeight/(m_lf.lfItalic ? 2 : 4);

				COLORREF crBack = m_crBackSelected;
				if (GetFocus() != this)
				{
					if (m_always_show_selection)
						crBack = ::GetSysColor(COLOR_GRAYTEXT);
					else
					{
						dc.SetTextColor(m_crText);
						crBack = m_crBack;
					}
				}
				else
				{
					dc.SetTextColor(m_crTextSelected);
				}

				dc.FillSolidRect(text_rect, crBack);
			}
			else
			{
				dc.SetTextColor(m_crText);
			}

			dc.DrawText(item.text, text_rect, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

			//현재 항목이 그려질 때 그 이전 노드와의 관계를 고려해서 라인을 그려준다.
			if (m_use_node_line && it.node->parent != NULL)
			{
				line_rect = get_item_rect(it.node->parent, false);

				//if (m_use_expand_button)
					line_rect.left += (m_expand_button_size/2);

				line_rect.right = rect.left;	//rect.left는 현재 노드의 indent만 적용된 좌표다.

				if (it.node->prev_sibling)
					line_rect.bottom = get_item_rect(it.node->prev_sibling, false).CenterPoint().y;
				else
					line_rect.bottom = line_rect.CenterPoint().y + m_expand_button_size/2;

				if (m_tr.depth(it) > 1)
				{
					DrawLine(&dc, line_rect.left, line_rect.bottom,
							line_rect.left, rect.CenterPoint().y, m_node_line_color, m_node_line_width, m_node_line_style);
					DrawLine(&dc, line_rect.left, rect.CenterPoint().y,
							rect.left + (m_use_expand_button && (it.number_of_children()==0) ? m_expand_button_size : 4), rect.CenterPoint().y, m_node_line_color, m_node_line_width, m_node_line_style);
				}
			}

			prev_rect = rect;
			prev_depth = m_tr.depth(it);
		}

		CRect expand_rect = makeCenterRect(rect.left + m_expand_button_size/2, rect.CenterPoint().y, m_expand_button_size, m_expand_button_size);
		expand_rect.DeflateRect(3, 3, 2, 2);

		//차일드가 있을 경우
		//축소된 노드이면 +를 그려주고 it을 next sibling으로 옮기고
		//확장된 노드이면 -를 그려주고 it++ 시킨다.
		//차일드가 없을 경우 그냥 it++ 시킨다.
		if (it.number_of_children() > 0)
		{
			COLORREF crExpand = get_color(m_crCheck, (m_crText > m_crBack ? -64 : 164));
			if (it->expanded == false)
			{
				if (m_use_expand_button)
				{
					DrawRectangle(&dc, expand_rect, crExpand);//, GRAY(128));
					expand_rect.DeflateRect(3, 3);
					DrawLine(&dc, expand_rect.left-1, expand_rect.CenterPoint().y, expand_rect.right, expand_rect.CenterPoint().y, crExpand);
					DrawLine(&dc, expand_rect.CenterPoint().x, expand_rect.top-1, expand_rect.CenterPoint().x, expand_rect.bottom, crExpand);
				}

				//현재 노드가 축소된 경우 next_sibling을 만날때까지 올라간다.
				while (true)
				{
					if (it.node->next_sibling)
					{
						it = it.node->next_sibling;
						break;
					}
					it = it.node->parent;
					if (m_tr.depth(it) == 0)
						break;
				}

				//top까지 올라갔다면 더 이상 그릴게 없는 경우이다.
				if (it.node->parent == NULL)
				{
					break;
				}
			}
			else
			{
				if (m_use_expand_button)
				{
					DrawRectangle(&dc, expand_rect, crExpand);//, GRAY(128));
					expand_rect.DeflateRect(3, 3);
					DrawLine(&dc, expand_rect.left-1, expand_rect.CenterPoint().y, expand_rect.right, expand_rect.CenterPoint().y, crExpand);
				}
				it++;
			}
		}
		else
		{
			it++;
		}

		//다음 노드를 그리기 위해 노드 사각형의 위치를 이동시킨다.
		rect.left = m_margin.cx;
		rect.OffsetRect(0, m_line_height);
	}

	dc.SelectObject(pOldFont);
	dc.SetBkMode(oldBkMode);

	if (GetFocus() == this)
		dc.DrawFocusRect(rc);
		//DrawRectangle(&dc, rc, BLUE, NULL_BRUSH, 4);
	//else
		//DrawRectangle(&dc, rc, RED, NULL_BRUSH, 4);
}


BOOL CTreeCtrlEx::OnEraseBkgnd(CDC* pDC)
{
	// TODO: Add your message handler code here and/or call default
	return FALSE;
	return CWnd::OnEraseBkgnd(pDC);
}

void CTreeCtrlEx::on_mouse_event(int mouse_event, CPoint point)
{
	//custom control이라서 그런지 마우스로 클릭해도 포커스를 가지지 않는다.
	//포커스를 안가지니 전용 키 처리가 되지 않는다.
	//간단히 검색해봤으나 답을 찾지 못해서 우선 강제로 포커스를 가지게 했다.
	/*
	if (mouse_event == WM_LBUTTONDBLCLK)
		TRACE(_T("WM_LBUTTONDBLCLK\n"));
	else
		TRACE(_T("WM_LBUTTONDOWN\n"));
	*/

	SetFocus();

	CPoint pt = point;

	pt.Offset(-m_margin.cx, -m_margin.cy);

	//int index = (pt.y + GetScrollPos(SB_VERT)) / (m_line_height);
	int index = (pt.y + m_scroll_pos) / m_line_height;

	tree<CTreeItem>::pre_order_iterator it = get_item_by_index(index);
	//TRACE(_T("selected index = %d, item = %s\n"), index, it->text);
	if (it == NULL)
		return;

	//체크박스를 누르면 체크가 토글되고 그 외의 영역이 눌리면 해당 항목은 selected가 된다.
	CRect rect = get_item_rect(it, false);
#if 0
	CClientDC dc(this);
	DrawRectangle(&dc, rect, RED);
	return;
#endif	

	//+, - 버튼이 눌렸는지
	if (m_use_expand_button)
	{
		if (point.x >= rect.left && point.x <= rect.left + m_expand_button_size)
		{
			expand(it, -1);
			return;
		}
		rect.left += m_expand_button_size;
	}

	if (m_use_check_box && (m_check_box_at_root || (m_tr.depth(it) >= 2)))
	{
		if (point.x >= rect.left && point.x <= rect.left + m_check_box_size)
		{
			it->toggle_checked();
			//TRACE(_T("%s : %d, %s, check=%d\n"), __FUNCTION__, index, it->text, it->checked);
			HWND hwnd = GetParent()->GetSafeHwnd();
			InvalidateRect(rect);
			::SendMessage(GetParent()->GetSafeHwnd(), MESSAGE_TREECTRLEX_CHECK_CHANGED, (WPARAM)GetDlgCtrlID(), (LPARAM)&it);
			return;
		}
		rect.left += m_check_box_size;
	}

	//아이콘 및 텍스트가 눌려진 경우이다.
	if (!m_use_multiple_selection)
		find_item(_T(""), -1, 0);
	it->set_select(true);
	if (mouse_event == WM_LBUTTONDBLCLK)
		expand(it, -1);

	InvalidateRect(rect);
}

void CTreeCtrlEx::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	on_mouse_event(WM_LBUTTONDOWN, point);
	//CWnd::OnLButtonDown(nFlags, point);
}


void CTreeCtrlEx::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	CWnd::OnLButtonUp(nFlags, point);
}


void CTreeCtrlEx::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	CWnd::OnMouseMove(nFlags, point);
}


void CTreeCtrlEx::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	on_mouse_event(WM_LBUTTONDBLCLK, point);
	//CWnd::OnLButtonDblClk(nFlags, point);
}

BOOL CTreeCtrlEx::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	if (pMsg->message == WM_KEYDOWN)
	{
		if (!m_use_keyboard)
			return false;

		if (GetFocus() == this)
		{
			switch (pMsg->wParam)
			{
			case VK_UP :
			case VK_DOWN :
			case VK_ADD :
			case VK_SUBTRACT :
			case VK_PRIOR :
			case VK_NEXT :
			case VK_HOME :
			case VK_END :
				SendMessage(pMsg->message, pMsg->wParam, pMsg->lParam);
				return true;
			default :
				break;
			}
		}
	}

	return CWnd::PreTranslateMessage(pMsg);
}

void CTreeCtrlEx::use_check_box(bool use)
{
	m_use_check_box = use;
	Invalidate();
}

void CTreeCtrlEx::use_check_box_at_root(bool use)
{
	m_check_box_at_root = use;
	Invalidate();
}

void CTreeCtrlEx::set_check_box_size(int size /*= 16*/)
{
	if (size < 8)
		size = 16;
	m_check_box_size = size;
	Invalidate();
}

void CTreeCtrlEx::always_show_selection(bool show)
{
	m_always_show_selection = show;
	Invalidate();
}

//이 함수에서는 m_lf 정보를 이용해서 폰트를 재생성한다.
//즉, m_lf.lfHeight 값을 이용해서 폰트가 만들어지므로
//m_font_size 멤버 변수의 값이 변경되었다면
//공식을 이용해 이를 m_lf.lfHeight 값으로 변경한 후 이 함수가 호출되어야 한다.
//m_lf.lfHeight값의 절대값이 MM_TEXT모드의 현재 DC에서의 실제 픽셀크기가 된다.
//따라서 스크롤 크기 등을 계산할때는 m_font_size를 이용하는게 아니라
//m_lf.lfHeight값을 이용해야 정확한 스크롤 크기가 계산된다.
//m_font_size는 단지 사용자에게 일반적인 폰트 크기 설정 수치로 쓰이는 직관적인 수치이다.
void CTreeCtrlEx::reconstruct_font()
{
	m_font.DeleteObject();
	BOOL bCreated = m_font.CreateFontIndirect(&m_lf);
	SetFont( &m_font, true );

	m_font_size = get_font_size();

	if (m_auto_line_height)
		recalculate_line_height();

	recalculate_scroll_size();
	ASSERT(bCreated);
}

int CTreeCtrlEx::get_font_size()
{
	m_font_size = -MulDiv(m_lf.lfHeight, 72, GetDeviceCaps(::GetDC(GetParent()->GetSafeHwnd()), LOGPIXELSY));
	return m_font_size;
}

//예를 들어 폰트 크기를 10으로 설정하면
void CTreeCtrlEx::set_font_size( int font_size )
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

void CTreeCtrlEx::enlarge_font_size(bool enlarge)
{
	m_font_size = get_font_size();
	enlarge ? m_font_size++ : m_font_size--;
	m_lf.lfHeight = -MulDiv(m_font_size, GetDeviceCaps(::GetDC(GetParent()->GetSafeHwnd()), LOGPIXELSY), 72);
	reconstruct_font();
}

void CTreeCtrlEx::set_font_name(LPCTSTR sFontname, BYTE byCharSet)
{
	if (sFontname == _T(""))
		return;
	m_lf.lfCharSet = byCharSet;
	_tcscpy( m_lf.lfFaceName, sFontname);
	reconstruct_font();
}

void CTreeCtrlEx::set_font_bold( bool bBold )
{
	m_lf.lfWeight = ( bBold ? FW_BOLD : FW_NORMAL );
	reconstruct_font();
}

void CTreeCtrlEx::set_font_italic(bool italic)
{
	m_lf.lfItalic = italic;
	reconstruct_font();
}

void CTreeCtrlEx::set_log_font(LOGFONT lf)
{
	memcpy(&m_lf, &lf, sizeof(LOGFONT));
	reconstruct_font();
	Invalidate();
}

void CTreeCtrlEx::set_back_color(COLORREF crBack)
{
	m_crBack = crBack;
	Invalidate();
}

void CTreeCtrlEx::set_text_color(COLORREF crText)
{
	m_crText = crText;
	Invalidate();
}
void CTreeCtrlEx::set_check_color(COLORREF crCheck)
{
	m_crCheck = crCheck;
	Invalidate();
}

void CTreeCtrlEx::PreSubclassWindow()
{
	// TODO: Add your specialized code here and/or call the base class
	//자기 자신에게 부여된 폰트가 없다면 null이 리턴된다.
	//dlg의 parent의 font를 얻어와야 한다.
	CFont* font = GetParent()->GetFont();

	if ( font != NULL )
		font->GetObject(sizeof(m_lf), &m_lf);
	else
		GetObject(GetStockObject(SYSTEM_FONT), sizeof(m_lf), &m_lf);

	reconstruct_font();

	//WS_TABSTOP을 넣어줘도 컨트롤 클릭시 Focus를 가지지 않는다.
	//우선 클릭시 SetFocus()로 강제로 Focus를 건네주자.
	ModifyStyle(NULL, WS_BORDER | WS_EX_CLIENTEDGE/*| WS_HSCROLL | WS_VSCROLL*/);

	CWnd::PreSubclassWindow();
}

//주의!
//만약 index가 1이고 상위 index인 0이 축소된 상태라고 하면
//0의 첫번째 child가 리턴되는 오류가 발생한다.
//따라서 현재 index의 prev_sibling이 축소된 상태라고 하면
//현재 index는 prev_sibling의 next_sibling이 되어야 한다.
tree<CTreeItem>::pre_order_iterator CTreeCtrlEx::get_item_by_index(int index)
{
	int item_count = 0;
	tree<CTreeItem>::pre_order_iterator it = m_tr.begin();
	tree<CTreeItem>::pre_order_iterator end = m_tr.end();

	if (!m_tr.is_valid(it))
		return NULL;

	while (it != end)
	{
		if (m_tr.parent(it) != NULL)
		{
			if (item_count == index)
			{
				return it;
			}
			
			item_count++;

			if (it->expanded == false)
			{
				//현재 노드가 축소되어 있다면
				//현재 노드의 children들은 탐색할 필요가 없으니
				//현재 노드의 next_sibling으로 건너뛴다.
				while (true)
				{
					if (it.node->next_sibling)
					{
						it = it.node->next_sibling;
						break;
					}
					it = it.node->parent;
					if (m_tr.depth(it) == 0)
						break;
				}
				continue;
			}
		}
		++it;
	}

	return NULL;
}

/*
tree<CTreeItem>::pre_order_iterator CTreeCtrlEx::get_item_by_index(int index)
{
	int item_count = 0;
	tree<CTreeItem>::pre_order_iterator it = m_tr.begin();
	tree<CTreeItem>::pre_order_iterator end = m_tr.end();

	if (!m_tr.is_valid(it))
		return NULL;

	while (it != end)
	{
		if (m_tr.parent(it) != NULL)
		{
			if (item_count == index)
			{
				return it;
			}
			else if (it->expanded == false)
			{
				//현재 노드가 축소되어 있다면
				//그 안에 있는 children들은 탐색할 필요가 없으니
				//parent의 next_sibling으로 건너뛴다.
				while (true)
				{
					if (it.node->next_sibling)
					{
						it = it.node->next_sibling;
						break;
					}
					it = it.node->parent;
					//if (m_tr.depth(it) == 0)
					//break;
				}
				item_count++;
				continue;
			}

			item_count++;
		}
		++it;
	}

	return NULL;
}
*/
//확대, 축소를 고려하여 다음 항목을 리턴한다. 현재 노드가 축소된 상태라면 다음 노드는 현재 노드의 차일드가 아닌 sibling이 된다.
tree<CTreeItem>::pre_order_iterator CTreeCtrlEx::get_next_visible_item(tree<CTreeItem>::pre_order_iterator it)
{
	tree<CTreeItem>::pre_order_iterator end = m_tr.end();

	if (!m_tr.is_valid(it))
		return NULL;

	if (m_tr.parent(it) != NULL)
	{
		if (it->expanded == false)
		{
			//현재 노드가 축소되어 있다면
			//그 안에 있는 children들은 탐색할 필요가 없으니
			//parent의 next_sibling으로 건너뛴다.
			while (true)
			{
				if (it.node->next_sibling)
				{
					it = it.node->next_sibling;
					return it;
				}
				it = it.node->parent;
					
				if (m_tr.depth(it) == 0)
					return NULL;
			}
		}
		it++;
		return it;
	}

	return NULL;
}

//확대, 축소를 고려하여 이전 항목을 리턴한다. 현재 노드의 바로 위부터 찾으면 그 노드는 collapsed일 수 있다.
//현재 노드의 prev_sibling을 찾아서 내려가면서 get_next_visible_item을 찾아야한다.
tree<CTreeItem>::pre_order_iterator CTreeCtrlEx::get_previous_visible_item(tree<CTreeItem>::pre_order_iterator it)
{
	tree<CTreeItem>::pre_order_iterator iter = NULL;
	tree<CTreeItem>::pre_order_iterator next = NULL;

	//현재 노드의 이전 노드가 있다면 반드시 리턴한다.
	if (it.node->prev_sibling)
	{
		iter = it.node->prev_sibling;
		while (true)
		{
			next = get_next_visible_item(iter);
			if ( next == it)
				return iter;
			iter = next;
		}
	}
	//현재 노드의 이전 노드가 없다면 parent의 first_child인 경우다.
	//단, 현재 노드가 루트가 아니어야 한다.
	else if (m_tr.depth(it) > 1)
	{
		if (it.node->parent->parent != NULL)
			return it.node->parent;
	}

	return NULL;
}


//축소된 노드를 제외하고 화면에 표시해야 하는 총 노드의 개수. 스크롤 크기 재계산시에 사용된다.
int CTreeCtrlEx::get_visible_item_count()
{
	int item_count = 0;
	tree<CTreeItem>::pre_order_iterator it = m_tr.begin();
	tree<CTreeItem>::pre_order_iterator end = m_tr.end();

	if (!m_tr.is_valid(it))
		return NULL;

	while (it != end)
	{
		if (m_tr.parent(it) != NULL)
		{
			item_count++;

			if (it->expanded == false)
			{
				//현재 노드가 축소되어 있다면
				//현재 노드의 children들은 탐색할 필요가 없으니
				//현재 노드의 next_sibling으로 건너뛴다.
				while (true)
				{
					if (it.node->next_sibling)
					{
						it = it.node->next_sibling;
						break;
					}
					else
					{
						//현재 항목의 깊이가 1인데 next_sibling이 없다면 맨 마지막 차일드 항목이다.
						if (m_tr.depth(it) == 1)
							return item_count;
					}
					it = it.node->parent;
					if (m_tr.depth(it) == 0)
						break;
				}
				continue;
			}
		}
		++it;
	}

	return item_count;
}


//findText가 ""이면 모든 노드를 순회해서 check가 0또는 1이라면 체크 변경.
//findText가 ""이 아니면 해당 아이템만 찾아서 check.
//단, check가 0또는 1이 아니라면 단순히 findText의 노드를 찾고자 하는 목적으로 이용된다.
tree<CTreeItem>::pre_order_iterator CTreeCtrlEx::find_item(const CString& findText, int check, int select)
{
	tree<CTreeItem>::pre_order_iterator it = m_tr.begin();
	tree<CTreeItem>::pre_order_iterator end = m_tr.end();

	if (!m_tr.is_valid(it))
		return NULL;

	bool modified = false;

	while (it != end)
	{
		if (m_tr.parent(it) != NULL)
		{
			if (findText.IsEmpty() || (it->text == findText))
			{
				if (check == 0 || check == 1)
				{
					it->set_check(check);
					modified = true;
				}

				if (select == 0 || select == 1)
				{
					it->set_select(select);
					modified = true;
				}

				if (it->text == findText)
					break;
			}
		}
		++it;
	}

	if (modified)
		Invalidate();

	if (it == end)
		return NULL;

	return it;
}

//findText가 ""이면 모든 노드를 순회해서 check가 0또는 1이라면 체크 변경.
//findText가 ""이 아니면 해당 아이템만 찾아서 check.
//단, check가 0또는 1이 아니라면 단순히 findText의 노드를 찾고자 하는 목적으로 이용된다.
tree<CTreeItem>::pre_order_iterator CTreeCtrlEx::find_item(const CString& parent, const CString& findText, int check, int select)
{
	tree<CTreeItem>::pre_order_iterator it = find_item(parent);
	if (!m_tr.is_valid(it))
		return NULL;

	tree<CTreeItem>::pre_order_iterator end = m_tr.end();

	if (!m_tr.is_valid(it))
		return NULL;

	bool modified = false;

	while (it != end)
	{
		if (m_tr.parent(it) != NULL)
		{
			if (findText.IsEmpty() || (it->text == findText))
			{
				if (check == 0 || check == 1)
				{
					it->set_check(check);
					modified = true;
				}
				if (select == 0 || select == 1)
				{
					it->set_select(select);
					modified = true;
				}

				if (it->text == findText)
					break;
			}
		}
		++it;
	}

	if (modified)
		Invalidate();

	return it;
}

tree<CTreeItem>::pre_order_iterator CTreeCtrlEx::get_selected_item(tree<CTreeItem>::pre_order_iterator it)
{
	tree<CTreeItem>::pre_order_iterator end = m_tr.end();

	if (it == NULL)
		it = m_tr.begin();

	while (it != end)
	{
		if (it->get_selected())
			return it;
		it++;
	}

	return NULL;
}

//it 노드 하위 항목들중에서 체크된 항목들을 리턴한다.
//include_current_node	: 현재 노드의 체크 여부도 포함할 지
//only_current_group	: 현재 노드내의 노드들만 검사할 지. false이면 트리 끝까지 검사한다.
//include_child_nodes	: 노드들의 하위 노드들도 검사할 지. false이면 first_sibling들만 검사한다.
std::deque<CTreeItem> CTreeCtrlEx::get_checked_item(tree<CTreeItem>::pre_order_iterator it,
													bool include_current_node, bool only_current_group, bool include_sub_nodes)
{
	std::deque<CTreeItem> dqChecked;
	int first_node_depth = -1;

	if (it == NULL)
	{
		it = m_tr.begin();
		if (it == NULL)
			return dqChecked;
	}

	if (include_current_node)
	{
		first_node_depth = m_tr.depth(it);

		if (it->get_check())
			dqChecked.push_back(*it);
	}

	it++;
	if (it == NULL || it == m_tr.end())
		return dqChecked;

	while (true)
	{
		if (it->get_check())
			dqChecked.push_back(*it);

		if (include_sub_nodes)
		{
			it++;

			if (only_current_group && m_tr.depth(it) <= first_node_depth)
				break;
		}
		else
		{
			it = it.node->next_sibling;
		}

		if (it == NULL)
			break;
	}
	return dqChecked;
}

//선택된 항목의 문자열들을 deque에 넣어서 리턴한다.
std::deque<CString> CTreeCtrlEx::get_checked_item_text(tree<CTreeItem>::pre_order_iterator it,
	bool include_current_node, bool only_current_group, bool include_sub_nodes)
{
	std::deque<CTreeItem> items = get_checked_item(it, include_current_node, only_current_group, include_sub_nodes);

	std::deque<CString> dqText;

	for (int i = 0; i < items.size(); i++)
		dqText.push_back(items[i].text);

	return dqText;
}

int CTreeCtrlEx::get_checked_count(tree<CTreeItem>::pre_order_iterator it, bool include_current_node, bool only_current_group, bool include_sub_nodes)
{
	std::deque<CTreeItem> dqChecked = get_checked_item(it, include_current_node, only_current_group, include_sub_nodes);
	return dqChecked.size();
}

void CTreeCtrlEx::check_item(tree<CTreeItem>::pre_order_iterator it, int check)
{
	it->set_check(check);
	InvalidateRect(get_item_rect(it, false));
}

void CTreeCtrlEx::select_item(tree<CTreeItem>::pre_order_iterator it, bool single_select)
{
	if (single_select)
		find_item(_T(""), -1, 0);

	ensure_visible(it);
	it->set_select(true);
	InvalidateRect(get_item_rect(it, true));
}

CRect CTreeCtrlEx::get_item_rect(tree<CTreeItem>::pre_order_iterator itFind, bool text_only)
{
	CRect	rc;

	GetClientRect(rc);

	CRect	rect(m_margin.cx,
				 m_margin.cy - m_scroll_pos,
				 rc.Width(),
				 m_margin.cy + m_line_height - m_scroll_pos);

	tree<CTreeItem>::pre_order_iterator it = m_tr.begin();
	tree<CTreeItem>::pre_order_iterator end = m_tr.end();
	CTreeItem item;

	if (!m_tr.is_valid(it) || m_tr.parent(itFind) == NULL)
		return 0;

	while (it != end)
	{
		rect.left = m_margin.cx + m_indent_size * (m_tr.depth(it) - 1);

		if (text_only)
		{
			//expand button 사용이라면 child가 있든 없든 무조건 그 크기 만큼의 공간을 차지한다.
			rect.left += (m_use_expand_button ? m_expand_button_size : 0);
			rect.left += (m_use_check_box ? ((m_tr.depth(it) > 1 || m_check_box_at_root) ? m_check_box_size : 0) : 0);
			rect.left += (m_use_image_list ? m_image_list_size : 0);
		}

		//최상위 루트이면 패스
		if (m_tr.parent(it) == NULL)
		{
			it++;
			continue;
		}

		if (it == itFind)
			return rect;

		it = get_next_visible_item(it);
		if (it == NULL)
			return 0;

		rect.OffsetRect(0, m_line_height);
	}

	return 0;
}

CString CTreeCtrlEx::get_text(tree<CTreeItem>::pre_order_iterator it)
{
	return it->text;
}

void CTreeCtrlEx::ensure_visible(tree<CTreeItem>::pre_order_iterator it)
{
	CRect rc;
	CRect rect;
	int offset = 0;
	
	if (it == NULL || it == m_tr.begin())
		return;

	rect = get_item_rect(it, false);
	GetClientRect(rc);

	if (rect.top <= rc.top + m_line_height)
		offset = rect.top - rc.top - m_line_height;
	else if (rect.bottom >= rc.bottom - m_line_height)
		offset = rect.bottom - rc.bottom + m_line_height;

	SCROLLINFO	si;
	si.cbSize = sizeof( SCROLLINFO );

	memset( &si, 0, sizeof(SCROLLINFO) );

	m_scroll_pos += offset;
	Clamp(m_scroll_pos, 0, m_scroll_total - rc.Height());

	if (m_use_scrollbar && GetScrollInfo( SB_VERT, &si ))
	{
		si.nPos = m_scroll_pos;
		SetScrollPos( SB_VERT, si.nPos );
	}

	Invalidate(false);
}

void CTreeCtrlEx::use_expand_button(bool use)
{
	m_use_expand_button = use;
	Invalidate(false);
}

//expand (1:expand, 0:collapse, -1:toggle)
void CTreeCtrlEx::expand(tree<CTreeItem>::pre_order_iterator it, int expand)
{
	if (it.number_of_children() == 0)
	{
		it->expanded = false;
		return;
	}

	if (expand == -1)
		it->expanded = !(it->expanded);
	else
		it->expanded = expand;
	recalculate_scroll_size();
	Invalidate();
}

void CTreeCtrlEx::recalculate_scroll_size()
{
	CRect	rc;
	GetClientRect(rc);
	int total = get_visible_item_count() + 1;//m_tr.size();
	//TRACE( _T("total visible node = %d items\n"), total );

	//최근 윈도우의 스크롤바는 그 범위 크기에 따라 thumb의 크기가 자동 변경되는데
	//그래야만 thumb이 크게 표시되어 드래그하기 편해지기 떄문이다.
	//이렇게 구현하기 위해서는 nPage의 값을 적정값으로 고정하고
	//nMax = realMax + si.nPage - 1;로 주면 원하는 범위로 동작된다.
	//물론 정밀한 이동이 필요한 경우는 그 값을 작게 줘야 정밀하게 스크롤할 수 있다.
	SCROLLINFO	si;
	si.cbSize = sizeof( SCROLLINFO );

	memset( &si, 0, sizeof(SCROLLINFO) );
	GetScrollInfo( SB_VERT, &si );
	si.fMask = SIF_RANGE | SIF_PAGE;
	si.nPage = 1080 + 1;

	m_scroll_total = m_margin.cy + m_line_height * (total - 1);
	if ( m_scroll_total > rc.Height() )
	{
		//SetScrollRange( SB_VERT, 0, totalScrollHeight - rc.Height() );
		si.nMax = m_scroll_total - rc.Height() + si.nPage - 1;
	}
	else
	{
		si.nMax = 0;
		m_scroll_pos = 0;
	}

	Clamp(m_scroll_pos, 0, m_scroll_total - rc.Height());

	if (m_use_scrollbar)
	{
		si.nPos = m_scroll_pos;
		SetScrollInfo( SB_VERT, &si );
	}
}

void CTreeCtrlEx::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: Add your message handler code here and/or call default
	SCROLLINFO	si;
	si.cbSize = sizeof( SCROLLINFO );

	memset( &si, 0, sizeof(SCROLLINFO) );

	if ( GetScrollInfo( SB_VERT, &si ) )
	{
		//TRACE( "nIndex = %d, nPos = %d\n", m_nIndex, si.nPos );
		switch ( nSBCode )
		{
		case SB_PAGEUP	:
			si.nPos -= m_line_height * m_scroll_unit;
			break;
		case SB_LINEUP	:	//스크롤 바의 왼쪽(위쪽) 화살표를 클릭한 경우
			si.nPos -= m_line_height;
			break;
		case SB_PAGEDOWN	:
			si.nPos += m_line_height * m_scroll_unit;
			break;
		case SB_LINEDOWN	:	//스크롤 바의 오른쪽(아래) 화살표를 클릭한 경우
			si.nPos += m_line_height;
			break;
		case SB_THUMBPOSITION :	//트랙을 잡고 이동한 후
		case SB_THUMBTRACK	:	//트랙을 잡고 움직이는 동안
			si.nPos = si.nTrackPos;
			break;
		}

		m_scroll_pos = si.nPos;
		SetScrollPos( SB_VERT, si.nPos );
		Invalidate(false);
	}
	CWnd::OnVScroll(nSBCode, nPos, pScrollBar);
}


BOOL CTreeCtrlEx::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: Add your message handler code here and/or call default
	//int pos = m_scroll_pos;
	//int pos = GetScrollPos( SB_VERT );
	CRect	rc;
	GetClientRect(rc);

	//if (m_use_scrollbar)
		//m_scroll_pos = GetScrollPos(SB_VERT);

	if ( zDelta < 0 )
		m_scroll_pos += m_line_height * m_scroll_unit;
	else
		m_scroll_pos -= m_line_height * m_scroll_unit;

	Clamp(m_scroll_pos, 0, m_scroll_total - rc.Height());

	if (m_use_scrollbar)
		SetScrollPos( SB_VERT, m_scroll_pos );

	Invalidate(false);

	//여기서 기본 리턴하면 parent에서도 처리가 이어진다.
	//여기서 동작 처리가 완료되었음을 알리기 위해 TRUE를 리턴해준다.
	return TRUE;
	return CWnd::OnMouseWheel(nFlags, zDelta, pt);
}


void CTreeCtrlEx::OnWindowPosChanging(WINDOWPOS* lpwndpos)
{
	CWnd::OnWindowPosChanging(lpwndpos);

	// TODO: Add your message handler code here
}


void CTreeCtrlEx::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CWnd::OnWindowPosChanged(lpwndpos);

	// TODO: Add your message handler code here
	if ( !IsWindowVisible() || IsIconic() )
		return;

	recalculate_scroll_size();
}

void CTreeCtrlEx::use_scrollbar(bool use)
{
	m_use_scrollbar = use;

	DWORD style = GetStyle();

	if (m_use_scrollbar)
	{
		ModifyStyle(NULL, style | WS_VSCROLL, SWP_DRAWFRAME);
	}
	else
	{
		ModifyStyle(WS_VSCROLL, NULL, SWP_DRAWFRAME);
	}
	//ModifyStyleEx(0, WS_EX_CLIENTEDGE, SWP_DRAWFRAME);

	recalculate_scroll_size();
	
	//UpdateWindow();
	//RedrawWindow();
	//Invalidate();
}

void CTreeCtrlEx::set_color_theme(int theme, bool invalidate)
{
	switch ( theme )
	{
	case color_theme_default :
		m_crText			= ::GetSysColor(COLOR_BTNTEXT);
		m_crBack			= ::GetSysColor(COLOR_WINDOW);
		m_crCheck			= ::GetSysColor(COLOR_BTNTEXT);
		m_crTextSelected	= ::GetSysColor(COLOR_HIGHLIGHTTEXT);
		m_crBackSelected	= ::GetSysColor(COLOR_HIGHLIGHT);
		break;
	case color_theme_dark_gray :
		m_crText			= GRAY(192);
		m_crBack			= GRAY(32);
		m_crCheck			= GRAY(192);
		m_crTextSelected	= GRAY(32);
		m_crBackSelected	= GRAY(192);
		break;
	}

	if (invalidate)
		Invalidate();
}

void CTreeCtrlEx::draw_line_node(bool draw)
{
	m_use_node_line = draw;
	Invalidate();
}


void CTreeCtrlEx::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	tree<CTreeItem>::pre_order_iterator it = get_selected_item();
	tree<CTreeItem>::pre_order_iterator end = m_tr.end();

	if (m_tr.size() == 0)
		return;

	if (it == NULL)
		it = m_tr.begin();

	if (!m_tr.is_valid(it))
		return;

	CRect	rc;
	GetClientRect(rc);

	switch (nChar)
	{
		case VK_UP :
		case VK_DOWN :
		{
			bool up = (nChar == VK_UP);

			if (up && (it != m_tr.begin().node->first_child))
			{
				it = get_previous_visible_item(it);
				select_item(it);
			}
			else if (!up && (it != m_tr.end()) && (get_next_visible_item(it) != NULL))
			{
				it = get_next_visible_item(it);
				if (it.node->parent == NULL)
					return;
				select_item(it);
			}
			break;
		}
		case VK_SPACE :
			it->toggle_checked();
			InvalidateRect(get_item_rect(it, false));
			break;
		case VK_ADD :
			expand(it, true);
			break;
		case VK_SUBTRACT :
			expand(it, false);
			break;
		case VK_PRIOR :
			m_scroll_pos -= m_line_height * 8;
			Clamp(m_scroll_pos, 0, m_scroll_total - rc.Height());
			if (m_use_scrollbar)
				SetScrollPos( SB_VERT, m_scroll_pos );
			Invalidate(false);
			break;
		case VK_NEXT :
			m_scroll_pos += m_line_height * 8;
			Clamp(m_scroll_pos, 0, m_scroll_total - rc.Height());
			if (m_use_scrollbar)
				SetScrollPos( SB_VERT, m_scroll_pos );
			Invalidate(false);
			break;
		case VK_HOME :
			m_scroll_pos = 0;
			if (m_use_scrollbar)
				SetScrollPos( SB_VERT, m_scroll_pos );
			Invalidate(false);
			break;
		case VK_END :
			m_scroll_pos = m_scroll_total - rc.Height();
			if (m_use_scrollbar)
				SetScrollPos( SB_VERT, m_scroll_pos );
			Invalidate(false);
			break;
	}
}


void CTreeCtrlEx::OnSetFocus(CWnd* pOldWnd)
{
	CWnd::OnSetFocus(pOldWnd);

	// TODO: Add your message handler code here
	Invalidate();
}


void CTreeCtrlEx::OnKillFocus(CWnd* pNewWnd)
{
	CWnd::OnKillFocus(pNewWnd);

	// TODO: Add your message handler code here
	Invalidate();
}



void CTreeCtrlEx::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	CWnd::OnActivate(nState, pWndOther, bMinimized);

	// TODO: Add your message handler code here
	MessageBeep(0);
}

void CTreeCtrlEx::recalculate_line_height()
{
	m_line_height = m_line_gap;
	if (m_use_image_list)
		m_line_height += MAX(m_image_list_size, -m_lf.lfHeight);
	else
		m_line_height += (-m_lf.lfHeight);

	recalculate_scroll_size();
}

void CTreeCtrlEx::set_image_list_size(int image_size)
{
	m_image_list_size = image_size;

	if (m_auto_line_height)
		recalculate_line_height();

	Invalidate();
}

//image_list를 테스트하기 위해 각 노드에 랜덤으로 이미지 인덱스를 부여한다.
void CTreeCtrlEx::set_image_random()
{
	tree<CTreeItem>::pre_order_iterator it = m_tr.begin();
	tree<CTreeItem>::pre_order_iterator end = m_tr.end();

	if (!m_tr.is_valid(it))
		return;

	while (it != end)
	{
		it->image_index = random19937(0, m_image_list.GetImageCount()-1);
		++it;
	}

	Invalidate();
}