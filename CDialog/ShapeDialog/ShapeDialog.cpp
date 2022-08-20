// ShapeDialog.cpp : implementation file
//

#include "stdafx.h"
#include "ShapeDialog.h"


// CShapeDialog dialog

IMPLEMENT_DYNAMIC(CShapeDialog, CDialogEx)

CShapeDialog::CShapeDialog(UINT nIDTemplate, CWnd* pParent)
	: CDialogEx(nIDTemplate, pParent)
{

}

CShapeDialog::~CShapeDialog()
{
}

void CShapeDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CShapeDialog, CDialogEx)
END_MESSAGE_MAP()


// CShapeDialog message handlers


BOOL CShapeDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  Add extra initialization here

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

void CShapeDialog::set_image(LPCTSTR shape_image_file, byte dlg_alpha)
{
	m_pImage = this->LoadImage(shape_image_file);
	m_alpha = dlg_alpha;
	Render();
}

// Load the PNG file
Image* CShapeDialog::LoadImage(LPCTSTR lpszFile)
{
	HANDLE hFile = ::CreateFile( lpszFile
		, GENERIC_READ | GENERIC_WRITE
		, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE
		, NULL
		, OPEN_EXISTING
		, NULL
		, NULL
	);
	VERIFY(hFile);

	DWORD dwSize = ::GetFileSize( hFile, NULL);

	HGLOBAL hMem = ::GlobalAlloc(GMEM_FIXED, dwSize);
	LPBYTE pMem = (LPBYTE)::GlobalLock(hMem);
	DWORD dwRead = 0;
	::ReadFile( hFile, pMem, dwSize, &dwRead, NULL);
	::CloseHandle(hFile);
	VERIFY( dwSize != 0 );
	VERIFY( dwRead == dwSize );

	IStream * pStream = NULL;
	::CreateStreamOnHGlobal( hMem, FALSE, &pStream);

	Image * pImage = Image::FromStream(pStream);

	::GlobalUnlock(hMem);
	pStream->Release();


	return pImage;
}

//-------------------------------------------------------------------------
// Function Name    :Render
// Parameter(s)     :
// Return           :
// Create			:2008-8-18 20:21
// Memo             :Draw with GDI+
//-------------------------------------------------------------------------
void CShapeDialog::Render(void)
{
	if( !IsWindow(m_hWnd) || m_pImage == NULL)
		return;

	int width = m_pImage->GetWidth();
	int height = m_pImage->GetHeight();
	RECT rc;
	::GetWindowRect( m_hWnd, &rc);
	POINT ptSrc = { 0, 0};
	POINT ptWinPos = { rc.left, rc.top};
	SIZE szWin = { width, height };
	BLENDFUNCTION stBlend = { AC_SRC_OVER, 0, m_alpha, AC_SRC_ALPHA };


	HDC hDC = ::GetDC(m_hWnd);
	HDC hdcMemory = ::CreateCompatibleDC(hDC);

	BITMAPINFOHEADER bmih = { 0 };   
	int nBytesPerLine = ((width * 32 + 31) & (~31)) >> 3;
	bmih.biSize = sizeof(BITMAPINFOHEADER);   
	bmih.biWidth = width;   
	bmih.biHeight = height;   
	bmih.biPlanes = 1;   
	bmih.biBitCount = 32;   
	bmih.biCompression = BI_RGB;   
	bmih.biClrUsed = 0;   
	bmih.biSizeImage = nBytesPerLine * height;   

	PVOID pvBits = NULL;   
	HBITMAP hbmpMem = ::CreateDIBSection(NULL, (PBITMAPINFO)&bmih, DIB_RGB_COLORS, &pvBits, NULL, 0);
	ASSERT(hbmpMem != NULL);
	memset( pvBits, 0, width * 4 * height);
	if(hbmpMem)   
	{   
		HGDIOBJ hbmpOld = ::SelectObject( hdcMemory, hbmpMem); 
		Graphics graph(hdcMemory);

		graph.SetPageScale(1.0);
		graph.SetPageUnit(UnitPixel);
		graph.SetSmoothingMode(SmoothingModeNone);

		// Draw the dial
		graph.DrawImage( m_pImage, 0, 0, width, height);
		graph.DrawRectangle(&Pen(Color(255,0,0), 2.0f), Rect(20, 20, 50, 50)); 

		::UpdateLayeredWindow( m_hWnd
			, hDC
			, &ptWinPos
			, &szWin
			, hdcMemory
			, &ptSrc
			, 0
			, &stBlend
			, ULW_ALPHA
		);

		graph.ReleaseHDC(hdcMemory);
		::SelectObject( hdcMemory, hbmpOld);   
		::DeleteObject(hbmpMem); 
	}

	::DeleteDC(hdcMemory);
	::DeleteDC(hDC);
}