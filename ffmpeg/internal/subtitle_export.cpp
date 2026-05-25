#include "subtitle_export.h"
#include "../../log/SCLog/SCLog.h"
#include "../../subtitle/Subtitle.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/dict.h>
}

#include <string>

//codec_descriptor 의 AV_CODEC_PROP_TEXT_SUB property 로 text/bitmap 판별.
static bool codec_is_text_subtitle(AVCodecID codec_id)
{
	const AVCodecDescriptor* desc = avcodec_descriptor_get(codec_id);
	if (!desc)
		return false;
	return (desc->props & AV_CODEC_PROP_TEXT_SUB) != 0;
}

//ffmpeg codec long name → CString. UTF-8 → wide.
static CString codec_long_name(AVCodecID codec_id)
{
	const AVCodecDescriptor* desc = avcodec_descriptor_get(codec_id);
	if (!desc || !desc->long_name)
		return _T("");
	int wlen = MultiByteToWideChar(CP_UTF8, 0, desc->long_name, -1, NULL, 0);
	if (wlen <= 0) return _T("");
	CString s;
	MultiByteToWideChar(CP_UTF8, 0, desc->long_name, -1, s.GetBuffer(wlen), wlen);
	s.ReleaseBuffer();
	return s;
}

//AVDictionary 에서 key 값 추출 → CString (UTF-8 → wide).
static CString dict_get_string(AVDictionary* dict, const char* key)
{
	if (!dict) return _T("");
	AVDictionaryEntry* e = av_dict_get(dict, key, NULL, 0);
	if (!e || !e->value) return _T("");
	int wlen = MultiByteToWideChar(CP_UTF8, 0, e->value, -1, NULL, 0);
	if (wlen <= 0) return _T("");
	CString s;
	MultiByteToWideChar(CP_UTF8, 0, e->value, -1, s.GetBuffer(wlen), wlen);
	s.ReleaseBuffer();
	return s;
}

//ffmpeg stream language 메타데이터 (ISO 639) → SAMI CC class. 미지원/없음이면 빈 문자열 (호출자가 default).
static CString lang_to_cc_class(const CString& lang)
{
	CString l = lang;
	l.MakeLower();
	if (l == _T("eng") || l == _T("en"))										return _T("ENCC");
	if (l == _T("kor") || l == _T("ko"))										return _T("KRCC");
	if (l == _T("jpn") || l == _T("ja") || l == _T("jp"))						return _T("JPCC");
	if (l == _T("chi") || l == _T("zho") || l == _T("zh") || l == _T("cn"))		return _T("CNCC");
	return _T("");
}

bool list_embedded_subtitle_streams(LPCTSTR media_path, std::vector<EmbeddedSubtitleStream>& out)
{
	out.clear();
	if (!media_path || !*media_path)
		return false;

	//wide path → utf-8.
	int u8len = WideCharToMultiByte(CP_UTF8, 0, media_path, -1, NULL, 0, NULL, NULL);
	if (u8len <= 0) return false;
	std::string u8path(u8len, 0);
	WideCharToMultiByte(CP_UTF8, 0, media_path, -1, &u8path[0], u8len, NULL, NULL);

	AVFormatContext* fmt = NULL;
	int hr = avformat_open_input(&fmt, u8path.c_str(), NULL, NULL);
	if (hr < 0 || !fmt)
	{
		logWrite(_T("[sub_export/list] open fail hr=%d"), hr);
		return false;
	}

	if (avformat_find_stream_info(fmt, NULL) < 0)
	{
		avformat_close_input(&fmt);
		return false;
	}

	for (unsigned int i = 0; i < fmt->nb_streams; i++)
	{
		AVStream* st = fmt->streams[i];
		if (!st || !st->codecpar) continue;
		if (st->codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) continue;

		EmbeddedSubtitleStream es;
		es.index   = (int)i;
		es.is_text = codec_is_text_subtitle(st->codecpar->codec_id);
		es.lang    = dict_get_string(st->metadata, "language");
		es.title   = dict_get_string(st->metadata, "title");
		es.codec_name = codec_long_name(st->codecpar->codec_id);
		out.push_back(es);
	}

	logWrite(_T("[sub_export/list] %d subtitle streams"), (int)out.size());
	avformat_close_input(&fmt);
	return true;
}


//ASS event 의 Text 필드 추출 — "0,0,Default,,0,0,0,,Hello\Nworld" 의 마지막 콤마 뒤 부분.
//ASS event 는 10 fields: Layer,Start,End,Style,Name,MarginL,MarginR,MarginV,Effect,Text.
//우리 input 은 ffmpeg 의 decoded AVSubtitle.rects[].ass — start/end 가 빠진 8 fields (Layer,ReadingOrder,Style,Name,MarginL,MarginR,MarginV,Effect,Text) 또는 다른 변형.
//최후 콤마 *9번째* 이후 텍스트.
static CString extract_ass_text(const char* ass_line)
{
	if (!ass_line) return _T("");

	//field 9번째 (0-based 8) 콤마 위치 찾기. ffmpeg ASS event 의 Text 는 항상 마지막 field.
	const char* p = ass_line;
	int commas = 0;
	while (*p)
	{
		if (*p == ',')
		{
			commas++;
			if (commas == 8 || commas == 9)
			{
				//일부 codec 은 9 fields (ReadingOrder 포함), 일부는 8 — 둘 다 시도.
				const char* text_start = p + 1;
				//ASS line break: \N → 실제 newline.
				std::string s(text_start);
				size_t pos = 0;
				while ((pos = s.find("\\N", pos)) != std::string::npos)
				{
					s.replace(pos, 2, "\n");
					pos += 1;
				}
				while ((pos = s.find("\\n", pos)) != std::string::npos)
				{
					s.replace(pos, 2, "\n");
					pos += 1;
				}
				//ASS override tag 제거 — {\i1}, {\b1}, {\an8}, {\fnArial}, {\c&Hffffff&} 등 inline format 명령.
				//SMI/SRT 등 다른 format 으로 export 시 의미 없는 ASS 전용 문법이라 strip.
				while (true)
				{
					size_t a = s.find('{');
					if (a == std::string::npos) break;
					size_t b = s.find('}', a);
					if (b == std::string::npos) break;
					s.erase(a, b - a + 1);
				}
				//trailing whitespace / newline 정리.
				while (!s.empty() && (s.back() == '\r' || s.back() == '\n' || s.back() == ' '))
					s.pop_back();

				int wlen = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, NULL, 0);
				if (wlen <= 0) return _T("");
				CString w;
				MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, w.GetBuffer(wlen), wlen);
				w.ReleaseBuffer();
				return w;
			}
		}
		p++;
	}
	return _T("");
}


bool export_subtitle_stream(LPCTSTR media_path, int stream_index, LPCTSTR out_path, SubtitleExportFormat fmt)
{
	if (!media_path || !*media_path || !out_path || !*out_path || stream_index < 0)
		return false;

	int u8len = WideCharToMultiByte(CP_UTF8, 0, media_path, -1, NULL, 0, NULL, NULL);
	if (u8len <= 0) return false;
	std::string u8path(u8len, 0);
	WideCharToMultiByte(CP_UTF8, 0, media_path, -1, &u8path[0], u8len, NULL, NULL);

	AVFormatContext* in_fmt = NULL;
	if (avformat_open_input(&in_fmt, u8path.c_str(), NULL, NULL) < 0 || !in_fmt)
	{
		logWrite(_T("[sub_export/export] open fail"));
		return false;
	}
	if (avformat_find_stream_info(in_fmt, NULL) < 0)
	{
		avformat_close_input(&in_fmt);
		return false;
	}
	if (stream_index < 0 || stream_index >= (int)in_fmt->nb_streams)
	{
		avformat_close_input(&in_fmt);
		return false;
	}

	AVStream* st = in_fmt->streams[stream_index];
	if (!st || !st->codecpar || st->codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE)
	{
		avformat_close_input(&in_fmt);
		return false;
	}
	if (!codec_is_text_subtitle(st->codecpar->codec_id))
	{
		logWrite(_T("[sub_export/export] non-text codec — skip"));
		avformat_close_input(&in_fmt);
		return false;
	}

	const AVCodec* codec = avcodec_find_decoder(st->codecpar->codec_id);
	if (!codec)
	{
		avformat_close_input(&in_fmt);
		return false;
	}
	AVCodecContext* dec = avcodec_alloc_context3(codec);
	if (!dec)
	{
		avformat_close_input(&in_fmt);
		return false;
	}
	if (avcodec_parameters_to_context(dec, st->codecpar) < 0 ||
	    avcodec_open2(dec, codec, NULL) < 0)
	{
		avcodec_free_context(&dec);
		avformat_close_input(&in_fmt);
		return false;
	}

	//스트림 언어 메타데이터 → SAMI CC class. 미지원/없음이면 ENCC (내부 자막 대부분 영어).
	//set 안 하면 save_smi 가 빈 m_sLanguage 를 JPCC 로 폴백해 영어 자막이 JPCC 로 잘못 태깅됨.
	CString lang = dict_get_string(st->metadata, "language");
	CString cc_class = lang_to_cc_class(lang);
	if (cc_class.IsEmpty())
		cc_class = _T("ENCC");
	logWrite(_T("[sub_export/export] stream lang='%s' -> class=%s"), lang.GetString(), cc_class.GetString());

	CSubtitle sub;
	sub.set_subtitle_file(out_path);
	sub.set_language(cc_class);

	//packet read loop — subtitle packet 만 decode.
	AVPacket* pkt = av_packet_alloc();
	if (!pkt)
	{
		avcodec_free_context(&dec);
		avformat_close_input(&in_fmt);
		return false;
	}

	const AVRational tb = st->time_base;
	int caption_count = 0;

	while (av_read_frame(in_fmt, pkt) >= 0)
	{
		if (pkt->stream_index != stream_index)
		{
			av_packet_unref(pkt);
			continue;
		}

		AVSubtitle av_sub;
		memset(&av_sub, 0, sizeof(av_sub));
		int got_sub = 0;
		int decoded = avcodec_decode_subtitle2(dec, &av_sub, &got_sub, pkt);
		if (decoded >= 0 && got_sub && av_sub.num_rects > 0)
		{
			//start_ms / end_ms 계산.
			int64_t pts_ms = (pkt->pts != AV_NOPTS_VALUE) ?
				(int64_t)(pkt->pts * av_q2d(tb) * 1000.0) : 0;
			//AVSubtitle.start_display_time / end_display_time 는 ms 단위 offset.
			int64_t start_ms = pts_ms + (int64_t)av_sub.start_display_time;
			int64_t end_ms   = pts_ms + (int64_t)av_sub.end_display_time;
			if (av_sub.end_display_time == 0 && pkt->duration > 0)
				end_ms = pts_ms + (int64_t)(pkt->duration * av_q2d(tb) * 1000.0);
			if (end_ms <= start_ms)
				end_ms = start_ms + 3000;	//fallback — 3초

			//각 rect 의 ass / text 합쳐서 한 caption.
			CString combined;
			for (unsigned int r = 0; r < av_sub.num_rects; r++)
			{
				AVSubtitleRect* rect = av_sub.rects[r];
				if (!rect) continue;
				CString line;
				if (rect->ass)
					line = extract_ass_text(rect->ass);
				else if (rect->text)
				{
					int wlen = MultiByteToWideChar(CP_UTF8, 0, rect->text, -1, NULL, 0);
					if (wlen > 0)
					{
						MultiByteToWideChar(CP_UTF8, 0, rect->text, -1, line.GetBuffer(wlen), wlen);
						line.ReleaseBuffer();
					}
				}
				if (!line.IsEmpty())
				{
					if (!combined.IsEmpty()) combined += _T("\n");
					combined += line;
				}
			}

			if (!combined.IsEmpty())
			{
				CCaption cap;
				cap.start = (int)start_ms;
				cap.end   = (int)end_ms;

				//멀티라인(\n) → line_index 별 CSentence 분리. save_smi 가 line_index 변화 시 <br> 출력 →
				//"- A.<br>- B." 처럼 화자 구분 두 줄이 보존됨.
				//(한 sentence 에 \n 박으면 raw 줄바꿈으로 저장돼 파서가 한 줄로 붙임.)
				int line = 0;
				int pos = 0;
				while (pos >= 0)
				{
					int nl = combined.Find(_T('\n'), pos);
					CString one = (nl < 0) ? combined.Mid(pos) : combined.Mid(pos, nl - pos);
					one.Trim();
					if (!one.IsEmpty())
					{
						CSentence sent;
						sent.sentence = one;
						sent.line_index = line++;
						cap.sentences.push_back(sent);
					}
					pos = (nl < 0) ? -1 : nl + 1;
				}

				if (!cap.sentences.empty())
				{
					sub.m_subtitle.push_back(cap);
					caption_count++;
				}
			}

			avsubtitle_free(&av_sub);
		}

		av_packet_unref(pkt);
	}

	av_packet_free(&pkt);
	avcodec_free_context(&dec);
	avformat_close_input(&in_fmt);

	logWrite(_T("[sub_export/export] decoded %d captions"), caption_count);

	if (caption_count == 0)
		return false;

	//save_subtitle_file 가 out_path 의 확장자 자동 dispatch (smi/srt/ass).
	bool ok = sub.save_subtitle_file(out_path);
	logWrite(_T("[sub_export/export] save fmt=%d ok=%d path='%s'"),
		(int)fmt, (int)ok, out_path);
	return ok;
}
