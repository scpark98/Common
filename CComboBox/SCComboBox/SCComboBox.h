#pragma once

/*
- 리소스 속성에서 Owner Draw = Fixed, Has String = true로 변경.
*/

#include <afxwin.h>

#include "../../colors.h"

// CColorComboBox

class CSCComboBox : public CComboBox
{
	DECLARE_DYNAMIC(CSCComboBox)

public:
	CSCComboBox();
	virtual ~CSCComboBox();

	CSCColorTheme	m_theme = CSCColorTheme(this);
	void			set_color_theme(int theme);

	CString			get_cur_text() { CString text; GetLBText(GetCurSel(), text); return text; }

	//현재 입력된 텍스트를 읽어오고 항목에 존재하지 않으면 추가시킨다. 레지스트리에도 저장한다.
	//색상을 별도로 지정하지 않으면 기본 cr_text를 사용한다.
	int				add(CString text = _T(""), Gdiplus::Color cr_text = Gdiplus::Color::Transparent);

//design
	//설정값은 픽셀 단위가 아닌 logical_unit이다. 즉 set_font_size()의 단위와 통일시킨다.
	//픽셀 크기로 높이를 설정한다면 set_line_height_px()을 호출한다.
	void			set_line_height(int height_logical_unit);
	void			set_line_height_px(int height_pixel_unit);

//font
	void			set_font_name(LPCTSTR sFontname, BYTE byCharSet = DEFAULT_CHARSET);
	void			set_font_size(int font_size);
	void			set_font_bold(int weight = FW_BOLD);
	LOGFONT			get_log_font() { return m_lf; }
	void			set_log_font(LOGFONT lf);

	void			load_history(CWinApp* app, CString section);
	void			save_history(CWinApp* app, CString section);

	//src내에 존재하는 콤보박스 아이템의 인덱스를 리턴.
	int				find_string(CString src);

//편집 관련
	void			edit_end(bool valid);
	LRESULT			on_message_CSCEdit(WPARAM wParam, LPARAM lParam);


protected:
//design
	//-1이면 폰트크기에 따라 자동 조정
	int				m_line_height = -1;

//폰트 관련
	LOGFONT			m_lf;
	CFont			m_font;
	int				m_font_size;
	void			reconstruct_font();

//편집 관련
	bool			m_use_edit = true;		//폴더 항목 이외의 공간 클릭시 수동 편집기능을 사용할 것인지
	CEdit*			m_pEdit = NULL;
	CString			m_old_text;				//편집되기 전의 원본 텍스트
	CRect			m_edit_margin;			//edit box 내부 여백(세로로 가운데 정렬되게 표시하기 위해)
	void			repos_edit();			//resize를 하면 여백이 리셋되므로 위치와 여백을 다시 계산

//즐겨찾기 관련
	CString			m_reg_section;		//load or save할 때 넘어온 section값을 기억해놓는다.

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
	afx_msg void OnCbnSelchange();
	afx_msg void OnCbnSelendok();
	afx_msg void OnCbnSelendcancel();
	afx_msg HBRUSH CtlColor(CDC* /*pDC*/, UINT /*nCtlColor*/);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};


