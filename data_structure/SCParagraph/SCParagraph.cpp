#include "SCParagraph.h"
#include "../../Functions.h"
#include "../../SCGdiplusBitmap.h"

#include <map>
#include <mutex>

namespace
{
	//AA 임계치 관련 캐시·override 맵. UI 스레드 외부 호출 가능성을 대비해 mutex 로 보호.
	std::map<CString, int>&	aa_override_map() { static std::map<CString, int> m; return m; }
	std::map<CString, int>&	aa_cache_map()    { static std::map<CString, int> m; return m; }
	std::mutex&				aa_maps_mutex()   { static std::mutex mx; return mx; }

	//다크 배경 보정 — 0 = 비활성.
	//최초 가설: "다크 배경에서는 ClearType subpixel fringe 가 거슬리니 grayscale AA 가 깔끔" → 임계치 감산.
	//실측 결과: 한글 작은 글씨(9pt 본문 등) 는 자모의 직선/가로획이 픽셀 grid 에 정확히 떨어지는
	//ClearTypeGridFit 의 hinting 이 결정적이라 다크여도 ClearType 이 더 또렷.
	//→ 보정 무력화. 다크/라이트 동일 임계치. 다크에서만 임계치를 *높이고* 싶으면 음수 사용 가능 (구현 그대로).
	constexpr int AA_DARK_BOOST = 0;

	//TrueType table tag 를 Win32 GetFontData 가 받는 little-endian 32bit 로 변환.
	//(TTF spec 은 4-byte ASCII big-endian, Win32 는 그 바이트 순서를 뒤집어 받는다.)
	constexpr DWORD make_tt_tag(char a, char b, char c, char d)
	{
		return ((DWORD)(BYTE)a) | ((DWORD)(BYTE)b << 8) | ((DWORD)(BYTE)c << 16) | ((DWORD)(BYTE)d << 24);
	}

	//폰트의 임베디드 비트맵 strike 보유 PPEM 최대값(px) 측정.
	// 0   = strike 미보유 (순수 outline — 전 크기 AA 가 또렷)
	//>0   = strike 의 max ppemY
	//-1   = GDI 호출 실패 (폴리시 fallback 으로 호출자가 대응)
	int probe_max_embedded_bitmap_ppem(LPCTSTR face)
	{
		if (face == NULL || *face == 0) return -1;

		HDC hdc = ::CreateCompatibleDC(NULL);
		if (!hdc) return -1;

		LOGFONT lf = {};
		lf.lfHeight = -16;	//strike 메트릭은 size 무관 — 임의 값. CHARSET 도 마찬가지.
		lf.lfCharSet = DEFAULT_CHARSET;
		_tcscpy_s(lf.lfFaceName, _countof(lf.lfFaceName), face);
		HFONT hf = ::CreateFontIndirect(&lf);
		if (!hf) { ::DeleteDC(hdc); return -1; }
		HFONT hf_old = (HFONT)::SelectObject(hdc, hf);

		int result = 0;	//기본 = strike 없음

		//EBLC (OT/TT 표준 한글 비트맵 포함) 와 bloc (Apple variant) 둘 다 시도.
		const DWORD tags[] = { make_tt_tag('E','B','L','C'), make_tt_tag('b','l','o','c') };
		for (DWORD tag : tags)
		{
			DWORD size = ::GetFontData(hdc, tag, 0, NULL, 0);
			if (size == GDI_ERROR || size < 8) continue;

			BYTE* buf = new BYTE[size];
			if (::GetFontData(hdc, tag, 0, buf, size) == size)
			{
				//EBLC 헤더 (big-endian): version uint32 (offset 0) + numSizes uint32 (offset 4).
				//그 뒤 bitmapSize entry 가 48 bytes 씩. entry 의 ppemX (+44), ppemY (+45) 는 BYTE.
				DWORD numSizes = ((DWORD)buf[4] << 24) | ((DWORD)buf[5] << 16) | ((DWORD)buf[6] << 8) | (DWORD)buf[7];
				if ((unsigned long long)8 + (unsigned long long)numSizes * 48 <= size)
				{
					int max_ppem = 0;
					for (DWORD i = 0; i < numSizes; i++)
					{
						BYTE ppemY = buf[8 + i * 48 + 45];
						if (ppemY > max_ppem) max_ppem = ppemY;
					}
					if (max_ppem > result) result = max_ppem;
				}
			}
			delete[] buf;
		}

		::SelectObject(hdc, hf_old);
		::DeleteObject(hf);
		::DeleteDC(hdc);
		return result;
	}
}

CSCParagraph::CSCParagraph()
{

}

CSCParagraph::~CSCParagraph()
{
}

//<style=이름> 으로 참조할 속성 묶음. 태그를 길게 나열하는 대신 이름 하나로 적용한다.
static std::map<CString, CSCTextProperty>& style_map()
{
	static std::map<CString, CSCTextProperty> m;
	return m;
}

void CSCParagraph::register_style(LPCTSTR name, const CSCTextProperty& prop)
{
	CString key(name);
	key.MakeLower();
	style_map()[key] = prop;
}

void CSCParagraph::clear_styles()
{
	style_map().clear();
}

//<img=별칭> 등록표. 값은 파일 경로 또는 "#타입:id" / "#id" 형태의 리소스 스펙.
static std::map<CString, CString>& image_alias_map()
{
	static std::map<CString, CString> m;
	return m;
}

//로드 결과 캐시. 매 렌더마다 디코드하지 않도록 스펙 문자열을 키로 보관한다.
//로드 실패도 빈 unique_ptr 로 캐시해 실패한 경로를 반복해서 재시도하지 않는다.
//raw 포인터로 담으면 map 소멸자가 노드만 지우고 비트맵은 남겨 프로그램 종료 시 leak 으로 잡힌다.
static std::map<CString, std::unique_ptr<CSCGdiplusBitmap>>& image_cache()
{
	static std::map<CString, std::unique_ptr<CSCGdiplusBitmap>> m;
	return m;
}

void CSCParagraph::register_image(LPCTSTR name, LPCTSTR file)
{
	CString key(name);
	key.MakeLower();
	image_alias_map()[key] = file;
}

void CSCParagraph::register_image(LPCTSTR name, LPCTSTR res_type, UINT res_id)
{
	CString key(name);
	key.MakeLower();

	CString spec;
	spec.Format(_T("#%s:%u"), res_type, res_id);
	image_alias_map()[key] = spec;
}

void CSCParagraph::clear_images()
{
	image_cache().clear();
	image_alias_map().clear();
}

CSCGdiplusBitmap* CSCParagraph::get_image(LPCTSTR key)
{
	CString k(key);
	if (k.IsEmpty())
		return NULL;

	//별칭으로 등록돼 있으면 실제 파일 경로 / 리소스 스펙으로 치환한다.
	CString lower = k;
	lower.MakeLower();

	auto alias = image_alias_map().find(lower);
	CString spec = (alias != image_alias_map().end()) ? alias->second : k;

	auto cached = image_cache().find(spec);
	if (cached != image_cache().end())
		return cached->second.get();

	std::unique_ptr<CSCGdiplusBitmap> img = std::make_unique<CSCGdiplusBitmap>();
	bool ok = false;

	if (spec[0] == _T('#'))
	{
		//"#PNG:142" = 타입 지정 리소스, "#142" = 타입 없이 id 만.
		CString body = spec.Mid(1);
		int colon = body.Find(_T(':'));

		if (colon >= 0)
		{
			CString res_type = body.Left(colon);
			UINT    res_id = (UINT)_ttoi(body.Mid(colon + 1));

			//ICON 은 커스텀 데이터 리소스가 아니라 GDI 아이콘 리소스라 전용 로더가 필요하다.
			//48px 로 뽑아 두고 배치 단계에서 요청 높이로 축소한다 (인라인 아이콘은 대개 그보다 작다).
			if (res_type.CompareNoCase(_T("ICON")) == 0)
				ok = img->load_icon(res_id, 48);
			else
				ok = img->load(res_type, res_id);
		}
		else
		{
			ok = img->load((UINT)_ttoi(body));
		}
	}
	else
	{
		ok = img->load(spec);
	}

	if (!ok)
		img.reset();

	CSCGdiplusBitmap* result = img.get();
	image_cache()[spec] = std::move(img);
	return result;
}

//text의 태그를 파싱하여 각 구문의 속성을 설정한 후 para에 저장한다.
//cr_text, cr_back은 글자, 배경 기본값
void CSCParagraph::build_paragraph_str(CString& text, std::deque<std::deque<CSCParagraph>>& para, CSCTextProperty* text_prop)
{
	std::deque<CString> tags;

	//"<b><cr=red>This</b></cr> is a <cr=blue><i>sample</i> <b>paragraph</b>."
	get_tag_str(text, tags);

	CSCParagraph basic_para, para_temp;
	basic_para.text_prop = *text_prop;
	para_temp = basic_para;

	std::deque<CSCParagraph> para_line;

	//라인 단위 태그(<ls> <vsp> <al> <la> <indent> <hang>)의 값은 라인이 확정될 때 그 라인의 "모든" run 에 찍는다.
	//run[0] 에만 넣으면 word-wrap 이 라인을 다시 쪼갤 때 대표 run 이 바뀌면서 값이 사라진다.
	float pending_line_spacing = -1.0f;
	float pending_line_vspace = -1.0f;
	DWORD pending_h_align = (DWORD)-1;
	DWORD pending_v_align = DT_TOP;
	float pending_indent = 0.0f;
	float pending_hang = 0.0f;

	//바로 다음 run 하나에만 적용되는 값.
	int     pending_tab_x = -1;
	CString pending_id;
	CString pending_ruby;

	//태그별 속성 스택. 닫는 태그가 "자기 그룹의 필드만" 되돌리므로
	//<b><cr=red>This</b></cr> 처럼 교차 중첩된 기존 문서도 그대로 동작하면서
	//<cr=red>바깥<cr=blue>안쪽</cr>다시바깥</cr> 같은 정상 중첩도 이제 맞게 복원된다.
	std::map<CString, std::deque<CSCTextProperty>> stacks;

	auto push_attr = [&](LPCTSTR key) { stacks[key].push_back(para_temp.text_prop); };
	auto pop_attr = [&](LPCTSTR key) -> CSCTextProperty
	{
		auto it = stacks.find(key);
		if (it == stacks.end() || it->second.empty())
			return basic_para.text_prop;	//짝 없는 닫는 태그 — 기존 동작대로 기본값 복귀.

		CSCTextProperty prev = it->second.back();
		it->second.pop_back();
		return prev;
	};

	//"<name=value>" / "<name>" / "</name>" 에서 name 과 value 를 분리.
	auto get_tag_name = [](const CString& tag) -> CString
	{
		int eq = tag.Find(_T('='));
		int end = (eq >= 0) ? eq : tag.GetLength() - 1;
		if (end < 1)
			return _T("");
		return tag.Mid(1, end - 1);
	};
	auto get_tag_value = [](const CString& tag) -> CString
	{
		int eq = tag.Find(_T('='));
		if (eq < 0)
			return _T("");
		return tag.Mid(eq + 1, tag.GetLength() - eq - 2);
	};
	//"a,b,c" 의 index 번째 인자. 없으면 빈 문자열.
	auto arg = [](const CString& value, int index) -> CString
	{
		int start = 0;
		for (int n = 0; ; n++)
		{
			int comma = value.Find(_T(','), start);
			CString token = (comma < 0) ? value.Mid(start) : value.Mid(start, comma - start);
			if (n == index)
				return token;
			if (comma < 0)
				return _T("");
			start = comma + 1;
		}
	};

	auto flush_line = [&](bool make_empty_line)
	{
		if (para_line.empty())
		{
			//<br>에 의해 공백 라인이 추가된 경우
			if (!make_empty_line)
				return;
			para_line.push_back(basic_para);
		}

		for (auto& run : para_line)
		{
			run.line_spacing = pending_line_spacing;
			run.line_vspace = pending_line_vspace;
			run.line_h_align = pending_h_align;
			run.line_align = pending_v_align;
			run.line_indent = pending_indent;
			run.line_hang = pending_hang;
		}

		para.push_back(para_line);
		para_line.clear();

		pending_line_spacing = -1.0f;
		pending_line_vspace = -1.0f;
		pending_h_align = (DWORD)-1;
		pending_v_align = DT_TOP;
		pending_indent = 0.0f;
		pending_hang = 0.0f;
	};

	//시작 태그를 만나면 속성을 세팅하고
	//단순 텍스트를 만나면 해당 속성과 함께 paragraph로 push하고
	//끝 태그를 만나면 그 태그가 담당하는 속성만 직전 값으로 되돌린다.
	for (int i = 0; i < (int)tags.size(); i++)
	{
		const CString& tag = tags[i];

		CString name, value;
		if (tag.GetLength() >= 3 && tag[0] == _T('<') && tag[tag.GetLength() - 1] == _T('>'))
		{
			name = get_tag_name(tag);
			name.MakeLower();
			value = get_tag_value(tag);
		}

		//---- 글꼴 스타일 (누적 비트라 스택 불필요) ----
		if (name == _T("b"))				para_temp.text_prop.style |= Gdiplus::FontStyleBold;
		else if (name == _T("/b"))			para_temp.text_prop.style &= ~Gdiplus::FontStyleBold;
		else if (name == _T("i"))			para_temp.text_prop.style |= Gdiplus::FontStyleItalic;
		else if (name == _T("/i"))			para_temp.text_prop.style &= ~Gdiplus::FontStyleItalic;
		else if (name == _T("u"))			para_temp.text_prop.style |= Gdiplus::FontStyleUnderline;
		else if (name == _T("/u"))			para_temp.text_prop.style &= ~Gdiplus::FontStyleUnderline;
		else if (name == _T("s"))			para_temp.text_prop.style |= Gdiplus::FontStyleStrikeout;
		else if (name == _T("/s"))			para_temp.text_prop.style &= ~Gdiplus::FontStyleStrikeout;

		//---- 글자색 / 배경색 ----
		else if (name == _T("cr") || name == _T("ct"))
		{
			push_attr(_T("cr"));
			para_temp.text_prop.cr_text = get_color(value);
		}
		else if (name == _T("/cr") || name == _T("/ct"))
		{
			para_temp.text_prop.cr_text = pop_attr(_T("cr")).cr_text;
		}
		else if (name == _T("cb") || name == _T("crb"))
		{
			push_attr(_T("cb"));
			para_temp.text_prop.cr_back = get_color(value);
		}
		else if (name == _T("/cb") || name == _T("/crb"))
		{
			para_temp.text_prop.cr_back = pop_attr(_T("cb")).cr_back;
		}

		//---- 폰트 이름 / 크기 ----
		else if (name == _T("f") || name == _T("font") || name == _T("name") || name == _T("fontname"))
		{
			push_attr(_T("f"));
			_tcscpy_s(para_temp.text_prop.name, value);
		}
		else if (name == _T("/f") || name == _T("/font") || name == _T("/name") || name == _T("/fontname"))
		{
			CSCTextProperty prev = pop_attr(_T("f"));
			_tcscpy_s(para_temp.text_prop.name, prev.name);
		}
		else if (name == _T("sz") || name == _T("size") || name == _T("fontsize"))
		{
			push_attr(_T("sz"));
			para_temp.text_prop.size = (float)_tstof(value);
		}
		else if (name == _T("/sz") || name == _T("/size") || name == _T("/fontsize"))
		{
			para_temp.text_prop.size = pop_attr(_T("sz")).size;
		}

		//---- 외곽선 ----
		else if (name == _T("st") || name == _T("stroke"))
		{
			push_attr(_T("st"));
			para_temp.text_prop.thickness = (float)_tstof(value);
		}
		else if (name == _T("/st") || name == _T("/stroke"))
		{
			para_temp.text_prop.thickness = pop_attr(_T("st")).thickness;
		}
		else if (name == _T("cs"))
		{
			push_attr(_T("cs"));
			para_temp.text_prop.cr_stroke = get_color(value);
		}
		else if (name == _T("/cs"))
		{
			para_temp.text_prop.cr_stroke = pop_attr(_T("cs")).cr_stroke;
		}

		//---- 그림자 ----
		else if (name == _T("sd") || name == _T("shadow"))
		{
			push_attr(_T("sd"));
			para_temp.text_prop.shadow_depth = (float)_tstof(value);
		}
		else if (name == _T("/sd") || name == _T("/shadow"))
		{
			para_temp.text_prop.shadow_depth = pop_attr(_T("sd")).shadow_depth;
		}
		else if (name == _T("csh"))
		{
			push_attr(_T("csh"));
			para_temp.text_prop.cr_shadow = get_color(value);
		}
		else if (name == _T("/csh"))
		{
			para_temp.text_prop.cr_shadow = pop_attr(_T("csh")).cr_shadow;
		}
		else if (name == _T("sb"))
		{
			push_attr(_T("sb"));
			para_temp.text_prop.shadow_blur_sigma = (float)_tstof(value);
		}
		else if (name == _T("/sb"))
		{
			para_temp.text_prop.shadow_blur_sigma = pop_attr(_T("sb")).shadow_blur_sigma;
		}

		//---- 그림자 목록 (CSS text-shadow 형식) ----
		//<ts=x,y,blur,색[,spread]; x,y,blur,색[,spread]; ...> — 세미콜론으로 여러 개를 쌓는다.
		//앞쪽 항목이 글자에 더 가깝다(CSS 와 동일). <sd>/<csh>/<sb>/<glow> 는 이것의 축약 표기다.
		else if (name == _T("ts"))
		{
			push_attr(_T("ts"));
			para_temp.text_prop.shadows.clear();

			int start = 0;
			while (start <= value.GetLength())
			{
				int semi = value.Find(_T(';'), start);
				CString item = (semi < 0) ? value.Mid(start) : value.Mid(start, semi - start);

				item.Trim();
				if (!item.IsEmpty())
				{
					CSCTextShadow s;
					s.dx = (float)_tstof(arg(item, 0));
					s.dy = (float)_tstof(arg(item, 1));
					s.blur = (float)_tstof(arg(item, 2));
					s.color = get_color(arg(item, 3));

					CString spread = arg(item, 4);
					if (!spread.IsEmpty())
						s.spread = (float)_tstof(spread);

					if (s.color.GetA() > 0)
						para_temp.text_prop.shadows.push_back(s);
				}

				if (semi < 0)
					break;
				start = semi + 1;
			}
		}
		else if (name == _T("/ts"))
		{
			para_temp.text_prop.shadows = pop_attr(_T("ts")).shadows;
		}

		//---- 외곽 발광 ----
		else if (name == _T("glow"))
		{
			push_attr(_T("glow"));
			para_temp.text_prop.cr_glow = get_color(arg(value, 0));
			CString sigma = arg(value, 1);
			para_temp.text_prop.glow_sigma = sigma.IsEmpty() ? 4.0f : (float)_tstof(sigma);
		}
		else if (name == _T("/glow"))
		{
			CSCTextProperty prev = pop_attr(_T("glow"));
			para_temp.text_prop.cr_glow = prev.cr_glow;
			para_temp.text_prop.glow_sigma = prev.glow_sigma;
		}

		//---- 그라디언트 채우기 ----
		else if (name == _T("grad"))
		{
			push_attr(_T("grad"));
			para_temp.text_prop.cr_grad2 = get_color(arg(value, 0));
			CString dir = arg(value, 1);
			dir.MakeLower();
			para_temp.text_prop.grad_horz = (dir == _T("h"));
		}
		else if (name == _T("/grad"))
		{
			CSCTextProperty prev = pop_attr(_T("grad"));
			para_temp.text_prop.cr_grad2 = prev.cr_grad2;
			para_temp.text_prop.grad_horz = prev.grad_horz;
		}

		//---- run 단위 라운드 배경 ----
		else if (name == _T("box"))
		{
			push_attr(_T("box"));
			para_temp.text_prop.cr_box = get_color(arg(value, 0));
			CString radius = arg(value, 1);
			CString pad = arg(value, 2);
			para_temp.text_prop.box_round = radius.IsEmpty() ? 8.0f : (float)_tstof(radius);
			para_temp.text_prop.box_pad = pad.IsEmpty() ? 4 : _ttoi(pad);
		}
		else if (name == _T("/box"))
		{
			CSCTextProperty prev = pop_attr(_T("box"));
			para_temp.text_prop.cr_box = prev.cr_box;
			para_temp.text_prop.box_round = prev.box_round;
			para_temp.text_prop.box_pad = prev.box_pad;
		}

		//---- 자간 ----
		else if (name == _T("sp"))
		{
			push_attr(_T("sp"));
			para_temp.text_prop.char_spacing = (float)_tstof(value);
		}
		else if (name == _T("/sp"))
		{
			para_temp.text_prop.char_spacing = pop_attr(_T("sp")).char_spacing;
		}

		//---- 위/아래 첨자 ----
		else if (name == _T("sup") || name == _T("sub"))
		{
			push_attr(_T("script"));
			para_temp.text_prop.script_scale = 0.62f;
			para_temp.text_prop.script_offset = (name == _T("sup")) ? -0.42f : 0.28f;
		}
		else if (name == _T("/sup") || name == _T("/sub"))
		{
			CSCTextProperty prev = pop_attr(_T("script"));
			para_temp.text_prop.script_scale = prev.script_scale;
			para_temp.text_prop.script_offset = prev.script_offset;
		}

		//---- 줄바꿈 금지 ----
		else if (name == _T("nowrap"))
		{
			push_attr(_T("nowrap"));
			para_temp.text_prop.nowrap = true;
		}
		else if (name == _T("/nowrap"))
		{
			para_temp.text_prop.nowrap = pop_attr(_T("nowrap")).nowrap;
		}

		//---- 스타일 묶음 ----
		else if (name == _T("style"))
		{
			push_attr(_T("style"));
			CString key = value;
			key.MakeLower();
			auto it = style_map().find(key);
			if (it != style_map().end())
				para_temp.text_prop = it->second;
		}
		else if (name == _T("/style"))
		{
			para_temp.text_prop = pop_attr(_T("style"));
		}

		//---- 루비(본문 위 작은 주석) ----
		else if (name == _T("ruby"))
		{
			//바로 다음 텍스트 run 하나에 붙는다. <ruby=にほんご>日本語</ruby> 형태로 쓰며 </ruby> 는 생략 가능.
			pending_ruby = value;
		}
		else if (name == _T("/ruby"))
		{
			//붙일 run 없이 닫힌 경우를 대비해 비운다.
			pending_ruby.Empty();
		}
		else if (name == _T("cru"))
		{
			push_attr(_T("cru"));
			para_temp.text_prop.cr_ruby = get_color(value);
		}
		else if (name == _T("/cru"))
		{
			para_temp.text_prop.cr_ruby = pop_attr(_T("cru")).cr_ruby;
		}

		//---- 인라인 이미지 ----
		else if (name == _T("img"))
		{
			//이미지는 텍스트가 없는 독립 run 이므로 태그를 만난 자리에서 바로 push 한다.
			CSCParagraph run = para_temp;
			run.text.Empty();
			run.img_key = value;
			run.img_height = 0;

			//"경로,높이" 에서 높이만 떼어낸다. 마지막 콤마 뒤가 전부 숫자일 때만 높이로 보므로
			//"D:\a,b\x.png" 처럼 경로에 콤마가 들어 있어도 안전하다.
			int comma = value.ReverseFind(_T(','));
			if (comma >= 0)
			{
				CString tail = value.Mid(comma + 1);
				bool all_digit = !tail.IsEmpty();

				for (int k = 0; k < tail.GetLength() && all_digit; k++)
					all_digit = (tail[k] >= _T('0') && tail[k] <= _T('9'));

				if (all_digit)
				{
					run.img_key = value.Left(comma);
					run.img_height = _ttoi(tail);
				}
			}

			run.tab_x = pending_tab_x;
			run.id = pending_id;
			pending_tab_x = -1;
			pending_id.Empty();

			para_line.push_back(run);
		}

		//---- 다음 run 하나에만 적용 ----
		else if (name == _T("id"))
		{
			pending_id = value;
		}
		else if (name == _T("tab"))
		{
			pending_tab_x = _ttoi(value);
		}

		//---- 라인 단위 ----
		else if (name == _T("al") || name == _T("align"))
		{
			CString v = value;
			v.MakeLower();
			pending_h_align = (v == _T("right")) ? DT_RIGHT : (v == _T("center")) ? DT_CENTER : DT_LEFT;
		}
		else if (name == _T("la") || name == _T("valign"))
		{
			CString v = value;
			v.MakeLower();
			pending_v_align = (v == _T("vcenter") || v == _T("center")) ? DT_VCENTER : (v == _T("bottom")) ? DT_BOTTOM : DT_TOP;
		}
		else if (name == _T("indent"))
		{
			pending_indent = (float)_tstof(value);
		}
		else if (name == _T("hang"))
		{
			pending_hang = (float)_tstof(value);
		}
		else if (name == _T("vsp"))
		{
			pending_line_vspace = (float)_tstof(value);
		}
		else if (name == _T("ls"))
		{
			//<ls=값> 은 "이전 줄과의 간격" 이라 그 자체로 줄 경계 — 진행 중인 라인이 있으면 flush 하여 새 라인을 시작한다.
			//별도 <br> 불필요. 이미 라인이 비어 있으면 (직전 <br> 등) 줄바꿈은 건너뛰고 다음 라인의 간격만 설정 → <br><ls=..> 도 빈 줄·중복 줄바꿈 없이 동작.
			flush_line(false);
			pending_line_spacing = (float)_tstof(value);
		}
		else if (name == _T("br"))
		{
			flush_line(true);
		}

		//---- 일반 텍스트 (인식하지 못한 태그도 기존처럼 그대로 출력된다) ----
		else
		{
			//전체 배경색인 cr_back이 Transparent가 아닐 때 색상이 별도로 지정되지 않은 para[][].cr_back에 cr_back을 줄 경우 중복으로 그려지게 된다.
			//cr_back이 불투명이면 덮어써서 그려져서 표가 나지 않지만 반투명이면 겹쳐져 그려지게 된다.
			if (para_temp.text_prop.cr_back.GetValue() == basic_para.text_prop.cr_back.GetValue())
				para_temp.text_prop.cr_back = Gdiplus::Color::Transparent;

			CSCParagraph run = para_temp;
			run.text = tag;
			run.tab_x = pending_tab_x;
			run.id = pending_id;
			run.ruby = pending_ruby;

			//<sp> 자간은 "글자 사이" 간격이므로 run 을 글자 단위로 쪼개야 한다.
			//calc_text_rect 가 인접 run 사이에 char_spacing 을 넣어주므로 쪼개기만 하면 그대로 자간이 된다.
			if (run.text_prop.char_spacing != 0.0f && run.text.GetLength() > 1)
			{
				CString whole = run.text;
				for (int k = 0; k < whole.GetLength(); k++)
				{
					CSCParagraph one = run;
					one.text = whole.Mid(k, 1);
					if (k > 0)
					{
						one.tab_x = -1;
						one.id.Empty();
						one.ruby.Empty();
					}
					para_line.push_back(one);
				}
			}
			else
			{
				para_line.push_back(run);
			}

			pending_tab_x = -1;
			pending_id.Empty();
			pending_ruby.Empty();
		}
	}

	flush_line(false);
}

//run 들을 character 단위로 split.
void CSCParagraph::split_runs_per_char(std::deque<std::deque<CSCParagraph>>& para)
{
	for (auto& line : para)
	{
		std::deque<CSCParagraph> split_line;
		for (auto& run : line)
		{
			if (run.text.GetLength() <= 1)
			{
				split_line.push_back(run);
				continue;
			}
			for (int k = 0; k < run.text.GetLength(); k++)
			{
				CSCParagraph one = run;
				one.text = run.text.Mid(k, 1);
				split_line.push_back(one);
			}
		}
		line.swap(split_line);
	}
}

//paragraph text 정보를 dc에 출력할 때 출력 크기를 계산하고 각 텍스트가 출력될 위치까지 CSCParagraph 멤버에 저장한다.
CRect CSCParagraph::calc_text_rect(CRect rc, CDC* pDC, std::deque<std::deque<CSCParagraph>>& para, DWORD align, int max_width, int char_spacing)
{
	if (para.empty())
		return CRect();

	int i, j;
	int sx = 0;
	int sy = 0;				//각 라인의 시작 위치(높이값 누적)
	int total_text_height;
	CRect rect_text;
	CFont font;

	Gdiplus::Graphics g(pDC->m_hDC);

	g.SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);
	g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
	g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

	Gdiplus::StringFormat sf;

	sf.SetAlignment(Gdiplus::StringAlignmentNear);
	sf.SetLineAlignment(Gdiplus::StringAlignmentNear);
	//sf.SetTrimming(Gdiplus::StringTrimmingNone);

	//[Phase 0] word-wrap pre-pass — max_width > 0 일 때 라인을 max_width 안으로 재분배.
	//whitespace 우선 boundary, 없으면 character boundary (CJK 자막 등).
	//tag/run 단위 (text_prop) 는 보존 — chunk 가 같은 run 의 속성을 그대로 상속.
	if (max_width > 0)
	{
		//binary search 가 한 run 안에서 measure 를 O(log n) 번 호출하므로 Font 를 매번 Clone 하면 leak 누적.
		//caller 가 한 run 처리하는 동안에만 같은 Font 재사용하도록 cache.
		Gdiplus::Font* cached_font = NULL;
		const CSCParagraph* cached_run = NULL;
		float cached_thickness = 0.0f;
		float cached_pipe_w = 0.0f;

		auto release_cache = [&]() {
			if (cached_font) { delete cached_font; cached_font = NULL; }
			cached_run = NULL;
		};

		auto measure_run_w = [&](CSCParagraph& run, const CString& text) -> int
		{
			if (text.IsEmpty())
				return 0;
			if (cached_run != &run)
			{
				release_cache();
				run.get_paragraph_font(g, &cached_font);
				cached_run = &run;
				cached_thickness = run.text_prop.thickness;
				Gdiplus::RectF bp;
				g.MeasureString(L"|", -1, cached_font, Gdiplus::PointF(0, 0), sf.GenericTypographic(), &bp);
				cached_pipe_w = bp.Width;
			}
			Gdiplus::RectF b;
			g.MeasureString(CStringW(text + _T("|")), -1, cached_font, Gdiplus::PointF(0, 0), sf.GenericTypographic(), &b);
			int w = (int)(b.Width - cached_pipe_w + cached_thickness);
			return w > 0 ? w : 0;
		};

		std::deque<std::deque<CSCParagraph>> wrapped;
		std::deque<CSCParagraph> cur;
		int cur_w = 0;

		//word-boundary-aware flush — char_spacing != 0 일 때 split_runs_per_char 가 글자 1개씩 run 으로 쪼개므로
		//run 안에서의 backward boundary 검색이 효력 없음 (n=1, fit=0/1 만 가능). 이미 cur 에 누적된 runs 를 char 단위로 walk-back 하여 마지막 boundary 위치를 찾고 거기서 split.
		auto flush_with_word_boundary = [&]()
		{
			if (cur.empty())
				return;

			//walk back: cur[i].text 의 char 단위로 ' '/'\t'/',' 검색.
			int found_run = -1;
			int found_pos = -1;	//1-based, like split_at semantics.
			for (int i = (int)cur.size() - 1; i >= 0 && found_run < 0; --i)
			{
				const CString& t = cur[i].text;
				for (int k = t.GetLength(); k >= 1; --k)
				{
					TCHAR c = t[k - 1];
					if (c == _T(' ') || c == _T('\t') || c == _T(','))
					{
						found_run = i;
						found_pos = k;
						break;
					}
				}
			}

			if (found_run < 0)
			{
				//boundary 전혀 없음 — 통째로 flush.
				wrapped.push_back(cur);
				cur.clear();
				cur_w = 0;
				return;
			}

			//head = cur[0..found_run-1] + cur[found_run].Left(found_pos)
			std::deque<CSCParagraph> head;
			for (int i = 0; i < found_run; ++i)
				head.push_back(cur[i]);
			{
				CSCParagraph head_chunk = cur[found_run];
				head_chunk.text = cur[found_run].text.Left(found_pos);
				head.push_back(head_chunk);
			}
			wrapped.push_back(head);

			//tail = cur[found_run].Mid(found_pos) + cur[found_run+1..]
			std::deque<CSCParagraph> tail;
			{
				CString rest = cur[found_run].text.Mid(found_pos);
				while (!rest.IsEmpty() && (rest[0] == _T(' ') || rest[0] == _T('\t')))
					rest = rest.Mid(1);
				if (!rest.IsEmpty())
				{
					CSCParagraph tail_chunk = cur[found_run];
					tail_chunk.text = rest;
					tail.push_back(tail_chunk);
				}
			}
			for (int i = found_run + 1; i < (int)cur.size(); ++i)
				tail.push_back(cur[i]);

			//char-spacing 1글자 run 케이스: tail 머리에 ' '/'\t' 단일 run 이 남아있으면 strip.
			while (!tail.empty())
			{
				const CString& t = tail.front().text;
				if (t.GetLength() == 1 && (t[0] == _T(' ') || t[0] == _T('\t')))
					tail.pop_front();
				else
					break;
			}

			cur = tail;
			cur_w = 0;
			for (auto& r2 : cur)
				cur_w += measure_run_w(r2, r2.text);
		};

		for (auto& line : para)
		{
			cur.clear();
			cur_w = 0;
			int wrapped_size_before = (int)wrapped.size();

			for (auto& run : line)
			{
				CString text = run.text;

				//<nowrap> run 은 쪼개지 않는다 — 라인이 넘치면 먼저 라인을 끊고, 그래도 넘치면 넘친 채로 둔다.
				//"홍길동 님", "12.5 GB" 처럼 붙어 있어야 의미가 사는 덩어리용.
				if (run.text_prop.nowrap && !text.IsEmpty())
				{
					int w = measure_run_w(run, text);
					if (!cur.empty() && cur_w + w > max_width)
						flush_with_word_boundary();

					CSCParagraph chunk = run;
					chunk.text = text;
					cur.push_back(chunk);
					cur_w += w;
					continue;
				}

				while (!text.IsEmpty())
				{
					int avail = max_width - cur_w;
					if (avail <= 0 && !cur.empty())
					{
						flush_with_word_boundary();
						avail = max_width - cur_w;
						if (avail <= 0)
							avail = max_width;
					}
					if (avail <= 0)
						avail = max_width;

					//binary search — text 의 longest prefix 가 avail 안에 들어가는 길이.
					int n = text.GetLength();
					int lo = 1, hi = n, fit = 0;
					while (lo <= hi)
					{
						int mid = (lo + hi) / 2;
						int w = measure_run_w(run, text.Left(mid));
						if (w <= avail) { fit = mid; lo = mid + 1; }
						else { hi = mid - 1; }
					}

					if (fit == 0)
					{
						//1 글자도 avail 에 안 들어가면 라인 flush 후 fresh 라인에 재시도.
						if (!cur.empty())
						{
							flush_with_word_boundary();
							continue;
						}
						//빈 라인인데도 1 글자가 max_width 보다 넓으면 무한루프 방지로 강제 1 글자 진행.
						fit = 1;
					}

					if (fit == n)
					{
						//전체 text 가 들어감 — 그대로 append.
						CSCParagraph chunk = run;
						chunk.text = text;
						cur.push_back(chunk);
						cur_w += measure_run_w(run, text);
						text.Empty();
						continue;
					}

					//split 필요. boundary = 공백/탭/쉼표. 단어 중간 자르지 않도록.
					int split_at = -1;
					//1) backward: fit 이전의 마지막 boundary
					for (int k = fit; k >= 1; --k)
					{
						TCHAR c = text[k - 1];
						if (c == _T(' ') || c == _T('\t') || c == _T(','))
						{
							split_at = k;
							break;
						}
					}

					if (split_at < 0)
					{
						//2) fit 안에 boundary 없음 — 현재 라인이 비어있지 않으면 flush 후 재시도 (단어 통째를 다음 라인으로).
						if (!cur.empty())
						{
							wrapped.push_back(cur);
							cur.clear();
							cur_w = 0;
							continue;
						}
						//3) 빈 라인 — forward look-ahead 으로 단어 끝(=다음 boundary)까지 포함, 라인 너비 초과 허용.
						for (int k = fit + 1; k <= n; ++k)
						{
							TCHAR c = text[k - 1];
							if (c == _T(' ') || c == _T('\t') || c == _T(','))
							{
								split_at = k;
								break;
							}
						}
						//4) 전체에 boundary 없음 (공백 없는 CJK 등) — char boundary fallback.
						if (split_at < 0)
							split_at = fit;
					}

					CSCParagraph chunk = run;
					chunk.text = text.Left(split_at);
					cur.push_back(chunk);
					cur_w += measure_run_w(run, chunk.text);

					//라인 flush.
					wrapped.push_back(cur);
					cur.clear();
					cur_w = 0;

					text = text.Mid(split_at);
					//다음 라인 시작 시 leading whitespace 제거.
					while (!text.IsEmpty() && (text[0] == _T(' ') || text[0] == _T('\t')))
						text = text.Mid(1);
				}
			}

			if (!cur.empty())
			{
				wrapped.push_back(cur);
				cur.clear();
				cur_w = 0;
			}

			//현재 source line 으로부터 생성된 wrapped 라인이 2개 이상이면 첫 라인 외에는 wrap 연속 라인.
			for (int wi = wrapped_size_before + 1; wi < (int)wrapped.size(); ++wi)
			{
				if (!wrapped[wi].empty())
					wrapped[wi][0].wrap_continuation = true;
			}
		}

		if (!wrapped.empty())
			para.swap(wrapped);

		release_cache();
	}

	int max_width_measured = 0;
	int max_width_line = 0;

	for (i = 0; i < para.size(); i++)
	{
		CSize sz_text = CSize(0, 0);

		//이 라인에서 <sup> 등으로 라인 top 위로 올라간 최대 픽셀 수.
		int line_rise = 0;

		for (j = 0; j < para[i].size(); j++)
		{
			//char_spacing: 같은 라인의 두 번째 run 부터 spacing 만큼 left 를 미리 옮긴다 (run 사이 간격).
			if (j > 0 && char_spacing != 0)
				sz_text.cx += char_spacing;

			//<sp=값> 의 run 별 자간도 같은 방식으로 run 사이에 더한다.
			//(파서가 <sp> 구간을 글자 단위 run 으로 쪼개 두었으므로 이게 곧 글자 사이 간격이 된다.)
			if (j > 0 && para[i][j].text_prop.char_spacing != 0.0f)
				sz_text.cx += (int)para[i][j].text_prop.char_spacing;

			//<tab=x> — 이 run 의 라인 내 시작 x 를 강제. 이미 그 지점을 지났으면 무시한다.
			if (para[i][j].tab_x > sz_text.cx)
				sz_text.cx = para[i][j].tab_x;

			CSize sz;
#if 0
			pOldFont = select_paragraph_font(pDC, para, i, j, lf, &font);

			//GetTextExtent()와 DrawText(DT_CALCRECT)로 구한 크기는 동일하며 italic은 약간 잘림.
			sz = pDC->GetTextExtent(para[i][j].text);
			//GetTextExtentExPoint(dc.m_hDC, m_paragraph[i].text, m_paragraph[i].text.GetLength(), 0, NULL, NULL, &sz);

			//if (para[i][j].italic)
			//{
			//	TEXTMETRIC tm;
			//	GetTextMetrics(pDC->m_hDC, &tm);
			//	if (tm.tmOverhang > 0)
			//		sz.cx += tm.tmOverhang;
			//	else
			//		sz.cx += (pDC->GetTextExtent(_T("M")).cx / 4);
			//}

			para[i][j].r = make_rect(sz_text.cx, sy, sz.cx, sz.cy);
#else
			Gdiplus::Font* font = NULL;
			para[i][j].get_paragraph_font(g, &font);

			//<img=...> run — 텍스트 대신 이미지를 한 글자처럼 배치한다.
			//높이 0 이면 이 run 의 폰트 높이에 맞춘다 → <la=vcenter> 와 조합하면 "아이콘 + 텍스트" 가 자동 정렬된다.
			if (!para[i][j].img_key.IsEmpty())
			{
				CSCGdiplusBitmap* img = CSCParagraph::get_image(para[i][j].img_key);

				int img_h = para[i][j].img_height;
				if (img_h <= 0)
					img_h = (int)font->GetHeight(&g);

				int img_w = (img && img->height > 0)
							? (int)((float)img->width * img_h / img->height + 0.5f)
							: img_h;	//로드 실패 시 정사각 자리만 잡아 레이아웃이 무너지지 않게 한다.

				sz.cx = img_w;
				sz.cy = img_h;
				para[i][j].r = make_rect(sz_text.cx, sy, sz.cx, sz.cy);
				para[i][j].ink_height = (float)img_h;

				sz_text.cx += sz.cx;
				sz_text.cy = MAX(sz_text.cy, sz.cy);

				delete font;
				continue;
			}

			Gdiplus::RectF boundRect;
			Gdiplus::RectF boundRect_temp;

			//"text...    "와 같이 뒤에 공백이 있을 경우 공백이 무시되어 출력되지 않는다.
			//방법1. 맨 끝에 "|"와 같은 문자를 넣어 계산한 후 "|"의 width를 뺸다.
			//방법2. SetMeasurableCharacterRanges(), MeasureCharacterRanges() 등을 이용하는 것이 더 좋음
			//우선 간단하게 1번 방식을 사용한다.
			g.MeasureString(CStringW(para[i][j].text + _T("|")), -1, font, Gdiplus::PointF(0, 0), sf.GenericTypographic(), &boundRect);
			g.MeasureString(L"|", -1, font, Gdiplus::PointF(0, 0), sf.GenericTypographic(), &boundRect_temp);

			if (boundRect.IsEmptyArea())
			{
				boundRect.Width = 1;
				boundRect.Height = 40;
			}

			//stroke 두께까지 포함한 크기여야 한다.
			sz.cx = boundRect.Width - boundRect_temp.Width + para[i][j].text_prop.thickness;// *2.0f;
			sz.cy = boundRect.Height + para[i][j].text_prop.thickness;// *2.0f;
			para[i][j].r = make_rect(sz_text.cx, sy, sz.cx, sz.cy);

			//<sup>/<sub> — 폰트는 이미 script_scale 로 작아진 상태이고, 여기서 baseline 만 위/아래로 옮긴다.
			//위로 올라간 만큼(line_rise)은 라인 루프가 끝난 뒤 라인 전체를 내리고 라인 높이를 키워 흡수한다.
			//그렇게 하지 않으면 첫 줄의 <sup> 이 캔버스 상단 밖으로 나가 잘린다.
			if (para[i][j].text_prop.script_offset != 0.0f)
			{
				int script_dy = (int)(para[i][j].text_prop.script_offset * sz.cy);
				para[i][j].r.OffsetRect(0, script_dy);

				if (script_dy < 0)
					line_rise = MAX(line_rise, -script_dy);
			}

			//<ls> 줄간격용 실제 글자 높이 — sz.cy(=boundRect.Height) 는 MeasureString 의 라인 높이(박스)로 폰트 ascent/descent/leading 을 포함해 실제 글자보다 크다.
			//set_line_spacing 이 '여백 = box - 글자높이' 로 여백만 ls 배율 적용하므로, 실제 글자 윤곽 높이를 GraphicsPath 로 직접 잰다.
			Gdiplus::FontFamily ff;
			if (font && font->GetFamily(&ff) == Gdiplus::Ok && !para[i][j].text.IsEmpty())
			{
				Gdiplus::GraphicsPath ink_path;
				ink_path.AddString(CStringW(para[i][j].text), -1, &ff, para[i][j].text_prop.style,
					font->GetSize(), Gdiplus::PointF(0, 0), sf.GenericTypographic());

				Gdiplus::RectF ink_bounds;
				if (ink_path.GetBounds(&ink_bounds) == Gdiplus::Ok && ink_bounds.Height > 0)
					para[i][j].ink_height = ink_bounds.Height + para[i][j].text_prop.thickness;
			}
#endif
			//<ruby=주석> — 본문 위에 ruby_scale 배 크기의 주석을 얹는다.
			//r 은 [루비 + 본문] 전체 박스가 되고 폭은 max(본문, 루비) 라 좁은 쪽이 가운데로 정렬된다.
			//본문 글자의 실제 출력 위치는 get_text_origin() 이 돌려준다.
			if (!para[i][j].ruby.IsEmpty())
			{
				CSCParagraph ruby_run;
				ruby_run.text_prop = para[i][j].text_prop;
				ruby_run.text_prop.size *= para[i][j].text_prop.ruby_scale;

				Gdiplus::Font* ruby_font = NULL;
				ruby_run.get_paragraph_font(g, &ruby_font);

				Gdiplus::RectF rubyRect, rubyRect_temp;
				g.MeasureString(CStringW(para[i][j].ruby + _T("|")), -1, ruby_font, Gdiplus::PointF(0, 0), sf.GenericTypographic(), &rubyRect);
				g.MeasureString(L"|", -1, ruby_font, Gdiplus::PointF(0, 0), sf.GenericTypographic(), &rubyRect_temp);
				delete ruby_font;

				int ruby_w = (int)(rubyRect.Width - rubyRect_temp.Width);
				int ruby_h = (int)rubyRect.Height;
				int base_w = sz.cx;
				int advance = MAX(base_w, ruby_w);

				para[i][j].ruby_height = (float)ruby_h;
				para[i][j].base_dx = (advance - base_w) / 2;
				para[i][j].ruby_dx = (advance - ruby_w) / 2;

				sz.cx = advance;
				sz.cy += ruby_h;
				para[i][j].r = make_rect(sz_text.cx, sy, sz.cx, sz.cy);

				//루비도 '글자' 이므로 ink 로 친다 — <ls=0> 에서 루비 높이가 여백으로 깎여 윗줄과 겹치는 것을 막는다.
				para[i][j].ink_height += (float)ruby_h;
			}

			//TRACE(_T("[%d][%d] text = %s, sz = %dx%d, r = %s\n"), i, j, para[i][j].text, sz.cx, sz.cy, get_rect_info_string(para[i][j].r));
			sz_text.cx += sz.cx;

			//한 라인에서 가장 cy가 큰 값을 기억시킨다.
			sz_text.cy = MAX(sz_text.cy, sz.cy);

			//get_paragraph_font() 는 Clone() 한 Font 를 넘겨주므로 호출자가 해제해야 한다.
			//레이아웃은 텍스트가 바뀔 때마다 run 수만큼 돌기 때문에 그냥 두면 계속 쌓인다.
			delete font;
		}

		//루비 공간은 그 run 혼자가 아니라 라인 전체가 나눠 갖는다.
		//안 그러면 <ruby=にほんご>日本語</ruby>를 처럼 섞였을 때 루비가 붙은 本문만 아래로 내려가
		//"日本語" 와 "를" 의 세로 위치가 어긋난다. HTML/CSS ruby, JIS X 4051 모두 본문 baseline 은
		//주변 글자와 같게 두고 루비가 라인 위쪽 공간을 차지하는 것이 맞다.
		//→ 라인 최대 루비 높이를 루비 없는 run 에도 그대로 얹어 본문 출력 원점을 맞춘다.
		{
			float line_ruby = 0.0f;
			for (j = 0; j < para[i].size(); j++)
				line_ruby = max(line_ruby, para[i][j].ruby_height);

			if (line_ruby > 0.0f)
			{
				for (j = 0; j < para[i].size(); j++)
				{
					if (para[i][j].ruby_height >= line_ruby)
						continue;

					//박스를 위로 키우는 대신 아래로 늘린다 — r.top 은 라인 top 으로 유지되어야
					//라인 stacking 과 set_per_line_align 의 기준이 흔들리지 않는다.
					float delta = line_ruby - para[i][j].ruby_height;
					para[i][j].r.bottom += (int)delta;
					para[i][j].ruby_height = line_ruby;

					//늘어난 만큼은 여백이 아니라 루비가 쓰는 공간이므로 ink 로 친다
					//(이 run 이 라인 최고 높이가 되면 set_line_spacing 이 그 공간을 여백으로 보고 압축한다).
					para[i][j].ink_height += delta;
				}

				//루비 공간을 얹은 만큼 라인 높이도 다시 잡는다.
				for (j = 0; j < para[i].size(); j++)
					sz_text.cy = MAX(sz_text.cy, para[i][j].r.Height());
			}
		}

		//<sup> 이 라인 박스 위로 삐져나간 만큼 라인 전체를 내리고 라인 높이를 키운다.
		//이렇게 해야 첫 줄의 첨자도 캔버스 안에 들어오고, 아랫줄과의 간격도 그만큼 유지된다.
		if (line_rise > 0)
		{
			for (j = 0; j < para[i].size(); j++)
				para[i][j].r.OffsetRect(0, line_rise);

			sz_text.cy += line_rise;
		}

		//각 라인들 중에서 최대 너비를 구한다.
		if (sz_text.cx > max_width_measured)
		{
			max_width_measured = sz_text.cx;
			max_width_line = i;
		}

		//각 라인 시작 위치는 누적된다.
		sy += sz_text.cy;
	}

	total_text_height = sy;

	font.DeleteObject();

	//한 라인내에서 height가 가장 높은 항목으로 통일시키느냐? 아니면 각자의 높이를 그대로 유지하느냐...
	//for (i = 0; i < m_paragraph.size(); i++)
	//{
	//	m_paragraph[i].r.bottom = m_paragraph[i].r.top + m_sz_text.cy;
	//}

	//align 옵션에 따른 보정
	/*
	DWORD dwStyle = GetStyle();
	DWORD dwText = DT_NOCLIP;// | DT_WORDBREAK;

	if (m_dwStyle == 0)
	{
		MAP_STYLE(SS_LEFT, DT_LEFT);
		MAP_STYLE(SS_RIGHT, DT_RIGHT);
		MAP_STYLE(SS_CENTER, DT_CENTER);
		MAP_STYLE(SS_NOPREFIX, DT_NOPREFIX);
		MAP_STYLE(SS_WORDELLIPSIS, DT_WORD_ELLIPSIS);
		MAP_STYLE(SS_ENDELLIPSIS, DT_END_ELLIPSIS);
		MAP_STYLE(SS_PATHELLIPSIS, DT_PATH_ELLIPSIS);
	}
	*/

	CRect margin;

	//align에 따른 보정.
	//라인에 <al=..> 로 지정된 line_h_align 이 있으면 그 라인만 다르게 정렬한다 (0 = 미지정 → 인자 align 사용).
	//라인 너비는 run 들의 r 로부터 직접 구한다 — r.left 에 char_spacing 과 <tab> 이 이미 반영돼 있어
	//폭을 따로 합산하는 것보다 정확하다.
	for (i = 0; i < para.size(); i++)
	{
		if (para[i].empty())
			continue;

		DWORD align_h = (para[i][0].line_h_align != (DWORD)-1) ? para[i][0].line_h_align : align;
		int total_width = para[i][para[i].size() - 1].r.right - para[i][0].r.left;

		//left 기준은 rc.left — 호출자가 rc 를 좌우로 inset 해서 넘기면 그만큼이 좌우 여백이 된다
		//(기존 호출은 모두 rc.left == 0 이라 동작 변화 없음).
		sx = rc.left;
		if (align_h & DT_CENTER)
			sx = rc.CenterPoint().x - total_width / 2;
		else if (align_h & DT_RIGHT)
			sx = rc.right - margin.right - total_width;

		//<indent> 는 라인 전체를, <hang> 은 wrap 으로 이어진 라인만 추가로 민다.
		sx += (int)para[i][0].line_indent;
		if (para[i][0].wrap_continuation)
			sx += (int)para[i][0].line_hang;

		if (sx != 0)
		{
			for (j = 0; j < para[i].size(); j++)
				para[i][j].r.OffsetRect(sx, 0);
		}
	}

	if (align & DT_VCENTER)// SS_CENTERIMAGE)
	{
		//전체 높이에서 전체 텍스트 높이 합계를 뺀 1/2 만큼 shift 시킨다.
		sy = (rc.Height() - total_text_height) / 2;
		for (i = 0; i < para.size(); i++)
		{
			for (j = 0; j < para[i].size(); j++)
			{
				para[i][j].r.OffsetRect(0, sy);
			}
		}
	}
	else //top align
	{
		for (i = 0; i < para.size(); i++)
		{
			for (j = 0; j < para[i].size(); j++)
			{
				para[i][j].r.OffsetRect(0, 0);// m_margin.top);
			}
		}
	}


	if (para.size() > 0)
	{
		//m_pt_icon.x = m_para[m_max_width_line][0].r.left - m_sz_icon.cx - 4;
		//아이콘을 top 정렬하느냐, 모든 라인의 vcenter에 정렬하느냐...
		//m_pt_icon.y = m_para[0][0].r.top;

		//모든 run 의 min-left / max-right 로 잡는다.
		//"최대 넓이 라인의 첫/마지막 run" 만 보면 <tab>/<indent> 로 시작 x 가 0 이 아닌 라인에서
		//그 앞 여백이 폭에서 빠져 캔버스가 좁게 잡히고 오른쪽 끝 글자가 잘린다.
		//라인마다 <al> 정렬이 다를 수 있는 것도 같은 이유로 union 이 맞다.
		bool first_run = true;

		for (i = 0; i < (int)para.size(); i++)
		{
			for (j = 0; j < (int)para[i].size(); j++)
			{
				if (first_run)
				{
					rect_text.left = para[i][j].r.left;
					rect_text.right = para[i][j].r.right;
					first_run = false;
				}
				else
				{
					rect_text.left = MIN(rect_text.left, para[i][j].r.left);
					rect_text.right = MAX(rect_text.right, para[i][j].r.right);
				}
			}
		}

		rect_text.top = para[0][0].r.top;					//최상단 항목의 top
		rect_text.bottom = para[0][0].r.top + total_text_height;	//최상단 항목의 top + 전체 텍스트 높이
	}
	else
	{
		//m_pt_icon.x = sx - m_sz_icon.cx;
		//m_pt_icon.y = sy - m_sz_icon.cy / 2;

		//m_rect_text = make_rect(m_pt_icon.x, m_pt_icon.y, m_sz_icon.cx, m_sz_icon.cy);
	}

	//text 크기에 맞춰 컨트롤의 크기를 조정하는 것은 해당 윈도우에서 처리할 일이다.
	/*
	if (false)//m_auto_ctrl_size)
	{
		if (rect_text.Width() > rc.Width() || rect_text.Height() > rc.Height())
		{
			//MoveWindow(m_rect_text);
			SetWindowPos(NULL, 0, 0, rect_text.Width(), rect_text.Height(), SWP_NOMOVE | SWP_NOZORDER);
		}
	}
	*/

	//20260706 by claude. 각 라인에 저장된 valign(line_align) 재적용 — 위에서 run 들을 top 정렬로 재계산했으므로,
	//DT_TOP 이 아닌 라인만 라인 최대높이 기준으로 다시 정렬한다(재레이아웃돼도 데이터에 남아 지속). 기본(DT_TOP)은 무동작이라 회귀 없음.
	for (i = 0; i < (int)para.size(); i++)
	{
		if (!para[i].empty() && para[i][0].line_align != DT_TOP)
			set_per_line_align(para, i, para[i][0].line_align);
	}

	return rect_text;
}

//특정 라인(line_idx)의 run 들을 그 라인 최대높이 기준으로 valign(DT_TOP/DT_VCENTER/DT_BOTTOM) 재배치한다.
//line_idx < 0 이면 모든 라인에 적용. calc_text_rect() 로 각 run 의 r 이 결정된 뒤(= 기본 top 정렬 상태) 호출한다.
//calc_text_rect 는 한 라인의 run 들을 같은 top 에 놓으므로, 라인 top = run 들의 최소 top(이미 center/bottom 이어도 안전),
//라인 높이 = run 들의 최대 height 로 잡아 각 run 을 정렬 방식에 맞춰 세로로만 이동(가로 위치·글자 높이 불변, 라인 stacking 불변).
void CSCParagraph::set_per_line_align(std::deque<std::deque<CSCParagraph>>& para, int line_idx, DWORD align)
{
	int begin = (line_idx < 0) ? 0 : line_idx;
	int end   = (line_idx < 0) ? (int)para.size() - 1 : line_idx;

	if (begin < 0 || end >= (int)para.size())
		return;

	for (int i = begin; i <= end; i++)
	{
		if (para[i].empty())
			continue;

		int line_top = para[i][0].r.top;
		int line_h   = 0;
		for (int j = 0; j < (int)para[i].size(); j++)
		{
			if (para[i][j].r.top < line_top)
				line_top = para[i][j].r.top;
			if (para[i][j].r.Height() > line_h)
				line_h = para[i][j].r.Height();
		}

		for (int j = 0; j < (int)para[i].size(); j++)
		{
			para[i][j].line_align = align;				//라인의 valign 을 데이터(para)에 저장 → calc_text_rect 재실행 시 자동 재적용(지속).

			int rh = para[i][j].r.Height();

			int target_top = line_top;					//DT_TOP (기본)
			if (align & DT_VCENTER)
				target_top = line_top + (line_h - rh) / 2;
			else if (align & DT_BOTTOM)
				target_top = line_top + (line_h - rh);

			int dy = target_top - para[i][j].r.top;
			if (dy != 0)
				para[i][j].r.OffsetRect(0, dy);
		}
	}
}

int CSCParagraph::get_max_width_line(std::deque<std::deque<CSCParagraph>>& para)
{
	int i, j;
	int max_width = 0;
	int max_width_line = -1;

	for (i = 0; i < para.size(); i++)
	{
		int line_width = 0;

		for (j = 0; j < para[i].size(); j++)
			line_width += para[i][j].r.Width();

		if (line_width > max_width)
		{
			max_width = line_width;
			max_width_line = i;
		}
	}

	return max_width_line;
}

//calc_text_rect()로 각 paragraph의 r이 결정된 이후에 호출.
//라인 사이 간격을 조정한다. 글자 자체 크기는 그대로 두고 다음 라인의 시작 위치만 아래로 밀어낸다.
//spacing = 1.0f이면 변경 없음, 1.5f이면 라인 i의 max height의 0.5배만큼 이후 라인부터 누적 shift된다.
//spacing < 1.0f도 허용하나 음수 shift로 라인이 겹칠 수 있다.
CRect CSCParagraph::set_line_spacing(std::deque<std::deque<CSCParagraph>>& para, float spacing, float wrap_continuation_delta, float paragraph_break_delta)
{
	if (para.empty())
		return CRect();

	int i, j;

	//각 라인의 max height(글자 박스)와, 그 박스를 만든 run 의 실측 글자 높이(ink)를 미리 구한다.
	std::deque<int> line_heights;
	std::deque<float> line_inks;

	for (i = 0; i < para.size(); i++)
	{
		int line_h = 0;
		float line_ink = 0.0f;

		for (j = 0; j < para[i].size(); j++)
		{
			if (para[i][j].r.Height() > line_h)
			{
				line_h = para[i][j].r.Height();
				line_ink = para[i][j].ink_height;
			}
		}

		line_heights.push_back(line_h);
		line_inks.push_back(line_ink);
	}

	//라인 0은 그대로 두고 라인 1부터 누적 shift를 적용.
	int shift_y = 0;

	for (i = 1; i < (int)para.size(); i++)
	{
		float box = (float)line_heights[i - 1];
		float increment;

		//<ls=값> 으로 명시된 라인 (wrap 연속 라인 제외): "보이는 여백 = ls * 기본여백" 이 되도록 shift 한다.
		//보이는 여백 = pitch - ink(실측 글자 높이). 기본여백 = (기본 pitch - ink). 따라서 ls=1.0 = 기본과 동일, 0.5 = 정확히 절반, 0 = 딱 붙음, 2.0 = 2배.
		//이 식은 ink 추정이 다소 부정확해도 0.5 가 1.0 의 절반임을 정확히 보장한다 (같은 padding 을 양쪽에 쓰므로).
		//<vsp=값> 은 이 라인 윗 "여백"을 픽셀로 직접 지정한다 — 배수인 <ls> 보다 우선.
		//여백 = pitch - ink 이므로, 기본 여백(padding)에서 목표 여백까지의 차이만큼만 shift 하면 된다.
		if (!para[i].empty() && para[i][0].line_vspace >= 0.0f)
		{
			float ink = line_inks[i - 1];
			if (ink <= 0.0f || ink > box)
				ink = box;

			increment = para[i][0].line_vspace - (box - ink);
		}
		else if (!para[i].empty() && para[i][0].line_spacing >= 0.0f && !para[i][0].wrap_continuation)
		{
			float ink = line_inks[i - 1];
			if (ink <= 0.0f || ink > box)
				ink = box;	//메트릭 실패 시 padding 0 (= 글자 박스 기준 fallback).

			float padding = box - ink;
			float default_shift = box * ((spacing + paragraph_break_delta) - 1.0f);
			increment = para[i][0].line_spacing * (padding + default_shift) - padding;
		}
		else if (!para[i].empty() && para[i][0].wrap_continuation)
			increment = box * ((spacing + wrap_continuation_delta) - 1.0f);
		else
			increment = box * ((spacing + paragraph_break_delta) - 1.0f);

		shift_y += (int)increment;

		for (j = 0; j < para[i].size(); j++)
			para[i][j].r.OffsetRect(0, shift_y);
	}

	//전체 영역 산출. calc_text_rect()와 동일한 방식으로 max_width_line 기준.
	CRect rect_text;
	int max_width_line = get_max_width_line(para);

	if (max_width_line < 0)
		max_width_line = 0;

	rect_text.left = para[max_width_line][0].r.left;
	rect_text.top = para[0][0].r.top;
	rect_text.right = para[max_width_line][para[max_width_line].size() - 1].r.right;

	//bottom은 마지막 라인의 가장 큰 bottom (shift 적용 결과 그대로).
	int last_line = (int)para.size() - 1;
	int max_bottom = para[last_line][0].r.bottom;

	for (j = 1; j < para[last_line].size(); j++)
		max_bottom = MAX(max_bottom, para[last_line][j].r.bottom);

	rect_text.bottom = max_bottom;

	return rect_text;
}

//float 오버로드와 공식은 동일하되(shift = (spacing-1) * 윗라인 max height) line 번째 라인부터 끝까지만 이동시킨다.
//line < 1 이거나 line >= para.size() 이면 아무 것도 하지 않고 빈 CRect 를 반환한다.
CRect CSCParagraph::set_line_spacing(std::deque<std::deque<CSCParagraph>>& para, int line, float spacing)
{
	if (para.empty())
		return CRect();

	if (line < 1 || line >= (int)para.size())
		return CRect();

	int i, j;

	int line_above_h = 0;
	for (j = 0; j < (int)para[line - 1].size(); j++)
		line_above_h = MAX(line_above_h, para[line - 1][j].r.Height());

	int shift_y = (int)((float)line_above_h * (spacing - 1.0f));

	for (i = line; i < (int)para.size(); i++)
	{
		for (j = 0; j < (int)para[i].size(); j++)
			para[i][j].r.OffsetRect(0, shift_y);
	}

	CRect rect_text;
	int max_width_line = get_max_width_line(para);

	if (max_width_line < 0)
		max_width_line = 0;

	rect_text.left = para[max_width_line][0].r.left;
	rect_text.top = para[0][0].r.top;
	rect_text.right = para[max_width_line][para[max_width_line].size() - 1].r.right;

	//bottom은 마지막 라인의 가장 큰 bottom (shift 있든 없든 그대로).
	int last_line = (int)para.size() - 1;
	int max_bottom = para[last_line][0].r.bottom;
	for (j = 1; j < (int)para[last_line].size(); j++)
		max_bottom = MAX(max_bottom, para[last_line][j].r.bottom);
	rect_text.bottom = max_bottom;

	return rect_text;
}

void CSCParagraph::get_paragraph_font(Gdiplus::Graphics& g, Gdiplus::Font** font)
{
	Gdiplus::Unit unit = g.GetPageUnit();
	float fDpiX = g.GetDpiX();
	float fDpiY = g.GetDpiY();

	int logPixelsY = ::GetDeviceCaps(NULL, LOGPIXELSY);
	float emSize = fDpiY * text_prop.size * text_prop.script_scale / 96.0;	//script_scale = <sup>/<sub> 배율(기본 1.0). 측정·렌더가 같은 크기를 쓰도록 폰트 생성 지점에서 곱한다.

	Gdiplus::FontFamily fontFamily((WCHAR*)(const WCHAR*)CStringW(text_prop.name));

	std::unique_ptr<Gdiplus::Font> ff;

	if (fontFamily.IsAvailable())
		ff = std::make_unique<Gdiplus::Font>(&fontFamily, emSize, text_prop.style);
	else
		ff = std::make_unique<Gdiplus::Font>(Gdiplus::FontFamily::GenericSansSerif(), emSize, text_prop.style);

	*font = ff->Clone();
}

void CSCParagraph::get_shadow_list(std::vector<CSCTextShadow>& out) const
{
	out.clear();

	//<ts> 로 명시했으면 그대로 쓴다.
	if (!text_prop.shadows.empty())
	{
		out = text_prop.shadows;
		return;
	}

	//기존 태그로부터 합성. glow 를 먼저 담는 이유는 CSS 규칙상 리스트 앞쪽이 글자에 더 가깝기 때문이며,
	//그 결과 그리는 순서가 기존과 같은 "그림자 → glow → 글자" 가 된다.
	if (text_prop.cr_glow.GetA() > 0 && text_prop.glow_sigma > 0.0f)
	{
		CSCTextShadow s;
		s.blur = text_prop.glow_sigma;
		s.spread = text_prop.glow_sigma;	//광원 path 를 굵혀 사방으로 더 멀리 번지게 하는 것이 glow 의 실체.
		s.color = text_prop.cr_glow;
		out.push_back(s);
	}

	if (text_prop.shadow_depth != 0.0f && text_prop.cr_shadow.GetA() > 0)
	{
		CSCTextShadow s;

		if (text_prop.shadow_depth > 0.0f)
		{
			s.dx = s.dy = text_prop.shadow_depth;
		}
		else
		{
			//자동 offset — 글자 높이에 비례하되 외곽선 두께보다는 커야 외곽선에 묻히지 않는다.
			float d = max((float)r.Height() / 30.0f, 2.0f);
			d = max(d, text_prop.thickness / 1.4f);
			s.dx = s.dy = d;
		}

		s.blur = text_prop.shadow_blur_sigma;
		s.color = text_prop.cr_shadow;
		out.push_back(s);
	}
}

/*
//ex. add("<font size=12><font bold = 1>first line") 입력된 후
//add("second line")이면 이 때 스타일은 전 스타일값을 그대로 유지한다.
void CSCParagraph::add(CString data)
{
	CSCParagraphData	para_data;
	int font_size = get_value(data, _T("font size"));
}

//tag_name이 실제 문자열 데이터에도 있을 수 있으므로 반드시 <, > 기호로 묶여있음을 확인해야 한다.
DWORD CSCParagraph::get_value(CString data, CString tag_name, int start)
{
	int tag_pos = data.Find(tag_name, start);
	if (tag_pos < 0)
		return -1;

	//< font size=1>과 같이 '<'과 tag_name 사이의 공백 제거
	int tag_start = tag_pos - 1;
	while (tag_start >= 0)
	{
		if (data[tag_start] == '<')
			break;
		else
			tag_start--;
	}

	if (tag_start < 0)
		return -1;

	int tag_end = data.Find('>', tag_start + 1);
	if (tag_end < 0)
		return -1;

	//끝 태그부터 역으로 '='기호까지 찾아간다.
	int value_start = tag_end;
	while (value_start >= 0)
	{
		if (data[value_start] == '=')
			break;
		else
			value_start--;
	}

	if (value_start <= 0)
		return -1;

	CString value;
	//value = data.Mid(value_start + 1, tag_end - )
}
*/

//우선순위: override > EBLC 자동측정(캐시) > 화이트리스트 > fallback_pt. 마지막에 dark_background 보정.
//- override: add_AA_override 로 face 등록된 강제값.
//- EBLC 자동: TrueType 임베디드 비트맵 strike 의 max ppemY+1 (px=pt 가정, 96 DPI). strike 없으면 1.
//- 화이트리스트: EBLC 호출 실패 등 fallback 경로의 안전망. 알려진 케이스 적중 시 정확.
//  · Segoe UI / 맑은 고딕(Malgun Gothic) — 순수 outline, strike 없음 → 14 (사용자 실측 기준 작은 글씨 ClearType 우선)
//  · 굴림/돋움/궁서/바탕 — 15px 이하 strike → 16부터 AA
//  · Tahoma/Verdana — 12px 이하 strike → 13부터 AA
//- dark_background: ClearType subpixel fringe 가 어두운 배경에서 거슬리므로 결과를 AA_DARK_BOOST 만큼 추가 감산.
int CSCParagraph::get_AA_from_pt(LPCTSTR font_name, int fallback_pt, bool dark_background)
{
	auto apply_dark = [dark_background](int v) -> int
	{
		if (!dark_background) return v;
		int r = v - AA_DARK_BOOST;
		return r < 1 ? 1 : r;
	};

	if (font_name == NULL || *font_name == 0)
		return apply_dark(fallback_pt);

	//1) override (사용자 강제) — 가장 우선.
	{
		std::lock_guard<std::mutex> lk(aa_maps_mutex());
		auto& ov = aa_override_map();
		auto it = ov.find(CString(font_name));
		if (it != ov.end())
			return apply_dark(it->second);
	}

	//2) EBLC 자동 측정 (face 별 캐시). 한 폰트당 최초 1회만 GetFontData 호출.
	//strike 보유 (>0) 시에만 자동 임계치 반환 — 객관적 메트릭.
	//strike 미보유 (==0) 또는 probe 실패 (-1) 는 fall-through — 호출자가 지정한 fallback_pt 가 유지되어
	//"strike 없는 outline 폰트 작은 글씨는 ClearType 이 또렷" 이라는 사용자 취향이 무시되지 않는다.
	{
		std::lock_guard<std::mutex> lk(aa_maps_mutex());
		auto& cache = aa_cache_map();
		auto it = cache.find(CString(font_name));
		int max_ppem;
		if (it != cache.end())
			max_ppem = it->second;
		else
		{
			max_ppem = probe_max_embedded_bitmap_ppem(font_name);
			cache[CString(font_name)] = max_ppem;
		}
		if (max_ppem > 0)
		{
			//ppem 은 EM 박스의 픽셀 height. 96 DPI 가정 시 px → pt: pt = px * 72/96.
			//strike 가 ppem 16 까지 → 폰트 사이즈 16px (=12pt @96DPI) 이하에서 비트맵 강제.
			return apply_dark(max_ppem * 72 / 96 + 1);
		}
	}

	//3) 화이트리스트 (probe 실패 fallback).
	if (_tcsicmp(font_name, _T("Segoe UI")) == 0 ||
	    _tcsicmp(font_name, _T("맑은 고딕")) == 0 ||
	    _tcsicmp(font_name, _T("Malgun Gothic")) == 0)
		return apply_dark(14);

	if (_tcsicmp(font_name, _T("굴림")) == 0 || _tcsicmp(font_name, _T("Gulim")) == 0 ||
	    _tcsicmp(font_name, _T("돋움")) == 0 || _tcsicmp(font_name, _T("Dotum")) == 0 ||
	    _tcsicmp(font_name, _T("궁서")) == 0 || _tcsicmp(font_name, _T("Gungsuh")) == 0 ||
	    _tcsicmp(font_name, _T("바탕")) == 0 || _tcsicmp(font_name, _T("Batang")) == 0)
		return apply_dark(16);

	if (_tcsicmp(font_name, _T("Tahoma")) == 0 ||
	    _tcsicmp(font_name, _T("Verdana")) == 0)
		return apply_dark(13);

	return apply_dark(fallback_pt);
}

void CSCParagraph::add_AA_override(LPCTSTR font_name, int pt)
{
	if (font_name == NULL || *font_name == 0) return;
	std::lock_guard<std::mutex> lk(aa_maps_mutex());
	auto& ov = aa_override_map();
	if (pt <= 0)
		ov.erase(CString(font_name));
	else
		ov[CString(font_name)] = pt;
	//override 변경은 즉시 효력 — 단 EBLC 캐시는 폰트 고유 메트릭이라 무효화 불필요.
}

void CSCParagraph::clear_AA_overrides()
{
	std::lock_guard<std::mutex> lk(aa_maps_mutex());
	aa_override_map().clear();
}

CRect CSCParagraph::draw_text(Gdiplus::Graphics& g, std::deque<std::deque<CSCParagraph>>& para, int AA_from_pt, bool dark_background)
{
	int i, j;
	CFont font, * pOldFont = NULL;

	Gdiplus::StringFormat sf;

	Gdiplus::Unit unit = g.GetPageUnit();
	float fDpiX = g.GetDpiX();
	float fDpiY = g.GetDpiY();

	int logPixelsY = ::GetDeviceCaps(NULL, LOGPIXELSY);
	//Gdiplus::REAL emSize = (Gdiplus::REAL)MulDiv(font_size, 96, logPixelsY);

	//g를 이용해서 pDC를 구해서 사용하는 경우는 g.ReleaseHDC(hdc);를 호출하기 전까지는 g의 어떤 함수 사용도 하지 않아야 한다.
	// Make GDI calls, but don't call any methods
	// on g until after the call to ReleaseHDC.
#ifdef USING_HDC
	HDC hdc = g.GetHDC();
	CDC* pDC = CDC::FromHandle(hdc);
	pDC->SetBkMode(TRANSPARENT);
#endif
	sf.SetAlignment(Gdiplus::StringAlignmentNear);
	sf.SetLineAlignment(Gdiplus::StringAlignmentNear);

	//dc.SetBkColor()로 지정된 배경색을 설정하면 편하지만
	//글자 속성에 따라 그 높낮이가 다른 경우도 있다.
	//따라서 calc_text_rect()에서 max height를 모든 paragraph에 적용했으며
	//여기서도 배경색으로 칠한 뒤 텍스트를 표시한다.

	//그림자 / 외곽 발광 사전 패스.
	//그림자와 glow 는 "offset + blur 된 광원" 이라는 같은 것이므로 (CSS text-shadow) 한 경로로 처리한다.
	//항목마다 캔버스 크기 레이어에 모든 음절의 광원을 모아 한 번 blur 한 뒤 합성한다 —
	//음절별로 따로 그리는 것보다 음절 사이가 자연스럽게 번지고, 반투명 색이 겹쳐 진해지지도 않는다.
	float global_gray_weight = 1.0f;
	if (!para.empty() && !para[0].empty())
		global_gray_weight = para[0][0].text_prop.shadow_gray_weight;

	{
		//run 마다 목록이 다를 수 있으므로 항목 수의 최대치를 먼저 구한다.
		//목록 자체는 값이 몇 개 안 되는 vector 라 필요할 때마다 다시 만드는 편이 2차원으로 들고 있는 것보다 간단하다.
		std::vector<CSCTextShadow> list;
		int shadow_layers = 0;

		for (i = 0; i < para.size(); i++)
		{
			for (j = 0; j < (int)para[i].size(); j++)
			{
				para[i][j].get_shadow_list(list);
				shadow_layers = MAX(shadow_layers, (int)list.size());
			}
		}

		Gdiplus::RectF clip;
		g.GetVisibleClipBounds(&clip);
		int layer_w = (int)ceil(clip.X + clip.Width);
		int layer_h = (int)ceil(clip.Y + clip.Height);

		//CSS 와 같이 리스트 앞쪽 항목이 글자에 더 가깝다 → 뒤 항목부터 그려야 앞 항목이 위에 온다.
		for (int k = shadow_layers - 1; k >= 0 && layer_w > 0 && layer_h > 0; k--)
		{
			//한 레이어는 blur 를 한 번만 걸 수 있으므로 sigma 가 다른 run 을 같이 담으면 안 된다.
			//(그렇게 하면 <sb=0> 하드 엣지 그림자가 옆 run 의 큰 sigma 로 같이 흐려진다.)
			//이 index 에서 쓰이는 sigma 들을 모아 값마다 레이어를 따로 만든다. 실제로는 1~2 개다.
			std::vector<float> sigmas;

			for (i = 0; i < para.size(); i++)
			{
				for (j = 0; j < (int)para[i].size(); j++)
				{
					para[i][j].get_shadow_list(list);
					if (k >= (int)list.size() || list[k].color.GetA() == 0 || para[i][j].text.IsEmpty())
						continue;

					bool found = false;
					for (auto v : sigmas)
					{
						if (fabs(v - list[k].blur) < 0.01f)
						{
							found = true;
							break;
						}
					}

					if (!found)
						sigmas.push_back(list[k].blur);
				}
			}

			//흐린 것부터 깔아야 선명한 그림자가 위에 온다.
			std::sort(sigmas.begin(), sigmas.end(), std::greater<float>());

			for (auto sigma : sigmas)
			{
			CSCGdiplusBitmap layer(layer_w, layer_h, Gdiplus::Color::Transparent, PixelFormat32bppARGB);
			Gdiplus::Graphics gl(layer.m_pBitmap);
			gl.SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);
			gl.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

			for (i = 0; i < para.size(); i++)
			{
				for (j = (int)para[i].size() - 1; j >= 0; j--)
				{
					para[i][j].get_shadow_list(list);
						if (k >= (int)list.size() || list[k].color.GetA() == 0 || para[i][j].text.IsEmpty())
							continue;

						//이 레이어의 sigma 와 다른 run 은 다음(또는 이전) 레이어가 담당한다.
						if (fabs(list[k].blur - sigma) >= 0.01f)
							continue;

					const CSCTextShadow& s = list[k];

					Gdiplus::FontFamily* ff = new Gdiplus::FontFamily((WCHAR*)(const WCHAR*)CStringW(para[i][j].text_prop.name));
					if (!ff->IsAvailable())
					{
						delete ff;
						ff = Gdiplus::FontFamily::GenericSansSerif()->Clone();
					}

					float emSize = fDpiY * para[i][j].text_prop.size * para[i][j].text_prop.script_scale / 72.0f;
					CPoint pt_base = para[i][j].get_text_origin();

					Gdiplus::GraphicsPath path;
					path.SetFillMode(Gdiplus::FillModeWinding);
					path.AddString(CStringW(para[i][j].text), para[i][j].text.GetLength(), ff,
						para[i][j].text_prop.style, emSize,
						Gdiplus::Point(pt_base.x + (int)s.dx, pt_base.y + (int)s.dy),
						sf.GenericTypographic());

					Gdiplus::SolidBrush br(s.color);
					gl.FillPath(&br, &path);

					//spread 는 광원 자체를 굵혀 더 멀리·진하게 번지게 한다. glow 가 이걸로 표현된다.
					if (s.spread > 0.0f)
					{
						Gdiplus::Pen pen(s.color, s.spread);
						pen.SetLineJoin(Gdiplus::LineJoinRound);
						gl.DrawPath(&pen, &path);
					}

					delete ff;
				}
			}

			//order 는 box blur 반복 횟수. 1 이면 단일 box 라 감쇠가 직선이고 끝이 각져 "원뿔" 처럼 보인다.
			//3 이면 box*3 이 Gaussian 에 수렴해 글자 주변 감쇠가 부드러워진다 (비용은 여전히 O(n)).
			//blur 가 0 이면 하드 엣지 offset 그림자 — blur 를 건너뛰기만 하면 된다.
			if (sigma > 0.0f)
				layer.blur(sigma, 3);

			if (global_gray_weight > 0.0f && global_gray_weight < 1.0f)
				layer.gray(global_gray_weight);

			g.DrawImage(layer.m_pBitmap, 0, 0);
			}
		}
	}

	for (i = 0; i < para.size(); i++)
	{
		//각 항목을 출력하되 뒤에서부터 출력시킨다.
		//이는 italic인 경우 다음 항목에 의해 일부 가려지는 현상을 방지하기 위함이다.
		for (j = (int)(para[i].size() - 1); j >= 0; j--)
		{
#ifdef USING_HDC
			pOldFont = select_paragraph_font(pDC, para, i, j, lf, &font);

			//text 배경색을 칠하고
			if (para[i][j].cr_back.GetA() != 0)
				pDC->FillSolidRect(para[i][j].r, para[i][j].cr_back.ToCOLORREF());

			//text를 출력한다.
			pDC->SetTextColor(para[i][j].cr_text.ToCOLORREF());
			pDC->DrawText(para[i][j].text, para[i][j].r, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP);
			//draw_text(g, m_para[i][j].r, m_para[i][j].cr_text, m_para[i][j].text, m_para[i][j].size, m_para[i][j].);
			//g.DrawString(CStringW(m_para[i][j].text), m_para[i][j].text.GetLength(), font, Gdiplus::PointF((Gdiplus::REAL)m_para[i][j].r.left, (Gdiplus::REAL)m_para[i][j].r.top), &sf);
			pDC->SelectObject(pOldFont);
#else
			//text 배경색을 칠하고
			draw_rect(g, para[i][j].r, Gdiplus::Color::Transparent, para[i][j].text_prop.cr_back);

			//<box=색,radius,pad> — run 단위 라운드 배경. 사각형인 cr_back 과 달리 "태그 칩" 모양이 된다.
			if (para[i][j].text_prop.cr_box.GetA() > 0)
			{
				CRect rb = para[i][j].r;
				rb.InflateRect(para[i][j].text_prop.box_pad, para[i][j].text_prop.box_pad);
				draw_round_rect(&g, Gdiplus::Rect(rb.left, rb.top, rb.Width(), rb.Height()),
					Gdiplus::Color::Transparent, para[i][j].text_prop.cr_box,
					(int)para[i][j].text_prop.box_round, 0);
			}

			//<img=...> run — 계산된 r 에 이미지를 그리고 끝낸다.
			if (!para[i][j].img_key.IsEmpty())
			{
				CSCGdiplusBitmap* img = CSCParagraph::get_image(para[i][j].img_key);
				if (img && img->m_pBitmap)
				{
					//같은 라인에 ruby 가 있으면 그 높이만큼 아래로 내려 본문 글자와 세로 위치를 맞춘다.
					//(ruby 가 없으면 get_text_origin() 은 r 의 좌상단이고 ruby_height 는 0 이라 기존과 동일.)
					CPoint pt_img = para[i][j].get_text_origin();

					Gdiplus::InterpolationMode old_interp = g.GetInterpolationMode();
					g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
					g.DrawImage(img->m_pBitmap,
						Gdiplus::Rect(pt_img.x, pt_img.y,
							para[i][j].r.Width(), para[i][j].r.Height() - (int)para[i][j].ruby_height));
					g.SetInterpolationMode(old_interp);
				}

				//이 run 의 r 은 함수 끝의 합집합 계산에 이미 포함되므로 여기서 따로 누적할 필요가 없다.
				continue;
			}

			Gdiplus::FontFamily* fontFamily = new Gdiplus::FontFamily((WCHAR*)(const WCHAR*)CStringW(para[i][j].text_prop.name));

			//시스템에 등록되지 않은 폰트를 설정할 경우 ff는 null이므로 기본 폰트로라도 대체시켜야 한다.
			if (!fontFamily->IsAvailable())
			{
				delete fontFamily;
				fontFamily = Gdiplus::FontFamily::GenericSansSerif()->Clone();
			}

			Gdiplus::Font* font = NULL;
			para[i][j].get_paragraph_font(g, &font);
			//float emSize = fDpiY * para[i][j].text_prop.size / 96.0;
			//Gdiplus::Font font(&ff, emSize, para[i][j].text_prop.style);

			//GraphicsPath를 이용하면 stroke, shadow 등 다양한 효과를 구현할 수 있지만
			//DrawString()보다 글자가 선명하게 보이지 않는 단점이 있다.
			//stroke/shadow 없는 평문 run 은 GDI+ DrawString 으로 그린다. 작은 글자 또렷함은 아래에서 set 하는
			//ClearTypeGridFit 힌트가 담당. (GDI TextOut 으로 그리면 렌더 폭이 GDI+ 측정 rect 와 어긋나
			// run 겹침·배경 오정렬이 발생하므로 측정·렌더를 GDI+ 로 일원화한다.)
			if (para[i][j].text_prop.shadow_depth == 0 && para[i][j].text_prop.thickness == 0)
			{
				//음절별 폰트 종류 + 크기로 hint 자동 결정.
				//각 폰트는 비트맵 보유 범위가 달라 단일 임계치로 부정확 — 폰트별 매핑(get_AA_from_pt)을 거쳐
				//effective threshold 를 구한 뒤 size 와 비교한다.
				//- size >= effective → AntiAliasGridFit (큰 글씨 외곽 매끈)
				//- size <  effective → ClearTypeGridFit  (작은 글씨 비트맵/서브픽셀로 또렷)
				//AA_from_pt == 0 이면 자동 결정 비활성 (호출자가 미리 설정한 hint 유지).
				//SetTextRenderingHint 는 DrawString/MeasureString 에만 적용된다. 아래 else 의 GraphicsPath
				//경로(FillPath/DrawPath)는 SmoothingMode 만 보므로 여기서만 세팅한다.
				if (AA_from_pt > 0)
				{
					int effective = CSCParagraph::get_AA_from_pt(para[i][j].text_prop.name, AA_from_pt, dark_background);
					g.SetTextRenderingHint(para[i][j].text_prop.size >= effective
						? Gdiplus::TextRenderingHintAntiAliasGridFit
						: Gdiplus::TextRenderingHintClearTypeGridFit);
				}

				CPoint pt_base = para[i][j].get_text_origin();

				Gdiplus::SolidBrush text_brush(para[i][j].text_prop.cr_text);
				g.DrawString(CStringW(para[i][j].text), -1, font,
					Gdiplus::PointF((Gdiplus::REAL)pt_base.x, (Gdiplus::REAL)pt_base.y), sf.GenericTypographic(), &text_brush);
			}
			else
			{
				float emSize = fDpiY * para[i][j].text_prop.size * para[i][j].text_prop.script_scale / 72.0;
				Gdiplus::GraphicsPath str_path;

				//겹치는 부분을 반전시키지 않는다. FillModeAlternate는 반전시킴.
				str_path.SetFillMode(Gdiplus::FillModeWinding);

				//AddString() 파라미터 중 출력위치를 줄 때 Gdiplus::Rect() 또는 Gdiplus::Point()로 줄 수 있는데
				//stroke 또는 shadow가 추가되어 r이 작으면 텍스트가 출력되지 않는 현상이 있다.
				//r을 정확히 계산하는 것이 정석이나 굳이 r을 주지 않고 Gdiplus::Point()로 주면 문제되지 않는다.
				//ruby 가 붙은 run 은 r 이 [루비 + 본문] 전체 박스이므로 본문 글자의 출력 원점을 따로 구한다.
				//ruby 가 없으면 get_text_origin() == r 의 좌상단이라 기존과 동일하다.
				CRect r = para[i][j].r;
				r.MoveToXY(para[i][j].get_text_origin());

				str_path.AddString(CStringW(para[i][j].text), para[i][j].text.GetLength(), fontFamily,
					para[i][j].text_prop.style, emSize, Gdiplus::Point(r.left, r.top), sf.GenericTypographic());

				//20260718 by claude. 반투명(글자 alpha<255)+외곽선 글자는 아래 if(ss_translucent) 에서 외곽선·fill 을
				//한 임시 비트맵에 SS 배로 그린 뒤 고품질 축소해 AA 를 얻는다.
				bool ss_translucent = (para[i][j].text_prop.thickness > 0.0f && para[i][j].text_prop.cr_text.GetA() < 255);

				//그림자는 blur 유무와 관계없이 위 사전 패스가 전부 처리하므로 여기서 그릴 것이 없다.
				//(예전엔 sigma == 0 인 하드 엣지 그림자만 음절 단위로 따로 그렸는데, 사전 패스가 blur 를
				// 건너뛰기만 하면 같은 결과라 경로를 하나로 합쳤다.)

				Gdiplus::Pen   pen(para[i][j].text_prop.cr_stroke, para[i][j].text_prop.thickness);
				Gdiplus::SolidBrush solid_brush(para[i][j].text_prop.cr_text);

				//<grad=색2[,h]> — 글자 채우기를 cr_text → cr_grad2 그라디언트로 교체.
				//브러시 좌표는 world 기준이라 아래 SS 경로의 ScaleTransform 아래에서도 그대로 맞는다.
				Gdiplus::Brush* brush = &solid_brush;
				std::unique_ptr<Gdiplus::LinearGradientBrush> grad_brush;

				if (para[i][j].text_prop.cr_grad2.GetA() > 0)
				{
					Gdiplus::RectF pb;
					str_path.GetBounds(&pb);
					//폭/높이가 0 이면 브러시 생성이 실패하므로 최소 1 로 보정.
					if (pb.Width < 1.0f)  pb.Width = 1.0f;
					if (pb.Height < 1.0f) pb.Height = 1.0f;

					grad_brush = std::make_unique<Gdiplus::LinearGradientBrush>(pb,
						para[i][j].text_prop.cr_text, para[i][j].text_prop.cr_grad2,
						para[i][j].text_prop.grad_horz ? Gdiplus::LinearGradientModeHorizontal
													   : Gdiplus::LinearGradientModeVertical);
					brush = grad_brush.get();
				}

				//pen.SetLineJoin(Gdiplus::LineJoinMiter);
				pen.SetLineJoin(Gdiplus::LineJoinRound);

				//20260718 by claude. 반투명 글자 + 외곽선: 외곽선을 fill 밑에 두고 내부를 SourceCopy 로 교체하면 (1) 외곽선이
				//바깥 절반만 보여 글자 침범이 없고 (2) 뒤 stroke/shadow 가 안 비친다. 다만 SourceCopy 는 AA 가 안 돼 경계가 거칠다.
				//→ 이 그룹(그림자+외곽선+fill)을 SS 배 크기의 임시 비트맵에 그린 뒤 고품질 축소 → 축소가 AA 를 만들어 매끄럽다.
				//불투명(alpha==255) 글자는 기존 1x 경로(외곽선 먼저 → fill) 그대로라 회귀 없음.
				if (ss_translucent)
				{
					const int SS = 3;

					//그림자는 사전 패스가 이미 g 에 그려 뒀으므로 여기서는 글자(외곽선 포함)만 감싸면 된다.
					Gdiplus::RectF gb;
					str_path.GetBounds(&gb, NULL, &pen);
					const float margin = 2.0f;
					float minx = gb.X - margin;
					float miny = gb.Y - margin;
					int bw = (int)ceil(gb.Width + margin * 2);
					int bh = (int)ceil(gb.Height + margin * 2);

					if (bw > 0 && bh > 0)
					{
						CSCGdiplusBitmap tmp(bw * SS, bh * SS, Gdiplus::Color::Transparent, PixelFormat32bppARGB);
						Gdiplus::Graphics gt(tmp.m_pBitmap);
						gt.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
						gt.SetTextRenderingHint(g.GetTextRenderingHint());
						gt.ScaleTransform((Gdiplus::REAL)SS, (Gdiplus::REAL)SS);
						gt.TranslateTransform(-minx, -miny);

						//외곽선(밑) → 내부 SourceCopy 교체(안쪽 외곽선/그림자 제거) → 바깥 외곽선만 남음.
						gt.DrawPath(&pen, &str_path);
						gt.SetCompositingMode(Gdiplus::CompositingModeSourceCopy);
						gt.FillPath(brush, &str_path);
						gt.SetCompositingMode(Gdiplus::CompositingModeSourceOver);

						//g 에 이미 그려진 그림자(사전 blur 패스 등)가 글자 내부에 깔려 있으면 반투명 fill 뒤로 비친다.
						//SS temp 를 얹기 전에 g 의 글자 내부(str_path)를 투명으로 punch — 그 자리 그림자 제거. 바깥 소프트
						//그림자 halo 는 유지된다. 경계는 아래 축소된 temp 의 AA 외곽선이 덮어 감춘다.
						{
							Gdiplus::CompositingMode gcm = g.GetCompositingMode();
							Gdiplus::SmoothingMode  gsm = g.GetSmoothingMode();
							g.SetCompositingMode(Gdiplus::CompositingModeSourceCopy);
							g.SetSmoothingMode(Gdiplus::SmoothingModeNone);
							Gdiplus::SolidBrush clear_br(Gdiplus::Color(0, 0, 0, 0));
							g.FillPath(&clear_br, &str_path);
							g.SetSmoothingMode(gsm);
							g.SetCompositingMode(gcm);
						}

						//고품질 축소 → AA.
						Gdiplus::InterpolationMode old_interp = g.GetInterpolationMode();
						Gdiplus::PixelOffsetMode  old_pom = g.GetPixelOffsetMode();
						g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
						g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
						g.DrawImage(tmp.m_pBitmap,
							Gdiplus::RectF(minx, miny, (Gdiplus::REAL)bw, (Gdiplus::REAL)bh),
							0.0f, 0.0f, (Gdiplus::REAL)(bw * SS), (Gdiplus::REAL)(bh * SS), Gdiplus::UnitPixel);
						g.SetInterpolationMode(old_interp);
						g.SetPixelOffsetMode(old_pom);
					}
				}
				else
				{
					//불투명 글자 + 외곽선: GDI+ 는 pen widening 과 path flattening 을 "최종 픽셀 해상도"에서 수행한다.
					//그래서 폭이 2~3px 정도인 얇은 외곽선은 곡선 구간에서 폭이 들쭉날쭉해지거나 아예 끊긴다
					//(직선 stem 은 멀쩡한데 'g','C' 같은 라운드에서만 지저분해 보이는 이유).
					//SS 배 캔버스에 그린 뒤 고품질 축소하면 widening/flattening 오차가 1/SS 로 줄어 외곽선이 균일해진다.
					//외곽선이 없으면 (thickness == 0) 단순 fill 이라 1x 로도 충분하므로 건너뛴다.
					int SS = (para[i][j].text_prop.thickness > 0.0f) ? para[i][j].text_prop.path_supersample : 1;

					Gdiplus::RectF gb;
					if (SS > 1)
					{
						str_path.GetBounds(&gb, NULL, &pen);
						//SS 배 임시 비트맵이 과도하게 커지면 (초대형 글씨) 배율을 낮추거나 포기한다.
						while (SS > 1 && (double)(gb.Width + 4) * (gb.Height + 4) * SS * SS > 64.0 * 1024 * 1024)
							SS--;
					}

					if (SS > 1)
					{
						const float margin = 2.0f;
						float minx = gb.X - margin;
						float miny = gb.Y - margin;
						int bw = (int)ceil(gb.Width + margin * 2);
						int bh = (int)ceil(gb.Height + margin * 2);

						CSCGdiplusBitmap tmp(bw * SS, bh * SS, Gdiplus::Color::Transparent, PixelFormat32bppARGB);
						Gdiplus::Graphics gt(tmp.m_pBitmap);

						//축소 시 bicubic 은 RGB 를 alpha 와 무관하게 섞으므로, 투명 영역의 RGB 가 0 이면
						//글자 가장자리 색이 검게 끌려간다 (blur() 의 premultiply 문제와 동일). 투명하되 RGB 는
						//바깥쪽 색 = 외곽선 색으로 채워 두면 축소 결과가 그대로 외곽선 색으로 수렴한다.
						//alpha=0 을 SourceOver 로 Clear 하면 무시되므로 SourceCopy 로 강제한다.
						{
							Gdiplus::Color cs = para[i][j].text_prop.cr_stroke;
							Gdiplus::SolidBrush br_clear(Gdiplus::Color(0, cs.GetR(), cs.GetG(), cs.GetB()));
							gt.SetCompositingMode(Gdiplus::CompositingModeSourceCopy);
							gt.FillRectangle(&br_clear, 0, 0, bw * SS, bh * SS);
							gt.SetCompositingMode(Gdiplus::CompositingModeSourceOver);
						}

						gt.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
						gt.ScaleTransform((Gdiplus::REAL)SS, (Gdiplus::REAL)SS);
						gt.TranslateTransform(-minx, -miny);

						gt.DrawPath(&pen, &str_path);
						gt.FillPath(brush, &str_path);

						Gdiplus::InterpolationMode old_interp = g.GetInterpolationMode();
						Gdiplus::PixelOffsetMode  old_pom = g.GetPixelOffsetMode();
						g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
						g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
						g.DrawImage(tmp.m_pBitmap,
							Gdiplus::RectF(minx, miny, (Gdiplus::REAL)bw, (Gdiplus::REAL)bh),
							0.0f, 0.0f, (Gdiplus::REAL)(bw * SS), (Gdiplus::REAL)(bh * SS), Gdiplus::UnitPixel);
						g.SetInterpolationMode(old_interp);
						g.SetPixelOffsetMode(old_pom);
					}
					else
					{
						//thickness가 0.0f이면 g.DrawPath()가 아닌 g.DrawString()으로 그리면 되고 이전 버전은 잘 그려졌으나
						//뭔가 옵셋이 틀어진 현상이 발생하여 우선 아래와 같이 조건에 의해 g.DrawPath()를 실행하도록 한다.
						if (para[i][j].text_prop.thickness > 0.0f)
							g.DrawPath(&pen, &str_path);

						g.FillPath(brush, &str_path);
					}
				}
			}

			//<ruby=주석> — 본문 위 작은 주석.
			//본문에 외곽선이 있으면 같은 비율로 줄인 외곽선을 함께 준다. 영상 위 자막에서 루비만 외곽선이 없으면
			//그 부분만 배경에 묻혀 안 읽히기 때문이다.
			if (!para[i][j].ruby.IsEmpty())
			{
				CSCParagraph ruby_run;
				ruby_run.text_prop = para[i][j].text_prop;
				ruby_run.text_prop.size *= para[i][j].text_prop.ruby_scale;

				Gdiplus::Font* ruby_font = NULL;
				ruby_run.get_paragraph_font(g, &ruby_font);

				Gdiplus::Color cr_ruby = (para[i][j].text_prop.cr_ruby.GetA() > 0)
										 ? para[i][j].text_prop.cr_ruby : para[i][j].text_prop.cr_text;

				CPoint pt_ruby(para[i][j].r.left + para[i][j].ruby_dx, para[i][j].r.top);

				if (para[i][j].text_prop.thickness > 0.0f)
				{
					float ruby_em = fDpiY * ruby_run.text_prop.size * ruby_run.text_prop.script_scale / 72.0f;

					Gdiplus::GraphicsPath ruby_path;
					ruby_path.SetFillMode(Gdiplus::FillModeWinding);
					ruby_path.AddString(CStringW(para[i][j].ruby), para[i][j].ruby.GetLength(), fontFamily,
						para[i][j].text_prop.style, ruby_em, Gdiplus::Point(pt_ruby.x, pt_ruby.y), sf.GenericTypographic());

					Gdiplus::Pen ruby_pen(para[i][j].text_prop.cr_stroke,
						para[i][j].text_prop.thickness * para[i][j].text_prop.ruby_scale);
					ruby_pen.SetLineJoin(Gdiplus::LineJoinRound);

					Gdiplus::SolidBrush ruby_brush(cr_ruby);
					g.DrawPath(&ruby_pen, &ruby_path);
					g.FillPath(&ruby_brush, &ruby_path);
				}
				else
				{
					Gdiplus::SolidBrush ruby_brush(cr_ruby);
					g.DrawString(CStringW(para[i][j].ruby), -1, ruby_font,
						Gdiplus::PointF((Gdiplus::REAL)pt_ruby.x, (Gdiplus::REAL)pt_ruby.y), sf.GenericTypographic(), &ruby_brush);
				}

				delete ruby_font;
			}
#endif

			//각 para 영역 확인용 코드
#ifdef _DEBUG
			//"\n"에 의한 공백 라인은 영역 사각형을 굳이 표시하지 않는다.
			//if (para[i][j].r.Width() > 2)
				//draw_rect(g, para[i][j].r, Gdiplus::Color::Blue);// , Gdiplus::Color(255, 255, 0, 0));
#endif
			if (font)
				delete font;

			if (fontFamily)
				delete fontFamily;
		}
	}

	//텍스트 출력 영역 확인용
#ifdef _DEBUG
	//draw_rect(g, m_rect_text, Gdiplus::Color::Blue, Gdiplus::Color::Transparent, 1);
#endif
	//TRACE(_T("m_rect_text = %s\n"), get_rect_info_string(m_rect_text));

#ifdef USING_HDC
	font.DeleteObject();
	pDC->SelectObject(pOldFont);

	g.ReleaseHDC(hdc);
#endif

	//실제 그려진 텍스트 영역 = 모든 음절(run) r 의 합집합. 호출측(CSCStatic::OnPaint)이 m_text_rect 로 사용한다.
	//OnPaint 의 plain-text 경로(DT_CALCRECT/align)가 m_text_rect 를 덮어쓰므로, 그린 직후 이 값으로 되돌려야
	//단락 모드의 get_text_rect() 가 정확해진다.
	CRect drawn;
	drawn.SetRectEmpty();
	bool first = true;
	for (i = 0; i < (int)para.size(); i++)
	{
		for (j = 0; j < (int)para[i].size(); j++)
		{
			const CRect& r = para[i][j].r;
			if (first)
			{
				drawn = r;
				first = false;
			}
			else
			{
				drawn.left   = min(drawn.left,   r.left);
				drawn.top    = min(drawn.top,    r.top);
				drawn.right  = max(drawn.right,  r.right);
				drawn.bottom = max(drawn.bottom, r.bottom);
			}
		}
	}
	return drawn;
}

//calc_text_rect()에서 이미 각 paragraph의 r이 align에 따라 정해지지만 이를 동적으로 변경하고자 할 경우 호출.
CRect CSCParagraph::set_text_align(CRect rc, std::deque<std::deque<CSCParagraph>>& para, DWORD align)
{
	int i, j;
	int sx, sy;
	int total_text_height = 0;
	CRect margin;	//추후 static 멤버변수로 분리할 예정

	if (align & DT_CENTER)
	{
		//각 라인마다 total_width를 구하고
		for (i = 0; i < para.size(); i++)
		{
			int total_width = 0;
			for (j = 0; j < para[i].size(); j++)
				total_width += para[i][j].r.Width();

			//아이콘을 포함하여 center에 표시할 지, 텍스트만 center에 표시할 지...
			//if (m_hIcon)
			//	total_width -= (m_sz_icon.cx + 4);

			//cx에서 total_width/2를 뺀 위치가 첫 번째 항목의 sx이므로 그 만큼 shift시키면 된다.
			sx = rc.CenterPoint().x - total_width / 2;
			for (j = 0; j < para[i].size(); j++)
				para[i][j].r.OffsetRect(sx, 0);
		}
	}
	else if (align & DT_RIGHT)
	{
		//각 라인마다 total_width를 구하고
		for (i = 0; i < para.size(); i++)
		{
			int total_width = 0;
			for (j = 0; j < para[i].size(); j++)
				total_width += para[i][j].r.Width();

			//rc.right에서 total_width를 뺀 위치가 첫 번째 항목의 sx이므로 그 만큼 shift시키면 된다.
			sx = rc.right - margin.right - total_width;
			for (j = 0; j < para[i].size(); j++)
				para[i][j].r.OffsetRect(sx, 0);
		}
	}
	else //SS_LEFT (default)
	{
		if (false)//m_hIcon)
		{
			for (i = 0; i < para.size(); i++)
			{
				for (j = 0; j < para[i].size(); j++)
				{
					//para[i][j].r.OffsetRect(m_margin.left + m_sz_icon.cx + 4, 0);
				}
			}
		}
	}

	if (align & DT_VCENTER)// SS_CENTERIMAGE)
	{
		//전체 높이에서 전체 텍스트 높이 합계를 뺀 1/2 만큼 shift 시킨다.
		sy = (rc.Height() - total_text_height) / 2;
		for (i = 0; i < para.size(); i++)
		{
			for (j = 0; j < para[i].size(); j++)
			{
				para[i][j].r.OffsetRect(0, sy);
			}
		}
	}
	else //top align
	{
		for (i = 0; i < para.size(); i++)
		{
			for (j = 0; j < para[i].size(); j++)
			{
				para[i][j].r.OffsetRect(0, 0);// m_margin.top);
			}
		}
	}

	CRect rect_text;
	int max_width_line = get_max_width_line(para);

	if (para.size() > 0)
	{
		rect_text.left = para[max_width_line][0].r.left;	//최대 넓이 라인의 0번 아이템의 left
		rect_text.top = para[0][0].r.top;					//최상단 항목의 top
		rect_text.right = para[max_width_line][para[max_width_line].size() - 1].r.right;	//최대 넓이 라인의 마지막 항목의 right
		rect_text.bottom = para[0][0].r.top + total_text_height;	//최상단 항목의 top + 전체 텍스트 높이
	}

	return rect_text;
}

//텍스트 상하좌우 여백
//void CSCParagraph::set_margin(std::deque<std::deque<CSCParagraph>>& para, float margin)
//{
//	para.text_prop.
//}

//para의 정보를 문자열로 리턴한다.
CString	CSCParagraph::get_paragraph_info_string(std::deque<std::deque<CSCParagraph>>& para)
{
	int i, j;
	CString str;
	CString info;

	for (i = 0; i < para.size(); i++)
	{
		for (j = 0; j < para[i].size(); j++)
		{
			str.Format(_T("(%d, %d) : text = %s, r = %s, name = %s, size = %.1f, style = %d\n"),
				i, j,
				para[i][j].text,
				get_rect_info_str(para[i][j].r),
				para[i][j].text_prop.name,
				para[i][j].text_prop.size,
				para[i][j].text_prop.style);

			info += str;
		}
	}

	TRACE(_T("%s\n"), info);
	return info;
}
