#include "SCWinOcr.h"

//C++/WinRT 는 NOMINMAX 를 선호하지만 gdiplus.h 는 min/max 를 요구한다.
//NOMINMAX 로 매크로를 끄고, gdiplus.h 직전에 std::min/max 를 Gdiplus 네임스페이스로 끌어와 충돌 해소.
//WIN32_LEAN_AND_MEAN 은 쓰지 않는다 — gdiplus.h 가 objidl.h 의 IStream 등을 필요로 한다.
#define NOMINMAX
#include <windows.h>
#include <objidl.h>

#include <algorithm>
namespace Gdiplus { using std::min; using std::max; }
#include <gdiplus.h>

#include <thread>
#include <vector>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Globalization.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Media.Ocr.h>
#include <winrt/Windows.Security.Cryptography.h>

#pragma comment(lib, "windowsapp.lib")

using namespace winrt;
using namespace winrt::Windows::Globalization;
using namespace winrt::Windows::Graphics::Imaging;
using namespace winrt::Windows::Media::Ocr;
using namespace winrt::Windows::Security::Cryptography;

static SCOcrResult recognize_on_mta(const std::vector<uint8_t>& bgra, int w, int h)
{
	SCOcrResult r;
	try
	{
		init_apartment(apartment_type::multi_threaded);

		for (auto const& lang : OcrEngine::AvailableRecognizerLanguages())
		{
			if (!r.languages.empty())
				r.languages += L", ";
			r.languages += std::wstring(lang.DisplayName());
		}

		OcrEngine engine = OcrEngine::TryCreateFromUserProfileLanguages();
		if (!engine)
			engine = OcrEngine::TryCreateFromLanguage(Language(L"en"));

		if (!engine)
		{
			r.error = L"OcrEngine 생성 실패 (OCR 언어팩 미설치)";
		}
		else
		{
			auto ibuf = CryptographicBuffer::CreateFromByteArray(
				array_view<uint8_t const>(bgra.data(), bgra.data() + bgra.size()));
			SoftwareBitmap sb = SoftwareBitmap::CreateCopyFromBuffer(
				ibuf, BitmapPixelFormat::Bgra8, w, h);

			OcrResult result = engine.RecognizeAsync(sb).get();
			r.text = std::wstring(result.Text());
		}

		uninit_apartment();
	}
	catch (winrt::hresult_error const& e)
	{
		r.error = std::wstring(L"hresult: ") + e.message().c_str();
	}
	catch (...)
	{
		r.error = L"unknown exception";
	}
	return r;
}

SCOcrResult sc_win_ocr_ex(Gdiplus::Bitmap* bmp)
{
	SCOcrResult r;
	if (!bmp)
		return r;

	int w = (int)bmp->GetWidth();
	int h = (int)bmp->GetHeight();
	if (w <= 0 || h <= 0)
		return r;

	Gdiplus::Rect rc(0, 0, w, h);
	Gdiplus::BitmapData bd;
	if (bmp->LockBits(&rc, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bd) != Gdiplus::Ok)
	{
		r.error = L"LockBits 실패";
		return r;
	}

	std::vector<uint8_t> buf((size_t)w * h * 4);
	const uint8_t* src = (const uint8_t*)bd.Scan0;
	for (int y = 0; y < h; ++y)
		memcpy(buf.data() + (size_t)y * w * 4, src + (size_t)y * bd.Stride, (size_t)w * 4);
	bmp->UnlockBits(&bd);

	//OcrResult.get() 은 STA(UI) 스레드에서 block-wait 시 RO_E_BLOCKING. MTA worker 에서 실행.
	std::thread t([&]() { r = recognize_on_mta(buf, w, h); });
	t.join();
	return r;
}

std::wstring sc_win_ocr(Gdiplus::Bitmap* bmp)
{
	return sc_win_ocr_ex(bmp).text;
}
