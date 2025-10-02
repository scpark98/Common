#include "SCD2Image.h"

CSCD2Image::CSCD2Image()
{
	//CoInitializeEx(nullptr, COINIT_MULTITHREADED);
}

CSCD2Image::~CSCD2Image()
{
	//CoUninitialize();
}
/*
HRESULT CSCD2Image::create_factory()
{
	HRESULT hr = S_OK;

	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, IID_PPV_ARGS(&m_d2factory));

	if (SUCCEEDED(hr))
	{
		hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_WICFactory));
	}

	return hr;
}

HRESULT CSCD2Image::create_device_resources()
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

HRESULT CSCD2Image::create_device_context()
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
*/

HRESULT CSCD2Image::load(IWICImagingFactory2* WICfactory, ID2D1DeviceContext* d2context, UINT resource_id, CString type)
{
	IWICStream* pStream = NULL;
	IWICBitmapDecoder* pDecoder = NULL;
	//IWICBitmapFrameDecode* pIDecoderFrame = NULL;
	ComPtr<IWICFormatConverter> pConverter;
	ComPtr<IWICBitmapScaler> pScaler;

	// Resource management.
	HRSRC imageResHandle = NULL;
	HGLOBAL imageResDataHandle = NULL;
	void* pImageFile = NULL;
	DWORD imageFileSize = 0;
	HRESULT hr = S_OK;

	// Locate the resource in the application's executable.
	imageResHandle = FindResource(GetModuleHandle(NULL), MAKEINTRESOURCE(resource_id), type);

	if (FAILED(hr))
		return hr;

	imageResDataHandle = LoadResource(GetModuleHandle(NULL), imageResHandle);
	hr = (imageResDataHandle ? S_OK : E_FAIL);

	if (FAILED(hr))
		return hr;

	pImageFile = LockResource(imageResDataHandle);
	hr = (pImageFile ? S_OK : E_FAIL);

	if (FAILED(hr))
		return hr;

	imageFileSize = SizeofResource(GetModuleHandle(NULL), imageResHandle);
	hr = (imageFileSize ? S_OK : E_FAIL);

	// Create a WIC stream to map onto the memory.
	if (FAILED(hr))
		return hr;

	hr = WICfactory->CreateStream(&pStream);

	// Initialize the stream with the memory pointer and size.
	if (FAILED(hr))
		return hr;

	hr = pStream->InitializeFromMemory(reinterpret_cast<BYTE*>(pImageFile), imageFileSize);

	// Create a decoder for the stream.
	if (FAILED(hr))
		return hr;

	hr = WICfactory->CreateDecoderFromStream(
		pStream,                   // The stream to use to create the decoder
		NULL,                          // Do not prefer a particular vendor
		WICDecodeMetadataCacheOnLoad,  // Cache metadata when needed
		&pDecoder);                   // Pointer to the decoder

	// Retrieve the first bitmap frame.
	if (FAILED(hr))
		return hr;

	return load(WICfactory, d2context, pDecoder);
	//hr = pDecoder->GetFrame(0, &pSource);
	//return load(WICfactory, d2context, pSource.Get());
}

HRESULT CSCD2Image::load(IWICImagingFactory2* WICfactory, ID2D1DeviceContext* d2context, CString path)
{
	ComPtr<IWICBitmapDecoder> pDecoder;
	ComPtr<IWICStream> pStream;

	if (!WICfactory)
		return S_FALSE;// CreateDeviceIndependentResources();

	if (!d2context)
		return S_FALSE; //CreateDeviceContext();

	HRESULT hr = WICfactory->CreateDecoderFromFilename(
		path,
		NULL,
		GENERIC_READ,
		WICDecodeMetadataCacheOnLoad,
		&pDecoder
	);

	if (FAILED(hr))
		return hr;

	return load(WICfactory, d2context, pDecoder.Get());

	/*
	hr = pDecoder->GetFrame(0, &pSource);
	if (FAILED(hr))
		return hr;

	return load(WICfactory, d2context, pSource.Get());
	*/
}

HRESULT CSCD2Image::load(IWICImagingFactory2* WICfactory, ID2D1DeviceContext* d2context, IWICBitmapDecoder* pDecoder)
{

	m_frame_index = 0;
	m_img.clear();

	UINT frame_count = 1;
	HRESULT	hr = pDecoder->GetFrameCount(&frame_count);

	if (FAILED(hr))
		return hr;


	if (FAILED(hr))
		return hr;

	for (int i = 0; i < frame_count; i++)
	{
		ComPtr<IWICFormatConverter> pConverter;
		ComPtr<IWICBitmapFrameDecode> pSource;
		//ComPtr<IWICBitmapScaler> pScaler;

		TRACE(_T("%d frame\n"), i);

		hr = WICfactory->CreateFormatConverter(&pConverter);
		hr = pDecoder->GetFrame(i, &pSource);

		hr = pConverter->Initialize(
			pSource.Get(),
			GUID_WICPixelFormat32bppPBGRA,
			WICBitmapDitherTypeNone,
			NULL,
			0.f,
			WICBitmapPaletteTypeMedianCut
		);

		if (FAILED(hr))
			return hr;

		ComPtr<ID2D1Bitmap> img;
		hr = d2context->CreateBitmapFromWicBitmap(pConverter.Get(), NULL, &img);
		m_img.push_back(std::move(img));
	}

	D2D1_SIZE_F sz = m_img[m_frame_index]->GetSize();
	m_width = sz.width;
	m_height = sz.height;

	return hr;
}
/*
HRESULT CSCD2Image::render()
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
*/
HRESULT CSCD2Image::on_resize(ID2D1DeviceContext* d2context, IDXGISwapChain* swapchain, int cx, int cy)
{
	HRESULT hr = S_FALSE;

	if (!d2context)
		return hr;

	d2context->SetTarget(nullptr);

	if (SUCCEEDED(hr)) {
		hr = swapchain->ResizeBuffers(
			0,
			cx,
			cy,
			DXGI_FORMAT_B8G8R8A8_UNORM,
			0
		);
	}

	ComPtr<IDXGISurface> surface = nullptr;
	if (SUCCEEDED(hr)) {
		hr = swapchain->GetBuffer(
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
		hr = d2context->CreateBitmapFromDxgiSurface(
			surface.Get(),
			&properties,
			&bitmap
		);
	}

	if (SUCCEEDED(hr)) {
		d2context->SetTarget(bitmap.Get());
	}
}

D2D1_RECT_F CSCD2Image::draw(ID2D1DeviceContext* d2dc, eSCD2Image_DRAW_MODE draw_mode)
{
	D2D1_SIZE_F sz = d2dc->GetSize();
	return draw(d2dc, D2D1::RectF(0, 0, sz.width, sz.height), draw_mode);
}

//dx, dy 좌표에 그려준다.
D2D1_RECT_F CSCD2Image::draw(ID2D1DeviceContext* d2dc, int dx, int dy)
{
	return draw(d2dc, D2D1::RectF(dx, dy, dx + m_width, dy + m_height));
}

D2D1_RECT_F CSCD2Image::draw(ID2D1DeviceContext* d2dc, D2D1_POINT_2F pt)
{
	return draw(d2dc, D2D1::RectF(pt.x, pt.y, pt.x + m_width, pt.y + m_height));
}

D2D1_RECT_F CSCD2Image::draw(ID2D1DeviceContext* d2dc, D2D1_RECT_F target, eSCD2Image_DRAW_MODE draw_mode)
{
	D2D1_RECT_F r;

	if (draw_mode == eSCD2Image_DRAW_MODE::draw_mode_stretch)
		r = target;
	else if (draw_mode == eSCD2Image_DRAW_MODE::draw_mode_zoom)
		r = get_ratio_rect(target, m_width / m_height);
	else if (draw_mode == eSCD2Image_DRAW_MODE::draw_mode_original)
	{
		r = target;
		r.right = r.left + m_width;
		r.bottom = r.top + m_height;
	}
	else //eSCD2Image_DRAW_MODE::draw_mode_original_center
	{
		r = D2D1::RectF(0, 0, m_width, m_height);
		r.left = target.left + (target.right - target.left - m_width) / 2.0;
		r.top = target.top + (target.bottom - target.top - m_height) / 2.0;
	}

	d2dc->DrawBitmap(m_img[m_frame_index].Get(), r);
	return r;
}

D2D1_RECT_F CSCD2Image::get_ratio_rect(D2D1_RECT_F target, float width, float height, int attach, bool stretch)
{
	if (height == 0.0f)
		return D2D1_RECT_F();

	return get_ratio_rect(target, width / height, attach, stretch);
}

D2D1_RECT_F CSCD2Image::get_ratio_rect(D2D1_RECT_F target, float ratio, int attach, bool stretch)
{
	int		w = target.right - target.left;
	int		h = target.bottom - target.top;
	int		nNewW;
	int		nNewH;
	double	dTargetRatio = double(w) / double(h);

	D2D1_RECT_F	result;

	if (w == 0 || h == 0)
		return D2D1_RECT_F();

	bool bResizeWidth;

	if (ratio > 1.0)
	{
		if (dTargetRatio < ratio)
			bResizeWidth = false;
		else
			bResizeWidth = true;
	}
	else
	{
		if (dTargetRatio > ratio)
			bResizeWidth = true;
		else
			bResizeWidth = false;
	}


	if (bResizeWidth)
	{
		result.top = target.top;
		result.bottom = target.bottom;

		nNewW = (double)(h) * ratio;
		if (attach & attach_left)
			result.left = target.left;
		else if (attach & attach_right)
			result.left = target.right - nNewW;
		else
			result.left = target.left + (w - nNewW) / 2.0;

		result.right = result.left + nNewW;
	}
	else
	{
		result.left = target.left;
		result.right = target.right;

		nNewH = (double)(w) / ratio;

		if (attach & attach_top)
			result.top = target.top;
		else if (attach & attach_bottom)
			result.top = target.bottom - nNewH;
		else
			result.top = target.top + (h - nNewH) / 2.0;

		result.bottom = result.top + nNewH;
	}

	return result;
}

//n개의 이미지로 구성된 gif와 같은 이미지일 경우 프레임 이동
int CSCD2Image::goto_frame(int index)
{
	if (index >= m_img.size())
		index = 0;

	m_frame_index = index;

	return m_frame_index;
}
