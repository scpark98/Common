#pragma once

#include <afxwin.h>
#include <deque>
#include "../../GdiplusBitmap.h"

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

	//thumb와 text의 색상을 모두 변경한다.
	void			set_step_color(int index, Gdiplus::Color cr);
	void			set_thumb_color(int index, Gdiplus::Color cr);
	void			set_text_color(int index, Gdiplus::Color cr);

	//특정 step의 thumb와 text의 색상을 리셋시키고 기본색을 사용하게 한다.
	//index == -1이면 모든 스텝 리셋
	void			reset_step_color(int index = -1);

	//각 스텝의 thumb 크기
	void			set_thumb_size(int size);

	void			set_draw_line(bool draw) { m_draw_line = draw; }

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
	Gdiplus::Color	m_cr_back;	//보통 dlg를 만들면 배경이 GetSysColor(COLOR_3DFACE)인데 이 값을 여기서는 초기화 할 수 없고 생성자에서 해야 함.

	Gdiplus::Color	m_cr_text_active = Gdiplus::Color::RoyalBlue;
	Gdiplus::Color	m_cr_text_current = Gdiplus::Color::Blue;
	Gdiplus::Color	m_cr_text_inactive = Gdiplus::Color::Gray;

	Gdiplus::Color	m_cr_thumb_active = Gdiplus::Color::RoyalBlue;
	Gdiplus::Color	m_cr_thumb_current = Gdiplus::Color::Blue;
	Gdiplus::Color	m_cr_thumb_inactive = Gdiplus::Color::Gray;

	Gdiplus::Color	m_cr_thumb_outline = Gdiplus::Color::RoyalBlue;
	Gdiplus::Color	m_cr_line_active = Gdiplus::Color::RoyalBlue;
	Gdiplus::Color	m_cr_line_inactive = Gdiplus::Color::LightGray;

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


