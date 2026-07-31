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
#include <functional>

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
	// (화면 표시용) 첫 프레임 실측 비용을 총 예산(1500ms)에 맞춰 프레임 수를 적응 감축한다.
	bool build_frames(int w, int h,
					  std::vector<std::vector<uint8_t>>& frames,
					  std::vector<int>& delays_ms) const;

	// 프레임을 한 장 굽는 즉시 sink 로 넘기는 스트리밍 베이커(export 처럼 프레임이 많아
	// 전부 메모리에 담기 부담스러운 경우용). sink(idx, total, bgra(straight, w*h*4), delay_ms).
	// sink 가 false 를 반환하면 즉시 중단(취소)하고 false 를 리턴한다.
	//   fps       : 목표 프레임/초.
	//   budget_ms : > 0 이면 첫 프레임 실측으로 프레임 수를 이 예산에 맞춰 감축(화면용).
	//               <= 0 이면 예산 캡 없이 round(T*fps) 프레임을 굽는다(export 용, 부드러움 우선).
	// 정상적으로 N(>=2) 프레임을 모두 sink 로 넘겼으면 true.
	bool build_frames_stream(int w, int h, int fps, double budget_ms,
		const std::function<bool(int idx, int total,
								 const std::vector<uint8_t>& bgra,
								 int delay_ms)>& sink) const;

private:
	std::unique_ptr<lunasvg::Document> m_doc;
	std::string                        m_svg_data;   // 원본 SVG 바이트(프레임 베이킹 재파싱용)
	bool                               m_animated = false;
};

// ── SVG 애니메이션 → 애니메이션 GIF 내보내기 ──────────────────────────
// svg_path 를 새 sc_svg 로 파싱(호출자 표시용 인스턴스와 상태 공유 없음 → 워커 스레드 안전)해
// 프레임을 스트리밍으로 굽고, WIC(GUID_ContainerFormatGif)로 무한 루프 애니메이션 GIF 를 쓴다.
//   out_gif_path : 출력 GIF 경로. null/빈문자열이면 svg_path 와 같은 폴더·같은 이름 + ".gif".
//   out_w/out_h  : <= 0 이면 SVG 자연 크기.
//   fps          : GIF 부드러움. GIF delay 최소 단위가 2cs(=50fps)라 내부에서 [1,50] 로 클램프.
//   progress     : progress(cur, total). false 반환 시 취소(부분 파일 삭제). null 허용.
// GIF 는 알파(1비트)의 안티에일리어스 경계가 지저분하므로, 각 프레임을 흰 배경 위에 합성해
// 불투명 GIF 로 낸다(브라우저에서 흰 페이지에 얹혀 보이는 것과 동일). 성공 시 true.
bool export_svg_to_animated_gif(const wchar_t* svg_path,
								const wchar_t* out_gif_path,
								int out_w, int out_h, int fps,
								std::function<bool(int cur, int total)> progress);

// SVG 애니메이션을 애니메이션 WebP 로 내보낸다(libwebp WebPAnimEncoder, x64 전용).
// GIF 와 달리 24비트 풀컬러 + 8비트 알파를 보존하므로 디더/밴딩/256색·1비트 투명 제약이 없다.
//   out_webp_path : null/빈문자열이면 svg_path 와 같은 폴더·같은 이름 + ".webp".
//   out_w/out_h   : <= 0 이면 SVG 자연 크기.
//   fps           : 프레임/초(1 이상). WebP delay 는 ms 단위라 GIF 같은 2cs 하한 없음.
//   progress      : progress(cur, total). false 반환 시 취소(부분 파일 삭제). null 허용.
// 무한 루프. 무손실(lossless) 인코딩. 성공 시 true. (x64 아닌 빌드에서는 항상 false 스텁.)
bool export_svg_to_animated_webp(const wchar_t* svg_path,
								 const wchar_t* out_webp_path,
								 int out_w, int out_h, int fps,
								 std::function<bool(int cur, int total)> progress);
