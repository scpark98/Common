#pragma once

/*
* scpark 20240411
* resource의 IDD 없이 동적으로 modeless dialog를 생성하고
* progress를 표시하기 위해 제작.
* 
* [사용 방법]
	//.h 파일에 인스턴스 선언.
	CSCProgressDlg		m_progressDlg;

	//.cpp의 OnInitDialog()에 생성
	m_progressDlg.create(this, msg, 0, 0, 320, 120);
	m_progressDlg.set_text_color(cr_text);
	m_progressDlg.set_back_color(cr_back);
	m_progressDlg.set_indeterminate();
	m_progressDlg.SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	m_progressDlg.CenterWindow(this);

	//필요한 곳에서 show / hide
	m_progressDlg.ShowWindow(SW_SHOW);

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
	void			set_text_color(Gdiplus::Color cr);
	void			set_back_color(Gdiplus::Color cr);
	void			set_font_size(int size);

	void			set_range(int32_t lower, int32_t upper);
	void			SetRange(int32_t lower, int32_t upper) { set_range(lower, upper); }
	void			get_range(int32_t& lower, int32_t upper);
	int32_t			get_lower();
	int32_t			get_upper();

	void			set_pos(int32_t pos) { m_progress.SetPos(pos); }
	void			SetPos(int32_t pos) { m_progress.SetPos(pos); }
	void			set_indeterminate(bool indeterminate = true);

	//마우스로 드래그하여 이동가능하게 설정
	void			set_enable_move(bool enable = true) { m_enable_move = enable; }

	//step 단위 증감, 변경된 위치값 리턴.
	int32_t			step(int step = 1);

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

	bool			m_enable_move = false;

	LOGFONT			m_lf;
	CFont			m_font;
	void			reconstruct_font();

	Gdiplus::Color	m_cr_back = ::GetSysColor(COLOR_3DFACE);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
};
