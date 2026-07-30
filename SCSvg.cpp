#include "SCSvg.h"

#include <fstream>
#include <iterator>
#include <set>
#include <vector>
#include <utility>
#include <mutex>
#include <sstream>
#include <map>
#include <cmath>
#include <cstdio>
#include <algorithm>

#include <Windows.h>
#include <dwrite.h>
#include <wrl/client.h>
#pragma comment(lib, "dwrite.lib")

// 벤더링된 lunasvg (정적 링크). LUNASVG_BUILD_STATIC 은 프로젝트 전역 정의.
#include "lunasvg.h"

// SMIL 애니메이션 베이킹용 DOM 파서/직렬화 (Common/xml/pugixml, 정적 소스).
#include "pugixml.hpp"

// ── 폰트 온디맨드 등록 ─────────────────────────────────────────────────────
// lunasvg 는 시스템 폰트 전체를 로드(load_sys)하면 소멸자 없는 전역 캐시에 수백 개를
// 부어 종료 시 대량 릭이 난다. 그래서 빌드 매크로 LUNASVG_DISABLE_LOAD_SYSTEM_FONTS
// 로 전체 로드를 끄고, 대신 "이 SVG 가 실제로 참조하는 font-family 만" DirectWrite 로
// 해석해 lunasvg_add_font_face_from_file 로 등록한다. 텍스트 없는 아이콘 SVG 는 아무
// 폰트도 등록하지 않아 zero-leak 경로가 그대로 유지된다.
namespace
{
	using Microsoft::WRL::ComPtr;

	std::wstring utf8_to_wide(const std::string& s)
	{
		if (s.empty()) return std::wstring();
		int n = ::MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
		std::wstring w((size_t)n, L'\0');
		::MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), &w[0], n);
		return w;
	}

	std::string wide_to_utf8(const std::wstring& w)
	{
		if (w.empty()) return std::string();
		int n = ::WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
		std::string s((size_t)n, '\0');
		::WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), &s[0], n, nullptr, nullptr);
		return s;
	}

	IDWriteFactory* dwrite_factory()
	{
		// 함수-로컬 static ComPtr: 프로세스 종료 시 소멸자에서 Release 되어 릭 없음.
		static ComPtr<IDWriteFactory> factory;
		if (!factory)
			::DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
				reinterpret_cast<IUnknown**>(factory.GetAddressOf()));
		return factory.Get();
	}

	// family 이름 → 실제 폰트 파일 경로. 로컬 파일 폰트가 아니면 실패(false).
	bool resolve_font_file(const std::wstring& family, bool bold, bool italic, std::wstring& out_path)
	{
		IDWriteFactory* factory = dwrite_factory();
		if (!factory) return false;

		ComPtr<IDWriteFontCollection> coll;
		if (FAILED(factory->GetSystemFontCollection(coll.GetAddressOf(), FALSE)))
			return false;

		UINT32 index = 0; BOOL exists = FALSE;
		if (FAILED(coll->FindFamilyName(family.c_str(), &index, &exists)) || !exists)
			return false;

		ComPtr<IDWriteFontFamily> ff;
		if (FAILED(coll->GetFontFamily(index, ff.GetAddressOf())))
			return false;

		ComPtr<IDWriteFont> font;
		if (FAILED(ff->GetFirstMatchingFont(
				bold   ? DWRITE_FONT_WEIGHT_BOLD  : DWRITE_FONT_WEIGHT_NORMAL,
				DWRITE_FONT_STRETCH_NORMAL,
				italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL,
				font.GetAddressOf())))
			return false;

		ComPtr<IDWriteFontFace> face;
		if (FAILED(font->CreateFontFace(face.GetAddressOf())))
			return false;

		UINT32 nfiles = 0;
		if (FAILED(face->GetFiles(&nfiles, nullptr)) || nfiles == 0)
			return false;
		std::vector<IDWriteFontFile*> files(nfiles, nullptr);
		if (FAILED(face->GetFiles(&nfiles, files.data())))
			return false;
		ComPtr<IDWriteFontFile> file;
		file.Attach(files[0]);                                   // 첫 파일만 사용
		for (UINT32 i = 1; i < nfiles; ++i) if (files[i]) files[i]->Release();

		const void* key = nullptr; UINT32 keySize = 0;
		if (FAILED(file->GetReferenceKey(&key, &keySize)))
			return false;
		ComPtr<IDWriteFontFileLoader> loader;
		if (FAILED(file->GetLoader(loader.GetAddressOf())))
			return false;
		ComPtr<IDWriteLocalFontFileLoader> local;
		if (FAILED(loader.As(&local)))                           // 메모리 폰트 등 로컬 아님 → 스킵
			return false;

		UINT32 len = 0;
		if (FAILED(local->GetFilePathLengthFromKey(key, keySize, &len)))
			return false;
		std::wstring path((size_t)len + 1, L'\0');
		if (FAILED(local->GetFilePathFromKey(key, keySize, &path[0], len + 1)))
			return false;
		path.resize(len);
		out_path = path;
		return true;
	}

	// (family, bold, italic) 1건을 해석·등록. 프로세스 전역 캐시이므로 done 으로 중복 방지.
	void register_one(const std::string& family_utf8, const std::wstring& family_wide,
					  bool bold, bool italic, std::set<std::string>& done)
	{
		std::string key = family_utf8;
		key.push_back(bold   ? 'B' : 'b');
		key.push_back(italic ? 'I' : 'i');
		if (done.count(key)) return;
		done.insert(key);

		std::wstring path;
		if (!resolve_font_file(family_wide, bold, italic, path))
			return;
		lunasvg_add_font_face_from_file(family_utf8.c_str(), bold, italic, wide_to_utf8(path).c_str());
	}

	bool has_text_element(const std::string& d)
	{
		return d.find("<text")     != std::string::npos
			|| d.find("<tspan")    != std::string::npos
			|| d.find("<textPath") != std::string::npos;
	}

	std::string trim_family_token(std::string t)
	{
		size_t a = t.find_first_not_of(" \t\r\n'\"");
		size_t b = t.find_last_not_of(" \t\r\n'\"");
		if (a == std::string::npos) return std::string();
		return t.substr(a, b - a + 1);
	}

	bool is_generic(const std::string& lower)
	{
		return lower == "serif" || lower == "sans-serif" || lower == "monospace"
			|| lower == "cursive" || lower == "fantasy" || lower == "inherit"
			|| lower == "initial" || lower == "unset";
	}

	std::string to_lower(std::string s)
	{
		for (char& c : s) c = (char)::tolower((unsigned char)c);
		return s;
	}

	// data 에서 font-family 선언값을 수집. explicit_families = 실제 폰트명, generics = 등장한 generic.
	void collect_families(const std::string& d, std::set<std::string>& explicit_families,
						  std::set<std::string>& generics)
	{
		const std::string tag = "font-family";
		size_t pos = 0;
		while ((pos = d.find(tag, pos)) != std::string::npos)
		{
			size_t i = pos + tag.size();
			while (i < d.size() && (d[i] == ' ' || d[i] == '\t')) ++i;
			if (i < d.size() && (d[i] == ':' || d[i] == '=')) ++i;
			else { pos = i; continue; }
			size_t start = i;
			while (i < d.size() && d[i] != ';' && d[i] != '}' && d[i] != '<' && d[i] != '\r' && d[i] != '\n')
			{
				// 속성 형태 font-family="A, B" 는 닫는 따옴표에서 종료
				if ((d[start] == ' ' || start == pos + tag.size()) && (d[i] == '"' || d[i] == '\''))
				{
					char q = d[i];
					size_t vstart = i + 1;
					size_t vend = d.find(q, vstart);
					if (vend == std::string::npos) vend = d.size();
					start = vstart; i = vend;
					break;
				}
				++i;
			}
			std::string value = d.substr(start, i - start);
			pos = (i > pos) ? i : pos + tag.size();

			size_t s = 0;
			while (s <= value.size())
			{
				size_t c = value.find(',', s);
				std::string one = trim_family_token(value.substr(s, (c == std::string::npos ? value.size() : c) - s));
				if (!one.empty())
				{
					std::string low = to_lower(one);
					if (is_generic(low)) generics.insert(low);
					else                 explicit_families.insert(one);
				}
				if (c == std::string::npos) break;
				s = c + 1;
			}
		}
	}

	// SVG 원본 바이트에서 참조 폰트만 골라 lunasvg 에 등록.
	void register_svg_fonts(const std::string& data)
	{
		static std::mutex mtx;                 // 썸네일 로드가 워커 스레드일 수 있어 직렬화
		std::lock_guard<std::mutex> lk(mtx);

		if (!has_text_element(data)) return;   // 텍스트 없으면 폰트 미등록(zero-leak 유지)

		std::set<std::string> explicit_families, generics;
		collect_families(data, explicit_families, generics);

		std::vector<std::pair<std::string, std::wstring>> targets;
		for (const auto& f : explicit_families)
			targets.emplace_back(f, utf8_to_wide(f));

		// generic / 미지정 대응 — lunasvg 는 빈 family·sans-serif 를 "Arial" 로 조회하므로
		// 구체 폰트명으로 등록한다. 등장한 generic 만(+기본 sans-serif) 등록해 과다 로드 방지.
		auto add_concrete = [&](const char* name) { targets.emplace_back(name, utf8_to_wide(name)); };
		add_concrete("Arial");
		if (generics.count("serif"))     add_concrete("Times New Roman");
		if (generics.count("monospace")) add_concrete("Courier New");
		if (generics.count("cursive"))   add_concrete("Comic Sans MS");
		if (generics.count("fantasy"))   add_concrete("Impact");

		static std::set<std::string> done;     // 전역 캐시 대응 프로세스 전역 dedup
		for (const auto& t : targets)
			for (int s = 0; s < 4; ++s)
				register_one(t.first, t.second, (s & 1) != 0, (s & 2) != 0, done);
	}

	// lunasvg 출력(premultiplied ARGB, 메모리상 BGRA premul) → straight BGRA 로 되돌려 out 에 채운다.
	// 성공 시 out.size = bw*bh*4. bmp 가 null 이면 false.
	bool unpremultiply_to_bgra(const lunasvg::Bitmap& bmp, std::vector<uint8_t>& out)
	{
		if (bmp.isNull() || bmp.data() == nullptr)
			return false;

		const int      bw     = bmp.width();
		const int      bh     = bmp.height();
		const int      stride = bmp.stride();
		const uint8_t* src    = bmp.data();

		out.assign(static_cast<size_t>(bw) * bh * 4, 0);
		for (int y = 0; y < bh; ++y)
		{
			const uint8_t* srow = src + static_cast<size_t>(y) * stride;
			uint8_t*       drow = out.data() + static_cast<size_t>(y) * bw * 4;
			for (int x = 0; x < bw; ++x)
			{
				uint8_t b = srow[x * 4 + 0];
				uint8_t g = srow[x * 4 + 1];
				uint8_t r = srow[x * 4 + 2];
				uint8_t a = srow[x * 4 + 3];
				if (a != 0 && a != 255)
				{
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
}

sc_svg::sc_svg() = default;

sc_svg::~sc_svg() = default;   // unique_ptr<Document> 소멸을 위해 완전한 타입이 보이는 이 지점에 둔다.

void sc_svg::clear()
{
	m_doc.reset();
	m_svg_data.clear();
	m_animated = false;
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
	if (m_doc)
	{
		register_svg_fonts(data);   // 이 파일이 참조하는 폰트만 온디맨드 등록

		m_svg_data = data;          // 프레임 베이킹 시 재파싱용 보관
		// 잠정 감지 — SMIL 애니메이션 요소가 있으면 build_frames 로 실제 굽기 시도.
		m_animated = m_svg_data.find("<animate")  != std::string::npos
				  || m_svg_data.find("<set")      != std::string::npos;
	}
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

	// 투명 배경(0x00000000)으로 w x h 래스터화 후 straight BGRA 로 변환.
	lunasvg::Bitmap bmp = m_doc->renderToBitmap(w, h, 0x00000000);
	return unpremultiply_to_bgra(bmp, bgra_out);
}

// ── SMIL 애니메이션 파싱/베이킹 ─────────────────────────────────────────────
namespace
{
namespace anim
{
	// ---- 문자열 유틸 ------------------------------------------------------
	void trim(std::string& s)
	{
		size_t a = s.find_first_not_of(" \t\r\n");
		size_t b = s.find_last_not_of(" \t\r\n");
		if (a == std::string::npos) { s.clear(); return; }
		s = s.substr(a, b - a + 1);
	}

	std::string lower(std::string s)
	{
		for (char& c : s) c = (char)::tolower((unsigned char)c);
		return s;
	}

	void split(const std::string& s, char sep, std::vector<std::string>& out)
	{
		out.clear();
		size_t start = 0;
		while (true)
		{
			size_t p = s.find(sep, start);
			out.push_back(s.substr(start, (p == std::string::npos ? s.size() : p) - start));
			if (p == std::string::npos) break;
			start = p + 1;
		}
	}

	// pugixml 은 PUGIXML_WCHAR_MODE(공유 설정) → char_t=wchar_t. 경계에서 UTF-8↔UTF-16 변환한다.
	// utf8_to_wide/wide_to_utf8 는 위 익명 네임스페이스(폰트 등록부)에 정의돼 있어 그대로 재사용.
	std::string attr(const pugi::xml_node& n, const char* name)
	{
		pugi::xml_attribute a = n.attribute(utf8_to_wide(name).c_str());
		return a ? wide_to_utf8(a.value()) : std::string();
	}

	std::string num_to_str(double v)
	{
		char b[32];
		std::snprintf(b, sizeof(b), "%g", v);
		return b;
	}

	std::string join_nums(const std::vector<double>& v)
	{
		std::string s;
		for (size_t i = 0; i < v.size(); ++i) { if (i) s += ' '; s += num_to_str(v[i]); }
		return s;
	}

	// "10 20, 30" / "10px" → [10,20,30] (단위 문자 무시)
	std::vector<double> parse_num_list(const std::string& s)
	{
		std::vector<double> v;
		const char* p = s.c_str();
		while (*p)
		{
			while (*p && (std::isspace((unsigned char)*p) || *p == ',' || *p == '(')) ++p;
			if (!*p) break;
			char* end = nullptr;
			double d = std::strtod(p, &end);
			if (end == p) { ++p; continue; }
			v.push_back(d);
			p = end;
			while (*p && (std::isalpha((unsigned char)*p) || *p == '%')) ++p;   // 단위 스킵
		}
		return v;
	}

	// ---- 시간(clock) 파싱 -------------------------------------------------
	double parse_clock_value(std::string t, bool& ok)
	{
		trim(t);
		ok = false;
		if (t.empty()) return 0.0;
		char* end = nullptr;
		double v = std::strtod(t.c_str(), &end);
		if (end == t.c_str()) return 0.0;      // 비수치(예: "click","a.end")
		ok = true;
		std::string u(end); trim(u);
		if (u == "ms")  return v / 1000.0;
		if (u == "min") return v * 60.0;
		if (u == "h")   return v * 3600.0;
		return v;                              // "s" 또는 단위없음 = 초
	}

	// begin/dur 등에서 첫 ';' 토큰만 파싱. 비수치면 ok=false.
	double parse_clock_first(const std::string& s, bool& ok, double def)
	{
		if (s.empty()) { ok = true; return def; }
		std::string t = s.substr(0, s.find(';'));
		bool o; double v = parse_clock_value(t, o);
		if (!o) { ok = false; return def; }
		ok = true; return v;
	}

	// ---- 색 파싱 ----------------------------------------------------------
	int hex1(char c)
	{
		if (c >= '0' && c <= '9') return c - '0';
		c = (char)::tolower((unsigned char)c);
		if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
		return -1;
	}
	bool hex2(char hi, char lo, double& out)
	{
		int a = hex1(hi), b = hex1(lo);
		if (a < 0 || b < 0) return false;
		out = a * 16 + b; return true;
	}

	bool named_color(const std::string& s, double c[4])
	{
		struct { const char* n; int r, g, b; } tbl[] = {
			{"black",0,0,0},{"white",255,255,255},{"red",255,0,0},{"lime",0,255,0},
			{"green",0,128,0},{"blue",0,0,255},{"yellow",255,255,0},{"cyan",0,255,255},
			{"aqua",0,255,255},{"magenta",255,0,255},{"fuchsia",255,0,255},{"gray",128,128,128},
			{"grey",128,128,128},{"silver",192,192,192},{"orange",255,165,0},{"purple",128,0,128},
			{"navy",0,0,128},{"teal",0,128,128},{"maroon",128,0,0},{"olive",128,128,0},
		};
		std::string k = lower(s);
		for (auto& e : tbl)
			if (k == e.n) { c[0] = e.r; c[1] = e.g; c[2] = e.b; c[3] = 255; return true; }
		return false;
	}

	bool parse_color(std::string s, double c[4])
	{
		trim(s);
		if (s.empty()) return false;
		if (s[0] == '#')
		{
			std::string h = s.substr(1);
			if (h.size() == 3)
			{
				int r = hex1(h[0]), g = hex1(h[1]), b = hex1(h[2]);
				if (r < 0 || g < 0 || b < 0) return false;
				c[0] = r * 17; c[1] = g * 17; c[2] = b * 17; c[3] = 255;   // #abc → #aabbcc
				return true;
			}
			if (h.size() == 6)
				return hex2(h[0], h[1], c[0]) && hex2(h[2], h[3], c[1]) && hex2(h[4], h[5], c[2]) && (c[3] = 255, true);
			return false;
		}
		if (lower(s).rfind("rgb", 0) == 0)
		{
			size_t lp = s.find('('), rp = s.find(')');
			if (lp == std::string::npos || rp == std::string::npos) return false;
			std::vector<double> n = parse_num_list(s.substr(lp + 1, rp - lp - 1));
			if (n.size() < 3) return false;
			c[0] = n[0]; c[1] = n[1]; c[2] = n[2];
			c[3] = (n.size() >= 4) ? n[3] * 255.0 : 255.0;   // rgba 알파 0..1
			return true;
		}
		return named_color(s, c);
	}

	int clampi(double v) { int i = (int)std::lround(v); return i < 0 ? 0 : (i > 255 ? 255 : i); }
	std::string color_hex(const double c[4])
	{
		char b[8];
		std::snprintf(b, sizeof(b), "#%02x%02x%02x", clampi(c[0]), clampi(c[1]), clampi(c[2]));
		return b;
	}

	std::vector<double> lerp_vec(const std::vector<double>& a, const std::vector<double>& b, double t)
	{
		size_t n = (std::min)(a.size(), b.size());   // (): <Windows.h> min 매크로 회피
		std::vector<double> r(n);
		for (size_t i = 0; i < n; ++i) r[i] = a[i] + (b[i] - a[i]) * t;
		return r;
	}

	// ---- 애니메이션 모델 --------------------------------------------------
	struct animation
	{
		pugi::xml_node target;
		std::string    attr;             // attributeName (animateTransform 은 "transform")
		bool           is_transform = false;
		std::string    transform_type;   // translate/scale/rotate/skewX/skewY
		bool           is_set       = false;
		bool           is_color     = false;
		bool           interp_ok    = false;   // stops 를 수치 보간에 쓸 수 있는가(아니면 raw 이산)
		bool           discrete     = false;
		bool           indefinite   = false;
		bool           freeze       = false;
		double         begin        = 0.0;
		double         dur          = 0.0;
		double         repeat       = 1.0;
		std::vector<std::string>          raw_stops;   // 원본 문자열(이산/폴백용)
		std::vector<std::vector<double>>  stops;       // 수치 파싱(색=[r,g,b,a])
		std::vector<double>               keytimes;    // 0..1, size==raw_stops 또는 empty
	};

	std::string format_vec(const animation& a, const std::vector<double>& v)
	{
		if (a.is_color)
		{
			double c[4] = { 0,0,0,255 };
			for (size_t i = 0; i < v.size() && i < 4; ++i) c[i] = v[i];
			return color_hex(c);
		}
		if (a.is_transform)
			return a.transform_type + "(" + join_nums(v) + ")";
		return join_nums(v);
	}
	std::string out_index(const animation& a, size_t i)
	{
		return a.interp_ok ? format_vec(a, a.stops[i]) : a.raw_stops[i];
	}
	std::string out_interp(const animation& a, size_t i, size_t j, double t)
	{
		return a.interp_ok ? format_vec(a, lerp_vec(a.stops[i], a.stops[j], t)) : a.raw_stops[i];
	}

	// 시각 t 의 속성값 문자열. 비활성(미시작/제거)면 false → 호출부는 속성을 건드리지 않음.
	bool sample(const animation& a, double t, std::string& out)
	{
		double ta = t - a.begin;
		if (ta < -1e-9) return false;

		if (a.is_set)
		{
			out = a.raw_stops.empty() ? std::string() : a.raw_stops.back();
			return true;
		}
		if (a.dur <= 0.0) return false;

		double p;
		double active_end = a.indefinite ? 1e18 : a.dur * a.repeat;
		if (ta >= active_end)
		{
			if (a.freeze) p = 1.0; else return false;   // 종료 후 fill=remove → 제거
		}
		else
		{
			p = std::fmod(ta, a.dur) / a.dur;
		}

		size_t n = a.raw_stops.size();
		if (n == 0) return false;
		if (n == 1) { out = out_index(a, 0); return true; }

		std::vector<double> kt = a.keytimes;
		if (kt.size() != n) { kt.resize(n); for (size_t i = 0; i < n; ++i) kt[i] = (double)i / (n - 1); }

		if (p <= kt.front()) { out = out_index(a, 0);     return true; }
		if (p >= kt.back())  { out = out_index(a, n - 1); return true; }

		size_t i = 0;
		while (i + 1 < n && p > kt[i + 1]) ++i;
		if (a.discrete) { out = out_index(a, i); return true; }

		double denom = kt[i + 1] - kt[i];
		double local = denom > 1e-9 ? (p - kt[i]) / denom : 0.0;
		out = out_interp(a, i, i + 1, local);
		return true;
	}

	pugi::xml_node find_by_id(const pugi::xml_node& n, const std::string& id)
	{
		for (pugi::xml_node c = n.first_child(); c; c = c.next_sibling())
		{
			if (c.type() != pugi::node_element) continue;
			pugi::xml_attribute a = c.attribute(L"id");
			if (a && utf8_to_wide(id) == a.value()) return c;
			pugi::xml_node r = find_by_id(c, id);
			if (r) return r;
		}
		return pugi::xml_node();
	}

	bool parse_anim(const pugi::xml_node& node, const pugi::xml_node& root, animation& a)
	{
		std::string nm = wide_to_utf8(node.name());
		a.is_set       = (nm == "set");
		a.is_transform = (nm == "animateTransform");

		// 대상: href/xlink:href="#id" 우선, 없으면 부모.
		std::string href = attr(node, "href");
		if (href.empty()) href = attr(node, "xlink:href");
		pugi::xml_node target;
		if (!href.empty() && href[0] == '#') target = find_by_id(root, href.substr(1));
		else                                 target = node.parent();
		if (!target || target.type() != pugi::node_element) return false;
		a.target = target;

		a.attr = a.is_transform ? "transform" : attr(node, "attributeName");
		if (a.attr.empty()) return false;
		if (a.is_transform)
		{
			a.transform_type = attr(node, "type");
			if (a.transform_type.empty()) a.transform_type = "translate";
		}

		bool ok;
		a.begin = parse_clock_first(attr(node, "begin"), ok, 0.0);
		if (!ok) return false;                                   // 이벤트/동기 트리거 미지원 → 스킵
		a.dur = parse_clock_first(attr(node, "dur"), ok, 0.0);
		if (!a.is_set && a.dur <= 0.0) return false;

		std::string rc = attr(node, "repeatCount");
		a.indefinite = (rc == "indefinite");
		if (!a.indefinite && !rc.empty()) { a.repeat = std::atof(rc.c_str()); if (a.repeat <= 0) a.repeat = 1; }
		a.freeze = a.is_set || (attr(node, "fill") == "freeze");
		a.discrete = (attr(node, "calcMode") == "discrete");

		// 값 스톱 수집
		std::vector<std::string> raws;
		bool by_mode = false;
		std::string values = attr(node, "values");
		if (!values.empty())
		{
			split(values, ';', raws);
		}
		else if (a.is_set)
		{
			raws.push_back(attr(node, "to"));
		}
		else
		{
			std::string from = attr(node, "from"), to = attr(node, "to"), by = attr(node, "by");
			if (!from.empty() && !to.empty()) { raws.push_back(from); raws.push_back(to); }
			else if (!from.empty() && !by.empty()) { raws.push_back(from); raws.push_back(by); by_mode = true; }
			else if (!to.empty()) { raws.push_back(to); a.discrete = true; }
			else if (!from.empty()) { raws.push_back(from); }
			else return false;
		}
		for (auto& s : raws) trim(s);
		if (raws.empty()) return false;
		a.raw_stops = raws;

		std::string kt = attr(node, "keyTimes");
		if (!kt.empty())
		{
			std::vector<std::string> ks; split(kt, ';', ks);
			for (auto& x : ks) { trim(x); a.keytimes.push_back(std::atof(x.c_str())); }
		}

		// 보간 가능 여부 분류
		static const std::set<std::string> color_attrs =
			{ "fill","stroke","stop-color","flood-color","lighting-color","color" };
		static const std::set<std::string> discrete_attrs =
			{ "d","points","visibility","display","transform-origin" };

		a.is_color = color_attrs.count(a.attr) > 0;

		if (a.is_set || discrete_attrs.count(a.attr) > 0)
		{
			if (!a.is_set) a.discrete = true;
			a.interp_ok = false;                    // raw 이산 사용
		}
		else if (a.is_color)
		{
			a.interp_ok = true;
			for (auto& s : raws)
			{
				double c[4];
				if (!parse_color(s, c)) { a.interp_ok = false; break; }
				a.stops.push_back({ c[0], c[1], c[2], c[3] });
			}
		}
		else
		{
			a.interp_ok = true;
			for (size_t i = 0; i < raws.size(); ++i)
			{
				std::vector<double> nums = parse_num_list(raws[i]);
				if (nums.empty()) { a.interp_ok = false; break; }
				if (by_mode && i == 1 && !a.stops.empty())     // by = from + by
					for (size_t k = 0; k < nums.size() && k < a.stops[0].size(); ++k)
						nums[k] += a.stops[0][k];
				a.stops.push_back(nums);
			}
		}
		if (a.interp_ok && a.stops.size() != raws.size()) a.interp_ok = false;
		return true;
	}

	void collect(const pugi::xml_node& n, const pugi::xml_node& root, std::vector<animation>& out)
	{
		for (pugi::xml_node c = n.first_child(); c; c = c.next_sibling())
		{
			if (c.type() != pugi::node_element) continue;
			std::string nm = wide_to_utf8(c.name());
			if (nm == "animate" || nm == "animateColor" || nm == "animateTransform" || nm == "set")
			{
				animation a;
				if (parse_anim(c, root, a)) out.push_back(a);
			}
			collect(c, root, out);
		}
	}

	void set_attr(pugi::xml_node node, const std::string& name, const std::string& val)
	{
		std::wstring wname = utf8_to_wide(name);
		pugi::xml_attribute a = node.attribute(wname.c_str());
		if (!a) a = node.append_attribute(wname.c_str());
		a.set_value(utf8_to_wide(val).c_str());
	}
}
}

bool sc_svg::build_frames(int w, int h,
						  std::vector<std::vector<uint8_t>>& frames,
						  std::vector<int>& delays_ms) const
{
	frames.clear();
	delays_ms.clear();
	if (m_svg_data.empty() || w <= 0 || h <= 0)
		return false;

	// DOM 을 1회만 파싱하고, 프레임마다 애니메이션 속성만 갱신한다(프레임별 재파싱 제거).
	pugi::xml_document doc;
	if (!doc.load_buffer(m_svg_data.data(), m_svg_data.size()))
		return false;
	std::vector<anim::animation> anims;
	anim::collect(doc, doc, anims);
	if (anims.empty())
		return false;

	// 전체 주기 T
	double T = 0.0;
	for (const auto& a : anims)
	{
		double cyc = a.is_set ? a.begin
				   : (a.indefinite ? (a.begin + a.dur) : (a.begin + a.dur * a.repeat));
		if (cyc > T) T = cyc;
	}
	if (T <= 1e-6)
		return false;
	if (T < 0.2) T = 0.2;
	if (T > 8.0) T = 8.0;                        // 상한(무한 루프/과대 주기 방지)

	// 애니메이션이 건드리는 (대상,속성)의 base(원본) 값 스냅샷. 프레임마다 이걸로 복원 후
	// t 시점 값을 주입한다 → 비활성 애니메이션 속성이 이전 프레임 값으로 남는 잔상 방지.
	struct base_val { bool had; std::string val; };
	std::map<pugi::xml_node, std::map<std::string, base_val>> base;
	for (const auto& a : anims)
	{
		auto& m = base[a.target];
		if (m.find(a.attr) == m.end())
		{
			pugi::xml_attribute at = a.target.attribute(utf8_to_wide(a.attr).c_str());
			base_val bv;
			bv.had = (bool)at;
			bv.val = at ? wide_to_utf8(at.value()) : std::string();
			m[a.attr] = bv;
		}
	}

	// 한 프레임을 굽는다: base 복원 → t 값 주입 → 직렬화 → lunasvg 래스터 → frames 에 push.
	auto bake = [&](double t)
	{
		for (auto& np : base)
		{
			pugi::xml_node node = np.first;
			for (auto& ap : np.second)
			{
				if (ap.second.had)
					anim::set_attr(node, ap.first, ap.second.val);
				else
				{
					pugi::xml_attribute ex = node.attribute(utf8_to_wide(ap.first).c_str());
					if (ex) node.remove_attribute(ex);
				}
			}
		}

		std::map<pugi::xml_node, std::string> xf;   // 대상별 transform 누적
		for (const auto& a : anims)
		{
			std::string val;
			if (!anim::sample(a, t, val))
				continue;
			if (a.is_transform)
			{
				std::string& s = xf[a.target];
				if (!s.empty()) s += ' ';
				s += val;
			}
			else
			{
				anim::set_attr(a.target, a.attr, val);
			}
		}
		for (auto& kv : xf)
			anim::set_attr(kv.first, "transform", kv.second);

		// narrow ostream 으로 save → WCHAR 트리를 UTF-8 바이트로 인코딩(indent 는 char_t* 라 L"").
		std::ostringstream ss;
		doc.save(ss, L"", pugi::format_raw);
		std::string svg = ss.str();

		std::vector<uint8_t> bgra;
		auto sub = lunasvg::Document::loadFromData(svg.data(), svg.size());
		if (sub)
		{
			lunasvg::Bitmap bmp = sub->renderToBitmap(w, h, 0x00000000);
			unpremultiply_to_bgra(bmp, bgra);
		}
		if (bgra.empty())
			bgra.assign((size_t)w * h * 4, 0);      // 실패 프레임은 투명으로
		frames.push_back(std::move(bgra));
	};

	// 프레임 0 을 굽고 실제 비용을 측정 → 총 베이킹 예산에 맞춰 프레임 수 N 을 적응 결정.
	// (무거운 SVG 는 lunasvg 재파싱·래스터가 프레임당 비싸므로 N 을 줄여 로딩 시간을 상한한다.)
	LARGE_INTEGER freq, c0, c1;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&c0);
	bake(0.0);
	QueryPerformanceCounter(&c1);
	const double frame_ms = (double)(c1.QuadPart - c0.QuadPart) * 1000.0 / (double)freq.QuadPart;

	int n_ideal = (int)std::lround(T * 25.0);       // 목표 25fps
	if (n_ideal < 2) n_ideal = 2;

	const double budget_ms = 1500.0;                // 총 베이킹 예산(첫 프레임 포함)
	int n_budget = (int)(budget_ms / (frame_ms > 0.5 ? frame_ms : 0.5)) + 1;
	if (n_budget < 2) n_budget = 2;

	int N = (n_ideal < n_budget) ? n_ideal : n_budget;
	if (N < 2)   N = 2;
	if (N > 150) N = 150;

	// 나머지(1..N-1)를 T 균등 샘플로. delay 는 T/N 로 보정해 총 재생시간=T 유지.
	for (int i = 1; i < N; ++i)
		bake((double)i * T / (double)N);

	const int delay = (int)std::lround(T * 1000.0 / (double)N);
	delays_ms.assign(frames.size(), delay);

	return frames.size() >= 2;
}
