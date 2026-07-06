//Test Project: https://github.com/scpark98/Test_CEdit.git

#if !defined(AFX_SCSTATIC_INCLUDED_)
#define AFX_SCSTATIC_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#pragma once

#include <afxwin.h>
#include <deque>
#include <map>

//.ico가 아닌 png 이미지들을 앞에 그려주고 필요에 따라 변경되도록 하기 위해 사용.
#include "../../SCGdiplusBitmap.h"
#include "../../data_structure/SCParagraph/SCParagraph.h"
#include "../../CEdit/CSCStaticEdit/SCStaticEdit.h"
/*
//scpark

[gradient usage]
m_static1.set_gradient();
m_static1.set_gradient_color(RED);
//m_static1.set_vertical_gradient);
//m_static1.set_gradient_color(4, RED, GREEN, YELLOW, BLUE);

[주의사항]
- parent의 WM_SIZE에 의해 CSCStatic의 인스턴스도 resize할 경우 깜빡인다면
  parent 속성에서 Clip_Children을 true로 설정해야 깜빡이지 않는다.

[투명처리 규칙]
- 사각형 영역 배경색이 정해지면 불투명으로 처리된다.
- m_round > 0 이거나 투명 이미지를 표시할 경우는 먼저 parent_back을 칠해줘야 한다.

- parent에 배경 그림이 있을 경우는 parent_back_image를 알고 있어야 하고 컨트롤의 위치에 해당하는 이미지를 crop해서
  먼저 그려준 후에 static을 표현해야 한다.
  물론 parent가 resize를 지원하거나 parent_back_image가 1:1크기로 그려진 게 아니라면 이마저도 불가하다.
  이럴 경우 parent DC를 얻어와서 해당 배경 정보를 추출하고 이를 그려준 후 static을 그려주는 코드를 사용했었으나
  깜빡임이 발생하기도 하고 투명 이미지를 덮어써서 그리는 이벤트가 발생되면 투명정보가 점점 불투명해지는 부작용이 있다.
  배경 그림이 있거나 그림의 크기가 1:1이 아닌 경우는 우선 현재 시점에서는 배재시킨다.

[이미지 추가 시]
- set_icon()을 이용해서 텍스트 앞에 아이콘을 표시할 수 있다.
- add_header_image()를 이용해서 텍스트 앞에 이미지를 표시할 수 있다.
  set_icon()과 다른 점은 .ico 파일이 아닌 png 등의 이미지 포맷을 지원하고
  2장 이상의 이미지를 추가한 후 필요에 따라 이미지를 변경할 수도 있다.
- 두 함수 모두 left_align_fix를 true로 설정하면 이미지는 왼쪽 정렬로 고정되고 텍스트는 설정된 정렬방식으로 표시된다.
  이 파라미터를 추가한 이유 CSCMessageBox에서 이 CSCStatic을 사용하는데 메시지박스를 보면 아이콘은 항상 왼쪽에, 텍스트는 왼쪽 또는 중앙정렬로 표시된다.
  false일 경우는 텍스트의 위치에 따라 아이콘과 이미지의 위치가 변경된다.

[label + value 형태로 사용할 때]
- set_use_edit(true)를 하면 click으로 편집이 가능해진다. 편집이 시작되면 내부적으로 CSCStaticEdit 컨트롤이 표시되고 편집이 끝나면 숨겨진다.
- Resource Editor에서 해당 static의 속성에서 "이미지 가운데 맞춤" 선택, 컨트롤의 높이는 default로 8 DLU로 생성되지만 12 DLU로 조정해준다.


[2014-11-26]

[수정될 내용]
- CSCStatic과 CSCParagraphStatic을 합친다? CSCTagStatic 이름변경?
  tag가 포함되었느냐에 따라 OnPaint() 등이 달라진다.

- 배경이 있는 앱에서는 투명이 잘 적용되지만 기본 스타일의 dlg에서는 화면 갱신이 잘 되지 않는다.
  우선 기본 스타일의 dlg에서는 배경색을 지정해주자.
- 출력크기보다 rc가 작으면 키워줘야 한다?
- "\r\n"으로 multiline을 출력할 경우 center image, no wrap, simple 속성은 반드시 false가 되어야 한다.
  따라서 이러한 속성을 살리기 위해서는 파싱하여 각 라인을 출력하도록 수정이 필요하다.

- set_text(_T("<cr=red>test</cr>"))와 같이 html 태그를 지원하려면? <b>, <i>, <u>, <s> 정도 지원 예정.
  태그가 있는 텍스트와 없는 텍스트를 혼합하여 출력할 경우는 paragraph 구조체가 필요하다.

[2014-11-27]
- blink시에 아이콘도 함께 적용되지 않던 문제 수정
- blink를 false로 할 경우 숨겨진 상태로 blink가 종료되던 문제 수정

*/

// CSCStatic

typedef UINT (CALLBACK* LPFNDLLFUNC1)(HDC,CONST PTRIVERTEX,DWORD,CONST PVOID,DWORD,DWORD);

static const UINT Message_CSCStatic = ::RegisterWindowMessage(_T("MessageString_CSCStatic"));

class CSCStatic;

class CSCStaticMsg
{
public:
	CSCStaticMsg(int _msg, CSCStatic* _this, CString  _sValue = _T(""), int _nValue = 0)
	{
		pThis = _this;
		msg = _msg;
		sValue = _sValue;
		nValue = _nValue;
	}

	enum CSCStaticMsgs
	{
		//color picker 모드는 "R, G, B" (color24) 또는 "R, G, B, A" (color32) 를 sValue 로 전파한다.
		msg_text_value_changed,
	};

	CSCStatic* pThis = NULL;
	int		msg;
	CString	sValue;
	int		nValue;
};

class CSCStatic : public CStatic
{
	DECLARE_DYNAMIC(CSCStatic)

public:
	CSCStatic();
	~CSCStatic();


	//동적생성시에 사용
	BOOL			create(LPCTSTR lpszText, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID = 0xffff);
	
	//설정된 속성들을 동일한 종류의 대상 컨트롤에 그대로 적용할 때 사용된다. text는 제외된다.
	void			copy_properties(CSCStatic& dst);

	void			set_transparent(bool bTransparent = true, Gdiplus::Color cr_parent_back = Gdiplus::Color::Transparent);
	void			SetWindowText(CString sText) { set_text(sText); }

	CString			get_text();

	//text는 일반 문자열 또는 tag가 포함된 문자열이 될 수 있다.
	CRect			set_text(CString text, Gdiplus::Color cr_text = Gdiplus::Color::Transparent);

	//printf()와 같이 변수값을 바로 문자열로 세팅할 수 있다.
	//초기 버전에서는 첫번째 파라미터로 컬러를 주고 그 값이 -1이면 default text color를 사용하도록 구현했었으나
	//-1 비교가 제대로 되지 않아 우선 제거함.
	void			set_textf(LPCTSTR format, ...);

//tagged text (paragraph mode)
	//tag(<br><b><i><u><s><f=><sz=><cr=><ct=><crb=><cb=><ls=>)가 포함된 텍스트를 설정한다.
	//set_text 와 상호 배타 — 한 쪽 호출 시 다른 쪽 데이터가 무효화된다. 기본 글자색·폰트 등 속성은
	//이 함수 호출 *전*에 모두 설정되어 있어야 m_text_prop 에 반영된다.
	//단락 모드에서도 부모의 round / back fill / border / header-image 비주얼 기능이 그대로 작동한다.
	CRect			set_tagged_text(CString text);

	//특정 라인의 세로 정렬 지정(한 라인 안에 크기가 다른 run 이 있을 때 유용). line_idx < 0 = 모든 라인. align = DT_TOP/DT_VCENTER/DT_BOTTOM.
	//set_tagged_text 로 텍스트를 세팅(레이아웃)한 *뒤* 에 호출한다(m_para 필요). 값은 para 에 저장돼 재레이아웃돼도 유지된다.
	void			set_line_align(int line_idx, DWORD align);

	//halign + valign 을 한 번에 설정. DT_LEFT|DT_CENTER|DT_RIGHT + DT_TOP|DT_VCENTER|DT_BOTTOM 조합.
	//내부에서 m_halign / m_valign 으로 분리 저장된 뒤 (단락 모드면) layout 이 재빌드된다.
	void			set_text_align(DWORD align);

	//라인 사이 간격 배수. 단락 모드 전용. 1.0f = 기본, 1.5f = 1.5배.
	void			set_line_spacing(float spacing = 1.0f);
	//특정 라인의 위쪽 간격 배수. line >= 1.
	void			set_line_spacing(int line, float spacing = 1.0f);
	float			get_line_spacing() { return m_line_spacing; }
	float			get_line_spacing(int line);

	//특정 라인의 가로 정렬 override. 단락 모드 전용. global m_halign 위에 덮어쓴다.
	//set_halign(DWORD) 와 시그니처 구분.
	void			set_halign_line(int line, DWORD halign);
	DWORD			get_halign_line(int line);

	//단락 모드에서 마우스가 hover 된 단어에 사각형 표시.
	void			draw_word_hover_rect(bool draw = true, Gdiplus::Color cr_rect = Gdiplus::Color::Transparent);

	//단락 모드의 텍스트 antialiasing. 작은 글자(예: 9pt)는 false 로 두면 더 또렷.
	void			set_font_antialiasing(bool antialias = true) { m_font_antialiasing = antialias; Invalidate(); }

	//단락 모드의 음절별 hint 자동 결정. 음절 폰트 size(pt) 가 AA_from_pt 이상이면 AntiAliasGridFit(큰 글씨 매끈),
	//미만이면 ClearTypeGridFit(작은 글씨 또렷). 다중 size 가 섞인 태그 텍스트(<sz=24>+default 9pt)에서 자연스러움.
	//on=false 면 OnPaint 의 단일 hint(ClearTypeGridFit) 유지.
	//AA_from_pt 는 baseline — 실제 임계치는 CSCParagraph::get_AA_from_pt 가 폰트 EBLC 메트릭과
	//다크 배경 여부를 고려해 자동 보정한다.
	void			set_font_quality_auto(bool on = true, int AA_from_pt = 14)
					{ m_auto_font_quality = on; m_AA_from_pt = AA_from_pt; Invalidate(); }


//자체 편집 기능
	//편집 기능 허용. click으로 편집시작, esc, return, 다른 항목 클릭으로 편집 종료된다.
	void			set_use_edit(bool use = true, UINT align = ES_RIGHT);
	//편집 시 표시되는 edit의 width를 지정한다. 0 또는 음수값이면 텍스트의 너비 + 8만큼 edit의 width가 지정된다. (편집이 시작될 때 계산되어 적용된다.)
	void			set_edit_width(int width);
	CString			get_text_value() { return m_text_value; }
	void			set_text_value(CString value = _T(""));
	void			set_text_value(LPCTSTR format, ...);

	//label + value로 표시하는 경우 value 편집할 때 CSCStaticEdit의 색상을 지정한다.
	void			set_edit_text_color(Gdiplus::Color cr_edit_text = Gdiplus::Color::Transparent);
	void			set_edit_back_color(Gdiplus::Color cr_edit_back = Gdiplus::Color::Transparent);
	void			edit_begin();
	void			edit_end(bool valid);
	LRESULT			on_message_CSCStaticEdit(WPARAM wParam, LPARAM lParam);

	//color picker 전용. swatch 옆 "R, G, B" (color24) 또는 "R, G, B, A" (color32) 단일 텍스트를 편집한다.
	void			edit_begin_color_text();
	void			edit_end_color_text(bool valid);

	//편집 중 Shift + Up/Down 또는 Shift + MouseWheel 로 숫자값 증감.
	//텍스트가 정수/실수로 파싱되는 경우에만 동작 (그 외엔 base 위임).
	//set_use_edit() 보다 먼저 호출되어도 m_edit 인스턴스 자체엔 적용되며, 이후 create 시에도 유지.
	void			set_use_updown_key(bool use = true, float interval = 1.0f)
					{ m_edit.set_use_updown_key(use, interval); }


	//CStatic의 SS_LEFT, SS_RIGHT 등의 align 설정값을 DrawText()에서 사용하는 DT_LEFT, DT_RIGHT 등으로 치환하여 리턴
	DWORD			get_text_align();

	//VALIGN이 SS_CENTERIMAGE가 아니면 상단정렬 둘 뿐이므로 DT_TOP, DT_VCENTER, DT_BOTTOM으로 정렬할 수 있도록 확장한다.
	int				get_halign();
	int				get_valign();

	//horizontal align을 동적으로 변경한다. halign은 DT_LEFT|DT_CENTER|DT_RIGHT 중 하나.
	//원래 CStatic 에는 SS_BOTTOM 같은 세로 하단 정렬 스타일이 없으나, 이 컨트롤은 m_text_rect 좌표를 직접 보정하는 방식으로 DT_BOTTOM 도 지원한다.
	void			set_halign(DWORD halign);
	//vertical align을 동적으로 변경한다. valign은 DT_TOP|DT_VCENTER|DT_BOTTOM 중 하나.
	//원래 CStatic 에는 SS_BOTTOM 같은 세로 하단 정렬 스타일이 없으나, 이 컨트롤은 m_text_rect 좌표를 직접 보정하는 방식으로 DT_BOTTOM 도 지원한다.
	void			set_valign(DWORD valign);

	//텍스트의 '&' 를 니모닉 접두(prefix)로 처리할지 여부. 기본은 스타일(SS_NOPREFIX)에 따름.
	//파일명 등 '&' 를 글자 그대로 표시해야 하는 경우 set_no_prefix(true) 로 강제한다(DT_NOPREFIX).
	void			set_no_prefix(bool no_prefix = true) { m_no_prefix = no_prefix; if (::IsWindow(GetSafeHwnd())) Invalidate(); }

	//label + value 형태로 표시될 때 value 의 가로 정렬. DT_LEFT|DT_CENTER|DT_RIGHT 중 하나. 기본값 DT_RIGHT.
	//편집 모드 진입 시 CSCStaticEdit 의 텍스트 정렬에도 자동 반영된다.
	void			set_value_halign(DWORD halign);

	//label + value 형태일 때 label 영역의 고정 너비. 0 (기본) 이면 텍스트 길이 + 8 픽셀 자동 계산.
	//여러 항목을 같은 컬럼에 정렬하려면 동일 값으로 호출. value/edit 의 시작 위치가 일치한다.
	void			set_label_width(int width) { m_label_width = width; Invalidate(); }

	//set_use_edit + 값(m_text_value) 이 있는 라벨+값 셀에서 라벨이 m_label_width 컬럼 안에 안 들어갈 때의 처리 방식.
	//
	//  false (default):
	//      기본 동작. 라벨이 m_label_width 를 넘어도 잘리지 않고 그대로 그려진다(WORDBREAK 적용으로 다음 줄로
	//      넘어갈 수도 있음). 라벨 컬럼 폭 vs 라벨 측정폭의 비교만 본다 — 값과의 관계는 고려 안 함.
	//
	//  true:
	//      라벨을 한 줄로 표시하되, *값과 겹치지 않게* 라벨 right 를 동적으로 제한한 뒤 "..." (ellipsis) 처리한다.
	//      라벨 right = 값의 실제 left(우측 정렬 값은 = 셀 right - value 측정폭 - 우측 여백) - 4px gap.
	//      즉 *값이 짧으면 라벨이 m_label_width 컬럼 경계를 넘어 값 직전까지 자유롭게* 그려지고, *값이 길어져
	//      라벨을 밀어내야 비로소* ellipsis 발동. 값 영역과의 시각적 충돌은 절대 발생하지 않는다. 대신 라벨 컬럼
	//      정렬(여러 셀의 라벨 right 가 한 줄로 맞춰지는 시각) 은 일부 양보된다.
	//      값 영역이 동적으로 변하는 라벨+값 패턴(예: CSCPropertyCtrl 의 셀)에 적합.
	void			set_label_auto_ellipsis(bool on = true) { m_label_auto_ellipsis = on; Invalidate(); }

	//color picker 모드에서 alpha 표시·편집 여부.
	//true(=color32) → 값이 "R, G, B, A", false(=color24) → "R, G, B" (alpha 무시·불투명 취급).
	void			set_show_alpha(bool show) { m_show_alpha = show; Invalidate(); }

	//color picker 모드로 동작시킨다. true 면 라벨(있으면) 우측에 색상 swatch + "R,G,B[,A]" 값을 그리고
	//swatch 클릭→CSCColorPicker, 값 클릭→직접 편집. caption "_color picker_" 로 줘도 자동 활성화된다(하위호환).
	//라벨이 비어 있어도 swatch+값은 그려진다. swatch/값 색은 라벨색(m_theme.cr_text) 과 분리된 m_cr_color_picker
	//에 저장하므로 라벨과 색이 충돌하지 않는다(= 한 컨트롤로 "Background ■ 0,0,0" 표현 가능).
	void			set_as_color_picker(bool as = true);
	bool			is_color_picker() const { return m_as_color_picker; }

	//텍스트가 실제 출력되는 tight rect. plain/단락 모드 모두 정확. 너비만 필요하면 .Width().
	CRect			get_text_rect() { return m_text_rect; }
	//너비/높이가 필요할 때. 내부적으로 get_text_rect().Size() 를 돌려준다.
	CSize			get_text_extent();

	//parent에서 현재 이 static의 위치를 리턴.
	CRect			get_rect();

	//color theme 설정
	CSCColorTheme	m_theme = CSCColorTheme(this);
	void			set_color_theme(int theme, bool invalidate = true);
	//부모 dlg 의 m_theme 객체를 그대로 전달받는 경로 — 커스터마이즈된 색까지 자식이 반영하기 위해 사용.
	void			set_color_theme(const CSCColorTheme& theme, bool invalidate = true);

	//글자색, 배경색 동시 설정
	void			set_color(Gdiplus::Color cr_text, Gdiplus::Color cr_back);
	void			set_text_color(Gdiplus::Color cr_text);
	Gdiplus::Color	get_text_color() { return m_as_color_picker ? m_cr_color_picker : m_theme.cr_text; }
	Gdiplus::Color	get_back_color() { return m_theme.cr_back; }
	void			set_back_color(Gdiplus::Color cr_back);

	//gradient
	//m_crGradient deque에 색상들을 넣고 n등분하여 gradient fill을 해서 표시한다.
	void			set_gradient(bool bGradient = true);
	void			set_gradient_color(Gdiplus::Color crGradient);			//배경~crGradient 2컬러로 표현됨.
	void			set_gradient_color(int idx, Gdiplus::Color crGradient);	//idx의 색상을 crGradient로 변경. idx가 없으면 idx까지 crGradient로 채움.

	template <typename ... T> CSCStatic& set_gradient_color(T... colors)
	{
		set_gradient();
		m_crGradient.clear();

		int n = sizeof...(colors);
		int arg[] = { colors... };
		for (auto cr : arg)
		{
			m_crGradient.push_back(cr);
		}

		return *this;
	}

	void			add_gradient_color(Gdiplus::Color crGradient);			//색상 추가
	void			insert_gradient_color(int idx, Gdiplus::Color crGradient);//idx번째 색상 추가
	void			set_vertical_gradient(bool bVertical = true);

	void			sunken(bool sunken = true) { m_bSunken = sunken; Invalidate(); }


//marquee효과를 위해서 간단히 테스트해 보았으나 부드럽게 표현하기 위해서는
//rc의 width 또는 height 크기에 적당할 정도로 m_crGradient를 자동으로 채워준 후에 돌려야한다.
//필요성이 있나 싶어서 일단 보류한다.
	//start = 1:start, 0:stop, -1:toggle
	bool			m_start_marquee;
	//void			start_marquee(int start);

	//gif도 가능.
	//m_static_copy.set_back_image(_T("GIF"), IDR_GIF_COPY, Gdiplus::Color::White);
	//만약 원본 크기가 static ctrl의 크기보다 클 경우는 보이지 않게 되므로
	//m_static_copy.fit_to_back_image(false);를 호출해서 이미지 출력 크기를 컨트롤에 맞춰서 출력되도록 해야 한다.
	void			set_back_image(CString type, UINT nIDBack, Gdiplus::Color cr_back = Gdiplus::Color::Transparent);
	void			set_back_image(CSCGdiplusBitmap& img, Gdiplus::Color cr_back = Gdiplus::Color::Transparent);

	//배경 이미지를 좌우대칭하는데 만약 animated gif라면 역재생처럼 동작시킬 수 있다.
	void			set_back_image_mirror(bool is_mirror);

	//배경 이미지 크기에 맞게 컨트롤을 resize한다.
	//fit_to_image = false이면 컨트롤의 크기에 맞게 이미지를 resize한다.
	void			fit_to_back_image(bool fit_to_image = true);
	//m_fit_to_back_image = false이면 컨트롤이 resize될 때 이미지도 resize된다.
	bool			m_fit_to_back_image = false;

	int				m_round = 0;				//round rect
	void			set_round(int round, Gdiplus::Color cr_border = Gdiplus::Color::Transparent, Gdiplus::Color gcr_parent_back = Gdiplus::Color::Transparent);

	void			get_auto_font_size(CWnd* pWnd, CRect r, CString text, LOGFONT *lf);

	//기본 CWnd::SetFont() override
	void			SetFont(CFont* font, BOOL bRedraw = TRUE);
	void			set_font_name(const CString& strFont, BYTE byCharSet = DEFAULT_CHARSET);
	void			set_font_size(int nFontSize);
	void			enlarge_font_size(bool enlarge);
	void			set_auto_font_size(bool auto_font_size = true);
	//default bold = FW_BOLD
	void			set_font_weight(int weight = FW_BOLD);
	void			set_font_underline(bool underline = true);
	void			set_font_italic(bool italic = true);
	void			set_font_antialiased(bool antiAliased = true);

	//void			set_font_width(int nFontWidth) { m_nFontWidth = nFontWidth; Invalidate(); }
	//bool			is_bold() { return m_bFontBold; }
	void			set_font(CFont* font);

	void			set_border_thick(int thick) { m_border_thick = thick; Invalidate(); }
	void			draw_border(bool draw, int thick, Gdiplus::Color cr_outline) { m_draw_border = draw; m_border_thick = thick; m_theme.cr_border_inactive = cr_outline; Invalidate(); }

	void			set_blink_time(int nTime0 = 400, int nTime1 = 1200);
	void			set_blink(bool blink = true, int time0 = 400, int time1 = 400);

	//CStatic의 클릭 이벤트를 처리하려면 SS_NOTIFY 속성 필수이므로 자동 설정되도록 수정함.
	void			set_link(CString url, Gdiplus::Color cr_link = Gdiplus::Color::RoyalBlue);

	//static의 맨 앞에 nSpace 개수만큼의 공백을 추가하여 출력한다.(= left margin)
	void			set_prefix_space(int nSpace = 1) { m_nPrefixSpace = nSpace; Invalidate(); }
	//use_edit 값(특히 우측정렬)의 우측 여백(px). 정적 표시(OnPaint)와 편집 박스(edit_begin) 양쪽에
	//동일하게 적용되어, 값이 박스 우측에 너무 붙지 않게 하면서 편집 전후 정렬이 유지된다.
	void			set_value_right_space(int n = 0) { m_value_right_space = n; Invalidate(); }

	void			set_margin(int l, int t, int r, int b) { set_margin(CRect(l, t, r, b)); }
	void			set_margin(CRect margin);
	void			get_margin(CSize sz_margin) { set_margin(CRect(sz_margin.cx, sz_margin.cy, sz_margin.cx, sz_margin.cy)); }

	void			set_icon(UINT nIDResource, int nSize = 16, bool left_align_fix = false);
	void			set_icon(HICON hIcon, int nSize = 16, bool left_align_fix = false);

	//png 이미지를 label의 앞에 표시한다. 2장 이상일 경우 alt효과를 줄 수 있다. id가 0이면 clear()로 동작한다.
	void			add_header_image(UINT id, bool left_align_fix = false);
	//20260705 by claude. 아이콘/헤더이미지와 라벨(텍스트) 사이의 간격(px). 기본 4. (예전엔 OnPaint 에 gap=4 로 하드코딩돼 조정 불가였음.)
	void			set_header_gap(int gap) { m_header_gap = gap; if (::IsWindow(GetSafeHwnd())) Invalidate(); }
	int				get_header_gap() { return m_header_gap; }
	//png 이미지를 label의 앞에 표시한다. 2장 이상일 경우 alt효과를 줄 수 있다.
	template <typename ... T> void set_header_images(T... args)
	{
		int n = sizeof...(args);
		int arg[] = { args... };

		//set_이므로 기존 항목들을 모두 제거한 후 추가해야 한다.
		//그냥 하나를 추가한다면 add_header_image()를 사용한다.
		for (int i = 0; i < m_header_images.size(); i++)
		{
			CSCGdiplusBitmap* img = m_header_images[i];
			delete img;
		}

		m_header_images.clear();

		for (auto id : arg)
		{
			//여기서 직접 new로 할당받고 load하여 deque에 넣으려했으나
			//여기서는 들어간 것처럼 보였지만 실제 OnPaint()에서 보면 deque가 비어있었다.
			//template이라 그런지 여기서 바로는 들어가지 않는다.
			//멤버함수를 통해 넣어야 한다.
			add_header_image(id);
		}
	}

//animated gif 관련
	void			play_gif();
	//pos위치로 이동한 후 일시정지한다. -1이면 pause <-> play를 토글한다.
	void			pause_gif(int pos = 0);
	//animation thread가 종료되고 화면에도 더 이상 표시되지 않는다. 만약 그대로 멈추길 원한다면 pause_animation()을 호출한다.
	void			stop_gif();

	HICON			m_hIcon;
	CSize			m_sz_icon;

//tooltip
	//기본적인 툴팁은 이 컨트롤 내에서 지원하지만
	//disabled인 컨트롤은 main의 PreTranslateMessage()에서 처리하지 않으면 나타나지 않는다.
	void			use_tooltip(bool use) { m_use_tooltip = use; }
	void			set_tooltip_text(CString text);


protected:
	//SetWindowText(), GetWindowText()를 쓰면 m_text를 굳이 선언해서 사용안해도 될 듯 하지만
	//SetWindowText()를 호출하는 순간 화면갱신이 일어나고 MFC내부적으로는 많은 처리를 할 것이다.
	//특히 transparent일 경우는 그 깜빡임이 크므로 m_text를 별도 선언하여 사용한다.
	CString			m_text;

	//텍스트가 실제 출력되는 tight rect. plain(set_text)·단락(set_tagged_text) 모드 모두
	//OnPaint / rebuild_layout 에서 실제 그려진 텍스트 영역으로 세팅된다.
	//너비가 필요하면 m_text_rect.Width() (또는 get_text_extent()) 사용 — 별도 m_text_extent 는 제거됨.
	CRect			m_text_rect;

	CSCStaticEdit	m_edit;
	bool			m_use_edit = false;
	CString			m_text_value;		//m_use_edit = true이면 label + value로 표시되는데 이 때 value의 내용이 저장된다.
	CString			m_edit_begin_value;	//edit_begin 시점의 m_text_value snapshot — edit_end 에서 실제 변경 여부 비교용(미변경 시 통지 생략).
	int				m_edit_width = 0;	//편집모드일 때 edit의 너비. 이 값이 0이면 (m_text_rect.Width() + 8)부터 edit이 시작된다.

	//color picker 모드 전용 layout 계산. swatch / 값 텍스트 영역.
	void			get_color_picker_layout(CRect& rc_swatch, CRect& rc_text);
	//color picker 모드의 값 문자열. m_show_alpha(=color32) 면 "R, G, B, A", 아니면(color24) "R, G, B".
	CString			format_color_text() const;

	enum ENUM_TIMER
	{
		TIMER_BLINK = 0,
		TIMER_MARQUEE,
	};

	CBitmap			m_Bmp;

	bool			m_auto_font_size = false;
	
	//font관련 변수는 모두 m_lf로 대체함.
	//CString			m_sFontName;
	//int				m_nFontSize;
	//int				m_nFontWidth;
	//bool			m_bFontBold;
	//bool			m_bFontUnderline;
	//bool			m_bFontAntiAliased;

	//gradient background fill
	std::deque<Gdiplus::Color> m_crGradient;
	bool			m_bGradient;		//gradient background?
	bool			m_bVertical;		//vertical gradient?


	//this function will be used only if msimg32.dll library is not available
	void			draw_gradient_rect(CDC *pDC, CRect r, Gdiplus::Color cLeft, Gdiplus::Color cRight, BOOL a_bVertical);


	bool			m_draw_border = false;
 	int				m_border_thick = 1;
	//Gdiplus::Color	m_cr_border = Gdiplus::Color::DimGray;

	bool			m_bSunken;			//default = false;

	//Gdiplus::Color	m_cr_text = Gdiplus::Color::Black;
	//Gdiplus::Color	m_cr_back = Gdiplus::Color::White;
	//Gdiplus::Color	m_cr_edit_text = gGRAY(32);	//편집모드일때 edit의 text color
	//Gdiplus::Color	m_cr_edit_back = m_cr_back;	//편집모드일때 edit의 back color. 기본값은 m_cr_back과 동일하다.
	bool			m_transparent;		//default = false
	//Gdiplus::Color	m_cr_parent_back = Gdiplus::Color::Transparent;

	BOOL			m_bBlink;
	BOOL			m_blink_status;
	int				m_nBlinkTime0;		//blink type is Show/Hide, time0 = shown duration, time1 = hidden duration in millisecond.
	int				m_nBlinkTime1;

	CString			m_link_url;
	Gdiplus::Color	m_cr_link = Gdiplus::Color::RoyalBlue;

	int				m_nPrefixSpace;
	int				m_value_right_space = 0;	// use_edit 값의 우측 여백(px) — 정적/편집 공통
	CSCGdiplusBitmap	m_img_back;

	int				m_halign = -1;
	int				m_valign = -1;

	//true 면 '&' 를 니모닉 접두로 처리하지 않고 글자 그대로 표시(DT_NOPREFIX 강제). 기본 false(스타일 SS_NOPREFIX 따름).
	bool			m_no_prefix = false;

	//label + value 형태일 때 value 의 가로 정렬. 기본 DT_RIGHT.
	DWORD			m_value_halign = DT_RIGHT;

	//label + value 형태일 때 label 영역의 고정 너비. 0 이면 m_text_rect.Width() + 8 사용.
	int				m_label_width = 0;

	//set_label_auto_ellipsis 참고. false (default) = 기본 wordbreak 동작 유지.
	//true = 라벨이 값과 겹치지 않게 right 동적 제한 + 안 들어가면 ellipsis. 라벨+값 셀(예 CSCPropertyCtrl) 의 opt-in.
	bool			m_label_auto_ellipsis = false;

	//color picker 모드에서 alpha 를 값에 포함할지. true → "R,G,B,A"(color32), false → "R,G,B"(color24).
	bool			m_show_alpha = true;

	//color picker 모드 활성 플래그. (구) m_text=="_color picker_" 트리거를 대체. set_as_color_picker / 매직스트링으로 켜짐.
	bool			m_as_color_picker = false;
	//color picker 의 swatch/값 색. 라벨색(m_theme.cr_text) 과 분리 저장 → 라벨과 색이 충돌하지 않음.
	Gdiplus::Color	m_cr_color_picker = Gdiplus::Color(255, 0, 0, 0);

	LOGFONT			m_lf;
	CFont			m_font;
	void			update_surface();
	void			reconstruct_font();

	CRect			m_margin;			//텍스트의 여백을 설정한다. 기본값은 CStatic과 동일하게 0.

	//label의 앞에 그려질 이미지이며 만약 2개 이상일 경우 타이머에 의해 alt되기도 한다.
	std::deque<CSCGdiplusBitmap*> m_header_images;
	int				m_header_image_index;
	int				m_header_gap = 4;		//20260705 by claude. 아이콘/헤더이미지 ↔ 라벨 간격(px). set_header_gap 으로 조정.

	//텍스트 앞에 표시되는 아이콘 또는 헤더 이미지를 텍스트와 별개로 무조건 왼쪽에 그려줄 경우
	//CSCMessageBox에서 아이콘은 왼쪽에, 텍스트는 중앙정렬되어 표시되야 하므로 이 옵션이 추가됨.
	bool			m_image_left_align_fix = false;

//tooltip
	//enable상태일때는 잘 표시되나 disable일때는 표시되지 않는다.
	//이를 해결하려면 parent의 PreTranslateMessage()에서 처리해야 한다.
	//pointer 타입으로 선언한 이유는 동적생성시에도 툴팁을 적용하기 위해.
	CToolTipCtrl*	m_tooltip = NULL;
	//default = true
	bool			m_use_tooltip = true;
	CString			m_tooltip_text = _T("");

	//정적으로 만든 컨트롤은 문제없으나 동적으로 컨트롤을 생성하여 사용하는 경우
	//PreSubclassWindow()에서 툴팁을 초기화하려니 예외가 발생함.
	//그래서 Create()후에 별도로 prepare_tooltip()을 호출하여 준비되도록 수정.
	//동적 생성한 컨트롤에서도 정상 표시됨을 확인함.
	void			prepare_tooltip();

//단락 모드 (tagged text) 의 데이터.
//m_para 가 비어있으면 plain text 모드 (기존 동작), 채워지면 paragraph 모드 (set_tagged_text 가 build).
//OnPaint 의 텍스트 그리기 단계만 분기하므로 round/back/border/header-image 비주얼은 두 모드 공통.
protected:
	std::deque<std::deque<CSCParagraph>> m_para;	//m_para[line][idx]
	CSCTextProperty	m_text_prop;					//기본 폰트/색상 속성
	void			update_text_property();			//현재 m_lf + m_theme 를 m_text_prop 에 반영
	//단락 모드(m_para 채워짐)일 때만 m_lf 변경을 m_text_prop 에 반영하고 m_para 를 재파싱·재배치. plain 모드면 no-op.
	void			rebuild_paragraph();

	bool			m_font_antialiasing = false;
	//음절별 hint 자동 결정 — 기본 ON. AA_from_pt 미만은 ClearType(작은 글씨 또렷), 이상은 AntiAlias(큰 글씨 매끈).
	bool			m_auto_font_quality = true;
	int				m_AA_from_pt = 14;
	//기본 line spacing — CSS line-height 권장 범위(1.2~1.5)의 하한. 1.0(=single) 보다 약간 여유,
	//1.5 처럼 과하지 않음. 다중 라인 한글 텍스트에서 자연스러운 간격.
	float			m_line_spacing = 1.2f;
	std::map<int, float> m_line_spacings;	//per-line override (key = line>=1)
	std::map<int, DWORD> m_line_haligns;	//per-line override (key = line>=0)

	//단락 layout 재빌드: calc_text_rect → reapply_line_spacings → apply_halign → apply_valign → Invalidate.
	//set_tagged_text, OnSize, 모든 align/line_spacing setter 가 이 경로를 탄다.
	void			rebuild_layout();
	CRect			reapply_line_spacings();
	CRect			apply_halign();
	CRect			apply_valign();
	int				get_header_lead_width();	//20260705 by claude. 좌측 헤더이미지/아이콘 폭(2+폭+gap). 단락 layout·정렬 좌측 오프셋용.

	int				m_max_width = 0;
	int				m_max_width_line = 0;
	CPoint			m_pt_icon = CPoint(0, 0);
	bool			m_auto_ctrl_size = true;

	bool			m_draw_word_hover_rect = false;
	Gdiplus::Color	m_cr_word_hover_rect = Gdiplus::Color::Red;
	CPoint			m_pos_word_hover = CPoint(-1, -1);


protected:
	//{{AFX_MSG(CSCStatic)
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void PreSubclassWindow();

	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnDestroy();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	//이 컨트롤은 OnPaint 로 외관을 전적으로 자체 그리므로, enable/disable 시 OS 기본 STATIC
	//프로시저의 disable(회색) 칠을 받지 않는다. base 로 위임하지 않아 회색 칠 자체를 막는다.
	afx_msg void OnEnable(BOOL bEnable);
};
#endif
