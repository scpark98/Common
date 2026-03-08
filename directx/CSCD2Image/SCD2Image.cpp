#include "SCD2Image.h"

#include <thread>
#include <mutex>
#include <webp/mux.h>
#include <webp/demux.h>
#include <webp/encode.h>
#include <webp/decode.h>

#include "../../SCGdiplusBitmap.h"

#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "Windowscodecs.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "d3d11.lib")

#pragma comment(lib, "libwebp.lib")
#pragma comment(lib, "libwebpmux.lib")
#pragma comment(lib, "libwebpdemux.lib")

static std::recursive_mutex g_d2d_dc_mutex;

CSCD2Image::CSCD2Image()
{
}

CSCD2Image::CSCD2Image(CSCD2Image&& other) noexcept  // 이동 생성자
: m_filename(std::move(other.m_filename))
, m_pWICFactory(other.m_pWICFactory)
, m_d2dc(other.m_d2dc)
, m_img(std::move(other.m_img))
, m_img_origin(other.m_img_origin)
, m_data(other.m_data)
, m_stride(other.m_stride)
, m_channel(other.m_channel)
, m_frame_index(other.m_frame_index)
, m_pixel_format_str(std::move(other.m_pixel_format_str))
, m_alpha_pixel_count(other.m_alpha_pixel_count)
, m_exif_info(other.m_exif_info)
, m_interpolation_mode(other.m_interpolation_mode)
, m_parent(other.m_parent)
, m_frame_delay(std::move(other.m_frame_delay))
, m_ani_mirror(other.m_ani_mirror)
, m_run_thread_animation(other.m_run_thread_animation)
, m_thread_animation_terminated(other.m_thread_animation_terminated)
, m_ani_paused(other.m_ani_paused)
{
	other.m_data = nullptr;
	other.m_pWICFactory = nullptr;
	other.m_d2dc = nullptr;
	other.m_parent = NULL;
	other.m_frame_index = 0;
	other.m_stride = 0;
	other.m_channel = 0;
}

CSCD2Image& CSCD2Image::operator=(CSCD2Image&& other) noexcept
{
	if (this != &other)
	{
		release();

		m_filename = std::move(other.m_filename);
		m_pWICFactory = other.m_pWICFactory;
		m_d2dc = other.m_d2dc;
		m_img = std::move(other.m_img);
		m_img_origin = other.m_img_origin;
		m_data = other.m_data;
		m_stride = other.m_stride;
		m_channel = other.m_channel;
		m_frame_index = other.m_frame_index;
		m_pixel_format_str = std::move(other.m_pixel_format_str);
		m_alpha_pixel_count = other.m_alpha_pixel_count;
		m_exif_info = other.m_exif_info;
		m_interpolation_mode = other.m_interpolation_mode;
		m_parent = other.m_parent;
		m_frame_delay = std::move(other.m_frame_delay);
		m_ani_mirror = other.m_ani_mirror;
		m_run_thread_animation = other.m_run_thread_animation;
		m_thread_animation_terminated = other.m_thread_animation_terminated;
		m_ani_paused = other.m_ani_paused;

		other.m_data = nullptr;
		other.m_pWICFactory = nullptr;
		other.m_d2dc = nullptr;
		other.m_parent = NULL;
		other.m_frame_index = 0;
		other.m_stride = 0;
		other.m_channel = 0;
	}
	return *this;
}

CSCD2Image::~CSCD2Image()
{
	release();
}

void CSCD2Image::release()
{
	// 1. 애니메이션 스레드 안전하게 종료
	if (m_run_thread_animation)
	{
		m_run_thread_animation = false;

		// 타임아웃 설정: 최대 500ms 대기
		int timeout_count = 0;
		const int MAX_TIMEOUT = 50;  // 50 * 10ms = 500ms

		while (!m_thread_animation_terminated && timeout_count < MAX_TIMEOUT)
		{
			Wait(10);
			timeout_count++;
		}

		// 타임아웃 발생 시 경고 로그
		if (!m_thread_animation_terminated)
		{
			TRACE(_T("Warning: Animation thread did not terminate within timeout at line %d\n"), __LINE__);
			// 스레드가 종료될 때까지 무한 대기하지 않고 계속 진행
		}
	}

	// 2. m_data 메모리 정리
	if (m_data != nullptr)
	{
		delete[] m_data;
		m_data = nullptr;
	}

	// 3. 이미지 deque 정리 (ComPtr 자동 정리됨)
	m_img.clear();
	m_img_origin.Reset();

	// 4. 프레임 딜레이 정리
	m_frame_delay.clear();

	// 5. 참조 포인터 NULL 처리
	m_pWICFactory = nullptr;
	m_d2dc = nullptr;
	m_parent = NULL;

	// 6. 상태 초기화
	m_frame_index = 0;
	m_stride = 0;
	m_channel = 0;
	m_run_thread_animation = false;
	m_thread_animation_terminated = true;
	m_img_origin_for_back_transparency.Reset();
}

HRESULT CSCD2Image::create(IWICImagingFactory2* WICfactory, ID2D1DeviceContext* d2context, int width, int height)
{
	m_img.clear();

	m_pWICFactory = WICfactory;
	m_d2dc = d2context;

	HRESULT hr;
	ComPtr<ID2D1Bitmap1> img;
	D2D1_BITMAP_PROPERTIES1 properties = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
	hr = d2context->CreateBitmap(D2D1::SizeU(width, height), nullptr, 0, properties, img.GetAddressOf());

	m_img.push_back(img);

	return hr;
}

HRESULT CSCD2Image::load(IWICImagingFactory2* pWICFactory, ID2D1DeviceContext* d2context, UINT resource_id, CString type, bool auto_play)
{
	while (!stop())
		Wait(10);

	m_filename.Format(_T("Resource Image(id:%d)"), resource_id);

	ComPtr<IWICStream> pStream;
	IWICBitmapDecoder* pDecoder = NULL;

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
		pStream.Get(),                   // The stream to use to create the decoder
		NULL,                          // Do not prefer a particular vendor
		WICDecodeMetadataCacheOnLoad,  // Cache metadata when needed
		&pDecoder);                   // Pointer to the decoder

	// Retrieve the first bitmap frame.
	if (FAILED(hr) || !pDecoder)
	{
		pStream->Release();
		return hr;
	}

	return load(pWICFactory, d2context, pDecoder, auto_play);
}

HRESULT CSCD2Image::load(IWICImagingFactory2* pWICFactory, ID2D1DeviceContext* d2context, CString path, bool auto_play)
{
	while (!stop())
		Wait(10);

	m_filename = path;

	HRESULT hr = S_FALSE;
	ComPtr<IWICBitmapDecoder> pDecoder;

	if (!pWICFactory)
		return S_FALSE;

	if (!d2context)
		return S_FALSE;

	/*
	HRESULT hr = pWICFactory->CreateDecoderFromFilename(
		path,
		NULL,
		GENERIC_READ,
		WICDecodeMetadataCacheOnLoad,
		pDecoder.GetAddressOf()
	);
	*/

	//20260129 scpark 위와 같이 외부 파일을 CreateDecoderFromFilename()로 열면 잠겨져서 파일명 변경, 삭제, 이동 등 아무것도 할 수 없다.
	//아래와 같이 stream으로 읽어서 해결함.
	ULONGLONG file_size = get_file_size(path);

	if (file_size == 0)
		return S_FALSE;

	std::vector<uint8_t> data(file_size);
	read_raw(path, data.data(), file_size);

	ComPtr<IWICStream> stream;
	pWICFactory->CreateStream(&stream);

	stream->InitializeFromMemory(
		data.data(),
		static_cast<DWORD>(file_size)
	);

	pWICFactory->CreateDecoderFromStream(
		stream.Get(),
		nullptr,
		WICDecodeMetadataCacheOnLoad,
		&pDecoder
	);

	if (FAILED(hr) || !pDecoder)
		return hr;

	hr = load(pWICFactory, d2context, pDecoder.Get(), auto_play);

	// WebP인 경우 실제 프레임 딜레이를 덮어쓴다
	if (SUCCEEDED(hr))
	{
		CString ext = get_part(path, fn_ext).MakeLower();
		if (ext == _T("webp"))
			read_webp_frame_delay(path);
	}

	return hr;
}

//load from raw data
HRESULT CSCD2Image::load(IWICImagingFactory2* pWICFactory, ID2D1DeviceContext* d2context, void* data, int width, int height, int channel)
{
	HRESULT	hr = S_FALSE;

	m_pWICFactory = pWICFactory;
	m_d2dc = d2context;

	m_frame_index = 0;
	m_img.clear();
	m_frame_delay.clear();
	m_img_origin_for_back_transparency.Reset();

	IWICBitmap* pWicBitmap = NULL;
	IWICFormatConverter* pFormatConverter = NULL;
	ComPtr<ID2D1Bitmap1> img;

	//투명 png를 GUID_WICPixelFormat32bppBGRA가 아닌 PBGRA로 읽으면 알파채널이 올바르게 표시되지 않는다.
	WICPixelFormatGUID guid = GUID_WICPixelFormat32bppBGRA;

	switch (channel)
	{
		case 1:
			guid = GUID_WICPixelFormat8bppIndexed;
			break;
		case 2:
			guid = GUID_WICPixelFormat16bppBGR565;
			break;
		case 3:
			guid = GUID_WICPixelFormat24bppBGR;
			break;
	}

	{
		//if ((((HRESULT)(HRESULT _hr = (m_pWICFactory->CreateBitmapFromMemory(width, height, guid, width * channel, width * height * channel, (BYTE*)data, &pWicBitmap)))) < 0)) TraceHR(__LPREFIX(__FUNCTION__), 158, _hr); _hr;
	}

	_M(hr, m_pWICFactory->CreateBitmapFromMemory(width, height, guid, width * channel, width * height * channel, (BYTE*)data, &pWicBitmap));
	_M(hr, d2context->CreateBitmapFromWicBitmap(pWicBitmap, 0, &img));

	if (!img)
	{
		//pFormatConverter->Release();
		m_pWICFactory->CreateFormatConverter(&pFormatConverter);
		if (pFormatConverter)
		{
			pFormatConverter->Initialize(pWicBitmap, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.0f, WICBitmapPaletteTypeMedianCut);
			_M(hr, d2context->CreateBitmapFromWicBitmap(pFormatConverter, 0, &img));
		}
	}

	if (img)
	{
		m_img.push_back(std::move(img));
	}

	return hr;
}

HRESULT CSCD2Image::extract_exif_info(IWICBitmapDecoder* pDecoder)
{
	HRESULT	hr = S_FALSE;
	m_exif_info = CSCEXIFInfo();

	ComPtr<IWICBitmapFrameDecode> pBitmapFrameDecode;
	hr = pDecoder->GetFrame(0, &pBitmapFrameDecode);
	
	//JPG, TIFF 등의 exif 정보가 다르므로 Ifd, exif reader를 별도로 구해서 얻어온다.
	ComPtr<IWICMetadataQueryReader> pRootQueryReader;
	_M(hr, pBitmapFrameDecode->GetMetadataQueryReader(&pRootQueryReader));
	if (hr != S_OK || !pRootQueryReader)
		return hr;

	//get IFD query reader
	ComPtr<IWICMetadataQueryReader> pIfdQueryReader;
	CString sIFDPath = _T("/ifd");

	GUID guidFormat = { 0 };
	_M(hr, pDecoder->GetContainerFormat(&guidFormat));
	if (IsEqualGUID(guidFormat, GUID_ContainerFormatJpeg))
		sIFDPath = _T("/app1/ifd");

	PROPVARIANT value;
	//이 초기화를 해주지 않으면 아래 frame delay를 구하는 코드에서 올바른 값이 들어가지 않는다.
	PropVariantClear(&value);

	//get ifd query reader
	hr = pRootQueryReader->GetMetadataByName(sIFDPath, &value);
	if (FAILED(hr))
		return hr;
	else if (value.vt != VT_UNKNOWN)
		return E_FAIL;

	value.punkVal->QueryInterface(IID_IWICMetadataQueryReader, &pIfdQueryReader);
	PropVariantClear(&value);

	//get embedded EXIF query reader
	ComPtr<IWICMetadataQueryReader> pExifQueryReader;
	hr = pIfdQueryReader->GetMetadataByName(_T("/exif"), &value);
	if (FAILED(hr))
		return hr;
	else if (value.vt != VT_UNKNOWN)
		return E_FAIL;

	value.punkVal->QueryInterface(IID_IWICMetadataQueryReader, &pExifQueryReader);
	PropVariantClear(&value);


	hr = pIfdQueryReader->GetMetadataByName(L"/{ushort=271}", &value);
	if (SUCCEEDED(hr) && (value.vt == VT_LPSTR))
		m_exif_info.camera_make = value.pszVal;
	PropVariantClear(&value);

	hr = pIfdQueryReader->GetMetadataByName(L"/{ushort=272}", &value);
	if (SUCCEEDED(hr) && (value.vt == VT_LPSTR))
		m_exif_info.camera_model = value.pszVal;
	PropVariantClear(&value);

	hr = pIfdQueryReader->GetMetadataByName(L"/{ushort=274}", &value);
	if (SUCCEEDED(hr) && (value.vt == VT_UI2))
		m_exif_info.orientation = value.uiVal;

	// Orientation을 문자열로 변환
	switch (m_exif_info.orientation)
	{
		case 1:
			m_exif_info.orientation_str = _T("Normal");
			break;
		case 2:
			m_exif_info.orientation_str = _T("FlipX");
			break;
		case 3:
			m_exif_info.orientation_str = _T("Rotate 180°");
			break;
		case 4:
			m_exif_info.orientation_str = _T("FlipY");
			break;
		case 5:
			m_exif_info.orientation_str = _T("Rotate 90° CCW + FlipY");
			break;
		case 6:
			m_exif_info.orientation_str = _T("Rotate 90° CW");
			break;
		case 7:
			m_exif_info.orientation_str = _T("Rotate 90° CW + FlipY");
			break;
		case 8:
			m_exif_info.orientation_str = _T("90° CCW");
			break;
		default:
			m_exif_info.orientation_str = _T("Unknown");
			break;
	}
	PropVariantClear(&value);


	hr = pIfdQueryReader->GetMetadataByName(L"/{ushort=305}", &value);
	if (SUCCEEDED(hr) && (value.vt == VT_LPSTR))
		m_exif_info.software = value.pszVal;
	PropVariantClear(&value);

	hr = pIfdQueryReader->GetMetadataByName(L"/{ushort=306}", &value);
	if (SUCCEEDED(hr) && (value.vt == VT_LPSTR))
		m_exif_info.last_modified_datetime = value.pszVal;
	PropVariantClear(&value);
	if (m_exif_info.last_modified_datetime.GetLength() > 8)
	{
		int pos = m_exif_info.last_modified_datetime.Find(':');
		if (pos > 0)
		{
			m_exif_info.last_modified_datetime.SetAt(pos, '/');
			pos = m_exif_info.last_modified_datetime.Find(':');
			if (pos > 0)
				m_exif_info.last_modified_datetime.SetAt(pos, '/');
		}
	}

	hr = pIfdQueryReader->GetMetadataByName(L"/{ushort=270}", &value);
	if (SUCCEEDED(hr) && (value.vt == VT_LPSTR))
		m_exif_info.image_description = value.pszVal;
	PropVariantClear(&value);

	hr = pExifQueryReader->GetMetadataByName(L"/{ushort=33432}", &value);
	if (SUCCEEDED(hr) && (value.vt == VT_LPSTR))
		m_exif_info.image_copyright = value.pszVal;
	PropVariantClear(&value);

	hr = pExifQueryReader->GetMetadataByName(L"/{ushort=36867}", &value);
	if (SUCCEEDED(hr) && (value.vt == VT_LPSTR))
		m_exif_info.original_datetime = value.pszVal;
	PropVariantClear(&value);
	if (m_exif_info.original_datetime.GetLength() > 8)
	{
		int pos = m_exif_info.original_datetime.Find(':');
		if (pos > 0)
		{
			m_exif_info.original_datetime.SetAt(pos, '/');
			pos = m_exif_info.original_datetime.Find(':');
			if (pos > 0)
				m_exif_info.original_datetime.SetAt(pos, '/');
		}
	}


	long* plong = NULL;
	hr = pExifQueryReader->GetMetadataByName(L"/{ushort=33434}", &value);
	if (SUCCEEDED(hr) && (value.vt == VT_UI8))
	{
		m_exif_info.exposure_time = double(value.cyVal.Lo) / double(value.cyVal.Hi);
	}
	PropVariantClear(&value);

	hr = pExifQueryReader->GetMetadataByName(L"/{ushort=37380}", &value);
	if (SUCCEEDED(hr) && (value.vt == VT_I8))
		m_exif_info.exposure_bias = value.dblVal;
	PropVariantClear(&value);

	hr = pExifQueryReader->GetMetadataByName(L"/{ushort=33437}", &value);
	if (SUCCEEDED(hr) && (value.vt == VT_UI8))
	{
		m_exif_info.f_number = double(value.cyVal.Lo) / double(value.cyVal.Hi);
	}
	PropVariantClear(&value);

	hr = pExifQueryReader->GetMetadataByName(L"/{ushort=34855}", &value);
	if (SUCCEEDED(hr) && (value.vt == VT_UI2))
	{
		m_exif_info.iso_speed = value.uiVal;
	}
	PropVariantClear(&value);

	hr = pExifQueryReader->GetMetadataByName(L"/{ushort=37386}", &value);
	if (SUCCEEDED(hr) && (value.vt == VT_UI8))
	{
		m_exif_info.focal_length = double(value.cyVal.Lo) / double(value.cyVal.Hi);
	}
	PropVariantClear(&value);

	hr = pExifQueryReader->GetMetadataByName(L"/{ushort=41989}", &value);
	if (SUCCEEDED(hr) && (value.vt == VT_UI2))
	{
		m_exif_info.focal_length_in_35mm = value.uiVal;
	}
	PropVariantClear(&value);

	/*
	hr = pExifQueryReader->GetMetadataByName(L"/{ushort=4}", &value);
	if (SUCCEEDED(hr) && (value.vt == VT_UI8))
	{
		m_exif_info.gps_longitude = double(value.cyVal.Lo) / double(value.cyVal.Hi);
	}
	PropVariantClear(&value);

	hr = pExifQueryReader->GetMetadataByName(L"/{ushort=6}", &value);
	if (SUCCEEDED(hr) && (value.vt == VT_UI8))
	{
		m_exif_info.gps_altitude = double(value.cyVal.Lo) / double(value.cyVal.Hi);
	}
	PropVariantClear(&value);
	*/
	// ===== GPS 정보 추출 (보완) =====
	// GPS 정보는 IFD 레벨에 있으므로 pIfdQueryReader에서 찾음
	ComPtr<IWICMetadataQueryReader> pGpsQueryReader;
	hr = pIfdQueryReader->GetMetadataByName(_T("/gps"), &value);

	if (SUCCEEDED(hr) && value.vt == VT_UNKNOWN)
	{
		value.punkVal->QueryInterface(IID_IWICMetadataQueryReader, &pGpsQueryReader);
		PropVariantClear(&value);

		if (pGpsQueryReader)
		{
			// ===== GPS Latitude (ushort=2) =====
			// GPSLatitudeRef (N/S)
			hr = pGpsQueryReader->GetMetadataByName(L"/{ushort=1}", &value);
			CString gps_lat_ref;
			if (SUCCEEDED(hr) && (value.vt == VT_LPSTR))
				gps_lat_ref = value.pszVal;
			PropVariantClear(&value);

			// GPSLatitude (3개의 Rational: 도/분/초)
			hr = pGpsQueryReader->GetMetadataByName(L"/{ushort=2}", &value);
			double gps_latitude = 0.0;
			if (SUCCEEDED(hr) && (value.vt == (VT_VECTOR | VT_UI8)))
			{
				// Rational 배열: [도의분자/분모, 분의분자/분모, 초의분자/분모]
				if (value.capropvar.cElems >= 3 && value.capropvar.pElems != nullptr)
				{
					// DWORD 배열로 캐스팅 (더 명확함)
					DWORD* pRationals = reinterpret_cast<DWORD*>(value.capropvar.pElems);

					// 각 Rational: [분자0, 분모0, 분자1, 분모1, 분자2, 분모2]
					double degrees = (double)pRationals[0] / (double)pRationals[1];
					double minutes = (double)pRationals[2] / (double)pRationals[3];
					double seconds = (double)pRationals[4] / (double)pRationals[5];

					gps_latitude = degrees + minutes / 60.0 + seconds / 3600.0;

					// 남반구는 음수
					if (gps_lat_ref == "S")
						gps_latitude = -gps_latitude;

					m_exif_info.gps_latitude = gps_latitude;
					m_exif_info.gps_latitude_str = double_to_gps(gps_latitude, true);
					TRACE(_T("GPS Latitude: %.6f (%s)\n"), gps_latitude, gps_lat_ref);
				}
			}
			PropVariantClear(&value);

			// GPSLongitudeRef (E/W)
			hr = pGpsQueryReader->GetMetadataByName(L"/{ushort=3}", &value);
			CString gps_lon_ref;
			if (SUCCEEDED(hr) && (value.vt == VT_LPSTR))
				gps_lon_ref = value.pszVal;
			PropVariantClear(&value);

			// GPSLongitude (3개의 Rational)
			hr = pGpsQueryReader->GetMetadataByName(L"/{ushort=4}", &value);
			double gps_longitude = 0.0;
			if (SUCCEEDED(hr) && (value.vt == (VT_VECTOR | VT_UI8)))
			{
				if (value.capropvar.cElems >= 3 && value.capropvar.pElems != nullptr)
				{
					DWORD* pRationals = reinterpret_cast<DWORD*>(value.capropvar.pElems);

					double degrees = (double)pRationals[0] / (double)pRationals[1];
					double minutes = (double)pRationals[2] / (double)pRationals[3];
					double seconds = (double)pRationals[4] / (double)pRationals[5];

					gps_longitude = degrees + minutes / 60.0 + seconds / 3600.0;

					// 서반구는 음수
					if (gps_lon_ref == "W")
						gps_longitude = -gps_longitude;

					m_exif_info.gps_longitude = gps_longitude;
					m_exif_info.gps_longitude_str = double_to_gps(gps_longitude, false);
					TRACE(_T("GPS Longitude: %.6f (%s)\n"), gps_longitude, gps_lon_ref);
				}
			}
			PropVariantClear(&value);

			// ===== GPS Altitude (ushort=6) =====
			// GPSAltitudeRef (0=위, 1=아래)
			hr = pGpsQueryReader->GetMetadataByName(L"/{ushort=5}", &value);
			int gps_alt_ref = 0;
			if (SUCCEEDED(hr) && (value.vt == VT_UI1))
				gps_alt_ref = value.bVal;
			PropVariantClear(&value);

			// GPSAltitude (1개의 Rational)
			hr = pGpsQueryReader->GetMetadataByName(L"/{ushort=6}", &value);
			double gps_altitude = 0.0;
			if (SUCCEEDED(hr) && (value.vt == VT_UI8))
			{
				// Rational: 분자/분모
				double numerator = (double)(value.cyVal.Lo);
				double denominator = (double)(value.cyVal.Hi);

				if (denominator != 0.0)
				{
					gps_altitude = numerator / denominator;

					// 아래(음수)면 부호 반전
					if (gps_alt_ref == 1)
						gps_altitude = -gps_altitude;

					m_exif_info.gps_altitude = gps_altitude;
					TRACE(_T("GPS Altitude: %.2f m\n"), gps_altitude);
				}
			}
			PropVariantClear(&value);
		}
	}
	else
	{
		PropVariantClear(&value);
	}

	return hr;
}

CString CSCD2Image::get_exif_str()
{
	if (m_exif_info.camera_make.IsEmpty() && m_exif_info.camera_model.IsEmpty())
		return _T("");

	return m_exif_info.get_exif_str();
}

//일부 animated gif의 경우 이전 프레임 이미지에서 변경된 이미지만 저장하고 있는 경우도 있으므로
//아래 코드를 참조하여 보완 필요.
//https://github.com/microsoft/DirectXTex/blob/main/Texassemble/AnimatedGif.cpp
HRESULT CSCD2Image::load(IWICImagingFactory2* pWICFactory, ID2D1DeviceContext* d2context, IWICBitmapDecoder* pDecoder, bool auto_play)
{
	std::lock_guard<std::recursive_mutex> lock(g_d2d_dc_mutex);

	m_pWICFactory = pWICFactory;
	m_d2dc = d2context;

	m_frame_index = 0;
	m_img.clear();
	m_frame_delay.clear();

	D2D1_SIZE_U img_size = { 0, 0 };
	UINT frame_count = 1;
	HRESULT	hr = pDecoder->GetFrameCount(&frame_count);

	//TRACE(_T("total %d frames.\n"), frame_count);

	PROPVARIANT propValue;
	//이 초기화를 해주지 않으면 아래 frame delay를 구하는 코드에서 올바른 값이 들어가지 않는다.
	PropVariantClear(&propValue);

	if (FAILED(hr))
		return hr;

	ComPtr<IWICBitmapFrameDecode> pFrameDecode;
	ComPtr<IWICMetadataQueryReader> meta_reader;

	pDecoder->GetMetadataQueryReader(meta_reader.GetAddressOf());

	//일반 이미지, gif, tiff, webp 등의 구조가 조금씩 달라 우선 여기서 이미지의 크기를 먼저 구한다.
	//ex. gif의 0번 프레임을 얻고 그 크기를 구하면 실제 이미지 크기와 다른 경우가 있다.
	if (meta_reader)
	{
		hr = meta_reader->GetMetadataByName(L"/logscrdesc/Width", &propValue);
		if (SUCCEEDED(hr) && (propValue.vt == VT_UI2))
			img_size.width = propValue.uiVal;
		PropVariantClear(&propValue);

		hr = meta_reader->GetMetadataByName(L"/logscrdesc/Height", &propValue);
		if (SUCCEEDED(hr) && (propValue.vt == VT_UI2))
			img_size.height = propValue.uiVal;
		PropVariantClear(&propValue);
	}

	extract_exif_info(pDecoder);

	// Get palette
	WICColor rgbColors[256] = {};
	UINT actualColors = 0;

	ComPtr<IWICFormatConverter> pConverter;
	ComPtr<ID2D1Bitmap1> img;
	ComPtr<IWICBitmap> wicBitmap;
	WICPixelFormatGUID pf;
	D2D_RECT_U r = { 0, 0, img_size.width, img_size.height };
	UINT stride;

	//frame_count = 1인 일반 이미지들도 아래의 for문으로 공통 처리하려니 예외적인 경우가 많아 별도 처리함.
	if (frame_count == 1)
	{
		_M(hr, pWICFactory->CreateFormatConverter(pConverter.GetAddressOf()));
		_M(hr, pDecoder->GetFrame(0, pFrameDecode.GetAddressOf()));

		if (FAILED(hr) || !pFrameDecode)
			return hr;

		pFrameDecode->GetSize(&img_size.width, &img_size.height);

		pFrameDecode->GetPixelFormat(&pf);
		get_pixel_format_str(&pf);
		m_stride = m_channel * img_size.width;

		hr = pConverter->Initialize(
			pFrameDecode.Get(),
			GUID_WICPixelFormat32bppPBGRA,
			WICBitmapDitherTypeNone,
			nullptr,
			0.f,
			WICBitmapPaletteTypeMedianCut
		);

		if (FAILED(hr))
		{
			TRACE(_T("FormatConverter Initialize failed: 0x%08X\n"), hr);
			return hr;
		}

		// 5) D2D bitmap 생성 (여기서 멈춘다면 hr를 출력해야 함)
		_M(hr, d2context->CreateBitmapFromWicBitmap(pConverter.Get(), nullptr, img.GetAddressOf()));
		if (FAILED(hr) || !img)
		{
			TRACE(_T("CreateBitmapFromWicBitmap(img) failed: 0x%08X\n"), hr);
			return hr;
		}

		_M(hr, d2context->CreateBitmapFromWicBitmap(pConverter.Get(), nullptr, m_img_origin.GetAddressOf()));
		if (FAILED(hr) || !m_img_origin)
		{
			TRACE(_T("CreateBitmapFromWicBitmap(origin) failed: 0x%08X\n"), hr);
			return hr;
		}

		//pixel data loading start.
		//회전된 이미지는 apply_orientation_transform()에서 처리함.
		if (m_exif_info.orientation <= 1 || m_exif_info.orientation > 8)
		{
			pWICFactory->CreateBitmapFromSource(
				pConverter.Get(),
				WICBitmapCacheOnLoad,
				&wicBitmap
			);

			WICRect rc = { 0, 0, img_size.width, img_size.height };
			ComPtr<IWICBitmapLock> lock;
			wicBitmap->Lock(&rc, WICBitmapLockRead, &lock);

			lock->GetStride(&stride);

			UINT bufferSize = 0;
			BYTE* pixels = nullptr;
			lock->GetDataPointer(&bufferSize, &pixels);

			if (m_data)
				delete[] m_data;

			m_data = new uint8_t[bufferSize];
			memcpy(m_data, pixels, bufferSize);
			lock.Reset();
			wicBitmap.Reset();
		}
		//pixel data loading end.

		m_img.push_back(std::move(img));
		apply_orientation_transform();
	}
	else if (frame_count > 1)
	{
		//gif의 bgColor 정보를 추출하기 위한 코드들인데 webp인 경우는 에러가 발생하므로 우선 주석처리함.
		/*
		ComPtr<IWICPalette> palette;
		_M(pWICFactory->CreatePalette(palette.GetAddressOf()));

		//webp의 경우는 팔레트를 사용할 수 없다.
		_M(pDecoder->CopyPalette(palette.Get()));
		_M(palette->GetColors(static_cast<UINT>(std::size(rgbColors)), rgbColors, &actualColors));

		// Get background color
		UINT bgColor = 0;
		bool usebgcolor = false;
		if (usebgcolor && meta_reader)
		{
			// Most browsers just ignore the background color metadata and always use transparency
			_M(meta_reader->GetMetadataByName(L"/logscrdesc/GlobalColorTableFlag", &propValue));

			const bool hasTable = (propValue.vt == VT_BOOL && propValue.boolVal);
			PropVariantClear(&propValue);

			if (hasTable)
			{
				_M(meta_reader->GetMetadataByName(L"/logscrdesc/BackgroundColorIndex", &propValue));

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
		*/

		UINT previousFrame = 0;
		UINT disposal = DM_UNDEFINED;
		RECT rt = {};
		D2D1_POINT_2U pt = { 0, 0 };

		bool save_img_for_debug = false;

		for (int i = 0; i < frame_count; i++)
		{
			int transparentIndex = -1;

			ComPtr<IWICMetadataQueryReader> pMetadataReader;

			_M(hr, pWICFactory->CreateFormatConverter(pConverter.GetAddressOf()));
			_M(hr, pDecoder->GetFrame(i, pFrameDecode.GetAddressOf()));
			_M(hr, pFrameDecode->GetMetadataQueryReader(pMetadataReader.GetAddressOf()));

			if (i == 0)
			{
				pFrameDecode->GetPixelFormat(&pf);
				get_pixel_format_str(&pf);

				//만약 위에서 이미지 크기를 구해오지 못했을 경우에만 여기서 새로 구한다.
				//일부 gif이미지는 각 프레임마다 크기가 다를 수 있으므로 매번 구해서는 안된다.
				if (img_size.width == 0 || img_size.height == 0)
					pFrameDecode->GetSize(&img_size.width, &img_size.height);

				m_stride = m_channel * img_size.width;
			}

			ComPtr<ID2D1Bitmap1> frame_img;
			D2D1_BITMAP_PROPERTIES1 properties = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
			hr = d2context->CreateBitmap(img_size, nullptr, 0, properties, frame_img.GetAddressOf());

			if (i == 0)
			{
				d2context->CreateBitmap(img_size, nullptr, 0, properties, m_img_origin.GetAddressOf());
			}

			//프레임 구성에 따라 전 프레임을 복사하는 경우도 있고, 특정 키프레임을 복사하는 경우도 있다.
			//둘 다 아니라면 투명 배경을 가진 이미지로 시작한다.
			if (disposal == DM_PREVIOUS)
			{
				frame_img->CopyFromBitmap(&pt, m_img[previousFrame].Get(), &r);
			}
			else if ((i > 0) && (disposal != DM_BACKGROUND))
			{
				frame_img->CopyFromBitmap(&pt, m_img[i - 1].Get(), &r);
			}

			if (save_img_for_debug)
				save(frame_img.Get(), 1.0f, _T("d:\\direct2d_test\\frame_img.png"));

			//특정 배경색을 사용하는 경우의 처리인데 일단 불필요하여 주석처리 함.
			/*
			ID2D1SolidColorBrush* br;
			//d2context->CreateSolidColorBrush(D2D1::ColorF(bgColor), &br);
			//d2context->CreateSolidColorBrush(D2D1::ColorF(1.0f, 0.0f, 0.0f, 0.5f), &br);
			d2context->CreateSolidColorBrush(D2D1::ColorF(1.0f, 0.0f, 0.0f, 0.0f), &br);

			d2context->SetTarget(frame_img.Get());
			d2context->BeginDraw();
			if (i == 0)
			{
				d2context->FillRectangle(D2D1::RectF(0, 0, img_size.width, img_size.height), br);
			}
			else if (disposal == DM_BACKGROUND)
			{
				d2context->FillRectangle(D2D1::RectF(rt.left, rt.top, rt.right, rt.bottom), br);
			}
			hr = d2context->EndDraw();
			d2context->SetTarget(nullptr);

			if (save_img_for_debug)
				save(frame_img.Get(), _T("d:\\direct2d_test\\frame_img.png"));
			*/


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
						//TRACE(_T("#%dth frame. delay = %d ms\n"), i, delay);
					}
					PropVariantClear(&propValue);
				}
				else
				{
					//webp는 위 방식으로 읽을 수 없고 파일 로딩 후 read_webp_frame_delay() 함수를 호출해서 다시 불러오도록 구현되어 있다.
					m_frame_delay.push_back(10);
				}

				//calling.gif와 같이 1번 프레임부터 변경된 이미지 정보만 저장된 gif의 경우는 disposal 값은 DM_NONE로 추출된다.
				//이럴 경우 이전 프레임을 복사해서 사용해야 한다.
				//그 외 각 프레임이 온전한 경우는 DM_BACKGROUND로 추출된다.
				//webp의 경우 아래 모든 속성값은 무의미하며 S_FAIL로 떨어진다.
				disposal = DM_UNDEFINED;
				hr = pMetadataReader->GetMetadataByName(L"/grctlext/Disposal", &propValue);
				if (SUCCEEDED(hr))
				{
					hr = (propValue.vt == VT_UI1 ? S_OK : E_FAIL);
					if (SUCCEEDED(hr))
					{
						disposal = propValue.bVal;
						//trace(disposal);
					}
					PropVariantClear(&propValue);
				}

				hr = pMetadataReader->GetMetadataByName(L"/grctlext/TransparencyFlag", &propValue);
				if (SUCCEEDED(hr))
				{
					hr = (propValue.vt == VT_BOOL ? S_OK : E_FAIL);
					if (SUCCEEDED(hr) && propValue.boolVal)
					{
						//TRACE(_T("is transparent frame = %d\n"), propValue.boolVal);
						PropVariantClear(&propValue);
						hr = pMetadataReader->GetMetadataByName(L"/grctlext/TransparentColorIndex", &propValue);
						if (SUCCEEDED(hr))
						{
							hr = (propValue.vt == VT_UI1 ? S_OK : E_FAIL);
							if (SUCCEEDED(hr) && propValue.uiVal < actualColors)
							{
								transparentIndex = static_cast<int>(propValue.uiVal);
								//trace(transparentIndex);
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

				hr = pMetadataReader->GetMetadataByName(L"/imgdesc/Width", &propValue);
				if (SUCCEEDED(hr))
				{
					hr = (propValue.vt == VT_UI2 ? S_OK : E_FAIL);
					if (SUCCEEDED(hr))
					{
						rt.right = rt.left + static_cast<long>(propValue.uiVal);
					}
					PropVariantClear(&propValue);
				}

				hr = pMetadataReader->GetMetadataByName(L"/imgdesc/Height", &propValue);
				if (SUCCEEDED(hr))
				{
					hr = (propValue.vt == VT_UI2 ? S_OK : E_FAIL);
					if (SUCCEEDED(hr))
					{
						rt.bottom = rt.top + static_cast<long>(propValue.uiVal);
					}
					PropVariantClear(&propValue);
				}
			}

			hr = pConverter->Initialize(
				pFrameDecode.Get(),
				GUID_WICPixelFormat32bppPBGRA,
				WICBitmapDitherTypeNone,
				nullptr,
				0.f,
				WICBitmapPaletteTypeMedianCut
			);

			if (FAILED(hr))
				return hr;

			hr = d2context->CreateBitmapFromWicBitmap(pConverter.Get(), nullptr, img.GetAddressOf());

			//여러장의 이미지를 담고 있는 이미지일 경우 0번 이미지의 색상정보만 유지한다.
			if (i == 0)
			{
				//pixel data loading start.
				pWICFactory->CreateBitmapFromSource(
					pConverter.Get(),
					WICBitmapCacheOnLoad,
					&wicBitmap
				);

				//webp인 경우는 rt값이 0으로 넘어오므로 img_size를 사용해야 한다.
				WICRect rc = { 0, 0, rt.right - rt.left, rt.bottom - rt.top };
				if (rt.right == 0 || rt.bottom == 0)
				{
					rc.Width = img_size.width;
					rc.Height = img_size.height;
				}

				ComPtr<IWICBitmapLock> lock;
				wicBitmap->Lock(&rc, WICBitmapLockRead, &lock);

				lock->GetStride(&stride);
				//m_channel = m_stride / rc.Width;

				UINT bufferSize = 0;
				BYTE* pixels = nullptr;
				lock->GetDataPointer(&bufferSize, &pixels);

				if (m_data)
				{
					delete[] m_data;
					m_data = nullptr;
				}

				m_data = new uint8_t[bufferSize];
				memcpy(m_data, pixels, bufferSize);
				lock.Reset();
				//pixel data loading end.
			}

			d2context->SetTarget(frame_img.Get());
			d2context->BeginDraw();
			if ((rt.right - rt.left <= 0) || (rt.bottom - rt.top <= 0))//false)//i == 0 || transparentIndex == -1)
			{
				//현재 프레임의 이미지를 그려준다.
				d2context->DrawBitmap(img.Get(), D2D1::RectF(0, 0, img_size.width, img_size.height));
			}
			else
			{
				//현재 프레임의 이미지를 그려준다.
				d2context->DrawBitmap(img.Get(), D2D1::RectF((FLOAT)rt.left, (FLOAT)rt.top, (FLOAT)(rt.right), (FLOAT)(rt.bottom)));
			}
			hr = d2context->EndDraw();
			d2context->SetTarget(nullptr);

			if (disposal == DM_UNDEFINED || disposal == DM_NONE)
				previousFrame = i;

			if (save_img_for_debug)
			{
				save(img.Get(), 1.0f, _T("d:\\direct2d_test\\img.png"));
				save(frame_img.Get(), 1.0f, _T("d:\\direct2d_test\\frame_img.png"));
			}

			m_img.push_back(std::move(frame_img));
			/*
			if (i == 0)
			{
				//어떤 gif는 위의 "/logscrdesc/Width"로 가져오지 못하는 경우가 있다.
				//이런 경우는 0번 프레임의 크기로 설정한다.
				if (img_size.width == 0 || img_size.height == 0)
				{
					img_size.width = frame_img->GetSize().width;
					img_size.height = frame_img->GetSize().height;
				}

				pFrameDecode->GetPixelFormat(&pf);
				get_pixel_format_str(&pf);
			}

			//webp의 경우 위에서 rt정보를 얻을 수 없다. empty라면 기본 이미지 크기로 채워준다.
			if (rt.right == 0 || rt.bottom == 0)
			{
				rt.right = img_size.width;
				rt.bottom = img_size.height;
			}

			TRACE(_T("#%03d. rt = %s\n"), i, get_rect_info_str(rt));

			//어떤 gif는 전 프레임 이미지에 변경된 정보만 가진 gif도 존재한다.
			//이럴 경우는 이전 프레임에 현재 이미지를 blend하여 m_img에 push해야 한다.
			if (disposal == DM_NONE || ((rt.right - rt.left) < img_size.width) || ((rt.bottom - rt.top) < img_size.height))
			{
				//이전 프레임 이미지를 복사하여 새 이미지를 생성한다.
				ComPtr<ID2D1Bitmap1> blendedBitmap;
				D2D1_BITMAP_PROPERTIES1 properties = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET, frame_img->GetPixelFormat());
				hr = d2context->CreateBitmap(img_size, nullptr, 0, properties, blendedBitmap.GetAddressOf());

				D2D1_POINT_2U pt = { 0, 0 };
				D2D_RECT_U r = { 0, 0, img_size.width, img_size.height };
				D2D1_SIZE_U size = frame_img->GetPixelSize();
				if (i > 0)// && transparentIndex != 255)
					blendedBitmap->CopyFromBitmap(&pt, m_img[previousFrame].Get(), &r);

				//이전 프레임 이미지위에
				d2context->SetTarget(blendedBitmap.Get());
				d2context->BeginDraw();

				//현재 프레임의 이미지를 그려준다.
				d2context->DrawBitmap(frame_img.Get(), D2D1::RectF((FLOAT)rt.left, (FLOAT)rt.top, (FLOAT)(rt.right), (FLOAT)(rt.bottom)));// , 1.0f, D2D1_INTERPOLATION_MODE_LINEAR, D2D1::RectF(0, 0, (FLOAT)size.width, (FLOAT)size.height));
				hr = d2context->EndDraw();
				d2context->SetTarget(nullptr);

				m_img.push_back(std::move(blendedBitmap));
				//save(img.Get(), _T("d:\\gif\\img\\%02d img (%d, %d).png"), i, rt.left, rt.top);
				//save(blendedBitmap.Get(), _T("d:\\gif\\blended\\%02d blendedBitmap.png"), i);
			}
			else
			{
				m_img.push_back(std::move(frame_img));
			}
			*/
		}
	}

	D2D1_SIZE_F sz = m_img[m_frame_index]->GetSize();

	//TRACE(_T("before reset meta_reader\n"));
	meta_reader.Reset();
	//TRACE(_T("after reset meta_reader\n"));

	if (frame_count > 1 && auto_play)
		play();

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
	d2dc->CreateEffect(CLSID_D2D1BitmapSource, bitmapEffectA.GetAddressOf());
	bitmapEffectA->SetInput(0, src);

	d2dc->CreateEffect(CLSID_D2D1BitmapSource, bitmapEffectB.GetAddressOf());
	bitmapEffectB->SetInput(0, blend_img);

	// Offset BitmapB using transform effect
	d2dc->CreateEffect(CLSID_D2D12DAffineTransform, transformEffectB.GetAddressOf());
	transformEffectB->SetInputEffect(0, bitmapEffectB.Get());

	D2D1_MATRIX_3X2_F offsetMatrix = D2D1::Matrix3x2F::Translation(dx, dy);
	transformEffectB->SetValue(D2D1_2DAFFINETRANSFORM_PROP_TRANSFORM_MATRIX, offsetMatrix);

	// Composite the two together
	//d2dc->CreateEffect(CLSID_D2D1Composite, &compositeEffect);
	//compositeEffect->SetInputEffect(0, bitmapEffectA.Get());
	//compositeEffect->SetInputEffect(1, transformEffectB.Get());

	ComPtr<ID2D1Effect> blendEffect;
	HRESULT hr = d2dc->CreateEffect(CLSID_D2D1Blend, blendEffect.GetAddressOf());
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

//void CSCD2Image::copy(ID2D1DeviceContext* d2dc, ID2D1Bitmap* src, ID2D1Bitmap* dst)
//{
//
//}

//copy는 deep copy이며 이미지 자체 뿐 만 아니라 CSCD2Image의 속성값들도 모두 복사해줘야 한다.
//protected에 나열된 순서대로 필요 항목들을 복사해준다.
void CSCD2Image::copy(CSCD2Image* dst)
{
	dst->m_filename = m_filename;

	dst->m_pWICFactory = m_pWICFactory;
	dst->m_d2dc = m_d2dc;

	dst->m_img.clear();
	dst->m_img.assign(m_img.begin(), m_img.end());

	dst->m_channel = m_channel;
	dst->m_stride = m_stride;

	dst->m_pixel_format_str = m_pixel_format_str;
	dst->m_alpha_pixel_count = m_alpha_pixel_count;

	memcpy(&(dst->m_exif_info), &m_exif_info, sizeof(CSCEXIFInfo));

	dst->m_interpolation_mode = m_interpolation_mode;

	dst->m_parent = m_parent;
	dst->m_frame_delay.clear();
	dst->m_frame_delay.assign(m_frame_delay.begin(), m_frame_delay.end());
	dst->m_ani_mirror = m_ani_mirror;
}

//m_img의 특정 인덱스 이미지를 dst에 복사한다.
//반드시 new 또는 정적으로 메모리가 할당된 상태의 dst를 넘겨줘야 한다.
//null의 dst를 넘기고 copy()에서 new로 할당해도 되지만 new와 delete이 서로 다른 thread에서 수행될 수 없으므로 문제가 될 수 있다.
HRESULT CSCD2Image::copy(int src_index, ID2D1Bitmap1* dst)
{
	HRESULT hr = dst->CopyFromBitmap(nullptr, m_img[src_index].Get(), nullptr);
	return hr;
}

HRESULT CSCD2Image::get_sub_img(CRect r, CSCD2Image* dest)
{
	D2D1_RECT_U rsrc = { r.left, r.top, r.right, r.bottom };
	return get_sub_img(rsrc, dest);
}

HRESULT CSCD2Image::get_sub_img(D2D1_RECT_U r, CSCD2Image* dest)
{
	if (m_img.size() == 0 || m_img[m_frame_index] == nullptr)
		return S_FALSE;

	D2D1_POINT_2U pt = { 0, 0 };
	return dest->get_cur_img()->CopyFromBitmap(&pt, m_img[m_frame_index].Get(), &r);
}

void CSCD2Image::restore_original_image()
{
	if (m_img.size() == 0 || m_img[0] == nullptr)
		return;

	D2D1_POINT_2U pt = { 0, 0 };
	D2D1_RECT_U r = { 0, 0, get_width(), get_height()};
	m_img[0].Get()->CopyFromBitmap(&pt, m_img_origin.Get(), &r);
}

void CSCD2Image::blur_effect(float dev)
{
	restore_original_image();

	Microsoft::WRL::ComPtr<ID2D1BitmapRenderTarget> rt;
	Microsoft::WRL::ComPtr<ID2D1Bitmap> bitmap;
	Microsoft::WRL::ComPtr<ID2D1Effect> blurEffect;
	Microsoft::WRL::ComPtr<ID2D1Effect> compositeEffect;

	//m_d2dc->CreateCompatibleRenderTarget(m_d2dc->GetSize(), &rt);
	//rt->BeginDraw();
	//rt->Clear(D2D1::ColorF(0, 0, 0, 0));

	//rt->DrawBitmap(m_img[0].Get());
	//rt->EndDraw();

	m_d2dc->CreateEffect(CLSID_D2D1GaussianBlur, &blurEffect);
	blurEffect->SetInput(0, m_img[0].Get());
	blurEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION, dev);
	//blurEffect->GetOutput()

	ComPtr<ID2D1Image> swapChainImageBuffer;
	m_d2dc->GetTarget(swapChainImageBuffer.GetAddressOf());
	m_d2dc->SetTarget(m_img[0].Get());
	m_d2dc->BeginDraw();
	m_d2dc->Clear(D2D1::ColorF(0, 0, 0, 1.0f));
	m_d2dc->DrawImage(blurEffect.Get(), D2D1::Point2F(0, 0));
	m_d2dc->EndDraw();
	m_d2dc->SetTarget(swapChainImageBuffer.Get());
	swapChainImageBuffer = nullptr;


}

void CSCD2Image::convert_PBGRA_to_RGBA(byte* pixels, int width, int height, int stride)
{
	for (UINT y = 0; y < height; ++y)
	{
		BYTE* row = pixels + y * stride;

		for (UINT x = 0; x < width; ++x)
		{
			BYTE* px = row + x * 4;

			BYTE b = px[0];
			BYTE g = px[1];
			BYTE r = px[2];
			BYTE a = px[3];

			if (a != 0)
			{
				r = (BYTE)(min(255, (r * 255) / a));
				g = (BYTE)(min(255, (g * 255) / a));
				b = (BYTE)(min(255, (b * 255) / a));
			}

			px[0] = r;
			px[1] = g;
			px[2] = b;
			px[3] = a;
		}
	}
}

// un-premultiply만 수행, 채널 순서(BGRA)는 유지
void CSCD2Image::convert_PBGRA_to_BGRA(byte* pixels, int width, int height, int stride)
{
	for (UINT y = 0; y < height; ++y)
	{
		BYTE* row = pixels + y * stride;

		for (UINT x = 0; x < width; ++x)
		{
			BYTE* px = row + x * 4;
			BYTE a = px[3];

			if (a > 0 && a < 255)
			{
				px[0] = (BYTE)(min(255, px[0] * 255 / a));  // B
				px[1] = (BYTE)(min(255, px[1] * 255 / a));  // G
				px[2] = (BYTE)(min(255, px[2] * 255 / a));  // R
			}
			// a==0 또는 a==255이면 변환 불필요
		}
	}
}

HRESULT CSCD2Image::save_webp(LPCTSTR path, ...)
{
	//이미지가 정상적으로 로딩되어있지 않은 경우
	if (m_pWICFactory == nullptr)
		return S_FALSE;

	HRESULT hr = S_OK;

	va_list args;
	va_start(args, path);
	va_end(args);

	CString filename;
	filename.FormatV(path, args);

	int width = get_width();
	int height = get_height();
	WebPAnimEncoderOptions enc_options;
	WebPAnimEncoderOptionsInit(&enc_options);

	// 키프레임 관련 설정: 모든 프레임을 독립적으로 인코딩
	enc_options.kmin = 0;
	enc_options.kmax = 0;
	enc_options.allow_mixed = 1;

	WebPAnimEncoder* enc = WebPAnimEncoderNew(width, height, &enc_options);

	int timestamp = 0;

	for (int i = 0; i < get_frame_count(); ++i)
	{
		D2D1_BITMAP_PROPERTIES1 props =
		{
			{ DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED },
			96.0f,
			96.0f,
			D2D1_BITMAP_OPTIONS_CPU_READ | D2D1_BITMAP_OPTIONS_CANNOT_DRAW
		};

		ComPtr<ID2D1Bitmap1> cpuBitmap;
		m_d2dc->CreateBitmap(
			D2D1::SizeU(width, height),
			nullptr,
			0,
			&props,
			&cpuBitmap
		);

		cpuBitmap->CopyFromBitmap(nullptr, m_img[i].Get(), nullptr);

		D2D1_MAPPED_RECT mapped;
		cpuBitmap->Map(D2D1_MAP_OPTIONS_READ, &mapped);

		BYTE* pixels = mapped.bits;
		UINT stride = mapped.pitch;

		convert_PBGRA_to_RGBA(pixels, width, height, stride);

		WebPPicture pic;
		WebPPictureInit(&pic);

		pic.width = width;
		pic.height = height;
		pic.use_argb = 1;

		WebPPictureImportRGBA(&pic, pixels, stride);

		// 프레임별 인코딩 품질 설정
		WebPConfig config;
		WebPConfigInit(&config);
		config.lossless = 1;       // 무손실 (필요 시 0으로 변경)
		config.quality = 100.0f;

		WebPAnimEncoderAdd(enc, &pic, timestamp, &config);

		timestamp += m_frame_delay[i];

		WebPPictureFree(&pic);
		cpuBitmap->Unmap();
	}

	// 마지막 프레임 flush
	WebPAnimEncoderAdd(enc, nullptr, timestamp, nullptr);

	WebPData webp_data;
	WebPDataInit(&webp_data);

	WebPAnimEncoderAssemble(enc, &webp_data);

	FILE* fp = NULL;
	_tfopen_s(&fp, filename, _T("wb"));

	if (fp == NULL)
	{
		TRACE(_T("can't create file : %s\n"), filename);
		hr = S_FALSE;
	}
	else
	{
		fwrite(webp_data.bytes, webp_data.size, 1, fp);
		fclose(fp);
	}

	WebPDataClear(&webp_data);
	WebPAnimEncoderDelete(enc);

	return hr;
}

HRESULT CSCD2Image::save_gif(LPCTSTR path, ...)
{
	if (m_pWICFactory == nullptr)
		return S_FALSE;

	HRESULT hr = S_OK;

	va_list args;
	va_start(args, path);
	va_end(args);

	CString filename;
	filename.FormatV(path, args);

	int width = (int)get_width();
	int height = (int)get_height();
	int frame_count = get_frame_count();

	if (width <= 0 || height <= 0 || frame_count <= 0)
		return S_FALSE;

	// GIF 인코더 생성
	ComPtr<IWICStream> pStream;
	_M(hr, m_pWICFactory->CreateStream(pStream.GetAddressOf()));
	_M(hr, pStream->InitializeFromFilename(CStringW(filename), GENERIC_WRITE));
	if (FAILED(hr)) return hr;

	ComPtr<IWICBitmapEncoder> pEncoder;
	_M(hr, m_pWICFactory->CreateEncoder(GUID_ContainerFormatGif, nullptr, pEncoder.GetAddressOf()));
	_M(hr, pEncoder->Initialize(pStream.Get(), WICBitmapEncoderNoCache));
	if (FAILED(hr)) return hr;

	// 애니메이션 GIF: NETSCAPE2.0 루프 확장 설정 (무한 반복)
	if (frame_count > 1)
	{
		ComPtr<IWICMetadataQueryWriter> pGlobalMeta;
		hr = pEncoder->GetMetadataQueryWriter(&pGlobalMeta);
		if (SUCCEEDED(hr) && pGlobalMeta)
		{
			PROPVARIANT propValue;
			PropVariantInit(&propValue);

			// Application: "NETSCAPE2.0"
			propValue.vt = VT_UI1 | VT_VECTOR;
			propValue.caub.cElems = 11;
			BYTE appId[] = "NETSCAPE2.0";
			propValue.caub.pElems = appId;
			pGlobalMeta->SetMetadataByName(L"/appext/Application", &propValue);
			//PropVariantInit(&propValue);

			// Data: [sub-block size=3, ID=1, loopLo=0, loopHi=0] → 무한 반복
			BYTE appData[] = { 3, 1, 0, 0 };
			propValue.caub.cElems = 4;
			propValue.caub.pElems = appData;
			pGlobalMeta->SetMetadataByName(L"/appext/Data", &propValue);
			PropVariantInit(&propValue);
		}
	}

	bool hasAlpha = (m_alpha_pixel_count > 0);

	for (int i = 0; i < frame_count; ++i)
	{
		// D2D 비트맵 → CPU 읽기 가능한 비트맵 복사
		D2D1_BITMAP_PROPERTIES1 cpuProps = {
			{ DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED },
			96.0f, 96.0f,
			D2D1_BITMAP_OPTIONS_CPU_READ | D2D1_BITMAP_OPTIONS_CANNOT_DRAW
		};

		ComPtr<ID2D1Bitmap1> cpuBitmap;
		_M(hr, m_d2dc->CreateBitmap(D2D1::SizeU(width, height), nullptr, 0, &cpuProps, &cpuBitmap));
		if (FAILED(hr)) return hr;

		_M(hr, cpuBitmap->CopyFromBitmap(nullptr, m_img[i].Get(), nullptr));
		if (FAILED(hr)) return hr;

		D2D1_MAPPED_RECT mapped;
		_M(hr, cpuBitmap->Map(D2D1_MAP_OPTIONS_READ, &mapped));
		if (FAILED(hr)) return hr;

		// Premultiplied Alpha → Straight Alpha (BGRA 채널 순서 유지)
		UINT stride = width * 4;
		UINT imageSize = stride * height;
		std::vector<BYTE> pixels(imageSize);
		for (UINT y = 0; y < (UINT)height; ++y)
			memcpy(pixels.data() + y * stride, mapped.bits + y * mapped.pitch, stride);

		cpuBitmap->Unmap();

		convert_PBGRA_to_BGRA(pixels.data(), width, height, stride);

		// WIC 비트맵 생성 (BGRA 32bit)
		ComPtr<IWICBitmap> wicBitmap;
		_M(hr, m_pWICFactory->CreateBitmapFromMemory(
			width, height, GUID_WICPixelFormat32bppBGRA,
			stride, imageSize, pixels.data(), &wicBitmap));
		if (FAILED(hr)) return hr;

		// 팔레트 생성 (GIF는 8bpp 인덱스 컬러 필수)
		ComPtr<IWICPalette> pPalette;
		_M(hr, m_pWICFactory->CreatePalette(&pPalette));

		// 투명 픽셀이 있으면 255색 + 투명 1색, 없으면 256색
		if (hasAlpha)
		{
			_M(hr, pPalette->InitializeFromBitmap(wicBitmap.Get(), 255, TRUE));
		}
		else
		{
			_M(hr, pPalette->InitializeFromBitmap(wicBitmap.Get(), 256, FALSE));
		}

		if (FAILED(hr))
			return hr;

		// 32bpp BGRA → 8bpp Indexed 변환
		ComPtr<IWICFormatConverter> pConverter;
		_M(hr, m_pWICFactory->CreateFormatConverter(&pConverter));
		_M(hr, pConverter->Initialize(
			wicBitmap.Get(),
			GUID_WICPixelFormat8bppIndexed,
			WICBitmapDitherTypeErrorDiffusion,
			pPalette.Get(),
			hasAlpha ? 50.0 : 0.0,
			WICBitmapPaletteTypeCustom));
		if (FAILED(hr)) return hr;

		// 프레임 인코딩
		ComPtr<IWICBitmapFrameEncode> pFrameEncode;
		CComPtr<IPropertyBag2> pProperties;
		_M(hr, pEncoder->CreateNewFrame(&pFrameEncode, &pProperties));
		_M(hr, pFrameEncode->Initialize(nullptr));
		_M(hr, pFrameEncode->SetSize(width, height));

		WICPixelFormatGUID pixelFormat = GUID_WICPixelFormat8bppIndexed;
		_M(hr, pFrameEncode->SetPixelFormat(&pixelFormat));
		_M(hr, pFrameEncode->SetPalette(pPalette.Get()));

		// 프레임 메타데이터: delay, disposal, transparency
		if (frame_count > 1 && i < (int)m_frame_delay.size())
		{
			ComPtr<IWICMetadataQueryWriter> pFrameMeta;
			hr = pFrameEncode->GetMetadataQueryWriter(&pFrameMeta);
			if (SUCCEEDED(hr) && pFrameMeta)
			{
				PROPVARIANT propValue;
				PropVariantInit(&propValue);

				// 프레임 딜레이 (GIF는 1/100초 단위, m_frame_delay는 ms 단위)
				propValue.vt = VT_UI2;
				propValue.uiVal = (USHORT)(m_frame_delay[i] / 10);
				pFrameMeta->SetMetadataByName(L"/grctlext/Delay", &propValue);

				// Disposal: 2 = restore to background
				propValue.vt = VT_UI1;
				propValue.bVal = 2;
				pFrameMeta->SetMetadataByName(L"/grctlext/Disposal", &propValue);

				if (hasAlpha)
				{
					// 투명 플래그 활성화
					propValue.vt = VT_BOOL;
					propValue.boolVal = VARIANT_TRUE;
					pFrameMeta->SetMetadataByName(L"/grctlext/TransparencyFlag", &propValue);

					// 투명 색상 인덱스 (팔레트의 마지막 엔트리)
					UINT colorCount = 0;
					pPalette->GetColorCount(&colorCount);
					propValue.vt = VT_UI1;
					propValue.bVal = (BYTE)(colorCount > 0 ? colorCount - 1 : 0);
					pFrameMeta->SetMetadataByName(L"/grctlext/TransparentColorIndex", &propValue);
				}

				PropVariantClear(&propValue);
			}
		}

		_M(hr, pFrameEncode->WriteSource(pConverter.Get(), nullptr));
		_M(hr, pFrameEncode->Commit());
	}

	_M(hr, pEncoder->Commit());

	return hr;
}

HRESULT CSCD2Image::save(CString path, float quality)
{
	return save(m_img[m_frame_index].Get(), quality, path);
}

HRESULT CSCD2Image::save(ID2D1Bitmap* img, float quality, LPCTSTR path, ...)
{
	//이미지가 정상적으로 로딩되어있지 않은 경우
	if (m_pWICFactory == nullptr)
		return S_FALSE;

	HRESULT hr = S_OK;

	va_list args;
	va_start(args, path);

	CString filename;
	filename.FormatV(path, args);

	CString ext = get_part(filename, fn_ext).MakeLower();
	CString folder = get_part(filename, fn_folder);
	make_full_directory(folder);

	//webp일 경우는 별도 처리한다.
	if (ext == _T("webp"))
	{
		hr = save_webp(filename);
		return hr;
	}
	else if (ext == _T("gif"))
	{
		hr = save_gif(filename);
		return hr;
	}

	// Create a file stream
	ComPtr<IWICStream> pStream;
	_M(hr, m_pWICFactory->CreateStream(pStream.GetAddressOf()));
	_M(hr, pStream->InitializeFromFilename(filename, GENERIC_WRITE));

	PROPBAG2 name = { 0 };
	name.dwType = PROPBAG2_TYPE_DATA;
	name.vt = VT_R4;
	wchar_t propName[] = L"ImageQuality";
	name.pstrName = propName;

	// Create WIC encoder
	ComPtr<IWICBitmapEncoder> pEncoder;
	GUID wicFormat = GUID_ContainerFormatRaw;

	//wicFormat = CLSID_WICPngEncoder;
	if (ext == _T("png"))
	{
		wicFormat = GUID_ContainerFormatPng;
		name.vt = VT_I2;
		wchar_t propName[] = L"CompressionLevel";
		name.pstrName = propName;
	}
	else if (ext == _T("jpg"))
		wicFormat = GUID_ContainerFormatJpeg;
	else if (ext == _T("bmp"))
		wicFormat = GUID_ContainerFormatBmp;
	else if (ext == _T("raw"))
		wicFormat = GUID_ContainerFormatRaw;

	_M(hr, m_pWICFactory->CreateEncoder(wicFormat, nullptr, pEncoder.GetAddressOf()));
	_M(hr, pEncoder->Initialize(pStream.Get(), WICBitmapEncoderNoCache));

	// Create a new frame
	ComPtr<IWICBitmapFrameEncode> pFrameEncode;
	CComPtr<IPropertyBag2> properties;

	_M(hr, pEncoder->CreateNewFrame(pFrameEncode.GetAddressOf(), &properties));

	if (ext == _T("jpg"))
	{
		CComVariant value(quality);
		_M(hr, properties->Write(1, &name, &value));
		_M(hr, pFrameEncode->Initialize(properties));
	}
	else
	{
		_M(hr, pFrameEncode->Initialize(nullptr));
	}

	// Get IWICImageEncoder
	ComPtr<IWICImageEncoder> pImageEncoder;
	ComPtr<ID2D1Device> d2dDevice;
	m_d2dc->GetDevice(d2dDevice.GetAddressOf());
	_M(hr, m_pWICFactory->CreateImageEncoder(d2dDevice.Get(), pImageEncoder.GetAddressOf()));

	// Write the Direct2D bitmap to the WIC frame
	_M(hr, pImageEncoder->WriteFrame(img, pFrameEncode.Get(), nullptr));

	// Commit the frame and encoder
	_M(hr, pFrameEncode->Commit());
	_M(hr, pEncoder->Commit());

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
	//	TRACE(_T("rc = %dx%d, sz_img = %.0fx%.0f, r = %s\n"), width, height, sz_img.width, sz_img.height, get_rect_info_str(r));
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
/*
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
			IID_PPV_ARGS(surface.GetAddressOf())
		);
	}

	ComPtr<ID2D1Bitmap1> bitmap = nullptr;
	if (SUCCEEDED(hr)) {
		FLOAT dpiX, dpiY;
		dpiX = 96.0f;// (FLOAT)GetDpiForWindow(::GetDesktopWindow());
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
			bitmap.GetAddressOf()
		);
	}

	if (SUCCEEDED(hr)) {
		d2context->SetTarget(bitmap.Get());
	}

	return S_OK;
}
*/

ID2D1Bitmap1* CSCD2Image::get_frame_img(int index)
{
	if (m_img.size() == 0 || index < 0 || index >= m_img.size())
		return nullptr;

	return m_img[index].Get();
}

float CSCD2Image::get_width()
{
	if (m_img.size() == 0 || m_frame_index < 0 || m_frame_index >= m_img.size())
		return 0.0f;

	if (m_img[m_frame_index])
	{
		D2D1_SIZE_F sz = m_img[m_frame_index]->GetSize();
		return sz.width;
	}

	return 0.0f;
}

float CSCD2Image::get_height()
{
	if (m_img.size() == 0 || m_frame_index < 0 || m_frame_index >= m_img.size())
		return 0.0f;

	if (m_img[m_frame_index])
	{
		D2D1_SIZE_F sz = m_img[m_frame_index]->GetSize();
		return sz.height;
	}

	return 0.0f;
}

D2D1_SIZE_F CSCD2Image::get_size()
{
	if (m_img.size() == 0 || m_frame_index < 0 || m_frame_index >= m_img.size())
		return D2D1::SizeF();

	D2D1_SIZE_F sz = m_img[m_frame_index]->GetSize();
	return sz;
}

float CSCD2Image::get_ratio()
{
	if (get_height() <= 0.0f)
		return 0.0f;
	return get_width() / get_height();
}

//index 위치의 이미지가 nullptr이 아니고, width, height 모두 0보다 커야 한다.
bool CSCD2Image::is_empty(int index)
{
	if (this == NULL)
		return true;

	if (index < 0 || index >= (int)m_img.size())
		return true;

	if (m_img[index].Get() == NULL)
		return true;

	if (get_width() <= 0.0f || get_height() <= 0.0f)
		return true;

	return false;
}

bool CSCD2Image::is_valid(int index)
{
	return !is_empty(index);
}

CString CSCD2Image::get_filename(bool fullpath)
{
	if (!fullpath)
		return get_part(m_filename, fn_name);

	return m_filename;
}

void CSCD2Image::set_interpolation_mode(int mode)
{
	m_interpolation_mode = ((D2D1_BITMAP_INTERPOLATION_MODE)mode);
}

D2D1_RECT_F CSCD2Image::draw(ID2D1DeviceContext* d2dc, eSCD2Image_DRAW_MODE draw_mode)
{
	if (is_empty())
		return D2D1::RectF();

	D2D1_SIZE_F sz = d2dc->GetSize();
	return draw(d2dc, D2D1::RectF(0, 0, sz.width, sz.height), draw_mode);
}

//dx, dy 좌표에 그려준다.
D2D1_RECT_F CSCD2Image::draw(ID2D1DeviceContext* d2dc, int dx, int dy, int dw, int dh, float dwr, float dhr)
{
	//dh가 주어진 경우
	if (dw <= 0 && dh > 0)
	{
		dw = (int)(get_width() * dh / get_height());
	}
	//dw가 주어진 경우
	else if (dw > 0 && dh <= 0)
	{
		dh = (int)(get_height() * dw / get_width());
	}
	//dwr or dwh가 주어진 경우
	else if (dw <= 0 && dh <= 0)
	{
		if (dwr > 0.f && dhr > 0.f)
		{
			dw = get_width() * dwr;
			dh = get_height() * dhr;
		}
		else if (dwr <= 0.f && dhr > 0.f)
		{
			dw = get_width() * dhr;
			dh = get_height() * dhr;
		}
		else if (dwr > 0.f && dhr <= 0.f)
		{
			dw = get_width() * dwr;
			dh = get_height() * dwr;
		}
		else
		{
			dw = get_width();
			dh = get_height();
		}
	}

	return draw(d2dc, D2D1::RectF(dx, dy, dx + dw, dy + dh));
}

D2D1_RECT_F CSCD2Image::draw(ID2D1DeviceContext* d2dc, D2D1_POINT_2F pt)
{
	return draw(d2dc, D2D1::RectF(pt.x, pt.y, pt.x + get_width(), pt.y + get_height()));
}

D2D1_RECT_F CSCD2Image::draw(ID2D1DeviceContext* d2dc, D2D1_RECT_F target, eSCD2Image_DRAW_MODE draw_mode)
{
	std::lock_guard<std::recursive_mutex> lock(g_d2d_dc_mutex);

	D2D1_RECT_F r = { 0, 0, 0, 0 };

	if (m_frame_index >= (int)m_img.size())
		return r;

	if (draw_mode == eSCD2Image_DRAW_MODE::draw_mode_stretch)
		r = target;
	else if (draw_mode == eSCD2Image_DRAW_MODE::draw_mode_zoom)
	{
		r = get_ratio_rect(target, get_width() / get_height());

		if ((get_width() <= (target.right - target.left)) && (get_height() <= (target.bottom - target.top)))
		{
			float w = r.right - r.left;
			float h = r.bottom - r.top;
			r.left = target.left;
			r.top = target.top;
			r.right = r.left + w;
			r.bottom = r.top + h;
		}
	}
	else if (draw_mode == eSCD2Image_DRAW_MODE::draw_mode_original)
	{
		r = target;
		r.right = r.left + get_width();
		r.bottom = r.top + get_height();
	}
	else //eSCD2Image_DRAW_MODE::draw_mode_original_center
	{
		r = D2D1::RectF(0, 0, get_width(), get_height());
		r.left = target.left + (target.right - target.left - get_width()) / 2.0;
		r.top = target.top + (target.bottom - target.top - get_height()) / 2.0;
	}

	//D2D1_POINT_2F cp = { (r.right - r.left) / 2.0f, (r.bottom - r.top) / 2.0f };
	//d2dc->SetTransform(D2D1::Matrix3x2F::Rotation(90.0f, D2D1::Point2F(cp.x, cp.y)));

	d2dc->DrawBitmap(m_img[m_frame_index].Get(), r, 1.0f, (D2D1_BITMAP_INTERPOLATION_MODE)m_interpolation_mode);
	//d2dc->SetTransform(D2D1::Matrix3x2F::Rotation(-90.0f, D2D1::Point2F(cp.x, cp.y)));
	return r;
}

//그림을 그리지 않고 표시될 영역 정보만 얻는다.
CRect CSCD2Image::calc_rect(CRect targetRect, eSCD2Image_DRAW_MODE draw_mode)
{
	CRect result;
	D2D1_RECT_F d2target = { targetRect.left, targetRect.top, targetRect.right, targetRect.bottom };

	if (draw_mode == eSCD2Image_DRAW_MODE::draw_mode_stretch)
		result = targetRect;
	else if (draw_mode == eSCD2Image_DRAW_MODE::draw_mode_zoom)
		result = d2RectF_to_CRect(get_ratio_rect(d2target, get_width(), get_height()));
	else
	{
		result = CRect(0, 0, get_width(), get_height());
		result.OffsetRect(targetRect.left + (targetRect.Width() - get_width()) / 2, targetRect.top + (targetRect.Height() - get_height()) / 2);
	}

	return result;
}

//n개의 이미지로 구성된 gif와 같은 이미지일 경우 프레임 이동
int CSCD2Image::goto_frame(int index, bool pause)
{
	if (index >= m_img.size())
		index = 0;

	m_frame_index = index;
	m_img_origin_for_back_transparency.Reset();

	if (pause)
	{
		m_ani_paused = true;
		CSCD2ImageMessage msg(this, message_frame_changed, m_frame_index, m_img.size());
		::SendMessage(m_parent, Message_CSCD2Image, (WPARAM)&msg, 0);
	}

	return m_frame_index;
}

void CSCD2Image::step(int interval)
{
	m_ani_paused = true;

	if (m_img.size() <= 1)
		return;

	m_frame_index += interval;

	//범위는 순환처리한다.
	if (m_frame_index < 0)
		m_frame_index = m_img.size() - 1;
	if (m_frame_index >= (int)m_img.size())
		m_frame_index = 0;
	
	m_img_origin_for_back_transparency.Reset();

	CSCD2ImageMessage msg(this, message_frame_changed, m_frame_index, m_img.size());
	::SendMessage(m_parent, Message_CSCD2Image, (WPARAM)&msg, 0);
}

int CSCD2Image::get_frame_delay(int index)
{
	if (index >= m_img.size() || index >= m_frame_delay.size())
		return 0;

	return m_frame_delay[index];
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

	// 스레드 생성 전에 플래그를 먼저 설정하여 중복 생성 방지
	m_run_thread_animation = true;
	m_thread_animation_terminated = false;
	m_ani_paused = false;

	std::thread t(&CSCD2Image::thread_animation, this);
	t.detach();
}

//특정 위치에서 멈추게 할 경우도 있어서 원래는 파라미터로 pos를 받았지만
//pause는 그냥 재생중이든 아니든 그냥 일시정지 명령으로 처리하는 것이 명확하므로 pos는 제거한다.
//특정 위치에서 멈추게 할 경우는 goto_frame()을 이용하면 된다.
void CSCD2Image::pause()
{
	if (m_img.size() <= 1 || !m_run_thread_animation)
		return;
	
	//if (pos >= (int)m_img.size())
	//	pos = 0;

	//default -1이 넘어오면 play()가 호출되고 
	//if (pos < 0)
	//{
	//	play();
	//	return;
	//}

	m_ani_paused = true;
	m_img_origin_for_back_transparency.Reset();
}

bool CSCD2Image::stop()
{
	if (m_img.size() <= 1)
		return true;

	if (m_run_thread_animation)
	{
		m_run_thread_animation = false;
		while (!m_thread_animation_terminated)
			Wait(10);
	}

	return true;
}

void CSCD2Image::thread_animation()
{
	long t0;
	long t1;

	CSCD2ImageMessage msg(this, message_frame_changed, m_frame_index, m_img.size());

	while (m_run_thread_animation)
	{
		t0 = clock();

		msg.frame_index = m_frame_index;
		//TRACE(_T("frame index = %d\n"), msg.frame_index);
		::PostMessage(m_parent, Message_CSCD2Image, (WPARAM)&msg, 0);

		if (!m_run_thread_animation)
			break;

		//프레임이 바뀔때마다 m_img_origin_for_back_transparency는 리셋되어야 한다.
		m_img_origin_for_back_transparency.Reset();

		int delay = 100;
		if (m_frame_delay.size() != 0 && m_frame_index < m_frame_delay.size())
			delay = m_frame_delay[m_frame_index];// -elapsed;
		std::this_thread::sleep_for(std::chrono::milliseconds(MAX(1, delay)));

		while (m_ani_paused)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(33));

			if (!m_run_thread_animation)
			{
				m_run_thread_animation = false;
				m_thread_animation_terminated = true;
				TRACE(_T("%s terminated.\n"), __function__);
				return;
			}
		}

		m_frame_index++;
		if (m_frame_index >= (int)m_img.size())
			m_frame_index = 0;
	}

	m_run_thread_animation = false;
	m_thread_animation_terminated = true;
	TRACE(_T("%s terminated.\n"), __function__);
}

//data멤버에 픽셀 데이터를 가리키도록 한다.
void CSCD2Image::get_raw_data()
{
	/*
	if (m_data)
		delete[] m_data;

	data = new uint8_t[(int)(get_width() * get_height() * 4)];

	WICPixelFormatGUID format = GUID_WICPixelFormat32bppBGRA;
	WICRect rcLock = { 0, 0, (INT)get_width(), (INT)get_height() };
	IWICBitmapLock* pLock = NULL;

	HRESULT hr = S_OK;
	*/
}

Gdiplus::Color CSCD2Image::get_pixel(int x, int y)
{
	if (m_data == nullptr)
		return Gdiplus::Color();

	byte r, g, b, a;
	r = g = b = a = 0;
	int width = (int)get_width();
	int height = (int)get_height();

	//실제 이미지는 1, 3, 4채널일 수 있지만 load()에서 무조건 32BitPRGBA 포맷으로 읽고 m_data도 출력하므로 4채널로 계산해야 한다.
	int channel = 4;
	int stride = channel * width;
	b = *(m_data + y * stride + x * channel + 0);
	g = *(m_data + y * stride + x * channel + 1);
	r = *(m_data + y * stride + x * channel + 2);
	a = *(m_data + y * stride + x * channel + 3);

/*
	b = *(m_data + y * m_stride + x * m_channel + 0);
	g = *(m_data + y * m_stride + x * m_channel + 1);
	r = *(m_data + y * m_stride + x * m_channel + 2);

	if (m_channel == 1)
	{
		return Gdiplus::Color();
	}
	else if (m_channel == 3)
	{
		return Gdiplus::Color(255, r, g, b);
	}

	a = *(m_data + y * m_stride + x * m_channel + 3);
*/
	return Gdiplus::Color(a, r, g, b);
}

CString CSCD2Image::get_pixel_format_str(WICPixelFormatGUID *pf, bool simple, bool reset)
{
	if (reset)
		m_pixel_format_str.Empty();

	if (!m_pixel_format_str.IsEmpty())
		return m_pixel_format_str;

	CString str_fmt = _T("Unknown PixelFormat");

	if (pf == NULL)
		return str_fmt;

	m_stride = m_channel = 0;

	if (IsEqualGUID(*pf, GUID_WICPixelFormat32bppBGRA) ||
		IsEqualGUID(*pf, GUID_WICPixelFormat64bppBGRA))
	{
		m_channel = 4;
		str_fmt = _T("GUID_WICPixelFormat32bppBGRA");
	}
	else if (IsEqualGUID(*pf, GUID_WICPixelFormat32bppRGBA) ||
			 IsEqualGUID(*pf, GUID_WICPixelFormat64bppRGBA))
	{
		m_channel = 4;
		str_fmt = _T("GUID_WICPixelFormat32bppRGBA");
	}
	else if (IsEqualGUID(*pf, GUID_WICPixelFormat32bppPRGBA))
	{
		m_channel = 4;
		str_fmt = _T("GUID_WICPixelFormat32bppPBGRA");
	}
	else if (IsEqualGUID(*pf, GUID_WICPixelFormat32bppCMYK))
	{
		m_channel = 4;
		str_fmt = _T("GUID_WICPixelFormat32bppCMYK");
	}
	else if (IsEqualGUID(*pf, GUID_WICPixelFormat32bppBGR))
	{
		m_channel = 3;
		str_fmt = _T("GUID_WICPixelFormat32bppBGR");
	}
	else if (IsEqualGUID(*pf, GUID_WICPixelFormat24bppBGR))
	{
		m_channel = 3;
		str_fmt = _T("GUID_WICPixelFormat24bppBGR");
	}
	else if (IsEqualGUID(*pf, GUID_WICPixelFormat24bppRGB))
	{
		m_channel = 3;
		str_fmt = _T("GUID_WICPixelFormat24bppRGB");
	}
	else if (IsEqualGUID(*pf, GUID_WICPixelFormat8bppGray))
	{
		m_channel = 1;
		str_fmt = _T("GUID_WICPixelFormat8bppGray");
	}
	else if (IsEqualGUID(*pf, GUID_WICPixelFormat8bppIndexed))
	{
		m_channel = 1;
		str_fmt = _T("GUID_WICPixelFormat8bppIndexed");
	}
	else if (IsEqualGUID(*pf, GUID_WICPixelFormat4bppGray))
	{
		m_channel = 1;
		str_fmt = _T("GUID_WICPixelFormat4bppGray");
	}
	else if (IsEqualGUID(*pf, GUID_WICPixelFormat4bppIndexed))
	{
		m_channel = 1;
		str_fmt = _T("GUID_WICPixelFormat4bppIndexed");
	}
	else if (IsEqualGUID(*pf, GUID_WICPixelFormat2bppGray))
	{
		m_channel = 1;
		str_fmt = _T("GUID_WICPixelFormat2bppGray");
	}
	else if (IsEqualGUID(*pf, GUID_WICPixelFormat2bppIndexed))
	{
		m_channel = 1;
		str_fmt = _T("GUID_WICPixelFormat2bppIndexed");
	}
	else if (IsEqualGUID(*pf, GUID_WICPixelFormatBlackWhite))
	{
		m_channel = 1;
		str_fmt = _T("GUID_WICPixelFormatBlackWhite");
	}
	else if (IsEqualGUID(*pf, GUID_WICPixelFormat1bppIndexed))
	{
		m_channel = 1;
		str_fmt = _T("GUID_WICPixelFormat1bppIndexed");
	}
	else
	{
		CString str;
		str.Format(_T("not defiend pixel format. %d"), *pf);
		TRACE(_T("%s\n"), str);
		AfxMessageBox(str);
	}

	if (simple && (str_fmt.Find(_T("Unknown")) < 0))
	{
		str_fmt.Replace(_T("GUID_WICPixelFormat"), _T(""));
		str_fmt.Replace(_T("bpp"), _T(""));
		int bits = 0;
		int bits_pos = get_number_from_string(str_fmt, bits);
		str_fmt.Format(_T("%s %dbit"), str_fmt.Mid(bits_pos), bits);
		//TRACE(_T("str_fmt = %s\n"), str_fmt);
	}

	m_pixel_format_str = str_fmt;
	return m_pixel_format_str;
}

//alpha pixel들의 count를 구한다. (m_alpha_pixel_count < 0 || recount = true)이면 새로 계산한다. 그렇지 않으면 이미 구해놓은 정보를 리턴한다.
//현재는 gif, webp라도 0번 이미지에 대한 m_data를 대상으로 한다.
int CSCD2Image::get_alpha_pixel_count(bool recount)
{
	if (m_alpha_pixel_count >= 0 && !recount)
		return m_alpha_pixel_count;

	if (m_data == nullptr)
		return 0;

	byte a;
	int width = (int)get_width();
	int height = (int)get_height();

	//실제 이미지는 1, 3, 4채널일 수 있지만 load()에서 무조건 32BitPRGBA 포맷으로 읽고 m_data도 추출하므로 4채널로 계산해야 한다.
	int channel = 4;
	int stride = channel * width;

	m_alpha_pixel_count = 0;

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			a = *(m_data + y * stride + x * channel + 3);

			//1ch, 3ch, 4ch 모두 4ch 포맷으로 로딩하므로 설령 1ch, 3ch의 이미지라도
			//a값은 항상 0 ~ 255 사이의 값을 가진다. 255 미만인 픽셀만 카운트하면 된다.
			if (a < 255)
				m_alpha_pixel_count++;
		}
	}

	return m_alpha_pixel_count;
}

//테두리 픽셀들을 이용하여 배경색을 자동 감지한다.
Gdiplus::Color CSCD2Image::detect_back_color(int index)
{
	if (index < 0 || index >= (int)m_img.size())
		index = m_frame_index;

	Gdiplus::Color cr_back(0, 0, 0, 0);

	if (!m_img[index])
		return cr_back;

	D2D1_SIZE_U px = m_img[index]->GetPixelSize();
	UINT width = px.width;
	UINT height = px.height;
	if (width == 0 || height == 0)
		return cr_back;

	// ── 1) GPU → CPU: 읽기 가능한 비트맵 생성 및 복사 ──
	D2D1_BITMAP_PROPERTIES1 cpuProps = D2D1::BitmapProperties1(
		D2D1_BITMAP_OPTIONS_CPU_READ | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

	ComPtr<ID2D1Bitmap1> cpuBitmap;
	HRESULT hr = m_d2dc->CreateBitmap(D2D1::SizeU(width, height), nullptr, 0, &cpuProps, &cpuBitmap);
	if (FAILED(hr))
		return cr_back;

	hr = cpuBitmap->CopyFromBitmap(nullptr, m_img[index].Get(), nullptr);
	if (FAILED(hr))
		return cr_back;

	D2D1_MAPPED_RECT mapped = {};
	hr = cpuBitmap->Map(D2D1_MAP_OPTIONS_READ, &mapped);
	if (FAILED(hr))
		return cr_back;

	// ── 2) 패딩 제거하며 로컬 버퍼로 복사 ──
	UINT stride = width * 4;
	UINT bufSize = stride * height;
	std::vector<BYTE> pixels(bufSize);

	for (UINT y = 0; y < height; ++y)
		memcpy(pixels.data() + y * stride, mapped.bits + y * mapped.pitch, stride);

	cpuBitmap->Unmap();


	// ── 3) 배경색 자동 탐지: n픽셀 두께의 border 영역 전체 샘플링 ──
	// 이미지 외곽 border_size 픽셀 두께의 테두리를 구성하는 모든 픽셀을
	// un-premultiply 후 평균 RGB를 구한다.
	const UINT border_size = max(1, min(4, min(width, height) / 8));
	float sumR = 0.f;
	float sumG = 0.f;
	float sumB = 0.f;
	int sampleCount = 0;

	// 테두리 픽셀인지 판별하는 람다
	// 4개의 strip으로 순회하면 중복 없이 효율적으로 처리할 수 있다.
	// ┌────────────────────┐
	// │  Top strip         │  y: [0, border_size)           x: [0, width)
	// ├──┬──────────┬──┤
	// │L │          │R │  y: [border_size, height-border)  x: [0, border) or [width-border, width)
	// ├──┴──────────┴──┤
	// │  Bottom strip      │  y: [height-border, height)    x: [0, width)
	// └────────────────────┘

	auto sample_pixel = [&](UINT x, UINT y)
		{
			BYTE* p = pixels.data() + y * stride + x * 4;
			BYTE pB = p[0];
			BYTE pG = p[1];
			BYTE pR = p[2];
			BYTE pA = p[3];

			// 완전 투명 픽셀은 배경색 후보에서 제외
			if (pA == 0)
				return;

			// un-premultiply
			float fR, fG, fB;
			if (pA == 255)
			{
				fR = (float)pR;
				fG = (float)pG;
				fB = (float)pB;
			}
			else
			{
				float invA = 255.0f / (float)pA;
				fR = min(255.0f, pR * invA);
				fG = min(255.0f, pG * invA);
				fB = min(255.0f, pB * invA);
			}

			sumR += fR;
			sumG += fG;
			sumB += fB;
			sampleCount++;
		};

	// Top strip: y=[0, border_size), x=[0, width)
	for (UINT y = 0; y < border_size && y < height; ++y)
		for (UINT x = 0; x < width; ++x)
			sample_pixel(x, y);

	// Bottom strip: y=[height-border_size, height), x=[0, width)
	for (UINT y = (height > border_size ? height - border_size : 0); y < height; ++y)
	{
		if (y < border_size) continue;  // top strip과 겹치는 경우 방지 (이미지가 매우 작을 때)
		for (UINT x = 0; x < width; ++x)
			sample_pixel(x, y);
	}

	// Left strip: y=[border_size, height-border_size), x=[0, border_size)
	UINT y_start = border_size;
	UINT y_end = (height > border_size ? height - border_size : 0);
	for (UINT y = y_start; y < y_end; ++y)
		for (UINT x = 0; x < border_size && x < width; ++x)
			sample_pixel(x, y);

	// Right strip: y=[border_size, height-border_size), x=[width-border_size, width)
	for (UINT y = y_start; y < y_end; ++y)
		for (UINT x = (width > border_size ? width - border_size : 0); x < width; ++x)
		{
			if (x < border_size) continue;  // left strip과 겹치는 경우 방지
			sample_pixel(x, y);
		}

	// 샘플이 하나도 없으면 (테두리가 모두 투명) 이 프레임은 건너뛴다
	if (sampleCount == 0)
		return cr_back;

	// ── 샘플 비율 검증: 이미 배경이 제거된 상태인지 판별 ──
	// border 전체 픽셀 수 대비 비투명 샘플 비율이 너무 낮으면
	// 남은 샘플이 전경 콘텐츠일 가능성이 높으므로 건너뛴다.
	UINT totalBorderPixels = (width * border_size * 2) +
		((height > border_size * 2 ? height - border_size * 2 : 0) * border_size * 2);
	float sampleRatio = (totalBorderPixels > 0) ? (float)sampleCount / (float)totalBorderPixels : 0.f;

	if (sampleRatio < 0.2f)
	{
		TRACE(_T("make_back_transparent: frame %d skipped - border already mostly transparent (ratio=%.1f%%, samples=%d/%d)\n"),
			index, sampleRatio * 100.f, sampleCount, totalBorderPixels);
		return cr_back;
	}

	const float bgR = sumR / sampleCount;
	const float bgG = sumG / sampleCount;
	const float bgB = sumB / sampleCount;

	TRACE(_T("make_back_transparent: frame %d, detected bg = RGB(%.0f, %.0f, %.0f) from %d border samples (border=%d)\n"), index, bgR, bgG, bgB, sampleCount, border_size);

	cr_back = Gdiplus::Color(255, (BYTE)bgR, (BYTE)bgG, (BYTE)bgB);

	return cr_back;
}

//index 위치의 이미지에서 배경 색상을 transparent 색상으로 변경한다.
//내부적으로는 make_back_transparent()를 호출하지만
//set_back_transparency()는 현재 이미지의 원본을 보존하면서 투명하게 만들고자 하는 경우에 사용한다. make_back_transparent()는 현재 이미지 자체를 변경한다.
void CSCD2Image::set_back_transparency(int index, int inner_threshold, int outer_threshold, Gdiplus::Color cr_back)
{
	//투명 처리 파라미터를 변경해가면서 이미지의 변화를 비교하려면 원본 백업 및 재사용이 필요하다.
	//단, index = -1로 넘어왔다면 모든 프레임에 대해 일괄 적용이므로 백업이 의미없다.
	if (index >= 0)
	{
		if (m_img_origin_for_back_transparency == nullptr)
		{
			D2D1_BITMAP_PROPERTIES1 properties = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
			get_d2dc()->CreateBitmap(D2D1::SizeU(get_width(), get_height()), nullptr, 0, properties, m_img_origin_for_back_transparency.GetAddressOf());
			copy(index, m_img_origin_for_back_transparency.Get());
		}
		else
		{
			m_img[index].Get()->CopyFromBitmap(nullptr, m_img_origin_for_back_transparency.Get(), nullptr);
		}
	}

	make_back_transparent(index, inner_threshold, outer_threshold, cr_back);
}


//배경 색상을 자동 탐지하여 transparent 색상으로 변경한다.
//cr_back == Gdiplus::Color::Transparent인 경우는 border에서 일정 두께만큼의 픽셀들을 읽어서 배경색을 자동 탐지하게 되고
//cr_back != Gdiplus::Color::Transparent이 아니면 해당 색을 투명처리하게 된다.
//index = -1이면 모든 프레임에 대해 적용한다.
//inner_threshold: 이 거리 이내의 픽셀은 완전 투명 (기본값 30)
//outer_threshold: 이 거리 이상의 픽셀은 원본 유지 (기본값 120)
void CSCD2Image::make_back_transparent(int index, int inner_threshold, int outer_threshold, Gdiplus::Color cr_back)
{
	if (!m_d2dc || m_img.empty())
		return;

	// threshold 유효성 보정
	if (inner_threshold < 0) inner_threshold = 0;
	if (outer_threshold <= inner_threshold) outer_threshold = inner_threshold + 1;

	int from = index;
	int to = index;

	if (index < 0)
	{
		from = 0;
		to = get_frame_count() - 1;
	}

	for (int i = from; i <= to; i++)
	{
		if (!m_img[i])
			continue;

		D2D1_SIZE_U px = m_img[i]->GetPixelSize();
		UINT width = px.width;
		UINT height = px.height;
		if (width == 0 || height == 0)
			continue;

		// ── 1) GPU → CPU: 읽기 가능한 비트맵 생성 및 복사 ──
		D2D1_BITMAP_PROPERTIES1 cpuProps = D2D1::BitmapProperties1(
			D2D1_BITMAP_OPTIONS_CPU_READ | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
			D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

		ComPtr<ID2D1Bitmap1> cpuBitmap;
		HRESULT hr = m_d2dc->CreateBitmap(D2D1::SizeU(width, height), nullptr, 0, &cpuProps, &cpuBitmap);
		if (FAILED(hr))
			continue;

		hr = cpuBitmap->CopyFromBitmap(nullptr, m_img[i].Get(), nullptr);
		if (FAILED(hr))
			continue;

		D2D1_MAPPED_RECT mapped = {};
		hr = cpuBitmap->Map(D2D1_MAP_OPTIONS_READ, &mapped);
		if (FAILED(hr))
			continue;

		// ── 2) 패딩 제거하며 로컬 버퍼로 복사 ──
		UINT stride = width * 4;
		UINT bufSize = stride * height;
		std::vector<BYTE> pixels(bufSize);

		for (UINT y = 0; y < height; ++y)
			memcpy(pixels.data() + y * stride, mapped.bits + y * mapped.pitch, stride);

		cpuBitmap->Unmap();


		// ── 3) 배경색 자동 탐지: n픽셀 두께의 border 영역 전체 샘플링 ──
		// 이미지 외곽 border_size 픽셀 두께의 테두리를 구성하는 모든 픽셀을
		// un-premultiply 후 평균 RGB를 구한다.
		const UINT border_size = max(1, min(4, min(width, height) / 8));
		float sumR = 0.f;
		float sumG = 0.f;
		float sumB = 0.f;
		int sampleCount = 0;

		// 테두리 픽셀인지 판별하는 람다
		// 4개의 strip으로 순회하면 중복 없이 효율적으로 처리할 수 있다.
		// ┌────────────────────┐
		// │  Top strip         │  y: [0, border_size)           x: [0, width)
		// ├──┬──────────┬──┤
		// │L │          │R │  y: [border_size, height-border)  x: [0, border) or [width-border, width)
		// ├──┴──────────┴──┤
		// │  Bottom strip      │  y: [height-border, height)    x: [0, width)
		// └────────────────────┘

		auto sample_pixel = [&](UINT x, UINT y)
			{
				BYTE* p = pixels.data() + y * stride + x * 4;
				BYTE pB = p[0];
				BYTE pG = p[1];
				BYTE pR = p[2];
				BYTE pA = p[3];

				// 완전 투명 픽셀은 배경색 후보에서 제외
				if (pA == 0)
					return;

				// un-premultiply
				float fR, fG, fB;
				if (pA == 255)
				{
					fR = (float)pR;
					fG = (float)pG;
					fB = (float)pB;
				}
				else
				{
					float invA = 255.0f / (float)pA;
					fR = min(255.0f, pR * invA);
					fG = min(255.0f, pG * invA);
					fB = min(255.0f, pB * invA);
				}

				sumR += fR;
				sumG += fG;
				sumB += fB;
				sampleCount++;
			};

		// Top strip: y=[0, border_size), x=[0, width)
		for (UINT y = 0; y < border_size && y < height; ++y)
			for (UINT x = 0; x < width; ++x)
				sample_pixel(x, y);

		// Bottom strip: y=[height-border_size, height), x=[0, width)
		for (UINT y = (height > border_size ? height - border_size : 0); y < height; ++y)
		{
			if (y < border_size) continue;  // top strip과 겹치는 경우 방지 (이미지가 매우 작을 때)
			for (UINT x = 0; x < width; ++x)
				sample_pixel(x, y);
		}

		// Left strip: y=[border_size, height-border_size), x=[0, border_size)
		UINT y_start = border_size;
		UINT y_end = (height > border_size ? height - border_size : 0);
		for (UINT y = y_start; y < y_end; ++y)
			for (UINT x = 0; x < border_size && x < width; ++x)
				sample_pixel(x, y);

		// Right strip: y=[border_size, height-border_size), x=[width-border_size, width)
		for (UINT y = y_start; y < y_end; ++y)
			for (UINT x = (width > border_size ? width - border_size : 0); x < width; ++x)
			{
				if (x < border_size) continue;  // left strip과 겹치는 경우 방지
				sample_pixel(x, y);
			}

		// 샘플이 하나도 없으면 (테두리가 모두 투명) 이 프레임은 건너뛴다
		if (sampleCount == 0)
			continue;

		// ── 샘플 비율 검증: 이미 배경이 제거된 상태인지 판별 ──
		// border 전체 픽셀 수 대비 비투명 샘플 비율이 너무 낮으면
		// 남은 샘플이 전경 콘텐츠일 가능성이 높으므로 건너뛴다.
		UINT totalBorderPixels = (width * border_size * 2) +
			((height > border_size * 2 ? height - border_size * 2 : 0) * border_size * 2);
		float sampleRatio = (totalBorderPixels > 0) ? (float)sampleCount / (float)totalBorderPixels : 0.f;

		if (sampleRatio < 0.2f)
		{
			TRACE(_T("make_back_transparent: frame %d skipped - border already mostly transparent (ratio=%.1f%%, samples=%d/%d)\n"),
				i, sampleRatio * 100.f, sampleCount, totalBorderPixels);
			continue;
		}

		float bgR = sumR / sampleCount;
		float bgG = sumG / sampleCount;
		float bgB = sumB / sampleCount;

		TRACE(_T("make_back_transparent: frame %d, detected bg = RGB(%.0f, %.0f, %.0f) from %d border samples (border=%d)\n"), i, bgR, bgG, bgB, sampleCount, border_size);

		if (cr_back.GetValue() != Gdiplus::Color::Transparent)
		{
			bgR = (float)cr_back.GetR();
			bgG = (float)cr_back.GetG();
			bgB = (float)cr_back.GetB();
			TRACE(_T("use specified cr_back = RGB(%.0f, %.0f, %.0f)\n"), i, bgR, bgG, bgB);
		}

		// ── 4) 소프트 키 처리 (PBGRA → un-premultiply → 거리 계산 → 알파 수정 → re-premultiply) ──
		for (UINT y = 0; y < height; ++y)
		{
			BYTE* row = pixels.data() + y * stride;
			BYTE* data_row = m_data + y * stride;

			for (UINT x = 0; x < width; ++x)
			{
				BYTE* px = row + x * 4;
				BYTE* data_px = data_row + x * 4;

				// PBGRA 레이아웃: [B, G, R, A]
				BYTE pB = px[0];
				BYTE pG = px[1];
				BYTE pR = px[2];
				BYTE pA = px[3];

				// 완전 투명 픽셀은 건너뛰기
				if (pA == 0)
					continue;

				// un-premultiply: straight BGRA 복원
				float fR, fG, fB;
				if (pA == 255)
				{
					fR = (float)pR;
					fG = (float)pG;
					fB = (float)pB;
				}
				else
				{
					float invA = 255.0f / (float)pA;
					fR = min(255.0f, pR * invA);
					fG = min(255.0f, pG * invA);
					fB = min(255.0f, pB * invA);
				}

				// 배경색과의 유클리드 거리 계산
				float dR = fR - bgR;
				float dG = fG - bgG;
				float dB = fB - bgB;
				float distance = sqrtf(dR * dR + dG * dG + dB * dB);

				// 소프트 키: 거리에 따른 새 알파 결정
				float newAlphaF;
				if (distance <= (float)inner_threshold)
				{
					// 배경색과 매우 가까움 → 완전 투명
					newAlphaF = 0.0f;
				}
				else if (distance >= (float)outer_threshold)
				{
					// 배경색과 충분히 다름 → 원본 알파 유지
					newAlphaF = (float)pA;
				}
				else
				{
					// 전환 구간: 선형 보간
					float t = (distance - (float)inner_threshold) / (float)(outer_threshold - inner_threshold);
					newAlphaF = (float)pA * t;
				}

				BYTE newA = (BYTE)(newAlphaF + 0.5f);

				// re-premultiply: D2D가 요구하는 PBGRA 형식으로 복원
				if (newA == 0)
				{
					px[0] = px[1] = px[2] = px[3] = 0;
					data_px[0] = data_px[1] = data_px[2] = data_px[3] = 0;
				}
				else
				{
					float scale = (float)newA / 255.0f;
					px[0] = (BYTE)(fB * scale + 0.5f);  // B
					px[1] = (BYTE)(fG * scale + 0.5f);  // G
					px[2] = (BYTE)(fR * scale + 0.5f);  // R
					px[3] = newA;
					data_px[0] = (BYTE)fB;
					data_px[1] = (BYTE)fG;
					data_px[2] = (BYTE)fR;
					data_px[3] = px[3];
				}
			}
		}

		// ── 5) CPU → GPU: 수정된 픽셀을 D2D 비트맵에 다시 쓰기 ──
		D2D1_RECT_U destRect = D2D1::RectU(0, 0, width, height);
		hr = m_img[i]->CopyFromMemory(&destRect, pixels.data(), stride);

		if (FAILED(hr))
		{
			TRACE(_T("make_transparent: CopyFromMemory failed frame %d, hr=0x%08X\n"), i, hr);
		}
		else
		{
			TRACE(_T("make_transparent done. frame %d\n"), i);
		}
	}

	// 알파 픽셀 수 변경되었으므로 재계산 플래그 설정을 하고 다시 계산하도록 해야 하지만
	//실제 알파 픽셀의 정확한 갯수가 필요한게 아니고 알파 픽셀을 포함하는지를 판별하는 용도이므로
	//0보다 큰 값을 지정한다.
	//또는 위에서 계산 시 a < 255인 픽셀의 갯수를 센 후 여기 할당해줘도 된다.
	m_alpha_pixel_count = 1;
}

bool ConvertPBGRAtoBGRA(
	IWICImagingFactory* factory,
	IWICBitmap* src,
	IWICBitmap** outBgraBitmap
)
{
	UINT w = 0, h = 0;
	src->GetSize(&w, &h);

	ComPtr<IWICFormatConverter> conv;
	if (FAILED(factory->CreateFormatConverter(&conv)))
		return false;

	if (FAILED(conv->Initialize(
		src,
		GUID_WICPixelFormat32bppBGRA,
		WICBitmapDitherTypeNone,
		nullptr,
		0.0,
		WICBitmapPaletteTypeCustom)))
		return false;

	if (FAILED(factory->CreateBitmap(
		w, h,
		GUID_WICPixelFormat32bppBGRA,
		WICBitmapCacheOnLoad,
		outBgraBitmap)))
		return false;

	WICRect rc = { 0, 0, (INT)w, (INT)h };
	std::vector<BYTE> buffer(w * h * 4);

	if (FAILED(conv->CopyPixels(&rc, w * 4, (UINT)buffer.size(), buffer.data())))
		return false;

	(*outBgraBitmap)->CopyPixels(&rc, w * 4, (UINT)buffer.size(), buffer.data());

	return true;
}

bool CSCD2Image::copy_to_clipboard(int index)
{
	if (index < 0 || index >= get_frame_count())
		index = m_frame_index;

	if (!m_img[index] || !m_d2dc || !m_pWICFactory)
		return false;

	// 1) 비트맵 크기 (GetSurface 대신 직접 취득)
	D2D1_SIZE_U px = m_img[index]->GetPixelSize();
	const UINT width = px.width;
	const UINT height = px.height;
	if (width == 0 || height == 0)
		return false;

	// 2) CPU 읽기 가능한 비트맵 생성
	D2D1_BITMAP_PROPERTIES1 cpuProps = D2D1::BitmapProperties1(
		D2D1_BITMAP_OPTIONS_CPU_READ | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

	ComPtr<ID2D1Bitmap1> cpuBitmap;
	HRESULT hr = m_d2dc->CreateBitmap(D2D1::SizeU(width, height), nullptr, 0, &cpuProps, &cpuBitmap);
	if (FAILED(hr))
		return false;

	// 3) GPU → CPU 복사
	hr = cpuBitmap->CopyFromBitmap(nullptr, m_img[index].Get(), nullptr);
	if (FAILED(hr))
		return false;

	// 4) Map
	D2D1_MAPPED_RECT mapped = {};
	hr = cpuBitmap->Map(D2D1_MAP_OPTIONS_READ, &mapped);
	if (FAILED(hr))
		return false;

	const UINT stride = width * 4;
	const UINT imageSize = stride * height;

	// 5) Premultiplied Alpha → Straight Alpha 변환 (BGRA 채널 순서 유지)
	std::vector<BYTE> pixels(imageSize);
	for (UINT y = 0; y < height; ++y)
	{
		memcpy(pixels.data() + y * stride, mapped.bits + y * mapped.pitch, stride);
	}
	convert_PBGRA_to_BGRA(pixels.data(), width, height, stride);

	cpuBitmap->Unmap();

	// 6) CF_DIBV5 생성
	const UINT totalSize = sizeof(BITMAPV5HEADER) + imageSize;
	HGLOBAL hDibv5 = GlobalAlloc(GHND, totalSize);
	if (!hDibv5)
		return false;

	{
		BYTE* pData = (BYTE*)GlobalLock(hDibv5);
		BITMAPV5HEADER* pbv5 = (BITMAPV5HEADER*)pData;
		ZeroMemory(pbv5, sizeof(*pbv5));

		pbv5->bV5Size = sizeof(BITMAPV5HEADER);
		pbv5->bV5Width = width;
		pbv5->bV5Height = -(LONG)height;   // top-down
		pbv5->bV5Planes = 1;
		pbv5->bV5BitCount = 32;
		pbv5->bV5Compression = BI_BITFIELDS;
		pbv5->bV5SizeImage = imageSize;
		pbv5->bV5RedMask = 0x00FF0000;
		pbv5->bV5GreenMask = 0x0000FF00;
		pbv5->bV5BlueMask = 0x000000FF;
		pbv5->bV5AlphaMask = 0xFF000000;
		pbv5->bV5CSType = LCS_sRGB;
		pbv5->bV5Intent = LCS_GM_IMAGES;

		memcpy(pData + sizeof(BITMAPV5HEADER), pixels.data(), imageSize);
		GlobalUnlock(hDibv5);
	}

	// 7) PNG 생성 (alpha 보존의 핵심 ? 많은 앱이 CF_DIBV5 alpha를 무시함)
	HGLOBAL hPng = nullptr;
	{
		ComPtr<IWICBitmap> wicBmp;
		hr = m_pWICFactory->CreateBitmapFromMemory(
			width, height,
			GUID_WICPixelFormat32bppBGRA,
			stride, imageSize,
			pixels.data(),
			&wicBmp);

		if (SUCCEEDED(hr))
		{
			ComPtr<IStream> stream;
			CreateStreamOnHGlobal(nullptr, TRUE, &stream);

			ComPtr<IWICBitmapEncoder> encoder;
			hr = m_pWICFactory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder);

			if (SUCCEEDED(hr))
			{
				encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache);

				ComPtr<IWICBitmapFrameEncode> frame;
				encoder->CreateNewFrame(&frame, nullptr);
				frame->Initialize(nullptr);
				frame->SetSize(width, height);

				WICPixelFormatGUID fmt = GUID_WICPixelFormat32bppBGRA;
				frame->SetPixelFormat(&fmt);
				frame->WriteSource(wicBmp.Get(), nullptr);
				frame->Commit();
				encoder->Commit();

				// 스트림에서 PNG 데이터를 별도 HGLOBAL에 복사
				HGLOBAL hStreamMem = nullptr;
				GetHGlobalFromStream(stream.Get(), &hStreamMem);

				if (hStreamMem)
				{
					STATSTG st = {};
					stream->Stat(&st, STATFLAG_NONAME);
					SIZE_T pngSize = (SIZE_T)st.cbSize.QuadPart;

					if (pngSize > 0)
					{
						hPng = GlobalAlloc(GMEM_MOVEABLE, pngSize);
						if (hPng)
						{
							void* pSrc = GlobalLock(hStreamMem);
							void* pDst = GlobalLock(hPng);
							memcpy(pDst, pSrc, pngSize);
							GlobalUnlock(hPng);
							GlobalUnlock(hStreamMem);
						}
					}
				}
			}
		}
		// stream 소멸 시 fDeleteOnRelease=TRUE이므로 hStreamMem 자동 해제
	}

	// 8) 클립보드 등록
	if (!OpenClipboard(nullptr))
	{
		GlobalFree(hDibv5);
		if (hPng) GlobalFree(hPng);
		return false;
	}

	EmptyClipboard();

	// CF_DIBV5 등록
	if (!SetClipboardData(CF_DIBV5, hDibv5))
	{
		GlobalFree(hDibv5);
		if (hPng) GlobalFree(hPng);
		CloseClipboard();
		return false;
	}

	// PNG 등록 (Photoshop, Chrome 등에서 alpha 인식)
	if (hPng)
	{
		static UINT cfPng = RegisterClipboardFormat(_T("PNG"));
		if (!SetClipboardData(cfPng, hPng))
			GlobalFree(hPng);
	}

	CloseClipboard();
	return true;
#if 0
	if (index < 0 || index >= get_frame_count())
		index = m_frame_index;

	if (!m_img[index])
		return false;

	//20260203 GPT에 물어보며 아래와 같은 방법으로 구현해봤으나 계속 검은 화면만 클립보드로 복사된다.
	//CSCGdiplusBitmap으로 임시 폴더에 저장한 후 그 파일을 다시 열어서 클립보드에 복사하는 방법으로 우선 구현한다.
	CString temp_file;
	
	temp_file.Format(_T("%s\\__asee_temp__.png"), get_known_folder(CSIDL_APPDATA));
	HRESULT hr = save(m_img[index].Get(), 1.0f, temp_file);

	if (FAILED(hr))
		return false;

	CSCGdiplusBitmap img(temp_file);
	return img.copy_to_clipboard();
#endif

#if 0
	// 0) 크기 정보
	ComPtr<IDXGISurface> surface;
	if (FAILED(m_img[m_frame_index]->GetSurface(&surface)))
		return false;

	DXGI_SURFACE_DESC desc = {};
	surface->GetDesc(&desc);

	const UINT width = desc.Width;
	const UINT height = desc.Height;

	// 1) CPU 읽기 가능한 D2D bitmap 생성
	D2D1_BITMAP_PROPERTIES1 cpuProps = {};
	cpuProps.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
	cpuProps.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
	cpuProps.bitmapOptions =
		D2D1_BITMAP_OPTIONS_CPU_READ |
		D2D1_BITMAP_OPTIONS_CANNOT_DRAW;

	ComPtr<ID2D1Bitmap1> cpuBitmap;
	if (FAILED(m_d2dc->CreateBitmap(
		D2D1::SizeU(width, height),
		nullptr,
		0,
		&cpuProps,
		&cpuBitmap)))
		return false;

	// 2) GPU → CPU 복사 (필수)
	if (FAILED(cpuBitmap->CopyFromBitmap(
		nullptr,
		m_img[m_frame_index].Get(),
		nullptr)))
		return false;

	// 3) Map 해서 실제 픽셀 얻기
	D2D1_MAPPED_RECT mapped = {};
	if (FAILED(cpuBitmap->Map(D2D1_MAP_OPTIONS_READ, &mapped)))
		return false;

	for (int y = 20; y < 40; y++)
	{
		BYTE* row = mapped.bits + y * 2427;

		for (int x = 20; x < 80; x++)
		{
			BYTE* px = row + x * 4;

			BYTE b = px[0];
			BYTE g = px[1];
			BYTE r = px[2];
			BYTE a = px[3];

			TRACE(_T("%d, %d = %d, %d, %d, %d\n"),
				x, y, a, r, g, b);
		}
	}


	// 4) WICBitmap(PBGRA) 생성
	ComPtr<IWICBitmap> wicPbgra;
	if (FAILED(m_pWICFactory->CreateBitmap(
		width,
		height,
		GUID_WICPixelFormat32bppPBGRA,
		WICBitmapCacheOnLoad,
		&wicPbgra)))
	{
		cpuBitmap->Unmap();
		return false;
	}

	// 5) WICBitmap에 "쓰기" (중요)
	{
		WICRect rc = { 0, 0, (INT)width, (INT)height };
		ComPtr<IWICBitmapLock> wicLock;

		if (FAILED(wicPbgra->Lock(&rc, WICBitmapLockWrite, &wicLock)))
		{
			cpuBitmap->Unmap();
			return false;
		}

		BYTE* wicBits = nullptr;
		UINT  wicStride = 0;
		UINT  wicSize = 0;

		wicLock->GetDataPointer(&wicSize, &wicBits);
		wicLock->GetStride(&wicStride);

		for (UINT y = 0; y < height; ++y)
		{
			memcpy(
				wicBits + y * wicStride,
				mapped.bits + y * mapped.pitch,
				width * 4
			);
		}
	}

	cpuBitmap->Unmap();

	// 6) PBGRA → BGRA (Straight Alpha)
	ComPtr<IWICBitmap> wicBgra;
	if (!ConvertPBGRAtoBGRA(
		m_pWICFactory,
		wicPbgra.Get(),
		&wicBgra))
		return false;

	// 7) BGRA Lock
	WICRect rc = { 0, 0, (INT)width, (INT)height };
	ComPtr<IWICBitmapLock> lock;

	if (FAILED(wicBgra->Lock(&rc, WICBitmapLockRead, &lock)))
		return false;

	BYTE* pixels = nullptr;
	UINT  stride = 0;
	UINT  bufSize = 0;

	lock->GetDataPointer(&bufSize, &pixels);
	lock->GetStride(&stride);
	/*
	for (int y = 20; y < 40; y++)
	{
		BYTE* row = pixels + y * stride;

		for (int x = 20; x < 80; x++)
		{
			BYTE* px = row + x * 4;

			BYTE b = px[0];
			BYTE g = px[1];
			BYTE r = px[2];
			BYTE a = px[3];

			TRACE(_T("%d, %d = %d, %d, %d, %d\n"),
				x, y, a, r, g, b);
		}
	}
	*/

	// 8) CF_DIBV5 메모리 구성
	const UINT imageSize = height * stride;
	const UINT totalSize = sizeof(BITMAPV5HEADER) + imageSize;

	HGLOBAL hMem = GlobalAlloc(GHND, totalSize);
	if (!hMem)
		return false;

	BYTE* pData = (BYTE*)GlobalLock(hMem);
	if (!pData)
	{
		GlobalFree(hMem);
		return false;
	}

	BITMAPV5HEADER* pbv5 = (BITMAPV5HEADER*)pData;
	ZeroMemory(pbv5, sizeof(*pbv5));

	pbv5->bV5Size = sizeof(*pbv5);
	pbv5->bV5Width = width;
	pbv5->bV5Height = -((LONG)height); // top-down
	pbv5->bV5Planes = 1;
	pbv5->bV5BitCount = 32;
	pbv5->bV5Compression = BI_BITFIELDS;
	pbv5->bV5RedMask = 0x00FF0000;
	pbv5->bV5GreenMask = 0x0000FF00;
	pbv5->bV5BlueMask = 0x000000FF;
	pbv5->bV5AlphaMask = 0xFF000000;
	pbv5->bV5CSType = LCS_sRGB;

	memcpy(
		pData + sizeof(BITMAPV5HEADER),
		pixels,
		imageSize
	);

	GlobalUnlock(hMem);

	// 9) 클립보드 등록
	if (!OpenClipboard(nullptr))
	{
		GlobalFree(hMem);
		return false;
	}

	EmptyClipboard();
	SetClipboardData(CF_DIBV5, hMem); // 소유권 시스템 이전
	CloseClipboard();

	return true;
#endif
}

//클립보드에 이미지가 있다면 클립보드의 이미지를 index 위치에 붙여넣는다. 0보다 작으면 현재 프레임 이미지에 붙여넣는다.
bool CSCD2Image::paste_from_clipboard(int index)
{
	if (!m_d2dc || !m_pWICFactory)
		return false;

	if (index < 0)
		index = m_frame_index;

	if (index < 0 || index >= (int)m_img.size() || !m_img[index])
		return false;

	if (!OpenClipboard(nullptr))
		return false;

	ComPtr<IWICBitmapSource> wicSource;
	HRESULT hr = E_FAIL;

	// 1) PNG 포맷 시도 (alpha 보존)
	static UINT cfPng = RegisterClipboardFormat(_T("PNG"));
	HANDLE hPng = GetClipboardData(cfPng);
	if (hPng)
	{
		SIZE_T size = GlobalSize(hPng);
		void* pData = GlobalLock(hPng);
		if (pData && size > 0)
		{
			ComPtr<IWICStream> stream;
			hr = m_pWICFactory->CreateStream(&stream);
			if (SUCCEEDED(hr))
				hr = stream->InitializeFromMemory(reinterpret_cast<BYTE*>(pData), static_cast<DWORD>(size));

			ComPtr<IWICBitmapDecoder> decoder;
			if (SUCCEEDED(hr))
				hr = m_pWICFactory->CreateDecoderFromStream(stream.Get(), nullptr, WICDecodeMetadataCacheOnLoad, &decoder);

			ComPtr<IWICBitmapFrameDecode> frame;
			if (SUCCEEDED(hr))
				hr = decoder->GetFrame(0, &frame);

			if (SUCCEEDED(hr))
				wicSource = frame;

			GlobalUnlock(hPng);
		}
	}

	// 2) CF_DIBV5 시도
	if (!wicSource)
	{
		HANDLE hDibv5 = GetClipboardData(CF_DIBV5);
		if (hDibv5)
		{
			SIZE_T size = GlobalSize(hDibv5);
			BYTE* pData = (BYTE*)GlobalLock(hDibv5);
			if (pData && size > sizeof(BITMAPV5HEADER))
			{
				BITMAPV5HEADER* pbv5 = (BITMAPV5HEADER*)pData;
				UINT w = abs((int)pbv5->bV5Width);
				UINT h = abs((int)pbv5->bV5Height);
				BYTE* pixels = pData + pbv5->bV5Size;
				UINT stride = w * 4;

				// top-down이면 그대로, bottom-up이면 뒤집기
				std::vector<BYTE> flipped;
				if (pbv5->bV5Height > 0)
				{
					flipped.resize(stride * h);
					for (UINT y = 0; y < h; y++)
						memcpy(flipped.data() + y * stride, pixels + (h - 1 - y) * stride, stride);
					pixels = flipped.data();
				}

				ComPtr<IWICBitmap> wicBmp;
				hr = m_pWICFactory->CreateBitmapFromMemory(w, h, GUID_WICPixelFormat32bppBGRA, stride, stride * h, pixels, &wicBmp);
				if (SUCCEEDED(hr))
					wicSource = wicBmp;
			}
			GlobalUnlock(hDibv5);
		}
	}

	// 3) CF_DIB 시도 (alpha 없음)
	if (!wicSource)
	{
		HANDLE hDib = GetClipboardData(CF_DIB);
		if (hDib)
		{
			SIZE_T size = GlobalSize(hDib);
			BYTE* pData = (BYTE*)GlobalLock(hDib);
			if (pData && size > sizeof(BITMAPINFOHEADER))
			{
				BITMAPINFOHEADER* pbi = (BITMAPINFOHEADER*)pData;
				UINT w = abs((int)pbi->biWidth);
				UINT h = abs((int)pbi->biHeight);
				UINT bpp = pbi->biBitCount;
				UINT srcStride = ((w * bpp + 31) / 32) * 4;
				BYTE* pixels = pData + pbi->biSize;

				// 팔레트가 있으면 건너뛰기
				if (bpp <= 8)
				{
					UINT paletteSize = pbi->biClrUsed ? pbi->biClrUsed : (1u << bpp);
					pixels += paletteSize * sizeof(RGBQUAD);
				}

				// WIC 비트맵으로 변환
				WICPixelFormatGUID srcFormat;
				if (bpp == 32)      srcFormat = GUID_WICPixelFormat32bppBGRA;
				else if (bpp == 24) srcFormat = GUID_WICPixelFormat24bppBGR;
				else
				{
					GlobalUnlock(hDib);
					CloseClipboard();
					return false;
				}

				// bottom-up → top-down 변환
				std::vector<BYTE> flipped(srcStride * h);
				if (pbi->biHeight > 0)
				{
					for (UINT y = 0; y < h; y++)
						memcpy(flipped.data() + y * srcStride, pixels + (h - 1 - y) * srcStride, srcStride);
				}
				else
				{
					memcpy(flipped.data(), pixels, srcStride * h);
				}

				ComPtr<IWICBitmap> wicBmp;
				hr = m_pWICFactory->CreateBitmapFromMemory(w, h, srcFormat, srcStride, srcStride * h, flipped.data(), &wicBmp);
				if (SUCCEEDED(hr))
					wicSource = wicBmp;
			}
			GlobalUnlock(hDib);
		}
	}

	CloseClipboard();

	if (!wicSource)
		return false;

	// PBGRA로 변환
	ComPtr<IWICFormatConverter> converter;
	hr = m_pWICFactory->CreateFormatConverter(&converter);
	if (FAILED(hr))
		return false;

	hr = converter->Initialize(wicSource.Get(), GUID_WICPixelFormat32bppPBGRA,
		WICBitmapDitherTypeNone, nullptr, 0.f, WICBitmapPaletteTypeMedianCut);
	if (FAILED(hr))
		return false;

	// D2D 비트맵 생성
	ComPtr<ID2D1Bitmap1> srcBitmap;
	hr = m_d2dc->CreateBitmapFromWicBitmap(converter.Get(), nullptr, &srcBitmap);
	if (FAILED(hr) || !srcBitmap)
		return false;

	// 대상 프레임에 복사 (크기가 다를 수 있으므로 DrawBitmap으로 리사이즈)
	D2D1_SIZE_U dstSize = m_img[index]->GetPixelSize();
	D2D1_SIZE_F srcSize = srcBitmap->GetSize();

	ComPtr<ID2D1Image> prevTarget;
	m_d2dc->GetTarget(&prevTarget);

	m_d2dc->SetTarget(m_img[index].Get());
	m_d2dc->BeginDraw();
	m_d2dc->Clear(D2D1::ColorF(0, 0, 0, 0));

	D2D1_RECT_F dstRect = get_ratio_rect(
		D2D1::RectF(0, 0, (float)dstSize.width, (float)dstSize.height),
		srcSize.width, srcSize.height);

	m_d2dc->DrawBitmap(srcBitmap.Get(), dstRect);
	hr = m_d2dc->EndDraw();

	m_d2dc->SetTarget(prevTarget.Get());

	return SUCCEEDED(hr);
}

WICBitmapTransformOptions CSCD2Image::ExifOrientationToWicTransform(USHORT orientation)
{
	switch (orientation)
	{
	case 2: return WICBitmapTransformFlipHorizontal;
	case 3: return WICBitmapTransformRotate180;
	case 4: return WICBitmapTransformFlipVertical;
	case 5: return (WICBitmapTransformOptions)(WICBitmapTransformRotate270 | WICBitmapTransformFlipHorizontal); // transpose 계열
	case 6: return WICBitmapTransformRotate90;
	case 7: return (WICBitmapTransformOptions)(WICBitmapTransformRotate90 | WICBitmapTransformFlipHorizontal);
	case 8: return WICBitmapTransformRotate270;
	case 1:
	default: return WICBitmapTransformRotate0;
	}
}

bool CSCD2Image::read_webp_frame_delay(const CString& path)
{
	ULONGLONG file_size = get_file_size(path);
	if (file_size == 0)
		return false;

	std::vector<uint8_t> data(file_size);
	read_raw(path, data.data(), file_size);

	WebPData webp_data;
	webp_data.bytes = data.data();
	webp_data.size = data.size();

	WebPDemuxer* demux = WebPDemux(&webp_data);
	if (!demux)
		return false;

	uint32_t frame_count = WebPDemuxGetI(demux, WEBP_FF_FRAME_COUNT);
	if (frame_count <= 1)
	{
		WebPDemuxDelete(demux);
		return false;
	}

	m_frame_delay.clear();

	WebPIterator iter;
	if (WebPDemuxGetFrame(demux, 1, &iter))  // 1-based index
	{
		do {
			m_frame_delay.push_back(iter.duration);  // 이미 밀리초 단위
		} while (WebPDemuxNextFrame(&iter));

		WebPDemuxReleaseIterator(&iter);
	}

	WebPDemuxDelete(demux);
	return !m_frame_delay.empty();
}

void CSCD2Image::apply_orientation_transform()
{
	std::lock_guard<std::recursive_mutex> lock(g_d2d_dc_mutex);

	if (m_exif_info.orientation <= 1 || m_exif_info.orientation > 8)
		return;

	if (m_img.size() == 0 || !m_img[0] || !m_d2dc)
		return;

	ComPtr<ID2D1Bitmap1> src = m_img[0];

	const D2D1_SIZE_U srcPx = src->GetPixelSize();
	if (srcPx.width == 0 || srcPx.height == 0)
		return;

	const bool swapWH = (m_exif_info.orientation >= 5 && m_exif_info.orientation <= 8);
	const UINT dstW = swapWH ? srcPx.height : srcPx.width;
	const UINT dstH = swapWH ? srcPx.width : srcPx.height;

	// 출력 비트맵(타겟 가능해야 DrawImage로 렌더 가능)
	D2D1_BITMAP_PROPERTIES1 dstProps =
		D2D1::BitmapProperties1(
			D2D1_BITMAP_OPTIONS_TARGET,
			D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

	ComPtr<ID2D1Bitmap1> dst;
	HRESULT hr = m_d2dc->CreateBitmap(D2D1::SizeU(dstW, dstH), nullptr, 0, &dstProps, dst.GetAddressOf());
	if (FAILED(hr) || !dst)
	{
		TRACE(_T("apply_orientation_transform: CreateBitmap failed 0x%08X\n"), hr);
		return;
	}

	// orientation -> 변환행렬 계산 (dst 좌표계 기준으로 src를 배치)
	// 주의: D2D1_MATRIX_3X2_F는 "출력 좌표 -> 입력 좌표" 개념이 아니라
	// 일반적인 2D affine이며, 여기서는 "src를 dst로 옮기는" 변환으로 구성.
	D2D1_MATRIX_3X2_F m = D2D1::Matrix3x2F::Identity();
	const float w = (float)srcPx.width;
	const float h = (float)srcPx.height;

	switch (m_exif_info.orientation)
	{
	case 2: // Mirror horizontal
		// x' = w - x
		m = D2D1::Matrix3x2F::Scale(-1.0f, 1.0f) * D2D1::Matrix3x2F::Translation(w, 0.0f);
		break;

	case 3: // Rotate 180
		// (x,y)->(-x,-y) then translate
		m = D2D1::Matrix3x2F::Rotation(180.0f) * D2D1::Matrix3x2F::Translation(w, h);
		break;

	case 4: // Mirror vertical
		m = D2D1::Matrix3x2F::Scale(1.0f, -1.0f) * D2D1::Matrix3x2F::Translation(0.0f, h);
		break;

	case 5: // Transpose (대각 반사: x<->y) = Rotate 90 + FlipH (근사)
		// 구현: rotate 90 CW 후 mirror horizontal을 섞는 것보다
		// 여기서는 (x,y)->(y,x) 형태로 만들기 위해 회전/이동 조합
		// D2D에서 transpose는 직접 행렬로 가능:
		// [0 1; 1 0] (x<->y)
		m = D2D1::Matrix3x2F(0, 1, 1, 0, 0, 0);
		break;

	case 6: // Rotate 90 CW
		// dst(x,y) = src(y, h - x) 개념
		// 행렬: rotate 90 then translate
		m = D2D1::Matrix3x2F::Rotation(90.0f) * D2D1::Matrix3x2F::Translation(h, 0.0f);
		break;

	case 7: // Transverse (대각 반사 + 180) = (x,y)->(h - y, w - x) with swap
		// [0 -1; -1 0] + translate
		m = D2D1::Matrix3x2F(0, -1, -1, 0, h, w);
		break;

	case 8: // Rotate 90 CCW
		m = D2D1::Matrix3x2F::Rotation(-90.0f) * D2D1::Matrix3x2F::Translation(0.0f, w);
		break;

	default:
		return;
	}

	// 원래 상태 백업
	ComPtr<ID2D1Image> prevTarget;
	m_d2dc->GetTarget(prevTarget.GetAddressOf());

	D2D1_MATRIX_3X2_F prevTransform;
	m_d2dc->GetTransform(&prevTransform);

	// 렌더
	m_d2dc->SetTarget(dst.Get());
	m_d2dc->BeginDraw();
	m_d2dc->Clear(D2D1::ColorF(0, 0, 0, 0));
	m_d2dc->SetTransform(D2D1::Matrix3x2F::Identity());

	ComPtr<ID2D1Effect> affine;
	hr = m_d2dc->CreateEffect(CLSID_D2D12DAffineTransform, affine.GetAddressOf());
	if (FAILED(hr) || !affine)
	{
		TRACE(_T("apply_orientation_transform: CreateEffect(Affine) failed 0x%08X\n"), hr);
		m_d2dc->EndDraw();
		m_d2dc->SetTarget(prevTarget.Get());
		m_d2dc->SetTransform(prevTransform);
		return;
	}

	// 입력: bitmap
	affine->SetInput(0, src.Get());

	// 변환행렬
	affine->SetValue(D2D1_2DAFFINETRANSFORM_PROP_TRANSFORM_MATRIX, m);

	// 경계 밖은 투명 처리(기본이지만 명시)
	affine->SetValue(D2D1_2DAFFINETRANSFORM_PROP_BORDER_MODE, D2D1_BORDER_MODE_HARD);

	// 품질(보간) - 필요시 LINEAR
	affine->SetValue(D2D1_2DAFFINETRANSFORM_PROP_INTERPOLATION_MODE, D2D1_2DAFFINETRANSFORM_INTERPOLATION_MODE_LINEAR);

	m_d2dc->DrawImage(affine.Get(), D2D1::Point2F(0, 0));

	hr = m_d2dc->EndDraw();

	// 원복
	m_d2dc->SetTarget(prevTarget.Get());
	m_d2dc->SetTransform(prevTransform);

	if (FAILED(hr))
	{
		TRACE(_T("apply_orientation_transform: EndDraw failed 0x%08X\n"), hr);
		return;
	}

	// 결과 반영
	m_img[0] = dst;
	m_img_origin = dst;

	UINT stride = 0;
	const auto px = m_img[0]->GetPixelSize();
	const size_t size = (size_t)(px.height) * (size_t)(px.width) * 4; // 이 값은 "최소" 크기라 pitch가 더 크면 부족함!

	extract_raw_data_from_bitmap(m_d2dc, m_img[0].Get(), &m_data);
}

HRESULT CSCD2Image::extract_raw_data_from_bitmap(
	ID2D1DeviceContext* d2dc,
	ID2D1Bitmap1* src,
	uint8_t** data)
{
	if (!d2dc || !src)
		return E_INVALIDARG;

	const D2D1_SIZE_U px = src->GetPixelSize();
	if (px.width == 0 || px.height == 0)
		return E_FAIL;

	// 우리가 원하는 최종 stride는 width*4 (packed BGRA)
	const UINT packedStride = px.width * 4;
	const size_t bufSize = (size_t)packedStride * (size_t)px.height;

	if (*data != nullptr)
		delete[] *data;

	*data = new uint8_t[bufSize];

	// CPU 읽기 가능한 bitmap 생성
	D2D1_BITMAP_PROPERTIES1 cpuProps =
		D2D1::BitmapProperties1(
			D2D1_BITMAP_OPTIONS_CPU_READ | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
			D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

	ComPtr<ID2D1Bitmap1> cpuBmp;
	HRESULT hr = d2dc->CreateBitmap(px, nullptr, 0, &cpuProps, cpuBmp.GetAddressOf());
	if (FAILED(hr) || !cpuBmp)
		return FAILED(hr) ? hr : E_FAIL;

	// GPU -> CPU 복사
	hr = cpuBmp->CopyFromBitmap(nullptr, src, nullptr);
	if (FAILED(hr))
		return hr;

	// Map
	D2D1_MAPPED_RECT mapped = {};
	hr = cpuBmp->Map(D2D1_MAP_OPTIONS_READ, &mapped);
	if (FAILED(hr))
		return hr;

	// 행 단위로 packedStride만 복사해서 padding 제거
	for (UINT y = 0; y < px.height; ++y)
	{
		memcpy(
			*data + (size_t)y * packedStride,
			mapped.bits + (size_t)y * mapped.pitch,
			packedStride);
	}

	cpuBmp->Unmap();
	m_stride = packedStride;
	return S_OK;
}

bool CSCD2Image::set_frame_delay(int index, int delay_ms)
{
	if ((index < 0) || (index >= m_img.size()) || (delay_ms <= 0))
		return false;

	if (m_frame_delay.size() <= index)
		m_frame_delay.resize(index + 1);

	m_frame_delay[index] = delay_ms;
	return true;
}
