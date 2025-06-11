#pragma once

#include <afxwin.h>
#include <afxdialogex.h>

/*
* png, gif 등의 이미지 또는 CGdiplusBitmap 이미지 모양대로 dlg를 생성하여 표시한다.
* create()으로 직접 생성하므로 IDD_DIALOG와 같은 리소스가 불필요하다.
* 
* //OnInitDialog()에서 이미지를 지정한다.
* load(_T("Z:\\내 드라이브\\media\\test_image\\progress\\checking.gif"));
* 
* 또는 CGdiplusBitmap 이미지를 생성한 후 set_image() 함수를 호출하여
* 이미지를 설정할 수 있다.
* 
* set_text()를 사용하면 이미지 대신 텍스트를 출력한다.

[자막 이펙트 프리셋
- 한 문장의 한 글자씩 컬러로 표시되는 등의 text effect 들을 구현할 수 있다.
- 한 글자씩 alpha와 위치 변경

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
	CSCShapeDlgTextSetting(CString _text, CSCLogFont _lf)	//alpha=0이면 투명 영역은 클릭되지 않는다.
	{
		text = _text;
		lf = _lf;
	}

	CString text;
	CSCLogFont	lf;
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

	//keyboard, mouse 이벤트 처리 여부. false이면 모든 마우스, 키보드 이벤트가 무시된다.
	void			use_control(bool use);

	//0 : 투명, 255 : 불투명
	void			set_alpha(int alpha);

	//gdiplus를 이용한 text 출력. create()없이 호출되면 자동 생성 후 텍스트 윈도우를 출력함.
	//default는 hide 상태로 시작되므로 set_text()로 세팅한 후 ShowWindow(SW_SHOW)를 호출해야 함.
	//font_name을 지정하지 않으면 mainDlg에 설정된 font를 사용함.
	//shadow_depth는 출력되는 텍스트의 height에 대한 비율이지만 현재는 자동 조정되므로 사용되지 않음.
	CSCShapeDlgTextSetting*	set_text(CWnd* parent, CString text,
							float font_size,
							int font_style,
							float shadow_depth = 0.0f,
							float thickness = 0.0f,
							CString font_name = _T("맑은 고딕"),
							Gdiplus::Color cr_text = Gdiplus::Color::RoyalBlue,
							Gdiplus::Color cr_stroke = Gdiplus::Color::Blue,
							Gdiplus::Color cr_shadow = Gdiplus::Color::DimGray,
							Gdiplus::Color cr_back = Gdiplus::Color(0, 0, 0, 0));
	CSCShapeDlgTextSetting* set_text(CString str);
	CSCShapeDlgTextSetting*	set_text(CSCShapeDlgTextSetting* setting = NULL);

	void			set_text_color(Gdiplus::Color cr) { m_text_setting.lf.cr_text = cr; set_text(&m_text_setting); }
	void			set_stroke_color(Gdiplus::Color cr) { m_text_setting.lf.cr_stroke = cr; set_text(&m_text_setting); }
	void			set_shadow_color(Gdiplus::Color cr) { m_text_setting.lf.cr_shadow = cr; set_text(&m_text_setting); }
	void			set_back_color(Gdiplus::Color cr) { m_text_setting.lf.cr_back = cr; set_text(&m_text_setting); }
	void			set_thickness(float thickness) { m_text_setting.lf.thickness = thickness; set_text(&m_text_setting); }

	//show상태로 만들고 time후에 hide된다.
	void			time_out(int time, bool fadein, bool fadeout);

	//set_image(), set_text()를 호출해도 아직 hide상태다.
	//ShowWindow()시키거나 fadein()으로 보여지게 한다.
	//0 ~ 255까지 5간격으로 alpha를 변경한다.
	//hide_after_ms, fadeout 파라미터는 fade_in에서만 사용된다.
	void			fade_in(int delay_ms = 50, int hide_after_ms = 0, bool fadeout = false);
	void			fade_out();
	bool			is_fadeinout_ing() { return m_fadeinout_ing; }

	//
	CSCShapeDlgTextSetting	m_text_setting;
	//get_text_setting()으로 리턴받은 세팅값을 직접 수정하여 set_text(setting);를 호출한다.
	CSCShapeDlgTextSetting*	get_text_setting() { return &m_text_setting; }

	CSCLogFont*		get_logfont() { &m_text_setting.lf;}

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
	void			thread_fadeinout(bool fadein, int delay_ms = 50, int hide_after_ms = 0, bool fadeout = false);

	//m_para 항목 중 마우스 커서가 올라간 음절 정보를 parent에게 메시지로 전달한다.
	bool			m_send_hover_info = false;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	virtual void PreSubclassWindow();
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
};
