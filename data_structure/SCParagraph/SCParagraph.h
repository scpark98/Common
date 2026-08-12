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
	태그명(= '=' 앞부분)에 공백, 하이픈, underscore는 모두 무시된다. 값에서는 하이픈이 유지되므로 <sp=-2> 같은 음수도 쓸 수 있다.
	닫는 태그는 "자기가 담당하는 속성"만 직전 값으로 되돌린다(속성별 스택).
	따라서 <b><cr=red>This</b></cr> 처럼 교차 중첩된 기존 문서도 그대로 동작하고,
	<cr=red>바깥<cr=blue>안</cr>다시바깥</cr> 처럼 같은 속성을 중첩해도 정확히 복원된다.
	지원하는 태그는 다음과 같다.

	[글꼴]
	<f=굴림> = <font=굴림> = <name=굴림> = <font_name=굴림>
	<sz=10> = <size=10> = <font_size=10>
	<b>						//bold
	<i>						//italic
	<u>						//underline
	<s>						//strike out
	<sup> </sup>			//위 첨자 (크기 0.62배 + baseline 위로)
	<sub> </sub>			//아래 첨자
	<sp=2>	</sp>			//자간(픽셀). 구간을 글자 단위 run 으로 쪼개 글자 사이에 간격을 넣는다. 음수 = 좁힘.

	[색]
	<cr=red> = <ct=red>		//cr_text
	<crb=Blue> = <cb=Blue>	//cr_back (run 영역 사각형)
	<grad=gold,orangered>	//글자를 cr_text → 지정색 그라디언트로 채운다. 세 번째 인자 h 를 주면 가로 방향.
	<box=royalblue,12,7>	//run 단위 라운드 배경(색, 반지름, 여백). cr_back 의 사각형과 달리 "태그 칩" 모양.

	[외곽선 / 그림자 / 발광]
	<st=4> = <stroke=4>		//외곽선 두께. 펜이 path 중앙 정렬이라 눈에 보이는 두께는 이 값의 절반이다.
	<cs=black>				//외곽선 색
	<sd=3> = <shadow=3>		//그림자 offset(px). 0 = 없음, 음수 = 글자 높이에 비례한 자동값.
	<csh=dimgray>			//그림자 색
	<sb=3>					//그림자 blur sigma. 0 이면 blur 없이 하드 엣지 offset 그림자.
	<glow=aqua,14>			//외곽 발광(색, sigma). 그림자와 달리 offset 0 이라 글자 사방으로 퍼진다.

	[줄 / 배치]
	<br>					//line break — 줄 "종결자"다. 진행 중인 텍스트가 있으면 그 줄을 확정만 하므로
							//20260721 by claude. 빈 줄(공백 한 줄)을 넣으려면 <br> 를 두 개 쓴다. 단 맨 앞은 진행 중인 줄이 없어 하나로 빈 줄이 생긴다.
	<ls=0.5>				//line spacing — 이 태그 이후 텍스트를 새 라인으로 시작하며, 그 라인과 바로 전 라인 사이의 '빈 여백' 크기를 지정한다.
							//<ls> 자체가 줄바꿈을 내포하므로 <br> 를 따로 둘 필요 없다 (둘 다 써도 빈 줄·중복 줄바꿈 없음).
							//값은 "기본 여백의 배수": 1.0 = 기본(태그 없을 때)과 동일, 0.5 = 그 절반(여백만 좁아짐, 안 겹침), 0 = 딱 붙음, 2.0 = 2배.
	<vsp=14>				//<ls> 의 절대 픽셀 버전 — 이 라인의 윗 여백을 픽셀로 직접 지정. 둘 다 있으면 vsp 가 이긴다.
	<al=left|center|right>	//이 라인만의 가로 정렬. 미지정 라인은 calc_text_rect 의 align 인자를 따른다.
	<la=top|vcenter|bottom>	//이 라인만의 세로 정렬. 한 라인에 크기가 다른 run 이 섞였을 때 기준선을 고른다.
	<indent=30>				//이 라인 전체를 오른쪽으로 미는 픽셀(들여쓰기)
	<hang=20>				//word-wrap 으로 이어진 라인만 추가로 미는 픽셀(내어쓰기)
	<tab=170>				//다음 run 의 라인 내 시작 x 를 강제. 열 맞추기용. 이미 지난 위치면 무시.
	<nowrap> </nowrap>		//word-wrap 이 이 구간을 쪼개지 않는다. "홍길동 님", "12.5 GB" 처럼 붙어야 하는 덩어리용.

	[묶음 / 식별]
	<style=title> </style>	//CSCParagraph::register_style() 로 등록해 둔 속성 묶음을 통째로 적용
	<id=score>				//다음 run 에 이름을 붙인다. 렌더에는 영향 없고 호출자가 특정 run 을 찾아 갱신/hit-test 하는 용도.

	ex. "<b><cr=red>This</b></cr > is a <ct=blue><i>sample</i> <b>paragraph</b>."

	- cr은 Gdiplus::Color에 명시된 컬러 외에 다음 형식들도 지원한다.
	  <cr=#RRGGBB>,				//#으로 시작되는 16진수값. 6자리는 alpha = 255.
	  <cr=#AARRGGBB>,			//20260721 by claude. 8자리는 alpha 가 **앞**에 온다 (get_gcolor_from_hexa_str). RRGGBBAA 아님.
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

	//외곽선(thickness > 0)이 있는 글자를 SS 배 캔버스에 그린 뒤 축소한다. 1 = 끔.
	//GDI+ 의 pen widening/flattening 은 최종 픽셀 해상도에서 수행되므로 얇은 외곽선이 곡선 구간에서
	//끊기거나 폭이 들쭉날쭉해진다. SS 배로 그리면 그 오차가 1/SS 로 줄어든다.
	int			path_supersample = 3;

	float		round = 0.0f;
	float		round_thickness = 0.0f;
	Gdiplus::Color cr_text = Gdiplus::Color::Black;
	Gdiplus::Color cr_back = Gdiplus::Color::Transparent;
	Gdiplus::Color cr_stroke = Gdiplus::Color::Transparent;
	Gdiplus::Color cr_shadow = Gdiplus::Color::Transparent;
	Gdiplus::Color cr_round = Gdiplus::Color::Transparent;
	Gdiplus::Color cr_round_stroke = Gdiplus::Color::Transparent;

	//<grad=색2[,h]> — Transparent 가 아니면 cr_text → cr_grad2 로 글자를 채운다(기본 세로, h 면 가로).
	//GraphicsPath 렌더의 장점을 그대로 쓰는 부분이라 SolidBrush 대신 LinearGradientBrush 만 갈아끼우면 된다.
	Gdiplus::Color cr_grad2 = Gdiplus::Color::Transparent;
	bool		grad_horz = false;

	//<glow=색,sigma> — 외곽 발광. 그림자와 같은 blur layer 를 offset 0 으로 그린 것.
	Gdiplus::Color cr_glow = Gdiplus::Color::Transparent;
	float		glow_sigma = 0.0f;

	//<box=색,radius,pad> — run 단위 라운드 배경. 사각형인 cr_back 과 별개.
	Gdiplus::Color cr_box = Gdiplus::Color::Transparent;
	float		box_round = 0.0f;
	int			box_pad = 0;

	//<sp=값> — 이 run 안의 글자 사이 간격(픽셀). calc_text_rect 가 run 폭에 반영한다.
	float		char_spacing = 0.0f;

	//<sup>/<sub> — 첨자. size 에 곱할 배율과, 글자 높이 대비 baseline 이동 비율(양수 = 아래로).
	float		script_scale = 1.0f;
	float		script_offset = 0.0f;

	//<nowrap> 구간 — word wrap 이 이 run 을 쪼개지 않는다.
	bool		nowrap = false;
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

	//이 라인의 세로 정렬(DT_TOP/DT_VCENTER/DT_BOTTOM). set_per_line_align 이 세팅하며, calc_text_rect 끝에서 라인마다 재적용된다.
	//DT_TOP(기본) = run 들이 라인 top 에 정렬(기존 동작, 무동작). 라인 안에 크기가 다른 run(예: 큰 화살표+작은 텍스트)이 있을 때
	//라인 최대높이 기준으로 세로 정렬. 데이터(para)에 저장되므로 재레이아웃돼도 지속된다. 라인의 각 run 에 동일 값 저장(대표=para[i][0]).
	DWORD			line_align = DT_TOP;

	//<al=left|center|right> 로 지정한 이 라인만의 가로 정렬. (DWORD)-1 = 미지정(calc_text_rect 의 align 인자를 따름).
	//DT_LEFT 가 0 이라 0 을 "미지정" 으로 쓸 수 없어 -1 을 센티널로 쓴다.
	//line_align 과 마찬가지로 라인의 모든 run 에 같은 값이 들어간다(대표 = para[i][0]).
	DWORD			line_h_align = (DWORD)-1;

	//<indent=값> 이 라인 전체를 오른쪽으로 미는 픽셀. <hang=값> 은 wrap 으로 이어진 라인에만 적용되도록
	//파서가 wrap_continuation 여부와 무관하게 값을 넣고 calc_text_rect 가 wrap 라인에만 더한다.
	float			line_indent = 0.0f;
	float			line_hang = 0.0f;

	//<vsp=값> 이 라인의 윗 여백을 "픽셀"로 직접 지정. < 0 = 미지정.
	//line_spacing(<ls>) 은 배수라 폰트 크기에 딸려가지만 이쪽은 절대값이다. 둘 다 있으면 vsp 가 이긴다.
	float			line_vspace = -1.0f;

	//<tab=x> 이 run 의 라인 내 시작 x 를 강제(현재 누적 폭이 이미 x 를 넘었으면 무시). < 0 = 미지정.
	int				tab_x = -1;

	//<id=이름> run 식별자. 렌더에는 영향 없고 호출자가 특정 run 을 찾아 부분 갱신/hit-test 하는 용도.
	CString			id;

	//calc_text_rect 가 채우는 실측 글자 높이(폰트 ascent+descent 기준). r.Height()(글자 박스)는 패딩을 포함해 더 크다.
	//set_line_spacing 이 <ls> 줄간격을 "보이는 여백 = pitch - ink_height" 로 계산할 때 사용.
	float			ink_height = 0.0f;

	//이 paragraph의 CSCTextProperty 설정에 맞는 Gdiplus::Font를 구한다.
	void			get_paragraph_font(Gdiplus::Graphics& g, Gdiplus::Font** font);

	//<style=이름> 으로 한 번에 적용할 속성 묶음을 등록한다. 태그를 길게 나열하는 대신
	//register_style(_T("title"), prop) 후 "<style=title>제목</style>" 로 쓴다.
	static void		register_style(LPCTSTR name, const CSCTextProperty& prop);
	static void		clear_styles();


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

	//특정 라인의 세로 정렬을 별도로 설정할 경우 호출. 만약 line_idx < 0이면 모든 라인에 적용된다.
	//align = DT_TOP or DT_VCENTER or DT_BOTTOM
	static void		set_per_line_align(std::deque<std::deque<CSCParagraph>>& para, int line_idx, DWORD line_align);

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
