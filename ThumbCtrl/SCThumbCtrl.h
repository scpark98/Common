#pragma once

#include <afxdialogex.h>

#include <deque>
#include "../GdiplusBitmap.h"
#include "../thread/thread_manager.h"


#define MIN_TILE_SIZE		100
#define MAX_TILE_SIZE		400
#define MIN_GAP				20

static const UINT Message_CSCThumbCtrl = ::RegisterWindowMessage(_T("MessageString_CSCThumbCtrl"));

class CSCThumbCtrlMsg
{
public:
	CSCThumbCtrlMsg(int _ctrl_id, int _msg, int _index)
		: ctrl_id(_ctrl_id), msg(_msg), index(_index) {
	}

	enum ENUM_SCThumbCtrlMsgs
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

// CSCThumbCtrl 대화 상자

class CThumbImage
{
public:
	CGdiplusBitmap img;
	CString		title;
	bool		key_thumb;		//Thumbnail들 중에서 T1과 같은 특정 thumbnail일 경우의 표시를 위해.
	CString		info[4];		//info text 표시용. 0(lt info) ~ 3(rb info)
	CString		full_path;
	int			width;			//원래 이미지의 크기 정보
	int			height;
	int			channel;
	CRect		r;				//thumb+text가 실제 그려진 좌표값을 저장해둔다.
	int			thumb_bottom;	//thumb image의 하단 좌표, 타이틀 편집시 사용
	int			line_index;		//몇번째 라인에 있는지, 반쯤 가려진 항목을 클릭하면 그 항목이 다 보이게 자동 스크롤되는데 이때 해당 라인에서 가장 height가 높은 항목이 누군지 알 필요가 있다.
	float		score = 0.0;
	float*		feature = NULL;
};

class CSCThumbCtrl : public CDialogEx
{
	DECLARE_DYNAMIC(CSCThumbCtrl)

public:
	CSCThumbCtrl();   // 표준 생성자입니다.
	virtual ~CSCThumbCtrl();

public:
	bool			create(CWnd* parent, int left = 0, int top = 0, int right = 0, int bottom = 0);
	void			set_path(CString path);

	void			add_files(std::deque<CString> files, bool reset = true);
	int				insert(int index, CString full_path, CString sTitle = _T("\0"), bool bKeyThumb = false, bool invalidate = true);

	std::deque<CString> m_files;

	std::deque<CThumbImage> m_thumb;
	int				size() { return m_thumb.size(); }
	CGdiplusBitmap	get_img(int index);

//선택 관련
	std::deque<int> m_selected;
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

protected:
	CWnd*			m_parent = NULL;

	//썸네일이 올려질 타일의 크기
	CSize			m_szTile = CSize(128, 128);
	//썸네일의 시작 여백
	CSize			m_szMargin = CSize(20, 20);;
	//썸네일 간격
	CSize			m_szGap = CSize(20, 20);;


//스크롤 기능 관련
	int				m_scroll_pos = 0;
	int				m_scroll_total = 0;
	int				m_per_line;		//한 라인에 표시되는 썸네일 개수
	int				m_max_line;		//총 표시해야 될 라인 수
	std::deque<int> m_line_height;	//각 라인마다 타이틀의 길이에 따라 라인의 높이가 다름.

	void			set_scroll_pos(int pos);
	void			recalculate_scroll_size();
	void			scroll_up(bool up);

	bool			m_scroll_drag;
	int				m_scroll_grip_size = 40;
	CRect			m_rScroll;
	Gdiplus::Color	m_cr_scroll;
	double			m_scroll_trans;


	//로딩 관련
	bool			m_loading_completed = false;
	CThreadManager	m_thread;
	static void		loading_function(int idx, int start, int end);
	static void		loading_completed_callback();
	void			on_loading_completed();


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
};
