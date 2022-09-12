#include "GdiPlusBitmap.h"

void CGdiPlusBitmap::clone(CGdiPlusBitmap* dst)
{
	dst->m_pBitmap = m_pBitmap->Clone(0, 0, m_pBitmap->GetWidth(), m_pBitmap->GetHeight(), m_pBitmap->GetPixelFormat());
}

void CGdiPlusBitmap::deep_copy(CGdiPlusBitmap* dst)
{
	/*https://still.tistory.com/211
	* 이 코드 블록은 그리기로 복사하는 방식인데 속도가 느리므로
	* 아래 블록과 같이 메모리 복사 방식으로 복사한다.
	Gdiplus::Bitmap* pBitmap = new Gdiplus::Bitmap(bitmap.GetWidth(), bitmap.GetHeight(), bitmap.GetPixelFormat());
	Gdiplus::Graphics* g = Gdiplus::Graphics::FromImage(pBitmap);
	g->DrawImage(&bitmap, 0, 0);
	delete g;
	*/

	//메모리 복사 방식
	dst->m_pBitmap = new Gdiplus::Bitmap(m_pBitmap->GetWidth(), m_pBitmap->GetHeight(), m_pBitmap->GetPixelFormat());
	Gdiplus::Rect rect(0, 0, m_pBitmap->GetWidth(), m_pBitmap->GetHeight());
	Gdiplus::BitmapData bmpData;
	dst->m_pBitmap->LockBits(&rect, Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeWrite, dst->m_pBitmap->GetPixelFormat(), &bmpData);
	m_pBitmap->LockBits(&rect, Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeUserInputBuf, dst->m_pBitmap->GetPixelFormat(), &bmpData);
	m_pBitmap->UnlockBits(&bmpData);
	dst->m_pBitmap->UnlockBits(&bmpData);
}

void CGdiPlusBitmap::rotate(Gdiplus::RotateFlipType type)
{
	m_pBitmap->RotateFlip(type);
}

void CGdiPlusBitmap::rotate(float degree)
{
	UINT w = m_pBitmap->GetWidth();
	UINT h = m_pBitmap->GetHeight();

	//create a new empty bitmap to hold rotated image 
	Bitmap* result = new Bitmap(w, h);
	//make a graphics object from the empty bitmap
	Graphics g(result);

	if (true)
	{
		//move rotation point to center of image
		g.TranslateTransform((float)w / 2, (float)h / 2);
		//rotate
		g.RotateTransform(degree);
		//move image back
		g.TranslateTransform(-(float)w / 2, -(float)h / 2);
		//draw passed in image onto graphics object
	}
	else
	{
		Gdiplus::Matrix matrix;
		matrix.RotateAt(degree, Gdiplus::PointF((float)(m_pBitmap->GetWidth() / 2), (float)(m_pBitmap->GetHeight() / 2)));
		//해당 이미지의 정중앙을 기준으로 235.7도 돌리라는 얘기입니다. 
		//참고로 가운데를 기준으로 돌리기 위해서는 이미지의 가로세로 사이즈가 홀수여야 깔끔하게 돌아갈겁니다.
		g.SetTransform(&matrix); //ScreenG를 위에 규칙대로 돌려버렸습니다.
	}

	g.DrawImage(m_pBitmap, 0, 0);
	delete m_pBitmap;

	m_pBitmap = result->Clone(0, 0, w, h, PixelFormatDontCare);
}

void CGdiPlusBitmap::set_transparent(float transparent)
{
	ColorMatrix colorMatrix = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
								0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
								0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
								0.0f, 0.0f, 0.0f, transparent, 0.0f,
								0.0f, 0.0f, 0.0f, 0.0f, 1.0f };

	int w = width();
	int h = height();

	//create a new empty bitmap to hold rotated image 
	Bitmap* result = new Bitmap(w, h);
	//make a graphics object from the empty bitmap
	Graphics g(result);

	ImageAttributes ia;

	ia.SetColorMatrix(&colorMatrix, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);
	g.DrawImage(m_pBitmap, Rect(0, 0, w, h), 0, 0, w, h, UnitPixel, &ia);
	delete m_pBitmap;

	m_pBitmap = result->Clone(0, 0, w, h, PixelFormatDontCare);
}

void CGdiPlusBitmap::set_colorkey(Color low, Color high)
{
	int w = width();
	int h = height();

	//create a new empty bitmap to hold rotated image 
	Bitmap* result = new Bitmap(w, h);
	//make a graphics object from the empty bitmap
	Graphics g(result);

	ImageAttributes imageAttr;
	imageAttr.SetColorKey(low, high);

	g.DrawImage(m_pBitmap, Rect(0, 0, w, h), 0, 0, w, h, UnitPixel, &imageAttr);
	delete m_pBitmap;

	m_pBitmap = result->Clone(0, 0, w, h, PixelFormatDontCare);
}
