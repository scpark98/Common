#pragma once

#include <afxwin.h>
#include <deque>

#include "../../colors.h"
#include "../../SCGdiplusBitmap.h"

// CSCStepCtrl
/*
* CSliderCtrl or CProgressCtrl과는 약간 다르게 진행 스텝을 텍스트와 함께 표시할 필요가 있어
  CStatic을 상속받아 제작함.
  CSCSliderCtrl에 style_step을 추가하여 제작중이었으나 CSliderCtrl or CProgressCtrl과는
  다르게 처리되어야 하는 부분들이 있으므로 별도 제작함.

  위 두 컨트롤은 SetRange의 범위를 벗어나지 않지만 StepCtrl은 시작전과 완료후까지의 범위가 필요하다.
  m_step.set_range(0, 10);이라면 -1 ~ 11까지 표현 가능하다. -1=시작전, 11=완료

  또한 각 스텝마다 텍스트를 표시할 수 있다.

* 변수명 규칙
  step	: 각각의 단계 항목을 지칭하며 thumb와 text를 포함하는 의미로 사용
  thumb	: 각 스텝에 그려지는 도형 (원형 또는 사각형)
  text	: 각 스텝의 우측 또는 하단에 표시되는 레이블

* 사용법
  //.h에 #include 및 변수 선언
  CSCStepCtrl		m_step;

  //.cpp에서 설정
  m_step.set_direction(true);
  m_step.set_step_style(step_style_circle);
  m_step.set_step_count(4);
*/

//각 스텝의 모양
enum THUMB_STYLE
{
	thumb_style_none = -1,
	thumb_style_circle,
	thumb_style_circle_with_num,
	thumb_style_rect,
	thumb_style_rect_round,
	thumb_style_diamond,
	thumb_style_arrow,
};

class CSCStepElement
{
public:
	CRect			r;		//step이 그려지는 rect이며 텍스트는 r을 기준으로 그려지므로 텍스트 영역을 포함하지 않는다.
	CString			text;

	//각 스텝 모양은 m_step_style을 기본 스타일로 하지만 어느 경우는 달리 표시할 필요도 있다.
	int				thumb_style = thumb_style_none;

	//이 기본값인 투명색 그대로이면 대표 컬러인 m_cr_thumb를 사용하고 지정되어 있으면 해당 색으로 표시
	Gdiplus::Color	cr_thumb = Gdiplus::Color::Transparent;
	Gdiplus::Color	cr_text = Gdiplus::Color::Transparent;
};

class CSCStepCtrl : public CStatic
{
	DECLARE_DYNAMIC(CSCStepCtrl)

public:
	CSCStepCtrl();
	virtual ~CSCStepCtrl();

	void			set_style(bool is_horz, int thumb_style = thumb_style_circle) { m_horz = is_horz; m_thumb_style = thumb_style; }

	//총 단계 수
	void			set_step_count(int count) { m_pos = -1; m_step.resize(count); }
	int				get_step_count() { return m_step.size(); }

	//index < 0이면 모든 thumb에 적용.
	void			set_thumb_style(int index, int style);

	//l, t, r, b의 margin. 가로형일 경우 텍스트가 충분히 표시되도록 좌우 여백을 줘야 한다.
	void			set_margin(int left, int top, int right, int bottom) { m_margin = CRect(left, top, right, bottom); }
	//step과 텍스트 간격
	void			set_gap_to_text(int gap) { m_gap_to_text = gap; }

	int				get_pos() { return m_pos; }
	void			set_pos(int pos) { m_pos = pos; Invalidate(); }
	void			step() { m_pos++; Invalidate(); }

	//각 스텝에 텍스트 지정. Transparent값이면 해당 항목 무시됨.
	void			set_text(int index, CString text, Gdiplus::Color cr = Gdiplus::Color::Transparent);
	template <typename ... Types> void set_texts(Types... args)
	{
		int n = min(m_step.size(), sizeof...(args));
		CString texts[] = { args... };

		for (int i = 0; i < n; i++)
			m_step[i].text = texts[i];
	}

	//thumb와 text의 색상을 모두 변경한다. cr_current가 투명이면 m_cr_text_current, m_cr_thumb_current 값은 변경하지 않는다.
	void			set_step_color(int index, Gdiplus::Color cr_active, Gdiplus::Color cr_current = Gdiplus::Color::Transparent);
	void			set_thumb_color(int index, Gdiplus::Color cr_active, Gdiplus::Color cr_current = Gdiplus::Color::Transparent);
	void			set_text_color(int index, Gdiplus::Color cr_active, Gdiplus::Color cr_current = Gdiplus::Color::Transparent);

	//특정 step의 thumb와 text의 색상을 리셋시키고 기본색을 사용하게 한다.
	//index == -1이면 모든 스텝 리셋
	void			reset_step_color(int index = -1);

	//각 스텝의 thumb 크기
	void			set_thumb_size(int size);

	void			set_draw_line(bool draw) { m_draw_line = draw; }

	//20260729 by claude. 완료 스텝 thumb 안에 그리는 체크 표시 색.
	//기본값 Transparent 는 "자동" 을 뜻하며, 그 스텝의 채움색보다 64 어두운 색을 쓴다(get_check_color).
	//흰 체크는 제품 테마마다 채움색 대비가 제각각이라 어울리는 테마가 한정된다.
	void			set_check_color(Gdiplus::Color cr) { m_cr_check = cr; m_check_color_user_set = (cr.GetValue() != Gdiplus::Color::Transparent); Invalidate(); }

	//자동 파생 시 채움색과 벌릴 WCAG 대비비. 기본 4.5 = 본문 가독(AA) 기준.
	//3.0 으로 낮추면 은은해지고, 7.0(AAA) 이면 거의 흑/백에 가깝게 벌어진다.
	void			set_check_contrast(double ratio) { m_check_contrast = ratio; Invalidate(); }

	//20260729 by claude. 테마 accent 한 색에서 완료 / 진행 중 / 대기 색을 모두 파생시킨다.
	//제품별 빌드 구성마다 테마가 달라(anysupport teal-blue, helpu teal, linkmemine navy/orange ...)
	//개별 색을 직접 지정하면 한 제품에서만 어울리는 배색이 된다.
	void			set_color_theme(const CSCColorTheme& theme, bool invalidate = true);

	void			set_back_color(Gdiplus::Color cr) { m_cr_back = cr; }
	void			set_color_line_active(Gdiplus::Color cr) { m_cr_line_active = cr; }
	void			set_color_line_inactive(Gdiplus::Color cr) { m_cr_line_inactive = cr; }

protected:
	bool			m_horz = true;
	bool			m_draw_line = true;
	int				m_pos = -1;
	int				m_thumb_style = thumb_style_circle;
	int				m_thumb_size = 18;
	CRect			m_margin = CRect(8, 8, 8, 8);
	int				m_gap_to_text = 8;	//step과 텍스트 사이 간격

	std::deque<CSCStepElement> m_step;

//color
	Gdiplus::Color	m_cr_back = get_sys_color(COLOR_3DFACE);

	Gdiplus::Color	m_cr_text_active = Gdiplus::Color::RoyalBlue;
	Gdiplus::Color	m_cr_text_current = Gdiplus::Color::Blue;
	Gdiplus::Color	m_cr_text_inactive = Gdiplus::Color::Gray;

	Gdiplus::Color	m_cr_thumb_active = Gdiplus::Color::RoyalBlue;
	Gdiplus::Color	m_cr_thumb_current = Gdiplus::Color::Blue;
	Gdiplus::Color	m_cr_thumb_inactive = Gdiplus::Color::Gray;

	Gdiplus::Color	m_cr_thumb_outline = Gdiplus::Color::RoyalBlue;
	//완료 스텝의 체크 표시 색. Transparent 면 채움색에서 자동 파생. set_check_color 로 고정.
	Gdiplus::Color	m_cr_check = Gdiplus::Color::Transparent;
	bool			m_check_color_user_set = false;
	double			m_check_contrast = 4.5;
	Gdiplus::Color	m_cr_line_active = Gdiplus::Color::RoyalBlue;
	Gdiplus::Color	m_cr_line_inactive = Gdiplus::Color::LightGray;

	//20260728 by claude. index 스텝의 thumb 색(진행 상태 + set_thumb_color 개별 지정 반영).
	Gdiplus::Color	get_thumb_color(int index);
	//index 스텝에서 나가는 연결선의 색. 진행 중인 스텝의 나가는 선은 아직 지나지 않은 구간이라 thumb 색과 규칙이 다르다.
	Gdiplus::Color	get_line_color(int index);

	//20260729 by claude. cr 보다 offset 만큼 어두운 색. 단 이미 어두운 색은 검정에 묻히므로 방향을 뒤집어 밝게 한다.
	//기준을 128(get_weak_color) 이 아니라 96 으로 둔 것은, anysupport accent(#309AC0, luma 127) 처럼
	//중간 밝기 accent 까지는 "더 어둡게" 가 의도한 결과이기 때문이다.
	static Gdiplus::Color get_deep_color(Gdiplus::Color cr, int offset = 64);
	//채움색 cr_thumb 위에 그릴 체크 표시 색.
	Gdiplus::Color	get_check_color(Gdiplus::Color cr_thumb);

//font
	LOGFONT			m_lf;
	CFont			m_font;
	void			reconstruct_font();

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual void PreSubclassWindow();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};


