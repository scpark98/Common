#include "Subtitle.h"
#include <fstream>

#pragma warning(disable: 4018)	//'>=': signed/unsigned mismatch
#pragma warning(disable: 4477)	//'fwprintf' : format string '%s' requires an argument of type 'wchar_t *', but variadic argument 1 has type 'CString'

CSubtitle::CSubtitle()
{
	m_fp = NULL;
	reset();
}

CStringW UTF8toUTF16(const CStringA& utf8)
{
	CStringW utf16;
	int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
	if (len>1)
	{ 
		wchar_t *ptr = utf16.GetBuffer(len-1);
		if (ptr) MultiByteToWideChar(CP_UTF8, 0, utf8, -1, ptr, len);
		utf16.ReleaseBuffer();
	}
	return utf16;
}

bool CSubtitle::load_smi(CString sfile)
{
	char chLine[1024];
	TCHAR tcLine[1024];
	CString sLine = _T("");		//char문자열을 CString으로 저장하고
	CString sSync = _T("");		//sync start단위씩 읽어온 후 처리하자.
	CString str;
	bool header_part = true;	//first sync를 만나기 전까지는 모두 헤더에 넣는다.
	int pos1, pos2;
	CCaption caption;
	CString color = _T("");

	caption.start = -1;

	while (true)
	{
		if (m_unicode)
		{
			if (_fgetts(tcLine, 1024, m_fp) == 0)
				break;
			sLine = tcLine;
		}
		else
		{
			if (fgets(chLine, 1024, m_fp) == 0)
				break;
			sLine = chLine;
		}

		if (header_part)
		{
			//<BODY>를 만나기 전까지는 모두 header로 저장한다.
			if (find_string(sLine, _T("<body>")) >= 0)
				header_part = false;

			if (m_unicode)
				m_header += tcLine;
			else
				m_header += chLine;

			continue;
		}

		//sync를 만나면 그 전까지 저장해 둔 문자열이 있다면 파싱을 시작하고
		if (find_string(sLine, _T("<sync start")) >= 0)
		{
			//저장해 둔 문자열이 없으면 일단 저장하고
			if (sSync.IsEmpty())
			{
				sSync += sLine;
			}
			//자막끝 표시 문자열을 만나면 현재까지 저장해두었던 sSync를 파싱하고
			//그 자막의 end값으로 넣어준다.
			else if (find_string(sLine, _T("&nbsp;")) > 0)
			{
				parse_subtltle(sSync);
				sSync.Empty();
				pos1 = sLine.Find(_T("=")) + 1;
				pos2 = sLine.Find(BRACKET_CLOSE, pos1 + 1);
				str = sLine.Mid(pos1, pos2 - pos1);
				m_subtitle[m_subtitle.size()-1].end = _ttoi(str);
			}
			//그것도 아니면 자막 블럭이므로 분석해서 넣어준다.
			else
			{
				parse_subtltle(sSync);
				sSync = sLine;
			}
		}
		//바디의 끝 테그를 만나면 더 이상 파싱할 게 없을것이다.
		else if (find_string(sLine, _T("</body>")) >= 0)
		{
			parse_subtltle(sSync);
			break;
		}
		//sync를 만나기 전까지는 일단 저장해둔다.
		else
		{
			sSync += sLine;
		}



		//한 라인씩 처리하는 방식으로 문제가 될 소지가 있다.
		//sync start단위로 처리하는 것이 좋으므로
		//다음 sync start를 만나기 전까지 우선 읽어들이고
		//그 읽어들인 문장을 파싱하자.
		/*
		if (sline.Find(_T("<SYNC Start")) >= 0)
		{
		first_sync = true;
		pos1 = sline.Find(_T("=")) + 1;
		pos2 = sline.Find(BRACKET_CLOSE, pos1 + 1);

		str = sline.Mid(pos1, pos2 - pos1);

		//자막 끝문자를 만나면 현재까지 추출한 자막을 저장한다.
		if (sline.Find(_T("&nbsp;")) > 0)
		{
		if (caption.sentences.size())
		{
		caption.end = _ttoi(str);
		m_subtitle.push_back(caption);
		caption.sentences.clear();
		caption.start = -1;
		color.Empty();
		}
		}
		//추출한 자막이 있는데 &nbsp;와 같은 끝문자를 안만나더라도
		//다음 싱크 시작 테그를 만났다면 저장해주고 넘어가야 한다.
		else if (caption.start >= 0 && caption.sentences.size())
		{
		caption.end = -1;
		m_subtitle.push_back(caption);
		caption.sentences.clear();
		caption.start = _ttoi(str);
		color.Empty();
		}
		else
		{
		caption.start = _ttoi(str);
		}
		}
		else if (caption.start < 0)
		{
		if (!first_sync)
		{
		if (isUnicode)
		m_header += tLine;
		else
		m_header += sLine;
		}

		continue;
		}
		//바디의 끝을 만났다면 마지막 저장해두었던 자막 데이터를 추가한다.
		else if (sline.Find(_T("</BODY>")) >= 0)
		{
		m_subtitle.push_back(caption);
		}
		else
		{
		//<font color="#FF0000">카밀로 2번째 자막<br>
		//<font color="#ffb400">新あたしんち <font color="#dcdcdc">신 우리집<br>
		while (true)
		{
		int font_tag = sline.Find(_T("<font color"));
		if (font_tag < 0)
		{
		if (caption.start >= 0)
		{
		sline.Replace(_T("\n"), _T(""));
		if (sline.GetLength())
		caption.sentences.push_back(CSentence(sline, color));
		}
		break;
		}
		else
		{
		if (font_tag > 0)
		{
		str = sline.Left(font_tag);
		caption.sentences.push_back(CSentence(str, color));
		sline = sline.Mid(str.GetLength());
		}

		//컬러 태그는 조금씩 다르다. '='과 '>'는 반드시 존재한다.
		//<font color=FF6600>제 1 화<br>
		//<font color="#FF0000">카밀로 2번째 자막<br>
		sline = sline.Mid(sline.Find(_T("=")) + 1);
		pos1 = sline.Find(_T(">"));
		color = sline.Left(pos1);
		remove_chars(color, _T("\"#"));
		//color.Replace(_T("\""), _T(""));
		//color.Replace(_T("#"), _T(""));
		sline = sline.Mid(pos1 + 1);
		}
		}
		}
		*/
	}

	fclose(m_fp);

	if (find_string(m_header, _T(".encc")) >= 0)
		m_sLanguage = _T("ENCC");
	else if (find_string(m_header, _T(".jpcc")) >= 0)
		m_sLanguage = _T("JPCC");
	else
		m_sLanguage = _T("KRCC");

	return true;
}

bool CSubtitle::load_srt(CString sfile)
{
	char chLine[1024];
	TCHAR tcLine[1024];
	CString sLine = _T("");		//char문자열을 CString으로 저장하고
	CString str;
	int pos1, pos2;
	CCaption caption;
	CString color;

	while (true)
	{
		if (m_unicode)
		{
			if (_fgetts(tcLine, 1024, m_fp) == 0)
				break;
			sLine = tcLine;
		}
		else
		{
			if (fgets(chLine, 1024, m_fp) == 0)
				break;
			sLine = chLine;
		}

		//시간 태그인 경우
		//00:00:29,468 --> 00:00:32,988
		if ((get_char_count(sLine, ':') == 4) && (get_char_count(sLine, ',') == 2) &&
			(get_char_count(sLine, '-') >= 1) && (get_char_count(sLine, '>') == 1))
		{
			sLine.Trim();

			pos1 = sLine.Find('-');
			str = sLine.Left(pos1);
			caption.start = GetMilliSecondsFromTimeString(str);

			sLine = sLine.Mid(sLine.ReverseFind('>') + 1);
			caption.end = GetMilliSecondsFromTimeString(sLine);
		}
		//아직 시간 태그를 만나기 전이라면 순번이므로 패스
		else if (caption.start == -1)
		{
			continue;
		}
		//자막 내용인 경우
		else
		{
			sLine.Trim();

			//자막이 비어있으면 해당 자막의 끝이고
			if (sLine.IsEmpty())
			{
				m_subtitle.push_back(caption);
				caption.reset();
			}
			//자막의 내용이 있으면 계속 추가한다.
			else
			{
				//색생값이 있으면 추출해준다.
				if (find_string(sLine, _T("<font color")) >= 0)
				{
					pos1 = sLine.Find(_T("=")) + 1;
					pos2 = sLine.Find(_T(">"));
					color = sLine.Mid(pos1, pos2 - pos1);
					color.Trim();
					remove_chars(color, _T("\"#"));
					sLine = sLine.Mid(pos2 + 1);

					//현재 라인에 컬러끝 태그가 있다면 그냥 삭제시켜주면 된다.
					if (find_string(sLine, _T("</font>")) >= 0)
						sLine.Replace(_T("</font>"), _T(""));
				}
				//컬러끝 태그가 있다면 바로 전 캡션의 색상을 계속 유지하는 경우다.
				else if (find_string(sLine, _T("</font>")) >= 0)
				{
					if (caption.sentences.size() > 0)
						color = caption.sentences[caption.sentences.size() - 1].color;
					sLine.Replace(_T("</font>"), _T(""));
				}
				else
				{
					color.Empty();
				}

				//caption.sentences.push_back(sLine);
				caption.sentences.push_back(CSentence(sLine, color));
			}
		}
	}

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

	if (text_encoding <= text_ansi)
	{
		m_fp = _tfopen(sfile, _T("rt"));
		m_unicode = false;
	}
	else
	{
		m_fp = _tfopen(sfile, _T("rt, ccs=UNICODE"));
		m_unicode = true;
	}

	if (m_fp == NULL)
		return false;


	CString ext = GetFileExtensionFromFilename(sfile);
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
			if (find_string(sToken, _T("<sync start")) >= 0)
			{
				pos1 = sToken.Find(_T("=")) + 1;
				caption.start = _ttoi(sToken.Mid(pos1));
				sToken.Empty();
			}
			else if (find_string(sToken, _T("<p class")) >= 0)
			{
				sToken.Empty();
			}
			else if (find_string(sToken, _T("<font color")) >= 0)
			{
				pos1 = sToken.Find(_T("=")) + 1;
				color = sToken.Mid(pos1);
				remove_chars(color, _T("\"#"));
				sToken.Empty();
			}
			//한 문장의 끝에 <br>이 붙어있다면 제거하고 넣어준다.
			else if (find_string(sToken, _T("<br")) >= 0)
			{
				//sToken += src[i];
				sToken = sToken.Left(sToken.GetLength() - 3);
				caption.sentences.push_back(CSentence(sToken, color));
				sToken.Empty();
				color.Empty();
			}
			else if (find_string(sToken, _T("</font")) >= 0)
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
			//간혹 end싱크가 start싱크보다 빠르게 작성된 smi도 있으므로 이는 스킵시킨다.
			if (caption.start <= m_subtitle[m_subtitle.size()-1].start)
				return;
			m_subtitle[m_subtitle.size()-1].end = caption.start;
			return;
		}
	}

	//하나의 sync절의 분석이 끝났는데 sToken이 존재하면 이는 자막이니 넣어주자.
	if (sToken.IsEmpty() == false)
	{
		caption.sentences.push_back(CSentence(sToken, color));
	}

	if (caption.sentences.size())
	{
		//만약 현재 자막이 마지막 자막의 시작 시간과 같다면
		//이전 자막에 추가시켜준다.
		if ((m_subtitle.size() > 0) && (caption.start == m_subtitle[m_subtitle.size()-1].start))
		{
			for (i = 0; i < caption.sentences.size(); i++)
				m_subtitle[m_subtitle.size()-1].sentences.push_back(caption.sentences[i]);
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

	CString ext = GetFileExtensionFromFilename(m_sfile);
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
	FILE*	fp = _tfopen(sfile, _T("wt, ccs=UNICODE"));
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

	FILE* fp = _tfopen(sfile, _T("wt,ccs=UNICODE"));
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

