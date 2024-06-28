#pragma once

#include <Afxwin.h>
#include <afxcmn.h>

/*
* �ð�ǥ�ø� ���� �ʴ� ���� ��������� �ð�ǥ�ø� �ϴ� ����
  text�� �� �Ǵ� �ڿ� \n�� ���� ��� �ΰ����� ó���� �ʿ��ϴ�.

  - �տ� ���� ��� 

*/
//scpark.
//call AfxInitRichEdit2() at App()::InitInstance() for using RECHED20.dll

// CRichEditCtrlEx

class CRichEditCtrlEx : public CRichEditCtrl
{
	DECLARE_DYNAMIC(CRichEditCtrlEx)

public:
	CRichEditCtrlEx();
	virtual ~CRichEditCtrlEx();

	enum CRichEditCtrlExMenu
	{
		id_menu_richedit_toggle_log = 9900,
		id_menu_richedit_clearl_log,
		id_menu_richedit_line_space10,
		id_menu_richedit_line_space15,
		id_menu_richedit_line_space20,
		id_menu_richedit_toggle_time,
	};

	void		SetDefaultTextColor(COLORREF crText) { m_crText = crText; }

	//back color�� �����Ѵ�.
	void		SetBackColor(COLORREF crBack);
	void		SetClearLogInterval(int nInterval);
	void		SetMaxCharLimit(int nMaxChar) { m_nMaxCharLimit = nMaxChar; }

	void		ClearAll();

	void		ToggleShowLog();
	void		ShowLog(bool bShow = true) { m_show_log = bShow; }
	bool		IsShowLog() { return m_show_log; }

	void		ToggleShowTime();
	void		ShowTimeInfo(bool bShow = true) { m_show_time = bShow; }
	bool		IsShowTime() { return m_show_time; }

	void		use_popup_menu(bool use) { m_use_popup_menu = use; }

	UINT		GetLineSpacing();
	//�ٰ���. 0=1��, 1=1.5��, 2=2.0��
	void		SetLineSpacing(UINT nLineSpace);

	//20231004. Append~�� ���۵Ǵ� 4���� �Լ��� 1���� ����ȭ�Ѵ�.
	//�� ���� "\n"�� �ڵ����� �ٿ����� �����Ƿ� �ʿ��ϴٸ� addl()�Լ��� ����Ѵ�.
	//��Ȥ �� ���ο� �߰����� �α׸� ǥ���� �ʿ䰡 �����Ƿ� add(), addl()�� �и���.
	CString		add(COLORREF cr, LPCTSTR lpszFormat, ...);
	//addl�� add line�� ���ڷ� �� ���� "\n"�� �߰��� �� add�� �����ϴ�.
	CString		addl(COLORREF cr, LPCTSTR lpszFormat, ...);

	int			AppendToLog(CString str, COLORREF color = -1, BOOL bAddNewLine = TRUE);	//color�� -1�̸� �⺻ �÷��� ����Ѵ�.(m_crText)
	void		Append(LPCTSTR lpszFormat, ...);
	void		Append(COLORREF cr, LPCTSTR lpszFormat, ...);
	int			AppendToLogAndScroll(CString str, COLORREF color = -1, BOOL bAddNewLine = TRUE);



	int			GetNumVisibleLines();

	COLORREF	GetComplementaryColor(COLORREF crColor);

	void		auto_scroll(bool auto_scroll) { m_auto_scroll = auto_scroll; }
	void		SetScrollSize(int nLines) { m_nScrollSize = nLines; }

	int			get_align() { return m_align; }
	//PFA_LEFT(1), PFA_RIGHT(2), PFA_CENTER(3)
	void		set_align(int align);

	virtual		CRichEditCtrlEx& SetFontName(TCHAR *sfontname);
	virtual		CRichEditCtrlEx& SetFontSize(int nSize);
	virtual		CRichEditCtrlEx& SetFontBold(bool bBold = true);

protected:
	COLORREF	m_crText;
	COLORREF	m_crBack;
	bool		m_show_log;
	bool		m_show_time;
	bool		m_use_popup_menu = true;

	int			m_nClearLogInterval;	//KIOSK���� �޸� ������ ���� ���� �ֱ������� �α� ������ �����ִ� Ÿ�̸� ����(����.��, 0�̸� ���� ����)
	int			m_nMaxCharLimit;		//������ ���ڼ� �̻��̸� ��� ����� ���� ����.
	int			m_nScrollSize;
	bool		m_auto_scroll = true;

	int			m_align = PFA_LEFT;

	void		OnPopupMenu(UINT menuID);

	LOGFONT		m_lf;
	CFont		m_font;
	int			m_nDefaultHeight;
	void		UpdateSurface();
	void		ReconstructFont();


protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnMouseHWheel(UINT nFlags, short zDelta, CPoint pt);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void PreSubclassWindow();
};
