
#include "subtitle_setting.h"
#include "../Functions.h"


CSubtitleSetting::CSubtitleSetting()
{
	lf = (LOGFONT*)new LOGFONT;
	set_default();
}

CSubtitleSetting::~CSubtitleSetting()
{
	if (lf)
		delete lf;
}

void CSubtitleSetting::set_default()
{
	memset(lf, 0, sizeof(LOGFONT));

	font_size = 36;
	font_auto_size = false;
	char_per_line = 20;
	font_scaleX = font_scaleY = 100;

	_tcscpy_s(lf->lfFaceName, LF_FACESIZE, _T("Yu Gothic UI"));
	lf->lfCharSet = DEFAULT_CHARSET;
	lf->lfHeight = -MulDiv(font_size, GetDeviceCaps(::GetDC(NULL), LOGPIXELSY), 72);
	lf->lfWeight = FW_DONTCARE;
	lf->lfItalic = false;
	lf->lfUnderline = false;
	lf->lfStrikeOut = false;
	lf->lfQuality = ANTIALIASED_QUALITY;

	display_method = 1;

	marginRect = CRect(20, 20, 20, 20);

	border_style = 0;

	outline_widthX = outline_widthY = 3;
	shadow_depthX = shadow_depthY = 3;

	cr[0] = Gdiplus::Color(0xff, 255, 243, 212);
	cr[1] = Gdiplus::Color(0xff,   0, 255, 255);
	cr[2] = Gdiplus::Color(0xff,   0,   0,   0);
	cr[3] = Gdiplus::Color(0x80,  32,  32,  32);

	pos_x = 50;
	pos_y = 90;

	char_spacing = -2;
	line_spacing = 0;
	text_align = 0;
}


CString& operator <<= (CString& style, CSubtitleSetting& s)
{
	style.Format(_T("\
%s|%d|%d|%d|%d|%d|%d|%d|\
%d|%d|%d|%f|%f|%d|\
%d|\
%d|%d|%d|%d|\
%d|\
%f|%f|\
%f|%f|\
0x%08x|0x%08x|0x%08x|0x%08x|\
%d|%d|\
%d|\
%d"),
		s.lf->lfFaceName,
		s.lf->lfCharSet,
		s.lf->lfHeight,
		s.lf->lfWeight,
		s.lf->lfItalic,
		s.lf->lfUnderline,
		s.lf->lfStrikeOut,
		s.lf->lfQuality,

		s.font_size,
		s.font_auto_size,
		s.char_per_line,
		s.font_scaleX,
		s.font_scaleY,
		s.char_spacing,

		s.display_method,

		s.marginRect.left, s.marginRect.top, s.marginRect.right, s.marginRect.bottom,

		s.border_style,

		s.outline_widthX, s.outline_widthY,
		s.shadow_depthX, s.shadow_depthY,

		s.cr[0].GetValue(), s.cr[1].GetValue(), s.cr[2].GetValue(), s.cr[3].GetValue(),

		s.pos_x, s.pos_y,
		s.line_spacing,
		s.text_align
	);

	return(style);
}

CSubtitleSetting& operator <<= (CSubtitleSetting& s, CString& style)
{
	s.set_default();

	try
	{
		CString str = style;
		if(str.Find(';') >= 0)
			str.Replace(_T(";"), _T("|"));

		if(str.Find('|') >= 0)
		{
			_tcscpy_s(s.lf->lfFaceName, LF_FACESIZE, get_str(str));
			s.lf->lfCharSet = get_int(str);
			s.lf->lfHeight = get_int(str);
			s.lf->lfWeight = get_int(str);
			s.lf->lfItalic = get_int(str);
			s.lf->lfUnderline = get_int(str);
			s.lf->lfStrikeOut = get_int(str);
			s.lf->lfQuality = get_int(str);

			s.font_size = get_int(str);
			s.font_auto_size = get_int(str);
			s.char_per_line = get_int(str);
			s.font_scaleX = get_double(str);
			s.font_scaleY = get_double(str);	//bug fix: 이전엔 font_scaleX 가 두 번 호출되어 font_scaleY 토큰이 무시되고 다음 필드들이 한 칸씩 밀려 읽혔음.
			s.char_spacing = get_int(str);

			s.display_method = get_int(str);

			s.marginRect.left = get_int(str);
			s.marginRect.top = get_int(str);
			s.marginRect.right = get_int(str);
			s.marginRect.bottom = get_int(str);

			s.border_style = get_int(str);

			s.outline_widthX = get_double(str);
			s.outline_widthY = get_double(str);
			s.shadow_depthX = get_double(str);
			s.shadow_depthY = get_double(str);

			for(ptrdiff_t i = 0; i < 4; i++)
				s.cr[i] = Gdiplus::Color((Gdiplus::ARGB)get_int(str));

			s.pos_x = get_int(str);
			s.pos_y = get_int(str);

			s.line_spacing = get_int(str);
			s.text_align = get_int(str);
		}
	}
	catch(...)
	{
		s.set_default();
	}

	//파싱 후 값 범위 검증. 디버거 강제종료로 인한 부분 쓰기 / 옛 schema 잔여 / 손상된 토큰 등으로
	//garbage 값이 들어오면 GdiPlus path 연산이나 VMR9 SetAlphaBitmap 호출에서 hang 유발 가능.
	//한 필드라도 임계 범위 밖이면 전체 default 로 reset (사용자 customization 유실 < hang 회피).
	if (!s.is_sane())
		s.set_default();

	return(s);
}

bool CSubtitleSetting::is_sane() const
{
	if (!lf)
		return false;

	//font face — empty 면 GdiPlus FontFamily 생성 fail 또는 default fallback. 빈 문자열 차단.
	if (lf->lfFaceName[0] == _T('\0'))
		return false;

	//lfHeight: 음수 = 픽셀 단위, 양수 = 논리 포인트, 0 = 정의 안 됨 (GdiPlus path 가 fail).
	int abs_h = lf->lfHeight < 0 ? -lf->lfHeight : lf->lfHeight;
	if (abs_h < 4 || abs_h > 400)
		return false;

	if (font_size < 4 || font_size > 400)
		return false;
	if (font_scaleX < 10 || font_scaleX > 1000)
		return false;
	if (font_scaleY < 10 || font_scaleY > 1000)
		return false;
	if (char_spacing < -100 || char_spacing > 100)
		return false;

	//marginRect: % 단위 (0~100). 양측 합 100% 이상이면 자막 표시 영역 0 → 의미 없음.
	if (marginRect.left < 0 || marginRect.left > 100 ||
		marginRect.top < 0 || marginRect.top > 100 ||
		marginRect.right < 0 || marginRect.right > 100 ||
		marginRect.bottom < 0 || marginRect.bottom > 100)
		return false;

	if (outline_widthX < 0 || outline_widthX > 100)
		return false;
	if (outline_widthY < 0 || outline_widthY > 100)
		return false;
	if (shadow_depthX < -100 || shadow_depthX > 100)
		return false;
	if (shadow_depthY < -100 || shadow_depthY > 100)
		return false;

	if (pos_x < 0 || pos_x > 100)
		return false;
	if (pos_y < 0 || pos_y > 100)
		return false;

	if (char_per_line < 0 || char_per_line > 999)
		return false;

	//cr[]: 옛 schema (alpha=0 garbage) 잔여 검출 — 4 채널 모두 alpha=0 이면 invalid.
	bool any_alpha = false;
	for (int i = 0; i < 4; i++)
	{
		if (cr[i].GetA() != 0)
		{
			any_alpha = true;
			break;
		}
	}
	if (!any_alpha)
		return false;

	return true;
}
