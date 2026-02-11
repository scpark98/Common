#pragma once

#include <afxwin.h>
#include <d2d1_1.h>
#include <d2d1helper.h>
#include <d2d1effects_2.h>
#include <d3d11.h>
#include <wincodec.h>
#include <wrl/client.h>

/*
[초기화 및 사용 방법]
- CSCD2Context, CSCD2Image 두 클래스를 프로젝트에 추가.

- .h 파일에 include 추가. (이미지 로드 및 디스플레이 기능이 필요없다면 CSCD2Image 제외 가능)
	#include "Common/directx/CSCD2Context/SCD2Context.h"
	#include "Common/directx/CSCD2Image/SCD2Image.h"

- 멤버 변수로 CSCD2Context, CSCD2Image 객체 선언.
	CSCD2Context		m_d2dc;
	CSCD2Image			m_img;

- OnInitDialog() 등 초기화 루틴에서 CSCD2Context::init() 호출.
	HRESULT hr = m_d2dc.init(m_hWnd);
	m_img.load(m_d2dc.get_WICFactory(), m_d2dc.get_d2dc(), _T("D:\\test.jpg"));

- OnPaint()에서 다음과 같이 이미지 렌더링.
		//이 한줄을 안써주면 컨트롤을 클릭해도 화면이 갱신되지 않는다.
		CPaintDC dc(this);

		CRect rc;
		GetClientRect(rc);

		ID2D1DeviceContext* d2dc = m_d2dc.get_d2dc();
		D2D1_SIZE_F dc_size = m_d2dc.get_size();

		d2dc->BeginDraw();
		d2dc->SetTransform(D2D1::Matrix3x2F::Identity());

		d2dc->Clear(get_sys_d2color(COLOR_3DFACE));

		if (m_img.is_valid())
		{
			//m_img.draw(d2dc, eSCD2Image_DRAW_MODE::draw_mode_original_center);
			m_img.draw(d2dc, 0, 0);
		}

		HRESULT hr = d2dc->EndDraw();

		if (SUCCEEDED(hr))
			hr = m_d2dc.get_swapchain()->Present(0, 0);

- OnSize()에서
	if (m_d2dc.get_d2dc() != nullptr)
	{
		m_d2dc.on_size_changed(cx, cy);
		Invalidate();
	}

 
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

	HRESULT						init(HWND hWnd, int cx = 0, int cy = 0);
	IWICImagingFactory2*		get_WICFactory() { return m_WICFactory.Get(); }
	ID2D1Factory1*				get_factory() { return m_d2factory.Get(); }
	ID2D1DeviceContext*			get_d2dc() { return m_d2context.Get(); }
	IDXGISwapChain*				get_swapchain() { return m_swapchain.Get(); }

	D2D1_SIZE_F					get_size() { return m_d2context->GetSize(); }
	ComPtr<ID2D1BitmapBrush>	get_zigzag_brush() { return m_br_zigzag; }

	HRESULT						on_size_changed(int cx, int cy);

	HRESULT						save(CString path);

protected:
	D2D1_SIZE_F					m_sz;
	HWND						m_hWnd;
	ComPtr<ID2D1Factory1>       m_d2factory;
	ComPtr<IWICImagingFactory2> m_WICFactory;
	ComPtr<ID2D1Device>         m_d2device;
	ComPtr<ID2D1DeviceContext>  m_d2context;
	ComPtr<IDXGISwapChain>      m_swapchain;
	//ComPtr<ID2D1Bitmap1>		m_target_bitmap;
	//ComPtr<ID2D1DCRenderTarget>	m_pDCRT;
	//CComQIPtr<ID2D1GdiInteropRenderTarget> m_spGdiInteropRenderTarget;

	HRESULT						create_factory();
	HRESULT						create_device_resources();
	HRESULT						create_device_context();

	ComPtr<ID2D1BitmapBrush>	m_br_zigzag;
	ComPtr<ID2D1BitmapBrush>	create_zigzag_brush(float cell_size = 8.f, byte fore = 200, byte back = 255);
};

