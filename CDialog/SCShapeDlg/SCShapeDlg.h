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
* //OnInitDialog()에서 이미지를 지정한다.
* load(_T("Z:\\내 드라이브\\media\\test_image\\progress\\checking.gif"));
* 
* 또는 CGdiplusBitmap 이미지를 생성한 후 render(Gdiplus::Bitmap* img)함수를 호출하여
* 이미지를 설정한다.
* 
* set_text()를 사용하면 이미지 대신 텍스트가 표시되는 shapeDlg를 표시할 수 있다.
*/

#include "../../GdiplusBitmap.h"
#include "../../data_structure/SCParagraph/SCParagraph.h"

static const UINT Message_CSCShapeDlg = ::RegisterWindowMessage(_T("MessageString_CSCShapeDlg"));

class CSCShapeDlgMessage
{
	CSCShapeDlgMessage(CWnd* _pThis)
	{
		pThis = _pThis;
	}

	enum Messages
	{
		msg_window_moved = 0,
	};

	CWnd*	pThis = NULL;
};

class CSCShapeDlgTextSetting
{
public:
	CSCShapeDlgTextSetting() {};
	CSCShapeDlgTextSetting(CString _text,
		float _font_size = 40,
		int _font_style = Gdiplus::FontStyleBold,
		int _shadow_depth = 2,
		float _thickness = 2,
		CString _font_name = _T("Arial"),
		Gdiplus::Color _cr_text = Gdiplus::Color::RoyalBlue,
		Gdiplus::Color _cr_stroke = Gdiplus::Color::LightGray,
		Gdiplus::Color _cr_shadow = Gdiplus::Color::DarkGray,
		Gdiplus::Color _cr_back = Gdiplus::Color(1, 0, 0, 0))	//alpha=0이면 투명 영역은 클릭되지 않는다.
	{
		text = _text;
		font_name = _font_name;
		font_size = _font_size;
		font_style = _font_style;
		shadow_depth = _shadow_depth;
		thickness = _thickness;
		cr_text = _cr_text;
		cr_stroke = _cr_stroke;
		cr_shadow = _cr_shadow;
		cr_back = _cr_back;
	}

	CString text;
	CString font_name;
	float	font_size;
	int		font_style;
	int		shadow_depth;
	float	thickness;
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

	//default : SW_HIDE
	bool			create(CWnd* parent, int left, int top, int right, int bottom);

	//set_image()로 CGdiplusBitmap를 받는 경우는 반드시 deep_copy를 해야 하지만
	//load()를 통해서 직접 로딩하여 m_img에 넣을 경우는 불필요하다.
	void			set_image(CWnd* parent, CGdiplusBitmap* img, bool deep_copy = true);
	bool			load(CWnd* parent, UINT id);
	bool			load(CWnd* parent, CString sType, UINT id);
	bool			load(CWnd* parent, CString sFile);

	//keyboard, mouse 이벤트 처리 여부.
	void			use_control(bool use);

	//0 : 투명, 255 : 불투명
	void			set_alpha(int alpha);

	//gdiplus를 이용한 text 출력. create()없이 호출되면 자동 생성 후 텍스트 윈도우를 출력함.
	//default는 hide 상태로 시작함.
	//font_name을 지정하지 않으면 mainDlg에 설정된 font를 사용함.
	CSCShapeDlgTextSetting*	set_text(CWnd* parent, CString text,
							float font_size,
							int font_style,
							int shadow_depth = 2,
							float thickness = 2.0f,
							CString font_name = _T("맑은 고딕"),
							Gdiplus::Color cr_text = Gdiplus::Color::RoyalBlue,
							Gdiplus::Color cr_stroke = Gdiplus::Color::LightGray,
							Gdiplus::Color cr_shadow = Gdiplus::Color::DimGray,
							Gdiplus::Color cr_back = Gdiplus::Color(1, 0, 0, 0));
	CSCShapeDlgTextSetting* set_text(CString str);
	CSCShapeDlgTextSetting*	set_text(CSCShapeDlgTextSetting* setting = NULL);

	void			set_text_color(Gdiplus::Color cr) { m_text_setting.cr_text = cr; set_text(&m_text_setting); }
	void			set_stroke_color(Gdiplus::Color cr) { m_text_setting.cr_stroke = cr; set_text(&m_text_setting); }
	void			set_shadow_color(Gdiplus::Color cr) { m_text_setting.cr_shadow = cr; set_text(&m_text_setting); }
	void			set_back_color(Gdiplus::Color cr) { m_text_setting.cr_back = cr; set_text(&m_text_setting); }
	void			set_thickness(float thickness) { m_text_setting.thickness = thickness; set_text(&m_text_setting); }

	//show상태로 만들고 time후에 hide된다.
	void			time_out(int time, bool fadein, bool fadeout);

	//set_image(), set_text()를 호출해도 아직 hide상태다.
	//ShowWindow()시키거나 fadein()으로 보여지게 한다.
	//0 ~ 255까지 5간격으로 alpha를 변경한다.
	//hide_after_ms, fadeout 파라미터는 fade_in에서만 사용된다.
	void			fade_in(int delay_ms = 50, int hide_after_ms = 0, bool fadeout = false);
	void			fade_out();
	bool			is_fadeinout_ing() { return m_fadeinout_ing; }
	void			thread_fadeinout(bool fadein, int delay_ms = 50, int hide_after_ms = 0, bool fadeout = false);

	//
	CSCShapeDlgTextSetting	m_text_setting;
	//get_text_setting()으로 리턴받은 세팅값을 직접 수정하여 set_text(setting);를 호출한다.
	CSCShapeDlgTextSetting*	get_text_setting() { return &m_text_setting; }

	void			get_logfont(LOGFONT *lf);

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

	std::deque<std::deque<CSCParagraph>> m_para;	//m_para[0][1] : 0번 라인의 1번 인덱스 음절

	CGdiplusBitmap	m_img;
	//0 : 투명, 255 : 불투명
	int				m_alpha = 255;
	void			render(Gdiplus::Bitmap* img);

	bool			m_fadeinout_ing = false;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	virtual void PreSubclassWindow();
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
};
