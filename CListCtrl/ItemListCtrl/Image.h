#if !defined(AFX_PICTURE_H__3E10466B_FF90_4D10_B51E_EE458C0A2CB2__INCLUDED_)
#define AFX_PICTURE_H__3E10466B_FF90_4D10_B51E_EE458C0A2CB2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Picture.h : header file
//
//----------------------------------------------------------------------------
// 클래스 버전정보 : 2003 년 11월 27일 버전 
//----------------------------------------------------------------------------

#ifndef	_CAST_8
	#define _CAST_8
	#define CAST_8U(d)	( d<0.0 ? 0 : ( d>255.0 ? 255 : BYTE(d)))
	#define CAST_8D(d)	( d<0.0 ? 0.0 : ( d>255.0 ? 255.0 : d))
	
	#define CAST_RECT(r)( Rect(r.left, r.top, r.Width(), r.Height()) )
	#define CAST_CRECT(r)( CRect(r.X, r.Y, r.X+r.Width, r.Y+r.Height) )

	#define CAST_POINT(p)( Point(p.x, p.y) )
	#define CAST_CPOINT(p)( CPoint(p.X, p.Y) )

	#define GET_AU(u)	( (u >> 24) & 0xff )
	#define GET_RU(u)	( (u >> 16) & 0xff )
	#define GET_GU(u)	( (u >> 8) & 0xff )
	#define GET_BU(u)	( u & 0xff )

	#define SET_ARGB(VALUE,A,R,G,B) (VALUE = A << 24 | R << 16 | G << 8 | B)

	#define ARGB(A,R,G,B)			(A << 24 | R << 16 | G << 8 | B)
	#define CAST_ARGB(R,G,B)		(255 << 24 | R << 16 | G << 8 | B)

	#include "math.h"

	#define CEIL(u)		int(ceil(u))
#endif

/////////////////////////////////////////////////////////////////////////////
// CImage command target
#include "GraphicObject.h"

class CImage : public CGraphicObject
{
public:
	CImage();           

	CImage(CImage& imgSource);
	CImage(Bitmap* pBitmapSource);
	CImage(Image* pImageSource);

	CImage(CString strFile);	
	CImage(UINT nResource );
	CImage(CString strResourceType, CString strResource );
	CImage(int nWidth, int nHeight, PixelFormat format);
	CImage(UINT * pBuffer, int nWidth, int nHeight);

	virtual ~CImage();

public:
	Bitmap * m_pImage;

protected:
	CString m_strImage;

protected:
	InterpolationMode m_interpolationMode;

public:
	CImage& operator=(CImage& imgSource);

public:
	CImage * Clone();

public:
	bool Create(UINT * pBuffer, int nWidth, int nHeight, bool bDelete = true);

public:
	void Draw(CDC * pDC, int x, int y, int nWidth , int nHeight , COLORREF clrMask , float fAlpha );
	void Draw(CDC * pDC, int x, int y, COLORREF clrMask , float fAlpha);
	void Draw(CDC * pDC, int x, int y, int nWidth , int nHeight, float fAlpha );
	void Draw(CDC * pDC, int x, int y, float fAlpha );
	void Draw(CDC * pDC, int x, int y, int nWidth , int nHeight );
	void Draw(CDC * pDC, int x = 0 , int y = 0 );

	void Draw(CDC * pDC, Rect rcDest, Rect rcSrc , COLORREF clrMask , float fAlpha );
	void Draw(CDC * pDC, Rect rcDest, Rect rcSrc , float fAlpha );

	void Draw(CDC * pDC, Rect rcImage, COLORREF clrMask , float fAlpha );
	void Draw(CDC * pDC, Rect rcImage, float fAlpha );
	void Draw(CDC * pDC, Rect rcImage );


	void Draw(Graphics* pG, int x, int y, int nWidth , int nHeight , COLORREF clrMask , float fAlpha );
	void Draw(Graphics* pG, int x, int y, COLORREF clrMask , float fAlpha);
	void Draw(Graphics* pG, int x, int y, int nWidth , int nHeight, float fAlpha );
	void Draw(Graphics* pG, int x, int y, float fAlpha );
	void Draw(Graphics* pG, int x, int y, int nWidth , int nHeight );
	void Draw(Graphics* pG, int x = 0 , int y = 0 );

	void Draw(Graphics* pG, Rect rcDest, Rect rcSrc , COLORREF clrMask , float fAlpha );
	void Draw(Graphics* pG, Rect rcDest, Rect rcSrc , float fAlpha );

	void Draw(Graphics* pG, Rect rcImage, COLORREF clrMask , float fAlpha );
	void Draw(Graphics* pG, Rect rcImage, float fAlpha );
	void Draw(Graphics* pG, Rect rcImage);

public:
	static BYTE * GetBuffer(Bitmap * pBitmap, PixelFormat pixelFormat);
	static BYTE * GetBuffer(Bitmap * pBitmap, Rect rcImage, PixelFormat pixelFormat);

	static bool SetBuffer(Bitmap * pBitmap, UINT * pBuffer);
	static bool SetBuffer(Bitmap * pBitmap, BYTE * pBuffer, PixelFormat pixelFormat);

	static bool SetBuffer(Bitmap * pBitmap, UINT * pBuffer, Rect rcImage);
	static bool SetBuffer(Bitmap * pBitmap, BYTE * pBuffer, Rect rcImage, PixelFormat pixelFormat);

	static int GetBytesForPixel(PixelFormat pixelFormat);

public:
	UINT GetHeight();
	UINT GetWidth();

	bool SetBuffer( BYTE * pBuffer );
	bool SetBuffer( BYTE * pBuffer, PixelFormat pixelFormat);

	bool SetBuffer( BYTE * pBuffer, Rect rcImage);
	bool SetBuffer( BYTE * pBuffer, Rect rcImage, PixelFormat pixelFormat);

	bool SetBuffer( UINT * pBuffer );
	bool SetBuffer( UINT * pBuffer, Rect rcImage);

	bool SetBuffer( CImage * pImage, Rect rcImage);

	BYTE * GetBuffer();
	BYTE * GetBuffer(Rect rcImage);
	BYTE * GetBuffer(PixelFormat pixelFormat);
	BYTE * GetBuffer(Rect rcImage, PixelFormat pixelFormat);

public:
	COLORREF GetPixel(int x, int y);
	COLORREF GetPixel(Point ptPos);

	bool SetPixel(int x, int y, COLORREF clrPixel);
	bool SetPixel(Point ptPos, COLORREF clrPixel);

	void GetPixel(int x, int y, Color& clrPixel);
	void GetPixel(Point ptPos, Color& clrPixel);

	bool SetPixel(int x, int y, Color& clrPixel);
	bool SetPixel(Point ptPos, Color& clrPixel);

public:
	CImage * GetThumbnailImage(int nWidth, int nHeight, 
							   InterpolationMode interpolationMode = InterpolationModeDefault );
	CImage * GetThumbnailImage(float fXScale, float fYScale,
							   InterpolationMode interpolationMode = InterpolationModeDefault );

	CString GetFileName();
	void SetFileName( CString strImage );

public:
	bool Save(CString strFile, long lQuality = 100 );

	bool SaveToJpeg(CString strFile , long lQuality = 100);
	bool SaveToGif(CString strFormat);
	bool SaveToBmp(CString strFile);

	bool Load( CString strFile );
	bool Load( UINT nResource ); 
	bool Load(CString strResourceType, CString strResource);

	void UnLoad();

public:
	bool CreateStream(CString strResourceType, CString strResource, IStream * pStream);

protected:
	int GetCodecClsid(const WCHAR *format, CLSID *pClsid);

public:
	bool SetSize(int nWidth, int nHeight );
	bool SetScale(double dX, double dY);
	bool SetRotate(double dRotate);
	bool SetRotateFlip(RotateFlipType enumRotate);

	CImage* GetSizeImage(int nWidth, int nHeight);
	CImage* GetScaleImage(double dX, double dY);
	CImage* GetRotateImage(double dRotate, Color clrBackground = Color::Black);
	CImage* GetRotateFlipImage(RotateFlipType enumRotate);

public:
	Rect GetRect();
	bool IsValid();
	bool SetActiveFrame( int nFramePos );
	int GetFrameCount();

	Graphics * GetGraphics();

	void	SetPalette( ColorPalette * pPal );
	ColorPalette * GetPalette();

	int GetPaletteSize();

	PixelFormat GetPixelFormat();
	int GetBytesForPixel();

	InterpolationMode GetInterpolationMode();
	InterpolationMode SetInterpolationMode(InterpolationMode interpolationMode);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PICTURE_H__3E10466B_FF90_4D10_B51E_EE458C0A2CB2__INCLUDED_)
