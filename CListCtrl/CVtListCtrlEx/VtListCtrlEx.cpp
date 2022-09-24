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
		//���� ���� ����, focus���� � ���� ���� �׷��� ���ڻ�, ������ ����ְ�...

		//LVIR_BOUNDS�� ���� ��� 0�� �÷��� �� ������ �簢�� ������ �����Ѵ�.
		//0�� �÷� ���ĺ��ʹ� �ش� ���� �簢�� �������� �����Ѵ�.
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

			//���� �׸��� �ؽ�Ʈ ������ ������ �÷� ��Ŵ�� �����°� �ƴ϶�
			//������ ���� ������ �� ������ ǥ���ϴ°� �������ϴ�.
			//������ ������ �׳� �÷� ��Ŵ�� ������.
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

		//percentage Ÿ���̸� �ٱ׷��� ���·� �׷��ְ�
		if (get_column_data_type(iSubItem) == column_data_type_percentage_bar)
		{
			pDC->FillSolidRect(itemRect, crBack);

			CRect r = itemRect;

			//�ٱ׷����� ���� ����, �� ���� ���ݰ��� �������. ��Ʈ�� ���̰��� ����ϴ� ���̷� �׷�����.
			int cy = r.CenterPoint().y;
			r.DeflateRect(4, 0);
			r.top = cy + m_lf.lfHeight/3.4;		//�� ���� Ű��� �ٱ׷����� ���̰� ��������.
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

			//�ٱ׷����� ���� ����, �� ���� ���ݰ��� �������. ��Ʈ�� ���̰��� ����ϴ� ���̷� �׷�����.
			int cy = r.CenterPoint().y;
			r.DeflateRect(4, 0);
			r.top = cy + m_lf.lfHeight / 3.4;		//�� ���� Ű��� �ٱ׷����� ���̰� ��������.
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
		//�ؽ�Ʈ �����̸� ���ڿ��� ������ش�.
		else
		{
			pDC->SetTextColor(crText);
			pDC->FillSolidRect( itemRect, crBack );
			//�ؽ�Ʈ�� �׷��� �� itemRect�� �׸��� �¿� ������ ��� ������ �������δ�.
			//�ణ �ٿ��� ����ؾ� ���� ����.
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

	//�÷��� ���� �����Ǹ� �÷��� ���õ� ���� ���� ������ ũ�⸦ �������ش�.
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

	gapImage.Create(1, height, ILC_COLORDDB, 1, 0); //2��° �Ķ���ͷ� ��������.....

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

	//�ణ�� ���� ���� �ʿ�.
	Rect.left += (Offset + 1);
	Rect.right = Rect.left + GetColumnWidth(subItem) - 1;
	Rect.top += 2;
	//Rect.bottom += 1;

	return Rect;
}

//debug��忡���� �ſ� ������. lambda�������� �𸣰ڴ�.
//0�� �÷��� �ƴ� �ٸ� �÷����� ������ �� �� ���� ������ 0�� �÷����� �ѹ� �� �˻��Ѵ�.
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
			//������ ������ ���ϸ����� �����Ϸ� ������
			//���� ���ϴ´�� ǥ�õ��� �ʰ� �ð��� �� ����ȴ�.
			//�ϴ� �������� �����Ѵ�.
			//������ ���� ��� ���ϸ����� ������ ��
			//�Ϲ� ���� �׸��� ���ܽ�Ű�� ���ϴ´�� ���۵�.
			//return (a.crText[iSub] > b.crText[iSub]);
			
			if (a.crText[iSub] > b.crText[iSub])
				return true;
			else if (a.crText[iSub] < b.crText[iSub])
				return false;
			//�ؽ�Ʈ ������ ������ �ؽ�Ʈ�� ���ϵ� �Ϲ� �׸��� ���ܽ�Ų��.
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
			else //�ؽ�Ʈ ������ ������ �ؽ�Ʈ�� ��
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
	//��Ʈ�� Ű�� �������� �������� �����Ѵ�.
	if (IsCtrlPressed())
		sort_by_text_color(pNMLV->iSubItem, -1);
	else
		sort(pNMLV->iSubItem, -1);

	*pResult = 0;
}


void CVtListCtrlEx::PreSubclassWindow()
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


	//�����Ʈ���� ������ ���� �ִ��� Ȯ�� �ʿ�.
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
	// TODO: ���⿡ Ư��ȭ�� �ڵ带 �߰� ��/�Ǵ� �⺻ Ŭ������ ȣ���մϴ�.
	if ( pMsg->message == WM_MOUSEWHEEL && m_in_editing )
	{
		return true;

		/*
		//�����߿� ��ũ���� �Ǹ�E�������� ���� �簢?E������ ���ؼ�
		//edit�� �� ��ǥ�� �̵�������� �Ѵ�. client������ ����Ϫ��Eȭ�鿡�� ��������E�ȵȴ�.
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
		case 220		:	return true;	//'\'Ű�� ������ ����Ʈ �� ó������ �̵��Ǵ� ���� ����.
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
		//Ű���忡 ���� �׸� ���� ó���� ���ο��� �ؾ� �����ϴ�.
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

	/* �� �ڵ��� �ǹ̴�?
	//����� ��ĥ�������ؼ� �ȱ��ڰŸ��� �ϰ��� �ѵ��ѵ� �����δ�. �ϴ� �н�.
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

		//��Ƽ ���õ� ���¿��ٸ� ���õ� �׸� ��� �ش� �÷����� ���� �������ش�.
		//���� ���ο����� OnLvnEndlabeledit()�� ȣ���ؼ�
		//ini�� �ٸ� ���Ͽ��� ����ؾ� �Ѵٸ� �ű⼭�� �� ó���� �� ������Ѵ�.
		//��, ���콺�� �ٸ� ���� Ŭ���ϸ鼭 ������ �����ϴ� ���
		//���⼭ get_selected_items()�� �̿��Ͽ� ���ϸ� �ε����� �޶����� ������ �ȴ�.
		//������ ���۵� �� �̸� ���õ� �׸���� ��ȣ�� ����� ���� �Ѵ�.
		//������ �Ϸ�Ǹ� ���õ� �׸���� ��� ����������
		//������ ���� ������������ ������� �ִ�.
		//���� CtrlŰ�� ������ ������ �����ϴ� ��쿡�� ���õ� �׸���� ��� �����ϵ��� �����Ѵ�.
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
	//�⺻ �����Ǵ� ���� ����� �̿�E���E�ʰ�EEditSubItem�̶�� �Լ��� �����ؼ� �翁E߱�E������
	//parent���Դ� LVN_BEGINLABELEDIT �޽����� ��E޵���E�ʴ´�.
	//�Ʒ� �޽����� �������� ��E��Ͽ?parent���� �ش�E����Ʈ ��Ʈ���� ������ ���۵Ǿ����� �˸���.
	//parent�� OnLvnBeginlabeleditListCtrl() �޽���E�ڵ鷯���� �ʿ��� ó���� ������.
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

//���� ���� �ؽ�Ʈ�� �ǵ�����.(���� ���̺��� ���ϸ��̰� ���ϸ� ������ ������ ��� �� �� �ִ�.)
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
	//item�� -1�̸� �ش� ���� ������ �ƴ϶� �⺻ �ؽ�Ʈ ������ ���ϴ� ���̴�.
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

//index ��ġ�� 0�� �÷��� text�� ������ �߰��Ѵ�.(-1�̸� �� �������� �߰�)
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

//����� �� Ȯ��â�� ȣ���ƾ���� ó���ؾ� ��
void CVtListCtrlEx::delete_selected_items()
{
	//�ڿ������� ���� �ָ�.
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

//���õ� �׸���� dqSelected�� ��´�. dqSelected�� null�̸� �׳� ���� ������ ���Ϲ޾� ����Ѵ�.
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

//0�� �÷������� �����͸� ã�´�.
int CVtListCtrlEx::find_string(CString str, int start, bool bWholeWord, bool bCaseSensitive)
{
	/*
	LVFINDINFO info;

	info.flags = (bWholeWord ? LVFI_STRING : LVFI_PARTIAL|LVFI_STRING);
	info.psz = str;

	// Delete all of the items that begin with the string.
	return FindItem(&info, indexFrom);
	*/
	//virtual list�� �ƴ϶�� �� �ڵ尡 �����ϰ�����
	//virtual list��� OnLvnOdfinditem()�Լ��� �ٽ� ȣ��ǰ�
	//�ű⼭ �ᱹ ã�� �ڵ尡 ���� �Ѵ�.
	//�⺻ FindItem�Լ��� �̿��Ѵٸ� �� �Լ� �ȿ� �˻� �ڵ带 �߰��ؾ߰�����
	//���⼭ ������ �����ؼ� �������.
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

//CListCtrl�� FindItem���ε� ��� ������ ��� �÷��� �˻��� �� ������
//�ϴ� ���� �����ؼ� ����.
//�� ��� result���� �÷���ȣ�� ������� �ʰ� ���ι�ȣ�� ����ǹǷ�
//�� ���ο��� �̹� ã�Ҵٸ� �� ������ �ٸ� �÷������� ã�� �ʰ� �׳� break�ؾ� �Ѵ�.
//�׷��� ������ ������ ���� ��ȣ�� result�� ��� ����ȴ�.
//result_reset�� true�̸� �˻� ����� ������ �� �˻��ϰ� false�̸� �߰��� �ִ´�.
//'|'�� �����ϸ� �� �ܾ���� �ϳ��� ���Ե� ����� �����ϰ�(OR)
//'&'�� �����ϸ� �� �ܾ ��� �� �����(AND)
//���� ��ȣ�� ������ wholeword�� �˻��Ѵ�.
//��ǰ���� ��� �����빮��-������ ������ �����Ƿ� '-'�� ���ٸ� ���� �տ� �ڵ� �־��ش�.
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
	std::deque<CString> dqTarget;	//separator�� ""�� �ƴ� ���� ��ū���� �и��Ͽ� ��� ã�´�.

	if (start_idx < 0)
		start_idx = 0;

	if (end_idx < 0 || end_idx >= size())
		end_idx = size() - 1;
	if (end_idx < 0)
		return 0;

	//ã�� �׸��� �������� ǥ���ϴ� ��� ���� ���� ���õ� �׸���� �ʱ�ȭ�����ش�.
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
		//sline ���ڿ����� dqTarget ���ڿ����� �����ϴ��� op��Ŀ� ���� �˻�.
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
		//�� ������ �����͸� �ϳ��� ��Ʈ�� ����Ʈ�� ���ͼ� ��
		std::deque<CString> dqLine = get_line_text_list(i, column_start, column_end);

		//functions�� find_string���� �����Ͽ����� �ν��� �ȵǼ� find_dqstring���� �̸� ����
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
	
	//dbclick�� �������� ����ϰų� � �׼����� ����ϴ� ����
	//�� Ŭ������ ����ϴ� ������ ���ϵ��� �Ѵ�.
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

		//shiftŰ��?E���յǸ�E?E���݁E�����Ѵ�.
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
				int result = AfxMessageBox(_T("�ٿ����� �������� �÷� ������ ��E�E����Ʈ�� �÷� ������ �ٸ��ϴ�.\n�״�E?�ٿ��ֱ�E�ұ�?E"), MB_YESNO);
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

//���Ͽ��� �ҷ��ͼ� ����Ʈ�� ä����. �÷��� ���� �����ؾ� �Ѵ�.
//�÷� ������ �ٸ� ������ ���ϵ��� �˾Ƽ� �ҷ�����E�ʴ´�.
//�̹� �÷��� ������ �Ƚ��Ǿ�E�ְ�E�� �������� ����� ���ϸ� �ҷ������� �Ǿ�E�ִ�.
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

//����Ʈ�� ����E?���Ϸ� �����Ѵ�.
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

//�� �Լ������� m_lf ������ �̿��ؼ� ��Ʈ�� ������Ѵ�.
//��, m_lf.lfHeight ���� �̿��ؼ� ��Ʈ�� ��������Ƿ�
//m_font_size ��� ������ ���� ����Ǿ��ٸ�
//������ �̿��� �̸� m_lf.lfHeight ������ ������ �� �� �Լ��� ȣ��Ǿ�� �Ѵ�.
//m_lf.lfHeight���� ���밪�� MM_TEXT����� ���� DC������ ���� �ȼ�ũ�Ⱑ �ȴ�.
//���� ��ũ�� ũ�� ���� ����Ҷ��� m_font_size�� �̿��ϴ°� �ƴ϶�
//m_lf.lfHeight���� �̿��ؾ� ��Ȯ�� ��ũ�� ũ�Ⱑ ���ȴ�.
//m_font_size�� ���� ����ڿ��� �Ϲ����� ��Ʈ ũ�� ���� ��ġ�� ���̴� �������� ��ġ�̴�.
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

//� �׸��� Ŭ���� �� ����Ű F2�� ������ �ش� �ؽ�Ʈ�� �����ϴ� �뵵�̹Ƿ�
//�� �� Ŭ���� subItem�� ����س��´�.
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
		//name������ Ŭ���� �� 1~2�� ���̿� �ٽ� Ŭ���ϸ� name �������� ��ȯ�ȴ�.
		/*
		if ((m_edit_item == pNMItemActivate->iItem) &&
			(m_edit_subItem == pNMItemActivate->iSubItem) &&
			(abs(t1 - m_last_clicked) >= 800) &&
			(abs(t1 - m_last_clicked) < 1600))
		*/
		//���õ� �׸��� �ٽ� ��Ŭ���ϸ� �������� ��ȯ�Ѵ�.
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

//� �׸��� Ư�� ��ġ�� ǥ�õǵ��� ��ũ�ѽ�Ų��.
//mode�� visible_first�̰� offset�� 3�̸� ������ 3�� �ε���, �� 4��° ��ġ�� �ش� �������� ǥ�õǵ��� ��ũ�ѽ�Ų��.
//mode�� visible_center�̰� offset�� 0�̸� �߾� ��ġ�� �ش� �������� ǥ�õǵ��� ��ũ�ѽ�Ų��.
//mode�� visible_last�̰� offset�� 3�̸� �Ʒ����� -3-1�� ��ġ�� �ش� �������� ǥ�õǵ��� ��ũ�ѽ�Ų��.
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
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
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
