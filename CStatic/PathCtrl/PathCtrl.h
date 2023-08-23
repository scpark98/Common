#pragma once

/*
* scpark 2023041110
* 윈도우탐색기의 주소표시줄과 동일한 UI로 표시되는 컨트롤.
* 로컬 컴퓨터일 경우는 어느 폴더의 하위 폴더를 검색해서 리스트로 보여주지만
* 원격일 경우에는 리스트를 받아서 리스트에 표시해줘야 한다.
* 
* 간혹 resize시에 깜빡거림이 발생한다면(mainDlg의 배경색을 직접 칠하는 앱의 경우)
* mainDlg의 속성에서 Clip Children을 true로 세팅해줘야 한다.
*/

#include <afxwin.h>
#include <deque>
#include <map>
#include "../../CListBox/ColorListBox/ColorListBox.h"

//#define ROOT_LABEL	_T("_r")
#define ROOT_WIDTH	21
// CPathCtrl

static const UINT Message_CPathCtrl = ::RegisterWindowMessage(_T("MessageString_CPathCtrl"));

class CPathCtrlMessage
{
public:
	CPathCtrlMessage(CWnd* _this, int _message, CString _cur_path)
	{
		pThis = _this;
		message = _message;
		cur_path = _cur_path;
	}

	CWnd* pThis = NULL;
	CString cur_path;
	int message;
};

class CPathElement
{
public:
	CPathElement(CString _label, CRect _r = CRect(0,0,0,0), bool _ellipsis = false)
	{
		label = _label;
		r = _r;
		ellipsis = _ellipsis;
	}

	CString label;
	CRect	r;
	bool	ellipsis;
	int		ellipsis_pos = -1;	//label이 길어서 생략될 경우 표시될 문자위치. ellipsis가 true일때만 의미있음
};

class CPathCtrl : public CStatic
{
	DECLARE_DYNAMIC(CPathCtrl)

public:
	CPathCtrl();
	virtual ~CPathCtrl();

	enum PathCtrlMessage
	{
		message_pathctrl_request_remote_drive_volumes = 0,
		message_pathctrl_path_changed,						//컨트롤 자체에서 패스를 변경하면 이를 parent에 알린다.
		message_pathctrl_request_remote_subfolders,
		message_pathctrl_is_remote_path_file_exist,
	};


	//remote path일 경우는 다음 정보를 parent에 요청해야 한다.
	//디스크 볼륨, 어떤 폴더의 하위 폴더 리스트, 파일 또는 폴더의 존재 유무

	//로컬인지 원격인지 세팅
	void		set_is_local_device(bool is_local);

	//원격일 경우 드라이브 볼륨 리스트를 얻어와서 이 함수를 통해 미리 넣어줘야 한다.
	//({drive0 letter, drive0 volume}, {drive1 letter, drive1 volume}, ...)
	void		set_remote_drive_volume(std::map<TCHAR, CString>* remote_drive_volume);
	//한번에 map에 넣기 힘든 구조라면 아래 함수를 통해 하나씩 추가해도 된다.(add_remote_drive_volume(_T("로컬 디스크 (C:)"));
	void		add_remote_drive_volume(CString remote_drive_volume);

	//특정 인덱스까지의 fullpath를 리턴한다. -1이면 현재 over 또는 down된 경로까지 리턴.
	CString		get_full_path(int index = -1);

	void		set_path(CString path, std::deque<CString> *sub_folders = NULL);
	void		SetWindowText(CString path, std::deque<CString>* sub_folders = NULL);

	bool		use_edit() { return m_use_edit; }
	void		use_edit(bool use) { m_use_edit = use; }

	COLORREF	text_color() { return m_crText; }
	void		text_color(COLORREF crText) { m_crText = crText; }
	COLORREF	back_color() { return m_crBack; }
	void		back_color(COLORREF crBack) { m_crBack = crBack; }

	CShellImageList* m_pShellImageList = NULL;
	void		set_shell_imagelist(CShellImageList* pShellImageList)
	{
		m_pShellImageList = pShellImageList;
		m_list_folder.set_shell_imagelist(pShellImageList);
	}

protected:
	enum TIMER_ID
	{
		timer_mouse_over = 0,
	};

	bool						m_is_local_device = true;
	std::map<TCHAR, CString>	m_remote_drive_volume;

	//path항목의 오른쪽 pulldown을 눌렀을때 탐색기는 특수폴더, 폴더, 압축파일까지 모두 보여주고 있지만
	//여기서는 특수폴더와 폴더만 보여주자.
	CColorListBox		m_list_folder;
	CSize				m_sz_list_folder = CSize(228, 386);
	std::deque<CString>	m_remote_sub_folders;
	void				show_sub_folder_list(bool show);

	bool		m_use_edit = true;		//폴더 항목 이외의 공간 클릭시 수동 편집기능을 사용할 것인지
	CEdit*		m_pEdit;
	CString		m_old_text;				//편집되기 전의 원본 텍스트
	CRect		m_edit_margin;			//edit box 내부 여백(세로로 가운데 정렬되게 표시하기 위해)
	void		repos_edit();			//resize를 하면 여백이 리셋되므로 위치와 여백을 다시 계산

	std::deque<CPathElement> m_path;
	int			m_max_width = 200;	//한 path element에 표시할 수 있는 최대 label width in pixel
	//int			m_root_width = 35;	//20+15. 맨 왼쪽에 항상 표시되는 영역으로 이 영역은 m_path에는 포함되지 않는다.(width가 작을땐 최하위 항목부터 표시하는데 이때에도 루트는 항상 표시된다.)
	int			m_width_margin = 5;			//label의 좌우 마진. 양쪽이므로 총 마진 = m_width_margin*2
	int			m_arrow_area_width = 15;
	void		recalc_path_width();
	void		recalc_path_position();

	COLORREF	m_crText = ::GetSysColor(COLOR_BTNTEXT);
	COLORREF	m_crBack = ::GetSysColor(COLOR_WINDOW);
	COLORREF	m_crOver = RGB(229, 243, 255);
	COLORREF	m_crOverBorder = RGB(204, 232, 255);
	COLORREF	m_crDown = RGB(204, 232, 255);
	COLORREF	m_crDownBorder = RGB(153, 209, 255);

	int			m_index = -1;	//현재 over되거나 down인 항목
	bool		m_down = false;
	int			m_start_index = 1;	//width가 rc.right보다 큰 경우 표시되는 시작 인덱스. 0번은 항상 표시되는 항목이므로 m_start_index는 반드시 1이상이다.
	bool		m_has_subfolder;	//fullpath아래 subfolder가 존재하지는지 여부에 따라 UI가 다르다.

	LOGFONT		m_lf;
	CFont		m_font;
	void		UpdateSurface();
	void		ReconstructFont();

	LRESULT		OnMessageColorListBox(WPARAM wParam, LPARAM lParam);

protected:
	DECLARE_MESSAGE_MAP()
	virtual void PreSubclassWindow();
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnLbnSelchange(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
};
