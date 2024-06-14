#pragma once

/*
* scpark 20240411
* resource의 IDD 없이 동적으로 modeless dialog를 생성하고
* progress를 표시하기 위해 제작.
* 
*/

#include <afxdialogex.h>
#include "../../CStatic/SCStatic/SCStatic.h"
#include "../../CProgressCtrl/MacProgressCtrl/MacProgressCtrl.h"

// CSCProgressDlg 대화 상자

class CSCProgressDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSCProgressDlg)

public:
	CSCProgressDlg();   // 표준 생성자입니다.
	~CSCProgressDlg();

	bool			create(CWnd* parent, CString text, int left, int top, int right, int bottom);
	void			set_text(CString text);
	void			set_text_color(COLORREF cr);
	void			set_back_color(COLORREF cr);
	void			set_font_size(int size);
	void			set_range(int32_t lower, int32_t upper);
	void			set_indeterminate(bool indeterminate = true);

	//
	void			timeout(int ms);
	//특정 실행파일이 실행되면 자동 숨김
	//max_timeout_ms후에는 숨김처리.
	//hide조건이 되도 intentional_delay_before_hide_ms후에 숨긴다.
	void			auto_hide_when_app_found(CString exe_name, int max_timeout_ms, int intentional_delay_before_hide_ms = 0);

	CSCStatic		m_static;
	CMacProgressCtrl m_progress;

protected:
	enum TIMER_ID
	{
		timer_auto_hide = 0,
	};

	CWnd*			m_parent = NULL;

	LOGFONT			m_lf;
	CFont			m_font;
	void			reconstruct_font();

	COLORREF		m_cr_back = ::GetSysColor(COLOR_3DFACE);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};
