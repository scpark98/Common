#include "GdiplusBitmap.h"


#include "Functions.h"
#include "MemoryDC.h"

#include <thread>
#include <afxcmn.h>

#include "image_processing/fast_gaussian_blur/fast_gaussian_blur_template.h"


static CGdiplusDummyForInitialization gdi_dummy_for_gdi_initialization;

CGdiplusBitmap::CGdiplusBitmap()
{
	release();
}

CGdiplusBitmap::CGdiplusBitmap(Gdiplus::Bitmap* src)
{
	release();

	m_pBitmap = new Gdiplus::Bitmap(src->GetWidth(), src->GetHeight(), src->GetPixelFormat());
	Gdiplus::Rect rect(0, 0, src->GetWidth(), src->GetHeight());
	Gdiplus::BitmapData bmpData;
	m_pBitmap->LockBits(&rect, Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeWrite, m_pBitmap->GetPixelFormat(), &bmpData);
	src->LockBits(&rect, Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeUserInputBuf, m_pBitmap->GetPixelFormat(), &bmpData);

	src->UnlockBits(&bmpData);
	m_pBitmap->UnlockBits(&bmpData);

	resolution();
}

CGdiplusBitmap::CGdiplusBitmap(HBITMAP hBitmap)
{
	release();

	DIBSECTION ds = {};
	if (!::GetObject(hBitmap, sizeof(ds), &ds))
		return;

	if (ds.dsBm.bmBits && (ds.dsBm.bmBitsPixel == 32) && (ds.dsBmih.biCompression == BI_RGB))
	{
		m_pBitmap = CreateARGBBitmapFromDIB(ds);
	}
	else
	{
		m_pBitmap = Gdiplus::Bitmap::FromHBITMAP(hBitmap, NULL);
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

//gif이미지를 각 프레임별로 stream을 구하고 이를 로딩하려 했으나
//gdiplus에서는 한번에 gif를 로딩하는 api를 제공하므로
//이 함수를 사용하지 않는다.
CGdiplusBitmap::CGdiplusBitmap(IStream* pStream)
{
	release();

	Gdiplus::Bitmap* temp = Gdiplus::Bitmap::FromStream(pStream, TRUE);

	m_pBitmap = new Gdiplus::Bitmap(temp->GetWidth(), temp->GetHeight(), PixelFormat32bppARGB);

	Gdiplus::Graphics g(m_pBitmap);

	Gdiplus::SolidBrush brush_tr(Gdiplus::Color(0, 0, 0, 0));
	g.FillRectangle(&brush_tr, 0, 0, temp->GetWidth(), temp->GetHeight());

	g.DrawImage(temp, 0, 0);

	SAFE_DELETE(temp);

	resolution();
}

CGdiplusBitmap::CGdiplusBitmap(CString sFile)
{
	load(sFile);
}

CGdiplusBitmap::CGdiplusBitmap(CGdiplusBitmap* src)
{
	src->deep_copy(this);
	resolution();
}

CGdiplusBitmap::CGdiplusBitmap(CString lpType, UINT id)
{
	load(lpType, id);
}

CGdiplusBitmap::CGdiplusBitmap(int cx, int cy, Gdiplus::PixelFormat format, Gdiplus::Color cr)
{
	release();

	m_pBitmap = new Gdiplus::Bitmap(cx, cy, format);
	Gdiplus::Graphics g(m_pBitmap);
	g.Clear(cr);
	resolution();
}

void CGdiplusBitmap::create(int cx, int cy, Gdiplus::PixelFormat format, Gdiplus::Color cr)
{
	release();

	m_pBitmap = new Gdiplus::Bitmap(cx, cy, format);
	Gdiplus::Graphics g(m_pBitmap);
	g.Clear(cr);
	resolution();
}

//CTreeCtrl, CListCtrl등에서 선택된 항목 자체를 이미지로 리턴(drag시에 사용)
void CGdiplusBitmap::create_drag_image(CWnd* pWnd)
{
	release();

	if (pWnd->IsKindOf(RUNTIME_CLASS(CTreeCtrl)))
	{
		CTreeCtrl* ctrl = (CTreeCtrl*)pWnd;
		HTREEITEM hItem = ctrl->GetSelectedItem();
		if (hItem == NULL)
			return;

		//CImageList* imglist = ctrl->CreateDragImage(hItem);

		CString text = ctrl->GetItemText(hItem);
		CRect rItem;

		ctrl->GetItemRect(hItem, rItem, TRUE);
		m_pBitmap = new Gdiplus::Bitmap(rItem.Width(), rItem.Height(), PixelFormat32bppARGB);

		CFont* font = ctrl->GetFont();

		Gdiplus::Graphics g(m_pBitmap);

		//draw_text(0, 0, text, 12, 1);

		resolution();
	}
}

bool CGdiplusBitmap::load(CString file, bool gif_play_in_this)
{
	if (!PathFileExists(file))
		return false;

	release();

	CGdiplusBitmap temp;
	
	//멀티바이트 환경에서 CString2LPCWSTR()를 써서
	//LPCWSTR로 바꿨으나 일부 파일 열기 실패하는 현상 발생.
	//위 함수를 호출하지 않고 해당 코드를 그대로 쓰면 문제 없음.
	//LPCWSTR wFile = CString2LPCWSTR(sFile);
	//temp.m_pBitmap = Gdiplus::Bitmap::FromFile(wFile);

	//temp로 읽어서 deep_copy해주지 않으면
	//열린 파일은 lock걸린 상태가 되어 삭제 또는 파일명 변경 등의 수정이 불가능하다.
	//단점은 animatedGif 파일일 경우 그와 관련된 정보를 얻을 수 없다.
	//deep_copy()함수로도 모든 정보가 복사되진 않는다.
	//우선은 gif인 경우만 직접 열고 그 외의 포맷은 copy방식으로 열도록 한다.
	//(이 규칙은 외부 파일 로딩일 경우에만 해당됨)
	bool use_copied_open = true;
	
	if (get_part(file, fn_ext).MakeLower() == _T("gif"))
	{
		m_gif_play_in_this = gif_play_in_this;
		use_copied_open = false;
	}

#ifdef UNICODE
	if (use_copied_open)
		temp.m_pBitmap = Gdiplus::Bitmap::FromFile(file);// Gdiplus::Bitmap(sFile);
	else
		m_pBitmap = Gdiplus::Bitmap::FromFile(file); //new Gdiplus::Bitmap(sFile);

	//GdipLoadImageFromFile()
#else
	CA2W wFile(file);
	if (use_copied_open)
		temp.m_pBitmap = Gdiplus::Bitmap::FromFile(wFile);
	else
		m_pBitmap = Gdiplus::Bitmap::FromFile(wFile);
#endif

	bool open_success = false;

	if (use_copied_open)
	{
		if (temp.m_pBitmap)
			open_success = true;
	}
	else if (m_pBitmap && (m_pBitmap->GetLastStatus() == Gdiplus::Ok))
	{
		open_success = true;
	}

	if (open_success)
	{
		if (use_copied_open)
			temp.deep_copy(this);

		resolution();

		if (width == 0 || height == 0)
			return false;

		m_filename = file;
		return true;
	}

	return false;
}

//png일 경우는 sType을 생략할 수 있다.
bool CGdiplusBitmap::load(UINT id, bool gif_play_in_this)
{
	if (id <= 0)
		return false;

	return load(_T("PNG"), id, gif_play_in_this);
}

bool CGdiplusBitmap::load(CString sType, UINT id, bool gif_play_in_this)
{
	release();

	if (id <= 0)
		return false;

	sType.MakeLower();

	if (sType == _T("png") || sType == _T("jpg") || sType == _T("gif"))
	{
		m_gif_play_in_this = gif_play_in_this;
		m_pBitmap = GetImageFromResource(sType, id);
	}
	else
	{
		m_pBitmap = Gdiplus::Bitmap::FromResource(NULL, (WCHAR*)MAKEINTRESOURCE(id));
	}

	if (!m_pBitmap)
		return false;

	m_filename = _T("resource_image.") + sType;
	resolution();
	return true;
}

bool CGdiplusBitmap::load_icon(UINT id, int size)
{
	release();

	HICON hIcon = ::load_icon(NULL, id, size);
	if (hIcon == NULL)
		return false;

	//FromHICON()을 이용하면 16bit로? 로드되므로 깔끔하게 그려지지 않는다.
	//m_pBitmap = Gdiplus::Bitmap::FromHICON(hIcon);

	//실패
	//m_pBitmap = GetImageFromResource(_T("ICO"), id);

	//직접 m_pBitmap을 생성하고 DC를 얻어 그려주는 방법 시도.
	create(size, size);
	Gdiplus::Graphics g(m_pBitmap);

	g.SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);
	g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);

	HDC hdc = g.GetHDC();
	// Make GDI calls, but don't call any methods
	// on g until after the call to ReleaseHDC.
	::DrawIconEx(hdc, 0, 0, hIcon, size, size, 0, NULL, DI_NORMAL);
	g.ReleaseHDC(hdc);

	//g.DrawImage(m_pBitmap, Gdiplus::Rect(0, 0, size, size));


	m_filename = _T("resource ico image.");
	resolution();

	save(_T("d:\\ico.png"));
	return true;
}

Gdiplus::Bitmap* CGdiplusBitmap::GetImageFromResource(CString sType, UINT id)
{
	HRSRC hResource;

	//ICO를 이렇게 바로 로딩하는 것은 일단 실패함.
	if (sType == _T("ICO"))
	{
		//HMODULE hMod = LoadLibraryExW(L"C:\\Windows\\SysWOW64\\explorer.exe",
		//	NULL, LOAD_LIBRARY_AS_IMAGE_RESOURCE);
		hResource = FindResource(AfxGetInstanceHandle(), MAKEINTRESOURCE(id), RT_ICON);
	}
	else
	{
		hResource = FindResource(NULL, MAKEINTRESOURCE(id), sType);
	}

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
	Gdiplus::Image image(pStream);
	pStream->Release();

	if (image.GetLastStatus() != Gdiplus::Ok)
		return NULL;

	Gdiplus::Bitmap* pBitmap = static_cast<Gdiplus::Bitmap*>(image.Clone());

	return pBitmap;
}

IStream* CGdiplusBitmap::CreateStreamOnFile(LPCTSTR pszPathName)
{
	HANDLE hFile = ::CreateFile(pszPathName,
		FILE_READ_DATA,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (!hFile)
		return NULL;

	DWORD len = ::GetFileSize(hFile, NULL); // only 32-bit of the actual file size is retained
	if (len == 0)
		return NULL;

	HGLOBAL hGlobal = ::GlobalAlloc(GMEM_MOVEABLE, len);
	if (!hGlobal)
	{
		::CloseHandle(hFile);
		return NULL;
	}

	char* lpBuffer = reinterpret_cast<char*> (::GlobalLock(hGlobal));
	DWORD dwBytesRead = 0;

	while (::ReadFile(hFile, lpBuffer, 4096, &dwBytesRead, NULL))
	{
		lpBuffer += dwBytesRead;
		if (dwBytesRead == 0)
			break;
		dwBytesRead = 0;
	}

	::CloseHandle(hFile);


	::GlobalUnlock(hGlobal);

	// don't delete memory on object's release
	IStream* pStream = NULL;
	if (::CreateStreamOnHGlobal(hGlobal, FALSE, &pStream) != S_OK)
	{
		::GlobalFree(hGlobal);
		return NULL;
	}

	return pStream;
}

IStream* CGdiplusBitmap::CreateStreamOnResource(LPCTSTR lpName, LPCTSTR lpType)
{
	// initialize return value
	IStream* ipStream = NULL;

	// find the resource
	HRSRC hrsrc = FindResource(NULL, lpName, lpType);
	if (hrsrc == NULL)
		return NULL;

	// load the resource
	DWORD dwResourceSize = SizeofResource(NULL, hrsrc);
	HGLOBAL hglbImage = LoadResource(NULL, hrsrc);
	if (hglbImage == NULL)
		return NULL;

	// lock the resource, getting a pointer to its data
	LPVOID pvSourceResourceData = LockResource(hglbImage);
	if (pvSourceResourceData == NULL)
		return NULL;

	// allocate memory to hold the resource data
	HGLOBAL hgblResourceData = GlobalAlloc(GMEM_MOVEABLE, dwResourceSize);
	if (hgblResourceData == NULL)
		return NULL;

	// get a pointer to the allocated memory
	LPVOID pvResourceData = GlobalLock(hgblResourceData);
	if (pvResourceData == NULL)
	{
		GlobalFree(hgblResourceData);
		return NULL;
	}

	// copy the data from the resource to the new memory block
	CopyMemory(pvResourceData, pvSourceResourceData, dwResourceSize);
	GlobalUnlock(hgblResourceData);

	// create a stream on the HGLOBAL containing the data
	if (SUCCEEDED(CreateStreamOnHGlobal(hgblResourceData, TRUE, &ipStream)))
		return ipStream;

	// couldn't create stream; free the memory
	GlobalFree(hgblResourceData);

	// no need to unlock or free the resource
	return ipStream;
}

IWICBitmapSource* CGdiplusBitmap::LoadBitmapFromStream(IStream* ipImageStream)
{
	// initialize return value
	IWICBitmapSource* ipBitmap = NULL;

	// load WIC's PNG decoder
	IWICBitmapDecoder* ipDecoder = NULL;
	if (FAILED(CoCreateInstance(CLSID_WICPngDecoder, NULL, CLSCTX_INPROC_SERVER, __uuidof(ipDecoder), reinterpret_cast<void**>(&ipDecoder))))
		return NULL;

	// load the PNG
	if (FAILED(ipDecoder->Initialize(ipImageStream, WICDecodeMetadataCacheOnLoad)))
	{
		ipDecoder->Release();
		return NULL;
	}

	// check for the presence of the first frame in the bitmap
	UINT nFrameCount = 0;
	if (FAILED(ipDecoder->GetFrameCount(&nFrameCount)) || nFrameCount != 1)
	{
		ipDecoder->Release();
		return NULL;
	}

	// load the first frame (i.e., the image)
	IWICBitmapFrameDecode* ipFrame = NULL;
	if (FAILED(ipDecoder->GetFrame(0, &ipFrame)))
	{
		ipDecoder->Release();
		return NULL;
	}

	// convert the image to 32bpp BGRA format with pre-multiplied alpha
	//   (it may not be stored in that format natively in the PNG resource,
	//   but we need this format to create the DIB to use on-screen)
	WICConvertBitmapSource(GUID_WICPixelFormat32bppPBGRA, ipFrame, &ipBitmap);
	ipFrame->Release();

	ipDecoder->Release();
	return ipBitmap;
}

CGdiplusBitmap::~CGdiplusBitmap()
{
	release();
}

void CGdiplusBitmap::release()
{
	if (m_run_thread_animation)
	{
		m_run_thread_animation = false;
		Wait(500);
	}

	//int palSize = m_pBitmap->GetPaletteSize();
	//ColorPalette* palette = (ColorPalette*)malloc(palSize);
	//bitmap->GetPalette(palette, palSize);

	SAFE_DELETE(m_pBitmap);

	if (m_pPropertyItem != NULL)
	{
		free(m_pPropertyItem);
		m_pPropertyItem = NULL;
	}

	SAFE_DELETE_ARRAY(data);
	width = height = channel = stride = 0;
	m_has_alpha_pixel = -1;
	m_filename.Empty();
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
	//이미지 데이터를 추출한 인스턴스라면 변경 후 재추출해준다?
	//data 주소가 변경되므로 위험하다. 좀 더 검토 후 변경할 것!
	if (data)
	{

	}

	width = 0;
	height = 0;
	channel = 0;
	stride = 0;
	m_has_alpha_pixel = -1;

	if (!m_pBitmap)
		return;

	width = m_pBitmap->GetWidth();
	height = m_pBitmap->GetHeight();
	channel = channels();
	stride = width * channel;

	if (stride < 0)
		stride = -stride;

	//GUID guid;
	//m_pBitmap->GetRawFormat(&guid);
	//Gdiplus::PixelFormat pf = m_pBitmap->GetPixelFormat();

	//bool has_alpha = false;
	//if (pf & PixelFormatAlpha)
	//	has_alpha = true;

	//TRACE(_T("has_alpha = %d\n"), has_alpha);

	check_animate_gif();
}

bool CGdiplusBitmap::get_raw_data()
{
	SAFE_DELETE_ARRAY(data);

	Gdiplus::Rect rect(0, 0, width, height); //크기구하기
	Gdiplus::BitmapData bmpData; //비트맵데이터 객체

	//붙여넣기한 이미지가 24비트인데도 32비트로 입력되는 오류가 있다.
	//8비트는 8비트지만 32비트도 24비트로 처리해야
	//우선은 SeetaFace가 동작한다.
	//수정 필요한 부분임.
	Gdiplus::PixelFormat format = m_pBitmap->GetPixelFormat();
	//if (m_pBitmap->GetPixelFormat() == PixelFormat8bppIndexed)
	//	format = PixelFormat8bppIndexed;
	//else
	//	format = PixelFormat24bppRGB;

	//픽셀포맷 형식에따라 이미지 접근 권한 취득.
	//clipboard에서 읽어온 이미지일 경우 아래 코드에서 에러 발생.
	//이 경우 24bpp로 변경해주면 에러는 발생하지 않으나 해결책이 아님.
	if (m_pBitmap->LockBits(&rect, Gdiplus::ImageLockModeRead, format, &bmpData) == Gdiplus::Ok)
	{
		int len = bmpData.Height * std::abs(bmpData.Stride); //이미지 전체크기
		data = new BYTE[len]; //할당
		memcpy(data, bmpData.Scan0, len); //복사
		stride = std::abs(bmpData.Stride);
		m_pBitmap->UnlockBits(&bmpData); //락 풀기
		return true;
	}

	return false;
}

bool CGdiplusBitmap::set_raw_data()
{
	Gdiplus::Rect rect(0, 0, m_pBitmap->GetWidth(), m_pBitmap->GetHeight()); //크기구하기
	Gdiplus::BitmapData bmpData; //비트맵데이터 객체

	Gdiplus::PixelFormat format = m_pBitmap->GetPixelFormat();

	//if (format == PixelFormat8bppIndexed)
	//	format = PixelFormat8bppIndexed;
	//else
	//	format = PixelFormat24bppRGB;

	if (m_pBitmap->LockBits(&rect,
		Gdiplus::ImageLockModeRead,
		format/*m_pBitmap->GetPixelFormat()*/, &bmpData) == Gdiplus::Ok) { //픽셀포맷형식에따라 이미지접근권한 취득

		int len = bmpData.Height * std::abs(bmpData.Stride); //이미지 전체크기
		memcpy(bmpData.Scan0, data, len); //복사
		m_pBitmap->UnlockBits(&bmpData); //락 풀기
		return true;
	}

	return false;
}

bool CGdiplusBitmap::is_empty()
{
	return (m_pBitmap == NULL || width <= 0 || height <= 0);
}

bool CGdiplusBitmap::is_valid()
{
	return !is_empty();
}

int CGdiplusBitmap::channels()
{
	Gdiplus::PixelFormat pf = m_pBitmap->GetPixelFormat();

	if (pf == PixelFormat8bppIndexed)		//198659
		return 1;
	else if (pf == PixelFormat32bppARGB)	//2498570
		return 4;
	else if (pf == PixelFormat32bppRGB)		//139273
		return 4;

	return 3;
}

CRect CGdiplusBitmap::draw(Gdiplus::Graphics& g, CGdiplusBitmap mask1, CRect targetRect)
{
	CGdiplusBitmap temp;
	temp.load(_T("d:\\temp\\mask.bmp"));
	temp.convert2gray();

	/*
	temp.get_raw_data();

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			Gdiplus::Color cr;
			m_pBitmap->GetPixel(x, y, &cr);
			m_pBitmap->SetPixel(x, y, cr);
		}
	}
	*/

	Gdiplus::BitmapData bmData_dst;
	Gdiplus::Rect rect(0, 0, width, height);

	temp.m_pBitmap->LockBits(&rect,
		Gdiplus::ImageLockModeRead,
		temp.m_pBitmap->GetPixelFormat(),
		&bmData_dst);

	uint8_t* src = (uint8_t*)bmData_dst.Scan0;

	Gdiplus::Bitmap mask(temp.width, temp.height, temp.width, PixelFormat8bppIndexed, src);
	int palSize = m_pBitmap->GetPaletteSize();

	Gdiplus::ColorPalette* palette = (Gdiplus::ColorPalette*)malloc(palSize);
	m_pBitmap->GetPalette(palette, palSize);
	// invert - only shows the transparent  
	for (int i = 0; i < 256; i++)
		palette->Entries[i] = Gdiplus::Color::MakeARGB(255 - i, 0, 0, i);

	mask.SetPalette(palette);
	//temp.m_pBitmap->UnlockBits(&bmData_dst);
	//save(&mask, _T("d:\\temp\\mask.bmp"));

	CRect r = get_ratio_rect(targetRect, (double)width / (double)height);

	g.DrawImage(m_pBitmap, r.left, r.top, r.Width(), r.Height());
	g.DrawImage(&mask, r.left, r.top, r.Width(), r.Height());
	
	//CRect r = GetRatioRect(targetRect, (double)width / (double)height);
	//Graphics g(pDC->m_hDC);
	//g.DrawImage(m_pBitmap, r.left, r.top, r.Width(), r.Height());

	return r;
}

CRect CGdiplusBitmap::draw(CDC* pDC, CRect targetRect, int draw_mode)
{
	Gdiplus::Graphics g(pDC->GetSafeHdc());
	return draw(g, targetRect, draw_mode);
}

CRect CGdiplusBitmap::draw(CDC* pDC, int dx, int dy, int dw, int dh)
{
	if (dw <= 0)
		dw = width;
	if (dh <= 0)
		dh = height;

	return draw(pDC, CRect(dx, dy, dx + dw, dy + dh), draw_mode_stretch);
}

//CGdiplusBitmap 이미지를 현재 이미지의 targetRect에 그린다.
CRect CGdiplusBitmap::draw(CGdiplusBitmap *img, CRect* targetRect)
{
	Gdiplus::Graphics g(m_pBitmap);

	CRect dst(0, 0, img->width, img->height);

	if (targetRect)
	{
		dst = *targetRect;
	}

	g.DrawImage(img->m_pBitmap, dst.left, dst.top, dst.Width(), dst.Height());

	return dst;
}

CString CGdiplusBitmap::get_pixel_format_str(Gdiplus::PixelFormat fmt, bool simple)
{
	if (fmt == -1)
		fmt = m_pBitmap->GetPixelFormat();

	CString str_fmt = _T("Unknown PixelFormat");

	switch (fmt)
	{
		case PixelFormat1bppIndexed :
			str_fmt =  _T("PixelFormat1bppIndexed");
			break;
		case PixelFormat4bppIndexed:
			str_fmt = _T("PixelFormat4bppIndexed");
			break;
		case PixelFormat8bppIndexed:
			str_fmt = _T("PixelFormat8bppIndexed");
			break;
		case PixelFormat16bppGrayScale:
			str_fmt = _T("PixelFormat16bppGrayScale");
			break;
		case PixelFormat16bppRGB555:
			str_fmt = _T("PixelFormat16bppRGB555");
			break;
		case PixelFormat16bppRGB565:
			str_fmt = _T("PixelFormat16bppRGB565");
			break;
		case PixelFormat16bppARGB1555:
			str_fmt = _T("PixelFormat16bppARGB1555");
			break;
		case PixelFormat24bppRGB:
			str_fmt = _T("PixelFormat24bppRGB");
			break;
		case PixelFormat32bppRGB:
			str_fmt = _T("PixelFormat32bppRGB");
			break;
		case PixelFormat32bppARGB:
			str_fmt = _T("PixelFormat32bppARGB");
			break;
		case PixelFormat32bppPARGB:
			str_fmt = _T("PixelFormat32bppPARGB");
			break;
		case PixelFormat48bppRGB:
			str_fmt = _T("PixelFormat48bppRGB");
			break;
		case PixelFormat64bppARGB:
			str_fmt = _T("PixelFormat64bppARGB");
			break;
		case PixelFormat64bppPARGB:
			str_fmt = _T("PixelFormat64bppPARGB");
			break;
		case PixelFormat32bppCMYK:
			str_fmt = _T("PixelFormat32bppCMYK");
			break;
		case PixelFormatMax:
			str_fmt = _T("PixelFormatMax");
			break;
	}

	if (simple && (str_fmt.Find(_T("Unknown")) < 0))
	{
		str_fmt.Replace(_T("PixelFormat"), _T(""));
		str_fmt.Replace(_T("bpp"), _T(""));
		int bits = 0;
		int bits_pos = get_number_from_string(str_fmt, bits);
		str_fmt.Format(_T("%s %dbit"), str_fmt.Mid(bits_pos), bits);
		TRACE(_T("str_fmt = %s\n"), str_fmt);
	}

	return str_fmt;
}

//이미지 비율은 보통 width/height지만 반대 비율을 원할 경우는 wh = false로 준다.
float CGdiplusBitmap::get_ratio(bool wh)
{
	if (width == 0 || height == 0)
		return 0.0f;

	if (!wh)
		return (float)height / (float)width;

	return (float)width / (float)height;
}

//alpha 픽셀을 포함한 이미지인지 판별.
//-1 : 아직 판별되지 않은 상태이므로 판별 시작
// 0 : 3채널이거나 4채널의 모든 픽셀의 alpha = 255인 경우
// 1 : 한 점이라도 alpha < 255인 경우
int CGdiplusBitmap::has_alpha_pixel()
{
	//Gdiplus::IsAlphaPixelFormat() 은 PixelFormat32bppARGB 과 같은 Gdiplus::PixelFormat을 구해서 A 채널을 가졌는지를 판별하므로
	//실제 모든 픽셀의 alpha가 255라고 해도 true가 된다.
	//즉, 픽셀을 보고 판별하는 것이 아닌 PixelFormat 값만으로 판별하므로 실제 255가 아닌 alpha값 픽셀이 있는지는 판별하지 않는다.
	if (m_has_alpha_pixel == -1)
	{
		Gdiplus::PixelFormat fmt = m_pBitmap->GetPixelFormat();
		TRACE(_T("fmt = %d, %s\n"), fmt, get_pixel_format_str(fmt));
		m_has_alpha_pixel = (Gdiplus::IsAlphaPixelFormat(fmt) == TRUE);
	}

	return m_has_alpha_pixel;

	//4코너 등과 같은 특정 위치의 픽셀의 alpha값이 255인지 아닌지로 판별하는 것은 좋은 방법은 아니다.
	//alpha 채널의 값을 모두 더해서 그 값이 255 * width * height와 같다면 반투명 이미지가 아니고
	//위 값보다 작다면 반투명 픽셀을 포함하는 이미지라고 판별해야 한다.
	//단, animated gif의 경우는 각 프레임에 대해 get_raw_data()를 해야하므로 좀 복잡해진다.
	//우선 위와 같은 방식대로 한다.
	/*
	if (m_has_alpha_pixel == -1)
	{
		int x, y;
		uint64_t total = 0;

		if (!data)
			get_raw_data();

		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{
				total += *(data + y * width * channel + x * channel + 3);
			}
		}

		m_has_alpha_pixel = (total < width * height * 255);
	}

	return m_has_alpha_pixel;
	*/
}

std::unique_ptr<Gdiplus::TextureBrush> CGdiplusBitmap::get_zigzag_pattern(int sz_tile, Gdiplus::Color cr0, Gdiplus::Color cr1)
{
	auto tile = std::make_unique<Gdiplus::Bitmap>(sz_tile, sz_tile, PixelFormat32bppARGB);
	Gdiplus::Graphics g(tile.get());
	g.Clear(cr0);

	// Draw two black blocks per row, offset every other row
	Gdiplus::SolidBrush br_alt(cr1);

	for (int by = 0; by < sz_tile; by += sz_tile / 2)
	{
		bool offset = (by / (sz_tile / 2)) % 2;
		int x0 = offset ? sz_tile / 2 : 0;
		// each block is (sz/4) high × (sz/2) wide
		g.FillRectangle(&br_alt, x0, by, sz_tile / 2, sz_tile / 2);
	}

	return std::make_unique<Gdiplus::TextureBrush>(tile.get(), Gdiplus::WrapModeTile);
}

CRect CGdiplusBitmap::draw(Gdiplus::Graphics& g, CRect targetRect, int draw_mode)
{
	CRect r;
	
	if (draw_mode == draw_mode_stretch)
		r = targetRect;
	else if (draw_mode == draw_mode_zoom)
		r = get_ratio_rect(targetRect, (double)width / (double)height);
	else
	{
		r = CRect(0, 0, width, height);
		r.OffsetRect(targetRect.left + (targetRect.Width() - width) / 2, targetRect.top + (targetRect.Height() - height) / 2);
	}

	return draw(g, r.left, r.top, r.Width(), r.Height());
}

CRect CGdiplusBitmap::draw(Gdiplus::Graphics& g, int dx, int dy, int dw, int dh)
{
	if (dw <= 0)
		dw = width;
	if (dh <= 0)
		dh = height;

	//long t0 = clock();

	//두 그리기 방식은 Graphics가 더 빠르다.
	//Graphics를 이용하여 그리기
	//if (true)
	{
		//256 gray scale이미지가 올바르게 표시되지 않는다.
		if (channel == 1)
		{
			/*
			m_pBitmap->ConvertFormat(
				PixelFormat24bppRGB,
				Gdiplus::DitherTypeSolid,
				Gdiplus::PaletteTypeOptimal,
				NULL,
				0);
			
			//save(_T("d:\\temp\\converted.bmp"));
			*/
			g.DrawImage(m_pBitmap, dx, dy, dw, dh);
		}
		else
		{
			g.DrawImage(m_pBitmap, dx, dy, dw, dh);
		}
	}
	//StretchBlt를 이용하여 그리기
	/*
	else
	{
		pDC->SetStretchBltMode(HALFTONE);
		HDC MemoryDC = CreateCompatibleDC(pDC->GetSafeHdc()); //상용가능한 DC얻기 
		HBITMAP hBit;
		m_pBitmap->GetHBITMAP(Gdiplus::Color(0, 0, 0, 0), &hBit); //hBitmap얻기
		HGDIOBJ obj = SelectObject(MemoryDC, hBit); //오브젝트 정하기
		BITMAP bm;
		GetObject(hBit, sizeof BITMAP, &bm); //비트맵의 크기구하기

		//출력하기
		StretchBlt(pDC->GetSafeHdc(), dx, dy, dw, dh, MemoryDC, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);

		SelectObject(MemoryDC, obj); //메모리누수방지
		DeleteObject(hBit);
		DeleteDC(MemoryDC);
	}
	*/

	//TRACE(_T("clock = %d\n"), clock() - t0);
	return CRect(dx, dy, dx + dw, dy + dh);

}

//그림을 그리지 않고 표시될 영역 정보만 얻는다.
CRect CGdiplusBitmap::calc_rect(CRect targetRect, int draw_mode)
{
	CRect r;

	if (draw_mode == draw_mode_stretch)
		r = targetRect;
	else if (draw_mode == draw_mode_zoom)
		r = get_ratio_rect(targetRect, (double)width / (double)height);
	else
	{
		r = CRect(0, 0, width, height);
		r.OffsetRect(targetRect.left + (targetRect.Width() - width) / 2, targetRect.top + (targetRect.Height() - height) / 2);
	}

	return r;
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
	if (dst == NULL)
		dst = new CGdiplusBitmap();

	dst->m_pBitmap = new Gdiplus::Bitmap(m_pBitmap->GetWidth(), m_pBitmap->GetHeight(), m_pBitmap->GetPixelFormat());
	Gdiplus::Rect rect(0, 0, m_pBitmap->GetWidth(), m_pBitmap->GetHeight());
	Gdiplus::BitmapData bmpData;
	dst->m_pBitmap->LockBits(&rect, Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeWrite, dst->m_pBitmap->GetPixelFormat(), &bmpData);
	m_pBitmap->LockBits(&rect, Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeUserInputBuf, dst->m_pBitmap->GetPixelFormat(), &bmpData);

	m_pBitmap->UnlockBits(&bmpData);
	dst->m_pBitmap->UnlockBits(&bmpData);

	if (data)
	{
		dst->data = new uint8_t[width * height * channel];
		memcpy(dst->data, data, width * height * channel);
	}

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

void CGdiplusBitmap::rotate(float degree, bool auto_resize, Gdiplus::Color remove_back_color)
{
	int originw = width;
	int originh = height;
	CRect rotated(0, 0, originw, originh);

	if (auto_resize)
		std::vector<CPoint> m_pts = get_rotated(originw / 2, originh / 2, &rotated, degree);

	int neww = rotated.Width();
	int newh = rotated.Height();

	//create a new empty bitmap to hold rotated image 
	Gdiplus::Bitmap* result = new Gdiplus::Bitmap(neww, newh);
	//make a graphics object from the empty bitmap
	Gdiplus::Graphics g(result);

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

CRect CGdiplusBitmap::draw_text(int x, int y, int w, int h,
							CString text,
							float font_size,
							int font_style,
							int shadow_depth,
							float thickness,
							CString font_name,
							Gdiplus::Color cr_text,
							Gdiplus::Color cr_stroke,
							Gdiplus::Color cr_shadow,
							Gdiplus::Color cr_back,
							UINT align)
{
	CRect rTarget(x, y, w, h);
	//adjust_rect_range()
	return draw_text(rTarget, text, font_size, font_style, shadow_depth, thickness, font_name,
		cr_text, cr_stroke, cr_shadow, cr_back, align);
}

CRect CGdiplusBitmap::draw_text(CRect rTarget,
							CString text,
							float font_size,
							int font_style,
							int shadow_depth,
							float thickness,
							CString font_name,
							Gdiplus::Color cr_text,
							Gdiplus::Color cr_stroke,
							Gdiplus::Color cr_shadow,
							Gdiplus::Color cr_back,
							UINT align)
{
	Gdiplus::Graphics g(m_pBitmap);

	if (rTarget.IsRectEmpty())
		rTarget = CRect(0, 0, width, height);

	return ::draw_text(g, rTarget, text, font_size, font_style, shadow_depth, thickness, font_name,
		cr_text, cr_stroke, cr_shadow, cr_back, align);

	/*
	g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
	g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

	//Gdiplus::FontFamily   ffami(CStringW(font_name));
	Gdiplus::FontFamily   ff((WCHAR*)(const WCHAR*)CStringW(font_name));
	Gdiplus::Font font(&ff, (Gdiplus::REAL)font_size, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
	Gdiplus::StringFormat fmt;
	//DT_TOP
	Gdiplus::RectF boundingBox;
	g.MeasureString(CStringW(text), -1, &font, Gdiplus::PointF(0, 0), &boundingBox);

	if (align & DT_CENTER)
		x = x - boundingBox.Width / 2.0f;
	else if (align & DT_RIGHT)
		x = x - boundingBox.Width;

	if (align & DT_VCENTER)
		y = y - boundingBox.Height / 2.0f;
	else if (align & DT_BOTTOM)
		y = y - boundingBox.Height;

	Gdiplus::GraphicsPath   str_path;
	str_path.AddString(CStringW(text), -1, &ff,
		Gdiplus::FontStyleRegular, (Gdiplus::REAL)font_size, Gdiplus::Point(x, y), &fmt);

	Gdiplus::Pen   gp(crOutline, (Gdiplus::REAL)thick);
	gp.SetLineJoin(Gdiplus::LineJoinRound);

	Gdiplus::Rect    rc(x, y, 30, 60);
	//Gdiplus::Color   cStart(255, 128, 0);
	//Gdiplus::Color   cEnd(0, 128, 255);
	//Gdiplus::LinearGradientBrush  gb(rc, cStart, cEnd,
	//	Gdiplus::LinearGradientModeVertical);
	Gdiplus::SolidBrush gb(crFill);

	g.DrawPath(&gp, &str_path);
	g.FillPath(&gb, &str_path);
	*/
}

void CGdiplusBitmap::fit_to_image(Gdiplus::Color remove_back_color)
{
	int x, y;
	CRect r(0, 0, width, height);

	Gdiplus::Color pixel;
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

void CGdiplusBitmap::resize(int cx, int cy, Gdiplus::InterpolationMode mode)
{
	if (cx <= 0 && cy <= 0)
		return;

	if (cx <= 0)
	{
		double ratio = (double)height / (double)cy;
		cx = (double)width / ratio;
	}

	if (cy <= 0)
	{
		double ratio = (double)width / (double)cx;
		cy = (double)height / ratio;
	}

	Gdiplus::Bitmap* result = new Gdiplus::Bitmap(cx, cy);
	Gdiplus::Graphics g(result);

	g.SetInterpolationMode(mode);

	g.DrawImage(m_pBitmap, Gdiplus::Rect(0, 0, cx, cy), 0, 0, width, height, Gdiplus::UnitPixel);

	delete m_pBitmap;
	m_pBitmap = result->Clone(0, 0, cx, cy, PixelFormatDontCare);
	delete result;

	resolution();
}

void CGdiplusBitmap::resize(float fx, float fy, Gdiplus::InterpolationMode mode)
{
	int nw = (float)width * fx;
	int nh = (float)height * fy;
	resize(nw, nh, mode);
}

//이미지 캔버스 크기를 조정한다.
void CGdiplusBitmap::canvas_size(int cx, int cy, Gdiplus::Color cr_fill)
{
	Gdiplus::Bitmap* result = new Gdiplus::Bitmap(cx, cy);
	Gdiplus::Graphics g(result);
	g.Clear(cr_fill);

	int x = (cx - width) / 2;
	int y = (cy - height) / 2;
	g.DrawImage(m_pBitmap, x, y);

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

void CGdiplusBitmap::sub_image(Gdiplus::RectF r)
{
	sub_image(r.X, r.Y, r.Width, r.Height);
}

void CGdiplusBitmap::sub_image(int x, int y, int w, int h)
{
	Gdiplus::Bitmap* result = new Gdiplus::Bitmap(w, h);
	Gdiplus::Graphics g(result);

	g.DrawImage(m_pBitmap, Gdiplus::Rect(0, 0, w, h), x, y, w, h, Gdiplus::UnitPixel);

	delete m_pBitmap;
	m_pBitmap = result->Clone(0, 0, w, h, PixelFormatDontCare);
	delete result;

	resolution();
}

//회색톤으로 변경만 할 뿐 실제 픽셀포맷은 변경되지 않는다.
//weight가 1.0f이면 original gray이미지로 변경되지만 1.0f보다 크면 밝아지고 작으면 어두워짐.
void CGdiplusBitmap::gray(float weight)
{

	Gdiplus::ColorMatrix colorMatrix = { 0.299f * weight, 0.299f * weight, 0.299f * weight, 0.0f, 0.0f,		//red
										 0.587f * weight, 0.587f * weight, 0.587f * weight, 0.0f, 0.0f,		//green
										 0.114f * weight, 0.114f * weight, 0.114f * weight, 0.0f, 0.0f,		//blue
										 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,				//alpha
										 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };

	//원본을 복사해 둘 이미지를 준비하고
	CGdiplusBitmap temp;
	clone(&temp);

	//원래의 이미지로 캔버스를 준비하고 투명하게 비워둔 후
	Gdiplus::Graphics g(m_pBitmap);
	g.Clear(Gdiplus::Color(0, 0, 0, 0));

	Gdiplus::ImageAttributes ia;
	ia.SetColorMatrix(&colorMatrix, Gdiplus::ColorMatrixFlagsDefault, Gdiplus::ColorAdjustTypeBitmap);

	//사본을 ia처리하여 캔버스에 그려준다.
	g.DrawImage(temp, Gdiplus::Rect(0, 0, width, height), 0, 0, width, height, Gdiplus::UnitPixel, &ia);
}

void CGdiplusBitmap::negative()
{
	Gdiplus::ColorMatrix colorMatrix = { -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
								0.0f, -1.0f, 0.0f, 0.0f, 0.0f,
								0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
								0.0f, 0.0f, 0.0f, 1.0, 0.0f,
								1.0f, 1.0f, 1.0f, 0.0f, 1.0f };

	//원본을 복사해 둘 이미지를 준비하고
	CGdiplusBitmap temp;
	clone(&temp);

	//원래의 이미지로 캔버스를 준비하고 투명하게 비워둔 후
	Gdiplus::Graphics g(m_pBitmap);
	g.Clear(Gdiplus::Color(0, 0, 0, 0));

	Gdiplus::ImageAttributes ia;
	ia.SetColorMatrix(&colorMatrix, Gdiplus::ColorMatrixFlagsDefault, Gdiplus::ColorAdjustTypeBitmap);

	//사본을 ia처리하여 캔버스에 그려준다.
	g.DrawImage(temp, Gdiplus::Rect(0, 0, width, height), 0, 0, width, height, Gdiplus::UnitPixel, &ia);
}

Gdiplus::Color  CGdiplusBitmap::get_color(int x, int y)
{
	Gdiplus::Color cr;
	m_pBitmap->GetPixel(x, y, &cr);
	return cr;
}

void CGdiplusBitmap::replace_color(int tx, int ty, Gdiplus::Color dst)
{
	Gdiplus::Color cr;
	m_pBitmap->GetPixel(tx, ty, &cr);
	replace_color(cr, dst);
}

void CGdiplusBitmap::replace_color(Gdiplus::Color src, Gdiplus::Color dst)
{
	//원본을 복사해 둘 이미지를 준비하고
	CGdiplusBitmap temp;
	clone(&temp);

	//원래의 이미지로 캔버스를 준비하고 투명하게 비워둔 후
	Gdiplus::Graphics g(m_pBitmap);
	g.Clear(Gdiplus::Color(0, 0, 0, 0));

	Gdiplus::ImageAttributes ia;

	Gdiplus::ColorMap crMap;
	crMap.oldColor = src;
	crMap.newColor = dst;
	ia.SetRemapTable(1, &crMap);

	//사본을 ia처리하여 캔버스에 그려준다.
	g.DrawImage(temp, Gdiplus::Rect(0, 0, width, height), 0, 0, width, height, Gdiplus::UnitPixel, &ia);
}

//투명 png의 배경색을 변경한다. undo는 지원되지 않는다.
void CGdiplusBitmap::set_back_color(Gdiplus::Color cr_back)
{
	m_cr_back = cr_back;
	if (is_animated_gif())
		return;

	Gdiplus::Bitmap* result = new Gdiplus::Bitmap(width, height);
	Gdiplus::Graphics g(result);
	g.Clear(cr_back);

	g.DrawImage(m_pBitmap, 0, 0);

	delete m_pBitmap;
	m_pBitmap = result->Clone(0, 0, width, height, PixelFormatDontCare);
	delete result;
}

//지정된 색으로 채운다.
void CGdiplusBitmap::clear(Gdiplus::Color cr)
{
	if (is_empty())
		return;

	Gdiplus::Graphics g(m_pBitmap);
	g.Clear(cr);
}

//현재 이미지에 더해지는 것이므로 계속 누적될 것이다.
//즉, red를 +255늘린 후 다시 -255해도 원본이 될 수 없다.
//원본에 적용하는 것이 정석이나 구조 수정이 필요하다.
void CGdiplusBitmap::add_rgb(int red, int green, int blue)
{
	float fr = (float)red / 255.0f;
	float fg = (float)green / 255.0f;
	float fb = (float)blue / 255.0f;

	Gdiplus::ColorMatrix colorMatrix = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
								0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
								0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
								0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
								  fr,   fg,   fb, 0.0f, 1.0f };

	//원본을 복사해 둘 이미지를 준비하고
	CGdiplusBitmap temp;
	clone(&temp);

	//원래의 이미지로 캔버스를 준비하고 투명하게 비워둔 후
	Gdiplus::Graphics g(m_pBitmap);
	g.Clear(Gdiplus::Color(0, 0, 0, 0));

	Gdiplus::ImageAttributes ia;
	ia.SetColorMatrix(&colorMatrix, Gdiplus::ColorMatrixFlagsDefault, Gdiplus::ColorAdjustTypeBitmap);
	//ia.SetColorMatrix(&colorMatrix, ColorMatrixFlagsSkipGray, ColorAdjustTypeBitmap);
	//ia.SetColorMatrix(&colorMatrix, ColorMatrixFlagsAltGray, ColorAdjustTypeBitmap);

	//사본을 ia처리하여 캔버스에 그려준다.
	g.DrawImage(temp, Gdiplus::Rect(0, 0, width, height), 0, 0, width, height, Gdiplus::UnitPixel, &ia);
}

/*
void CGdiplusBitmap::add_rgb_loop(int red, int green, int blue, COLORREF crExcept)
{
	int x, y;
	int a, r, g, b;

	Gdiplus::BitmapData bmData;
	Gdiplus::Rect rect(0, 0, width, height);

	m_pBitmap->LockBits(&rect,
		Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeWrite,
		m_pBitmap->GetPixelFormat(),
		&bmData);

	byte* p = (byte*)(void*)bmData.Scan0;
	int padding = bmData.Stride - width * channel;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			b = (byte)p[0];
			g = (byte)p[1];
			r = (byte)p[2];
			if (channel == 4)
				a = (byte)p[3];
			else
				a = 255;

			if (a != 0 && RGB(r, g, b) != crExcept)
			{
				r += red;
				Clamp(r, 0, 255);

				g += green;
				Clamp(g, 0, 255);

				b += blue;
				Clamp(b, 0, 255);

				p[0] = b;
				p[1] = g;
				p[2] = r;
				if (channel == 4)
					p[3] = a;
			}

			p += channel;
		}
		p += padding;
	}

	m_pBitmap->UnlockBits(&bmData);
}
*/
void CGdiplusBitmap::black_and_white(float red, float green, float blue)
{
	Gdiplus::ColorMatrix cm = { 1.5f, 1.5f, 1.5f, 0.0f, 0.0f,
								1.5f, 1.5f, 1.5f, 0.0f, 0.0f,
								1.5f, 1.5f, 1.5f, 0.0f, 0.0f,
								0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
								red,  green, blue, 0.0f, 1.0f
	};

	Gdiplus::ColorMatrixEffect cmEffect;
	cmEffect.SetParameters(&cm);
	m_pBitmap->ApplyEffect(&cmEffect, NULL);
}

void CGdiplusBitmap::adjust_bright(int bright)
{
	float fb = (float)bright / 255.0f;
	Gdiplus::ColorMatrix cm = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
								0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
								0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
								0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
								fb, fb, fb, 0.0f, 1.0f
	};

	Gdiplus::ColorMatrixEffect cmEffect;
	cmEffect.SetParameters(&cm);
	m_pBitmap->ApplyEffect(&cmEffect, NULL);
}

void CGdiplusBitmap::adjust_contrast(int contrast)
{
	float fc = (float)contrast / 255.0f;
	float ft = (1.0 - fc) / 2.0;
	Gdiplus::ColorMatrix cm = { fc, 0.0f, 0.0f, 0.0f, 0.0f,
								0.0f, fc, 0.0f, 0.0f, 0.0f,
								0.0f, 0.0f, fc, 0.0f, 0.0f,
								0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
								ft, ft, ft, 0.0f, 1.0f
	};

	Gdiplus::ColorMatrixEffect cmEffect;
	cmEffect.SetParameters(&cm);
	m_pBitmap->ApplyEffect(&cmEffect, NULL);
}

void CGdiplusBitmap::sepia()
{
	Gdiplus::ColorMatrix cm = { 0.393f, 0.349f, 0.272f, 0.0f, 0.0f,
								0.769f, 0.686f, 0.534f, 0.0f, 0.0f,
								0.189f, 0.168f, 0.131f, 0.0f, 0.0f,
								0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
								0.0f, 0.0f, 0.0f, 0.0f, 1.0f
	};

	Gdiplus::ColorMatrixEffect cmEffect;
	cmEffect.SetParameters(&cm);
	m_pBitmap->ApplyEffect(&cmEffect, NULL);
}

void CGdiplusBitmap::polaroid()
{
	Gdiplus::ColorMatrix cm = { 1.439, -0.062f, -0.062f, 0.0f, 0.0f,
								-0.122f, 1.378f, -0.122f, 0.0f, 0.0f,
								-0.016f, -0.016f, 1.483f, 0.0f, 0.0f,
								0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
								-0.030f, 0.050f, -0.020f, 0.0f, 1.0f
	};

	Gdiplus::ColorMatrixEffect cmEffect;
	cmEffect.SetParameters(&cm);
	m_pBitmap->ApplyEffect(&cmEffect, NULL);
}

void CGdiplusBitmap::rgb_to_bgr()
{
	Gdiplus::ColorMatrix cm = { 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
								0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
								1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
								0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
								0.0f, 0.0f, 0.0f, 0.0f, 1.0f
	};

	Gdiplus::ColorMatrixEffect cmEffect;
	cmEffect.SetParameters(&cm);
	m_pBitmap->ApplyEffect(&cmEffect, NULL);
}

void CGdiplusBitmap::adjust_hsl(int hue, int sat, int light)
{
	Gdiplus::HueSaturationLightness hsl;
	Gdiplus::HueSaturationLightnessParams hslParam;

	hslParam.hueLevel = hue;// random19937(-180, 180);
	hslParam.saturationLevel = sat;
	hslParam.lightnessLevel = light;

	hsl.SetParameters(&hslParam);

	//Gdiplus::Bitmap::ApplyEffect(&m_pBitmap, 1, &hsl, NULL, NULL, &m_pBitmap);
	m_pBitmap->ApplyEffect(&hsl, NULL);
}

void CGdiplusBitmap::adjust_rgba(float r, float g, float b, float a)
{
	Gdiplus::ColorMatrix cm = {	r, 0.0f, 0.0f, 0.0f, 0.0f,
						0.0f, g, 0.0f, 0.0f, 0.0f,
						0.0f, 0.0f, b, 0.0f, 0.0f,
						0.0f, 0.0f, 0.0f, a, 0.0f,
						0.0f, 0.0f, 0.0f, 0.0f, 1.0f
					 };

	Gdiplus::ColorMatrixEffect cmEffect;
	cmEffect.SetParameters(&cm);

	//CGdiplusBitmap* temp;
	//deep_copy(temp);
	
	//Gdiplus::Bitmap::ApplyEffect(&m_pBitmap, 1, &cmEffect, NULL, NULL, &m_pBitmap);
	m_pBitmap->ApplyEffect(&cmEffect, NULL);
}

void CGdiplusBitmap::gdip_blur(float radius, BOOL expandEdge)
{
	Gdiplus::Blur gdi_blur;
	Gdiplus::BlurParams param;
	param.radius = radius;
	param.expandEdge = expandEdge;
	gdi_blur.SetParameters(&param);
	m_pBitmap->ApplyEffect(&gdi_blur, NULL);
}

void CGdiplusBitmap::blur(float sigma, int order)
{
	CGdiplusBitmap temp;
	deep_copy(&temp);
	temp.get_raw_data();

	get_raw_data();
	
	fast_gaussian_blur(temp.data, data, width, height, channel, sigma, order, Border::kKernelCrop);
	set_raw_data();

#ifdef _DEBUG
	save(_T("d:\\fast_blur (sigma=%.1f, order=%d).png"), sigma, order);
#endif
}

void CGdiplusBitmap::round_shadow_rect(int w, int h, float radius, float blur_sigma, Gdiplus::Color cr_shadow)
{
	release();

	m_pBitmap = new Gdiplus::Bitmap(w, h, PixelFormat32bppARGB);
	resolution();

	Gdiplus::Graphics g(m_pBitmap);
	g.Clear(Gdiplus::Color::Transparent);
	//g.Clear(Color::Blue);


	//SolidBrush brush(Color(255, 255, 0, 0));
	//g.FillRectangle(&brush, 4, 4, w - 8, h - 8);
	//apply_effect_blur(20.0f, FALSE);
	
	Gdiplus::Matrix matrix(1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
	CGdiplusBitmap shadow(w, h, PixelFormat32bppARGB, cr_shadow);
#ifdef _DEBUG
	shadow.save(_T("d:\\temp\\shadow_1default.png"));
#endif

	shadow.round_corner(radius);// , 0.90f, 0.1f);
#ifdef _DEBUG
	shadow.save(_T("d:\\temp\\shadow_2round.png"));
#endif

	shadow.gdip_blur(14.0f, FALSE);
#ifdef _DEBUG
	shadow.save(_T("d:\\temp\\shadow_3blur.png"));
#endif

	g.DrawImage(shadow, 0, 0, shadow.width, shadow.height);
	/*
	Gdiplus::BlurParams myBlurParams;
	myBlurParams.expandEdge = true;
	myBlurParams.radius = 49.0f;
	Gdiplus::Blur myBlur;
	myBlur.SetParameters(&myBlurParams);

	Gdiplus::RectF r(0, 0, w, h);
	g.DrawImage(m_pBitmap, &r, &matrix, &myBlur, NULL, Gdiplus::UnitPixel);
	*/

	/*
	GraphicsPath path;

	float depth = 4.0f;
	path.AddArc(depth, depth, radius, radius, 180.0, 90.0);
	path.AddArc(width - depth - radius, depth, radius, radius, 270.0, 90.0);
	path.AddArc(width - depth - radius, height - depth - radius, radius, radius, 0.0, 90.0);
	path.AddArc(depth, height - depth - radius, radius, radius, 90.0, 90.0);
	path.CloseFigure();

	g.SetSmoothingMode(SmoothingMode::SmoothingModeAntiAlias);

	SolidBrush brush(Color::White);
	g.FillPath(&brush, &path);
	*/
}

void CGdiplusBitmap::round_corner(float radius, float factor, float position, bool tl, bool tr, bool br, bool bl)
{
	if (channel != 4)
		cvtColor32ARGB();

	Gdiplus::GraphicsPath path;
	float ratio = 0.0f;// ((float)width / (float)height);

	get_round_rect_path(&path, Gdiplus::Rect(0, 0, width, height), radius);
	/*
	if (tl)
		path.AddArc(0.0, ratio, radius, radius, 180.0, 90.0);
	else
		path.AddArc(0.0, ratio, 0.0f, 0.0f, 180.0, 90.0);
	path.AddArc(width - radius, ratio, radius, radius, 270.0, 90.0);
	path.AddArc(width - radius, height - ratio - radius, radius, radius, 0.0, 90.0);
	path.AddArc(0.0, height - ratio - radius, radius, radius, 90.0, 90.0);
	path.CloseFigure();
	*/

	if (factor <= 0.0f || position <= 0.0f)
	{
		//원본을 복사해 둘 이미지를 준비하고
		CGdiplusBitmap temp;
		clone(&temp);

		//원래의 이미지로 캔버스를 준비하고 투명하게 비워둔 후
		Gdiplus::Graphics g(m_pBitmap);
		g.Clear(Gdiplus::Color::Transparent);

		g.SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);
		//Brush* brush = new TextureBrush(temp);
		Gdiplus::TextureBrush brush(temp);
		g.FillPath(&brush, &path);
		return;
	}

	//width, height가 다르면 blur의 두께가 다른 문제가 있다.
	//이를 어디선가는 보정해줘야 한다.
	int in_out_count = 1;	//3으로 해선 안된다.
	float blendFactor[] = {
		0.0f,
		factor,		//중점부터 밖으로 까매지는 정도
		1.0f,
	};
	float blendPosition[] = {
		0.0f,
		position,	//바깥에서 중점으로 밝아지는 정도
		1.0f,
	};

	Gdiplus::PathGradientBrush pgb(&path);

	//중점 세팅
	pgb.SetCenterPoint(Gdiplus::Point(width / 2, height / 2));

	//블렌드 수준값 세팅
	pgb.SetBlend(blendFactor, blendPosition, 3);	//blendFactor, blendPosition의 갯수

	pgb.SetCenterColor(Gdiplus::Color::Black);
	Gdiplus::Color colors[] = { Gdiplus::Color(Gdiplus::Color::Transparent) };
	pgb.SetSurroundColors(colors, &in_out_count);

	//mask를 CGdiplusBitmap 타입으로 정적 생성하니 오류발생하여 동적 생성함.
	Gdiplus::Bitmap *mask = new Gdiplus::Bitmap(width, height, PixelFormat32bppARGB);
	Gdiplus::Graphics gMask(mask);
	gMask.FillRectangle(&Gdiplus::SolidBrush(Gdiplus::Color::Transparent), Gdiplus::Rect(0, 0, width, height));
	gMask.SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeHighQuality);
	gMask.FillPath(&pgb, &path);

#ifdef _DEBUG
	CString str;
	str.Format(_T("z:\\내 드라이브\\media\\test_image\\temp\\mask_%.2f_%.2f.png"), blendFactor[1], blendPosition[1]);
	::save(mask, str);
#endif

	replace_channel(mask, m_pBitmap, 3, 3);

	delete mask;
}

void CGdiplusBitmap::replace_channel(CString type, UINT srcID, int src_bgra_index, int dst_bgra_index)
{
	CGdiplusBitmap src(type, srcID);
	replace_channel(src, m_pBitmap, src_bgra_index, dst_bgra_index);
}

void CGdiplusBitmap::replace_channel(CString src_file, int src_bgra_index, int dst_bgra_index)
{
	CGdiplusBitmap src(src_file);
	replace_channel(src, m_pBitmap, src_bgra_index, dst_bgra_index);
}

void CGdiplusBitmap::replace_channel(Gdiplus::Bitmap* src, int src_bgra_index, int dst_bgra_index)
{
	replace_channel(src, m_pBitmap, src_bgra_index, dst_bgra_index);
}

void CGdiplusBitmap::replace_channel(Gdiplus::Bitmap* src, Gdiplus::Bitmap* dst, int src_bgra_index, int dst_bgra_index)
{
	//이미지 크기는 동일해야 한다.
	if ((src->GetWidth() != dst->GetWidth()) || (src->GetHeight() != dst->GetHeight()))
	{
		return;
	};

	Gdiplus::Rect r(0, 0, src->GetWidth(), src->GetHeight());

	Gdiplus::BitmapData* bdSrc = new Gdiplus::BitmapData;
	src->LockBits(&r, Gdiplus::ImageLockMode::ImageLockModeRead, PixelFormat32bppARGB, bdSrc); //PixelFormat(Format32bppArgb)  
	Gdiplus::BitmapData* bdDst = new Gdiplus::BitmapData;
	dst->LockBits(&r, Gdiplus::ImageLockMode::ImageLockModeRead, PixelFormat32bppARGB, bdDst);

	int src_ch = 4;
	int dst_ch = 4;

	try
	{
		byte* bpSrc = (byte*)bdSrc->Scan0;
		byte* bpDst = (byte*)bdDst->Scan0;
		bpSrc += (int)src_bgra_index;
		bpDst += (int)dst_bgra_index;

		for (int i = r.Height * r.Width; i > 0; i--)
		{
			*bpDst = *bpSrc;
			bpSrc += src_ch;
			bpDst += dst_ch;
		}
	}
	catch (const std::exception&)
	{

	}

	src->UnlockBits(bdSrc);
	dst->UnlockBits(bdDst);
}

//실제 8bit(256color) gray이미지로 변경해준다.
void CGdiplusBitmap::convert2gray()
{
	//뭔가 이 방식은 문제가 있어서 일단 보류
	/*
	Gdiplus::Bitmap* pBitmap = m_pBitmap->Clone(0, 0, m_pBitmap->GetWidth(), m_pBitmap->GetHeight(), PixelFormat8bppIndexed);// m_pBitmap->GetPixelFormat());
	Gdiplus::ColorPalette* pal = (Gdiplus::ColorPalette*)malloc(sizeof(Gdiplus::ColorPalette) + 255 * sizeof(Gdiplus::ARGB));

	pal->Count = 256;
	pal->Flags = Gdiplus::PaletteFlagsGrayScale;
	for (int i = 0; i < 256; i++)
	{
		pal->Entries[i] = Gdiplus::Color::MakeARGB((BYTE)255, i, i, i);
	}
	pBitmap->SetPalette(pal);

	Gdiplus::Status status = Gdiplus::Bitmap::InitializePalette(pal, Gdiplus::PaletteTypeOptimal, 256, FALSE, pBitmap);

	status = pBitmap->ConvertFormat(
		PixelFormat8bppIndexed,
		Gdiplus::DitherTypeNone,
		Gdiplus::PaletteTypeCustom,
		pal,
		0);

	save(pBitmap, _T("d:\\temp\\gray.bmp"));
	//release();
	

	resolution();

	free(pal);
	*/
	
	Gdiplus::Bitmap* bitmap = new Gdiplus::Bitmap(width, height, PixelFormat8bppIndexed);
	int palSize = bitmap->GetPaletteSize();
	
	Gdiplus::ColorPalette *palette = (Gdiplus::ColorPalette*)malloc(palSize);

	bitmap->GetPalette(palette, palSize);
	palette->Flags = Gdiplus::PaletteFlagsGrayScale;
	palette->Count = 256;
	for (int i = 0; i < 256; i++)
	{
		palette->Entries[i] = Gdiplus::Color::MakeARGB((BYTE)255, i, i, i);
	}
	bitmap->SetPalette(palette);

	Gdiplus::BitmapData bmData_src;
	Gdiplus::BitmapData bmData_dst;
	Gdiplus::Rect rect(0, 0, width, height);

	gray();

	m_pBitmap->LockBits(&rect,
		Gdiplus::ImageLockModeRead,
		m_pBitmap->GetPixelFormat(),
		&bmData_src);

	bitmap->LockBits(&rect,
		Gdiplus::ImageLockModeRead,
		bitmap->GetPixelFormat(),
		&bmData_dst);

	int x, y;
	uint8_t* src = (uint8_t*)bmData_src.Scan0;
	uint8_t* dst = (uint8_t*)bmData_dst.Scan0;
	int padding = bmData_src.Stride - width * channel;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			dst[y * width + x] = src[0];
			src += channel;
		}
		//padding을 더해주면 결과가 어긋나게 나온다.
		//src += padding;
	}

	m_pBitmap->UnlockBits(&bmData_src);
	bitmap->UnlockBits(&bmData_dst);

	release();

	m_pBitmap = bitmap;
	resolution();

	free(palette);
	save(_T("d:\\temp\\gray.bmp"));
}

void CGdiplusBitmap::cvtColor(Gdiplus::PixelFormat old_format, Gdiplus::PixelFormat new_format)
{

}

void CGdiplusBitmap::cvtColor32ARGB()
{
	/*
	//원본을 복사해 둘 이미지를 준비하고
	CGdiplusBitmap temp;
	clone(&temp);

	//원래의 이미지로 캔버스를 준비하고 투명하게 비워둔 후
	Graphics g(m_pBitmap);
	g.Clear(Color(0, 0, 0, 0));

	//사본을 ia처리하여 캔버스에 그려준다.
	g.DrawImage(temp, 0, 0, width, height);
	resolution();
	*/
	
	Gdiplus::Bitmap* result = new Gdiplus::Bitmap(width, height, PixelFormat32bppARGB);
	Gdiplus::Graphics g(result);

	Gdiplus::ImageAttributes ia;

	g.DrawImage(m_pBitmap, 0, 0, width, height);

	delete m_pBitmap;
	m_pBitmap = result->Clone(0, 0, width, height, PixelFormatDontCare);
	delete result;

	resolution();
}

void CGdiplusBitmap::set_matrix(Gdiplus::ColorMatrix* colorMatrix, Gdiplus::ColorMatrix* grayMatrix)
{
	//원본을 복사해 둘 이미지를 준비하고
	CGdiplusBitmap temp;
	clone(&temp);

	//원래의 이미지로 캔버스를 준비하고 투명하게 비워둔 후
	Gdiplus::Graphics g(m_pBitmap);
	g.Clear(Gdiplus::Color(0, 0, 0, 0));

	Gdiplus::ImageAttributes ia;

	if (grayMatrix)
		ia.SetColorMatrices(colorMatrix, grayMatrix, Gdiplus::ColorMatrixFlagsDefault, Gdiplus::ColorAdjustTypeBitmap);
	else
		ia.SetColorMatrix(colorMatrix, Gdiplus::ColorMatrixFlagsDefault, Gdiplus::ColorAdjustTypeBitmap);

	//사본을 ia처리하여 캔버스에 그려준다.
	g.DrawImage(temp, Gdiplus::Rect(0, 0, width, height), 0, 0, width, height, Gdiplus::UnitPixel, &ia);
}

//0(transparent) ~ 255(opaque)
void CGdiplusBitmap::set_alpha(int alpha)
{
	Gdiplus::ColorMatrix colorMatrix = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
								0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
								0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
								0.0f, 0.0f, 0.0f, (float)alpha / 255.0f, 0.0f,
								0.0f, 0.0f, 0.0f, 0.0f, 1.0f };

	//new를 사용하지 않는 아래 방식으로 할 경우
	//기존 32비트 또는 24비트 YUV의 png, jpg는 투명하게 되나
	//RGB24였던 이미지는 투명하게 안된다.
	//우선 new 방식으로 한다.
#if 1

#if	(GDIPVER >= 0x0110)
	//m_pBitmap->ConvertFormat();
#endif

	Gdiplus::Bitmap* result = new Gdiplus::Bitmap(width, height, PixelFormat32bppARGB);
	Gdiplus::Graphics g(result);

	//g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);

	Gdiplus::ImageAttributes ia;

	ia.SetColorMatrix(&colorMatrix, Gdiplus::ColorMatrixFlagsDefault, Gdiplus::ColorAdjustTypeBitmap);
	g.DrawImage(m_pBitmap, Gdiplus::Rect(0, 0, width, height), 0, 0, width, height, Gdiplus::UnitPixel, &ia);

	delete m_pBitmap;
	m_pBitmap = result->Clone(0, 0, width, height, PixelFormatDontCare);
	delete result;
#else
	//원본을 복사해 둘 이미지를 준비하고
	CGdiplusBitmap temp;
	deep_copy(&temp);

	//m_pBitmap->ConvertFormat();

	//원래의 이미지로 캔버스를 준비하고 투명하게 비워둔 후
	Gdiplus::Graphics g(m_pBitmap);
	g.Clear(Gdiplus::Color(0, 0, 0, 0));

	Gdiplus::ImageAttributes ia;
	ia.SetColorMatrix(&colorMatrix);// , ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);

	//사본을 ia처리하여 캔버스에 그려준다.
	g.DrawImage(temp, Gdiplus::Rect(0, 0, width, height), 0, 0, width, height, Gdiplus::UnitPixel, &ia);
#endif
}

void CGdiplusBitmap::set_colorkey(Gdiplus::Color low, Gdiplus::Color high)
{
	//원본을 복사해 둘 이미지를 준비하고
	CGdiplusBitmap temp;
	clone(&temp);

	//원래의 이미지로 캔버스를 준비하고 투명하게 비워둔 후
	Gdiplus::Graphics g(m_pBitmap);
	g.Clear(Gdiplus::Color(0, 0, 0, 0));

	Gdiplus::ImageAttributes ia;
	ia.SetColorKey(low, high);

	//사본을 ia처리하여 캔버스에 그려준다.
	g.DrawImage(m_pBitmap, Gdiplus::Rect(0, 0, width, height), 0, 0, width, height, Gdiplus::UnitPixel, &ia);
}

bool CGdiplusBitmap::is_equal(Gdiplus::Color cr0, Gdiplus::Color cr1, int channel)
{
	return (cr0.GetValue() == cr1.GetValue());

	if (channel == 1)
	{

	}
	else if (channel == 3)
	{
		if (cr0.GetRed() == cr1.GetRed() &&
			cr0.GetGreen() == cr1.GetGreen() &&
			cr0.GetBlue() == cr1.GetBlue())
			return true;
	}
	else if (channel == 4)
	{
		if (cr0.GetAlpha() == 0 && cr1.GetAlpha() == 0)
			return true;

		if (cr0.GetRed() == cr1.GetRed() &&
			cr0.GetGreen() == cr1.GetGreen() &&
			cr0.GetBlue() == cr1.GetBlue() &&
			cr0.GetAlpha() == cr1.GetAlpha())
			return true;
	}

	return false;
}


bool CGdiplusBitmap::save(LPCTSTR filepath, ...)
{
	va_list args;
	va_start(args, filepath);

	CString filename;
	filename.FormatV(filepath, args);

	return ::save(m_pBitmap, filename);
}

bool CGdiplusBitmap::copy_to_clipbard()
{
	//복사는 잘 되지만 32bit PNG는 투명처리 안됨
#if 1
	HBITMAP hbitmap;
	auto status = m_pBitmap->GetHBITMAP(0, &hbitmap);
	if (status != Gdiplus::Ok)
		return false;
	BITMAP bm;
	GetObject(hbitmap, sizeof bm, &bm);
	DIBSECTION ds;
	if (sizeof ds == GetObject(hbitmap, sizeof ds, &ds))
	{
		HDC hdc = GetDC(NULL);
		HBITMAP hbitmap_ddb = CreateDIBitmap(hdc, &ds.dsBmih, CBM_INIT,
			ds.dsBm.bmBits, (BITMAPINFO*)&ds.dsBmih, DIB_RGB_COLORS);
		ReleaseDC(NULL, hdc);
		if (OpenClipboard(NULL))
		{
			EmptyClipboard();
			SetClipboardData(CF_BITMAP, hbitmap_ddb);
			CloseClipboard();
		}
		DeleteObject(hbitmap_ddb);
	}
	DeleteObject(hbitmap);
#elif 0
	bool res = false;
	HBITMAP hbitmap;
	auto status = m_pBitmap->GetHBITMAP(NULL, &hbitmap);

	if (status != Gdiplus::Ok)
		return false;

	BITMAP bm;

	GetObject(hbitmap, sizeof(bm), &bm);

	uint64_t bufSize = width * height * 4;

	BITMAPINFOHEADER bi = { sizeof bi, bm.bmWidth, bm.bmHeight, 1, bm.bmBitsPixel, BI_PNG };

	BITMAPV5HEADER header;
	header.bV5Size = sizeof(BITMAPV5HEADER);
	header.bV5Width = width; // <-- size of the bitmap in pixels, width and height
	header.bV5Height = height;
	header.bV5Planes = 1;
	header.bV5BitCount = 0;
	header.bV5Compression = BI_PNG;
	header.bV5SizeImage = bufSize;
	header.bV5XPelsPerMeter = 0;
	header.bV5YPelsPerMeter = 0;
	header.bV5ClrUsed = 0;
	header.bV5ClrImportant = 0;
	header.bV5RedMask = 0xFF000000;
	header.bV5GreenMask = 0x00FF0000;
	header.bV5BlueMask = 0x0000FF00;
	header.bV5AlphaMask = 0x000000FF;
	header.bV5CSType = LCS_sRGB;
	header.bV5Endpoints;    // ignored
	header.bV5GammaRed = 0;
	header.bV5GammaGreen = 0;
	header.bV5GammaBlue = 0;
	header.bV5Intent = 0;
	header.bV5ProfileData = 0;
	header.bV5ProfileSize = 0;
	header.bV5Reserved = 0;

	std::vector<BYTE> vec(bm.bmWidthBytes * bm.bmHeight);
	HDC hDC = GetDC(NULL);
	GetDIBits(hDC, hbitmap, 0, bi.biHeight, vec.data(), (BITMAPINFO*)&bi, 0);
	::DeleteDC(hDC);

	HGLOBAL hmem = GlobalAlloc(GMEM_MOVEABLE, sizeof(header) + bufSize);
	void* buffer = (BYTE*)GlobalLock(hmem);
	memcpy(buffer, &header, sizeof(BITMAPV5HEADER));
	//memcpy((char*)buffer + sizeof(BITMAPV5HEADER), vec.data(), vec.size());

	get_raw_data();
	memcpy((char*)buffer + sizeof(BITMAPV5HEADER), data, bufSize);
	GlobalUnlock(hmem);

	if (OpenClipboard(NULL))
	{
		EmptyClipboard();
		auto fmt = RegisterClipboardFormat(_T("PNG"));
		SetClipboardData(fmt, hmem);
		//SetClipboardData(CF_DIBV5, hmem);
		//SetClipboardData(CF_BITMAP, hbitmap);
		CloseClipboard();
		MessageBeep(0);
		res = true;
	}
	else
	{
		GlobalFree(hmem);
	}

	DeleteObject(hbitmap);
	TRACE(_T("copied to clipboard.\n"));
	return res;
#elif 0
	//  this section is my code for creating a png file
	StreamWrite stream = StreamWrite::asBufferCreate();
	in.savePng(stream);
	uint64 bufSize = 0;
	char* buf = stream._takeBuffer(bufSize, false);
	// "buf"      <-- contains the PNG payload
	// "bufSize"  <-- is the size of this payload

	HGLOBAL gift = GlobalAlloc(GMEM_MOVEABLE, bufSize);
	if (gift == NULL)
		return false;

	HWND win = window.getWindowHandle();
	if (!OpenClipboard(win)) {
		GlobalFree(gift);
		return false;
	}
	EmptyClipboard();

	auto fmt = RegisterClipboardFormat(_T("PNG")); // or `L"PNG", as applicable

	void* giftLocked = GlobalLock(gift);
	if (giftLocked) {
		memcpy((char*)giftLocked, buf, bufSize);
	}
	GlobalUnlock(gift);

	SetClipboardData(fmt, gift);

	CloseClipboard();
#endif
	return true;
}

bool CGdiplusBitmap::paste_from_clipboard()
{
	HBITMAP bitmap;
	if (OpenClipboard(NULL) == false)
		return false;

	bitmap = (HBITMAP)GetClipboardData(CF_BITMAP);
	if (!bitmap)
	{
		CloseClipboard();
		return false;
	}

	//int n = sizeof(bitmap);
	CloseClipboard();

	release();

	DIBSECTION ds = {};
	if (!::GetObject(bitmap, sizeof(ds), &ds))
		return false;

	if (ds.dsBm.bmBits && (ds.dsBm.bmBitsPixel == 32) && (ds.dsBmih.biCompression == BI_RGB))
	{
		m_pBitmap = CreateARGBBitmapFromDIB(ds);
	}
	else
	{
		m_pBitmap = Gdiplus::Bitmap::FromHBITMAP(bitmap, NULL);
	}

	resolution();

	return true;
}

void CGdiplusBitmap::check_animate_gif()
{
	m_run_thread_animation = false;
	m_frame_index = 0;

	UINT count = m_pBitmap->GetFrameDimensionsCount();
	GUID* pDimensionIDs = new GUID[count];

	// Get the list of frame dimensions from the Image object.
	m_pBitmap->GetFrameDimensionsList(pDimensionIDs, count);

	// Get the number of frames in the first dimension.
	m_frame_count = m_pBitmap->GetFrameCount(&pDimensionIDs[0]);
	//TRACE(_T("m_frame_count = %d\n"), m_frame_count);

	// Assume that the image has a property item of type PropertyItemEquipMake.
	// Get the size of that property item.
	int nSize = m_pBitmap->GetPropertyItemSize(PropertyTagFrameDelay);
	if (nSize == 0)
	{
		delete[]pDimensionIDs;
		return;
	}

	// Allocate a buffer to receive the property item.
	m_pPropertyItem = (Gdiplus::PropertyItem*)malloc(nSize);

	m_pBitmap->GetPropertyItem(PropertyTagFrameDelay, nSize, m_pPropertyItem);

	delete []pDimensionIDs;
}

//gif 프레임 이미지들을 저장
bool CGdiplusBitmap::save_gif_frames(CString folder)
{
	//현재 재생중이었다면 재생 정보를 기억해놓는다.
	bool is_playing = !m_paused;
	if (is_playing)
	{
		m_paused = !m_paused;
		Wait(100);
	}

	if (!PathIsDirectory(folder))
	{
		folder = get_part(folder, fn_folder);
	}

	GUID   pageGuid = Gdiplus::FrameDimensionTime;

	CString str;

	for (size_t i = 0; i < m_frame_count; i++)
	{
		m_pBitmap->SelectActiveFrame(&pageGuid, i);
		str.Format(_T("%s\\%s_%04d.png"), folder, get_part(m_filename, fn_name), i);
		save(str);
	}

	if (is_playing)
	{
		m_pBitmap->SelectActiveFrame(&pageGuid, m_frame_index);
		m_paused = !m_paused;
	}

	return true;
}

void CGdiplusBitmap::get_gif_frames(std::vector<Gdiplus::Bitmap*>& dqBitmap, std::vector<long>& dqDelay)
{
	if (m_frame_count < 2)
		return;

	//현재 재생중이었다면 재생 정보를 기억해놓고 일시정지시키고
	bool is_playing = !m_paused;
	if (is_playing)
	{
		m_paused = !m_paused;
		Wait(100);
	}

	GUID   pageGuid = Gdiplus::FrameDimensionTime;

	CString str;

	dqBitmap.clear();
	dqDelay.clear();

	for (size_t i = 0; i < m_frame_count; i++)
	{
		m_pBitmap->SelectActiveFrame(&pageGuid, i);
		Gdiplus::Bitmap* img = new Gdiplus::Bitmap(width, height, PixelFormat32bppARGB);
		Gdiplus::Graphics g(img);
		g.DrawImage(m_pBitmap, 0, 0, width, height);
		dqBitmap.push_back(img);
		dqDelay.push_back(((long*)m_pPropertyItem->value)[i] * 10);
	}

	//일시정지된 재생을 다시 재개시킨다.
	if (is_playing)
	{
		m_pBitmap->SelectActiveFrame(&pageGuid, m_frame_index);
		m_paused = !m_paused;
	}
}

//총 재생시간을 ms단위로 리턴한다.
int CGdiplusBitmap::get_total_duration()
{
	if (m_frame_count < 2)
		return 0;

	long total_duration = 0;

	for (size_t i = 0; i < m_frame_count; i++)
	{
		total_duration += ((long*)m_pPropertyItem->value)[i] * 10;
	}

	return (int)total_duration;
}

void CGdiplusBitmap::set_animation(HWND hWnd, int x, int y, int w, int h, bool auto_play)
{
	if (m_frame_count < 2)
		return;

	m_target_hwnd = hWnd;
	m_ani_sx = x;
	m_ani_sy = y;
	m_ani_width = (w == 0 ? width : w);
	m_ani_height = (h == 0 ? height : h);

	m_paused = !auto_play;

	play_gif();
}

void CGdiplusBitmap::set_animation(HWND hWnd, CRect r, bool start)
{
	int w = (r.Width() > 0 ? r.Width() : width);
	int h = (r.Height() > 0 ? r.Height() : height);

	CRect fit = get_ratio_rect(CRect(r.left, r.top, r.left + w, r.top + h), width, height);
	set_animation(hWnd, fit.left, fit.top, fit.Width(), fit.Height(), start);
}


void CGdiplusBitmap::move_gif(int x, int y, int w, int h)
{
	m_ani_sx = x;
	m_ani_sy = y;
	m_ani_width = (w == 0 ? width : w);
	m_ani_height = (h == 0 ? height : h);
}

void CGdiplusBitmap::move_gif(CRect r)
{
	CRect fit = get_ratio_rect(CRect(r.left, r.top, r.left + r.Width(), r.top + r.Height()), width, height);
	move_gif(fit.left, fit.top, fit.Width(), fit.Height());
}

void CGdiplusBitmap::play_gif()
{
	if (!m_gif_play_in_this)
		return;

	if (m_run_thread_animation)
	{
		if (m_paused)
		{
			m_paused = false;
			return;
		}

		return;
	}

	if (m_frame_count < 2)
		return;

	m_run_thread_animation = true;

	GUID   pageGuid = Gdiplus::FrameDimensionTime;
	m_frame_index = 0;
	m_pBitmap->SelectActiveFrame(&pageGuid, m_frame_index);
	//replace_color(Gdiplus::Color(255, 76, 86, 164), Gdiplus::Color(0, 255, 112, 109));

	std::thread t(&CGdiplusBitmap::thread_gif_animation, this);
	t.detach();
}

//pos위치로 이동한 후 일시정지한다. -1이면 pause <-> play를 토글한다.
void CGdiplusBitmap::pause_gif(int pos)
{
	if (!m_gif_play_in_this)
		return;

	if (m_frame_count < 2 || !m_run_thread_animation)
		return;

	//m_frame_count가 UINT이고 pos = -1일 경우 아래 if문은 true가 된다.
	//if (-1 >= (UINT)20) => true
	//취약하므로 m_frame_count의 type을 UINT에서 int로 변경한다.
	if (pos >= m_frame_count)
		pos = 0;

	if (pos < 0)
	{
		m_paused = !m_paused;
	}
	else
	{
		goto_frame(pos, true);
	}
}

void CGdiplusBitmap::stop_gif()
{
	if (m_frame_count < 2)
		return;

	m_run_thread_animation = false;
	Wait(500);
	RECT r;
	r.left = m_ani_sx;
	r.top = m_ani_sy;
	r.right = r.left + m_ani_width;
	r.bottom = r.top + m_ani_height;
	::InvalidateRect(m_target_hwnd, &r, TRUE);
}

void CGdiplusBitmap::goto_frame(int pos, bool pause)
{
	if (m_frame_count < 2)
		return;

	if (pos >= m_frame_count)
		m_frame_index = 0;

	m_frame_index = pos;
	m_paused = pause;

	GUID   pageGuid = Gdiplus::FrameDimensionTime;
	m_pBitmap->SelectActiveFrame(&pageGuid, m_frame_index);

	if (m_paused)
	{
		RECT r;
		r.left = m_ani_sx;
		r.top = m_ani_sy;
		r.right = r.left + m_ani_width;
		r.bottom = r.top + m_ani_height;
		::InvalidateRect(m_target_hwnd, &r, TRUE);
	}
}

//지정 % 위치의 프레임으로 이동
void CGdiplusBitmap::goto_frame_percent(int pos, bool pause)
{
	double dpos = ((double)pos / (double)m_frame_count) * 100.0;
	goto_frame((int)dpos, pause);
}

void CGdiplusBitmap::thread_gif_animation()
{
	if (m_frame_count < 2)
		return;

	HDC hDC = GetDC(m_target_hwnd);
	CDC* pDC = CDC::FromHandle(hDC);
	Gdiplus::SolidBrush brush_tr(m_cr_back);

	bool mirror_applied = false;

	CRect r(m_ani_sx, m_ani_sy, m_ani_sx + m_ani_width, m_ani_sy + m_ani_height);
	CMemoryDC dc(pDC, &r);
	Gdiplus::Graphics g(dc.GetSafeHdc());

	auto br_zigzag = get_zigzag_pattern(32);

	long t0 = clock();
	long t1;

	while (m_run_thread_animation)
	{
		GUID   pageGuid = Gdiplus::FrameDimensionTime;

		if (hDC)
		{
			CRect r(m_ani_sx, m_ani_sy, m_ani_sx + m_ani_width, m_ani_sy + m_ani_height);
			CMemoryDC dc(pDC, &r);
			Gdiplus::Graphics g(dc.GetSafeHdc());

			g.FillRectangle(br_zigzag.get(), CRect2GpRect(r));

			//if (m_cr_back.GetValue() != 
			//g.FillRectangle(&brush_tr, m_ani_sx, m_ani_sy, m_ani_width, m_ani_height);

			//CGdiButton과 같이 배경이 투명하게 표시하려 했으나 뭔가 다르다.
			/*
			CRect Rect = r;
			CWnd* pParent = CWnd::FromHandle(m_target_hwnd);
			ASSERT(pParent);
			pParent->ScreenToClient(&Rect);  //convert our corrdinates to our parents
			//copy what's on the parents at this point
			CDC* pParentDC = pParent->GetDC();
			CDC MemDC;
			CBitmap bmp;
			MemDC.CreateCompatibleDC(pParentDC);
			bmp.CreateCompatibleBitmap(pParentDC, Rect.Width(), Rect.Height());
			CBitmap* pOldBmp = MemDC.SelectObject(&bmp);
			MemDC.BitBlt(0, 0, Rect.Width(), Rect.Height(), pParentDC, Rect.left, Rect.top, SRCCOPY);
			pParentDC->BitBlt(m_ani_sx, m_ani_sy, Rect.Width(), Rect.Height(), &MemDC, 0, 0, SRCCOPY);
			MemDC.SelectObject(pOldBmp);
			pParent->ReleaseDC(pParentDC);
			MemDC.DeleteDC();
			*/
			//Graphics g(hDC, r);
			//배경색을 투명하게 한다 해도 이미지의 투명 영역이
			//parent에서도 투명하게 표시되진 않는다.
			//parent의 영역을 그려준 후 이미지를 그려줘야 한다.
			//if (!is_equal(m_cr_back, Gdiplus::Color(0, 0, 0, 0), 4))
			//{
			//	Gdiplus::SolidBrush brush_tr(m_cr_back);
			//	g.FillRectangle(&brush_tr, m_ani_sx, m_ani_sy, m_ani_width, m_ani_height);
			//}

			//save(_T("d:\\copy.png"));

			//mirror할 경우 m_pBitmap을 mirror()하면 gif정보가 사라지므로 clone한 이미지를 mirror하여 표시해야 한다.
			Gdiplus::Bitmap* pBitmap = m_pBitmap;
			if (m_is_gif_mirror)
			{
				pBitmap = m_pBitmap->Clone(0, 0, m_pBitmap->GetWidth(), m_pBitmap->GetHeight(), m_pBitmap->GetPixelFormat());
				pBitmap->RotateFlip(Gdiplus::RotateNoneFlipX);
			}

			//save(_T("d:\\copy.png"));
			g.DrawImage(pBitmap, m_ani_sx, m_ani_sy, m_ani_width, m_ani_height);

			//clone된 경우는 반드시 release해줘야 한다.
			if (pBitmap != m_pBitmap)
				SAFE_DELETE(pBitmap);
		}

		if (m_paused)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			continue;
		}

		::PostMessage(m_target_hwnd, Message_CGdiplusBitmap, (WPARAM)&CGdiplusBitmapMessage(m_pBitmap, message_gif_frame_changed, m_frame_index, m_frame_count), 0);

		m_frame_index++;
		if (m_frame_index >= m_frame_count)
			m_frame_index = 0;

		m_pBitmap->SelectActiveFrame(&pageGuid, m_frame_index);

		//replace_color(Gdiplus::Color(255, 76, 86, 164), Gdiplus::Color(0, 255, 112, 109));

		//delay는 해당 프레임에 설정된 delay를 그대로 주면 안되고
		//전 프레임부터 현 프레임까지 재생하는데 걸린 시간은 빼줘야 한다.
		t1 = clock();
		long display_delay = t1 - t0;
		long delay = ((long*)m_pPropertyItem->value)[m_frame_index] * 10;

		if (delay > display_delay)
			std::this_thread::sleep_for(std::chrono::milliseconds((delay - display_delay)));
		t0 = clock();
	}

	::ReleaseDC(m_target_hwnd, hDC);
}

void CGdiplusBitmap::goto_gif_frame(int frame)
{
	if (m_frame_count < 2 || frame >= m_frame_count)
		return;

	HDC hDC = GetDC(m_target_hwnd);
	CDC* pDC = CDC::FromHandle(hDC);
	CRect r(m_ani_sx, m_ani_sy, m_ani_sx + m_ani_width, m_ani_sy + m_ani_height);

	//CRect Rect = r;
	//CWnd* pParent = CWnd::FromHandle(m_target_hwnd);
	//ASSERT(pParent);
	//pParent->ScreenToClient(&Rect);  //convert our corrdinates to our parents
	////copy what's on the parents at this point
	//CDC* pParentDC = pParent->GetDC();
	//CDC MemDC;
	//CBitmap bmp;
	//MemDC.CreateCompatibleDC(pParentDC);
	//bmp.CreateCompatibleBitmap(pParentDC, Rect.Width(), Rect.Height());
	//CBitmap* pOldBmp = MemDC.SelectObject(&bmp);
	//MemDC.BitBlt(0, 0, Rect.Width(), Rect.Height(), pParentDC, Rect.left, Rect.top, SRCCOPY);
	//pParentDC->BitBlt(0, 0, Rect.Width(), Rect.Height(), &MemDC, 0, 0, SRCCOPY);
	//MemDC.SelectObject(pOldBmp);
	//pParent->ReleaseDC(pParentDC);
	//MemDC.DeleteDC();

	GUID   pageGuid = Gdiplus::FrameDimensionTime;

	if (hDC)
	{
		CMemoryDC dc(pDC, &r);
		Gdiplus::Graphics g(dc.m_hDC, r);

		m_pBitmap->SelectActiveFrame(&pageGuid, frame);

		//Graphics g(hDC, r);
		//배경색을 투명하게 한다 해도 이미지의 투명 영역이
		//parent에서도 투명하게 표시되진 않는다.
		//parent의 영역을 그려준 후 이미지를 그려줘야 한다.
		if (!is_equal(m_cr_back, Gdiplus::Color(0, 0, 0, 0), 4))
		{
			Gdiplus::SolidBrush brush_tr(m_cr_back);
			g.FillRectangle(&brush_tr, m_ani_sx, m_ani_sy, m_ani_width, m_ani_height);
		}

		g.DrawImage(m_pBitmap, m_ani_sx, m_ani_sy, m_ani_width, m_ani_height);
		::SendMessage(m_target_hwnd, Message_CGdiplusBitmap, (WPARAM)&CGdiplusBitmapMessage(m_pBitmap, message_gif_frame_changed, m_frame_index, m_frame_count), 0);
	}

	::ReleaseDC(m_target_hwnd, hDC);
}

void CGdiplusBitmap::save_multi_image()//std::vector<Gdiplus::Bitmap*>& dqBitmap)
{
	std::vector<Gdiplus::Bitmap*> dq;
	std::vector<long> delay;
	get_gif_frames(dq, delay);

	CString str;

	Gdiplus::EncoderParameters	encoderParameters;
	ULONG				parameterValue;
	Gdiplus::Status				stat;

	// An EncoderParameters object has an array of
   // EncoderParameter objects. In this case, there is only
   // one EncoderParameter object in the array.
	encoderParameters.Count = 1;

	// Initialize the one EncoderParameter object.
	encoderParameters.Parameter[0].Guid = Gdiplus::EncoderSaveFlag;// FrameDimensionTime;// Gdiplus::ImageFormatGIF;// EncoderSaveFlag;
	encoderParameters.Parameter[0].Type = Gdiplus::EncoderParameterValueType::EncoderParameterValueTypeLong;
	encoderParameters.Parameter[0].NumberOfValues = 1;
	encoderParameters.Parameter[0].Value = &parameterValue;

	// Get the CLSID of the TIFF encoder.
	CLSID encoderClsid;
	GetEncoderClsid(L"image/gif", &encoderClsid);

	// Set the loop count.
	Gdiplus::PropertyItem* propItemLoopCount = new Gdiplus::PropertyItem;
	SHORT loopCount = 0; //A value of 0 specifies that the animation should be displayed infinitely.

	propItemLoopCount->id = PropertyTagLoopCount;
	propItemLoopCount->length = sizeof(loopCount);
	propItemLoopCount->type = PropertyTagTypeShort;
	propItemLoopCount->value = &loopCount;

	for (size_t i = 0; i < dq.size(); i++)
	{
		if (i == 0)
		{
			dq[0]->SetPropertyItem(propItemLoopCount);

			// Save the first page (frame).
			parameterValue = Gdiplus::EncoderValue::EncoderValueMultiFrame;
			stat = dq[0]->Save(L"d:\\temp\\MultiFrame.gif", &encoderClsid, NULL);// &encoderParameters);
			if (stat == Gdiplus::Ok)
				TRACE(_T("Page %d saved successfully.\n"), i);
		}
		else
		{
			//뭘 줘도 Win32Error가 리턴된다...
			// Save the second page (frame).
			//parameterValue = Gdiplus::EncoderValue::EncoderValueMultiFrame;
			//stat = dq[0]->SaveAdd(dq[i], &encoderParameters);
			//parameterValue = Gdiplus::EncoderValue::EncoderValueVersionGif89;
			//stat = dq[0]->SaveAdd(dq[i], &encoderParameters);
			//parameterValue = Gdiplus::EncoderValue::EncoderValueFrameDimensionTime;
			//stat = dq[0]->SaveAdd(dq[i], &encoderParameters);
			parameterValue = Gdiplus::EncoderValue::EncoderValueFrameDimensionTime;
			stat = dq[0]->SaveAdd(dq[i], &encoderParameters);

			if (stat == Gdiplus::Ok)
				TRACE(_T("Page %d saved successfully.\n"), i);
		}
	}

	// Close the multiframe file.
	parameterValue = Gdiplus::EncoderValue::EncoderValueFlush;
	stat = dq[0]->SaveAdd(&encoderParameters);
	if (stat == Gdiplus::Ok)
		printf("File closed successfully.\n");

	//str.Format(_T("z:\\내 드라이브\\media\\test_image\\temp\\multi.tif"));
	//save(dq[0], str);
}

//투명 png의 l, t, r, b의 투명한 영역 크기를 구한다.
CRect CGdiplusBitmap::get_transparent_rect()
{
	CRect r;

	if (is_empty())
		return r;

	//이미지가 큰 경우 탐색시간이 길어지므로 작게 줄여서 검사한다. 미완성!
	double ratio = 1.0;

	CGdiplusBitmap	temp;
	deep_copy(&temp);
	//temp.save(_T("d:\\temp.png"));
	//if (width > height)
	//{
	//	temp.resize(50, 0);
	//	ratio = (double)width / (double)temp.width;
	//}
	//else
	//{
	//	temp.resize(0, 50);
	//	ratio = (double)height / (double)temp.height;
	//}

	temp.get_raw_data();

	int x, y;

	//scan left
	for (x = 0; x < width; x++)
	{
		for (y = 0; y < height; y++)
		{
			if (temp.get_pixel(x, y).GetValue() != Gdiplus::Color::Transparent)
				r.left = x;
		}
	}

	//scan top
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			if (temp.get_pixel(x, y).GetValue() != Gdiplus::Color::Transparent)
				r.top = y;
		}
	}

	//scan right
	for (x = width - 1; x >= 0; x--)
	{
		for (y = 0; y < height; y++)
		{
			if (temp.get_pixel(x, y).GetValue() != Gdiplus::Color::Transparent)
				r.right = x;
		}
	}


	//scan bottom
	for (y = height - 1; y >= 0; y--)
	{
		for (x = 0; x < width; x++)
		{
			if (temp.get_pixel(x, y).GetValue() != Gdiplus::Color::Transparent)
				r.bottom = y;
		}
	}

	return r;
}

//data를 추출하여 주소값으로 구하므로 속도가 빠름. get_raw_data()를 호출하여 data를 추출한 경우에만 사용할 것!
//단, indexed 8bit 이미지에 대한 처리는 아직 구현되지 않음.
Gdiplus::Color CGdiplusBitmap::get_pixel(int x, int y)
{
	//data가 불려지지 않은 상태에서 get_pixel()이 호출되면 raw_data를 추출한다.
	if (data == NULL)
		get_raw_data();

	if (x < 0 || x >= width || y < 0 || y >= height)
		return Gdiplus::Color(0, 0, 0, 0);

	if (channel == 1)
		return Gdiplus::Color(*(data + y * width * channel + x * channel));

	else if (channel == 3)
		return Gdiplus::Color(	*(data + y * width * channel + x * channel + 2),
								*(data + y * width * channel + x * channel + 1),
								*(data + y * width * channel + x * channel + 0));

	return Gdiplus::Color(	*(data + y * width * channel + x * channel + 3),
							*(data + y * width * channel + x * channel + 2),
							*(data + y * width * channel + x * channel + 1),
							*(data + y * width * channel + x * channel + 0));
}
