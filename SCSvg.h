#pragma once

// lunasvg 기반 SVG 래스터라이저 래퍼.
// - Document 를 파싱해 보관하고, 임의의 픽셀 크기로 재래스터화(zoom 대응)한다.
// - 출력은 straight(비-premultiplied) BGRA 32bpp. CSCD2Image::load(raw, ..., channel=4)
//   및 GDI+ 소비 측이 기대하는 포맷이다. lunasvg 는 premultiplied ARGB(=메모리상 BGRA premul)
//   로 내므로 render() 안에서 un-premultiply 하여 straight BGRA 로 되돌린다.

#include <vector>
#include <memory>
#include <cstdint>

namespace lunasvg { class Document; }

class sc_svg
{
public:
	sc_svg();
	~sc_svg();

	// 파일을 파싱해 보관. 성공 시 true.
	// lunasvg::Document::loadFromFile 은 내부적으로 fopen(멀티바이트 경로)을 쓰므로
	// 한글 경로에서 실패할 수 있다. 이를 피하려고 파일 바이트를 wide-path 로 직접 읽어
	// loadFromData 로 파싱한다.
	bool load(const wchar_t* path);

	bool is_valid() const { return m_doc != nullptr; }
	void clear();

	// SVG 고유(자연) 크기 — width/height 속성 또는 viewBox 기반. 없으면 0.
	float natural_width() const;
	float natural_height() const;

	// w x h 픽셀로 래스터화 → straight BGRA(channel 4, stride = w*4) 를 bgra_out 에 채운다.
	// 성공 시 true, bgra_out 크기 = w*h*4.
	bool render(int w, int h, std::vector<uint8_t>& bgra_out) const;

private:
	std::unique_ptr<lunasvg::Document> m_doc;
};
