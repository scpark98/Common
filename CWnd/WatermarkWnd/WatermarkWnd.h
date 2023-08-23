#pragma once

//scpark. 단순 투명한 워터마크를 표시하는 목적의 창이므로
//굳이 CDialog를 상속받지 않고 IDD와 같은 resource가 필요없는 투명창으로 만듬.
/*
[usage]
	.h에 변수 선언
	CWatermarkWnd	m_watermarkWnd;
 
	.cpp의 OnInitDialog()에서 생성
	m_watermarkWnd.create(m_hWnd, m_edit_text, rc);

	OnSize() 또는 OnWindowPosChanged()에서 원하는 크기로 MoveWindow()
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

	bool		create(HWND parentHwnd, CString text = _T(""), CRect rw = 0);

	void		set_text(CString text, int font_size = -1, int font_angle = -1);
	void		set_font_size(int size);
	void		set_font_angle(int angle);
	void		set_font_color(COLORREF color);

	LOGFONT		m_lf;
	CFont		m_font;
	CString		m_text = _T("Watermark Test String");
	int			m_font_size = 20;
	int			m_font_angle = 20;
	COLORREF	m_font_color = RGB(132, 132, 132);

protected:
	BOOL RegisterWindowClass();

	void		reconstruct_font();

protected:
	DECLARE_MESSAGE_MAP()
};

static LRESULT CALLBACK WatermarkWndMessageProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

