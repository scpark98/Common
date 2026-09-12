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

#include <chrono>
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
			r.language = std::wstring(engine.RecognizerLanguage().LanguageTag());

			auto ibuf = CryptographicBuffer::CreateFromByteArray(
				array_view<uint8_t const>(bgra.data(), bgra.data() + bgra.size()));
			SoftwareBitmap sb = SoftwareBitmap::CreateCopyFromBuffer(
				ibuf, BitmapPixelFormat::Bgra8, w, h);

			OcrResult result = engine.RecognizeAsync(sb).get();
			r.text = std::wstring(result.Text());

			//단어 상자 높이의 중앙값 — 글자가 엔진이 잘 읽는 크기인지 판단할 수 있는 유일한 런타임 지표다.
			std::vector<int> heights;
			for (auto const& line : result.Lines())
			{
				for (auto const& word : line.Words())
				{
					auto const rc = word.BoundingRect();

					SCOcrWord w;
					w.text   = std::wstring(word.Text());
					w.line   = r.line_count;
					w.x      = (int)(rc.X + 0.5f);
					w.y      = (int)(rc.Y + 0.5f);
					w.width  = (int)(rc.Width + 0.5f);
					w.height = (int)(rc.Height + 0.5f);
					r.words.push_back(w);

					r.word_count++;
					heights.push_back(w.height);
				}
				r.line_count++;
			}
			if (!heights.empty())
			{
				std::sort(heights.begin(), heights.end());
				r.median_word_height = heights[heights.size() / 2];
			}
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

	//alpha 가 전부 0 이면 SoftwareBitmap(Bgra8) 이 전면 투명으로 해석돼 아무것도 인식되지 않는다.
	//호출자가 32bppRGB 로 넘겼는지(=alpha 가 255 로 채워졌는지) 로그로 확인할 수 있게 범위를 남긴다.
	uint8_t a_min = 255;
	uint8_t a_max = 0;
	for (size_t i = 3; i < buf.size(); i += 4)
	{
		a_min = (std::min)(a_min, buf[i]);
		a_max = (std::max)(a_max, buf[i]);
	}

	const auto started = std::chrono::steady_clock::now();

	//OcrResult.get() 은 STA(UI) 스레드에서 block-wait 시 RO_E_BLOCKING. MTA worker 에서 실행.
	std::thread t([&]() { r = recognize_on_mta(buf, w, h); });
	t.join();

	r.width = w;
	r.height = h;
	r.alpha_min = a_min;
	r.alpha_max = a_max;
	r.elapsed_ms = (int)std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now() - started).count();
	return r;
}

std::wstring sc_win_ocr(Gdiplus::Bitmap* bmp)
{
	return sc_win_ocr_ex(bmp).text;
}
