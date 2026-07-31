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
#include <thread>
#include <atomic>

#include <Windows.h>
#include <dwrite.h>
#include <wincodec.h>      // 애니메이션 GIF 인코딩(WIC)
#include <wrl/client.h>
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "windowscodecs.lib")

// 애니메이션 WebP 인코딩(libwebp, x64 전용 lib). include dir/lib dir 는 vcxproj 에 이미 설정됨.
#ifdef _WIN64
#include <fstream>
#include <encode.h>
#include <mux.h>
#pragma comment(lib, "libwebp.lib")
#pragma comment(lib, "libwebpmux.lib")
#endif

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

		//20260731 by claude. attributeName="d"/"points" 애니메이션 보간용 부가 정보.
		//stops 에는 숫자만, path_cmds/path_ncounts 에 커맨드 시퀀스와 커맨드당 숫자 개수 저장.
		//모든 stop 의 (path_cmds[i], path_ncounts[i]) 가 동일할 때만 interp_ok=true → 숫자만 lerp.
		//시퀀스 다르면 SMIL 규격상 보간 불가 → discrete 폴백.
		bool                              is_path_data = false;   // attr == "d"
		bool                              is_points    = false;   // attr == "points"
		std::vector<std::vector<char>>    path_cmds;               // stop 별 커맨드 시퀀스(points 는 pseudo 'P')
		std::vector<std::vector<size_t>>  path_ncounts;            // stop 별 커맨드마다 딸린 숫자 개수
	};

	//20260731 by claude. SVG path "d" / polyline "points" 를 (cmds, nums, ncounts) 로 파싱.
	//"M10,20 C1,2,3,4,5,6 Z" → cmds=['M','C','Z'], nums=[10,20,1,2,3,4,5,6], ncounts=[2,6,0].
	//반환 false 면 파싱 실패(호출부는 discrete 폴백).
	bool parse_path_d(const std::string& s,
					  std::vector<char>& cmds,
					  std::vector<double>& nums,
					  std::vector<size_t>& ncounts)
	{
		cmds.clear();
		nums.clear();
		ncounts.clear();

		const char* p   = s.c_str();
		const char* end = p + s.size();
		size_t cur_count = 0;
		bool   have_cmd  = false;

		while (p < end)
		{
			while (p < end && (std::isspace((unsigned char)*p) || *p == ','))
				++p;
			if (p >= end) break;

			unsigned char ch = (unsigned char)*p;
			if (std::isalpha(ch))
			{
				if (have_cmd)
					ncounts.push_back(cur_count);
				cmds.push_back((char)ch);
				cur_count = 0;
				have_cmd  = true;
				++p;
			}
			else if (std::isdigit(ch) || ch == '.' || ch == '-' || ch == '+')
			{
				if (!have_cmd) return false;
				char* e = nullptr;
				double v = std::strtod(p, &e);
				if (e == p) return false;
				nums.push_back(v);
				++cur_count;
				p = e;
			}
			else
			{
				++p;   // 알 수 없는 문자는 스킵(관대)
			}
		}
		if (have_cmd) ncounts.push_back(cur_count);
		return !cmds.empty();
	}

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
		//20260731 by claude. path/points: path_cmds[0] + ncounts[0] 로 재조립. 모든 stop 이 같은 시퀀스임이 parse_anim 에서 보장.
		if ((a.is_path_data || a.is_points) && !a.path_cmds.empty())
		{
			std::string s;
			size_t idx = 0;
			for (size_t k = 0; k < a.path_cmds[0].size(); ++k)
			{
				if (a.is_path_data)
				{
					if (!s.empty()) s += ' ';
					s += a.path_cmds[0][k];
				}
				for (size_t j = 0; j < a.path_ncounts[0][k]; ++j)
				{
					if (idx >= v.size()) break;
					if (!s.empty()) s += ' ';
					s += num_to_str(v[idx++]);
				}
			}
			return s;
		}
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
		//20260731 by claude. "d","points" 는 아래 path 분기에서 처리(커맨드 시퀀스 동일 시 숫자만 lerp).
		//discrete_attrs 는 순수 이산 attr 만 (수치 보간이 개념적으로 불가능한 것).
		static const std::set<std::string> discrete_attrs =
			{ "visibility","display","transform-origin" };
		static const std::set<std::string> path_data_attrs = { "d", "points" };

		a.is_color = color_attrs.count(a.attr) > 0;

		if (a.is_set || discrete_attrs.count(a.attr) > 0)
		{
			if (!a.is_set) a.discrete = true;
			a.interp_ok = false;                    // raw 이산 사용
		}
		//20260731 by claude. path d / polyline points: 각 stop 을 (cmds, nums, ncounts) 로 파싱.
		//모든 stop 이 같은 커맨드 시퀀스 + 카운트면 숫자만 lerp(interp_ok=true), 아니면 discrete 폴백.
		//SMIL 규격도 path 보간은 이 조건을 요구.
		else if (path_data_attrs.count(a.attr) > 0)
		{
			a.is_path_data = (a.attr == "d");
			a.is_points    = (a.attr == "points");
			a.interp_ok    = true;
			for (const auto& s : raws)
			{
				std::vector<char>   cmds;
				std::vector<double> nums;
				std::vector<size_t> ncnt;
				if (a.is_points)
				{
					// points 는 커맨드 없이 x,y 페어. pseudo 커맨드 'P' 하나로 통일.
					nums = parse_num_list(s);
					if (nums.empty()) { a.interp_ok = false; break; }
					cmds.push_back('P');
					ncnt.push_back(nums.size());
				}
				else
				{
					if (!parse_path_d(s, cmds, nums, ncnt)) { a.interp_ok = false; break; }
				}
				if (!a.path_cmds.empty() && (cmds != a.path_cmds[0] || ncnt != a.path_ncounts[0]))
				{
					a.interp_ok = false;   // 시퀀스 불일치 → 보간 불가
					break;
				}
				a.path_cmds.push_back(std::move(cmds));
				a.path_ncounts.push_back(std::move(ncnt));
				a.stops.push_back(std::move(nums));
			}
			if (!a.interp_ok)
			{
				// discrete 폴백: raw 문자열 그대로 이산 재생.
				a.discrete = true;
				a.stops.clear();
				a.path_cmds.clear();
				a.path_ncounts.clear();
				a.is_path_data = false;
				a.is_points    = false;
			}
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

bool sc_svg::build_frames_stream(int w, int h, int fps, double budget_ms,
	const std::function<bool(int, int, const std::vector<uint8_t>&, int)>& sink) const
{
	if (m_svg_data.empty() || w <= 0 || h <= 0 || !sink)
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
	if (T < 0.2)  T = 0.2;
	if (T > 30.0) T = 30.0;                      // 상한(과대 주기 방지). 흔한 10~15s 루프는 온전히 굽는다.
												 // (예전 8s 클램프가 dur=10s SVG 의 뒤 2s(끝 패널 등장)를 잘라 버그였음)

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

	// 한 프레임을 굽는다: base 복원 → t 값 주입 → 직렬화 → lunasvg 래스터 → out(straight BGRA).
	auto bake_to = [&](double t, std::vector<uint8_t>& out)
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

		out.clear();
		auto sub = lunasvg::Document::loadFromData(svg.data(), svg.size());
		if (sub)
		{
			lunasvg::Bitmap bmp = sub->renderToBitmap(w, h, 0x00000000);
			unpremultiply_to_bgra(bmp, out);
		}
		if (out.empty())
			out.assign((size_t)w * h * 4, 0);       // 실패 프레임은 투명으로
	};

	// 프레임 0 을 굽고 실제 비용을 측정.
	std::vector<uint8_t> buf;
	LARGE_INTEGER freq, c0, c1;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&c0);
	bake_to(0.0, buf);
	QueryPerformanceCounter(&c1);
	const double frame_ms = (double)(c1.QuadPart - c0.QuadPart) * 1000.0 / (double)freq.QuadPart;

	int f = fps; if (f < 1) f = 1;
	int n_ideal = (int)std::lround(T * (double)f);
	if (n_ideal < 2) n_ideal = 2;

	int N;
	if (budget_ms > 0.0)
	{
		// 화면 표시용: 무거운 SVG 는 프레임당 비싸므로 총 예산에 맞춰 프레임 수를 감축.
		int n_budget = (int)(budget_ms / (frame_ms > 0.5 ? frame_ms : 0.5)) + 1;
		if (n_budget < 2) n_budget = 2;
		N = (n_ideal < n_budget) ? n_ideal : n_budget;
		if (N > 150) N = 150;
	}
	else
	{
		// export 용: 예산 캡 없이 round(T*fps). 상한만 둔다(8s*50fps=400 여유).
		N = n_ideal;
		if (N > 600) N = 600;
	}
	if (N < 2) N = 2;

	// delay 는 T/N 로 보정해 총 재생시간=T 유지.
	const int delay = (int)std::lround(T * 1000.0 / (double)N);

	// 프레임 0 은 이미 buf 에 있음 → 곧바로 sink.
	if (!sink(0, N, buf, delay))
		return false;
	for (int i = 1; i < N; ++i)
	{
		bake_to((double)i * T / (double)N, buf);
		if (!sink(i, N, buf, delay))
			return false;
	}
	return true;
}

//20260731 by claude. 화면 표시용 병렬 베이킹.
//stage 1(순차): pugi doc 를 mutate 해 N개 프레임 SVG 문자열을 생성. doc 공유 mutate 는 스레드 안전 X 라 순차 필수.
//stage 2(병렬): 각 워커가 자기 프레임의 svg 문자열로 lunasvg::Document 를 새로 파싱하고 renderToBitmap → un-premultiply.
//병렬 안전 근거: 서로 다른 Document 인스턴스 → renderToBitmap 은 자기 m_rootElement 만 mutate.
//폰트 캐시(plutovg_font_face_cache_t) 는 내부 CRITICAL_SECTION 으로 자체 보호(plutovg-font.c:521,652 등).
//프레임 0 은 순차로 굽고 실측(frame_ms) → 병렬 예산 반영해 N 을 상향(순차 대비 fps 복원).
bool sc_svg::build_frames(int w, int h,
						  std::vector<std::vector<uint8_t>>& frames,
						  std::vector<int>& delays_ms) const
{
	frames.clear();
	delays_ms.clear();

	if (m_svg_data.empty() || w <= 0 || h <= 0)
		return false;

	pugi::xml_document doc;
	if (!doc.load_buffer(m_svg_data.data(), m_svg_data.size()))
		return false;
	std::vector<anim::animation> anims;
	anim::collect(doc, doc, anims);
	if (anims.empty())
		return false;

	// 전체 주기 T — build_frames_stream 과 동일한 산정/클램프.
	double T = 0.0;
	for (const auto& a : anims)
	{
		double cyc = a.is_set ? a.begin
				   : (a.indefinite ? (a.begin + a.dur) : (a.begin + a.dur * a.repeat));
		if (cyc > T) T = cyc;
	}
	if (T <= 1e-6)
		return false;
	if (T < 0.2)  T = 0.2;
	if (T > 30.0) T = 30.0;

	// 애니메이션이 건드리는 (대상,속성)의 원본 값 스냅샷. 프레임마다 이것으로 복원 후 t 값을 주입.
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

	// stage 1: 시각 t 의 SVG 문자열 생성. doc 를 공유 mutate 하므로 반드시 순차.
	auto make_svg_at = [&](double t) -> std::string
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
		std::map<pugi::xml_node, std::string> xf;
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

		std::ostringstream ss;
		doc.save(ss, L"", pugi::format_raw);
		return ss.str();
	};

	// stage 2: SVG 문자열 → 새 Document → renderToBitmap → un-premul. 프레임 로컬이라 스레드 안전.
	auto render_svg = [w, h](const std::string& svg, std::vector<uint8_t>& out)
	{
		out.clear();
		auto sub = lunasvg::Document::loadFromData(svg.data(), svg.size());
		if (sub)
		{
			lunasvg::Bitmap bmp = sub->renderToBitmap(w, h, 0x00000000);
			unpremultiply_to_bgra(bmp, out);
		}
		if (out.empty())
			out.assign((size_t)w * h * 4, 0);       // 실패 프레임은 투명
	};

	// 프레임 0 을 순차로 굽고 실측. 텍스트 있는 SVG 는 이 순차 굽기에서 폰트 캐시가 예열된다.
	std::string svg0 = make_svg_at(0.0);
	std::vector<uint8_t> frame0;
	LARGE_INTEGER freq, c0, c1;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&c0);
	render_svg(svg0, frame0);
	QueryPerformanceCounter(&c1);
	const double frame_ms = (double)(c1.QuadPart - c0.QuadPart) * 1000.0 / (double)freq.QuadPart;

	// 워커 수 P — 하드웨어 병렬성 상한. 지나친 스레드 생성 오버헤드 억제.
	unsigned int P = std::thread::hardware_concurrency();
	if (P < 2) P = 2;
	if (P > 8) P = 8;

	// N 결정. 병렬 P 배 처리량을 예산에 반영해 순차 버전 대비 fps 를 복원한다.
	const int    fps       = 25;
	const double budget_ms = 1500.0;
	int n_ideal  = (int)std::lround(T * (double)fps);
	if (n_ideal < 2) n_ideal = 2;
	int n_budget = (int)((double)P * budget_ms / (frame_ms > 0.5 ? frame_ms : 0.5)) + 1;
	if (n_budget < 2) n_budget = 2;
	int N = (n_ideal < n_budget) ? n_ideal : n_budget;
	if (N > 150) N = 150;
	if (N < 2)   N = 2;

	// delay = T/N (총 재생시간 = T 유지).
	const int delay = (int)std::lround(T * 1000.0 / (double)N);

	// 나머지 프레임 SVG 문자열 순차 생성. doc mutate 는 여기서 완결 → stage 2 이후 doc 는 안 건드림.
	std::vector<std::string> svgs((size_t)N);
	svgs[0] = std::move(svg0);
	for (int i = 1; i < N; ++i)
		svgs[i] = make_svg_at((double)i * T / (double)N);

	// 결과 슬롯 사전 할당. 워커는 자기 인덱스 슬롯에만 쓴다(다른 슬롯 접근 없음).
	frames.assign((size_t)N, std::vector<uint8_t>());
	frames[0] = std::move(frame0);
	delays_ms.assign((size_t)N, delay);

	// 병렬 렌더 (프레임 1..N-1). 인덱스는 atomic fetch_add 로 work-stealing.
	if (N > 1)
	{
		std::atomic<int> next_idx(1);
		std::vector<std::thread> workers;
		workers.reserve(P);
		for (unsigned int wi = 0; wi < P; ++wi)
		{
			workers.emplace_back([&]()
			{
				while (true)
				{
					int i = next_idx.fetch_add(1);
					if (i >= N) break;
					render_svg(svgs[i], frames[i]);
				}
			});
		}
		for (auto& th : workers)
			th.join();
	}

	return frames.size() >= 2;
}

// ── SVG 애니메이션 → 애니메이션 GIF 내보내기(WIC) ────────────────────────────
// GIF 는 1비트 투명(반투명 없음)만 지원한다. straight BGRA 의 알파를 임계로 이진화해
// 팔레트의 투명 엔트리로 매핑하고, 프레임 메타데이터(TransparencyFlag/Index + Disposal=2)
// 로 투명을 보존한다. 안티에일리어스 경계의 반투명은 완전 투명/불투명으로 이진화되므로
// 경계가 다소 거칠 수 있다(포맷 한계). 매끈한 불투명이 필요하면 향후 배경색 합성 옵션 추가.

bool export_svg_to_animated_gif(const wchar_t* svg_path,
								const wchar_t* out_gif_path,
								int out_w, int out_h, int fps,
								std::function<bool(int cur, int total)> progress)
{
	if (!svg_path || !*svg_path)
		return false;

	// 출력 경로: 지정되면 그대로, 아니면 svg 와 같은 폴더·같은 이름 + .gif
	std::wstring gif;
	if (out_gif_path && *out_gif_path)
	{
		gif = out_gif_path;
	}
	else
	{
		gif = svg_path;
		size_t dot = gif.find_last_of(L'.');
		size_t sep = gif.find_last_of(L"\\/");
		if (dot != std::wstring::npos && (sep == std::wstring::npos || dot > sep))
			gif.erase(dot);
		gif += L".gif";
	}

	// 워커 스레드 안전: 표시용 인스턴스와 상태 공유 없이 새로 파싱.
	sc_svg svg;
	if (!svg.load(svg_path) || !svg.is_animated())
		return false;

	int w = (out_w > 0) ? out_w : (int)std::lround(svg.natural_width());
	int h = (out_h > 0) ? out_h : (int)std::lround(svg.natural_height());
	if (w < 1) w = 512;
	if (h < 1) h = 512;

	int f = fps; if (f < 1) f = 1; if (f > 50) f = 50;   // GIF delay 최소 2cs → 실효 50fps 상한

	// COM(워커 스레드). 이미 초기화돼 있으면(S_FALSE) 짝 맞춰 Uninit, 모드 충돌이면 그대로 사용.
	HRESULT hrco = ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	const bool need_uninit = SUCCEEDED(hrco);

	bool ok = false;
	{
		ComPtr<IWICImagingFactory> factory;
		ComPtr<IWICStream>         stream;
		ComPtr<IWICBitmapEncoder>  encoder;

		HRESULT hr = ::CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
										IID_PPV_ARGS(&factory));
		if (SUCCEEDED(hr)) hr = factory->CreateStream(&stream);
		if (SUCCEEDED(hr)) hr = stream->InitializeFromFilename(gif.c_str(), GENERIC_WRITE);
		if (SUCCEEDED(hr)) hr = factory->CreateEncoder(GUID_ContainerFormatGif, nullptr, &encoder);
		if (SUCCEEDED(hr)) hr = encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache);

		// 전역 메타데이터: NETSCAPE2.0 application extension → 무한 루프.
		if (SUCCEEDED(hr))
		{
			ComPtr<IWICMetadataQueryWriter> gw;
			if (SUCCEEDED(encoder->GetMetadataQueryWriter(&gw)))
			{
				PROPVARIANT app;  app.vt = VT_UI1 | VT_VECTOR;
				app.caub.cElems = 11; app.caub.pElems = (UCHAR*)"NETSCAPE2.0";
				gw->SetMetadataByName(L"/appext/Application", &app);

				UCHAR loopbytes[] = { 3, 1, 0, 0, 0 };   // block=3, id=1, loop=0(무한), 종료
				PROPVARIANT dat;  dat.vt = VT_UI1 | VT_VECTOR;
				dat.caub.cElems = 5; dat.caub.pElems = loopbytes;
				gw->SetMetadataByName(L"/appext/Data", &dat);
				// pElems 는 리터럴/스택 → PropVariantClear 호출 안 함.
			}
		}

		if (SUCCEEDED(hr))
		{
			bool cancelled = false, frame_err = false;

			svg.build_frames_stream(w, h, f, 0.0,
				[&](int idx, int total, const std::vector<uint8_t>& bgra, int delay_ms) -> bool
				{
					// straight BGRA(알파 유지) 복사 → WIC 소스.
					std::vector<uint8_t> px = bgra;

					ComPtr<IWICBitmap> src;
					HRESULT fh = factory->CreateBitmapFromMemory(
						w, h, GUID_WICPixelFormat32bppBGRA, w * 4,
						(UINT)px.size(), px.data(), &src);

					// 팔레트: 255 색 + 투명 엔트리 1개(fAddTransparentColor=TRUE).
					ComPtr<IWICPalette> pal;
					if (SUCCEEDED(fh)) fh = factory->CreatePalette(&pal);
					if (SUCCEEDED(fh)) fh = pal->InitializeFromBitmap(src.Get(), 255, TRUE);

					// 투명 인덱스(알파 0 엔트리)를 찾아 프레임 메타데이터에 지정한다.
					int trans_idx = -1;
					if (SUCCEEDED(fh))
					{
						UINT cc = 0; pal->GetColorCount(&cc);
						if (cc > 0)
						{
							std::vector<WICColor> cols(cc);
							UINT got = 0;
							if (SUCCEEDED(pal->GetColors(cc, cols.data(), &got)))
								for (UINT i = 0; i < got; ++i)
									if ((cols[i] >> 24) == 0) { trans_idx = (int)i; break; }
						}
					}

					// alphaThresholdPercent=50: 알파가 낮은 픽셀은 위 투명색으로 매핑된다.
					// 디더링 None: 아이콘/일러스트 SVG 는 평면 색이라 error-diffusion 이 매끈한
					// 면에 점 노이즈를 뿌린다(작게 낼수록 도드라짐). nearest-color 로 깨끗하게.
					// (그라데이션은 약간 밴딩될 수 있으나 점 노이즈보다 자연스럽다.)
					ComPtr<IWICFormatConverter> conv;
					if (SUCCEEDED(fh)) fh = factory->CreateFormatConverter(&conv);
					if (SUCCEEDED(fh)) fh = conv->Initialize(src.Get(),
						GUID_WICPixelFormat8bppIndexed, WICBitmapDitherTypeNone,
						pal.Get(), 50.0, WICBitmapPaletteTypeCustom);

					ComPtr<IWICBitmapFrameEncode> frame;
					ComPtr<IPropertyBag2>         propbag;
					if (SUCCEEDED(fh)) fh = encoder->CreateNewFrame(&frame, &propbag);
					if (SUCCEEDED(fh)) fh = frame->Initialize(propbag.Get());
					if (SUCCEEDED(fh)) fh = frame->SetSize(w, h);
					if (SUCCEEDED(fh))
					{
						WICPixelFormatGUID pf = GUID_WICPixelFormat8bppIndexed;
						fh = frame->SetPixelFormat(&pf);
					}
					if (SUCCEEDED(fh)) fh = frame->SetPalette(pal.Get());

					// 프레임 메타데이터: delay(centisecond), disposal.
					if (SUCCEEDED(fh))
					{
						ComPtr<IWICMetadataQueryWriter> fw;
						if (SUCCEEDED(frame->GetMetadataQueryWriter(&fw)))
						{
							int cs = (int)std::lround(delay_ms / 10.0);
							if (cs < 2) cs = 2;                 // 2cs 미만은 뷰어가 10cs 로 뭉갬
							PROPVARIANT d; d.vt = VT_UI2; d.uiVal = (USHORT)cs;
							fw->SetMetadataByName(L"/grctlext/Delay", &d);
							// disposal 2 = restore to background: 다음 프레임 전 투명 배경으로
							// 복원 → 투명 영역이 이전 프레임을 덮지 않아 잔상 없음.
							PROPVARIANT dp; dp.vt = VT_UI1; dp.bVal = 2;
							fw->SetMetadataByName(L"/grctlext/Disposal", &dp);
							if (trans_idx >= 0)
							{
								PROPVARIANT tf; tf.vt = VT_BOOL; tf.boolVal = VARIANT_TRUE;
								fw->SetMetadataByName(L"/grctlext/TransparencyFlag", &tf);
								PROPVARIANT ti; ti.vt = VT_UI1; ti.bVal = (BYTE)trans_idx;
								fw->SetMetadataByName(L"/grctlext/TransparentColorIndex", &ti);
							}
						}
					}

					if (SUCCEEDED(fh)) fh = frame->WriteSource(conv.Get(), nullptr);
					if (SUCCEEDED(fh)) fh = frame->Commit();

					if (FAILED(fh)) { frame_err = true; return false; }

					if (progress && !progress(idx + 1, total)) { cancelled = true; return false; }
					return true;
				});

			if (!frame_err && !cancelled)
				ok = SUCCEEDED(encoder->Commit());
		}
	}   // ComPtr 들 여기서 Release(스트림이 파일 핸들을 놓음) → 실패 시 파일 삭제 가능

	if (!ok)
		::DeleteFileW(gif.c_str());   // 취소/실패 시 부분 파일 제거

	if (need_uninit)
		::CoUninitialize();

	return ok;
}

// ── SVG 애니메이션 → 애니메이션 WebP 내보내기(libwebp) ──────────────────────
bool export_svg_to_animated_webp(const wchar_t* svg_path,
								 const wchar_t* out_webp_path,
								 int out_w, int out_h, int fps,
								 std::function<bool(int cur, int total)> progress)
{
#ifndef _WIN64
	(void)svg_path; (void)out_webp_path; (void)out_w; (void)out_h; (void)fps; (void)progress;
	return false;   // libwebp lib 은 x64 전용
#else
	if (!svg_path || !*svg_path)
		return false;

	// 출력 경로: 지정되면 그대로, 아니면 svg 와 같은 폴더·같은 이름 + .webp
	std::wstring webp;
	if (out_webp_path && *out_webp_path)
	{
		webp = out_webp_path;
	}
	else
	{
		webp = svg_path;
		size_t dot = webp.find_last_of(L'.');
		size_t sep = webp.find_last_of(L"\\/");
		if (dot != std::wstring::npos && (sep == std::wstring::npos || dot > sep))
			webp.erase(dot);
		webp += L".webp";
	}

	sc_svg svg;
	if (!svg.load(svg_path) || !svg.is_animated())
		return false;

	int w = (out_w > 0) ? out_w : (int)std::lround(svg.natural_width());
	int h = (out_h > 0) ? out_h : (int)std::lround(svg.natural_height());
	if (w < 1) w = 512;
	if (h < 1) h = 512;

	int f = fps; if (f < 1) f = 1;

	WebPAnimEncoderOptions enc_options;
	if (!WebPAnimEncoderOptionsInit(&enc_options))
		return false;
	enc_options.anim_params.loop_count = 0;      // 무한 루프

	WebPAnimEncoder* enc = WebPAnimEncoderNew(w, h, &enc_options);
	if (!enc)
		return false;

	WebPConfig config;
	if (!WebPConfigInit(&config)) { WebPAnimEncoderDelete(enc); return false; }
	config.lossless = 1;        // 무손실(알파·풀컬러 보존, 디더/밴딩 없음)
	config.quality  = 90.0f;    // lossless 에서는 압축 노력(높을수록 작고 느림)
	config.method   = 4;
	if (!WebPValidateConfig(&config)) { WebPAnimEncoderDelete(enc); return false; }

	bool cancelled = false, frame_err = false;
	int  timestamp = 0;         // 각 프레임의 시작 시각(ms), 누적

	svg.build_frames_stream(w, h, f, 0.0,
		[&](int idx, int total, const std::vector<uint8_t>& bgra, int delay_ms) -> bool
		{
			WebPPicture pic;
			if (!WebPPictureInit(&pic)) { frame_err = true; return false; }
			pic.use_argb = 1;   // lossless 는 argb 입력 필요
			pic.width  = w;
			pic.height = h;
			// 프레임은 straight BGRA — WebP 가 기대하는 포맷 그대로.
			if (!WebPPictureImportBGRA(&pic, bgra.data(), w * 4)) { frame_err = true; return false; }

			if (!WebPAnimEncoderAdd(enc, &pic, timestamp, &config))
			{
				WebPPictureFree(&pic);
				frame_err = true;
				return false;
			}
			WebPPictureFree(&pic);
			timestamp += delay_ms;

			if (progress && !progress(idx + 1, total)) { cancelled = true; return false; }
			return true;
		});

	bool ok = false;
	if (!frame_err && !cancelled)
	{
		// 마지막 프레임 duration 반영을 위해 NULL 프레임을 총 재생시간(timestamp)에 add.
		if (WebPAnimEncoderAdd(enc, nullptr, timestamp, nullptr))
		{
			WebPData data; WebPDataInit(&data);
			if (WebPAnimEncoderAssemble(enc, &data) && data.bytes && data.size)
			{
				std::ofstream ofs(webp.c_str(), std::ios::binary);
				if (ofs)
				{
					ofs.write((const char*)data.bytes, (std::streamsize)data.size);
					ok = (bool)ofs;
				}
			}
			WebPDataClear(&data);
		}
	}

	WebPAnimEncoderDelete(enc);

	if (!ok)
		::DeleteFileW(webp.c_str());   // 취소/실패 시 부분 파일 제거

	return ok;
#endif
}
