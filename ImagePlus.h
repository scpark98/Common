#if !defined(AFX_IMAGEPLUS_H__0DC72627_C2E5_4BCA_BC32_361377247047__INCLUDED_)
#define AFX_IMAGEPLUS_H__0DC72627_C2E5_4BCA_BC32_361377247047__INCLUDED_

using namespace Gdiplus;

class CImagePlus// : public Image
{
public :
	CImagePlus();
	~CImagePlus();
	
	Bitmap*	m_Image;
	int		nRotate;
	
	void	Release();
	BOOL	IsValid() {	return ( m_Image != NULL );	}
	BOOL	LoadResource( int nResourceID );
	BOOL	LoadImageFile( CString sFileName, bool bShowError = true );
	BOOL	SaveImage2Jpg( CString sFileName );
	BOOL	SaveImage2Jpg( CString sFileName, ULONG quality );
	BOOL	SaveBitmap2Jpg( CString sFileName, Bitmap* Bitmap = NULL, ULONG quality = 100 );
	BOOL	GetHBitmap( HBITMAP* hBitmap );
	int		Width();
	int		Height();
	void	Resize( int nWidth, int nHeight );
	void	Rotate( BOOL bClockWise );
	void	RotateDraw( CDC* pDC, int x, int y, REAL angle, int alpha = 0 );
	void	Negative();
	void	GrayScale();
	void	BrightContrast( CDC* pDC, int x, int y, int w, int h, int nBright, int nContrast );
	void	MaskImage( COLORREF rRGB, int nAlpha );
	BOOL	SetAlpha( int nAlpha );
	void	Draw( CDC* pDC, int x, int y);
//	void	Draw( CDC* pDC, int xSrc, int ySrc, int w, int h, int x, int y );
	void	Draw( CDC* pDC, CRect Rect, BOOL bAspect = FALSE, BOOL bStretch = TRUE, COLORREF BackColor = RGB(0,0,0) );
	void	Draw( CDC* pDC, int x, int y, int w, int h );
	void	Draw( CDC* pDC, int x, int y, int w, int h, int nAlpha );
	void	Draw( CDC* pDC, int x, int y, int nRotateType );
//	void	DrawRotate( CDC* pDC, )
	int*	Bitmap2PixelData(Bitmap *b);
	void	SetSubImage( CImagePlus ImageSrc, int dx, int dy, int dw = 0, int dh = 0, int sx = 0, int sy = 0, int sw = 0, int sh = 0 );
	Bitmap*	GetSubImage( int x, int y, int w, int h );
	Bitmap*	PixelData2BitMap(int width, int height, int *pixData_ARGB);
	int		GetEncoderClsid(const WCHAR* format, CLSID* pClsid);
};

#endif // !defined(AFX_IMAGEPLUS_H__0DC72627_C2E5_4BCA_BC32_361377247047__INCLUDED_)
