#pragma once

#include <afxwin.h>
#include <afxdialogex.h>

/*
* png, gif 등의 이미지 또는 CGdiplusBitmap 이미지 모양대로 dlg를 표시한다.
* 위 목적의 dlg를 생성한 후 CDialogEx 대신 CSCShapeDlg를 상속받는다.
* class CPopupDlg : public CSCShapeDlg
* {
*		...
* }
* 
* => 불필요한 중간 dlg를 생략하고 create으로 직접 생성해서 사용하자!
* 
* //CPopupDlg::OnInitDialog()에서 이미지를 지정한다.
* load(_T("Z:\\내 드라이브\\media\\test_image\\progress\\checking.gif"));
* 
* 또는 CGdiplusBitmap 이미지를 생성한 후 render(Gdiplus::Bitmap* img)함수를 호출하여
* 이미지를 설정한다.
*/

#include "../../GdiplusBitmap.h"

class CShapeDlgSetting
{
public:
	CShapeDlgSetting(int _font_size = 40,
					int _font_bold = true,
					int _shadow_depth = 2,
					int _thickness = 2,
					CString _font_name = _T("Arial"),
					Gdiplus::Color _cr_text = Gdiplus::Color::RoyalBlue,
					Gdiplus::Color _cr_stroke = Gdiplus::Color::LightGray,
					Gdiplus::Color _cr_shadow = Gdiplus::Color::DarkGray,
					Gdiplus::Color _cr_back = Gdiplus::Color::Transparent)
	{
		font_size = _font_size;
		font_bold = _font_bold;
		shadow_depth = _shadow_depth;
		thickness = _thickness;
		font_name = _font_name;
		cr_text = _cr_text;
		cr_stroke = _cr_stroke;
		cr_shadow = _cr_shadow;
		cr_back = _cr_back;
	}

	int		font_size;
	bool	font_bold;
	int		shadow_depth;
	int		thickness;
	CString	font_name;
	Gdiplus::Color cr_text;
	Gdiplus::Color cr_stroke;
	Gdiplus::Color cr_shadow;
	Gdiplus::Color cr_back;
};

//CSCShapeDlg
class CSCShapeDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSCShapeDlg)

public:
	CSCShapeDlg();   // 표준 생성자입니다.
	virtual ~CSCShapeDlg();

	bool			create(CWnd* parent, int left, int top, int right, int bottom);

	//set_image()로 CGdiplusBitmap를 받는 경우는 반드시 deep_copy를 해야하지만
	//load()를 통해서 직접 로딩하여 m_img에 넣을 경우는 불필요하다.
	void			set_image(CGdiplusBitmap* img, bool deep_copy = true);
	bool			load(UINT id);
	bool			load(CString sType, UINT id);
	bool			load(CString sFile);


	//gdiplus를 이용한 text 출력용 dlg 생성 및 출력
	bool			set_text(CWnd* parent, CString text, int font_size, bool font_bold,
							int shadow_depth = 2, int thickness = 2,
							CString font_name = _T(""),
							Gdiplus::Color cr_text = Gdiplus::Color::RoyalBlue,
							Gdiplus::Color cr_stroke = Gdiplus::Color::LightGray,
							Gdiplus::Color cr_shadow = Gdiplus::Color::DarkGray,
							Gdiplus::Color cr_back = Gdiplus::Color::Transparent);
	bool			set_text(CString text, CShapeDlgSetting* setting = NULL);
	CShapeDlgSetting m_setting;

	//alpha = 0 ~ 255
	void			alpha(int alpha);

	//animated gif인 경우
	enum GIF_PLAY_STATE
	{
		state_stop = -1,
		state_pause,
		state_play,
		state_toggle,
	};

	int				m_gif_state = state_stop;
	int				m_gif_index = 0;

	void			gif_thread();
	void			gif_play(int new_state = state_play);
	void			gif_pause();
	void			gif_stop();
	void			gif_goto(int pos, bool pause = false);

protected:
	CWnd*			m_parent = NULL;

	CGdiplusBitmap	m_img;
	int				m_alpha = 255;
	void			render(Gdiplus::Bitmap* img);

	LOGFONT			m_lf;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	virtual void PreSubclassWindow();
};
