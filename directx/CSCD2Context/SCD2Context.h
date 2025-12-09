#pragma once

#include <afxwin.h>
#include <d2d1_1.h>
#include <d2d1helper.h>
#include <d2d1effects_2.h>
#include <d3d11.h>
#include <wincodec.h>
#include <wrl/client.h>

/*
ComPtr::operator&의 잘못된 사용 : operator&는 ComPtr가 관리하는 인터페이스를 먼저 해제한 다음, 해당 주소를 검색합니다.
ComPtr를 in / out 매개변수로 전달할 때 GetAddressOf() 대신 operator&를 사용하면 기존 포인터가 의도치 않게 해제될 수 있습니다.
해결책 : 함수가 포인터의 주소를 받으면서 기존 포인터를 보존해야 하는 경우 ComPtr::GetAddressOf()를 사용하십시오.

[CSCD2Context를 MFC Control과 함께 사용할 경우]
- 해당 윈도우에서 반드시 Clip Children을 해줘야만 Invalidate()에서 Control들이 깜빡이지 않는다.

*/

using namespace Microsoft::WRL;

class CSCD2Context
{
public:
	CSCD2Context();
	~CSCD2Context();

	HRESULT						init(HWND hWnd);
	IWICImagingFactory2*		get_WICFactory() { return m_WICFactory.Get(); }
	ID2D1Factory1*				get_factory() { return m_d2factory.Get(); }
	ID2D1DeviceContext*			get_d2dc() { return m_d2context.Get(); }
	//ID2D1DCRenderTarget*		get_d2DC() { return m_pDCRT.Get(); }
	IDXGISwapChain*				get_swapchain() { return m_swapchain.Get(); }

	D2D1_SIZE_F					get_size() { return m_d2context->GetSize(); }
	ComPtr<ID2D1BitmapBrush>	get_zigzag_brush() { return m_br_zigzag; }

	HRESULT						on_size_changed(int cx, int cy);

	//ID2D1PathGeometry*			create_round_rect(int x, int y, int width, int height, int leftTop, int rightTop, int rightBottom, int leftBottom);

protected:
	HWND						m_hWnd;
	ComPtr<ID2D1Factory1>       m_d2factory;
	ComPtr<IWICImagingFactory2> m_WICFactory;
	ComPtr<ID2D1Device>         m_d2device;
	ComPtr<ID2D1DeviceContext>  m_d2context;
	ComPtr<IDXGISwapChain>      m_swapchain;
	//ComPtr<ID2D1DCRenderTarget>	m_pDCRT;
	//CComQIPtr<ID2D1GdiInteropRenderTarget> m_spGdiInteropRenderTarget;

	HRESULT						create_factory();
	HRESULT						create_device_resources();
	HRESULT						create_device_context();

	ComPtr<ID2D1BitmapBrush>	m_br_zigzag;
	ComPtr<ID2D1BitmapBrush>	create_zigzag_brush(float cell_size = 8.f, byte fore = 200, byte back = 255);
};

