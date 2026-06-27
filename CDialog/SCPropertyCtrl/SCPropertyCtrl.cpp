// CSCPropertyCtrl.cpp : 데이터 바인딩형 속성 컨트롤
// 이 프로젝트의 Common .cpp 들은 PCH 를 쓰지 않는다(vcxproj 에서 NotUsing). pch.h 를 include 하지 않음.

#include "SCPropertyCtrl.h"
#include "../../MemoryDC.h"	// CMemoryDC — 더블 버퍼(다른 프로젝트 공통 사용)

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNAMIC(CSCPropertyCtrl, CDialogEx)

CSCPropertyCtrl::CSCPropertyCtrl()
{
}

CSCPropertyCtrl::~CSCPropertyCtrl()
{
}

// Enter/Esc 로 패널(자식 다이얼로그)이 닫히지 않게 — 기본 동작 무력화.
void CSCPropertyCtrl::OnOK()     {}
void CSCPropertyCtrl::OnCancel() {}

// bool/combo/info 셀 label 위 hover 를 패널 툴팁으로 relay(수동 relay 라 EnableToolTips 불필요).
BOOL CSCPropertyCtrl::PreTranslateMessage(MSG* pMsg)
{
	if (m_tooltip.GetSafeHwnd())
		m_tooltip.RelayEvent(pMsg);
	return CDialogEx::PreTranslateMessage(pMsg);
}

BEGIN_MESSAGE_MAP(CSCPropertyCtrl, CDialogEx)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_MOUSEWHEEL()
	ON_WM_LBUTTONDOWN()
	ON_REGISTERED_MESSAGE(Message_CSCStatic, &CSCPropertyCtrl::on_message_CSCStatic)
	ON_REGISTERED_MESSAGE(Message_CSCMenu, &CSCPropertyCtrl::on_message_CSCMenu)
END_MESSAGE_MAP()

// ── 생성/테마 ───────────────────────────────────────────────────────────────
bool CSCPropertyCtrl::create(CWnd* parent, int left, int top, int width, int height)
{
	// #32770(다이얼로그) 클래스를 복제해 우리 이름으로 등록 → 다이얼로그 동작(자식 호스팅·키보드
	// 내비·DDX) 유지하면서 IDD 템플릿 없이 코드로 생성. (CSCThumbCtrl 과 동일 idiom)
	WNDCLASS wc = {};
	if (!::GetClassInfo(AfxGetInstanceHandle(), _T("CSCPropertyCtrl"), &wc))
	{
		::GetClassInfo(AfxGetInstanceHandle(), _T("#32770"), &wc);
		wc.lpszClassName = _T("CSCPropertyCtrl");
		wc.style |= CS_DBLCLKS;
		AfxRegisterClass(&wc);
	}

	if (width  <= 0) width  = 280;
	if (height <= 0) height = 400;

	const DWORD style = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN;
	if (!CreateEx(0, _T("CSCPropertyCtrl"), _T("CSCPropertyCtrl"), style,
				  CRect(left, top, left + width, top + height), parent, 0))
		return false;

	ModifyStyle(WS_CAPTION, 0);

	// 다크 배경에서 ClearType 서브픽셀은 색번짐(파랑/주황 가장자리)이 거슬리므로 ANTIALIASED_QUALITY(그레이스케일 AA)로 그린다.
	// 세부 항목(m_font, -10)은 섹션 타이틀(m_font_bold, -12)보다 2px 작게 → Figma 속성창의 위계(타이틀 > 항목)와 동일.
	m_font.CreateFont(-11, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, _T("Segoe UI"));
	m_font_bold.CreateFont(-12, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, _T("Segoe UI"));

	return true;
}

void CSCPropertyCtrl::set_color_theme(const CSCColorTheme& theme, bool invalidate)
{
	m_theme.copy_colors_from(theme);

	// 자식 CSCStatic 의 prefix 라벨은 cr_text 로 그려진다(CSCStatic OnPaint 내부 기본). 값보다 *흐리게* 표시하기 위해
	// propagate 시 cr_text 만 cr_text_dim 으로 override 한 사본을 전달 → CSCStatic 의 m_theme.cr_text(=라벨)가 자동 dim.
	// 값은 set_edit_text_color (별개 멤버 cr_edit_text) 로 cr_text 셋팅 — 라벨 < 값 대비.
	// color 셀의 swatch 색(m_cr_color_picker)과 RGB 값 텍스트(cr_edit_text) 는 별개 변수라 영향 없음.
	// 기존엔 값 색으로 cr_text_selected 를 썼지만, 그 색의 본래 의미는 *selected 배경 위 텍스트* 라 deep_black 처럼
	// sel_bg 가 밝은 다크 테마에서 값이 검정으로 산출되어 안 보이던 버그가 있었다.
	CSCColorTheme dim_label_theme;
	dim_label_theme.copy_colors_from(m_theme);
	dim_label_theme.cr_text = m_theme.cr_text_dim;

	for (prop_row& row : m_rows)			// 이미 생성된 자식에도 전파
		for (prop_cell& c : row.cells)
			if (c.ctrl && c.ctrl->GetSafeHwnd())
			{
				c.ctrl->set_color_theme(dim_label_theme, false);
				c.ctrl->set_edit_text_color(m_theme.cr_text);			// 값 / color RGB·alpha 텍스트
				c.ctrl->set_edit_back_color(field_back_color());
				c.ctrl->set_back_color(field_back_color());
				c.ctrl->set_round(m_field_round, m_theme.cr_back, m_theme.cr_back);

				apply_enabled_visual(c);	// disabled 셀의 값 dim 색을 theme 교체 후에도 유지
			}
	if (invalidate && GetSafeHwnd())
		Invalidate(FALSE);
}

// ── 선언적 빌드 ─────────────────────────────────────────────────────────────
void CSCPropertyCtrl::begin()
{
	destroy_controls();
	m_rows.clear();
	m_cur_section = -1;
	m_open_line = -1;
}

void CSCPropertyCtrl::section(const CString& title, bool expanded)
{
	m_open_line = -1;	// 섹션 시작 시 열린 라인 닫기

	prop_row row;
	row.is_section = true;
	row.section_title = title;
	row.expanded = expanded;
	m_rows.push_back(std::move(row));
	m_cur_section = static_cast<int>(m_rows.size()) - 1;
	m_rows.back().section_index = m_cur_section;
}

void CSCPropertyCtrl::begin_row()
{
	prop_row row;
	row.is_section = false;
	row.section_index = m_cur_section;
	m_rows.push_back(std::move(row));
	m_open_line = static_cast<int>(m_rows.size()) - 1;
}

void CSCPropertyCtrl::end_row()
{
	m_open_line = -1;
}

CSCPropertyCtrl::prop_cell& CSCPropertyCtrl::add_cell(field_type type, const CString& label)
{
	// begin_row() 로 열린 라인이 있으면 그 라인에 셀 추가, 없으면 셀 하나짜리 라인을 새로 만든다.
	if (m_open_line >= 0 && m_open_line < static_cast<int>(m_rows.size()))
	{
		prop_row& row = m_rows[m_open_line];
		row.cells.emplace_back();
		prop_cell& c = row.cells.back();
		c.type = type;
		c.label = label;
		return c;
	}

	prop_row row;
	row.is_section = false;
	row.section_index = m_cur_section;
	row.cells.emplace_back();
	row.cells.back().type = type;
	row.cells.back().label = label;
	m_rows.push_back(std::move(row));
	return m_rows.back().cells.back();
}

// add<T> 본체 = 헤더 inline (template). 여기엔 combo overload / color32 / info 만 정의 (T 추론으로 통합 불가능한 케이스들).
void CSCPropertyCtrl::add(int* index, const CString& label, const std::vector<CString>& options, const CString& tooltip)
{
	prop_cell& c = add_cell(field_type::combo, label);
	c.p_index = index;
	c.options = options;
	c.tooltip = tooltip;
}
void CSCPropertyCtrl::add_color32(Gdiplus::Color* value, const CString& label, const CString& tooltip)
{
	prop_cell& c = add_cell(field_type::color32, label); c.p_color = value; c.tooltip = tooltip;
}
void CSCPropertyCtrl::add_info(const CString& label, const CString& text, const CString& tooltip)
{
	prop_cell& c = add_cell(field_type::info, label); c.info_text = text; c.tooltip = tooltip;
}

// ── 항목 식별 (public) ──────────────────────────────────────────────────────
// const 변형은 본체, 비-const 변형은 const_cast 위임 — 코드 중복 방지.
const CSCPropertyCtrl::prop_cell* CSCPropertyCtrl::find_property(const CString& label) const
{
	for (const prop_row& row : m_rows)
	{
		if (row.is_section)
			continue;
		for (const prop_cell& c : row.cells)
			if (c.label == label)
				return &c;
	}
	return nullptr;
}

CSCPropertyCtrl::prop_cell* CSCPropertyCtrl::find_property(const CString& label)
{
	return const_cast<prop_cell*>(static_cast<const CSCPropertyCtrl*>(this)->find_property(label));
}

const CSCPropertyCtrl::prop_cell* CSCPropertyCtrl::find_property_by_binding(const void* binding) const
{
	for (const prop_row& row : m_rows)
	{
		if (row.is_section)
			continue;
		for (const prop_cell& c : row.cells)
		{
			if ((const void*)c.p_text == binding || (const void*)c.p_int == binding || (const void*)c.p_real == binding
				|| (const void*)c.p_bool == binding || (const void*)c.p_color == binding || (const void*)c.p_index == binding)
				return &c;
		}
	}
	return nullptr;
}

CSCPropertyCtrl::prop_cell* CSCPropertyCtrl::find_property_by_binding(const void* binding)
{
	return const_cast<prop_cell*>(static_cast<const CSCPropertyCtrl*>(this)->find_property_by_binding(binding));
}

bool CSCPropertyCtrl::has_property           (const CString& label)   const { return find_property(label)            != nullptr; }
bool CSCPropertyCtrl::has_property_by_binding(const void*    binding) const { return find_property_by_binding(binding) != nullptr; }

// ── 활성/비활성 ─────────────────────────────────────────────────────────────
// 비활성 시 자식 CSCStatic 은 EnableWindow(FALSE)로 입력 차단 + dim 색, 패널 그리기 셀(bool/combo/info)은
// OnLButtonDown/OnMouseWheel 에서 enabled 체크로 입력 무시 + draw_* 에서 dim 색 사용.
bool CSCPropertyCtrl::apply_enabled(prop_cell* pc, bool enabled)
{
	if (!pc)
		return false;
	if (pc->enabled == enabled)
		return true;
	pc->enabled = enabled;
	apply_enabled_visual(*pc);
	if (GetSafeHwnd())
		Invalidate(FALSE);					// 패널 그리기 셀의 dim 색 반영 위해 재그리기
	return true;
}

void CSCPropertyCtrl::apply_enabled_visual(prop_cell& c)
{
	if (!c.ctrl || !c.ctrl->GetSafeHwnd())
		return;
	c.ctrl->EnableWindow(c.enabled ? TRUE : FALSE);
	// 라벨 색은 dim_label_theme propagate(=cr_text_dim) 으로 결정되어 항상 dim. 여기선 *값 색* 만 enabled 에 따라 분기:
	//   enabled  → cr_text (값 > 라벨 대비 = 정상)
	//   disabled → cr_text_dim (값 = 라벨 = 같은 색 → "비활성" 시각 신호)
	// + EnableWindow(FALSE) 로 입력도 차단. color 셀의 swatch 색은 m_cr_color_picker(별개) 라 영향 없음.
	c.ctrl->set_edit_text_color(c.enabled ? m_theme.cr_text : m_theme.cr_text_dim);
}

bool CSCPropertyCtrl::set_enabled           (const CString& label,   bool enabled) { return apply_enabled(find_property(label),            enabled); }
bool CSCPropertyCtrl::set_enabled_by_binding(const void*    binding, bool enabled) { return apply_enabled(find_property_by_binding(binding), enabled); }

bool CSCPropertyCtrl::get_enabled(const CString& label) const
{
	const prop_cell* pc = find_property(label);
	return pc ? pc->enabled : true;			// 없는 항목 = enabled 로 취급(가짜 false 로 사용자 로직 오염 방지)
}
bool CSCPropertyCtrl::get_enabled_by_binding(const void* binding) const
{
	const prop_cell* pc = find_property_by_binding(binding);
	return pc ? pc->enabled : true;
}

// ── 라벨 변경 ───────────────────────────────────────────────────────────────
bool CSCPropertyCtrl::apply_label(prop_cell* pc, const CString& new_label)
{
	if (!pc)
		return false;
	pc->label = new_label;
	if (pc->ctrl && pc->ctrl->GetSafeHwnd())
		pc->ctrl->SetWindowText(new_label);	// CSCStatic 의 caption = prefix 라벨
	if (GetSafeHwnd())
	{
		layout();							// 라벨 길이 변화 → m_shared_label_w / label_w 재계산
		Invalidate(FALSE);
	}
	return true;
}

bool CSCPropertyCtrl::set_label           (const CString& old_label, const CString& new_label) { return apply_label(find_property(old_label),         new_label); }
bool CSCPropertyCtrl::set_label_by_binding(const void*    binding,   const CString& new_label) { return apply_label(find_property_by_binding(binding), new_label); }

CString CSCPropertyCtrl::get_label_by_binding(const void* binding) const
{
	const prop_cell* pc = find_property_by_binding(binding);
	return pc ? pc->label : CString();
}

// apply_set<T> / apply_get<T> / set_value<T> / get_value<T> 본체 = 헤더 inline (template).
// apply_set 의 if constexpr 분기가 옛 apply_value_str/int/real/bool/color 5 워커를 통합 흡수.

// ── combo 옵션 교체 ────────────────────────────────────────────────────────
bool CSCPropertyCtrl::apply_options(prop_cell* pc, const std::vector<CString>& options)
{
	if (!pc || pc->type != field_type::combo)
		return false;
	pc->options = options;
	if (pc->p_index)
	{
		const int n = static_cast<int>(pc->options.size());
		*pc->p_index = (n == 0) ? -1 : (std::max)(0, (std::min)(n - 1, *pc->p_index));
	}
	if (GetSafeHwnd())
		Invalidate(FALSE);
	return true;
}

bool CSCPropertyCtrl::set_options           (const CString& label,   const std::vector<CString>& options) { return apply_options(find_property(label),            options); }
bool CSCPropertyCtrl::set_options_by_binding(const void*    binding, const std::vector<CString>& options) { return apply_options(find_property_by_binding(binding), options); }

const std::vector<CString>& CSCPropertyCtrl::get_options(const CString& label) const
{
	static const std::vector<CString> empty;
	const prop_cell* pc = find_property(label);
	return (pc && pc->type == field_type::combo) ? pc->options : empty;
}

// ── 툴팁 ────────────────────────────────────────────────────────────────────
// 빈 문자열 → 툴팁 제거. text/int/real/color 는 자식 CSCStatic 내장 툴팁, bool/combo/info 는 패널이 label 영역에 띄움.
bool CSCPropertyCtrl::apply_tooltip(prop_cell* pc, const CString& tooltip)
{
	if (!pc)
		return false;
	pc->tooltip = tooltip;
	if (pc->ctrl && pc->ctrl->GetSafeHwnd())
		pc->ctrl->set_tooltip_text(tooltip);
	else if (GetSafeHwnd())
		rebuild_widget_tooltips();
	return true;
}

bool CSCPropertyCtrl::set_tooltip           (const CString& label,   const CString& tooltip) { return apply_tooltip(find_property(label),            tooltip); }
bool CSCPropertyCtrl::set_tooltip_by_binding(const void*    binding, const CString& tooltip) { return apply_tooltip(find_property_by_binding(binding), tooltip); }

void CSCPropertyCtrl::end()
{
	ensure_controls();
	layout();
	if (GetSafeHwnd())
		Invalidate(FALSE);
}

void CSCPropertyCtrl::refresh()
{
	for (prop_row& row : m_rows)
		for (prop_cell& c : row.cells)
			sync_value(c);
	if (GetSafeHwnd())
		Invalidate(FALSE);
}

// ── 자식 컨트롤(CSCStatic) 관리 ────────────────────────────────────────────
void CSCPropertyCtrl::destroy_controls()
{
	for (prop_row& row : m_rows)
		for (prop_cell& c : row.cells)
			if (c.ctrl && c.ctrl->GetSafeHwnd())
				c.ctrl->DestroyWindow();
}

void CSCPropertyCtrl::ensure_controls()
{
	if (!GetSafeHwnd())
		return;

	UINT id = 100;
	for (prop_row& row : m_rows)
	{
		if (row.is_section)
			continue;

		for (prop_cell& c : row.cells)
		{
			if (!prop_cell::uses_static(c.type))
				continue;
			if (c.ctrl && c.ctrl->GetSafeHwnd())
			{
				++id;
				continue;
			}

			const bool is_color = (c.type == field_type::color24 || c.type == field_type::color32);

			c.ctrl = std::make_shared<CSCStatic>();
			// 셀 하나 = CSCStatic 하나(값 필드 text/int/real/color 모두 동일). UXStudio CPropertyDlg 의
			// m_static_canvas_size_cx 와 동일 idiom — caption=라벨, use_edit 로 한 컨트롤이 라벨+값을 그리고
			// (값은 라벨 우측), 값 클릭→CSCEdit. set_round 로 컨트롤이 자기 라운드 박스까지 직접 그림 → 패널 무관.
			//  - color24/32 : set_as_color_picker(true) → 라벨 + swatch + "R,G,B"(24)/"R,G,B,A"(32) 까지 한 컨트롤이
			//                 그림. swatch 클릭→CSCColorPicker, 값 클릭→편집. (CSCStatic 확장: swatch 색은 라벨색과
			//                 분리된 m_cr_color_picker 에 저장 → 라벨과 색 충돌 없이 "Background ■ 0,0,0" 한 컨트롤.)
			c.ctrl->create(static_cast<LPCTSTR>(c.label), WS_CHILD | SS_NOTIFY, CRect(0, 0, 10, 10), this, id++);
			c.ctrl->SetFont(&m_font);	// 패널과 동일한 그레이스케일 AA 폰트(라벨/값 색번짐 방지 + 크기 통일)
			// dim_label_theme = m_theme 사본 + cr_text 만 cr_text_dim 으로 override → CSCStatic 의 prefix 라벨이 dim.
			// 값은 set_edit_text_color 로 cr_text 별도 셋(라벨 < 값 대비). set_color_theme 와 동일 패턴.
			CSCColorTheme dim_label_theme;
			dim_label_theme.copy_colors_from(m_theme);
			dim_label_theme.cr_text = m_theme.cr_text_dim;
			c.ctrl->set_color_theme(dim_label_theme, false);
			c.ctrl->set_edit_text_color(m_theme.cr_text);				// 값 / color 의 "R,G,B"·alpha 텍스트
			c.ctrl->set_edit_back_color(field_back_color());
			c.ctrl->set_back_color(field_back_color());					// 박스 채움(패널 배경보다 약간 밝게)
			c.ctrl->set_round(m_field_round, m_theme.cr_back, m_theme.cr_back);	// 무테 라운드 박스(코너는 패널 배경색과 블렌드)
			c.ctrl->set_prefix_space(2);								// 박스 안 라벨(prefix) 좌측 여백
			c.ctrl->set_valign(DT_VCENTER);								// 라벨도 값과 같은 세로 가운데(기본 top → 라벨이 위로 떠 어긋남)
			c.ctrl->set_label_auto_ellipsis(true);						// 라벨이 값과 충돌 시에만 "..." 처리(값이 짧으면 라벨이 컬럼 너머까지 자유롭게)

			if (is_color)
			{
				c.ctrl->set_as_color_picker(true);						// 라벨 + swatch + "R,G,B[,A]" 를 한 컨트롤이 그림
				c.ctrl->set_show_alpha(c.type == field_type::color32);	// color32 만 alpha 포함("R,G,B,A")
				c.ctrl->set_use_edit(true);								// swatch 클릭→CSCColorPicker, 값 클릭→편집
			}
			else
			{
				const bool is_number = (c.type == field_type::integer || c.type == field_type::real);
				// 숫자(int/real)는 값/편집을 우측정렬, 텍스트는 좌측정렬.
				c.ctrl->set_use_edit(true, is_number ? ES_RIGHT : ES_LEFT);	// 라벨+값. 값 영역 클릭→CSCEdit
				c.ctrl->set_value_halign(is_number ? DT_RIGHT : DT_LEFT);
				if (is_number)
				{
					c.ctrl->set_value_right_space(m_field_lpad - 4);	// 값 우측 여백 8px(내장 4 + 4) = 좌측 prefix 와 대칭
					// real 셀은 interval=0 (auto) — 입력 텍스트의 소숫점 자릿수로 step 자동 결정.
					//   "0.5" → 0.1 step, "0.000345" → 1e-6 step. integer 셀은 1 고정.
					c.ctrl->set_use_updown_key(true, c.type == field_type::real ? 0.0f : 1.0f);
				}
			}

			sync_value(c);

			if (!c.tooltip.IsEmpty())
				c.ctrl->set_tooltip_text(c.tooltip);	// 자식 CSCStatic 내장 툴팁(SS_NOTIFY 보유 → hover relay)

			apply_enabled_visual(c);					// end() 이전에 set_enabled(false) 했어도 자식 생성 후 일관 반영
		}
	}
}

void CSCPropertyCtrl::sync_value(prop_cell& c)
{
	if (!c.ctrl || !c.ctrl->GetSafeHwnd())
		return;

	switch (c.type)
	{
	case field_type::text:
		c.ctrl->set_text_value(c.p_text ? *c.p_text : CString());
		break;
	case field_type::integer:
	{
		CString s;
		if (c.p_int)
			s.Format(_T("%d"), *c.p_int);
		c.ctrl->set_text_value(s);
		break;
	}
	case field_type::real:
	{
		CString s;
		if (c.p_real)
			s.Format(_T("%g"), *c.p_real);
		c.ctrl->set_text_value(s);
		break;
	}
	case field_type::color24:
	case field_type::color32:
		if (c.p_color)
			c.ctrl->set_text_color(*c.p_color);	// "_color picker_" 모드의 swatch 색
		break;
	case field_type::info:
		c.ctrl->set_text_value(c.info_text);
		break;
	default:
		break;
	}
}

CSCPropertyCtrl::prop_cell* CSCPropertyCtrl::find_cell_by_ctrl(const CWnd* pCtrl)
{
	for (prop_row& row : m_rows)
		for (prop_cell& c : row.cells)
			if (c.ctrl && c.ctrl.get() == pCtrl)
				return &c;
	return nullptr;
}

// 필드 박스 채움: 패널 배경보다 약간 밝게(테두리 없이도 박스가 보이도록). dark 테마 cr_back=(37,37,38).
Gdiplus::Color CSCPropertyCtrl::field_back_color() const
{
	return get_color(m_theme.cr_back, 18);
}

// 라벨(prefix) 색 — 패널이 그리는 라벨(color/bool/combo/info)과 컨트롤이 그리는 라벨(text/int/real)을
// 한 색으로 통일한다. 양쪽 모두 cr_text_dim 사용 → 값(cr_text) 보다 흐림 → Figma 스타일의 라벨<값 대비.
// 컨트롤 쪽은 dim_label_theme propagate(cr_text=cr_text_dim) 로, 패널 쪽은 이 함수 직접 사용.
Gdiplus::Color CSCPropertyCtrl::field_label_color() const
{
	return m_theme.cr_text_dim;
}

// 패널 그리기 셀의 라벨/값 색 — enabled 면 normal, disabled 면 dim. draw_cell_label / draw_widget / draw_bool 공통.
Gdiplus::Color CSCPropertyCtrl::cell_label_color(const prop_cell& c) const
{
	return c.enabled ? field_label_color() : m_theme.cr_text_dim;
}
Gdiplus::Color CSCPropertyCtrl::cell_text_color(const prop_cell& c) const
{
	return c.enabled ? m_theme.cr_text : m_theme.cr_text_dim;
}

int CSCPropertyCtrl::measure_label_width(const CString& text) const
{
	if (!GetSafeHwnd() || text.IsEmpty())
		return 0;
	CClientDC dc(const_cast<CSCPropertyCtrl*>(this));
	CFont* old = dc.SelectObject(const_cast<CFont*>(&m_font));
	const CSize sz = dc.GetTextExtent(text);
	dc.SelectObject(old);
	return sz.cx;
}

CRect CSCPropertyCtrl::cell_value_rect(const CRect& cell_rc, int label_w) const
{
	return CRect(cell_rc.left + label_w, cell_rc.top, cell_rc.right, cell_rc.bottom);
}

// ── 레이아웃(세로 누적 + 셀 분할 + 자식 위치) ───────────────────────────────
void CSCPropertyCtrl::position_cell_ctrl(prop_cell& c)
{
	if (!c.ctrl || !c.ctrl->GetSafeHwnd())
		return;

	// 값 필드(text/int/real/color)는 컨트롤이 박스+라벨+값(+swatch)을 직접 그리므로 셀 전체를 덮는다.
	CRect box = c.rect;
	box.OffsetRect(0, -m_scroll_y);
	box.DeflateRect(0, m_field_vmargin);
	// MoveWindow 의 default fRepaint=TRUE 는 자식의 즉시 sync paint 를 trigger 한다.
	// splitter drag 중 매 mouse 이동마다 모든 자식이 sync paint → UI thread 점유 → 호스트의 animation
	// 갱신(view) 이 끊김. fRepaint=FALSE + async InvalidateRect 로 자식 paint 를 큐로 보내 dispatch.
	c.ctrl->MoveWindow(box, FALSE);
	c.ctrl->ShowWindow(SW_SHOW);
	c.ctrl->Invalidate(FALSE);
}

void CSCPropertyCtrl::layout_cells(prop_row& row)
{
	const int n = static_cast<int>(row.cells.size());
	if (n == 0)
		return;

	const int content_left  = row.rect.left + m_pad;
	const int content_right = row.rect.right - m_pad;
	const int total = (std::max)(0, content_right - content_left);
	const int gap = (n > 1) ? m_cell_gap : 0;					// 셀(필드) 사이 가로 간격
	const int cell_w = (total - (n - 1) * gap) / n;

	for (int i = 0; i < n; ++i)
	{
		prop_cell& c = row.cells[i];
		const int cl = content_left + i * (cell_w + gap);
		const int cr = (i == n - 1) ? content_right : cl + cell_w;	// 마지막 셀이 나머지 폭 흡수
		c.rect = CRect(cl, row.rect.top, cr, row.rect.bottom);		// 스크롤 적용 전(패널 콘텐츠 좌표)

		// 라벨 컬럼 폭: 단일 셀 행은 공통 폭(값 left 정렬용), 여러 셀이면 셀별 측정값. 둘 다 셀폭 상한 cap.
		const int cap = c.rect.Width() * m_label_ratio / 100;
		const int measured = m_field_lpad + measure_label_width(c.label) + m_label_gap;
		c.label_w = (n == 1) ? (std::min)(m_shared_label_w, cap) : (std::min)(measured, cap);

		// 값 필드는 컨트롤이 라벨+값(또는 라벨+swatch+값)을 직접 그리므로 값/swatch 시작(=라벨 컬럼 폭)을 알려 정렬.
		if (c.ctrl && c.ctrl->GetSafeHwnd())
			c.ctrl->set_label_width(c.label_w);

		position_cell_ctrl(c);
	}
}

void CSCPropertyCtrl::layout()
{
	if (!GetSafeHwnd())
		return;
	CRect rc;
	GetClientRect(rc);
	const int W = rc.Width();

	// 단일 셀 행들의 공통 라벨 폭 = 가장 긴 라벨 + 간격 → 값 필드 left 가 한 줄로 정렬된다.
	int max_label = 0;
	for (const prop_row& row : m_rows)
		if (!row.is_section && row.cells.size() == 1)
			max_label = (std::max)(max_label, measure_label_width(row.cells[0].label));
	m_shared_label_w = m_field_lpad + max_label + m_label_gap;	// 라벨 왼쪽여백 + 가장 긴 라벨 + 간격

	int y = 0;
	bool seen_section = false;
	for (prop_row& row : m_rows)
	{
		if (row.is_section)
		{
			if (seen_section)		// 첫 섹션 제외하고 위에 여백 → 카테고리를 위 항목과 떨어뜨림
				y += m_section_gap;
			seen_section = true;

			row.visible = true;
			row.rect = CRect(0, y, W, y + m_section_h);
			y += m_section_h;
			continue;
		}

		const bool exp = (row.section_index >= 0 && row.section_index < static_cast<int>(m_rows.size()))
			? m_rows[row.section_index].expanded : true;

		if (!exp)
		{
			row.visible = false;
			row.rect = CRect(0, y, W, y);
			for (prop_cell& c : row.cells)
				if (c.ctrl && c.ctrl->GetSafeHwnd())
					c.ctrl->ShowWindow(SW_HIDE);
			continue;
		}

		row.visible = true;
		row.rect = CRect(0, y, W, y + m_row_h);
		y += m_row_h;
		layout_cells(row);
	}

	m_content_h = y;
	clamp_scroll();
	rebuild_widget_tooltips();
}

// bool/combo/info 셀(값 컬럼 자식 없음)의 label 영역에 설명 툴팁을 rect 툴로 (재)등록한다.
// 스크롤/리레이아웃마다 on-screen rect 가 바뀌므로 매 layout 끝에서 통째로 재구성한다.
void CSCPropertyCtrl::rebuild_widget_tooltips()
{
	if (!GetSafeHwnd())
		return;
	if (m_tooltip.GetSafeHwnd() == NULL)
	{
		m_tooltip.Create(this, TTS_ALWAYSTIP | TTS_NOPREFIX);
		m_tooltip.SetMaxTipWidth(280);
		m_tooltip.Activate(TRUE);
	}

	for (UINT i = 1; i <= m_tip_tool_count; ++i)	// 직전에 등록한 rect 툴 제거
		m_tooltip.DelTool(this, i);
	m_tip_tool_count = 0;

	UINT id = 1;
	for (const prop_row& row : m_rows)
	{
		if (row.is_section || !row.visible)
			continue;
		for (const prop_cell& c : row.cells)
		{
			// text/int/real/color 는 자식 CSCStatic 이 자체 툴팁 처리 → 제외. bool/combo/info 만 label 영역에.
			if (prop_cell::uses_static(c.type) || c.tooltip.IsEmpty())
				continue;
			CRect lr(c.rect.left + m_field_lpad, c.rect.top, c.rect.left + c.label_w, c.rect.bottom);
			lr.OffsetRect(0, -m_scroll_y);
			m_tooltip.AddTool(this, c.tooltip, lr, id);
			++id;
		}
	}
	m_tip_tool_count = id - 1;
}

void CSCPropertyCtrl::clamp_scroll()
{
	if (!GetSafeHwnd())
		return;
	CRect rc;
	GetClientRect(rc);
	const int maxs = (std::max)(0, m_content_h - rc.Height());
	m_scroll_y = (std::max)(0, (std::min)(maxs, m_scroll_y));
}

int CSCPropertyCtrl::hit_test_row(CPoint pt) const
{
	const CPoint cp(pt.x, pt.y + m_scroll_y);
	for (int i = 0; i < static_cast<int>(m_rows.size()); ++i)
		if (m_rows[i].visible && m_rows[i].rect.PtInRect(cp))
			return i;
	return -1;
}

void CSCPropertyCtrl::toggle_section(int row_index)
{
	if (row_index < 0 || row_index >= static_cast<int>(m_rows.size()))
		return;
	if (!m_rows[row_index].is_section)
		return;
	m_rows[row_index].expanded = !m_rows[row_index].expanded;
	layout();
	Invalidate(FALSE);
}

// ── 그리기 ──────────────────────────────────────────────────────────────────
// 삼각형은 Gdiplus 안티앨리어스로 그린다(GDI Polygon 은 계단현상). 색은 COLORREF→불투명 Gdiplus::Color.
void CSCPropertyCtrl::draw_caret(CDC& dc, const CRect& box, bool expanded, COLORREF cr)
{
	const CPoint c = box.CenterPoint();
	Gdiplus::PointF p[3];
	if (expanded)	// ▾ (아래)
	{
		p[0] = Gdiplus::PointF(c.x - 4.f, c.y - 2.f);
		p[1] = Gdiplus::PointF(c.x + 4.f, c.y - 2.f);
		p[2] = Gdiplus::PointF((float)c.x, c.y + 3.f);
	}
	else			// ▸ (오른쪽)
	{
		p[0] = Gdiplus::PointF(c.x - 2.f, c.y - 4.f);
		p[1] = Gdiplus::PointF(c.x - 2.f, c.y + 4.f);
		p[2] = Gdiplus::PointF(c.x + 3.f, (float)c.y);
	}
	Gdiplus::Graphics g(dc.GetSafeHdc());
	g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	Gdiplus::SolidBrush br(Gdiplus::Color(GetRValue(cr), GetGValue(cr), GetBValue(cr)));
	g.FillPolygon(&br, p, 3);
}

void CSCPropertyCtrl::draw_tri_down(CDC& dc, const CRect& box, COLORREF cr)
{
	const CPoint c = box.CenterPoint();
	Gdiplus::PointF p[3] = {
		Gdiplus::PointF(c.x - 4.f, c.y - 2.f),
		Gdiplus::PointF(c.x + 4.f, c.y - 2.f),
		Gdiplus::PointF((float)c.x, c.y + 3.f) };
	Gdiplus::Graphics g(dc.GetSafeHdc());
	g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	Gdiplus::SolidBrush br(Gdiplus::Color(GetRValue(cr), GetGValue(cr), GetBValue(cr)));
	g.FillPolygon(&br, p, 3);
}

void CSCPropertyCtrl::draw_section(CDC& dc, const prop_row& r, const CRect& rc)
{
	// 카테고리 헤더: 배경 밴드 없이 굵은 글씨. 위 구분선은 좌우 m_pad 여백(= 필드 박스 left/right 와 정렬)으로 대칭.
	dc.FillSolidRect(CRect(rc.left + m_pad, rc.top, rc.right - m_pad, rc.top + 1),
		m_theme.cr_border_inactive.ToCOLORREF());

	// 캐럿은 우측 끝(콤보 화살표와 동일 x), 제목은 왼쪽(필드 박스 left = m_pad 와 정렬)으로 당긴다.
	CRect caret(rc.right - m_pad - 20, rc.top, rc.right - m_pad - 6, rc.bottom);
	draw_caret(dc, caret, r.expanded, m_theme.cr_text_dim.ToCOLORREF());

	CFont* old = dc.SelectObject(&m_font_bold);
	CRect tr(rc.left + m_pad, rc.top, caret.left - 4, rc.bottom);
	dc.SetTextColor(m_theme.cr_text.ToCOLORREF());
	dc.DrawText(r.section_title, tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
	dc.SelectObject(old);
}

void CSCPropertyCtrl::draw_cell_label(CDC& dc, const prop_cell& c, const CRect& cell_rc)
{
	// 박스 안 흐린 라벨(prefix). 왼쪽 여백 + 세로 가운데. 모든 셀의 라벨을 여기 한 곳에서 통일되게 그린다.
	CRect lr(cell_rc.left + m_field_lpad, cell_rc.top, cell_rc.left + c.label_w, cell_rc.bottom);
	dc.SetTextColor(cell_label_color(c).ToCOLORREF());
	dc.DrawText(c.label, lr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
}

void CSCPropertyCtrl::draw_widget(CDC& dc, const prop_cell& c, const CRect& cell_rc)
{
	// combo 위젯(값 컬럼). bool 은 draw_bool 에서 별도 처리(라벨을 체크박스 직전까지 넓게 쓰기 위해).
	// 값 텍스트는 우측 정렬 — text/int/real/color 셀의 값 우측 정렬과 통일. 드롭다운 메뉴는 표준 컨벤션(좌측+radio mark) 유지.
	CRect vr = cell_value_rect(cell_rc, c.label_w);
	vr.right -= 6;
	dc.SetTextColor(cell_text_color(c).ToCOLORREF());

	CRect ar(vr.right - 14, vr.top, vr.right, vr.bottom);
	draw_tri_down(dc, ar, m_theme.cr_text_dim.ToCOLORREF());	// 화살표는 평시에도 dim — disabled 와 시각 구분 없음(체크박스 fill 로 식별)
	CString s;
	if (c.p_index && *c.p_index >= 0 && *c.p_index < static_cast<int>(c.options.size()))
		s = c.options[*c.p_index];
	CRect tr(vr.left, vr.top, ar.left - 4, vr.bottom);
	dc.DrawText(s, tr, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
}

// bool 셀: 체크박스를 셀 우측(여백 m_field_lpad)에 그리고, 라벨은 label_w 에 갇히지 않고 체크박스
// 직전까지 넓게 그린다(2-cell 배치에서 label_w 가 작아 라벨이 "..." 로 일찍 잘리던 문제 해결).
void CSCPropertyCtrl::draw_bool(CDC& dc, const prop_cell& c, const CRect& cell_rc)
{
	const bool v = c.p_bool && *c.p_bool;
	const int cyc = cell_rc.CenterPoint().y;
	CRect bx(cell_rc.right - m_field_lpad - 16, cyc - 8, cell_rc.right - m_field_lpad, cyc + 8);	// 우측 여백 = 좌측 prefix 와 대칭
	dc.FillSolidRect(bx, (v ? m_theme.cr_back_selected : m_theme.cr_back_alternate).ToCOLORREF());
	dc.Draw3dRect(bx, m_theme.cr_border_inactive.ToCOLORREF(), m_theme.cr_border_inactive.ToCOLORREF());
	if (v)
	{
		CRect t(bx);
		t.DeflateRect(4, 4);
		dc.FillSolidRect(t, cell_text_color(c).ToCOLORREF());	// disabled 면 체크표시도 dim
	}

	CRect lr(cell_rc.left + m_field_lpad, cell_rc.top, bx.left - m_label_gap, cell_rc.bottom);
	dc.SetTextColor(cell_label_color(c).ToCOLORREF());
	dc.DrawText(c.label, lr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
}

void CSCPropertyCtrl::OnPaint()
{
	CPaintDC dc1(this);
	CRect rc;
	GetClientRect(rc);

	CMemoryDC dc(&dc1, &rc);	// 더블 버퍼(소멸 시 화면으로 BitBlt). bBg=FALSE → 우리가 전체를 채워 그림.
	dc.FillSolidRect(rc, m_theme.cr_back.ToCOLORREF());
	CFont* old_font = dc.SelectObject(&m_font);
	dc.SetBkMode(TRANSPARENT);

	Gdiplus::Graphics g(dc.GetSafeHdc());
	g.SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);

	for (const prop_row& row : m_rows)
	{
		if (!row.visible)
			continue;

		CRect rr = row.rect;
		rr.OffsetRect(0, -m_scroll_y);
		if (rr.bottom < 0 || rr.top > rc.bottom)
			continue;

		if (row.is_section)
		{
			draw_section(dc, row, rr);
			continue;
		}

		for (const prop_cell& c : row.cells)
		{
			CRect cell_rc = c.rect;
			cell_rc.OffsetRect(0, -m_scroll_y);

			// 값 필드(text/int/real/color)는 CSCStatic 이 박스+라벨+값(+swatch)을 직접 그리므로 패널은 관여하지 않는다.
			if (prop_cell::uses_static(c.type))
				continue;

			// 나머지(bool/combo/info)는 패널이 라운드 박스(채움만, 테두리 없음) + 라벨 + 위젯/값을 그린다.
			CRect box = cell_rc;
			box.DeflateRect(0, m_field_vmargin);
			draw_round_rect(&g, CRect_to_gpRect(box),
				Gdiplus::Color::Transparent, field_back_color(), m_field_round, 0);

			if (c.type == field_type::boolean)
			{
				draw_bool(dc, c, cell_rc);	// 라벨(넓게) + 체크박스(우측) 자체 처리
			}
			else
			{
				draw_cell_label(dc, c, cell_rc);
				if (c.type == field_type::combo)
					draw_widget(dc, c, cell_rc);
				else if (c.type == field_type::info)
				{
					CRect vr = cell_value_rect(cell_rc, c.label_w);
					dc.SetTextColor(cell_text_color(c).ToCOLORREF());	// 값 색 — enabled=cr_text / disabled=cr_text_dim
					dc.DrawText(c.info_text, vr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
				}
			}
		}
	}

	dc.SelectObject(old_font);
}

BOOL CSCPropertyCtrl::OnEraseBkgnd(CDC* /*pDC*/)
{
	return TRUE;
}

void CSCPropertyCtrl::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);
	layout();
	// 절충안 — *패널 자체* 만 sync paint (RDW_UPDATENOW, 자식 제외), *자식 CSCStatic* 은 position_cell_ctrl 의
	// async InvalidateRect 로 큐 dispatch. 이전 RDW_UPDATENOW|RDW_ALLCHILDREN 은 자식까지 sync paint 라
	// UI thread 가 splitter drag 중 점유돼 호스트 animation(view) 가 끊겼었다. 자식 async 로 view 부드러움
	// + 패널 자체 sync 로 PropertyCtrl 배경/section header 잔떨림 최소.
	RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
}

BOOL CSCPropertyCtrl::OnMouseWheel(UINT /*nFlags*/, short zDelta, CPoint pt)
{
	// 커서가 콤보 값 영역 위면 패널 스크롤 대신 항목을 바꾼다(CSCMenu 기반이라 기본 콤보의 휠 선택이 없으므로 보완).
	// pt 는 스크린 좌표 → 클라이언트로 변환 후 hit-test.
	CPoint cp = pt;
	ScreenToClient(&cp);
	const int i = hit_test_row(cp);
	if (i >= 0 && !m_rows[i].is_section)
	{
		const CPoint hit(cp.x, cp.y + m_scroll_y);	// 스크롤 보정(셀 rect 는 스크롤 적용 전 좌표)
		for (prop_cell& c : m_rows[i].cells)
		{
			if (c.type != field_type::combo || !c.p_index || c.options.empty())
				continue;
			if (!c.enabled)
				continue;					// disabled combo: 휠 cycle 차단(패널 스크롤로 폴-스루)
			if (!cell_value_rect(c.rect, c.label_w).PtInRect(hit))
				continue;
			const int n = static_cast<int>(c.options.size());
			int idx = *c.p_index + (zDelta > 0 ? -1 : 1);	// 위로 굴림=이전 항목, 아래=다음 항목
			idx = (std::max)(0, (std::min)(n - 1, idx));	// 양 끝에서 멈춤(wrap 안 함, 기본 콤보와 동일)
			if (idx != *c.p_index)
			{
				*c.p_index = idx;
				if (on_change)
					on_change(c.label);
				Invalidate(FALSE);
			}
			return TRUE;	// 콤보에서 소비 — 패널 스크롤로 흘리지 않음
		}
	}

	const int step = m_row_h * 3;
	m_scroll_y -= zDelta * step / WHEEL_DELTA;
	clamp_scroll();
	layout();		// 자식 위치 재배치
	Invalidate(FALSE);
	return TRUE;
}

void CSCPropertyCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetFocus();

	const int i = hit_test_row(point);
	if (i >= 0)
	{
		prop_row& row = m_rows[i];
		if (row.is_section)
		{
			toggle_section(i);
		}
		else
		{
			const CPoint cp(point.x, point.y + m_scroll_y);
			for (prop_cell& c : row.cells)
			{
				if (!c.rect.PtInRect(cp))
					continue;
				if (!c.enabled)
					break;					// disabled 셀: 클릭 소비(다른 셀로 전파 막음)하되 아무 동작 안 함

				if (c.type == field_type::boolean)
				{
					if (c.p_bool)
					{
						*c.p_bool = !*c.p_bool;
						if (on_change)
							on_change(c.label);
						Invalidate(FALSE);
					}
				}
				else if (c.type == field_type::combo)
				{
					// 값 컬럼 클릭일 때만 드롭다운(라벨 클릭은 무시).
					if (!cell_value_rect(c.rect, c.label_w).PtInRect(cp))
						break;
					// 메뉴가 떠 있을 때 트리거를 다시 누르면 토글 닫기여야 한다(재오픈 금지).
					// 그 디바운스는 CSCMenu::popup_menu 가 자체 처리(공통 모듈에서 해결).
					if (c.p_index && !c.options.empty())
						open_combo_menu(c);
				}
				// text/int/real/color/info 는 자식 CSCStatic 이 클릭/편집 처리.
				break;
			}
		}
	}

	CDialogEx::OnLButtonDown(nFlags, point);
}

// combo 셀 클릭 → 다크 테마 CSCMenu 팝업. 옵션을 항목으로 채우고 현재 선택을 radio 로 표시한 뒤
// 값 영역 좌하단에서 띄운다. 선택은 동기 반환이 아니라 Message_CSCMenu(selchanged) 로 통지된다.
void CSCPropertyCtrl::open_combo_menu(prop_cell& c)
{
	if (m_combo_menu.GetSafeHwnd() == NULL)
	{
		// 값 컬럼 너비를 최소 폭으로 — 항목 텍스트가 더 길면 CSCMenu 가 자동 확장.
		const int w = (std::max)(80, cell_value_rect(c.rect, c.label_w).Width());
		m_combo_menu.create(this, w);
	}
	m_combo_menu.set_color_theme(m_theme);	// 패널 테마와 동기(테마 교체에도 일관)

	m_combo_active = &c;

	// 옵션을 항목으로 채운다. id = 인덱스+1 (CSCMenu 는 id<=0 을 separator 로 취급).
	m_combo_menu.clear();
	m_combo_menu.begin_update();
	for (int k = 0; k < static_cast<int>(c.options.size()); ++k)
		m_combo_menu.add(k + 1, c.options[k]);
	m_combo_menu.end_update();
	if (*c.p_index >= 0 && *c.p_index < static_cast<int>(c.options.size()))
		m_combo_menu.check_radio_item(1, static_cast<int>(c.options.size()), *c.p_index + 1);

	// 값 영역 좌하단(스크린 좌표)에서 팝업.
	CRect vr = cell_value_rect(c.rect, c.label_w);
	vr.OffsetRect(0, -m_scroll_y);
	CPoint pt(vr.left, vr.bottom);
	ClientToScreen(&pt);
	m_combo_menu.popup_menu(pt.x, pt.y);
}

// combo CSCMenu 선택 통지 → 대상 셀(m_combo_active)의 바인딩 인덱스 갱신 + on_change.
LRESULT CSCPropertyCtrl::on_message_CSCMenu(WPARAM wParam, LPARAM /*lParam*/)
{
	CSCMenuMessage* msg = reinterpret_cast<CSCMenuMessage*>(wParam);
	if (msg == nullptr)
		return 0;

	if (msg->m_message == CSCMenu::message_scmenu_selchanged && m_combo_active && msg->m_menu_item)
	{
		const int idx = msg->m_menu_item->m_id - 1;	// id = 인덱스+1
		if (m_combo_active->p_index && idx >= 0 && idx < static_cast<int>(m_combo_active->options.size()))
		{
			*m_combo_active->p_index = idx;
			if (on_change)
				on_change(m_combo_active->label);
			Invalidate(FALSE);
		}
	}
	else if (msg->m_message == CSCMenu::message_scmenu_hide)
	{
		m_combo_active = nullptr;	// 메뉴 닫힘 — dangling 방지
	}
	return 0;
}

// CSCStatic 값 변경 통지 → 어느 셀인지 찾아 바인딩 포인터에 되쓰고 on_change.
LRESULT CSCPropertyCtrl::on_message_CSCStatic(WPARAM wParam, LPARAM /*lParam*/)
{
	CSCStaticMsg* msg = reinterpret_cast<CSCStaticMsg*>(wParam);
	if (msg == nullptr)
		return 0;

	prop_cell* pc = find_cell_by_ctrl(msg->pThis);
	if (pc == nullptr)
		return 0;
	prop_cell& c = *pc;

	if (msg->msg == CSCStaticMsg::msg_text_value_changed)
	{
		const CString s = msg->sValue;
		switch (c.type)
		{
		case field_type::text:
			if (c.p_text)
				*c.p_text = s;
			break;
		case field_type::integer:
			if (c.p_int)
				*c.p_int = _ttoi(s);
			break;
		case field_type::real:
			if (c.p_real)
				*c.p_real = static_cast<float>(_ttof(s));
			break;
		case field_type::color24:
		case field_type::color32:
			if (c.p_color)
			{
				// color32 → "R,G,B,A"(4값), color24 → "R,G,B"(3값, alpha 무시 → 불투명 255).
				int cr = 0, cg = 0, cb = 0, ca = 255;
				int n = _stscanf_s(s, _T("%d , %d , %d , %d"), &cr, &cg, &cb, &ca);
				if (n == 3 || n == 4)
					*c.p_color = Gdiplus::Color(
						static_cast<BYTE>((n == 4) ? (std::max)(0, (std::min)(255, ca)) : 255),
						static_cast<BYTE>((std::max)(0, (std::min)(255, cr))),
						static_cast<BYTE>((std::max)(0, (std::min)(255, cg))),
						static_cast<BYTE>((std::max)(0, (std::min)(255, cb))));
			}
			break;
		default:
			break;
		}
		if (on_change)
			on_change(c.label);
	}
	return 0;
}
