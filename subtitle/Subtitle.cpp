#include "Subtitle.h"
#include "../log/SCLog/SCLog.h"
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
	//# 와 색상이름은 SCParagraph 의 get_color 가 모두 인식 — 보존.
	return result;
}

//line 의 첫 <cr=X> 의 X 값. ASS convert_ass_tags 결과의 inline tag 에서 추출 (SubtitleDlg list 의 색상 컬럼 표시용).
static CString extract_first_cr_color(const CString& s)
{
	CString lower = s;
	lower.MakeLower();
	int p = lower.Find(_T("<cr="));
	if (p < 0)
		return _T("");
	int gt = lower.Find(_T('>'), p);
	if (gt < 0)
		return _T("");
	CString val = s.Mid(p + 4, gt - p - 4);
	val.Trim();
	return val;
}

//<cr=...> / </cr> / </ct> 만 제거. <b>, <i>, <u>, <br> 등 다른 tag 는 보존.
//Contract: sentence.sentence 는 *색 외* 의 SCParagraph 호환 inline tag 만 보존 — 색은 sentence.color 로 분리.
//단일 패스 — case-insensitive 매칭으로 lower 사본 + 두 번 delete 회피.
static void strip_cr_tags(CString& s)
{
	for (int i = 0; i < s.GetLength(); )
	{
		if (s[i] != _T('<'))
		{
			i++;
			continue;
		}
		//<cr=...> : <c + r + = ... > (case-insensitive)
		if (i + 4 <= s.GetLength() && (s[i+1] == _T('c') || s[i+1] == _T('C')) &&
			(s[i+2] == _T('r') || s[i+2] == _T('R')) && s[i+3] == _T('='))
		{
			int gt = s.Find(_T('>'), i);
			if (gt < 0) break;
			s.Delete(i, gt - i + 1);
			continue;
		}
		//</cr> 또는 </ct>
		if (i + 5 <= s.GetLength() && s[i+1] == _T('/') &&
			(s[i+2] == _T('c') || s[i+2] == _T('C')) &&
			(s[i+3] == _T('r') || s[i+3] == _T('R') || s[i+3] == _T('t') || s[i+3] == _T('T')) &&
			s[i+4] == _T('>'))
		{
			s.Delete(i, 5);
			continue;
		}
		i++;
	}
}

CSubtitle::CSubtitle()
{
	reset();
}

bool CSubtitle::load_smi(CString sfile)
{
	DWORD __t0 = GetTickCount();
	//read() 가 BOM/byte 통계로 인코딩 정확 판별 → CString 으로 한 번에 변환. get_token_str 으로 라인 분리.
	std::deque<CString> lines;
	if (!read_lines(sfile, &lines))
		return false;

	CString sSync;			//sync 시작 단위로 누적 후 한 번에 parse_subtltle 호출.
	bool header_part = true;
	bool in_html_comment = false;	//<!-- ... --> body 내 주석 skip — 첫 cue 직전에 들어있으면 sSync 에 누적돼 가짜 caption 으로 표시됨 (Roman.Holiday.smi 등 흔함).

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

		//HTML 주석 skip — 단일 라인 / multi-line 모두.
		if (in_html_comment)
		{
			if (find(sLine, _T("-->")) >= 0)
				in_html_comment = false;
			continue;
		}
		if (find(sLine, _T("<!--")) >= 0)
		{
			if (find(sLine, _T("-->")) < 0)
				in_html_comment = true;
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
		//라인 경계는 공백으로 — SAMI/HTML 규칙상 raw 줄바꿈 = whitespace. 구분자 없이 붙이면
		//멀티라인 <P> 본문이 "tomorrow.Nothing" 처럼 단어가 들러붙는다 (각 segment 는 flush 시 Trim).
		sSync += _T(" ");
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
	DWORD __t1 = GetTickCount();
	rebuild_active_view();
	DWORD __t2 = GetTickCount();
	int total_track_cues = 0;
	for (const auto& kv : m_tracks) total_track_cues += (int)kv.second.size();
	logWrite(_T("[sub] load_smi total=%lums parse=%lums rebuild=%lums tracks=%d total_cues=%d active_cues=%d"),
		__t2 - __t0, __t1 - __t0, __t2 - __t1, (int)m_tracks.size(), total_track_cues, (int)m_subtitle.size());
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

	//각 sentence 의 source_class 를 *원래 track 의 Class 명* 으로 set — lists_to_subtitle 의 m_tracks update 위해.
	//copy 1번만 — push_back 의 copy 후 m_subtitle.back() 에서 in-place source_class set (이전 deep copy 2번 → 로딩 시간 2배).
	auto append_track = [this](const CString& cls, const std::deque<CCaption>& track)
	{
		for (const auto& cap : track)
		{
			m_subtitle.push_back(cap);
			for (auto& s : m_subtitle.back().sentences)
				s.source_class = cls;
		}
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
			append_track(cls, it->second);
	}

	//start 기준 정렬.
	std::sort(m_subtitle.begin(), m_subtitle.end(),
		[](const CCaption& a, const CCaption& b) { return a.start < b.start; });

	//같은 start 의 caption merge — 새 deque 생성 패턴 (O(n)). 이전 *erase from middle* 은
	//deque O(n) × n번 = O(n^2) — multi-class 통합 (KRCC 700 + JPCC 700) 시 큰 병목.
	std::deque<CCaption> merged;
	for (size_t i = 0; i < m_subtitle.size(); ++i)
	{
		if (!merged.empty() && m_subtitle[i].start == merged.back().start)
		{
			//다른 track (다국어) 의 같은 start caption 을 합칠 때 line_index 충돌 회피 —
			//이전 caption 의 max line_index + 1 부터 부여해야 각 언어가 별도 줄로 표시된다.
			//(안 하면 ENCC li=0 과 KRCC li=0 이 같은 줄로 들러붙음.)
			int base_line = 0;
			for (const auto& s : merged.back().sentences)
				if (s.line_index >= base_line)
					base_line = s.line_index + 1;
			for (const auto& s : m_subtitle[i].sentences)
			{
				CSentence shifted = s;
				shifted.line_index += base_line;
				merged.back().sentences.push_back(shifted);
			}
			if (m_subtitle[i].end > merged.back().end)
				merged.back().end = m_subtitle[i].end;
		}
		else
		{
			merged.push_back(std::move(m_subtitle[i]));
		}
	}
	m_subtitle = std::move(merged);
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
		{
			//SRT 는 각 caption 내 새 line 마다 sentence — line_index = 현재 sentence 카운트.
			//같은 line 안 multi-color 는 SRT 가 거의 표현 안 함 → 한 sentence 가 곧 한 line 으로 가정.
			caption.sentences.push_back(CSentence(sLine, color, (int)caption.sentences.size()));
		}
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
			//\n split — 각 line 별 CSentence. parse_subtltle 와 동일 contract:
			//sentence.color = line 의 첫 <cr=X>, sentence.sentence = 색 tag 제외 본문, line_index = \n 누적 카운트.
			std::deque<CString> sub_lines;
			get_token_str(text, sub_lines, _T("\n"));
			int sub_line_idx = 0;
			for (CString& ln : sub_lines)
			{
				ln.Trim();
				if (ln.IsEmpty())
				{
					sub_line_idx++;
					continue;
				}
				CString ln_color = extract_first_cr_color(ln);
				strip_cr_tags(ln);
				ln.Trim();
				if (ln.IsEmpty())
				{
					sub_line_idx++;
					continue;
				}
				caption.sentences.push_back(CSentence(ln, ln_color, sub_line_idx));
				sub_line_idx++;
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
	//SMI sync 블록 → CCaption.sentences (segment 단위).
	//Contract:
	//  sentence.sentence  = 그 segment 의 본문 (<b>/<i>/<u> 같은 비색 SCParagraph tag 는 inline 보존)
	//  sentence.color     = 그 segment 의 단일 색 (없으면 빈)
	//  sentence.line_index = 같은 cue 안 <br> 그룹 id (같은 line 의 색 segment 들끼리 같은 값)
	//
	//알고리즘 (글자 단위 state machine):
	//  - 텍스트 누적 중 <font color=X> 만나면 → 직전 text_buf flush (current_color, current_line_index),
	//    current_color = X.
	//  - </font> → flush, current_color = "".
	//  - <br> → flush, current_line_index++. color 는 carry-over 안 함 (</font> 만난 직후 빈 상태에서 br),
	//    단 <font> open 중에 <br> 만나면 그 색이 다음 line 까지 자동 유지 — current_color 가 그대로라.
	//    (Roman.Holiday.smi 의 `<font color=yellow>로마의 휴일<br>(Roman Holiday)` 케이스.)
	//  - <b>/<i>/<u>/<s> 등 비색 tag → text_buf 에 그대로 보존 → SCParagraph 가 OSD 렌더링 시 처리.
	src.Trim();
	src.Replace(_T("\r\n"), _T(""));
	src.Replace(_T("\n"), _T(""));
	if (src.IsEmpty())
		return;

	int sync_start = -1;
	CString cls;
	CString text_buf;
	CString current_color;
	int current_line_index = 0;

	CCaption caption;

	auto flush_segment = [&]()
	{
		if (text_buf.IsEmpty())
			return;
		text_buf.Replace(_T("&nbsp;"), _T(" "));
		text_buf.Trim();		//라인 경계 공백·들여쓰기 정규화 (SAMI/HTML whitespace 규칙). 공백만 남으면 skip.
		if (text_buf.IsEmpty())
			return;
		caption.sentences.push_back(CSentence(text_buf, current_color, current_line_index));
		text_buf.Empty();
	};

	//한 caption 을 해당 class 트랙으로 확정. 한 SYNC 안 다중 <P Class> 분리(class 전환 시) +
	//파싱 끝에서 공통 호출. caption 비우기는 호출자 책임.
	auto commit_caption = [&](CString commit_cls)
	{
		if (commit_cls.IsEmpty())
			commit_cls = default_class;

		auto& track = m_tracks[commit_cls];
		caption.start = sync_start;

		//자막끝 마커 (빈 본문) — 직전 caption end 갱신.
		if (caption.sentences.empty())
		{
			if (track.empty())
				return;
			if (sync_start <= track.back().start)
				return;
			track.back().end = sync_start;
			return;
		}

		//같은 start 면 이전 caption 에 sentence merge (multi-line sync).
		//merge 시 line_index 충돌 회피 — 이전 caption 의 max line_index + 1 부터 부여.
		if (!track.empty() && caption.start == track.back().start)
		{
			int base_line = 0;
			for (const auto& s : track.back().sentences)
				if (s.line_index >= base_line)
					base_line = s.line_index + 1;
			for (const auto& s : caption.sentences)
			{
				CSentence shifted = s;
				shifted.line_index += base_line;
				track.back().sentences.push_back(shifted);
			}
		}
		else
		{
			track.push_back(caption);
		}
	};

	auto handle_tag = [&](const CString& tag)
	{
		CString lower = tag;
		lower.MakeLower();

		if (lower.Find(_T("<sync start")) == 0)
		{
			int eq = tag.Find(_T('='));
			if (eq > 0)
				sync_start = _ttoi(tag.Mid(eq + 1));
		}
		else if (lower.Find(_T("<p class")) == 0)
		{
			//한 SYNC 안에 <P Class> 가 여럿일 수 있다 (표준 다국어 SAMI). class 가 바뀌면
			//직전 class 의 누적을 그 class 트랙으로 commit 하고 새 caption 으로 시작한다.
			//안 하면 ENCC 본문과 KRCC 본문이 한 segment 로 들러붙고, 전체가 마지막 class 트랙에 들어간다.
			flush_segment();
			if (!cls.IsEmpty() && !caption.sentences.empty())
			{
				commit_caption(cls);
				caption.sentences.clear();
				current_line_index = 0;
				current_color.Empty();
			}

			int eq = tag.Find(_T('='));
			if (eq > 0)
			{
				cls = tag.Mid(eq + 1);
				cls.TrimRight(_T(">"));
				cls.Trim();
				cls.MakeUpper();
			}
		}
		else if (lower.Find(_T("<p")) == 0)
		{
			//class 없는 <P> — 무시.
		}
		else if (lower.Find(_T("</p")) == 0)
		{
			//</P> — 무시.
		}
		else if (lower.Find(_T("<font")) == 0)
		{
			//<font ...> — color attr 있으면 추출 + segment 경계. 없으면 (face/size only) tag 무시.
			CString inner = tag.Mid(5, tag.GetLength() - 6);	//"<font " ~ ">"
			CString color = extract_font_color_attr(inner);
			if (!color.IsEmpty())
			{
				flush_segment();
				current_color = color;
			}
		}
		else if (lower.Find(_T("</font")) == 0)
		{
			flush_segment();
			current_color.Empty();
		}
		else if (lower.Find(_T("<br")) == 0)
		{
			flush_segment();
			current_line_index++;
		}
		else
		{
			//그 외 inline tag (<b>, </b>, <i>, </i>, <u>, </u>, <s>, </s> ...) — text_buf 에 그대로 보존.
			//SCParagraph 가 OSD 렌더링 시 인식. list 표시는 strip 헬퍼가 제거.
			text_buf += tag;
		}
	};

	CString sToken;
	bool in_tag = false;
	for (int i = 0; i < src.GetLength(); i++)
	{
		TCHAR c = src[i];
		if (c == _T('<'))
		{
			in_tag = true;
			sToken = _T("<");
		}
		else if (c == _T('>'))
		{
			if (!in_tag)
				continue;
			in_tag = false;
			sToken += _T(">");
			handle_tag(sToken);
			sToken.Empty();
		}
		else
		{
			if (in_tag)
				sToken += c;
			else
				text_buf += c;
		}
	}
	flush_segment();

	if (sync_start < 0)
		return;

	//마지막 (또는 유일한) class 의 누적을 commit.
	commit_caption(cls);
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
	else if (ext == _T("ass") || ext == _T("ssa"))
		return save_ass(m_sfile);
	else
		return false;
}

bool CSubtitle::save_srt(CString sfile)
{
	FILE* fp = NULL;
	_tfopen_s(&fp, sfile, _T("wt, ccs=UTF-8"));
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

		//line_index 변화 = \n (새 line), 같은 line_index = segment 직접 이어붙임.
		int prev_line = -1;
		for (j = 0; j < m_subtitle[i].sentences.size(); j++)
		{
			const CSentence& s = m_subtitle[i].sentences[j];
			if (prev_line >= 0 && s.line_index != prev_line)
				_ftprintf(fp, _T("\n"));

			if (s.color.IsEmpty() == false)
				_ftprintf(fp, _T("<font color=%s>%s</font>"), s.color, s.sentence);
			else
				_ftprintf(fp, _T("%s"), s.sentence);

			prev_line = s.line_index;
		}
		_ftprintf(fp, _T("\n\n"));	//마지막 line 종료 + cue 구분 blank line.
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

	//*** multi-class (다국어) 자막 보호 ***
	//m_tracks 에 2 개 이상 Class 가 있다면 다국어 통합 자막. 단순 m_subtitle (현재 active view) 만 저장하면
	//다른 언어 영구 손실. 자동 백업 (.bak) 후 m_tracks 의 모든 Class 통째로 출력.
	//단일 class (m_tracks.size() <= 1) 면 기존 동작 (m_subtitle + m_sLanguage).
	if (m_tracks.size() > 1)
	{
		//backup — 동일 폴더에 .bak (이미 있으면 덮어쓰지 않음, 첫 save 만 보존).
		CString bak = sfile + _T(".bak");
		if (GetFileAttributes(bak) == INVALID_FILE_ATTRIBUTES)
			CopyFile(sfile, bak, TRUE);

		FILE* fp_m = nullptr;
		_tfopen_s(&fp_m, sfile, _T("wt,ccs=UTF-8"));
		if (!fp_m) return false;

		_ftprintf(fp_m, _T("<SAMI>\n<HEAD>\n<TITLE>Multi-language</TITLE>\n<STYLE TYPE=\"text/css\"><!--\n"));
		_ftprintf(fp_m, _T("P { margin-left:8pt; margin-right:8pt; font-size:14pt; text-align:center; font-family:Arial; color:white; background-color:black; }\n"));
		for (const auto& kv : m_tracks)
		{
			LPCTSTR cls = kv.first.GetString();
			LPCTSTR lang =
				kv.first == _T("KRCC") ? _T("ko-KR") :
				kv.first == _T("ENCC") ? _T("en-US") :
				kv.first == _T("JPCC") ? _T("ja-JP") :
				kv.first == _T("CNCC") ? _T("zh-CN") : _T("und");
			_ftprintf(fp_m, _T(".%s { Name:%s; lang:%s; SAMIType:CC; }\n"), cls, cls, lang);
		}
		_ftprintf(fp_m, _T("--></STYLE>\n</HEAD>\n<BODY>\n"));

		//표준 SMI format — *같은 SYNC 별도 entry* per Class. 같은 (start, class) 그룹의 sentence 들은
		//한 SYNC entry 안에 line_index 변화 시 <br> join + color 있으면 <font color=X>seg</font> wrap
		//(single class save_smi 와 동일 로직).
		std::map<int, std::map<CString, std::vector<const CSentence*>>> by_start_class;
		for (const auto& kv : m_tracks)
		{
			const CString& cls = kv.first;
			for (const auto& cap : kv.second)
				for (const auto& sent : cap.sentences)
					by_start_class[cap.start][cls].push_back(&sent);
		}
		for (const auto& g : by_start_class)
		{
			for (const auto& cs : g.second)
			{
				const CString& cls = cs.first;
				const auto& sents = cs.second;
				_ftprintf(fp_m, _T("<SYNC Start=%d><P Class=%s>"), g.first, cls.GetString());
				int prev_line = -1;
				for (const CSentence* s : sents)
				{
					if (prev_line >= 0 && s->line_index != prev_line)
						_ftprintf(fp_m, _T("<br>"));
					if (!s->color.IsEmpty())
						_ftprintf(fp_m, _T("<font color=%s>%s</font>"), s->color.GetString(), s->sentence.GetString());
					else
						_ftprintf(fp_m, _T("%s"), s->sentence.GetString());
					prev_line = s->line_index;
				}
				_ftprintf(fp_m, _T("\n"));
			}
		}

		_ftprintf(fp_m, _T("\n</BODY>\n</SAMI>\n"));
		fclose(fp_m);
		return true;
	}

	FILE* fp = nullptr;
	_tfopen_s(&fp, sfile, _T("wt,ccs=UTF-8"));

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
		_ftprintf(fp, _T("<SYNC Start=%d><P Class=%s>"), m_subtitle[i].start, m_sLanguage);

		//line_index 변화 = <br> 출력, 같은 line_index 의 segment 들은 직접 이어붙임.
		//color 있으면 <font color=X>seg</font>, 없으면 plain.
		int prev_line = -1;
		for (j = 0; j < m_subtitle[i].sentences.size(); j++)
		{
			const CSentence& s = m_subtitle[i].sentences[j];
			if (prev_line >= 0 && s.line_index != prev_line)
				_ftprintf(fp, _T("<br>"));

			if (s.color.IsEmpty() == false)
				_ftprintf(fp, _T("<font color=%s>%s</font>"), s.color, s.sentence);
			else
				_ftprintf(fp, _T("%s"), s.sentence);

			prev_line = s.line_index;
		}
		_ftprintf(fp, _T("\n"));

		if (m_subtitle[i].end > 0)
			_ftprintf(fp, _T("<SYNC Start=%d><P Class=%s>&nbsp;\n"), m_subtitle[i].end, m_sLanguage);
	}

	_ftprintf(fp, _T("\n</BODY>\n</SAMI>\n"));

	fclose(fp);
	return true;
}

//ASS save — 단순 skeleton + Default style + Dialogue lines. embedded subtitle export 용도.
//원본 style 정보는 없음 (decoder 후 평문만 보유). 호환 viewer (PotPlayer, MPC, VLC) 에서 정상 표시.
bool CSubtitle::save_ass(CString sfile)
{
	DWORD dwAttrs = GetFileAttributes(sfile);
	if (dwAttrs != INVALID_FILE_ATTRIBUTES && (dwAttrs & FILE_ATTRIBUTE_READONLY))
		SetFileAttributes(sfile, FILE_ATTRIBUTE_NORMAL);

	FILE* fp = nullptr;
	_tfopen_s(&fp, sfile, _T("wt,ccs=UTF-8"));
	if (!fp) return false;

	_ftprintf(fp, _T("[Script Info]\n"));
	_ftprintf(fp, _T("Title: Exported\n"));
	_ftprintf(fp, _T("ScriptType: v4.00+\n"));
	_ftprintf(fp, _T("WrapStyle: 0\n"));
	_ftprintf(fp, _T("ScaledBorderAndShadow: yes\n"));
	_ftprintf(fp, _T("Collisions: Normal\n\n"));

	_ftprintf(fp, _T("[V4+ Styles]\n"));
	_ftprintf(fp, _T("Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding\n"));
	_ftprintf(fp, _T("Style: Default,Arial,20,&H00FFFFFF,&H000000FF,&H00000000,&H80000000,0,0,0,0,100,100,0,0,1,1,0,2,10,10,10,1\n\n"));

	_ftprintf(fp, _T("[Events]\n"));
	_ftprintf(fp, _T("Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text\n"));

	for (int i = 0; i < (int)m_subtitle.size(); i++)
	{
		const CCaption& cap = m_subtitle[i];
		int s_ms = cap.start;
		int e_ms = cap.end >= 0 ? cap.end : (cap.start + 3000);

		CString s_time, e_time;
		s_time.Format(_T("%d:%02d:%02d.%02d"), s_ms/3600000, (s_ms/60000)%60, (s_ms/1000)%60, (s_ms%1000)/10);
		e_time.Format(_T("%d:%02d:%02d.%02d"), e_ms/3600000, (e_ms/60000)%60, (e_ms/1000)%60, (e_ms%1000)/10);

		//multi-sentence 한 caption 의 모든 sentence 를 \N 으로 join.
		CString text;
		for (int j = 0; j < (int)cap.sentences.size(); j++)
		{
			if (!text.IsEmpty()) text += _T("\\N");
			CString s = cap.sentences[j].sentence;
			s.Replace(_T("\r\n"), _T("\\N"));
			s.Replace(_T("\n"), _T("\\N"));
			text += s;
		}

		_ftprintf(fp, _T("Dialogue: 0,%s,%s,Default,,0,0,0,,%s\n"),
			s_time.GetString(), e_time.GetString(), text.GetString());
	}

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

