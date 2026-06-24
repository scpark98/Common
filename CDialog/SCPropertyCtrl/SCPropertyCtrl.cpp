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

BEGIN_MESSAGE_MAP(CSCPropertyCtrl, CDialogEx)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_MOUSEWHEEL()
	ON_WM_LBUTTONDOWN()
	ON_REGISTERED_MESSAGE(Message_CSCStatic, &CSCPropertyCtrl::on_message_CSCStatic)
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

	m_font.CreateFont(-13, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, _T("Segoe UI"));

	return true;
}

void CSCPropertyCtrl::set_color_theme(const CSCColorTheme& theme, bool invalidate)
{
	m_theme.copy_colors_from(theme);
	for (prop_row& r : m_rows)			// 이미 생성된 자식에도 전파
		if (r.ctrl && r.ctrl->GetSafeHwnd())
		{
			r.ctrl->set_color_theme(m_theme, false);
			if (r.type != row_type::color && r.type != row_type::info)
			{
				r.ctrl->set_edit_text_color(m_theme.cr_text);
				r.ctrl->set_edit_back_color(m_theme.cr_back_alternate);
			}
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
}

void CSCPropertyCtrl::section(const CString& title, bool expanded)
{
	prop_row r;
	r.type = row_type::section;
	r.label = title;
	r.expanded = expanded;
	m_rows.push_back(r);
	m_cur_section = static_cast<int>(m_rows.size()) - 1;
	m_rows.back().section_index = m_cur_section;
}

void CSCPropertyCtrl::add_text(const CString& label, CString* value)
{
	prop_row r; r.type = row_type::text; r.label = label; r.p_text = value; r.section_index = m_cur_section;
	m_rows.push_back(r);
}
void CSCPropertyCtrl::add_int(const CString& label, int* value)
{
	prop_row r; r.type = row_type::integer; r.label = label; r.p_int = value; r.section_index = m_cur_section;
	m_rows.push_back(r);
}
void CSCPropertyCtrl::add_real(const CString& label, float* value)
{
	prop_row r; r.type = row_type::real; r.label = label; r.p_real = value; r.section_index = m_cur_section;
	m_rows.push_back(r);
}
void CSCPropertyCtrl::add_bool(const CString& label, bool* value)
{
	prop_row r; r.type = row_type::boolean; r.label = label; r.p_bool = value; r.section_index = m_cur_section;
	m_rows.push_back(r);
}
void CSCPropertyCtrl::add_color(const CString& label, Gdiplus::Color* value)
{
	prop_row r; r.type = row_type::color; r.label = label; r.p_color = value; r.section_index = m_cur_section;
	m_rows.push_back(r);
}
void CSCPropertyCtrl::add_combo(const CString& label, int* index, const std::vector<CString>& options)
{
	prop_row r; r.type = row_type::combo; r.label = label; r.p_index = index; r.options = options; r.section_index = m_cur_section;
	m_rows.push_back(r);
}
void CSCPropertyCtrl::add_info(const CString& label, const CString& text)
{
	prop_row r; r.type = row_type::info; r.label = label; r.info_text = text; r.section_index = m_cur_section;
	m_rows.push_back(r);
}

void CSCPropertyCtrl::end()
{
	ensure_controls();
	layout();
	if (GetSafeHwnd())
		Invalidate(FALSE);
}

void CSCPropertyCtrl::refresh()
{
	for (prop_row& r : m_rows)
		sync_value(r);
	if (GetSafeHwnd())
		Invalidate(FALSE);
}

// ── 자식 컨트롤(CSCStatic) 관리 ────────────────────────────────────────────
void CSCPropertyCtrl::destroy_controls()
{
	for (prop_row& r : m_rows)
		if (r.ctrl && r.ctrl->GetSafeHwnd())
			r.ctrl->DestroyWindow();
}

void CSCPropertyCtrl::ensure_controls()
{
	if (!GetSafeHwnd())
		return;

	CRect rc;
	GetClientRect(rc);
	const int label_w = (rc.Width() - 2 * m_pad) * m_label_ratio / 100;	// 컬럼 정렬용 라벨 폭

	UINT id = 100;
	for (prop_row& r : m_rows)
	{
		if (!uses_static(r.type))
			continue;
		if (r.ctrl && r.ctrl->GetSafeHwnd())
		{
			++id;
			continue;
		}

		const bool is_color = (r.type == row_type::color);

		r.ctrl = std::make_shared<CSCStatic>();
		// color: "_color picker_" 캡션 → swatch+RGB, swatch 클릭 시 색 대화상자. 라벨은 패널이 그림.
		// 그 외(text/int/real/info): CSCStatic 이 label+value 를 직접 표시(UXStudio 방식). 값은 m_use_edit 일 때만 그려진다.
		const LPCTSTR caption = is_color ? _T("_color picker_") : static_cast<LPCTSTR>(r.label);
		r.ctrl->create(caption, WS_CHILD | SS_NOTIFY, CRect(0, 0, 10, 10), this, id++);
		r.ctrl->set_color_theme(m_theme, false);

		// 값(RGB/숫자) 텍스트·편집 색 — 다크 패널 대비 보장(빠지면 색이 묻힘).
		r.ctrl->set_edit_text_color(m_theme.cr_text);
		r.ctrl->set_edit_back_color(m_theme.cr_back_alternate);

		if (is_color)
		{
			// "_color picker_" 모드 — swatch 클릭 핸들러가 m_use_edit 게이트 안에 있으므로 켜야 동작.
			r.ctrl->set_use_edit(true);
		}
		else
		{
			r.ctrl->set_label_width(label_w);	// label 컬럼 고정 → 값 시작 위치 항목마다 일치
			if (r.type != row_type::info)		// info 는 읽기 전용(라벨만). 편집 행만 set_use_edit.
			{
				// 값/편집을 좌측 정렬 → swatch·콤보 텍스트의 left 와 동일 위치에서 시작.
				r.ctrl->set_use_edit(true, ES_LEFT);
				r.ctrl->set_value_halign(DT_LEFT);
				if (r.type == row_type::integer || r.type == row_type::real)
					r.ctrl->set_use_updown_key(true, r.type == row_type::real ? 0.1f : 1.0f);
			}
		}
		sync_value(r);
	}
}

void CSCPropertyCtrl::sync_value(prop_row& r)
{
	if (!r.ctrl || !r.ctrl->GetSafeHwnd())
		return;
	switch (r.type)
	{
	case row_type::text:
		r.ctrl->set_text_value(r.p_text ? *r.p_text : CString());
		break;
	case row_type::integer:
	{
		CString s; if (r.p_int) s.Format(_T("%d"), *r.p_int);
		r.ctrl->set_text_value(s);
		break;
	}
	case row_type::real:
	{
		CString s; if (r.p_real) s.Format(_T("%g"), *r.p_real);
		r.ctrl->set_text_value(s);
		break;
	}
	case row_type::color:
		if (r.p_color)
			r.ctrl->set_text_color(*r.p_color);	// "_color picker_" 모드의 swatch 색 = m_theme.cr_text
		break;
	case row_type::info:
		r.ctrl->set_text_value(r.info_text);
		break;
	default:
		break;
	}
}

int CSCPropertyCtrl::find_row_by_ctrl(const CWnd* pCtrl) const
{
	for (int i = 0; i < static_cast<int>(m_rows.size()); ++i)
		if (m_rows[i].ctrl && m_rows[i].ctrl.get() == pCtrl)
			return i;
	return -1;
}

CRect CSCPropertyCtrl::value_rect(const CRect& row_rect) const
{
	const int label_w = (row_rect.Width() - 2 * m_pad) * m_label_ratio / 100;
	return CRect(row_rect.left + m_pad + label_w, row_rect.top, row_rect.right - m_pad, row_rect.bottom);
}

// ── 레이아웃(세로 누적 + 자식 위치) ─────────────────────────────────────────
void CSCPropertyCtrl::layout()
{
	if (!GetSafeHwnd())
		return;
	CRect rc;
	GetClientRect(rc);
	const int W = rc.Width();

	int y = 0;
	for (prop_row& r : m_rows)
	{
		if (r.type == row_type::section)
		{
			r.visible = true;
			r.rect = CRect(0, y, W, y + m_section_h);
			y += m_section_h;
		}
		else
		{
			const bool exp = (r.section_index >= 0 && r.section_index < static_cast<int>(m_rows.size()))
				? m_rows[r.section_index].expanded : true;
			if (exp)
			{
				r.visible = true;
				r.rect = CRect(0, y, W, y + m_row_h);
				y += m_row_h;
			}
			else
			{
				r.visible = false;
				r.rect = CRect(0, y, W, y);
			}
		}

		// 값 행 자식 위치/표시.
		if (r.ctrl && r.ctrl->GetSafeHwnd())
		{
			if (r.visible)
			{
				CRect rr = r.rect;
				rr.OffsetRect(0, -m_scroll_y);
				// color 는 라벨을 패널이 그리므로 값 컬럼만, 그 외는 label+value 를 CSCStatic 이 그리므로 콘텐츠 전체.
				CRect cr = (r.type == row_type::color)
					? value_rect(rr)
					: CRect(rr.left + m_pad, rr.top, rr.right - m_pad, rr.bottom);
				cr.DeflateRect(0, 2);
				r.ctrl->MoveWindow(cr);
				r.ctrl->ShowWindow(SW_SHOW);
			}
			else
			{
				r.ctrl->ShowWindow(SW_HIDE);
			}
		}
	}
	m_content_h = y;
	clamp_scroll();
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

int CSCPropertyCtrl::hit_test(CPoint pt) const
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
	if (m_rows[row_index].type != row_type::section)
		return;
	m_rows[row_index].expanded = !m_rows[row_index].expanded;
	layout();
	Invalidate(FALSE);
}

// ── 그리기 ──────────────────────────────────────────────────────────────────
void CSCPropertyCtrl::draw_caret(CDC& dc, const CRect& box, bool expanded, COLORREF cr)
{
	const CPoint c = box.CenterPoint();
	CPoint p[3];
	if (expanded)	// ▾ (아래)
	{
		p[0] = CPoint(c.x - 4, c.y - 2);
		p[1] = CPoint(c.x + 4, c.y - 2);
		p[2] = CPoint(c.x,     c.y + 3);
	}
	else			// ▸ (오른쪽)
	{
		p[0] = CPoint(c.x - 2, c.y - 4);
		p[1] = CPoint(c.x - 2, c.y + 4);
		p[2] = CPoint(c.x + 3, c.y);
	}
	CBrush br(cr); CPen pen(PS_SOLID, 1, cr);
	CBrush* ob = dc.SelectObject(&br); CPen* op = dc.SelectObject(&pen);
	dc.Polygon(p, 3);
	dc.SelectObject(ob); dc.SelectObject(op);
}

void CSCPropertyCtrl::draw_tri_down(CDC& dc, const CRect& box, COLORREF cr)
{
	const CPoint c = box.CenterPoint();
	CPoint p[3] = { CPoint(c.x - 4, c.y - 2), CPoint(c.x + 4, c.y - 2), CPoint(c.x, c.y + 3) };
	CBrush br(cr); CPen pen(PS_SOLID, 1, cr);
	CBrush* ob = dc.SelectObject(&br); CPen* op = dc.SelectObject(&pen);
	dc.Polygon(p, 3);
	dc.SelectObject(ob); dc.SelectObject(op);
}

void CSCPropertyCtrl::draw_section(CDC& dc, const prop_row& r, const CRect& rc)
{
	dc.FillSolidRect(rc, m_theme.cr_back_alternate.ToCOLORREF());

	CRect caret(rc.left + m_pad, rc.top, rc.left + m_pad + 12, rc.bottom);
	draw_caret(dc, caret, r.expanded, m_theme.cr_text.ToCOLORREF());

	CRect tr(caret.right + 6, rc.top, rc.right - m_pad, rc.bottom);
	dc.SetTextColor(m_theme.cr_text.ToCOLORREF());
	dc.DrawText(r.label, tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

	dc.FillSolidRect(CRect(rc.left, rc.bottom - 1, rc.right, rc.bottom), m_theme.cr_border_inactive.ToCOLORREF());
}

void CSCPropertyCtrl::draw_row_label(CDC& dc, const prop_row& r, const CRect& rc)
{
	const int label_w = (rc.Width() - 2 * m_pad) * m_label_ratio / 100;
	CRect lr(rc.left + m_pad, rc.top, rc.left + m_pad + label_w, rc.bottom);
	dc.SetTextColor(m_theme.cr_text.ToCOLORREF());	// CSCStatic 라벨(cr_text)과 동일 밝기로 일관
	dc.DrawText(r.label, lr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
}

void CSCPropertyCtrl::draw_widget(CDC& dc, const prop_row& r, const CRect& rc)
{
	const CRect vr = value_rect(rc);
	dc.SetTextColor(m_theme.cr_text.ToCOLORREF());

	if (r.type == row_type::boolean)
	{
		const bool v = r.p_bool && *r.p_bool;
		const int cyc = vr.CenterPoint().y;
		CRect bx(vr.left, cyc - 8, vr.left + 16, cyc + 8);
		dc.FillSolidRect(bx, (v ? m_theme.cr_back_selected : m_theme.cr_back_alternate).ToCOLORREF());
		dc.Draw3dRect(bx, m_theme.cr_border_inactive.ToCOLORREF(), m_theme.cr_border_inactive.ToCOLORREF());
		if (v)
		{
			CRect t(bx); t.DeflateRect(4, 4);
			dc.FillSolidRect(t, m_theme.cr_text.ToCOLORREF());
		}
	}
	else if (r.type == row_type::combo)
	{
		CRect ar(vr.right - 14, vr.top, vr.right, vr.bottom);
		draw_tri_down(dc, ar, m_theme.cr_text_dim.ToCOLORREF());
		CString s;
		if (r.p_index && *r.p_index >= 0 && *r.p_index < static_cast<int>(r.options.size()))
			s = r.options[*r.p_index];
		CRect tr(vr.left, vr.top, ar.left - 4, vr.bottom);
		dc.DrawText(s, tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
	}
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

	for (const prop_row& r : m_rows)
	{
		if (!r.visible)
			continue;
		CRect rr = r.rect;
		rr.OffsetRect(0, -m_scroll_y);
		if (rr.bottom < 0 || rr.top > rc.bottom)
			continue;

		if (r.type == row_type::section)
		{
			draw_section(dc, r, rr);
		}
		else if (r.type == row_type::color || r.type == row_type::boolean || r.type == row_type::combo)
		{
			// 라벨은 패널이 그림. color 는 값 컬럼을 CSCStatic 이, bool/combo 는 위젯을 패널이 그린다.
			draw_row_label(dc, r, rr);
			if (r.type == row_type::boolean || r.type == row_type::combo)
				draw_widget(dc, r, rr);
		}
		// text/int/real/info: CSCStatic 이 label+value 를 모두 그리므로 패널은 그리지 않음.
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
	Invalidate(FALSE);
}

BOOL CSCPropertyCtrl::OnMouseWheel(UINT /*nFlags*/, short zDelta, CPoint /*pt*/)
{
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

	const int i = hit_test(point);
	if (i >= 0)
	{
		prop_row& r = m_rows[i];
		switch (r.type)
		{
		case row_type::section:
			toggle_section(i);
			break;

		case row_type::boolean:
			if (r.p_bool)
			{
				*r.p_bool = !*r.p_bool;
				if (on_change) on_change(r.label);
				Invalidate(FALSE);
			}
			break;

		case row_type::combo:		// 값 컬럼 클릭일 때만 드롭다운(라벨 클릭은 무시)
		{
			CRect rrc = r.rect;
			rrc.OffsetRect(0, -m_scroll_y);
			if (!value_rect(rrc).PtInRect(point))
				break;
			if (r.p_index && !r.options.empty())
			{
				CMenu menu;
				menu.CreatePopupMenu();
				for (int k = 0; k < static_cast<int>(r.options.size()); ++k)
					menu.AppendMenu(MF_STRING, static_cast<UINT>(k + 1), r.options[k]);
				if (*r.p_index >= 0 && *r.p_index < static_cast<int>(r.options.size()))
					menu.CheckMenuRadioItem(0, static_cast<UINT>(r.options.size()) - 1,
						static_cast<UINT>(*r.p_index), MF_BYPOSITION);

				CRect rr = r.rect;
				rr.OffsetRect(0, -m_scroll_y);
				const CRect vr = value_rect(rr);
				CPoint pt(vr.left, vr.bottom);
				ClientToScreen(&pt);

				const int cmd = menu.TrackPopupMenu(
					TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN, pt.x, pt.y, this);
				if (cmd > 0)
				{
					*r.p_index = cmd - 1;
					if (on_change) on_change(r.label);
					Invalidate(FALSE);
				}
			}
			break;
		}

		default:	// text/int/real/color/info 는 자식 CSCStatic 이 클릭/편집 처리.
			break;
		}
	}

	CDialogEx::OnLButtonDown(nFlags, point);
}

// CSCStatic 값 변경 통지 → 어느 행인지 찾아 바인딩 포인터에 되쓰고 on_change.
LRESULT CSCPropertyCtrl::on_message_CSCStatic(WPARAM wParam, LPARAM /*lParam*/)
{
	CSCStaticMsg* msg = reinterpret_cast<CSCStaticMsg*>(wParam);
	if (msg == nullptr)
		return 0;

	const int i = find_row_by_ctrl(msg->pThis);
	if (i < 0)
		return 0;
	prop_row& r = m_rows[i];

	if (msg->msg == CSCStaticMsg::msg_text_value_changed)
	{
		const CString s = msg->sValue;
		switch (r.type)
		{
		case row_type::text:
			if (r.p_text) *r.p_text = s;
			break;
		case row_type::integer:
			if (r.p_int) *r.p_int = _ttoi(s);
			break;
		case row_type::real:
			if (r.p_real) *r.p_real = static_cast<float>(_ttof(s));
			break;
		case row_type::color:
			if (r.p_color)
			{
				int cr = 0, cg = 0, cb = 0;
				if (_stscanf_s(s, _T("%d , %d , %d"), &cr, &cg, &cb) == 3)
					*r.p_color = Gdiplus::Color(255,
						static_cast<BYTE>((std::max)(0, (std::min)(255, cr))),
						static_cast<BYTE>((std::max)(0, (std::min)(255, cg))),
						static_cast<BYTE>((std::max)(0, (std::min)(255, cb))));
			}
			break;
		default:
			break;
		}
		if (on_change)
			on_change(r.label);
	}
	return 0;
}
