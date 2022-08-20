#include "stdafx.h"
//#include "listPrint.h"

#include "helper.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

int g_nTotalPrintedPageNumber = 0;
///////////////////////////////////////////////////
//
// SortListCtrl
//
// Sort the list based on column text
//
///////////////////////////////////////////////////
//
// Returns		- Returns true for success
// nCol			- column that contains the text to be sorted
// bAscending		- indicate sort order
// low			- row to start scanning from - default row is 0
// high			- row to end scan. -1 indicates last row
//
// taken from www.codeguru.com
// based upon an article "Sorting the list based on text in any column"
// author is unknown
//
BOOL SortListCtrl(CListCtrl *pCtrl, int nCol, BOOL bAscending, int low, int high)
{
	CWaitCursor			waiting;

	if( nCol >= ((CHeaderCtrl*)pCtrl->GetDlgItem(0))->GetItemCount() )
		return FALSE;

	if( high == -1 ) high = pCtrl->GetItemCount() - 1;

	int lo = low;
	int hi = high;
	CString midItem;

	if( hi <= lo ) return FALSE;

	midItem = pCtrl->GetItemText( (lo+hi)/2, nCol );

	// loop through the list until indices cross
	while( lo <= hi )
	{
		// rowText will hold all column text for one row
		CStringArray rowText;

		// find the first element that is greater than or equal to?
		// the partition element starting from the left Index.
		if( bAscending )
			while( ( lo < high ) && ( pCtrl->GetItemText(lo, nCol) < midItem ) )
				++lo;
		else
			while( ( lo < high ) && ( pCtrl->GetItemText(lo, nCol) > midItem ) )
				++lo;

		// find an element that is smaller than or equal to?
		// the partition element starting from the right Index.
		if( bAscending )
			while( ( hi > low ) && ( pCtrl->GetItemText(hi, nCol) > midItem ) )
				--hi;
		else
			while( ( hi > low ) && ( pCtrl->GetItemText(hi, nCol) < midItem ) )
				--hi;

		// if the indexes have not crossed, swap
		// and if the items are not equal
		if( lo <= hi )
		{
			// swap only if the items are not equal
			if( pCtrl->GetItemText(lo, nCol) != pCtrl->GetItemText(hi, nCol))
			{
				// swap the rows
				LV_ITEM lvitemlo, lvitemhi;
				int nColCount = ((CHeaderCtrl*) pCtrl->GetDlgItem(0))->GetItemCount();
				rowText.SetSize( nColCount );
				int i;
				for( i=0; i<nColCount; i++)
					rowText[i] = pCtrl->GetItemText(lo, i);
				lvitemlo.mask = LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
				lvitemlo.iItem = lo;
				lvitemlo.iSubItem = 0;
				lvitemlo.stateMask = LVIS_CUT | LVIS_DROPHILITED | LVIS_FOCUSED | 
									LVIS_SELECTED | LVIS_OVERLAYMASK | LVIS_STATEIMAGEMASK;

				lvitemhi = lvitemlo;
				lvitemhi.iItem = hi;

				pCtrl->GetItem( &lvitemlo );
				pCtrl->GetItem( &lvitemhi );

				for( i=0; i<nColCount; i++)
					pCtrl->SetItemText(lo, i, pCtrl->GetItemText(hi, i));

				lvitemhi.iItem = lo;
				pCtrl->SetItem( &lvitemhi );

				for( i=0; i<nColCount; i++)
					pCtrl->SetItemText(hi, i, rowText[i]);

				lvitemlo.iItem = hi;
				pCtrl->SetItem( &lvitemlo );
			}

			++lo;
			--hi;
		}
	}

	// If the right index has not reached the left side of array
	// must now sort the left partition.
	if( low < hi )
		SortListCtrl(pCtrl, nCol, bAscending , low, hi);

	// If the left index has not reached the right side of array
	// must now sort the right partition.
	if( lo < high )
		SortListCtrl(pCtrl, nCol, bAscending , lo, high );

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CListCtrlPrintJob
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CListCtrlPrintJob::CListCtrlPrintJob(CListCtrl *pCtrl, BOOL bPrintToDefaultPrinter)
{
	LV_COLUMN	col;
	
	col.mask			= LVCF_WIDTH;
	m_pListCtrl			= pCtrl;
	m_ColCount			= 0;
	m_bPrintToDefault	= bPrintToDefaultPrinter;
	g_nTotalPrintedPageNumber = 0;

	if( m_pListCtrl )
	{

		while( m_pListCtrl->GetColumn(m_ColCount, &col) )
			m_ColCount++;

	}	
}

CListCtrlPrintJob::~CListCtrlPrintJob()
{
}

CDialog *CListCtrlPrintJob::CreatePrintDialog()
{
	if( m_bPrintToDefault )
	{
		UseDefaults();
		return NULL;
	}
	else
		return GPrintJob::CreatePrintDialog();
}

void CListCtrlPrintJob::OnPrint()
{
	int			i,
				iStart,
				widthOfPage,		// Pixel
				widthThisCol;		// Pixel
	BOOL		bNewPage;

	if( !m_pListCtrl )		return;

	CClientDC	dc(m_pListCtrl);
	double		sizePixel			= dc.GetDeviceCaps(HORZRES),
				sizeMM				= dc.GetDeviceCaps(HORZSIZE),
				pixelsPerCM			= 10.0 * sizePixel / sizeMM,
				paperWidthInCM		= m_rectClient.Width()/100.0,
				paperWidthInPixel	= paperWidthInCM * pixelsPerCM / 1.5;

	//
	// a Header Page ?
	//
	//	CListCtrlHeaderPage		header(this, m_pView);
	//	header.Print();
	//

	// look how many columns fit to the page and print them
	// the first column is printed on every page
	widthOfPage = m_pListCtrl->GetColumnWidth(0);
	iStart		= 1;

	for( i=1 ; i<m_ColCount ; i++ )
	{
		widthThisCol = m_pListCtrl->GetColumnWidth(i);
		bNewPage = FALSE;

		if( widthOfPage+widthThisCol > paperWidthInPixel )
		{	
			// not enough place on side, begin new one
			bNewPage = TRUE;
		}
		else
		{
			// also print the last one
			if( i == (m_ColCount-1) )
				bNewPage = TRUE;
			
			// this col needs this space:
			widthOfPage += widthThisCol;
		}

		if( bNewPage )
		{
			// ok, print it on a new page
			CListCtrlDataPage	data(this, m_pListCtrl, iStart, i);
			data.SetHeadAndFootString( m_sHead, m_sLeftFoot, m_sFoot, m_sRightFoot );
			data.Print();

			iStart = i+1;
			widthOfPage = m_pListCtrl->GetColumnWidth(0);
		}
	}
}

int CListCtrlPrintJob::GetTotalPrintedPageNumber()
{
	return g_nTotalPrintedPageNumber;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CListCtrlDataPage
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CListCtrlDataPage::CListCtrlDataPage(GPrintJob *pJob, CListCtrl *pCtrl, int firstCol, int lastCol)
	:GPrintUnit(pJob)
{
	m_pListCtrl		= pCtrl;
	m_firstCol		= firstCol;
	m_lastCol		= lastCol;

	m_sHead		= "Product name in Header";	//Top center
	m_sLeftFoot	= "CopyRight in Footer";	//Bottom left
	m_sFoot		= "Footer";					//Bottom center
	m_sRightFoot= "UserName in Footer";		//Bottom right
}

CListCtrlDataPage::~CListCtrlDataPage()
{
}

void CListCtrlDataPage::DefineColHeadings()
{
	int			colId		= 0,
				i;
	double		widthOfPage;
	LV_COLUMN	col;
	char		szText[200];  
	
	if( !m_pListCtrl )		return;

	col.mask		= LVCF_TEXT; 
	col.pszText		= szText; 
	col.cchTextMax	= sizeof(szText);

	// this is the spacing of all columns to print
	widthOfPage = m_pListCtrl->GetColumnWidth(0);
	for( i= m_firstCol ; i<=m_lastCol ; i++ )
		widthOfPage += m_pListCtrl->GetColumnWidth(i);

	// define my columns...percentages should all up to 1.00
	m_pListCtrl->GetColumn(0, &col);
	InsertPrintCol(colId++, szText, ((double)m_pListCtrl->GetColumnWidth(0))/widthOfPage);

	for( i= m_firstCol ; i<=m_lastCol ; i++ )
	{
		m_pListCtrl->GetColumn(i, &col);
		InsertPrintCol(colId++, szText, ((double)m_pListCtrl->GetColumnWidth(i))/widthOfPage);
	}

	// must call base class
	GPrintUnit::DefineColHeadings();
}

BOOL CListCtrlDataPage::Print()
{
	char		szText[200];
	int			zeile,
				spalte,
				zeilenMax,
				colId,
				nRC;

	GPrintUnit::Print();

	if( !m_pListCtrl )	
		return FALSE;

	StartPage();

/*	{
		GSELECT_OBJECT(&JDC, &m_fontHeading);
		PrintColHeadings(DT_LEFT);
	}
*/	GSELECT_PUFONT(&JDC, &m_fontPairBody);

	zeilenMax = m_pListCtrl->GetItemCount();

	LVCOLUMN	lvColumn;
	int			nAlign;

	for( zeile=0 ; zeile<zeilenMax ; zeile++ )
	{
		nRC = StartRow();
/*		if( nRC == SR_ADVANCEDPAGE )
		{
			{
				GSELECT_OBJECT(&JDC, &m_fontHeading);
				PrintColHeadings(DT_LEFT);
			}
			GSELECT_PUFONT(&JDC, &m_fontPairBody);
		}
*/		
		colId = 0;

		// erste Spalte
		if( m_pListCtrl->GetItemText(zeile, 0, szText, sizeof(szText)) )
			PrintCol(colId, szText, DT_LEFT);
		colId++;

		for( spalte=m_firstCol ; spalte<=m_lastCol ; spalte++ )
		{
			lvColumn.mask = LVCF_FMT;
			m_pListCtrl->GetColumn( spalte, &lvColumn );

			if ( lvColumn.fmt == LVCFMT_LEFT )
				nAlign = DT_LEFT;
			else if ( lvColumn.fmt == LVCFMT_CENTER )
				nAlign = DT_CENTER;
			if ( lvColumn.fmt == LVCFMT_RIGHT )
				nAlign = DT_RIGHT;

			if( m_pListCtrl->GetItemText(zeile, spalte, szText, sizeof(szText)) )
				PrintCol(colId, szText, nAlign);
			colId++;
		}

		EndRow();
	}

	EndPage();

	return TRUE;
}

void CListCtrlDataPage::StartPage()
{
	// call base class first
	GPrintUnit::StartPage(); 
	// this guarantees headings will be printed at
	// the top of every page
	{
		GSELECT_OBJECT(&JDC, &m_fontHeading);
		PrintColHeadings(DT_LEFT);
	}
	GSELECT_PUFONT(&JDC, &m_fontPairBody);
}

void CListCtrlDataPage::PrintColHeadings(UINT nFormat, UINT nEffects)
{
	// call base class first
	GPrintUnit::PrintColHeadings(nFormat, nEffects);
	// a line from right to left
	JDC.MoveTo(JRECT.left, JCUR.y);
	JDC.LineTo(JRECT.right, JCUR.y);
//	JDC.FillSolidRect( JRECT.left, JCUR.y, JRECT.Width(), 10, RGB(255,0,0) );
	// be sure to adjust the cursor location
	JCUR.y += 10;
}

void CListCtrlDataPage::PrintHeader()
{
	GSELECT_OBJECT(&JDC, &m_fontHeader);

	CString strDateTime;
	// get the current time
	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);
	// format it...
	TCHAR szBuf[100];

	GMAKENUL(szBuf);
	// get the time
//	GetDateFormat(LOCALE_USER_DEFAULT, NULL, &sysTime, NULL, szBuf, sizeof(szBuf));
	sprintf( szBuf, "%d-%02d-%02d", sysTime.wYear, sysTime.wMonth, sysTime.wDay );
	strDateTime = szBuf;
	strDateTime += "   ";
	
	// get the date
	GMAKENUL(szBuf);
//	GetTimeFormat(LOCALE_USER_DEFAULT, NULL, &sysTime, NULL, szBuf, sizeof(szBuf));
	sprintf( szBuf, "%02d:%02d:%02d", sysTime.wHour, sysTime.wMinute, sysTime.wSecond );
	strDateTime += szBuf;

	CString strHeader = strDateTime,
			strProduct;

	strProduct = m_sHead;

	strHeader += HFC_CENTER;
	strHeader += strProduct;
	strHeader += HFC_RIGHTJUSTIFY;

	CString strPage;
	strPage.Format( "Page %d", JINFO.m_nCurPage);
	g_nTotalPrintedPageNumber = JINFO.m_nCurPage;

	strHeader += strPage;

	PrintHeaderText(strHeader);
}


void CListCtrlDataPage::PrintFooter()
{
	GSELECT_OBJECT(&JDC, &m_fontFooter);

	CString strFooter,
			strCopyright,
			strManufactor,
			strUser;

	strCopyright = m_sLeftFoot;
	strManufactor = m_sFoot;
	strUser = m_sRightFoot;

	strFooter = strCopyright;
	strFooter += HFC_CENTER;
	strFooter += strManufactor;
	strFooter += HFC_RIGHTJUSTIFY;
	strFooter += strUser;

	PrintFooterText(strFooter);
}

void CListCtrlDataPage::CreatePrintFonts()
{
	LOGFONT logFont;
	GMAKESTNUL(logFont);

	LPCTSTR lpszFaceName = _T("±¼¸²");//I18nok

	logFont.lfCharSet = DEFAULT_CHARSET;
	logFont.lfHeight = 90;
	lstrcpyn(logFont.lfFaceName, lpszFaceName, GCOUNTOF(logFont.lfFaceName));
	logFont.lfWeight = FW_NORMAL;

	m_fontHeading.CreatePointFontIndirect(&logFont, &JDC);
	m_fontPairBody.fontPrinter.CreatePointFont(90, lpszFaceName, &JDC);
	m_fontPairBody.fontScreen.CreatePointFont(90, lpszFaceName);

	m_fontHeader.CreatePointFont(110, _T("Garamond"), &JDC);//I18nOK
	m_fontFooter.CreatePointFont(90, _T("Garamond"), &JDC);//I18nOK
}


void CListCtrlDataPage::InitPrintMetrics()
{
	TEXTMETRIC tm;

	{
		GSELECT_OBJECT(&JDC, &m_fontHeader);
		JDC.GetTextMetrics(&tm);

		m_pum.pumHeaderHeight = tm.tmHeight * 2;
		m_pum.pumHeaderLineHeight = tm.tmHeight;
	}
	{
		GSELECT_OBJECT(&JDC, &m_fontFooter);
		JDC.GetTextMetrics(&tm);

		m_pum.pumFooterHeight = tm.tmHeight * 2;
		m_pum.pumFooterLineHeight = tm.tmHeight;
	}
	{
		GSELECT_OBJECT(&JDC, &(m_fontPairBody.fontPrinter));
		JDC.GetTextMetrics(&tm);
		m_pum.pumLineOfText = tm.tmHeight;
	}
	{
		GSELECT_OBJECT(&JDC, &(m_fontHeading));
		JDC.GetTextMetrics(&tm);
		m_pum.pumHeadingHeight = m_pum.pumLineOfText;
	}

	RealizeMetrics();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CListCtrlHeaderPage
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CListCtrlHeaderPage::CListCtrlHeaderPage(GPrintJob *pJob)
	:GPrintUnit(pJob)
{
}

CListCtrlHeaderPage::~CListCtrlHeaderPage()
{
}

BOOL CListCtrlHeaderPage::Print()
{
	GPrintUnit::Print();

	StartPage();

	CFont font;
	font.CreatePointFont(480, "Arial", &JDC);
	GSELECT_OBJECT(&JDC, &font);

	CString str("The Header Page");

	CSize size = JDC.GetTextExtent(str);

	CRect rect;
	rect.left = (JRECT.Width() - size.cx)/2;
	rect.right = rect.left + size.cx;
	rect.top = (JRECT.Height() - size.cy)/2;
	rect.bottom = rect.top + size.cy;

	JDC.DrawText(str, &rect, DT_SINGLELINE);

	EndPage();

	return TRUE;
}
