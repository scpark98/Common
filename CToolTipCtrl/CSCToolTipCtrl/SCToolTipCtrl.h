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
	//20260826 by claude. 실제로 창을 둥글게 잘라내는 것은 DWM(Win11+) 이고 그 반경은 OS 가 정한다(ROUNDSMALL).
	//이 값은 "라운드를 쓸지(>0)" 와 우리가 그리는 테두리의 곡률만 결정하므로 기본값 4 근처를 권장한다.
	//Win11 미만에서는 잘라낼 방법이 없어(불투명 창) 각진 툴팁으로 그린다.
	void			set_round(int radius);

	//줄 간격 배수. 1.0 이면 폰트 기본 간격, 2.0 이면 두 배.
	void			set_line_spacing(float spacing);

	//20260826 by claude. 나타나고 사라질 때 윈도우 기본 툴팁과 같은 fade 애니메이션을 쓸지 여부.
	//on(기본) : 부드럽게 뜨고 사라진다. 대신 fade 가 끝날 때까지(수백 ms) 글자가 약간 흐리다.
	//off      : 즉시 뜨고 사라지는 대신 처음부터 글자가 또렷하다.
	//(사유는 .cpp 의 on_show 주석 참조 — fade 에 필요한 WS_EX_LAYERED 가 ClearType 을 막는다.)
	void			set_fade(bool on);

	//20260807 by claude. dlg 의 PreTranslateMessage 에서 매 메시지를 그대로 넘겨준다.
	//disabled 컨트롤은 마우스 메시지를 아예 dispatch 받지 못하므로, 컨트롤 쪽에서 relay 해봐야 소용없다.
	//dispatch 이전인 top-level dlg 의 PreTranslateMessage 에서 relay 해야만 disabled 상태에서도 툴팁이 뜬다.
	//Create 전(m_hWnd == NULL)이면 아무 일도 하지 않으므로 호출측에서 따로 가드하지 않아도 된다.
	//CSCThemeDlg 파생 dlg 는 base 가 이미 호출하므로 직접 부를 필요 없다.
	void			relay_message(MSG* pMsg);

	//글자 렌더 품질을 run 단위로 자동 결정할지 여부. CSCStatic 과 같은 기준을 쓴다.
	//on  : 크기 < 폰트별 임계 → ClearTypeGridFit(작은 글자 또렷), 크기 >= 임계 → AntiAliasGridFit(큰 글자 매끈)
	//off : ClearTypeGridFit 로 고정
	//AA_from_pt 는 폰트별 임계를 구하지 못했을 때 쓰는 fallback(pt).
	void			set_auto_font_quality(bool on, int AA_from_pt = 14);

protected:
	//툴팁 창이 뜨기 직전. 여기서 크기를 확정하지 않으면 시스템이 계산한(태그를 포함한 원문 기준)
	//엉뚱한 크기로 뜬다.
	afx_msg void	on_show(NMHDR* nmhdr, LRESULT* result);

	//배경·테두리·본문을 직접 그린다.
	afx_msg void	on_custom_draw(NMHDR* nmhdr, LRESULT* result);

	//20260826 by claude. 툴팁 창이 숨겨지기 직전. fade-out 을 위해 WS_EX_LAYERED 를 되돌려준다.
	afx_msg void	on_pop(NMHDR* nmhdr, LRESULT* result);

	//fade-in 이 끝나는 시점에 WS_EX_LAYERED 를 벗겨 글자를 또렷하게 되돌린다.
	afx_msg void	OnTimer(UINT_PTR nIDEvent);

	//배경 지우기를 막는다(깜빡임 방지). on_custom_draw 가 클라이언트 전체를 칠한다.
	afx_msg BOOL	OnEraseBkgnd(CDC* pDC);

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
	//CSCStatic 의 m_line_spacing 과 같은 기본값 — 같은 태그 문자열이 같은 모양으로 나오게 한다.
	float			m_line_spacing = 1.2f;

	//기본값은 CSCStatic 과 동일 (m_auto_font_quality = true, m_AA_from_pt = 14).
	bool			m_auto_font_quality = true;
	int				m_AA_from_pt = 14;

	//20260826 by claude. DWM 이 실제로 모서리를 잘라줬는지(Win11+). 테두리를 라운드로 그릴지 사각으로 그릴지를 정한다.
	bool			m_dwm_round = false;

	//20260826 by claude. fade 애니메이션 사용 여부. fade 종료 판정은 시간이 아니라 창의 현재 알파로 하고,
	//알파를 읽을 수 없는 경우에만 fade_max_wait 에서 끊는다(OnTimer 참조).
	//timer_unlayer 는 comctl32 가 이 창에서 쓰는 내부 타이머 id(작은 값)와 겹치지 않도록 큰 값을 쓴다.
	bool			m_fade = true;
	int				m_fade_elapsed = 0;
	enum { timer_unlayer = 0x5C10, fade_poll_interval = 30, fade_max_wait = 800 };

	//on_show 에서 계산한 크기를 on_custom_draw 가 다시 쓰기 위해 보관한다.
	CSize			m_sz_content;
};
