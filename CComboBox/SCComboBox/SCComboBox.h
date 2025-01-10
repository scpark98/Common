#pragma once

/*
- 리소스 속성에서 Has String = true, Owner Draw = Fixed로 변경.
*/

#include <afxwin.h>

// CColorComboBox

class CSCComboBox : public CComboBox
{
	DECLARE_DYNAMIC(CSCComboBox)

public:
	CSCComboBox();
	virtual ~CSCComboBox();

	//현재 입력된 텍스트를 읽어오고 항목에 존재하지 않으면 추가시킨다. 레지스트리에도 저장한다.
	int				add(CString text = _T(""));

	COLORREF		m_crText;
	COLORREF		m_crBack;
	COLORREF		m_crHighlightText;
	COLORREF		m_crHighlightTextBack;

	void			SetTextColor( COLORREF crText ) { m_crText = crText; RedrawWindow(); }
	void			SetBackColor( COLORREF crBack ) { m_crBack = crBack; RedrawWindow(); }
	void			SetHighlightTextColor( COLORREF cr ) { m_crHighlightText = cr; RedrawWindow(); }
	void			SetHighlightTextBackColor( COLORREF cr ) { m_crHighlightTextBack = cr; RedrawWindow(); }

	void			set_font_name(LPCTSTR sFontname, BYTE byCharSet = DEFAULT_CHARSET);
	//-1 : reduce, +1 : enlarge
	void			set_font_size(int font_size);
	//void	enlarge_font_size(bool enlarge);
	//void	set_font_bold(bool bold = true);
	//void	set_font_italic(bool italic = true);
	//LOGFONT	get_log_font() { return m_lf; }
	//void	set_log_font(LOGFONT lf);

	void			load_history(CWinApp* app, CString section);
	void			save_history(CWinApp* app, CString section);

	//src내에 존재하는 콤보박스 아이템의 인덱스를 리턴.
	int				find_string(CString src);

//편집 관련
	void			edit_end(bool valid);
	LRESULT			on_message_CSCEdit(WPARAM wParam, LPARAM lParam);


protected:

//폰트 관련
	LOGFONT			m_lf;
	CFont			m_font;
	int				m_font_size;
	void			reconstruct_font();

//편집 관련
	bool		m_use_edit = true;		//폴더 항목 이외의 공간 클릭시 수동 편집기능을 사용할 것인지
	CEdit*		m_pEdit = NULL;
	CString		m_old_text;				//편집되기 전의 원본 텍스트
	CRect		m_edit_margin;			//edit box 내부 여백(세로로 가운데 정렬되게 표시하기 위해)
	void		repos_edit();			//resize를 하면 여백이 리셋되므로 위치와 여백을 다시 계산

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


