#include "SCD2Context.h"

#pragma comment(lib, "d2d1.lib")

CSCD2Context::CSCD2Context()
{
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	//CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
}

CSCD2Context::~CSCD2Context()
{
	CoUninitialize();
}

HRESULT CSCD2Context::init(HWND hWnd)
{
	m_hWnd = hWnd;
	create_factory();
	create_device_resources();
	return S_OK;
}

HRESULT CSCD2Context::create_factory()
{
	HRESULT hr = S_OK;

	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, IID_PPV_ARGS(m_d2factory.GetAddressOf()));

	if (SUCCEEDED(hr))
	{
		hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(m_WICFactory.GetAddressOf()));
	}

	return hr;
}

HRESULT CSCD2Context::create_device_resources()
{
	HRESULT hr = S_OK;

	if (!m_d2context) {
		hr = create_device_context();

		ComPtr<IDXGISurface> surface = nullptr;
		if (SUCCEEDED(hr)) {
			hr = m_swapchain->GetBuffer(
				0,
				IID_PPV_ARGS(surface.GetAddressOf())
			);
		}
		ComPtr<ID2D1Bitmap1> bitmap = nullptr;
		if (SUCCEEDED(hr))
		{
			FLOAT dpiX, dpiY;
			dpiX = (FLOAT)GetDpiForWindow(::GetDesktopWindow());
			//dpiX = 96.0f;// (FLOAT)GetSystemMetricsForDpi(SM_MOUSEPRESENT);
			dpiY = dpiX;

			D2D1_BITMAP_PROPERTIES1 properties = D2D1::BitmapProperties1(
				D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
				D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), dpiX, dpiY);

			hr = m_d2context->CreateBitmapFromDxgiSurface(
				surface.Get(),
				&properties,
				&bitmap
			);
		}
		if (SUCCEEDED(hr)) {
			m_d2context->SetTarget(bitmap.Get());
			//D2D1_ANTIALIAS_MODE_ALIASED로 하지 않고 수평 라인을 그리면 stroke width가 1.0, 2.0의 두께가 동일하게 표시된다.
			//1.0이 2.0보다는 약간 옅은색으로 표시되나 thickness가 동일하게 표시된다. 이 세팅을 해줘야 실제 1 pixel 두께로 그려진다.
			m_d2context->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
		}
	}

	return hr;
}

HRESULT CSCD2Context::create_device_context()
{
	HRESULT hr = S_OK;

	RECT rc;
	::GetClientRect(m_hWnd, &rc);

	D2D1_SIZE_U size;
	size.width = rc.right;
	size.height = rc.bottom;

	UINT createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
	};
	UINT countOfDriverTypes = ARRAYSIZE(driverTypes);

	DXGI_SWAP_CHAIN_DESC swapDescription;
	ZeroMemory(&swapDescription, sizeof(swapDescription));
	swapDescription.BufferDesc.Width = size.width;
	swapDescription.BufferDesc.Height = size.height;
	swapDescription.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapDescription.BufferDesc.RefreshRate.Numerator = 60;
	swapDescription.BufferDesc.RefreshRate.Denominator = 1;
	swapDescription.SampleDesc.Count = 1;
	swapDescription.SampleDesc.Quality = 0;
	swapDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapDescription.BufferCount = 1;
	swapDescription.OutputWindow = m_hWnd;
	swapDescription.Windowed = TRUE;

	ComPtr<ID3D11Device> d3dDevice;
	for (UINT driverTypeIndex = 0; driverTypeIndex < countOfDriverTypes; driverTypeIndex++)
	{
		hr = D3D11CreateDeviceAndSwapChain(
			nullptr,
			driverTypes[driverTypeIndex],
			nullptr,
			createDeviceFlags,
			nullptr,
			0,
			D3D11_SDK_VERSION,
			&swapDescription,
			m_swapchain.GetAddressOf(),
			d3dDevice.GetAddressOf(),
			nullptr,
			nullptr
		);

		if (SUCCEEDED(hr))
		{
			break;
		}
	}

	ComPtr<IDXGIDevice> dxgiDevice;
	if (SUCCEEDED(hr))
		hr = d3dDevice->QueryInterface(IID_PPV_ARGS(dxgiDevice.GetAddressOf()));

	if (SUCCEEDED(hr))
		hr = m_d2factory->CreateDevice(dxgiDevice.Get(), m_d2device.GetAddressOf());

	if (SUCCEEDED(hr))
		hr = m_d2device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, m_d2context.GetAddressOf());

	m_br_zigzag = create_zigzag_brush();

	return hr;
}

ComPtr<ID2D1BitmapBrush> CSCD2Context::create_zigzag_brush(float cell_size, byte fore, byte back)
{
	const UINT w = 2, h = 2, stride = w * 4;
	BYTE pixels[stride * h];
	auto px = [&](UINT x, UINT y, BYTE B, BYTE G, BYTE R, BYTE A) {
		BYTE* p = pixels + y * stride + x * 4; p[0] = B; p[1] = G; p[2] = R; p[3] = A;
		};
	// W B / B W
	px(0, 0, fore, fore, fore, 255); px(1, 0, back, back, back, 255);
	px(0, 1, back, back, back, 255);       px(1, 1, fore, fore, fore, 255);

	D2D1_BITMAP_PROPERTIES props =
		D2D1::BitmapProperties(
			D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
			96.0f, 96.0f);

	ComPtr<ID2D1Bitmap> bmp;
	if (FAILED(m_d2context->CreateBitmap(D2D1::SizeU(w, h), pixels, stride, &props, &bmp))) return nullptr;

	D2D1_BITMAP_BRUSH_PROPERTIES bbp =
		D2D1::BitmapBrushProperties(D2D1_EXTEND_MODE_WRAP,
			D2D1_EXTEND_MODE_WRAP,
			D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);

	ComPtr<ID2D1BitmapBrush> brush;
	if (FAILED(m_d2context->CreateBitmapBrush(bmp.Get(), bbp, &brush))) return nullptr;

	// Each source pixel becomes one checker cell of size 'cell'
	brush->SetTransform(D2D1::Matrix3x2F::Scale(cell_size, cell_size));
	return brush;
}

/*
bool CSCD2Context::load(HWND hWnd, CString path)
{
	m_hWnd = hWnd;

	if (m_img)
		m_img->Release();

	if (!m_d2factory || !m_WICFactory)
		create_factory();

	if (!m_d2context)
		create_device_resources();

	ComPtr<IWICBitmapDecoder> pDecoder;
	ComPtr<IWICBitmapFrameDecode> pSource;
	ComPtr<IWICStream> pStream;
	ComPtr<IWICFormatConverter> pConverter;
	ComPtr<IWICBitmapScaler> pScaler;

	if (!m_WICFactory)
		return S_FALSE;// CreateDeviceIndependentResources();

	if (!m_d2context)
		return S_FALSE; //CreateDeviceContext();

	HRESULT hr = m_WICFactory->CreateDecoderFromFilename(
		path,
		NULL,
		GENERIC_READ,
		WICDecodeMetadataCacheOnLoad,
		&pDecoder
	);

	if (FAILED(hr))
		return false;

	hr = pDecoder->GetFrame(0, &pSource);

	if (FAILED(hr))
		return false;

	hr = m_WICFactory->CreateFormatConverter(&pConverter);

	if (FAILED(hr))
		return false;

	hr = pConverter->Initialize(
		pSource.Get(),
		GUID_WICPixelFormat32bppPBGRA,
		WICBitmapDitherTypeNone,
		NULL,
		0.f,
		WICBitmapPaletteTypeMedianCut
		);

	if (FAILED(hr))
		return false;

	hr = m_d2context->CreateBitmapFromWicBitmap(pConverter.Get(), NULL, &m_img);

	render();

	return true;
}

HRESULT CSCD2Context::render()
{
	HRESULT hr = S_FALSE;

	if (!m_d2context || !m_img)
		return hr;

	m_d2context->BeginDraw();
	m_d2context->SetTransform(D2D1::Matrix3x2F::Identity());

	D2D1_SIZE_F rtSize = m_d2context->GetSize();

	int width = static_cast<int>(rtSize.width);
	int height = static_cast<int>(rtSize.height);

	//{
	//	//m_d2context->DrawImage(affineTransformEffect.Get());
	//	D2D1_SIZE_F sz_img = m_img_back->GetSize();
	//	CRect r = get_ratio_rect(CRect(0, 0, width * 2, height * 2), (int)(sz_img.width), (int)(sz_img.height));
	//	TRACE(_T("rc = %dx%d, sz_img = %.0fx%.0f, r = %s\n"), width, height, sz_img.width, sz_img.height, get_rect_info_string(r));
	m_d2context->DrawBitmap(m_img.Get(), D2D1::RectF(0.0f, 0.0f, rtSize.width, rtSize.height));
	//m_d2context->DrawBitmap(m_img_back.Get(), D2D1::RectF(r.left, r.top, r.right, r.bottom));

	hr = m_d2context->EndDraw();

	if (SUCCEEDED(hr))
		hr = m_swapchain->Present(0, 0);

	if (hr == D2DERR_RECREATE_TARGET)
		hr = S_OK;

	return hr;
}

HRESULT CSCD2Context::on_resize(int cx, int cy)
{
	HRESULT hr = S_FALSE;

	if (!m_d2context)
		return hr;

	m_d2context->SetTarget(nullptr);

	if (SUCCEEDED(hr)) {
		hr = m_swapchain->ResizeBuffers(
			0,
			cx,
			cy,
			DXGI_FORMAT_B8G8R8A8_UNORM,
			0
		);
	}

	ComPtr<IDXGISurface> surface = nullptr;
	if (SUCCEEDED(hr)) {
		hr = m_swapchain->GetBuffer(
			0,
			IID_PPV_ARGS(&surface)
		);
	}

	ComPtr<ID2D1Bitmap1> bitmap = nullptr;
	if (SUCCEEDED(hr)) {
		FLOAT dpiX, dpiY;
		dpiX = (FLOAT)GetDpiForWindow(::GetDesktopWindow());
		dpiY = dpiX;
		D2D1_BITMAP_PROPERTIES1 properties = D2D1::BitmapProperties1(
			D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
			D2D1::PixelFormat(
				DXGI_FORMAT_B8G8R8A8_UNORM,
				D2D1_ALPHA_MODE_IGNORE
			),
			dpiX,
			dpiY
		);
		hr = m_d2context->CreateBitmapFromDxgiSurface(
			surface.Get(),
			&properties,
			&bitmap
		);
	}

	if (SUCCEEDED(hr)) {
		m_d2context->SetTarget(bitmap.Get());
	}

	//render();
}
*/