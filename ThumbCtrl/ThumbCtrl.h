/*
[사용법]

[수정될 내용]
	- margin, gap에 대한 옵션 처리

[수정된 내용]
* 20221119
	- 문자열 인자 타입을 LPCTSTR -> CString으로 통일.
	- 일부 함수를 UNICODE, MultiByte 모드에 따라 다르게 처리.

[구현 예정]
2019 09 11 09
현재는 멀티라인으로 세로 스크롤만 지원된다.
최대 라인을 선택할 수 있고 가로 스크롤을 지원해야 한다.

- 최대 썸네일 라인수 설정 : set_max_line()
	hscroll만 표시됨

- 최대 썸네일 컬럼수 설정 : set_max_column()
	vscroll만 표시됨

- 이미지들의 해상도가 상대적으로 다를 경우 그 상대 크기를 고려해서 썸네일이 그려져야 한다.
  그렇지 않으면 상대적으로 작은 이미지가 그보다 큰 이미지보다 썸네일이 크게 그려지는 경우도 존재한다.
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

//CThumbCtrl에서 사용하는 CThumbImage 구조체는 범용이고
//이 컨트롤을 이용하는 프로젝트에서 사용하는 FACE_ID_DATA와 같은 구조체는 사용자 정의형이다.
//CThumbImage 구조체는 범용이므로 이를 사용하는 프로젝트의 구조체와는 당연히 다를 수 있다.
//CThumbImage 구조체를 응용 프로그램의 구조체에 의존적이게 만들지 말것.
//필요에 따라서 응용 프로그램의 구조체를 정의하고
//범용으로 정의된 CThumbImage 구조체에 맞춰서 사용해야 한다.

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
	bool		key_thumb;	//Thumbnail들 중에서 T1과 같은 특정 thumbnail일 경우의 표시를 위해.
	CString		info[4];	//info text 표시용. 0(lt info) ~ 3(rb info)
	CString		full_path;
	int			width;		//원래 이미지의 크기 정보
	int			height;
	int			channel;
	CRect		rect;		//thumb+text가 실제 그려진 좌표값을 저장해둔다.
	int			thumb_bottom;//thumb image의 하단 좌표, 타이틀 편집시 사용
	int			line_index;	//몇번째 라인에 있는지, 반쯤 가려진 항목을 클릭하면 그 항목이 다 보이게 자동 스크롤되는데 이때 해당 라인에서 가장 height가 높은 항목이 누군지 알 필요가 있다.
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

	//절대로 소멸자에서 data라는 메모리 주소의 공간을 delete해서는 안된다.
	//메모리를 할당받고 데이터를 담아서 이를 m_dqThumb에 저장하는 방식이므로
	//local에서 CThumbImage의 instance인 img를 선언하여
	//data라는 번지에 메모리를 동적 할당받고 이를 push_back()한 상태에서
	//위의 img라는 로컬 변수가 자동 소멸되는 시점이 되면
	//m_dqThumb는 시작 포인터만 가지고 있을 뿐 실제 데이터 영역은 delete되기 때문이다.
	//remove_all 또는 프로그램이 종료되는 시점에서 수동으로 delete시켜줘야 한다.
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

	//정해진 개수를 넘어가면 맨 뒤부터(?) 지워준다.
	void	set_max_count( int limit_count ) { m_max_thumbs = limit_count; }

//컬러 관련
	enum COLOR_THEME
	{
		color_theme_default = 0,
		//color_theme_blue,
		//color_theme_dark_blue,
		color_theme_dark_gray,
	};
	//인덱스 글자색
	COLORREF		m_crIndex;
	COLORREF		m_crIndexShadow;
	//타이틀 글자색
	COLORREF		m_crTitle;
	//해상도 글자색
	COLORREF		m_crResolution;
	//선택항목 사각형
	COLORREF		m_crSelected;
	//컨트롤 전체 배경색
	COLORREF		m_crBack;
	//썸네일 타일 배경색
	COLORREF		m_crBackThumb;

	void			color_theme(int theme, bool invalidate = true);
	void			back_color(COLORREF cr) { m_crBack = cr; Invalidate(); }
	void			thumb_back_color(COLORREF cr) { m_crBackThumb = cr; Invalidate(); }
	void			text_color(COLORREF cr) { m_crTitle = cr; Invalidate(); }
	void			index_color(COLORREF cr) { m_crIndex = cr; Invalidate(); }
	void			index_shadow_color(COLORREF cr) { m_crIndexShadow = cr; Invalidate(); }
	void			resolution_color(COLORREF cr) { m_crResolution = cr; Invalidate(); }

//선택 관련
	std::deque<int> m_selected;
	//bool			m_use_click_selection = true;
	//void			use_click_selection(bool click_selection) { m_use_click_selection = click_selection; }
	//bool			m_use_selection;		//default = true
	bool			m_use_multi_selection = false;	//default = false
	//m_use_multi_selection이 false라도 Ctrl키를 누르면 다중 선택이 가능하다.
	//m_use_multi_selection는 그냥 좌클릭만으로도 다중 선택이 가능하게 할 지에 대한 플래그임.
	void			use_multi_selection(bool multi_select = true) { m_use_multi_selection = multi_select; }
	//선택된 첫번째 항목의 인덱스를 리턴.
	int				get_selected_item();
	//선택된 항목들을 dqSelected에 담는다. dqSelected가 null이면 그냥 선택 갯수를 리턴받아 사용한다.
	int				get_selected_items(std::deque<int>* dqSelected = NULL);

	bool			m_use_circle_number = false;
	void			use_circle_number(bool use) { m_use_circle_number = use; Invalidate(); }

	CGdiplusBitmap	m_img_selection_mark;
	void			set_selection_mark_image(CString image_path, int w = 0, int h = 0);
	void			set_selection_mark_image(CString sType, UINT id, int w = 0, int h = 0);

	int				get_index_from_point(CPoint pt);

	//index = -1 : 전체선택
	void			select_item(int index, bool select = true, bool make_ensure_visible = true);
	void			ensure_visible(int index);

//검색 관련
	//show_inputbox가 true이면 입력박스가 표시되고 text는 갱신된다. false이면 text 값으로 검색.
	std::deque<int> find_text(bool show_inputbox, CString text, bool select);
	std::deque<int> find_text(bool show_inputbox, bool select);

//추가, 삭제 관련
	bool			is_loading_completed() { return m_loading_completed; }
	void			add_files(std::deque<CString> files, bool reset = true);
	int				insert(int index, uint8_t* pData, int width, int height, int format, CString sTitle = _T(""), bool bKeyThumb = false, bool bModifiedFlag = true );
	int				insert(int index, CString full_path, CString sTitle = _T("\0"), bool bKeyThumb = false, bool invalidate = true );
	//index == -1이면 전체 삭제
	void			remove(int index, bool bRepaint = false);
	void			remove_selected(bool bRepaint = false);
	
	CSize			tile_size() { return m_szTile; }
	void			tile_size(CSize szTile) { m_szTile = szTile; }

	CSize			margin_size() { return m_szMargin; }
	void			margin_size(CSize szMargin) { m_szMargin = szMargin; }

	CSize			gap_size() { return m_szGap; }
	void			gap_size(CSize szGap) { m_szGap = szGap; }

	void			enlarge_size(bool enlarge);

//부가 기능
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

//정보 텍스트 표시
	bool			m_show_info_text[4];
	COLORREF		m_crInfoText[4];
	void			info_text(int thumb_index, int idx, CString sInfo, bool refresh );


//폰트 관련
	void			set_font_name(LPCTSTR font_name, BYTE char_set = DEFAULT_CHARSET );
	void			set_font_size(int nSize);
	void			set_font_bold(bool bBold = true);


//타이틀 편집 관련
	void			edit_begin(int index);
	void			edit_end(bool valid = true);
	bool			is_editing() { return m_in_editing; }
	CString			get_old_title() { return m_old_title; }
	void			set_title(int index, CString title);
	int				find_by_title(CString title, bool bWholeWord = false);

	void			on_menu_delete();

//정렬 관련
	void			sort_by_title();
	void			sort_by_info(int idx);	//info text를 기준으로 리스트를 정렬시킨다.
	void			sort_by_score();

protected:
	BOOL			RegisterWindowClass();

	bool			m_modified;

//컬러
	//...


	int				m_index;		//현재 화면에 디스플레이되는 첫 번째 썸네일의 실제 인덱스
									//선택 관련


	bool			m_show_index;	//썸네일의 인덱스 번호를 표시할지
	bool			m_show_title;	//썸네일 아래 타이틀 문자열을 표시할지...
	bool			m_show_resolution;
	//int				m_title_height;	//타이틀 텍스트 표시 영역의 높이
	int				m_max_thumbs;	//아이템이 계속 증가하면 메모리도 계속 증가된다. 제한을 둬야 할 경우도 있다. -1이면 제한없음
	bool			m_has_focus;

	//썸네일이 올려질 타일의 크기
	CSize			m_szTile;
	//썸네일의 시작 여백
	CSize			m_szMargin;
	//썸네일 간격
	CSize			m_szGap;


//스크롤 기능 관련
	int				m_scroll_pos;
	int				m_scroll_total;
	int				m_per_line;		//한 라인에 표시되는 썸네일 개수
	int				m_max_line;		//총 표시해야 될 라인 수
	std::deque<int> m_line_height;	//각 라인마다 타이틀의 길이에 따라 라인의 높이가 다름.

	void			set_scroll_pos(int pos);
	void			recalculate_scroll_size();
	void			scroll_up(bool up);

	bool			m_scroll_drag;
	int				m_scroll_grip_size;
	CRect			m_rScroll;
	COLORREF		m_crScroll;
	double			m_scroll_trans;

//키보드 입력 관련
	void			on_key_down(int key);

//폰트 관련
	LOGFONT			m_lf;
	CFont			m_font;
	//int				m_nDefaultHeight;
	//void			UpdateSurface();
	void			ReconstructFont();

//다중 파일 로딩 관련
	int				m_loading_index;
	std::deque<CString> m_loading_files;
	int				m_index_select_after_loading;
	long			m_clock_start;

	static void		loading_function(int idx, int start, int end);
	static void		loading_completed_callback();
	void			on_loading_completed();
	bool			m_loading_completed;

	void			draw_function(CDC* pDC, bool draw);

//타이틀 변경 관련
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


