// CSCTreeCtrl.cpp: 구현 파일
//

#include "SCTreeCtrl.h"
#include <thread>
#include "../../Functions.h"
#include "../../MemoryDC.h"
#include "../../SCGdiPlusBitmap.h"
#include "Common/drag_scroll_message.h"		//20260707 by claude. 드래그 자동스크롤을 대상 타입 무관 메시지로 위임 — CVtListCtrlEx 하드 의존 제거(점진 이행용).

#include <fstream>
// CSCTreeCtrl

IMPLEMENT_DYNAMIC(CSCTreeCtrl, CTreeCtrl)

#define WM_TREE_END_EDIT		(WM_USER + 9013)
CSCTreeCtrl* CSCTreeCtrl::s_editing_tree = NULL;

CSCTreeCtrl::CSCTreeCtrl()
{
	memset(&m_lf, 0, sizeof(LOGFONT));
}

CSCTreeCtrl::~CSCTreeCtrl()
{
}

void CSCTreeCtrl::OnDestroy()
{
	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	if (m_pEdit)
	{
		m_pEdit->DestroyWindow();
		delete m_pEdit;
		m_pEdit = NULL;
	}

	//각 노드에 동적 할당된 데이터가 있다면 해제시켜야 한다.
	//release_iterator(GetRootItem());

	CTreeCtrl::OnDestroy();
}

//하위 노드까지 재귀적으로 순회하면서 각 노드에 동적 할당된 데이터를 해제시키고자 아래 함수를 추가했으나
//정상적으로 해제되지 않는다.
//할당을 parent에서 하고 해제를 여기서 해서 그런건지는 확인이 필요하다.
//parent에서의 해제는 정상적으로 동작한다.
void CSCTreeCtrl::release_iterator(HTREEITEM hItem)
{
	for (HTREEITEM hNext = hItem; hNext; hNext = GetNextItem(hNext, TVGN_NEXT))
	{
		DWORD_PTR item = GetItemData(hNext);
		if (item)
		{
			delete reinterpret_cast<void*>(item);
		}

		release_iterator(GetChildItem(hNext));
	}
}

BEGIN_MESSAGE_MAP(CSCTreeCtrl, CTreeCtrl)
	ON_WM_ACTIVATE()
	ON_WM_ERASEBKGND()
	ON_WM_KEYDOWN()
	//ON_WM_PAINT()
	ON_NOTIFY_REFLECT_EX(TVN_SELCHANGING, &CSCTreeCtrl::OnTvnSelchanging)
	ON_NOTIFY_REFLECT_EX(TVN_SELCHANGED, &CSCTreeCtrl::OnTvnSelchanged)
	ON_WM_WINDOWPOSCHANGED()
	ON_NOTIFY_REFLECT_EX(TVN_ITEMEXPANDING, &CSCTreeCtrl::OnTvnItemexpanding)
	ON_NOTIFY_REFLECT_EX(NM_CLICK, &CSCTreeCtrl::OnNMClick)
	ON_NOTIFY_REFLECT_EX(TVN_BEGINDRAG, &CSCTreeCtrl::OnTvnBegindrag)
	ON_NOTIFY_REFLECT_EX(NM_DBLCLK, &CSCTreeCtrl::OnNMDblclk)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_HSCROLL()
	ON_NOTIFY_REFLECT_EX(TVN_BEGINLABELEDIT, &CSCTreeCtrl::OnTvnBeginlabeledit)
	ON_NOTIFY_REFLECT_EX(TVN_ENDLABELEDIT, &CSCTreeCtrl::OnTvnEndlabeledit)
	ON_NOTIFY_REFLECT_EX(TVN_ITEMEXPANDED, &CSCTreeCtrl::OnTvnItemexpanded)
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, &CSCTreeCtrl::OnNMCustomDraw)
	ON_WM_TIMER()
	ON_WM_CONTEXTMENU()
	ON_REGISTERED_MESSAGE(Message_CSCMenu, &CSCTreeCtrl::OnMessageCSCMenu)
	ON_REGISTERED_MESSAGE(Message_CSCScrollbar, &CSCTreeCtrl::on_message_CSCScrollbar)
	ON_WM_VSCROLL()
	ON_WM_MOUSEHWHEEL()
	ON_WM_MOUSEWHEEL()
	ON_WM_MOUSEHOVER()
	ON_WM_MOUSELEAVE()
	ON_WM_GETMINMAXINFO()
	ON_COMMAND_RANGE(menu_add_item, menu_favorite, &CSCTreeCtrl::OnPopupMenu)
	ON_REGISTERED_MESSAGE(Message_CSCStaticEdit, &CSCTreeCtrl::on_message_CSCStaticEdit)
	ON_REGISTERED_MESSAGE(Message_DragScrollBy, &CSCTreeCtrl::on_message_DragScrollBy)
	ON_MESSAGE(WM_TREE_END_EDIT, &CSCTreeCtrl::on_end_edit_posted)
	ON_WM_DESTROY()
	ON_WM_NCPAINT()
	ON_WM_NCCALCSIZE()
	ON_WM_STYLECHANGING()
END_MESSAGE_MAP()



// CSCTreeCtrl 메시지 처리기
void CSCTreeCtrl::PreSubclassWindow()
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

	// 원하는 색상으로 테두리 그리기
	DWORD dwStyle = GetStyle();
	DWORD exStyle = GetExStyle();

	//native TVS_TRACKSELECT(hot-tracking) 제거 — 이 스타일이 켜져 있으면 native 컨트롤이 hover 항목을
	//하이퍼링크처럼 밑줄(underline) 로 그린다. 커스텀 드로우가 보통은 CDRF_SKIPDEFAULT 로 억제하지만,
	//부분 invalidate 로 item rc 가 빈 순간 CDRF_DODEFAULT 로 빠지면 native 밑줄이 새어 보인다(hover 시 간헐 underline).
	//hover 강조는 이 컨트롤이 m_hot_item(full-row) 으로 직접 처리하므로 native hot-track 은 불필요 → 제거.
	ModifyStyle(TVS_TRACKSELECT, 0);

	//분명 WS_BORDER 스타일이 있음에도 불구하고 (GetStyle() & WS_BORDER) 값은 false로 나온다.
	//exStyle까지 함께 체크하니 리소스 에디터에서 테두리 설정 여부에 따라 정상 동작한다.
	if ((GetStyle() & WS_BORDER) || (exStyle & WS_EX_CLIENTEDGE))
	{
		m_draw_border = true;

		//기본 border 제거 — default OnNcPaint 는 이후 scrollbar/corner 만 그리고
		//border 는 m_draw_border 에 따라 우리가 직접 그림.
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



	CTreeCtrl::PreSubclassWindow();

	//native CTreeCtrl 의 default item paint 가 native bg color 로 row 를 칠한 후 우리 customdraw POSTPAINT 가
	//덮어쓰는 구조. native bg color 가 default (COLOR_WINDOW = 흰색) 면 splitter rapid resize 시 흰색 깜빡임 가시화 —
	//native bg 를 theme cr_back 으로 동기화하면 default item paint 부터 theme 색이라 중간 frame 흰색 없음.
	//TVS_EX_DOUBLEBUFFER 가 BufferedPaint 로 전체 paint 를 atomic 합성 → 추가로 깜빡임 차단.
	SendMessage(TVM_SETEXTENDEDSTYLE, TVS_EX_DOUBLEBUFFER, TVS_EX_DOUBLEBUFFER);
	CTreeCtrl::SetBkColor(m_theme.cr_back.ToCOLORREF());

	setup_scrollbar();
}

void CSCTreeCtrl::reconstruct_font()
{
	m_font.DeleteObject();
	BOOL bCreated = m_font.CreateFontIndirect(&m_lf);
	SetFont(&m_font, true);

	m_font_size = get_font_size_from_pixel_size(m_hWnd, m_lf.lfHeight);

	//20260705 by claude. 명시 지정(set_line_height, m_line_height>0)이 있으면 그 값으로 고정 — 폰트 변경으로 이 함수가 다시 불려도
	//라인 간격 유지. 없으면 기존대로 폰트픽셀높이+12 와 현재 높이 중 큰 값.
	int height = GetItemHeight();
	if (m_line_height > 0)
		SetItemHeight(m_line_height);
	else
		SetItemHeight(MAX(height, -m_lf.lfHeight + 12));

	m_content_width_dirty = true;	//폰트가 바뀌면 라벨 폭이 달라지므로 콘텐츠 폭 재측정 필요.

	ASSERT(bCreated);
}

//20260705 by claude. 항목(행) 높이를 명시 고정. reconstruct_font·이미지 설정이 m_line_height>0 이면 이 값을 그대로 SetItemHeight.
void CSCTreeCtrl::set_line_height(int height, bool invalidate)
{
	m_line_height = height;

	if (GetSafeHwnd() && height > 0)
		SetItemHeight(height);

	if (invalidate)
		Invalidate();
}

BOOL CSCTreeCtrl::PreTranslateMessage(MSG* pMsg)
{
	//20260705 by claude. 드래그 중 Ctrl/Shift 눌림·뗌 = 이동↔복사 토글 → 마우스가 안 움직여도 문구를 즉시 갱신.
	//(GetAsyncKeyState 는 이 시점의 실시간 키 상태를 반영. 소비하지 않고 계속 진행.)
	if ((pMsg->message == WM_KEYDOWN || pMsg->message == WM_KEYUP) &&
		(pMsg->wParam == VK_CONTROL || pMsg->wParam == VK_SHIFT) && m_bDragging)
	{
		refresh_drag_hint();
	}

	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	if (pMsg->message == WM_KEYDOWN)
	{
		TRACE(_T("CSCTreeCtrl::PreTranslateMessage() WM_KEYDOWN\n"));
		switch (pMsg->wParam)
		{
			case VK_F2:
			{
				//편집을 컨트롤 내부에서 처리하는게 편할수도 있지만 때로는 편집을 메인에서 관여해야 하는 경우도 존재한다.
				//실제 label data와 UI상에 표시하는 label이 다를 경우, CSManager에서 그룹명 옆에 에이전트 개수를 표시하는 기능 등...
				//아래와 같이 SendMessage()를 통해서 이벤트를 전달해도 되고 메인에서 직접 F2키에 대한 처리를 추가해도 된다.
				::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCTreeCtrl, (WPARAM)&(CSCTreeCtrlMessage(this, message_edit_item, NULL)), (LPARAM)0);
				return FALSE;
			}

			case VK_RETURN:
			{
				if (m_in_editing)
				{
					edit_end(true);
					return TRUE;
				}
				break;
			}
			case VK_ESCAPE:
			{
				if (m_bDragging)	//드래그 중 ESC = 드롭 없이 드래그 완전 취소(위험 방지).
				{
					cancel_drag();
					return TRUE;
				}
				if (m_in_editing)
				{
					edit_end(false);
					return TRUE;
				}
				break;
			}
			case VK_DELETE :
			{
				/*
				if (m_in_editing)
					return FALSE;

				//여기서 삭제하는 것은 매우 위험하다. 메인에서 처리해야 한다.
				//delete_item();

				return TRUE;
				*/
				break;
			}
		}
	}

	return CTreeCtrl::PreTranslateMessage(pMsg);
}


void CSCTreeCtrl::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	CTreeCtrl::OnActivate(nState, pWndOther, bMinimized);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
}


BOOL CSCTreeCtrl::OnEraseBkgnd(CDC* pDC)
{
	//native bg color 를 SetBkColor 로 theme 색 설정 + TVS_EX_DOUBLEBUFFER 가 buffer 합성 시 native bg 로 fill.
	//여기서 직접 PatBlt 하면 buffer 아닌 화면 DC 에 fill 되어 splitter rapid resize 시 깜빡임 원인이 됨.
	return TRUE;
}


void CSCTreeCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	CTreeCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}

//수동으로 직접 그려줄 경우는 BEGIN_MESSAGE_MAP에서 ON_WM_PAINT() 주석을 해제하면
//OnPaint()가 호출되고 여기서 직접 그릴 수 있다.
//폰트 색상 등의 UI를 자유롭게 변경할 수 있으나 스크롤 처리 등이 복잡해진다.
//GetItemRect()를 이용해서 그려주니 스크롤도 자동 처리된다.
//하지만 여전히 확장버튼, 체크박스, 아이콘 등 그 위치를 정확히 얻기 힘들고
//스크롤 처리도 힘들다. 이를 처리하기 위해서는 많은 처리가 필요하게 되어
//WM_PAINT를 사용하지 않고 CustomDraw 방식으로 변경함
/*
void CSCTreeCtrl::OnPaint()
{
	//tree가 제대로 구성되어 있는지 기본 OnPaint()로 확인 목적 코드
	if (m_use_custom_draw == false)
	{
		CTreeCtrl::OnPaint();
		return;
	}

	CPaintDC dc1(this); // device context for painting
					   // TODO: 여기에 메시지 처리기 코드를 추가합니다.
					   // 그리기 메시지에 대해서는 CTreeCtrl::OnPaint()을(를) 호출하지 마십시오.
	CRect rc;
	GetClientRect(rc);

	CMemoryDC dc(&dc1, &rc);
	Gdiplus::Graphics g(dc.m_hDC, rc);

	COLORREF	crText = m_cr_text;
	COLORREF	crBack = m_cr_back;

	dc.FillSolidRect(rc, m_cr_back);

	CFont font;
	CFont* pOldFont;

	font.CreateFontIndirect(&m_lf);
	pOldFont = (CFont*)dc.SelectObject(&font);

	dc.SetBkMode(TRANSPARENT);

	HTREEITEM hItem = GetRootItem();

	int		tab_count = 0;
	int		image_count = 0;
	int		image_index = -1, image_selected_index = -1;
	CRect	rItem[5];
	CString label;

	if (!m_is_shell_treectrl && m_use_own_imagelist)
	{
		if (m_imagelist.GetSafeHandle())
			image_count = m_imagelist.GetImageCount();
	}

	while (hItem)
	{
		crText = m_cr_text;

		//GetItemRect(hItem, rRow, FALSE);
		get_item_rect(hItem, rItem);
		label = GetItemText(hItem);

		//배경색을 그려주고
		if (GetItemState(hItem, TVIS_SELECTED) & TVIS_SELECTED)
		{
			dc.SetTextColor(red);

			//포커스에 따라 다르다. 단, 편집중일때도 CEdit이 focus를 가지게 되는데 inactive 색상 대신 원래의 선택배경색을 그대로 사용하자.
			//우클릭 메뉴 표시 중 (m_in_context_menu) 도 동일 — menu 가 focus 를 가져가도 right-clicked 항목은 active 색 유지.
			if (GetFocus() == this || m_in_editing || m_in_context_menu)
			{
				crText = m_cr_text_selected;
				DrawRectangle(&dc, (GetStyle() & TVS_FULLROWSELECT) ? rItem[rect_row] : rItem[rect_label], m_cr_selected_border, m_cr_back_selected);
			}
			else if (GetStyle() & TVS_SHOWSELALWAYS)
			{
				crText = m_cr_text_selected_inactive;
				dc.FillSolidRect((GetStyle() & TVS_FULLROWSELECT) ? rItem[rect_row] : rItem[rect_label], m_cr_back_selected_inactive);
			}
		}


		//확장버튼을 그려주고
		if (rItem[rect_button].Width() > 0)//GetStyle() & TVS_HASBUTTONS)
		{
			//SetItemHeight() 또는 m_image_size가 16이 아니어도 확장버튼의 크기는 동일해야 한다.
			CRect expand_rect = makeCenterRect(rItem[rect_button].left + 16 / 2 - 1, rItem[rect_button].CenterPoint().y, 16, 16);
			expand_rect.DeflateRect(4, 4, 3, 3);

			if (ItemHasChildren(hItem))
			{
				//확장되어 있으면 '-' 버튼을 
				if (GetItemState(hItem, TVIF_STATE) & TVIS_EXPANDED)
				{
					DrawRectangle(&dc, expand_rect, GRAY(128));
					expand_rect.DeflateRect(3, 3);
					DrawLine(&dc, expand_rect.left - 1, expand_rect.CenterPoint().y, expand_rect.right, expand_rect.CenterPoint().y, GRAY(128));
				}
				//축소되어 있으면 '+' 버튼을 그려준다.
				else
				{
					DrawRectangle(&dc, expand_rect, GRAY(128));//, GRAY(128));
					expand_rect.DeflateRect(3, 3);
					DrawLine(&dc, expand_rect.left - 1, expand_rect.CenterPoint().y, expand_rect.right, expand_rect.CenterPoint().y, GRAY(128));
					DrawLine(&dc, expand_rect.CenterPoint().x, expand_rect.top - 1, expand_rect.CenterPoint().x, expand_rect.bottom, GRAY(128));
				}
			}
		}

		//체크박스를 그려주고
		if (rItem[rect_check].Width() > 0)//GetStyle() & TVS_CHECKBOXES)
		{
			//체크박스도 그 크기는 16으로 동일하다.
			CRect rCheck = makeCenterRect(rItem[rect_check].left + 16 / 2, rItem[rect_check].CenterPoint().y, 16, 16);
			rCheck.DeflateRect(2, 2, 2, 2);

#if 1
			draw_checkbox(&dc, rCheck, GetCheck(hItem));
#else
			DrawRectangle(&dc, rCheck, GRAY128, white);

			//3D border가 그려져서 별로다. <uxtheme.h>를 이용하여 윈도우에서 기본 제공하는 모습으로 표시 가능하나
			//우선 gdiplus로 직접 그려준다.
			//dc.DrawFrameControl(rCheck, DFC_BUTTON, GetCheck(hItem) ? DFCS_BUTTONCHECK | DFCS_CHECKED : DFCS_BUTTONCHECK);
			if (GetCheck(hItem))
			{
				Pen pen(Color(255, 64, 64, 64), 1.51);

				g.DrawLine(&pen, rCheck.left + 1, rCheck.CenterPoint().y - 2, rCheck.left + 4, rCheck.CenterPoint().y + 2);
				g.DrawLine(&pen, rCheck.left + 4, rCheck.CenterPoint().y + 3, rCheck.right - 2, rCheck.top + 2);
			}
#endif
		}

		//아이콘을 그려주고
		if (m_is_shell_treectrl)
		{
			GetItemImage(hItem, image_index, image_selected_index);
			image_index = m_pShellImageList->GetSystemImageListIcon(get_path(GetSelectedItem()), true);
			m_pShellImageList->m_imagelist_small.Draw(&dc, image_index,
				CPoint(rItem[rect_icon].left, rItem[rect_icon].CenterPoint().y - m_image_size / 2), ILD_TRANSPARENT);
		}
		else if (m_use_own_imagelist && m_imagelist.GetSafeHandle() && (image_index < m_imagelist.GetImageCount()))
		{
			GetItemImage(hItem, image_index, image_selected_index);
			if (image_index >= 0 && image_index < image_count)
			{
				m_imagelist.Draw(&dc, image_index, CPoint(rItem[rect_icon].left, rItem[rect_icon].CenterPoint().y - m_image_size / 2), ILD_TRANSPARENT);
				rItem[rect_icon].left += m_image_size;
			}
		}

		dc.SetTextColor(crText);
		dc.DrawText(label, rItem[rect_label], DT_LEFT | DT_SINGLELINE | DT_VCENTER);

		//현재 노드가 EXPAND 상태이고 child가 존재한다면 child로 이동
		if (get_item_state(hItem, TVIS_EXPANDED) && ItemHasChildren(hItem))
		{
			tab_count++;
			hItem = GetChildItem(hItem);

			continue;
		}
		//EXPAND상태가 아니거나 child가 없다면 현재 node의 sibling으로 이동한다.
		else
		{
			HTREEITEM hSiblingItem = GetNextSiblingItem(hItem);
			if (hSiblingItem)
			{
				hItem = hSiblingItem;
				continue;
			}
			else
			{
				//만약 sibling도 없다면 parent의 sibling부터 다시 탐색한다.
				while (true)
				{
					tab_count--;
					hItem = GetParentItem(hItem);

					//parent가 NULL인 경우는 최상위 node들인 경우이므로 모든 탐색을 중지한다.
					if (hItem == NULL)
						return;

					//sibling이 있으면 그 node부터 다시 탐색을 시작하고 없다면 다시 parent의 sibling을 찾는다.
					hSiblingItem = GetNextSiblingItem(hItem);
					if (hSiblingItem)
					{
						hItem = hSiblingItem;
						break;
					}
				}
			}
		}
	}
#if 0
	for (int i = 0; i < m_folder_list.size(); i++)
	{
		GetItemRect(m_folder_list[i].item, &r, true);
		GetItemRect(m_folder_list[i].item, &rRow, false);

		r.right = rc.right - 1;

		//rc와 일부분도 겹치지 않는 아이템은 그리지 않는다.
		if (getIntersectionRect(rc, CRect(r.left - 32, r.top, r.right, r.bottom)).IsRectEmpty())
			continue;

		long t0 = getClock();

		indent = get_char_count(m_folder_list[i].fullpath, '\\');
		
		if (indent > 0)
		{
			if (m_folder_list[i].fullpath.GetLength() > 3)
				indent++;
		}

		//선택된 항목은 다른 색상으로 표시한다.
		//if (GetItemState(m_folder_list[i].item, TVIS_DROPHILITED) & TVIS_DROPHILITED)//m_folder_list[i].item == GetDropHilightItem())
		if (GetDropHilightItem() == m_folder_list[i].item)
		{
			TRACE(_T("drophilited\n"));
			//GetItemState(m_folder_list[i].item, TVIS_DROPHILITED))
			crText = m_cr_text_dropHilited;
			dc.FillSolidRect(rRow, m_cr_back_dropHilited);
		}
		else if (GetSelectedItem() == m_folder_list[i].item)
		//else if (GetItemState(m_folder_list[i].item, TVIS_SELECTED) & TVIS_SELECTED)
		{
			TRACE(_T("selected\n"));

			//포커스에 따라 다르다.
			if (GetFocus() == this)
			{
				crText = m_cr_text_selected;
				DrawRectangle(&dc, rRow, m_cr_selected_border, m_cr_back_selected);
			}
			else if ((GetStyle() & TVS_SHOWSELALWAYS))
			{
				crText = m_cr_text_selected_inactive;
				dc.FillSolidRect(rRow, m_cr_back_selected_inactive);
			}
		}


		//텍스트를 출력할 때 DT_NOCLIP을 주지 않으면 가로 스크롤시에 잔상이 남게된다.
		dc.SetTextColor(crText);
		dc.DrawText(_T(" ") + m_folder_list[i].folder, r, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);

		//텍스트 왼쪽에 아이콘을 그려주고
		icon_index = m_pShellImageList->GetSystemImageListIcon(m_folder_list[i].fullpath, true);
		m_pShellImageList->m_imagelist_small.Draw(&dc, icon_index,
						CPoint(r.left - 16, r.CenterPoint().y - 8), ILD_TRANSPARENT);

		//그 왼쪽에는 확장/축소 버튼을 그려준다.
		if (has_sub_folders(m_folder_list[i].fullpath))
		{
			flags = GetItemState(m_folder_list[i].item, TVIF_STATE);
			CRect rArrow = r;
			rArrow.left = rArrow.left - 16 - 16;
			rArrow.right = rArrow.left + 16;
			CPoint cp(rArrow.CenterPoint());

			cp.Offset(-6, 0);

			//아래로 향한 화살표 표시
			if (flags & TVIS_EXPANDED)
			{
				DrawLine(&dc, cp.x - 4, cp.y - 2, cp.x + 1, cp.y + 3, GRAY192, 2);
				DrawLine(&dc, cp.x + 5, cp.y - 2, cp.x - 0, cp.y + 3, GRAY192, 2);
			}
			//일반상태의 > 화살표 표시
			else
			{
				DrawLine(&dc, cp.x - 2, cp.y - 4, cp.x + 3, cp.y + 1, GRAY192, 2);
				DrawLine(&dc, cp.x - 2, cp.y + 4, cp.x + 3, cp.y - 1, GRAY192, 2);
			}
		}

		long t1 = getClock();
		tTotal += (t1 - t0);
		//trace(_T("%ld : %s\n"), t1 - t0, m_folder_list[i].folder);
	}
#endif

	dc.SelectObject(pOldFont);

	//TRACE(_T("total %ld\n"), tTotal);
}
*/

/*
void CSCTreeCtrl::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	CRect rcClip, rcClient;
	dc.GetClipBox(&rcClip);
	GetClientRect(&rcClient);

	// Set clip region to be same as that in paint DC
	CRgn rgn;
	rgn.CreateRectRgnIndirect(&rcClip);
	dc.SelectClipRgn(&rgn);
	rgn.DeleteObject();

	//COLORREF m_wndColor = m_cr_back;// GetSysColor(COLOR_WINDOW);

	dc.SetViewportOrg(0, 0);
	//dc.SetTextColor(m_wndColor);

	// First let the control do its default drawing.
	CWnd::DefWindowProc(WM_PAINT, (WPARAM)dc.m_hDC, 0);

	HTREEITEM hItem = GetFirstVisibleItem();
	int n = GetVisibleCount();

	// the most urgent thing is to erase the labels that were drawn by the tree
	while (hItem != NULL && n >= 0)
	{
		CRect rect;
		GetItemRect(hItem, &rect, TRUE);

		//dc.FillSolidRect(rect, Gdiplus::Color(255, 255, 0, 0));// m_cr_back);
		hItem = GetNextVisibleItem(hItem);
		n--;
	}

	// create the font
	CFont* pFontDC;
	CFont fontDC, boldFontDC;
	LOGFONT logfont;

	CFont* pFont = GetFont();
	pFont->GetLogFont(&logfont);

	fontDC.CreateFontIndirect(&logfont);
	pFontDC = dc.SelectObject(&fontDC);

	logfont.lfWeight = 700;
	boldFontDC.CreateFontIndirect(&logfont);

	// and now let's get to the painting itself
	UINT nDrawFormat = DT_CENTER;

	BOOL bFoundSpl = FALSE;
	int m_nSplIndex = 0;

	hItem = GetFirstVisibleItem();
	n = GetVisibleCount();
	while (hItem != NULL && n >= 0)
	{
		CRect rect;

		BOOL bBold = (GetParentItem(hItem) == NULL);
		BOOL bSplColor = FALSE;
		if (bBold && !bFoundSpl && m_nSplIndex >= 0)
		{
			HTREEITEM hTmpItem = GetRootItem();
			int index = 0;
			while (hTmpItem != NULL)
			{
				if (index == m_nSplIndex && hTmpItem == hItem)
				{
					bFoundSpl = TRUE;
					bSplColor = TRUE;
				}
				hTmpItem = GetNextSiblingItem(hTmpItem);
				index++;
			}
		}

		UINT selflag = TVIS_DROPHILITED | TVIS_SELECTED;

		if (!(GetItemState(hItem, selflag) & selflag))
		{
			dc.SetBkMode(TRANSPARENT);

			CString text = GetItemText(hItem);
			GetItemRect(hItem, &rect, TRUE);


			//dc.SetBkColor(m_cr_back);
			dc.SetTextColor(bSplColor ? Gdiplus::Color(255, 255, 0, 0) : Gdiplus::Color(255, 0, 0, 255));//m_cr_text);

			if (bBold)
				dc.SelectObject(&boldFontDC);

			dc.DrawText(text, rect, nDrawFormat);

			if (bBold)
				dc.SelectObject(&fontDC);
			//dc.SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));
		}
		else
		{
			GetItemRect(hItem, &rect, TRUE);

			COLORREF m_highlightColor = m_cr_back_selected;// ::GetSysColor(COLOR_HIGHLIGHT);
			CBrush brush(m_highlightColor);

			//dc.FillRect(rect, &brush);

			// draw a dotted focus rectangle
			dc.DrawFocusRect(rect);

			CString text = GetItemText(hItem);

			//dc.SetBkColor(m_highlightColor);
			dc.SetTextColor(m_cr_text_selected);// ::GetSysColor(COLOR_HIGHLIGHTTEXT));

			if (bBold)
				dc.SelectObject(&boldFontDC);

			dc.DrawText(text, rect, nDrawFormat);

			if (bBold)
				dc.SelectObject(&fontDC);
		}

		hItem = GetNextVisibleItem(hItem);
		n--;
	}

	dc.SelectObject(pFontDC);
}
*/

//아직 CSCTreeCtrl::OnTvnSelchanging(), OnTvnSelchanged() 함수는 별도의 코드는 처리하고 있지 않으나
//parent에서 자주 처리되는 이벤트이므로 ON_COMMAND_REFLECT_EX()로 선언하여
//parent에서도 처리할 수 있도록 한다.
//또한 아래와 같은 문제에 대한 처리를 위해서 함수 바디를 추가해 놓음.
// 
//리스트 항목을 편집을 완료하지 않고 트리의 제품 항목을 선택하면
//OnTvnSelChangedTree() 함수에서 새로 선택된 트리 항목이 선택되고
//편집된 항목의 값이 새 제품의 필드에 들어가는 오류가 발생한다.
//따라서 트리의 선택이 바뀌기 전에 parent::OnTvnSelchanging()에서 리스트의 편집을 종료시켜 줘야 한다.
BOOL CSCTreeCtrl::OnTvnSelchanging(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	*pResult = 0;
	return FALSE;
}

BOOL CSCTreeCtrl::OnTvnSelchanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	*pResult = 0;
	return FALSE;
}


void CSCTreeCtrl::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CTreeCtrl::OnWindowPosChanged(lpwndpos);
	sync_scrollbar();

	//size 변경시 — splitter drag 의 빠른 resize race 에서 invalidate 누락. 강제 redraw 동기 paint.
	//bErase=FALSE — OnEraseBkgnd 의 PatBlt 가 직접 화면에 dark gray fill 후 item 재그림 사이클이 splitter drag rapid
	//resize 에서 text 깜빡임 유발. native bg color (SetBkColor) + TVS_EX_DOUBLEBUFFER 가 atomic 합성하므로 erase pass 불필요.
	if (lpwndpos && !(lpwndpos->flags & SWP_NOSIZE) && ::IsWindow(m_hWnd))
	{
		Invalidate(FALSE);
		UpdateWindow();
	}
}

void CSCTreeCtrl::set_as_shell_treectrl(CShellImageList* pShellImageList, bool is_local, CString default_path)
{
	m_is_shell_treectrl = true;
	m_is_local = is_local;
	m_use_own_imagelist = true;
	m_image_size = 16;
	m_pShellImageList = pShellImageList;

	//20260705 by claude. shell 트리(탐색기 용도)는 윈도우11 탐색기와 유사하게 라인 간격을 고정(현재 28px. 리스트
	//set_as_shell_listctrl 의 set_line_height(26) 과 동일 패턴). m_line_height 를 세팅해 두면 이후 imagelist SetItemHeight·reconstruct_font 가 이 값을 존중.
	set_line_height(28, false);

	if (GetImageList(TVSIL_NORMAL) == NULL)
		SetImageList(m_pShellImageList->get_imagelist(true), TVSIL_NORMAL);

	refresh();

	set_path(default_path);
}

//드라이브 폴더를 다시 읽어들인다.
void CSCTreeCtrl::refresh(HTREEITEM hParent)
{
	if (hParent == m_computerItem)
		hParent = NULL;

	if (hParent == NULL)
	{
		DeleteAllItems();

		m_desktopItem = insert_special_folder(CSIDL_DESKTOP);
		m_documentItem = insert_special_folder(CSIDL_MYDOCUMENTS);
		m_computerItem = insert_special_folder(CSIDL_DRIVES);

		std::deque<CDiskDriveInfo>* drive_list = m_pShellImageList->m_volume[!m_is_local].get_drive_list();

		for (int i = 0; i < drive_list->size(); i++)
			insert_drive(drive_list->at(i));

		Expand(m_computerItem, TVE_EXPAND);
	}
	else
	{
		//Expand(hParent, TVE_COLLAPSE);

		delete_item(hParent, true);

		if (m_is_local)
		{
			insert_folder(hParent, get_path(hParent));
			//Expand(hParent, TVE_EXPAND);
		}
		else
		{
			//remote라면 요청해서 넣어야 한다.
			m_expanding_item = hParent;
			TRACE(_T("send message_request_folder_list, cur_path = %s\n"), get_path(m_expanding_item));
			::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCTreeCtrl, (WPARAM) & (CSCTreeCtrlMessage(this, message_request_folder_list, NULL)), (LPARAM)&get_path(hParent));
		}
	}
}

/*
//주어진 노드 아래 새 폴더를 생성하고 편집모드로 표시한다.
bool CSCTreeCtrl::new_folder(HTREEITEM hParent, CString new_folder_title, bool edit_mode)
{
	if (!m_is_shell_treectrl)
		return false;

	CString folder = convert_special_folder_to_real_path(get_path(), m_pShellImageList, !m_is_local);

	int index = get_file_index(folder, new_folder_title);


	if (index == 1)
		folder.AppendFormat(_T("\\%s"), new_folder_title.GetString());
	else
		folder.AppendFormat(_T("\\%s (%d)"), new_folder_title.GetString(), index);

	//실제 폴더를 생성한 후 리스트에 목록을 추가한다.
	BOOL res = CreateDirectory(folder, NULL);
	if (!res)
	{
		TRACE(_T("fail to create folder : %s. error = %d"), folder, GetLastError());
		return false;
	}

	folder = get_part(folder, fn_name);
	index = insert_folder(-1, folder);

	if (index < 0)
		return false;

	select_item(index, true, true, true);
	edit_item(index, 0);

	return true;
}
*/

//현재 폴더에서 "새 폴더" 생성 시 인덱스를 구한다. ex. "새 폴더 (2)"
int CSCTreeCtrl::get_file_index(CString path, CString new_folder_title)
{
	int i;
	int max_index = -1;
	std::set<int> idx_set;

	std::deque<WIN32_FIND_DATA> dq;
	find_all_files(path, &dq, _T("*"), true);

	CString folder_name;
	
	for (i = 0; i < dq.size(); i++)
	{
		if (dq[i].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			//fn_name이지만 fullpath가 폴더경로를 담고 있으므로 실제로는 맨 끝 폴더명을 리턴한다.
			folder_name = get_part(dq[i].cFileName, fn_name);

			if (folder_name == new_folder_title)
			{
				idx_set.insert(1);
				continue;
			}

			//끝 ')'를 찾고
			int start_paren = -1;
			int end_paren = folder_name.ReverseFind(')');
			int found_index = -1;

			if (end_paren > 0)
			{
				//시작 '('를 찾아서 그 사이의 숫자를 추출
				folder_name = folder_name.Left(end_paren);
				start_paren = folder_name.ReverseFind('(');

				if (start_paren > 0)
				{
					folder_name = folder_name.Mid(start_paren + 1);
					found_index = _ttoi(folder_name);
				}
			}

			if (found_index > 0)
				idx_set.insert(found_index);
		}
	}

	//set 항목 중 비어있는 인덱스를 리턴해준다.
	int index = 0;
	bool found = false;

	for (int elem : idx_set)
	{
		index++;
		if (elem != index)
		{
			found = true;
			break;
		}
	}

	//만약 1 ~ n까지 모든 순번이 순차적으로 들어있다면 1 증가된 값을 리턴해주면 된다.
	if (!found)
		index++;

	return index;
}

void CSCTreeCtrl::thread_insert_folders(HTREEITEM hItem)
{
	if (hItem)
	{
		TRACE(_T("%s\n"), GetItemText(hItem));
		hItem = GetNextItem(hItem, TVGN_CHILD);
		while (hItem)
		{
			iterate_tree(hItem);
			hItem = GetNextItem(hItem, TVGN_NEXT);
		}
	}
	else
	{
		HTREEITEM hItem = GetNextItem(NULL, TVGN_ROOT);
		while (hItem)
		{
			iterate_tree(hItem);
			hItem = GetNextItem(hItem, TVGN_NEXT);
		}
	}
}

HTREEITEM CSCTreeCtrl::insert_special_folder(int csidl)
{
	if (false)//m_osType == OS_MAC)
	{
		//TV_INSERTSTRUCT tvInsert;
		//tvInsert.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN;
		//tvInsert.item.iImage = m_pShellList->GetSystemImageListIcon(GetStringById(NFTD_IDS_COMPUTER));
		//tvInsert.item.iSelectedImage = m_pShellList->GetSystemImageListIcon(GetStringById(NFTD_IDS_COMPUTER));
		//tvInsert.item.cChildren = TRUE;
		//tvInsert.hInsertAfter = TVI_LAST;
		//tvInsert.hParent = TVI_ROOT;
		//tvInsert.item.pszText = (LPWSTR)(LPCWSTR)GetStringById(NFTD_IDS_ROOT);
		//return InsertItem(&tvInsert);
	}
	else
	{
		CString path = m_pShellImageList->m_volume[!m_is_local].get_path(csidl);
		CString text = m_pShellImageList->m_volume[!m_is_local].get_label(csidl);

		TV_INSERTSTRUCT tvInsert;
		tvInsert.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN;
		tvInsert.item.pszText = (LPTSTR)(LPCTSTR)text;
		tvInsert.item.iImage = m_pShellImageList->GetSystemImageListIcon(!m_is_local, csidl);
		tvInsert.item.iSelectedImage = m_pShellImageList->GetSystemImageListIcon(!m_is_local, csidl);

		//내 PC인 경우는 가상폴더이므로 물리적 path가 없다.
		if (PathFileExists(path))
			tvInsert.item.cChildren = has_sub_folders(path);
		else
			tvInsert.item.cChildren = 1;

		tvInsert.hInsertAfter = TVI_LAST;
		tvInsert.hParent = NULL;
		return InsertItem(&tvInsert);
	}
}

void CSCTreeCtrl::insert_drive(CDiskDriveInfo drive_info)
{
	TV_INSERTSTRUCT tvInsert;
	tvInsert.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN;

	if (m_is_local || (_tcsicmp(drive_info.path, _T("C:\\")) == 0))
	{
		tvInsert.item.iImage = m_pShellImageList->GetSystemImageListIcon(!m_is_local, drive_info.path);
		tvInsert.item.iSelectedImage = m_pShellImageList->GetSystemImageListIcon(!m_is_local, drive_info.path);
	}
	else
	{
		tvInsert.item.iImage = m_pShellImageList->get_drive_icon(drive_info.type);
		tvInsert.item.iSelectedImage = m_pShellImageList->get_drive_icon(drive_info.type);
	}

	tvInsert.hInsertAfter = TVI_LAST;
	tvInsert.hParent = m_computerItem;
	tvInsert.item.pszText = (LPTSTR)(LPCTSTR)drive_info.label;
	//local일 경우는 has_sub_folders()로 검사하지만 remote인 경우는 요청해서 얻어와야 한다.
	//단, 특정 폴더가 아닌 disk drive이므로 하위 폴더가 있다고 가정한다.
	tvInsert.item.cChildren = 1;// has_sub_folders(drive_info.path);
	HTREEITEM hItem = InsertItem(&tvInsert);

	//아래 코드는 드라이브 루트를 추가할 때 1레벨 하위 폴더까지 기본으로 추가
	//insert_folder(hItem, real_path);
}

void CSCTreeCtrl::insert_folder(HTREEITEM hParent, CString sParentPath)
{
	sParentPath = m_pShellImageList->convert_special_folder_to_real_path(!m_is_local, sParentPath);

	if (sParentPath.Right(1) != "\\")
		sParentPath += "\\";

	std::deque<WIN32_FIND_DATA> dq;
	find_all_files(sParentPath, &dq, _T("*"), true);

	bool folder_inserted = false;

	::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCTreeCtrl,
		(WPARAM) & (CSCTreeCtrlMessage(this, message_tree_processing, NULL, _T(""), _T(""), dq.size())), (LPARAM)(-1));

	//C:\Windows, C:\Windows\WinSxS 등과 같은 폴더는 그 갯수가 많으므로 parent에게 이를 알린다.
	for (int i = 0; i < dq.size(); i++)
	{
		::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCTreeCtrl,
						(WPARAM) & (CSCTreeCtrlMessage(this, message_tree_processing, NULL, _T(""), _T(""), dq.size())), (LPARAM)(i+1));

		if (dq[i].dwFileAttributes & FILE_ATTRIBUTE_HIDDEN ||
			dq[i].dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)
			continue;

		if (dq[i].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			insert_folder(hParent, &dq[i], has_sub_folders(m_pShellImageList->convert_special_folder_to_real_path(!m_is_local, dq[i].cFileName)));
			folder_inserted = true;
		}
	}

	//hParent에 insert_folder()가 수행되면 children 유무에 따라 확장버튼을 갱신시켜줘야 한다.
	TVITEM tvItem;
	memset(&tvItem, 0, sizeof(TVITEM));

	tvItem.mask = TVIF_HANDLE | TVIF_CHILDREN;
	tvItem.hItem = hParent;
	tvItem.cChildren = folder_inserted;
	SetItem(&tvItem);

	//자식이 새로 채워졌으니 가시항목·콘텐츠폭 캐시 재빌드 필요(refresh 로 같은 개수 재삽입 시 GetCount 변화만으론
	//감지 안 되는 경우 대비 — handle 이 바뀌므로 명시 무효화).
	m_visible_cache_dirty = true;
	m_content_width_dirty = true;
}

HTREEITEM CSCTreeCtrl::insert_folder(HTREEITEM hParent, WIN32_FIND_DATA* data, bool has_children, HTREEITEM hInsertAfter)
{
	TV_INSERTSTRUCT tvInsertItem;
	tvInsertItem.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN;
	tvInsertItem.item.iImage = m_pShellImageList->GetSystemImageListIcon(0, _T("C:\\windows"));
	tvInsertItem.item.iSelectedImage = m_pShellImageList->GetSystemImageListIcon(0, _T("C:\\windows")) + 1;
	tvInsertItem.hInsertAfter = hInsertAfter;
	tvInsertItem.hParent = hParent;

	//cFileName이 fullpath인 경우는 폴더명만 취해야 한다.
	if (get_char_count(data->cFileName, '\\') > 0)
	{
		TCHAR* p = data->cFileName;
		CString parent = get_path(hParent);
		parent = m_pShellImageList->convert_special_folder_to_real_path(!m_is_local, parent);
		if (parent.Right(1) == '\\')
			p += parent.GetLength();
		else
			p += (parent.GetLength() + 1);
		_tcscpy_s(data->cFileName, _countof(data->cFileName), p);
	}

	tvInsertItem.item.pszText = data->cFileName;

	//하위 폴더가 있을때만 확장버튼이 표시되도록.
	//remote일 경우는 일단 true로 세팅한다.
	if (m_is_shell_treectrl)
	{
		if (m_is_local)
			tvInsertItem.item.cChildren = has_sub_folders(m_pShellImageList->convert_special_folder_to_real_path(!m_is_local, concat_path(get_path(hParent), data->cFileName)));
		else
			tvInsertItem.item.cChildren = has_children;
	}

	HTREEITEM hItem = InsertItem(&tvInsertItem);

	//hParent는 확장버튼을 가진 노드로 표시되어야 한다.
	TVITEM tvItem;
	memset(&tvItem, 0, sizeof(TVITEM));

	tvItem.mask = TVIF_HANDLE | TVIF_CHILDREN;
	tvItem.hItem = hParent;
	tvItem.cChildren = true;
	SetItem(&tvItem);

	return hItem;
}

//is_greater_with_numeric(탐색기식 숫자 인식) 정렬 순서에 맞는 위치를 찾아 폴더 노드 1개를 삽입한다.
//열거(insert_folder(hParent, path))가 쓰는 std::sort 비교자와 동일 규칙이라, 단일 노드 삽입 후에도 정렬이 어긋나지 않는다.
HTREEITEM CSCTreeCtrl::insert_folder_sorted(HTREEITEM hParent, WIN32_FIND_DATA* data)
{
	//data->cFileName 이 fullpath 일 수 있으므로 leaf 폴더명으로 비교한다.
	CString name = (get_char_count(data->cFileName, '\\') > 0) ? get_part(data->cFileName, fn_leaf_folder) : CString(data->cFileName);

	//이미 오름차순 정렬된 형제들 사이에서 name 보다 큰 첫 형제 '앞'(=그 이전 형제 뒤)에 삽입.
	HTREEITEM hAfter = TVI_FIRST;
	for (HTREEITEM hChild = GetChildItem(hParent); hChild != NULL; hChild = GetNextSiblingItem(hChild))
	{
		if (is_greater_with_numeric(GetItemText(hChild), name))	//child > name → 여기서 멈추고 그 앞에 삽입
			break;
		hAfter = hChild;										//child <= name → name 은 이 형제 뒤로
	}

	return insert_folder(hParent, data, true, hAfter);
}

//local이면 drive_list를 NULL로 주고 remote이면 실제 리스트를 주고 갱신시킨다.
void CSCTreeCtrl::update_drive_list(CString thisPC, std::deque<CDiskDriveInfo>* drive_list)
{
	//local이 아닌데 drive_list가 NULL이면 잘못된 호출.
	if (!m_is_local && drive_list == NULL)
		return;

	HTREEITEM hItem = find_item(thisPC);
	DeleteItem(hItem);

	m_computerItem = insert_special_folder(CSIDL_DRIVES);

	for (int i = 0; i < drive_list->size(); i++)
		insert_drive(drive_list->at(i));

	Expand(m_computerItem, TVE_EXPAND);
}

/*
HTREEITEM CSCTreeCtrl::find_item(const CString& name)
{
	HTREEITEM root = GetRootItem();
	while (root)
	{
		HTREEITEM item = find_item(name, root);
		if (item)
			return item;

		root = GetNextSiblingItem(root);
	}

	return NULL;
}
*/

//item위치부터 child, sibling들을 탐색하여 label을 찾는다. 
HTREEITEM CSCTreeCtrl::find_item(const CString& label, HTREEITEM hItem)
{
	CString cur_label;

	if (hItem)
	{
		cur_label = GetItemText(hItem);
		//TRACE(_T("%s\n"), cur_label);
		if (cur_label == label)
			return hItem;

		hItem = GetNextItem(hItem, TVGN_CHILD);
		while (hItem)
		{
			HTREEITEM hFound = find_item(label, hItem);
			if (hFound)
				return hFound;

			hItem = GetNextItem(hItem, TVGN_NEXT);
		}
	}
	else
	{
		HTREEITEM hItem = GetNextItem(NULL, TVGN_ROOT);
		while (hItem)
		{
			HTREEITEM hFound = find_item(label, hItem);
			if (hFound)
				return hFound;

			hItem = GetNextItem(hItem, TVGN_NEXT);
		}
	}

	// return NULL if nothing was found
	return NULL;
}

//hItem의 첫 레벨 children만 검사한다. recursive를 사용하지 않는다.
HTREEITEM CSCTreeCtrl::find_children_item(const CString& label, HTREEITEM hParentItem)
{
	//hParentItem == NULL → root level 전체 순회 (GetRootItem() 으로 첫 root 부터 sibling 으로 진행).
	//기존 구현은 GetRootItem() 의 children 만 검색해 같은 level 의 다른 root 항목을 못 찾는 버그가 있었음.
	HTREEITEM hItem = (hParentItem == NULL) ? GetRootItem() : GetChildItem(hParentItem);

	while (hItem)
	{
		//파일 경로 / 시스템 라벨 매칭은 case-insensitive 가 일반적 — Windows path / drive letter 모두 case 무관.
		if (GetItemText(hItem).CompareNoCase(label) == 0)
			return hItem;
		hItem = GetNextSiblingItem(hItem);
	}

	return NULL;
}

CString CSCTreeCtrl::get_selected_item_text(bool include_parent)
{
	return get_item_text(GetSelectedItem(), include_parent);
}

CString CSCTreeCtrl::get_item_text(HTREEITEM hItem, bool include_parent, CString sep)
{
	CString label;
	if (hItem == NULL)
		return label;

	label = GetItemText(hItem);
	if (!include_parent)
		return label;

	while (hItem)
	{
		hItem = GetParentItem(hItem);
		if (hItem == NULL)
			break;

		CString parent_label = GetItemText(hItem);
		//parent 라벨이 이미 sep 로 끝나면 추가 sep 안 넣음 (drive "C:\" 처럼 trailing 분리자가 라벨에 포함된 경우 "C:\\\\path" 중복 회피).
		if (!parent_label.IsEmpty() && !sep.IsEmpty() &&
			parent_label.Right(sep.GetLength()) == sep)
			label = parent_label + label;
		else
			label = parent_label + sep + label;
	}

	return label;
}

//hItem = NULL인 경우는 모든 노드를 unselect로 만드는데 사용된다.
//NULL이 아닌 어떤 노드를 select상태로 만들지만 기존 selected 노드에는 영향을 주지 않는다.
void CSCTreeCtrl::select_item(HTREEITEM hItem)
{
	if (hItem == NULL)
	{
		hItem = GetSelectedItem();
		if (hItem)
		{
			SetItemState(hItem, 0, TVIS_SELECTED);
		}

		return;
	}

	SelectItem(hItem);
}

//hRoot의 하위 폴더들중에서 먼저 발견되는 label의 노드를 찾아서 선택상태로 표시한다.
//만약 find_label = "item1\\item2"와 같이 full 경로로 전달된다면 recursive하게 경로를 찾아간다.
//단, 이 방식은 top부터 순차적으로 label을 비교해서 찾게 되므로 서로 다른 위치에 동일한 label이 있을 경우에 대해서도
//테스트가 필요하다.
HTREEITEM CSCTreeCtrl::select_item(CString find_label, HTREEITEM hItem, CString sep)
{
	if (hItem == NULL)
		hItem = GetRootItem();

	int tab_count = 0;
	CString cur_label;

	//다단계 경로인지 확인하고
	int pos = find_label.Find(sep);
	CString find_path;
	
	//다단계라면 우선 '\\' 전까지의 경로를 찾아간다.
	if (pos > 0)
		find_path = find_label.Left(pos);
	else
		find_path = find_label;

	while (hItem)
	{
		cur_label = GetItemText(hItem);

		//현재 label이 찾고자 하는 label인 경우...
		if (cur_label.CompareNoCase(find_path) == 0)
		{
			//더 이상 '\\'가 발견되지 않으면 해당 항목을 선택항목으로 만들고 리턴하면 되고
			if (pos <= 0)
			{
				select_item(hItem);
				return hItem;
			}
			//아직 다단계 경로라면 재귀호출로 다음 경로를 찾아간다.
			else
			{
				select_item(find_label.Mid(pos + 1), hItem, sep);
			}
		}

		Trace_only(duplicate_str(_T("\t"), tab_count));
		Trace_only(_T("%s\n"), find_label);

		//child가 있다면 child로 이동하고
		if (ItemHasChildren(hItem))
		{
			tab_count++;
			hItem = GetChildItem(hItem);
			continue;
		}
		//child가 없다면 현재 node의 sibling으로 이동한다.
		else
		{
			HTREEITEM hSiblingItem = GetNextSiblingItem(hItem);
			if (hSiblingItem)
			{
				hItem = hSiblingItem;
				continue;
			}
			else
			{
				//만약 sibling도 없다면 parent의 sibling부터 다시 탐색한다.
				while (true)
				{
					tab_count--;
					hItem = GetParentItem(hItem);

					//parent가 NULL인 경우는 최상위 node들인 경우이므로 모든 탐색을 중지한다.
					if (hItem == NULL)
						return NULL;

					//sibling이 있으면 그 node부터 다시 탐색을 시작하고 없다면 다시 parent의 sibling을 찾는다.
					hSiblingItem = GetNextSiblingItem(hItem);
					if (hSiblingItem)
					{
						hItem = hSiblingItem;
						break;
					}
				}
			}
		}
	}

	return NULL;
}

//해당 아이템이 축소되서 보이지 않는 상태인지(height가 음수로 리턴된다.)
bool CSCTreeCtrl::is_visible_item(HTREEITEM hItem)
{
	CRect r;
	GetItemRect(hItem, r, FALSE);
	return (r.Height() > 0);
}

BOOL CSCTreeCtrl::OnTvnItemexpanding(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	//TRACE(_T("%s\n"), __function__);
	m_expanding_item = pNMTreeView->itemNew.hItem;
	//TRACE(_T("OnTvnItemexpanding. %s\n"), GetItemText(m_expanding_item));

	//펼침/접힘(키보드·더블클릭·chevron 등 모든 트리거의 before 알림)으로 가시 집합이 곧 바뀌므로 캐시 무효화.
	//특히 collapse 는 OnTvnItemexpanded 가 호출되지 않으므로 여기서 dirty 를 세팅해야 다음 ensure 에서 재빌드된다.
	m_content_width_dirty = true;
	m_visible_cache_dirty = true;

	if (m_is_shell_treectrl)
	{
		//만약 child가 없다면 아직 로딩되지 않은 노드이므로 검색해서 추가한다.
		//물론 실제 child가 없는 폴더일수도 있다.
		//if (!ItemHasChildren(m_expanding_item))
		if (GetChildItem(m_expanding_item) == NULL)
		{
			if (m_is_local)
			{
				::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_WAIT));
				insert_folder(m_expanding_item, get_path(m_expanding_item));
				::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));
			}
			else
			{
				//remote라면 요청해서 넣어야 한다.
				TRACE(_T("send message_request_folder_list, cur_path = %s\n"), get_path(m_expanding_item));
				::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCTreeCtrl, (WPARAM) & (CSCTreeCtrlMessage(this, message_request_folder_list, NULL)), (LPARAM)&get_path(m_expanding_item));
			}
		}
	}

	*pResult = 0;

	//expand/collapse paint 1-cycle 화 — native expand paint 차단, OnTvnItemexpanded 에서 sync + Invalidate 후 SetRedraw(TRUE).
	SetRedraw(FALSE);

	return FALSE;
}

//check, has button등에 의해 HitTest가 알아서 해당 영역을 알려주리라 기대했으나
//hit 영역을 정확히 리턴해주지 않는다.
//button, check, text 영역을 내가 정해서 그려주고 이벤트도 수동으로 처리해주는 것이 나을듯하다.
//이것을 수동으로 해보니 절대 정확히 알아낼 수 없다.
//결국 OnPaint()에서 직접 그려주는 방법이 아닌 OnCustomDraw()로 전환함.
BOOL CSCTreeCtrl::OnNMClick(NMHDR* pNMHDR, LRESULT* pResult)
{
	//TRACE(_T("OnNMClick\n"));

	if (m_in_editing)
	{
		edit_end(true);
	}

	LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);

	long t0 = clock();

	CPoint pt;
	GetCursorPos(&pt);
	ScreenToClient(&pt);
	HTREEITEM hCurItem = HitTest(pt);
	HTREEITEM hDispItem = pTVDispInfo->item.hItem;

	CRect r;
	GetItemRect(hCurItem, r, TRUE);

	//TRACE(_T("cur = %ld, last = %ld, m_last_clicked_item = %p, hCurItem = %p, hDispItem = %p\n"),
	//	t0, m_last_clicked_time, m_last_clicked_item, hCurItem, hDispItem);

	if (r.PtInRect(pt))
	{
		if ((t0 - m_last_clicked_time > 500) && (t0 - m_last_clicked_time < 2000))
		{
			if (hCurItem == m_last_clicked_item)
			{
				//편집을 컨트롤 내부에서 처리하는게 편하지만 때로는 편집을 메인에서 관여해야 하는 경우도 존재한다.
				//(실제 label data와 UI상에 표시하는 label이 다를 경우, CSManager에서 그룹명 옆에 에이전트 개수를 표시하는 기능)
				//edit_item();
				::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCTreeCtrl, (WPARAM) & (CSCTreeCtrlMessage(this, message_edit_item, NULL)), (LPARAM)0);
			}
			else
			{
				//TRACE(_T("diff item\n"));
				m_last_clicked_time = t0;
				m_last_clicked_item = hCurItem;
			}
		}
		else
		{
			//TRACE(_T("time over\n"));
			m_last_clicked_time = t0;
			m_last_clicked_item = hCurItem;
		}
	}
	else
	{
		m_last_clicked_time = 0;
		m_last_clicked_item = NULL;
		//TRACE(_T("m_last_clicked_time = %ld\n"), m_last_clicked_time);
	}

	*pResult = 0;

	return FALSE;
}

CString CSCTreeCtrl::get_path(HTREEITEM hItem)
{
	if (hItem == NULL)
	{
		hItem = GetSelectedItem();
		if (hItem == NULL)
			return _T("");
	}

	CString fullpath;
	CString text;
	CString temp;

	while (hItem)
	{
		text = GetItemText(hItem);
		//만약 folder가 내 PC, 바탕 화면, 문서와 같은 특수 폴더라면 절대경로로 변경해준다.
		//temp = m_pShellImageList->m_volume[!m_is_local].get_path(text);
		//if (!temp.IsEmpty())
		//	text = temp;

		fullpath = text + _T("\\") + fullpath;

		if (text.Right(2) == _T(":)"))
			break;

		hItem = GetParentItem(hItem);
	}

	//fullpath의 맨 끝에 '\\'가 붙어있다면 제거한다.
	if (fullpath.Right(1) == '\\')
		fullpath.Truncate(fullpath.GetLength() - 1);

	return fullpath;

	/*
	if (m_is_local)
		return convert_special_folder_to_real_path(fullpath, m_pShellImageList->m_volume[!m_is_local].get_label_map());

	//remote의 바탕화면, 내 문서, 내 PC는 별도 처리해야 한다.
	//내 문서의 기본 레이블인 "문서"를 리턴하면 이를 m_remoteDocumentPath로 변경하여 사용한다.
	if (fullpath == get_system_label(CSIDL_DESKTOP) + _T("\\") ||
		fullpath == get_system_label(CSIDL_MYDOCUMENTS) + _T("\\") ||
		fullpath == get_system_label(CSIDL_DRIVES) + _T("\\"))
	{
		return folder;
	}

	return convert_special_folder_to_real_path(fullpath, m_pShellImageList->m_volume[!m_is_local].get_label_map());
	*/
}

void CSCTreeCtrl::set_path(CString fullpath, bool expand)
{
	//set_path로 넘어온 경로가 현재 경로와 동일하다면 스킵.
	if (fullpath == get_path())
		return;

	//"작업 디스크 (D:)\\temp"
	fullpath = m_pShellImageList->convert_real_path_to_special_folder(!m_is_local, fullpath);
	if (fullpath.GetLength() > 0 && fullpath.Right(1) == '\\')
		truncate(fullpath, 1);

	//"내 PC\\작업 디스크 (D:)\\temp"
	//':)' 기호가 있는 디스크 드라이브 경로인 경우에는 맨 앞에 '내 PC'를 붙여준다.
	if (fullpath.Find(_T(":)")) > 0)
		fullpath = concat_path(m_pShellImageList->m_volume[!m_is_local].get_label(CSIDL_DRIVES), fullpath);

	//'\\'로 각 경로를 분리해야 하는데 "내 PC\\작업 디스크 (D:)\\temp"와 같은 경우는 3개로 잘 분리되지만
	//네트워크 드라이브 인 경우는 fullpath가 "내 PC\\연구소문서2(\\\\192.168.1.103) (X:)"와 같이 되므로
	//get_token_str()으로 간단히 분리하면 안된다.
	std::deque<CString> dq;
	get_exact_token_str(fullpath, dq, '\\');

	if (dq.size() == 0)
		dq.push_back(m_pShellImageList->get_system_label(!m_is_local, CSIDL_DRIVES));

	HTREEITEM item = find_item(dq[0]);
	if (item == NULL)
		return;

	//fullpath가 "작업 디스크 (D:)\\temp"라고 넘어오면 이를 토큰분리한 후 "작업 디스크 (D:)" -> "temp" 순서로 폴더를 찾아간다.
	for (int i = 1; i < dq.size(); i++)
	{
		TRACE(_T("CSCTreeCtrl::set_path(). finding '%s' from '%s' node...\n"), dq[i], (item ? GetItemText(item) : _T("root")));

		//만약 현재 노드에 아직 child가 추가된 상태가 아니라면 우선 children을 넣어준 후 검색해야 한다.
		if (item && (GetChildItem(item) == NULL))
		{
			m_expanding_item = item;

			//if (m_is_local)
			//{
			//	insert_folder(item, get_path(item));
			//}
			//else
			//{
			//	//remote라면 요청해서 넣어야 한다.
			//	TRACE(_T("send message_request_folder_list of %s\n"), get_path(m_expanding_item));
			//	::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCTreeCtrl, (WPARAM) & (CSCTreeCtrlMessage(this, message_request_folder_list, NULL)), (LPARAM)&get_path(m_expanding_item));
			//}

			Expand(item, TVE_EXPAND);
		}

		item = find_children_item(dq[i], item);

		if (!item)
  			return;

		if (expand)
			Expand(item, TVE_EXPAND);
	}

	TRACE(_T("text = %s\n"), GetItemText(item));
	if (item)
		SelectItem(item);
}

//set_path 의 경로-walk 를 재사용하되 강제 확장/선택 없이, 이미 로드된 노드만 찾아 반환한다(없으면 NULL).
HTREEITEM CSCTreeCtrl::get_item_by_fullpath(CString fullpath)
{
	fullpath = m_pShellImageList->convert_real_path_to_special_folder(!m_is_local, fullpath);
	if (fullpath.GetLength() > 0 && fullpath.Right(1) == '\\')
		truncate(fullpath, 1);

	//':)' 있는 드라이브 경로면 맨 앞에 '내 PC' 라벨을 붙인다(set_path 와 동일).
	if (fullpath.Find(_T(":)")) > 0)
		fullpath = concat_path(m_pShellImageList->m_volume[!m_is_local].get_label(CSIDL_DRIVES), fullpath);

	std::deque<CString> dq;
	get_exact_token_str(fullpath, dq, '\\');
	if (dq.size() == 0)
		return NULL;

	HTREEITEM item = find_item(dq[0]);
	for (int i = 1; item != NULL && i < (int)dq.size(); i++)
		item = find_children_item(dq[i], item);	//중간 노드 자식이 로드 안 됐으면 NULL — 강제 확장하지 않는다.

	return item;
}

void CSCTreeCtrl::iterate_tree(HTREEITEM hItem)
{
	if (hItem)
	{
		TRACE(_T("%s\n"), GetItemText(hItem));
		hItem = GetNextItem(hItem, TVGN_CHILD);
		while (hItem)
		{
			iterate_tree(hItem);
			hItem = GetNextItem(hItem, TVGN_NEXT);
		}
	}
	else
	{
		HTREEITEM hItem = GetNextItem(NULL, TVGN_ROOT);
		while (hItem)
		{
			iterate_tree(hItem);
			hItem = GetNextItem(hItem, TVGN_NEXT);
		}
	}
}

void CSCTreeCtrl::iterate_tree_in_order(HTREEITEM hItem)
{
	Trace(_T("\n"));

	if (hItem == NULL)
		hItem = GetRootItem();

	int tab_count = 0;
	CString label;

	while (hItem)
	{
		label = GetItemText(hItem);
		
		Trace_only(duplicate_str(_T("\t"), tab_count));

		Trace_only(_T("%s\n"), label);

		//child가 있다면 child로 이동하고
		if (ItemHasChildren(hItem))
		{
			tab_count++;
			hItem = GetChildItem(hItem);
			continue;
		}
		//child가 없다면 현재 node의 sibling으로 이동한다.
		else
		{
			HTREEITEM hSiblingItem = GetNextSiblingItem(hItem);
			if (hSiblingItem)
			{
				hItem = hSiblingItem;
				continue;
			}
			else
			{
				//만약 sibling도 없다면 parent의 sibling부터 다시 탐색한다.
				while (true)
				{
					tab_count--;
					hItem = GetParentItem(hItem);

					//parent가 NULL인 경우는 최상위 node들인 경우이므로 모든 탐색을 중지한다.
					if (hItem == NULL)
						return;

					//sibling이 있으면 그 node부터 다시 탐색을 시작하고 없다면 다시 parent의 sibling을 찾는다.
					hSiblingItem = GetNextSiblingItem(hItem);
					if (hSiblingItem)
					{
						hItem = hSiblingItem;
						break;
					}
				}
			}
		}
	}

	//expand 상태가 아닌 node의 children은 옵션으로 탐색여부가 달라진다.
}

//recursion을 사용하지 않고 모든 node를 검색한다.
//stack을 이용하므로 그 차례가 실제 UI와 다르므로 deque에 넣은 후 sort를 이용한다.
//탐색기의 트리일 경우에 특화된 코드가 있으므로 범용으로 사용하려면 좀 더 보완이 필요함.
std::deque<CSCTreeCtrlFolder> CSCTreeCtrl::iterate_tree_with_no_recursion(HTREEITEM hItem)
{
	std::deque<HTREEITEM> s;
	std::deque<CSCTreeCtrlFolder> folders;
	CString fullpath;
	CString text;

	HTREEITEM item = (hItem ? hItem : GetRootItem());

	while (item != NULL || s.size())
	{
		while (item != NULL)
		{
			s.push_back(item);
			item = GetChildItem(item);
		}

		item = s[0];
		s.pop_front();

		if (m_is_shell_treectrl)
		{
			text = GetItemText(item);

			if (text == m_pShellImageList->m_volume[0].get_label(CSIDL_DESKTOP) ||
				text == m_pShellImageList->m_volume[0].get_label(CSIDL_MYDOCUMENTS))
			{
				fullpath = GetItemText(item);
			}
			else
			{
				fullpath = get_path(item);
			}

			//내 PC일 경우는 ""로 리턴되므로
			if (fullpath.IsEmpty())
				folders.push_back(CSCTreeCtrlFolder(item, fullpath, m_pShellImageList->m_volume[0].get_label(CSIDL_DRIVES)));
			else
				folders.push_back(CSCTreeCtrlFolder(item, fullpath, get_part(fullpath, fn_leaf_folder)));
		}

		item = GetNextSiblingItem(item);
	}

	//바탕 화면, 문서, 내 PC 3개 항목을 제외하고 정렬.
	//기본 정렬과 탐색기의 정렬은 약간 다르므로 탐색기와 같은 정렬이 되도록.
	if (m_is_shell_treectrl)
	{
		std::sort(folders.begin() + 3, folders.end(),
			[](CSCTreeCtrlFolder a, CSCTreeCtrlFolder b)
			{
				return !is_greater_with_numeric(a.fullpath, b.fullpath);
			}
		);

		for (int i = 0; i < folders.size(); i++)
		{
			//trace(_T("%s\n"), folders[i]);

			//"C:\\" => "로컬 디스크 (C:)"로 변경해준다.
			if (folders[i].fullpath.Right(2) == _T(":\\"))
			{
				folders[i].folder = m_pShellImageList->m_volume[!m_is_local].get_drive_volume(folders[i].fullpath[0]);
			}
		}
	}

	return folders;
}

void CSCTreeCtrl::expand_all(bool expand)
{
	HTREEITEM hItem;

	hItem = GetFirstVisibleItem();

	while (hItem != NULL)
	{
		Expand(hItem, expand ? TVE_EXPAND : TVE_COLLAPSE);
		hItem = GetNextItem(hItem, TVGN_NEXTVISIBLE);
	}
}

/*
HTREEITEM CSCTreeCtrl::hit_test(UINT* nFlags)
{
	CPoint pt;

	GetCursorPos(&pt);
	ScreenToClient(&pt);

	HTREEITEM hItem = HitTest(pt, nFlags);

	CRect r[5];

	get_item_rect(hItem, r);

	if ((GetStyle() & TVS_HASBUTTONS) && r[rect_button].PtInRect(pt))
	{
		*nFlags = TVHT_ONITEMBUTTON;
		TRACE(_T("hItem = %p, nFlags = %s\n"), hItem, ENUM_TO_CSTRING(TVHT_ONITEMBUTTON));
	}
	else if ((GetStyle() & TVS_CHECKBOXES) && r[rect_check].PtInRect(pt))
	{
		*nFlags = TVHT_ONITEMSTATEICON;
		TRACE(_T("hItem = %p, nFlags = %s\n"), hItem, ENUM_TO_CSTRING(TVHT_ONITEMSTATEICON));
	}
	else if (r[rect_icon].PtInRect(pt))
	{
		*nFlags = TVHT_ONITEMICON;
		TRACE(_T("hItem = %p, nFlags = %s\n"), hItem, ENUM_TO_CSTRING(TVHT_ONITEMICON));
	}
	else if (r[rect_label].PtInRect(pt))
	{
		*nFlags = TVHT_ONITEMLABEL;
		TRACE(_T("hItem = %p, nFlags = %s\n"), hItem, ENUM_TO_CSTRING(TVHT_ONITEMLABEL));
	}
	else if (pt.x > r[rect_label].right)
	{
		*nFlags = TVHT_ONITEMRIGHT;
		TRACE(_T("hItem = %p, nFlags = %s\n"), hItem, ENUM_TO_CSTRING(TVHT_ONITEMRIGHT));
	}
	else
	{
		*nFlags = TVHT_ONITEMINDENT;
		TRACE(_T("hItem = %p, nFlags = %d (%s)\n"), hItem, *nFlags, ENUM_TO_CSTRING(TVHT_ONITEMINDENT));
	}

	return hItem;
}
*/

/*
void CSCTreeCtrl::set_color_theme(int theme, bool apply_now)
{
	switch (theme)
	{
		//최근 윈도우 탐색기의 색상을 보면 텍스트 색상은 선택여부, inactive에 무관하게 동일하다.
	case color_theme_default:
		m_cr_text.SetFromCOLORREF(::GetSysColor(COLOR_BTNTEXT));
		m_cr_text_selected = m_cr_text;// ::GetSysColor(COLOR_HIGHLIGHTTEXT);
		m_cr_text_selected_inactive = m_cr_text;// ::GetSysColor(COLOR_INACTIVECAPTIONTEXT);
		m_cr_text_dropHilited.SetFromCOLORREF(::GetSysColor(COLOR_HIGHLIGHTTEXT));
		m_cr_back.SetFromCOLORREF(::GetSysColor(COLOR_WINDOW));
		m_cr_back_selected = Gdiplus::Color(255, 205, 232, 255);// ::GetSysColor(COLOR_HIGHLIGHT);
		m_cr_back_selected_inactive = Gdiplus::Color(255, 217, 217, 217);// ::GetSysColor(COLOR_HIGHLIGHT);
		m_crBackTrackSelect = Gdiplus::Color(255, 229, 243, 255);
		m_cr_back_dropHilited.SetFromCOLORREF(::GetSysColor(COLOR_HIGHLIGHT));
		m_cr_selected_border = Gdiplus::Color(255, 153, 209, 255);
		break;
	case color_theme_light_blue:
		m_cr_text.SetFromCOLORREF(::GetSysColor(COLOR_BTNTEXT));
		m_cr_text_selected = Gdiplus::Color(255, 65, 102, 146);
		m_cr_text_selected_inactive = Gdiplus::Color(255, 65, 102, 146);
		m_cr_text_dropHilited = get_color(m_cr_text, -48);
		m_cr_back = Gdiplus::Color(255, 193, 219, 252);
		m_cr_back_selected = get_color(m_cr_back, -48);
		m_cr_back_selected_inactive = get_color(m_cr_back, -48);
		m_crBackTrackSelect = get_color(m_cr_back_selected, 48);
		m_cr_back_dropHilited = get_color(m_cr_back, -48);;
		m_cr_selected_border = Gdiplus::Color(255, 153, 209, 255);
		break;
	case color_theme_navy_blue:
		m_cr_text = Gdiplus::Color(255, 204, 216, 225);
		m_cr_text_selected = Gdiplus::Color(255, 234, 246, 255);
		m_cr_text_selected_inactive = Gdiplus::Color(255, 105, 142, 186);
		m_cr_text_dropHilited = get_color(m_cr_text, 48);
		m_cr_back = Gdiplus::Color(255, 74, 94, 127);
		m_cr_back_selected = Gdiplus::Color(255, 15, 36, 41);
		m_cr_back_selected_inactive = Gdiplus::Color(255, 15, 36, 41);
		m_crBackTrackSelect = get_color(m_cr_back_selected, 48);
		m_cr_back_dropHilited = get_color(m_cr_back, 48);
		m_cr_selected_border = Gdiplus::Color(255, 153, 209, 255);
		break;
	case color_theme_dark_blue:
		m_cr_text = Gdiplus::Color(255, 16, 177, 224);
		m_cr_text_selected = Gdiplus::Color(255, 224, 180, 59);
		m_cr_text_selected_inactive = Gdiplus::Color(255, 105, 142, 186);
		m_cr_text_dropHilited = get_color(m_cr_text, 48);
		m_cr_back = Gdiplus::Color(255, 2, 21, 36);
		m_cr_back_selected = Gdiplus::Color(255, 3, 42, 59);
		m_cr_back_selected_inactive = Gdiplus::Color(255, 15, 36, 41);
		m_crBackTrackSelect = get_color(m_cr_back_selected, 48);
		m_cr_back_dropHilited = blue;
		m_cr_selected_border = Gdiplus::Color(255, 153, 209, 255);
		break;
	case color_theme_dark_gray:
		m_cr_text = Gdiplus::Color(255, 164, 164, 164);
		m_cr_text_selected = Gdiplus::Color(255, 241, 241, 241);
		m_cr_text_selected_inactive = get_color(m_cr_text_selected, -36);
		m_cr_text_dropHilited = get_color(m_cr_text, 255);
		m_cr_back = Gdiplus::Color(255, 64, 64, 64);
		m_cr_back_selected = get_color(m_cr_back, -16);
		m_cr_back_selected_inactive = get_color(m_cr_back, -32);
		m_crBackTrackSelect = get_color(m_cr_back_selected, 48);
		m_cr_back_dropHilited = blue;
		m_cr_selected_border = Gdiplus::Color(255, 128, 128, 128);
		break;
	case color_theme_dark:
		m_cr_text = Gdiplus::Color(255, 212, 212, 212);
		m_cr_text_selected = Gdiplus::Color(255, 241, 241, 241);
		m_cr_text_selected_inactive = get_color(m_cr_text_selected, -36);
		m_cr_text_dropHilited = white;
		m_cr_back = Gdiplus::Color(255, 37, 37, 38);
		m_cr_back_selected = Gdiplus::Color(255, 0, 120, 215);
		m_cr_back_selected_inactive = Gdiplus::Color(255, 0, 120, 215);
		m_crBackTrackSelect = get_color(m_cr_back_selected, 48);
		m_cr_back_dropHilited = blue;
		m_cr_selected_border = m_cr_back_selected;// Gdiplus::Color(255, 128, 128, 128);
		break;
	}

	if (apply_now)
		Invalidate();
}
*/

BOOL CSCTreeCtrl::OnNMDblclk(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	HTREEITEM hItem = GetSelectedItem();

	/*
	CPoint pt;
	UINT nFlags = 0;

	GetCursorPos(&pt);
	ScreenToClient(&pt);

	HTREEITEM hItem = HitTest(pt, &nFlags);
	TRACE(_T("%s, %d, %d, %d\n"), get_path(hItem), pt.x, pt.y, nFlags);

	//아이콘+레이블 영역이라면
	if (nFlags & TVHT_ONITEM)
	{
		TRACE(_T("button\n"));
		nFlags = GetItemState(hItem, TVIF_STATE);
		TRACE(_T("expanded = %d\n"), nFlags & TVIS_EXPANDED);

		//if (nFlags & TVIS_EXPANDED)
		{
			//만약 child가 없다면 아직 로딩되지 않은 노드이므로 검색해서 추가한다.
			//물론 실제 child가 없는 폴더일수도 있다.
			if (GetChildItem(hItem) == NULL)
			{
				insert_folder(hItem, get_path(hItem));
				m_folder_list = iterate_tree_with_no_recursion();
			}
		}
	}
	*/
	*pResult = 0;

	return FALSE;
}


BOOL CSCTreeCtrl::OnTvnBegindrag(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.

	if (!m_use_drag_and_drop)// || !m_is_shell_treectrl)
		return FALSE;

	CRect	rc;
	GetClientRect(rc);

	m_DragItem = pNMTreeView->itemNew.hItem;
	CString path;

	if (m_is_shell_treectrl)
	{
		path = m_pShellImageList->convert_special_folder_to_real_path(!m_is_local, get_path(m_DragItem));

		//보호 폴더(드라이브 루트·시스템 폴더 등)는 소스가 될 수 없다 — 드래그 시작 자체를 차단(위험 원천 차단).
		if (m_pShellImageList->is_protected(!m_is_local, path))
		{
			m_DragItem = NULL;
			*pResult = 0;
			return FALSE;
		}
	}
	
	//focus가 없거나 선택되지 않은 상태에서 바로 drag가 시작되면
	//drag 이미지만 표시되므로 focus를 주고 drag하고 있는 아이템을 선택상태로 표시해줘야 한다.
	//"선택 영역 항상 표시" 속성 또한 true여야 한다.
	::SetFocus(m_hWnd);
	SetItemState(m_DragItem, TVIS_SELECTED, TVIF_STATE);

	bool sub_folder_exist = (m_is_shell_treectrl ? has_sub_folders(path) : false);
	CSCGdiplusBitmap drag_img(64, 64, Gdiplus::Color(128, 255, 0, 0), PixelFormat32bppARGB);

	//이전 드래그의 잔여 이미지 파기. m_pDragImage 는 new CImageList 이므로 DeleteImageList()(=HIMAGELIST 만 해제)가
	//아니라 delete 로 C++ 객체까지 해제해야 한다(그냥 =NULL 하면 wrapper 8바이트 릭). delete 는 소멸자에서 DeleteImageList 호출.
	if (m_pDragImage)
	{
		delete m_pDragImage;
		m_pDragImage = NULL;
	}

	//drag_image가 없다면 노드 자체 아이콘 및 레이블을 이용한다.
	//GDI를 이용해서 create_drag_image()를 사용했으나 아이콘과 함께 레이블을 출력할 때 오동작함. 수정 필요.
	//GDIPlus를 이용한 create_drag_image()를 이용해서 만드는 것도 좋을듯함.
	if (m_drag_images_id.size() == 0)
	{
		//bmpRes.create_drag_image(this, m_imagelist.ExtractIcon(0));
		//bmpRes = create_drag_image((CTreeCtrl*)this, &pNMTreeView->ptDrag);
		create_drag_image(drag_img);
	}
	else
	{
		//drag_image가 1개일 경우
		if (m_drag_images_id.size() == 1)
		{
			drag_img.load(m_drag_images_id[0]);
		}
		//drag_image가 2개 이상일 경우는 drag count에 따라 0번 또는 1번 이미지를 사용한다.
		else if (m_drag_images_id.size() > 1)
		{
			drag_img.load(sub_folder_exist ? m_drag_images_id[1] : m_drag_images_id[0]);
		}
	}
	//drag_img.draw_text(drag_img.width / 2 + 10, drag_img.height / 2, i2S(item_count), 20, 2,
	//	_T("맑은 고딕"), Gdiplus::Color(192, 0, 0, 0), Gdiplus::Color(192, 255, 128, 128), DT_CENTER | DT_VCENTER);

	//// Set dragging flag and others
	m_bDragging = TRUE;	//we are in a drag and drop operation
	m_pDragWnd = this; //make note of which list we are dragging from
	m_pDropWnd = this;	//at present the drag list is the drop list

	//// Capture all mouse messages
	SetCapture();

	//20260705 by claude. 레이어드 드래그 이미지 표시(리스트 CVtListCtrlEx 와 동일 방식). use_control(false) → WS_EX_TRANSPARENT
	//(마우스 통과 → WindowFromPoint 가 아래 트리/리스트를 반환, 드롭 판정 정상) + 포커스 안 뺏음. 문구는 이미지 아래에 CSCParagraph
	//로 그려 하나의 비트맵으로 합성(update_drag_hint). 이미지(위)만 먼저 표시하고, 대상에 따라 문구가 붙으면 재합성해 set_image.
	drag_img.deep_copy(&m_drag_base_img);	//문구 없는 원본 보관(이미지+문구 재합성용)
	m_drag_hint_text = _T("\x01");			//문구 캐시 무효화 — 첫 update_drag_hint 는 무조건 반영
	m_drag_shape.set_image(GetTopLevelParent(), &drag_img, true);
	m_drag_shape.use_control(false);
	m_drag_shape_offset = CPoint(-10, -14);
	CPoint scr;
	GetCursorPos(&scr);
	m_drag_shape.SetWindowPos(&CWnd::wndTopMost, scr.x + m_drag_shape_offset.x, scr.y + m_drag_shape_offset.y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
	m_drag_shape.ShowWindow(SW_SHOWNOACTIVATE);

	TRACE(_T("start drag...\n"));

	*pResult = 0;

	return FALSE;
}

//https://jiniya.net/tt/594/
//이 함수는 드래그 이미지를 직접 생성해주는 코드지만 취약점이 많은 코드이므로 사용 중지! 참고만 할것.
CImageList* CSCTreeCtrl::create_drag_image(CTreeCtrl* pTree, LPPOINT lpPoint)
{
	//DWORD dwStyle = GetWindowLongPtr(pTree->m_hWnd, GWL_STYLE) & LVS_TYPEMASK;
	CRect rectComplete(0, 0, 0, 0);
	HTREEITEM hItem = GetSelectedItem();

	if (hItem == NULL)
		return NULL;

	// Determine List Control Client width size  
	CRect rc;
	pTree->GetClientRect(rc);
	int nWidth = rc.Width() + 50;

	GetItemRect(hItem, rectComplete, TRUE);

	// Create memory device context  
	CClientDC dcClient(this);
	CDC dcMem;
	CBitmap Bitmap;

	if (!dcMem.CreateCompatibleDC(&dcClient))
		return NULL;

	if (!Bitmap.CreateCompatibleBitmap(&dcClient
		, rectComplete.Width()
		, rectComplete.Height()))
		return NULL;

	CBitmap* pOldMemDCBitmap = dcMem.SelectObject(&Bitmap);
	// Use green as mask color  
	dcMem.FillSolidRect(0
		, 0
		, rectComplete.Width()
		, rectComplete.Height()
		, m_theme.cr_back_selected.ToCOLORREF());

	// 안티알리아스 안된 폰트를 사용하는게 핵심
	CFont* pFont = pTree->GetFont();
	LOGFONT lf;
	pFont->GetLogFont(&lf);
	//lf.lfQuality = NONANTIALIASED_QUALITY;
	CFont newFont;
	newFont.CreateFontIndirect(&lf);

	CFont* oldFont = dcMem.SelectObject(&newFont);
	////////////////////////////////////////////////  

	// Paint each DragImage in the DC  
	TCHAR buffer[1000];
	TVITEM item = { 0 };
	item.mask = LVIF_TEXT | LVIF_IMAGE;
	item.hItem = hItem;
	item.pszText = buffer;
	item.cchTextMax = 999;

	pTree->GetItem(&item);

	// Draw the icon  

	CImageList* pSingleImageList = pTree->GetImageList(TVSIL_NORMAL);

	if (pSingleImageList)
	{
		CRect rectIcon;
		pTree->GetItemRect(hItem, rectIcon, LVIR_ICON);

		IMAGEINFO info;
		pSingleImageList->GetImageInfo(item.iImage, &info);
		CPoint p((rectIcon.left - rectComplete.left
			+ rectIcon.right - rectComplete.left) / 2
			- (info.rcImage.right - info.rcImage.left) / 2,
			(rectIcon.top - rectComplete.top
				+ rectIcon.bottom - rectComplete.top) / 2
			- (info.rcImage.bottom - info.rcImage.top) / 2);

		pSingleImageList->Draw(&dcMem, item.iImage, p, ILD_TRANSPARENT);
	}

	// Draw the text  
	CString text;
	text = item.pszText;
	CRect textRect;
	pTree->GetItemRect(hItem, textRect, LVIR_LABEL);

	textRect.top -= rectComplete.top - 2;
	textRect.bottom -= rectComplete.top + 1;
	textRect.left -= rectComplete.left - 2;
	textRect.right -= rectComplete.left;

	//dcMem.FillSolidRect(textRect, Gdiplus::Color(255, 255, 0, 0));
	dcMem.SetTextColor(m_theme.cr_text_selected.ToCOLORREF());
	DWORD flags = DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOCLIP | DT_WORDBREAK;
	dcMem.DrawText(text, -1, textRect, flags);

	dcMem.SelectObject(oldFont);
	dcMem.SelectObject(pOldMemDCBitmap);

	// Create drag image(list)  
	CImageList* pCompleteImageList = new CImageList;
	pCompleteImageList->Create(rectComplete.Width()
		, rectComplete.Height()
		, ILC_COLOR32 | ILC_MASK
		, 0
		, 1);
	pCompleteImageList->Add(&Bitmap, RGB(0, 255, 0));
	Bitmap.DeleteObject();

	if (lpPoint)
	{
		lpPoint->x = rectComplete.left;
		lpPoint->y = rectComplete.top;
	}

	return pCompleteImageList;
}

void CSCTreeCtrl::create_drag_image(CSCGdiplusBitmap& drag_img)
{
	if (m_DragItem == NULL)
		return;

	//드래그 항목 + 그 아래 '펼쳐진(현재 화면에 보이는)' 하위 항목들을 하나의 이미지로 합성한다(탐색기처럼 펼친 구조까지 표시).
	//항목이 너무 많으면 최대 높이에서 자르고, 하단을 gradient alpha 로 흐리게 처리한다.
	const int base_level = get_indent_level(m_DragItem);
	const int gap        = 4;	//아이콘-텍스트 간격
	const int pad_r      = 8;	//텍스트 오른쪽 여유
	const int font_size  = get_font_size();

	//아이콘 크기 — 트리의 '실제' 이미지리스트에서 얻는다. shell tree 는 시스템 이미지리스트, 그 외는 m_imagelist 를
	//SetImageList 해두므로 GetImageList(TVSIL_NORMAL) 가 두 경우 모두 올바르다. (m_imagelist 직접 참조는 shell tree 에서 NULL → 크래시였음)
	CImageList* pIL = GetImageList(TVSIL_NORMAL);
	int icon_w = m_image_size, icon_h = m_image_size;
	if (pIL && pIL->GetSafeHandle())
	{
		int cx = 0, cy = 0;
		if (ImageList_GetIconSize(pIL->GetSafeHandle(), &cx, &cy) && cx > 0)
		{
			icon_w = cx;
			icon_h = cy;
		}
	}

	//행 높이.
	int row_h = 0;
	{ CRect r; if (GetItemRect(m_DragItem, r, TRUE)) row_h = r.Height(); }
	if (row_h < icon_h + 2)
		row_h = icon_h + 2;

	//최대 높이 + 담을 수 있는 행 수(capacity). 초과분이 있으면 clipped.
	const int MAX_HEIGHT = max(row_h * 3, 300);
	const int capacity   = max(1, MAX_HEIGHT / row_h);

	std::deque<HTREEITEM> rows;
	rows.push_back(m_DragItem);
	bool clipped = false;
	for (HTREEITEM h = GetNextVisibleItem(m_DragItem); h != NULL; h = GetNextVisibleItem(h))
	{
		if (get_indent_level(h) <= base_level)		//서브트리를 벗어남(형제/상위) → 중단
			break;
		if ((int)rows.size() >= capacity)			//최대 높이 초과 → 페이드 처리 대상
		{
			clipped = true;
			break;
		}
		rows.push_back(h);
	}

	//폭 측정 — 트리가 실제로 그리는 아이템 텍스트 폭(GetItemRect, bTextOnly)을 그대로 쓴다(동일 폰트라 wrap 없음).
	std::deque<int> row_indent;
	int max_w = 1;
	for (auto h : rows)
	{
		int indent = (get_indent_level(h) - base_level) * get_indent_size();
		row_indent.push_back(indent);

		int text_w = 0;
		{ CRect r; if (GetItemRect(h, r, TRUE)) text_w = r.Width(); }
		int w = indent + icon_w + gap + text_w + pad_r;
		if (w > max_w)
			max_w = w;
	}

	int total_h = (int)rows.size() * row_h;
	if (total_h <= 0)
		total_h = row_h;

	drag_img.release();
	drag_img.create(max_w, total_h, PixelFormat32bppARGB);
	Gdiplus::Graphics g(drag_img.m_pBitmap);

	int y = 0;
	for (size_t i = 0; i < rows.size(); i++)
	{
		HTREEITEM h      = rows[i];
		int       indent = row_indent[i];

		//아이콘.
		int img_index = -1, sel_index = -1;
		GetItemImage(h, img_index, sel_index);
		if (pIL && pIL->GetSafeHandle() && img_index >= 0)
		{
			HICON hIcon = pIL->ExtractIcon(img_index);
			if (hIcon)
			{
				//배경 투명 + 반투명 가장자리 깔끔하게 — 32bpp 컬러비트맵의 알파를 직접 추출해 그린다(FromHICON=불투명배경, DrawIconEx=까만점 회피).
				draw_hicon_alpha(g, hIcon, indent, y + (row_h - icon_h) / 2, icon_w, icon_h);
				::DestroyIcon(hIcon);
			}
		}

		//텍스트.
		CRect rText(indent + icon_w + gap, y, max_w, y + row_h);
		::draw_text(g, rText, GetItemText(h), font_size, Gdiplus::FontStyleRegular, 2, 0.0f, m_lf.lfFaceName,
			m_theme.cr_text, Gdiplus::Color::Transparent, Gdiplus::Color::LightGray, Gdiplus::Color::Transparent, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

		y += row_h;
	}

	//하단 gradient alpha 페이드 — 하위 항목이 최대 높이를 초과해 '잘린 경우에만' 하단을 아래로 갈수록 투명하게.
	if (clipped)
	{
		int fade_h = min(row_h * 2, total_h);
		Gdiplus::Rect lr(0, total_h - fade_h, max_w, fade_h);
		Gdiplus::BitmapData bd;
		if (drag_img.m_pBitmap->LockBits(&lr, Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &bd) == Gdiplus::Ok)
		{
			for (int yy = 0; yy < fade_h; yy++)
			{
				BYTE* line   = (BYTE*)bd.Scan0 + yy * bd.Stride;
				int   factor = 255 - (yy * 255 / max(1, fade_h - 1));
				for (int xx = 0; xx < max_w; xx++)
				{
					BYTE* px = line + xx * 4;	//BGRA
					px[3] = (BYTE)(px[3] * factor / 255);
				}
			}
			drag_img.m_pBitmap->UnlockBits(&bd);
		}
	}

#ifdef _DEBUG
	drag_img.save(_T("D:\\drag_img.png"));
#endif
}

//20260705 by claude. base 드래그 이미지(위) 하단에 tagged 문구(아래)를 CSCParagraph 로 그려 out 비트맵에 합성.
void CSCTreeCtrl::compose_drag_image_with_hint(CSCGdiplusBitmap& base, const CString& tagged_text, CSCGdiplusBitmap& out)
{
	const int	band_pad_x = 10;	//문구-배경 좌우 여백
	const int	band_pad_y = 4;		//문구-배경 상하 여백
	const int	gap        = 4;		//base 이미지와 문구 밴드 사이 간격

	//문구 기본 속성 — XP 호환 위해 컨트롤 폰트(m_lf). <b>/<cr=..> 태그가 개별 run 을 override(강조).
	CSCTextProperty tp;
	_tcsncpy_s(tp.name, _countof(tp.name), m_lf.lfFaceName, _TRUNCATE);
	tp.size    = (float)get_font_size() + 1.0f;
	tp.style   = Gdiplus::FontStyleRegular;
	tp.cr_text = Gdiplus::Color(255, 40, 40, 40);	//폴더명 등 기본(어두운 회색)

	//태그 파싱 → para. calc_text_rect 가 출력 크기를 자동 산출(MeasureString 불필요).
	std::deque<std::deque<CSCParagraph>> para;
	CString t = tagged_text;
	CSCParagraph::build_paragraph_str(t, para, &tp);

	//<b> 강조 run 에 같은 색 stroke 로 굵기 보강(faux-bold). 실제 bold face 가 있어도 작은 크기에선 weight 차가 약해 stroke 로 보강.
	//단, 기호(+/→)는 얇아서 강하게(0.7), 한글/영문 단어는 stroke 가 과하면 글자가 뭉개지므로 약하게(0.3, semibold) 준다.
	for (auto& line : para)
		for (auto& run : line)
		{
			if (!(run.text_prop.style & Gdiplus::FontStyleBold))
				continue;
			bool has_word_char = false;
			for (int k = 0; k < run.text.GetLength(); k++)
			{
				TCHAR c = run.text[k];
				if (_istalnum(c) || (c >= 0xAC00 && c <= 0xD7A3)) { has_word_char = true; break; }
			}
			run.text_prop.thickness = has_word_char ? 0.0f : 0.7f;	//단어는 실제 bold 만(stroke 0 — CJK 뭉개짐 방지), 기호만 stroke 로 강조
			run.text_prop.cr_stroke  = run.text_prop.cr_text;
		}

	CClientDC dc(this);
	CRect tr = CSCParagraph::calc_text_rect(CRect(0, 0, 4096, 256), &dc, para, DT_LEFT | DT_TOP);	//넉넉한 박스 → 반환 rect = 실제 문구 크기
	int band_w = tr.Width()  + band_pad_x * 2;
	int band_h = tr.Height() + band_pad_y * 2;

	int W = max(base.width, band_w);
	int H = base.height + gap + band_h;

	out.release();
	out.create(W, H, PixelFormat32bppARGB);
	Gdiplus::Graphics g(out.m_pBitmap);
	g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

	//원본 드래그 이미지(위, 가로 중앙).
	int img_x = (W - base.width) / 2;
	g.DrawImage(base.m_pBitmap, img_x, 0, base.width, base.height);

	//문구 밴드 배경(둥근 사각형) — near-white 반투명 + 옅은 테두리.
	int band_x = (W - band_w) / 2;
	Gdiplus::Rect band(band_x, base.height + gap, band_w, band_h);
	draw_round_rect(&g, band, Gdiplus::Color(255, 120, 120, 120), Gdiplus::Color(240, 250, 250, 250), 5);

	//문구(아래). calc_text_rect 는 수직 위치를 rc.top 이 아니라 0 기준으로 잡으므로(내부 구현), 전체 폭 W 에 가로 중앙
	//(DT_CENTER|DT_TOP)으로 배치해 y=0 에 둔 뒤 각 run 을 밴드 안쪽 top 으로 직접 내린다.
	int band_content_top = base.height + gap + band_pad_y - 1;	//-1: 시각적으로 살짝 아래로 치우쳐 보여 1px 위로
	CSCParagraph::calc_text_rect(CRect(0, 0, W, 4096), &dc, para, DT_CENTER | DT_TOP);
	for (auto& line : para)
		for (auto& run : line)
			run.r.OffsetRect(0, band_content_top);

	g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);	//layered 창(per-pixel alpha)엔 grayscale AA 가 깔끔 — 안 하면 계단현상. draw_text 는 AA_from_pt=0 이면 이 hint 를 그대로 사용.
	CSCParagraph::draw_text(g, para);
}

void CSCTreeCtrl::update_drag_hint(const CString& hint_text)
{
	if (hint_text == m_drag_hint_text)
		return;
	m_drag_hint_text = hint_text;

	if (!::IsWindow(m_drag_shape.GetSafeHwnd()))
		return;

	if (hint_text.IsEmpty())
	{
		//밴드 없이 원본만(예: 같은 드라이브 이동).
		m_drag_shape.set_image(GetTopLevelParent(), &m_drag_base_img, true);
		return;
	}

	CSCGdiplusBitmap composed;
	compose_drag_image_with_hint(m_drag_base_img, hint_text, composed);
	m_drag_shape.set_image(GetTopLevelParent(), &composed, true);
}

void CSCTreeCtrl::refresh_drag_hint()
{
	if (!m_bDragging || !m_fn_drag_hint)
		return;
	CPoint pt;
	GetCursorPos(&pt);
	CWnd* pDropWnd = WindowFromPoint(pt);
	if (pDropWnd)
		update_drag_hint(m_fn_drag_hint(pDropWnd, pt));	//update_drag_hint 가 값 변화 시에만 재합성(캐시)
}

void CSCTreeCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (!m_is_hovering)
	{
		//TRACE(_T("tree. move\n"));
		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(tme);
		tme.hwndTrack = m_hWnd;
		//TME_HOVER를 넣으면 마우스가 hover되면 자동으로 focus를 가진다.
		tme.dwFlags = TME_LEAVE;// | TME_HOVER;
		tme.dwHoverTime = 1;
		m_is_hovering = true;
		/*m_bIsTracking = */_TrackMouseEvent(&tme);
		//RedrawWindow();
	}

	//full-row hot tracking — Y 좌표 기준 row 찾고 m_hot_item 갱신. native TVS_TRACKSELECT 의 CDIS_HOT 가 label 영역에서만 발사되는 한계를 보완.
	//[중요] 드래그 중에는 hot-tracking 금지 — SetCapture 로 커서가 다른 컨트롤(원격 트리 등) 위에 있어도 이 트리의
	//OnMouseMove 가 계속 불려, point.y 가 자기 항목 행에 매핑되면 엉뚱하게 자기 항목에 hover 가 표시되는 버그 방지.
	HTREEITEM new_hot = NULL;
	if (!m_bDragging)
	{
		HTREEITEM cur = GetFirstVisibleItem();
		while (cur)
		{
			CRect r;
			if (GetItemRect(cur, &r, FALSE) && point.y >= r.top && point.y < r.bottom)
			{
				new_hot = cur;
				break;
			}
			cur = GetNextVisibleItem(cur);
		}
	}
	if (new_hot != m_hot_item)
	{
		HTREEITEM old_hot = m_hot_item;
		m_hot_item = new_hot;
		CRect r;
		if (old_hot && GetItemRect(old_hot, &r, FALSE))
			InvalidateRect(&r, FALSE);
		if (new_hot && GetItemRect(new_hot, &r, FALSE))
			InvalidateRect(&r, FALSE);
	}

	if (m_bDragging)
	{
		GetCursorPos(&point);
		//20260705 by claude. 레이어드 드래그 이미지를 커서+오프셋으로 이동(위치만, 재렌더 없음). point 는 screen 좌표(GetCursorPos).
		if (::IsWindow(m_drag_shape.GetSafeHwnd()))
			m_drag_shape.SetWindowPos(&CWnd::wndTopMost, point.x + m_drag_shape_offset.x, point.y + m_drag_shape_offset.y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);

		//// Get the CWnd pointer of the window that is under the mouse cursor
		CWnd* pDropWnd = WindowFromPoint(point);
		TRACE(_T("this = %p, pDropWnd = %p\n"), this, pDropWnd);
		ASSERT(pDropWnd); //make sure we have a window

		//20260704 by claude. 대상 창이 바뀌면 이전 창의 drop-highlight 를 그 창 타입 기준으로 해제한다. 기존엔 정리 없이
		//m_pDropWnd 를 덮어써서 드래그가 컨트롤을 벗어나도 이전 하이라이트(리스트 LVIS_DROPHILITED / 트리 SelectDropTarget)가 남았다.
		if (pDropWnd != m_pDropWnd)
		{
			clear_drop_highlight(m_pDropWnd);
			m_nDropIndex = -1;
			m_DropItem = NULL;
		}

		m_pDropWnd = pDropWnd;

		//20260705 by claude. 드래그 중 대상에 맞는 문구(예: "+ 대상폴더(으)로 복사")를 app provider 로 계산해 이미지에 반영.
		//update_drag_hint 가 값 변화 시에만 재합성하므로 매 mousemove 호출해도 안전(캐시). point 는 아직 screen 좌표.
		if (m_fn_drag_hint)
			update_drag_hint(m_fn_drag_hint(pDropWnd, point));

		//drag되는 위치가 대상 컨트롤(트리/리스트) 가장자리면 자동 스크롤(거리 비례 속도 + 타이머 연속). point 는 아직 screen 좌표.
		update_drag_auto_scroll(point);

		pDropWnd->ScreenToClient(&point);

		//drag되는 위치가 컨트롤의 상하끝단에 위치하면 스크롤시켜줘야 한다.

		//If we are hovering over a CListCtrl we need to adjust the highlights
		if (pDropWnd->IsKindOf(RUNTIME_CLASS(CListCtrl)))
		{
			//Note that we can drop here
			::SetCursor(AfxGetApp()->LoadStandardCursor(MAKEINTRESOURCE(IDC_ARROW)));
			UINT uFlags;
			CListCtrl* pList = (CListCtrl*)pDropWnd;

			//기존 LVIS_DROPHILITED를 해제해주지 않으니 아래 새로 LVIS_DROPHILITED하는 항목이 갱신되지 않는다.
			if (m_nDropIndex >= 0)
			{
				// Turn off hilight for previous drop target
				pList->SetItemState(m_nDropIndex, 0, LVIS_DROPHILITED);
				// Redraw previous item
				pList->RedrawItems(m_nDropIndex, m_nDropIndex);
			}

			// Get the item that is below cursor
			m_nDropIndex = ((CListCtrl*)pDropWnd)->HitTest(point, &uFlags);
			TRACE(_T("nDropIndex in ListCtrl = %d\n"), m_nDropIndex);
			// Highlight it (폴더인 경우에만 hilite시킨다. 폴더는 크기 컬럼이 empty임)
			if (m_nDropIndex >= 0 && ((CListCtrl*)pDropWnd)->GetItemText(m_nDropIndex, 1) == _T(""))
			{
				TRACE(_T("new LVIS_DROPHILITED\n"), m_nDropIndex);
				pList->SetItemState(m_nDropIndex, LVIS_DROPHILITED, LVIS_DROPHILITED);
			}

			// Redraw item
			pList->RedrawItems(m_nDropIndex, m_nDropIndex);
			pList->UpdateWindow();
		}
		else if (pDropWnd->IsKindOf(RUNTIME_CLASS(CTreeCtrl)))
		{
			::SetCursor(AfxGetApp()->LoadStandardCursor(MAKEINTRESOURCE(IDC_ARROW)));
			UINT uFlags;
			CTreeCtrl* pTree = (CTreeCtrl*)pDropWnd;

			// Get the item that is below cursor
			m_DropItem = ((CTreeCtrl*)pDropWnd)->HitTest(point, &uFlags);
			TRACE(_T("%d, %d, hItem = %p\n"), point.x, point.y, m_DropItem);

			// 이전 삽입 마크를 지우기 위해 기억
			HTREEITEM hOldInsertMark = m_hInsertMarkItem;
			DropPosition oldDropPos = m_dropPosition;

			if (m_DropItem && m_DropItem != m_DragItem)
			{
				// 드롭 위치 판별 (상단/중앙/하단)
				// 드롭 위치 판별 (상단/중앙/하단)
				if (m_use_rearrange_order)
					m_dropPosition = hit_test_drop_position(point, m_DropItem);
				else
					m_dropPosition = drop_on_item;

				m_hInsertMarkItem = m_DropItem;

				// 자기 자신의 바로 앞뒤에 놓는 것은 의미 없으므로 drop_on_item으로 전환
				if (m_dropPosition == drop_after_item && GetNextSiblingItem(m_DragItem) == m_DropItem)
				{
					// DragItem 바로 다음 sibling의 before == 현재 위치 그대로
					// 의미 없는 이동이지만 표시는 해줌
				}

				if (m_dropPosition == drop_on_item)
				{
					// 기존 방식: DropHilight 표시
					pTree->SelectDropTarget(m_DropItem);
				}
				else
				{
					// 삽입 마크 모드: DropHilight 해제
					pTree->SelectDropTarget(NULL);
				}
			}
			else
			{
				m_hInsertMarkItem = NULL;
				m_dropPosition = drop_on_item;
				pTree->SelectDropTarget(m_DropItem);
			}

			// 삽입 마크 위치가 변경되면 다시 그리기
			if (hOldInsertMark != m_hInsertMarkItem || oldDropPos != m_dropPosition)
			{
				//20260705 by claude. 레이어드 드래그 이미지는 화면 lock 이 없어 DragShowNolock 불필요(CImageList 잔재 제거).

				// 이전 삽입 마크가 걸쳐있던 아이템 행 전체를 무효화
				if (hOldInsertMark && oldDropPos != drop_on_item)
				{
					CRect rcOld;
					GetItemRect(hOldInsertMark, &rcOld, FALSE);

					// 위쪽 아이템도 포함 (drop_before라면 이전 아이템의 하단에도 걸침)
					HTREEITEM hPrev = GetPrevVisibleItem(hOldInsertMark);
					if (hPrev)
					{
						CRect rcPrev;
						GetItemRect(hPrev, &rcPrev, FALSE);
						rcOld.UnionRect(&rcOld, &rcPrev);
					}

					// 아래쪽 아이템도 포함
					HTREEITEM hNext = GetNextVisibleItem(hOldInsertMark);
					if (hNext)
					{
						CRect rcNext;
						GetItemRect(hNext, &rcNext, FALSE);
						rcOld.UnionRect(&rcOld, &rcNext);
					}

					// 전체 너비로 확장
					CRect rcClient;
					GetClientRect(&rcClient);
					rcOld.left = rcClient.left;
					rcOld.right = rcClient.right;

					InvalidateRect(&rcOld, TRUE);
				}

				// 새 삽입 마크 영역 무효화
				if (m_hInsertMarkItem && m_dropPosition != drop_on_item)
				{
					CRect rcNew;
					GetItemRect(m_hInsertMarkItem, &rcNew, FALSE);

					HTREEITEM hPrev = GetPrevVisibleItem(m_hInsertMarkItem);
					if (hPrev)
					{
						CRect rcPrev;
						GetItemRect(hPrev, &rcPrev, FALSE);
						rcNew.UnionRect(&rcNew, &rcPrev);
					}

					HTREEITEM hNext = GetNextVisibleItem(m_hInsertMarkItem);
					if (hNext)
					{
						CRect rcNext;
						GetItemRect(hNext, &rcNext, FALSE);
						rcNew.UnionRect(&rcNew, &rcNext);
					}

					CRect rcClient;
					GetClientRect(&rcClient);
					rcNew.left = rcClient.left;
					rcNew.right = rcClient.right;

					InvalidateRect(&rcNew, TRUE);
				}

				UpdateWindow();
			}

			ASSERT(m_DropItem == NULL || m_dropPosition != drop_on_item || m_DropItem == pTree->GetDropHilightItem());
		}
		else
		{
			//If we are not hovering over a CListCtrl, change the cursor
			// to note that we cannot drop here
			::SetCursor(AfxGetApp()->LoadStandardCursor(MAKEINTRESOURCE(IDC_NO)));
		}
	}

	CTreeCtrl::OnMouseMove(nFlags, point);
}


void CSCTreeCtrl::OnMouseHover(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	TRACE(_T("tree. hover\n"));
	CTreeCtrl::OnMouseHover(nFlags, point);
}


void CSCTreeCtrl::OnMouseLeave()
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	m_is_hovering = false;
	KillTimer(timer_expand_for_drag_hover);
	//TRACE(_T("tree. leave\n"));
	SelectDropTarget(NULL);
	clear_insert_mark();

	if (m_hot_item)
	{
		HTREEITEM old_hot = m_hot_item;
		m_hot_item = NULL;
		CRect r;
		if (GetItemRect(old_hot, &r, FALSE))
			InvalidateRect(&r, FALSE);
	}

	CTreeCtrl::OnMouseLeave();
}

void CSCTreeCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	//탐색기 동작: full-row selection 표시 + row 어디 클릭이든 그 항목 선택.
	//base CTreeCtrl::OnLButtonDown 는 Default() → DefWindowProc 로 *원래 message 의 lParam* (= 사용자 클릭 좌표) 그대로 보내므로
	//point 인자를 label 좌표로 바꿔 호출해도 redirect 가 무효 → SelectItem 직접 호출로 명시적 선택.
	UINT hit_flags = 0;
	HTREEITEM hItem = HitTest(point, &hit_flags);

	if (!hItem)
	{
		HTREEITEM cur = GetFirstVisibleItem();
		while (cur)
		{
			CRect r;
			if (GetItemRect(cur, &r, FALSE) && point.y >= r.top && point.y < r.bottom)
			{
				hItem = cur;
				break;
			}
			cur = GetNextVisibleItem(cur);
		}
	}

	//체크박스 클릭 — 우리 customdraw 의 체크박스 위치 기준 hit-test (native TVHT_ONITEMSTATEICON 영역과 어긋남).
	if (hItem && (GetStyle() & TVS_CHECKBOXES))
	{
		CRect rcCheck = get_checkbox_rect(hItem);
		rcCheck.OffsetRect(-get_over_shift(), 0);   //over-scroll 시 그리기와 동일하게 좌측 shift (안 하면 클릭 영역이 어긋남)
		if (!rcCheck.IsRectEmpty() && rcCheck.PtInRect(point))
		{
			SetCheck(hItem, !GetCheck(hItem));
			SelectItem(hItem);
			SetFocus();
			return;
		}
	}

	//expand button (▶/▼) 클릭 — customdraw 의 glyph 위치 기준 hit-test (native TVHT_ONITEMBUTTON 영역과 어긋남, 특히 왼쪽).
	//SetRedraw(FALSE/TRUE) wrap — collapse 시 tree 의 ScrollWindowEx BitBlt 가 row pixel 을 위로 끌어올려 화면에 잔상 (motion blur) 으로 보이는 거 차단.
	//Expand 처리 동안 paint 차단, 끝에 Invalidate + UpdateWindow 로 단일 cycle paint.
	if (hItem)
	{
		CRect rcButton = get_expand_button_rect(hItem);
		rcButton.OffsetRect(-get_over_shift(), 0);   //over-scroll 시 그리기와 동일하게 좌측 shift (안 하면 H 스크롤 후 클릭 영역이 어긋남)
		if (!rcButton.IsRectEmpty() && rcButton.PtInRect(point))
		{
			SetRedraw(FALSE);
			Expand(hItem, TVE_TOGGLE);
			//collapse 시엔 OnTvnItemexpanded 가 호출되지 않아(Win32 가 collapse 에 ITEMEXPANDED 미발송) 캐시 무효화·
			//range 갱신이 누락된다 → 접어도 세로 스크롤바 range 가 펼친 상태로 남던 버그. toggle(펼침/접힘) 완료 후
			//여기서 직접 무효화 + sync 한다.
			m_content_width_dirty = true;
			m_visible_cache_dirty = true;
			SetRedraw(TRUE);
			sync_scrollbar();
			Invalidate(FALSE);
			UpdateWindow();
			return;
		}
	}

	if (hItem)
	{
		SelectItem(hItem);
		SetFocus();
	}

	CTreeCtrl::OnLButtonDown(nFlags, point);
}

void CSCTreeCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_bDragging)
	{
		TRACE(_T("OnLButtonUp\n"));
		ReleaseCapture();

		m_bDragging = false;

		//드래그 자동 스크롤 타이머 정지.
		KillTimer(timer_drag_auto_scroll);
		m_drag_scroll_vx = 0;
		m_drag_scroll_vy = 0;

		//20260705 by claude. 레이어드 드래그이미지 숨김(파괴 대신 재사용 — 다음 드래그의 set_image 가 갱신). CImageList 릭 경로 소멸.
		m_drag_shape.ShowWindow(SW_HIDE);

		// clear_insert_mark()를 여기서 호출하지 않는다.
		// DroppedHandler() 내부에서 m_dropPosition을 사용한 후 clear_insert_mark()가 호출됨.

		CPoint pt(point);
		ClientToScreen(&pt);

		CWnd* pDropWnd = WindowFromPoint(pt);
		ASSERT(pDropWnd);

		if (pDropWnd->IsKindOf(RUNTIME_CLASS(CListCtrl)) ||
			pDropWnd->IsKindOf(RUNTIME_CLASS(CTreeCtrl)))
		{
			m_pDropWnd = pDropWnd;
			DroppedHandler(m_pDragWnd, m_pDropWnd);
		}
		else
		{
			// 유효하지 않은 영역에 드롭한 경우에도 삽입 마크를 초기화
			clear_insert_mark();
		}

		::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCTreeCtrl, (WPARAM) & (CSCTreeCtrlMessage(this, message_drag_and_drop, NULL)), (LPARAM)0);
	}
	else
	{
		return;
	}

	CTreeCtrl::OnLButtonUp(nFlags, point);
}

//드래그 중 ESC 등으로 드래그를 '드롭 없이' 완전 취소한다. DroppedHandler 를 부르지 않으므로 어떤 이동/전송도 일어나지 않는다.
void CSCTreeCtrl::cancel_drag()
{
	if (!m_bDragging)
		return;

	ReleaseCapture();
	m_bDragging = false;

	KillTimer(timer_drag_auto_scroll);
	KillTimer(timer_expand_for_drag_hover);
	m_drag_scroll_vx = 0;
	m_drag_scroll_vy = 0;

	//20260705 by claude. 레이어드 드래그이미지 숨김(파괴 대신 재사용).
	m_drag_shape.ShowWindow(SW_HIDE);

	//드롭 하이라이트/삽입마크 해제(자기 + 대상 트리 모두).
	SelectDropTarget(NULL);
	if (m_pDropWnd && m_pDropWnd != this && m_pDropWnd->IsKindOf(RUNTIME_CLASS(CTreeCtrl)))
		((CTreeCtrl*)m_pDropWnd)->SelectDropTarget(NULL);
	clear_insert_mark();

	m_DragItem = NULL;
	m_DropItem = NULL;
	m_pDropWnd = NULL;

	Invalidate();
}

void CSCTreeCtrl::OnRButtonDown(UINT nFlags, CPoint point)
{
	//드래그 중 우클릭 = ESC 와 동일하게 드래그 완전 취소(위험 방지). 이 우클릭 제스처는 여기서 끝 —
	//이어질 RBUTTONUP 도 소비해 NM_RCLICK/WM_CONTEXTMENU(팝업 메뉴)가 생기지 않게 한다.
	if (m_bDragging)
	{
		cancel_drag();
		m_swallow_rbutton = true;
		return;
	}

	//우클릭 항목을 먼저 선택 + 즉시 동기 paint 로 selected(skyblue) 표시한다. native base(CTreeCtrl::OnRButtonDown)는 호출하지
	//않는다 — native 는 우클릭 항목을 drop-hilight(blue)로 잡아 blue↔skyblue 깜빡임의 원인이므로. base 를 건너뛰면 drop-hilight
	//가 애초에 안 생긴다. 컨텍스트 메뉴는 OnRButtonUp 에서 직접 띄운다(base 미호출이라 native 의 NM_RCLICK 은 발생하지 않는다).
	UINT uFlags = 0;
	HTREEITEM hItem = HitTest(point, &uFlags);
	if (hItem)
	{
		SelectItem(hItem);
		SetFocus();
		UpdateWindow();		//SelectItem/SetFocus 의 무효화를 즉시 그려 우클릭 순간 selected 로 보이게(그 뒤 UP 에서 메뉴).
	}
}

void CSCTreeCtrl::OnRButtonUp(UINT nFlags, CPoint point)
{
	//드래그 취소용으로 눌린 우클릭의 UP — 소비(base 미호출)해서 팝업 메뉴가 뜨지 않게 한다.
	if (m_swallow_rbutton)
	{
		m_swallow_rbutton = false;
		return;
	}

	//OnRButtonDown 이 native base 를 호출하지 않으므로(drop-hilight 방지) native 가 보내던 NM_RCLICK 통지와 WM_CONTEXTMENU 를
	//여기서 서로 '독립적으로' 낸다. 이 둘은 별개다 — 컨텍스트 메뉴는 WM_CONTEXTMENU 로만 처리하고, NM_RCLICK 은 순수 통지다.

	//(1) NM_RCLICK 통지 — 우클릭됨을 parent 에 알린다(원하는 앱만 ON_NOTIFY(NM_RCLICK) 로 처리; 컨트롤 자체가 처리하려면
	//    subclass 가 ON_NOTIFY_REFLECT 를 추가). 메뉴와 무관 — 여기서 메뉴를 띄우지 않는다.
	NMHDR nmhdr;
	nmhdr.hwndFrom = m_hWnd;
	nmhdr.idFrom   = (UINT_PTR)GetDlgCtrlID();
	nmhdr.code     = NM_RCLICK;

	CWnd* parent = GetParent();
	if (parent)
		parent->SendMessage(WM_NOTIFY, (WPARAM)nmhdr.idFrom, (LPARAM)&nmhdr);

	//(2) WM_CONTEXTMENU 요청(정석 단일 경로) — OnContextMenu 가 받아 m_use_own_context_menu==false 면 parent 로 forward 한다.
	CPoint pt_screen = point;
	ClientToScreen(&pt_screen);
	SendMessage(WM_CONTEXTMENU, (WPARAM)m_hWnd, MAKELPARAM(pt_screen.x, pt_screen.y));
}

void CSCTreeCtrl::DroppedHandler(CWnd* pDragWnd, CWnd* pDropWnd)
{
	CString droppedItem;

	if (pDropWnd->IsKindOf(RUNTIME_CLASS(CListCtrl)))
	{
		CListCtrl* pDropListCtrl = (CListCtrl*)pDropWnd;

		if (m_nDropIndex >= 0)
			droppedItem = pDropListCtrl->GetItemText(m_nDropIndex, 0);

		TRACE(_T("drag item = %s\n"), GetItemText(m_DragItem));
		TRACE(_T("dropped on = %s\n"), (droppedItem.IsEmpty() ? _T("dropped on ListCtrl") : droppedItem));
	}
	else if (pDropWnd->IsKindOf(RUNTIME_CLASS(CTreeCtrl)))
	{
		CTreeCtrl* pDropTreeCtrl = (CTreeCtrl*)pDropWnd;

		TRACE(_T("drag item = %s\n"), GetItemText(m_DragItem));
		TRACE(_T("dropped on = %s\n"), pDropTreeCtrl->GetItemText(m_DropItem));

		// 같은 CTreeCtrl 내에서의 이동
		// [주의] shell 트리(파일시스템 미러)는 노드를 수동으로 옮기면 안 된다 — 앱 핸들러가 실제 파일 이동 후
		// refresh 로 트리를 재구성한다. 게다가 move_tree_item()은 m_DragItem 핸들을 삭제/무효화하는데,
		// 바로 아래에서 parent 로 보낸 message_drag_and_drop 핸들러가 get_path(m_DragItem)로 소스 경로를 구하므로
		// shell 트리에서 이걸 실행하면 소스가 빈 값이 되어 파일 이동이 무동작이 된다. 그래서 shell 트리는 제외.
		if ((pDragWnd == pDropWnd) && (m_DragItem != m_DropItem) && !m_is_shell_treectrl)
		{
			if (m_dropPosition == drop_on_item)
			{
				// 기존 동작: 자식으로 이동 (parent 변경)
				move_tree_item(this, m_DragItem, m_DropItem);
			}
			else
			{
				// 새 동작: 같은 레벨에서 순서 변경 (sibling reorder)
				move_tree_item_as_sibling(this, m_DragItem, m_DropItem,
					(m_dropPosition == drop_after_item));
			}
		}
	}

	// 삽입 마크 초기화
	clear_insert_mark();

	::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCTreeCtrl, (WPARAM) & (CSCTreeCtrlMessage(this, message_drag_and_drop, pDropWnd)), (LPARAM)0);
}

void CSCTreeCtrl::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	{
		SCROLLINFO _dbg = { sizeof(_dbg), SIF_ALL };
		::GetScrollInfo(m_hWnd, SB_HORZ, &_dbg);
		TRACE(_T("OnHScroll: nSBCode=%u nPos=%u | si.nPos=%d nMax=%d nPage=%u | internal=%d m_h_scroll_pos=%d\n"),
			(unsigned)nSBCode, (unsigned)nPos, _dbg.nPos, _dbg.nMax, _dbg.nPage,
			m_h_internal_thumb ? 1 : 0, m_h_scroll_pos);
	}

	//트리 가로 스크롤은 native 가 아니라 m_h_scroll_pos + customdraw paint-shift 로 동작한다(native H scroll 불가).
	//따라서 외부에서 온 WM_HSCROLL(마우스 H-wheel, 드래그 자동스크롤 등)도 여기서 m_h_scroll_pos 를 직접 옮겨야
	//화면 스크롤 + 오버레이 가로 스크롤바(m_scrollbar_h)가 함께 연동된다. (기존엔 native si.nPos(SB_HORZ)만 건드려
	//화면도 스크롤바도 아무 반응이 없었음 = 드래그 자동 가로스크롤 미동작의 원인.)
	if (nSBCode == SB_LINELEFT || nSBCode == SB_LINERIGHT || nSBCode == SB_PAGELEFT || nSBCode == SB_PAGERIGHT)
	{
		CRect rc; GetClientRect(&rc);
		int right_limit_h = rc.Width() - (m_v_visible_state ? m_scrollbar.get_width() : 0);
		int max_pos = max(0, m_h_content_width - right_limit_h);

		//같은 SBCode 로 밀려온 메시지들을 합쳐(chunk) 한 번에 이동 — 깜빡임/지연 방지.
		int chunks = 1;
		MSG peek_msg;
		while (::PeekMessage(&peek_msg, m_hWnd, WM_HSCROLL, WM_HSCROLL, PM_NOREMOVE))
		{
			if (LOWORD(peek_msg.wParam) != nSBCode)
				break;
			::PeekMessage(&peek_msg, m_hWnd, WM_HSCROLL, WM_HSCROLL, PM_REMOVE);
			chunks++;
		}

		bool is_page  = (nSBCode == SB_PAGELEFT || nSBCode == SB_PAGERIGHT);
		int  direction = (nSBCode == SB_LINELEFT || nSBCode == SB_PAGELEFT) ? -1 : 1;
		int  step = is_page ? max(60, right_limit_h - 40) : (chunks * 60);

		int new_pos = m_h_scroll_pos + direction * step;
		if (new_pos < 0)       new_pos = 0;
		if (new_pos > max_pos) new_pos = max_pos;
		m_h_scroll_pos = new_pos;
	}
	else
		CTreeCtrl::OnHScroll(nSBCode, nPos, pScrollBar);

	Invalidate(FALSE);
	sync_scrollbar();
}

//file의 내용을 읽어서 load_from_string()를 호출한다.
bool CSCTreeCtrl::load(CString file)
{
	load_from_string(read(file));
	return true;
}

bool CSCTreeCtrl::save(CString file)
{
	return true;
}

bool CSCTreeCtrl::load_from_string(CString text)
{
	//가상 루트가 있다면 그 children들만 제거하고 불러와야 한다.
	if (m_use_root)
	{
		delete_item(m_root_item, true);
	}
	else
	{
		DeleteAllItems();
	}

	text.Replace(_T("\r\n"), _T("\n"));
	text.Trim();

	std::deque<CString> lines;
	get_token_str(text, lines, '\n', false);

	int			img_index = -1;
	int			img_count = (m_imagelist.GetSafeHandle() ? m_imagelist.GetImageCount() : 0);
	int			tab_count = 0;
	int			prev_indent = 0;
	CString		label;
	HTREEITEM	hItem = get_root_item();

	Trace(_T("\n"));

	for (int i = 0; i < lines.size(); i++)
	{
		label = lines[i];
		label.TrimRight();
		tab_count = get_char_count(label, '\t');

		label.TrimLeft();

		//Trace_only(make_string(_T("\t"), tab_count));
		//Trace_only(_T("%s\n"), label);

		if (tab_count < img_count)
			img_index = tab_count + 1;
		else
			img_index = -1;

		if (tab_count == 0)
		{
			if (img_index >= 0)
				hItem = InsertItem(label, img_index, img_index, (m_use_root ? m_root_item : TVI_ROOT));
			else
				hItem = InsertItem(label, (m_use_root ? m_root_item : TVI_ROOT));
			//TRACE(_T("hItem = %p, label = %s\n"), hItem, label);

			prev_indent = 0;
			continue;
		}
		//전보다 1개 많으면 child이고
		if (tab_count == prev_indent + 1)
		{
			if (img_index >= 0)
				hItem = InsertItem(label, img_index, img_index, hItem);
			else
				hItem = InsertItem(label, hItem);
			//TRACE(_T("hItem = %p, label = %s\n"), hItem, label);

			prev_indent = tab_count;
		}
		//전과 같으면 sibling이고
		else if (tab_count == prev_indent)
		{
			hItem = GetParentItem(hItem);
			if (img_index >= 0)
				hItem = InsertItem(label, img_index, img_index, hItem);
			else
				hItem = InsertItem(label, hItem);
			//TRACE(_T("hItem = %p, label = %s\n"), hItem, label);
		}
		//전보다 적으면 (prev_index-tabCount) * parent의 sibling이다.
		else if (tab_count < prev_indent)
		{
			while (prev_indent >= tab_count)
			{
				hItem = GetParentItem(hItem);
				prev_indent--;
			}

			if (img_index >= 0)
				hItem = InsertItem(label, img_index, img_index, hItem);
			else
				hItem = InsertItem(label, hItem);
			TRACE(_T("hItem = %p, label = %s\n"), hItem, label);

			prev_indent++;
		}
		else
		{
			AfxMessageBox(lines[i] + _T("\n문자열이 올바른 트리 구조 형식이 아닙니다."));
			break;
		}
	}

	return true;
}

void CSCTreeCtrl::create_imagelist()
{
	m_use_own_imagelist = true;

	if (m_image_size < 0)
		m_image_size = 16;

	//SetImageList(NULL, TVSIL_NORMAL);

	m_imagelist.DeleteImageList();
	m_imagelist.Create(m_image_size, m_image_size, ILC_COLOR32 | ILC_MASK, 0, 1);

	for (int i = 0; i < m_image_IDs.size(); i++)
		m_imagelist.Add(AfxGetApp()->LoadIcon(m_image_IDs[i]));

	SetImageList(&m_imagelist, TVSIL_NORMAL);

	int height = GetItemHeight();
	if (m_line_height > 0)
		SetItemHeight(m_line_height);		//20260705 by claude. 명시 line height 우선.
	else
		SetItemHeight(MAX(m_image_size, height));

	Invalidate();
}

void CSCTreeCtrl::set_imagelist(const std::vector<HICON>& icons, int image_size)
{
	m_use_own_imagelist = true;
	m_image_size = image_size;

	m_imagelist.DeleteImageList();
	m_imagelist.Create(image_size, image_size, ILC_COLOR32 | ILC_MASK, 0, 1);

	for (HICON h : icons)
	{
		if (h)
			m_imagelist.Add(h);
	}

	SetImageList(&m_imagelist, TVSIL_NORMAL);

	int height = GetItemHeight();
	if (m_line_height > 0)
		SetItemHeight(m_line_height);		//20260705 by claude. 명시 line height 우선.
	else
		SetItemHeight(MAX(image_size, height));

	Invalidate();
}

void CSCTreeCtrl::set_use_own_imagelist(bool use, int image_size)
{
	m_use_own_imagelist = use;
	m_imagelist.DeleteImageList();

	if (m_use_own_imagelist)
	{
		m_image_size = image_size;
		create_imagelist();
	}

	Invalidate();
}

void CSCTreeCtrl::set_image_size(int image_size)
{
	set_use_own_imagelist(true, image_size);
}

// 아이템 데이터 이동

BOOL CSCTreeCtrl::move_tree_item(CTreeCtrl* pTree, HTREEITEM hSrcItem, HTREEITEM hDestItem)
{
	//src의 parent가 dst이면 스킵.
	if (GetParentItem(hSrcItem) == hDestItem)
	{
		TRACE(_T("move to parent node is meaningless. skip.\n"));
		return TRUE;
	}

	//ShellTreeCtrl이고 destItem이 아직 확장되기 전의 상태라면 확장시켜준 후 추가해야 한다.
	if (m_is_shell_treectrl)
	{
		if ((GetItemState(hDestItem, TVIF_STATE) & TVIS_EXPANDED) == false)
		{
			insert_folder(hDestItem, get_path(hDestItem));
			Expand(hDestItem, TVE_EXPAND);
			//m_folder_list = iterate_tree_with_no_recursion();
		}
	}

	bool folder_operation = true;

	//ShellTreeCtrl이면 폴더를 move or copy가 성공한 후에 UI를 갱신시킨다.
	if (m_is_shell_treectrl)
	{
		CString srcPath = get_path(hSrcItem);
		CString dstPath = get_path(hDestItem);

		if (IsCtrlPressed())
		{
			TRACE(_T("copy node. %s to %s\n"), srcPath, dstPath);
			//folder_operation = CopyFile(srcPath, dstPath,);
		}
		else
		{
			TRACE(_T("move node. %s to %s\n"), srcPath, dstPath);
			//folder_operation = MoveFile(srcPath, dstPath,);
		}
	}


	// 이동할 아이템의 정보를 알아내자.
	TVITEM	TV;
	TCHAR	str[256];
	ZeroMemory(str, sizeof(str));

	TV.hItem = hSrcItem;
	TV.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	TV.pszText = str;
	TV.cchTextMax = _countof(str);	//cchTextMax 는 '문자 수' — sizeof(=바이트 512)면 GetItem 이 256칸 버퍼에 최대 512자 써 스택 오버플로.
	GetItem(&TV);

	DWORD_PTR dwData = pTree->GetItemData(hSrcItem);

	// 아이템을 추가 하자.
	TVINSERTSTRUCT  TI;

	TI.hParent = hDestItem;
	TI.hInsertAfter = TVI_LAST;
	TI.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	TI.item.iImage = TV.iImage;
	TI.item.iSelectedImage = TV.iSelectedImage;
	TI.item.pszText = TV.pszText;

	HTREEITEM hItem = pTree->InsertItem(&TI);

	pTree->SetItemData(hItem, dwData);

	// 현재 아이템에 자식 아이템이 있다면
	HTREEITEM hChildItem = pTree->GetChildItem(hSrcItem);

	if (hChildItem)
	{
		// 자식 아이템이 있다면 같이 이동하자.
		move_child_tree_item(pTree, hChildItem, hItem);
	}

	// 확장 여부를 알아서 똑같이 하자.
	TVITEM  item;

	item.mask = TVIF_HANDLE;
	item.hItem = hSrcItem;

	pTree->GetItem(&item);

	if (item.state & TVIS_EXPANDED)
	{
		pTree->Expand(hItem, TVE_EXPAND);
	}

	// 아이템을 선택하자.
	pTree->SelectItem(hItem);

	// 기존 아이템을 제거한다.
	pTree->DeleteItem(hSrcItem);

	//노드 이동후에는 dropItem의 children을 정렬시켜줘야 한다.
	pTree->SortChildren(hDestItem);

	return TRUE;
}

// 현재 트리의 모든 아이템 데이터 이동
BOOL CSCTreeCtrl::move_child_tree_item(CTreeCtrl* pTree, HTREEITEM hChildItem, HTREEITEM hDestItem)
{
	HTREEITEM hSrcItem = hChildItem;

	while (hSrcItem)
	{
		// 이동할 아이템의 정보를 알아내자.
		TVITEM	TV;
		TCHAR	str[256];

		ZeroMemory(str, sizeof(str));

		TV.hItem = hSrcItem;
		TV.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		TV.pszText = str;
		TV.cchTextMax = _countof(str);	//cchTextMax 는 '문자 수' — sizeof(=바이트 512)면 GetItem 이 256칸 버퍼에 최대 512자 써 스택 오버플로.
		GetItem(&TV);

		DWORD_PTR dwData = pTree->GetItemData(hSrcItem);

		// 아이템을 추가 하자.
		TVINSERTSTRUCT  TI;

		TI.hParent = hDestItem;
		TI.hInsertAfter = TVI_LAST;
		TI.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		TI.item.iImage = TV.iImage;
		TI.item.iSelectedImage = TV.iSelectedImage;
		TI.item.pszText = TV.pszText;

		HTREEITEM hItem = pTree->InsertItem(&TI);

		pTree->SetItemData(hItem, dwData);

		// 현재 아이템에 자식 아이템이 있다면
		HTREEITEM hChildItem = pTree->GetChildItem(hSrcItem);

		// pTree->GetNextItem(hSrcItem, TVGN_CHILD;
		if (hChildItem)
		{
			move_child_tree_item(pTree, hChildItem, hItem);
		}

		// 확장 여부를 알아서 똑같이 하자.
		TVITEM  item;

		item.mask = TVIF_HANDLE;
		item.hItem = hSrcItem;

		pTree->GetItem(&item);

		if (item.state & TVIS_EXPANDED)
		{
			pTree->Expand(hItem, TVE_EXPAND);
		}

		// 다음 아이템을 알아보자.
		hSrcItem = pTree->GetNextItem(hSrcItem, TVGN_NEXT);
	}

	// 기존 아이템을 제거한다.
	pTree->DeleteItem(hChildItem);

	return TRUE;
}

void CSCTreeCtrl::winctrl_theme_init()
{
	if (!m_winctrl_theme_initialized)
	{
		m_winctrl_theme.Init(m_hWnd);
		m_winctrl_theme_initialized = true;
	}
}

CRect CSCTreeCtrl::get_expand_button_rect(HTREEITEM hItem)
{
	if (hItem == NULL || !(GetStyle() & TVS_HASBUTTONS) || !ItemHasChildren(hItem))
		return CRect();

	int level = get_indent_level(hItem);
	if (!(GetStyle() & TVS_LINESATROOT) && level == 0)
		return CRect();

	int indent_step = (int)SendMessage(TVM_GETINDENT, 0, 0);
	if (indent_step <= 0)
		indent_step = 19;

	CRect rcText, rcRow;
	GetItemRect(hItem, &rcText, TRUE);
	GetItemRect(hItem, &rcRow, FALSE);

	int icon_w = 0;
	HIMAGELIST hil = (HIMAGELIST)SendMessage(TVM_GETIMAGELIST, TVSIL_NORMAL, 0);
	if (hil)
	{
		int cx = 0, cy = 0;
		ImageList_GetIconSize(hil, &cx, &cy);
		icon_w = cx;
	}
	int state_offset = (GetStyle() & TVS_CHECKBOXES) ? (16 + 3) : 0;
	int gx = rcText.left - icon_w - 3 - state_offset - indent_step / 2;
	int gy = (rcRow.top + rcRow.bottom) / 2;

	//hit-test 양쪽 padding — 삼각형 반 폭 (~4) + 여백 (~4) = 8.
	const int hit_pad = 8;
	return CRect(gx - hit_pad, gy - hit_pad, gx + hit_pad, gy + hit_pad);
}

int CSCTreeCtrl::measure_content_width()
{
	if (GetCount() == 0)
		return 0;

	//level → text-left 는 선형(slope = indent step). 현재 보이는 항목 하나로 origin(level 0 의 text-left)을
	//보정하면 화면 밖 항목도 GetItemRect 없이 정확히 계산된다 (수직 스크롤 무관).
	int indent_step = (int)SendMessage(TVM_GETINDENT, 0, 0);
	if (indent_step <= 0)
		indent_step = 19;

	int origin = indent_step;
	HTREEITEM cal = GetFirstVisibleItem();
	CRect rcal;
	if (cal && GetItemRect(cal, &rcal, TRUE))
		origin = rcal.left - get_indent_level(cal) * indent_step;

	CClientDC dc(this);
	CFont* base_font = GetFont();
	CFont* old_font = base_font ? dc.SelectObject(base_font) : nullptr;

	//전체(펼쳐진) 항목을 순회해 가장 긴 라벨 right 를 구한다. 비싸지만 get_content_width 가 *변경 시에만*
	//호출하므로(나머지는 캐시), 매 스크롤 비용은 없다. 큰 트리에서도 가로 범위가 정확.
	int content_w = 0;
	HTREEITEM cur = GetRootItem();
	while (cur)
	{
		int level = get_indent_level(cur);
		CSize sz = dc.GetTextExtent(GetItemText(cur));
		//customdraw 의 텍스트 그리기 좌측 패딩(+2) + 우측 여백 약간 = 4.
		int text_right = origin + level * indent_step + sz.cx + 4;
		if (text_right > content_w)
			content_w = text_right;
		cur = GetNextVisibleItem(cur);
	}

	if (old_font)
		dc.SelectObject(old_font);

	return content_w;
}

int CSCTreeCtrl::get_content_width()
{
	//insert/delete 는 GetCount() 변화로, expand/collapse·폰트 변경은 m_content_width_dirty 로 감지.
	//둘 다 아니면 직전 측정값을 그대로 반환 — sync_scrollbar 가 스크롤·페인트마다 불러도 재측정 안 함.
	int count = (int)GetCount();
	if (m_content_width_dirty || count != m_content_width_count)
	{
		m_content_width_cache = measure_content_width();
		m_content_width_count = count;
		m_content_width_dirty = false;
	}
	return m_content_width_cache;
}

void CSCTreeCtrl::ensure_visible_cache()
{
	//insert/delete 는 GetCount() 변화로, expand/collapse·폰트 변경은 m_visible_cache_dirty 로 감지.
	int count = (int)GetCount();
	if (!m_visible_cache_dirty && count == m_visible_cache_count)
		return;

	m_visible_items.clear();
	m_visible_index.clear();
	m_visible_items.reserve(count);

	//펼쳐진 항목을 display 순서로 1회만 순회(O(n)). 이후 모든 스크롤 매핑은 vector/map 으로 O(1).
	HTREEITEM cur = GetRootItem();
	if (m_use_root && cur == m_root_item)
		cur = GetChildItem(cur);
	while (cur)
	{
		m_visible_index[cur] = (int)m_visible_items.size();
		m_visible_items.push_back(cur);
		cur = GetNextVisibleItem(cur);
	}

	m_visible_cache_count = count;
	m_visible_cache_dirty = false;
}

int CSCTreeCtrl::get_visible_total()
{
	ensure_visible_cache();
	return (int)m_visible_items.size();
}

int CSCTreeCtrl::index_of_visible(HTREEITEM hItem)
{
	ensure_visible_cache();
	auto it = m_visible_index.find(hItem);
	return (it != m_visible_index.end()) ? it->second : -1;
}

bool CSCTreeCtrl::is_item_displayable(HTREEITEM hItem)
{
	//모든 조상이 펼쳐져 있어야 스크롤로 도달 가능한(보일 수 있는) 항목. 하나라도 collapsed 면 숨은 항목.
	for (HTREEITEM p = GetParentItem(hItem); p != NULL; p = GetParentItem(p))
	{
		if (!(GetItemState(p, TVIS_EXPANDED) & TVIS_EXPANDED))
			return false;
	}
	return true;
}

HTREEITEM CSCTreeCtrl::visible_at(int index)
{
	ensure_visible_cache();
	if (index < 0 || index >= (int)m_visible_items.size())
		return NULL;

	HTREEITEM hItem = m_visible_items[index];

	//자가치유 — 캐시가 어떤 이유로 stale(축소된 항목의 숨은 자식 포함)이면, 그 숨은 항목을 TVM_SELECTITEM
	//(TVGN_FIRSTVISIBLE)에 넘기는 순간 트리가 그 항목을 보이려고 부모를 자동 펼친다(= 축소 후 스크롤 시 재펼침
	//버그). 숨은 항목이면 강제 재빌드 후 재취득해 항상 *현재 보이는* 항목만 반환한다.
	if (!is_item_displayable(hItem))
	{
		m_visible_cache_dirty = true;
		ensure_visible_cache();
		int n = (int)m_visible_items.size();
		if (index >= n)
			index = n - 1;
		hItem = (index >= 0 && index < n) ? m_visible_items[index] : NULL;
	}
	return hItem;
}

int CSCTreeCtrl::get_over_shift() const
{
	//native tree 는 WS_HSCROLL strip 으로 H scroll 불가 → 전량 paint-shift. m_h_natural_max>=0 (need_h) 일 때
	//콘텐츠를 m_h_scroll_pos 만큼 좌측으로 시프트해 우측 콘텐츠를 노출한다.
	if (m_h_natural_max < 0)
		return 0;
	return m_h_scroll_pos > 0 ? m_h_scroll_pos : 0;
}

CRect CSCTreeCtrl::get_checkbox_rect(HTREEITEM hItem)
{
	if (hItem == NULL || !(GetStyle() & TVS_CHECKBOXES))
		return CRect();

	CRect rcText, rcRow;
	GetItemRect(hItem, &rcText, TRUE);
	GetItemRect(hItem, &rcRow, FALSE);

	int icon_w = 0;
	HIMAGELIST hil = (HIMAGELIST)SendMessage(TVM_GETIMAGELIST, TVSIL_NORMAL, 0);
	if (hil)
	{
		int cx = 0, cy = 0;
		ImageList_GetIconSize(hil, &cx, &cy);
		icon_w = cx;
	}
	const int gap_icon_text = 3;
	const int gap_check_icon = 3;
	const int state_w = 16;
	int cx_left = rcText.left - icon_w - gap_icon_text - gap_check_icon - state_w;
	int cy = (rcRow.top + rcRow.bottom) / 2;
	int half = state_w / 2;
	return CRect(cx_left, cy - half, cx_left + state_w, cy + half);
}

void CSCTreeCtrl::draw_checkbox(CDC* pDC, CRect r, int check_state)
{
	winctrl_theme_init();

	if (m_winctrl_theme.GetAppearance())
	{
		int	nState = 0;

		// Translate from the button state to the appropriate draw state flags
		switch (check_state)
		{
		case BST_CHECKED:
			nState = CBS_CHECKEDNORMAL;
			break;

		case BST_UNCHECKED:
			nState = CBS_UNCHECKEDNORMAL;
			break;

		default:
			nState = CBS_MIXEDNORMAL;
			break;
		}

		// Now do the actual drawing...
		m_winctrl_theme.DrawThemeBackground(pDC->GetSafeHdc(), r, BP_CHECKBOX, nState);
	}
	else // No themes - just draw it conventionally
	{
		UINT nState = 0;

		// Translate from the button state to the appropriate draw state flags
		switch (check_state)
		{
		case BST_CHECKED:
			nState = DFCS_BUTTONCHECK | DFCS_CHECKED;
			break;

		case BST_UNCHECKED:
			nState = DFCS_BUTTONCHECK;
			break;

		default:
			nState = DFCS_BUTTON3STATE;
			break;
		}

		// Now do the actual drawing...
		pDC->DrawFrameControl(r, DFC_BUTTON, nState);
	}
}

void CSCTreeCtrl::use_checkbox(bool use)
{
	m_use_checkbox = use;

	ModifyStyle(TVS_CHECKBOXES, 0);

	if (use)
	{
		ModifyStyle(0, TVS_CHECKBOXES);
	}
	else
	{
		//Win32 quirk: TVS_CHECKBOXES 를 제거해도 native CTreeCtrl 은 state image list 를 그대로 유지해서
		//state column slot 이 살아남음 → 항목이 우측으로 밀린 채 안 돌아옴. state image list 를 NULL 로 풀어줘야
		//column 이 회수됨.
		SendMessage(TVM_SETIMAGELIST, TVSIL_STATE, (LPARAM)NULL);
	}

	Invalidate();
}

void CSCTreeCtrl::use_expand_button(bool use)
{
	m_use_expand_button = use;

	ModifyStyle(TVS_HASBUTTONS, 0);

	if (use)
		ModifyStyle(0, TVS_HASBUTTONS);

	Invalidate();
}

BOOL CSCTreeCtrl::OnTvnBeginlabeledit(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	*pResult = 0;

	TRACE(_T("%s\n"), __function__);

	return FALSE;
}


BOOL CSCTreeCtrl::OnTvnEndlabeledit(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	*pResult = 0;

	TRACE(_T("%s\n"), __function__);

	return FALSE;
}


void CSCTreeCtrl::edit_item(HTREEITEM hItem)
{
	if (!m_allow_edit)
		return;

	if (hItem == NULL)
	{
		hItem = GetSelectedItem();
		if (hItem == NULL)
			return;
	}

	//20260704 by claude. EnsureVisible 는 부분적으로 가려진 항목을 native H-scroll 로 reveal 하는데(스마트 동작 — 유지),
	//트리의 가로 스크롤은 native 가 아니라 m_h_scroll_pos + paint-shift 모델이라 이 native reveal 이 오버레이 바/m_h_scroll_pos
	//와 어긋난다(콘텐츠는 스크롤됐는데 바는 0). → native reveal 델타를 custom m_h_scroll_pos 로 접고 native 는 원위치로
	//되돌린다: 화면(paint-shift 로 동일 reveal) + 오버레이 바가 함께 동기화된다. (V-wheel 의 native H 부수효과 복원 패턴과 동일.)
	SCROLLINFO si_h_before = { sizeof(si_h_before), SIF_POS };
	::GetScrollInfo(m_hWnd, SB_HORZ, &si_h_before);

	EnsureVisible(hItem);

	SCROLLINFO si_h_after = { sizeof(si_h_after), SIF_POS };
	::GetScrollInfo(m_hWnd, SB_HORZ, &si_h_after);

	int native_h_delta = si_h_after.nPos - si_h_before.nPos;
	if (native_h_delta != 0)
	{
		//native H 를 EnsureVisible 이전 위치로 되돌림(트리 모델: native H = 0, 전량 over_shift).
		::SendMessage(m_hWnd, WM_HSCROLL, MAKEWPARAM(SB_THUMBPOSITION, (WORD)si_h_before.nPos), 0);
		::SendMessage(m_hWnd, WM_HSCROLL, MAKEWPARAM(SB_ENDSCROLL, 0), 0);

		//reveal 량을 custom paint-shift 로 접기 + 오버레이 바 동기.
		CRect rc;
		GetClientRect(&rc);
		int right_limit_h = rc.Width() - (m_v_visible_state ? m_scrollbar.get_width() : 0);
		int max_pos = max(0, m_h_content_width - right_limit_h);
		int new_pos = m_h_scroll_pos + native_h_delta;
		if (new_pos < 0)       new_pos = 0;
		if (new_pos > max_pos) new_pos = max_pos;
		m_h_scroll_pos = new_pos;
		sync_scrollbar();
	}

	m_edit_item = hItem;
	m_edit_old_text = GetItemText(hItem);
	m_last_clicked_item = NULL;
	m_last_clicked_time = 0;

	//편집 사각형의 right는 레이블 너비보다 좀 더 넉넉하게 준다. 단, rc.right를 넘어가서는 안된다.
	CRect rc;
	CRect r;

	GetItemRect(hItem, r, TRUE);

	//20260704 by claude. 가로 over-scroll 은 paint-only 시프트(get_over_shift)라 GetItemRect(native 좌표)엔 반영 안 됨.
	//그리기와 동일하게 -over_shift 를 적용해 edit 을 실제 보이는 노드 위치에 배치(체크박스/확장 hit-test 와 동일 처리).
	r.OffsetRect(-get_over_shift(), 0);

	//아이템이 선택되었을 때 label의 크기가 좌우 타이트하여 left -= 2;
	//CustomDraw에서도 -2하여 그림.
	//r.left -= 2;

	//label영역은 실제 텍스트 너비보다 24픽셀 더 크게 잡아준다.(윈도우 탐색기와 동일)
	r.right += 24;

	//단, 그 오른쪽 끝이 rc.right를 넘어가지 않게 보정한다.
	GetClientRect(rc);
	//세로 오버레이가 보이면 edit 의 우측이 그 영역을 침범하지 않도록 m_scrollbar.get_width() 만큼 줄임.
	int right_limit = rc.right - 1;
	if (::IsWindow(m_scrollbar.GetSafeHwnd()) && m_scrollbar.IsWindowVisible())
		right_limit -= m_scrollbar.get_width();
	if (r.right > right_limit)
		r.right = right_limit;

	r.DeflateRect(0, 0);

	if (m_pEdit == NULL)
	{
		m_pEdit = new CSCStaticEdit;
		m_pEdit->Create(WS_BORDER, r, this, 1004);
	}

	m_pEdit->MoveWindow(r);
	m_pEdit->SetFont(&m_font, true);
	m_pEdit->set_line_align(DT_VCENTER);
	//셀 편집기 — 항목 높이 = edit 높이 이므로 자체 padding 을 2px 로 줄여 텍스트가 위/아래로 자연스럽게 채워지게.
	//0 으로 두면 선택 하이라이트가 보더에 붙어 일반 edit 컨트롤 외관과 어긋남.
	m_pEdit->set_padding(2);

	m_pEdit->SetWindowText(m_edit_old_text);

	m_pEdit->ShowWindow(SW_SHOW);
	m_pEdit->set_sel(0, -1);
	m_pEdit->SetFocus();

	m_in_editing = true;

	//편집 모드 외부 클릭 감지용 마우스 훅 설치 (다른 컨트롤·다이얼로그 빈 영역·NC 어디든) — CSCListBox/CPathCtrl 패턴.
	install_edit_mouse_hook();

	TV_DISPINFO dispinfo;
	dispinfo.hdr.hwndFrom = m_hWnd;
	dispinfo.hdr.idFrom = GetDlgCtrlID();
	dispinfo.hdr.code = TVN_BEGINLABELEDIT;

	dispinfo.item.mask = LVIF_TEXT;
	dispinfo.item.hItem = hItem;
	//dispinfo.item.pszText = bEscape ? NULL : LPTSTR((LPCTSTR)Text);
	//dispinfo.item.cchTextMax = Text.GetLength();

	GetParent()->SendMessage(WM_NOTIFY, GetDlgCtrlID(), (LPARAM)&dispinfo);
}

void CSCTreeCtrl::edit_end(bool valid)
{
	remove_edit_mouse_hook();

	if (m_in_editing == false || m_pEdit == NULL)
		return;

	m_in_editing = false;

	m_pEdit->GetWindowText(m_edit_new_text);
	m_pEdit->ShowWindow(SW_HIDE);

	//valid==false (Escape) 또는 텍스트 미변경 → rename / SetItemText 모두 건너뜀. 외부 클릭으로 들어온 commit
	//의 경우 텍스트 미변경이면 MoveFile(same, same) 이 실패하고 아래 edit_item() 재진입 경로로 빠져 마치
	//편집이 종료되지 않은 것처럼 보이는 버그 회피 (CVtListCtrlEx 와 동일).
	if (!valid || m_edit_new_text == m_edit_old_text)
	{
		TV_DISPINFO dispinfo;
		dispinfo.hdr.hwndFrom = m_hWnd;
		dispinfo.hdr.idFrom = GetDlgCtrlID();
		dispinfo.hdr.code = TVN_ENDLABELEDIT;
		dispinfo.item.mask = TVIF_TEXT;
		dispinfo.item.hItem = m_edit_item;
		GetParent()->SendMessage(WM_NOTIFY, GetDlgCtrlID(), (LPARAM)&dispinfo);
		Invalidate();
		return;
	}

	//shell tree의 label이 변경되면 실제 폴더명도 변경해줘야 한다.
	if (m_is_shell_treectrl)
	{
		CString parent_path = get_parent_dir(get_path(m_edit_item));
		parent_path = m_pShellImageList->convert_special_folder_to_real_path(!m_is_local, parent_path);
		BOOL res = FALSE;

		CString old_path = concat_path(parent_path, m_edit_old_text);
		CString new_path = concat_path(parent_path, m_edit_new_text);

		if (m_is_local)
		{
			//이미 동일한 폴더명이 존재하면 parent에게 알려 메시지를 표시하도록 한다.
			if (new_path != old_path && PathFileExists(new_path))
			{
				::SendMessage(GetParent()->GetSafeHwnd(),
					Message_CSCTreeCtrl,
					(WPARAM) & (CSCTreeCtrlMessage(this, message_rename_duplicated, NULL, old_path, new_path)),
					(LPARAM)&res);
				edit_item(m_edit_item);
				return;
			}

			res = MoveFile(old_path, new_path);
		}
		else
		{
			if (new_path != old_path)
			{
				::SendMessage(GetParent()->GetSafeHwnd(),
					Message_CSCTreeCtrl,
					(WPARAM) & (CSCTreeCtrlMessage(this, message_request_rename, NULL, old_path, new_path)),
					(LPARAM)&res);
				if (!res)
				{
					edit_item(m_edit_item);
					return;
				}
			}

			res = true;
		}
	
		if (res)
		{
			SetItemText(m_edit_item, m_edit_new_text);
			//path가 변경된 것을 parent에게 알려야 listctrl, pathctrl을 갱신한다.
			::SendMessage(GetParent()->GetSafeHwnd(),
				Message_CSCTreeCtrl,
				(WPARAM) & (CSCTreeCtrlMessage(this, message_path_changed, NULL, new_path)),
				(LPARAM)&res);
		}
		else
		{
			//undo_edit_label();
			edit_item(m_edit_item);
		}
	}
	else
	{
		SetItemText(m_edit_item, m_edit_new_text);
	}
	
	//실제 변경 유무와 관계없이 후처리는 main에 맞겨야 한다.
	TV_DISPINFO dispinfo;
	dispinfo.hdr.hwndFrom = m_hWnd;
	dispinfo.hdr.idFrom = GetDlgCtrlID();
	dispinfo.hdr.code = TVN_ENDLABELEDIT;

	dispinfo.item.mask = TVIF_TEXT;
	dispinfo.item.hItem = m_edit_item;

	GetParent()->SendMessage(WM_NOTIFY, GetDlgCtrlID(), (LPARAM)&dispinfo);

	Invalidate();
}

//편집 전의 텍스트로 되돌린다.(예를 들어 편집 레이블이 파일명이고 파일명 변경이 실패한 경우 쓸 수 있다.)
void CSCTreeCtrl::undo_edit_label()
{
	if (!m_edit_item)
		return;

	SetItemText(m_edit_item, m_edit_old_text);
}

LRESULT CSCTreeCtrl::on_message_CSCStaticEdit(WPARAM wParam, LPARAM lParam)
{
	CSCStaticEditMessage* msg = (CSCStaticEditMessage*)wParam;

	if (msg == NULL || msg->pThis == NULL || msg->pThis != m_pEdit)
		return 0;

	//killfocus 는 OnKillFocus → SendMessage(notify_parent) 경로라 *재진입* 안전 차원에서 PostMessage 로 분리:
	//여기서 즉시 edit_end() 하면 ShowWindow(SW_HIDE) 가 OnKillFocus 내부에서 호출돼 mfc 의 default
	//OnKillFocus 가 stale state 위에서 crash 하는 case 가 있다. PostMessage 로 OnKillFocus 가 완료된 후
	//깨끗한 메시지 큐에서 edit_end 를 실행.
	if (msg->message == CSCStaticEdit::message_scstaticedit_killfocus)
	{
		PostMessage(WM_TREE_END_EDIT, 1, 0);
	}
	else if (msg->message == CSCStaticEdit::message_scstaticedit_enter)
	{
		edit_end(true);
		Invalidate();
	}
	else if (msg->message == CSCStaticEdit::message_scstaticedit_escape)
	{
		edit_end(false);
		Invalidate();
	}

	return 0;
}

//---- 편집 모드 외부 클릭 감지 (CSCListBox/CPathCtrl 동일 패턴) -----------------------------------------

void CSCTreeCtrl::install_edit_mouse_hook()
{
	if (m_mouse_hook != NULL)
		return;

	//WH_MOUSE 는 자기 스레드 메시지 큐만 감시 → 다른 앱 클릭은 보지 않는다(그쪽은 WM_KILLFOCUS 가 처리).
	//이 훅의 목적은 "같은 스레드(같은 dlg) 의 edit 바깥 클릭" 감지뿐이다.
	s_editing_tree = this;
	m_mouse_hook = ::SetWindowsHookEx(WH_MOUSE, edit_mouse_hook_proc, NULL, ::GetCurrentThreadId());
}

void CSCTreeCtrl::remove_edit_mouse_hook()
{
	if (m_mouse_hook != NULL)
	{
		::UnhookWindowsHookEx(m_mouse_hook);
		m_mouse_hook = NULL;
	}
	if (s_editing_tree == this)
		s_editing_tree = NULL;
}

LRESULT CALLBACK CSCTreeCtrl::edit_mouse_hook_proc(int code, WPARAM wParam, LPARAM lParam)
{
	if (code == HC_ACTION && s_editing_tree != NULL && s_editing_tree->m_pEdit != NULL)
	{
		if (wParam == WM_LBUTTONDOWN || wParam == WM_RBUTTONDOWN || wParam == WM_MBUTTONDOWN ||
			wParam == WM_NCLBUTTONDOWN || wParam == WM_NCRBUTTONDOWN || wParam == WM_NCMBUTTONDOWN)
		{
			MOUSEHOOKSTRUCT* mh = (MOUSEHOOKSTRUCT*)lParam;
			CRect rc_edit;
			s_editing_tree->m_pEdit->GetWindowRect(&rc_edit);

			//edit 내부 클릭은 캐럿 이동이므로 종료 금지. 바깥이면 종료를 post (클릭 자체는 그대로 통과시킴).
			if (!rc_edit.PtInRect(mh->pt))
				s_editing_tree->PostMessage(WM_TREE_END_EDIT, 0, 0);
		}
	}

	return ::CallNextHookEx(NULL, code, wParam, lParam);
}

LRESULT CSCTreeCtrl::on_end_edit_posted(WPARAM wParam, LPARAM lParam)
{
	edit_end(true);
	return 0;
}

void CSCTreeCtrl::set_log_font(LOGFONT lf)
{
	if (_tcslen(lf.lfFaceName) == 0)
		return;

	memcpy(&m_lf, &lf, sizeof(LOGFONT));
	reconstruct_font();
}

int CSCTreeCtrl::get_font_size()
{
	m_font_size = get_font_size_from_pixel_size(m_hWnd, m_lf.lfHeight);
	return m_font_size;
}

//예를 들어 폰트 크기를 10으로 설정하면
void CSCTreeCtrl::set_font_size(int font_size)
{
	if (font_size == 0)
		return;

	m_font_size = font_size;
	//For the MM_TEXT mapping mode,
	//you can use the following formula to specify 
	//a height for a font with a specified point size:
	m_lf.lfHeight = get_pixel_size_from_font_size(m_hWnd, m_font_size);
	reconstruct_font();
}

void CSCTreeCtrl::enlarge_font_size(bool enlarge)
{
	m_font_size = get_font_size();
	enlarge ? m_font_size++ : m_font_size--;
	m_lf.lfHeight = get_pixel_size_from_font_size(m_hWnd, m_font_size);
	reconstruct_font();
}

void CSCTreeCtrl::set_font_bold(bool bold)
{
	m_lf.lfWeight = (bold ? FW_BOLD : FW_NORMAL);
	reconstruct_font();
}

void CSCTreeCtrl::set_font_italic(bool italic)
{
	m_lf.lfItalic = italic;
	reconstruct_font();
}


BOOL CSCTreeCtrl::OnTvnItemexpanded(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;

	//expand/collapse 로 펼쳐진 항목 집합이 바뀌므로 콘텐츠 폭·가시항목 캐시 모두 재빌드 필요.
	//(collapse 는 GetCount() 가 그대로라 dirty 명시가 필수 — count 변화만으론 감지 안 됨.)
	m_content_width_dirty = true;
	m_visible_cache_dirty = true;

	//OnTvnItemexpanding 에서 SetRedraw(FALSE). 여기서 sync 후 SetRedraw(TRUE) + RedrawWindow 로 tree + 모든 child (overlay) invalidate.
	//Invalidate(FALSE) 는 tree client 만 invalidate — WS_CLIPCHILDREN 때문에 overlay 가 paint cycle 안 거치면 customdraw fill 이 overlay 영역에 노출됨.
	sync_scrollbar();
	SetRedraw(TRUE);
	RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);

	//확장/축소로 '펼쳐진 폴더 집합'이 바뀌었음을 parent 에 알림(dir watcher 재설정 등). expand/collapse 모두 이 알림이 온다.
	::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCTreeCtrl, (WPARAM) & (CSCTreeCtrlMessage(this, message_expand_changed, NULL)), (LPARAM)0);

	return FALSE;
}

//해당 아이템에 지정된 이미지 인덱스를 리턴한다.
//이 노드가 어떤 종류의 노드인지 구분하기 위해 사용된다.
int CSCTreeCtrl::get_image_index(HTREEITEM hItem)
{
	if (hItem == NULL)
		return -1;

	TVITEM item;
	item.mask = TVIF_HANDLE | TVIF_IMAGE;
	item.hItem = hItem;
	GetItem(&item);
	TRACE(_T("image index = %d\n"), item.iImage);
	return item.iImage;
}

void CSCTreeCtrl::full_row_selection(bool full_row)
{
	ModifyStyle(full_row ? 0 : TVS_FULLROWSELECT, full_row ? TVS_FULLROWSELECT : 0, 0);
	if (GetStyle() & TVS_FULLROWSELECT)
		TRACE(_T("full row\n"));
	else
		TRACE(_T("no full row\n"));
}

void CSCTreeCtrl::has_line(bool line)
{
	ModifyStyle(line ? 0 : TVS_HASLINES, line ? TVS_HASLINES : 0, 0);
}


void CSCTreeCtrl::OnNMCustomDraw(NMHDR* pNMHDR, LRESULT* pResult)
{
	//시도 — Win11 DarkMode_Explorer 의 selection 시 system accent box border 차단 위해 CDRF_SKIPDEFAULT 로 native item paint 자체 차단 + 우리가 background / icon / +/- glyph / text 모두 paint.
	//(기존 NOTIFYPOSTPAINT 방식 본문은 아래 #if 0 ... #endif 로 보존 — 미세 조정 실패 시 비교/복원 용.)
	LPNMCUSTOMDRAW pNMCustomDraw = (LPNMCUSTOMDRAW)pNMHDR;
	HTREEITEM hItem = (HTREEITEM)pNMCustomDraw->dwItemSpec;
	*pResult = CDRF_DODEFAULT;

	CDC dc;
	dc.Attach(pNMCustomDraw->hdc);

	switch (pNMCustomDraw->dwDrawStage)
	{
		case CDDS_PREPAINT:
			*pResult = CDRF_NOTIFYITEMDRAW | CDRF_NOTIFYPOSTPAINT;
			break;

		case CDDS_POSTPAINT:
		{
			if (m_bDragging && m_hInsertMarkItem && m_dropPosition != drop_on_item)
				draw_insert_mark(&dc);

			//우하단 코너는 OnNcPaint 가 NC 띠에서 cr_back 으로 칠한다(listctrl 동일). 여기서 client DC 로 다시 칠하면
			//NC 로 gw 축소된 GetClientRect 기준이라 진짜 코너보다 gw 위(세로바 바닥)에 엉뚱한 사각형이 생긴다
			//(listctrl 5f90861 에서 제거한 rcCornerBottom 잔재와 동일). cr_back 과 트랙색이 대비되는 테마에서만 노출.
			*pResult = CDRF_DODEFAULT;
			break;
		}

		case CDDS_ITEMPREPAINT:
		{
			CRect rcRow = pNMCustomDraw->rc;
			if (rcRow.IsRectEmpty())
			{
				*pResult = CDRF_DODEFAULT;
				break;
			}

			CRect rcText;
			GetItemRect(hItem, &rcText, TRUE);

			//paint shift H — tree 의 자체 max scroll 초과 분 (= m_h_scroll_pos - tree.si.nPos) 만큼 콘텐츠 좌측으로 추가 shift.
			//우측 끝 콘텐츠가 V overlay 좌측까지 노출. X shift 만이라 row Y 위치 영향 없음 → hover partial invalidate 부작용 없음.
			int over_shift = get_over_shift();
			if (over_shift > 0)
			{
				rcText.left  -= over_shift;
				rcText.right -= over_shift;
			}

			Gdiplus::Color crText = m_theme.cr_text;
			Gdiplus::Color crBack = m_theme.cr_back;
			int item_font_style = -1;	//항목별 font_style — 아래 per-item override 블록에서 채움.
			bool state_highlight = false;	//selection / hover / drop 강조 상태인가 — FULLROWSELECT off 시 라벨만 칠할지 판단.

			if (function_check_is_dim_text && function_check_is_dim_text(this, hItem))
				crText = m_theme.cr_text_dim;

			//chevron 처럼 라벨 강조영역 밖에 그려지는 요소용 — 선택/hover 상태색이 아닌 항목 본래 텍스트색.
			Gdiplus::Color crTextNormal = crText;

			//state 색 결정 — DropHilited > Selected > Hot. selected 가 hover 보다 의미상 우선 — selected 인 항목 위에 hover 가 와도 selected 색 유지.
			//드롭 하이라이트: 이 트리가 직접 드래그 중이 아니어도(local tree→remote tree 처럼 *다른 컨트롤*이 드래그 소스로
			//이 트리 위에 드롭하려는 경우) DropHilightItem 이 설정돼 있으면 표시한다. 기존 m_bDragging 조건은 자기 트리 내
			//드래그만 커버해, cross-control 드롭 시 대상 트리에 하이라이트가 전혀 안 뜨던 원인.
			if (m_use_drag_and_drop && GetDropHilightItem() != NULL && hItem == GetDropHilightItem())
			{
				//20260704 by claude. drop-hilight 항목이 그려지는 동안(드래그로 폴더 위 hover) 1초 뒤 자동 확장 타이머 무장.
				//m_bDragging 으로 가드하지 않는다 — cross-control 드래그(리스트→트리)에서는 대상 트리의 m_bDragging 이 false 라
				//가드하면 확장이 안 걸린다. 우클릭은 이제 OnRButtonDown 이 base 를 호출하지 않아 drop-hilight 를 만들지 않으므로
				//(GetDropHilightItem()==NULL) 여기 진입하지 않는다 → 우클릭 자동펼침 재발 없음. leave 시 clear_drop_highlight 가
				//SelectDropTarget(NULL) 하고 OnTimer 의 GetDropHilightItem() NULL 가드가 있어 스퍼리어스 확장도 없다.
				SetTimer(timer_expand_for_drag_hover, 1000, NULL);
				crText = m_theme.cr_text_dropHilited;
				crBack = m_theme.cr_back_dropHilited;
				state_highlight = true;
			}
			else if (pNMCustomDraw->uItemState & CDIS_SELECTED)
			{
				state_highlight = true;
				bool hot_too = (pNMCustomDraw->uItemState & CDIS_HOT) || hItem == m_hot_item;
				if (GetFocus() == this || m_in_editing || m_in_context_menu)
				{
					crText = m_theme.cr_text_selected;
					crBack = hot_too ? m_theme.cr_back_selected_hover : m_theme.cr_back_selected;
				}
				else if (hot_too)
				{
					//inactive + hot 위 selected — bg 는 cr_back_selected_hover (selected 보다 darker, white text 가 잘 보임).
					//그러나 active 의 cr_text_selected (순 white) 와 *살짝 구분* 시켜 inactive identity 유지.
					//cr_text_selected 를 32 단계 darken → light gray.
					crText = get_color(m_theme.cr_text_selected, -32);
					crBack = m_theme.cr_back_selected_hover;
				}
				else
				{
					crText = m_theme.cr_text_selected_inactive;
					crBack = m_theme.cr_back_selected_inactive;
				}
			}
			else if ((pNMCustomDraw->uItemState & CDIS_HOT) || hItem == m_hot_item)
			{
				//hover 배경 (cr_back_hover) 이 logo color +32 인 brand theme 들은 여전히 dark — dark text 가 안 보인다.
				//cr_text_hover 도 함께 적용해 contrast 확보.
				crText = m_theme.cr_text_hover;
				crBack = m_theme.cr_back_hover;
				state_highlight = true;
			}

			//항목별 스타일 override — map 에 지정된 항목은 상태색(hover/selected/drag)보다 *우선*. 지정한 속성만 덮고
			//지정 안 한 속성은 위 상태색 유지. (사용자 명시: 지정색이 있으면 hover/selected 여도 그 색을 따른다.)
			{
				auto it_style = m_item_styles.find(hItem);
				if (it_style != m_item_styles.end())
				{
					if (it_style->second.use_text_color)
					{
						crText = it_style->second.text_color;
						crTextNormal = it_style->second.text_color;
					}
					if (it_style->second.use_back_color)
						crBack = it_style->second.back_color;
					item_font_style = it_style->second.font_style;
				}
			}

			//1. row fill.
			//   V scrollbar visible 시에만 우측 m_scrollbar.get_width() reserve. hide 시 풀폭. m_v_visible_state 는 sync_scrollbar 가 정한 stable cache 라 paint cycle stale 안전.
			CRect rcClient;
			GetClientRect(&rcClient);
			int right_limit = rcClient.right - (m_scrollbar_setup && m_v_visible_state ? m_scrollbar.get_width() : 0);
			CRect rcFill(rcClient.left, rcRow.top, right_limit, rcRow.bottom);

			//TVS_FULLROWSELECT on → 강조도 풀폭. off → 일반 배경만 풀폭으로 칠하고 selection/hover/drop 강조는 라벨(rcText) 영역만.
			//per-item override 배경(state_highlight=false)은 selection 과 무관하므로 옵션과 관계없이 풀폭 유지.
			if ((GetStyle() & TVS_FULLROWSELECT) || !state_highlight)
			{
				dc.FillSolidRect(&rcFill, crBack.ToCOLORREF());
			}
			else
			{
				dc.FillSolidRect(&rcFill, m_theme.cr_back.ToCOLORREF());
				CRect rcLabel(rcText.left - 2, rcRow.top, rcText.right + 2, rcRow.bottom);
				dc.FillSolidRect(&rcLabel, crBack.ToCOLORREF());
			}

			//2. expand/collapse glyph — rcText.left 기준 역산 (H scroll 따라 같이 움직이도록).
			//   pNMCustomDraw->rc.left 는 client.left 고정이라 사용 시 H scroll 무관 고정 위치에 그려져 trail 현상 발생.
			//   button center = rcText.left - icon_w - (checkbox slot) - gap_icon_text - indent_step/2.
			if ((GetStyle() & TVS_HASBUTTONS) && ItemHasChildren(hItem))
			{
				int level = get_indent_level(hItem);
				int indent_step = (int)SendMessage(TVM_GETINDENT, 0, 0);
				if (indent_step <= 0)
					indent_step = 19;

				//LINESATROOT 없으면 root level (0) 은 button column 없음 — 그릴 자리도 없음.
				if ((GetStyle() & TVS_LINESATROOT) || level > 0)
				{
					int icon_w_for_glyph = 0;
					HIMAGELIST hil_g = (HIMAGELIST)SendMessage(TVM_GETIMAGELIST, TVSIL_NORMAL, 0);
					if (hil_g)
					{
						int cx_il = 0, cy_il = 0;
						ImageList_GetIconSize(hil_g, &cx_il, &cy_il);
						icon_w_for_glyph = cx_il;
					}
					int state_offset = (GetStyle() & TVS_CHECKBOXES) ? (16 + 3) : 0;	//state_w + gap_check_icon
					int gx = rcText.left - icon_w_for_glyph - 3 - state_offset - indent_step / 2;	//-3 = gap_icon_text
					int gy = (rcRow.top + rcRow.bottom) / 2;

					//탐색기 트리의 chevron 은 폰트 글리프가 아니라 *얇은 벡터*다 — 폰트 글리프(MDL2/Fluent)는
					//같은 크기여도 더 두껍게 나온다. 그래서 얇은 폴리라인으로 직접 그려 두께를 제어한다.
					//★ 두께(chevron_thickness) 한 값만 조정하면 탐색기와 두께를 맞출 수 있다.
					//색은 항목 텍스트색(crText)을 get_weak_color 로 48 만큼 흐리게 — cr_text 원색은 너무
					//진해 탐색기 chevron 의 옅은 톤과 다르다. crText 는 상태 반영색이라 선택 시 cr_text_selected
					//기준으로 흐려진다.
					const float chevron_thickness = 1.6f;

					Gdiplus::Graphics g(dc.m_hDC);
					g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
					g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);

					//전체 행 선택이면 chevron 이 선택 배경 위에 오므로 상태색(crText)으로 대비시킨다. 아니면 chevron 은
					//라벨 강조 밖(일반 배경) 이라 선택색을 쓰면 배경에 안 묻고 어색 — 항목 본래색(crTextNormal)을 쓴다.
					Gdiplus::Color crGlyph = ((GetStyle() & TVS_FULLROWSELECT) || !state_highlight) ? crText : crTextNormal;
					//get_weak_color(절대 offset) 대신 get_color(glyph, cr_back, ratio) (대비 비례) — 저대비 테마에서 chevron 이
					//배경에 묻히는 부작용 방지. ratio 0.2 ≒ 기존 offset 48.
					Gdiplus::Pen pen(get_color(crGlyph, m_theme.cr_back, 0.2), chevron_thickness);
					pen.SetStartCap(Gdiplus::LineCapRound);
					pen.SetEndCap(Gdiplus::LineCapRound);
					pen.SetLineJoin(Gdiplus::LineJoinRound);

					bool expanded = (GetItemState(hItem, TVIS_EXPANDED) & TVIS_EXPANDED) != 0;
					Gdiplus::PointF pts[3];
					if (expanded)
					{
						//v
						pts[0] = Gdiplus::PointF((Gdiplus::REAL)(gx - 3.5f), (Gdiplus::REAL)(gy - 1.5f));
						pts[1] = Gdiplus::PointF((Gdiplus::REAL)(gx + 0.0f), (Gdiplus::REAL)(gy + 1.5f));
						pts[2] = Gdiplus::PointF((Gdiplus::REAL)(gx + 3.5f), (Gdiplus::REAL)(gy - 1.5f));
					}
					else
					{
						//>
						pts[0] = Gdiplus::PointF((Gdiplus::REAL)(gx - 1.5f), (Gdiplus::REAL)(gy - 3.5f));
						pts[1] = Gdiplus::PointF((Gdiplus::REAL)(gx + 1.5f), (Gdiplus::REAL)(gy + 0.0f));
						pts[2] = Gdiplus::PointF((Gdiplus::REAL)(gx - 1.5f), (Gdiplus::REAL)(gy + 3.5f));
					}
					g.DrawLines(&pen, pts, 3);
				}
			}

			//3. checkbox (state image) + 4. icon — 간격 변수로 통합 관리.
			int icon_w = 0;
			HIMAGELIST hil_n = (HIMAGELIST)SendMessage(TVM_GETIMAGELIST, TVSIL_NORMAL, 0);
			if (hil_n)
			{
				int cx_il = 0, cy_il = 0;
				ImageList_GetIconSize(hil_n, &cx_il, &cy_il);
				icon_w = cx_il;
			}
			const int gap_icon_text = 3;	//icon right ↔ text rcText.left 사이
			const int gap_check_icon = 3;	//checkbox right ↔ icon left 사이
			const int state_w = 16;

			if (GetStyle() & TVS_CHECKBOXES)
			{
				CRect rcCheck = get_checkbox_rect(hItem);
				//get_checkbox_rect 은 unshifted GetItemRect 기준 → over-scroll 분만큼 좌측으로 같이 이동시켜
				//아이콘/글자(이미 over_shift 반영된 rcText 기준)와 정렬 일치시킨다.
				rcCheck.OffsetRect(-over_shift, 0);
				rcCheck.DeflateRect(1, 1);

				int check_state = (GetCheck(hItem) != 0) ? BST_CHECKED : BST_UNCHECKED;

				Gdiplus::Graphics g(dc.m_hDC);
				Gdiplus::Rect box(rcCheck.left, rcCheck.top, rcCheck.Width(), rcCheck.Height());
				//체크마크 색: get_weak_color(절대 offset) 대신 cr_text_dim 을 cr_back 쪽으로 대비 비례 blend(ratio 0.13 ≒ 기존 offset 32) — 저대비 테마 안전.
				draw_check_box(&g, box, check_state, m_theme.cr_text_dim, m_theme.cr_back, get_color(m_theme.cr_text_dim, m_theme.cr_back, 0.13));
			}

			int img_normal = -1, img_selected = -1;
			GetItemImage(hItem, img_normal, img_selected);
			bool is_sel = (pNMCustomDraw->uItemState & CDIS_SELECTED) != 0;
			int img_use = (is_sel && img_selected >= 0) ? img_selected : img_normal;
			if (img_use >= 0 && hil_n)
			{
				int cx_il = 0, cy_il = 0;
				ImageList_GetIconSize(hil_n, &cx_il, &cy_il);
				int ix = rcText.left - cx_il - gap_icon_text;
				int iy = (rcRow.top + rcRow.bottom - cy_il) / 2;
				ImageList_Draw(hil_n, img_use, dc.m_hDC, ix, iy, ILD_TRANSPARENT);
			}

			//5. text.
			CString text = GetItemText(hItem);
			CRect rcDraw = rcText;
			rcDraw.left += 2;
			if (rcDraw.right > right_limit)
				rcDraw.right = right_limit;
			dc.SetBkMode(TRANSPARENT);
			dc.SetTextColor(crText.ToCOLORREF());

			//항목별 font_style (Bold/Italic/Underline/Strikeout) — base 폰트에서 플래그만 입혀 임시 폰트 select.
			CFont item_font;
			CFont* old_font = nullptr;
			if (item_font_style >= 0)
			{
				LOGFONT lf = {};
				CFont* base_font = GetFont();
				if (base_font)
					base_font->GetLogFont(&lf);
				if (item_font_style & Gdiplus::FontStyleBold)		lf.lfWeight = FW_BOLD;
				if (item_font_style & Gdiplus::FontStyleItalic)		lf.lfItalic = TRUE;
				if (item_font_style & Gdiplus::FontStyleUnderline)	lf.lfUnderline = TRUE;
				if (item_font_style & Gdiplus::FontStyleStrikeout)	lf.lfStrikeOut = TRUE;
				if (item_font.CreateFontIndirect(&lf))
					old_font = dc.SelectObject(&item_font);
			}

			dc.DrawText(text, &rcDraw, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

			if (old_font)
				dc.SelectObject(old_font);

			*pResult = CDRF_SKIPDEFAULT;
			break;
		}
	}

	dc.Detach();
	return;
}

//기존 NM_CUSTOMDRAW 본문 보존 — native item paint 후 우리 POSTPAINT 에서 background/text override 방식. SetWindowTheme(DarkMode_Explorer) 도입 후 selection 시 native 가 system accent border 추가 그려 양옆 형광 띠 보이는 문제로 신규 방식 (위) 으로 시도 중. #if 0 으로 비활성, 비교/복원 용.
#if 0
void CSCTreeCtrl::OnNMCustomDraw_old(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	LPNMCUSTOMDRAW pNMCustomDraw = (LPNMCUSTOMDRAW)pNMHDR;
	LPNMTVCUSTOMDRAW pNMTVCustomDraw = (LPNMTVCUSTOMDRAW)pNMHDR;
	HTREEITEM hItem = (HTREEITEM)pNMCustomDraw->dwItemSpec;
	*pResult = CDRF_DODEFAULT;

	CDC dc;
	dc.Attach(pNMCustomDraw->hdc);

	CRect rtemp, rcItem;
	switch (pNMCustomDraw->dwDrawStage)
	{
		case CDDS_PREPAINT:
			*pResult = (CDRF_NOTIFYPOSTPAINT | CDRF_NOTIFYITEMDRAW);
			//*pResult = (CDRF_NOTIFYITEMDRAW);
			break;
		case CDDS_POSTPAINT:
		{
			// 모든 아이템이 그려진 후 삽입 마크를 그려준다.
			if (m_bDragging && m_hInsertMarkItem && m_dropPosition != drop_on_item)
			{
				draw_insert_mark(&dc);
			}
			*pResult = CDRF_DODEFAULT;
			break;
		}
		case CDDS_ITEMPREPAINT:
			rcItem = pNMCustomDraw->rc;
			GetItemRect(hItem, &rtemp, TRUE);
			if (rcItem.IsRectEmpty())
			{
				*pResult = CDRF_DODEFAULT;
				break;
			}

			*pResult = CDRF_NOTIFYPOSTPAINT;
			break;

		case CDDS_ITEMPOSTPAINT:
		{
			GetItemRect(hItem, &rcItem, TRUE);
			//rcItem.left -= 2;

			Gdiplus::Color crText = m_theme.cr_text;
			Gdiplus::Color crBack = m_theme.cr_back;

			//간혹 특정 조건의 아이템의 색상을 달리 표현할 필요가 있는데
			//이를 판별하는 것은 parent에서 하도록 해야 이 컨트롤이 범용성을 유지한다.
			//이 범용 컨트롤에 특정 처리코드를 넣는다면 범용성은 깨지게 된다.
			//parent의 특정 처리 함수 포인터를 지정받아서 호출하고 그 결과에 따라 정해진 액션을 취한다.
			//이 컨트롤의 CWnd*와 아이템 핸들을 건네주면 parent에서 판별하여 리턴한다.
			if (function_check_is_dim_text && function_check_is_dim_text(this, hItem))
				crText = m_theme.cr_text_dim;


			int nOldBkMode = dc.SetBkMode(TRANSPARENT);
			
			//우선순위: CDIS_DROPHILITED > CDIS_SELECTED > CDIS_HOT. selected 항목 위 hover 가 와도 selected 색 유지.
			//else if (pNMCustomDraw->uItemState & CDIS_DROPHILITED)	//이건 동작안한다.
			if (m_use_drag_and_drop && m_bDragging && (hItem == GetDropHilightItem()))// */pNMCustomDraw->uItemState & CDIS_DROPHILITED)
			{
				//TRACE(_T("%s, CDIS_DROPHILITED\n"), GetItemText(hItem));
				//drop을 위해 폴더위에 머무를 경우 해당 폴더가 expand가 아니면 expand시켜준다.
				SetTimer(timer_expand_for_drag_hover, 1000, NULL);

				//TRACE(_T("CDIS_DROPHILITED\n"));

				crText = m_theme.cr_text_dropHilited;//VSLC_TREEVIEW_FOCUS_FONT_COLOR;
				crBack = m_theme.cr_back_dropHilited;
			}
			else if (pNMCustomDraw->uItemState & CDIS_SELECTED)
			{
				//TRACE(_T("%s, CDIS_SELECTED\n"), GetItemText(hItem));
				bool hot_too = (pNMCustomDraw->uItemState & CDIS_HOT) != 0;
				if (GetFocus() == this)
				{
					crText = m_theme.cr_text_selected;
					crBack = hot_too ? m_theme.cr_back_selected_hover : m_theme.cr_back_selected;
				}
				else
				{
					crText = m_theme.cr_text_selected_inactive;
					crBack = hot_too ? m_theme.cr_back_selected_hover : m_theme.cr_back_selected_inactive;
				}
			}
			else if (pNMCustomDraw->uItemState & CDIS_HOT)
			{
				//TRACE(_T("%s, CDIS_HOT\n"), GetItemText(hItem));
				//crText = m_cr_text_selected;
				crBack = m_theme.cr_back_hover;
			}
			//else if (pNMCustomDraw->uItemState & CDIS_FOCUS)
			//{
			//	TRACE(_T("CDIS_FOCUS\n"));
			//	crText = m_cr_text_selected;
			//	crBack = m_cr_back_selected;
			//}

			
			//if (pNMCustomDraw->uItemState & CDIS_SELECTED)
			{
				//TRACE(_T("%s, CDIS_SELECTED\n"), GetItemText(hItem));
				//배경색으로 그려주지 않으면 기본 텍스트 출력 + 커스텀 출력 텍스트가 중복되서 표시되므로
				//배경색을 칠해서 기본 텍스트 출력을 가리고 커스텀 출력을 해줘야 한다.
				if (GetFocus() == this && (pNMCustomDraw->uItemState & CDIS_SELECTED))
					draw_rect(&dc, rcItem, m_theme.cr_selected_border, crBack);
				else
					draw_rect(&dc, rcItem, crBack, crBack);
			}
			/*
			else
			{
				dc.FillSolidRect(&rcItem, crBack.ToCOLORREF());
			}
			*/

			if (pNMCustomDraw->uItemState & CDIS_FOCUS)
			{
				//TRACE(_T("CDIS_FOCUS\n"));
				//draw_rectangle(&dc, rcItem, m_cr_selected_border);
				//dc.DrawFocusRect(rcItem);
			}

			/*
			switch (pIData->level)
			{
			case 0:
			case 1:
				hOldFont = (HFONT)dc.SelectObject(pMainWnd->m_fontBold);
				break;
			default:
				hOldFont = (HFONT)dc.SelectObject(pMainWnd->m_fontRegular);
				break;
			}
			*/

			//CRect rIcon = rcItem;
			////rIcon.left = rcItem.left - m_image_size;
			//rIcon.right = rIcon.left + m_image_size;// VSLC_TREEVIEW_ICON_WIDTH;
			//m_imagelist.Draw(&dc, 0, CPoint(rIcon.CenterPoint().x - m_image_size / 2, rIcon.CenterPoint().y - m_image_size / 2), ILD_TRANSPARENT);

			CRect rText = rcItem;
			rText.left += 2;
			//CSize szText = dc.GetTextExtent(GetItemText(hItem));
			//rText.OffsetRect(m_image_size, 0);
			//rText.right = rText.left + szText.cx;
			dc.SetBkMode(TRANSPARENT);
			dc.SetTextColor(crText.ToCOLORREF());
			dc.DrawText(GetItemText(hItem), &rText, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

			//dc.DrawText(GetItemText(hItem), &rcItem, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

			/*
			for (int j = 0; j < m_ImageInfo.GetCount(); j++)
			{
				if (pIData->eStStyle & m_ImageInfo.GetAt(m_ImageInfo.FindIndex(j))->eKind)
				{
					m_ImageList->Draw(&dc, j, rcIcon.TopLeft(), ILD_TRANSPARENT);
					rcIcon.left = rcIcon.right;
					rcIcon.right = rcIcon.right + VSLC_TREEVIEW_ICON_WIDTH;
				}
			}
			*/

			//dc.SelectObject(hOldFont);
			* pResult = CDRF_DODEFAULT;
			break;
		}
	}

	dc.Detach();

	//*pResult = 0;
}
#endif


void CSCTreeCtrl::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (nIDEvent == timer_expand_for_drag_hover)
	{
		KillTimer(timer_expand_for_drag_hover);
		//1초 뒤 발화라 그 사이 드래그가 항목을 벗어났으면 GetDropHilightItem()==NULL → Expand(NULL) 방지.
		HTREEITEM h = GetDropHilightItem();
		if (h != NULL)
			Expand(h, TVE_EXPAND);
	}
	else if (nIDEvent == timer_drag_auto_scroll)
	{
		if (!m_bDragging || m_drag_scroll_target == NULL || (m_drag_scroll_vx == 0 && m_drag_scroll_vy == 0))
		{
			KillTimer(timer_drag_auto_scroll);
		}
		else
		{
			//m_pDropWnd 가 아니라 m_drag_scroll_target — 커서가 오버레이 스크롤바 위여도 실제 트리/리스트로 보낸다.
			//20260707 by claude. 대상의 구체 타입(CVtListCtrlEx/CSCListCtrl/CSCTreeCtrl)을 몰라도 Message_DragScrollBy 로 위임한다.
			//이들은 자체 오버레이 가로바(m_h_scroll_pos) 모델이라 raw Scroll() 로 밀면 바가 desync 되므로, 각자 handler 에서
			//drag_scroll_by(m_h_scroll_pos + sync_scrollbar 동기)를 수행하고 1 을 반환한다. 미처리(일반 CListCtrl/CTreeCtrl)면 폴백.
			//(레이어드 드래그 이미지라 화면 lock(DragShowNolock) 불필요.)
			LRESULT handled = m_drag_scroll_target->SendMessage(Message_DragScrollBy,
				(WPARAM)(m_drag_scroll_vx * 30), (LPARAM)m_drag_scroll_vy);	//가로 px, 세로 라인 수

			if (!handled)
			{
				if (m_drag_scroll_target->IsKindOf(RUNTIME_CLASS(CListCtrl)))
				{
					//일반 CListCtrl(오버레이 바 없음)은 WM_*SCROLL SB_LINE 이 안 먹으므로 raw Scroll(픽셀).
					CListCtrl* pl = (CListCtrl*)m_drag_scroll_target;
					pl->Scroll(CSize(m_drag_scroll_vx * 30, m_drag_scroll_vy * 20));
					pl->UpdateWindow();
				}
				else
				{
					//일반 CTreeCtrl 등 — SB_LINE 경로.
					for (int i = 0; i < abs(m_drag_scroll_vy); i++)
						m_drag_scroll_target->SendMessage(WM_VSCROLL, (m_drag_scroll_vy < 0) ? SB_LINEUP   : SB_LINEDOWN);
					for (int i = 0; i < abs(m_drag_scroll_vx); i++)
						m_drag_scroll_target->SendMessage(WM_HSCROLL, (m_drag_scroll_vx < 0) ? SB_LINELEFT : SB_LINERIGHT);
				}
			}
		}
	}

	CTreeCtrl::OnTimer(nIDEvent);
}

//드래그 중, 대상 컨트롤(m_pDropWnd) 가장자리 근처면 자동 스크롤 방향/속도(level)를 정하고 타이머로 연속 스크롤한다.
//속도는 가장자리와의 '거리에 비례'(가까울수록 빠름). 상/하/좌/우 4방향. screen_pt = 스크린 좌표. (CVtListCtrlEx 와 동일 규칙)
void CSCTreeCtrl::update_drag_auto_scroll(CPoint screen_pt)
{
	m_drag_scroll_vx = 0;
	m_drag_scroll_vy = 0;

	//오버레이 스크롤바/헤더는 부모 다이얼로그의 자식이라 커서가 그 위면 WindowFromPoint(→m_pDropWnd)가 트리/리스트가
	//아니게 된다. → 직전까지 유효했던 트리/리스트를 스크롤 대상으로 유지하고, 커서가 그 컨트롤 window rect 밖이면 해제.
	if (m_pDropWnd && (m_pDropWnd->IsKindOf(RUNTIME_CLASS(CTreeCtrl)) || m_pDropWnd->IsKindOf(RUNTIME_CLASS(CListCtrl))))
		m_drag_scroll_target = m_pDropWnd;

	if (m_drag_scroll_target)
	{
		CRect rw;
		m_drag_scroll_target->GetWindowRect(rw);

		//[의도적] rc 를 벗어나면 스크롤을 멈추던 두 게이트(!rwin.PtInRect → target=NULL, rw.PtInRect 안쪽만 계산)를 제거.
		//요구사항이 "SetCapture 처럼 rc 를 벗어나도 드래그·스크롤 계속"이므로 아래 거리 공식만으로 판단한다: 가운데면
		//d>=MARGIN 이라 자연히 0(스크롤 안 함), 가장자리에 가까워지면 시작, rc 밖으로(d 음수) 멀어질수록 빨라져 MAX_LEVEL 로 clamp.
		//커서가 오버레이 스크롤바(=부모 dlg 의 자식) 위여서 WindowFromPoint 가 트리/리스트를 못 짚어도, 위에서
		//m_drag_scroll_target 을 직전 유효 컨트롤로 유지하므로 계속 동작. (CVtListCtrlEx 와 동일 규칙)
		//리스트 대상은 상단 컬럼 헤더를 세로 트리거 존에서 제외(헤더 위에서 세로 스크롤이 걸리는 것 방지).
		int top_ref = rw.top;
		if (m_drag_scroll_target->IsKindOf(RUNTIME_CLASS(CListCtrl)))
		{
			CHeaderCtrl* ph = ((CListCtrl*)m_drag_scroll_target)->GetHeaderCtrl();
			if (ph && ph->GetSafeHwnd() && ph->IsWindowVisible())
			{
				CRect rh; ph->GetWindowRect(rh);
				if (rh.bottom > rw.top && rh.bottom < rw.bottom)
					top_ref = rh.bottom;
			}
		}

		const int MARGIN    = 48;	//가장자리 감지 폭(px)
		const int MAX_LEVEL = 3;	//tick 당 최대 스크롤 단위(level)

		//가장자리로부터의 거리 d(안쪽 양수 / 넘어가면 음수). d<MARGIN 이면 스크롤: 가장자리에 가깝거나 넘을수록 빠르게(최대 MAX).
		int dl = screen_pt.x - rw.left;			//왼쪽 가장자리까지
		int dr = rw.right - screen_pt.x;		//오른쪽
		int dt = screen_pt.y - top_ref;			//위(리스트면 헤더 아래 기준)
		int db = rw.bottom - screen_pt.y;		//아래

		if (dl < MARGIN)      m_drag_scroll_vx = -min(MAX_LEVEL, 1 + (MARGIN - dl) * (MAX_LEVEL - 1) / MARGIN);
		else if (dr < MARGIN) m_drag_scroll_vx =  min(MAX_LEVEL, 1 + (MARGIN - dr) * (MAX_LEVEL - 1) / MARGIN);

		if (dt < MARGIN)      m_drag_scroll_vy = -min(MAX_LEVEL, 1 + (MARGIN - dt) * (MAX_LEVEL - 1) / MARGIN);
		else if (db < MARGIN) m_drag_scroll_vy =  min(MAX_LEVEL, 1 + (MARGIN - db) * (MAX_LEVEL - 1) / MARGIN);
	}

	if (m_drag_scroll_vx != 0 || m_drag_scroll_vy != 0)
		SetTimer(timer_drag_auto_scroll, 70, NULL);	//~14fps 연속 스크롤(마우스가 멈춰 있어도 계속)
	else
		KillTimer(timer_drag_auto_scroll);
}

//20260704 by claude. 드래그 자동스크롤 실행 경로. 가로는 m_h_scroll_pos 를 px 단위로 직접 옮기고(OnHScroll 과 동일 clamp)
//sync_scrollbar 로 오버레이 바까지 맞춘다 — SB_LINELEFT(60px, 휠/스크롤바 공유)를 쓰지 않아 더 세밀·느린 가로 스텝이 가능.
//세로는 기존과 같이 SB_LINE 라인 단위. (자기 트리·다른 트리 인스턴스 모두 이 메서드로 스크롤)
void CSCTreeCtrl::drag_scroll_by(int dx_px, int dy_lines)
{
	if (!::IsWindow(m_hWnd))
		return;

	for (int i = 0; i < abs(dy_lines); i++)
		SendMessage(WM_VSCROLL, (dy_lines < 0) ? SB_LINEUP : SB_LINEDOWN);

	if (dx_px != 0)
	{
		CRect rc;
		GetClientRect(&rc);
		int right_limit_h = rc.Width() - (m_v_visible_state ? m_scrollbar.get_width() : 0);
		int max_pos = max(0, m_h_content_width - right_limit_h);
		int new_pos = m_h_scroll_pos + dx_px;
		if (new_pos < 0)       new_pos = 0;
		if (new_pos > max_pos) new_pos = max_pos;
		m_h_scroll_pos = new_pos;
	}

	Invalidate(FALSE);
	sync_scrollbar();
}

//20260707 by claude. 다른 SCTreeCtrl 에서 드래그해 이 트리로 자동스크롤 위임될 때 받는다. drag_scroll_by 로 위임 후 1 반환(=처리됨).
LRESULT CSCTreeCtrl::on_message_DragScrollBy(WPARAM wParam, LPARAM lParam)
{
	drag_scroll_by((int)wParam, (int)lParam);
	return 1;
}


//context menu를 컨트롤 내부에서 처리하면 레이블 변경, 삭제, 추가 등의 일반적인 메뉴항목들을 처리하는 것이 간단해지지만
//로그를 남기는 등 별도의 추가 처리가 복잡해진다. 따라서 가능한 한 parent에서 처리하는 것이 맞다.
void CSCTreeCtrl::OnContextMenu(CWnd* pWnd, CPoint point)
{
	//우클릭되면 해당 아이템을 SELECTED로 수동 설정해야 한다.
	if (point.x == -1 && point.y == -1)
		point = (CPoint)GetMessagePos();

	UINT uFlags;
	HTREEITEM hItem;
	CPoint pt = point;
	ScreenToClient(&pt);
	hItem = HitTest(pt, &uFlags);

	if (hItem == NULL)
		return;

	SelectItem(hItem);
	//드래그가 아닌 우클릭/메뉴키 진입이므로, 혹시 남아있을 drop-hilight 를 제거해 selected 색으로 표시(방어).
	SelectDropTarget(NULL);

	//자체 popup 사용 안 하면 parent 로 forward — wParam = source hwnd 로 parent OnContextMenu 가
	//pWnd 로 tree/thumb 구분 가능.
	if (!m_use_own_context_menu)
	{
		CWnd* parent = GetParent();
		if (parent)
		{
			//menu 표시 중 focus 가 menu 로 가도 selected 항목이 active 색으로 그려지도록 가드.
			//parent->SendMessage 는 popup_menu 가 modal 로 끝날 때까지 return 안 함.
			m_in_context_menu = true;
			parent->SendMessage(WM_CONTEXTMENU, (WPARAM)GetSafeHwnd(), MAKELPARAM(point.x, point.y));
			m_in_context_menu = false;
			CRect rc_item;
			if (GetItemRect(hItem, &rc_item, FALSE))
				InvalidateRect(rc_item, FALSE);
		}
		return;
	}

	LANGID langID = GetSystemDefaultUILanguage();

	/*
	if (m_menu.m_hWnd == NULL)
	{
		m_menu.create(this, 160);
		m_menu.add(menu_add_item, (m_is_shell_treectrl ? _T("새 폴더(&N)") : _T("새 항목(&N)")));

		//shell treectrl일 경우 rename, delete은 위험하므로 여기서는 허용하지 않는다.
		if (m_is_shell_treectrl)
		{
			m_menu.add(-1);
			m_menu.add(menu_property, _T("속성(&R)"));
		}
		else
		{
			m_menu.add(menu_rename_item, (m_is_shell_treectrl ? _T("이름 바꾸기(&M)") : _T("이름 바꾸기(&M)")));
			m_menu.add(-1);
			m_menu.add(menu_delete_item, (m_is_shell_treectrl ? _T("삭제(&D)") : _T("삭제(&D)")));
		}
	}
	*/
	CMenu	menu;
	menu.CreatePopupMenu();

	//menu.AppendMenu(MF_STRING, menu_favorite, _T("새로고침\tF5"));
	//menu.AppendMenu(MF_SEPARATOR);
	menu.AppendMenu(MF_STRING, menu_refresh, _T("새로고침\tF5"));
	menu.AppendMenu(MF_SEPARATOR);
	menu.AppendMenu(MF_STRING, menu_add_item, (m_is_shell_treectrl ? _T("새 폴더(&N)") : _T("새 항목(&N)")));

	//shell treectrl일 경우 rename, delete은 위험하므로 여기서는 허용하지 않는다.
	if (m_is_shell_treectrl)
	{
		menu.AppendMenu(MF_SEPARATOR);
		menu.AppendMenu(MF_STRING, menu_property, _T("속성(&R)"));
	}
	else
	{
		menu.AppendMenu(MF_SEPARATOR);
		menu.AppendMenu(MF_STRING, menu_rename_item, (m_is_shell_treectrl ? _T("이름 바꾸기(&M)") : _T("이름 바꾸기(&M)")));
		menu.AppendMenu(MF_SEPARATOR);
		menu.AppendMenu(MF_STRING, menu_delete_item, (m_is_shell_treectrl ? _T("삭제(&D)") : _T("삭제(&D)")));
	}

	//메뉴 표시(modal) 중 focus 가 menu 로 가도 우클릭한 항목이 active 색으로 유지되도록 가드 — parent-forward 분기와 동일.
	//(기존엔 OnNMRClick 이 WM_CONTEXTMENU 전송 전체에 이 플래그를 걸어 양 분기를 커버했음. OnNMRClick 제거로 여기 이관.)
	m_in_context_menu = true;
	menu.TrackPopupMenu(TPM_LEFTALIGN, point.x, point.y, this);
	m_in_context_menu = false;
	menu.DestroyMenu();

	CRect rc_item;
	if (GetItemRect(hItem, &rc_item, FALSE))
		InvalidateRect(rc_item, FALSE);
}

void CSCTreeCtrl::OnPopupMenu(UINT nMenuID)
{
	switch (nMenuID)
	{
		case menu_add_item:
		{
			add_new_item(NULL, _T("새 폴더"), true, true);
			break;
		}
		case menu_refresh :
		{
			refresh(GetSelectedItem());
			break;
		}
		case menu_rename_item:
		{
			rename_item();
			break;
		}
		case menu_delete_item:
		{
			delete_item();
			break;
		}
		case menu_property:
		{
			HTREEITEM hItem = GetSelectedItem();
			if (hItem)
			{
				CString path = m_pShellImageList->convert_special_folder_to_real_path(!m_is_local, get_path(hItem));
				if (m_is_local)
				{
					show_property_window(std::deque<CString> {path});
				}
				else
				{
					::SendMessage(GetParent()->GetSafeHwnd(),
						Message_CSCTreeCtrl,
						(WPARAM) & (CSCTreeCtrlMessage(this, message_request_property, NULL, path)), 0);
				}
			}
			break;
		}
	}
}

LRESULT CSCTreeCtrl::OnMessageCSCMenu(WPARAM wParam, LPARAM lParam)
{
	CSCMenuMessage* msg = (CSCMenuMessage*)wParam;

	if (msg->m_message == CSCMenu::message_scmenu_selchanged)
	{
		TRACE(_T("m_message = %d, m_menu_item->m_id = %d\n"), msg->m_message, msg->m_menu_item->m_id);
		switch (msg->m_menu_item->m_id)
		{
			case menu_add_item :
			{
				add_new_item();
				break;
			}
			case menu_rename_item:
			{
				rename_item();
				break;
			}
			case menu_delete_item:
			{
				delete_item();
				break;
			}
			case menu_property :
			{
				HTREEITEM hItem = GetSelectedItem();
				if (hItem)
				{
					CString path = m_pShellImageList->convert_special_folder_to_real_path(!m_is_local, get_path(hItem));
					show_property_window(std::deque<CString> {path});
				}
				break;
			}
		}
	}

	return 0;
}

//hParent 항목 아래 하위 항목을 추가한다. NULL이면 현재 선택된 항목이 hParent가 된다.
//label이 ""이면 기본 "새 폴더"명으로 추가한 후 edit_item() 호출.
//auto_index = true라면 새 폴더, 새 항목이 이미 존재할 경우 뒤에 숫자를 증가시켜 붙여줘야 한다.
HTREEITEM CSCTreeCtrl::add_new_item(HTREEITEM hParent, CString label, bool auto_index, bool edit_mode)
{
	if (hParent == NULL)
		hParent = GetSelectedItem();
	if (hParent == NULL)
		hParent = NULL;

	HTREEITEM hItem = NULL;

	if (m_is_shell_treectrl)
	{
		TRACE(_T("parent = %s\n"), get_path(hParent));

		//아직 펼쳐진 적이 없는 폴더라면 우선 서브 폴더들을 추가해준 후 새 폴더를 추가해야 한다.
		//if (GetChildItem(hParent) == NULL)
		//	insert_folder(hParent, get_path(hParent));
		Expand(hParent, TVE_EXPAND);

		//hItem = InsertItem(label.IsEmpty() ? _T("새 폴더") : label,
		//	m_pShellImageList->GetSystemImageListIcon(_T("C:\\windows")),
		//	m_pShellImageList->GetSystemImageListIcon(_T("C:\\windows")) + 1, hParent);

		CString path = m_pShellImageList->convert_special_folder_to_real_path(!m_is_local, get_path(hParent));

		if (auto_index)
		{
			int index = 0;

			//우선 새 폴더의 인덱스를 구하고
			if (m_is_local)
			{
				index = get_file_index(path, label);
			}
			else
			{
				::SendMessage(GetParent()->GetSafeHwnd(),
					Message_CSCTreeCtrl,
					(WPARAM) & (CSCTreeCtrlMessage(this, message_request_new_folder_index, NULL, path, label)),
					(LPARAM)&index);
			}

			if (index > 1)
				label.AppendFormat(_T(" (%d)"), index);
		}

		path = concat_path(path, label);

		BOOL res = FALSE;

		//local이면 폴더를 직접 생성하고 remote이면 폴더 생성 요청을 보내고 그 결과를 res로 받는다.
		if (m_is_local)
		{
			if (PathFileExists(path))
				res = true;
			else
				res = CreateDirectory(path, NULL);
		}
		else
		{
			::SendMessage(GetParent()->GetSafeHwnd(),
				Message_CSCTreeCtrl,
				(WPARAM) & (CSCTreeCtrlMessage(this, message_request_new_folder, NULL, path)),
				(LPARAM)&res);
		}

		if (res)
		{
			//fn_name이지만 fullpath가 폴더경로를 담고 있으므로 실제로는 맨 끝 폴더명을 리턴한다.
			path = get_part(path, fn_name);
			hItem = InsertItem(path,
				m_pShellImageList->GetSystemImageListIcon(0, _T("C:\\windows")),
				m_pShellImageList->GetSystemImageListIcon(0, _T("C:\\windows")),
				hParent);
		}
	}
	else
	{
		hItem = InsertItem(label.IsEmpty() ? _T("새 항목") : label, hParent);
		SelectItem(hItem);
	}


	if (hItem)
	{
		EnsureVisible(hItem);

		//새 항목을 추가하고 그 항목을 선택상태로 만드는 경우
		//list에서 새 폴더 추가 후 성공하면 트리에도 새 폴더를 만들어주는데 SelectItem()을 수행하면
		//list의 경로가 변경되면서 edit_end()할 때 오류가 발생한다.
		//일단 SelectItem()은 스킵한다.
		//SelectItem(hItem);

		//원래 하위 노드가 없는 상태에서 새 노드가 추가되면 cChildren의 속성도 true로 변경해줘야만 child가 나타난다.
			//hParent에 insert_folder()가 수행되면 children 유무에 따라 확장버튼을 갱신시켜줘야 한다.
		TVITEM tvItem;
		memset(&tvItem, 0, sizeof(TVITEM));

		tvItem.mask = TVIF_HANDLE | TVIF_CHILDREN;
		tvItem.hItem = hParent;
		tvItem.cChildren = true;
		SetItem(&tvItem);

		Expand(hParent, TVE_EXPAND);

		if (edit_mode)
			edit_item(hItem);
	}

	return hItem;
}

//주어진 항목의 label을 변경한다.
void CSCTreeCtrl::rename_item(HTREEITEM hItem, CString new_label)
{
	//shell treectrl일 경우 rename, delete은 위험하므로 여기서는 허용하지 않는다.
	if (m_is_shell_treectrl)
		return;

	if (hItem == NULL)
		hItem = GetSelectedItem();
	if (hItem == NULL)
		return;

	if (new_label.IsEmpty())
	{
		//shell_tree_ctrl이라고 해도 여기서는 편집모드까지만 제공할 뿐
		//편집 완료 후의 처리는 main dlg에서 처리하는 것이 맞다.
		edit_item(hItem);
	}
	else
	{
		SetItemText(hItem, new_label);
	}
}

//현재 선택된 노드의 하위 노드들 중에서 old_label을 찾아서 new_label로 이름을 변경한다.
void CSCTreeCtrl::rename_child_item(HTREEITEM hParent, CString old_label, CString new_label)
{
	if (hParent == NULL)
		hParent = GetSelectedItem();

	HTREEITEM hItem = find_children_item(old_label, hParent);
	if (!hItem)
		return;

	SetItemText(hItem, new_label);
}

//only_children이 true이면 해당 노드의 자식들만 제거한다.
void CSCTreeCtrl::delete_item(HTREEITEM hItem, bool only_children, bool confirm, bool delete_data)
{
	//hItem을 지정하지 않으면 선택된 아이템을 기준으로 동작한다.
	if (hItem == NULL)
		hItem = GetSelectedItem();

	//선택된 항목도 없다면 아무것도 하지 않는다.
	if (hItem == NULL)
		return;

	if (confirm)
	{
		CString msg;

		msg.Format(_T("%s\n\n이 항목을 완전히 삭제하시겠습니까?"), GetItemText(hItem));
		int res = AfxMessageBox(msg, MB_YESNO);
		if (res == IDNO)
			return;
	}

	//shell_tree_ctrl일 경우는 SHDeleteFolder()를 이용해서 폴더 및 하위폴더까지 모두 삭제시켜야 하지만
	//위험한 동작이므로 우선 노드만 삭제한다.

	//현재 노드 포함 모든 하위 노드까지 삭제
	if (!only_children)
	{
		if (delete_data)
		{
			DWORD_PTR pData = GetItemData(hItem);
			if (pData)
			{
				delete reinterpret_cast<void*>(pData);
				SetItemData(hItem, NULL);
			}
		}

		DeleteItem(hItem);
		return;
	}

	//only_children이면 모든 child들을 순회해서 삭제한다.
	HTREEITEM hChildItem = GetChildItem(hItem);

	SetRedraw(FALSE);

	while (hChildItem)
	{
		DWORD* pData = (DWORD*)GetItemData(hItem);
		if (pData)
		{
			delete pData;
			SetItemData(hItem, NULL);
		}

		DeleteItem(hChildItem);
		hChildItem = GetNextItem(hItem, TVGN_CHILD);
	}

	SetRedraw(TRUE);

	//children을 모두 삭제했으면 확장버튼도 갱신시켜준다.
	TVITEM tvItem;
	memset(&tvItem, 0, sizeof(TVITEM));

	tvItem.mask = TVIF_HANDLE | TVIF_CHILDREN;
	tvItem.hItem = hItem;
	tvItem.cChildren = 0;
	SetItem(&tvItem);
}

void CSCTreeCtrl::delete_all_items(bool confirm, bool delete_data)
{
	delete_item(GetRootItem(), false, confirm, delete_data);
}

//CTreeCtrl::DeleteAllItems() override.
//void CSCTreeCtrl::DeleteAllItems(bool confirm, bool delete_data)
//{
//	delete_all_items(confirm, delete_data);
//}


void CSCTreeCtrl::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	//편집중에 스크롤 할 경우는 편집 취소.
	edit_end(false);

	CTreeCtrl::OnVScroll(nSBCode, nPos, pScrollBar);
	sync_scrollbar();
	Invalidate(FALSE);
}

//H wheel: SB_THUMBPOSITION 한 번 호출로 큰 jump — line 누적은 매 line 마다 ScrollWindowEx 호출되어 깜빡임.
void CSCTreeCtrl::OnMouseHWheel(UINT nFlags, short zDelta, CPoint pt)
{
	edit_end(false);

	if (zDelta == 0 || !m_h_visible_state)
		return;

	//mouse driver 가 H wheel 한 번 굴림에 작은 zDelta 다수 메시지를 보내는 경우 — 매 메시지 처리 시 N * chunk 만큼 jump.
	//zDelta 누적해 WHEEL_DELTA 도달 시 1 notch 로 process, 나머지는 다음 메시지에 carry.
	m_h_wheel_accum += zDelta;
	if (abs(m_h_wheel_accum) < WHEEL_DELTA)
		return;

	int direction = (m_h_wheel_accum > 0) ? 1 : -1;
	int notches = abs(m_h_wheel_accum) / WHEEL_DELTA;
	m_h_wheel_accum -= direction * notches * WHEEL_DELTA;

	//native tree 는 H scroll 불가 → m_h_scroll_pos 를 직접 이동, customdraw paint-shift 로 시각화.
	CRect rc;
	GetClientRect(&rc);
	int right_limit_h = rc.Width() - (m_v_visible_state ? m_scrollbar.get_width() : 0);
	int max_pos = max(0, m_h_content_width - right_limit_h);
	int new_pos = m_h_scroll_pos + direction * notches * 60;
	if (new_pos < 0)
		new_pos = 0;
	if (new_pos > max_pos)
		new_pos = max_pos;
	m_h_scroll_pos = new_pos;

	sync_scrollbar();
	Invalidate(FALSE);
}


//V wheel: WS_VSCROLL 제거 상태에서 CTreeCtrl::OnMouseWheel 이 H scroll 로 fallback 하는 문제 회피 — base 우회.
//item 단위 jump — TVM_SELECTITEM(TVGN_FIRSTVISIBLE) 한 번 호출 (selection 변경 없음).
BOOL CSCTreeCtrl::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	edit_end(false);

	int notches = zDelta / WHEEL_DELTA;
	int delta_rows = -notches * 3;
	if (delta_rows == 0)
		return TRUE;

	int visible = (int)GetVisibleCount();
	if (visible <= 0)
		return TRUE;

	//visible 항목 캐시로 O(1) — 기존엔 매 휠 notch 마다 total/first_index/new_pos 를 GetNextVisibleItem O(n) walk
	//(각 SendMessage)했다. WinSxS(2.9만)면 휠 한 번에 수만 SendMessage.
	int total_visible_items = get_visible_total();
	if (total_visible_items <= 0)
		return TRUE;

	HTREEITEM first = GetFirstVisibleItem();
	int first_index = index_of_visible(first);
	if (first_index < 0)
		first_index = 0;

	//V wheel — tree natural max 까지만 (paint shift Y 의 hover 부작용 회피). 마지막 row 가 H overlay 아래 일부 가림 trade-off.
	int max_pos = max(0, total_visible_items - visible);
	int new_pos = first_index + delta_rows;
	if (new_pos < 0)
		new_pos = 0;
	if (new_pos > max_pos)
		new_pos = max_pos;

	if (new_pos == first_index)
		return TRUE;

	HTREEITEM cur = visible_at(new_pos);

	//TVM_SELECTITEM 부수효과로 tree 가 자기 H scroll 위치 reset 하는 케이스 차단 — V wheel 만 의도했는데 H 가 같이 변하는 현상 회피.
	SCROLLINFO si_h_before = { sizeof(si_h_before), SIF_POS };
	::GetScrollInfo(m_hWnd, SB_HORZ, &si_h_before);

	if (cur)
		SendMessage(TVM_SELECTITEM, TVGN_FIRSTVISIBLE, (LPARAM)cur);

	SCROLLINFO si_h_after = { sizeof(si_h_after), SIF_POS };
	::GetScrollInfo(m_hWnd, SB_HORZ, &si_h_after);
	if (si_h_after.nPos != si_h_before.nPos)
	{
		::SendMessage(m_hWnd, WM_HSCROLL, MAKEWPARAM(SB_THUMBPOSITION, (WORD)si_h_before.nPos), 0);
		::SendMessage(m_hWnd, WM_HSCROLL, MAKEWPARAM(SB_ENDSCROLL, 0), 0);
	}

	sync_scrollbar();
	Invalidate(FALSE);
	return TRUE;
}

void CSCTreeCtrl::set_color_theme(int theme, bool invalidate)
{
	m_theme.set_color_theme(theme);

	if (::IsWindow(m_scrollbar.m_hWnd))
		m_scrollbar.set_color_theme(m_theme, invalidate);
	if (::IsWindow(m_scrollbar_h.m_hWnd))
		m_scrollbar_h.set_color_theme(m_theme, invalidate);

	if (!m_hWnd)
		return;

	CTreeCtrl::SetBkColor(m_theme.cr_back.ToCOLORREF());

	//테두리(non-client)까지 갱신 — Invalidate() 만으로는 OnNcPaint 가 안 불려 테두리에 이전 테마 색 잔상.
	if (m_hWnd && invalidate)
		RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_FRAME);
}

void CSCTreeCtrl::set_color_theme(const CSCColorTheme& theme, bool invalidate)
{
	m_theme.copy_colors_from(theme);
	//m_has_parent_back_color = true;

	if (::IsWindow(m_scrollbar.m_hWnd))
		m_scrollbar.set_color_theme(m_theme, invalidate);
	if (::IsWindow(m_scrollbar_h.m_hWnd))
		m_scrollbar_h.set_color_theme(m_theme, invalidate);

	if (::IsWindow(m_hWnd))
		CTreeCtrl::SetBkColor(m_theme.cr_back.ToCOLORREF());

	//Invalidate() 는 client 만 다시 그리고 non-client(OnNcPaint 의 테두리)는 갱신하지 않아
	//테마를 바꾸면 테두리에 이전 cr_border_inactive 색이 잔상으로 남는다. RDW_FRAME 로 프레임까지 강제 갱신.
	if (invalidate && m_hWnd)
		RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_FRAME);
}

void CSCTreeCtrl::setup_scrollbar()
{
	if (m_scrollbar_setup || !::IsWindow(m_hWnd))
		return;

	//WS_VSCROLL + WS_HSCROLL 제거 — overlay 로 대체.
	//정공 NC reservation: WM_NCCALCSIZE 에서 우측·하단 m_scrollbar.get_width() 점유 → tree client 자체 좁아짐 → tree 의 자체 H/V scroll max 가 자연스럽게 늘어남.
	//overlay 는 tree 의 부모 dialog 의 child 로 만들어 NC 영역 (= tree 의 window 우측·하단) 에 위치. tree 의 child clipping 영향 안 받음.
	//set_message_target(this) — overlay 가 dialog 의 child 라도 Message_CSCScrollbar 는 우리 tree 로 직접 dispatch.
	//WS_VSCROLL/HSCROLL 제거 — overlay 로 대체.
	//WS_CLIPCHILDREN — overlay child 영역 부모가 paint 하지 않도록 → flicker / 잔상 회피.
	//WS_CLIPSIBLINGS — H overlay 가 dialog child(형제)로 tree NC 띠 위에 놓이므로, tree 가 형제 영역을 덮어 그리지 않도록.
	ModifyStyle(WS_VSCROLL | WS_HSCROLL, WS_CLIPCHILDREN | WS_CLIPSIBLINGS, SWP_FRAMECHANGED);
	::ShowScrollBar(m_hWnd, SB_BOTH, FALSE);

	CRect rc;
	GetClientRect(&rc);
	//V overlay: tree client 안 우측 끝. height 는 H overlay 자리만큼 짧게.
	m_scrollbar.create(this, CSCScrollbar::vertical,
		rc.right - m_scrollbar.get_width(), 0, m_scrollbar.get_width(), rc.Height() - m_scrollbar.get_width());
	m_scrollbar.set_color_theme(m_theme, false);
	m_scrollbar.set_line(3);
	m_scrollbar.ShowWindow(SW_HIDE);

	//H overlay 는 *부모 dialog 의 child* 로 만든다 — tree 의 하단 NC(가로바 자리)에 놓여야 하는데
	//tree child 면 client 밖이라 클리핑돼 안 보이기 때문. 위치는 sync_scrollbar 가 parent 좌표로 잡는다.
	//set_message_target(this) — parent 가 dialog 라도 Message_CSCScrollbar 를 우리 tree 로 직접 dispatch.
	CWnd* pParentH = GetParent();
	m_scrollbar_h.create(pParentH ? pParentH : (CWnd*)this, CSCScrollbar::horizontal,
		0, 0, 10, m_scrollbar.get_width());
	m_scrollbar_h.ModifyStyle(0, WS_CLIPSIBLINGS);
	m_scrollbar_h.set_message_target(this);
	m_scrollbar_h.set_color_theme(m_theme, false);
	m_scrollbar_h.set_line(20);
	m_scrollbar_h.ShowWindow(SW_HIDE);

	m_scrollbar_setup = true;

	//parent dialog 에 WS_CLIPCHILDREN — dialog 가 자체 OnPaint 시 tree window 영역 안 그리도록.
	//없으면 dialog paint cycle (system COLOR_3DFACE 등 fill) + tree paint cycle 둘 다 발생해 두 frame 사이 깜빡임.
	CWnd* pParent = GetParent();
	if (pParent && !(pParent->GetStyle() & WS_CLIPCHILDREN))
		pParent->ModifyStyle(0, WS_CLIPCHILDREN);

	//초기 sync — m_v_visible_state / m_h_visible_state setting + overlay 위치 정확화. paint 전에 정확한 값 보장.
	sync_scrollbar();
}

void CSCTreeCtrl::sync_scrollbar()
{
	if (!m_scrollbar_setup || !::IsWindow(m_scrollbar.m_hWnd))
		return;

	//tree 가 collapse/expand / SetScrollInfo 등으로 WS_VSCROLL/HSCROLL 자체 재추가하는 케이스 강제 strip.
	if (GetStyle() & (WS_VSCROLL | WS_HSCROLL))
		ModifyStyle(WS_VSCROLL | WS_HSCROLL, 0);
	::ShowScrollBar(m_hWnd, SB_BOTH, FALSE);

	CRect rc;
	GetClientRect(&rc);

	//---- 1단계: 모델 결정 (need_v / need_h) ----
	//V: visible item count 기준 (collapsed children 제외). 캐시로 O(1) — sync_scrollbar 는 스크롤·페인트마다
	//호출되므로 매번 O(n) walk 하면 세로 스크롤이 막힌다.
	int total_visible_items = get_visible_total();
	int visible = (int)GetVisibleCount();
	bool need_v = (total_visible_items > visible) && (visible > 0);

	//H: 콘텐츠 폭 직접 측정. native GetScrollInfo(SB_HORZ).nMax 는 setup/sync 의 WS_HSCROLL strip 으로
	//client 폭에 clamp 돼 신뢰 불가 (리사이즈 순간 stale nMax 만 잠깐 nPage 초과 → 간헐적 표시 버그였음).
	//listctrl(total_col_width)/listbox(m_max_horizontal_extent) 와 동일하게 *전체 펼침 항목* 기준으로 자연
	//콘텐츠 폭을 구한다(measure_content_width — 수직 스크롤 무관 고정값). 모든 H scroll 은 customdraw 의
	//over_shift paint-shift 로 처리하므로 native scroll 은 쓰지 않는다.
	int right_limit_h = rc.Width() - (need_v ? m_scrollbar.get_width() : 0);
	int content_w = get_content_width();	//캐시 — 변경(insert/delete/expand/collapse/font) 시에만 재측정.
	m_h_content_width = content_w;
	bool need_h = ::IsWindow(m_scrollbar_h.m_hWnd) && (right_limit_h > 0) && (content_w > right_limit_h);

	//customdraw 가 timing 무관하게 정확한 값 사용 — IsWindowVisible 의 paint cycle stale 회피.
	m_v_visible_state = need_v;
	bool nc_changed = (need_h != m_h_visible_state);
	m_h_visible_state = need_h;

	//need_h 가 바뀌면 하단 NC 예약량이 달라진다 → 프레임 재계산을 강제(OnNcCalcSize 가 새 m_h_visible_state 로 client 높이 갱신).
	//SetWindowPos 는 동기적으로 WM_NCCALCSIZE + WM_WINDOWPOSCHANGED → 재진입 sync 가 *갱신된* client 로 배치를 끝낸다.
	//여기선 return 하여 stale rc 로 중복 배치하지 않는다. m_syncing 가드로 재귀를 1회로 제한.
	if (nc_changed && !m_syncing)
	{
		m_syncing = true;
		SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
		m_syncing = false;
		return;
	}

	//---- 2단계: ShowWindow ----
	//visible 변화 시 tree client 도 invalidate — right_limit (m_v_visible_state 기반) 가 바뀐 새 폭으로 row fill 재계산.
	bool vis_changed = false;
	if (need_v && !m_scrollbar.IsWindowVisible())
	{
		m_scrollbar.ShowWindow(SW_SHOW);
		vis_changed = true;
	}
	else if (!need_v && m_scrollbar.IsWindowVisible())
	{
		m_scrollbar.ShowWindow(SW_HIDE);
		vis_changed = true;
	}

	if (::IsWindow(m_scrollbar_h.m_hWnd))
	{
		if (need_h && !m_scrollbar_h.IsWindowVisible())
		{
			m_scrollbar_h.ShowWindow(SW_SHOW);
			//dialog child 이므로 형제(tree) 위로 올려야 NC 띠에서 보인다.
			m_scrollbar_h.SetWindowPos(&CWnd::wndTop, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
			vis_changed = true;
		}
		else if (!need_h && m_scrollbar_h.IsWindowVisible())
		{
			m_scrollbar_h.ShowWindow(SW_HIDE);
			vis_changed = true;
		}
	}

	if (vis_changed)
		Invalidate(FALSE);

	//---- 3단계: 위치 sync ----
	//V overlay (tree child): client 우측 끝, 높이는 client 하단까지. H 가 보이면 NC 예약으로 client 높이가
	//이미 줄어 있으므로 그대로 rc.Height() 가 가로바 위에서 멈춘다(여기서 따로 빼지 않는다).
	{
		CRect rTarget(rc.right - m_scrollbar.get_width(), 0, rc.right, rc.Height());
		CRect rCur;
		m_scrollbar.GetWindowRect(&rCur);
		ScreenToClient(&rCur);
		if (rCur != rTarget)
			m_scrollbar.MoveWindow(rTarget);
	}

	//H overlay (parent dialog child): tree client 하단 바로 아래(예약된 NC 띠)에 parent 좌표로 배치.
	//세로바가 보이면 그 너비만큼 우측을 비워 우하단 코너 정사각형을 남긴다(listctrl 동일 — 코너는 OnNcPaint 가 cr_back 으로 채움).
	if (::IsWindow(m_scrollbar_h.m_hWnd))
	{
		CPoint strip_tl(0, rc.bottom);	//rc = NC 로 축소된 client → rc.bottom = 예약 띠 상단(=가로바 위치)
		ClientToScreen(&strip_tl);
		int h_width = rc.Width() - (need_v ? m_scrollbar.get_width() : 0);
		CRect rTarget(strip_tl.x, strip_tl.y, strip_tl.x + h_width, strip_tl.y + m_scrollbar.get_width());
		CWnd* pParent = GetParent();
		if (pParent)
			pParent->ScreenToClient(&rTarget);

		CRect rCur;
		m_scrollbar_h.GetWindowRect(&rCur);
		if (pParent)
			pParent->ScreenToClient(&rCur);
		if (rCur != rTarget)
			m_scrollbar_h.MoveWindow(rTarget);

		//H 바는 parent dialog 의 child 다. splitter/main drag-resize 중에는 메시지 펌프가 바빠
		//ShowWindow/MoveWindow 가 invalidate 만 마크하고 즉시 그려지지 않아 release 전까지 바가 갱신 안 되는
		//현상이 있다(listctrl 의 sync_scrollbar 와 동일 — 거기선 RDW_UPDATENOW 로 해결). 보일 때 즉시 paint 강제.
		if (need_h)
			m_scrollbar_h.RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
	}

	//---- 4단계: 모델 값 push ----
	if (need_v)
	{
		int first_index = index_of_visible(GetFirstVisibleItem());	//O(1) 캐시 룩업.
		if (first_index < 0)
			first_index = 0;
		m_scrollbar.set_range(0, total_visible_items - 1);
		m_scrollbar.set_page(visible);
		m_scrollbar.set_pos(first_index);
	}

	if (need_h)
	{
		int max_pos = max(0, content_w - right_limit_h);
		if (m_h_scroll_pos > max_pos)
			m_h_scroll_pos = max_pos;
		if (m_h_scroll_pos < 0)
			m_h_scroll_pos = 0;

		//m_h_natural_max >= 0 = over_shift 활성 신호(get_over_shift). native max 개념 없음 → 0.
		//scrollbar range=콘텐츠 폭, page=viewport → 최대 thumb pos = content_w - viewport = max_pos.
		m_h_natural_max = 0;
		m_scrollbar_h.set_range(0, content_w);
		m_scrollbar_h.set_page(right_limit_h);
		m_scrollbar_h.set_pos(m_h_scroll_pos);
	}
	else
	{
		m_h_natural_max = -1;
		m_h_scroll_pos = 0;
	}
}

LRESULT CSCTreeCtrl::on_message_CSCScrollbar(WPARAM wParam, LPARAM lParam)
{
	CSCScrollbarMsg* msg = (CSCScrollbarMsg*)wParam;
	if (msg == nullptr)
		return 0;

	if (msg->msg != CSCScrollbarMsg::msg_scrollbar_pos_changed)
		return 0;

	if (msg->pThis == &m_scrollbar)
	{
		//V scroll drag — tree natural max 까지만 (paint shift Y 부작용 회피).
		SCROLLINFO si_h_before = { sizeof(si_h_before), SIF_POS };
		::GetScrollInfo(m_hWnd, SB_HORZ, &si_h_before);

		HTREEITEM cur = visible_at(msg->pos);	//O(1) 캐시 룩업 — 기존엔 pos 까지 GetNextVisibleItem O(n) walk.

		SetRedraw(FALSE);
		if (cur)
			SendMessage(TVM_SELECTITEM, TVGN_FIRSTVISIBLE, (LPARAM)cur);

		SCROLLINFO si_h_after = { sizeof(si_h_after), SIF_POS };
		::GetScrollInfo(m_hWnd, SB_HORZ, &si_h_after);
		if (si_h_after.nPos != si_h_before.nPos)
		{
			::SendMessage(m_hWnd, WM_HSCROLL, MAKEWPARAM(SB_THUMBPOSITION, (WORD)si_h_before.nPos), 0);
			::SendMessage(m_hWnd, WM_HSCROLL, MAKEWPARAM(SB_ENDSCROLL, 0), 0);
		}
		SetRedraw(TRUE);
		Invalidate(FALSE);
	}
	else if (msg->pThis == &m_scrollbar_h)
	{
		//H scroll drag — native tree 는 H scroll 불가하므로 overlay thumb 위치(0..content_w-viewport)를
		//m_h_scroll_pos 에 저장하고 다시 그린다. customdraw 의 over_shift 가 그만큼 콘텐츠를 좌측 시프트.
		m_h_scroll_pos = msg->pos;
		Invalidate(FALSE);
	}
	return 0;
}

//가상 루트 항목 설정.
//가상 루트가 없는 상태에서 노드들이 추가된 경우, 가상 루트를 새로 추가할 경우
//가상 루트를 추가하고 기존 노드들을 모두 가상루트의 child로 이동시킨다.
//image_index, selected_image_index는 미리 정의한 imagelist에 추가된 아이콘의 index이므로
//반드시 set_imagelist()로 이미지들을 지정한 후에 호출해야 한다.
void CSCTreeCtrl::set_root_item(CString label, int image_index, int selected_image_index)
{
	//이미 가상 루트를 설정한 상태라면 label과 img만 변경시킨다.
	if (label.IsEmpty())
	{
		if (m_use_root && m_root_item)
			InsertItem(&m_root_tvItem);
		return;
	}
	else
	{

	}

	HTREEITEM hItem = GetRootItem();


	m_use_root = true;

	//해당 아이템을 루트로 추가하고
	m_root_tvItem.item.mask = TVIF_TEXT | TVIF_CHILDREN;

	if (image_index >= 0 && m_imagelist.m_hImageList && image_index < m_imagelist.GetImageCount())
	{
		m_root_tvItem.item.mask |= TVIF_IMAGE;
		m_root_tvItem.item.iImage = image_index;
	}

	if (selected_image_index >= 0 && m_imagelist.m_hImageList && selected_image_index < m_imagelist.GetImageCount())
	{
		m_root_tvItem.item.mask |= TVIF_SELECTEDIMAGE;
		m_root_tvItem.item.iSelectedImage = selected_image_index;
	}

	m_root_tvItem.hInsertAfter = TVI_FIRST;
	m_root_tvItem.hParent = TVI_ROOT;
	m_root_tvItem.item.pszText = (LPTSTR)(LPCTSTR)label;
	//기존에 노드들이 존재했다면 확장버튼을 표시한다.
	m_root_tvItem.item.cChildren = 1;// (GetRootItem() != NULL);
	m_root_item = InsertItem(&m_root_tvItem);

	//기존에 노드들이 이미 추가되어 있던 상태라면 그들을 모두 root의 child로 이동시킨다.
	//기존에 있던 노드들은 최상위에 추가된 m_root_item의 sibling으로 존재할 것이다.
	hItem = GetNextSiblingItem(m_root_item);

	while (hItem)
	{
		move_tree_item(this, hItem, m_root_item);
		hItem = GetNextSiblingItem(m_root_item);
	}

	TRACE(_T("m_root_item = %p\n"), m_root_item);
}


void CSCTreeCtrl::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	lpMMI->ptMinTrackSize.x = 50;
	lpMMI->ptMinTrackSize.y = 50;

	CTreeCtrl::OnGetMinMaxInfo(lpMMI);
}

//해당 아이템의 depth level을 리턴한다. hItem == NULL이면 -1을 리턴한다.
int CSCTreeCtrl::get_indent_level(HTREEITEM hItem)
{
	if (hItem == NULL)
		return -1;

	int indent = 0;

	while ((hItem = GetParentItem(hItem)) != NULL)
		indent++;

	return indent;
}

void CSCTreeCtrl::Serialize(CArchive& ar)
{
	serialize_item(ar, GetRootItem());
}
void CSCTreeCtrl::serialize_item(CArchive& ar, HTREEITEM hItem)
{
	if (ar.IsStoring())
	{
		// Count items on this level
		DWORD dwCnt = 0;
		for (HTREEITEM hNext = hItem; hNext; hNext = GetNextItem(hNext, TVGN_NEXT))
			++dwCnt;
		ar << dwCnt;

		// Store each item on this level
		for (HTREEITEM hNext = hItem; hNext; hNext = GetNextItem(hNext, TVGN_NEXT))
		{
			// Output data of this node
			//OutputDataOfNode(tree, hNext, ar);

			CString label = GetItemText(hNext);
			ar << label;
			int indent = get_indent_level(hNext);
			TRACE(_T("%s %s\n"), CString(' ', indent * 2), label);
			// Stream children of this node
			//if (GetChildItem(hNext) != NULL)
				serialize_item(ar, GetChildItem(hNext));
		}
	}
	else
	{
		// Get number of nodes on this level
		DWORD dwCnt;
		ar >> dwCnt;
		while (dwCnt--)
		{
			// Insert a new node

			HTREEITEM hNext = InsertItem(_T(""), hItem ? hItem : TVI_ROOT);

			// Read data of node into the new node hNext
			//ReadDataOfNode(tree, hNext, ar);
			CString label;
			ar >> label;
			TRACE(_T("%s\n"), label);
			SetItemText(hNext, label);
			
			// Now read all children
			serialize_item(ar, GetChildItem(hNext));
		}
	}
}

void CSCTreeCtrl::OnNcPaint()
{
	//NC = 0. native scrollbar 그릴 NC 공간 없고 우리 overlay 는 client 안. base 호출 skip — tree 의 자동 NC paint 가 깜빡임 유발.
	if (m_scrollbar_setup)
	{
		CWindowDC dc(this);
		CRect rc;
		GetWindowRect(&rc);
		rc.OffsetRect(-rc.TopLeft());

		//하단 NC 띠(가로바 자리 + 우하단 코너)를 border 1px 안쪽으로 풀폭 채움 — 가로바/세로바가 그 위에 그려지고
		//코너는 cr_back 으로 남는다(listctrl 5f90861 와 동일 처리). 코너만 gw×gw 로 부분 fill 하면, m_draw_border 시
		//OnNcCalcSize 의 1px 인셋 때문에 실제 client 바닥(H-1-gw)과 fill 시작(H-gw)이 1px 어긋나 코너 top/left 변에
		//native client edge 1px 라인이 노출된다(이미지 #49 의 검은 ⌐). 풀폭·border-반영 fill 이 그 라인을 덮는다.
		int bottom_band = m_h_visible_state ? m_scrollbar.get_width() : 0;
		if (bottom_band > 0)
		{
			int b = m_draw_border ? 1 : 0;
			CRect band(b, rc.bottom - b - bottom_band, rc.right - b, rc.bottom - b);
			dc.FillSolidRect(&band, m_theme.cr_back.ToCOLORREF());
		}

		if (m_draw_border)
			draw_rect(&dc, rc, m_theme.cr_border_inactive);
		return;
	}

	CTreeCtrl::OnNcPaint();        //스크롤바 + corner box 정상 그림 (border 는 스타일 없으니 skip).

	if (!m_draw_border)
		return;

	CWindowDC dc(this);
	CRect rc;
	GetWindowRect(&rc);
	rc.OffsetRect(-rc.TopLeft());
	draw_rect(&dc, rc, m_theme.cr_border_inactive);
}

bool CSCTreeCtrl::load_tree_from_json_file(CString json_file_path)
{
	std::string jsonText;
	if (!ReadFileToString(json_file_path, jsonText))
		return false;

	rapidjson::Document doc;
	rapidjson::ParseResult ok = doc.Parse(jsonText.c_str());

	if (!ok)
		return false;

	return load_tree_from_json_document(doc);
}

// JSON Document를 TreeCtrl에 복원
bool CSCTreeCtrl::load_tree_from_json_document(const rapidjson::Document& doc)
{
	if (!doc.IsArray())
		return false;

	DeleteAllItems();

	HTREEITEM hSelectedItem = NULL;

	for (rapidjson::SizeType i = 0; i < doc.Size(); ++i)
	{
		JsonValueToTreeItem(doc[i], NULL, TVI_LAST);
	}

	if (hSelectedItem != NULL)
		SelectItem(hSelectedItem);

	return true;
}

// 파일 전체 읽기
bool CSCTreeCtrl::ReadFileToString(const CString& filePath, std::string& outText)
{
#ifdef UNICODE
	CW2A pathA(filePath, CP_ACP);
	std::ifstream ifs(pathA, std::ios::binary);
#else
	std::ifstream ifs(filePath, std::ios::binary);
#endif

	if (!ifs.is_open())
		return false;

	std::ostringstream oss;
	oss << ifs.rdbuf();
	outText = oss.str();

	return true;
}

// JSON 객체 1개를 Tree 노드로 재귀 복원
HTREEITEM CSCTreeCtrl::JsonValueToTreeItem(const rapidjson::Value& nodeValue, HTREEITEM hParent, HTREEITEM hInsertAfter)
{
	HTREEITEM hItem = nullptr;

	if (!nodeValue.IsObject())
		return hItem;

	CString name;

	DWORD* node = 0;
	if (function_set_node_data_on_loading)
	{
		hItem = function_set_node_data_on_loading(this, &node, nodeValue, hParent, hInsertAfter);
	}

	SetItemData(hItem, (DWORD_PTR)node);

	// children 재귀 처리
	if (nodeValue.HasMember("nodes") && nodeValue["nodes"].IsArray())
	{
		const rapidjson::Value& children = nodeValue["nodes"];
		for (rapidjson::SizeType i = 0; i < children.Size(); ++i)
		{
			JsonValueToTreeItem(children[i], hItem, TVI_LAST);
		}
	}

	// 자식이 모두 들어간 뒤 확장
	Expand(hItem, TVE_EXPAND);

	return hItem;
}

//save to json file
bool CSCTreeCtrl::save_tree_to_json_file(CString json_file_path)
{
	rapidjson::Document doc;
	tree_to_json_document(doc);

	rapidjson::StringBuffer buffer;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
	writer.SetIndent(' ', 4);
	doc.Accept(writer);

#ifdef UNICODE
	CW2A pathA(json_file_path, CP_ACP);
	std::ofstream ofs(pathA, std::ios::binary);
#else
	std::ofstream ofs(json_file_path, std::ios::binary);
#endif

	if (!ofs.is_open())
		return false;

	ofs.write(buffer.GetString(), buffer.GetSize());
	return ofs.good();
}

// 트리 전체를 Document로 생성
void CSCTreeCtrl::tree_to_json_document(rapidjson::Document& doc)
{
	doc.SetArray();
	rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();

	HTREEITEM hRoot = GetRootItem();
	while (hRoot != NULL)
	{
		doc.PushBack(tree_item_to_json_value(hRoot, alloc), alloc);
		hRoot = GetNextSiblingItem(hRoot);
	}
}

rapidjson::Value CSCTreeCtrl::tree_item_to_json_value(HTREEITEM hItem, rapidjson::Document::AllocatorType& alloc)
{
	rapidjson::Value nodeValue(rapidjson::kObjectType);
	CString text = GetItemText(hItem);

	//nodeValue와 hItem을 넘겨주고 main에서 nodeValue에 type, name, description 등의 속성값들을 채워서 리턴한다.
	if (function_set_node_value_on_saving)
		function_set_node_value_on_saving(this, hItem, nodeValue, alloc);
	/*
	//프로젝트마다 data 구조가 다르므로 이를 메인의 콜백함수로 처리하도록 한다면
	//CSCTreeCtrl에 범용 json 저장 코드를 넣을 수 있다.
	CApiNode* data = (CApiNode*)tree.GetItemData(hItem);

	if (data)
	{
		nodeValue.AddMember("type", data->type, alloc);
		nodeValue.AddMember("name", rapidjson::Value(CString2string(text), alloc).Move(), alloc);
		//nodeValue.AddMember(rapidjson::Value("name", alloc).Move(), CString2string(text), alloc);	//UXStudio에서는 이렇게 저장했으나 여기서는 에러발생함. rapidjson::Value type이 달라서 그럴수도 있음.
		nodeValue.AddMember("description", rapidjson::Value(CString2string(data->desc), alloc).Move(), alloc);
		if (data->type == node_request)
		{
			nodeValue.AddMember("method", rapidjson::Value(CString2string(data->method), alloc).Move(), alloc);
			nodeValue.AddMember("url", rapidjson::Value(CString2string(data->url), alloc).Move(), alloc);
			nodeValue.AddMember("header", rapidjson::Value(CString2string(data->header), alloc).Move(), alloc);
			nodeValue.AddMember("body", rapidjson::Value(CString2string(data->body), alloc).Move(), alloc);
		}
	}
	else
	{
		TRACE(_T("[error] tree item data is null. name = %s\n"), text);
	}
	*/

	rapidjson::Value children(rapidjson::kArrayType);

	HTREEITEM hChild = GetChildItem(hItem);
	while (hChild != NULL)
	{
		children.PushBack(tree_item_to_json_value(hChild, alloc), alloc);
		hChild = GetNextSiblingItem(hChild);
	}

	if (!children.Empty())
		nodeValue.AddMember("nodes", children, alloc);

	return nodeValue;
}

// 마우스 위치로부터 drop 모드를 판별한다.
// 아이템 영역의 상단 25% → drop_before_item
// 아이템 영역의 하단 25% → drop_after_item
// 가운데 50% → drop_on_item (child로 이동)
CSCTreeCtrl::DropPosition CSCTreeCtrl::hit_test_drop_position(CPoint ptClient, HTREEITEM hItem)
{
	if (hItem == NULL)
		return drop_on_item;

	CRect rc;
	GetItemRect(hItem, &rc, FALSE);

	int itemHeight = rc.Height();
	int threshold = MAX(itemHeight / 4, 3);

	if (ptClient.y < rc.top + threshold)
		return drop_before_item;
	else if (ptClient.y > rc.bottom - threshold)
		return drop_after_item;
	else
		return drop_on_item;
}

// 드롭 위치를 나타내는 삽입 마크(수평선)를 그려준다.
void CSCTreeCtrl::draw_insert_mark(CDC* pDC)
{
	if (m_hInsertMarkItem == NULL || m_dropPosition == drop_on_item)
		return;

	CRect rc;
	GetItemRect(m_hInsertMarkItem, &rc, FALSE);


	int y = (m_dropPosition == drop_before_item) ? rc.top : rc.bottom;

	GetClientRect(&rc);

	// 삽입 마크 라인 그리기
	Gdiplus::Graphics g(pDC->GetSafeHdc());
	Gdiplus::Color cr_insert_line = m_theme.cr_selected_border;
	draw_line(g, rc.left + 4, y, rc.right - 4, y, cr_insert_line, 3.0f);
}

void CSCTreeCtrl::clear_insert_mark()
{
	if (m_hInsertMarkItem != NULL)
	{
		CRect rcInvalidate;
		GetItemRect(m_hInsertMarkItem, &rcInvalidate, FALSE);

		HTREEITEM hPrev = GetPrevVisibleItem(m_hInsertMarkItem);
		if (hPrev)
		{
			CRect rcPrev;
			GetItemRect(hPrev, &rcPrev, FALSE);
			rcInvalidate.UnionRect(&rcInvalidate, &rcPrev);
		}

		HTREEITEM hNext = GetNextVisibleItem(m_hInsertMarkItem);
		if (hNext)
		{
			CRect rcNext;
			GetItemRect(hNext, &rcNext, FALSE);
			rcInvalidate.UnionRect(&rcInvalidate, &rcNext);
		}

		CRect rcClient;
		GetClientRect(&rcClient);
		rcInvalidate.left = rcClient.left;
		rcInvalidate.right = rcClient.right;

		m_hInsertMarkItem = NULL;
		m_dropPosition = drop_on_item;

		InvalidateRect(&rcInvalidate, TRUE);
	}
}

// hSrcItem을 hRefItem의 앞(bAfter=false) 또는 뒤(bAfter=true)에 같은 레벨로 이동시킨다.
BOOL CSCTreeCtrl::move_tree_item_as_sibling(CTreeCtrl* pTree, HTREEITEM hSrcItem, HTREEITEM hRefItem, bool bAfter)
{
	if (hSrcItem == NULL || hRefItem == NULL || hSrcItem == hRefItem)
		return FALSE;

	// src가 ref의 조상이면 이동 불가 (자기 하위로 이동 방지)
	HTREEITEM hCheck = GetParentItem(hRefItem);
	while (hCheck)
	{
		if (hCheck == hSrcItem)
			return FALSE;
		hCheck = GetParentItem(hCheck);
	}

	HTREEITEM hDestParent = GetParentItem(hRefItem);

	// 이동할 아이템의 정보를 수집
	TVITEM TV;
	TCHAR str[256];
	ZeroMemory(str, sizeof(str));

	TV.hItem = hSrcItem;
	TV.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE | TVIF_CHILDREN;
	TV.stateMask = 0xFF;
	TV.pszText = str;
	TV.cchTextMax = _countof(str);
	pTree->GetItem(&TV);

	DWORD_PTR dwData = pTree->GetItemData(hSrcItem);
	BOOL bExpanded = (pTree->GetItemState(hSrcItem, TVIS_EXPANDED) & TVIS_EXPANDED);

	// 삽입 위치 결정
	HTREEITEM hInsertAfter;
	if (bAfter)
	{
		hInsertAfter = hRefItem;
	}
	else
	{
		// hRefItem 앞에 삽입하려면 hRefItem의 이전 sibling을 찾아야 한다.
		// 이전 sibling이 없으면 TVI_FIRST
		HTREEITEM hPrev = GetPrevSiblingItem(hRefItem);
		hInsertAfter = hPrev ? hPrev : TVI_FIRST;
	}

	// 새 위치에 아이템 삽입
	TVINSERTSTRUCT TI;
	TI.hParent = hDestParent ? hDestParent : TVI_ROOT;
	TI.hInsertAfter = hInsertAfter;
	TI.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN;
	TI.item.iImage = TV.iImage;
	TI.item.iSelectedImage = TV.iSelectedImage;
	TI.item.pszText = TV.pszText;
	TI.item.cChildren = TV.cChildren;

	HTREEITEM hNewItem = pTree->InsertItem(&TI);
	pTree->SetItemData(hNewItem, dwData);

	// 자식 노드들도 함께 복사
	HTREEITEM hChildItem = pTree->GetChildItem(hSrcItem);
	if (hChildItem)
	{
		move_child_tree_item(pTree, hChildItem, hNewItem);
	}

	// 확장 상태 복원
	if (bExpanded)
	{
		pTree->Expand(hNewItem, TVE_EXPAND);
	}

	// 원래 아이템 삭제
	pTree->DeleteItem(hSrcItem);

	// 새 아이템 선택
	pTree->SelectItem(hNewItem);

	return TRUE;
}

void CSCTreeCtrl::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp)
{
	if (!lpncsp)
	{
		CTreeCtrl::OnNcCalcSize(bCalcValidRects, lpncsp);
		return;
	}

	//NC = 0 (full client = window). overlay 는 client 안 우측·하단 끝에 위치하고 customdraw 의 paint shift 로 콘텐츠를 좌/상측으로 추가 이동 → 콘텐츠 끝이 overlay 좌측까지 노출.
	RECT rcOrig = lpncsp->rgrc[0];
	CTreeCtrl::OnNcCalcSize(bCalcValidRects, lpncsp);
	lpncsp->rgrc[0].right  = rcOrig.right;
	lpncsp->rgrc[0].bottom = rcOrig.bottom;

	if (m_draw_border)
		::InflateRect(&lpncsp->rgrc[0], -1, -1);

	//H 스크롤바가 보일 때만 하단 m_scrollbar.get_width() 를 NC 로 예약한다. 그러면 native 세로 스크롤이 client 바닥을
	//이만큼 위로 보고 마지막 항목을 그 위에 멈춘다 → 가로바(이 예약 띠에 놓이는 dialog-child overlay)에 안 가린다.
	//세로바는 우측 텍스트 클리핑으로 처리하므로 우측은 예약하지 않는다. (need_h 변화 시 sync_scrollbar 가 SWP_FRAMECHANGED 로 재계산.)
	if (m_h_visible_state && lpncsp->rgrc[0].bottom - lpncsp->rgrc[0].top > m_scrollbar.get_width())
		lpncsp->rgrc[0].bottom -= m_scrollbar.get_width();
}

void CSCTreeCtrl::OnStyleChanging(int nStyleType, LPSTYLESTRUCT lpStyleStruct)
{
	//CSCScrollbar overlay (V/H) 가 native scrollbar 역할 대체 — WS_VSCROLL / WS_HSCROLL 재추가 둘 다 차단.
	//tree-view 가 overflow 감지 시 ShowScrollBar / SetScrollInfo 경로로 다시 set 하려 함 → 여기서 강제 strip.
	if (nStyleType == GWL_STYLE && lpStyleStruct)
		lpStyleStruct->styleNew &= ~(WS_VSCROLL | WS_HSCROLL);

	CTreeCtrl::OnStyleChanging(nStyleType, lpStyleStruct);
}
