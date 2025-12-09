#include "SCD2Context.h"

#pragma comment(lib, "d2d1.lib")

CSCD2Context::CSCD2Context()
{
}

CSCD2Context::~CSCD2Context()
{
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
			dpiY = dpiX;

			D2D1_BITMAP_PROPERTIES1 properties = D2D1::BitmapProperties1(
				D2D1_BITMAP_OPTIONS_NONE | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
				D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), dpiX, dpiY);

			hr = m_d2context->CreateBitmapFromDxgiSurface(
				surface.Get(),
				&properties,
				&bitmap
			);
		}

		// create GDI-compatible window render target
		//D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties();
		//rtProps.usage = D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE;
		//rtProps.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
		//rtProps.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;

		//ID2D1HwndRenderTarget* pHwndRenderTarget = NULL;
		//HRESULT hr = m_d2factory->CreateHwndRenderTarget(
		//	&rtProps,
		//	&D2D1::HwndRenderTargetProperties(m_hWnd, CD2DSizeU()),
		//	&pHwndRenderTarget);
		//if (FAILED(hr))
		//	return -1;

		// Create a DC render target.
		/*
		D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
			D2D1_RENDER_TARGET_TYPE_DEFAULT,
			D2D1::PixelFormat(
				DXGI_FORMAT_B8G8R8A8_UNORM,
				D2D1_ALPHA_MODE_IGNORE),
			0,
			0,
			D2D1_RENDER_TARGET_USAGE_NONE,
			D2D1_FEATURE_LEVEL_DEFAULT
		);
		hr = m_d2factory->CreateDCRenderTarget(&props, m_pDCRT.GetAddressOf());
		*/
		// instantiate new CHwndRenderTarget and attach the GDI-compatible render target
		//CHwndRenderTarget* pRenderTarget = new CHwndRenderTarget;
		//GetRenderTarget()->Attach(pHwndRenderTarget);

		//// create ID2D1GdiInteropRenderTarget instance
		//m_spGdiInteropRenderTarget = pHwndRenderTarget;





		if (SUCCEEDED(hr)) {
			m_d2context->SetTarget(bitmap.Get());
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

/*
void CSCD2Context::get_d2DC(HDC* hDC)
{
	m_spGdiInteropRenderTarget->GetDC(D2D1_DC_INITIALIZE_MODE_COPY, hDC);
}
*/

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

HRESULT CSCD2Context::on_size_changed(int cx, int cy)
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
/*
ID2D1PathGeometry* CSCD2Context::create_round_rect(int x, int y, int width, int height, int leftTop, int rightTop, int rightBottom, int leftBottom)
{
	ID2D1GeometrySink* sink = nullptr;
	ID2D1PathGeometry* path = nullptr;

	m_d2factory->CreatePathGeometry(&path);
	path->Open(&sink);

	D2D1_POINT_2F p[2];

	p[0].x = x + leftTop;
	p[0].y = y;
	sink->BeginFigure(p[0], D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_FILLED);
	p[1].x = x + width - rightTop;
	p[1].y = y;
	sink->AddLines(p, 2);

	p[0].x = x + width;
	p[0].y = y + rightTop;

	if (rightTop)
	{
		D2D1_POINT_2F p2 = D2D1::Matrix3x2F::Rotation(0, p[1]).TransformPoint(p[0]);
		sink->AddArc(D2D1::ArcSegment(p2, D2D1::SizeF(rightTop, rightTop), 0, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
	}

	p[1].x = x + width;
	p[1].y = y + height - rightBottom;
	sink->AddLines(p, 2);

	p[0].x = x + width - rightBottom;
	p[0].y = y + height;

	if (rightBottom)
	{
		D2D1_POINT_2F p2 = D2D1::Matrix3x2F::Rotation(0, p[1]).TransformPoint(p[0]);
		sink->AddArc(D2D1::ArcSegment(p2, D2D1::SizeF(rightBottom, rightBottom), 0, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
	}

	p[1].x = x + leftBottom;
	p[1].y = y + height;
	sink->AddLines(p, 2);

	p[0].x = x;
	p[0].y = y + height - leftBottom;
	if (leftBottom)
	{
		D2D1_POINT_2F p2 = D2D1::Matrix3x2F::Rotation(0, p[1]).TransformPoint(p[0]);
		sink->AddArc(D2D1::ArcSegment(p2, D2D1::SizeF(leftBottom, leftBottom), 0, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
	}


	p[1].x = x;
	p[1].y = y + leftTop;
	sink->AddLines(p, 2);
	p[0].x = x + leftTop;
	p[0].y = y;
	if (leftTop)
	{
		D2D1_POINT_2F p2 = D2D1::Matrix3x2F::Rotation(0, p[1]).TransformPoint(p[0]);
		sink->AddArc(D2D1::ArcSegment(p2, D2D1::SizeF(leftTop, leftTop), 0, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
	}

	sink->EndFigure(D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED);
	sink->Close();
	//SafeRelease(&sink);
	sink->Release();

	return path;
}
*/