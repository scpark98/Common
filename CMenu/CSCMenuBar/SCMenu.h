#pragma once

/*
* menu를 sub popup menu까지 구현하려면 tree 형태의 자료구조가 필요하다.
* 우선 1레벨의 메뉴 형태만 고려하자.
*/

#include <afxwin.h>
#include <afxdialogex.h>
#include <deque>
#include "../../CButton/GdiButton/GdiButton.h"
#include "../../colors.h"

static const UINT Message_CSCMenu = ::RegisterWindowMessage(_T("MessageString_CSCMenu"));

class CSCMenuItem;
class CSCMenu;

class CSCMenuMessage
{
public:
	CSCMenuMessage(CWnd* _this, int _message, CSCMenuItem* _menu_item, int _button_index = -1, int _button_state = 0);

	CWnd*		m_pWnd = NULL;
	int			m_message;
	CSCMenuItem	*m_menu_item;
	int			m_button_index;
	int			m_button_state;
};

//한 메뉴 항목내에 선택 옵션 표시용 SubButton
//서브버튼이 하나라면 check로 동작하고 2개 이상이라면 radio로 동작하므로
//subButton들까지 굳이 CGdiButton으로 구현하진 않음.
class CSCMenuSubButton
{
public:
	//icon button — _id 가 PNG resource id. 이미지 로드 후 gray(unselect) / adjust_hsl(selected) 처리.
	CSCMenuSubButton(UINT _id, int menu_height);
	//text button — m_text 비어있지 않으면 paint 가 text 그림 (image 미사용).
	//width 는 menu_height * 1.6 fixed (짧은 라벨 "저장" 등). id 는 click 매핑용.
	CSCMenuSubButton(UINT _id, CString _text, int menu_height);
	~CSCMenuSubButton();

	int		m_id = 0;	//click 시 사용자 코드의 매핑 용도 (PNG resource id 또는 text button id).
	CString	m_text;		//text button 모드 — 비어있으면 image 사용.
	CSCGdiplusBitmap* m_button_image[2] = { NULL, };	//0:unselect, 1:select, (2:over, 3:down 추가 예정??)
	CRect	m_r = CRect(0, 0, 0, 0);
	//radio로 한다면 group 속성까지도 고려해야하지만 그렇게까지 메뉴를 복잡하게 구성하진 말자.
	//정 필요하다면 메뉴 항목을 나누자.
	//BS_CHECKBOX 추가 필요성도 검토할 필요는 있다.
	int		m_style = BS_RADIOBUTTON;
	int		m_state = 0;		//0:unchecked, 1:checked
};

class CSCMenuItem
{
public:
	enum item_type
	{
		item_normal,
		item_thumbnail,
		item_submenu,
	};

	CSCMenuItem(int id, CString caption, UINT icon_id = 0, CString hot_key = _T(""), int menu_height = -1);
	~CSCMenuItem();

	//하나의 메뉴 아이템을 추가하면서 resource의 png로 등록된 sub button들을 추가할 수 있다.
	//m_menu.add(control_start, _T("원격제어 시작하기"), IDB_CONTROL_START, _T(""), IDB_MENU_CHECK);
	//m_menu.add(control_mode, _T("제어모드"), IDB_CONTROL_MODE, _T(""), IDB_MENU_DRIVE_MODE, IDB_MENU_GDI_MODE);
	template <typename ... Types> CSCMenuItem(int _id, CString caption, UINT icon_id = 0, CString hot_key = _T(""), int menu_height = -1, Types... args)
	{
		m_id = _id;
		m_caption = _caption;
		m_hot_key = _hot_key;

		set_icon(icon_id);

		int n = sizeof...(args);
		int arg[] = { args... };

		for (auto id : arg)
		{
			CSCMenuSubButton* btn = new CSCMenuSubButton(id);
			m_buttons.push_back(btn);
		}
	}

	int				m_id;	//0 이하는 separator. Resouce의 Menu는 separator를 0으로 취급한다.
	bool			m_is_separator = false;
	CRect			m_r = CRect(0, 0, 0, 0);
	CString			m_caption;
	CString			m_hot_key;
	//caption 의 access-key 마커 ('&X') 가 가리키는 글자의 *cleaned m_caption 안의 인덱스*. 없으면 -1.
	//ctor 에서 caption 파싱: '&&' → '&' literal / '&X' → X 추출 후 X 의 위치 저장. OnPaint 가 이 위치 글자
	//아래에 underline 을 수동으로 그린다. DrawText 는 DT_NOPREFIX 로 호출되어 자동 underline 안 그림.
	int				m_access_key = -1;
	int				m_menu_height;

	//CMenu::EnableMenuItem / CheckMenuItem 동등. CSCMenu::enable_item / check_item / check_radio_item 으로 설정.
	bool			m_enabled = true;
	bool			m_checked = false;

	//icon 이미지는 투명 png로 제작하고 m_line_height의 높이와 동일한 정사각형 크기로 제작해야 한다.
	//그 크기가 32x32라면 실제 이미지는 그 안에 작게 표시될 정도로 그려진 이미지이어야 한다.
	//32x32 크기에 꽉차게 그려진 이미지를 사용해서는 안된다.
	CSCGdiplusBitmap	m_icon;
	std::deque<CSCMenuSubButton*> m_buttons;

	//1단계: 데이터 슬롯만. layout/paint 분기는 2단계, sub-popup 동작은 3단계.
	item_type			m_type = item_normal;
	CSCGdiplusBitmap	m_thumbnail;
	CString				m_sub_caption;
	CSCMenu*			m_sub_menu = nullptr;

	void			set_icon(UINT icon_id);
	void			add_button(UINT button_id, bool reset = false);
	//text button — PNG resource 없이 라벨 표시 (예: "저장"). click 시 button_id 로 매핑.
	void			add_text_button(UINT button_id, CString text, bool reset = false);

	//버튼이 1개인 경우는 check <-> uncheck toggle이고
	//2개 이상일 경우는 radio로 동작한다.
	void			set_check(int idx, bool _check = true);

	int				get_check(int idx);
	int				get_checked_button_index();
};


// CSCMenu

class CSCMenu : public CDialogEx
{
	DECLARE_DYNAMIC(CSCMenu)

public:
	CSCMenu();
	~CSCMenu();

	enum MESSAGES
	{
		message_scmenu_selchanged = 0,
		message_scmenu_button_state_changed,
		message_scmenu_hide,
	};

	bool			create(CWnd* parent, int width = 220);

	//resource 의 sub-popup 을 직접 load. idx0 = LoadMenu 직후 첫 번째 GetSubMenu, idx1 = 그 안의 nested sub-popup (생략 시 1 단계만).
	//IDR 의 nested 구조: load(IDR, 0, 4) = LoadMenu().GetSubMenu(0).GetSubMenu(4).
	//include_popup_placeholder=true 시 POPUP (sub menu) 항목 + VS 가 빈 POPUP 을 down-grade 한 MENUITEM id=0xFFFF 도
	//placeholder 로 add — 호출자가 attach_submenu_by_caption() 으로 .rc 의 POPUP 자리에 코드 측 CSCMenu instance 를 매핑.
	//.rc 가 menu 구조의 single source of truth.
	void			load(UINT resource_id, int idx0, int idx1 = -1, bool include_popup_placeholder = false);
	//이미 navigate 된 CMenu* 의 항목들을 load. 3단계 이상 nested 등 특수 경우.
	//subpopup_as_placeholder=true 시 자식 POPUP (내용 유무 무관) 을 재귀 auto-nest 하지 않고 placeholder
	//(item_submenu, sub_menu=nullptr) 로만 add — 모든 sub-popup 을 attach 로 채울 때 우클릭마다의 재귀 create() 비용 제거.
	void			load_from_menu(CMenu* pMenu, bool include_popup_placeholder = false, bool subpopup_as_placeholder = false);

	//인덱스 의존성 제거 — caption 으로 navigate. parent_caption 이 NULL/빈 문자열이면 top-level 의 target_caption POPUP 직접 load.
	//그 외엔 LoadMenu(resource_id) > parent_caption POPUP > target_caption POPUP 자식들을 load.
	//.rc 의 자식 순서 변경에 robust — .rc 가 source of truth.
	//subpopup_as_placeholder: load_from_menu 참고 — 직접 자식 POPUP 을 attach 용 placeholder 로만 load.
	void			load_by_caption(UINT resource_id, LPCTSTR parent_caption, LPCTSTR target_caption,
									bool include_popup_placeholder = false, bool subpopup_as_placeholder = false);

	//load_from_menu(include_popup_placeholder=true) 로 만들어진 sub-menu placeholder 항목 (m_sub_menu==nullptr) 중
	//caption 매칭 항목에 sub_menu attach. 못 찾으면 false. .rc 의 menu 구조 + 코드 instance 매핑 패턴.
	//caption 매칭은 정규화 후 비교 — `&` access-key marker 제거 + 좌우 공백 trim. `&&` literal 은 single `&` 로 보존.
	//new_id != 0 이면 placeholder 의 unique negative id 를 new_id 로 교체 — 호출자가 attach 후
	//enable_item(new_id, ...) / check_item(new_id, ...) 사용 가능. 0 이면 placeholder id 유지.
	bool			attach_submenu_by_caption(LPCTSTR caption, CSCMenu* sub_menu, int new_id = 0);

	//caption 대신 항목의 고정 ID 로 찾아 submenu 로 변환·attach. 못 찾으면 false.
	//.rc 의 POPUP 은 ID 가 없어 위치 특정이 어렵다 → 동적 생성 submenu (최근 파일 목록 등) 는 자리에
	//고정 ID 를 가진 MENUITEM 을 두고 이 함수로 attach 하면 caption 변경·번역·빈 POPUP down-grade 에 견고.
	//new_id != 0 이면 attach 후 id 를 교체 — enable_item(new_id) / check_item(new_id) 용.
	bool			attach_submenu_by_id(int id, CSCMenu* sub_menu, int new_id = 0);

	//add menu item manually. _id < 0 = separator
	void			add(int _id, CString _caption = _T(""), UINT icon_id = 0, CString _hot_key = _T(""));

	//bulk add 최적화. add() 는 매 호출 recalc_items_rect()(전 항목 GDI 측정, O(n)) 하므로 N개 추가 시 O(n²).
	//begin_update()~end_update() 로 감싸면 그 동안 recalc 를 보류하고 end_update() 에서 1회만 수행 → O(n).
	//중첩 호출 안전(카운터). 항목을 여러 개 add/load 하는 모든 경로에서 권장.
	void			begin_update();
	void			end_update();

	//썸네일 + 2줄 텍스트 항목. res_type 은 "PNG"/"JPG" 등 리소스 type 문자열, res_id 는 리소스 ID.
	//_secondary 가 빈 문자열이면 단일 라인으로 표시.
	void			add_thumbnail_item(int _id, CString _primary, CString _secondary,
									   CString thumbnail_res_type, UINT thumbnail_res_id);

	//bitmap 을 직접 받는 오버로드. 캡처된 프레임 / registry 에서 디코딩한 bitmap 등.
	//내부적으로 deep_copy 하므로 호출자는 인자 lifetime 신경 쓸 필요 없음.
	void			add_thumbnail_item(int _id, CString _primary, CString _secondary,
									   CSCGdiplusBitmap* thumbnail);

	//submenu 항목. sub_menu 의 lifecycle 은 호출자가 보유 (보통 dialog 멤버).
	//클릭 시 sub_menu 가 항목 우측 edge 에서 popup, 부모 메뉴는 자식이 닫힐 때까지 유지.
	void			add_submenu_item(int _id, CString _caption, CSCMenu* sub_menu);
	//id 의 submenu item 을 before_id / after_id 항목 *바로 앞/뒤* 에 삽입 — add + move_item_before/after 한 호출 단축.
	//ref_id 가 존재 안 하면 맨 뒤 append (= add_submenu_item 과 동일).
	void			insert_submenu_item_before(int _id, CString _caption, CSCMenu* sub_menu, int before_id);
	void			insert_submenu_item_after (int _id, CString _caption, CSCMenu* sub_menu, int after_id);
	//id 의 기존 항목 (MENUITEM 등) 을 같은 위치에서 submenu 항목으로 *교체*. remove_item + add_submenu_item 의
	//순서 안전판 — append 가 아니라 위치 보존. .rc 의 정적 MENUITEM 자리를 runtime 에 동적 submenu 로 변환할 때 사용.
	//caption == NULL 이면 기존 caption 유지. 못 찾으면 false (no-op). id 매칭 첫 항목만 교체.
	bool			replace_item_with_submenu(int id, CSCMenu* sub_menu, LPCTSTR caption = NULL);

	//sub button이 여러개일 때 args에 나열하여 호출.
	template <typename ... Types> void add(int _id, CString _caption = _T(""), UINT icon_id = 0, CString _hot_key = _T(""), Types... args)
	{
		CSCMenuItem* item = new CSCMenuItem(_id, _caption, icon_id, _hot_key);

		int n = sizeof...(args);
		int arg[] = { args... };

		for (auto id : arg)
		{
			CSCMenuSubButton* button = new CSCMenuSubButton(id);
			item->m_buttons.push_back(button);
		}

		m_items.push_back(item);
		recalc_items_rect();
	}

	template <typename ... Types> void	set_icon_and_buttons(UINT sub_menu_id, UINT icon_id, Types... button_ids)
	{
		CSCMenuItem* menu_item = get_menu_item(sub_menu_id);
		menu_item->set_icon(icon_id);

		int n = sizeof...(button_ids);
		if (n == 0)
			return;

		UINT arg[] = { button_ids... };
		for (auto button_id : arg)
		{
			if (button_id > 0)
				menu_item->add_button(button_id);
		}
	}

	void			add_sub_button_by_menu_index(int index, UINT id);
	void			add_sub_button_by_menu_id(int menu_id, UINT id);

	void			set_check(UINT menu_id, int sub_button_index, bool check);

	//CMenu 호환 API. sub-button 의 set_check 와 별개로, 항목 자체의 enable/check 상태.
	void			enable_item(int menu_id, bool enabled);			//CMenu::EnableMenuItem 동등
	//caption 으로 enable/disable. submenu placeholder (id 가 자동 nested 의 negative 라 호출자가 모름) 의
	//enable 제어용 — 예: m_menu_play.enable_item_by_caption("탐색", media_open).
	//매칭은 normalize_menu_caption 적용 (accelerator key strip).
	bool			enable_item_by_caption(LPCTSTR caption, bool enabled);
	void			check_item(int menu_id, bool checked);			//CMenu::CheckMenuItem 동등
	//[first_id, last_id] 범위 내에서 selected_id 만 checked, 나머지 uncheck. CMenu::CheckMenuRadioItem 동등.
	void			check_radio_item(int first_id, int last_id, int selected_id);


	//모니터 영역을 벗어나지 않도록 보정한 후 표시해야 한다.
	void			popup_menu(int x, int y);
	CSCMenuItem*	get_menu_item(int menu_id);
	int				get_menu_count() { return m_items.size(); }
	//모든 항목 destroy. 동적 메뉴 (북마크 / 최근 파일 등) 를 popup 마다 rebuild 할 때 사용.
	void			clear();
	//특정 menu_id 항목 1개 제거. 첫 매칭만 제거. resource load 후 일부 항목 위치를 옮기고 싶을 때 사용
	//(load 가 모든 항목을 append 하므로 duplicate 가 생기는 경우 — 호출자가 동일 ID 를 미리 add 한 후 load 끝에 remove).
	void			remove_item(int menu_id);

	//id 항목을 after_id 항목 바로 뒤로 이동. 동적 추가 항목 (add_submenu_item 등) 을 resource 로 load 된
	//특정 위치 뒤에 끼워 넣을 때 사용. 둘 중 하나라도 못 찾으면 no-op.
	void			move_item_after(int id, int after_id);
	//id 항목을 before_id 항목 바로 앞으로 이동. before_id 가 맨 앞이면 id 가 0번 위치로.
	void			move_item_before(int id, int before_id);

	//라인 간격
	int				get_line_height() { return m_line_height; }
	void			set_line_height(int _line_height);

	void			use_over(bool use = true) { m_use_over = use; }
	int				get_over_item() { return (m_use_over ? m_over_item : -1); }

	void			set_back_image(CSCGdiplusBitmap* img);

	//썸네일 항목 표시 영역 크기. 기본 80x45 (16:9 비대칭). 정사각 thumbnail 사용 시 128x128 등으로 설정.
	void			set_thumbnail_size(int w, int h);

	virtual			CSCMenu& set_font(LOGFONT& lf);
	virtual			CSCMenu& set_font_name(CString sFontname, BYTE byCharSet = DEFAULT_CHARSET);
	virtual			CSCMenu& set_font_size(int nSize);
	virtual			CSCMenu& set_font_bold(bool bBold = true);

	void			set_color_theme(int theme)
	{
		m_theme.set_color_theme(theme);
		//자동 nested (load_from_menu 가 생성) 도 동기화 — 외부 set_color_theme 한 번 호출로 nested 까지 동일 톤.
		for (auto* sub : m_owned_sub_menus) if (sub) sub->set_color_theme(theme);
		if (!m_hWnd) return; Invalidate();
	}
	void			set_color_theme(const CSCColorTheme& theme)
	{
		m_theme.copy_colors_from(theme);
		for (auto* sub : m_owned_sub_menus) if (sub) sub->set_color_theme(theme);
		if (!m_hWnd) return; Invalidate();
	}
	int				get_color_theme() const { return m_theme.get_color_theme(); }

protected:
	CSCColorTheme	m_theme = CSCColorTheme(this);

	CWnd*			m_parent = NULL;
	CSCMenu*		m_parent_menu = nullptr;
	bool			m_suppress_cascade_hide = false;
	CSCGdiplusBitmap	m_img_back;

	bool			is_in_menu_chain(CWnd* pWnd);
	bool			contains_descendant_hwnd(HWND hwnd);
	void			hide_visible_descendants();
	void			hide_visible_ancestors();		//자기 위쪽 (parent_menu 체인) 모두 hide.
	void			popup_submenu_for(int over_index);	//over_index 의 항목이 submenu 면 부모 우측 edge 너머에 popup.

	//키보드 navigation helpers — PreTranslateMessage 의 WM_KEYDOWN 분기에서 사용.
	void			set_over_item(int idx);				//hover 항목 변경 + 형제 submenu dismiss + Invalidate.
	void			navigate_over(int delta);			//+1 = 다음 enabled 항목, -1 = 이전. separator skip + wrap-around.
	void			activate_over_item();				//over_item 활성화 — submenu 면 popup, 일반은 selchanged + cascade close.
	bool			handle_access_key(TCHAR ch);		//ch (대소문자 무관) 매칭 access_key 항목 찾아 activate.

	//이번 popup session 동안 sub-menu 가 한 번이라도 열린 적 있으면 true. 다음 hover 의 delay 생략 결정에 사용.
	//popup_menu 진입 시 false 로 reset, popup_submenu_for 첫 호출 시 true.
	bool			m_submenu_ever_opened = false;

	//mouse vs keyboard 우선순위. 키보드 nav 후 mouse 가 실제로 움직일 때까지 hover 갱신 차단.
	//Invalidate 후 synthetic WM_MOUSEMOVE / 키 누를 때 손 jitter 가 keyboard hover 를 덮는 증상 회피.
	CPoint			m_last_mouse_pt = CPoint(-1, -1);
	bool			m_kb_priority = false;
	std::deque<CSCMenuItem*> m_items;

	//load_from_menu 가 .rc 의 자식 있는 POPUP 을 자동 재귀로 load 할 때 생성한 nested CSCMenu 의 ownership.
	//attach_submenu_by_caption 으로 외부 sub_menu 가 덮어써도 leak 회피 — destructor / clear() 가 일괄 delete.
	std::deque<CSCMenu*> m_owned_sub_menus;

	bool			m_use_over = true;			//hover hilighted
	int				m_over_item = -1;
	int				get_item_index(CPoint pt);

	LOGFONT			m_lf;
	CFont			m_font;
	void			ReconstructFont();

	//라인 높이는 글꼴 높이에 따라 자동 계산된다.
	//만약 수동으로 라인 간격을 변경하려면 set_line_height(32); 와 같이 함수를 이용해야 한다.
	//font_size를 증감했는데 라인 간격이 변경되지 않는 것도 문제가 되니 필요할 경우
	//해당 함수를 통해 설정하자.
	int				m_line_height;
	int				m_thumb_w = 80;
	int				m_thumb_h = 45;
	int				m_min_width = 220;		//create() 시 전달된 너비 — 항목 측정값이 이보다 크면 그만큼 확장.
	int				m_update_depth = 0;		//>0 이면 recalc_items_rect() 보류 (begin_update/end_update bulk add 최적화).
	//항목이 추가/삭제되거나 m_line_height가 변경되면 반드시 rect정보를 갱신해줘야 한다.
	//라인 간격은 font size에 따라 자동 조절되게 할 수도 있지만 장단점이 있다.
	//일단은 별개로 처리되도록 한다.
	void			recalc_items_rect();
	//컨텐츠가 모니터 높이를 초과할 때 스크롤. m_r 은 항상 컨텐츠 좌표.
	int				m_content_h = 0;			//전체 항목이 차지하는 높이 (recalc 에서 산출)
	int				m_scroll_offset = 0;		//현재 스크롤된 픽셀
	int				m_max_scroll = 0;			//최대 스크롤
	bool			m_scrollable = false;
	int				m_auto_scroll_dir = 0;		//-1=top arrow hover, +1=bottom, 0=none
	static const int scroll_arrow_h = 20;

	void			scroll_by(int dy);
	int				view_dy() const { return (m_scrollable ? scroll_arrow_h : 0) - m_scroll_offset; }
	CPoint			client_to_content(CPoint pt) const { return CPoint(pt.x, pt.y - view_dy()); }
	CRect			get_top_arrow_rect() const;
	CRect			get_bottom_arrow_rect() const;

protected:
	DECLARE_MESSAGE_MAP()
public:
	//virtual void DrawItem(LPDRAWITEMSTRUCT /*lpDrawItemStruct*/);
	virtual void PreSubclassWindow();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	static LRESULT CALLBACK menu_mouse_hook_proc(int nCode, WPARAM wParam, LPARAM lParam);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//afx_msg BOOL OnLbnSelchange();
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
};


