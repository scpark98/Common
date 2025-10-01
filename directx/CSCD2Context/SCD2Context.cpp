#include "SCD2Context.h"

CSCD2Context::CSCD2Context()
{
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);
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

	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, IID_PPV_ARGS(&m_d2factory));

	if (SUCCEEDED(hr))
	{
		hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_WICFactory));
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
				D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), dpiX, dpiY);

			hr = m_d2context->CreateBitmapFromDxgiSurface(
				surface.Get(),
				&properties,
				&bitmap
			);
		}
		if (SUCCEEDED(hr)) {
			m_d2context->SetTarget(bitmap.Get());
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
			&m_swapchain,
			&d3dDevice,
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
		hr = d3dDevice->QueryInterface(IID_PPV_ARGS(&dxgiDevice));

	if (SUCCEEDED(hr))
		hr = m_d2factory->CreateDevice(dxgiDevice.Get(), &m_d2device);

	if (SUCCEEDED(hr))
		hr = m_d2device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_d2context);

	return hr;
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