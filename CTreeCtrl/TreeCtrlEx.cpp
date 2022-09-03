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

//����� ������ ���� ���� ��� ������ ǥ�õ� ���ڿ������� �ε��� �����ϴ�.
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
			//�� ó�� ����̸� "top"���� �߰�
			if (it == NULL)
			{
				item = CTreeItem(sline);
				it = m_tr.append_child(top, item);
				prev_indent = 0;
				sline.Empty();
				continue;
			}

			//�о�� ���ڿ��� �� �� �� �׸����� ����
			tabCount = 0;
			while (sline[tabCount] == '\t')
				tabCount++;

			sline.Trim();
			//prev_indent�� ���Ͽ� ������ �°� �߰���Ų��.

			item = CTreeItem(sline);

			//������ 1�� ������ child�̰�
			if (tabCount == prev_indent + 1)
			{
				it = m_tr.append_child(it, item);
				prev_indent = tabCount;
			}
			//���� ������ sibling�̰�
			else if (tabCount == prev_indent)
			{
				it = m_tr.parent(it);
				it = m_tr.append_child(it, item);
			}
			//������ ������ (prev_index-tabCount) * parent�� sibling�̴�.
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
				AfxMessageBox(sline + _T("\ntext ���� �Է� ����"));
				break;
			}
			sline.Empty();
		}
	}

	recalculate_scroll_size();
	Invalidate();
}

//Tab indentation�� �̿��� �ε�, ������
//checkAtRoot:check tree�� ��� root���� checkbox�� ǥ���� ��
bool CTreeCtrlEx::load_from_file(bool load, CString sfile)
{
	//_wsetlocale(LC_ALL, L"korean"); 
	FILE* fp = NULL;
	CTreeItem item(_T("top"));

	if (load)
	{
		TCHAR	chLine[1000];
		CString sline;

		//encoding ����� �о�´�.
		int	text_encoding = get_text_encoding(sfile);
		if (text_encoding < 0)
			return false;

		//encoding ����� �Ǻ��Ͽ� read/write������
		//utf8�� �ٷ絵�� �����Ѵ�.
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

			//�� ������ ���� ���� �н�
			if (sline.GetLength() == 0)
				continue;

			//�� ó�� ����̸� ��Ʈ�� �߰�
			if (it == NULL)
			{
				item = CTreeItem(sline);
				it = m_tr.append_child(top, item);
				prev_indent = 0;
				continue;
			}

			//�о�� ���ڿ��� �� �� �� �׸����� ����
			tabCount = 0;
			while (sline[tabCount] == '\t')
				tabCount++;

			sline.Trim();
			//prev_indent�� ���Ͽ� ������ �°� �߰���Ų��.

			item = CTreeItem(sline);

			//������ 1�� ������ child�̰�
			if (tabCount == prev_indent + 1)
			{
				it = m_tr.append_child(it, item);
				prev_indent = tabCount;
			}
			//���� ������ sibling�̰�
			else if (tabCount == prev_indent)
			{
				it = m_tr.parent(it);
				it = m_tr.append_child(it, item);
			}
			//������ ������ (prev_index-tabCount) * parent�� sibling�̴�.
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
				AfxMessageBox(sline + _T("\ntext ���� �Է� ����"));
				break;
			}
		}

		int res = fclose(fp);
		recalculate_scroll_size();
		Invalidate();
	}
	//�ؽ�Ʈ ���Ϸ� ������ ���
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

	//��尣�� ������ �ٷ� ���� ���� �׷��ִ� ���� �ƴϰ�
	//���� ����� parent ������ ���� �׷��ִ� ���̴�.
	//�ٸ� ���� ��尡 ���� ����� sibling�̾��ٸ�
	//���� ���� parent ������ �׷����� �ʰ� ���� ������ �׷����� �Ѵ�.
	//�Ǽ����� �׸��ٸ� parent�κ��� �׷��� ���������
	//�������� �׸� ���� ������ �������� �Ǽ�ó�� ���̰� �Ǵ� ������ ���� �� �ִ�.
	CRect	line_rect;		//�׷��� ���� ��ǥ
	CRect	prev_rect;		//���� �׸� �簢��
	int		prev_depth = -1;//���� �׸� ����

	tree<CTreeItem>::pre_order_iterator it = m_tr.begin();
	tree<CTreeItem>::pre_order_iterator end = m_tr.end();
	CTreeItem item;
	CRect text_rect;
	
	
	if (!m_tr.is_valid(it))
		return;

	int oldBkMode = dc.SetBkMode(TRANSPARENT);

	while (it != end)
	{
		//�ֻ��� ��Ʈ�̸� �н�
		if (m_tr.parent(it) == NULL)
		{
			it++;
			continue;
		}

		//ȭ�� ���� ����� �������� �׸� �ʿ䰡 ����.
		//��, ������ ǥ���� ���� ���������� �������� ������ �ְ�
		//�Ʒ��� �������� ���α����� �׷���� �ϹǷ� ���,�ϴܿ� �� ������ ������ ����.
		if ((rect.top > rc.bottom + m_line_height) ||
			(rect.bottom < rc.top - m_line_height))
		{
			//ȭ�� �Ʒ��� ����� �������� ������ �� �̻� ������ �ʿ� ����.
			if (rect.top > rc.bottom + m_line_height)
				break;

			//Ʈ���� ������ �����ߴٸ� ������ �����Ѵ�.
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
			//��Ʈ�� ������ ũ�⿡ ���� ���� �߾ӿ� ǥ�õ��� �ʴ� ��Ʈ�� �ִ�.
			//�̸� �������ַ� ������ font matrics�� �˾ƾ� �ҵ��ؼ� �ϴ� ��ŵ�Ѵ�.
			if (item.get_selected())
			{
				//italicü�� ����� ��� �ؽ�Ʈ ��� ������ right�� ª�� ��찡 �ִ�.
				//�켱 �Ʒ� �� ���� �̿��ؼ� ������ �ش�.
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

			//���� �׸��� �׷��� �� �� ���� ������ ���踦 ����ؼ� ������ �׷��ش�.
			if (m_use_node_line && it.node->parent != NULL)
			{
				line_rect = get_item_rect(it.node->parent, false);

				//if (m_use_expand_button)
					line_rect.left += (m_expand_button_size/2);

				line_rect.right = rect.left;	//rect.left�� ���� ����� indent�� ����� ��ǥ��.

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

		//���ϵ尡 ���� ���
		//��ҵ� ����̸� +�� �׷��ְ� it�� next sibling���� �ű��
		//Ȯ��� ����̸� -�� �׷��ְ� it++ ��Ų��.
		//���ϵ尡 ���� ��� �׳� it++ ��Ų��.
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

				//���� ��尡 ��ҵ� ��� next_sibling�� ���������� �ö󰣴�.
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

				//top���� �ö󰬴ٸ� �� �̻� �׸��� ���� ����̴�.
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

		//���� ��带 �׸��� ���� ��� �簢���� ��ġ�� �̵���Ų��.
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
	//custom control�̶� �׷��� ���콺�� Ŭ���ص� ��Ŀ���� ������ �ʴ´�.
	//��Ŀ���� �Ȱ����� ���� Ű ó���� ���� �ʴ´�.
	//������ �˻��غ����� ���� ã�� ���ؼ� �켱 ������ ��Ŀ���� ������ �ߴ�.
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

	//üũ�ڽ��� ������ üũ�� ��۵ǰ� �� ���� ������ ������ �ش� �׸��� selected�� �ȴ�.
	CRect rect = get_item_rect(it, false);
#if 0
	CClientDC dc(this);
	DrawRectangle(&dc, rect, RED);
	return;
#endif	

	//+, - ��ư�� ���ȴ���
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

	//������ �� �ؽ�Ʈ�� ������ ����̴�.
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

//�� �Լ������� m_lf ������ �̿��ؼ� ��Ʈ�� ������Ѵ�.
//��, m_lf.lfHeight ���� �̿��ؼ� ��Ʈ�� ��������Ƿ�
//m_font_size ��� ������ ���� ����Ǿ��ٸ�
//������ �̿��� �̸� m_lf.lfHeight ������ ������ �� �� �Լ��� ȣ��Ǿ�� �Ѵ�.
//m_lf.lfHeight���� ���밪�� MM_TEXT����� ���� DC������ ���� �ȼ�ũ�Ⱑ �ȴ�.
//���� ��ũ�� ũ�� ���� ����Ҷ��� m_font_size�� �̿��ϴ°� �ƴ϶�
//m_lf.lfHeight���� �̿��ؾ� ��Ȯ�� ��ũ�� ũ�Ⱑ ���ȴ�.
//m_font_size�� ���� ����ڿ��� �Ϲ����� ��Ʈ ũ�� ���� ��ġ�� ���̴� �������� ��ġ�̴�.
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

//���� ��� ��Ʈ ũ�⸦ 10���� �����ϸ�
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
	//�ڱ� �ڽſ��� �ο��� ��Ʈ�� ���ٸ� null�� ���ϵȴ�.
	//dlg�� parent�� font�� ���;� �Ѵ�.
	CFont* font = GetParent()->GetFont();

	if ( font != NULL )
		font->GetObject(sizeof(m_lf), &m_lf);
	else
		GetObject(GetStockObject(SYSTEM_FONT), sizeof(m_lf), &m_lf);

	reconstruct_font();

	//WS_TABSTOP�� �־��൵ ��Ʈ�� Ŭ���� Focus�� ������ �ʴ´�.
	//�켱 Ŭ���� SetFocus()�� ������ Focus�� �ǳ�����.
	ModifyStyle(NULL, WS_BORDER | WS_EX_CLIENTEDGE/*| WS_HSCROLL | WS_VSCROLL*/);

	CWnd::PreSubclassWindow();
}

//����!
//���� index�� 1�̰� ���� index�� 0�� ��ҵ� ���¶�� �ϸ�
//0�� ù��° child�� ���ϵǴ� ������ �߻��Ѵ�.
//���� ���� index�� prev_sibling�� ��ҵ� ���¶�� �ϸ�
//���� index�� prev_sibling�� next_sibling�� �Ǿ�� �Ѵ�.
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
				//���� ��尡 ��ҵǾ� �ִٸ�
				//���� ����� children���� Ž���� �ʿ䰡 ������
				//���� ����� next_sibling���� �ǳʶڴ�.
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
				//���� ��尡 ��ҵǾ� �ִٸ�
				//�� �ȿ� �ִ� children���� Ž���� �ʿ䰡 ������
				//parent�� next_sibling���� �ǳʶڴ�.
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
//Ȯ��, ��Ҹ� ����Ͽ� ���� �׸��� �����Ѵ�. ���� ��尡 ��ҵ� ���¶�� ���� ���� ���� ����� ���ϵ尡 �ƴ� sibling�� �ȴ�.
tree<CTreeItem>::pre_order_iterator CTreeCtrlEx::get_next_visible_item(tree<CTreeItem>::pre_order_iterator it)
{
	tree<CTreeItem>::pre_order_iterator end = m_tr.end();

	if (!m_tr.is_valid(it))
		return NULL;

	if (m_tr.parent(it) != NULL)
	{
		if (it->expanded == false)
		{
			//���� ��尡 ��ҵǾ� �ִٸ�
			//�� �ȿ� �ִ� children���� Ž���� �ʿ䰡 ������
			//parent�� next_sibling���� �ǳʶڴ�.
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

//Ȯ��, ��Ҹ� ����Ͽ� ���� �׸��� �����Ѵ�. ���� ����� �ٷ� ������ ã���� �� ���� collapsed�� �� �ִ�.
//���� ����� prev_sibling�� ã�Ƽ� �������鼭 get_next_visible_item�� ã�ƾ��Ѵ�.
tree<CTreeItem>::pre_order_iterator CTreeCtrlEx::get_previous_visible_item(tree<CTreeItem>::pre_order_iterator it)
{
	tree<CTreeItem>::pre_order_iterator iter = NULL;
	tree<CTreeItem>::pre_order_iterator next = NULL;

	//���� ����� ���� ��尡 �ִٸ� �ݵ�� �����Ѵ�.
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
	//���� ����� ���� ��尡 ���ٸ� parent�� first_child�� ����.
	//��, ���� ��尡 ��Ʈ�� �ƴϾ�� �Ѵ�.
	else if (m_tr.depth(it) > 1)
	{
		if (it.node->parent->parent != NULL)
			return it.node->parent;
	}

	return NULL;
}


//��ҵ� ��带 �����ϰ� ȭ�鿡 ǥ���ؾ� �ϴ� �� ����� ����. ��ũ�� ũ�� ����ÿ� ���ȴ�.
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
				//���� ��尡 ��ҵǾ� �ִٸ�
				//���� ����� children���� Ž���� �ʿ䰡 ������
				//���� ����� next_sibling���� �ǳʶڴ�.
				while (true)
				{
					if (it.node->next_sibling)
					{
						it = it.node->next_sibling;
						break;
					}
					else
					{
						//���� �׸��� ���̰� 1�ε� next_sibling�� ���ٸ� �� ������ ���ϵ� �׸��̴�.
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


//findText�� ""�̸� ��� ��带 ��ȸ�ؼ� check�� 0�Ǵ� 1�̶�� üũ ����.
//findText�� ""�� �ƴϸ� �ش� �����۸� ã�Ƽ� check.
//��, check�� 0�Ǵ� 1�� �ƴ϶�� �ܼ��� findText�� ��带 ã���� �ϴ� �������� �̿�ȴ�.
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

//findText�� ""�̸� ��� ��带 ��ȸ�ؼ� check�� 0�Ǵ� 1�̶�� üũ ����.
//findText�� ""�� �ƴϸ� �ش� �����۸� ã�Ƽ� check.
//��, check�� 0�Ǵ� 1�� �ƴ϶�� �ܼ��� findText�� ��带 ã���� �ϴ� �������� �̿�ȴ�.
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

//it ��� ���� �׸���߿��� üũ�� �׸���� �����Ѵ�.
//include_current_node	: ���� ����� üũ ���ε� ������ ��
//only_current_group	: ���� ��峻�� ���鸸 �˻��� ��. false�̸� Ʈ�� ������ �˻��Ѵ�.
//include_child_nodes	: ������ ���� ���鵵 �˻��� ��. false�̸� first_sibling�鸸 �˻��Ѵ�.
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

//���õ� �׸��� ���ڿ����� deque�� �־ �����Ѵ�.
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
			//expand button ����̶�� child�� �ֵ� ���� ������ �� ũ�� ��ŭ�� ������ �����Ѵ�.
			rect.left += (m_use_expand_button ? m_expand_button_size : 0);
			rect.left += (m_use_check_box ? ((m_tr.depth(it) > 1 || m_check_box_at_root) ? m_check_box_size : 0) : 0);
			rect.left += (m_use_image_list ? m_image_list_size : 0);
		}

		//�ֻ��� ��Ʈ�̸� �н�
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

	//�ֱ� �������� ��ũ�ѹٴ� �� ���� ũ�⿡ ���� thumb�� ũ�Ⱑ �ڵ� ����Ǵµ�
	//�׷��߸� thumb�� ũ�� ǥ�õǾ� �巡���ϱ� �������� �����̴�.
	//�̷��� �����ϱ� ���ؼ��� nPage�� ���� ���������� �����ϰ�
	//nMax = realMax + si.nPage - 1;�� �ָ� ���ϴ� ������ ���۵ȴ�.
	//���� ������ �̵��� �ʿ��� ���� �� ���� �۰� ��� �����ϰ� ��ũ���� �� �ִ�.
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
		case SB_LINEUP	:	//��ũ�� ���� ����(����) ȭ��ǥ�� Ŭ���� ���
			si.nPos -= m_line_height;
			break;
		case SB_PAGEDOWN	:
			si.nPos += m_line_height * m_scroll_unit;
			break;
		case SB_LINEDOWN	:	//��ũ�� ���� ������(�Ʒ�) ȭ��ǥ�� Ŭ���� ���
			si.nPos += m_line_height;
			break;
		case SB_THUMBPOSITION :	//Ʈ���� ��� �̵��� ��
		case SB_THUMBTRACK	:	//Ʈ���� ��� �����̴� ����
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

	//���⼭ �⺻ �����ϸ� parent������ ó���� �̾�����.
	//���⼭ ���� ó���� �Ϸ�Ǿ����� �˸��� ���� TRUE�� �������ش�.
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

//image_list�� �׽�Ʈ�ϱ� ���� �� ��忡 �������� �̹��� �ε����� �ο��Ѵ�.
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