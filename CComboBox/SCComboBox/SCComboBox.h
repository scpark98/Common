#pragma once

/*
- ������ �����Ϸ��� ���ҽ� �Ӽ����� Owner Draw = Fixed, Has String = true�� ������ ��.
*/

#include <afxwin.h>
#include <vector>
#include "../../colors.h"

// CColorComboBox

class CSCComboBoxColor
{
public:
	CSCComboBoxColor(Gdiplus::Color _cr_text = Gdiplus::Color::Transparent, Gdiplus::Color _cr_back = Gdiplus::Color::Transparent)
	{
		cr_text = _cr_text;
		cr_back = _cr_back;
	}
	Gdiplus::Color	cr_text = Gdiplus::Color::Transparent;
	Gdiplus::Color	cr_back = Gdiplus::Color::Transparent;
};

class CSCComboBox : public CComboBox
{
	DECLARE_DYNAMIC(CSCComboBox)

public:
	CSCComboBox();
	~CSCComboBox();

	CSCColorTheme	m_theme = CSCColorTheme(this);
	void			set_color_theme(int theme);
	void			set_text_color(Gdiplus::Color cr_back);
	void			set_back_color(Gdiplus::Color cr_back);

	//font combo�� �����Ѵ�. add, delete, �⺻ font ������� ��� ���õȴ�.
	void			set_as_font_combo();

	//edit�� ���� �Էµ� text�� GetWindowText()�� ���ؾ��ϰ�
	//select�� ���� text�� GetCurSel()�� ���� �� GetLBText()�� ���ؾ� �´�.
	CString			get_cur_sel_text();

	bool			SetCurSel(int index) { return set_cur_sel(index); }
	bool			set_cur_sel(int index);

	//���� �Էµ� �ؽ�Ʈ�� �о���� �׸� �������� ������ �߰���Ų��. ������Ʈ������ �����Ѵ�.
	//������ ������ �������� ������ �⺻ cr_text�� ����Ѵ�.
	int				add(CString text = _T(""), Gdiplus::Color cr_text = Gdiplus::Color::Transparent);

//design
	//�������� �ȼ� ������ �ƴ� logical_unit�̴�. �� set_font_size()�� ������ ���Ͻ�Ų��.
	//�ȼ� ũ��� ���̸� �����Ѵٸ� set_line_height_px()�� ȣ���Ѵ�.
	void			set_line_height(int height_logical_unit);
	void			set_line_height_px(int height_pixel_unit);

//font
	CString			get_font_name();
	void			set_font_name(LPCTSTR sFontname, BYTE byCharSet = DEFAULT_CHARSET);
	void			set_font_size(int font_size);
	void			set_font_bold(int weight = FW_BOLD);
	LOGFONT			get_log_font() { return m_lf; }
	void			set_log_font(LOGFONT lf);

	void			load_history(CWinApp* app, CString section);
	void			save_history(CWinApp* app, CString section);

	//src���� �����ϴ� �޺��ڽ� �������� �ε����� ����.
	int				find_string(CString src);

//���� ����
	//void			edit_end(bool valid);
	//LRESULT			on_message_CSCEdit(WPARAM wParam, LPARAM lParam);

//tooltip
	//�⺻���� ������ �� ��Ʈ�� ������ ����������
	//disabled�� ��Ʈ���� main�� PreTranslateMessage()���� ó������ ������ ��Ÿ���� �ʴ´�.
	void			use_tooltip(bool use) { m_use_tooltip = use; }
	void			set_tooltip_text(CString text);

	void			add_font_list(CString font_name) { m_font_list.push_back(font_name); }
protected:
//design
	//-1�̸� ��Ʈũ�⿡ ���� �ڵ� ����
	int				m_line_height = -1;

	CBrush			m_br_back;

//��Ʈ ����
	LOGFONT			m_lf;
	CFont			m_font;
	int				m_font_size;
	void			reconstruct_font();

	bool			m_is_font_combo = false;
	std::vector<CString> m_font_list;

//���� ����
	//bool			m_use_edit = false;		//���� �׸� �̿��� ���� Ŭ���� ���� ��������� ����� ������
	//CEdit*			m_pEdit = NULL;
	//CString			m_old_text;				//�����Ǳ� ���� ���� �ؽ�Ʈ
	//CRect			m_edit_margin;			//edit box ���� ����(���η� ��� ���ĵǰ� ǥ���ϱ� ����)
	//void			repos_edit();			//resize�� �ϸ� ������ ���µǹǷ� ��ġ�� ������ �ٽ� ���

//���ã�� ����
	CString			m_reg_section;		//load or save�� �� �Ѿ�� section���� ����س��´�.

//tooltip ����
	//pointer Ÿ������ ������ ������ ���������ÿ��� ������ �����ϱ� ����.
	CToolTipCtrl*	m_tooltip = NULL;
	//default = true
	bool			m_use_tooltip = true;
	CString			m_tooltip_text = _T("");

	//�������� ���� ��Ʈ���� ���������� �������� ��Ʈ���� �����Ͽ� ����ϴ� ���
	//PreSubclassWindow()���� ������ �ʱ�ȭ�Ϸ��� ���ܰ� �߻���.
	//�׷��� Create()�Ŀ� ������ prepare_tooltip()�� ȣ���Ͽ� �غ�ǵ��� ����.
	//���� ������ ��Ʈ�ѿ����� ���� ǥ�õ��� Ȯ����.
	void			prepare_tooltip();

//�Է� �� ���� ���͸� ���
	//listbox�� ���ĳ��� ���¿����� �ڵ� ���͸��ȴ�.
	//�׷��ٸ� Ÿ������ ���۵Ǹ� listbox�� �ڵ����� ������� �Ѵ�.
	//�ϴ� �⺻���� true�� �Ѵ�.
	bool			m_use_input_filtering = true;

	// 필터링 결과 항목 수에 맞춰 dropdown 높이를 동적 조정. PreSubclassWindow 에서
	// 원본 리소스 지정 높이를 cache 해 상한으로 사용.
	int				m_initial_height = -1;
	void			adjust_dropdown_height();

	// WM_IME_COMPOSITION + CBN_EDITCHANGE 가 동일 상태에 대해 연속 호출되는 경우
	// listbox 재구성을 1회만 수행하기 위한 이전 filter 키워드.
	CString			m_last_filter;

	// 지연 필터링용 타이머. 입력 이벤트마다 타이머를 reset 해 "멈춘 시점" 을 감지.
	// 타이머 만료 시 composing 이면 IMM 을 CPS_COMPLETE 로 강제 commit → CBN_EDITCHANGE 가
	// non-composing 상태로 재진입. delay 값은 SCComboBox.cpp 에 상수로 정의.
	enum { TIMER_INPUT_FILTER = 0x5C01 };
	void			apply_filter_now();

	DECLARE_MESSAGE_MAP()
public:
	virtual void DrawItem(LPDRAWITEMSTRUCT /*lpDrawItemStruct*/);
	virtual void MeasureItem(LPMEASUREITEMSTRUCT /*lpMeasureItemStruct*/);
	virtual int CompareItem(LPCOMPAREITEMSTRUCT /*lpCompareItemStruct*/);
	//afx_msg void OnNcPaint();
	//afx_msg void OnPaint();
	//afx_msg void OnEditUpdate();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnCbnDropdown();
	virtual void PreSubclassWindow();
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnCbnSetfocus();
	afx_msg void OnCbnKillfocus();
	afx_msg BOOL OnCbnSelchange();
	afx_msg void OnCbnSelendok();
	afx_msg void OnCbnSelendcancel();
	afx_msg HBRUSH CtlColor(CDC* /*pDC*/, UINT /*nCtlColor*/);
	HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnNcPaint();
	afx_msg void OnDestroy();
	afx_msg void OnCbnEditchange();
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};


