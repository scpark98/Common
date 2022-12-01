#include "GdiplusBitmap.h"
#include "../Common/Functions.h"

CGdiplusBitmap::CGdiplusBitmap()
{
	release();
}

CGdiplusBitmap::CGdiplusBitmap(Bitmap* src)
{
	release();
	m_pBitmap = src;
}

CGdiplusBitmap::CGdiplusBitmap(HBITMAP hBitmap)
{
	release();

	DIBSECTION dib = {};
	if (::GetObject(hBitmap, sizeof(dib), &dib) == sizeof(DIBSECTION) && dib.dsBm.bmBitsPixel == 32)
	{
		m_pBitmap = CreateARGBBitmapFromDIB(dib);
	}
	else
	{
		m_pBitmap = Bitmap::FromHBITMAP(hBitmap, NULL);
	}

	/*
	BITMAP source_info = { 0 };
	if (!::GetObject(hBitmap, sizeof(source_info), &source_info))
		return;

	Gdiplus::Status s;
	Gdiplus::BitmapData target_info;
	Gdiplus::Rect rect(0, 0, source_info.bmWidth, source_info.bmHeight);

	Gdiplus::Bitmap* target(new Gdiplus::Bitmap(source_info.bmWidth, source_info.bmHeight, PixelFormat32bppRGB));
	if (!target)
		return;
	if ((s = target->GetLastStatus()) != Gdiplus::Ok)
		return;

	s = target->LockBits(&rect, Gdiplus::ImageLockModeWrite, PixelFormat32bppRGB, &target_info);
	if (s != Gdiplus::Ok)
		return;

	if (target_info.Stride != source_info.bmWidthBytes)
		return; // pixel_format is wrong!

	CopyMemory(target_info.Scan0, source_info.bmBits, source_info.bmWidthBytes * source_info.bmHeight);

	s = target->UnlockBits(&target_info);
	if (s != Gdiplus::Ok)
		return;

	m_pBitmap = target;
	*/

	resolution();
}

CGdiplusBitmap::CGdiplusBitmap(CString sFile, bool show_error)
{
	load(sFile);
}

CGdiplusBitmap::CGdiplusBitmap(CGdiplusBitmap* src)
{
	src->deep_copy(this);
	resolution();
}

CGdiplusBitmap::CGdiplusBitmap(CString lpType, UINT id, bool show_error)
{
	load(lpType, id);
}

bool CGdiplusBitmap::load(CString sFile, bool show_error)
{
	release();

	CGdiplusBitmap temp;
	
	//멀티바이트 환경에서 CString2LPCWSTR()를 써서
	//LPCWSTR로 바꿨으나 일부 파일 열기 실패하는 현상 발생.
	//위 함수를 호출하지 않고 해당 코드를 그대로 쓰면 문제 없음.
	//LPCWSTR wFile = CString2LPCWSTR(sFile);
	//temp.m_pBitmap = Gdiplus::Bitmap::FromFile(wFile);

#ifdef UNICODE
	temp.m_pBitmap = Gdiplus::Bitmap::FromFile(sFile);
#else
	USES_CONVERSION;
	LPCWSTR wFile = A2W(sFile);
	temp.m_pBitmap = Gdiplus::Bitmap::FromFile(wFile);
#endif


	if (!temp.empty())// m_pBitmap->GetLastStatus() == Gdiplus::Ok)
	{
		//temp로 읽어서 deep_copy해주지 않으면
		//열린 파일은 lock걸린 상태가 되어 접근할 수 없게 된다.
		temp.deep_copy(this);
		resolution();

		if (width == 0 || height == 0)
		{
			if (show_error)
			{
				CString str;
				str.Format(_T("%s\nImage width or height is 0."));
				AfxMessageBox(str);
			}
			return false;
		}

		return true;
	}

	if (show_error)
	{
		CString str;
		str.Format(_T("%s\nFile open failed."), sFile);
		AfxMessageBox(str);
	}
	return false;
}

void CGdiplusBitmap::load(CString sType, UINT id, bool show_error)
{
	release();

	sType.MakeLower();

	if (sType == _T("png") || sType == _T("jpg"))
	{
		m_pBitmap = GetImageFromResource(sType, id);
	}
	else
	{
		m_pBitmap = Bitmap::FromResource(NULL, (WCHAR*)MAKEINTRESOURCE(id));
	}

	if (m_pBitmap)
	{
		resolution();
	}
}

Bitmap* CGdiplusBitmap::GetImageFromResource(CString sType, UINT id)
{
	HRSRC hResource = FindResource(NULL, MAKEINTRESOURCE(id), sType);

	if (!hResource)
		return NULL;

	DWORD imageSize = SizeofResource(AfxGetApp()->m_hInstance, hResource);
	HGLOBAL hGlobal = LoadResource(AfxGetApp()->m_hInstance, hResource);
	LPVOID pData = LockResource(hGlobal);
	HGLOBAL hBuffer = GlobalAlloc(GMEM_MOVEABLE, imageSize);
	LPVOID pBuffer = GlobalLock(hBuffer);

	CopyMemory(pBuffer, pData, imageSize);
	GlobalUnlock(hBuffer);

	IStream* pStream;
	HRESULT hr = CreateStreamOnHGlobal(hBuffer, TRUE, &pStream);
	Image image(pStream);
	pStream->Release();

	if (image.GetLastStatus() != Ok)
		return NULL;

	Bitmap* pBitmap = static_cast<Bitmap*>(image.Clone());

	return pBitmap;
}

CGdiplusBitmap::~CGdiplusBitmap()
{
	release();
}

void CGdiplusBitmap::release()
{
	SAFE_DELETE(m_pBitmap);
	SAFE_DELETE_ARRAY(data);
	width = height = channel = stride = 0;
}

//---------------------------------------------------------------
// Create a 32bpp ARGB Gdiplus::Bitmap from a DIBSECTION
//---------------------------------------------------------------
Gdiplus::Bitmap* CGdiplusBitmap::CreateARGBBitmapFromDIB(const DIBSECTION& dib)
{
	int width = dib.dsBmih.biWidth;
	int pitch = dib.dsBm.bmWidthBytes;
	int height = dib.dsBmih.biHeight;
	BYTE* pBits = static_cast<BYTE*>(dib.dsBm.bmBits);

	if (height < 0)
	{
		// Top-down DIB
		height = -height;
	}
	else
	{
		// Bottom-up. Adjust the Scan0 to the start of the last row
		pBits += (height - 1) * pitch;
		// and set the pitch to a -ve value
		pitch = -pitch;
	}

#pragma push_macro("new")
#undef new
	return new Gdiplus::Bitmap(width, height, pitch, PixelFormat32bppARGB, pBits);
#pragma pop_macro("new")
}

void CGdiplusBitmap::resolution()
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

bool CGdiplusBitmap::get_raw_data()
{
	SAFE_DELETE_ARRAY(data);

	Gdiplus::Rect rect(0, 0, m_pBitmap->GetWidth(), m_pBitmap->GetHeight()); //크기구하기
	Gdiplus::BitmapData bmpData; //비트맵데이터 객체

	PixelFormat format = PixelFormat24bppRGB;
	format = m_pBitmap->GetPixelFormat();
	if (m_pBitmap->GetPixelFormat() == PixelFormat8bppIndexed)
		format = PixelFormat8bppIndexed;
	else
		format = PixelFormat24bppRGB;

	if (m_pBitmap->LockBits(&rect,
		Gdiplus::ImageLockModeRead,
		format/*m_pBitmap->GetPixelFormat()*/, &bmpData) == Gdiplus::Ok) { //픽셀포맷형식에따라 이미지접근권한 취득

		int len = bmpData.Height * std::abs(bmpData.Stride); //이미지 전체크기
		data = new BYTE[len]; //할당
		memcpy(data, bmpData.Scan0, len); //복사
		stride = bmpData.Stride;
		m_pBitmap->UnlockBits(&bmpData); //락 풀기
		return true;
	}

	return false;
}

bool CGdiplusBitmap::empty()
{
	return (m_pBitmap == NULL);
}

bool CGdiplusBitmap::valid()
{
	return !empty();
}

int CGdiplusBitmap::channels()
{
	PixelFormat pf = m_pBitmap->GetPixelFormat();

	if (pf == PixelFormat8bppIndexed)
		return 1;
	else if (pf == PixelFormat32bppARGB || pf == PixelFormat32bppRGB)
		return 4;

	return 3;
}

CRect CGdiplusBitmap::draw(CDC* pDC, CRect r, CRect* targetRect, Color crBack)
{
	return draw(pDC, r.left, r.top, r.Width(), r.Height(), targetRect, crBack);
}

CRect CGdiplusBitmap::draw(CDC* pDC, int dx, int dy, int dw, int dh, CRect* targetRect, Color crBack)
{
	if (dw <= 0)
		dw = width;
	if (dh <= 0)
		dh = height;

	Graphics g(pDC->m_hDC);
	g.DrawImage(m_pBitmap, dx, dy, dw, dh);
	return CRect(dx, dy, dx + dw, dy + dh);
}

//Clone은 shallow copy라 하여 아래 deep_copy 함수도 추가했으나
//좀 더 확인이 필요하다.
//파일을 그냥 열면 탐색기 등에서 해당 파일을 지우거나 변경할 수 없는데
//temp로 열고 deep_copy를 하니 그런 문제가 사라짐.
void CGdiplusBitmap::clone(CGdiplusBitmap* dst)
{
	dst->release();
	dst->m_pBitmap = m_pBitmap->Clone(0, 0, m_pBitmap->GetWidth(), m_pBitmap->GetHeight(), m_pBitmap->GetPixelFormat());
	dst->resolution();
}

void CGdiplusBitmap::deep_copy(CGdiplusBitmap* dst)
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

//좌우대칭
void CGdiplusBitmap::mirror()
{
	bool has_data = false;
	if (data)
	{
		has_data = true;
		SAFE_DELETE_ARRAY(data);
	}

	m_pBitmap->RotateFlip(Gdiplus::RotateNoneFlipX);

	if (has_data)
		get_raw_data();
}

//상하대칭
void CGdiplusBitmap::flip()
{
	bool has_data = false;
	if (data)
	{
		has_data = true;
		SAFE_DELETE_ARRAY(data);
	}

	m_pBitmap->RotateFlip(Gdiplus::RotateNoneFlipY);

	if (has_data)
		get_raw_data();
}

void CGdiplusBitmap::rotate(Gdiplus::RotateFlipType type)
{
	SAFE_DELETE_ARRAY(data);

	m_pBitmap->RotateFlip(type);
	resolution();
}

void CGdiplusBitmap::rotate(float degree, bool auto_resize, Color remove_back_color)
{
	int originw = width;
	int originh = height;
	CRect rotated(0, 0, originw, originh);

	if (auto_resize)
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

	//save(_T("d:\\temp\\rotated.png"));

	//회전시키면 캔버스의 크기가 커지거나 작아지는데
	//작아졌을 경우 불필요한 여백이 유지되므로 이를 제거한다.
	if (auto_resize)
	{
		//fit_to_image(remove_back_color);
	}

	resolution();

	//save(_T("d:\\temp\\rotated_fit.png"));
}

bool is_equal(Color cr0, Color cr1, int channel)
{
	bool equal = true;
	//if (cr0.)
	return equal;
}

void CGdiplusBitmap::fit_to_image(Color remove_back_color)
{
	int x, y;
	CRect r(0, 0, width, height);

	Color pixel;
	bool found = false;

	long t0 = clock();

	/* 전체 검사는 100~200ms 소요
	bool upperleftfound = false;
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			m_pBitmap->GetPixel(x, y, &pixel);
			if (pixel.GetValue() != remove_back_color.GetValue())
			{
				if (!upperleftfound)
				{
					r.left = r.right = x;
					r.top = r.bottom = y;
					upperleftfound = true;
				}
				else
				{
					if (x > r.right)
						r.right = x;
					else if (x < r.left)
						r.left = x;
					if (y > r.bottom)
						r.bottom = y;
				}
			}
		}
	}
	*/

	// in debug mode : 50~100ms	
	//left
	found = false;
	for (x = 0; x < width; x++)
	{
		for (y = 0; y < height; y++)
		{
			m_pBitmap->GetPixel(x, y, &pixel);
			if (pixel.GetValue() != remove_back_color.GetValue())
			{
				r.left = x;
				found = true;
				break;
			}
		}

		if (found)
			break;
	}

	//top
	found = false;
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			m_pBitmap->GetPixel(x, y, &pixel);
			if (pixel.GetValue() != remove_back_color.GetValue())
			{
				r.top = y;
				found = true;
				break;
			}
		}

		if (found)
			break;
	}

	//right
	found = false;
	for (x = width - 1; x >= 0; x--)
	{
		for (y = 0; y < height; y++)
		{
			m_pBitmap->GetPixel(x, y, &pixel);
			if (pixel.GetValue() != remove_back_color.GetValue())
			{
				r.right = x;
				found = true;
				break;
			}
		}

		if (found)
			break;
	}

	//bottom
	found = false;
	for (y = height - 1; y >= 0; y--)
	{
		for (x = 0; x < width; x++)
		{
			m_pBitmap->GetPixel(x, y, &pixel);
			if (pixel.GetValue() != remove_back_color.GetValue())
			{
				r.bottom = y;
				found = true;
				break;
			}
		}

		if (found)
			break;
	}

	TRACE(_T("fit to image = %ld ms\n"), clock() - t0);

	sub_image(r);
}

void CGdiplusBitmap::resize(int cx, int cy, InterpolationMode mode)
{
	Bitmap* result = new Bitmap(cx, cy);
	Graphics g(result);

	g.SetInterpolationMode(mode);

	g.DrawImage(m_pBitmap, Rect(0, 0, cx, cy), 0, 0, width, height, UnitPixel);

	delete m_pBitmap;
	m_pBitmap = result->Clone(0, 0, cx, cy, PixelFormatDontCare);
	delete result;

	resolution();
}

void CGdiplusBitmap::sub_image(CRect r)
{
	sub_image(r.left, r.top, r.Width(), r.Height());
}

void CGdiplusBitmap::sub_image(Gdiplus::Rect r)
{
	sub_image(r.X, r.Y, r.Width, r.Height);
}

void CGdiplusBitmap::sub_image(int x, int y, int w, int h)
{
	Bitmap* result = new Bitmap(w, h);
	Graphics g(result);

	g.DrawImage(m_pBitmap, Rect(0, 0, w, h), x, y, w, h, UnitPixel);

	delete m_pBitmap;
	m_pBitmap = result->Clone(0, 0, w, h, PixelFormatDontCare);
	delete result;

	resolution();
}

//회색톤으로 변경만 할 뿐 실제 픽셀포맷은 변경되지 않는다.
void CGdiplusBitmap::gray()
{
	ColorMatrix colorMatrix = { 0.299f, 0.299f, 0.299f, 0.0f, 0.0f,
								0.587f, 0.587f, 0.587f, 0.0f, 0.0f,
								0.114f, 0.114f, 0.114f, 0.0f, 0.0f,
								0.0f, 0.0f, 0.0f, 1.0, 0.0f,
								0.0f, 0.0f, 0.0f, 0.0f, 1.0f };

	//원본을 복사해 둘 이미지를 준비하고
	CGdiplusBitmap temp;
	clone(&temp);

	//원래의 이미지로 캔버스를 준비하고 투명하게 비워둔 후
	Graphics g(m_pBitmap);
	g.Clear(Color(0, 0, 0, 0));

	ImageAttributes ia;
	ia.SetColorMatrix(&colorMatrix, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);

	//사본을 ia처리하여 캔버스에 그려준다.
	g.DrawImage(temp, Rect(0, 0, width, height), 0, 0, width, height, UnitPixel, &ia);
}

void CGdiplusBitmap::negative()
{
	ColorMatrix colorMatrix = { -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
								0.0f, -1.0f, 0.0f, 0.0f, 0.0f,
								0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
								0.0f, 0.0f, 0.0f, 1.0, 0.0f,
								1.0f, 1.0f, 1.0f, 0.0f, 1.0f };

	//원본을 복사해 둘 이미지를 준비하고
	CGdiplusBitmap temp;
	clone(&temp);

	//원래의 이미지로 캔버스를 준비하고 투명하게 비워둔 후
	Graphics g(m_pBitmap);
	g.Clear(Color(0, 0, 0, 0));

	ImageAttributes ia;
	ia.SetColorMatrix(&colorMatrix, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);

	//사본을 ia처리하여 캔버스에 그려준다.
	g.DrawImage(temp, Rect(0, 0, width, height), 0, 0, width, height, UnitPixel, &ia);
}

//실제 8bit(256color) gray이미지로 변경해준다.
void CGdiplusBitmap::convert2gray()
{
	Bitmap* bitmap = new Bitmap(width, height, PixelFormat8bppIndexed);
	//Bitmap* bitmap = new Bitmap(width, height, PixelFormat24bppRGB);
	int palSize = bitmap->GetPaletteSize();
	
	ColorPalette *palette = (ColorPalette*)malloc(palSize);

	bitmap->GetPalette(palette, palSize);
	palette->Flags = PaletteFlagsGrayScale;
	palette->Count = 256;
	for (int i = 0; i < 256; i++)
	{
		palette->Entries[i] = Gdiplus::Color::MakeARGB((BYTE)255, i, i, i);
	}
	bitmap->SetPalette(palette);
	//*/

	BitmapData bmData_src;
	BitmapData bmData_dst;
	Rect rect(0, 0, width, height);

	gray();

	m_pBitmap->LockBits(&rect,
		ImageLockModeRead,
		/*PixelFormat24bppRGB,//*/m_pBitmap->GetPixelFormat(),
		&bmData_src);

	bitmap->LockBits(&rect,
		ImageLockModeRead,
		bitmap->GetPixelFormat(),
		&bmData_dst);

	uint8_t* src = (uint8_t*)bmData_src.Scan0;
	uint8_t* dst = (uint8_t*)bmData_dst.Scan0;

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			//uint8_t r = src[y * width * channel + x * channel + 0];
			//uint8_t g = src[y * width * channel + x * channel + 1];
			//uint8_t b = src[y * width * channel + x * channel + 2];
			//TRACE(_T("[%d][%d] = (%d, %d, %d)\n"), y, x, r, g, b);
			//dst[y * width * channel + x * channel + 0] = src[y * width * channel + x * channel + 2];
			//dst[y * width * channel + x * channel + 1] = src[y * width * channel + x * channel + 1];
			//dst[y * width * channel + x * channel + 2] = src[y * width * channel + x * channel + 0];
			dst[y * width + x] = src[y * width * channel + x * channel + 0];
		}
	}

	m_pBitmap->UnlockBits(&bmData_src);
	bitmap->UnlockBits(&bmData_dst);

	SAFE_DELETE(m_pBitmap);
	SAFE_DELETE_ARRAY(data);

	m_pBitmap = bitmap;
	//data = dst;
	channel = 1;
	stride = width * channel;

	//save(_T("d:\\temp\\gray.bmp"));
}

void CGdiplusBitmap::set_matrix(ColorMatrix* colorMatrix, ColorMatrix* grayMatrix)
{
	//원본을 복사해 둘 이미지를 준비하고
	CGdiplusBitmap temp;
	clone(&temp);

	//원래의 이미지로 캔버스를 준비하고 투명하게 비워둔 후
	Graphics g(m_pBitmap);
	g.Clear(Color(0, 0, 0, 0));

	ImageAttributes ia;

	if (grayMatrix)
		ia.SetColorMatrices(colorMatrix, grayMatrix, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);
	else
		ia.SetColorMatrix(colorMatrix, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);

	//사본을 ia처리하여 캔버스에 그려준다.
	g.DrawImage(temp, Rect(0, 0, width, height), 0, 0, width, height, UnitPixel, &ia);
}

void CGdiplusBitmap::set_transparent(float transparent)
{
	ColorMatrix colorMatrix = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
								0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
								0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
								0.0f, 0.0f, 0.0f, transparent, 0.0f,
								0.0f, 0.0f, 0.0f, 0.0f, 1.0f };

	//new를 사용하지 않는 아래 방식으로 할 경우
	//기존 32비트 또는 24비트 YUV의 png, jpg는 투명하게 되나
	//RGB24였던 이미지는 투명하게 안된다.
	//우선 new 방식으로 한다.
#if 1

#if	(GDIPVER >= 0x0110)
	//m_pBitmap->ConvertFormat();
#endif

	Bitmap* result = new Bitmap(width, height, PixelFormat32bppARGB);
	Graphics g(result);

	ImageAttributes ia;

	ia.SetColorMatrix(&colorMatrix, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);
	g.DrawImage(m_pBitmap, Rect(0, 0, width, height), 0, 0, width, height, UnitPixel, &ia);

	delete m_pBitmap;
	m_pBitmap = result->Clone(0, 0, width, height, PixelFormatDontCare);
	delete result;
#else
	//원본을 복사해 둘 이미지를 준비하고
	CGdiplusBitmap temp;
	deep_copy(&temp);

	//m_pBitmap->ConvertFormat();

	//원래의 이미지로 캔버스를 준비하고 투명하게 비워둔 후
	Graphics g(m_pBitmap);
	g.Clear(Color(0, 0, 0, 0));

	ImageAttributes ia;
	ia.SetColorMatrix(&colorMatrix);// , ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);

	//사본을 ia처리하여 캔버스에 그려준다.
	g.DrawImage(temp, Rect(0, 0, width, height), 0, 0, width, height, UnitPixel, &ia);
#endif
}

void CGdiplusBitmap::set_colorkey(Color low, Color high)
{
	//원본을 복사해 둘 이미지를 준비하고
	CGdiplusBitmap temp;
	clone(&temp);

	//원래의 이미지로 캔버스를 준비하고 투명하게 비워둔 후
	Graphics g(m_pBitmap);
	g.Clear(Color(0, 0, 0, 0));

	ImageAttributes ia;
	ia.SetColorKey(low, high);

	//사본을 ia처리하여 캔버스에 그려준다.
	g.DrawImage(m_pBitmap, Rect(0, 0, width, height), 0, 0, width, height, UnitPixel, &ia);
}

int CGdiplusBitmap::GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
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

bool CGdiplusBitmap::save(CString filename)//, ULONG quality/* = 100*/)
{
	if (empty())
		return false;

	CLSID				encoderClsid;

	CString ext = filename.Right(3).MakeLower();//GetFileExtension(filename).MakeLower();

	if (ext == _T("jpg") || ext == _T("jpeg"))
		GetEncoderClsid(L"image/jpeg", &encoderClsid);
	else if (ext == _T("png"))
		GetEncoderClsid(L"image/png", &encoderClsid);
	else if (ext == _T("bmp"))
		GetEncoderClsid(L"image/bmp", &encoderClsid);
	else
	{
		AfxMessageBox(_T("처리 코드가 추가되지 않은 포맷. 코드 수정 필요"));
		return false;
	}
	/*
	encoderParameters.Count = 1;
	encoderParameters.Parameter[0].Guid = EncoderQuality;
	encoderParameters.Parameter[0].Type = EncoderParameterValueTypeLong;
	encoderParameters.Parameter[0].NumberOfValues = 1;

	// Save the image as a JPEG with quality level 0.
	encoderParameters.Parameter[0].Value = &quality;
	*/
	Status s;

	s = m_pBitmap->Save(CStringW(filename), &encoderClsid);// , & encoderParameters);

	if (s == Ok)
		return true;

	return false;
}

bool CGdiplusBitmap::copy_to_clipbard()
{
	bool res = false;
	HBITMAP hbitmap;
	auto status = m_pBitmap->GetHBITMAP(NULL, &hbitmap);
	if (status != Gdiplus::Ok)
		return false;
	BITMAP bm;
	GetObject(hbitmap, sizeof bm, &bm);

	BITMAPINFOHEADER bi =
	{ sizeof bi, bm.bmWidth, bm.bmHeight, 1, bm.bmBitsPixel, BI_RGB };

	std::vector<BYTE> vec(bm.bmWidthBytes * bm.bmHeight);
	auto hdc = GetDC(NULL);
	GetDIBits(hdc, hbitmap, 0, bi.biHeight, vec.data(), (BITMAPINFO*)&bi, 0);
	ReleaseDC(NULL, hdc);

	auto hmem = GlobalAlloc(GMEM_MOVEABLE, sizeof bi + vec.size());
	auto buffer = (BYTE*)GlobalLock(hmem);
	memcpy(buffer, &bi, sizeof bi);
	memcpy(buffer + sizeof bi, vec.data(), vec.size());
	GlobalUnlock(hmem);

	if (OpenClipboard(NULL))
	{
		EmptyClipboard();
		SetClipboardData(CF_DIB, hmem);
		CloseClipboard();
		MessageBeep(0);
		res = true;
	}

	DeleteObject(hbitmap);

	return res;
}