#pragma once

#include <afxwin.h>
#include <gdiplus.h>

using namespace Gdiplus;


class CGdiPlusBitmap
{
public:
	Gdiplus::Bitmap* m_pBitmap = NULL;

public:
	CGdiPlusBitmap();
	CGdiPlusBitmap(Bitmap* src);
	CGdiPlusBitmap(LPCWSTR pFile);
	virtual ~CGdiPlusBitmap();

	void release();

	bool Load(LPCWSTR pFile)
	{
		release();
		m_pBitmap = Gdiplus::Bitmap::FromFile(pFile);
		if (m_pBitmap->GetLastStatus() == Gdiplus::Ok)
		{
			resolution();
			return true;
		}

		return false;
	}

	operator Gdiplus::Bitmap*() const			{ return m_pBitmap; }

	bool empty() { return (m_pBitmap == NULL); }
	int channels();
	CSize size() { return CSize(width, height); }

	CRect draw_image(CDC* pDC, CRect r, CRect* targetRect = NULL, Color crBack = Color(255, 0, 0, 0));
	CRect draw_image(CDC* pDC, int x, int y, int w, int h, CRect* targetRect = NULL, Color crBack = Color(255, 0, 0, 0));

	//Gdiplus::Bitmap::Clone�� shallow copy�̹Ƿ� ������ ���縦 ���ؼ��� deep_copy�� ����ؾ� �Ѵ�.
	void clone(CGdiPlusBitmap* dst);
	void deep_copy(CGdiPlusBitmap* dst);
	void rotate(Gdiplus::RotateFlipType type);
	//ȸ����Ű�� �̹����� ������ w, h�� ����Ƿ� ĵ������ �ڵ����� Ű����� �߸��� �ʴ´�.
	void rotate(float degree, bool auto_enlarge = false);
	void set_colorkey(Color low, Color high);

	void set_transparent(float transparent);
	void gray();
	void negative();

	int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);
	bool save(CString filename);// , ULONG quality/* = 100*/);

	int width = 0;
	int height = 0;
	int channel = 0;

protected:
	void resolution();
};


class CGdiPlusBitmapResource : public CGdiPlusBitmap
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
	CGdiPlusBitmap::release();
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
