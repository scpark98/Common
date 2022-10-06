#pragma once

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
	virtual ~CGdiplusBitmap();

	void release();

	bool Load(LPCWSTR pFile);

	operator Gdiplus::Bitmap*() const { return m_pBitmap; }

	//기본적으로는 이미지 raw data를 가지고 있지 않으나
	//cv::Mat의 data처럼 raw data가 필요한 경우 이 함수를 호출하면 사용 가능하다.
	bool get_raw_data();

	bool empty() { return (m_pBitmap == NULL); }
	int channels();
	CSize size() { return CSize(cols, rows); }

	CRect draw(CDC* pDC, CRect r, CRect* targetRect = NULL, Color crBack = Color(255, 0, 0, 0));
	CRect draw(CDC* pDC, int x = 0, int y = 0, int w = 0, int h = 0, CRect* targetRect = NULL, Color crBack = Color(255, 0, 0, 0));

	//Gdiplus::Bitmap::Clone은 shallow copy이므로 완전한 복사를 위해서는 deep_copy를 사용해야 한다.
	void clone(CGdiplusBitmap* dst);
	void deep_copy(CGdiplusBitmap* dst);
	void rotate(Gdiplus::RotateFlipType type);
	//회전시키면 w, h가 달라지므로 늘리거나 줄여줘야 한다.
	void rotate(float degree, bool auto_resize = false, Color remove_back_color = Color(0,0,0,0));
	//InterpolationModeNearestNeighbor		: 원본 화소를 거의 유지하지만 일부 화소는 사라짐. 그래서 더 거친 느낌
	//InterpolationModeHighQualityBilinear	: 부드럽게 resize되지만 약간 뿌옇게 변함
	//InterpolationModeHighQualityBicubic	: 속도는 느리지만 최고 품질 모드
	void resize(int cx, int cy, Gdiplus::InterpolationMode = Gdiplus::InterpolationModeHighQualityBicubic);
	void sub_image(int x, int y, int w, int h);
	void sub_image(CRect r);
	void fit_to_image(Color remove_back_color = Color(0, 0, 0, 0));
	void set_colorkey(Color low, Color high);
	bool is_equal(Color cr0, Color cr1, int channel = 3);

	void set_transparent(float transparent);
	void gray();
	void negative();
	void convert2gray();

	int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);
	bool save(CString filename);// , ULONG quality/* = 100*/);
	bool copy_to_clipbard();

	int cols = 0;
	int rows = 0;
	int channel = 0;
	int stride = 0;

protected:
	void resolution();
};


class CGdiPlusBitmapResource : public CGdiplusBitmap
{
protected:
	HGLOBAL m_hBuffer;

public:
	CGdiPlusBitmapResource()					{ m_hBuffer = NULL; }
	CGdiPlusBitmapResource(LPCTSTR pName, LPCTSTR pType = RT_RCDATA, HMODULE hInst = NULL)
												{ m_hBuffer = NULL; Load(pName, pType, hInst); }
	CGdiPlusBitmapResource(UINT id, LPCTSTR pType = RT_RCDATA, HMODULE hInst = NULL)
												{ m_hBuffer = NULL; Load(id, pType, hInst); }
	CGdiPlusBitmapResource(UINT id, UINT type, HMODULE hInst = NULL)
												{ m_hBuffer = NULL; Load(id, type, hInst); }
	virtual ~CGdiPlusBitmapResource()			{ release(); }

	void release();

	bool Load(LPCTSTR pName, LPCTSTR pType = RT_RCDATA, HMODULE hInst = NULL);
	bool Load(UINT id, LPCTSTR pType = RT_RCDATA, HMODULE hInst = NULL)
												{ return Load(MAKEINTRESOURCE(id), pType, hInst); }
	bool Load(UINT id, UINT type, HMODULE hInst = NULL)
												{ return Load(MAKEINTRESOURCE(id), MAKEINTRESOURCE(type), hInst); }
};

inline
void CGdiPlusBitmapResource::release()
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
bool CGdiPlusBitmapResource::Load(LPCTSTR pName, LPCTSTR pType, HMODULE hInst)
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
