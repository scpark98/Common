#include "GdiPlusBitmap.h"
#include "../Common/Functions.h"

CGdiPlusBitmap::CGdiPlusBitmap()
{
	m_pBitmap = NULL;
	release();
}

CGdiPlusBitmap::CGdiPlusBitmap(Bitmap* src)
{
	release();
	m_pBitmap = src;
}

CGdiPlusBitmap::CGdiPlusBitmap(LPCWSTR pFile)
{
	m_pBitmap = NULL;
	Load(pFile);
}

CGdiPlusBitmap::~CGdiPlusBitmap()
{
	release();
}

void CGdiPlusBitmap::release()
{
	if (m_pBitmap)
	{
		delete m_pBitmap;
		m_pBitmap = NULL;
	}

	width = height = channel = 0;
}

void CGdiPlusBitmap::resolution()
{
	width = 0;
	height = 0;
	channel = 0;

	if (!m_pBitmap)
		return;

	width = m_pBitmap->GetWidth();
	height = m_pBitmap->GetHeight();
	channel = channels();
}

int CGdiPlusBitmap::channels()
{
	PixelFormat pf = m_pBitmap->GetPixelFormat();

	if (pf == PixelFormat8bppIndexed)
		return 1;
	else if (pf == PixelFormat32bppARGB)
		return 4;

	return 3;
}

CRect CGdiPlusBitmap::draw_image(CDC* pDC, CRect r, CRect* targetRect, Color crBack)
{
	return draw_image(pDC, r.left, r.top, r.Width(), r.Height(), targetRect, crBack);
}

CRect CGdiPlusBitmap::draw_image(CDC* pDC, int x, int y, int w, int h, CRect* targetRect, Color crBack)
{
	Graphics g(pDC->m_hDC);
	g.DrawImage(m_pBitmap, x, y, w, h);
	return CRect(x, y, x + w, y + h);
}

//Clone은 shallow copy라 하여 아래 deep_copy 함수도 추가했으나
//좀 더 확인이 필요하다.
void CGdiPlusBitmap::clone(CGdiPlusBitmap* dst)
{
	dst->release();
	dst->m_pBitmap = m_pBitmap->Clone(0, 0, m_pBitmap->GetWidth(), m_pBitmap->GetHeight(), m_pBitmap->GetPixelFormat());
	dst->resolution();
}

void CGdiPlusBitmap::deep_copy(CGdiPlusBitmap* dst)
{
	dst->release();

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
	dst->resolution();
}

void CGdiPlusBitmap::rotate(Gdiplus::RotateFlipType type)
{
	m_pBitmap->RotateFlip(type);
}

void CGdiPlusBitmap::rotate(float degree, bool auto_enlarge)
{
	int originw = width;
	int originh = height;
	CRect rotated(0, 0, originw, originh);

	if (auto_enlarge)
		std::vector<CPoint> m_pts = get_rotated(originw / 2, originh / 2, &rotated, degree);

	int neww = rotated.Width();
	int newh = rotated.Height();

	//create a new empty bitmap to hold rotated image 
	Bitmap* result = new Bitmap(neww, newh);
	//make a graphics object from the empty bitmap
	Graphics g(result);

	//첫번째 방식이 정상 동작한다.
	//두번째 방식이 동작하려면 뭔가 보정해줘야하는데 일단 보류한다.
	//현재 알고리즘으로는 회전시 계단현상이 발생한다. 옵션을 줘야한다.
	if (true)
	{
		//move rotation point to center of image
		g.TranslateTransform((float)neww / 2, (float)newh / 2);
		//rotate
		g.RotateTransform(degree);
		//move image back
		g.TranslateTransform(-(float)neww / 2, -(float)newh / 2);
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

	//회전에 의해 캔버스는 커지지만 원래 이미지는 원래 크기대로 그려줘야 한다.
	g.DrawImage(m_pBitmap, -rotated.left, -rotated.top, originw, originh);

	release();
	m_pBitmap = result->Clone(0, 0, neww, newh, PixelFormatDontCare);
	delete result;

	width = neww;
	height = newh;
}

void CGdiPlusBitmap::gray()
{
	ColorMatrix colorMatrix = { 0.299f, 0.299f, 0.299f, 0.0f, 0.0f,
								0.587f, 0.587f, 0.587f, 0.0f, 0.0f,
								0.114f, 0.114f, 0.114f, 0.0f, 0.0f,
								0.0f, 0.0f, 0.0f, 1.0, 0.0f,
								0.0f, 0.0f, 0.0f, 0.0f, 1.0f };

	//원본을 복사해 둘 이미지를 준비하고
	CGdiPlusBitmap temp;
	clone(&temp);

	//원래의 이미지로 캔버스를 준비하고 투명하게 비워둔 후
	Graphics g(m_pBitmap);
	g.Clear(Color(0, 0, 0, 0));

	ImageAttributes ia;
	ia.SetColorMatrix(&colorMatrix, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);

	//사본을 ia처리하여 캔버스에 그려준다.
	g.DrawImage(temp, Rect(0, 0, width, height), 0, 0, width, height, UnitPixel, &ia);
}

void CGdiPlusBitmap::negative()
{
	ColorMatrix colorMatrix = { -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
								0.0f, -1.0f, 0.0f, 0.0f, 0.0f,
								0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
								0.0f, 0.0f, 0.0f, 1.0, 0.0f,
								1.0f, 1.0f, 1.0f, 0.0f, 1.0f };

	//원본을 복사해 둘 이미지를 준비하고
	CGdiPlusBitmap temp;
	clone(&temp);

	//원래의 이미지로 캔버스를 준비하고 투명하게 비워둔 후
	Graphics g(m_pBitmap);
	g.Clear(Color(0, 0, 0, 0));

	ImageAttributes ia;
	ia.SetColorMatrix(&colorMatrix, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);

	//사본을 ia처리하여 캔버스에 그려준다.
	g.DrawImage(temp, Rect(0, 0, width, height), 0, 0, width, height, UnitPixel, &ia);
}

void CGdiPlusBitmap::set_transparent(float transparent)
{
	ColorMatrix colorMatrix = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
								0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
								0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
								0.0f, 0.0f, 0.0f, transparent, 0.0f,
								0.0f, 0.0f, 0.0f, 0.0f, 1.0f };

#if 0
	Bitmap* result = new Bitmap(width, height);
	Graphics g(result);

	ImageAttributes ia;

	ia.SetColorMatrix(&colorMatrix, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);
	g.DrawImage(m_pBitmap, Rect(0, 0, width, height), 0, 0, width, height, UnitPixel, &ia);

	delete m_pBitmap;
	m_pBitmap = result->Clone(0, 0, width, height, PixelFormatDontCare);
	delete result;
#else
	//원본을 복사해 둘 이미지를 준비하고
	CGdiPlusBitmap temp;
	clone(&temp);

	//원래의 이미지로 캔버스를 준비하고 투명하게 비워둔 후
	Graphics g(m_pBitmap);
	g.Clear(Color(0, 0, 0, 0));

	ImageAttributes ia;
	ia.SetColorMatrix(&colorMatrix, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);

	//사본을 ia처리하여 캔버스에 그려준다.
	g.DrawImage(temp, Rect(0, 0, width, height), 0, 0, width, height, UnitPixel, &ia);
#endif
}

void CGdiPlusBitmap::set_colorkey(Color low, Color high)
{
	//원본을 복사해 둘 이미지를 준비하고
	CGdiPlusBitmap temp;
	clone(&temp);

	//원래의 이미지로 캔버스를 준비하고 투명하게 비워둔 후
	Graphics g(m_pBitmap);
	g.Clear(Color(0, 0, 0, 0));

	ImageAttributes ia;
	ia.SetColorKey(low, high);

	//사본을 ia처리하여 캔버스에 그려준다.
	g.DrawImage(m_pBitmap, Rect(0, 0, width, height), 0, 0, width, height, UnitPixel, &ia);
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
