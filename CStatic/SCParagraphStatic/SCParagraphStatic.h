#pragma once

#include <deque>
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
	void			set_text_align(DWORD align);

	//라인 사이 간격 배수. 1.0f = 기본, 1.5f = 1.5배. set_text() 이후 호출하면 즉시 반영되고
	//이후 set_text() / OnSize() 로 라인 위치가 재계산될 때도 자동 재적용된다.
	void			set_line_spacing(float spacing = 1.0f);
	float			get_line_spacing() { return m_line_spacing; }

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

	DWORD			m_static_option;				//SS_LEFT -> DT_LEFT 등 옵션값 저장
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

