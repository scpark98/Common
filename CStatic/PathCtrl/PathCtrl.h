#pragma once

/*
* scpark 2023041110
* ������Ž������ �ּ�ǥ���ٰ� ������ UI�� ǥ�õǴ� ��Ʈ��.
* ���� ��ǻ���� ���� ��� ������ ���� ������ �˻��ؼ� ����Ʈ�� ����������
* ������ ��쿡�� ����Ʈ�� �޾Ƽ� ����Ʈ�� ǥ������� �Ѵ�.
* 
* ��Ȥ resize�ÿ� �����Ÿ��� �߻��Ѵٸ�(mainDlg�� ������ ���� ĥ�ϴ� ���� ���)
* mainDlg�� �Ӽ����� Clip Children�� true�� ��������� �Ѵ�.
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
	int		ellipsis_pos = -1;	//label�� �� ������ ��� ǥ�õ� ������ġ. ellipsis�� true�϶��� �ǹ�����
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
		message_pathctrl_path_changed,						//��Ʈ�� ��ü���� �н��� �����ϸ� �̸� parent�� �˸���.
		message_pathctrl_request_remote_subfolders,
		message_pathctrl_is_remote_path_file_exist,
	};


	//remote path�� ���� ���� ������ parent�� ��û�ؾ� �Ѵ�.
	//��ũ ����, � ������ ���� ���� ����Ʈ, ���� �Ǵ� ������ ���� ����

	//�������� �������� ����
	void		set_is_local_device(bool is_local);

	//������ ��� ����̺� ���� ����Ʈ�� ���ͼ� �� �Լ��� ���� �̸� �־���� �Ѵ�.
	//({drive0 letter, drive0 volume}, {drive1 letter, drive1 volume}, ...)
	void		set_remote_drive_volume(std::map<TCHAR, CString>* remote_drive_volume);
	//�ѹ��� map�� �ֱ� ���� ������� �Ʒ� �Լ��� ���� �ϳ��� �߰��ص� �ȴ�.(add_remote_drive_volume(_T("���� ��ũ (C:)"));
	void		add_remote_drive_volume(CString remote_drive_volume);

	//Ư�� �ε��������� fullpath�� �����Ѵ�. -1�̸� ���� over �Ǵ� down�� ��α��� ����.
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

	//path�׸��� ������ pulldown�� �������� Ž����� Ư������, ����, �������ϱ��� ��� �����ְ� ������
	//���⼭�� Ư�������� ������ ��������.
	CColorListBox		m_list_folder;
	CSize				m_sz_list_folder = CSize(228, 386);
	std::deque<CString>	m_remote_sub_folders;
	void				show_sub_folder_list(bool show);

	bool		m_use_edit = true;		//���� �׸� �̿��� ���� Ŭ���� ���� ��������� ����� ������
	CEdit*		m_pEdit;
	CString		m_old_text;				//�����Ǳ� ���� ���� �ؽ�Ʈ
	CRect		m_edit_margin;			//edit box ���� ����(���η� ��� ���ĵǰ� ǥ���ϱ� ����)
	void		repos_edit();			//resize�� �ϸ� ������ ���µǹǷ� ��ġ�� ������ �ٽ� ���

	std::deque<CPathElement> m_path;
	int			m_max_width = 200;	//�� path element�� ǥ���� �� �ִ� �ִ� label width in pixel
	//int			m_root_width = 35;	//20+15. �� ���ʿ� �׻� ǥ�õǴ� �������� �� ������ m_path���� ���Ե��� �ʴ´�.(width�� ������ ������ �׸���� ǥ���ϴµ� �̶����� ��Ʈ�� �׻� ǥ�õȴ�.)
	int			m_width_margin = 5;			//label�� �¿� ����. �����̹Ƿ� �� ���� = m_width_margin*2
	int			m_arrow_area_width = 15;
	void		recalc_path_width();
	void		recalc_path_position();

	COLORREF	m_crText = ::GetSysColor(COLOR_BTNTEXT);
	COLORREF	m_crBack = ::GetSysColor(COLOR_WINDOW);
	COLORREF	m_crOver = RGB(229, 243, 255);
	COLORREF	m_crOverBorder = RGB(204, 232, 255);
	COLORREF	m_crDown = RGB(204, 232, 255);
	COLORREF	m_crDownBorder = RGB(153, 209, 255);

	int			m_index = -1;	//���� over�ǰų� down�� �׸�
	bool		m_down = false;
	int			m_start_index = 1;	//width�� rc.right���� ū ��� ǥ�õǴ� ���� �ε���. 0���� �׻� ǥ�õǴ� �׸��̹Ƿ� m_start_index�� �ݵ�� 1�̻��̴�.
	bool		m_has_subfolder;	//fullpath�Ʒ� subfolder�� ������������ ���ο� ���� UI�� �ٸ���.

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
