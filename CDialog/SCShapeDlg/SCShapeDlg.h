#pragma once

#include <afxwin.h>
#include <afxdialogex.h>

/*
* https://github.com/scpark98/Test_SCShapeDlg.git
* png, gif 등의 이미지 또는 CSCGdiplusBitmap 이미지 모양대로 dlg를 생성하여 표시한다.
* create()으로 직접 생성하므로 IDD_DIALOG와 같은 리소스가 불필요하다.
* 
* //OnInitDialog()에서 이미지를 지정한다.
* load(_T("Z:\\내 드라이브\\media\\test_image\\progress\\checking.gif"));
* 
* 또는 CSCGdiplusBitmap 이미지를 생성한 후 set_image() 함수를 호출하여
* 이미지를 설정할 수 있다.
* 
[텍스트 출력]
- set_text()를 사용하면 이미지 대신 텍스트를 출력한다.
- 기본 정렬은 DT_CENTER지만 set_text_align()으로 텍스트 정렬을 변경할 수 있다.

[자막 이펙트 프리셋]
- 한 문장의 한 글자씩 컬러로 표시되는 등의 text effect 들을 구현할 수 있다.
  CEffectSetting 클래스가 필요할 수 있다.
- 한 글자씩 alpha와 위치 변경

[Usage]
- .h에 #include "Common/CDialog/SCShapeDlg/SCShapeDlg.h"
- CSCShapeDlg		m_message;

- .cpp의 OnInitDialog()에 다음과 같이 초기화 (텍스트 출력 예시)
  	m_message.set_text(this, _T(""), 40, Gdiplus::FontStyleBold, 4.0f, 2.4f);
	m_message.set_stroke_color(Gdiplus::Color::Black);
	m_message.set_alpha(192);
	m_message.use_control(false);

- 텍스트 변경 시 m_message.set_text(_T("표시할 텍스트")); 와 같이 호출하거나
  아래와 같이 세부적인 설정을 한 후 표시할 수 있다.
	CSCShapeDlgTextSetting* setting = m_message.get_text_setting();
	setting->text = message;
	setting->text_prop.size = rc.Height() / 26.18f;		//rc 높이에 비례하는 글자 크기로 자동 조정
	Clamp(setting->text_prop.size, 16.0f, 44.0f);

	m_message.set_text(setting);
	m_message.CenterWindow();
	m_message.fade_in(0, 1000, true);					//표시한 후 1초 후 fade_out

*/

#include "../../SCGdiplusBitmap.h"
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
	CSCShapeDlgTextSetting(CString _text, CSCTextProperty _text_prop)	//alpha = 0이면 투명 영역은 클릭되지 않는다.
	{
		text = _text;
		text_prop = _text_prop;
	}

	CString text;
	CSCTextProperty	text_prop;
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

	//set_image()로 CSCGdiplusBitmap를 받는 경우는 반드시 deep_copy를 해야 하지만
	//load()를 통해서 직접 로딩하여 m_img에 넣을 경우는 불필요하다.
	void			set_image(CWnd* parent, CSCGdiplusBitmap* img, bool deep_copy = true);
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
	//shadow_depth는 출력되는 텍스트의 height에 대한 비율이지만 font_size에 비례하여 자동 조정되므로 지금은 사용되지 않음.
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

	void			set_text_color(Gdiplus::Color cr) { m_text_setting.text_prop.cr_text = cr; set_text(&m_text_setting); }
	void			set_stroke_color(Gdiplus::Color cr) { m_text_setting.text_prop.cr_stroke = cr; set_text(&m_text_setting); }
	void			set_shadow_color(Gdiplus::Color cr) { m_text_setting.text_prop.cr_shadow = cr; set_text(&m_text_setting); }
	void			set_back_color(Gdiplus::Color cr) { m_text_setting.text_prop.cr_back = cr; set_text(&m_text_setting); }
	void			set_thickness(float thickness) { m_text_setting.text_prop.thickness = thickness; set_text(&m_text_setting); }

	//기본 정렬은 센터정렬로 만들어지지만 DT_LEFT를 주면 모든 라인이 왼쪽 정렬된다. 각 라인마다 따로 정렬을 지정할 수도 있지만 필요성을 따져봐야 한다.
	void			set_text_align(int align);
	int				get_text_align() { return m_text_align; }	

	//show상태로 만들고 time후에 hide된다.
	void			time_out(int time, bool fadein, bool fadeout);

	//set_image(), set_text()를 호출해도 아직 hide상태다.
	//ShowWindow()시키거나 fadein()으로 보여지게 한다.
	//0 ~ 255까지 5간격으로 alpha를 변경한다.
	//hide_after_ms, fadeout 파라미터는 fade_in에서만 사용된다.
	void			fade_in(int fade_in_delay_ms = 10, int hide_after_ms = 0, bool fadeout = false, int fade_out_delay_ms = 10);
	void			fade_out(int fade_out_delay_ms = 10);
	bool			is_fadeinout_ing() { return m_fadeinout_ing; }

	//
	CSCShapeDlgTextSetting	m_text_setting;
	//get_text_setting()으로 리턴받은 세팅값을 직접 수정하여 set_text(setting);를 호출한다.
	CSCShapeDlgTextSetting*	get_text_setting() { return &m_text_setting; }

	CSCTextProperty*		get_logfont() { return &m_text_setting.text_prop; }

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

	//원래 protected로 선언되는게 일반적이지만 Gdiplus관련 멤버함수 등을 원활히 사용하기 위해 public으로 변경한다.
	CSCGdiplusBitmap	m_img;

protected:
	CWnd*			m_parent = NULL;

	std::deque<std::deque<CSCParagraph>> m_para;	//m_para[0][1] : 0번 라인의 1번 인덱스의 구절

	//0 : 투명, 255 : 불투명
	int				m_alpha = 255;
	void			render(Gdiplus::Bitmap* img);

	//DT_LEFT, DT_RIGHT, DT_CENTER 가로	정렬만 지원된다.
	int				m_text_align = DT_CENTER;

	bool			m_fadeinout_ing = false;
	//fade_in(), fade_out() 함수에서 호출하며
	//이미 이 thread가 돌고 있는중에 다시 fade_in(), fade_out()이 호출되면
	//이 thread는 즉시 중단되고 text를 변경한 후 다시 이 thread가 호출된다.
	//이 때 hide시키지 않고 text만 변경시킨다. 그렇지 않으면 사라졌다가 나타나므로 깜빡이게 된다.
	void			thread_fadeinout(bool fadein, int fadein_delay_ms = 10, int hide_after_ms = 0, bool fadeout = false, int fadeout_delay_ms = 10);

	//effect를 종류별로 thread로 돌리면 분명 충돌이 일어날 수 있다.
	//그렇다고 하나의 thread를 만들어서 모든 effect를 처리하게 하면
	//각 effect마다 range, interval이 모두 다를 수 있으므로 처리가 어렵다.

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
