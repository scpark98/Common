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
	Gdiplus::Color	cr[4];						// ARGB. usually: {primary, secondary, outline/background, shadow}

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

	void		set_default();
	//deserialize 후 또는 외부에서 값 정합성 검증. 한 필드라도 임계 범위 밖이면 false.
	//부적합한 값이 GdiPlus path / VMR9 SetAlphaBitmap 호출에 garbage 로 흘러가 hang 유발하는 것을 방지.
	bool		is_sane() const;
	friend CString& operator <<= (CString& style, CSubtitleSetting& s);
	friend CSubtitleSetting& operator <<= (CSubtitleSetting& s, CString& style);
};

#endif
