#include "SCSvg.h"

#include <fstream>
#include <iterator>

// 벤더링된 lunasvg (정적 링크). LUNASVG_BUILD_STATIC 은 프로젝트 전역 정의.
#include "lunasvg.h"

sc_svg::sc_svg() = default;

sc_svg::~sc_svg() = default;   // unique_ptr<Document> 소멸을 위해 완전한 타입이 보이는 이 지점에 둔다.

void sc_svg::clear()
{
	m_doc.reset();
}

bool sc_svg::load(const wchar_t* path)
{
	m_doc.reset();
	if (path == nullptr)
		return false;

	// wide-path 로 바이너리 오픈(한글 경로 대응) 후 전체 바이트를 읽어 메모리에서 파싱.
	std::ifstream f(path, std::ios::binary);
	if (!f)
		return false;

	std::string data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
	if (data.empty())
		return false;

	m_doc = lunasvg::Document::loadFromData(data.data(), data.size());
	return m_doc != nullptr;
}

float sc_svg::natural_width() const
{
	return m_doc ? m_doc->width() : 0.0f;
}

float sc_svg::natural_height() const
{
	return m_doc ? m_doc->height() : 0.0f;
}

bool sc_svg::render(int w, int h, std::vector<uint8_t>& bgra_out) const
{
	if (!m_doc || w <= 0 || h <= 0)
		return false;

	// 투명 배경(0x00000000)으로 w x h 래스터화.
	lunasvg::Bitmap bmp = m_doc->renderToBitmap(w, h, 0x00000000);
	if (bmp.isNull() || bmp.data() == nullptr)
		return false;

	const int      bw     = bmp.width();
	const int      bh     = bmp.height();
	const int      stride = bmp.stride();     // premul 이므로 row 마다 stride 바이트(≠ bw*4 일 수 있음)
	const uint8_t* src    = bmp.data();

	bgra_out.assign(static_cast<size_t>(bw) * bh * 4, 0);

	// lunasvg 출력은 ARGB32_Premultiplied → 리틀엔디안 메모리상 B,G,R,A(premultiplied).
	// raw 로더는 straight BGRA(channel 4) 를 기대하므로 알파로 나눠 un-premultiply 한다.
	for (int y = 0; y < bh; ++y)
	{
		const uint8_t* srow = src + static_cast<size_t>(y) * stride;
		uint8_t*       drow = bgra_out.data() + static_cast<size_t>(y) * bw * 4;

		for (int x = 0; x < bw; ++x)
		{
			uint8_t b = srow[x * 4 + 0];
			uint8_t g = srow[x * 4 + 1];
			uint8_t r = srow[x * 4 + 2];
			uint8_t a = srow[x * 4 + 3];

			if (a != 0 && a != 255)
			{
				// round-to-nearest 나눗셈. a==0 이면 색 성분도 0 이므로 그대로 둔다.
				b = static_cast<uint8_t>((b * 255 + a / 2) / a);
				g = static_cast<uint8_t>((g * 255 + a / 2) / a);
				r = static_cast<uint8_t>((r * 255 + a / 2) / a);
			}

			drow[x * 4 + 0] = b;
			drow[x * 4 + 1] = g;
			drow[x * 4 + 2] = r;
			drow[x * 4 + 3] = a;
		}
	}

	return true;
}
