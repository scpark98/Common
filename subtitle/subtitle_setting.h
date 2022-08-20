#ifndef _SUBTITLE_SETTING_H
#define _SUBTITLE_SETTING_H

#include <Afxwin.h>
#include <Afxdisp.h>

class CSubtitleSetting
{
public:
	CSubtitleSetting();
	~CSubtitleSetting();

	LOGFONT		*lf;

	int			display_method;					//on video or on overlay surface

	CRect		marginRect;						// measured from the sides
	int			border_style;					// 0: outline, 1: opaque box
	double		outline_widthX, outline_widthY;
	double		shadow_depthX, shadow_depthY;
	COLORREF	colors[4];						// usually: {primary, secondary, outline/background, shadow}
	BYTE		alpha[4];
	bool		alpha_link;

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
	friend CString& operator <<= (CString& style, CSubtitleSetting& s);
	friend CSubtitleSetting& operator <<= (CSubtitleSetting& s, CString& style);
};

#endif
