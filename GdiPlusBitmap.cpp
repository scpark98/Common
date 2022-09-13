#include "GdiPlusBitmap.h"
#include "../Common/Functions.h"

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
	CRect rotated(0, 0, w, h);
	std::vector<CPoint> m_pts = get_rotated(w / 2, h / 2, &rotated, degree);

	w = rotated.Width();
	h = rotated.Height();

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
		matrix.RotateAt(degree, Gdiplus::PointF((float)(rotated.CenterPoint().x), (float)(rotated.CenterPoint().y)));
		//해당 이미지의 정중앙을 기준으로 235.7도 돌리라는 얘기입니다. 
		//참고로 가운데를 기준으로 돌리기 위해서는 이미지의 가로세로 사이즈가 홀수여야 깔끔하게 돌아갈겁니다.
		g.SetTransform(&matrix); //ScreenG를 위에 규칙대로 돌려버렸습니다.
	}

	g.DrawImage(m_pBitmap, 0, 0, w, h);
	//g.DrawImage(m_pBitmap, -rotated.left, -rotated.top, w, h);

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

	Bitmap* result = new Bitmap(w, h);
	Graphics g(result);

	ImageAttributes imageAttr;
	imageAttr.SetColorKey(low, high);

	g.DrawImage(m_pBitmap, Rect(0, 0, w, h), 0, 0, w, h, UnitPixel, &imageAttr);
	delete m_pBitmap;

	m_pBitmap = result->Clone(0, 0, w, h, PixelFormatDontCare);
}

int CGdiPlusBitmap::GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes

	ImageCodecInfo* pImageCodecInfo = NULL;

	GetImageEncodersSize(&num, &size);
	if (size == 0)
		return -1;  // Failure

	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if (pImageCodecInfo == NULL)
		return -1;  // Failure

	GetImageEncoders(num, size, pImageCodecInfo);

	for (UINT j = 0; j < num; ++j)
	{
		if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;  // Success
		}
	}

	free(pImageCodecInfo);
	return -1;  // Failure
}

bool CGdiPlusBitmap::save(CString filename)//, ULONG quality/* = 100*/)
{
	CLSID				encoderClsid;
	EncoderParameters	encoderParameters;
	WCHAR				wFile[MAX_PATH];

	CString ext = GetFileExtension(filename).MakeLower();

	if (ext == _T("jpg") || ext == _T("jpeg"))
		GetEncoderClsid(L"image/jpeg", &encoderClsid);
	else if (ext == _T("png"))
		GetEncoderClsid(L"image/png", &encoderClsid);
	/*
	encoderParameters.Count = 1;
	encoderParameters.Parameter[0].Guid = EncoderQuality;
	encoderParameters.Parameter[0].Type = EncoderParameterValueTypeLong;
	encoderParameters.Parameter[0].NumberOfValues = 1;

	// Save the image as a JPEG with quality level 0.
	encoderParameters.Parameter[0].Value = &quality;
	*/
	Status s;

	s = m_pBitmap->Save(filename, &encoderClsid);// , & encoderParameters);

	if (s == Ok)
		return TRUE;
	else
		return FALSE;
}
