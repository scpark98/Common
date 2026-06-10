#pragma once

#include <algorithm>
#include <afxwin.h>
#include <gdiplus.h>
#include <deque>

/*
[태그된 텍스트 지원]
	- html과 같이 tagged text를 지원하기 위해 제작.
	- CSCShapeDlg, CSCParagraphStatic 등에서 사용되며 html, smi 등의 tagged text를 표현할 수 있다.

	태그는 소문자로, 컬러값은 Gdiplus::Color에 정의된 이름(web color와 naming이 동일함)을 사용해야 한다.
	컬러 이름은 대소문자를 구분하지 않는다. ("red", "Red", "RED" 모두 동일함)
	태그명에 공백, 하이픈, underscore는 모두 무시된다.
	지원하는 태그는 다음과 같다.

	<f=굴림> = <font=굴림> = <name=굴림> = <font_name=굴림>
	<sz=10> = <size=10> = <font_size=10>
	<b>						//bold
	<i>						//italic
	<u>						//underline
	<s>						//strike out
	<cr=red> = <ct=red>		//cr_text
	<crb=Blue> = <cb=Blue>	//cr_back
	<br>					//line break
	<ls=0.5>				//line spacing — 이 태그 이후 텍스트를 새 라인으로 시작하며, 그 라인과 바로 전 라인 사이의 '빈 여백' 크기를 지정한다.
							//<ls> 자체가 줄바꿈을 내포하므로 <br> 를 따로 둘 필요 없다 (둘 다 써도 빈 줄·중복 줄바꿈 없음).
							//값은 "기본 여백의 배수": 1.0 = 기본(태그 없을 때)과 동일, 0.5 = 그 절반(여백만 좁아짐, 안 겹침), 0 = 딱 붙음, 2.0 = 2배.
	ex. "<b><cr=red>This</b></cr > is a <ct=blue><i>sample</i> <b>paragraph</b>."

	- cr은 Gdiplus::Color에 명시된 컬러 외에 다음 형식들도 지원한다.
	  <cr=#RRGGBBAA>,			//rgba, #으로 시작되는 16진수값.
	  <cr=123,45,67,128>		//rgba, 숫자로만 3자리 또는 4자리를 콤마로 구분한다.
	  <cr=h90,30,100>,		//hsi, h로 시작되고 콤마로 구분한다.

[코드 흐름]
	- set_text()			: tag를 허용하는 텍스트를 설정한다.
	  get_tag_str()			: tag와 텍스트를 분리한다.
	  build_paragraph_str()	: tag를 파싱하여 각 음절의 속성을 설정한다.
	  calc_text_rect()		: 각 음절의 출력 위치를 계산한다.
	  OnPaint()				: 위에서 계산된 위치에 각 음절 텍스트를 출력한다.

	- calc_text_rect()에서 각 항목이 출력될 위치까지 모두 계산되고 OnPaint()에서는 정해진 위치에 그리기만 한다.

*/

//기존 LOGFONT는 GDI용이고 fontsize가 int만 지원되거나, lfFaceName이 TCHAR [] 등
//불편한 점이 많으므로 Gdi+에서도 사용할 수 있도록 CSCTextProperty 클래스를 정의함.
class CSCTextProperty
{
public:
	TCHAR		name[64] = _T("Tahoma");
	float		size = 10.0f;
	int			style = Gdiplus::FontStyleRegular;	//bold, italic, underline, strikeout
	float		shadow_depth = 0.0f;

	//> 0 이면 음절 그림자들을 별도 layer에 모아 fast_gaussian_blur로 한 번에 흐리게 처리한다.
	//0 이면 기존 하드 엣지 offset 그림자(호환). para[0][0] 값이 대표값으로 사용된다.
	float		shadow_blur_sigma = 3.0f;
	//< 1.0 이면 blur layer를 gray()로 톤 다운. 1.0 이면 컬러 톤 유지.
	float		shadow_gray_weight = 1.0f;

	float		thickness = 0.0f;
	float		round = 0.0f;
	float		round_thickness = 0.0f;
	Gdiplus::Color cr_text = Gdiplus::Color::Black;
	Gdiplus::Color cr_back = Gdiplus::Color::Transparent;
	Gdiplus::Color cr_stroke = Gdiplus::Color::Transparent;
	Gdiplus::Color cr_shadow = Gdiplus::Color::Transparent;
	Gdiplus::Color cr_round = Gdiplus::Color::Transparent;
	Gdiplus::Color cr_round_stroke = Gdiplus::Color::Transparent;
};

class CSCParagraph
{
public:
	CSCParagraph();
	~CSCParagraph();

	CString			text;
	CSCTextProperty	text_prop;
	CRect			r;						//이 텍스트가 그려질 위치(절대좌표가 아닌 0,0을 기준으로 상대좌표)

	//라인의 첫 run 에 set 되며, 그 라인이 source 의 같은 paragraph 에서 wrap 으로 분리된 연속 라인임을 표시.
	//set_line_spacing 이 wrap 연속 라인 위 간격에는 wrap_continuation_delta 를 추가 적용하여 <br> 분리 라인보다 좁게.
	bool			wrap_continuation = false;

	//<ls=값> 태그로 지정된 이 라인의 윗 간격(바로 전 라인과의 간격). < 0 = 미지정(기본 간격 사용).
	//값은 "기본 줄간격(보이는 여백)의 배수" — 1.0 = 기본, 0.5 = 절반, 0 = 딱 붙음, 2.0 = 2배.
	float			line_spacing = -1.0f;

	//calc_text_rect 가 채우는 실측 글자 높이(폰트 ascent+descent 기준). r.Height()(글자 박스)는 패딩을 포함해 더 크다.
	//set_line_spacing 이 <ls> 줄간격을 "보이는 여백 = pitch - ink_height" 로 계산할 때 사용.
	float			ink_height = 0.0f;

	//이 paragraph의 CSCTextProperty 설정에 맞는 Gdiplus::Font를 구한다.
	void			get_paragraph_font(Gdiplus::Graphics& g, Gdiplus::Font** font);


	//아래 static 함수들은 하나의 CSCParagraph에 대해 수행되는 함수들이 아니고
	//std::deque<std::deque<CSCParagraph>> 구조의 paragraph에 대해 수행해야 하므로
	//CSCParagraph의 멤버함수가 아닌 static으로 선언한다.

	//text의 태그를 파싱하여 각 음절의 속성을 설정한 후 para에 저장한다.
	//ti에는 font name, size, style, color 등이 세팅되어 있고
	//특별한 태그가 없으면 ti에 설정된 기본값을 사용한다.
	static void		build_paragraph_str(CString& text, std::deque<std::deque<CSCParagraph>>& para, CSCTextProperty* text_prop);

	//paragraph text 정보를 dc에 출력할 때 출력 크기를 계산하고 각 텍스트가 출력될 위치까지 CSCParagraph 멤버에 저장한다.
	//모든 para가 출력되는 최대 사각형을 리턴한다.
	//max_width > 0 이면 word-wrap 활성 — 라인 누적 너비가 max_width 초과 시 whitespace 우선 boundary,
	//	없으면 character boundary 로 split 하여 para 구조를 라인 단위로 재구성한다 (CJK 자막처럼 공백 없는 텍스트 대응).
	//char_spacing != 0 이면 같은 라인 안에서 인접 run 사이에 char_spacing 픽셀 만큼 간격 추가/축소.
	//	(자간을 진짜 글자 단위로 적용하려면 호출 측이 사전에 per-char 로 run 을 split 해 주어야 한다.)
	static CRect	calc_text_rect(CRect rc, CDC* pDC, std::deque<std::deque<CSCParagraph>>& para, DWORD align, int max_width = 0, int char_spacing = 0);

	//run 들을 character 단위로 split 한다 — 자간 (char_spacing) 적용 시 각 글자가 독립 run 이 되어
	//calc_text_rect 가 run 사이에 spacing 을 넣는 것이 곧 글자 사이에 spacing 이 되도록 한다.
	//이미 1 글자 이하인 run 은 그대로 두고, 다중 글자 run 만 분해.
	static void		split_runs_per_char(std::deque<std::deque<CSCParagraph>>& para);
	static int		get_max_width_line(std::deque<std::deque<CSCParagraph>>& para);

	//각 paragraph의 r이 계산된 후에 줄 간격을 spacing 배수로 조정한다. spacing이 1.0f이면 기본 줄 간격, 2.0f이면 줄 간격이 2배가 된다.
	//wrap_continuation_delta != 0 이면 wrap 연속 라인 (line[0].wrap_continuation == true) 위 간격에 (spacing + wrap_continuation_delta) 적용. 음수 = 좁힘.
	//paragraph_break_delta != 0 이면 그 외 라인 (i>0 이면서 wrap_continuation == false, =원래 <br> 분리) 위 간격에 (spacing + paragraph_break_delta) 적용. 양수 = 넓힘.
	//두 delta 를 같이 쓰면 wrap 과 paragraph break 의 시각적 차이를 강조 — wrap 라인 압축 + paragraph break 확장.
	//단, <ls=값> 태그로 line_spacing >= 0 이 지정된 라인 (wrap 연속 제외) 은 그 값을 "빈 여백(pitch - ink_height)의 배수" 로 해석 — 1.0 = 기본, 0.5 = 절반, 0 = 딱 붙음.
	static CRect	set_line_spacing(std::deque<std::deque<CSCParagraph>>& para,
								float spacing = 1.0f,
								float wrap_continuation_delta = 0.0f,
								float paragraph_break_delta = 0.0f);
	//line번째 라인의 윗 간격(line-1 번째 라인과의 간격)만 spacing 배수로 조정한다. line >= 1 이어야 한다.
	static CRect	set_line_spacing(std::deque<std::deque<CSCParagraph>>& para, int line, float spacing = 1.0f);

	//static void		draw_text(CDC* pDC, std::deque<std::deque<CSCParagraph>>& para);
	//AA_from_pt > 0 이면 음절별 폰트 size(pt) 와 비교해 매 음절 그리기 전에 SetTextRenderingHint 를 자동 결정
	//(< AA_from_pt → ClearTypeGridFit: 작은 글씨 또렷 / >= AA_from_pt → AntiAliasGridFit: 큰 글씨 매끄럽게).
	//AA_from_pt == 0 이면 호출자가 미리 g 에 설정한 hint 를 그대로 유지 (기존 동작과 호환).
	//dark_background=true 면 ClearType subpixel fringe 가 어두운 배경에서 두드러져 grayscale AA 가
	//깔끔하므로 음절별 임계치를 자동으로 더 낮춰 적용 (get_AA_from_pt 내부에서 처리).
	//리턴: 실제 그려진 텍스트 영역(모든 run r 의 합집합). 단락 모드에서 호출측이 m_text_rect 로 사용.
	static CRect	draw_text(Gdiplus::Graphics& g, std::deque<std::deque<CSCParagraph>>& para, int AA_from_pt = 0, bool dark_background = false);

	//폰트 이름별 AA 전환 임계치(pt) 결정 — 우선순위:
	//  1) add_AA_override 로 등록된 face 별 사용자 강제값 (있으면 즉시 반환)
	//  2) 폰트 자체 임베디드 비트맵 strike 메트릭 (EBLC 테이블에서 자동 측정, face 캐시)
	//     - strike 보유 안 함 → 1 (전 크기 AA 가 또렷한 순수 outline 폰트)
	//     - strike 보유 → max ppemY (px) → pt 환산 + 1 (그 위부터 AA, 이하 ClearType/비트맵)
	//  3) 화이트리스트 매핑 (Segoe UI / 굴림 / Tahoma 등 알려진 케이스)
	//  4) fallback_pt
	//마지막으로 dark_background=true 면 결과를 dark_boost 만큼 추가 감산해 grayscale AA 를 더 일찍 적용.
	//draw_text 가 내부에서 쓰지만, 단락 모드가 아닌 단순 텍스트 렌더 경로(CGdiButton 등) 에서도 공유.
	static int		get_AA_from_pt(LPCTSTR font_name, int fallback_pt, bool dark_background = false);

	//특정 face 에 대해 임계치를 강제. pt <= 0 이면 등록 해제. 호출 즉시 캐시 무효화.
	//운영 중 발견한 케이스 (예: 특정 사용자 폰트가 매핑·EBLC 둘 다 안 맞아 흐릿) 빠른 보정용.
	static void		add_AA_override(LPCTSTR font_name, int pt);
	static void		clear_AA_overrides();

	//calc_text_rect()에서 이미 각 paragraph의 r이 align에 따라 정해지지만 이를 동적으로 변경하고자 할 경우 호출.
	static CRect	set_text_align(CRect rc, std::deque<std::deque<CSCParagraph>>& para, DWORD align);

	//텍스트 상하좌우 여백
	//static void		set_margin(std::deque<std::deque<CSCParagraph>>& para, float margin);

	//출력 위치를 이동시킨다.
	static CRect	offset(int x, int y);

	//para의 정보를 문자열로 리턴한다.
	//"(row, col) : text = %s, r = %s, name = %s, size = %.1f, style = %d\n" 스타일로 리턴한다.
	//실제 
	static CString	get_paragraph_info_string(std::deque<std::deque<CSCParagraph>>& para);
protected:
};
