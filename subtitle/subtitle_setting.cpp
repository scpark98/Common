
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
			s.font_scaleX = get_double(str);
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

	return(s);
}
