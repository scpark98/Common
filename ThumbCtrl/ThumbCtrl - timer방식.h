//2019 09 11 09
//����� ��Ƽ�������� ���� ��ũ�Ѹ� �����ȴ�.
//�ִ� ������ ������ �� �ְ� ���� ��ũ���� �����ؾ� �Ѵ�.
//��Ƽ ���õ� �ʿ�

/* ���� ������ ��ũ�� ���� ����
- �ִ� ����� ���μ� ���� : set_max_line()
	hscroll�� ǥ�õ�

- �ִ� ����� �÷��� ���� : set_max_column()
	vscroll�� ǥ�õ�
*/

#pragma once

#include <sys/timeb.h>
#include <deque>
#include <algorithm>
#include "../OpenCVFunctions_4.30.h"

using namespace std;

#define MESSAGE_THUMBCTRL			WM_USER + 0x7FFF - 0x7465

class CThumbCtrlMsg
{
public:
	CThumbCtrlMsg(int ctrl_id1, int msg1, int index1)
	{
		ctrl_id = ctrl_id1;
		msg = msg1;
		index = index1;
	}

	enum ThumbCtrlMsgs
	{
		message_thumb_insert = 0,
		message_thumb_remove,
		message_thumb_remove_all,
		message_thumb_LButtonDblClk,
	};

	int		ctrl_id;
	int		msg;
	int		index;
};

//CThumbCtrl���� ����ϴ� CThumbImage ����ü�� �����̰�
//�� ��Ʈ���� �̿��ϴ� ������Ʈ���� ����ϴ� FACE_ID_DATA�� ���� ����ü�� ����� �������̴�.
//CThumbImage ����ü�� �����̹Ƿ� �̸� ����ϴ� ������Ʈ�� ����ü�ʹ� �翬�� �ٸ� �� �ִ�.
//CThumbImage ����ü�� ���� ���α׷��� ����ü�� �������̰� ������ ����.
//�ʿ信 ���� ���� ���α׷��� ����ü�� �����ϰ�
//�������� ���ǵ� CThumbImage ����ü�� ���缭 ����ؾ� �Ѵ�.

// CThumbCtrl
class CThumbImage 
{
public:
	cv::Mat		mat;
	CString		title;
	bool		key_thumb;	//Thumbnail�� �߿��� T1�� ���� Ư�� thumbnail�� ����� ǥ�ø� ����.
	CString		info[4];	//info text ǥ�ÿ�. 0(lt info) ~ 3(rb info)
	CString		full_path;
	int			width;		//���� �̹����� ũ�� ����
	int			height;
	int			channel;
	CRect		rect;		//thumb�� ���� �׷��� ��ǥ���� �����صд�.
	int			line_index;	//���° ���ο� �ִ���, ���� ������ �׸��� Ŭ���ϸ� �� �׸��� �� ���̰� �ڵ� ��ũ�ѵǴµ� �̶� �ش� ���ο��� ���� height�� ���� �׸��� ������ �� �ʿ䰡 �ִ�.

	CThumbImage()
	{
		key_thumb = false;
	}

	//����� �Ҹ��ڿ��� data��� �޸� �ּ��� ������ delete�ؼ��� �ȵȴ�.
	//�޸𸮸� �Ҵ�ް� �����͸� ��Ƽ� �̸� m_dqThumb�� �����ϴ� ����̹Ƿ�
	//local���� CThumbImage�� instance�� img�� �����Ͽ�
	//data��� ������ �޸𸮸� ���� �Ҵ�ް� �̸� push_back()�� ���¿���
	//���� img��� ���� ������ �ڵ� �Ҹ�Ǵ� ������ �Ǹ�
	//m_dqThumb�� ���� �����͸� ������ ���� �� ���� ������ ������ delete�Ǳ� �����̴�.
	//remove_all �Ǵ� ���α׷��� ����Ǵ� �������� �������� delete������� �Ѵ�.
};

class CThumbCtrl : public CWnd
{
	DECLARE_DYNAMIC(CThumbCtrl)

public:
	CThumbCtrl();
	virtual ~CThumbCtrl();

	enum CThumbCtrl_TIMER
	{
		timer_load_files = 0,
		timer_select_after_loading,
	};

	enum CONTEXT_MENU
	{
		idToggleIndex = 1,	//��Ȥ 0���� �����ϸ� �޴��� ������� �ʴ� ������ �ִ�. �޴� id�� 1���� �����ص� �����ϴ� �׻� 1���� ��������.
		idPromptMaxThumb,
		idDeleteThumb,
		idRemoveAll,
	};

	std::deque<CThumbImage>		m_dqThumb;
	void			sort_by_info( int idx );	//info text�� �������� ����Ʈ�� ���Ľ�Ų��.

	CSize			get_tile_size() { return m_szTile; }

	void			set_limit_count( int limit_count ) { m_limit_count = limit_count; }	//������ ������ �Ѿ�� �� �ں���(?) �����ش�.

//�÷� ����
	enum COLOR_THEME
	{
		color_theme_default = 0,
		//color_theme_blue,
		//color_theme_dark_blue,
		color_theme_dark_gray,
	};
	//Ÿ��Ʋ ���ڻ�
	COLORREF		m_crTitle;
	//�ػ� ���ڻ�
	COLORREF		m_crResolution;
	//�����׸� �簢��
	COLORREF		m_crSelected;
	//��Ʈ�� ��ü ����
	COLORREF		m_crBack;
	//����� Ÿ�� ����
	COLORREF		m_crBackThumb;

	void			set_color_theme(int theme, bool invalidate = true);
	void			set_back_color(COLORREF cr) { m_crBack = cr; Invalidate(); }
	void			set_thumb_back_color(COLORREF cr) { m_crBackThumb = cr; Invalidate(); }
	void			set_text_color(COLORREF cr) { m_crTitle = cr; Invalidate(); }
	void			set_resolution_color(COLORREF cr) { m_crResolution = cr; Invalidate(); }

//���� ����
	std::deque<int> m_selected;
	//start���� �� ó�� ���õ� �׸��� �ε����� �����Ѵ�.
	//int			get_selected_item(int start = 0);
	//�� ���������� ���õ� �� �׸��� �ε����� �����Ѵ�.
	//int			get_last_selected_item();
	//���õ� �׸���� dqSelected�� ��´�. dqSelected�� null�̸� �׳� ���� ������ ���Ϲ޾� ����Ѵ�.
	int				get_selected_item();
	int				get_selected_items(std::deque<int> *dqSelected = NULL);
	//index = -1 : ��ü����
	void			select_item(int index, bool select = true, bool make_ensure_visible = true);
	void			ensure_visible(int index);


	void			add(std::deque<CString> files, bool reset = true);
	int				insert(int index, uint8_t* pData, int width, int height, int channel, CString sTitle = _T(""), bool bKeyThumb = false, bool bModifiedFlag = true );
	int				insert(int index, cv::Mat mat, CString full_path, CString sTitle = _T("\0"), bool bKeyThumb = false, bool bModifiedFlag = true );
	void			remove(int nIndex, bool bRepaint = false );
	void			remove_all();
	int				get_thumb_count() { return m_dqThumb.size(); }

	int				get_min_width() { return m_min_width; }

	void			set_show_title(int show);
	bool			get_show_title() { return m_show_title; }

	void			set_show_resolution(int show);
	bool			get_show_resolution() { return m_show_resolution; }

	//show : 1(show), 0(hide), -1(toggle)
	void			set_show_index(int show);
	bool			get_show_index() { return m_show_index; }

	void			OnPopupMenu( UINT nID );

	bool			m_show_info_text[4];
	COLORREF		m_crInfoText[4];
	void			set_info_text(int thumb_index, int idx, CString sInfo, bool refresh );

	bool			isModified() { return m_modified; }

	//��Ʈ ����
	void			set_font_name(LPCTSTR font_name, BYTE char_set = DEFAULT_CHARSET );
	void			set_font_size(int nSize);
	void			set_font_bold(bool bBold = true);

protected:
	BOOL			RegisterWindowClass();

	bool			m_modified;

//�÷�
	//...


	int				m_index;		//���� ȭ�鿡 ���÷��̵Ǵ� ù ��° ������� ���� �ε���
									//���� ����


	bool			m_show_title;	//����� �Ʒ� Ÿ��Ʋ ���ڿ��� ǥ������...
	bool			m_show_resolution;
	bool			m_show_index;	//������� �ε��� ��ȣ�� ǥ������
	//int				m_title_height;	//Ÿ��Ʋ �ؽ�Ʈ ǥ�� ������ ����
	int				m_limit_count;	//�������� ��� �����ϸ� �޸𸮵� ��� �����ȴ�. ������ �־� �� ��쵵 �ִ�. -1�̸� ���Ѿ���
	bool			m_has_focus;

	CSize			m_szMargin;		//������� ���� ����
	CSize			m_szGap;		//����� ����

	//�ʱ� ������ 1�����θ� �����Ǿ��� ctrl�� height�� ���� ������� ũ�⸦ �ڵ� ����������
	//����� ũ��� ����ڰ� ������ �� �ְ� ��Ƽ�������� ��������.
	CSize			m_szTile;		//������� �÷��� Ÿ���� ũ��
	int				m_min_width;	//����� 1���� ǥ�õ� �ּ� width(margin*2 + tile.cx)

	//��ũ�� ��� ����
	bool			m_bUseMultiline;
	int				m_per_line;		//�� ���ο� ǥ�õǴ� ����� ����
	int				m_max_line;		//�� ǥ���ؾ� �� ���� ��
	std::deque<int> m_line_height;	//�� ���θ��� Ÿ��Ʋ�� ���̿� ���� ������ ���̰� �ٸ��� ������ �����̴�.

	void			RecalculateScrollSize();

	//selection
	bool			m_use_selection;		//default = true
	bool			m_use_multi_selection;	//default = false, if true, m_bUseSelection will be set to true
	std::deque<int> m_dqSelected;

	//��Ʈ ����
	LOGFONT			m_lf;
	CFont			m_font;
	//int				m_nDefaultHeight;
	//void			UpdateSurface();
	void			ReconstructFont();

//���� ���� �ε� ����
	int				m_loading_index;
	std::deque<CString> loading_files;
	int				m_index_select_after_loading;

	void			draw_function(CDC* pDC, bool draw);

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	virtual void PreSubclassWindow();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
//	afx_msg void OnMouseHWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	afx_msg void OnMouseHover(UINT nFlags, CPoint point);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnWindowPosChanging(WINDOWPOS* lpwndpos);
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};


