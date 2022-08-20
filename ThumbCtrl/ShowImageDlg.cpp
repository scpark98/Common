// ShowImageDlg.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "ShowImageDlg.h"

#define	ZOOM_NORMAL	1.1
#define	ZOOM_DETAIL	1.02

// CShowImageDlg 대화 상자입니다.

IMPLEMENT_DYNAMIC(CShowImageDlg, CDialogEx)

CShowImageDlg::CShowImageDlg(CWnd* pParent /*=NULL*/)
	//: CDialogEx(CShowImageDlg::IDD, pParent)
{
	m_pImage = NULL;
	m_dZoom = 1.0;
	m_bDrag	= false;
	m_ptStart = CPoint( 0, 0 );
}

CShowImageDlg::~CShowImageDlg()
{
	if ( m_pImage )
	{
		delete [] m_pImage;
	}
}

void CShowImageDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CShowImageDlg, CDialogEx)
	ON_WM_DESTROY()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONUP()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()


// CShowImageDlg 메시지 처리기입니다.


BOOL CShowImageDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  여기에 추가 초기화 작업을 추가합니다.

	return TRUE;  // return TRUE unless you set the focus to a control
	// 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}


void CShowImageDlg::OnOK()
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

	CDialogEx::OnOK();
}


void CShowImageDlg::OnCancel()
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

	CDialogEx::OnCancel();
}


void CShowImageDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
}


BOOL CShowImageDlg::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	return TRUE;
	return CDialogEx::OnEraseBkgnd(pDC);
}


void CShowImageDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
// 	m_bDrag		= true;
// 	m_ptDrag	= point;
// 	SetCapture();

	CDialogEx::OnLButtonDown(nFlags, point);
}


void CShowImageDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
// 	m_bDrag		= false;
// 	ReleaseCapture();

	CDialogEx::OnLButtonUp(nFlags, point);
}


void CShowImageDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
// 	if ( m_bDrag )
// 	{
// 		m_ptStart = (point - m_ptDrag);
// 		m_ptStart.x 
// 		Invalidate();
// 	}

	CDialogEx::OnMouseMove(nFlags, point);
}

void CShowImageDlg::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
 	ShowWindow( SW_HIDE );
 	return;

//	SaveRawDataToBmp( "c:\\test.bmp", m_pImage, m_nWidth, m_nHeight, m_nChannel );

	CDialogEx::OnRButtonUp(nFlags, point);
}

void CShowImageDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	// 그리기 메시지에 대해서는 CDialogEx::OnPaint()을(를) 호출하지 마십시오.

	CMemDC	memDC( dc, this );
	CDC*	pDC = &memDC.GetDC();

	::SetStretchBltMode( pDC->GetSafeHdc(), COLORONCOLOR );

	CRect	r;
	GetClientRect( r );

	int dx	= (int)(( r.Width() - m_nWidth * m_dZoom ) / 2.0);
	int dy	= (int)(( r.Height() - m_nHeight * m_dZoom ) / 2.0);

	pDC->FillSolidRect( r, 0 );

	::StretchDIBits	( pDC->GetSafeHdc(), dx, dy, (int)(m_nWidth * m_dZoom), (int)(m_nHeight * m_dZoom),
		0, 0, m_nWidth, m_nHeight, m_pImage, &m_bmpInfo, DIB_RGB_COLORS, SRCCOPY );
}


void CShowImageDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	Invalidate();
}

void CShowImageDlg::SetImageData( BYTE* pImage, int w, int h, int ch /*= 3*/, CString sTitle /*= ""*/ )
{
	if ( m_pImage )
		delete [] m_pImage;

	int rwsize = DIB_WIDTHBYTES( w * ch * 8 );
	m_pImage = new BYTE[ rwsize * h ];
	memset( m_pImage, 0, sizeof(BYTE) * rwsize * h );
	memcpy( m_pImage, pImage, sizeof(BYTE) * w * h * ch );

	m_nWidth	= w;
	m_nHeight	= h;
	m_nChannel	= ch;

	BITMAPINFOHEADER* pheader = &m_bmpInfo.bmiHeader;

	// Initialize permanent data in the bitmapinfo header.
	pheader->biSize          = sizeof( BITMAPINFOHEADER );
	pheader->biPlanes        = 1;
	pheader->biCompression   = BI_RGB;
	pheader->biXPelsPerMeter = 100;
	pheader->biYPelsPerMeter = 100;
	pheader->biClrUsed       = 0;
	pheader->biClrImportant  = 0;

	// Set a default window size.
	pheader->biWidth    = m_nWidth;
	pheader->biHeight   = -m_nHeight;
	pheader->biBitCount = 8 * m_nChannel;

	pheader->biSizeImage = sizeof(BYTE) * rwsize * m_nHeight;

	CRect	r;

	if ( !IsZoomed() )
	{
		SetWindowPos(	&wndNoTopMost, 0, 0,
						w + GetSystemMetrics( SM_CXSIZEFRAME ) * 2,
						h + GetSystemMetrics( SM_CYSIZEFRAME ) * 2 + GetSystemMetrics( SM_CYSMCAPTION ),
						SWP_NOMOVE | SWP_NOZORDER );
	}
	
	SetWindowText( sTitle );
	Invalidate();
}


void CShowImageDlg::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	m_dZoom = 1.0;

	SetWindowPos(	&wndNoTopMost, 0, 0,
					m_nWidth + GetSystemMetrics( SM_CXSIZEFRAME ) * 2,
					m_nHeight + GetSystemMetrics( SM_CYSIZEFRAME ) * 2 + GetSystemMetrics( SM_CYCAPTION ),
					SWP_NOMOVE | SWP_NOZORDER );

	Invalidate();

	CDialogEx::OnLButtonDblClk(nFlags, point);
}


BOOL CShowImageDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if ( GetKeyState( VK_SHIFT ) & 0x8000 )
		( zDelta < 0 ) ? m_dZoom /= ZOOM_DETAIL : m_dZoom *= ZOOM_DETAIL;
	else
		( zDelta < 0 ) ? m_dZoom /= ZOOM_NORMAL : m_dZoom *= ZOOM_NORMAL;

	Invalidate();

	return CDialogEx::OnMouseWheel(nFlags, zDelta, pt);
}
