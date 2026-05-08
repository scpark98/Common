#include "Subtitle.h"
#include <fstream>

#pragma warning(disable: 4018)	//'>=': signed/unsigned mismatch
#pragma warning(disable: 4477)	//'fwprintf' : format string '%s' requires an argument of type 'wchar_t *', but variadic argument 1 has type 'CString'

CSubtitle::CSubtitle()
{
	m_fp = NULL;
	reset();
}

bool CSubtitle::load_smi(CString sfile)
{
	//1024 → 4096: 긴 라인 (다국어 번역, 다중 font tag 등) 이 잘려 다음 read 가 같은 라인 후속 부분을 별개 라인으로 오인하던 문제 회피.
	char chLine[4096];
	TCHAR tcLine[4096];
	CString sLine;
	CString sSync;			//sync 시작 단위로 누적 후 한 번에 parse_subtltle 호출.
	bool header_part = true;

	while (true)
	{
		if (m_unicode)
		{
			if (_fgetts(tcLine, _countof(tcLine), m_fp) == 0)
				break;
			sLine = tcLine;
		}
		else
		{
			if (fgets(chLine, _countof(chLine), m_fp) == 0)
				break;
			sLine = chLine;
		}

		if (header_part)
		{
			//"<body" 부분 매칭 — "<BODY CLASS=...>" 같은 attribute 있는 형식도 처리.
			//기존 "<body>" literal 검사는 attribute 있는 흔한 형식을 놓쳐 모든 라인을 header 로 흡수했었음. find() 는 case-insensitive.
			if (find(sLine, _T("<body")) >= 0)
				header_part = false;

			if (m_unicode)
				m_header += tcLine;
			else
				m_header += chLine;

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

	fclose(m_fp);

	if (find(m_header, _T(".encc")) >= 0)
		m_sLanguage = _T("ENCC");
	else if (find(m_header, _T(".jpcc")) >= 0)
		m_sLanguage = _T("JPCC");
	else
		m_sLanguage = _T("KRCC");

	return true;
}

bool CSubtitle::load_srt(CString sfile)
{
	//1024 → 4096: 긴 자막 라인 (다국어 번역 등) 이 잘려 다음 read 가 같은 라인 후속 부분을 별개 라인으로 오인하던 문제 회피.
	char chLine[4096];
	TCHAR tcLine[4096];
	CString sLine;
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

	while (true)
	{
		if (m_unicode)
		{
			if (_fgetts(tcLine, _countof(tcLine), m_fp) == 0)
				break;
			sLine = tcLine;
		}
		else
		{
			if (fgets(chLine, _countof(chLine), m_fp) == 0)
				break;
			sLine = chLine;
		}

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

		//<font color="..."> ... </font>
		if (find(sLine, _T("<font color")) >= 0)
		{
			int pos1 = sLine.Find(_T("=")) + 1;
			int pos2 = sLine.Find(_T(">"));
			if (pos2 > pos1)
			{
				color = sLine.Mid(pos1, pos2 - pos1);
				color.Trim();
				remove_chars(color, _T("\"#"));
				sLine = sLine.Mid(pos2 + 1);
			}

			if (find(sLine, _T("</font>")) >= 0)
				sLine.Replace(_T("</font>"), _T(""));
		}
		//컬러끝 태그만 있으면 바로 전 캡션의 색상을 유지.
		else if (find(sLine, _T("</font>")) >= 0)
		{
			if (caption.sentences.size() > 0)
				color = caption.sentences[caption.sentences.size() - 1].color;
			sLine.Replace(_T("</font>"), _T(""));
		}
		else
		{
			color.Empty();
		}

		if (!sLine.IsEmpty())
			caption.sentences.push_back(CSentence(sLine, color));
	}

	//파일이 trailing blank line 없이 끝났을 때 마지막 caption 도 flush.
	if (caption.is_valid())
		m_subtitle.push_back(caption);

	fclose(m_fp);

	return true;
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

	if (PathFileExists(sfile) == false)
		return false;

	//encording 방식을 읽어온다.
	int	text_encoding = get_text_encoding(sfile);

	if (m_fp)
		fclose(m_fp);

	if (text_encoding <= text_encoding_ansi)
	{
		_tfopen_s(&m_fp, sfile, _T("rt"));
		m_unicode = false;
	}
	else
	{
		_tfopen_s(&m_fp, sfile, _T("rt, ccs=UNICODE"));
		m_unicode = true;
	}

	if (m_fp == NULL)
		return false;


	CString ext = get_part(sfile, fn_ext);
	ext.MakeLower();

	if (ext == _T("smi"))
		return load_smi(sfile);
	else if (ext == _T("srt"))
		return load_srt(sfile);

	return false;
}

void CSubtitle::parse_subtltle(CString src)
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

	//만약 여기까지 왔을 때 sToken에 값이 들어있다면?
	//caption에 sentences는 비어있고 자막끝 태그만 들어있다면
	//마지막으로 추가한 자막의 끝 태그인 것이므로 그 정보를 수정해준다.
	if (sToken.IsEmpty() == false)
	{
		sToken.Trim();
		if ((sToken == _T("&nbsp;")) && (caption.sentences.size() == 0))
		{
			//m_subtitle 이 비어있으면 직전 caption 이 없으므로 end 갱신 대상도 없음 — 그냥 skip.
			if (m_subtitle.empty())
				return;
			//간혹 end싱크가 start싱크보다 빠르게 작성된 smi도 있으므로 이는 스킵시킨다.
			if (caption.start <= m_subtitle.back().start)
				return;
			m_subtitle.back().end = caption.start;
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
		//이전 자막에 추가시켜준다.
		if (!m_subtitle.empty() && caption.start == m_subtitle.back().start)
		{
			for (i = 0; i < caption.sentences.size(); i++)
				m_subtitle.back().sentences.push_back(caption.sentences[i]);
		}
		else
		{
			m_subtitle.push_back(caption);
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
	while ((pos1 = lstr.Find(_T("<font face"))) >= 0)
	{
		pos2 = lstr.Find(BRACKET_CLOSE, pos1);
		if (pos2 > pos1)
		{
			lstr.Delete(pos1, pos2 - pos1 + 1);
			str.Delete(pos1, pos2 - pos1 + 1);
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
								m_subtitle[i].sentences[j]);
			}
			else
			{
				_ftprintf(fp, _T("%s\n"), m_subtitle[i].sentences[j]);
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

