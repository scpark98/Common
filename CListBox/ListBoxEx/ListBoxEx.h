#pragma once

/*
* CStatic�� ��ӹ޾� ��������� �ܼ� �׸� ǥ�ó� ���콺 �׼� ���� ���� ������
* COutputStatic�� ���� ���� Ű���� ó���� �ȵǰ� ��ũ�� ��
* �ٺ����� ������ ���� ������ �뵵�θ� ��� �����ϴ�.
*/

#include "afxwin.h"

#include <deque>
#include "../../Functions.h"

#define list_unused_color	RGB(17,235,53)

class CListBoxData
{
public:
	CListBoxData(CString _text, COLORREF _crText = list_unused_color, COLORREF _crBack = list_unused_color)
	{
		text = _text;
		crText = _crText;
		crBack = _crBack;
	}

	CString text;
	CString alt_text;
	COLORREF crText;
	COLORREF crBack;
	DWORD dwData;
};

class CListBoxEx : public CStatic
{
public:
	std::deque<CListBoxData>	m_list;

//�߰�, ����
	int				add_string(CString text, COLORREF crText = GetSysColor(COLOR_WINDOWTEXT), COLORREF crBack = GetSysColor(COLOR_WINDOW));
	void			set_alt_text(int index, CString alt_text);
	int				size() { return m_list.size(); }
	void			delete_all_items();

//���� ����
	int				get_selected() { return m_selected; }
	void			select(int index) { m_selected = index; Invalidate(); }
	int				get_highlighted() { return m_highlighted; }
	void			set_highlighted(int index);

//�ؽ�Ʈ �� ������
	CString			get_text(int index, bool alt_text = false);
	int				find_string(CString text, bool compare_alt_text = false);
	DWORD			get_data(int index);
	void			set_data(int index, DWORD dwData);



//�÷� ����
	enum listctrlex_color_theme
	{
		color_theme_default = 0,
		color_theme_light_blue,
		color_theme_navy_blue,
		color_theme_dark_blue,
		color_theme_dark_gray,
	};
	void			set_color_theme(int theme, bool apply_now = true);
	void			set_back_color(COLORREF cr) { m_crBack = cr; Invalidate(); }
	COLORREF		get_text_color(int item);
	COLORREF		get_back_color(int item);
	void			set_text_color(int item, COLORREF crText);		//Ư�� �׸��� ���ڻ� ����
	void			set_back_color(int item, COLORREF crBack);		//Ư�� �׸��� ���� ����
	void			set_item_color(int item, COLORREF crText, COLORREF crBack);
	void			set_default_item_color(COLORREF crText, COLORREF crBack);	//�⺻ ���ڻ�, ������ �����Ѵ�.

	//��Ʈ ����
	int				get_font_size();
	void			set_font_name(LPCTSTR sFontname, BYTE byCharSet = DEFAULT_CHARSET);
	void			set_font_size(int font_size);
	void			enlarge_font_size(bool enlarge);
	void			set_font_bold(bool bold = true);
	void			set_font_italic(bool italic = true);
	LOGFONT			get_log_font() { return m_lf; }
	void			set_log_font(LOGFONT lf);


protected:
//�÷� ����
	COLORREF		m_crText;					//�⺻ ���ڻ�
	COLORREF		m_crTextHover;
	COLORREF		m_crTextSelected;			//���� �׸��� Ȱ��ȭ(active) ���ڻ�
	COLORREF		m_crTextSelectedInactive;	//���� �׸��� ��Ȱ��ȭ(inactive) ���ڻ�
	COLORREF		m_crTextHighlighted;
	COLORREF		m_crBack;					//�⺻ ����
	COLORREF		m_crBackHover;
	COLORREF		m_crBackSelected;
	COLORREF		m_crBackSelectedInactive;

//��Ʈ ����
	LOGFONT			m_lf;
	CFont			m_font;
	int				m_font_size;
	void			reconstruct_font();

	int				m_index;					//���� ȭ�鿡 ǥ���ϴ� ù��° ������ �ε���

//���� ����
	int				m_highlighted;
	int				m_selected;
	bool			m_use_hover;				//default = false;
	int				m_hover;
	void			scroll(bool up, int interval = 1);

//��Ÿ
	CPoint			m_margin;
	int				m_line_height;

public:
	CListBoxEx();
	~CListBoxEx();
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void PreSubclassWindow();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	DECLARE_MESSAGE_MAP()
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
};

