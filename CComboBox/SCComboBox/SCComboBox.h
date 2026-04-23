#pragma once

/*
- 占쏙옙占쏙옙占쏙옙 占쏙옙占쏙옙占싹뤄옙占쏙옙 占쏙옙占쌀쏙옙 占쌈쇽옙占쏙옙占쏙옙 Owner Draw = Fixed, Has String = true占쏙옙 占쏙옙占쏙옙占쏙옙 占쏙옙.
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

	//font combo占쏙옙 占쏙옙占쏙옙占싼댐옙. add, delete, 占썩본 font 占쏙옙占쏙옙占쏙옙占?占쏙옙占?占쏙옙占시된댐옙.
	void			set_as_font_combo();

	//edit占쏙옙 占쏙옙占쏙옙 占쌉력듸옙 text占쏙옙 GetWindowText()占쏙옙 占쏙옙占쌔억옙占싹곤옙
	//select占쏙옙 占쏙옙占쏙옙 text占쏙옙 GetCurSel()占쏙옙 占쏙옙占쏙옙 占쏙옙 GetLBText()占쏙옙 占쏙옙占쌔억옙 占승댐옙.
	CString			get_cur_sel_text();

	bool			SetCurSel(int index) { return set_cur_sel(index); }
	bool			set_cur_sel(int index);

	//占쏙옙占쏙옙 占쌉력듸옙 占쌔쏙옙트占쏙옙 占싻억옙占쏙옙占?占쌓몌옙 占쏙옙占쏙옙占쏙옙占쏙옙 占쏙옙占쏙옙占쏙옙 占쌩곤옙占쏙옙킨占쏙옙. 占쏙옙占쏙옙占쏙옙트占쏙옙占쏙옙占쏙옙 占쏙옙占쏙옙占싼댐옙.
	//占쏙옙占쏙옙占쏙옙 占쏙옙占쏙옙占쏙옙 占쏙옙占쏙옙占쏙옙占쏙옙 占쏙옙占쏙옙占쏙옙 占썩본 cr_text占쏙옙 占쏙옙占쏙옙磯占?
	int				add(CString text = _T(""), Gdiplus::Color cr_text = Gdiplus::Color::Transparent);

//design
	//占쏙옙占쏙옙占쏙옙占쏙옙 占싫쇽옙 占쏙옙占쏙옙占쏙옙 占싣댐옙 logical_unit占싱댐옙. 占쏙옙 set_font_size()占쏙옙 占쏙옙占쏙옙占쏙옙 占쏙옙占싹쏙옙킨占쏙옙.
	//占싫쇽옙 크占쏙옙占?占쏙옙占싱몌옙 占쏙옙占쏙옙占싼다몌옙 set_line_height_px()占쏙옙 호占쏙옙占싼댐옙.
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

	//src占쏙옙占쏙옙 占쏙옙占쏙옙占싹댐옙 占쌨븝옙占쌘쏙옙 占쏙옙占쏙옙占쏙옙占쏙옙 占싸듸옙占쏙옙占쏙옙 占쏙옙占쏙옙.
	int				find_string(CString src);

//占쏙옙占쏙옙 占쏙옙占쏙옙
	//void			edit_end(bool valid);
	//LRESULT			on_message_CSCEdit(WPARAM wParam, LPARAM lParam);

//tooltip
	//占썩본占쏙옙占쏙옙 占쏙옙占쏙옙占쏙옙 占쏙옙 占쏙옙트占쏙옙 占쏙옙占쏙옙占쏙옙 占쏙옙占쏙옙占쏙옙占쏙옙占쏙옙
	//disabled占쏙옙 占쏙옙트占쏙옙占쏙옙 main占쏙옙 PreTranslateMessage()占쏙옙占쏙옙 처占쏙옙占쏙옙占쏙옙 占쏙옙占쏙옙占쏙옙 占쏙옙타占쏙옙占쏙옙 占십는댐옙.
	void			use_tooltip(bool use) { m_use_tooltip = use; }
	void			set_tooltip_text(CString text);

	void			add_font_list(CString font_name) { m_font_list.push_back(font_name); }
protected:
//design
	//-1占싱몌옙 占쏙옙트크占썩에 占쏙옙占쏙옙 占쌘듸옙 占쏙옙占쏙옙
	int				m_line_height = -1;

	CBrush			m_br_back;

//占쏙옙트 占쏙옙占쏙옙
	LOGFONT			m_lf;
	CFont			m_font;
	int				m_font_size;
	void			reconstruct_font();

	bool			m_is_font_combo = false;
	std::vector<CString> m_font_list;

//占쏙옙占쏙옙 占쏙옙占쏙옙
	//bool			m_use_edit = false;		//占쏙옙占쏙옙 占쌓몌옙 占싱울옙占쏙옙 占쏙옙占쏙옙 클占쏙옙占쏙옙 占쏙옙占쏙옙 占쏙옙占쏙옙占쏙옙占쏙옙占?占쏙옙占쏙옙占?占쏙옙占쏙옙占쏙옙
	//CEdit*			m_pEdit = NULL;
	//CString			m_old_text;				//占쏙옙占쏙옙占실깍옙 占쏙옙占쏙옙 占쏙옙占쏙옙 占쌔쏙옙트
	//CRect			m_edit_margin;			//edit box 占쏙옙占쏙옙 占쏙옙占쏙옙(占쏙옙占싸뤄옙 占쏙옙占쏘데 占쏙옙占식되곤옙 표占쏙옙占싹깍옙 占쏙옙占쏙옙)
	//void			repos_edit();			//resize占쏙옙 占싹몌옙 占쏙옙占쏙옙占쏙옙 占쏙옙占승되므뤄옙 占쏙옙치占쏙옙 占쏙옙占쏙옙占쏙옙 占쌕쏙옙 占쏙옙占?

//占쏙옙占시ｏ옙占?占쏙옙占쏙옙
	CString			m_reg_section;		//load or save占쏙옙 占쏙옙 占싼억옙占?section占쏙옙占쏙옙 占쏙옙占쏙옙卍占쏙옙쨈占?

//tooltip 占쏙옙占쏙옙
	//pointer 타占쏙옙占쏙옙占쏙옙 占쏙옙占쏙옙占쏙옙 占쏙옙占쏙옙占쏙옙 占쏙옙占쏙옙占쏙옙占쏙옙占시울옙占쏙옙 占쏙옙占쏙옙占쏙옙 占쏙옙占쏙옙占싹깍옙 占쏙옙占쏙옙.
	CToolTipCtrl*	m_tooltip = NULL;
	//default = true
	bool			m_use_tooltip = true;
	CString			m_tooltip_text = _T("");

	//占쏙옙占쏙옙占쏙옙占쏙옙 占쏙옙占쏙옙 占쏙옙트占쏙옙占쏙옙 占쏙옙占쏙옙占쏙옙占쏙옙占쏙옙 占쏙옙占쏙옙占쏙옙占쏙옙 占쏙옙트占쏙옙占쏙옙 占쏙옙占쏙옙占싹울옙 占쏙옙占쏙옙求占?占쏙옙占?
	//PreSubclassWindow()占쏙옙占쏙옙 占쏙옙占쏙옙占쏙옙 占십깍옙화占싹뤄옙占쏙옙 占쏙옙占쌤곤옙 占쌩삼옙占쏙옙.
	//占쌓뤄옙占쏙옙 Create()占식울옙 占쏙옙占쏙옙占쏙옙 prepare_tooltip()占쏙옙 호占쏙옙占싹울옙 占쌔븝옙풩占쏙옙占?占쏙옙占쏙옙.
	//占쏙옙占쏙옙 占쏙옙占쏙옙占쏙옙 占쏙옙트占싼울옙占쏙옙占쏙옙 占쏙옙占쏙옙 표占시듸옙占쏙옙 확占쏙옙占쏙옙.
	void			prepare_tooltip();

//占쌉뤄옙 占쏙옙 占쏙옙占쏙옙 占쏙옙占싶몌옙 占쏙옙占?
	//listbox占쏙옙 占쏙옙占식놂옙占쏙옙 占쏙옙占승울옙占쏙옙占쏙옙 占쌘듸옙 占쏙옙占싶몌옙占싫댐옙.
	//占쌓뤄옙占쌕몌옙 타占쏙옙占쏙옙占쏙옙 占쏙옙占쌜되몌옙 listbox占쏙옙 占쌘듸옙占쏙옙占쏙옙 占쏙옙占쏙옙占쏙옙占?占싼댐옙.
	//占싹댐옙 占썩본占쏙옙占쏙옙 true占쏙옙 占싼댐옙.
	bool			m_use_input_filtering = true;

	// ?꾪꽣留?寃곌낵 ??ぉ ?섏뿉 留욎떠 dropdown ?믪씠瑜??숈쟻 議곗젙. PreSubclassWindow ?먯꽌
	// ?먮낯 由ъ냼??吏???믪씠瑜?cache ???곹븳?쇰줈 ?ъ슜.
	int				m_initial_height = -1;
	void			adjust_dropdown_height();

	// WM_IME_COMPOSITION + CBN_EDITCHANGE 媛 ?숈씪 ?곹깭??????곗냽 ?몄텧?섎뒗 寃쎌슦
	// listbox ?ш뎄?깆쓣 1?뚮쭔 ?섑뻾?섍린 ?꾪븳 ?댁쟾 filter ?ㅼ썙??
	CString			m_last_filter;

	// 吏???꾪꽣留곸슜 ??대㉧. ?낅젰 ?대깽?몃쭏????대㉧瑜?reset ??"硫덉텣 ?쒖젏" ??媛먯?.
	// ??대㉧ 留뚮즺 ??composing ?대㈃ IMM ??CPS_COMPLETE 濡?媛뺤젣 commit ??CBN_EDITCHANGE 媛
	// non-composing ?곹깭濡??ъ쭊?? delay 媛믪? SCComboBox.cpp ???곸닔濡??뺤쓽.
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


