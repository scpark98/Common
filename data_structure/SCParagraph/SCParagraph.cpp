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

//text의 태그를 파싱하여 각 구문의 속성을 설정한 후 para에 저장한다.
//cr_text, cr_back은 글자, 배경 기본값
void CSCParagraph::build_paragraph_str(CString& text, std::deque<std::deque<CSCParagraph>>& para, CSCTextProperty* text_prop)
{
	int i;
	CString cr_str;
	CString str;
	std::deque<CString> tags;
	int line = 0;

	//<ls=값> 으로 지정된, 현재 build 중인 라인의 윗 간격 factor. 라인을 flush 할 때 그 라인 첫 run 에 기록 후 -1 로 리셋.
	float pending_line_spacing = -1.0f;

	//"<b><cr=red>This</b></cr> is a <cr=blue><i>sample</i> <b>paragraph</b>."
	get_tag_str(text, tags);

	CSCParagraph basic_para, para_temp;
	basic_para.text_prop = *text_prop;
	para_temp = basic_para;

	std::deque<CSCParagraph> para_line;

	//시작 태그를 만나면 속성을 세팅하고
	//단순 텍스트를 만나면 해당 속성과 함께 paragraph로 push하고
	//끝 태그를 만나면 끝 태그가 끝날때까지 진행하고 끝 태그를 만나면 그 속성을 해제한다. 
	for (i = 0; i < tags.size(); i++)
	{
		if (tags[i] == _T("<b>"))
		{
			para_temp.text_prop.style |= Gdiplus::FontStyleBold;
		}
		else if (tags[i] == _T("</b>"))
		{
			para_temp.text_prop.style &= ~Gdiplus::FontStyleBold;
		}
		else if (tags[i] == _T("<i>"))
		{
			para_temp.text_prop.style |= Gdiplus::FontStyleItalic;
		}
		else if (tags[i] == _T("</i>"))
		{
			para_temp.text_prop.style &= ~Gdiplus::FontStyleItalic;
		}
		else if (tags[i] == _T("<u>"))
		{
			para_temp.text_prop.style |= Gdiplus::FontStyleUnderline;
		}
		else if (tags[i] == _T("</u>"))
		{
			para_temp.text_prop.style &= ~Gdiplus::FontStyleUnderline;
		}
		else if (tags[i] == _T("<s>"))
		{
			para_temp.text_prop.style |= Gdiplus::FontStyleStrikeout;
		}
		else if (tags[i] == _T("</s>"))
		{
			para_temp.text_prop.style &= ~Gdiplus::FontStyleStrikeout;
		}
		else if (tags[i].Find(_T("<cr=")) >= 0 || tags[i].Find(_T("<ct=")) >= 0)
		{
			cr_str = tags[i].Mid(4, tags[i].GetLength() - 5);
			para_temp.text_prop.cr_text = get_color(cr_str);
		}
		else if (tags[i].Find(_T("</cr>")) >= 0 || tags[i].Find(_T("</ct>")) >= 0)
		{
			para_temp.text_prop.cr_text = basic_para.text_prop.cr_text;
		}
		else if (tags[i].Find(_T("<cb=")) >= 0)
		{
			cr_str = tags[i].Mid(4, tags[i].GetLength() - 5);
			para_temp.text_prop.cr_back = get_color(cr_str);
		}
		else if (tags[i].Find(_T("<crb=")) >= 0)
		{
			cr_str = tags[i].Mid(5, tags[i].GetLength() - 6);
			para_temp.text_prop.cr_back = get_color(cr_str);
		}
		else if (tags[i].Find(_T("</cb>")) >= 0 || tags[i].Find(_T("</crb>")) >= 0)
		{
			para_temp.text_prop.cr_back = basic_para.text_prop.cr_back;
		}
		else if (tags[i].Find(_T("<f=")) >= 0)
		{
			CString str_font = tags[i].Mid(3, tags[i].GetLength() - 4);
			_tcscpy_s(para_temp.text_prop.name, str_font);
		}
		else if (tags[i].Find(_T("<font=")) >= 0)
		{
			CString str_font = tags[i].Mid(6, tags[i].GetLength() - 7);
			_tcscpy_s(para_temp.text_prop.name, str_font);
		}
		else if (tags[i].Find(_T("<name=")) >= 0)
		{
			CString str_font = tags[i].Mid(6, tags[i].GetLength() - 7);
			_tcscpy_s(para_temp.text_prop.name, str_font);
		}
		else if (tags[i].Find(_T("<font_name=")) >= 0)
		{
			CString str_font = tags[i].Mid(11, tags[i].GetLength() - 12);
			_tcscpy_s(para_temp.text_prop.name, str_font);
		}
		else if (tags[i].Find(_T("</f>")) >= 0 || tags[i].Find(_T("</font>")) >= 0 || tags[i].Find(_T("</name>")) >= 0 || tags[i].Find(_T("</font_name>")) >= 0)
		{
			_tcscpy_s(para_temp.text_prop.name, basic_para.text_prop.name);
		}
		else if (tags[i].Find(_T("<sz=")) >= 0)
		{
			CString str_size = tags[i].Mid(4, tags[i].GetLength() - 5);
			para_temp.text_prop.size = _ttoi(str_size);
		}
		else if (tags[i].Find(_T("<size=")) >= 0)
		{
			CString str_size = tags[i].Mid(6, tags[i].GetLength() - 7);
			para_temp.text_prop.size = _ttoi(str_size);
		}
		else if (tags[i].Find(_T("<font_size=")) >= 0)
		{
			CString str_size = tags[i].Mid(11, tags[i].GetLength() - 12);
			para_temp.text_prop.size = _ttoi(str_size);
		}
		else if (tags[i].Find(_T("</sz>")) >= 0 || tags[i].Find(_T("</size>")) >= 0 || tags[i].Find(_T("</font_size>")) >= 0)
		{
			para_temp.text_prop.size = basic_para.text_prop.size;
		}
		else if (tags[i].Find(_T("<ls=")) >= 0)
		{
			//<ls=값> 은 "이전 줄과의 간격" 이라 그 자체로 줄 경계 — 진행 중인 라인이 있으면 flush 하여 새 라인을 시작한다.
			//별도 <br> 불필요. 이미 라인이 비어 있으면 (직전 <br> 등) 줄바꿈은 건너뛰고 다음 라인의 간격만 설정 → <br><ls=..> 도 빈 줄·중복 줄바꿈 없이 동작.
			if (para_line.size())
			{
				para_line[0].line_spacing = pending_line_spacing;
				para.push_back(para_line);
				para_line.clear();
				line++;
			}

			CString str_ls = tags[i].Mid(4, tags[i].GetLength() - 5);
			pending_line_spacing = (float)_tstof(str_ls);
		}
		else if (tags[i] == _T("<br>"))
		{
			//<br>에 의해 공백 라인이 추가된 경우
			if (para_line.size() == 0)
			{
				para_line.push_back(basic_para);
			}

			para_line[0].line_spacing = pending_line_spacing;
			para.push_back(para_line);
			para_line.clear();
			pending_line_spacing = -1.0f;
			line++;
		}
		else
		{
			para_temp.text = tags[i];

			//전체 배경색인 cr_back이 Transparent가 아닐 때 색상이 별도로 지정되지 않은 para[][].cr_back에 cr_back을 줄 경우 중복으로 그려지게 된다.
			//cr_back이 불투명이면 덮어써서 그려져서 표가 나지 않지만 반투명이면 겹쳐져 그려지게 된다.
			if (para_temp.text_prop.cr_back.GetValue() == basic_para.text_prop.cr_back.GetValue())
				para_temp.text_prop.cr_back = Gdiplus::Color::Transparent;

			para_line.push_back(para_temp);
		}
	}

	if (para_line.size())
	{
		para_line[0].line_spacing = pending_line_spacing;
		para.push_back(para_line);
	}
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

		for (j = 0; j < para[i].size(); j++)
		{
			//char_spacing: 같은 라인의 두 번째 run 부터 spacing 만큼 left 를 미리 옮긴다 (run 사이 간격).
			if (j > 0 && char_spacing != 0)
				sz_text.cx += char_spacing;

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
			//TRACE(_T("[%d][%d] text = %s, sz = %dx%d, r = %s\n"), i, j, para[i][j].text, sz.cx, sz.cy, get_rect_info_string(para[i][j].r));
			sz_text.cx += sz.cx;

			//한 라인에서 가장 cy가 큰 값을 기억시킨다.
			sz_text.cy = MAX(sz_text.cy, sz.cy);
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

	//align에 따른 보정
	if (align & DT_CENTER)
	{
		//각 라인마다 total_width를 구하고
		for (i = 0; i < para.size(); i++)
		{
			int total_width = 0;
			for (j = 0; j < para[i].size(); j++)
			{
				//char_spacing 도 라인 너비에 반영 — 안 그러면 negative spacing 시 라인이 실제보다 넓다고 계산되어 왼쪽으로 밀려 widest 라인의 첫 글자가 잘림.
				if (j > 0 && char_spacing != 0)
					total_width += char_spacing;
				total_width += para[i][j].r.Width();
			}

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
			{
				if (j > 0 && char_spacing != 0)
					total_width += char_spacing;
				total_width += para[i][j].r.Width();
			}

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


	if (para.size() > 0)
	{
		//m_pt_icon.x = m_para[m_max_width_line][0].r.left - m_sz_icon.cx - 4;
		//아이콘을 top 정렬하느냐, 모든 라인의 vcenter에 정렬하느냐...
		//m_pt_icon.y = m_para[0][0].r.top;

		rect_text.left = para[max_width_line][0].r.left;	//최대 넓이 라인의 0번 아이템의 left
		rect_text.top = para[0][0].r.top;					//최상단 항목의 top
		rect_text.right = para[max_width_line][para[max_width_line].size() - 1].r.right;	//최대 넓이 라인의 마지막 항목의 right
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
		if (!para[i].empty() && para[i][0].line_spacing >= 0.0f && !para[i][0].wrap_continuation)
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
	float emSize = fDpiY * text_prop.size / 96.0;

	Gdiplus::FontFamily fontFamily((WCHAR*)(const WCHAR*)CStringW(text_prop.name));

	std::unique_ptr<Gdiplus::Font> ff;

	if (fontFamily.IsAvailable())
		ff = std::make_unique<Gdiplus::Font>(&fontFamily, emSize, text_prop.style);
	else
		ff = std::make_unique<Gdiplus::Font>(Gdiplus::FontFamily::GenericSansSerif(), emSize, text_prop.style);

	*font = ff->Clone();
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

	//Gaussian blur 그림자 사전 패스.
	//음절별 단순 offset fillpath 가 아닌, 모든 음절을 별도 layer 에 모아 한 번 blur 하면
	//음절 사이가 자연스럽게 번지고 가장자리도 부드럽다. para[0][0] 의 sigma/weight 를 대표값으로 사용한다.
	float global_blur_sigma = 0.0f;
	float global_gray_weight = 1.0f;
	if (!para.empty() && !para[0].empty())
	{
		global_blur_sigma = para[0][0].text_prop.shadow_blur_sigma;
		global_gray_weight = para[0][0].text_prop.shadow_gray_weight;
	}

	if (global_blur_sigma > 0.0f)
	{
		Gdiplus::RectF clip;
		g.GetVisibleClipBounds(&clip);
		int layer_w = (int)ceil(clip.X + clip.Width);
		int layer_h = (int)ceil(clip.Y + clip.Height);

		if (layer_w > 0 && layer_h > 0)
		{
			CSCGdiplusBitmap shadow_layer(layer_w, layer_h, Gdiplus::Color::Transparent, PixelFormat32bppARGB);
			Gdiplus::Graphics gs(shadow_layer.m_pBitmap);
			gs.SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);
			gs.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

			for (i = 0; i < para.size(); i++)
			{
				for (j = (int)para[i].size() - 1; j >= 0; j--)
				{
					if (para[i][j].text_prop.shadow_depth == 0)
						continue;

					Gdiplus::FontFamily* ff = new Gdiplus::FontFamily((WCHAR*)(const WCHAR*)CStringW(para[i][j].text_prop.name));
					if (!ff->IsAvailable())
					{
						delete ff;
						ff = Gdiplus::FontFamily::GenericSansSerif()->Clone();
					}

					float emSize = fDpiY * para[i][j].text_prop.size / 72.0f;

					CPoint off;
					if (para[i][j].text_prop.shadow_depth > 0)
					{
						off.x = (LONG)para[i][j].text_prop.shadow_depth;
						off.y = (LONG)para[i][j].text_prop.shadow_depth;
					}
					else
					{
						off.x = (LONG)max((float)para[i][j].r.Height() / 30.0f, 2.0f);
						off.x = max(off.x, (LONG)(para[i][j].text_prop.thickness / 1.4f));
						off.y = (LONG)max((float)para[i][j].r.Height() / 30.0f, 2.0f);
						off.y = max(off.y, (LONG)(para[i][j].text_prop.thickness / 1.4f));
					}

					Gdiplus::GraphicsPath shadow_path;
					shadow_path.SetFillMode(Gdiplus::FillModeWinding);
					shadow_path.AddString(CStringW(para[i][j].text), para[i][j].text.GetLength(), ff,
						para[i][j].text_prop.style, emSize,
						Gdiplus::Point(para[i][j].r.left + off.x, para[i][j].r.top + off.y),
						sf.GenericTypographic());

					Gdiplus::SolidBrush br_shadow(para[i][j].text_prop.cr_shadow);
					gs.FillPath(&br_shadow, &shadow_path);

					delete ff;
				}
			}

			shadow_layer.blur(global_blur_sigma);
			if (global_gray_weight > 0.0f && global_gray_weight < 1.0f)
				shadow_layer.gray(global_gray_weight);

			g.DrawImage(shadow_layer.m_pBitmap, 0, 0);
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

			//음절별 폰트 종류 + 크기로 hint 자동 결정.
			//각 폰트는 비트맵 보유 범위가 달라 단일 임계치로 부정확 — 폰트별 매핑(get_AA_from_pt)을 거쳐
			//effective threshold 를 구한 뒤 size 와 비교한다.
			//- size >= effective → AntiAliasGridFit (큰 글씨 외곽 매끈)
			//- size <  effective → ClearTypeGridFit  (작은 글씨 비트맵/서브픽셀로 또렷)
			//AA_from_pt == 0 이면 자동 결정 비활성 (호출자가 미리 설정한 hint 유지).
			if (AA_from_pt > 0)
			{
				int effective = CSCParagraph::get_AA_from_pt(para[i][j].text_prop.name, AA_from_pt, dark_background);
				g.SetTextRenderingHint(para[i][j].text_prop.size >= effective
					? Gdiplus::TextRenderingHintAntiAliasGridFit
					: Gdiplus::TextRenderingHintClearTypeGridFit);
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
			//stroke/shadow 없는 평문 run 은 GDI+ DrawString 으로 그린다. 작은 글자 또렷함은 위에서 set 한
			//ClearTypeGridFit 힌트가 담당. (GDI TextOut 으로 그리면 렌더 폭이 GDI+ 측정 rect 와 어긋나
			// run 겹침·배경 오정렬이 발생하므로 측정·렌더를 GDI+ 로 일원화한다.)
			if (para[i][j].text_prop.shadow_depth == 0 && para[i][j].text_prop.thickness == 0)
			{
				Gdiplus::SolidBrush text_brush(para[i][j].text_prop.cr_text);
				g.DrawString(CStringW(para[i][j].text), -1, font,
					Gdiplus::PointF((Gdiplus::REAL)para[i][j].r.left, (Gdiplus::REAL)para[i][j].r.top), sf.GenericTypographic(), &text_brush);
			}
			else
			{
				float emSize = fDpiY * para[i][j].text_prop.size / 72.0;
				Gdiplus::GraphicsPath str_path, shadow_path;

				//겹치는 부분을 반전시키지 않는다. FillModeAlternate는 반전시킴.
				str_path.SetFillMode(Gdiplus::FillModeWinding);
				shadow_path.SetFillMode(Gdiplus::FillModeWinding);

				//AddString() 파라미터 중 출력위치를 줄 때 Gdiplus::Rect() 또는 Gdiplus::Point()로 줄 수 있는데
				//stroke 또는 shadow가 추가되어 r이 작으면 텍스트가 출력되지 않는 현상이 있다.
				//r을 정확히 계산하는 것이 정석이나 굳이 r을 주지 않고 Gdiplus::Point()로 주면 문제되지 않는다.
				CRect r = para[i][j].r;
				str_path.AddString(CStringW(para[i][j].text), para[i][j].text.GetLength(), fontFamily,
					para[i][j].text_prop.style, emSize, Gdiplus::Point(r.left, r.top), sf.GenericTypographic());

				//사전 패스에서 blur 그림자를 이미 그렸다면 음절 단위 하드 엣지 그림자는 건너뛴다.
				if (para[i][j].text_prop.shadow_depth != 0 && global_blur_sigma <= 0.0f)
				{
					Gdiplus::SolidBrush br_shadow(para[0][0].text_prop.cr_shadow);

					CPoint pt_shadow_offset;
					if (para[i][j].text_prop.shadow_depth > 0)
					{
						pt_shadow_offset.x = (LONG)para[i][j].text_prop.shadow_depth;
						pt_shadow_offset.y = (LONG)para[i][j].text_prop.shadow_depth;
					}
					else
					{
						//자동 계산: 텍스트 height에 비례하고 stroke thickness 유무와도 관계있다.
						pt_shadow_offset.x = max((float)(para[i][j].r.Height()) / 30.0f, 2.0f);
						pt_shadow_offset.x = max(pt_shadow_offset.x, para[i][j].text_prop.thickness / 1.4f);
						pt_shadow_offset.y = max((float)(para[i][j].r.Height()) / 30.0f, 2.0f);
						pt_shadow_offset.y = max(pt_shadow_offset.y, para[i][j].text_prop.thickness / 1.4f);
					}
					r.OffsetRect(pt_shadow_offset.x, pt_shadow_offset.y);

					shadow_path.AddString(CStringW(para[i][j].text), para[i][j].text.GetLength(), fontFamily,
						para[i][j].text_prop.style, emSize, Gdiplus::Point(r.left, r.top), sf.GenericTypographic());

					g.FillPath(&br_shadow, &shadow_path);
				}

				Gdiplus::Pen   pen(para[i][j].text_prop.cr_stroke, para[i][j].text_prop.thickness);
				Gdiplus::SolidBrush brush(para[i][j].text_prop.cr_text);

				//pen.SetLineJoin(Gdiplus::LineJoinMiter);
				pen.SetLineJoin(Gdiplus::LineJoinRound);

				//thickness가 0.0f이면 g.DrawPath()가 아닌 g.DrawString()으로 그리면 되고 이전 버전은 잘 그려졌으나
				//뭔가 옵셋이 틀어진 현상이 발생하여 우선 아래와 같이 조건에 의해 g.DrawPath()를 실행하도록 한다.
				if (para[i][j].text_prop.thickness > 0.0f)
					g.DrawPath(&pen, &str_path);

				g.FillPath(&brush, &str_path);
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
