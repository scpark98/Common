#pragma once

#include <afxwin.h>
#include <d2d1_1.h>
#include <d2d1helper.h>
#include <d2d1effects_2.h>
#include <d3d11.h>
#include <wincodec.h>
#include <wrl/client.h>

using namespace Microsoft::WRL;

class CSCD2Image
{
public:
	CSCD2Image();
	~CSCD2Image();

	HRESULT					load(IWICImagingFactory2* WICfactory, ID2D1DeviceContext* d2context, CString path);
	HRESULT					load(IWICImagingFactory2* WICfactory, ID2D1DeviceContext* d2context, UINT resource_id, CString type = _T("PNG"));
	HRESULT					on_resize(ID2D1DeviceContext* d2context, IDXGISwapChain* swapchain, int cx, int cy);
	ID2D1Bitmap*			get() { return m_img.Get(); }

protected:
	ComPtr<ID2D1Bitmap>		m_img;
	HRESULT					load(IWICImagingFactory2* WICfactory, ID2D1DeviceContext* d2context, IWICBitmapFrameDecode* source);
};

