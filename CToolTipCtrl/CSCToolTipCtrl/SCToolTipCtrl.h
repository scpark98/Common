#pragma once

#include <afxcmn.h>
#include <deque>

#include "../../colors.h"
#include "../../data_structure/SCParagraph/SCParagraph.h"

/*
	CSCToolTipCtrl

	표준 CToolTipCtrl 은 단일 폰트·단색으로만 그려지므로 서식 있는 텍스트를 표시할 수 없다.
	이 클래스는 CSCStatic 과 같은 tag 문법을 툴팁에서 그대로 쓸 수 있게 한다.

		<br> <b> <i> <u> <s> <f=폰트> <sz=크기> <cr=글자색> <ct=외곽선색> <crb=배경색> <cb=칠하기색> <ls=줄간격>

	렌더링은 CSCStatic 이 쓰는 것과 동일한 CSCParagraph 의 static 함수들에 위임한다.
	따라서 CSCStatic 과 서식 결과가 항상 일치하며, CSCStatic 창을 따로 만들지 않는다.

	동작 원리
		TTN_SHOW      : 태그를 파싱해 필요한 크기를 계산하고 SetWindowPos 로 툴팁 크기를 확정한다.
		                (시스템의 크기 계산은 태그를 모르므로 그대로 두면 크기가 맞지 않는다)
		NM_CUSTOMDRAW : CDDS_PREPAINT 에서 배경·테두리·본문을 직접 그리고 CDRF_SKIPDEFAULT 를 돌려준다.

	사용법은 CToolTipCtrl 과 동일하다. AddTool / UpdateTipText 에 tag 가 포함된 문자열을 넘기면 된다.

		m_tooltip.Create(this, TTS_ALWAYSTIP);
		m_tooltip.AddTool(GetDlgItem(IDC_EDIT), _T("<b>제목</b><br><cr=gray>설명</cr>"));

	주의 : 태그를 쓰지 않는 평문도 그대로 표시된다. 기존 CToolTipCtrl 코드를 그대로 교체할 수 있다.
*/
class CSCToolTipCtrl : public CToolTipCtrl
{
	DECLARE_DYNAMIC(CSCToolTipCtrl)

public:
	CSCToolTipCtrl();
	virtual ~CSCToolTipCtrl();

	//배경·글자·테두리 색을 한 번에 지정한다. 개별 setter 대신 이것을 쓴다.
	void			set_color_theme(CSCColorTheme theme);

	//본문과 툴팁 경계 사이의 여백(px).
	void			set_padding(int cx, int cy);

	//이 폭을 넘으면 자동 줄바꿈한다(px). 0 이면 줄바꿈하지 않는다.
	//표준 CToolTipCtrl 의 SetMaxTipWidth 는 태그를 모르므로 이 값을 대신 쓴다.
	void			set_max_width(int width);

	//기본 폰트와 크기. 태그(<f=>, <sz=>)가 없는 구간에 적용된다.
	void			set_font(LPCTSTR font_name, int font_size = 9);

	//모서리 둥글기(px). 0 이면 사각.
	void			set_round(int radius);

protected:
	//툴팁 창이 뜨기 직전. 여기서 크기를 확정하지 않으면 시스템이 계산한(태그를 포함한 원문 기준)
	//엉뚱한 크기로 뜬다.
	afx_msg void	on_show(NMHDR* nmhdr, LRESULT* result);

	//배경·테두리·본문을 직접 그린다.
	afx_msg void	on_custom_draw(NMHDR* nmhdr, LRESULT* result);

	DECLARE_MESSAGE_MAP()

protected:
	//현재 표시하려는 툴팁 문자열을 TTM_GETTEXT 로 얻는다.
	CString			get_current_text();

	//text 를 파싱해 m_para 를 채우고 그려질 크기를 돌려준다.
	//dc 는 폭 계산에 필요하다(NULL 이면 이 컨트롤의 DC 를 쓴다).
	CRect			build(CString text, CDC* dc);

	std::deque<std::deque<CSCParagraph>>	m_para;
	CSCTextProperty							m_text_prop;
	CSCColorTheme							m_theme = CSCColorTheme(this);

	int				m_padding_cx = 8;
	int				m_padding_cy = 6;
	int				m_max_width = 420;
	int				m_round = 4;

	//on_show 에서 계산한 크기를 on_custom_draw 가 다시 쓰기 위해 보관한다.
	CSize			m_sz_content;
};
