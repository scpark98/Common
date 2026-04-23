#include "SCD2Context.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "d3d11.lib")

CSCD2Context::CSCD2Context()
{
}

CSCD2Context::~CSCD2Context()
{
}

HRESULT CSCD2Context::init(HWND hWnd, int cx, int cy)
{
	m_hWnd = hWnd;

	CRect rc;
	::GetClientRect(m_hWnd, &rc);

	if (cx <= 0)
		cx = rc.Width();
	if (cy <= 0)
		cy = rc.Height();

	m_sz.width = cx;
	m_sz.height = cy;
	create_factory();
	create_device_resources();
	return S_OK;
}

HRESULT CSCD2Context::init(HWND hWnd, CSCD2Context* pShared, int cx, int cy)
{
	if (!pShared || !pShared->m_d2device)
		return E_INVALIDARG;

	m_hWnd = hWnd;

	CRect rc;
	::GetClientRect(m_hWnd, &rc);
	if (cx <= 0) cx = rc.Width();
	if (cy <= 0) cy = rc.Height();
	m_sz.width = (float)cx;
	m_sz.height = (float)cy;

	// 팩토리/디바이스 공유 (ComPtr 참조 카운트 증가)
	m_d2factory = pShared->m_d2factory;
	m_WICFactory = pShared->m_WICFactory;
	m_d2device = pShared->m_d2device;
	m_dxgiDevice = pShared->m_dxgiDevice;

	HRESULT hr = S_OK;

	if (!m_d2context)
	{
		// 공유 디바이스로부터 새 디바이스 컨텍스트 생성
		hr = m_d2device->CreateDeviceContext(
			D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
			m_d2context.GetAddressOf());
		if (FAILED(hr)) return hr;

		// 공유된 DXGI 디바이스로 스왑체인 생성 (새 D3D11 디바이스 생성 X)
		ComPtr<IDXGIAdapter> dxgiAdapter;
		ComPtr<IDXGIFactory> dxgiFactory;

		hr = m_dxgiDevice->GetAdapter(&dxgiAdapter);
		if (FAILED(hr)) return hr;

		hr = dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory));
		if (FAILED(hr)) return hr;

		DXGI_SWAP_CHAIN_DESC sd = {};
		sd.BufferDesc.Width = cx;
		sd.BufferDesc.Height = cy;
		sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		sd.BufferDesc.RefreshRate = { 60, 1 };
		sd.SampleDesc = { 1, 0 };
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.BufferCount = 1;
		sd.OutputWindow = m_hWnd;
		sd.Windowed = TRUE;

		hr = dxgiFactory->CreateSwapChain(m_dxgiDevice.Get(), &sd, m_swapchain.GetAddressOf());
		if (FAILED(hr)) return hr;

		// 렌더 타겟 설정
		ComPtr<IDXGISurface> surface;
		hr = m_swapchain->GetBuffer(0, IID_PPV_ARGS(&surface));
		if (FAILED(hr)) return hr;

		ComPtr<ID2D1Bitmap1> bitmap;
		FLOAT dpi = (FLOAT)GetDpiForWindow(::GetDesktopWindow());
		D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
			D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
			D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
			dpi, dpi);
		hr = m_d2context->CreateBitmapFromDxgiSurface(surface.Get(), &props, &bitmap);
		if (FAILED(hr)) return hr;

		m_d2context->SetTarget(bitmap.Get());
		m_d2context->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);

		m_br_zigzag = create_zigzag_brush();
	}

	return hr;
}

HRESULT CSCD2Context::create_factory()
{
	HRESULT hr = S_OK;

	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, IID_PPV_ARGS(m_d2factory.GetAddressOf()));

	if (SUCCEEDED(hr))
	{
		//CoInitialize()를 호출하지 않은 경우 아래 m_WICFactory를 얻어오지 못한다.
		//그렇다고 CSCD2Context 클래스에서 매번 호출하는 것은 부적절하다.
		//가장 적절한 위치는 해당 스레드의 시작 지점이고 MFC UI 스레드 기준으로는 CWinApp::InitInstance()에서 수행시켜야 한다.
		//또한 아래 주석과 같이 옵션에 따라 부작용이 발생할 수 있으므로 정확한 옵션으로 실행해야 한다.
		//CWinApp::InitInstance()에서 CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);를 호출할 것.

		//CoInitializeEx()를 호출하지 않거나 COINIT_MULTITHREADED로 실행할 경우 ::SHBrowseForFolder() api가 응답하지 않음.
		//기존 프로젝트들에서는 이를 호출하지 않아도 폴더선택창이 제대로 열렸으나
		//Direct2D를 이용하는 이 프로젝트에서는 문제가 발생했다.
		//CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

		hr = CoCreateInstance(CLSID_WICImagingFactory2, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(m_WICFactory.GetAddressOf()));
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

		ComPtr<ID2D1Bitmap1> bitmap = nullptr;
		//m_target_bitmap = nullptr;

		if (SUCCEEDED(hr))
		{
			FLOAT dpiX, dpiY;
			dpiX = (FLOAT)GetDpiForWindow(::GetDesktopWindow());
			dpiY = dpiX;

			D2D1_BITMAP_PROPERTIES1 properties = D2D1::BitmapProperties1(
				D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
				D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), dpiX, dpiY);

			/*
			hr = m_d2context->CreateBitmap(
				D2D1::SizeU(m_sz.width, m_sz.height),
				nullptr,
				0,
				&properties,
				&bitmap
				//&m_target_bitmap
			);
			*/
			hr = m_d2context->CreateBitmapFromDxgiSurface(
				surface.Get(),
				&properties,
				&bitmap
			);
		}


		if (SUCCEEDED(hr))
		{
			m_d2context->SetTarget(bitmap.Get());
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
	//size.width = m_sz.width;
	//size.height = m_sz.height;
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

	// DXGI 디바이스 저장 (공유 init에서 사용)
	if (SUCCEEDED(hr))
		hr = d3dDevice->QueryInterface(IID_PPV_ARGS(m_dxgiDevice.GetAddressOf()));

	if (SUCCEEDED(hr))
		hr = m_d2factory->CreateDevice(m_dxgiDevice.Get(), m_d2device.GetAddressOf());

	if (SUCCEEDED(hr))
		hr = m_d2device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, m_d2context.GetAddressOf());

	m_br_zigzag = create_zigzag_brush();

	return hr;
}

ComPtr<ID2D1BitmapBrush> CSCD2Context::create_zigzag_brush(Gdiplus::Color cr_fore, Gdiplus::Color cr_back)
{
	if (m_zigzag_size <= 0.0f)
		m_zigzag_size = 8.0f;

	const UINT w = 2, h = 2, stride = w * 4;
	BYTE pixels[stride * h];
	auto px = [&](UINT x, UINT y, BYTE B, BYTE G, BYTE R, BYTE A) {
		BYTE* p = pixels + y * stride + x * 4; p[0] = B; p[1] = G; p[2] = R; p[3] = A;
		};
	// W B / B W
	px(0, 0, cr_fore.GetB(), cr_fore.GetG(), cr_fore.GetR(), 255); px(1, 0, cr_back.GetB(), cr_back.GetG(), cr_back.GetR(), 255);
	px(0, 1, cr_back.GetB(), cr_back.GetG(), cr_back.GetR(), 255); px(1, 1, cr_fore.GetB(), cr_fore.GetG(), cr_fore.GetR(), 255);

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
	if (FAILED(m_d2context->CreateBitmapBrush(bmp.Get(), bbp, &brush)))
		return nullptr;

	// Each source pixel becomes one checker cell of size 'cell'
	brush->SetTransform(D2D1::Matrix3x2F::Scale(m_zigzag_size, m_zigzag_size));
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
			cx,
			cy,
			//m_sz.width,
			//m_sz.height,
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

	ComPtr<ID2D1Bitmap1> bitmap = nullptr;
	//m_target_bitmap = nullptr;
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

		/*
		hr = m_d2context->CreateBitmap(
			D2D1::SizeU(m_sz.width, m_sz.height),
			nullptr,
			0,
			&properties,
			&m_target_bitmap
		);
		*/
		hr = m_d2context->CreateBitmapFromDxgiSurface(
			surface.Get(),
			&properties,
			&bitmap
		);
	}

	if (SUCCEEDED(hr))
	{
		m_d2context->SetTarget(bitmap.Get());
		//m_d2context->SetTarget(m_target_bitmap.Get());
	}

	return hr;
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

	//D2D1_SIZE_U sz1 = m_target_bitmap.Get()->GetPixelSize();
	//D2D1_SIZE_F sz2 = m_target_bitmap.Get()->GetSize();

	D2D1_SIZE_U sz = { m_d2context->GetSize().width, m_d2context->GetSize().height };
	
	res = m_d2context->CreateBitmap(
		sz,
		nullptr,
		0,
		&cpuProps,
		&cpuBitmap
	);

	//cpuBitmap->CopyFromBitmap(nullptr, m_target_bitmap.Get(), nullptr);

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

void CSCD2Context::draw_text(IDWriteFactory* dwriteFactory,
	IDWriteTextFormat* writeFormat,
	const CString& text,
	const D2D1_RECT_F& rect,
	ID2D1Brush* brush,
	float min_fontsize,
	float max_fontsize)
{
	if (text.IsEmpty() || !dwriteFactory || !writeFormat || !brush)
		return;

	float rectWidth = rect.right - rect.left;
	float rectHeight = rect.bottom - rect.top;
	if (rectWidth < 1.0f || rectHeight < 1.0f)
		return;

	// 1. 무한 영역으로 원본 크기 측정
	Microsoft::WRL::ComPtr<IDWriteTextLayout> measureLayout;
	dwriteFactory->CreateTextLayout(
		text, text.GetLength(), writeFormat,
		10000.0f, 10000.0f,
		&measureLayout);

	if (!measureLayout)
		return;

	DWRITE_TEXT_METRICS metrics;
	measureLayout->GetMetrics(&metrics);

	if (metrics.width < 0.001f || metrics.height < 0.001f)
		return;

	// 2. 축소 비율 계산
	float scale = min(rectWidth / metrics.width, rectHeight / metrics.height);
	float newFontSize = writeFormat->GetFontSize() * scale;

	//폰트 크기 제한 적용
	if (min_fontsize > 0.0f && newFontSize < min_fontsize)
		newFontSize = min_fontsize;
	if (max_fontsize > 0.0f && newFontSize > max_fontsize)
		newFontSize = max_fontsize;

	// 3. ★ 셀 크기에 맞는 새 레이아웃을 재생성
	Microsoft::WRL::ComPtr<IDWriteTextLayout> drawLayout;
	dwriteFactory->CreateTextLayout(
		text, text.GetLength(), writeFormat,
		rectWidth, rectHeight,		// ← 10000이 아닌 실제 셀 크기
		&drawLayout);

	if (!drawLayout)
		return;

	DWRITE_TEXT_RANGE allRange = { 0, (UINT32)text.GetLength() };
	drawLayout->SetFontSize(newFontSize, allRange);

	// 4. 셀 영역의 좌상단 기준으로 출력
	//    (writeFormat의 center/center 정렬이 적용됨)
	m_d2context->DrawTextLayout(
		D2D1::Point2F(rect.left, rect.top),
		drawLayout.Get(),
		brush
	);
}