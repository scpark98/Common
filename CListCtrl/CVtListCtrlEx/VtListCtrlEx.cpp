// VtListCtrlEx.cpp : implementation file
//

//#include "stdafx.h"
#include "VtListCtrlEx.h"

#include <algorithm>

#include "../../MemoryDC.h"
#include "EditCell.h"
#include <afxvslistbox.h>

#define IDC_EDIT_CELL	1001

// CVtListCtrlEx

IMPLEMENT_DYNAMIC(CVtListCtrlEx, CListCtrl)

CVtListCtrlEx::CVtListCtrlEx()
{
	set_color_theme(color_theme_default, false);

	m_use_sort = true;

	m_allow_edit					= false;
	m_modified						= false;
	m_in_editing					= false;
	m_old_text						= "";
	m_edit_item	= m_edit_subItem	= 0;

	m_last_clicked = 0;

	memset( &m_lf, 0, sizeof( LOGFONT ) );
}

CVtListCtrlEx::~CVtListCtrlEx()
{
	m_font.DeleteObject();
}


BEGIN_MESSAGE_MAP(CVtListCtrlEx, CListCtrl)
	ON_NOTIFY_REFLECT(LVN_GETDISPINFO, &CVtListCtrlEx::OnLvnGetdispinfo)
	ON_NOTIFY_REFLECT(LVN_ODFINDITEM, &CVtListCtrlEx::OnLvnOdfinditem)
	ON_NOTIFY_REFLECT(LVN_ODCACHEHINT, &CVtListCtrlEx::OnLvnOdcachehint)
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, &CVtListCtrlEx::OnLvnColumnclick)
	ON_NOTIFY_REFLECT(LVN_ODSTATECHANGED, &CVtListCtrlEx::OnLvnOdstatechanged)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_NOTIFY_REFLECT(LVN_BEGINLABELEDIT, &CVtListCtrlEx::OnLvnBeginlabeledit)
	ON_NOTIFY_REFLECT_EX(LVN_ENDLABELEDIT, &CVtListCtrlEx::OnLvnEndlabeledit)
	ON_NOTIFY_REFLECT_EX(NM_DBLCLK, &CVtListCtrlEx::OnNMDblclk)
	ON_NOTIFY_REFLECT_EX(NM_CLICK, &CVtListCtrlEx::OnNMClickList)
//	ON_WM_HSCROLL()
ON_WM_DROPFILES()
END_MESSAGE_MAP()



// CVtListCtrlEx message handlers




void CVtListCtrlEx::OnLvnGetdispinfo(NMHDR *pNMHDR, LRESULT *pResult)
{
	//NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	// TODO: Add your control notification handler code here
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;

	//Create a pointer to the item
	LV_ITEM* pItem = &(pDispInfo)->item;

	//Which item number?

	//Do the list need text information?
	if (pItem->mask & LVIF_TEXT)
	{
		CString text;

		//Which column?
		text = m_list_db[pItem->iItem].text[pItem->iSubItem];

		//Copy the text to the LV_ITEM structure
		//Maximum number of characters is in pItem->cchTextMax
		lstrcpyn(pItem->pszText, text, pItem->cchTextMax);
	}
	/*
	//Do the list need image information?
	if( pItem->mask & LVIF_IMAGE) 
	{
		//Set which image to use
		pItem->iImage=*m_list_db[itemid].m_image;

		//Show check box?
		if(IsCheckBoxesVisible())
		{
			//To enable check box, we have to enable state mask...
			pItem->mask |= LVIF_STATE;
			pItem->stateMask = LVIS_STATEIMAGEMASK;

			if(m_database[itemid].m_checked)
			{
				//Turn check box on..
				pItem->state = INDEXTOSTATEIMAGEMASK(2);
			}
			else
			{
				//Turn check box off
				pItem->state = INDEXTOSTATEIMAGEMASK(1);
			}
		}
	}
	*/
	*pResult = 0;
}


void CVtListCtrlEx::OnLvnOdfinditem(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVFINDITEM pFindInfo = reinterpret_cast<LPNMLVFINDITEM>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
}


void CVtListCtrlEx::OnLvnOdcachehint(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVCACHEHINT pCacheHint = reinterpret_cast<LPNMLVCACHEHINT>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
}


void CVtListCtrlEx::DrawItem(LPDRAWITEMSTRUCT lpDIS/*lpDrawItemStruct*/)
{
	int			iItem		= (int)lpDIS->itemID;
	int			iSubItem;
	CDC			*pDC		= CDC::FromHandle(lpDIS->hDC);
	CRect		itemRect;
	CRect		textRect;
	COLORREF	crText = m_crText;
	COLORREF	crBack = m_crBack;
	//COLORREF	crProgress = m_crProgress;

	for ( iSubItem = 0; iSubItem < get_column_count(); iSubItem++ )
	{
		//먼저 선택 여부, focus여부 등에 따라 셀이 그려질 글자색, 배경색을 골라주고...

		//LVIR_BOUNDS로 구할 경우 0번 컬럼은 한 라인의 사각형 영역을 리턴한다.
		//0번 컬럼 이후부터는 해당 셀의 사각형 영역만을 리턴한다.
		GetSubItemRect( iItem, iSubItem, LVIR_BOUNDS, itemRect );
		if (iSubItem == 0)
		{
			itemRect.right = itemRect.left + GetColumnWidth(0);
		}

		//if(lpDIS->itemState & ODS_SELECTED) //ok
		if ( GetItemState( iItem, LVIS_SELECTED) ) //ok
		{
			if (GetFocus() == this)
			{
				crText = m_crTextSelected;
				crBack = m_crBackSelected;
			}
			else
			{
				crText = m_crTextSelectedInactive;
				crBack = m_crBackSelectedInactive;
			}

			//선택 항목의 텍스트 색상은 무조건 컬러 스킴을 따르는게 아니라
			//지정된 색이 있으면 그 색으로 표시하는게 좋은듯하다.
			//글자의 배경색은 그냥 컬러 스킴을 따른다.
			if (m_list_db[iItem].crText[iSubItem] != listctrlex_unused_color)
				crText = m_list_db[iItem].crText[iSubItem];
		}
		else
		{
			crText = m_list_db[iItem].crText[iSubItem];
			if (crText == listctrlex_unused_color)
				crText = m_crText;

			crBack = m_list_db[iItem].crBack[iSubItem];
			if (crBack == listctrlex_unused_color)
				crBack = m_crBack;
		}

		//percentage 타입이면 바그래프 형태로 그려주고
		if (get_column_data_type(iSubItem) == column_data_type_percentage_bar)
		{
			pDC->FillSolidRect(itemRect, crBack);

			CRect r = itemRect;

			//바그래프는 셀의 높이, 즉 라인 간격과는 관계없다. 폰트의 높이값에 비례하는 높이로 그려주자.
			int cy = r.CenterPoint().y;
			r.DeflateRect(4, 0);
			r.top = cy + m_lf.lfHeight/3.4;		//이 값을 키우면 바그래프의 높이가 낮아진다.
			r.bottom = cy - m_lf.lfHeight/3.4;

			double d = _ttof(m_list_db[iItem].text[iSubItem]);
			d /= 100.0;
			Clamp(d, 0.0, 1.0);
			
			r.right = r.left + (double)(r.Width()) * d;
			pDC->FillSolidRect(r, m_crProgress);
		}
		else if (get_column_data_type(iSubItem) == column_data_type_percentage_grid)
		{
			pDC->FillSolidRect(itemRect, crBack);

			CRect r = itemRect;

			//바그래프는 셀의 높이, 즉 라인 간격과는 관계없다. 폰트의 높이값에 비례하는 높이로 그려주자.
			int cy = r.CenterPoint().y;
			r.DeflateRect(4, 0);
			r.top = cy + m_lf.lfHeight / 3.4;		//이 값을 키우면 바그래프의 높이가 낮아진다.
			r.bottom = cy - m_lf.lfHeight / 3.4;

			double d = _ttof(m_list_db[iItem].text[iSubItem]);
			d /= 100.0;
			Clamp(d, 0.0, 1.0);

			r.right = r.left + (double)(r.Width()) * d;
			std::deque<COLORREF> dqColor;
			dqColor.push_back(m_crText);
			dqColor.push_back(crBack);
			gradient_rect(pDC, r, dqColor, false);

			int i;
			int half = r.Height() / 2.2;
			for (i = r.left + half; i < itemRect.right - half; i += (half * 1.5))
			{
				pDC->FillSolidRect(i, r.top, half / 2, r.Height(), crBack);
			}
		}
		//텍스트 형태이면 문자열을 출력해준다.
		else
		{
			pDC->SetTextColor(crText);
			pDC->FillSolidRect( itemRect, crBack );
			//텍스트가 그려질 때 itemRect에 그리면 좌우 여백이 없어서 양쪽이 꽉차보인다.
			//약간 줄여서 출력해야 보기 쉽다.
			textRect = itemRect;
			textRect.DeflateRect(4, 0);

			UINT format = 0;

			if ( get_column_text_align(iSubItem) == LVCFMT_LEFT )
				format = DT_LEFT;
			else if ( get_column_text_align(iSubItem) == LVCFMT_CENTER )
				format = DT_CENTER;
			else if ( get_column_text_align(iSubItem) == LVCFMT_RIGHT )
				format = DT_RIGHT;

			format = format | DT_SINGLELINE | DT_VCENTER | DT_WORD_ELLIPSIS;
			pDC->DrawText(m_list_db[iItem].text[iSubItem], textRect, format);
		}
	}
}

// ex. "No,20;Item1,50;Item2,50"
bool CVtListCtrlEx::set_headings(const CString& strHeadings)
{
	int iStart = 0;
	int	column = 0;

	for( ;; )
	{
		const int iComma = strHeadings.Find( _T(','), iStart );

		if( iComma == -1 )
			break;

		const CString strHeading = strHeadings.Mid( iStart, iComma - iStart );

		iStart = iComma + 1;

		int iSemiColon = strHeadings.Find( _T(';'), iStart );

		if( iSemiColon == -1 )
			iSemiColon = strHeadings.GetLength();

		const int iWidth = _tstoi((TCHAR*)(LPCTSTR)strHeadings.Mid( iStart, iSemiColon - iStart ));

		iStart = iSemiColon + 1;

		if (InsertColumn(column++, strHeading, LVCFMT_LEFT, iWidth) == -1)
			break;
	}

	//컬럼의 수가 결정되면 컬럼과 관련된 정보 저장 공간의 크기를 세팅해준다.
	m_column_data_type.resize(column);
	m_column_data_type.assign(column, column_data_type_text);

	m_column_sort_type.resize(column);
	m_column_sort_type.assign(column, sort_none);

	m_column_text_align.resize(column);
	m_column_text_align.assign(column, LVCFMT_LEFT);

	m_HeaderCtrlEx.m_header_text_align.resize(column);
	m_HeaderCtrlEx.m_header_text_align.assign(column, HDF_LEFT);

	m_allow_edit_column.resize(column);

	return TRUE;
}

CString	CVtListCtrlEx::get_header_text(int column)
{
	return m_HeaderCtrlEx.get_header_text( column );
}

void CVtListCtrlEx::set_header_text( int column, CString sText )
{
	m_HeaderCtrlEx.set_header_text( column, sText );
}

int CVtListCtrlEx::get_column_count()
{
	return m_HeaderCtrlEx.GetItemCount();
}

int	CVtListCtrlEx::get_column_text_align(int column)
{
	return m_column_text_align[column];
}

//default = LVCFMT_LEFT
void CVtListCtrlEx::set_column_text_align(int column, int format, bool set_to_header)
{
	m_column_text_align[column] = format;
	if (set_to_header)
		m_HeaderCtrlEx.set_header_text_align(column, format);
}

int	CVtListCtrlEx::get_header_text_align(int column)
{
	return m_HeaderCtrlEx.get_header_text_align(column);
}

void CVtListCtrlEx::set_header_text_align(int column, int format)
{
	if (column < 0)
	{
		for (int i = 0; i < m_HeaderCtrlEx.GetItemCount(); i++)
			m_HeaderCtrlEx.set_header_text_align( i, format );
	}
	else
	{
		m_HeaderCtrlEx.set_header_text_align( column, format );
	}
}

int	CVtListCtrlEx::get_column_data_type( int column )
{
	return m_column_data_type[column];
}

void CVtListCtrlEx::set_column_data_type(int column, int nType, bool invalidate)
{
	if (column >= m_column_data_type.size())
		m_column_data_type.resize(column + 1);

	m_column_data_type[column] = nType;

	if (invalidate)
		Invalidate();
}

void CVtListCtrlEx::set_header_height(int height)
{
	m_HeaderCtrlEx.set_header_height(height);
}

void CVtListCtrlEx::set_line_height(int height)
{
	CImageList gapImage;

	gapImage.Create(1, height, ILC_COLORDDB, 1, 0); //2번째 파라미터로 높이조절.....

	SetImageList(&gapImage,LVSIL_SMALL);
}

void CVtListCtrlEx::set_column_width(int nCol, int cx)
{
	if (nCol >= get_column_count())
		return;

	SetColumnWidth(nCol, cx);
}

void CVtListCtrlEx::load_column_width(CWinApp* pApp, CString sSection)
{
	int		i, width;
	CString str;

	for (i = 0; i < m_HeaderCtrlEx.GetItemCount(); i++)
	{
		width = pApp->GetProfileInt(sSection + _T("\\column width"), i2S(i), 0);
		if (width > 0)
			SetColumnWidth(i, width);
	}
}

void CVtListCtrlEx::save_column_width(CWinApp* pApp, CString sSection)
{
	int		i;
	CString str;

	for (i = 0; i < m_HeaderCtrlEx.GetItemCount(); i++)
	{
		pApp->WriteProfileInt(sSection + _T("\\column width"), i2S(i), GetColumnWidth(i));
	}
}

CRect CVtListCtrlEx::get_item_rect(int item, int subItem)
{
	// Get the column offset
	int Offset = 0;
	for (int iColumn = 0; iColumn < subItem; iColumn++)
		Offset += GetColumnWidth(iColumn);

	CRect Rect;
	CRect rc;

	GetItemRect(item, &Rect, LVIR_BOUNDS);

	// Now scroll if we need to expose the column
	GetClientRect(&rc);

	if (Offset + Rect.left < 0 || Offset + Rect.left > rc.right)
	{
		CSize Size;
		if (Offset + Rect.left > 0)
			Size.cx = -(Offset - Rect.left);
		else
			Size.cx = Offset - Rect.left;
		Size.cy = 0;
		Scroll(Size);
		Rect.left -= Size.cx;
	}

	//약간의 오차 보정 필요.
	Rect.left += (Offset + 1);
	Rect.right = Rect.left + GetColumnWidth(subItem) - 1;
	Rect.top += 2;
	//Rect.bottom += 1;

	return Rect;
}

//debug모드에서는 매우 느리다. lambda때문인지 모르겠다.
//0번 컬럼이 아닌 다른 컬럼으로 정렬할 때 두 값이 같으면 0번 컬럼으로 한번 더 검사한다.
void CVtListCtrlEx::sort(int subItem, int ascending)
{
	if (!m_use_sort)
		return;

	m_last_sorted_column = subItem;

	if (ascending == -1)
	{
		if (m_column_sort_type[subItem] == -1)
			m_column_sort_type[subItem] = sort_ascending;
		else
			m_column_sort_type[subItem] = !m_column_sort_type[subItem];
	}
	else
	{
		m_column_sort_type[subItem] = ascending;
	}

	int iSub = subItem;
	bool sort_asc = m_column_sort_type[subItem];
	int data_type = get_column_data_type(subItem);
	
	std::sort(m_list_db.begin(), m_list_db.end(),
		[sort_asc, iSub, data_type](CListCtrlData a, CListCtrlData b)
		{
			if (sort_asc)
			{
				if (data_type == column_data_type_text)
				{
					if (iSub != 0)
					{
						if (a.text[iSub].MakeLower() > b.text[iSub].MakeLower())
							return false;
						else if (a.text[iSub].MakeLower() < b.text[iSub].MakeLower())
							return true;
						else
						{
							if (a.text[0].MakeLower() > b.text[0].MakeLower())
								return false;
							else
								return true;
						}
					}
					else
					{
						return (a.text[iSub].MakeLower() < b.text[iSub].MakeLower());
					}
					return (a.text[iSub].MakeLower() < b.text[iSub].MakeLower());
				}
				else
				{
					//a.text[iSub].Replace(_T(","), _T(""));
					a.text[iSub].Remove(',');
					b.text[iSub].Remove(',');
					return (_ttof(a.text[iSub]) < _ttof(b.text[iSub]));
				}
			}
			else
			{
				if (data_type == column_data_type_text)
				{
					if (iSub != 0)
					{
						if (a.text[iSub].MakeLower() > b.text[iSub].MakeLower())
							return true;
						else if (a.text[iSub].MakeLower() < b.text[iSub].MakeLower())
							return false;
						else
						{
							if (a.text[0].MakeLower() > b.text[0].MakeLower())
								return true;
							else
								return false;
						}
					}
					else
					{
						return (a.text[iSub].MakeLower() > b.text[iSub].MakeLower());
					}
					return (_ttof(a.text[iSub].MakeLower()) > _ttof(b.text[iSub].MakeLower()));
				}
				else
				{
					a.text[iSub].Remove(',');
					b.text[iSub].Remove(',');
					return (_ttof(a.text[iSub]) > _ttof(b.text[iSub]));
				}
			}
		}
	);

	Invalidate();
}

void CVtListCtrlEx::sort_by_text_color(int subItem, int ascending)
{
	if (!m_use_sort)
		return;

	if (ascending == -1)
	{
		if (m_column_sort_type[subItem] == -1)
			m_column_sort_type[subItem] = sort_ascending;
		else
			m_column_sort_type[subItem] = !m_column_sort_type[subItem];
	}
	else
	{
		m_column_sort_type[subItem] = ascending;
	}

	int iSub = subItem;
	bool sort_asc = m_column_sort_type[subItem];

	std::sort(m_list_db.begin(), m_list_db.end(),
		[sort_asc, iSub](CListCtrlData a, CListCtrlData b)
	{
		if (sort_asc)
		{
			//색상이 같으면 파일명으로 정렬하려 했으나
			//뭔가 원하는대로 표시되지 않고 시간은 꽤 낭비된다.
			//일단 색상만으로 정렬한다.
			//색상이 같을 경우 파일명으로 정렬할 때
			//일반 색상 항목은 제외시키니 원하는대로 동작됨.
			//return (a.crText[iSub] > b.crText[iSub]);
			
			if (a.crText[iSub] > b.crText[iSub])
				return true;
			else if (a.crText[iSub] < b.crText[iSub])
				return false;
			//텍스트 색상이 같으면 텍스트로 비교하되 일반 항목은 제외시킨다.
			else if (a.crText[iSub] != listctrlex_unused_color)
			{
				if (a.text[iSub] < b.text[iSub])
					return true;
				else// if (a.text[iSub] < b.text[iSub])
					return false;
			}
			
		}
		else
		{
			return (a.crText[iSub] < b.crText[iSub]);
			/*
			if (a.crText[iSub] < b.crText[iSub])
				return true;
			else if (a.crText[iSub] > b.crText[iSub])
				return false;
			else //텍스트 색상이 같으면 텍스트로 비교
			{
				if (a.text[iSub] < b.text[iSub])
					return true;
				else// if (a.text[iSub] > b.text[iSub])
					return false;
			}
			*/
		}
	}
	);

	Invalidate();
}

void CVtListCtrlEx::OnLvnColumnclick(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: Add your control notification handler code here
	//컨트롤 키가 눌려지면 색상값으로 정렬한다.
	if (IsCtrlPressed())
		sort_by_text_color(pNMLV->iSubItem, -1);
	else
		sort(pNMLV->iSubItem, -1);

	*pResult = 0;
}


void CVtListCtrlEx::PreSubclassWindow()
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


	//헤더컨트롤을 제어할 일이 있는지 확인 필요.
#if 1
	// if view style is other than LVS_REPORT 
	// returned pointer will be NULL
	CHeaderCtrl* pHeader = GetHeaderCtrl();

	// get original view style
	DWORD dwStyle = GetStyle() & LVS_TYPEMASK;

	// modify to force header creation
	// now pointer is not NULL
	// and handle is valid
	ModifyStyle(LVS_TYPEMASK, LVS_REPORT);
	//ModifyStyle(LVS_TYPEMASK, LVS_OWNERDRAWFIXED);
	//ModifyStyle(LVS_TYPEMASK, LVS_OWNERDATA);
	pHeader = GetHeaderCtrl();

	// reset a style to original
	// check a pointer ... still there
	ModifyStyle(LVS_TYPEMASK, dwStyle| LVS_OWNERDRAWFIXED| LVS_OWNERDATA);
	pHeader = GetHeaderCtrl();

	// voila!
	m_HeaderCtrlEx.SubclassWindow(pHeader->m_hWnd);

	ASSERT(pHeader->m_hWnd != NULL);
#endif
	CListCtrl::PreSubclassWindow();
}


BOOL CVtListCtrlEx::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	if ( pMsg->message == WM_MOUSEWHEEL && m_in_editing )
	{
		return true;

		/*
		//폴翡중에 스크롤이 되툈E폴翡중인 셀의 사각?E정보를 구해서
		//edit을 그 좌표로 이동시켜줘야 한다. client영역을 벗엉廐툈E화면에서 보여지툈E안된다.
		CRect	r;

		GetSubItemRect( m_nEditItem, m_nEditSubItem, LVIR_BOUNDS, r );
		m_pEdit->MoveWindow( r );
		Invalidate();
		*/
	}
	else if ( pMsg->message == WM_KEYDOWN )
	{
		TRACE(_T("listctrl key = %d\n"), pMsg->wParam);
		switch (pMsg->wParam)
		{
		case 220		:	return true;	//'\'키를 누르면 리스트 맨 처음으로 이동되는 현상 방지.
		case 'A'		:	if (GetKeyState(VK_CONTROL) & 0x8000)
							{
								select_item(-1);
							}
							break;
		case 'C'		:	if ( GetKeyState( VK_CONTROL ) & 0x8000 )
							{
								//CopyToClipboard(_T("|") );
								MessageBeep( MB_ICONINFORMATION );
								return true;
							}
							break;
		case 'V'		:	if (GetKeyState(VK_CONTROL) & 0x8000)
							{
								//PasteInsertFromClipboard();
								MessageBeep(MB_ICONINFORMATION);
								return true;
							}
							break;
		case VK_F2		:	if ( m_in_editing )
								return true;
							else
								edit_item(get_selected_index(), m_edit_subItem );
							break;
		/*
		//키보드에 의한 항목 삭제 처리는 메인에서 해야 안전하다.
		case VK_DELETE	:	if ( m_bInEditing )
								return false;
							else
								DeleteSelectedItems();
			break;
		*/
		}
	}
	return CListCtrl::PreTranslateMessage(pMsg);
}


void CVtListCtrlEx::OnLvnOdstatechanged(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVODSTATECHANGE pStateChanged = reinterpret_cast<LPNMLVODSTATECHANGE>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
}


BOOL CVtListCtrlEx::OnEraseBkgnd(CDC* pDC)
{
	// TODO: Add your message handler code here and/or call default
	return false;
	return CListCtrl::OnEraseBkgnd(pDC);
}


void CVtListCtrlEx::OnPaint()
{
	CPaintDC dc1(this); // device context for painting
					   // TODO: Add your message handler code here
					   // Do not call CListCtrl::OnPaint() for painting messages
	CRect rc;
	CRect rcHeader;
	CHeaderCtrl* pHeaderCtrl = GetHeaderCtrl();
	pHeaderCtrl->GetClientRect(&rcHeader);

	GetClientRect(rc);
	rc.top += rcHeader.Height();

	CMemoryDC dc(&dc1, &rc, true);

	/* 이 코드의 의미는?
	//헤더는 안칠해지게해서 안깜박거리게 하고자 한듯한데 깜박인다. 일단 패스.
	CRect headerRect;
	GetDlgItem(0)->GetWindowRect(&headerRect);
	ScreenToClient(&headerRect);
	dc.ExcludeClipRect(&headerRect);

	CRect clip;
	dc.GetClipBox(&clip);
	clip.top -= 5;
	*/

	dc.FillSolidRect(rc, m_crBack);

	//CListCtrl::OnPaint();
	DefWindowProc(WM_PAINT, (WPARAM)dc.m_hDC, (LPARAM)0);
}


void CVtListCtrlEx::OnLvnBeginlabeledit(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
}


BOOL CVtListCtrlEx::OnLvnEndlabeledit(NMHDR *pNMHDR, LRESULT *pResult)
{
	LV_DISPINFO *plvDispInfo = (LV_DISPINFO *)pNMHDR;
	LV_ITEM	*plvItem = &plvDispInfo->item;

	if (plvItem->pszText != NULL && plvItem->pszText != m_old_text )
	{
		m_modified = true;

		//멀티 선택된 상태였다면 선택된 항목 모두 해당 컬럼들의 값을 변경해준다.
		//만약 메인에서도 OnLvnEndlabeledit()을 호출해서
		//ini등 다른 파일에도 기록해야 한다면 거기서도 이 처리를 꼭 해줘야한다.
		//단, 마우스로 다른 곳을 클릭하면서 편집을 종료하는 경우
		//여기서 get_selected_items()를 이용하여 구하면 인덱스가 달라지는 문제가 된다.
		//편집이 시작될 때 미리 선택된 항목들의 번호를 기억해 놔야 한다.
		//편집이 완료되면 선택된 항목들을 모두 변경해줄지
		//편집된 셀만 변경해줄지는 장단점이 있다.
		//따라서 Ctrl키를 누르고 편집을 종료하는 경우에는 선택된 항목들을 모두 변경하도록 수정한다.
		if (IsShiftPressed())
		{
			for (int i = 0; i < m_dqSelected_list_for_edit.size(); i++)
				set_text(m_dqSelected_list_for_edit[i], plvItem->iSubItem, plvItem->pszText);
		}
		else
		{
			set_text(m_edit_item, m_edit_subItem, plvItem->pszText);
		}
	}

	m_in_editing = false;

	*pResult = FALSE;

	return FALSE;
}

void CVtListCtrlEx::allow_edit(bool allow_edit)
{
	m_allow_edit = allow_edit;
	m_allow_edit_column.assign(get_column_count(), m_allow_edit);
}

void CVtListCtrlEx::allow_edit_column(int column, bool allow_edit)
{
	if (m_allow_edit_column.size() <= column)
		m_allow_edit_column.resize(column + 1);
	m_allow_edit_column[column] = allow_edit;
}


CEdit* CVtListCtrlEx::edit_item(int item, int subItem)
{
	if (!m_allow_edit_column[subItem])
		return NULL;
	// The returned pointer should not be saved

	// Make sure that the item is visible
	if (!EnsureVisible(item, false)) 
	{
		//InsertItem( )
		if (!EnsureVisible(item, TRUE)) 
			return NULL;
	}

	m_last_clicked = 0;

	// Make sure that nCol is valid
	CHeaderCtrl* pHeader = (CHeaderCtrl*) GetDlgItem(0);
	int nColumnCount = pHeader->GetItemCount();
	if (subItem >= nColumnCount || GetColumnWidth(subItem) < 5)
		return NULL;

	CRect Rect = get_item_rect(item, subItem);
	CRect rc;

	GetClientRect(rc);

	// Get Column alignment
	LV_COLUMN	lvCol;

	lvCol.mask = LVCF_FMT;
	GetColumn (subItem, &lvCol);

	DWORD dwStyle;

	if ((lvCol.fmt & LVCFMT_JUSTIFYMASK) == LVCFMT_LEFT)
		dwStyle = ES_LEFT;
	else if ((lvCol.fmt & LVCFMT_JUSTIFYMASK) == LVCFMT_RIGHT)
		dwStyle = ES_RIGHT;
	else
		dwStyle = ES_CENTER;

	DWORD dwExStyle = ListView_GetExtendedListViewStyle( GetSafeHwnd() );
	if ( (subItem == 0) && (dwExStyle & LVS_EX_CHECKBOXES) )
		Rect.left += 18;

	if (Rect.right > rc.right)
		Rect.right = rc.right;

	m_old_text = GetItemText(item, subItem);

	dwStyle |= WS_BORDER | WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL;
	CEdit *pEdit = new gxEditCell(this, item, subItem, GetItemText(item, subItem));
	pEdit->Create(dwStyle, Rect, this, IDC_EDIT_CELL);
	m_pEdit = pEdit;

	CString ext = GetFileExtension(m_old_text);
	if ((ext.GetLength() == 3 || ext.GetLength() == 4) && IsAlphaNumeric(ext))
	{
		m_pEdit->SetSel(0, m_old_text.GetLength() - ext.GetLength() - 1);
	}

	m_in_editing = true;
	m_edit_item = item;
	m_edit_subItem = subItem;
	get_selected_items(&m_dqSelected_list_for_edit);


	// Send Notification to parent of ListView ctrl
	//기본 지원되는 폴翡 기능을 이퓖E舊갋않컖EEditSubItem이라는 함수를 제작해서 사퓖E薩갋때문에
	//parent에게는 LVN_BEGINLABELEDIT 메시지가 픸E瀕프갋않는다.
	//아래 메시지를 수동으로 픸E徘臼?parent에서 해큱E리스트 컨트롤의 폴翡이 시작되었음을 알린다.
	//parent의 OnLvnBeginlabeleditListCtrl() 메시햨E핸들러에서 필요한 처리가 가능함.
	LV_DISPINFO dispinfo;
	dispinfo.hdr.hwndFrom = m_hWnd;
	dispinfo.hdr.idFrom = GetDlgCtrlID();
	dispinfo.hdr.code = LVN_BEGINLABELEDIT;

	dispinfo.item.mask = LVIF_TEXT;
	dispinfo.item.iItem = item;
	dispinfo.item.iSubItem = subItem;
	//dispinfo.item.pszText = bEscape ? NULL : LPTSTR((LPCTSTR)Text);
	//dispinfo.item.cchTextMax = Text.GetLength();

	GetParent()->SendMessage(WM_NOTIFY, GetDlgCtrlID(), (LPARAM)&dispinfo);

	return pEdit;
}

//편집 전의 텍스트로 되돌린다.(편집 레이블이 파일명이고 파일명 변경이 실패한 경우 쓸 수 있다.)
void CVtListCtrlEx::undo_edit_label()
{
	set_text(m_edit_item, m_edit_subItem, m_old_text);
}

void CVtListCtrlEx::set_color_theme(int theme, bool apply_now)
{
	switch ( theme )
	{
	case color_theme_default :
		m_crText				= ::GetSysColor( COLOR_BTNTEXT );
		m_crTextSelected		= ::GetSysColor( COLOR_HIGHLIGHTTEXT );
		m_crTextSelectedInactive= ::GetSysColor( COLOR_INACTIVECAPTIONTEXT );
		m_crBack				= ::GetSysColor( COLOR_WINDOW );
		m_crBackSelected		= ::GetSysColor( COLOR_HIGHLIGHT );
		m_crBackSelectedInactive= ::GetSysColor( COLOR_HIGHLIGHT );
		m_crHeaderBack			= ::GetSysColor( COLOR_3DFACE );
		m_crHeaderText			= ::GetSysColor( COLOR_BTNTEXT );
		m_crProgress = m_crText;
		break;
	case color_theme_light_blue :
		m_crText				= ::GetSysColor( COLOR_BTNTEXT );
		m_crTextSelected		= RGB(  65, 102, 146 );
		m_crTextSelectedInactive= RGB(  65, 102, 146 );
		m_crBack				= RGB( 193, 219, 252 );
		m_crBackSelected		= get_color( m_crBack, -32 );
		m_crBackSelectedInactive= RGB( 193, 219, 252 );
		m_crHeaderBack			= get_color( m_crBack, -32 );
		m_crHeaderText			= get_color( m_crText, -32 );
		m_crProgress = m_crText;
		break;
	case color_theme_navy_blue :
		m_crText				= RGB( 204, 216, 225 );
		m_crTextSelected		= RGB( 234, 246, 255 );
		m_crTextSelectedInactive= RGB( 105, 142, 186 );
		m_crBack				= RGB(  74,  94, 127 );
		m_crBackSelected		= RGB(  15,  36,  41 );
		m_crBackSelectedInactive= RGB(  15,  36,  41 );
		m_crHeaderBack			= get_color( m_crBack, -32 );
		m_crHeaderText			= get_color( m_crText, -32 );
		m_crProgress = m_crText;
		break; 
	case color_theme_dark_blue :
		m_crText				= RGB(  16, 177, 224 );
		m_crTextSelected		= RGB( 224, 180,  59 );
		m_crTextSelectedInactive= RGB( 105, 142, 186 );
		m_crBack				= RGB(   2,  21,  36 );
		m_crBackSelected		= RGB(   3,  42,  59 );
		m_crBackSelectedInactive= RGB(  15,  36,  41 );
		m_crHeaderBack			= RGB(   0,  13,  22 );
		m_crHeaderText			= RGB(   0, 180, 228 );
		m_crProgress = m_crText;
		break; 
	case color_theme_dark_gray :
		m_crText				= RGB( 164, 164, 164);
		m_crTextSelected		= RGB( 241, 241, 241 );
		m_crTextSelectedInactive= get_color(m_crTextSelected, -36);
		m_crBack				= RGB(  64,  64,  64 );
		m_crBackSelected		= get_color(m_crBack, -32);
		m_crBackSelectedInactive= get_color(m_crBack, -32);
		m_crHeaderBack			= get_color(m_crBack, -16);
		m_crHeaderText			= get_color(m_crText, -16);
		m_crProgress = m_crText;
		break;
	}

	m_HeaderCtrlEx.set_color(m_crHeaderText, m_crHeaderBack);

	if (apply_now)
		Invalidate();
}

COLORREF CVtListCtrlEx::get_text_color(int item, int subItem)
{
	//item이 -1이면 해당 셀의 색상이 아니라 기본 텍스트 색상값을 원하는 것이다.
	if (item < 0)
		return m_crText;

	COLORREF cr = m_list_db[item].crText[subItem];
	if (cr == listctrlex_unused_color)
		return m_crText;
	return cr;
}

COLORREF CVtListCtrlEx::get_back_color(int item, int subItem)
{
	COLORREF cr = m_list_db[item].crBack[subItem];
	if (cr == listctrlex_unused_color)
		return m_crBack;
	return cr;
}

void CVtListCtrlEx::set_text_color(int item, int subItem, COLORREF crText, bool erase, bool invalidate)
{
	int i, j;

	if (item < 0)
	{
		if (subItem < 0)
		{
			for (i = 0; i < m_list_db.size(); i++)
			{
				for (j = 0; j < get_column_count(); j++)
				{
					if (erase)
						m_list_db[i].crText[j] = listctrlex_unused_color;
					else
						m_list_db[i].crText[j] = crText;
				}
			}
		}
		else
		{
			for (i = 0; i < m_list_db.size(); i++)
			{
				if (erase)
					m_list_db[i].crText[subItem] = listctrlex_unused_color;
				else
					m_list_db[i].crText[subItem] = crText;
			}
		}
	}
	else
	{
		if (subItem < 0)
		{
			for (j = 0; j < get_column_count(); j++)
			{
				if (erase)
					m_list_db[item].crText[j] = listctrlex_unused_color;
				else
					m_list_db[item].crText[j] = crText;
			}
		}
		else
		{
			if (erase)
				m_list_db[item].crText[subItem] = listctrlex_unused_color;
			else
				m_list_db[item].crText[subItem] = crText;
		}
	}

	if (invalidate)
		Invalidate();
}

void CVtListCtrlEx::set_back_color(int item, int subItem, COLORREF crBack, bool erase)
{
	int i, j;

	if (item < 0)
	{
		if (subItem < 0)
		{
			for (i = 0; i < m_list_db.size(); i++)
			{
				for (j = 0; j < get_column_count(); j++)
				{
					if (erase)
						m_list_db[i].crBack[j] = listctrlex_unused_color;
					else
						m_list_db[i].crBack[j] = crBack;
				}
			}
		}
		else
		{
			for (i = 0; i < m_list_db.size(); i++)
			{
				if (erase)
					m_list_db[i].crBack[subItem] = listctrlex_unused_color;
				else
					m_list_db[i].crBack[subItem] = crBack;
			}
		}
	}
	else
	{
		if (subItem < 0)
		{
			for (j = 0; j < get_column_count(); j++)
			{
				if (erase)
					m_list_db[item].crBack[j] = listctrlex_unused_color;
				else
					m_list_db[item].crBack[j] = crBack;
			}
		}
		else
		{
			if (erase)
				m_list_db[item].crBack[subItem] = listctrlex_unused_color;
			else
				m_list_db[item].crBack[subItem] = crBack;
		}
	}

	Invalidate();
}

void CVtListCtrlEx::set_item_color(int item, int subItem, COLORREF crText, COLORREF crBack)
{
	int i, j;

	if (item < 0)
	{
		if (subItem < 0)
		{
			for (i = 0; i < m_list_db.size(); i++)
			{
				for (j = 0; j < get_column_count(); j++)
				{
					m_list_db[i].crText[j] = crText;
					m_list_db[i].crBack[j] = crBack;
				}
			}
		}
		else
		{
			for (i = 0; i < m_list_db.size(); i++)
			{
				m_list_db[i].crText[subItem] = crText;
				m_list_db[i].crBack[subItem] = crBack;
			}
		}
	}
	else
	{
		if (subItem < 0)
		{
			for (j = 0; j < get_column_count(); j++)
			{
				m_list_db[item].crText[j] = crText;
				m_list_db[item].crBack[j] = crBack;
			}
		}
		else
		{
			m_list_db[item].crText[subItem] = crText;
			m_list_db[item].crBack[subItem] = crBack;
		}
	}

	Invalidate();}

void CVtListCtrlEx::set_default_item_color(COLORREF crText, COLORREF crBack)
{
	//m_list_db->at(item).crText[subItem] = crText;
	//m_list_db->at(item).crBack[subItem] = crBack;
	Invalidate();
}

void CVtListCtrlEx::set_progress_color(COLORREF crProgress)
{
	m_crProgress = crProgress;
	Invalidate();
}

int CVtListCtrlEx::add_item(CString text, bool ensureVisible, bool invalidate)
{
	return insert_item(-1, text, ensureVisible, invalidate);
}

//index 위치에 0번 컬럼이 text인 라인을 추가한다.(-1이면 맨 마지막에 추가)
int CVtListCtrlEx::insert_item(int index, CString text, bool ensureVisible, bool invalidate)
{
	if (index < 0)
		index = size();

	m_list_db.insert(m_list_db.begin() + index, CListCtrlData(text, m_HeaderCtrlEx.GetItemCount()));

	SetItemCountEx(m_list_db.size());

	if (invalidate && ensureVisible)
		EnsureVisible(index, false);

	if (invalidate)
		Invalidate();

	return index;
}


int CVtListCtrlEx::insert_item(int index, std::deque<CString> dqText, bool ensureVisible, bool invalidate)
{
	if (dqText.size() == 0)
		return -1;

	int count = MAX(get_column_count(), dqText.size());
	
	index = insert_item(index, dqText[0], ensureVisible, invalidate);

	for (int i = 1; i < count; i++)
		set_text(index, i, dqText[i]);

	return index;
}

int CVtListCtrlEx::insert_items(int index, LPCTSTR pszText, ...)
{
	index = insert_item(index, pszText);

	if ( get_column_count() == 1 )
		return index;

	int subItem = 1;

	if (_tcslen(pszText) > 0)
	{
		va_list list;
		va_start(list, pszText);

		pszText = va_arg(list, LPCTSTR);

		while (pszText)
		{
			//CListCtrl::SetItem(index, nSubItem++, LVIF_TEXT, pszText, 0, 0, 0, 0);
			set_text(index, subItem++, pszText);

			if (subItem >= get_column_count())
				break;

			pszText = va_arg(list, LPCTSTR);
		}

		va_end(list);
	}

	return index;
}


int CVtListCtrlEx::size()
{
	return m_list_db.size();
}

//지우기 전 확인창은 호출루틴에서 처리해야 함
void CVtListCtrlEx::delete_selected_items()
{
	//뒤에서부터 지움에 주목.
	for ( int i = GetItemCount() - 1; i >= 0; i-- )
	{
		if ( GetItemState( i, LVIS_SELECTED ) )
		{
			CListCtrl::DeleteItem( i );
			m_list_db.erase(m_list_db.begin() + i);
		}
	}

	SetItemCount(m_list_db.size());
}

void CVtListCtrlEx::delete_item(int index)
{
	CListCtrl::DeleteItem(index);
	m_list_db.erase(m_list_db.begin() + index);
	SetItemCount(m_list_db.size());
}

void CVtListCtrlEx::delete_all_items()
{
	CListCtrl::DeleteAllItems();
	m_list_db.clear();
	SetItemCount(0);
}

CString CVtListCtrlEx::get_text(int item, int subItem)
{
	return m_list_db[item].text[subItem];
}

void CVtListCtrlEx::set_text(int item, int subItem, CString text, bool invalidate)
{
	m_list_db[item].text[subItem] = text;
	if (invalidate)
		InvalidateRect(get_item_rect(item, subItem), false);
}


CString CVtListCtrlEx::get_line_text(int index, int from, int to, CString sep)
{
	int		i;
	CString sText = _T("");

	if (index < 0 || index >= size())
		return _T("");

	if (to == -1 || to >= get_column_count())
		to = get_column_count() - 1;

	std::deque<int> dqColumn;
	for (i = from; i <= to; i++)
		dqColumn.push_back(i);

	return get_line_text(index, &dqColumn, sep);
}

CString CVtListCtrlEx::get_line_text(int index, std::deque<int>* dqColumn, CString sep)
{
	int i;
	CString linetext = _T("");

	for (i = 0; i < dqColumn->size(); i++)
		linetext = linetext + get_text(index, dqColumn->at(i)) + sep;

	return linetext;
}

std::deque<CString> CVtListCtrlEx::get_line_text_list(int index, int from, int to)
{
	if (index < 0 || index >= size())
		return std::deque<CString>();

	if (to == -1 || to >= get_column_count())
		to = get_column_count() - 1;

	std::deque<int> dqColumn;
	for (int i = from; i <= to; i++)
		dqColumn.push_back(i);

	return get_line_text_list(index, &dqColumn);
}

std::deque<CString> CVtListCtrlEx::get_line_text_list(int index, std::deque<int>* dqColumn)
{
	std::deque<CString> dqResult;

	int		i;
	CString sText = _T("");

	if (index < 0 || index >= size())
		return dqResult;

	for (i = 0; i < dqColumn->size(); i++)
		dqResult.push_back(get_text(index, dqColumn->at(i)));

	return dqResult;
}

int CVtListCtrlEx::get_selected_index(int start)
{
	int index = -1;

	if (size() == 0)
		return -1;
	
	POSITION pos = GetFirstSelectedItemPosition();
	if (pos == NULL)
		return index;

	while (true)
	{
		index = GetNextSelectedItem(pos);
		if (index >= start)
			return index;
		else if (pos == NULL)
			return index;
	}

	return index;
}

int CVtListCtrlEx::get_last_selected_item()
{
	for ( int i = GetItemCount() - 1; i >= 0; i-- )
	{
		if (GetItemState(i, LVIS_SELECTED))
			return i;
	}

	return -1;
}

//선택된 항목들을 dqSelected에 담는다. dqSelected가 null이면 그냥 선택 갯수를 리턴받아 사용한다.
int CVtListCtrlEx::get_selected_items(std::deque<int> *dqSelected)
{
	if (dqSelected != NULL)
		dqSelected->clear();

	int index;
	int selected_count = 0;
	POSITION pos = GetFirstSelectedItemPosition();

	while (pos)
	{
		index = GetNextSelectedItem(pos);
		selected_count++;
		if (dqSelected != NULL)
			dqSelected->push_back(index);
	}

	return selected_count;
}


void CVtListCtrlEx::select_item(int nIndex, bool bSelect /*= true*/, bool after_unselect)
{
	if (after_unselect)
		unselect_selected_item();

	SetItemState(nIndex, bSelect ? LVIS_SELECTED : 0, LVIS_SELECTED);
	SetItemState(nIndex, bSelect ? LVIS_FOCUSED : 0, LVIS_FOCUSED);
}

void CVtListCtrlEx::unselect_selected_item()
{
	std::deque<int> dqSelected;
	get_selected_items(&dqSelected);
	for (int i = 0; i < dqSelected.size(); i++)
		select_item(dqSelected[i], false);
}

//0번 컬럼에서만 데이터를 찾는다.
int CVtListCtrlEx::find_string(CString str, int start, bool bWholeWord, bool bCaseSensitive)
{
	/*
	LVFINDINFO info;

	info.flags = (bWholeWord ? LVFI_STRING : LVFI_PARTIAL|LVFI_STRING);
	info.psz = str;

	// Delete all of the items that begin with the string.
	return FindItem(&info, indexFrom);
	*/
	//virtual list가 아니라면 위 코드가 동작하겠지만
	//virtual list라면 OnLvnOdfinditem()함수가 다시 호출되고
	//거기서 결국 찾는 코드가 들어가야 한다.
	//기본 FindItem함수를 이용한다면 위 함수 안에 검색 코드를 추가해야겠지만
	//여기서 간단히 구현해서 사용하자.
	int i;
	CString sText;

	if (start < 0 )
		start = 0;

	for ( i = start; i < GetItemCount(); i++ )
	{
		sText = get_text(i, 0);

		if (!bCaseSensitive)
		{
			sText.MakeLower();
			str.MakeLower();
		}

		if ((bWholeWord && (sText == str)) ||
			(!bWholeWord && (sText.Find(str) >= 0)))
		{
			return i;
		}
	}

	return -1;
}

//CListCtrl의 FindItem으로도 모든 라인의 모든 컬럼을 검사할 수 있지만
//일단 직접 구현해서 쓴다.
//단 결과 result에는 컬럼번호는 저장되지 않고 라인번호만 저장되므로
//한 라인에서 이미 찾았다면 그 라인의 다른 컬럼에서는 찾지 않고 그냥 break해야 한다.
//그렇지 않으면 동일한 라인 번호가 result에 모두 저장된다.
//result_reset이 true이면 검색 결과를 리셋한 후 검색하고 false이면 추가로 넣는다.
//'|'로 구분하면 각 단어들이 하나라도 포함된 목록을 리턴하고(OR)
//'&'로 구분하면 각 단어가 모두 들어간 목록을(AND)
//구분 기호가 없으면 wholeword로 검색한다.
//작품명의 경우 영문대문자-숫자인 패턴이 많으므로 '-'가 없다면 숫자 앞에 자동 넣어준다.
void CVtListCtrlEx::find_string(CString find_target, std::deque<int>* result,
								int start_idx, int end_idx, int column_start, int column_end,
								bool result_reset, bool bWholeWord, bool bCaseSensitive, bool select)
{
	if (column_start < 0)
		column_start = 0;
	if (column_end < 0)
		column_end = get_column_count() - 1;

	if (column_start > column_end)
		return;

	if (result_reset && result->size())
		result->clear();

	int i;
	std::deque<int> dqColumn;
	for (i = column_start; i <= column_end; i++)
		dqColumn.push_back(i);

	find_string(find_target, result, start_idx, end_idx, &dqColumn);
}

int CVtListCtrlEx::find_string(CString find_target, std::deque<int>* result, int start_idx, int end_idx, std::deque<int>* dqColumn, bool stop_first_found)
{
	int		i;
	TCHAR	op;
	CString sText;
	std::deque<CString> dqTarget;	//separator가 ""이 아닐 경우는 토큰으로 분리하여 모두 찾는다.

	if (start_idx < 0)
		start_idx = 0;

	if (end_idx < 0 || end_idx >= size())
		end_idx = size() - 1;
	if (end_idx < 0)
		return 0;

	//찾은 항목을 선택으로 표시하는 경우 먼저 기존 선택된 항목들을 초기화시켜준다.
	//if (select)
	//	select_item(-1, false);

	if (find_target.Find('&') > 0)
		op = '&';
	else
		op = '|';

	GetTokenString(find_target, dqTarget, op, false);
	Trim(&dqTarget);

	if (dqColumn == NULL || dqColumn->size() == 0)
	{
		for (i = 0; i < get_column_count(); i++)
			dqColumn->push_back(i);
	}

	int cur_idx = start_idx;
	while (true)
	{
		::SendMessage(GetParent()->GetSafeHwnd(), MESSAGE_VTLISTCTRLEX, (WPARAM)message_progress_pos, (LPARAM)cur_idx);

		std::deque<CString> dqLine = get_line_text_list(cur_idx, dqColumn);
		//sline 문자열에서 dqTarget 문자열들이 존재하는지 op방식에 따라 검색.
		if (find_dqstring(dqLine, dqTarget, op))
		{
			result->push_back(cur_idx);

			if (stop_first_found)
			{
				::SendMessage(GetParent()->GetSafeHwnd(), MESSAGE_VTLISTCTRLEX, (WPARAM)message_progress_pos, (LPARAM)-1);
				return 1;
			}
		}

		if (start_idx < end_idx)
		{
			cur_idx++;
			if (cur_idx > end_idx)
				return result->size();
		}
		else
		{
			cur_idx--;
			if (cur_idx < end_idx)
				return result->size();
		}

		/*
		//한 라인의 데이터를 하나의 스트링 리스트로 얻어와서 비교
		std::deque<CString> dqLine = get_line_text_list(i, column_start, column_end);

		//functions에 find_string으로 선언하였으나 인식이 안되서 find_dqstring으로 이름 변경
		if (find_dqstring(dqLine, dqSrc, bWholeWord, bCaseSensitive))
		{
			result->push_back(i);
			if (select)
				select_item(i);
		}
		*/
	}

	return result->size();
}

BOOL CVtListCtrlEx::OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
	
	//dbclick을 편집으로 사용하거나 어떤 액션으로 사용하는 것은
	//이 클래스를 사용하는 메인이 정하도록 한다.
	return FALSE;

	/*
	if ( m_allow_edit == false )
		return FALSE;

	int iItem = pNMItemActivate->iItem;
	int iSubItem = pNMItemActivate->iSubItem;

	if (m_allow_edit_column[iSubItem] == false)
		return FALSE;

	if ( iItem >= 0 && iItem < GetItemCount() &&
		iSubItem >= 0 && iSubItem < get_column_count() )
	{
		edit_item(iItem, iSubItem);
		return TRUE;
	}
	*/

	/*
	else
	{
		CString str;

		str.Format(_T("%d"), GetItemCount());
		iItem = AddItem( str, NULL );
		SelectItem( iItem );
		EnsureVisible( iItem, false );
		EditSubItem( iItem, 0 );
		return TRUE;
	}
	*/
	return FALSE;
}


bool CVtListCtrlEx::list_copy_to_clipboard(bool onlySelected /*= true*/, CString sSeparator /*= _T("|")*/, bool bHead /*= false*/)
{
	ASSERT( ::IsWindow( GetSafeHwnd() ) );

	int		i;
	CString sResult = _T("");

	if (onlySelected && get_selected_items() == 0)
		return false;

	int		nCount = 0;
	int		nColumn = 1;
	CWaitCursor wait;

	if ((GetStyle() & LVS_TYPEMASK ) == LVS_REPORT &&
		(GetExtendedStyle() & LVS_EX_FULLROWSELECT))
	{
		CHeaderCtrl* pHeader = GetHeaderCtrl();
		nColumn = pHeader ? pHeader->GetItemCount() : 1;

		//shift키깩?E조합되툈E?E超蝴갋복사한다.
		if ((bHead || (GetKeyState(VK_SHIFT) < 0)) && pHeader)
		{
			for (i = 0; i < nColumn; ++i)
			{
				sResult += get_header_text(i);
				if (i != nColumn - 1)
					sResult += sSeparator;
			}

			sResult += _T("\n");
		}
	}

	/*
	while (pos)
	{
		nItem = GetNextSelectedItem(pos);
		if (0 != nCount)
			sResult += _T("\n");

		for ( i = 0; i < nColumn; ++i )
		{
			sResult += GetItemText(nItem, i);
			if (i != nColumn - 1)
				sResult += lpszSeparator;
		}
		++nCount;
	}
	*/
	for (i = 0; i < size(); i++)
	{
		if (!onlySelected || (GetItemState(i, LVIS_SELECTED) == LVIS_SELECTED))
		{
			sResult = sResult + get_line_text(i, 0, -1, sSeparator) + _T("\n");
		}
	}

	copy_to_clipboard(m_hWnd, sResult);

	return TRUE;
}
#if 0
void CVtListCtrlEx::paste_from_clipboard()
{
	TCHAR	*buffer = NULL;
	HGLOBAL	hGlobal;

	int		i;
	int		pos;
	int		iItem = get_selected_index();
	bool	bDifferWarning = true;
	CString sText, str, sToken;

	if (OpenClipboard() == false)
		return;

	if (hGlobal = GetClipboardData(CF_TEXT))
	{
		buffer = (TCHAR*)GlobalLock(hGlobal);
		GlobalUnlock(hGlobal);

		//1 | KIA | K9 | 150
		//2 | 99 | 9 | 999
		sText = buffer;

		sText.Replace(_T("\r\n"), _T("\n"));
		sText.Replace(_T("\t"), _T(","));

		while (sText != "")
		{
			pos = sText.Find(_T("\n"));
			if (pos == -1)
				str = sText;
			else
				str = sText.Left( pos );

			int idx;
			int nSepCount = get_char_count(str, '|');
			CStringArray ar;

			GetToken(str, _T("|"), ar);

			if (bDifferWarning && (get_column_count() != (nSepCount+1)))
			{
				int result = AfxMessageBox(_T("붙여넣을 데이터의 컬럼 갯수가 큱E갋리스트의 컬럼 갯수와 다릅니다.\n그큱E?붙여넣콅E할깩?E"), MB_YESNO);
				if (result == IDNO)
				{
					CloseClipboard();
					return;
				}
				bDifferWarning = false;
			}

			for (i = 0; i < MIN(GetColumnCount(), ar.GetSize()); i++)
			{
				if (i == 0)
				{
					if (iItem < GetItemCount())
					{
						idx = iItem;
						set_text(idx, 0, ar.GetAt(i));
					}
					else
					{
						idx = add_item(ar.GetAt(i), NULL);
					}
				}	
				else
				{
					set_text(idx, i, ar.GetAt(i));
				}
			}

			if (pos == -1)
				break;

			iItem++;
			sText = sText.Mid(pos + 1);
		}
	}

	CloseClipboard();

	::SendMessage(GetParent()->GetSafeHwnd(), MESSAGE_LISTCTRLEX_ITEM_CHANGED, GetDlgCtrlID(), 0);
}

//파일에서 불러와서 리스트를 채웝芩. 컬럼의 수가 동일해야 한다.
//컬럼 구성이 다른 데이터 파일들을 알아서 불러오햨E않는다.
//이미 컬럼의 구성이 픽스되푳E있컖E그 구성으로 저장된 파일만 불러오도록 되푳E있다.
bool CVtListCtrlEx::load_from_file( CString sfile, CString separator /*= _T("|")*/, bool match_column_count /*= true*/, bool reset_before_load /*= true*/, bool add_index /*= false*/ )
{
	if ( GetColumnCount() == 0 )
		return false;

	if ( reset_before_load )
		DeleteAllItems();

	int		i, j;
	TCHAR	sLine[512];
	CString str;

	FILE	*fp = _tfopen( sfile, _T("rt") );

	if ( fp == NULL )
		return false;


	while (_fgetts( sLine, 512, fp ))
	{
		if ( match_column_count && (get_char_count(sLine, separator[0])) >= GetColumnCount() )
		{
			fclose(fp);
			return false;
		}
		AddLineStringItem( CString(sLine), separator );
	}

	fclose(fp);
	return true;
}

//리스트의 내퓖E?파일로 저장한다.
bool CVtListCtrlEx::save_to_file( CString sfile, CString separator /*= _T("|")*/, bool includeHeader /*= false*/ )
{
	int		i, j;
	CString str = _T("");

	if ( GetItemCount() == 0 )
		return false;

	FILE	*fp = _tfopen( sfile, _T("wt") );

	if ( fp == NULL )
		return false;

	if ( includeHeader )
	{
		for ( i = 0; i < GetColumnCount(); i++ )
		{
			if ( i < GetColumnCount() - 1 )
				str = str + GetColumnText(i) + separator;
			else
				str += GetColumnText(i);
		}

		_ftprintf( fp, _T("%s\n"), str );
	}

	for ( i = 0; i < GetItemCount(); i++ )
	{
		str = _T("");

		for ( j = 0; j < GetColumnCount(); j++ )
		{
			if ( j < GetColumnCount() - 1 )
				str = str + GetItemText(i, j) + separator;
			else
				str += GetItemText(i, j);
		}

		_ftprintf( fp, _T("%s\n"), str );
	}

	fclose( fp );
	return true;
}
#endif

//이 함수에서는 m_lf 정보를 이용해서 폰트를 재생성한다.
//즉, m_lf.lfHeight 값을 이용해서 폰트가 만들어지므로
//m_font_size 멤버 변수의 값이 변경되었다면
//공식을 이용해 이를 m_lf.lfHeight 값으로 변경한 후 이 함수가 호출되어야 한다.
//m_lf.lfHeight값의 절대값이 MM_TEXT모드의 현재 DC에서의 실제 픽셀크기가 된다.
//따라서 스크롤 크기 등을 계산할때는 m_font_size를 이용하는게 아니라
//m_lf.lfHeight값을 이용해야 정확한 스크롤 크기가 계산된다.
//m_font_size는 단지 사용자에게 일반적인 폰트 크기 설정 수치로 쓰이는 직관적인 수치이다.
void CVtListCtrlEx::reconstruct_font()
{
	m_font.DeleteObject();
	BOOL bCreated = m_font.CreateFontIndirect(&m_lf);
	SetFont( &m_font, true );

	m_font_size = get_font_size();
	
	set_line_height(4-m_lf.lfHeight);
	//if (m_auto_line_height)
		//recalculate_line_height();
	
	ASSERT(bCreated);
}

int CVtListCtrlEx::get_font_size()
{
	m_font_size = -MulDiv(m_lf.lfHeight, 72, GetDeviceCaps(::GetDC(GetParent()->GetSafeHwnd()), LOGPIXELSY));
	return m_font_size;
}

//-1 : reduce, +1 : enlarge
void CVtListCtrlEx::set_font_size(int font_size)
{
	if (font_size == 0)
		return;

	if (font_size == -1)
	{
		enlarge_font_size(false);
		return;
	}
	else if (font_size == 1)
	{
		enlarge_font_size(true);
		return;
	}

	m_font_size = font_size;
	AfxGetApp()->WriteProfileInt(_T("file list"), _T("font size"), m_font_size);
	//For the MM_TEXT mapping mode,
	//you can use the following formula to specify 
	//a height for a font with a specified point size:
	m_lf.lfHeight = -MulDiv(m_font_size, GetDeviceCaps(::GetDC(GetParent()->GetSafeHwnd()), LOGPIXELSY), 72);
	reconstruct_font();
}

void CVtListCtrlEx::enlarge_font_size(bool enlarge)
{
	m_font_size = get_font_size();
	enlarge ? m_font_size++ : m_font_size--;

	if (m_font_size < 4)
		m_font_size = 4;
	if (m_font_size > 40)
		m_font_size = 40;

	AfxGetApp()->WriteProfileInt(_T("file list"), _T("font size"), m_font_size);
	m_lf.lfHeight = -MulDiv(m_font_size, GetDeviceCaps(::GetDC(GetParent()->GetSafeHwnd()), LOGPIXELSY), 72);
	reconstruct_font();
}

void CVtListCtrlEx::set_font_name(LPCTSTR sFontname, BYTE byCharSet)
{
	if (sFontname == _T(""))
		return;
	m_lf.lfCharSet = byCharSet;
	_tcscpy( m_lf.lfFaceName, sFontname);
	reconstruct_font();
}

void CVtListCtrlEx::set_font_bold( bool bBold )
{
	m_lf.lfWeight = ( bBold ? FW_BOLD : FW_NORMAL );
	reconstruct_font();
}

void CVtListCtrlEx::set_font_italic(bool italic)
{
	m_lf.lfItalic = italic;
	reconstruct_font();
}

void CVtListCtrlEx::set_log_font(LOGFONT lf)
{
	memcpy(&m_lf, &lf, sizeof(LOGFONT));
	reconstruct_font();
	Invalidate();
}

//어떤 항목을 클릭한 후 단축키 F2를 누르면 해당 텍스트를 편집하는 용도이므로
//이 때 클릭된 subItem을 기억해놓는다.
BOOL CVtListCtrlEx::OnNMClickList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: Add your control notification handler code here
	if (m_in_editing)
	{
		edit_end();
		//return;
	}

	if (m_allow_edit &&
		(pNMItemActivate->iSubItem >= 0) &&
		(pNMItemActivate->iSubItem < get_column_count()) &&
		m_allow_edit_column[pNMItemActivate->iSubItem])
	{
		//name영역을 클릭한 후 1~2초 사이에 다시 클릭하면 name 편집모드로 전환된다.
		/*
		if ((m_edit_item == pNMItemActivate->iItem) &&
			(m_edit_subItem == pNMItemActivate->iSubItem) &&
			(abs(t1 - m_last_clicked) >= 800) &&
			(abs(t1 - m_last_clicked) < 1600))
		*/
		//선택된 항목을 다시 원클릭하면 편집모드로 전환한다.
		if ((m_edit_item == pNMItemActivate->iItem) &&
			(m_edit_subItem == pNMItemActivate->iSubItem) &&
			(get_selected_items() == 1) &&
			(clock() - m_last_clicked < 3000))
		{
			edit_item(m_edit_item, m_edit_subItem);
		}
		else
		{
			m_last_clicked = clock();
		}

		m_edit_item = pNMItemActivate->iItem;
		m_edit_subItem = pNMItemActivate->iSubItem;
	}

	return FALSE;
	*pResult = 0;
}

void CVtListCtrlEx::edit_end(bool valid)
{
	m_in_editing = false;
	m_last_clicked = 0;
	m_pEdit->ShowWindow(SW_HIDE);

	/*
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

		m_dqThumb[m_editing_index].title = sText;
		Invalidate();

		::SendMessage(GetParent()->GetSafeHwnd(), MESSAGE_THUMBCTRL,
			(WPARAM)&CThumbCtrlMsg(GetDlgCtrlID(), CThumbCtrlMsg::message_thumb_rename, m_editing_index), 0);
	}
	*/
}

//어떤 항목이 특정 위치에 표시되도록 스크롤시킨다.
//mode가 visible_first이고 offset이 3이면 위에서 3인 인덱스, 즉 4번째 위치에 해당 아이템이 표시되도록 스크롤시킨다.
//mode가 visible_center이고 offset이 0이면 중앙 위치에 해당 아이템이 표시되도록 스크롤시킨다.
//mode가 visible_last이고 offset이 3이면 아래에서 -3-1인 위치에 해당 아이템이 표시되도록 스크롤시킨다.
void CVtListCtrlEx::ensure_visible(int index, int mode, int offset)
{
	int items_per_page = GetCountPerPage();
	int top_index = GetTopIndex();

	CRect r;
	GetItemRect(0, r, LVIR_BOUNDS);

	if (mode == visible_first)
	{
		Scroll(CSize(0, (index - offset - top_index) * r.Height()));
	}
	else if (mode == visible_center)
	{
		Scroll(CSize(0, (index - offset - items_per_page/2 - top_index) * r.Height()));
	}
	else if (mode == visible_last)
	{
		Scroll(CSize(0, (index + offset - items_per_page - top_index + 1) * r.Height()));
	}
}

bool CVtListCtrlEx::is_item_visible(int index, bool bPartial)
{
	CRect rc;
	CRect itemRect;
	
	GetClientRect(rc);
	GetItemRect(index, itemRect, LVIR_BOUNDS);

	rc.top += m_HeaderCtrlEx.get_header_height();

	if (!bPartial && (itemRect.top >= rc.top) && (itemRect.bottom <= rc.bottom))
		return true;
	else if (bPartial &&
			(((itemRect.top > rc.top) && (itemRect.top < rc.bottom)) ||
			 ((itemRect.bottom > rc.top) && (itemRect.bottom < rc.bottom))))
		return true;

	return false;
}

void CVtListCtrlEx::random()
{
	std::random_device rd;
	std::default_random_engine re(rd());
	std::shuffle(m_list_db.begin(), m_list_db.end(), re);
	Invalidate();
}

void CVtListCtrlEx::OnDropFiles(HDROP hDropInfo)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	CPoint pt;
	DragQueryPoint(hDropInfo, &pt);

	DWORD dropped_point = index_from_point(pt.x, pt.y);
	int dropped_index = HIWORD(dropped_point);
	int dropped_column = LOWORD(dropped_point);

	TRACE(_T("dropped point = %d, %d (index = %d, column = %d)\n"), pt.x, pt.y, dropped_index, dropped_column);

	CListCtrl::OnDropFiles(hDropInfo);

	::PostMessage(GetParent()->GetSafeHwnd(), WM_DROPFILES, (WPARAM)hDropInfo, (LPARAM)0);
}

DWORD CVtListCtrlEx::index_from_point(int x, int y)
{
	int first = GetTopIndex();
	int last = first + GetCountPerPage();
	CRect rItem;

	for (; first < last; first++)
	{
		for (int j = 0; j < get_column_count(); j++)
		{
			GetSubItemRect(first, j, LVIR_BOUNDS, rItem);
			if (j == 0)
			{
				rItem.right = rItem.left + GetColumnWidth(0);
			}

			if (rItem.PtInRect(CPoint(x, y)))
			{
				return MAKELONG(j, first);
			}
		}
	}

	return MAKELONG(-1, -1);
}
