#pragma once

#include <deque>
#include "../SCStatic/SCStatic.h"
#include "../../data_structure/SCParagraph/SCParagraph.h"

/*
* <name=궁서><b><cr=Red><u>This</b></cr> <size=18>is</size></u> a<br><crb=Red><i><size=14>샘</i>플</size></name><cr=Green><b><size=30>pa</fontname><cr=Yellow>ra<s>gr</cr>a</s>ph</b>.
* 
* 위와 같이 태그를 사용하여 텍스트를 표현할 수 있다.
* 
* 태그는 소문자로, 컬러값은 Gdiplus::Color에 정의된 이름을 사용해야 한다.
* 태그명에 공백, 하이픈, underscore는 모두 무시된다.
* 지원하는 태그는 다음과 같다.
* <n=굴림> = <name=굴림> = <font name=굴림> = <font_name=굴림>
* <sz=10> = <size=10> = <font size=10> = <font_size=10>
* <b>						//bold
* <i>						//italic
* <u>						//underline
* <s>						//strike out
* <cr=Red> = <ct=Red>		//text color. Red는 Gdiplus::Color에 정의된 색상 이름을 사용한다.
* <crb=Blue> = <cb=Blue>	//text back color
* <br>						//line break
* 
* 흐름 : set_text() -> get_tag_str() -> recalc_text_size() -> OnPaint()
* recalc_text_size()에서 각 항목이 출력될 위치까지 모두 계산되고 OnPaint()에서는 정해진 위치에 그리기만 한다.

* cr은 Gdiplus::Color에 명시된 컬러 외에 다음 형식들도 지원한다.
	<cr=#RRGGBBAA>,		//rgba, #으로 시작되는 16진수값.
	<cr=123,45,67,128>	//rgba, 숫자로만 3자리 또는 4자리를 콤마로 표현한다.
	<cr=h90,30,100>,	//hsi	h로 시작되고 콤마로 표현한다.
*
* [수정할 내용]
* - recalc_text_rect 및 OnPaint()에서 CDC가 아닌 Gdiplus로 모두 전환하고자 했으나 GetTextExtent()와 MeasureString()의 차이 문제로 우선 보류중.
* - outline <o>
* - shadow <sd>
* - <sdc=Red>		//shadow color
* - <sdd=3>			//shadow depth
*/

// CSCParagraphStatic은 CSCStatic을 상속받아 만들었으나 OnPaint()가 완전히 다르므로
//필요한 부분은 CSCParagraphStatic에 다시 구현해줘야 한다.
class CSCParagraphStatic : public CSCStatic
{
	DECLARE_DYNAMIC(CSCParagraphStatic)

public:
	CSCParagraphStatic();
	virtual ~CSCParagraphStatic();

	//태그는 소문자로, 컬러값은 Gdiplus::Color에 정의된 이름을 사용해야 한다.
	//태그명에 공백, 하이픈, underscore는 모두 무시된다.
	//지원하는 태그는 다음과 같다.
	//<f=굴림> = <font=굴림> = <font name=굴림> = <font_name=굴림>
	//<sz=10> = <size=10> = <font size=10> = <font_size=10>
	//<b>						//bold
	//<i>						//italic
	//<u>						//underline
	//<s>						//strike out
	//<cr=Red> = <ct=Red>		//text color. Red는 Gdiplus::Color에 정의된 색상 이름을 사용한다.
	//<crb=Blue> = <cb=Blue>	//text back color.
	//"<b><cr=Red>This</b></cr > is a <ct=Blue><i>sample</i> <b>paragraph</b>."
	CRect			set_text(CString sText, Gdiplus::Color cr_text = Gdiplus::Color::Transparent);

	//마우스가 hover된 음절에 사각형 표시
	void			draw_word_hover_rect(bool draw = true, Gdiplus::Color cr_rect = Gdiplus::Color::Transparent);

protected:
	std::deque<std::deque<CSCParagraph>> m_para;	//m_para[0][1] : 0번 라인의 1번 인덱스 음절

	CRect			recalc_text_size();

	//m_para[line][index]의 설정값대로 font를 SelectObject()하고 pOldFont를 리턴한다.
	CFont*			select_paragraph_font(int line, int index, CDC* pDC, CFont* font);
	void			get_paragraph_font(int line, int index, Gdiplus::Graphics& g, Gdiplus::Font** font);

	int				m_max_width;			//각 라인들 중 최대 너비
	int				m_max_width_line;		//각 라인들 중 최대 너비인 라인 번호
	CPoint			m_pt_icon;

//마우스가 hover된 단어를 표시
	bool			m_draw_word_hover_rect = false;				//마우스가 hover된 단어를 표시할 지
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

