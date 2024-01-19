#pragma once

//scpark. 단순 투명한 워터마크를 표시하는 목적의 창이므로
//굳이 CDialog를 상속받지 않고 IDD와 같은 resource도 필요없는 투명창으로 동작하도록 만듬.
/*
[usage]
	//.h에 변수 선언
	CWatermarkWnd	m_watermarkWnd;
 
	//.cpp의 OnInitDialog()에서 생성. rc가 주어지지 않으면 parent의 client 영역크기로 생성.
	m_watermarkWnd.create(m_hWnd, m_edit_text, rc);

	//parentWnd의 OnSize() 또는 OnWindowPosChanged()에서 보정함수 호출.
	m_watermarkWnd.adjust_window();
*/

#include <afxwin.h>

typedef BOOL(WINAPI* SLWA)(HWND hWnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);

// CWatermarkWnd

class CWatermarkWnd : public CWnd
{
	DECLARE_DYNAMIC(CWatermarkWnd)

public:
	CWatermarkWnd();
	virtual ~CWatermarkWnd();

	//watermarkWnd가 위치할 rw가 별도로 지정되지 않으면
	//parent의 client 영역 크기로 생성된다.
	bool		create(HWND parentHwnd, CString text = _T(""), CRect rw = 0);

	void		set_text(CString text, int font_size = -1, int font_angle = -1);
	void		set_font_name(CString fontname, BYTE byCharSet = DEFAULT_CHARSET);
	void		set_font_size(int size);
	void		set_font_angle(int angle);
	void		set_font_color(COLORREF color);

	void		adjust_window();

	LOGFONT		m_lf;
	CFont		m_font;
	CString		m_text = _T("Watermark Test String");
	int			m_font_size = 16;
	int			m_font_angle = 20;
	COLORREF	m_font_color = RGB(128, 128, 128);

protected:
	BOOL		RegisterWindowClass();

	void		reconstruct_font();

protected:
	DECLARE_MESSAGE_MAP()
};

static LRESULT CALLBACK WatermarkWndMessageProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

