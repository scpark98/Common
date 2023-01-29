/*
[����]

[������ ����]
	- margin, gap�� ���� �ɼ� ó��

[������ ����]
* 20221119
	- ���ڿ� ���� Ÿ���� LPCTSTR -> CString���� ����.
	- �Ϻ� �Լ��� UNICODE, MultiByte ��忡 ���� �ٸ��� ó��.

[���� ����]
2019 09 11 09
����� ��Ƽ�������� ���� ��ũ�Ѹ� �����ȴ�.
�ִ� ������ ������ �� �ְ� ���� ��ũ���� �����ؾ� �Ѵ�.

- �ִ� ����� ���μ� ���� : set_max_line()
	hscroll�� ǥ�õ�

- �ִ� ����� �÷��� ���� : set_max_column()
	vscroll�� ǥ�õ�

- �̹������� �ػ󵵰� ��������� �ٸ� ��� �� ��� ũ�⸦ ����ؼ� ������� �׷����� �Ѵ�.
  �׷��� ������ ��������� ���� �̹����� �׺��� ū �̹������� ������� ũ�� �׷����� ��쵵 �����Ѵ�.
*/

#pragma once

#include <afxwin.h>
#include <sys/timeb.h>
#include <deque>
#include <algorithm>
#include "../ui/ExtWndShadow.h"
#include "../thread/thread_manager.h"

#define USE_OPENCV			false

#if USE_OPENCV
#include "../../Common/OpenCVFunctions.h"
#else
#include "../GdiPlusBitmap.h"
#endif


#define ADD_USING_THREAD	true

#define MESSAGE_THUMBCTRL	WM_USER + 0x7FFF - 0x7465
#define MAX_THREAD			100
#define MIN_TILE_SIZE		100
#define MAX_TILE_SIZE		400
#define MIN_GAP				10

class CThumbCtrlMsg
{
public:
	CThumbCtrlMsg(int _ctrl_id, int _msg, int _index)
		: ctrl_id(_ctrl_id), msg(_msg), index(_index) {}

	enum ThumbCtrlMsgs
	{
		message_thumb_insert = 0,
		message_thumb_loading_completed,
		message_thumb_reload,
		message_thumb_reload_selected,
		message_thumb_lbutton_selected,
		message_thumb_lbutton_unselected,
		message_thumb_lbutton_dbclicked,
		message_thumb_rename,
		message_thumb_keydown,
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
#if USE_OPENCV
	cv::Mat		img;
#else
	CGdiplusBitmap*	img = NULL;
#endif
	CString		title;
	bool		key_thumb;	//Thumbnail�� �߿��� T1�� ���� Ư�� thumbnail�� ����� ǥ�ø� ����.
	CString		info[4];	//info text ǥ�ÿ�. 0(lt info) ~ 3(rb info)
	CString		full_path;
	int			width;		//���� �̹����� ũ�� ����
	int			height;
	int			channel;
	CRect		rect;		//thumb+text�� ���� �׷��� ��ǥ���� �����صд�.
	int			thumb_bottom;//thumb image�� �ϴ� ��ǥ, Ÿ��Ʋ ������ ���
	int			line_index;	//���° ���ο� �ִ���, ���� ������ �׸��� Ŭ���ϸ� �� �׸��� �� ���̰� �ڵ� ��ũ�ѵǴµ� �̶� �ش� ���ο��� ���� height�� ���� �׸��� ������ �� �ʿ䰡 �ִ�.
	float		score = 0.0;
	float*		feature = NULL;

	CThumbImage()
	{
		img = NULL;
		key_thumb = false;
		feature = NULL;
	}
	~CThumbImage()
	{
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
		timer_scroll_bar_disappear,
	};

	enum CONTEXT_MENU
	{
		idTotalCount = WM_USER + 1234,
		idFind,
		idReload,
		idReloadSelected,
		idSortByTitle,
		idCopyToClipboard,
		idToggleIndex,
		idToggleTitle,
		idToggleResolution,
		idPromptMaxThumb,
		idDeleteThumb,
		idRemoveAll,
	};

	CThreadManager m_thread;


	std::deque<CThumbImage> m_dqThumb;
	int size() { return m_dqThumb.size(); }
	void release(int index);

#if USE_OPENCV
	cv::Mat			get_img(int index);
#else
	CGdiplusBitmap	get_img(int index);
#endif

	//������ ������ �Ѿ�� �� �ں���(?) �����ش�.
	void	set_max_count( int limit_count ) { m_max_thumbs = limit_count; }

//�÷� ����
	enum COLOR_THEME
	{
		color_theme_default = 0,
		//color_theme_blue,
		//color_theme_dark_blue,
		color_theme_dark_gray,
	};
	//�ε��� ���ڻ�
	COLORREF		m_crIndex;
	COLORREF		m_crIndexShadow;
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

	void			color_theme(int theme, bool invalidate = true);
	void			back_color(COLORREF cr) { m_crBack = cr; Invalidate(); }
	void			thumb_back_color(COLORREF cr) { m_crBackThumb = cr; Invalidate(); }
	void			text_color(COLORREF cr) { m_crTitle = cr; Invalidate(); }
	void			index_color(COLORREF cr) { m_crIndex = cr; Invalidate(); }
	void			index_shadow_color(COLORREF cr) { m_crIndexShadow = cr; Invalidate(); }
	void			resolution_color(COLORREF cr) { m_crResolution = cr; Invalidate(); }

//���� ����
	std::deque<int> m_selected;
	//bool			m_use_click_selection = true;
	//void			use_click_selection(bool click_selection) { m_use_click_selection = click_selection; }
	//bool			m_use_selection;		//default = true
	bool			m_use_multi_selection = false;	//default = false
	//m_use_multi_selection�� false�� CtrlŰ�� ������ ���� ������ �����ϴ�.
	//m_use_multi_selection�� �׳� ��Ŭ�������ε� ���� ������ �����ϰ� �� ���� ���� �÷�����.
	void			use_multi_selection(bool multi_select = true) { m_use_multi_selection = multi_select; }
	//���õ� ù��° �׸��� �ε����� ����.
	int				get_selected_item();
	//���õ� �׸���� dqSelected�� ��´�. dqSelected�� null�̸� �׳� ���� ������ ���Ϲ޾� ����Ѵ�.
	int				get_selected_items(std::deque<int>* dqSelected = NULL);

	bool			m_use_circle_number = false;
	void			use_circle_number(bool use) { m_use_circle_number = use; Invalidate(); }

	CGdiplusBitmap	m_img_selection_mark;
	void			set_selection_mark_image(CString image_path, int w = 0, int h = 0);
	void			set_selection_mark_image(CString sType, UINT id, int w = 0, int h = 0);

	int				get_index_from_point(CPoint pt);

	//index = -1 : ��ü����
	void			select_item(int index, bool select = true, bool make_ensure_visible = true);
	void			ensure_visible(int index);

//�˻� ����
	//show_inputbox�� true�̸� �Է¹ڽ��� ǥ�õǰ� text�� ���ŵȴ�. false�̸� text ������ �˻�.
	std::deque<int> find_text(bool show_inputbox, CString text, bool select);
	std::deque<int> find_text(bool show_inputbox, bool select);

//�߰�, ���� ����
	bool			is_loading_completed() { return m_loading_completed; }
	void			add_files(std::deque<CString> files, bool reset = true);
	int				insert(int index, uint8_t* pData, int width, int height, int format, CString sTitle = _T(""), bool bKeyThumb = false, bool bModifiedFlag = true );
	int				insert(int index, CString full_path, CString sTitle = _T("\0"), bool bKeyThumb = false, bool invalidate = true );
	//index == -1�̸� ��ü ����
	void			remove(int index, bool bRepaint = false);
	void			remove_selected(bool bRepaint = false);
	
	CSize			tile_size() { return m_szTile; }
	void			tile_size(CSize szTile) { m_szTile = szTile; }

	CSize			margin_size() { return m_szMargin; }
	void			margin_size(CSize szMargin) { m_szMargin = szMargin; }

	CSize			gap_size() { return m_szGap; }
	void			gap_size(CSize szGap) { m_szGap = szGap; }

	void			enlarge_size(bool enlarge);

//�ΰ� ���
	void			show_title(int show);
	bool			show_title() { return m_show_title; }

	void			show_resolution(int show);
	bool			show_resolution() { return m_show_resolution; }

	//show : 1(show), 0(hide), -1(toggle)
	void			show_index(int show);
	bool			show_index() { return m_show_index; }

	bool			isModified() { return m_modified; }

	bool			m_use_popup;
	void			use_popup_menu(bool use) { m_use_popup = use; }
	void			OnPopupMenu(UINT nID);

//���� �ؽ�Ʈ ǥ��
	bool			m_show_info_text[4];
	COLORREF		m_crInfoText[4];
	void			info_text(int thumb_index, int idx, CString sInfo, bool refresh );


//��Ʈ ����
	void			set_font_name(LPCTSTR font_name, BYTE char_set = DEFAULT_CHARSET );
	void			set_font_size(int nSize);
	void			set_font_bold(bool bBold = true);


//Ÿ��Ʋ ���� ����
	void			edit_begin(int index);
	void			edit_end(bool valid = true);
	bool			is_editing() { return m_in_editing; }
	CString			get_old_title() { return m_old_title; }
	void			set_title(int index, CString title);
	int				find_by_title(CString title, bool bWholeWord = false);

	void			on_menu_delete();

//���� ����
	void			sort_by_title();
	void			sort_by_info(int idx);	//info text�� �������� ����Ʈ�� ���Ľ�Ų��.
	void			sort_by_score();

protected:
	BOOL			RegisterWindowClass();

	bool			m_modified;

//�÷�
	//...


	int				m_index;		//���� ȭ�鿡 ���÷��̵Ǵ� ù ��° ������� ���� �ε���
									//���� ����


	bool			m_show_index;	//������� �ε��� ��ȣ�� ǥ������
	bool			m_show_title;	//����� �Ʒ� Ÿ��Ʋ ���ڿ��� ǥ������...
	bool			m_show_resolution;
	//int				m_title_height;	//Ÿ��Ʋ �ؽ�Ʈ ǥ�� ������ ����
	int				m_max_thumbs;	//�������� ��� �����ϸ� �޸𸮵� ��� �����ȴ�. ������ �־� �� ��쵵 �ִ�. -1�̸� ���Ѿ���
	bool			m_has_focus;

	//������� �÷��� Ÿ���� ũ��
	CSize			m_szTile;
	//������� ���� ����
	CSize			m_szMargin;
	//����� ����
	CSize			m_szGap;


//��ũ�� ��� ����
	int				m_scroll_pos;
	int				m_scroll_total;
	int				m_per_line;		//�� ���ο� ǥ�õǴ� ����� ����
	int				m_max_line;		//�� ǥ���ؾ� �� ���� ��
	std::deque<int> m_line_height;	//�� ���θ��� Ÿ��Ʋ�� ���̿� ���� ������ ���̰� �ٸ�.

	void			set_scroll_pos(int pos);
	void			recalculate_scroll_size();
	void			scroll_up(bool up);

	bool			m_scroll_drag;
	int				m_scroll_grip_size;
	CRect			m_rScroll;
	COLORREF		m_crScroll;
	double			m_scroll_trans;

//Ű���� �Է� ����
	void			on_key_down(int key);

//��Ʈ ����
	LOGFONT			m_lf;
	CFont			m_font;
	//int				m_nDefaultHeight;
	//void			UpdateSurface();
	void			ReconstructFont();

//���� ���� �ε� ����
	int				m_loading_index;
	std::deque<CString> m_loading_files;
	int				m_index_select_after_loading;
	long			m_clock_start;

	static void		loading_function(int idx, int start, int end);
	static void		loading_completed_callback();
	void			on_loading_completed();
	bool			m_loading_completed;

	void			draw_function(CDC* pDC, bool draw);

//Ÿ��Ʋ ���� ����
	CEdit*			m_pEdit;
	bool			m_in_editing;
	int				m_editing_index;
	CString			m_old_title;
	long			m_last_clicked;



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


