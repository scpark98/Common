#ifndef _SUBTITLE_SETTING_H
#define _SUBTITLE_SETTING_H

#include <Afxwin.h>
#include <Afxdisp.h>
#include <gdiplus.h>

class CSubtitleSetting
{
public:
	CSubtitleSetting();
	~CSubtitleSetting();

	LOGFONT		*lf;

	int			display_method;					//on video or on overlay surface

	CRect		marginRect;						// parcentage of app width/height. left/right, top/bottom 각각 따로 설정 가능. 1080p 기준 픽셀 단위로 입력받아 display_subtitle 의 wrap_w 계산이 video_height/1080 scale 로 변환해 사용.
	int			border_style;					// 0: outline, 1: opaque box
	double		outline_widthX, outline_widthY;
	double		shadow_depthX, shadow_depthY;
	//20260717 by claude. 자막 4색 인덱스. ASS/SSA V4+ 표준 순서(Primary/Secondary/Outline/Back)를 그대로 미러링 —
	//이 순서가 직렬화(operator<<=)·ASS export(Subtitle.cpp) 와 묶여 있으므로 재배열 금지(저장값·ASS 호환 깨짐).
	//cr=색상, sub=자막, 마지막=용도 의 계층 네이밍.
	enum cr_sub_index
	{
		cr_sub_primary   = 0,	//자막 본색 (렌더 fill)
		cr_sub_secondary = 1,	//ASS SecondaryColour(가라오케 전색) — 이 플레이어는 렌더에 미사용, ASS 왕복 보존용 예약 슬롯
		cr_sub_outline   = 2,	//외곽선색
		cr_sub_shadow    = 3,	//그림자색 (= ASS BackColour; opaque-box 미구현이라 배경 아님)
		cr_sub_count     = 4,
	};
	Gdiplus::Color	cr[cr_sub_count];			// ARGB. 인덱스는 위 cr_sub_index 사용. (OSD 설정도 이 구조체를 재사용 — 그 경우 sub=OSD 로 읽음)

	int			font_size;						// height
	bool		font_auto_size;
	int			char_per_line;
	double		font_scaleX, font_scaleY;			// percent

	//percentage value of video resolution. 0% ~ 100%
	int			pos_x;
	int			pos_y;	

	int			char_spacing;					// space between charactors (+/- pixels)
												
	//폰트의 실제 픽셀 높이 * %값. 기본 120%에 -50% ~ +50% 범위로 변경 가능.
	//0%이면 가장 보기 좋은 120% 줄간격으로 출력된다.
	int			line_spacing;

	//0:center, 1:left of center
	int			text_align;

	//20260719 by claude. 자막 뒤 배경 라운드 박스 (사용자 요청). background_on 이 true 일 때만 cr_background 로 칠한다.
	//배경 테두리색은 별도 저장하지 않고 렌더 시 cr_background 에서 유도(조금 더 진한 색). 여백·반경·테두리색 등 세부 옵션은 추후 필요 시 추가.
	//직렬화는 append-only (operator<<= 끝에 추가) — 구 저장 문자열엔 이 토큰이 없어 deserialize 가 default 값으로 채운다(구 설정 유실 없음).
	bool			background_on;
	Gdiplus::Color	cr_background;					// ARGB. background_on 일 때 자막 뒤 라운드 박스 색.

	void		set_default();
	//deserialize 후 또는 외부에서 값 정합성 검증. 한 필드라도 임계 범위 밖이면 false.
	//부적합한 값이 GdiPlus path / VMR9 SetAlphaBitmap 호출에 garbage 로 흘러가 hang 유발하는 것을 방지.
	bool		is_sane() const;
	friend CString& operator <<= (CString& style, CSubtitleSetting& s);
	friend CSubtitleSetting& operator <<= (CSubtitleSetting& s, CString& style);
};

#endif
