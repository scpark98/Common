#pragma once

//scpark. �ܼ� ������ ���͸�ũ�� ǥ���ϴ� ������ â�̹Ƿ�
//���� CDialog�� ��ӹ��� �ʰ� IDD�� ���� resource�� �ʿ���� ����â���� ����.
/*
[usage]
	.h�� ���� ����
	CWatermarkWnd	m_watermarkWnd;
 
	.cpp�� OnInitDialog()���� ����
	m_watermarkWnd.create(m_hWnd, m_edit_text, rc);

	OnSize() �Ǵ� OnWindowPosChanged()���� ���ϴ� ũ��� MoveWindow()
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

