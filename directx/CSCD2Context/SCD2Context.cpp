#include "SCD2Context.h"

#pragma comment(lib, "d2d1.lib")

CSCD2Context::CSCD2Context()
{
}

CSCD2Context::~CSCD2Context()
{
}

HRESULT CSCD2Context::init(HWND hWnd, int cx, int cy)
{
	m_sz.width = cx;
	m_sz.height = cy;
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

	if (!m_d2context)
	{
		hr = create_device_context();

		ComPtr<IDXGISurface> surface = nullptr;
		if (SUCCEEDED(hr))
		{
			hr = m_swapchain->GetBuffer(
				0,
				IID_PPV_ARGS(surface.GetAddressOf())
			);
		}

		//ComPtr<ID2D1Bitmap1> bitmap = nullptr;
		m_target_bitmap = nullptr;

		if (SUCCEEDED(hr))
		{
			FLOAT dpiX, dpiY;
			dpiX = (FLOAT)GetDpiForWindow(::GetDesktopWindow());
			dpiY = dpiX;

			D2D1_BITMAP_PROPERTIES1 properties = D2D1::BitmapProperties1(
				D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
				D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), dpiX, dpiY);

			hr = m_d2context->CreateBitmap(
				D2D1::SizeU(m_sz.width, m_sz.height),
				nullptr,
				0,
				&properties,
				&m_target_bitmap
			);
			/*
			hr = m_d2context->CreateBitmapFromDxgiSurface(
				surface.Get(),
				&properties,
				&m_target_bitmap
			);
			*/
		}


		if (SUCCEEDED(hr))
		{
			m_d2context->SetTarget(m_target_bitmap.Get());
			//m_spGdiInteropRenderTarget = bitmap.Get();
			m_d2context->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);

			//D2D1_ANTIALIAS_MODE_ALIASED로 하지 않고 수평 라인을 그리면 stroke width가 1.0, 2.0의 두께가 동일하게 표시된다.
			//1.0이 2.0보다는 약간 옅은색으로 표시되나 thickness가 동일하게 표시된다.
			//이 세팅을 해줘야 실제 1 pixel 두께로 그려진다.
			//단, 이 모드로 세팅하면 대각선이 antialias 처리되지 않고 그대로 계단 현상이 표시된다.
			//m_d2context->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
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
	size.width = m_sz.width;// rc.right;
	size.height = m_sz.height;// rc.bottom;

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

HRESULT CSCD2Context::on_size_changed(int cx, int cy)
{
	HRESULT hr = S_OK;

	if (!m_d2context)
		return hr;

	m_d2context->SetTarget(nullptr);

	if (SUCCEEDED(hr))
	{
		hr = m_swapchain->ResizeBuffers(
			0,
			m_sz.width,// cx,
			m_sz.height,//cy,
			DXGI_FORMAT_B8G8R8A8_UNORM,
			0
		);
	}

	ComPtr<IDXGISurface> surface = nullptr;
	if (SUCCEEDED(hr))
	{
		hr = m_swapchain->GetBuffer(
			0,
			IID_PPV_ARGS(&surface)
		);
	}

	//ComPtr<ID2D1Bitmap1> bitmap = nullptr;
	m_target_bitmap = nullptr;
	if (SUCCEEDED(hr))
	{
		FLOAT dpiX, dpiY;
		dpiX = (FLOAT)GetDpiForWindow(::GetDesktopWindow());
		dpiY = dpiX;
		D2D1_BITMAP_PROPERTIES1 properties = D2D1::BitmapProperties1(
			D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
			D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED
			),
			dpiX,
			dpiY
		);

		hr = m_d2context->CreateBitmap(
			D2D1::SizeU(m_sz.width, m_sz.height),
			nullptr,
			0,
			&properties,
			&m_target_bitmap
		);
		/*
		hr = m_d2context->CreateBitmapFromDxgiSurface(
			surface.Get(),
			&properties,
			&m_target_bitmap
		);
		*/
	}

	if (SUCCEEDED(hr))
	{
		m_d2context->SetTarget(m_target_bitmap.Get());
	}

	//m_d2context->InvalidateEffectInputRectangle()
	//render();
}

HRESULT	CSCD2Context::save(CString path)
{
	HRESULT res = S_OK;

	ComPtr<ID2D1Bitmap1> cpuBitmap;

	D2D1_BITMAP_PROPERTIES1 props = {};
	props.pixelFormat = D2D1::PixelFormat(
		DXGI_FORMAT_B8G8R8A8_UNORM,
		D2D1_ALPHA_MODE_PREMULTIPLIED
	);
	props.bitmapOptions =
		D2D1_BITMAP_OPTIONS_TARGET |
		D2D1_BITMAP_OPTIONS_CANNOT_DRAW;

	D2D1_BITMAP_PROPERTIES1 cpuProps = props;
	cpuProps.bitmapOptions = D2D1_BITMAP_OPTIONS_CPU_READ | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;

	D2D1_SIZE_U sz1 = m_target_bitmap.Get()->GetPixelSize();
	D2D1_SIZE_F sz2 = m_target_bitmap.Get()->GetSize();

	D2D1_SIZE_U sz = { m_d2context->GetSize().width, m_d2context->GetSize().height };
	
	res = m_d2context->CreateBitmap(
		sz,
		nullptr,
		0,
		&cpuProps,
		&cpuBitmap
	);

	cpuBitmap->CopyFromBitmap(nullptr, m_target_bitmap.Get(), nullptr);

	D2D1_MAPPED_RECT mapped = {};
	cpuBitmap->Map(D2D1_MAP_OPTIONS_READ, &mapped);

	ComPtr<IWICImagingFactory> wicFactory;
	CoCreateInstance(
		CLSID_WICImagingFactory,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&wicFactory)
	);

	ComPtr<IWICBitmapEncoder> encoder;
	wicFactory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder);

	ComPtr<IWICStream> stream;
	wicFactory->CreateStream(&stream);

	stream->InitializeFromFilename(path, GENERIC_WRITE);
	encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache);

	ComPtr<IWICBitmapFrameEncode> frame;
	encoder->CreateNewFrame(&frame, nullptr);

	frame->Initialize(nullptr);
	frame->SetSize(sz.width, sz.height);

	WICPixelFormatGUID format = GUID_WICPixelFormat32bppBGRA;
	frame->SetPixelFormat(&format);

	frame->WritePixels(
		sz.height,
		mapped.pitch,
		mapped.pitch * sz.height,
		mapped.bits
	);

	frame->Commit();
	encoder->Commit();

	cpuBitmap->Unmap();

	return res;
}