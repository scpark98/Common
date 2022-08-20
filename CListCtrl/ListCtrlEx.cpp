// ListCtrlEx.cpp : implementation file
//

#include "stdafx.h"
#include "ListCtrlEx.h"

#include <algorithm>
#include <vector>

#include "EditCell.h"
#include "../MemoryDC.h"
#include "../Functions.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#pragma warning(disable:4996)	//discard => "error C4996: 'strcpy': This function or variable may be unsafe. Consider using strcpy_s instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS."


#define IDC_EDIT_CELL	1001


/////////////////////////////////////////////////////////////////////////////
// CListCtrlEx

CListCtrlEx::CListCtrlEx()
{
	m_bAllowSort					= false;

	m_bAllowEdit					= false;
	m_bModified						= false;
	m_bInEditing					= false;
	m_bUsePopupMenu					= false;
	m_bPopupMenuDisplayed			= false;
	m_use_item_color		= false;
	m_bCheckAll						= false;
	m_sOldText						= "";

	m_nEditItem	= m_nEditSubItem	= 0;

	m_crText				= ::GetSysColor( COLOR_BTNTEXT );
	m_crTextSelected		= ::GetSysColor( COLOR_HIGHLIGHTTEXT );//RGB( 193, 219, 252 );
	m_crTextSelectedInactive= ::GetSysColor( COLOR_INACTIVECAPTIONTEXT );
	m_crBack				= ::GetSysColor( COLOR_WINDOW );//RGB( 255, 255, 255 );
	m_crBackSelected		= ::GetSysColor( COLOR_HIGHLIGHT );//RGB( 193, 219, 252 );
	m_crBackSelectedInactive= ::GetSysColor( COLOR_HIGHLIGHT );//RGB( 193, 219, 252 );

	m_nSorted = SORT_NONE;

	m_font_size = 10;
	memset( &m_lf, 0, sizeof( LOGFONT ) );
}

CListCtrlEx::~CListCtrlEx()
{
	//소툈E渼?호출되햨E않는다.
	//OnDestroy에서 필퓖E코드를 수행하자.
}

void CListCtrlEx::OnDestroy()
{
	// TODO: 여기에 메시햨E처리콅E코드를 추가합니다.
	m_cImageListLarge.Detach();
	m_cImageListSmall.Detach();

	if ( m_use_item_color )
	{
		for ( int i = 0; i < GetItemCount(); i++ )
		{
			CListCtrlExItemColorDeque *dq = (CListCtrlExItemColorDeque*)GetItemData(i);
			if (dq)
				delete dq;
		}
	}

	CListCtrl::OnDestroy();
}

BEGIN_MESSAGE_MAP(CListCtrlEx, CListCtrl)
	//{{AFX_MSG_MAP(CListCtrlEx)
	ON_COMMAND_RANGE(listctrlex_menu_add_prior, listctrlex_menu_checkall, OnPopupMenu)
	//}}AFX_MSG_MAP
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, &CListCtrlEx::OnLvnColumnclick)
	ON_NOTIFY_REFLECT_EX(NM_RCLICK, &CListCtrlEx::OnNMRClick)
	ON_NOTIFY_REFLECT_EX(LVN_ENDLABELEDIT, &CListCtrlEx::OnLvnEndlabeledit)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_WM_KILLFOCUS()
	ON_WM_NCHITTEST()
	ON_WM_PAINT()
	//ON_MESSAGE(WM_SETFONT, OnSetFont)
	ON_WM_MEASUREITEM_REFLECT( )
	ON_WM_DESTROY()
	ON_WM_ERASEBKGND()
	//ON_NOTIFY_REFLECT_EX(LVN_ITEMCHANGED, &CListCtrlEx::OnLvnItemchanged)	//너무 많은 다양한 이벤트가 섞여 발생하므로 공흟E낯?獨値灌?적합하햨E않다.
	//ON_NOTIFY_REFLECT(NM_CLICK, &CListCtrlEx::OnNMClick)
	ON_WM_NCCALCSIZE()
#if USE_CUSTOMDRAW
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, &CListCtrlEx::OnNMCustomdraw)
#else
	ON_WM_DRAWITEM()
#endif
	ON_NOTIFY_REFLECT_EX(NM_DBLCLK, &CListCtrlEx::OnNMDblclk)
	//ON_WM_MEASUREITEM()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CListCtrlEx message handlers
void CListCtrlEx::SetBitmapImageList( UINT nBitmapIDLarge, UINT nBitmapIDSmall, COLORREF cTransparent, int nInitial, int nGrow /* = 1  */ )
{
	// Create 256 color image lists
	HIMAGELIST hList = ImageList_Create( 32,32, ILC_COLOR8 |ILC_MASK , nInitial, nGrow );
	m_cImageListLarge.Attach( hList );

	hList = ImageList_Create( 16, 16, ILC_COLOR8 | ILC_MASK, nInitial, nGrow );
	m_cImageListSmall.Attach( hList );


	// Load the large icons
	CBitmap cBmp;
	cBmp.LoadBitmap(nBitmapIDLarge);
	m_cImageListLarge.Add(&cBmp, cTransparent);
	cBmp.DeleteObject();

	// Load the small icons
	cBmp.LoadBitmap(nBitmapIDSmall);
	m_cImageListSmall.Add(&cBmp, cTransparent);

	// Attach them
	SetImageList(&m_cImageListLarge, LVSIL_NORMAL);
	SetImageList(&m_cImageListSmall, LVSIL_SMALL);
}

// Parent가 Resize.h를 이퓖E?ResizeDialog인 경퓖E
// Parent에서 ON_MESSAGE 선언시 오톩E발생
// 따라서 컬럼클릭 이벤트를 오버라이드하려툈E
// Parent에서 LVN_COLUMNCLICK 이벤트핸들러를 추가한 후
// 아래 OnColumnClickFunction 함수를 호출해준다.
void CListCtrlEx::OnColumnClickFunction(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here
	if ( m_bAllowSort == false )
		return;

	static int	nSortColumn = -1;
	static bool bSortAscending = TRUE;
	int			iColumn = pNMListView -> iSubItem;

	// Reverse sort order if column was just previously clicked.
	if ( iColumn == nSortColumn )
		bSortAscending = !bSortAscending;
	else
		bSortAscending = TRUE;

	nSortColumn = iColumn;

	if (m_vtDataType[pNMListView->iSubItem] == COLUMN_DATA_TYPE_NUMERIC )
		SortNumericItems( pNMListView->iSubItem, bSortAscending );
	else
		SortTextItems( pNMListView->iSubItem, bSortAscending );
	/*	
	CWnd* pWnd = GetParent();

	if ( pWnd )
	pWnd->SendMessage( MSG_COLUMN_CLICKED, GetDlgCtrlID(), 0 );
	*/
	*pResult = 0;
}

/*
int CALLBACK CListCtrlEx::CompareFunction( LPARAM lParam1, LPARAM lParam2, LPARAM lParamData )
{
CListCtrlEx* pListCtrl = reinterpret_cast<CListCtrlEx*>( lParamData );
ASSERT( pListCtrl->IsKindOf( RUNTIME_CLASS( CListCtrl ) ) );
return 0;
}
*/


bool CListCtrlEx::IsSorted()
{
	return m_nSorted;
	/*
	int		i;
	int		n1, n2;
	CString str1, str2;
	bool	bSorted = true;

	for ( i = 0; i < GetItemCount() - 2; i++ )
	{
	str1 = GetItemText( i, nColumn );
	str2 = GetItemText( i + 1, nColumn );

	if ( m_arDataType.GetAt( nColumn ) == "number" )
	{
	n1 = atoi( str1 );
	n2 = atoi( str2 );

	if ( n1 > n2 )
	return false;
	}
	else
	{
	if ( str1 > str2 )
	return false;
	}
	}

	return true;
	*/
}


// SortTextItems	- Sort the list based on column text
// Returns		- Returns true for success
// nCol			- column that contains the text to be sorted
// bAscending		- indicate sort order
// low			- row to start scanning from - default row is 0
// high			- row to end scan. -1 indicates last row
bool CListCtrlEx::SortTextItems( int nCol, bool bAscending, int low /*= 0*/, int high /*= -1*/ )
{
	if ( nCol >= ((CHeaderCtrl*)GetDlgItem(0))->GetItemCount() )
		return FALSE;

	if ( high == -1 )
		high = GetItemCount() - 1;

	int lo = low;
	int hi = high;
	CString midItem;

	if ( hi <= lo ) return FALSE;

	midItem = GetItemText( (lo+hi)/2, nCol );

	// loop through the list until indices cross
	while( lo <= hi )
	{
		// rowText will hold all column text for one row
		CStringArray rowText;

		// find the first element that is greater than or equal to 
		// the partition element starting from the left Index.
		if( bAscending )
			while( ( lo < high ) && ( GetItemText(lo, nCol) < midItem ) )
				++lo;
		else
			while( ( lo < high ) && ( GetItemText(lo, nCol) > midItem ) )
				++lo;

		// find an element that is smaller than or equal to 
		// the partition element starting from the right Index.
		if( bAscending )
			while( ( hi > low ) && ( GetItemText(hi, nCol) > midItem ) )
				--hi;
		else
			while( ( hi > low ) && ( GetItemText(hi, nCol) < midItem ) )
				--hi;

		// if the indexes have not crossed, swap
		// and if the items are not equal
		if( lo <= hi )
		{
			// swap only if the items are not equal
			if( GetItemText(lo, nCol) != GetItemText(hi, nCol))
			{
				// swap the rows
				LV_ITEM lvitemlo, lvitemhi;
				int nColCount = 
					((CHeaderCtrl*)GetDlgItem(0))->GetItemCount();
				rowText.SetSize( nColCount );
				int i;
				for( i=0; i<nColCount; i++)
					rowText[i] = GetItemText(lo, i);
				lvitemlo.mask = LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
				lvitemlo.iItem = lo;
				lvitemlo.iSubItem = 0;
				lvitemlo.stateMask = LVIS_CUT | LVIS_DROPHILITED | 
					LVIS_FOCUSED |  LVIS_SELECTED | 
					LVIS_OVERLAYMASK | LVIS_STATEIMAGEMASK;

				lvitemhi = lvitemlo;
				lvitemhi.iItem = hi;

				GetItem( &lvitemlo );
				GetItem( &lvitemhi );

				for( i=0; i<nColCount; i++)
					SetItemText(lo, i, GetItemText(hi, i));

				lvitemhi.iItem = lo;
				SetItem( &lvitemhi );

				for( i=0; i<nColCount; i++)
					SetItemText(hi, i, rowText[i]);

				lvitemlo.iItem = hi;
				SetItem( &lvitemlo );
			}

			++lo;
			--hi;
		}
	}

	// If the right index has not reached the left side of array
	// must now sort the left partition.
	if( low < hi )
		SortTextItems( nCol, bAscending , low, hi);

	// If the left index has not reached the right side of array
	// must now sort the right partition.
	if( lo < high )
		SortTextItems( nCol, bAscending , lo, high );

	return TRUE;
}

bool CListCtrlEx::SortNumericItems( int nCol, bool bAscending,int low/*=0*/, int high/*=-1*/ )
{
	if( nCol >= ((CHeaderCtrl*)GetDlgItem(0))->GetItemCount() )
		return FALSE;

	if( high == -1 ) high = GetItemCount() - 1;
	int lo = low;
	int hi = high;

	double midItem;

	if( hi <= lo ) return FALSE;

	midItem = _ttof( GetItemText( (lo+hi)/2, nCol ) );

	// loop through the list until indices cross
	while( lo <= hi )
	{
		// rowText will hold all column text for one row
		CStringArray rowText;

		// find the first element that is greater than or equal to 
		// the partition element starting from the left Index.
		if( bAscending )
			while( ( lo < high ) && (_ttof(GetItemText(lo, nCol)) < midItem ) )
				++lo;
		else
			while( ( lo < high ) && (_ttof(GetItemText(lo, nCol)) > midItem ) )
				++lo;

		// find an element that is smaller than or equal to 
		// the partition element starting from the right Index.
		if( bAscending )
			while( ( hi > low ) && (_ttof(GetItemText(hi, nCol)) > midItem ) )
				--hi;
		else
			while( ( hi > low ) && (_ttof(GetItemText(hi, nCol)) < midItem ) )
				--hi;

		// if the indexes have not crossed, swap                
		// and if the items are not equal
		if( lo <= hi )
		{
			// swap only if the items are not equal
			if(_ttof(GetItemText(lo, nCol)) != _ttof(GetItemText(hi, nCol)) )
			{
				// swap the rows
				LV_ITEM lvitemlo, lvitemhi;

				int nColCount =
					((CHeaderCtrl*)GetDlgItem(0))->GetItemCount();
				rowText.SetSize( nColCount );

				int i;
				for( i=0; i < nColCount; i++)
					rowText[i] = GetItemText(lo, i);

				lvitemlo.mask = LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
				lvitemlo.iItem = lo;
				lvitemlo.iSubItem = 0;
				lvitemlo.stateMask = LVIS_CUT | LVIS_DROPHILITED |
					LVIS_FOCUSED |  LVIS_SELECTED |
					LVIS_OVERLAYMASK | LVIS_STATEIMAGEMASK;
				lvitemhi = lvitemlo;
				lvitemhi.iItem = hi;

				GetItem( &lvitemlo );
				GetItem( &lvitemhi );

				for( i=0; i< nColCount; i++)
					SetItemText(lo, i, GetItemText(hi, i) );

				lvitemhi.iItem = lo;
				SetItem( &lvitemhi );

				for( i=0; i< nColCount; i++)
					SetItemText(hi, i, rowText[i]);

				lvitemlo.iItem = hi;
				SetItem( &lvitemlo );
			}

			++lo;
			--hi;
		}
	}

	// If the right index has not reached the left side of array
	// must now sort the left partition.
	if( low < hi )
		SortNumericItems( nCol, bAscending , low, hi);

	// If the left index has not reached the right side of array
	// must now sort the right partition.
	if( lo < high )
		SortNumericItems( nCol, bAscending , lo, high );

	return TRUE;
}

void CListCtrlEx::Sort(int nColumn)
{
	//일반적으로 리스트 데이터는 순정렬 ?E쨌?추가되는 경퓖E?큱E觀隙譴퓐?
	//맨 처음 컬럼을 클릭하툈E역정렬로 만들엉佛컖E그 다음부터는 토글시킨다.
	if (m_nSorted == 0 || m_nSorted == SORT_DESCENDING)
		m_nSorted = SORT_ASCENDING;
	//그렇햨E않으툈E역정렬로 정렬한다.
	else
		m_nSorted = SORT_DESCENDING;

	Sort(nColumn, m_nSorted == SORT_ASCENDING);
}

void CListCtrlEx::Sort( int nColumn, bool bAscending )
{
	if ( m_bAllowSort == false )
		return;

	if (m_vtDataType.size() <= nColumn)
	{
		m_vtDataType.resize(nColumn + 1);
		m_vtDataType[nColumn] = COLUMN_DATA_TYPE_TEXT;
	}

	m_nSorted = ( bAscending ? SORT_ASCENDING : SORT_DESCENDING );

	//for (int i = 0; i < m_arDataType.GetCount(); i++)
	//	TRACE( _T("%d type = %d\n"), i, m_arDataType.GetAt(i) );

	if (m_vtDataType[nColumn] == COLUMN_DATA_TYPE_NUMERIC )
		SortNumericItems( nColumn, bAscending );
	else
		SortTextItems( nColumn, bAscending );
}

//////////////////////////////////////////////////////////
//**************************************************
// GetItemImageIndex
// return the image index of the selected row and col
int CListCtrlEx::GetItemImageIndex(int nRow, int nCol/*=0*/)
{
	LV_ITEM lv = {0};
	lv.mask = LVIF_IMAGE;
	lv.iItem = nRow;
	lv.iSubItem = nCol;

	GetItem( &lv );

	return lv.iImage;
}


////////////////////////////////////////////////////////////////////
// Sort by image index of column
//
// This function is a simple modification of the text sorting function
// written by Zafir Anjum.
//
// SortInImageOrder	- Sort the list based on image index of column
// Returns		- Returns true for success
// nCol			- column that contains the text to be sorted
// bAscending	- indicate sort order
// nLow			- row to start scanning from - default row is 0
// nHigh		- row to end scan. -1 indicates last row
bool CListCtrlEx::SortInImageOrder(int nCol, bool bAscending, int nLow/*=0*/, int nHigh/*=-1*/)
{
	bool bRet = FALSE;

	if( nCol >= ((CHeaderCtrl*)GetDlgItem(0))->GetItemCount() )
		return FALSE;

	if( nHigh == -1 ) nHigh = GetItemCount() - 1;
	int nLo = nLow;
	int nHi = nHigh;

	int nMidItem;

	if( nHi <= nLo )
		return FALSE;

	nMidItem = GetItemImageIndex( (nLo+nHi)/2, nCol );

	// loop through the list until indices cross
	while( nLo <= nHi )
	{
		// arrsRowText will hold all column text for one row
		CStringArray arrsRowText;

		// find the first element that is greater than or equal to 
		// the partition element starting from the left Index.
		if( bAscending )
			while( ( nLo < nHigh ) && (GetItemImageIndex(nLo, nCol) < nMidItem ) )
				++nLo;
		else
			while( ( nLo < nHigh ) && (GetItemImageIndex(nLo, nCol) > nMidItem ) )
				++nLo;

		// find an element that is smaller than or equal to 
		// the partition element starting from the right Index.
		if( bAscending )
			while( ( nHi > nLow ) && (GetItemImageIndex(nHi, nCol) > nMidItem ) )
				--nHi;
		else
			while( ( nHi > nLow ) && (GetItemImageIndex(nHi, nCol) < nMidItem ) )
				--nHi;

		// if the indexes have not crossed, swap                
		// and if the items are not equal
		if( nLo <= nHi )
		{
			// swap only if the items are not equal
			if(GetItemImageIndex(nLo, nCol) != GetItemImageIndex(nHi, nCol) )
			{
				// swap the rows
				LV_ITEM lvitemlo = {0};
				LV_ITEM lvitemhi = {0};

				// Get the column count
				int nColCount =
					((CHeaderCtrl*)GetDlgItem(0))->GetItemCount();
				arrsRowText.SetSize( nColCount );

				// Load string array with text items of Lo row
				int i;
				for( i=0; i < nColCount; i++)
					arrsRowText[i] = GetItemText(nLo, i);

				// Setup Lo item structure
				lvitemlo.mask = LVIF_IMAGE | LVIF_STATE;

				// lParam only avail on col zero
				if (nCol == 0)
					lvitemlo.mask |= LVIF_PARAM;

				lvitemlo.iItem = nLo;
				lvitemlo.iSubItem = nCol; // 10/29/97 0;
				lvitemlo.stateMask =
					LVIS_CUT            | LVIS_DROPHILITED      |
					LVIS_FOCUSED        | LVIS_SELECTED         |
					LVIS_OVERLAYMASK    | LVIS_STATEIMAGEMASK;

				// duplicate to hi item except for iItem index
				lvitemhi = lvitemlo;
				lvitemhi.iItem = nHi;
				lvitemhi.iSubItem = nCol; // 10/29/97


				// get the current lo and hi items
				GetItem( &lvitemlo );
				GetItem( &lvitemhi );

				// set item text of lo to item text of hi
				for( i=0; i< nColCount; i++)
					SetItemText(nLo, i, GetItemText(nHi, i) );

				// save hi item
				lvitemhi.iItem = nLo;
				bRet = SetItem( &lvitemhi );
				_ASSERTE(bRet);

				// set item text of hi to item text of lo
				for( i=0; i< nColCount; i++)
					SetItemText(nHi, i, arrsRowText[i]);

				// save lo item
				lvitemlo.iItem = nHi;
				lvitemhi.iSubItem = nCol; // 10/29/97
				bRet = SetItem( &lvitemlo );
				_ASSERTE(bRet);
			}

			++nLo;
			--nHi;
		}
	}

	// If the right index has not reached the left side of array
	// must now sort the left partition.
	if( nLow < nHi )
		SortInImageOrder( nCol, bAscending , nLow, nHi);

	// If the left index has not reached the right side of array
	// must now sort the right partition.
	if( nLo < nHigh )
		SortInImageOrder( nCol, bAscending , nLo, nHigh );

	return TRUE;
}

bool CListCtrlEx::sort_in_text_color(int nSubItem, bool bAscending, int nLow, int nHigh)
{
	bool bRet = false;

	if( nSubItem >= ((CHeaderCtrl*)GetDlgItem(0))->GetItemCount() )
		return FALSE;

	if( nHigh == -1 )
		nHigh = GetItemCount() - 1;

	int nLo = nLow;
	int nHi = nHigh;

	COLORREF crMidItem;

	if( nHi <= nLo )
		return false;

	crMidItem = get_text_color((nLo+nHi)/2, nSubItem);

	// loop through the list until indices cross
	while( nLo <= nHi )
	{
		// arrsRowText will hold all column text for one row
		CStringArray arrsRowText;

		// find the first element that is greater than or equal to 
		// the partition element starting from the left Index.
		if( bAscending )
			while( ( nLo < nHigh ) && (get_text_color(nLo, nSubItem) < crMidItem ) )
				++nLo;
		else
			while( ( nLo < nHigh ) && (get_text_color(nLo, nSubItem) > crMidItem ) )
				++nLo;

		// find an element that is smaller than or equal to 
		// the partition element starting from the right Index.
		if( bAscending )
			while( ( nHi > nLow ) && (get_text_color(nHi, nSubItem) > crMidItem ) )
				--nHi;
		else
			while( ( nHi > nLow ) && (get_text_color(nHi, nSubItem) < crMidItem ) )
				--nHi;

		// if the indexes have not crossed, swap                
		// and if the items are not equal
		if( nLo <= nHi )
		{
			// swap only if the items are not equal
			if(get_text_color(nLo, nSubItem) != get_text_color(nHi, nSubItem) )
			{
				// swap the rows
				LV_ITEM lvitemlo = {0};
				LV_ITEM lvitemhi = {0};

				// Get the column count
				int nColCount =
					((CHeaderCtrl*)GetDlgItem(0))->GetItemCount();
				arrsRowText.SetSize( nColCount );

				// Load string array with text items of Lo row
				int i;
				for( i=0; i < nColCount; i++)
					arrsRowText[i] = GetItemText(nLo, i);

				// Setup Lo item structure
				lvitemlo.mask = LVIF_IMAGE | LVIF_STATE;

				// lParam only avail on col zero
				if (nSubItem == 0)
					lvitemlo.mask |= LVIF_PARAM;

				lvitemlo.iItem = nLo;
				lvitemlo.iSubItem = nSubItem; // 10/29/97 0;
				lvitemlo.stateMask =
					LVIS_CUT            | LVIS_DROPHILITED      |
					LVIS_FOCUSED        | LVIS_SELECTED         |
					LVIS_OVERLAYMASK    | LVIS_STATEIMAGEMASK;

				// duplicate to hi item except for iItem index
				lvitemhi = lvitemlo;
				lvitemhi.iItem = nHi;
				lvitemhi.iSubItem = nSubItem; // 10/29/97


										  // get the current lo and hi items
				GetItem( &lvitemlo );
				GetItem( &lvitemhi );

				// set item text of lo to item text of hi
				for( i=0; i< nColCount; i++)
					SetItemText(nLo, i, GetItemText(nHi, i) );

				// save hi item
				lvitemhi.iItem = nLo;
				bRet = SetItem( &lvitemhi );
				_ASSERTE(bRet);

				// set item text of hi to item text of lo
				for( i=0; i< nColCount; i++)
					SetItemText(nHi, i, arrsRowText[i]);

				// save lo item
				lvitemlo.iItem = nHi;
				lvitemhi.iSubItem = nSubItem; // 10/29/97
				bRet = SetItem( &lvitemlo );
				_ASSERTE(bRet);
			}

			++nLo;
			--nHi;
		}
	}

	// If the right index has not reached the left side of array
	// must now sort the left partition.
	if (nLow < nHi)
		sort_in_text_color(nSubItem, bAscending, nLow, nHi);

	// If the left index has not reached the right side of array
	// must now sort the right partition.
	if (nLo < nHigh)
		sort_in_text_color(nSubItem, bAscending, nLo, nHigh);

	return true;
}

void CListCtrlEx::DeleteSelectedItems()	// 지퓖E갋픸E확인창은 호출루틴에서 처리해야 함
{
	for ( int i = GetItemCount() - 1; i >= 0; i-- )
	{
		if ( GetItemState( i, LVIS_SELECTED ) )
		{
			// 			CListCtrlExItemColor	*pData = (CListCtrlExItemColor*)GetItemData( i );
			// 			
			// 			if ( pData )
			// 				delete pData;

			DeleteItem( i );
		}
	}
}

//컬럼의 수보다 적은 개수의 항목을 추가할때는 반드시 끝 인자는 NULL을 넣엉丕야 한다.
int CListCtrlEx::AddItem( LPCTSTR pszText, ... )
{
	int iIndex = InsertItem( GetItemCount(), pszText );

	if ( GetColumnCount() == 1 )
		return iIndex;

	int nSubItem = 1;

	if (_tcslen(pszText) > 0)
	{
		va_list list;
		va_start(list, pszText);

		pszText = va_arg(list, LPCTSTR);

		while (pszText)
		{
			CListCtrl::SetItem(iIndex, nSubItem++, LVIF_TEXT, pszText, 0, 0, 0, 0);

			if (nSubItem >= GetColumnCount())
				break;

			pszText = va_arg(list, LPCTSTR);
		}

		va_end(list);
	}

	::SendMessage(GetParent()->GetSafeHwnd(), MESSAGE_LISTCTRLEX_ITEM_ADDED, (WPARAM)GetSafeHwnd(), iIndex);
	return iIndex;
}

//컬럼의 수보다 적은 개수의 항목을 추가할때는 반드시 끝 인자는 NULL을 넣엉丕야 한다.
int CListCtrlEx::AddItem(int nIndex, LPCTSTR pszText, ...)
{
	if (nIndex == -1)
		nIndex = GetItemCount();

	int iIndex = InsertItem(nIndex, pszText);

	if (GetColumnCount() == 1)
		return iIndex;

	int nSubItem = 1;

	if (_tcslen(pszText) > 0)
	{
		va_list list;
		va_start(list, pszText);

		pszText = va_arg(list, LPCTSTR);

		while (pszText)
		{
			CListCtrl::SetItem(iIndex, nSubItem++, LVIF_TEXT, pszText, 0, 0, 0, 0);

			if (nSubItem >= GetColumnCount() || pszText == NULL)
				break;

			pszText = va_arg(list, LPCTSTR);
		}
		va_end(list);
	}

	::SendMessage(GetParent()->GetSafeHwnd(), MESSAGE_LISTCTRLEX_ITEM_ADDED, (WPARAM)GetSafeHwnd(), iIndex);
	return iIndex;
}

//컬럼의 수와 같거나 그 이하인 항목들이 separator로 구분되푳E있다툈E자동 파싱하여 리스트에 넣엉彿다.
int CListCtrlEx::AddLineStringItem( CString sText, TCHAR separator)
{
	int i;
	int index;

	std::deque<CString> dqToken;
	GetTokenString(sText, dqToken, separator, GetColumnCount());

	for ( i = 0; i < MIN(GetColumnCount(), dqToken.size()); i++ )
	{
		if ( i == 0 )
			index = AddItem( dqToken[i], NULL );
		else
			SetItemText( index, i, dqToken[i] );
	}

	return index;
}

//컬럼의 이름 그큱E?한 라인을 추가한다.
int CListCtrlEx::AddItemWithColumnText()
{
	return AddItemWithColumnText( GetItemCount() );
}

int CListCtrlEx::AddItemWithColumnText( int iItem )
{
	int nIndex;

	for ( int i = 0; i < GetColumnCount(); i++ )
	{
		if ( i == 0 )
			nIndex = AddItem( iItem, GetColumnText( i ), NULL );
		else
			SetItemText( nIndex, i, GetColumnText( i ) );
	}

	return nIndex;
}

bool CListCtrlEx::DeleteItem( int nItem )
{
	if ( m_use_item_color )
	{
		CListCtrlExItemColorDeque *dq = (CListCtrlExItemColorDeque*)GetItemData(nItem);
		if (dq)
		{
			delete dq;
			SetItemData(nItem, NULL);
		}
	}

	return CListCtrl::DeleteItem( nItem );
}

bool CListCtrlEx::DeleteAllItems()
{
	if ( m_use_item_color )
	{
		for ( int i = 0; i < GetItemCount(); i++ )
		{
			CListCtrlExItemColorDeque *dq = (CListCtrlExItemColorDeque*)GetItemData(i);
			if (dq)
			{
				delete dq;
				SetItemData(i, NULL);
			}
		}
	}

	return CListCtrl::DeleteAllItems();
}

/*
int CListCtrlEx::InsertItemString( CString sString, CString sDelimeter )
{
sString.Tokenize 를 이퓖E漫?수정 필퓖E!

int		nIndex = -1;
int		nSubItem = 0;
CString sItem;

while ( Token.MoreTokens() )
{
sItem = Token.GetNextToken();

if ( nSubItem == 0 )
nIndex = InsertItem( GetItemCount(), sItem );
else
SetItemText( nIndex, nSubItem, sItem );

nSubItem++;
}

return nIndex;
}
*/

int	CListCtrlEx::GetColumnDataType( int nColumn )
{
	return m_vtDataType[nColumn];
}

void CListCtrlEx::SetColumnDataType( int nColumn, int nType )
{
	if (nColumn >= m_vtDataType.size())
		m_vtDataType.resize(nColumn + 1);

	m_vtDataType[nColumn] = nType;
}

bool CListCtrlEx::SetHeadings( UINT uiStringID )
{
	CString strHeadings;
	VERIFY( strHeadings.LoadString( uiStringID ) );
	return SetHeadings( strHeadings );
}


// the heading text is in the format column 1 text,column 1 width;column 2 text,column 3 width;etc.
bool CListCtrlEx::SetHeadings( const CString& strHeadings )
{
	int iStart = 0;
	int	nColumn = 0;

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

		const int iWidth = _tstoi( (TCHAR*)(LPCTSTR)strHeadings.Mid( iStart, iSemiColon - iStart ) );

		iStart = iSemiColon + 1;

		if( InsertColumn( nColumn++, strHeading, LVCFMT_LEFT, iWidth ) == -1 )
			return FALSE;

		//for test
		LVCOLUMN col;
		col.mask = LVCF_FMT;
		GetColumn( (nColumn-1), &col );
		col.fmt = col.fmt;
	}

	return TRUE;
}

int	CListCtrlEx::GetColumnTextAlign( int nColumn )
{
	return m_HeaderCtrlEx.GetColumnTextAlign( nColumn );
}

void CListCtrlEx::SetColumnTextAlign(int nColumn, int format)
{
	if (nColumn < 0)
	{
		for (int i = 0; i < GetColumnCount(); i++)
			m_HeaderCtrlEx.SetColumnTextAlign( i, format );
	}
	else
	{
		m_HeaderCtrlEx.SetColumnTextAlign( nColumn, format );
	}
}

void CListCtrlEx::SelectItem( int nIndex, bool bSelect /*= true*/ )	//nIndex = -1 : 픸E세궈?
{
	SetItemState( nIndex, bSelect ? LVIS_SELECTED : 0, LVIS_SELECTED );
}

void CListCtrlEx::SetItemDataReset(int index, DWORD data)
{
	if (index >= 0)
	{
		SetItemData(index, data);
	}
	else
	{
		for (int i = 0; i < GetItemCount(); i++)
			SetItemData(i, data);
	}
}

CString	CListCtrlEx::GetColumnText( int nColumn )
{
	return m_HeaderCtrlEx.GetColumnText( nColumn );
}

void CListCtrlEx::SetColumnText( int nColumn, CString sText )
{
	m_HeaderCtrlEx.SetColumnText( nColumn, sText );
}

int CListCtrlEx::GetColumnCount()
{
	return m_HeaderCtrlEx.GetItemCount();
}

CString CListCtrlEx::GetText(int nIndex, int from, int to, CString sep)
{
	int		i;
	CString sText = _T("");

	if (nIndex < 0 || nIndex >= GetItemCount())
		return _T("");

	if (to == -1 || to >= GetColumnCount())
		to = GetColumnCount() - 1;

	for (i = from; i <= to; i++)
		sText = sText + GetItemText(nIndex, i) + sep;

	return sText;
}

//0번 컬럼에서만 데이터를 찾는다.
int CListCtrlEx::FindString(CString str, int indexFrom, bool bWholeWord)
{
	LVFINDINFO info;

	info.flags = (bWholeWord ? LVFI_STRING : LVFI_PARTIAL|LVFI_STRING);
	info.psz = str;

	// Delete all of the items that begin with the string.
	return FindItem(&info, indexFrom);
}

//CListCtrl의 FindItem으로도 모든 라인의 모든 컬럼을 검사할 수 있지만
//일단 직접 구현해서 쓴다.
//단 결과 result에는 컬럼번호는 저장되지 않고 라인번호만 저장되므로
//한 라인에서 이미 찾았다면 그 라인의 다른 컬럼에서는 찾지 않고 그냥 break해야 한다.
//그렇지 않으면 동일한 라인 번호가 result에 모두 저장된다.
void CListCtrlEx::FindString( CString str, int indexFrom, std::vector<int> *column, std::vector<int> &result, bool bWholeWord/* = TRUE*/, bool bCaseSensitive /*= FALSE*/ )
{
	int		i, j;
	CString sText;
	std::vector<int> targetColumn;

	if ( indexFrom < 0 )
		indexFrom = 0;

	if (column == NULL)
		column = &targetColumn;

	if (column->size() == 0)
	{
		for (i = 0; i < GetColumnCount(); i++)
			column->push_back(i);
	}

	result.clear();

	for ( i = indexFrom; i < GetItemCount(); i++ )
	{
		for ( j = 0; j < column->size(); j++ )
		{
			sText = GetItemText( i, column->at(j) );

			//큱E拈??구분없이 검색하는 경퓖E?둘 다 소문자로 변컖E후 비교한다.
			if ( !bCaseSensitive )
			{
				sText.MakeLower();
				str.MakeLower();
			}

			if ( bWholeWord )
			{
				if ( sText == str )
				{
					result.push_back(i);
					break;
				}
			}
			else
			{
				//콅E문자열에서 sub string을 찾지만 그 역은 찾햨E못한다.
				//또한 그렇게 찾으툈E오동작 예외의 경퓖E?존재할수도 있으므로
				//그렇게 검색해서도 안된다.
				//항퍊EFindString을 호출할때는 콅E문자열에서 sub string을 검색하도록 한다.
				if ( sText.Find( str ) >= 0 )
				{
					result.push_back(i);
					break;
				}
			}
		}
	}
}

int CListCtrlEx::GetSelectedItem( int nStart /*= 0*/ )
{
	for ( int i = nStart; i < GetItemCount(); i++ )
	{
		if ( GetItemState( i, LVIS_SELECTED ) )
			return i;
	}

	return -1;
}

int CListCtrlEx::GetLastSelectedItem()
{
	for ( int i = GetItemCount() - 1; i >= 0; i-- )
	{
		if ( GetItemState( i, LVIS_SELECTED ) )
			return i;
	}

	return -1;
}

std::deque<int>	CListCtrlEx::GetSelectedItems(int nStart)
{
	std::deque<int> dqSelected;
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		int nItem = GetNextSelectedItem(pos);
		dqSelected.push_back(nItem);
	}

	return dqSelected;
}

//ItemData를 컬러로 사용할 경우에 대한 처리가 필요하다. 우선 보류.
void CListCtrlEx::DeleteColumn( int nColumn )
{
	int		i;

	if ( nColumn == -1 )//SELECT_ALL )
	{
		for ( i = GetColumnCount() - 1; i >= 0 ; i-- )
			CListCtrl::DeleteColumn( i );
	}
	else
	{
		CListCtrl::DeleteColumn( nColumn );
	}
}

void CListCtrlEx::AutoAdjustColumnWidth()
{
	int			i;
	CClientDC	dc(this);
	CString		str;

	for ( i = 0; i < GetColumnCount(); i++ )
		SetColumnWidth( i, LVSCW_AUTOSIZE_USEHEADER );
}

void CListCtrlEx::LoadColumnWidth(CWinApp* pApp, CString sSection)
{
	int		i, width;
	CString str;

	for (i = 0; i < GetColumnCount(); i++)
	{
		width = pApp->GetProfileInt(sSection + _T("\\column width"), i2S(i), 0);
		if (width > 0)
			SetColumnWidth(i, width);
	}
}

void CListCtrlEx::SaveColumnWidth(CWinApp* pApp, CString sSection)
{
	int		i;
	CString str;

	for (i = 0; i < GetColumnCount(); i++)
	{
		pApp->WriteProfileInt(sSection + _T("\\column width"), i2S(i), GetColumnWidth(i));
	}
}



#if !USE_CUSTOMDRAW
void CListCtrlEx::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{
	int			nItem		= (int)lpDIS->itemID;
	CDC			*pDC		= CDC::FromHandle(lpDIS->hDC);
	CString		sItemText;

	COLORREF	crText					= m_crText;
	COLORREF	crTextSelected			= m_crTextSelected;
	COLORREF	crTextSelectedInactive	= m_crTextSelectedInactive;
	COLORREF	crBack					= m_crBack;
	COLORREF	crBackSelected			= m_crBackSelected;
	COLORREF	crBackSelectedInactive	= m_crBackSelectedInactive;

	CRect		r, rItem;
	CListCtrlExItemColor		*pData = NULL;

	for ( int i = 0; i < GetColumnCount(); i++ )
	{
		if ( m_use_item_color )
		{
			pData = (CListCtrlExItemColor*)GetItemData( nItem );

			//만푳E현픸E아이템에 특정 색이 할당되푳E있햨E않다툈E기본 글자색컖E배경색을 사퓖E蠻娩?
			if ( pData == NULL )
			{
				crText = m_crText;
				crBack = m_crBack;
			}
			else
			{
				if ( i == 0 && !m_bApplyTextColorToFirstColumn )
					crText = m_crText;
				else
					crText = pData->crText;//RGB565ToRGB24( HIWORD( dwData ) );

				crBack = pData->crBack;//RGB565ToRGB24( LOWORD( dwData ) );
			}
		}

		GetSubItemRect( nItem, i, LVIR_BOUNDS, r );

		if ( i == 0 )
			r.right = r.left + GetColumnWidth( i );

		//enabled, selected, focused 상태에 따른 색상값을 결정한다.
		if ( IsWindowEnabled() )
		{
			crText = m_crText;
			crBack = m_crBack;

			if ( GetItemState( nItem, LVIS_SELECTED) )
			{
				if ( GetFocus() == this )
				{
					crText = m_crTextSelected;
					crBack = m_crBackSelected;
				}
				else
				{
					crText = m_crTextSelectedInactive;
					crBack = m_crBackSelectedInactive;
				}
			}
		}
		else
		{
			crBack = ::GetSysColor( COLOR_INACTIVECAPTION );
		}

		m_crSelectedOutline = m_crBack;



		//if ( GetItemState( nItem, LVIS_SELECTED) )
		//{

		//	if ( GetFocus() == this )
		//		crBack = m_crSelected;
		//	else if ( GetStyle() & LVS_SHOWSELALWAYS )
		//		crBack = ::GetSysColor( COLOR_3DFACE );

		//	if ( IsWindowEnabled() )
		pDC->FillSolidRect( r, crBack );

		if ( (i == GetColumnCount() - 1) )//&& (GetFocus() == this) )
		{
			GetSubItemRect( nItem, 0, LVIR_BOUNDS, rItem );
			rItem.right -= 1;
			rItem.bottom -= 1;

			CPen	Pen( PS_SOLID, 1, m_crSelectedOutline );
			CPen*	pOldPen = (CPen*)pDC->SelectObject( &Pen );

			pDC->MoveTo( rItem.left, rItem.top );
			pDC->LineTo( rItem.right, rItem.top );
			pDC->LineTo( rItem.right, rItem.bottom - 1 );
			pDC->LineTo( rItem.left, rItem.bottom - 1 );
			pDC->LineTo( rItem.left, rItem.top );

			pDC->SelectObject( pOldPen );

			Pen.DeleteObject();
		}
		//}

		r.left	+= 2;
		r.top	+= 1;
		sItemText = GetItemText( nItem, i );

		UINT nFmt;

		if ( GetColumnTextAlign( i ) == LVCFMT_LEFT )
			nFmt = DT_LEFT;
		else if ( GetColumnTextAlign( i ) == LVCFMT_CENTER )
			nFmt = DT_CENTER;
		else if ( GetColumnTextAlign( i ) == LVCFMT_RIGHT )
			nFmt = DT_RIGHT;

		nFmt = nFmt | DT_SINGLELINE | DT_VCENTER;

		if ( GetItemState( nItem, LVIS_SELECTED) )
		{
			if ( GetFocus() == this )
				pDC->SetTextColor( crTextSelected );
			else
				pDC->SetTextColor( crTextSelectedInactive );
		}
		else
		{
			pDC->SetTextColor( crText );
		}

		pDC->DrawText( sItemText, r, nFmt );
	}
}
#endif

bool CListCtrlEx::GetCellRect(int nRow, int nCol, CRect& rect)
{
	// Find the top and bottom of the cell-rectangle
	CRect rowRect;
	if (GetItemRect(nRow, rowRect, LVIR_BOUNDS)==FALSE)
		return false;

	// Find the left and right of the cell-rectangle using the CHeaderCtrl
	CRect colRect;
	if (GetHeaderCtrl()->GetItemRect(nCol, colRect)==FALSE)
		return false;

	// Adjust for scrolling
	colRect.left -= GetScrollPos(SB_HORZ);
	colRect.right -= GetScrollPos(SB_HORZ);

	rect.left = colRect.left;
	rect.top = rowRect.top;
	rect.right = colRect.right;
	rect.bottom = rowRect.bottom;
	return TRUE;
}

void CListCtrlEx::InvalidateItem(int nIndex, bool bErase /*= TRUE*/)
{
	if ( nIndex < 0 || nIndex >= GetItemCount() )
		return;

	RECT rcItem;
	GetItemRect(nIndex, &rcItem, LVIR_BOUNDS);
	InvalidateRect(&rcItem, bErase);
}

void CListCtrlEx::SwapItem(int nIndex1, int nIndex2)
{
	ASSERT(__min(nIndex1, nIndex2) >= 0 && __max(nIndex1, nIndex2) < GetItemCount());

	CHeaderCtrl* pHeaderCtrl = GetHeaderCtrl();
	int nColumn = pHeaderCtrl ? pHeaderCtrl->GetItemCount() : 1;

	std::vector<CString> textArray1, textArray2;
	for (int i = 0; i < nColumn; ++i)
	{
		textArray1.push_back(GetItemText(nIndex1, i));
		textArray2.push_back(GetItemText(nIndex2, i));
	}

	LV_ITEM lvi1, lvi2;
	lvi1.mask = LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
	lvi1.iSubItem = 0;

	lvi1.stateMask =	LVIS_FOCUSED | LVIS_SELECTED | LVIS_CUT | 
		LVIS_DROPHILITED | LVIS_OVERLAYMASK | LVIS_STATEIMAGEMASK;
	lvi2 = lvi1;

	lvi1.iItem = nIndex1;
	lvi2.iItem = nIndex2;

	GetItem(&lvi1);
	GetItem(&lvi2);

	for (int i = 0; i < nColumn; ++i)
	{
		SetItemText(nIndex1, i, textArray2[i]);
		SetItemText(nIndex2, i, textArray1[i]);
	}

	std::swap(lvi1.iItem, lvi2.iItem);
	SetItem(&lvi2);
	SetItem(&lvi1);
}

void CListCtrlEx::MoveItem(int nFrom, int nTo)
{
	ASSERT(__min(nFrom, nTo) >= 0 && __max(nFrom, nTo) < GetItemCount());

	CHeaderCtrl* pHeaderCtrl = GetHeaderCtrl();
	int nColumn = pHeaderCtrl ? pHeaderCtrl->GetItemCount() : 1;

	std::vector<CString> textArray;
	for (int i = 0; i < nColumn; ++i)
		textArray.push_back(GetItemText(nFrom, i));

	LV_ITEM lvi;
	lvi.mask = LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
	lvi.iSubItem = 0;
	lvi.stateMask = LVIS_FOCUSED | LVIS_SELECTED | LVIS_CUT | 
		LVIS_DROPHILITED | LVIS_OVERLAYMASK | LVIS_STATEIMAGEMASK;
	lvi.iItem = nFrom;

	GetItem(&lvi);

	DeleteItem(nFrom);
	InsertItem(nTo, textArray[0]);

	for (int i = 1; i < nColumn; ++i)
		SetItemText(nTo, i, textArray[i]);

	lvi.iItem = nTo;
	SetItem(&lvi);

	m_bModified = true;
}

bool CListCtrlEx::MoveUp(int nIndex)
{
	if ( nIndex < 0 || nIndex >= GetItemCount() )
		return false;

	if (nIndex - 1 < 0)
		return FALSE;

	SwapItem(nIndex, nIndex - 1);
	EnsureVisible(nIndex - 1, FALSE);
	return TRUE;
}

bool CListCtrlEx::MoveDown(int nIndex)
{
	if ( nIndex < 0 || nIndex >= GetItemCount() )
		return false;

	if (nIndex + 1 >= GetItemCount())
		return FALSE;

	SwapItem(nIndex, nIndex + 1);
	EnsureVisible(nIndex + 1, FALSE);
	return TRUE;
}

bool CListCtrlEx::MoveTop(int nIndex)
{
	if ( nIndex < 0 || nIndex >= GetItemCount() )
		return false;

	if (0 == nIndex)
		return FALSE;

	MoveItem(nIndex, 0);
	EnsureVisible(0, FALSE);
	return TRUE;
}

bool CListCtrlEx::MoveBottom(int nIndex)
{
	if ( nIndex < 0 || nIndex >= GetItemCount() )
		return false;

	int nCount = GetItemCount();
	if (nCount - 1 == nIndex)
		return FALSE;

	MoveItem(nIndex, nCount - 1);
	EnsureVisible(nCount - 1, FALSE);
	return TRUE;
}

void CListCtrlEx::SetTopIndex(int nIndex)
{
	if ( nIndex < 0 || nIndex >= GetItemCount() )
		return;

	if (GetItemCount() > 0)
	{
		CRect rcItem;
		int nTopIndex = GetTopIndex();
		GetItemRect(nIndex, &rcItem, LVIR_BOUNDS);
		Scroll(CSize(0, (nIndex - nTopIndex) * rcItem.Height()));
	}
}

void CListCtrlEx::OnLvnColumnclick(NMHDR *pNMHDR, LRESULT *pResult)
{
	if ( m_bAllowSort && (GetStyle() & LVS_OWNERDATA) == 1 )
	{
		AfxMessageBox( _T("Sort is not supported with LVS_OWNERDATA style.") );
		m_bAllowSort = false;
		return;
	}

	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: 여기에 컨트롤 알림 처리콅E코드를 추가합니다.
	*pResult = 0;
	Sort( pNMLV->iSubItem );

	//TRACE( _T("%d\n"), pNMLV->iSubItem );
}

BOOL CListCtrlEx::OnNMRClick(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 여기에 컨트롤 알림 처리콅E코드를 추가합니다.

	//기본 팝업메뉴를 이용하지 않는다면 메인에서 RClick 메시지를 처리할 수 있도록 해야한다.
	if ( !m_bUsePopupMenu )
		return FALSE;

	if ( pNMItemActivate->iItem < 0 || pNMItemActivate->iItem >= GetItemCount() )
		return TRUE;

	DWORD style = ListView_GetExtendedListViewStyle( GetSafeHwnd() );
	bool bCheckListBox = ( style & LVS_EX_CHECKBOXES );
	CMenu	menu;

	menu.CreatePopupMenu();

	menu.AppendMenu( MF_STRING, listctrlex_menu_add_prior, _T("선택된 항목 위에 새 항목 추가(&W)") );
	menu.AppendMenu( MF_STRING, listctrlex_menu_add_next, _T("선택된 항목 아래 새 항목 추가(&S)") );
	if (GetKeyState(VK_SHIFT) & 0x8000)
	{
		menu.AppendMenu(MF_SEPARATOR);
		menu.AppendMenu(MF_STRING, listctrlex_menu_copy_prior, _T("선택된 항목을 복사하여 위에 추가"));
		menu.AppendMenu(MF_STRING, listctrlex_menu_copy_next, _T("선택된 항목을 복사하여 아래에 추가"));
	}

	menu.AppendMenu(MF_SEPARATOR);
	menu.AppendMenu( MF_STRING, listctrlex_menu_move_up, _T("선택된 항목을 위로 이동(&U)") );
	menu.AppendMenu( MF_STRING, listctrlex_menu_move_down, _T("선택된 항목을 아래로 이동(&D)") );
	menu.AppendMenu( MF_SEPARATOR);
	menu.AppendMenu( MF_STRING, listctrlex_menu_copy_clipboard, _T("복사(&C)\tCtrl+C") );
	menu.AppendMenu( MF_STRING, listctrlex_menu_copy_clipboard_head, _T("복사(헤더 포함)\tCtrl+Shift+C") );
	menu.AppendMenu(MF_STRING, listctrlex_menu_paste_insert, _T("붙여넣기(&V)\tCtrl+V"));

	if ( bCheckListBox )
	{
		menu.AppendMenu( MF_SEPARATOR);
		menu.AppendMenu( MF_STRING, listctrlex_menu_checkall, _T("모든 항목 체크(&A)") );
	}

	menu.AppendMenu( MF_SEPARATOR);
	menu.AppendMenu( MF_STRING, listctrlex_menu_delete, _T("선택된 항목 삭제(&X)") );

	if ( pNMItemActivate->iItem == 0 )
		menu.EnableMenuItem( listctrlex_menu_move_up, MF_DISABLED );

	if ( pNMItemActivate->iItem == GetItemCount() - 1 )
		menu.EnableMenuItem( listctrlex_menu_move_down, MF_DISABLED );

	menu.CheckMenuItem( listctrlex_menu_checkall, m_bCheckAll ? MF_CHECKED : MF_UNCHECKED );

	SetMenu( &menu );

	CPoint	pt = pNMItemActivate->ptAction;

	ClientToScreen( &pt );

	m_bPopupMenuDisplayed = true;
	menu.TrackPopupMenu( TPM_LEFTALIGN, pt.x, pt.y, this );
	m_bPopupMenuDisplayed = false;

	menu.DestroyMenu();

	*pResult = 0;

	return TRUE;
}

BOOL CListCtrlEx::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	if ( pMsg->message == WM_MOUSEWHEEL && m_bInEditing )
	{
		return true;

		/*
		//폴翡중에 스크롤이 되툈E폴翡중인 셀의 사각?E정보를 구해서
		//edit을 그 좌표로 이동시켜줘야 한다. client영역을 벗엉廐툈E화면에서 보여지툈E안된다.
		CRect	r;

		GetSubItemRect( m_nEditItem, m_nEditSubItem, LVIR_BOUNDS, r );
		m_pEditCtrl->MoveWindow( r );
		Invalidate();
		*/
	}
	else if ( pMsg->message == WM_KEYDOWN )
	{
		switch ( pMsg->wParam )
		{
		case 'C'		:	if ( GetKeyState( VK_CONTROL ) & 0x8000 )
							{
								CopyToClipboard(_T("|") );
								MessageBeep( MB_ICONINFORMATION );
								return true;
							}
							break;
		case 'V'		:	if (GetKeyState(VK_CONTROL) & 0x8000)
							{
								PasteInsertFromClipboard();
								MessageBeep(MB_ICONINFORMATION);
								return true;
							}
							break;
		case VK_F2		:	if ( m_bInEditing )
								return true;
							else
								EditSubItem( GetSelectedItem(), m_nEditSubItem );
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

void CListCtrlEx::OnPopupMenu( UINT nMenuID )
{
	switch ( nMenuID )
	{
	case listctrlex_menu_add_prior :
		AddItem( GetSelectedItem(), _T("") );
		break;
	case listctrlex_menu_add_next :
		AddItem( GetSelectedItem() + 1, _T("") );
		break;
	case listctrlex_menu_copy_prior :
	{
		int nSelected = GetSelectedItem();

		AddItem(nSelected, _T(""));

		nSelected++;

		for (int i = 0; i < GetColumnCount(); i++)
			SetItemText(nSelected - 1, i, GetItemText(nSelected, i));
	}
	break;
	case listctrlex_menu_copy_next:
	{
		int nSelected = GetSelectedItem();

		nSelected++;
		AddItem(nSelected, _T(""));

		for (int i = 0; i < GetColumnCount(); i++)
			SetItemText(nSelected, i, GetItemText(nSelected - 1, i));
	}
	break;
	case listctrlex_menu_move_up :
		if ( GetItemState( 0, LVIS_SELECTED ) )
			break;

		for ( int i = 1; i < GetItemCount(); i++ )
		{
			if ( GetItemState( i, LVIS_SELECTED ) )
				SwapItem( i - 1, i );
		}
		break;
	case listctrlex_menu_move_down :
		if ( GetItemState( GetItemCount() - 1, LVIS_SELECTED ) )
			break;

		for ( int i = GetItemCount() - 2; i >= 0; i-- )
		{
			if ( GetItemState( i, LVIS_SELECTED ) )
				SwapItem( i, i + 1 );
		}
		break;
/*
	case ID_MENU_MOVE_UP_STEP :
		BeginWaitCursor();
		while ( GetSelectedCount() >= 1 )
		{
			//선택된 항목들을 위로 이동시키컖E
			for ( int i = 1; i < GetItemCount(); i++ )
			{
				if ( GetItemState( i, LVIS_SELECTED ) )
					SwapItem( i - 1, i );
			}

			//맨 마지막 선택 항목은 선택 해제한 후 다시 이동을 반복한다.
			//최종적으로 선택된 항목이 1개 남을때깩?E반복한다.
			SetItemState( GetLastSelectedItem(), 0, LVIS_SELECTED );
		}
		EndWaitCursor();
		break;
	case ID_MENU_ADD_PRIOR_X :
		{
			int	n = AddItem( GetSelectedItem(), _T("") );
			SetItemText( n, 0, GetItemText( n + 1, 0 ) );
			SetItemText( n, 1, GetItemText( n + 1, 1 ) );
			SetItemText( n, 2, _T("X") );
			SetItemText( n, 4, _T("5") );
			SetItemText( n, 6, _T("X") );
		}
		break;
*/
	case listctrlex_menu_delete :
		DeleteSelectedItems();
		break;
	case listctrlex_menu_copy_clipboard :
		CopyToClipboard();
		break;
	case listctrlex_menu_copy_clipboard_head :
		CopyToClipboard(_T("|"), true );
		break;
	case listctrlex_menu_paste_insert :
		PasteInsertFromClipboard();
		break;
	case listctrlex_menu_checkall :
		m_bCheckAll = !m_bCheckAll;
		CheckAll( m_bCheckAll );
		break;
	}
}

// HitTestEx	- Determine the row index and column index for a point
// Returns	- the row index or -1 if point is not over a row
// point	- point to be tested.
// col		- to hold the column index
int CListCtrlEx::HitTestEx( CPoint &point, int *pColumn )
{
	int ColumnNum = 0;
	int Row = HitTest (point, NULL);

	if (pColumn)
		*pColumn = 0;

	// Make sure that the ListView is in LVS_REPORT
	if ((GetWindowLong (m_hWnd, GWL_STYLE) & LVS_TYPEMASK) != LVS_REPORT)
		return Row;

	// Get the top and bottom row visible
	Row = GetTopIndex();
	int Bottom = Row + GetCountPerPage();
	if (Bottom > GetItemCount())
		Bottom = GetItemCount();

	// Get the number of columns
	CHeaderCtrl* pHeader = (CHeaderCtrl*) GetDlgItem(0);
	int nColumnCount = pHeader->GetItemCount();

	// Loop through the visible rows
	for(; Row <= Bottom; Row++)
	{
		// Get bounding rect of item and check whether point falls in it.
		CRect Rect;
		GetItemRect (Row, &Rect, LVIR_BOUNDS);
		if (Rect.PtInRect (point))
		{
			// Now find the column
			for (ColumnNum = 0; ColumnNum < nColumnCount; ColumnNum++)
			{
				int ColWidth = GetColumnWidth (ColumnNum);
				if (point.x >= Rect.left && point.x <= (Rect.left + ColWidth))
				{
					if (pColumn)
						*pColumn = ColumnNum;
					return Row;
				}
				Rect.left += ColWidth;
			}
		}
	}
	return -1;
}

CEdit* CListCtrlEx::EditSubItem (int Item, int Column)
{
	if ( !m_bAllowEdit )
		return NULL;
	// The returned pointer should not be saved

	// Make sure that the item is visible
	if (!EnsureVisible (Item, TRUE)) 
	{
		//InsertItem( )
		if (!EnsureVisible (Item, TRUE)) 
			return NULL;
	}

	// Make sure that nCol is valid
	CHeaderCtrl* pHeader = (CHeaderCtrl*) GetDlgItem(0);
	int nColumnCount = pHeader->GetItemCount();
	if (Column >= nColumnCount || GetColumnWidth (Column) < 5)
		return NULL;

	// Get the column offset
	int Offset = 0;
	for (int iColumn = 0; iColumn < Column; iColumn++)
		Offset += GetColumnWidth (iColumn);

	CRect Rect;
	GetItemRect (Item, &Rect, LVIR_BOUNDS);

	// Now scroll if we need to expose the column
	CRect ClientRect;
	GetClientRect (&ClientRect);

	if ( Offset + Rect.left < 0 || Offset + Rect.left > ClientRect.right )
	{
		CSize Size;
		if (Offset + Rect.left > 0)
			Size.cx = -(Offset - Rect.left);
		else
			Size.cx = Offset - Rect.left;
		Size.cy = 0;
		Scroll (Size);
		Rect.left -= Size.cx;
	}

	// Get Column alignment
	LV_COLUMN	lvCol;

	lvCol.mask = LVCF_FMT;
	GetColumn (Column, &lvCol);

	DWORD dwStyle;

	if ((lvCol.fmt & LVCFMT_JUSTIFYMASK) == LVCFMT_LEFT)
		dwStyle = ES_LEFT;
	else if ((lvCol.fmt & LVCFMT_JUSTIFYMASK) == LVCFMT_RIGHT)
		dwStyle = ES_RIGHT;
	else
		dwStyle = ES_CENTER;


	Rect.left += (Offset + 1);
	Rect.right = Rect.left + GetColumnWidth (Column) - 1;
	Rect.top += 1;
	Rect.bottom -= 1;

	DWORD dwExStyle = ListView_GetExtendedListViewStyle( GetSafeHwnd() );
	if ( (Column == 0) && (dwExStyle & LVS_EX_CHECKBOXES) )
		Rect.left += 18;

	if (Rect.right > ClientRect.right)
		Rect.right = ClientRect.right;

	m_sOldText = GetItemText(Item, Column);

	dwStyle |= WS_BORDER | WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL;
	CEdit *pEdit = new gxEditCell (this, Item, Column, GetItemText (Item, Column));
	pEdit->Create (dwStyle, Rect, this, IDC_EDIT_CELL);
	m_pEditCtrl = pEdit;

	m_bInEditing = true;
	m_nEditItem = Item;
	m_nEditSubItem = Column;


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
	dispinfo.item.iItem = Item;
	dispinfo.item.iSubItem = Item;
	//dispinfo.item.pszText = bEscape ? NULL : LPTSTR((LPCTSTR)Text);
	//dispinfo.item.cchTextMax = Text.GetLength();

	GetParent()->SendMessage(WM_NOTIFY, GetDlgCtrlID(), (LPARAM)&dispinfo);

	return pEdit;
}


BOOL CListCtrlEx::OnLvnEndlabeledit(NMHDR *pNMHDR, LRESULT *pResult)
{
	LV_DISPINFO *plvDispInfo = (LV_DISPINFO *)pNMHDR;
	LV_ITEM	*plvItem = &plvDispInfo->item;

	if (plvItem->pszText != NULL && plvItem->pszText != m_sOldText )
	{
		m_bModified = true;
		SetItemText (plvItem->iItem, plvItem->iSubItem, plvItem->pszText);
	}

	m_bInEditing = false;

	*pResult = FALSE;

	return FALSE;
}

//편집 전의 텍스트로 되돌린다.(편집 레이블이 파일명이고 파일명 변경이 실패한 경우 쓸 수 있다.)
void CListCtrlEx::UndoEditLabel()
{
	SetItemText(m_nEditItem, m_nEditSubItem, m_sOldText);
}



BOOL CListCtrlEx::OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;

	if ( m_bAllowEdit == false )
		return FALSE;

	int iItem = pNMItemActivate->iItem;
	int iSubItem = pNMItemActivate->iSubItem;

	if ( iItem >= 0 && iItem < GetItemCount() &&
		iSubItem >= 0 && iSubItem < GetColumnCount() )
	{
		EditSubItem (iItem, iSubItem);
		return TRUE;
	}
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
	return FALSE;
}

void CListCtrlEx::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시햨E처리콅E코드를 추가 및/또는 기본값을 호출합니다.

	CListCtrl::OnLButtonDblClk(nFlags, point);
}


void CListCtrlEx::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시햨E처리콅E코드를 추가 및/또는 기본값을 호출합니다.
	CListCtrl::OnLButtonDown(nFlags, point);
}


void CListCtrlEx::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시햨E처리콅E코드를 추가 및/또는 기본값을 호출합니다.

	CListCtrl::OnLButtonUp(nFlags, point);
}


void CListCtrlEx::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: 여기에 메시햨E처리콅E코드를 추가 및/또는 기본값을 호출합니다.
	CListCtrl::OnHScroll(nSBCode, nPos, pScrollBar);
}


void CListCtrlEx::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: 여기에 메시햨E처리콅E코드를 추가 및/또는 기본값을 호출합니다.
	/*
	CRect	r;
	MessageBeep(0);
	GetSubItemRect( m_nEditItem, m_nEditSubItem, LVIR_BOUNDS, r );
	m_pEditCtrl->MoveWindow( r );
	Invalidate();
	*/

	//폴翡중일때 스크롤되툈E폴翡을 강제로 종료시킨다.
	CListCtrl::OnVScroll(nSBCode, nPos, pScrollBar);
}


void CListCtrlEx::OnKillFocus(CWnd* pNewWnd)
{
	CListCtrl::OnKillFocus(pNewWnd);

	// TODO: 여기에 메시햨E처리콅E코드를 추가합니다.
	Invalidate();
}


LRESULT CListCtrlEx::OnNcHitTest(CPoint point)
{
	// TODO: 여기에 메시햨E처리콅E코드를 추가 및/또는 기본값을 호출합니다.
	return CListCtrl::OnNcHitTest(point);
}


void CListCtrlEx::OnPaint()
{
	CRect rcHeader;
	CHeaderCtrl* pHeaderCtrl = GetHeaderCtrl();
	pHeaderCtrl->GetClientRect(&rcHeader);

	//항목이 없을 경퓖E화면에 메시지를 중앙에 표시해준다.
	//단, 띄엉梶기가 되엉復햨E않은 영문장은 자동 개행이 안됨.
	//출처 : http://www.devpia.com/MAEUL/Contents/Detail.aspx?BoardID=51&MAEULNO=20&no=8674&page=2
	if ( GetItemCount() == 0 )
	{
		CPaintDC	dc1(this); // device context for painting
		CRect rc;
		GetClientRect( &rc );

		CMemoryDC	dc( &dc1, &rc, false );
		//폰트를 잡아줘야 큱E?瓚悶?동일한 폰트로 표시된다.
		//그렇햨E않으툈E기본 폰트와 크기로 표시된다.
		//CFont* pPrevFont = dc.SelectObject ( GetFont() );

		//CString str = "표시할 항목이 없습니다.";
		CString str = _T("No data.");
		//GetWindowText( str );


		if ((GetStyle() & LVS_TYPEMASK) == LVS_REPORT)
		{
			CRect rcHeader;
			CHeaderCtrl* pHeaderCtrl = GetHeaderCtrl();
			pHeaderCtrl->GetClientRect(&rcHeader);
			rc.SubtractRect( &rc, &rcHeader);
		}

		CRect rcText = rc;

		dc.FillSolidRect( rc, m_crBack );
		return;
		dc.SetBkMode( TRANSPARENT );
		dc.DrawText( str, &rcText, DT_CENTER | DT_WORDBREAK | DT_CALCRECT  );
		rcText.OffsetRect( ((rc.Width()-rcText.Width())/2) , ((rc.Height()-rcText.Height())/2) );
		dc.DrawText( str, &rcText, DT_CENTER | DT_WORDBREAK );

		//dc.SelectObject( pPrevFont );

		//CListCtrl::OnPaint();
	}
	else
	{
#if 0
		CListCtrl::OnPaint();
#else
		//이 방식으로 그려주면 빈 컬럼 영역까지 색깔이 칠해지지만
		//약간 속도가 느림이 느껴진다.
		CPaintDC dc1(this); // device context for painting
		CRect rc;
		GetClientRect( rc );

		//CMemoryDC에 rc를 넘겨줄 때 그냥 넘겨주툈EHeaderCtrl이 제큱E?repaint가 안된다.
		rc.top += rcHeader.Height();
		CMemoryDC	dc( &dc1, &rc, true );

		CRect headerRect;
		GetDlgItem(0)->GetWindowRect(&headerRect);
		ScreenToClient(&headerRect);
		dc.ExcludeClipRect(&headerRect);

		CRect clip;
		dc.GetClipBox(&clip);
		clip.top -= 5;
		dc.FillSolidRect( clip, IsWindowEnabled() ? m_crBack : RGB(192,192,192) );
		DefWindowProc(WM_PAINT, (WPARAM)dc.m_hDC, (LPARAM)0);
#endif
	}
}

bool CListCtrlEx::CopyToClipboard( LPCTSTR lpszSeparator /*= _T("|")*/, bool bHead /*= false*/ )
{
	ASSERT( ::IsWindow( GetSafeHwnd() ) );

	int		i;
	CString sResult = _T("");
	POSITION pos = GetFirstSelectedItemPosition();

	if ( !pos )
		return TRUE;

	int		nItem, nCount = 0;
	int		nColumn = 1;
	CWaitCursor wait;

	if ( ( GetStyle() & LVS_TYPEMASK ) == LVS_REPORT &&
		( GetExtendedStyle() & LVS_EX_FULLROWSELECT ) )
	{
		CHeaderCtrl* pHeader = GetHeaderCtrl();
		nColumn = pHeader ? pHeader->GetItemCount() : 1;

		//shift키깩?E조합되툈E?E超蝴갋복사한다.
		if ( ( bHead || (GetKeyState( VK_SHIFT ) < 0) ) && pHeader )
		{
			for ( i = 0; i < nColumn; ++i )
			{
				sResult += GetColumnText( i );
				if ( i != nColumn - 1 )
					sResult += lpszSeparator;
			}

			sResult += _T("\n");
		}
	}

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

	sResult += _T("\n");

	if (OpenClipboard() == false)
	{
		AfxMessageBox(_T("can't open clipboard."));
		return false;
	}

	/*
	HGLOBAL hMem;
	char szAssert[256];
	char *pMem;

	sprintf( szAssert, "Put assert info here" );
	hMem = GlobalAlloc( GHND|GMEM_DDESHARE, strlen( szAssert ) + 1 );

	if( hMem )
	{
	pMem = (char*)GlobalLock( hMem );
	strcpy( pMem, sResult );
	GlobalUnlock( hMem );
	EmptyClipboard();
	SetClipboardData( CF_TEXT, hMem );
	}

	CloseClipboard();
	return true;
	*/

	/*
	EmptyClipboard();

	int nLen = sResult.GetLength() * sizeof(TCHAR) + 2;
	HGLOBAL hGlobal = GlobalAlloc( GMEM_MOVEABLE | GMEM_DDESHARE, nLen );
	LPSTR pGlobalData = (LPSTR)GlobalLock(hGlobal);

	USES_CONVERSION_EX;
	CopyMemory( pGlobalData, T2CW_EX(sResult, _ATL_SAFE_ALLOCA_DEF_THRESHOLD), nLen);
	//strncpy(pGlobalData, T2CW_EX(sResult.GetBuffer(0)), nLen);
	SetClipboardData(CF_UNICODETEXT, hGlobal);

	GlobalUnlock(hGlobal);
	GlobalFree(hGlobal);

	CloseClipboard();
	*/

	HGLOBAL glob = GlobalAlloc(GMEM_FIXED, sizeof(char) * sResult.GetLength());
	memcpy(glob, sResult, sizeof(char) * sResult.GetLength());
	OpenClipboard();
	EmptyClipboard();
	SetClipboardData(CF_TEXT, glob);
	CloseClipboard();
	MessageBeep(0);

	return TRUE;
}

void CListCtrlEx::PasteInsertFromClipboard()
{
	TCHAR	*buffer = NULL;
	HGLOBAL	hGlobal;

	int		i;
	int		pos;
	int		iItem = GetSelectedItem();
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

			if (bDifferWarning && (GetColumnCount() != (nSepCount+1)))
			{
				int result = AfxMessageBox(_T("붙여넣기 할 데이터의 컬럼 갯수가 리스트의 컬럼 갯수와 다릅니다.\n그대로 붙여넣기 할까요?"), MB_YESNO);
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
						SetItemText(idx, 0, ar.GetAt(i));
					}
					else
					{
						idx = AddItem(ar.GetAt(i), NULL);
					}
				}
				else
				{
					SetItemText(idx, i, ar.GetAt(i));
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
bool CListCtrlEx::load_from_file( CString sfile, TCHAR separator, bool match_column_count /*= true*/, bool reset_before_load /*= true*/, bool add_index /*= false*/ )
{
	if ( GetColumnCount() == 0 )
		return false;

	if ( reset_before_load )
		DeleteAllItems();

	TCHAR	sLine[512];
	CString str;

	FILE	*fp = _tfopen( sfile, _T("rt") );

	if ( fp == NULL )
		return false;


	while (_fgetts( sLine, 512, fp ))
	{
		if ( match_column_count && (get_char_count(sLine, separator)) >= GetColumnCount() )
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
bool CListCtrlEx::save_to_file( CString sfile, TCHAR separator, bool includeHeader /*= false*/ )
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

/*
LRESULT CListCtrlEx::OnSetFont(WPARAM wParam, LPARAM)
{
	TRACE( "%s\n" __FUNCTION__ );
	LRESULT res =  Default();

	CRect rc;
	GetWindowRect( &rc );

	WINDOWPOS wp;
	wp.hwnd = m_hWnd;
	wp.cx = rc.Width();
	wp.cy = rc.Height();
	wp.flags = SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER;

	SendMessage( WM_WINDOWPOSCHANGED, 0, (LPARAM)&wp );

	return res;
}
*/

//MeasureItem()에서도 라인 간격을 조정할 수 있는데 현재 설정에서는 실행되지 않는다.
//가상 ImageList를 이용해서 라인 간격을 조정하자.
void CListCtrlEx::MeasureItem( LPMEASUREITEMSTRUCT lpMeasureItemStruct )
{
	TRACE( "%s\n" __FUNCTION__ );
	LOGFONT lf;
	GetFont()->GetLogFont( &lf );

	if( lf.lfHeight < 0 )
		lpMeasureItemStruct->itemHeight = -lf.lfHeight + 18; 
	else
		lpMeasureItemStruct->itemHeight = lf.lfHeight + 18; 

}
/*
void CListCtrlEx::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct) 
{
	CListCtrl::OnMeasureItem(nIDCtl, lpMeasureItemStruct);
}
*/
//기본 글자퍊E 배경색을 설정한다.
void CListCtrlEx::SetDefaultItemColor( COLORREF crText, COLORREF crBack )
{
	m_crText = crText;
	m_crBack = crBack;
}

//32비트인 dw에 들엉復는 R, G, B를 추출하여 16비트(5+6+5) 컬러로 리턴한다.
WORD CListCtrlEx::RGB24ToRGB565( DWORD dw )
{
	BYTE	bt;
	WORD	wd = 0;

	bt = GetRValue( dw ) >> 3;
	wd = bt;
	wd <<= 6;

	bt = GetGValue( dw ) >> 2;
	wd |= bt;
	wd <<= 5;

	bt = GetBValue( dw ) >> 3;
	wd |= bt;

	return wd;
}

//두 DWORD를 WORD로 변환하여 하나의 DWORD로 변환한다.
DWORD CListCtrlEx::RGB24ToRGB565( DWORD dw1, DWORD dw2 )
{
	DWORD dw = RGB24ToRGB565( dw1 );
	dw <<= 16;

	dw |= RGB24ToRGB565( dw2 );
	return dw;
}

//RGB565로 축약된 WORD를 다시 24비트 RGB(DWORD)로 복원시킨다.
DWORD CListCtrlEx::RGB565ToRGB24( WORD wd )
{
	//565로 만들엉?Ewd에서 다시 rgb로 복원시켜보자.
	BYTE	r = ( wd & 0xf800 ) >> 11 << 3;
	BYTE	g = ( wd & 0x07e0 ) >> 5 << 2;
	BYTE	b = ( wd & 0x001f ) << 3;

	return RGB( r, g, b );
}

COLORREF CListCtrlEx::get_text_color(int iItem, int iSubItem)
{
	if ((iItem >= 0) && m_use_item_color)
	{
		CListCtrlExItemColorDeque *dq = (CListCtrlExItemColorDeque*)GetItemData( iItem );

		if ((dq == NULL) ||
			(dq->dqColor.size() <= iSubItem) ||
			(dq->dqColor[iSubItem].text == LISTCTRLEX_UNUSED_COLOR))
			return m_crText;

		return dq->dqColor[iSubItem].text;
	}

	return m_crText;
}

//특정 항목의 글자퍊E설정
void CListCtrlEx::set_text_color(int iItem, int iSubItem, COLORREF text)
{
	m_use_item_color = true;

	CListCtrlExItemColorDeque *dq = (CListCtrlExItemColorDeque*)GetItemData( iItem );

	if (dq == NULL)
	{
		dq = (CListCtrlExItemColorDeque *)new CListCtrlExItemColorDeque;
		SetItemData(iItem, (DWORD_PTR)dq);
	}

	while (dq->dqColor.size() <= iSubItem)
		dq->dqColor.push_back(CListCtrlExItemColor());

	dq->dqColor[iSubItem].text = text;
}

//특정 항목의 배경퍊E설정
void CListCtrlEx::set_back_color(int iItem, int iSubItem, COLORREF back)
{
	m_use_item_color = true;

	CListCtrlExItemColorDeque *dq = (CListCtrlExItemColorDeque*)GetItemData( iItem );

	if (dq == NULL)
	{
		dq = (CListCtrlExItemColorDeque *)new CListCtrlExItemColorDeque;
		SetItemData(iItem, (DWORD_PTR)dq);
	}

	while (dq->dqColor.size() <= iSubItem)
		dq->dqColor.push_back(CListCtrlExItemColor());

	dq->dqColor[iSubItem].back = back;
}


void CListCtrlEx::set_item_color(int iItem, int iSubItem, COLORREF crText, COLORREF crBack)
{
	m_use_item_color = true;

	CListCtrlExItemColorDeque *dq = (CListCtrlExItemColorDeque*)GetItemData( iItem );

	if (dq == NULL)
	{
		dq = (CListCtrlExItemColorDeque *)new CListCtrlExItemColorDeque;
		SetItemData(iItem, (DWORD_PTR)dq);
	}

	while (dq->dqColor.size() <= iSubItem)
		dq->dqColor.push_back(CListCtrlExItemColor());

	dq->dqColor[iSubItem].text = crText;
	dq->dqColor[iSubItem].back = crBack;
}


/*
namespace {
bool IsThemeEnabled()
{
HMODULE hinstDll;
bool XPStyle = false;
bool (__stdcall *pIsAppThemed)();
bool (__stdcall *pIsThemeActive)();

// Test if operating system has themes enabled
hinstDll = ::LoadLibrary("UxTheme.dll");
if (hinstDll)
{
(FARPROC&)pIsAppThemed = ::GetProcAddress(hinstDll, "IsAppThemed");
(FARPROC&)pIsThemeActive = ::GetProcAddress(hinstDll,"IsThemeActive");
::FreeLibrary(hinstDll);
if (pIsAppThemed != NULL && pIsThemeActive != NULL)
{
if (pIsAppThemed() && pIsThemeActive())
{
// Test if application has themes enabled by loading the proper DLL
hinstDll = LoadLibrary("comctl32.dll");
if (hinstDll)
{
DLLGETVERSIONPROC pDllGetVersion = (DLLGETVERSIONPROC)::GetProcAddress(hinstDll, "DllGetVersion");
::FreeLibrary(hinstDll);
if (pDllGetVersion != NULL)
{
DLLVERSIONINFO dvi;
ZeroMemory(&dvi, sizeof(dvi));
dvi.cbSize = sizeof(dvi);
HRESULT hRes = pDllGetVersion ((DLLVERSIONINFO *) &dvi);
if (SUCCEEDED(hRes))
XPStyle = dvi.dwMajorVersion >= 6;
}
}
}
}
}
return XPStyle;
}

LRESULT EnableWindowTheme(HWND hwnd, LPCWSTR app, LPCWSTR idlist)
{
HMODULE hinstDll;
HRESULT (__stdcall *pSetWindowTheme)(HWND hwnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList);
HANDLE (__stdcall *pOpenThemeData)(HWND hwnd, LPCWSTR pszClassList);
HRESULT (__stdcall *pCloseThemeData)(HANDLE hTheme);

hinstDll = ::LoadLibrary("UxTheme.dll");
if (hinstDll)
{
(FARPROC&)pOpenThemeData = ::GetProcAddress(hinstDll, TEXT("OpenThemeData"));
(FARPROC&)pCloseThemeData = ::GetProcAddress(hinstDll, TEXT("CloseThemeData"));
(FARPROC&)pSetWindowTheme = ::GetProcAddress(hinstDll, TEXT("SetWindowTheme"));
::FreeLibrary(hinstDll);
if (pSetWindowTheme && pOpenThemeData && pCloseThemeData)
{
HANDLE theme = pOpenThemeData(hwnd,L"ListView");
if (theme!=NULL)
{
VERIFY(pCloseThemeData(theme)==S_OK);
return pSetWindowTheme(hwnd, app, idlist);
}
}
}
return S_FALSE;
}
}

/*
void CListCtrlEx::SetSortArrow(int colIndex, bool ascending)
{
if (IsThemeEnabled())
{
#if (_WIN32_WINNT >= 0x501)
for(int i = 0; i < GetHeaderCtrl()->GetItemCount(); ++i)
{
HDITEM hditem = {0};
hditem.mask = HDI_FORMAT;
VERIFY( GetHeaderCtrl()->GetItem( i, &hditem ) );
hditem.fmt &= ~(HDF_SORTDOWN|HDF_SORTUP);
if (i == colIndex)
{
hditem.fmt |= ascending ? HDF_SORTDOWN : HDF_SORTUP;
}
VERIFY( CListCtrl::GetHeaderCtrl()->SetItem( i, &hditem ) );
}
#endif
}
else
{
UINT bitmapID = m_Ascending ? IDB_DOWNARROW : IDB_UPARROW; 
for(int i = 0; i < GetHeaderCtrl()->GetItemCount(); ++i)
{
HDITEM hditem = {0};
hditem.mask = HDI_BITMAP | HDI_FORMAT;
VERIFY( GetHeaderCtrl()->GetItem( i, &hditem ) );
if (hditem.fmt & HDF_BITMAP && hditem.fmt & HDF_BITMAP_ON_RIGHT)
{
if (hditem.hbm)
{
DeleteObject(hditem.hbm);
hditem.hbm = NULL;
}
hditem.fmt &= ~(HDF_BITMAP|HDF_BITMAP_ON_RIGHT);
VERIFY( CListCtrl::GetHeaderCtrl()->SetItem( i, &hditem ) );
}
if (i == colIndex)
{
hditem.fmt |= HDF_BITMAP|HDF_BITMAP_ON_RIGHT;
hditem.hbm = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(bitmapID), IMAGE_BITMAP, 0,0, LR_LOADMAP3DCOLORS); 
VERIFY( hditem.hbm!=NULL );
VERIFY( CListCtrl::GetHeaderCtrl()->SetItem( i, &hditem ) );
}
}
}
}
*/


void CListCtrlEx::reconstruct_font()
{
	m_font.DeleteObject();
	BOOL bCreated = m_font.CreateFontIndirect(&m_lf);

	m_font_size = -MulDiv(m_lf.lfHeight, 72, GetDeviceCaps(::GetDC(GetParent()->GetSafeHwnd()), LOGPIXELSY));

	SetFont( &m_font, true );

	ASSERT(bCreated);
}

void CListCtrlEx::SetFontSize( int font_size )
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

void CListCtrlEx::EnlargeFontSize(bool enlarge)
{
	enlarge ? m_lf.lfHeight++ : m_lf.lfHeight--;
	reconstruct_font();
}

void CListCtrlEx::SetFontName(LPCTSTR sFontname, BYTE byCharSet )
{
	m_lf.lfCharSet = byCharSet;
	_tcscpy( m_lf.lfFaceName, sFontname);
	reconstruct_font();
}

void CListCtrlEx::SetFontBold( bool bBold )
{
	m_lf.lfWeight = ( bBold ? FW_BOLD : FW_NORMAL );
	reconstruct_font();
}

void CListCtrlEx::SetLineHeight(int height)
{
	CImageList gapImage;

	gapImage.Create(1, height, ILC_COLORDDB, 1, 0); //2번째 파라미터로 높이조절.....

	SetImageList(&gapImage,LVSIL_SMALL);
}

void CListCtrlEx::PreSubclassWindow()
{
	CListCtrl::PreSubclassWindow();


	CFont* font = GetParent()->GetFont();

	if ( font != NULL )
		font->GetObject(sizeof(m_lf),&m_lf);
	else
		GetObject(GetStockObject(SYSTEM_FONT),sizeof(m_lf),&m_lf);

	reconstruct_font();


	// if view style is other than LVS_REPORT 
	// returned pointer will be NULL
	CHeaderCtrl* pHeader = GetHeaderCtrl();

	// get original view style
	DWORD dwStyle = GetStyle() & LVS_TYPEMASK;

	// modify to force header creation
	// now pointer is not NULL
	// and handle is valid
	ModifyStyle(LVS_TYPEMASK, LVS_REPORT);
	pHeader = GetHeaderCtrl();

	// reset a style to original
	// check a pointer ... still there
	ModifyStyle(LVS_TYPEMASK, dwStyle);
	pHeader = GetHeaderCtrl();

	// voila!
	m_HeaderCtrlEx.SubclassWindow(pHeader->m_hWnd);

	ASSERT(pHeader->m_hWnd != NULL);

	//ListCtrlEx가 invisible일 경퓖EGunTracer와 같은 특정 프로젝트에서 아래 GetHeaderCtrl()의 리턴값이 
	//null로 리턴되는 현상이 있다. Test_ListCtrlExDlg에서 테스트하니 정상적인 값이 리턴된다.
	//두 ListCtrl의 속성은 동일한데 왜 이러한 현상이 발생하는햨E모르겠다.
	//HWND hWnd = ::GetWindow(m_hWnd, GW_CHILD);
	//if (hWnd)
	//	m_HeaderCtrlEx.SubclassWindow( hWnd );

}

void CListCtrlEx::SetColorTheme( int nTheme )
{
	switch ( nTheme )
	{
	case color_theme_default :
		m_crBack				= ::GetSysColor( COLOR_WINDOW );
		m_crBackSelected		= ::GetSysColor( COLOR_HIGHLIGHT );
		m_crBackSelectedInactive= ::GetSysColor( COLOR_HIGHLIGHT );
		m_crText				= ::GetSysColor( COLOR_BTNTEXT );
		m_crTextSelected		= ::GetSysColor( COLOR_HIGHLIGHTTEXT );
		m_crTextSelectedInactive= ::GetSysColor( COLOR_INACTIVECAPTIONTEXT );
		m_crHeaderBack			= ::GetSysColor( COLOR_3DFACE );
		m_crHeaderText			= ::GetSysColor( COLOR_BTNTEXT );
		break;
	case color_theme_blue :
		m_crBack				= RGB( 193, 219, 252 );
		m_crBackSelected		= get_color( m_crBack, -32 );
		m_crBackSelectedInactive= RGB( 193, 219, 252 );
		m_crText				= ::GetSysColor( COLOR_BTNTEXT );
		m_crTextSelected		= RGB(  65, 102, 146 );
		m_crTextSelectedInactive= RGB(  65, 102, 146 );
		m_crHeaderBack			= get_color( m_crBack, -32 );
		m_crHeaderText			= get_color( m_crText, -32 );
		break;
	case color_theme_dark_blue :
		m_crBack				= RGB(  74,  94, 127 );
		m_crBackSelected		= RGB(  15,  36,  41 );
		m_crBackSelectedInactive= RGB(  15,  36,  41 );
		m_crText				= RGB( 204, 216, 225 );
		m_crTextSelected		= RGB( 234, 246, 255 );
		m_crTextSelectedInactive= RGB( 105, 142, 186 );
		m_crHeaderBack			= get_color( m_crBack, -32 );
		m_crHeaderText			= get_color( m_crText, -32 );
		break; 
	case color_theme_dark_gray :
		m_crText				= RGB( 164, 164, 164);
		m_crTextSelected		= RGB( 241, 241, 241 );
		m_crTextSelectedInactive= RGB( 241, 241, 241 );
		m_crBack				= RGB(  64,  64,  64 );
		m_crBackSelected		= RGB(  32,  32,  32 );
		m_crBackSelectedInactive= RGB(  32,  32,  32 );
		m_crHeaderBack			= get_color( m_crBack, -16 );
		m_crHeaderText			= get_color( m_crText, -16 );
		break;
	}

	m_HeaderCtrlEx.SetColor( m_crHeaderText, m_crHeaderBack );
	Invalidate();
}


BOOL CListCtrlEx::OnEraseBkgnd(CDC* pDC)
{
	// TODO: Add your message handler code here and/or call default
	return false;
	return CListCtrl::OnEraseBkgnd(pDC);
}

//for state changed event catch
//아이템이 추가되거나 선택이 변경되거나 체크가 변경될때마다 항퍊E발생하므로
//해큱E메시지를 구분해서 parent로 알려주컖E해큱E메시지에 큱E漫??처리해야만
//불필요한 메시햨E처리를 줄일 펯E있다.
BOOL CListCtrlEx::OnLvnItemchanged(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
	bool bTrace = false;

	if ( pNMLV->uChanged == LVIF_TEXT )
		return true;

#if 0
	if ( bTrace )
		TRACE( "old = %d, new = %d, uChanged = %d\n", pNMLV->uOldState, pNMLV->uNewState, pNMLV->uChanged );

	if (pNMLV->uOldState == pNMLV->uNewState)
		return true;	// No change

	if ( pNMLV->uOldState & LVIS_FOCUSED )
	{
		if ( bTrace )
			TRACE( "%d : old item unfocused\n", GetTickCount() );
		return true;
	}
	else if ( pNMLV->uOldState & LVIS_SELECTED )
	{
		if ( bTrace )
			TRACE( "%d : old item unselect\n", GetTickCount() );
		return true;
	}
	else if ( /*!(pNMLV->uChanged == LVIF_STATE && */pNMLV->uNewState & LVIS_SELECTED )// | LVIS_FOCUSED))
	//if ( (pNMLV->uChanged & LVIF_STATE) && (pNMLV->uNewState & LVIS_SELECTED ) )
	//if ( /*pNMLV->uOldState == (!LVIS_SELECTED | LVIS_FOCUSED) && */pNMLV->uNewState == (LVIS_SELECTED | LVIS_FOCUSED) )
	{
		if ( bTrace )
			TRACE( "%d : uchanged = %d, oldstate = %d, newstate = %d, lparam = %d\n",
					GetTickCount(), pNMLV->uChanged, pNMLV->uOldState, pNMLV->uNewState, pNMLV->lParam );
		CListItemState state( pNMLV->iItem, pNMLV->iSubItem, ITEM_SELECTED );
		::SendMessage( GetParent()->GetSafeHwnd(), MESSAGE_LISTCTRLEX_ITEM_CHANGED, GetDlgCtrlID(), (LPARAM)&state );
		return true;
	}
	else
#endif
	{
		DWORD dwExStyle = GetExtendedStyle();
		if ( (dwExStyle & LVS_EX_CHECKBOXES) == false )
			return true;
		
		BOOL bPrevState = (BOOL)(((pNMLV->uOldState & 
			LVIS_STATEIMAGEMASK)>>12)-1);   // Old check box state
		if (bPrevState < 0)	// On startup there's no previous state 
			bPrevState = 0; // so assign as false (unchecked)

		// New check box state
		BOOL bChecked=(BOOL)(((pNMLV->uNewState & LVIS_STATEIMAGEMASK)>>12)-1);   
		if (bChecked < 0) // On non-checkbox notifications assume false
			bChecked = 0;

		if (bPrevState == bChecked) // No change in check box
			return true;
		
		//TRACE( "%d : check changed. %d -> %d\n", GetTickCount(), bPrevState, bChecked );
		
		CListItemState state( pNMLV->iItem, pNMLV->iSubItem, (bChecked ? ITEM_CHECKED : ITEM_UNCHECKED) );
		::SendMessage( GetParent()->GetSafeHwnd(), MESSAGE_LISTCTRLEX_ITEM_CHANGED, GetDlgCtrlID(), (LPARAM)&state );
	}

	return true;//FALSE;
}

int CListCtrlEx::GetCheckedItemCount()
{
	int nCheckedCount = 0;

	for ( int i = 0; i < GetItemCount(); i++ )
	{
		if ( GetCheck(i) )
			nCheckedCount++;
	}

	return nCheckedCount;
}

void CListCtrlEx::CheckAll( bool bCheck )
{
	m_bCheckAll = bCheck;

	for ( int i = 0; i < GetItemCount(); i++ )
		SetCheck( i, bCheck );
}


void CListCtrlEx::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp)
{
	// TODO: Add your message handler code here and/or call default
	//UpdateWindow();

	CListCtrl::OnNcCalcSize(bCalcValidRects, lpncsp);
}

bool CListCtrlEx::IsRowHighlighted(int row)
{
	bool selected = GetItemState(row, LVIS_SELECTED) != 0;
	return selected;
}

void CListCtrlEx::EnableHighlighting(int row, bool enable)
{
	//SetItemState(row, enable ? 0xff : 0, LVIS_SELECTED);
	SetItemState(row, enable ? LVIS_SELECTED: 0, LVIS_SELECTED);
	/*
	m_internalStateChange = true;
	SetItemState(row, enable ? 0xff : 0, LVIS_SELECTED);
	m_internalStateChange = false;
	*/

}

#if USE_CUSTOMDRAW
#if 0
void CListCtrlEx::OnNMCustomdraw(NMHDR* pNMHDR, LRESULT* pResult)
{
	bool callParent = true;
	static bool bHighlighted = false;
	LPNMLVCUSTOMDRAW lpLVCustomDraw = reinterpret_cast<LPNMLVCUSTOMDRAW>(pNMHDR);
	NMCUSTOMDRAW nmcd = lpLVCustomDraw->nmcd;

	*pResult = CDRF_DODEFAULT;

	switch (lpLVCustomDraw->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		*pResult = CDRF_NOTIFYITEMDRAW;
		break;
	case CDDS_ITEMPREPAINT:
	{
		int row = nmcd.dwItemSpec;
		bHighlighted = IsRowHighlighted(row);
		if (bHighlighted)
		{
			lpLVCustomDraw->clrText = m_crTextSelected;
			lpLVCustomDraw->clrTextBk = m_crBackSelected;

			EnableHighlighting(row, false);
		}
		else
		{
			lpLVCustomDraw->clrText = m_crText;
			lpLVCustomDraw->clrTextBk = m_crBack;
		}

		*pResult = CDRF_NOTIFYSUBITEMDRAW;////CDRF_DODEFAULT | CDRF_NOTIFYPOSTPAINT;
		callParent = false;
	}
	break;
	case (CDDS_SUBITEM | CDDS_ITEMPREPAINT) ://CDDS_ITEMPOSTPAINT:
		if (bHighlighted)
		{
			int row = nmcd.dwItemSpec;
			EnableHighlighting(row, true);
			callParent = false;
		}
		else if (lpLVCustomDraw->iSubItem == 0)
		{
			lpLVCustomDraw->clrText = RGB(255,0,0);
			lpLVCustomDraw->clrTextBk = m_crBack;
		}

		*pResult = CDRF_DODEFAULT;

		break;
	default:
		break;
	}

	if (callParent)
	{
		//__super ::OnCustomDraw(pNMHDR, pResult);
		//CListCtrl::OnCustomdraw(pNMHDR, pResult);
	}
}
#endif
#if 0
void CListCtrlEx::OnNMCustomdraw(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLVCUSTOMDRAW lpLVCustomDraw = reinterpret_cast<LPNMLVCUSTOMDRAW>(pNMHDR);
	NMLVCUSTOMDRAW *pDraw = (NMLVCUSTOMDRAW*)(pNMHDR); 
	switch (lpLVCustomDraw->nmcd.dwDrawStage)
	{
	case CDDS_ITEMPREPAINT:
		/*
		//if()
		if (lpLVCustomDraw->nmcd.uItemState & CDIS_SELECTED)
		{
			// Your color definitions here:
			lpLVCustomDraw->clrText = RGB(255, 0, 0);
			lpLVCustomDraw->clrTextBk = RGB(255, 128, 255);
			lpLVCustomDraw->nmcd.uItemState &= ~CDIS_SELECTED;
		}
		*/
		/*
		else//if (lpLVCustomDraw->nmcd.uItemState & CDIS_SELECTED)
		{
			// Your color definitions here:
			lpLVCustomDraw->clrText = RGB(0, 255, 0);
			if(pDraw->iSubItem == 0)
				lpLVCustomDraw->clrTextBk = RGB(0, 70, 70);
			else
				lpLVCustomDraw->clrTextBk = RGB(0, 70, 255);
		}
		*/
		//*pResult = (LRESULT)CDRF_NOTIFYSUBITEMDRAW;
		//return;
		//break;
	case CDDS_SUBITEM:
		if(false)//lpLVCustomDraw->nmcd.uItemState & CDIS_SELECTED)
		{
			// Your color definitions here:

			lpLVCustomDraw->clrText = RGB(255, 0, 255);
			lpLVCustomDraw->clrTextBk = RGB(0, 70, 60);
		}
		
		if (lpLVCustomDraw->iSubItem == 0)
			lpLVCustomDraw->clrTextBk = RGB(255, 255, 0);
		//else
			//lpLVCustomDraw->clrTextBk = RGB(0, 255, 255);
			
		break;
	//case CDDS_POSTPAINT:
		//break;

	default:
		break;
	}

	*pResult = 0;
	*pResult |= CDRF_NOTIFYPOSTPAINT;
	*pResult |= CDRF_NOTIFYITEMDRAW;
	*pResult |= CDRF_NOTIFYSUBITEMDRAW;
}
#endif

#if 1
void CListCtrlEx::OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult)
{
	//TRACE( "%d, %s\n", GetTickCount(), __FUNCTION__ );
	//LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	//// TODO: Add your control notification handler code here
	//*pResult = 0;
	//return;
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>( pNMHDR );
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	static bool bHighlighted = false;

	//*pResult = CDRF_DODEFAULT;

	/*
	if ( CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage )
	{
		*pResult = CDRF_NOTIFYITEMDRAW;
	}
	*/
	if(pNMCD->dwDrawStage == CDDS_PREPAINT)
	{
		*pResult = (LRESULT)CDRF_NOTIFYITEMDRAW; 
		//return; // 여기서 함수를 빠져 나가야 *pResult 값이 유지된다.
	}
	else if ( CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage )
	{
		/*
		int iRow = (int)pLVCD->nmcd.dwItemSpec;
		bHighlighted = ListView_GetItemState(m_hWnd, iRow, LVIS_SELECTED);
	
		if (bHighlighted)
		{
			if ( IsWindowEnabled() == false )
			{
				pLVCD->clrText   = ::GetSysColor( COLOR_GRAYTEXT );
				pLVCD->clrTextBk = GRAY(164);
			}
			else
			{
				pLVCD->clrText   = m_crTextSelected;
				pLVCD->clrTextBk = m_crBackSelected;
			}

			ListView_SetItemState( m_hWnd, iRow, 0, LVIS_SELECTED);
		}
		else
		{
			if ( IsWindowEnabled() == false )
			{
				pLVCD->clrText   = ::GetSysColor( COLOR_GRAYTEXT );
				pLVCD->clrTextBk = GRAY(192);
			}
			else
			{
				//NMLVCUSTOMDRAW *pDraw = (NMLVCUSTOMDRAW*)(pNMHDR); 
				//if (pDraw->iSubItem == 0)
				//	pLVCD->clrText   = RGB(255,0,0);
				//else
					pLVCD->clrText   = m_crText;
				pLVCD->clrTextBk = m_crBack;
			}
		}
		*/
		/*
		// 한 줄 (row) 가 그려질 때. 여기서만 설정하면 한 줄이 모두 동일하게 설정이 된다.
		if(GetItemData(pNMCD->dwItemSpec) == 0){//dwItemSpec 이 현재 그려지는 row index
			NMLVCUSTOMDRAW *pDraw = (NMLVCUSTOMDRAW*)(pNMHDR); 
			pDraw->clrText = 0xffffff; 
			pDraw->clrTextBk = 0x0; 
			//*pResult = (LRESULT)CDRF_NEWFONT;//여기서 나가면 sub-item 이 변경되지 않는다.
			*pResult = (LRESULT)CDRF_NOTIFYSUBITEMDRAW;//sub-item 을 변경하기 위해서. 
			return;//여기서 중단해야 *pResult 값이 유지된다.
		}
		else{ // When all matrices are already made. 
			NMLVCUSTOMDRAW *pDraw = (NMLVCUSTOMDRAW*)(pNMHDR); 
			pDraw->clrText = 0x0; 
			pDraw->clrTextBk = RGB(255,255,196);  
			*pResult = (LRESULT)CDRF_NEWFONT; 
			return; 
		}
		*/

		//*pResult = /*CDRF_DODEFAULT | */CDRF_NOTIFYPOSTPAINT;
		*pResult = (LRESULT)CDRF_NOTIFYSUBITEMDRAW;//sub-item 을 변경하기 위해서. 
		//return;//여기서 중단해야 *pResult 값이 유지된다.
	}
	else if(pLVCD->nmcd.dwDrawStage == (CDDS_SUBITEM | CDDS_ITEMPREPAINT))
	{
		NMLVCUSTOMDRAW *pDraw = (NMLVCUSTOMDRAW*)(pNMHDR); 
		bHighlighted = ListView_GetItemState(m_hWnd, pLVCD->nmcd.dwItemSpec, LVIS_SELECTED);

		CListCtrlExItemColorDeque *dq = (CListCtrlExItemColorDeque*)GetItemData(pLVCD->nmcd.dwItemSpec);

		if ((dq != NULL) && 
			(dq->dqColor.size() > pDraw->iSubItem))
		{
			if (dq->dqColor[pDraw->iSubItem].text != LISTCTRLEX_UNUSED_COLOR)
				pDraw->clrText = dq->dqColor[pDraw->iSubItem].text; 
			else
				pDraw->clrText = m_crText; 
			if (dq->dqColor[pDraw->iSubItem].back != LISTCTRLEX_UNUSED_COLOR)
				pDraw->clrTextBk = dq->dqColor[pDraw->iSubItem].back; 
			else
				pDraw->clrTextBk = m_crBack; 
		}
		else
		{
			pDraw->clrText = m_crText; 
			pDraw->clrTextBk = m_crBack; 
		}
		
		/*
		int iRow = (int)pLVCD->nmcd.dwItemSpec;
		bHighlighted = ListView_GetItemState(m_hWnd, iRow, LVIS_SELECTED);

		if (bHighlighted)
		{
			if ( IsWindowEnabled() == false )
			{
				pDraw->clrText   = ::GetSysColor( COLOR_GRAYTEXT );
				pDraw->clrTextBk = GRAY(164);
			}
			else
			{
				pDraw->clrText   = m_crTextSelected;
				pDraw->clrTextBk = m_crBackSelected;
			}

			ListView_SetItemState( m_hWnd, iRow, 0, LVIS_SELECTED);
		}
		else
		{
			if ( IsWindowEnabled() == false )
			{
				pDraw->clrText   = ::GetSysColor( COLOR_GRAYTEXT );
				pDraw->clrTextBk = GRAY(192);
			}
			else
			{
				//NMLVCUSTOMDRAW *pDraw = (NMLVCUSTOMDRAW*)(pNMHDR); 
				//if (pDraw->iSubItem == 0)
				//	pLVCD->clrText   = RGB(255,0,0);
				//else
				pDraw->clrText   = m_crText;
				pDraw->clrTextBk = m_crBack;
			}
		}
		*/
		*pResult = (LRESULT)CDRF_NEWFONT; // 이렇게 해야 설정한 대로 그려진다.
		return;
		//*pResult = CDRF_NOTIFYPOSTPAINT;
	}
	
	else if(CDDS_ITEMPOSTPAINT == pLVCD->nmcd.dwDrawStage)
	{
		if (bHighlighted)
		{
			int  iRow = (int)pLVCD->nmcd.dwItemSpec;

			ListView_SetItemState( m_hWnd, iRow, 0xff, LVIS_SELECTED);
		}

		*pResult = CDRF_DODEFAULT;
	}
}
#endif
#endif

void CListCtrlEx::ModifyExtendedStyle( DWORD dwExStyle, bool bAdd )
{
	DWORD dwExistStyle = ListView_GetExtendedListViewStyle( GetSafeHwnd() );
	if ( bAdd )
		ListView_SetExtendedListViewStyle( GetSafeHwnd(), dwExistStyle | dwExStyle );
	else
		(DWORD)SNDMSG((GetSafeHwnd()), LVM_SETEXTENDEDLISTVIEWSTYLE, dwExStyle, 0);
}


void CListCtrlEx::OnNMClick(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: Add your control notification handler code here

	//TRACE( "%d : %s\n", GetTickCount(), __FUNCTION__ );
	CListItemState state( pNMItemActivate->iItem, pNMItemActivate->iSubItem, ITEM_SELECTED );
	::SendMessage( GetParent()->GetSafeHwnd(), MESSAGE_LISTCTRLEX_ITEM_CHANGED, GetDlgCtrlID(), (LPARAM)&state );

	*pResult = 0;
}


void CListCtrlEx::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	// TODO: Add your message handler code here and/or call default

	CListCtrl::OnMeasureItem(nIDCtl, lpMeasureItemStruct);
}

//어떤 항목이 특정 위치에 표시되도록 스크롤시킨다.
//mode가 visible_first이고 offset이 3이면 위에서 3+1인 위치에 해당 아이템이 표시되도록 스크롤시킨다.
//mode가 visible_center이고 offset이 0이면 중앙 위치에 해당 아이템이 표시되도록 스크롤시킨다.
//mode가 visible_last이고 offset이 3이면 아래에서 -3-1인 위치에 해당 아이템이 표시되도록 스크롤시킨다.
void CListCtrlEx::ensure_visible(int index, int mode, int offset)
{
	int scroll_pos;
	int items_per_page = GetCountPerPage();

	CRect r;
	GetItemRect(0, r, LVIR_BOUNDS);

	if (mode == visible_first)
	{
		Scroll(CSize(0, (index - GetTopIndex()) * r.Height()));
	}
	else if (mode == visible_center)
	{
		scroll_pos = items_per_page/2;
	}
}
