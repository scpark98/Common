#pragma once

//Embedded subtitle stream — 미디어 파일의 내장 자막 트랙을 ffmpeg 으로 list / export.
//
//지원 codec — text-based 만 (export 가능):
//  - SUBRIP (.srt)
//  - ASS / SSA
//  - WEBVTT
//  - MOV_TEXT (mp4)
//
//미지원 — bitmap (export 시 false 반환):
//  - HDMV_PGS_SUBTITLE (Blu-ray PGS)
//  - DVD_SUBTITLE (VobSub)
//  - DVB_SUBTITLE
//
//is_text 판정은 list_embedded_subtitle_streams 가 호출 시점에 codec descriptor 의
//AV_CODEC_PROP_TEXT_SUB property 로 결정. 호출자는 결과의 is_text 만 확인하면 됨.

#include <Afxwin.h>
#include <vector>

struct EmbeddedSubtitleStream
{
	int     index = -1;         //미디어 파일 안의 stream index (avformat 의 streams[index])
	bool    is_text = false;    //true = export 가능 (SRT/ASS/SSA/WEBVTT/MOV_TEXT), false = bitmap (PGS/VobSub 등)
	CString lang;               //ISO 639 language code (예: "kor", "eng"). 미디어 metadata 없으면 빈 문자열.
	CString title;              //stream metadata 의 title — 사용자 친화 이름. 없으면 빈 문자열.
	CString codec_name;         //ffmpeg codec long name (예: "SubRip subtitle", "ASS (Advanced SSA) subtitle")
};

enum SubtitleExportFormat
{
	subtitle_export_fmt_smi = 0,
	subtitle_export_fmt_srt,
	subtitle_export_fmt_ass,
};

//미디어 파일의 모든 내장 subtitle stream 을 list. out 에 각 stream 의 index + is_text + metadata 채움.
//성공 시 true. 미디어 open 실패 시 false.
bool list_embedded_subtitle_streams(LPCTSTR media_path, std::vector<EmbeddedSubtitleStream>& out);

//지정 stream 을 out_path 로 export. text codec 만 동작. fmt 따라 SMI/SRT/ASS 로 변환 (Common 의 Subtitle save 재사용).
//성공 시 true. bitmap stream 또는 packet read fail 시 false.
bool export_subtitle_stream(LPCTSTR media_path, int stream_index, LPCTSTR out_path, SubtitleExportFormat fmt);
