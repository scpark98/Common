#pragma once

/*
- set_path(folder); 또는 add_files()를 호출하면 파일들을 thumb로 만들어서 표시한다.
- m_thread_manager를 이용해서 병렬적으로 load되며 loading 도중에 앱이 종료되어도 안전하게 종료되도록 수정함.
*/

#include <afxdialogex.h>

#include <deque>
#include "../colors.h"
#include "../SCGdiplusBitmap.h"
#include "../thread/ThreadManager.h"
#include "../CEdit/SCEdit/SCEdit.h"


#define MIN_TILE_SIZE		40
#define MAX_TILE_SIZE		400
#define MIN_GAP				12

static const UINT Message_CSCThumbCtrl = ::RegisterWindowMessage(_T("MessageString_CSCThumbCtrl"));

class CSCThumbCtrlMessage
{
public:
	CSCThumbCtrlMessage(CWnd* _pThis, int _msg, int _index = 0)
		: pThis(_pThis), msg(_msg), index(_index)
	{
	}

	CWnd*	pThis;
	int		msg;
	int		index;
};

// CSCThumbCtrl 대화 상자

class CThumbImage
{
public:
	//CThumbImage(CThumbImage& _img)
	//{
	//	img.deep_copy(&(_img.img));
	//}

	//img를 동적변수로 선언하는 이유.
	//img는 그림파일마다 그 크기가 다양한데 이를 정적으로 잡는 것이 맞는가?
	//img.load()에서 m_pBitmap이 동적할당되는가?
	CSCGdiplusBitmap* img = NULL;
	bool		load_completed = false;
	CString		title;						//파일명 또는 지정된 타이틀
	bool		key_thumb = false;			//Thumbnail들 중에서 T1과 같은 특정 thumbnail일 경우의 표시를 위해.
	CString		info[4];					//info text 표시용. 0(lt info) ~ 3(rb info)
	CString		full_path;
	int			width = 0;					//원래 이미지의 크기 정보
	int			height = 0;
	int			channel = 0;
	CRect		r;							//이미지가 그려지는 thumb 영역이 아닌 thumb+title+여백까지 포함된 타일 영역
	int			thumb_bottom;				//thumb image의 하단 좌표, 타이틀 편집시 사용
	int			line_index;					//몇번째 라인에 있는지, 반쯤 가려진 항목을 클릭하면 그 항목이 다 보이게 자동 스크롤되는데 이때 해당 라인에서 가장 height가 높은 항목이 누군지 알 필요가 있다.
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
	//메모리를 할당받고 데이터를 담아서 이를 m_thumb에 저장하는 방식이므로
	//local에서 CThumbImage의 instance인 img를 선언하여
	//data라는 번지에 메모리를 동적 할당받고 이를 push_back()한 상태에서
	//위의 img라는 로컬 변수가 자동 소멸되는 시점이 되면
	//m_thumb는 시작 포인터만 가지고 있을 뿐 실제 데이터 영역은 delete되기 때문이다.
	//remove_all 또는 프로그램이 종료되는 시점에서 수동으로 delete시켜줘야 한다.
	void reload()
	{
		img->load(full_path);
	}
};

class CSCThumbCtrl : public CDialogEx
{
	DECLARE_DYNAMIC(CSCThumbCtrl)

public:
	CSCThumbCtrl();   // 표준 생성자입니다.
	virtual ~CSCThumbCtrl();

public:
	enum ENUM_SCThumbCtrlMessages
	{
		message_thumb_insert = 0,
		message_thumb_remove,
		message_thumb_remove_selected,
		message_thumb_loading_completed,
		message_thumb_reload,
		message_thumb_reload_selected,
		message_thumb_lbutton_selected,
		message_thumb_lbutton_unselected,
		message_thumb_lbutton_dbclicked,
		message_thumb_rename,
		message_thumb_keydown,
	};

	bool			create(CWnd* parent, int left = 0, int top = 0, int right = 0, int bottom = 0);
	void			set_path(CString path);

	void			add_files(std::deque<CString> files, bool reset = true);
	int				insert(int index, CString full_path, CString sTitle = _T("\0"), bool bKeyThumb = false, bool invalidate = true);

	//loading중에 앱을 종료시키면 m_thumb를 release하면서 충돌이 발생한다. 모든 thread를 정상 중지시킨 후 앱을 종료시켜야 한다.
	void			stop_loading();

	//index == -1이면 전체 삭제
	void			remove(int index, bool refresh = true);
	void			remove_selected(bool refresh = true);

	//m_thumb와 관련된 메모리만 release시킨다. -1이면 모든 thumb release.
	//m_files, m_selected는 필요한 경우 별도로 clear 시켜야 한다.
	void			release_thumb(int index);

	std::deque<CString> m_files;
	std::deque<CThumbImage> m_thumb;
	int				size() { return m_thumb.size(); }
	CSCGdiplusBitmap*	get_img(int index);


	bool			is_loading_completed() { return m_loading_completed; }
	long			get_loading_elapsed() { return (m_tloading_end - m_tloading_start); }

//크기 조정
	CSize			get_thumb_size() { return m_sz_thumb; }
	void			set_thumb_size(CSize sz_thumb);

	CSize			get_margin_size() { return m_sz_margin; }
	void			set_margin_size(CSize szMargin) { m_sz_margin = szMargin; recalc_tile_rect(); }

	CSize			get_gap_size() { return m_sz_gap; }
	void			set_gap_size(CSize szGap) { m_sz_gap = szGap; recalc_tile_rect(); }

	void			enlarge_size(bool enlarge);

//선택 관련
	std::deque<int> m_selected;
	bool			m_use_multi_selection = false;	//default = false
	//m_use_multi_selection이 false라도 Ctrl키를 누르면 다중 선택이 가능하다.
	//m_use_multi_selection는 그냥 좌클릭만으로도 다중 선택이 가능하게 할 지에 대한 플래그임.
	void			use_multi_selection(bool multi_select = true) { m_use_multi_selection = multi_select; }
	//선택된 첫번째 항목의 인덱스를 리턴.
	int				get_selected_item();
	//선택된 항목들은 이미 m_selected에 저장되어 있으므로 이를 dqSelected로 복사한다.
	//dqSelected가 null이면 그냥 선택 갯수를 리턴한다.
	int				get_selected_items(std::deque<int>* dqSelected = NULL);

	bool			m_use_circle_number = false;
	void			use_circle_number(bool use) { m_use_circle_number = use; Invalidate(); }

	CSCGdiplusBitmap	m_img_selection_mark;
	void			set_selection_mark_image(CString image_path, int w = 0, int h = 0);
	void			set_selection_mark_image(CString sType, UINT id, int w = 0, int h = 0);

	int				get_index_from_point(CPoint pt);
	void			on_key_down(int key);


	//index = -1 : 전체선택
	void			select_item(int index, bool select = true, bool make_ensure_visible = true);
	void			select_item(CString fullpath);
	void			ensure_visible(int index);

//검색 관련
	//show_inputbox가 true이면 입력박스가 표시되고 text는 갱신된다. false이면 text 값으로 검색.
	std::deque<int> find_text(bool show_inputbox, CString text, bool select);
	std::deque<int> find_text(bool show_inputbox, bool select);

//타이틀 편집 관련
	void			edit_begin(int index);
	void			edit_end(bool valid = true);
	bool			is_editing() { return m_in_editing; }
	CString			get_old_title() { return m_old_title; }
	void			set_title(int index, CString title);
	int				find_by_title(CString title, bool bWholeWord = false);
	LRESULT			on_message_CSCEdit(WPARAM wParam, LPARAM lParam);

//정렬 관련
	void			sort_by_title();
	void			sort_by_info(int idx);	//info text를 기준으로 리스트를 정렬시킨다.
	void			sort_by_score();

//옵션
	bool			get_show_file_extension() { return m_show_extension; }
	void			set_show_file_extension(bool show) { m_show_extension = show; Invalidate(); }


//color theme
	void			set_color_theme(int theme);

//정보 텍스트 표시
	bool			m_show_info_text[4];
	COLORREF		m_crInfoText[4];
	void			set_info_text(int thumb_index, int idx, CString sInfo, bool refresh);

//폰트 관련
	void			set_font_name(LPCTSTR font_name, BYTE char_set = DEFAULT_CHARSET);
	void			set_font_size(int nSize);
	void			set_font_bold(bool bBold = true);

protected:
	CWnd*			m_parent = NULL;

	//thumb 크기를 기준으로 tile 크기를 계산해야 한다. 그래야 이미지를 로드할 때 thumb 크기 비율로 resize 할 수 있다.
	CSize			m_sz_thumb = CSize(100, 120);

	//썸네일이 올려질 타일의 크기(타일 = 내부마진 + 썸네일크기 + 타이틀표시영역)
	CSize			m_sz_tile;	//m_sz_thumb의 크기에 따라 자동 계산되므로 직접 입력하지 말것!

	//타일과 썸네일의 상하좌우 여백
	CRect			m_r_inner = CRect(16, 8, 16, 8);

	int				m_title_height = 14;
	int				m_thumb_title_gap = 4;	//thumb와 title 사이 갭
	CRect			m_r_title;				//title 표시 영역. 자동 계산

	//이 컨트롤과 썸네일들이 표시되는 영역의 상하좌우 여백
	CSize			m_sz_margin = CSize(12, 12);

	//썸네일 사이의 간격
	CSize			m_sz_gap = CSize(12, 12);

	//계속 thumb를 추가하는 형태로 사용하는 앱에서 thumb가 계속 증가하면 메모리도 계속 증가된다.
	//제한을 둬야 할 경우도 있다. -1이면 제한없음.
	//m_max_thumbs를 초과할 경우 추가 불가? 순환?
	int				m_max_thumbs = -1;


	//타일 크기, 마진, 간격, 스크롤, 컨트롤 크기조정 등에 따라 각 썸네일이 표시되는 r이 재계산된다.
	//이러한 변화에 대해 재계산하고 Invalidate()까지 수행하므로 OnPaint()에서는 r에 표시만 하면 된다.
	void			recalc_tile_rect();

//옵션
	bool			m_show_index = true;		//썸네일의 인덱스 번호를 표시할지
	bool			m_show_title = true;		//썸네일 아래 타이틀 문자열을 표시할지...
	bool			m_show_resolution = false;
	bool			m_show_extension = true;

//Context Menu
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
		idProperty,
	};
	bool			m_use_context_menu;
	void			use_context_menu(bool use) { m_use_context_menu = use; }
	void			on_context_menu(UINT nID);

	void			on_menu_delete();


//스크롤 기능 관련
	int				m_scroll_pos = 0;
	int				m_scroll_total = 0;
	int				m_per_line = 0;				//한 라인에 표시되는 썸네일 개수
	int				m_max_line = 0;				//총 표시해야 될 라인 수
	//std::deque<int> m_line_height;			//각 라인마다 타이틀의 길이에 따라 라인의 높이가 다름.

	void			set_scroll_pos(int pos);
	//offset = 0일 경우는 (tile+gap)/4 크기만큼 scroll시킨다.
	void			scroll_up(bool up, int offset = 0);

	//직접 그린 scrollbar 제어
	bool			m_scrollbar_drag;
	int				m_scrollbar_grip_size = 40;
	CRect			m_r_scrollbar;
	Gdiplus::Color	m_cr_scrollbar;
	double			m_scrollbar_trans;

//drag에 의한 scroll
	bool			m_lbutton_down = false;
	CPoint			m_pt_old;

//로딩 관련
	bool			m_loading_completed = false;
	int				m_loading_completed_count = 0;
	CThreadManager	m_thread;
	long			m_tloading_start = 0;
	long			m_tloading_end = 0;
	static void		loading_function(int idx, int start, int end);
	static void		loading_completed_callback();
	void			on_loading_completed();

	bool			m_stop_loading = false;			//loading thread를 강제로 종료시킬 경우(loading중에 앱을 종료시키는 경우)

//폰트 관련
	LOGFONT			m_lf;
	CFont			m_font;
	void			reconstruct_font();

//타이틀 편집 관련
	CSCEdit*		m_pEdit = NULL;
	bool			m_in_editing = false;
	int				m_editing_index;
	CString			m_old_title;
	long			m_last_clicked;

//color theme
	CSCColorTheme	m_theme = CSCColorTheme(this);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	virtual void PreSubclassWindow();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
};
