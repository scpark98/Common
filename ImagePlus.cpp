#include "stdafx.h"
#include "ImagePlus.h"
#include "ImageOp.h"

//INT GetEncoderClsid(const WCHAR* format, CLSID* pClsid);  // helper function

CImagePlus::CImagePlus()
{
	m_Image = NULL;
}

CImagePlus::~CImagePlus()
{
	Release();
}

void CImagePlus::Release()
{
	if ( m_Image )
	{
		delete m_Image;
		m_Image = NULL;
	}
}

BOOL CImagePlus::LoadResource( int nResourceID )
{
	HINSTANCE hInst = 
		AfxFindResourceHandle(MAKEINTRESOURCE(nResourceID),MAKEINTRESOURCE(2));

	m_Image = Bitmap::FromResource( hInst, (WCHAR*)MAKEINTRESOURCE( nResourceID ) );

//	m_Image->SetPixel( 500, 10, Color(255,0,0) );
	if ( m_Image )
		return TRUE;

	return FALSE;
}

BOOL CImagePlus::LoadImageFile( CString sFileName, bool bShowError )
{
	UINT	nWidth, nHeight;
	CString str;

	if ( m_Image )
	{
		delete m_Image;
		m_Image = NULL;
	}

	Bitmap*		bitmap;

	BSTR bstrFilename = sFileName.AllocSysString();
	bitmap = Bitmap::FromFile(bstrFilename);
	::SysFreeString(bstrFilename);

	if ( bitmap->GetLastStatus() != Gdiplus::Ok )
	{
		if ( bShowError )
		{
			str.Format( _T("%s\n\n위 파일이 존재하지 않거나 열 수 없습니다."), sFileName );
			AfxMessageBox( str  );
		}
		return FALSE;
	}
	
	nWidth	= bitmap->GetWidth();
	nHeight	= bitmap->GetHeight();

	m_Image = new Bitmap( nWidth, nHeight, PixelFormat32bppARGB );

	Graphics	g( m_Image );

	g.DrawImage( bitmap, 0, 0, nWidth, nHeight );//, UnitPixel, &attr );
	
	delete bitmap;

	return TRUE;
}

int CImagePlus::Width()
{
	if ( m_Image )
		return m_Image->GetWidth();

	return 0;
}

int CImagePlus::Height()
{
	if ( m_Image )
		return m_Image->GetHeight();

	return 0;
}

void CImagePlus::Rotate( BOOL bClockWise )
{
	if ( m_Image )
		m_Image->RotateFlip( bClockWise ? RotateFlipType(Rotate90FlipNone) : RotateFlipType(Rotate270FlipNone) );
}

void CImagePlus::RotateDraw( CDC* pDC, int x, int y, REAL angle, int alpha )
{
	if ( !m_Image )
		return;

	int ix, iy;

	Bitmap tempbmp(Width(), Height(), PixelFormat32bppARGB);
	
	ix = int(Width() / (-2.0));
	iy = int(Height() / (-2.0));
	
	Graphics gx(pDC->m_hDC);
	Graphics tempgx(&tempbmp);
	tempgx.RotateTransform( angle );
	tempgx.TranslateTransform(REAL(-ix), REAL(-iy), MatrixOrderAppend);
	Point dest[3] = {Point(ix, iy), Point(ix + Width(), iy), Point(ix, iy + Height())};


	float	fAlpha = (float)(255.0f - alpha) / 255.0f;

	// Initialize the color matrix.
	// Notice the value 0.8 in row 4, column 4.
	ColorMatrix colorMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
							   0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
							   0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
							   0.0f, 0.0f, 0.0f, fAlpha, 0.0f,
							   0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
	// Create an ImageAttributes object and set its color matrix.
	ImageAttributes imageAtt;
	imageAtt.SetColorMatrix(&colorMatrix, ColorMatrixFlagsDefault,
						   ColorAdjustTypeBitmap);

	tempgx.DrawImage(m_Image, dest, 3, 0, 0, Width(), Height(), UnitPixel, &imageAtt );
	gx.DrawImage(&tempbmp, x, y);
}

void CImagePlus::Draw( CDC* pDC, int x, int y, int nRotateType )
{
	if ( !m_Image )
		return;

	Graphics graphics( pDC->m_hDC );
	m_Image->RotateFlip( RotateFlipType(nRotateType) );
	graphics.DrawImage( m_Image, x, y );
}

void CImagePlus::Draw( CDC* pDC, CRect Rect, BOOL bAspect, BOOL bStretch, COLORREF BackColor )
{
	if ( !m_Image )
		return;

	CSize		Size = CSize( Width(), Height() );
	Graphics	graphics( pDC->m_hDC );
	SolidBrush	BackBrush(Color(255, GetRValue(BackColor), GetGValue(BackColor), GetBValue(BackColor)));

	if ( !bAspect )
		Draw( pDC, Rect.left, Rect.top, Rect.Width(), Rect.Height() );
	else
	{
		if ( bAspect && !bStretch )
		{
			CSize	Margin;
			Margin.cx = (Rect.Width() - Width()) / 2;
			Margin.cy = (Rect.Height() - Height()) / 2;
			graphics.FillRectangle( &BackBrush, Rect.left, Rect.top, Rect.Width(), Rect.top + Margin.cy );
			graphics.FillRectangle( &BackBrush, Rect.left, Rect.top + Margin.cy + Height(),
									 Rect.Width(), Rect.Height() - Margin.cy - Height() );
			graphics.FillRectangle( &BackBrush, Rect.left, Rect.top + Margin.cy, Margin.cx, Height() );
			graphics.FillRectangle( &BackBrush, Rect.left + Margin.cx + Width(), Rect.top + Margin.cy,
									 Rect.Width() - Margin.cx - Width(), Height() );
			Draw( pDC, Rect.left + Margin.cx, Rect.top + Margin.cy );
		}
		else
		{
			int			nOffset;
			double		dZoom;
			// 표시될 사각형 영역에 대한 그림의 가로가 큰 경우
			if ( Size.cx * Rect.Height() > Size.cy * Rect.Width() )
			{
				dZoom = (double)Rect.Width() / (double)Size.cx;
				nOffset	= (int)( (double)( Rect.Height() - Size.cy * dZoom ) / 2.0 );
				graphics.FillRectangle( &BackBrush, Rect.left, Rect.top, Rect.Width(), nOffset );
				graphics.DrawImage( m_Image, Rect.left, Rect.top + nOffset, Rect.Width(), (int)( (double)Size.cy * dZoom ) );
				graphics.FillRectangle(	&BackBrush, Rect.left, Rect.top + nOffset + (int)( (double)Size.cy * dZoom ),
										Rect.Width(),
										Rect.Height() - Rect.top - nOffset - (int)( (double)Size.cy * dZoom ) );
			}
			else
			{
				dZoom = (double)Rect.Height() / (double)Size.cy;
				nOffset	= (int)( ( Rect.Width() - Size.cx * dZoom ) / 2.0 );
				graphics.FillRectangle( &BackBrush, Rect.left, Rect.top, nOffset, Rect.Height() );
				graphics.DrawImage( m_Image, Rect.left + nOffset, Rect.top, (int)( (double)Size.cx * dZoom ), Rect.Height() );
				graphics.FillRectangle(	&BackBrush, Rect.left + nOffset + (int)( (double)Size.cx * dZoom ), Rect.top,
										Rect.Width() - Rect.left - nOffset - (int)( (double)Size.cx * dZoom ),
										Rect.Height() );
			}
		}
	}
}

void CImagePlus::Draw( CDC* pDC, int x, int y, int w, int h )
{
	if ( !m_Image )
		return;

	Graphics graphics( pDC->m_hDC );
	graphics.DrawImage( m_Image, x, y, w, h );
}

void CImagePlus::Draw( CDC* pDC, int x, int y )
{
	if ( !m_Image )
		return;
	
	Graphics graphics( pDC->m_hDC );
	graphics.DrawImage( m_Image, x, y, m_Image->GetWidth(), m_Image->GetHeight() );
}

void CImagePlus::Draw( CDC* pDC, int x, int y, int w, int h, int nAlpha )
{
	if ( !m_Image )
		return;

	Graphics graphics( pDC->m_hDC );

	float	fAlpha = (float)(255 - nAlpha) / 255.0f;

	// Initialize the color matrix.
	// Notice the value 0.8 in row 4, column 4.
	ColorMatrix colorMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
							   0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
							   0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
							   0.0f, 0.0f, 0.0f, fAlpha, 0.0f,
							   0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
	// Create an ImageAttributes object and set its color matrix.
	ImageAttributes imageAtt;
	imageAtt.SetColorMatrix(&colorMatrix, ColorMatrixFlagsDefault,
						   ColorAdjustTypeBitmap);

//	imageAtt.SetGamma( fAlpha, ColorAdjustTypeBitmap );
	
	Rect destRect(x, y, w, h);
	graphics.DrawImage( m_Image, destRect, 0, 0,
						w, h, UnitPixel, &imageAtt );
}

BOOL CImagePlus::SetAlpha( int nAlpha )
{
	if ( !m_Image )
		return FALSE;

	float		fAlpha = (float)(255 - nAlpha) / 255.0f;
	Rect		destRect( 0, 0, m_Image->GetWidth(), m_Image->GetHeight() );

	Bitmap*		bitmap = new Bitmap( m_Image->GetWidth(), m_Image->GetHeight() );
	Graphics	g( bitmap );

	// Initialize the color matrix.
	// Notice the value 0.8 in row 4, column 4.
	ColorMatrix colorMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
							   0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
							   0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
							   0.0f, 0.0f, 0.0f, fAlpha, 0.0f,
							   0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
	// Create an ImageAttributes object and set its color matrix.
	ImageAttributes imageAtt;
	imageAtt.SetColorMatrix(&colorMatrix, ColorMatrixFlagsDefault,
						   ColorAdjustTypeBitmap);

	g.DrawImage( m_Image, destRect, 0, 0,
				 m_Image->GetWidth(), m_Image->GetHeight(), UnitPixel, &imageAtt );

	m_Image = bitmap;
	return TRUE;
//	delete bitmap;
}

void CImagePlus::BrightContrast( CDC* pDC, int x, int y, int w, int h, int nBright, int nContrast )
{
	if ( !m_Image )
		return;

	Graphics graphics( pDC->m_hDC );

	// Initialize the color matrix.
	// Notice the value 0.8 in row 4, column 4.
	Matrix myMatrix( 1.0f, 0.0f, 0.0f, 1.0f, 200.0f, 20.0f );
/*
	BrightnessContrastParams briConParams;
	briConParams.brightnessLevel	= nBright;
	briConParams.contrastLevel		= nContrast;
	BrightnessContrast briCon;
	briCon.SetParameters(&briConParams);
	
	// Draw the image with no change in contrast.
	graphics.DrawImage(&myImage, 20.0f, 20.0f, srcWidth, srcHeight);
	
	// Draw the image with increased contrast.
	graphics.DrawImage(&myImage, &srcRect, &myMatrix, &briCon, NULL, UnitPixel);
*/
}

void CImagePlus::Resize( int nWidth, int nHeight )
{
	m_Image = (Bitmap*)(m_Image->GetThumbnailImage( nWidth, nHeight ));
}

void CImagePlus::SetSubImage(	CImagePlus ImageSrc,
								int dx, int dy, int dw/* = 0*/, int dh/* = 0*/,
								int sx/* = 0*/, int sy/* = 0*/, int sw/* = 0*/, int sh/* = 0*/ )
{
	Graphics	g( m_Image );

	if ( dw == 0 )
		dw = ImageSrc.Width();

	if ( dh == 0 )
		dh = ImageSrc.Height();

	if ( sw == 0 )
		sw = dw;

	if ( sh == 0 )
		sh = dh;

	Rect		sourceRect( sx, sy, sw, sh );

	Bitmap*		clone = ImageSrc.m_Image->Clone( sourceRect, PixelFormatDontCare );
	g.DrawImage( ImageSrc.m_Image, dx, dy, dw, dh );

	delete clone;
}

Bitmap* CImagePlus::GetSubImage( int x, int y, int w, int h )
{
	Rect sourceRect( x, y, w, h );

	return m_Image->Clone( sourceRect, PixelFormatDontCare );
}

void CImagePlus::MaskImage( COLORREF rRGB, int nAlpha )
{
	int*	pixels = Bitmap2PixelData( m_Image );
	MaskImageOP( m_Image->GetWidth(), m_Image->GetHeight(), pixels, rRGB, nAlpha );
	m_Image = PixelData2BitMap( m_Image->GetWidth(), m_Image->GetHeight(), pixels);
	
	delete pixels;
}

void CImagePlus::Negative()
{
	int*	pixels = Bitmap2PixelData( m_Image );
	NegativeOP( m_Image->GetWidth(), m_Image->GetHeight(), pixels );
	m_Image = PixelData2BitMap( m_Image->GetWidth(), m_Image->GetHeight(), pixels);
	
	delete pixels;
}

void CImagePlus::GrayScale()
{
	//아래 코드는 예전에 작성된 코드이며 ImageAttribute으로 빠르게 구현될 수 있다.
	// GrayScale기능은 현재 토글되지 않는다.
	int*	pixels = Bitmap2PixelData( m_Image );
	GrayScaleOP( m_Image->GetWidth(), m_Image->GetHeight(), pixels );
	m_Image = PixelData2BitMap( m_Image->GetWidth(), m_Image->GetHeight(), pixels);
	
	delete pixels;
}

int* CImagePlus::Bitmap2PixelData(Bitmap *b)
{
	int width = b->GetWidth();
	int height = b->GetHeight();

	if ( width * height <= 0 ) 
		return NULL;

	int *pixelData_ARGB = new int[width*height];

	if ( pixelData_ARGB == NULL )
		return NULL;

    BitmapData bmData;
	Rect rect(0, 0, width, height);

	b->LockBits(&rect, 
        ImageLockModeRead, 
		PixelFormat32bppARGB,
		&bmData); 

    int stride = bmData.Stride; 
    BYTE *p = (BYTE *)((void *)bmData.Scan0);

	int nOffset = stride - width*4; // bytes to skip at end of each row

	for(int y=0; y < height;y++) {
		for(int x=0; x < width; ++x ) {
			// GDI lies about RGB - internally its BGR
			pixelData_ARGB[y*width+x] =  ARGB2Pixel(p[3], p[2], p[1], p[0]);
			p += 4;
		}
		p += nOffset;
	}
	b->UnlockBits(&bmData);

	return (pixelData_ARGB);
}

Bitmap *CImagePlus::PixelData2BitMap(int width, int height, int *pixData_ARGB)
{
	Bitmap bit(width, height, PixelFormat32bppARGB);
	Bitmap *b = bit.Clone(0, 0, bit.GetWidth(), bit.GetHeight(), PixelFormat32bppARGB);

    BitmapData bmData;
	Rect rect(0, 0, b->GetWidth(), b->GetHeight());
	b->LockBits(&rect, 
        ImageLockModeRead | ImageLockModeWrite, 
		PixelFormat32bppARGB,
		&bmData); 

    int stride = bmData.Stride; 
    BYTE *p = (BYTE *)((void *)bmData.Scan0);
	int nOffset = stride - width*4; // bytes to skip at end of each row

	int pixel;

	for(int y=0; y < height;y++) {
		for(int x=0; x < width; ++x ) {
			// GDI lies about RGB - internally its BGR
			pixel = pixData_ARGB[y*width+x];
			p[3] = (BYTE) ((pixel >> 24) & 0xff); // alpha
			p[2] = (BYTE) ((pixel >> 16) & 0xff); // pixel red
			p[1] = (BYTE) ((pixel >> 8 ) & 0xff); // pixel green
			p[0] = (BYTE) ((pixel      ) & 0xff); // pixel blue
			p += 4;
		}
		p += nOffset;
	}
	b->UnlockBits(&bmData);

	return (b);
}

int CImagePlus::GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
   UINT  num = 0;          // number of image encoders
   UINT  size = 0;         // size of the image encoder array in bytes

   ImageCodecInfo* pImageCodecInfo = NULL;

   GetImageEncodersSize(&num, &size);
   if(size == 0)
      return -1;  // Failure

   pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
   if(pImageCodecInfo == NULL)
      return -1;  // Failure

   GetImageEncoders(num, size, pImageCodecInfo);

   for(UINT j = 0; j < num; ++j)
   {
      if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
      {
         *pClsid = pImageCodecInfo[j].Clsid;
         free(pImageCodecInfo);
         return j;  // Success
      }    
   }

   free(pImageCodecInfo);
   return -1;  // Failure
}

BOOL CImagePlus::SaveImage2Jpg( CString sFileName )
{
	return SaveImage2Jpg( sFileName, 100 );
}

BOOL CImagePlus::SaveImage2Jpg( CString sFileName, ULONG quality )
{
	return SaveBitmap2Jpg( sFileName );
/*
	CLSID				encoderClsid;
	EncoderParameters	encoderParameters;
	WCHAR				wFile[MAX_PATH];
	
	MultiByteToWideChar( CP_ACP, 0, sFileName, -1, wFile, MAX_PATH );

	GetEncoderClsid(L"image/jpeg", &encoderClsid);

	encoderParameters.Count = 1;
	encoderParameters.Parameter[0].Guid = EncoderQuality;
	encoderParameters.Parameter[0].Type = EncoderParameterValueTypeLong;
	encoderParameters.Parameter[0].NumberOfValues = 1;
	
	// Save the image as a JPEG with quality level 0.
	encoderParameters.Parameter[0].Value = &quality;

	Status s = m_Image->Save( wFile, &encoderClsid, &encoderParameters);

	if ( s == Ok )
		return TRUE;
	else
		return FALSE;
*/
}

BOOL CImagePlus::GetHBitmap( HBITMAP* hBitmap )
{
	if ( m_Image == NULL )
		return FALSE;

	Status s = m_Image->GetHBITMAP( NULL, hBitmap );

	if ( s == Ok )
		return TRUE;
	else
		return FALSE;
}

BOOL CImagePlus::SaveBitmap2Jpg( CString sFileName, Bitmap* Bitmap/* = NULL*/, ULONG quality/* = 100*/ )
{
	CLSID				encoderClsid;
	EncoderParameters	encoderParameters;
	WCHAR				wFile[MAX_PATH];
	
	//MultiByteToWideChar( CP_ACP, 0, sFileName, -1, wFile, MAX_PATH );

	GetEncoderClsid(L"image/jpeg", &encoderClsid);

	encoderParameters.Count = 1;
	encoderParameters.Parameter[0].Guid = EncoderQuality;
	encoderParameters.Parameter[0].Type = EncoderParameterValueTypeLong;
	encoderParameters.Parameter[0].NumberOfValues = 1;
	
	// Save the image as a JPEG with quality level 0.
	encoderParameters.Parameter[0].Value = &quality;

	Status s;
	
	if ( Bitmap == NULL )
		s = m_Image->Save( wFile, &encoderClsid, &encoderParameters);
	else
		s = Bitmap->Save( wFile, &encoderClsid, &encoderParameters);

	if ( s == Ok )
		return TRUE;
	else
		return FALSE;
}