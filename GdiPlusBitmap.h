#pragma once

/*
scpark.
기존 CGdiPlusBitmap 및 CGdiPlusBitmapResource를 CGdiplusBitmap이라는 하나의 클래스로 합치고
Gdiplus에서 제공하는 다양한 이미지 효과를 추가함.
*/

#include <afxwin.h>
#include <gdiplus.h>
#include <stdint.h>	//for uint8_t in vs2015
#include <algorithm>

using namespace Gdiplus;

class CGdiplusBitmap
{
public:
	Gdiplus::Bitmap* m_pBitmap = NULL;
	uint8_t* data = NULL;

public:
	CGdiplusBitmap();
	CGdiplusBitmap(Bitmap* src);
	CGdiplusBitmap(HBITMAP hBitmap);
	CGdiplusBitmap(LPCWSTR pFile);
	CGdiplusBitmap(CGdiplusBitmap* src);
	CGdiplusBitmap(LPCTSTR lpType, UINT id);

	virtual ~CGdiplusBitmap();

	void	release();

	//CGdiplusBitmap과 CGdiplusBitmapResource 두 개의 클래스가 있었으나 통합함.
	bool	load(LPCWSTR pFile);
	void	load(LPCTSTR lpType, UINT id);

	Gdiplus::Bitmap* CreateARGBBitmapFromDIB(const DIBSECTION& dib);


	operator Gdiplus::Bitmap*() const { return m_pBitmap; }

	//기본적으로는 이미지 raw data를 추출하진 않는다.
	//cv::Mat의 data처럼 raw data가 필요한 경우에 이 함수를 호출하면 사용이 가능해진다.
	bool	get_raw_data();

	bool	empty() { return (m_pBitmap == NULL); }
	bool	valid() { return (m_pBitmap != NULL); }
	int		channels();
	CSize	size() { return CSize(width, height); }

	CRect	draw(CDC* pDC, CRect r, CRect* targetRect = NULL, Color crBack = Color(255, 0, 0, 0));
	CRect	draw(CDC* pDC, int x = 0, int y = 0, int w = 0, int h = 0, CRect* targetRect = NULL, Color crBack = Color(255, 0, 0, 0));

	//Gdiplus::Bitmap::Clone은 shallow copy이므로 완전한 복사를 위해서는 deep_copy를 사용해야 한다.
	void	clone(CGdiplusBitmap* dst);
	void	deep_copy(CGdiplusBitmap* dst);
	void	rotate(Gdiplus::RotateFlipType type);

	//회전시키면 w, h가 달라지므로 이미지의 크기를 보정해줘야만 하는 경우도 있다.
	//그럴 경우는 auto_resize를 true로 주고 불필요한 배경이 생겼을 경우는
	//불필요한 배경의 색상을 지정하여 이미지 크기를 fit하게 줄일수도 있다.
	void rotate(float degree, bool auto_resize = false, Color remove_back_color = Color(0,0,0,0));

	//InterpolationModeNearestNeighbor		: 원본 화소를 거의 유지하지만 일부 화소는 사라짐. 그래서 더 거친 느낌
	//InterpolationModeHighQualityBilinear	: 부드럽게 resize되지만 약간 뿌옇게 변함
	//InterpolationModeHighQualityBicubic	: 속도는 느리지만 최고 품질 모드
	void resize(int cx, int cy, Gdiplus::InterpolationMode = Gdiplus::InterpolationModeHighQualityBicubic);
	void sub_image(int x, int y, int w, int h);
	void sub_image(CRect r);
	void sub_image(Gdiplus::Rect r);
	void fit_to_image(Color remove_back_color = Color(0, 0, 0, 0));
	void set_colorkey(Color low, Color high);
	bool is_equal(Color cr0, Color cr1, int channel = 3);

	void set_matrix(ColorMatrix *colorMatrix, ColorMatrix *grayMatrix = NULL);
	void set_transparent(float transparent);
	void gray();
	void negative();

	//ColorMatrix를 이용하여 간단히 흑백이미지를 만들 수 있지만
	//그건 3채널의 흑백톤의 이미지이므로 1채널 256 gray이미지가 아니다.
	void convert2gray();

	int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);
	bool save(CString filename);// , ULONG quality/* = 100*/);
	bool copy_to_clipbard();

	int width = 0;
	int height = 0;
	int channel = 0;
	int stride = 0;

protected:
	void resolution();
	Bitmap* GetImageFromResource(LPCTSTR lpType, UINT id);
};

#if 0
class CGdiplusBitmapResource : public CGdiplusBitmap
{
protected:
	HGLOBAL m_hBuffer;

public:
	CGdiplusBitmapResource()					{ m_hBuffer = NULL; }
	CGdiplusBitmapResource(LPCTSTR pName, LPCTSTR pType = RT_RCDATA, HMODULE hInst = NULL)
												{ m_hBuffer = NULL; Load(pName, pType, hInst); }
	CGdiplusBitmapResource(UINT id, LPCTSTR pType = RT_RCDATA, HMODULE hInst = NULL)
												{ m_hBuffer = NULL; Load(id, pType, hInst); }
	CGdiplusBitmapResource(UINT id, UINT type, HMODULE hInst = NULL)
												{ m_hBuffer = NULL; Load(id, type, hInst); }
	virtual ~CGdiplusBitmapResource()			{ release(); }

	void release();

	bool Load(LPCTSTR pName, LPCTSTR pType = RT_RCDATA, HMODULE hInst = NULL);
	bool Load(UINT id, LPCTSTR pType = RT_RCDATA, HMODULE hInst = NULL)
												{ return Load(MAKEINTRESOURCE(id), pType, hInst); }
	bool Load(UINT id, UINT type, HMODULE hInst = NULL)
												{ return Load(MAKEINTRESOURCE(id), MAKEINTRESOURCE(type), hInst); }
};

inline
void CGdiplusBitmapResource::release()
{
	CGdiplusBitmap::release();
	if (m_hBuffer)
	{
		::GlobalUnlock(m_hBuffer);
		::GlobalFree(m_hBuffer);
		m_hBuffer = NULL;
	} 
}

inline
bool CGdiplusBitmapResource::Load(LPCTSTR pName, LPCTSTR pType, HMODULE hInst)
{
	release();

	HRSRC hResource = ::FindResource(hInst, pName, pType);
	if (!hResource)
		return false;
	
	DWORD imageSize = ::SizeofResource(hInst, hResource);
	if (!imageSize)
		return false;

	const void* pResourceData = ::LockResource(::LoadResource(hInst, hResource));
	if (!pResourceData)
		return false;

	m_hBuffer  = ::GlobalAlloc(GMEM_MOVEABLE, imageSize);
	if (m_hBuffer)
	{
		void* pBuffer = ::GlobalLock(m_hBuffer);
		if (pBuffer)
		{
			CopyMemory(pBuffer, pResourceData, imageSize);

			IStream* pStream = NULL;
			if (::CreateStreamOnHGlobal(m_hBuffer, FALSE, &pStream) == S_OK)
			{
				m_pBitmap = Gdiplus::Bitmap::FromStream(pStream);
				pStream->Release();
				if (m_pBitmap)
				{ 
					if (m_pBitmap->GetLastStatus() == Gdiplus::Ok)
					{
						resolution();
						return true;
					}

					delete m_pBitmap;
					m_pBitmap = NULL;
				}
			}
			::GlobalUnlock(m_hBuffer);
		}
		::GlobalFree(m_hBuffer);
		m_hBuffer = NULL;
	}
	return false;
}
#endif