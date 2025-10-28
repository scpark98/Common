#pragma once

#include <afxwin.h>
#include <d2d1_1.h>
#include <d2d1helper.h>
#include <d2d1effects_2.h>
#include <d3d11.h>
#include <wincodec.h>
#include <wrl/client.h>

/*
ComPtr::operator&�� �߸��� ��� : operator&�� ComPtr�� �����ϴ� �������̽��� ���� ������ ����, �ش� �ּҸ� �˻��մϴ�.
ComPtr�� in / out �Ű������� ������ �� GetAddressOf() ��� operator&�� ����ϸ� ���� �����Ͱ� �ǵ�ġ �ʰ� ������ �� �ֽ��ϴ�.
�ذ�å : �Լ��� �������� �ּҸ� �����鼭 ���� �����͸� �����ؾ� �ϴ� ��� ComPtr::GetAddressOf()�� ����Ͻʽÿ�.
*/

using namespace Microsoft::WRL;

class CSCD2Context
{
public:
	CSCD2Context();
	~CSCD2Context();

	HRESULT						init(HWND hWnd);
	IWICImagingFactory2*		get_WICFactory() { return m_WICFactory.Get(); }
	ID2D1DeviceContext*			get_d2dc() { return m_d2context.Get(); }
	IDXGISwapChain*				get_swapchain() { return m_swapchain.Get(); }

	D2D1_SIZE_F					get_size() { return m_d2context->GetSize(); }
	ComPtr<ID2D1BitmapBrush>	get_zigzag_brush() { return m_br_zigzag; }

protected:
	HWND						m_hWnd;
	ComPtr<ID2D1Factory1>       m_d2factory;
	ComPtr<IWICImagingFactory2> m_WICFactory;
	ComPtr<ID2D1Device>         m_d2device;
	ComPtr<ID2D1DeviceContext>  m_d2context;
	ComPtr<IDXGISwapChain>      m_swapchain;

	HRESULT						create_factory();
	HRESULT						create_device_resources();
	HRESULT						create_device_context();

	ComPtr<ID2D1BitmapBrush>	m_br_zigzag;
	ComPtr<ID2D1BitmapBrush>	create_zigzag_brush(float cell_size = 8.f, byte fore = 200, byte back = 255);
};

