#pragma once

// lunasvg 기반 SVG 래스터라이저 래퍼.
// - Document 를 파싱해 보관하고, 임의의 픽셀 크기로 재래스터화(zoom 대응)한다.
// - 출력은 straight(비-premultiplied) BGRA 32bpp. CSCD2Image::load(raw, ..., channel=4)
//   및 GDI+ 소비 측이 기대하는 포맷이다. lunasvg 는 premultiplied ARGB(=메모리상 BGRA premul)
//   로 내므로 render() 안에서 un-premultiply 하여 straight BGRA 로 되돌린다.

#include <vector>
#include <memory>
#include <cstdint>
#include <string>

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

	// ── 애니메이션(SMIL) 베이킹 ────────────────────────────────
	// lunasvg 는 애니메이션을 재생하지 못한다(파싱 후 무시). 그래서 SMIL 애니메이션
	// (<animate>/<animateTransform>/<set>)을 우리가 직접 해석해, 시각 t 마다 대상
	// 속성을 그 시점 값으로 써넣은 "정적 스냅샷 SVG" 를 만들고 lunasvg 로 한 장씩
	// 래스터화한다. 결과 프레임들을 GIF 처럼 재생하면 된다.
	// 지원: <animate>/<animateColor>/<animateTransform>/<set>, from/to/by·values·keyTimes,
	//       calcMode linear/discrete, repeatCount 수치/indefinite, fill freeze/remove,
	//       begin 수치 오프셋, href/xlink:href="#id" 또는 부모 대상.
	// 미지원: CSS @keyframes, JS/상호작용 트리거, animateMotion, keySplines(선형 근사),
	//         additive="sum"/accumulate(부분).

	// load 시 SMIL 애니메이션 요소가 감지됐으면 true(잠정). 실제 2프레임 이상 생성은 build_frames 결과로 확정.
	bool is_animated() const { return m_animated; }

	// 애니메이션을 w x h 로 프레임 시퀀스로 굽는다. 각 frames[i] 는 straight BGRA(w*h*4),
	// delays_ms[i] 는 해당 프레임 표시 시간(ms). 성공 시 true 이고 프레임 수 >= 2.
	// 애니메이션이 없거나 구울 수 없으면 false(호출부는 정적 render 로 폴백).
	bool build_frames(int w, int h,
					  std::vector<std::vector<uint8_t>>& frames,
					  std::vector<int>& delays_ms) const;

private:
	std::unique_ptr<lunasvg::Document> m_doc;
	std::string                        m_svg_data;   // 원본 SVG 바이트(프레임 베이킹 재파싱용)
	bool                               m_animated = false;
};
