#include "Subtitle.h"
#include <algorithm>
#include <fstream>

#pragma warning(disable: 4018)	//'>=': signed/unsigned mismatch
#pragma warning(disable: 4477)	//'fwprintf' : format string '%s' requires an argument of type 'wchar_t *', but variadic argument 1 has type 'CString'

//<font ...> 의 attribute body (open '<font' 와 close '>' 사이) 에서 color 값 추출.
//face/size 와 순서 무관, 인용·비인용 모두 지원. 못 찾으면 빈 문자열.
static CString extract_font_color_attr(const CString& body)
{
	CString lower = body;
	lower.MakeLower();
	int c = lower.Find(_T("color"));
	if (c < 0)
		return _T("");
	int eq = lower.Find(_T('='), c);
	if (eq < 0)
		return _T("");
	int v = eq + 1;
	int len = body.GetLength();
	while (v < len && (body[v] == _T(' ') || body[v] == _T('\t')))
		v++;
	if (v >= len)
		return _T("");
	int v_end;
	if (body[v] == _T('"'))
	{
		v++;
		v_end = v;
		while (v_end < len && body[v_end] != _T('"'))
			v_end++;
	}
	else
	{
		v_end = v;
		while (v_end < len && body[v_end] != _T(' ') && body[v_end] != _T('\t') && body[v_end] != _T('"'))
			v_end++;
	}
	CString result = body.Mid(v, v_end - v);
	result.Trim();
	result.Remove(_T('"'));
	result.Remove(_T('#'));
	return result;
}

CSubtitle::CSubtitle()
{
	reset();
}

bool CSubtitle::load_smi(CString sfile)
{
	//read() 가 BOM/byte 통계로 인코딩 정확 판별 → CString 으로 한 번에 변환. get_token_str 으로 라인 분리.
	std::deque<CString> lines;
	if (!read_lines(sfile, &lines))
		return false;

	CString sSync;			//sync 시작 단위로 누적 후 한 번에 parse_subtltle 호출.
	bool header_part = true;

	for (CString& sLine : lines)
	{
		if (header_part)
		{
			//"<body" 부분 매칭 — "<BODY CLASS=...>" 같은 attribute 있는 형식도 처리.
			//기존 "<body>" literal 검사는 attribute 있는 흔한 형식을 놓쳐 모든 라인을 header 로 흡수했었음. find() 는 case-insensitive.
			if (find(sLine, _T("<body")) >= 0)
				header_part = false;

			//read_lines 가 \r\n → \n 정규화 후 \n separator 로 분리 → 각 라인에 \n 미포함. 헤더 재구성 시 명시 추가.
			m_header += sLine;
			m_header += _T("\n");

			continue;
		}

		//"</body" 매칭 — "</BODY >" 같은 변형도 처리.
		if (find(sLine, _T("</body")) >= 0)
		{
			if (!sSync.IsEmpty())
				parse_subtltle(sSync);
			sSync.Empty();
			break;
		}

		if (find(sLine, _T("<sync start")) >= 0)
		{
			//이전 sync 누적이 있으면 먼저 flush.
			if (!sSync.IsEmpty())
			{
				parse_subtltle(sSync);
				sSync.Empty();
			}

			//자막끝 (&nbsp;) 마커 라인이면 직전 caption 의 end 만 갱신, sSync 누적은 건너뜀.
			if (find(sLine, _T("&nbsp;")) > 0)
			{
				int eq = sLine.Find(_T('='));
				int gt = (eq >= 0) ? sLine.Find(_T('>'), eq) : -1;
				if (eq >= 0 && gt > eq && !m_subtitle.empty())
				{
					int t = _ttoi(sLine.Mid(eq + 1, gt - eq - 1));
					//역행 방지 — 간혹 end 가 직전 start 보다 작은 잘못된 SMI 가 있음.
					if (t > m_subtitle.back().start)
						m_subtitle.back().end = t;
				}
			}
			else
			{
				sSync = sLine;
			}
			continue;
		}

		//sync 시작 라인이 아닌 일반 텍스트 라인 — 현재 sync 에 누적.
		sSync += sLine;
	}

	//파일이 </body> 없이 끝났을 때 마지막 sync 도 flush.
	if (!sSync.IsEmpty())
		parse_subtltle(sSync);

	if (find(m_header, _T(".encc")) >= 0)
		m_sLanguage = _T("ENCC");
	else if (find(m_header, _T(".jpcc")) >= 0)
		m_sLanguage = _T("JPCC");
	else
		m_sLanguage = _T("KRCC");

	//default 활성 트랙 — KRCC 가 있으면 KRCC, 없으면 첫 트랙.
	//사용자 메뉴 선택으로 set_active_classes 로 변경 가능.
	m_active_classes.clear();
	if (!m_tracks.empty())
	{
		if (m_tracks.find(_T("KRCC")) != m_tracks.end())
			m_active_classes.push_back(_T("KRCC"));
		else
			m_active_classes.push_back(m_tracks.begin()->first);
	}
	rebuild_active_view();

	return true;
}

std::vector<CString> CSubtitle::get_classes() const
{
	std::vector<CString> result;
	//정렬: KRCC → ENCC → JPCC → 기타 (alpha).
	const TCHAR* priority[] = { _T("KRCC"), _T("ENCC"), _T("JPCC") };
	for (const TCHAR* p : priority)
		if (m_tracks.find(p) != m_tracks.end())
			result.push_back(p);
	for (const auto& kv : m_tracks)
	{
		bool already = false;
		for (const TCHAR* p : priority)
			if (kv.first == p) { already = true; break; }
		if (!already)
			result.push_back(kv.first);
	}
	return result;
}

void CSubtitle::set_active_classes(const std::vector<CString>& classes)
{
	m_active_classes = classes;
	rebuild_active_view();
}

void CSubtitle::rebuild_active_view()
{
	m_subtitle.clear();

	//m_tracks 가 비어있으면 srt 등 단일 트랙 경로 — m_subtitle 은 이미 load_srt 가 채움.
	//rebuild 가 m_subtitle 을 지우면 srt 자막 손실 → 그 케이스는 m_tracks 가 없음을 검사해 skip.
	if (m_tracks.empty())
		return;

	auto append_track = [this](const std::deque<CCaption>& track)
	{
		for (const auto& cap : track)
			m_subtitle.push_back(cap);
	};

	if (m_active_classes.empty())
	{
		//명시적 비활성 — 자막 표시 끄기. m_subtitle 비워둠.
		return;
	}

	for (const CString& cls : m_active_classes)
	{
		auto it = m_tracks.find(cls);
		if (it != m_tracks.end())
			append_track(it->second);
	}

	//start 기준 정렬.
	std::sort(m_subtitle.begin(), m_subtitle.end(),
		[](const CCaption& a, const CCaption& b) { return a.start < b.start; });

	//같은 start 의 caption merge — 두 Class 동시 활성 시 한 sync 에 양 언어 sentences 합침.
	for (size_t i = 1; i < m_subtitle.size();)
	{
		if (m_subtitle[i].start == m_subtitle[i - 1].start)
		{
			for (const auto& s : m_subtitle[i].sentences)
				m_subtitle[i - 1].sentences.push_back(s);
			if (m_subtitle[i].end > m_subtitle[i - 1].end)
				m_subtitle[i - 1].end = m_subtitle[i].end;
			m_subtitle.erase(m_subtitle.begin() + i);
		}
		else
			++i;
	}
}

bool CSubtitle::load_srt(CString sfile)
{
	//read() 가 BOM/byte 통계로 인코딩 정확 판별 → CString 으로 한 번에 변환. get_token_str 으로 라인 분리.
	std::deque<CString> lines;
	if (!read_lines(sfile, &lines))
		return false;

	CCaption caption;
	CString color;

	//인라인 스타일 태그 제거 (color 는 별도 처리). i/b/u/s 대소문자 + ASS override {\an8} 등.
	auto strip_inline_tags = [](CString& s)
	{
		static const TCHAR* tags[] = {
			_T("<i>"), _T("</i>"), _T("<I>"), _T("</I>"),
			_T("<b>"), _T("</b>"), _T("<B>"), _T("</B>"),
			_T("<u>"), _T("</u>"), _T("<U>"), _T("</U>"),
			_T("<s>"), _T("</s>"), _T("<S>"), _T("</S>")
		};
		for (auto t : tags)
			s.Replace(t, _T(""));

		int p1;
		while ((p1 = s.Find(_T('{'))) >= 0)
		{
			int p2 = s.Find(_T('}'), p1);
			if (p2 < 0)
				break;
			s.Delete(p1, p2 - p1 + 1);
		}
	};

	for (CString& sLine : lines)
	{
		sLine.Trim();

		//시간 라인 감지: "HH:MM:SS,mmm --> HH:MM:SS,mmm [X1:N X2:N ...]"
		//기존 char-count 휴리스틱은 점(.) decimal 또는 positioning extension 이 붙은 형태에서 fail.
		//"-->" 토큰 + 양쪽 콜론 개수 ≥ 2 로 판정. right 에 따라오는 cue settings 는 첫 whitespace 에서 cut.
		int arrow = sLine.Find(_T("-->"));
		if (arrow > 0)
		{
			CString left = sLine.Left(arrow);
			CString right = sLine.Mid(arrow + 3);
			left.Trim();
			right.Trim();

			int sp = right.FindOneOf(_T(" \t"));
			if (sp > 0)
				right = right.Left(sp);

			if (get_char_count(left, ':') >= 2 && get_char_count(right, ':') >= 2)
			{
				//이전 caption 이 blank line 누락 등으로 push 안 됐으면 여기서 flush.
				if (caption.is_valid())
				{
					m_subtitle.push_back(caption);
					caption.reset();
				}
				caption.start = GetMilliSecondsFromTimeString(left);
				caption.end = GetMilliSecondsFromTimeString(right);
				continue;
			}
		}

		//아직 시간 태그를 만나기 전이라면 순번 (또는 WEBVTT 헤더 등) 으로 보고 pass.
		if (caption.start == -1)
			continue;

		//blank line 이면 해당 caption 의 끝.
		if (sLine.IsEmpty())
		{
			if (caption.is_valid())
				m_subtitle.push_back(caption);
			caption.reset();
			continue;
		}

		//스타일/ASS 태그 먼저 제거 — 그래야 다음의 <font color> 추출이 first '=' / '>' 매칭 오류를 안 일으킴.
		strip_inline_tags(sLine);

		//<font ...> 안의 color attribute 추출 — face/size 등이 함께 있어도 무관.
		int font_open = sLine.Find(_T("<font"));
		if (font_open < 0)
		{
			CString lo = sLine;
			lo.MakeLower();
			font_open = lo.Find(_T("<font"));
		}

		if (font_open >= 0)
		{
			int font_close = sLine.Find(_T('>'), font_open);
			if (font_close > font_open + 5)
			{
				CString body = sLine.Mid(font_open + 5, font_close - font_open - 5);
				color = extract_font_color_attr(body);
				sLine.Delete(font_open, font_close - font_open + 1);
			}
		}
		else if (find(sLine, _T("</font>")) >= 0)
		{
			//컬러끝 태그만 있으면 바로 전 캡션의 색상을 유지.
			if (caption.sentences.size() > 0)
				color = caption.sentences[caption.sentences.size() - 1].color;
		}
		else
		{
			color.Empty();
		}

		sLine.Replace(_T("</font>"), _T(""));
		sLine.Replace(_T("</FONT>"), _T(""));

		if (!sLine.IsEmpty())
			caption.sentences.push_back(CSentence(sLine, color));
	}

	//파일이 trailing blank line 없이 끝났을 때 마지막 caption 도 flush.
	if (caption.is_valid())
		m_subtitle.push_back(caption);

	return true;
}

//ASS / SSA — [Events] section 의 Format header 로 Start/End/Text 컬럼 위치 파악 후 Dialogue 라인 parse.
//Time 형식 H:MM:SS.cc (centisecond) 는 cs × 10 = ms 직접 변환 — SRT 의 3-digit ms 와 단위 다름.
//Text 안의 override tag ({\an8}, {\pos}, {\1c&H...} 등) 제거. \N → newline.
bool CSubtitle::load_ass(CString sfile)
{
	std::deque<CString> lines;
	if (!read_lines(sfile, &lines))
		return false;

	auto parse_ass_time_ms = [](const CString& s) -> int
	{
		int h = 0, m = 0, sec = 0, cs = 0;
		if (_stscanf_s(s, _T("%d:%d:%d.%d"), &h, &m, &sec, &cs) >= 3)
			return h * 3600000 + m * 60000 + sec * 1000 + cs * 10;
		return -1;
	};

	//ASS override block parser — {...} 단위 처리.
	//지원: \an<n> (caption alignment) / \1c|\c (primary color) / \b1|\b0 (bold) / \i1|\i0 (italic).
	//변환된 SCParagraph tag (<cr=...>, <b>, <i>) 는 block 위치에 insert. 미지원 tag (\fad, \t, \move, \frz, \k 등) 는 skip.
	auto convert_ass_tags = [](CString& s, int& alignment_out)
	{
		alignment_out = 0;
		CString out;
		const int len = s.GetLength();
		int i = 0;

		while (i < len)
		{
			if (s[i] != _T('{'))
			{
				out += s[i];
				i++;
				continue;
			}
			//{ block 시작 — 짝 } 찾기.
			int close = s.Find(_T('}'), i);
			if (close < 0)
			{
				out += s.Mid(i);
				break;
			}
			CString block = s.Mid(i + 1, close - i - 1);
			i = close + 1;

			//block 안의 모든 \tag 순회.
			CString sc_tags;
			const int blen = block.GetLength();
			int j = 0;
			while (j < blen)
			{
				if (block[j] != _T('\\'))
				{
					j++;
					continue;
				}
				//\an<n>
				if (j + 3 < blen && block.Mid(j, 3) == _T("\\an") &&
					block[j + 3] >= _T('1') && block[j + 3] <= _T('9'))
				{
					alignment_out = block[j + 3] - _T('0');
					j += 4;
					continue;
				}
				//\1c&H...& or \c&H...&
				bool is_color = false;
				int prefix_len = 0;
				if (j + 5 <= blen && block.Mid(j, 5) == _T("\\1c&H")) { is_color = true; prefix_len = 5; }
				else if (j + 4 <= blen && block.Mid(j, 4) == _T("\\c&H")) { is_color = true; prefix_len = 4; }
				if (is_color)
				{
					int cs = j + prefix_len;
					int ce = cs;
					while (ce < blen && block[ce] != _T('&'))
						ce++;
					if (ce <= blen && (ce - cs) == 6)
					{
						CString bbggrr = block.Mid(cs, 6);
						sc_tags.AppendFormat(_T("<cr=#%s%s%s>"),
							bbggrr.Mid(4, 2).GetString(),
							bbggrr.Mid(2, 2).GetString(),
							bbggrr.Mid(0, 2).GetString());
						j = ce + 1;
						continue;
					}
				}
				//\b1 / \b0
				if (j + 2 < blen && block[j + 1] == _T('b') &&
					(block[j + 2] == _T('0') || block[j + 2] == _T('1')))
				{
					sc_tags += (block[j + 2] == _T('1')) ? _T("<b>") : _T("</b>");
					j += 3;
					continue;
				}
				//\i1 / \i0
				if (j + 2 < blen && block[j + 1] == _T('i') &&
					(block[j + 2] == _T('0') || block[j + 2] == _T('1')))
				{
					sc_tags += (block[j + 2] == _T('1')) ? _T("<i>") : _T("</i>");
					j += 3;
					continue;
				}
				//미지원 tag — 다음 \ 또는 block 끝까지 skip.
				int next = j + 1;
				while (next < blen && block[next] != _T('\\'))
					next++;
				j = next;
			}
			out += sc_tags;
		}

		//ASS escape: \N = newline, \h = non-breaking space.
		out.Replace(_T("\\N"), _T("\n"));
		out.Replace(_T("\\n"), _T("\n"));
		out.Replace(_T("\\h"), _T(" "));
		s = out;
	};

	bool in_events_section = false;
	int idx_start = -1, idx_end = -1, idx_text = -1;
	int format_field_count = 0;

	for (CString& sLine : lines)
	{
		sLine.Trim();
		if (sLine.IsEmpty())
			continue;

		//섹션 헤더 — [Events] 시작 / 다른 섹션 진입 시 종료.
		if (sLine[0] == _T('['))
		{
			CString lower = sLine; lower.MakeLower();
			in_events_section = (lower == _T("[events]"));
			idx_start = idx_end = idx_text = -1;
			format_field_count = 0;
			continue;
		}

		if (!in_events_section)
			continue;

		//"Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text"
		if (sLine.Left(7).CompareNoCase(_T("Format:")) == 0)
		{
			CString fields = sLine.Mid(7);
			std::deque<CString> tokens;
			get_token_str(fields, tokens, _T(","));
			format_field_count = (int)tokens.size();
			for (int i = 0; i < (int)tokens.size(); i++)
			{
				CString t = tokens[i]; t.Trim(); t.MakeLower();
				if (t == _T("start"))      idx_start = i;
				else if (t == _T("end"))   idx_end = i;
				else if (t == _T("text"))  idx_text = i;
			}
			continue;
		}

		//"Dialogue: 0,0:00:01.00,0:00:02.00,Default,,0,0,0,,1초"
		if (sLine.Left(9).CompareNoCase(_T("Dialogue:")) == 0)
		{
			if (idx_start < 0 || idx_end < 0 || idx_text < 0 || format_field_count <= 0)
				continue;	//Format header 없이는 parse 불가.

			CString body = sLine.Mid(9);
			//앞 (format_field_count - 1) 개 까지만 comma 로 split. 마지막 Text 필드는 콤마 포함 가능.
			std::deque<CString> tokens;
			int pos = 0;
			int field = 0;
			while (field < format_field_count - 1)
			{
				int comma = body.Find(_T(','), pos);
				if (comma < 0)
					break;
				CString t = body.Mid(pos, comma - pos); t.Trim();
				tokens.push_back(t);
				pos = comma + 1;
				field++;
			}
			tokens.push_back(body.Mid(pos));	//마지막 Text 필드 — 콤마 포함 가능.

			if ((int)tokens.size() != format_field_count)
				continue;

			int start_ms = parse_ass_time_ms(tokens[idx_start]);
			int end_ms = parse_ass_time_ms(tokens[idx_end]);
			if (start_ms < 0 || end_ms < 0)
				continue;

			CString text = tokens[idx_text];
			int alignment = 0;
			convert_ass_tags(text, alignment);
			text.Trim();
			if (text.IsEmpty())
				continue;

			CCaption caption;
			caption.start = start_ms;
			caption.end = end_ms;
			caption.alignment = alignment;
			//\n split — 각 line 별 CSentence.
			std::deque<CString> sub_lines;
			get_token_str(text, sub_lines, _T("\n"));
			for (CString& ln : sub_lines)
			{
				ln.Trim();
				if (!ln.IsEmpty())
					caption.sentences.push_back(CSentence(ln, _T("")));
			}
			if (caption.is_valid())
				m_subtitle.push_back(caption);
		}
	}

	return !m_subtitle.empty();
}

bool CSubtitle::load_subtitle_file(CString sfile)
{
	if (sfile.IsEmpty())
	{
		if (m_sfile.IsEmpty() == false)
				sfile = m_sfile;
		else
				return false;
	}

	m_sfile = sfile;
	m_header.Empty();
	m_sLanguage.Empty();
	m_subtitle.clear();
	m_tracks.clear();
	m_active_classes.clear();

	if (PathFileExists(sfile) == false)
		return false;

	//인코딩 판별·파일 열기는 load_smi / load_srt 가 read_lines() (Common helper) 로 처리.
	//여기서는 확장자 dispatch 만.
	CString ext = get_part(sfile, fn_ext);
	ext.MakeLower();

	if (ext == _T("smi"))
		return load_smi(sfile);
	else if (ext == _T("srt"))
		return load_srt(sfile);
	else if (ext == _T("ass") || ext == _T("ssa"))
		return load_ass(sfile);

	return false;
}

void CSubtitle::parse_subtltle(CString src, const CString& default_class)
{
	//<SYNC Start=0><P Class=KRCC>\n<font color=FF0000>카밀로 9번째 자막<br>\n<font color=dcdcdc>http://chan5132.egloos.com/
	//L"<SYNC Start=2400><font color=\"#ff0000\">신 아따맘마 15화</font><br>\n<font color=\"#d097ff\">초원의 발자막[greenhill.o-r.kr]</font><br>\n<font color=\"#dcdcdc\">-'카밀로'님의 테마곡 자막 발췌-</font>\n"
	remove_other_tags(src);

	if (src.IsEmpty())
		return;

	CCaption caption;
	CString sToken;
	CString str;
	CString color;
	CString cls;	//이 sync 블록의 P Class — KRCC/ENCC/JPCC 등. 없으면 default 사용.
	int i;
	int pos1;

	sToken.Empty();

	for (i = 0; i < src.GetLength(); i++)
	{
		if (src[i] == '>')
		{
			//현재까지 추출된 태그를 파싱하여 정보를 추출한다.

			//시작시간인 경우
			if (find(sToken, _T("<sync start")) >= 0)
			{
				pos1 = sToken.Find(_T("=")) + 1;
				caption.start = _ttoi(sToken.Mid(pos1));
				sToken.Empty();
			}
			else if (find(sToken, _T("<p class")) >= 0)
			{
				pos1 = sToken.Find(_T("=")) + 1;
				if (pos1 > 0)
				{
					cls = sToken.Mid(pos1);
					cls.Trim();
					cls.MakeUpper();
				}
				sToken.Empty();
			}
			else if (find(sToken, _T("<font color")) >= 0)
			{
				pos1 = sToken.Find(_T("=")) + 1;
				color = sToken.Mid(pos1);
				remove_chars(color, _T("\"#"));
				sToken.Empty();
			}
			//한 문장의 끝에 <br> / <br/> / <br /> 등이 붙어있다면 제거하고 넣어준다.
			//기존 "Length-3" 고정 절단은 <br/> 같은 변형에서 잘못된 위치를 자름. 마지막 "<br" 위치부터 끝까지 모두 제거.
			else if (find(sToken, _T("<br")) >= 0)
			{
				CString lower = sToken;
				lower.MakeLower();
				int br_pos = lower.ReverseFind(_T('<'));
				if (br_pos >= 0 && lower.Mid(br_pos, 3) == _T("<br"))
					sToken = sToken.Left(br_pos);
				sToken.Replace(_T("&nbsp;"), _T(" "));
				caption.sentences.push_back(CSentence(sToken, color));
				sToken.Empty();
				color.Empty();
			}
			else if (find(sToken, _T("</font")) >= 0)
			{
				sToken.Empty();
			}
		}
		else
		{
			sToken += src[i];
		}
	}

	//Class 미지정 시 default 사용 (호출자가 알려준 값. 보통 KRCC).
	if (cls.IsEmpty())
		cls = default_class;

	auto& track = m_tracks[cls];

	//만약 여기까지 왔을 때 sToken에 값이 들어있다면?
	//caption에 sentences는 비어있고 자막끝 태그만 들어있다면
	//마지막으로 추가한 자막의 끝 태그인 것이므로 그 정보를 수정해준다.
	if (sToken.IsEmpty() == false)
	{
		sToken.Trim();
		if ((sToken == _T("&nbsp;")) && (caption.sentences.size() == 0))
		{
			//트랙이 비어있으면 직전 caption 이 없으므로 end 갱신 대상도 없음 — 그냥 skip.
			if (track.empty())
				return;
			//간혹 end싱크가 start싱크보다 빠르게 작성된 smi도 있으므로 이는 스킵시킨다.
			if (caption.start <= track.back().start)
				return;
			track.back().end = caption.start;
			return;
		}
	}

	//하나의 sync절의 분석이 끝났는데 sToken이 존재하면 이는 자막이니 넣어주자.
	if (sToken.IsEmpty() == false)
	{
		sToken.Replace(_T("&nbsp;"), _T(" "));
		caption.sentences.push_back(CSentence(sToken, color));
	}

	if (caption.sentences.size())
	{
		//만약 현재 자막이 마지막 자막의 시작 시간과 같다면
		//이전 자막에 추가시켜준다 (같은 Class 안에서 multi-line sync 처리).
		if (!track.empty() && caption.start == track.back().start)
		{
			for (i = 0; i < caption.sentences.size(); i++)
				track.back().sentences.push_back(caption.sentences[i]);
		}
		else
		{
			track.push_back(caption);
		}
	}
}

//처리 대상이 아닌 태그들을 삭제한다.
//ex. <font face>, </font>, <b>, <i>
void CSubtitle::remove_other_tags(CString &str)
{
	int pos1, pos2;
	CString lstr;
	
	str.Trim();
	str.Replace(_T("\n"), _T(""));
	lstr = str;
	lstr.MakeLower();

	//<font face=HY견고딕 size=6 color=yellowgreen>화 양 연 화</font><br><font face=HY중고딕 size=4 color=lightyellow>in the mood for love
	//face/size 와 color 가 섞여 있어도 color 만은 보존해 <font color=VALUE> 로 정규화 — 그래야 뒤따르는 parse_subtltle 의 <font color> 인식이 동작.
	while ((pos1 = lstr.Find(_T("<font face"))) >= 0)
	{
		pos2 = lstr.Find(BRACKET_CLOSE, pos1);
		if (pos2 > pos1)
		{
			int tag_len = pos2 - pos1 + 1;
			CString body = str.Mid(pos1 + 5, pos2 - pos1 - 5);
			CString color = extract_font_color_attr(body);

			CString replacement;
			if (!color.IsEmpty())
				replacement.Format(_T("<font color=%s>"), (LPCTSTR)color);

			lstr.Delete(pos1, tag_len);
			str.Delete(pos1, tag_len);
			if (!replacement.IsEmpty())
			{
				CString rep_lower = replacement;
				rep_lower.MakeLower();
				lstr.Insert(pos1, rep_lower);
				str.Insert(pos1, replacement);
			}
		}
	}

	while ((pos1 = lstr.Find(_T("</font"))) >= 0)
	{
		pos2 = lstr.Find(BRACKET_CLOSE, pos1);
		if (pos2 > pos1)
		{
			lstr.Delete(pos1, pos2 - pos1 + 1);
			str.Delete(pos1, pos2 - pos1 + 1);
		}
	}

	str.Replace(_T("<b>"), _T(""));
	str.Replace(_T("</b>"), _T(""));
	str.Replace(_T("<B>"), _T(""));
	str.Replace(_T("</B>"), _T(""));

	str.Replace(_T("<i>"), _T(""));
	str.Replace(_T("</i>"), _T(""));
	str.Replace(_T("<I>"), _T(""));
	str.Replace(_T("</I>"), _T(""));

	str.Replace(_T("</P>"), _T(""));
	str.Replace(_T("</p>"), _T(""));
}

bool CSubtitle::save_subtitle_file(CString sfile)
{
	if (sfile.IsEmpty())
		sfile = m_sfile;
	else
		m_sfile = sfile;

	CString ext = get_part(m_sfile, fn_ext);
	ext.MakeLower();

	if (ext == _T("smi"))
		return save_smi(m_sfile);
	else if (ext == _T("srt"))
		return save_srt(m_sfile);
	else
		return false;
}

bool CSubtitle::save_srt(CString sfile)
{
	FILE* fp = NULL;
	_tfopen_s(&fp, sfile, _T("wt, ccs=UNICODE"));
	if (fp == NULL)
		return false;

	int i, j;
	CString start, end;

	for (i = 0; i < m_subtitle.size(); i++)
	{
		_ftprintf(fp, _T("%d\n"), i + 1);

		start = GetTimeStringFromMilliSeconds(m_subtitle[i].start);
		end = GetTimeStringFromMilliSeconds(m_subtitle[i].end);

		start.Replace(_T("."), _T(","));
		end.Replace(_T("."), _T(","));
		_ftprintf(fp, _T("%s --> %s\n"), start, end);

		for (j = 0; j < m_subtitle[i].sentences.size(); j++)
		{
			if (m_subtitle[i].sentences[j].color.IsEmpty() == false)
			{
				_ftprintf(fp, _T("<font color=%s>%s</font>\n"),
								m_subtitle[i].sentences[j].color,
								m_subtitle[i].sentences[j].sentence);
			}
			else
			{
				_ftprintf(fp, _T("%s\n"), m_subtitle[i].sentences[j].sentence);
			}
		}

		_ftprintf(fp, _T("\n"));
	}

	fclose(fp);
	return true;
}

bool CSubtitle::save_smi(CString sfile)
{
	DWORD dwAttrs;
	dwAttrs = GetFileAttributes(sfile);

	//파일이 읽기전용이면 해제시켜서 저장한다.
	if (dwAttrs & FILE_ATTRIBUTE_READONLY)
		SetFileAttributes(sfile, FILE_ATTRIBUTE_NORMAL);

	FILE* fp = nullptr;
	_tfopen_s(&fp, sfile, _T("wt,ccs=UNICODE"));

	if (fp == NULL)
		return false;

	if (m_header.IsEmpty())
	{
		m_header.Format(_T("\
<SAMI>\n\
<HEAD>\n\
<TITLE>Legends Software 2020</TITLE>\n\
<STYLE TYPE=\"text/css\">\n\
<!--\n\
P { margin-left:8pt; margin-right:8pt; margin-bottom:2pt;\n\
\tmargin-top:2pt; font-size:14pt; text-align:center;\n\
\tfont-family:굴림, Arial; font-weight:normal; color:white;\n\
\tbackground-color:black; }\n\
.JPCC { Name:Japanese; lang:jp-JP; SAMIType:CC; }\n\
#STDPrn { Name:Standard Print; }\n\
#LargePrn { Name:Large Print; font-size:20pt; }\n\
#SmallPrn { Name:Small Print; font-size:10pt; }\n\
-->\n\
</STYLE>\n\
</HEAD>\n\
<BODY>\n\n"));
	}

	_ftprintf(fp, _T("%s"), m_header);

	if (m_sLanguage.IsEmpty())
		m_sLanguage = _T("JPCC");

	int i, j;

	for (i = 0; i < m_subtitle.size(); i++)
	{
		_ftprintf(fp, _T("<SYNC Start=%d><P Class=%s>\n"), m_subtitle[i].start, m_sLanguage);

		for (j = 0; j < m_subtitle[i].sentences.size(); j++)
		{
			//if (m_subtitle[i].sentences[j].color.IsEmpty() == false)
			//	_ftprintf(fp, _T("<font color=%s>"), m_subtitle[i].sentences[j].color);
			if (m_subtitle[i].sentences[j].color.IsEmpty() == false)
			{
				_ftprintf(fp, _T("<font color=%s>"), m_subtitle[i].sentences[j].color);
			}

			//끝문장이 아니고 현재 자막의 끝에 <br>이 없다면 다음 문장과 한문장으로 이어지므로
			//끝에 <br>을 붙여서 저장하자.
			m_subtitle[i].sentences[j].sentence.TrimRight();

			if (j < m_subtitle[i].sentences.size() - 1)
			{
				if (m_subtitle[i].sentences[j].sentence.GetLength() >= 4)
				{
					if (m_subtitle[i].sentences[j].sentence.Right(4) != _T("<br>"))
						m_subtitle[i].sentences[j].sentence += _T("<br>");
				}
				else
				{
					m_subtitle[i].sentences[j].sentence += _T("<br>");
				}
			}

			_ftprintf(fp, _T("%s"), m_subtitle[i].sentences[j].sentence);

			if (m_subtitle[i].sentences[j].color.IsEmpty() == false)
			{
				_ftprintf(fp, _T("</font>\n"));
			}
			else
			{
				_ftprintf(fp, _T("\n"));
			}
		}

		if (m_subtitle[i].end > 0)
			_ftprintf(fp, _T("<SYNC Start=%d><P Class=%s>&nbsp;\n"), m_subtitle[i].end, m_sLanguage);
	}

	_ftprintf(fp, _T("\n</BODY>\n</SAMI>\n"));

	fclose(fp);
	return true;
}

//field : 0(start), 1(end), 2(color), 3(caption)
void CSubtitle::modify(int sub_index, int sentence_index, int field, CString text)
{
	int value;

	if (field <= 1)
		value = _ttoi(text);

	if (field == 0)
		m_subtitle[sub_index].start = value;
	else if (field == 1)
		m_subtitle[sub_index].end = value;
	else if (field == 2 && m_subtitle.size())
	{
		if (sentence_index >= m_subtitle[sub_index].sentences.size())
			m_subtitle[sub_index].sentences.resize(sentence_index + 1);
		m_subtitle[sub_index].sentences[sentence_index].color = text;
	}
	else if (field == 3 && m_subtitle.size())
	{
		if (sentence_index >= m_subtitle[sub_index].sentences.size())
			m_subtitle[sub_index].sentences.resize(sentence_index + 1);
		m_subtitle[sub_index].sentences[sentence_index].sentence = text;
	}
	else
		AfxMessageBox(_T("never"));
}

int CSubtitle::get_subtitle_range(int pos, int &start, int &end)
{
	start = end = -1;

	if (m_subtitle.size() == 0 || pos < 0)
	{
		return -1;
	}

	int i;

	for (i = 0; i < m_subtitle.size(); i++)
	{
		if (pos < m_subtitle[i].start)
		{
			start = m_subtitle[i-1].start;

			if (m_subtitle[i-1].end > start)
			{
				end = m_subtitle[i-1].end;
				return i-1;
			}
			else
			{
				end = m_subtitle[i].start - 100;
				return i-1;
			}
		}
	}

	return -1;
}

//라인 단위로 삭제한다. sentence_index가 < 0이면 sub_index를 삭제한다.
void CSubtitle::delete_subtitle(int sub_index, int sentence_index)
{
	if (sub_index >= m_subtitle.size())
		return;

	if (sentence_index < 0)
	{

		m_subtitle.erase(m_subtitle.begin() + sub_index);
	}
	else
	{
		if (sentence_index >= m_subtitle[sub_index].sentences.size())
			return;

		m_subtitle[sub_index].sentences.erase(m_subtitle[sub_index].sentences.begin() + sentence_index);

		//자막이 모두 지워진 경우라면 해당 자막 항목을 삭제시켜준다.
		if (m_subtitle[sub_index].sentences.size() == 0)
			m_subtitle.erase(m_subtitle.begin() + sub_index);
	}
}

//시간값까지 있는 새로운 자막 항목을 추가한다.
void CSubtitle::add_subtitle(int sub_index, int start, int end, CCaption caption)
{
	caption.start = start;
	caption.end = end;
	m_subtitle.insert(m_subtitle.begin() + sub_index, caption);
}

//sub_index 자막의 sentence_index 다음에 문장을 추가시킨다.
//start시간이 다른 새로운 자막을 추가하는 것은 add_subtitle함수를 이용한다.
void CSubtitle::add_sentence(int sub_index, int sentence_index, CSentence sentence)
{
	m_subtitle[sub_index].sentences.insert(m_subtitle[sub_index].sentences.begin() + sentence_index + 1, sentence);
}

