#pragma once

#include <afxwin.h>
#include <afxdialogex.h>
#include <deque>

#include "../../CButton/GdiButton/GdiButton.h"

#define SCTOOLBAR_BUTTON_ID	WM_USER + 200

// CSCToolBar 대화 상자

class CSCToolBar : public CDialogEx
{
	DECLARE_DYNAMIC(CSCToolBar)

public:
	CSCToolBar();   // 표준 생성자입니다.
	~CSCToolBar();

	void		init(CWnd* parent, int x = 0, int y = 0, int width = 0, int height = 0);

	//resourceID와 button_type(BS_PUSHBUTTON or BS_CHECKBOX or BS_RADIOBUTTON...)만 주면 버튼 추가
	void		add(CString caption, UINT nID, int button_type = BS_PUSHBUTTON);
	void		insert(int index, CString caption, UINT nID, int button_type = BS_PUSHBUTTON);
	void		remove(int index);
	void		set_back_color(COLORREF cr_back);

protected:
	CWnd*		m_parent = NULL;

	bool		create();
	void		rearrange_toolbar();

	LRESULT		on_message_GdiButton(WPARAM wParam, LPARAM lParam);

	int			m_sx = 0;
	int			m_sy = 0;
	int			m_width = 50;	//단일 툴바 버튼 너비
	int			m_height = 50;	//단일 툴바 버튼 높이

	COLORREF	m_cr_back = ::GetSysColor(COLOR_3DFACE);

	std::deque<CGdiButton*> m_toolbar;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};
