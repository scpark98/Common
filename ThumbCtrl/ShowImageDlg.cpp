// ShowImageDlg.cpp : ���� �����Դϴ�.
//

#include "stdafx.h"
#include "ShowImageDlg.h"

#define	ZOOM_NORMAL	1.1
#define	ZOOM_DETAIL	1.02

// CShowImageDlg ��ȭ �����Դϴ�.

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


// CShowImageDlg �޽��� ó�����Դϴ�.


BOOL CShowImageDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  ���⿡ �߰� �ʱ�ȭ �۾��� �߰��մϴ�.

	return TRUE;  // return TRUE unless you set the focus to a control
	// ����: OCX �Ӽ� �������� FALSE�� ��ȯ�ؾ� �մϴ�.
}


void CShowImageDlg::OnOK()
{
	// TODO: ���⿡ Ư��ȭ�� �ڵ带 �߰� ��/�Ǵ� �⺻ Ŭ������ ȣ���մϴ�.

	CDialogEx::OnOK();
}


void CShowImageDlg::OnCancel()
{
	// TODO: ���⿡ Ư��ȭ�� �ڵ带 �߰� ��/�Ǵ� �⺻ Ŭ������ ȣ���մϴ�.

	CDialogEx::OnCancel();
}


void CShowImageDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰��մϴ�.
}


BOOL CShowImageDlg::OnEraseBkgnd(CDC* pDC)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	return TRUE;
	return CDialogEx::OnEraseBkgnd(pDC);
}


void CShowImageDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
// 	m_bDrag		= true;
// 	m_ptDrag	= point;
// 	SetCapture();

	CDialogEx::OnLButtonDown(nFlags, point);
}


void CShowImageDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
// 	m_bDrag		= false;
// 	ReleaseCapture();

	CDialogEx::OnLButtonUp(nFlags, point);
}


void CShowImageDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
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
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
 	ShowWindow( SW_HIDE );
 	return;

//	SaveRawDataToBmp( "c:\\test.bmp", m_pImage, m_nWidth, m_nHeight, m_nChannel );

	CDialogEx::OnRButtonUp(nFlags, point);
}

void CShowImageDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰��մϴ�.
	// �׸��� �޽����� ���ؼ��� CDialogEx::OnPaint()��(��) ȣ������ ���ʽÿ�.

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

	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰��մϴ�.
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
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
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
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	if ( GetKeyState( VK_SHIFT ) & 0x8000 )
		( zDelta < 0 ) ? m_dZoom /= ZOOM_DETAIL : m_dZoom *= ZOOM_DETAIL;
	else
		( zDelta < 0 ) ? m_dZoom /= ZOOM_NORMAL : m_dZoom *= ZOOM_NORMAL;

	Invalidate();

	return CDialogEx::OnMouseWheel(nFlags, zDelta, pt);
}
