//CAutoFont class implementation

//CAutoFont class implementation
//#include "stdafx.h"
#include "AutoFont.h"

CAutoFont::CAutoFont()
{
	lf.lfHeight=16;
	lf.lfWidth=0;
	lf.lfEscapement=0;
	lf.lfOrientation=0;
	lf.lfWeight=FW_NORMAL;
	lf.lfItalic=0;
	lf.lfUnderline=0;
	lf.lfStrikeOut=0;
	lf.lfCharSet=DEFAULT_CHARSET;
	lf.lfOutPrecision=OUT_DEFAULT_PRECIS;
	lf.lfClipPrecision=CLIP_DEFAULT_PRECIS;
	lf.lfQuality = CLEARTYPE_NATURAL_QUALITY;// ANTIALIASED_QUALITY;
	lf.lfPitchAndFamily=VARIABLE_PITCH | FF_ROMAN;
	_tcscpy_s(lf.lfFaceName, _countof(lf.lfFaceName), TEXT("Arial"));

	CreateFontIndirect(&lf);
}

CAutoFont::CAutoFont(CString facename)
{
	lf.lfHeight=16;
	lf.lfWidth=0;
	lf.lfEscapement=0;
	lf.lfOrientation=0;
	lf.lfWeight=FW_NORMAL;
	lf.lfItalic=0;
	lf.lfUnderline=0;
	lf.lfStrikeOut=0;
	lf.lfCharSet=DEFAULT_CHARSET;
	lf.lfOutPrecision=OUT_DEFAULT_PRECIS;
	lf.lfClipPrecision=CLIP_DEFAULT_PRECIS;
	lf.lfQuality = CLEARTYPE_NATURAL_QUALITY;// ANTIALIASED_QUALITY;
	lf.lfPitchAndFamily=VARIABLE_PITCH | FF_ROMAN;
	_tcscpy_s(lf.lfFaceName, _countof(lf.lfFaceName), (LPCTSTR)facename);

	CreateFontIndirect(&lf);
}

CAutoFont::CAutoFont(LOGFONT& logfont)
{
	lf=logfont;
	CreateFontIndirect(&lf);
}

CAutoFont::CAutoFont(CFont font)
{
	HFONT hFont=(HFONT)font;
	Attach((HFONT)hFont);

	GetLogFont(&lf);
}

CAutoFont::~CAutoFont()
{
}

LONG CAutoFont::SetHeight(LONG height)
{
	LONG l=lf.lfHeight;

	DeleteObject();
	lf.lfHeight=height;
	CreateFontIndirect(&lf);

	return l;
}

LONG CAutoFont::SetWidth(LONG width)
{
	LONG l=lf.lfWidth;

	DeleteObject();
	lf.lfWidth=width;
	CreateFontIndirect(&lf);

	return l;
}

LONG CAutoFont::SetEscapement(LONG esc)
{
	LONG l=lf.lfEscapement;

	DeleteObject();
	lf.lfEscapement=esc;
	CreateFontIndirect(&lf);

	return l;
}

LONG CAutoFont::SetOrientation(LONG or)
{
	LONG l=lf.lfOrientation;

	DeleteObject();
	lf.lfOrientation=or;
	CreateFontIndirect(&lf);

	return l;
}

LONG CAutoFont::SetWeight(LONG weight)
{
	LONG l=lf.lfWeight;

	DeleteObject();
	lf.lfWeight=weight;
	CreateFontIndirect(&lf);

	return l;
}

BYTE CAutoFont::SetCharset(BYTE charset)
{
	BYTE b=lf.lfCharSet;

	DeleteObject();
	lf.lfCharSet=charset;
	CreateFontIndirect(&lf);

	return b;
}

BYTE CAutoFont::SetOutPrecision(BYTE op)
{
	BYTE b=lf.lfOutPrecision;

	DeleteObject();
	lf.lfOutPrecision=op;
	CreateFontIndirect(&lf);

	return b;
}

BYTE CAutoFont::SetClipPrecision(BYTE cp)
{
	BYTE b=lf.lfClipPrecision;

	DeleteObject();
	lf.lfClipPrecision=cp;
	CreateFontIndirect(&lf);

	return b;
}

BYTE CAutoFont::SetQuality(BYTE qual)
{
	BYTE b=lf.lfQuality;

	DeleteObject();
	lf.lfQuality = qual;
	CreateFontIndirect(&lf);

	return b;
}

BYTE CAutoFont::SetPitchAndFamily(BYTE paf)
{
	BYTE b=lf.lfPitchAndFamily;

	DeleteObject();
	lf.lfPitchAndFamily=paf;
	CreateFontIndirect(&lf);

	return b;
}

CString CAutoFont::SetFaceName(CString facename)
{
	CString str=lf.lfFaceName;

	DeleteObject();
	_tcscpy_s(lf.lfFaceName, _countof(lf.lfFaceName), (LPCTSTR)facename);
	CreateFontIndirect(&lf);

	return str;
}

LPCTSTR CAutoFont::SetFaceName(LPCTSTR facename)
{
	LPCTSTR str=lf.lfFaceName;

	DeleteObject();
	_tcscpy_s(lf.lfFaceName, _countof(lf.lfFaceName), facename);
	CreateFontIndirect(&lf);

	return str;
}

BOOL CAutoFont::SetBold(BOOL B)
{
	BOOL b;

	if (B)
		b=SetWeight(FW_BOLD);
	else
		b=SetWeight(FW_NORMAL);

	if (b >= FW_MEDIUM)
		return TRUE;
	else
		return FALSE;
}

BOOL CAutoFont::SetItalic(BOOL i)
{
	BOOL b=(BOOL)lf.lfItalic;

	DeleteObject();
	lf.lfItalic=i;
	CreateFontIndirect(&lf);

	return b;
}

BOOL CAutoFont::SetUnderline(BOOL u)
{
	BOOL b=(BOOL)lf.lfUnderline;

	DeleteObject();
	lf.lfUnderline=u;
	CreateFontIndirect(&lf);

	return b;
}

BOOL CAutoFont::SetStrikeOut(BOOL s)
{
	BOOL b=(BOOL)lf.lfStrikeOut;

	DeleteObject();
	lf.lfStrikeOut=s;
	CreateFontIndirect(&lf);

	return b;
}

void CAutoFont::SetLogFont(LOGFONT& logfont)
{
	lf=logfont;
	DeleteObject();
	CreateFontIndirect(&lf);
}

LONG CAutoFont::GetHeight()
{
	return lf.lfHeight;
}

LONG CAutoFont::GetWidth()
{
	return lf.lfWidth;
}

LONG CAutoFont::GetEscapement()
{
	return lf.lfEscapement;
}

LONG CAutoFont::GetOrientation()
{
	return lf.lfEscapement;
}

LONG CAutoFont::GetWeight()
{
	return lf.lfWeight;
}

BYTE CAutoFont::GetCharset()
{
	return lf.lfCharSet;
}

BYTE CAutoFont::GetOutPrecision()
{
	return lf.lfOutPrecision;
}

BYTE CAutoFont::GetClipPrecision()
{
	return lf.lfClipPrecision;
}

BYTE CAutoFont::GetQuality()
{
	return lf.lfQuality;
}

BYTE CAutoFont::GetPitchAndFamily()
{
	return lf.lfPitchAndFamily;
}

LPCTSTR CAutoFont::GetFaceName()
{
	return lf.lfFaceName;
}

BOOL CAutoFont::GetBold()
{
	return lf.lfWeight >= FW_MEDIUM ? TRUE : FALSE;
}

BOOL CAutoFont::GetItalic()
{
	return (BOOL)lf.lfItalic;
}

BOOL CAutoFont::GetUnderline()
{
	return (BOOL)lf.lfUnderline;
}

BOOL CAutoFont::GetStrikeOut()
{
	return (BOOL)lf.lfStrikeOut;
}

CString CAutoFont::ContractFont()
{
	CString str;

	str.Format(TEXT("%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%s"),
		lf.lfHeight,
		lf.lfWidth,
		lf.lfEscapement,
		lf.lfOrientation,
		lf.lfWeight,
		lf.lfItalic,
		lf.lfUnderline,
		lf.lfStrikeOut,
		lf.lfCharSet,
		lf.lfOutPrecision,
		lf.lfClipPrecision,
		lf.lfQuality,
		lf.lfPitchAndFamily,
		lf.lfFaceName);

	return str;
}

void CAutoFont::ExtractFont(CString& str)
{
	lf.lfHeight				=	_ttol((LPCTSTR)GetToken(str, TEXT(",")));
	lf.lfWidth				=	_ttol((LPCTSTR)GetToken(str, TEXT(",")));
	lf.lfEscapement			=	_ttol((LPCTSTR)GetToken(str, TEXT(",")));
	lf.lfOrientation		=	_ttol((LPCTSTR)GetToken(str, TEXT(",")));
	lf.lfWeight				=	_ttol((LPCTSTR)GetToken(str, TEXT(",")));
	lf.lfItalic				=	_ttoi((LPCTSTR)GetToken(str, TEXT(",")));
	lf.lfUnderline			=	_ttoi((LPCTSTR)GetToken(str, TEXT(",")));
	lf.lfStrikeOut			=	_ttoi((LPCTSTR)GetToken(str, TEXT(",")));
	lf.lfCharSet			=	_ttoi((LPCTSTR)GetToken(str, TEXT(",")));
	lf.lfOutPrecision		=	_ttoi((LPCTSTR)GetToken(str, TEXT(",")));
	lf.lfClipPrecision		=	_ttoi((LPCTSTR)GetToken(str, TEXT(",")));
	lf.lfQuality			=	_ttoi((LPCTSTR)GetToken(str, TEXT(",")));
	lf.lfPitchAndFamily		=	_ttoi((LPCTSTR)GetToken(str, TEXT(",")));
	_tcscpy_s(lf.lfFaceName, _countof(lf.lfFaceName), (LPCTSTR)str);

	DeleteObject();
	CreateFontIndirect(&lf);
}

CString CAutoFont::GetToken(CString& str, LPCTSTR c)
{
	int pos;
	CString token;

	pos=str.Find(c);
	token=str.Left(pos);
	str=str.Mid(pos+1);

	return token;
}

void CAutoFont::GetFontFromDialog(CFont *f, DWORD *color,
				CDC *pPrinterDC, CWnd *pParentWnd)
{
	LOGFONT tlf;
	if (f==NULL)
		tlf=lf;
	else
		f->GetLogFont(&tlf);

	CFontDialog dlg(&tlf, CF_EFFECTS | CF_SCREENFONTS,
		pPrinterDC, pParentWnd);
	dlg.m_cf.rgbColors=fontColor;
	
	if (dlg.DoModal()==IDOK)
	{
		dlg.GetCurrentFont(&lf);
		DeleteObject();
		CreateFontIndirect(&lf);
		f=(CFont *)this;
		color=&dlg.m_cf.rgbColors;
		SetFontColor(dlg.m_cf.rgbColors);
	}
}

void CAutoFont::SetFontColor(COLORREF color)
{
	fontColor=color;
	if (hDC!=NULL)
		::SetTextColor(hDC, color);
}

COLORREF CAutoFont::GetFontColor()
{
	return fontColor;
}

void CAutoFont::SetDC(HDC dc)
{
	hDC=dc;
}