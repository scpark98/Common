#pragma once

#include <deque>
#include <map>
#include "../SCStatic/SCStatic.h"
#include "../../data_structure/SCParagraph/SCParagraph.h"

/*
[Usage]
- SCParagraph.h 참조.

*/

// CSCParagraphStatic은 CSCStatic을 상속받아 만들었으나 OnPaint()가 완전히 다르므로
//필요한 부분은 CSCParagraphStatic에 다시 구현해줘야 한다.
class CSCParagraphStatic : public CSCStatic
{
	DECLARE_DYNAMIC(CSCParagraphStatic)

public:
	CSCParagraphStatic();
	virtual ~CSCParagraphStatic();

	//tag가 포함된 모든 텍스트를 설정한다. 기본 글자색, 폰트 등 설정값을 모두 한 후에 이 함수를 호출해야 한다.
	CRect			set_text(CString sText);
	void			set_textf(LPCTSTR format, ...);

//align
	//align 에 halign(DT_LEFT|DT_CENTER|DT_RIGHT) 과 valign(DT_TOP|DT_VCENTER|DT_BOTTOM) 을 조합해 넘길 수 있다.
	//내부적으로 m_halign / m_valign 으로 분리 저장된 뒤 layout 이 재빌드된다.
	void			set_text_align(DWORD align);
	//가로 정렬만 변경. DT_LEFT | DT_CENTER | DT_RIGHT 중 하나.
	void			set_halign(DWORD halign);
	//특정 line(>=0) 의 가로 정렬 override. global 위에 덮어쓴다. line_spacing 의 line 과 달리 0번 라인도 유효.
	void			set_halign(int line, DWORD halign);
	//세로 정렬만 변경. DT_TOP | DT_VCENTER | DT_BOTTOM 중 하나.
	void			set_valign(DWORD valign);

	DWORD			get_halign() { return m_halign; }
	//line 에 override 가 있으면 그 값을, 없으면 global m_halign 을 반환.
	DWORD			get_halign(int line);
	DWORD			get_valign() { return m_valign; }

	//라인 사이 간격 배수. 1.0f = 기본, 1.5f = 1.5배. set_text() 이후 호출하면 즉시 반영되고
	//이후 set_text() / OnSize() 로 라인 위치가 재계산될 때도 자동 재적용된다.
	void			set_line_spacing(float spacing = 1.0f);
	//line = 1에서 1은 인덱스이므로 두번째 라인(인덱스=1)의 윗 간격을 조정하는 것이다.
	//즉 해당 라인이 시작될 때 위에서 얼마나 간격을 넓힐것인지를 설정하는 것이다. line >= 1 이어야 한다.
	//line = 0이면 0번 라인이 출력되기 전의 라인 간격이라고 할 수 있으나
	//claude가 그건 아니라고 한다. 그건 top margin이란다.
	//line=3, spacing=2.0f 라고 하면 index=3, 즉 4번째 라인 앞의 공백이 두배가 되는 것이고
	//이는 결국 3번째 라인 다음에 2배 간격이 생기는 것과 같다.
	void			set_line_spacing(int line, float spacing = 1.0f);

	float			get_line_spacing() { return m_line_spacing; }
	//line 에 per-line override 가 있으면 그 값을, 없으면 global m_line_spacing 을 반환.
	float			get_line_spacing(int line);

	//마우스가 hover된 음절에 사각형 표시
	void			draw_word_hover_rect(bool draw = true, Gdiplus::Color cr_rect = Gdiplus::Color::Transparent);

	//폰트의 antialiasing으로 텍스트를 출력할지의 여부
	//일반 CStatic과 같이 작은 글자들은 그냥 출력시킬때보다 오히려 AA를 적용하면 글자가 선명하지 않고 흐릿해보이게 된다.
	//이때는 false로 사용해야 또렷하게 표시된다.
	void			set_font_antialiasing(bool antialias = true) { m_font_antialiasing = antialias; Invalidate(); }

protected:
	std::deque<std::deque<CSCParagraph>> m_para;	//m_para[0][1] : 0번 라인의 1번 인덱스 단어
	CSCTextProperty	m_text_prop;					//기본 폰트 정보
	void			update_text_property();			//현재 변경값을 m_text_prop에 갱신

	//
	bool			m_font_antialiasing = false;

	//라인 사이 간격 배수. set_text() / OnSize() 에서 calc_text_rect() 후에 자동 재적용된다.
	float			m_line_spacing = 1.5f;
	//특정 line 에만 적용되는 per-line override. key = line 인덱스(>=1), value = 해당 line 의 gap 배수.
	//비어있으면 모든 line 이 m_line_spacing 을 따른다.
	std::map<int, float> m_line_spacings;

	//m_para 가 calc_text_rect() 직후의 baseline 위치일 때 m_line_spacing + m_line_spacings 를 적용하여 최종 배치로 만든다.
	//global 값과 override 가 모두 1.0f 이면 shift 없이 m_rect_text 만 재계산해 반환.
	CRect			reapply_line_spacings();

	//전체 가로/세로 정렬.
	DWORD			m_halign = DT_LEFT;
	DWORD			m_valign = DT_TOP;
	//특정 line 의 가로 정렬 override. key = line 인덱스(>=0), value = DT_LEFT | DT_CENTER | DT_RIGHT.
	std::map<int, DWORD> m_line_haligns;

	//baseline 상태의 m_para 에 m_halign + m_line_haligns 를 적용. idempotent 아님 — baseline 에서만 호출해야 한다.
	CRect			apply_halign();
	//halign 적용 후의 m_para 에 m_valign 을 적용. 각 라인의 상대 y 는 유지한 채 블록 전체를 위/중/아래로 이동.
	CRect			apply_valign();

	//전체 재빌드: calc_text_rect(baseline) -> reapply_line_spacings -> apply_halign -> apply_valign -> Invalidate.
	//set_text, OnSize, 모든 align/line_spacing setter 가 이 경로를 탄다.
	void			rebuild_layout();

	int				m_max_width;					//각 라인들 중 최대 너비
	int				m_max_width_line;				//각 라인들 중 최대 너비인 라인 번호
	CPoint			m_pt_icon;

	bool			m_auto_ctrl_size = true;//출력된 텍스트의 크기가 컨트롤의 크기보다 클 경우 컨트롤의 크기를 자동으로 늘려준다. default = true

//마우스가 hover된 단어를 표시
	bool			m_draw_word_hover_rect = false;				//마우스가 hover된 단어를 표시할 지. default = false
	Gdiplus::Color	m_cr_word_hover_rect = Gdiplus::Color::Red;	//hover된 단어를 표시할 색상
	CPoint			m_pos_word_hover = CPoint(-1, -1);			//마우스가 위치한 단어의 i, j 인덱스 (좌표값이 아닌 인덱스)

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
	virtual void PreSubclassWindow();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
};

