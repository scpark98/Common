#include "SCD2Image.h"

#include <thread>

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

HRESULT CSCD2Image::load(IWICImagingFactory2* pWICFactory, ID2D1DeviceContext* d2context, UINT resource_id, CString type)
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

	hr = pWICFactory->CreateStream(&pStream);

	// Initialize the stream with the memory pointer and size.
	if (FAILED(hr))
		return hr;

	hr = pStream->InitializeFromMemory(reinterpret_cast<BYTE*>(pImageFile), imageFileSize);

	// Create a decoder for the stream.
	if (FAILED(hr))
		return hr;

	hr = pWICFactory->CreateDecoderFromStream(
		pStream,                   // The stream to use to create the decoder
		NULL,                          // Do not prefer a particular vendor
		WICDecodeMetadataCacheOnLoad,  // Cache metadata when needed
		&pDecoder);                   // Pointer to the decoder

	// Retrieve the first bitmap frame.
	if (FAILED(hr))
		return hr;

	return load(pWICFactory, d2context, pDecoder);
	//hr = pDecoder->GetFrame(0, &pSource);
	//return load(pWICFactory, d2context, pSource.Get());
}

HRESULT CSCD2Image::load(IWICImagingFactory2* pWICFactory, ID2D1DeviceContext* d2context, CString path)
{
	ComPtr<IWICBitmapDecoder> pDecoder;
	ComPtr<IWICStream> pStream;

	if (!pWICFactory)
		return S_FALSE;// CreateDeviceIndependentResources();

	if (!d2context)
		return S_FALSE; //CreateDeviceContext();

	HRESULT hr = pWICFactory->CreateDecoderFromFilename(
		path,
		NULL,
		GENERIC_READ,
		WICDecodeMetadataCacheOnLoad,
		&pDecoder
	);

	if (FAILED(hr))
		return hr;

	return load(pWICFactory, d2context, pDecoder.Get());

	/*
	hr = pDecoder->GetFrame(0, &pSource);
	if (FAILED(hr))
		return hr;

	return load(pWICFactory, d2context, pSource.Get());
	*/
}

//일부 animated gif의 경우 이전 프레임 이미지에서 변경된 이미지만 저장하고 있는 경우도 있으므로
//아래 코드를 참조하여 보완 필요.
//https://github.com/microsoft/DirectXTex/blob/main/Texassemble/AnimatedGif.cpp
HRESULT CSCD2Image::load(IWICImagingFactory2* pWICFactory, ID2D1DeviceContext* d2context, IWICBitmapDecoder* pDecoder)
{
	m_pWICFactory = pWICFactory;
	m_d2dc = d2context;

	m_frame_index = 0;
	m_img.clear();
	m_frame_delay.clear();

	UINT frame_count = 1;
	HRESULT	hr = pDecoder->GetFrameCount(&frame_count);

	TRACE(_T("total %d frames.\n"), frame_count);

	PROPVARIANT propValue;
	//이 초기화를 해주지 않으면 아래 frame delay를 구하는 코드에서 올바른 값이 들어가지 않는다.
	PropVariantClear(&propValue);

	if (FAILED(hr))
		return hr;

	ComPtr<IWICMetadataQueryReader> metareader;
	hr = pDecoder->GetMetadataQueryReader(&metareader);

	// Get palette
	WICColor rgbColors[256] = {};
	UINT actualColors = 0;
	if (frame_count > 1)
	{
		ComPtr<IWICPalette> palette;
		hr = pWICFactory->CreatePalette(&palette);
		if (FAILED(hr))
			return hr;

		//webp의 경우는 팔레트를 사용할 수 없다.
		hr = pDecoder->CopyPalette(palette.Get());
		if (SUCCEEDED(hr))
		{
			hr = palette->GetColors(static_cast<UINT>(std::size(rgbColors)), rgbColors, &actualColors);
		}
	}

	// Get background color
	UINT bgColor = 0;
	bool usebgcolor = false;
	if (usebgcolor && metareader)
	{
		// Most browsers just ignore the background color metadata and always use transparency
		hr = metareader->GetMetadataByName(L"/logscrdesc/GlobalColorTableFlag", &propValue);
		if (SUCCEEDED(hr))
		{
			const bool hasTable = (propValue.vt == VT_BOOL && propValue.boolVal);
			PropVariantClear(&propValue);

			if (hasTable)
			{
				hr = metareader->GetMetadataByName(L"/logscrdesc/BackgroundColorIndex", &propValue);
				if (SUCCEEDED(hr))
				{
					if (propValue.vt == VT_UI1)
					{
						const uint8_t index = propValue.bVal;

						if (index < actualColors)
						{
							bgColor = rgbColors[index];
						}
					}
					PropVariantClear(&propValue);
				}
			}
		}
	}


	UINT previousFrame = 0;
	UINT disposal = DM_UNDEFINED;
	D2D1_SIZE_U img_size = { 0, 0 };

	for (int i = 0; i < frame_count; i++)
	{
		int transparentIndex = -1;

		ComPtr<IWICFormatConverter> pConverter;
		ComPtr<IWICBitmapFrameDecode> pFrameDecode;
		ComPtr<IWICMetadataQueryReader> pMetadataReader;

		hr = pWICFactory->CreateFormatConverter(&pConverter);
		hr = pDecoder->GetFrame(i, &pFrameDecode);
		RECT rt = { 0, 0, 0, 0 };

		pFrameDecode->GetMetadataQueryReader(&pMetadataReader);

		if (pMetadataReader)
		{
			hr = pMetadataReader->GetMetadataByName(L"/grctlext/Delay", &propValue);

			if (SUCCEEDED(hr))
			{
				if (propValue.vt == VT_UI2)
				{
					// Frame delay in 1/100 second units.
					UINT delay = propValue.uiVal * 10;
					//if (delay < 20) delay = 100; // GIF frame delays less than 20ms are not honored.
					m_frame_delay.push_back(delay);
					TRACE(_T("%2dth frame. delay = %d ms\n"), i, delay);
				}
				PropVariantClear(&propValue);
			}
			else
			{
				m_frame_delay.push_back(100); // default 100ms
			}

			//calling.gif와 같이 1번 프레임부터 변경된 이미지 정보만 저장된 gif의 경우는 disposal 값은 DM_NONE로 추출된다.
			//이럴 경우 이전 프레임을 복사해서 사용해야 한다.
			//그 외 각 프레임이 온전한 경우는 DM_BACKGROUND로 추출된다.
			disposal = DM_UNDEFINED;
			hr = pMetadataReader->GetMetadataByName(L"/grctlext/Disposal", &propValue);
			if (SUCCEEDED(hr))
			{
				hr = (propValue.vt == VT_UI1 ? S_OK : E_FAIL);
				if (SUCCEEDED(hr))
				{
					disposal = propValue.bVal;
					trace(disposal);
				}
				PropVariantClear(&propValue);
			}

			hr = pMetadataReader->GetMetadataByName(L"/grctlext/TransparencyFlag", &propValue);
			if (SUCCEEDED(hr))
			{
				hr = (propValue.vt == VT_BOOL ? S_OK : E_FAIL);
				if (SUCCEEDED(hr) && propValue.boolVal)
				{
					TRACE(_T("is transparent frame = %d\n"), propValue.boolVal);
					PropVariantClear(&propValue);
					hr = pMetadataReader->GetMetadataByName(L"/grctlext/TransparentColorIndex", &propValue);
					if (SUCCEEDED(hr))
					{
						hr = (propValue.vt == VT_UI1 ? S_OK : E_FAIL);
						if (SUCCEEDED(hr) && propValue.uiVal < actualColors)
						{
							transparentIndex = static_cast<int>(propValue.uiVal);
							trace(transparentIndex);
						}
					}
				}
				PropVariantClear(&propValue);
			}

			hr = pMetadataReader->GetMetadataByName(L"/imgdesc/Left", &propValue);
			if (SUCCEEDED(hr))
			{
				hr = (propValue.vt == VT_UI2 ? S_OK : E_FAIL);
				if (SUCCEEDED(hr))
				{
					rt.left = static_cast<long>(propValue.uiVal);
				}
				PropVariantClear(&propValue);
			}

			hr = pMetadataReader->GetMetadataByName(L"/imgdesc/Top", &propValue);
			if (SUCCEEDED(hr))
			{
				hr = (propValue.vt == VT_UI2 ? S_OK : E_FAIL);
				if (SUCCEEDED(hr))
				{
					rt.top = static_cast<long>(propValue.uiVal);
				}
				PropVariantClear(&propValue);
			}
		}

		hr = pConverter->Initialize(
			pFrameDecode.Get(),
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

		if (i == 0)
			img_size = img->GetPixelSize();

		if (i > 0 && disposal == DM_NONE)
		{
			TRACE(_T("rt = %d, %d\n"), rt.left, rt.top);

			ComPtr<ID2D1Bitmap1> blendedBitmap;
			D2D1_BITMAP_PROPERTIES1 properties = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET, img->GetPixelFormat());
			hr = d2context->CreateBitmap(img_size, nullptr, 0, properties, &blendedBitmap);

			D2D1_POINT_2U pt = { 0, 0 };
			D2D_RECT_U r = { 0, 0, img_size.width, img_size.height };
			D2D1_SIZE_U size = img->GetPixelSize();
			blendedBitmap->CopyFromBitmap(&pt, m_img[previousFrame].Get(), &r);

#if 0

			save(L"d:\\img.png", img.Get());
			save(L"d:\\blended.png", blendedBitmap.Get());

			blend(d2context, blendedBitmap.Get(), img.Get(), rt.left, rt.top, 0, 0, size.width, size.height);
#elif 0
			// Offset BitmapB using transform effect
			ComPtr<ID2D1Effect> transformEffectB;
			d2context->CreateEffect(CLSID_D2D12DAffineTransform, &transformEffectB);
			transformEffectB->SetInput(0, img.Get());

			D2D1_MATRIX_3X2_F offsetMatrix = D2D1::Matrix3x2F::Translation(rt.left, rt.top);
			transformEffectB->SetValue(D2D1_2DAFFINETRANSFORM_PROP_TRANSFORM_MATRIX, offsetMatrix);


			ComPtr<ID2D1Effect> blendEffect;
			hr = d2context->CreateEffect(CLSID_D2D1Blend, &blendEffect);
			blendEffect->SetInput(0, m_img[previousFrame].Get());
			blendEffect->SetInputEffect(1, transformEffectB.Get());
			blendEffect->SetValue(D2D1_BLEND_PROP_MODE, D2D1_BLEND_MODE_MULTIPLY);

			d2context->SetTarget(blendedBitmap.Get());
			d2context->BeginDraw();
			d2context->DrawImage(blendEffect.Get());
			hr = d2context->EndDraw();
			save(L"d:\\img.png", img.Get());
			save(L"d:\\blended.png", blendedBitmap.Get());
#else
			d2context->SetTarget(blendedBitmap.Get());
			d2context->BeginDraw();
			//d2context->DrawBitmap(m_img[i - 1].Get());// , D2D1::RectF(0, 0, (FLOAT)img_size.width, (FLOAT)img_size.height), 1.0f, D2D1_INTERPOLATION_MODE_LINEAR, D2D1::RectF(0, 0, (FLOAT)img_size.width, (FLOAT)img_size.height));
			//D2D1_SIZE_U size = img->GetPixelSize();
			d2context->DrawBitmap(img.Get(), D2D1::RectF((FLOAT)rt.left, (FLOAT)rt.top, (FLOAT)(rt.left + size.width), (FLOAT)(rt.top + size.height)));// , 1.0f, D2D1_INTERPOLATION_MODE_LINEAR, D2D1::RectF(0, 0, (FLOAT)size.width, (FLOAT)size.height));
			hr = d2context->EndDraw();
			d2context->SetTarget(nullptr);
			save(img.Get(), _T("d:\\gif\\img\\%02d img (%d, %d).png"), i, rt.left, rt.top);
			save(blendedBitmap.Get(), _T("d:\\gif\\blended\\%02d blendedBitmap.png"), i);
#endif

			img->CopyFromBitmap(&pt, blendedBitmap.Get(), &r);
			/*
			//이전 프레임을 복사한다.
			//DM_NONE인 경우는 이전 프레임을 그대로 사용하는 것이므로
			//이전 프레임을 복사해서 사용한다.
			D2D1_SIZE_F sz = m_img[previousFrame]->GetSize();
			if (sz.width == img->GetSize().width && sz.height == img->GetSize().height)
			{
				m_img[previousFrame]->CopyFromBitmap()
					img.Get(),
					m_img[previousFrame].Get(),
					NULL,
					D2D1_POINT_2U{ 0, 0 }
				);
			}
			*/
		}

		m_img.push_back(std::move(img));
	}

	D2D1_SIZE_F sz = m_img[m_frame_index]->GetSize();
	m_width = sz.width;
	m_height = sz.height;

	return hr;
}

void CSCD2Image::blend(ID2D1DeviceContext* d2dc, ID2D1Bitmap* src, ID2D1Bitmap* blend_img, int dx, int dy, int sx, int sy, int sw, int sh)
{
	// Assume d2dContext is your ID2D1DeviceContext
	Microsoft::WRL::ComPtr<ID2D1Effect> bitmapEffectA;
	Microsoft::WRL::ComPtr<ID2D1Effect> bitmapEffectB;
	Microsoft::WRL::ComPtr<ID2D1Effect> transformEffectB;
	Microsoft::WRL::ComPtr<ID2D1Effect> compositeEffect;

	// Load bitmap into effects
	d2dc->CreateEffect(CLSID_D2D1BitmapSource, &bitmapEffectA);
	bitmapEffectA->SetInput(0, src);

	d2dc->CreateEffect(CLSID_D2D1BitmapSource, &bitmapEffectB);
	bitmapEffectB->SetInput(0, blend_img);

	// Offset BitmapB using transform effect
	d2dc->CreateEffect(CLSID_D2D12DAffineTransform, &transformEffectB);
	transformEffectB->SetInputEffect(0, bitmapEffectB.Get());

	D2D1_MATRIX_3X2_F offsetMatrix = D2D1::Matrix3x2F::Translation(dx, dy);
	transformEffectB->SetValue(D2D1_2DAFFINETRANSFORM_PROP_TRANSFORM_MATRIX, offsetMatrix);

	// Composite the two together
	//d2dc->CreateEffect(CLSID_D2D1Composite, &compositeEffect);
	//compositeEffect->SetInputEffect(0, bitmapEffectA.Get());
	//compositeEffect->SetInputEffect(1, transformEffectB.Get());

	ComPtr<ID2D1Effect> blendEffect;
	HRESULT hr = d2dc->CreateEffect(CLSID_D2D1Blend, &blendEffect);
	blendEffect->SetInputEffect(0, bitmapEffectA.Get());
	blendEffect->SetInputEffect(1, transformEffectB.Get());
	blendEffect->SetValue(D2D1_BLEND_PROP_MODE, D2D1_BLEND_MODE_MULTIPLY);

	// Draw the result
	d2dc->SetTarget(src);
	d2dc->BeginDraw();
	d2dc->DrawImage(blendEffect.Get());
	d2dc->EndDraw();
	d2dc->SetTarget(nullptr);

	//save(src, L"d:\\blended_src.png", );
	//save(L"d:\\blended_transformed.png", blend_img);
}

void CSCD2Image::copy(ID2D1DeviceContext* d2dc, ID2D1Bitmap* src, ID2D1Bitmap* dst)
{

}

void CSCD2Image::save(CString path)
{
	save(m_img[m_frame_index].Get(), path);
}

void CSCD2Image::save(ID2D1Bitmap* img, LPCTSTR path, ...)
{
	va_list args;
	va_start(args, path);

	CString filename;
	filename.FormatV(path, args);

	CString folder = get_part(filename, fn_folder);
	make_full_directory(folder);

	// Create a file stream
	ComPtr<IWICStream> pStream;
	HRESULT hr = m_pWICFactory->CreateStream(&pStream);
	if (SUCCEEDED(hr))
	{
		hr = pStream->InitializeFromFilename(filename, GENERIC_WRITE);
	}

	// Create WIC encoder
	ComPtr<IWICBitmapEncoder> pEncoder;
	GUID wicFormat = GUID_ContainerFormatPng;

	hr = m_pWICFactory->CreateEncoder(wicFormat, nullptr, &pEncoder);

	hr = pEncoder->Initialize(pStream.Get(), WICBitmapEncoderNoCache);

	// Create a new frame
	ComPtr<IWICBitmapFrameEncode> pFrameEncode;
	hr = pEncoder->CreateNewFrame(&pFrameEncode, nullptr);

	if (!pFrameEncode)
		return;

	hr = pFrameEncode->Initialize(nullptr);

	// Get IWICImageEncoder
	ComPtr<IWICImageEncoder> pImageEncoder;
	if (SUCCEEDED(hr))
	{
		CComPtr<ID2D1Device> d2dDevice;
		m_d2dc->GetDevice(&d2dDevice);
		hr = m_pWICFactory->CreateImageEncoder(d2dDevice, &pImageEncoder);
	}

	// Write the Direct2D bitmap to the WIC frame
	if (SUCCEEDED(hr))
	{
		hr = pImageEncoder->WriteFrame(img, pFrameEncode.Get(), nullptr);
	}

	// Commit the frame and encoder
	if (SUCCEEDED(hr))
	{
		hr = pFrameEncode->Commit();
	}
	if (SUCCEEDED(hr))
	{
		hr = pEncoder->Commit();
	}
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
D2D1_RECT_F CSCD2Image::draw(ID2D1DeviceContext* d2dc, int dx, int dy, int dw, int dh)
{
	//dw, dh가 0이하이면 원본 크기로 그려준다.
	if (dw <= 0 && dh > 0)
	{
		dw = (int)(m_width * dh / m_height);
	}
	else if (dw > 0 && dh <= 0)
	{
		dh = (int)(m_height * dw / m_width);
	}
	else if (dw <= 0 && dh <= 0)
	{
		dw = (int)m_width;
		dh = (int)m_height;
	}

	return draw(d2dc, D2D1::RectF(dx, dy, dx + dw, dy + dh));
}

D2D1_RECT_F CSCD2Image::draw(ID2D1DeviceContext* d2dc, D2D1_POINT_2F pt)
{
	return draw(d2dc, D2D1::RectF(pt.x, pt.y, pt.x + m_width, pt.y + m_height));
}

D2D1_RECT_F CSCD2Image::draw(ID2D1DeviceContext* d2dc, D2D1_RECT_F target, eSCD2Image_DRAW_MODE draw_mode)
{
	D2D1_RECT_F r = { 0, 0, 0, 0 };

	if (m_frame_index >= m_img.size())
		return r;

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

void CSCD2Image::play()
{
	if (m_img.size() <= 1)
		return;
	//thread를 이용하여 현재 프레임의 delay가 지나면 m_frame_index를 증가시키고
	//parent에게 이를 알려 re-rendering하도록 한다.
	if (m_run_thread_animation)
	{
		m_ani_paused = !m_ani_paused;
		return;
	}

	std::thread t(&CSCD2Image::thread_animation, this);
	t.detach();
}

void CSCD2Image::pause(int pos)
{
	if (m_img.size() <= 1 || !m_run_thread_animation)
		return;
	
	if (pos >= m_img.size())
		pos = 0;

	m_frame_index = pos;
	m_ani_paused = true;
}

void CSCD2Image::stop()
{
	if (m_img.size() <= 1)
		return;
	pause(0);
}

void CSCD2Image::thread_animation()
{
	m_run_thread_animation = true;
	m_ani_paused = false;

	while (m_run_thread_animation)
	{
		if (m_ani_paused)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			continue;
		}

		m_frame_index++;
		if (m_frame_index >= m_img.size())
			m_frame_index = 0;

		//TRACE(_T("frame index = %d\n"), m_frame_index);
		::PostMessage(m_parent, Message_CSCD2Image, (WPARAM)0, m_frame_index);

		int delay = m_frame_delay[m_frame_index];
		std::this_thread::sleep_for(std::chrono::milliseconds(delay));
	}

	m_run_thread_animation = false;
}

//data멤버에 픽셀 데이터를 가리키도록 한다.
void CSCD2Image::get_raw_data()
{
	if (data)
		delete[] data;

	data = new uint8_t[(int)(m_width * m_height * 4)];

	WICPixelFormatGUID format = GUID_WICPixelFormat32bppBGRA;
	WICRect rcLock = { 0, 0, (INT)m_width, (INT)m_height };
	IWICBitmapLock* pLock = NULL;

	HRESULT hr = S_OK;

}
