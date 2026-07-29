// SCStepCtrl.cpp: 구현 파일
//

#include "SCStepCtrl.h"

#include "../../Functions.h"
#include "../../MemoryDC.h"

// CSCStepCtrl

IMPLEMENT_DYNAMIC(CSCStepCtrl, CStatic)

CSCStepCtrl::CSCStepCtrl()
{
	m_cr_back.SetFromCOLORREF(::GetSysColor(COLOR_3DFACE));
	memset(&m_lf, 0, sizeof(LOGFONT));
}

CSCStepCtrl::~CSCStepCtrl()
{
}


BEGIN_MESSAGE_MAP(CSCStepCtrl, CStatic)
	ON_WM_PAINT()
	ON_WM_SIZE()
END_MESSAGE_MAP()



// CSCStepCtrl 메시지 처리기
void CSCStepCtrl::OnPaint()
{
	CPaintDC dc1(this); // device context for painting

	int i;
	CRect rc;
	GetClientRect(rc);

	CMemoryDC dc(&dc1, &rc);

	Gdiplus::Graphics g(dc.m_hDC);
	g.SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);
	g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);

	CFont* pOldFont = (CFont*)dc.SelectObject(&m_font);

	dc.FillSolidRect(rc, m_cr_back.ToCOLORREF());
	//draw_rect(g, rc, Gdiplus::Color::Red, m_cr_back);

	if (m_step.size() == 0)
		return;


	//스텝의 시작과 끝의 여백.
	int interval;
	if (m_horz)
		interval = (rc.Width() - m_margin.left - m_margin.right - m_thumb_size * m_step.size()) / (m_step.size() - 1);
	else
		interval = (rc.Height() - m_margin.top - m_margin.bottom - m_thumb_size * m_step.size()) / (m_step.size() - 1);

	for (i = 0; i < m_step.size(); i++)
	{
		int thumb_style = m_thumb_style;

		Gdiplus::Color cr_thumb = get_thumb_color(i);
		Gdiplus::Color cr_text = m_cr_text_active;
		Gdiplus::Color cr_line = m_cr_line_active;

		if (i > m_pos)
		{
			cr_text = m_cr_text_inactive;
			cr_line = m_cr_line_inactive;
		}
		else if (i == m_pos)
		{
			cr_text = m_cr_text_current;
		}

		//thumb_style, text의 색상을 별도로 지정한 위치라면 그 설정대로 그려준다.
		if (m_step[i].thumb_style != thumb_style_none)
			thumb_style = m_step[i].thumb_style;

		if (m_step[i].cr_text.GetValue() != Gdiplus::Color::Transparent)
			cr_text = m_step[i].cr_text;


		if (m_step[i].r.IsRectEmpty())
		{
			if (m_horz)
			{
				m_step[i].r = make_rect(m_margin.left + (interval + m_thumb_size) * i, m_margin.top, m_thumb_size, m_thumb_size);
			}
			else
			{
				m_step[i].r = make_rect(m_margin.left, m_margin.top + (interval + m_thumb_size) * i, m_thumb_size, m_thumb_size);
			}
		}

		if (thumb_style == thumb_style_rect)
			g.FillRectangle(&Gdiplus::SolidBrush(cr_thumb), CRect_to_gpRect(m_step[i].r));
		else
			g.FillEllipse(&Gdiplus::SolidBrush(cr_thumb), CRect_to_gpRect(m_step[i].r));

		//pos 미만은 채워진 원에 체크 표시를
		if (i < m_pos)
		{
			//20260729 by claude. 체크 색은 그 스텝의 채움색에서 파생한다 — 실패로 개별 지정된 스텝이면 그 색 기준.
			Gdiplus::Color cr_check = get_check_color(cr_thumb);

			//20260728 by claude. 체크 표시 굵기 1.0 -> 1.8. 1.0 은 thumb(18px) 안에서 너무 가늘어 잘 안 보였다.
			//짧은 왼쪽 획은 굵어지면서 상대적으로 더 짧아 보여 시작점을 left/top 으로 1px 씩 늘렸다.
			draw_line(g, m_step[i].r.CenterPoint().x - 4, m_step[i].r.CenterPoint().y - 1, m_step[i].r.CenterPoint().x - 1, m_step[i].r.CenterPoint().y + 2, cr_check, 1.8f);
			draw_line(g, m_step[i].r.CenterPoint().x - 1, m_step[i].r.CenterPoint().y + 2, m_step[i].r.CenterPoint().x + 3, m_step[i].r.CenterPoint().y - 2, cr_check, 1.8f);
		}
		else if (i == m_pos)
		{
			CRect rthumb_small = m_step[i].r;

			//20260729 by claude. 링 안쪽 여백은 흰색 고정이 아니라 컨트롤 배경색이어야 한다.
			//흰색으로 두면 배경이 흰색이 아닌 테마(본문 250, dark 테마 등)에서 흰 고리가 도드라진다.
			rthumb_small.DeflateRect(2, 2);
			if (thumb_style == thumb_style_rect)
				g.FillRectangle(&Gdiplus::SolidBrush(m_cr_back), CRect_to_gpRect(rthumb_small));
			else
				g.FillEllipse(&Gdiplus::SolidBrush(m_cr_back), CRect_to_gpRect(rthumb_small));

			//20260728 by claude. 안쪽 채움 원을 한 변당 1px 더 줄인다(지름 -2). 흰 원은 그대로.
			rthumb_small.DeflateRect(3, 3);
			if (thumb_style == thumb_style_rect)
				//g.FillRectangle(&Gdiplus::SolidBrush(cr_thumb), CRect_to_gpRect(rthumb_small));
				draw_rect(g, rthumb_small, Gdiplus::Color::Transparent, cr_thumb);
			else if (thumb_style == thumb_style_diamond)
				;// draw_diamond(g, rthumb_small, Gdiplus::Color::Transparent, cr_thumb);
			else if (thumb_style == thumb_style_circle)
				g.FillEllipse(&Gdiplus::SolidBrush(cr_thumb), CRect_to_gpRect(rthumb_small));
				//draw_ellipse(&dc,)
		}
		else
		{
			CRect rthumb_small = m_step[i].r;

			//대기 스텝은 1px 테두리만 남긴다. 안쪽은 위와 같은 이유로 컨트롤 배경색.
			rthumb_small.DeflateRect(1, 1);
			if (thumb_style == thumb_style_rect)
				g.FillRectangle(&Gdiplus::SolidBrush(m_cr_back), CRect_to_gpRect(rthumb_small));
			else
				g.FillEllipse(&Gdiplus::SolidBrush(m_cr_back), CRect_to_gpRect(rthumb_small));
		}

		//draw text
		if (!m_step[i].text.IsEmpty())
		{
			LOGFONT lf;
			CFont* pOldFont = NULL;

			m_font.GetLogFont(&lf);

			if (i == m_pos)
				lf.lfWeight = FW_BOLD;
			else
				lf.lfWeight = FW_NORMAL;

			m_font.DeleteObject();
			m_font.CreateFontIndirect(&lf);
			pOldFont = (CFont*)dc.SelectObject(&m_font);

			CRect rtext;
			CSize sz_text = dc.GetTextExtent(m_step[i].text);

			dc.SetBkMode(TRANSPARENT);

			if (m_horz)
				rtext = make_rect(m_step[i].r.CenterPoint().x - sz_text.cx / 2, m_step[i].r.bottom + m_gap_to_text, sz_text.cx, m_step[i].r.Height());
			else
				rtext = make_rect(m_step[i].r.right + m_gap_to_text, m_step[i].r.top, sz_text.cx, m_step[i].r.Height());

			//if (i > m_pos)
			//	dc.SetTextColor(m_cr_text_inactive.ToCOLORREF());
			//else if (i == m_pos)
			//	dc.SetTextColor(m_cr_text_current.ToCOLORREF());
			//else
			//	dc.SetTextColor(m_cr_text_active.ToCOLORREF());
			dc.SetTextColor(cr_text.ToCOLORREF());

			dc.DrawText(m_step[i].text, rtext, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);
#ifdef _DEBUG
			//draw_rect(g, rtext, Gdiplus::Color::Red);	//text 영역 확인용
#endif
			dc.SelectObject(pOldFont);
		}

		//각 스텝 사이의 라인을 그려준다.
		if (i > 0)
		{
			//20260728 by claude. 연결선은 "이전 스텝에서 나가는 선"이므로 이전 스텝 기준으로 색을 정한다.
			//예전엔 이번 스텝(i)의 색으로 그려서, 마지막 스텝이 실패로 빨간색이면 거기까지 정상 진행했던
			//직전 구간의 선까지 빨갛게 보였다(실패는 그 스텝에서 났는데 오는 길이 실패로 표시됨).
			Gdiplus::Pen pen_line(get_line_color(i - 1), 1.7f);

			if (m_horz)
			{
				//draw_line(&dc, m_step[i - 1].r.right + 0, m_step[i].r.CenterPoint().y,
				//	m_step[i].r.left - 0, m_step[i].r.CenterPoint().y, (i > m_pos) ? m_cr_line_inactive : m_cr_line_inactive, 2.0f, Gdiplus::DashStyleDash);
				g.DrawLine(&pen_line,
					m_step[i - 1].r.right + 0, m_step[i].r.CenterPoint().y,
					m_step[i].r.left - 0, m_step[i].r.CenterPoint().y);
			}
			else
			{
				g.DrawLine(&pen_line,
					m_step[i - 1].r.CenterPoint().x, m_step[i - 1].r.bottom,
					m_step[i].r.CenterPoint().x, m_step[i].r.top);
			}
		}
	}
}

//20260728 by claude. index 스텝에서 "나가는" 연결선의 색.
//thumb 색과 규칙이 하나 다르다 — 진행 중(index == m_pos)인 스텝의 나가는 선은 아직 지나지 않은 구간이므로 비활성색이다.
//(thumb 색을 그대로 쓰면 현재 항목보다 한 칸 앞선 선까지 진행색으로 그려진다.)
Gdiplus::Color CSCStepCtrl::get_line_color(int index)
{
	if (index < 0 || index >= (int)m_step.size())
		return m_cr_thumb_inactive;

	//개별 지정(set_thumb_color)이 있으면 그 색 — 실패한 스텝에서 나가는 선은 실패색으로 이어진다.
	if (m_step[index].cr_thumb.GetValue() != Gdiplus::Color::Transparent)
		return m_step[index].cr_thumb;

	return (index < m_pos) ? m_cr_thumb_active : m_cr_thumb_inactive;
}

//20260729 by claude. cr 보다 offset 만큼 어두운 색. 이미 어두운 색은 검정에 묻히므로 방향을 뒤집는다.
//분기 기준을 get_weak_color 의 128 이 아니라 96 으로 낮춘 이유 — anysupport accent(#309AC0)의 luma 가
//127 이라 128 기준이면 "더 밝게" 로 넘어가 의도(어둡게)와 반대가 된다.
Gdiplus::Color CSCStepCtrl::get_deep_color(Gdiplus::Color cr, int offset)
{
	return get_color(cr, (get_luminance(cr) >= 96) ? -offset : offset);
}

//완료 스텝 채움색 위에 그릴 체크 표시 색.
//20260729 by claude. 고정 offset(-64) 은 채움색에 따라 실제 대비가 들쭉날쭉하다 — helpu accent(#20B2AE)
//기준으로 2.2:1 밖에 안 나와 18px 원 안의 1.8px 획이 묻혔다. 그래서 offset 이 아니라 목표 대비비를 정하고
//거기 도달할 때까지 단계적으로 벌린다. 밝기 조절은 hue 가 틀어지지 않는 방식으로 —
//어둡게는 곱셈(검정 쪽은 채널 포화가 없음), 밝게는 HSL 의 L 만 흰색 쪽으로 이동.
Gdiplus::Color CSCStepCtrl::get_check_color(Gdiplus::Color cr_thumb)
{
	if (m_check_color_user_set)
		return m_cr_check;

	const bool darken = (get_luminance(cr_thumb) >= 96);
	Gdiplus::Color cr = cr_thumb;

	for (int i = 1; i <= 20; i++)
	{
		cr = darken ? get_ratio_color(cr_thumb, 1.0f - 0.05f * i)
					: get_lightened_color(cr_thumb, 0.05 * i);

		if (get_wcag_contrast(cr_thumb, cr) >= m_check_contrast)
			break;
	}

	return cr;
}

//테마 accent 한 색에서 전체 배색을 파생시킨다. 사용하는 색은 accent / accent_deep / 테마 회색 / 테마 본문색 4 개뿐이다.
//  대기    : 테마 회색 테두리만            (아직 지나지 않음)
//  진행 중 : accent_deep 링 + 안쪽 점      (가장 진해 시선이 여기 머문다)
//  완료    : accent 채움 + accent_deep 체크 (체크가 진행 중 색과 같은 톤이라 단계가 이어져 보인다)
void CSCStepCtrl::set_color_theme(const CSCColorTheme& theme, bool invalidate)
{
	//accent 는 테마가 명시한 버튼색을 우선한다 — 같은 화면의 버튼과 같은 색이어야 배색이 흩어지지 않는다.
	//미지정(alpha 0)인 테마는 타이틀바 배경색이 사실상의 accent 다.
	Gdiplus::Color accent = (theme.cr_button_back.GetA() != 0) ? theme.cr_button_back : theme.cr_title_back_active;
	Gdiplus::Color accent_deep = get_deep_color(accent);

	m_cr_back = theme.cr_back;

	m_cr_thumb_active = accent;
	m_cr_thumb_current = accent_deep;
	m_cr_thumb_inactive = theme.cr_border_inactive;
	m_cr_thumb_outline = accent;

	m_cr_line_active = accent;
	m_cr_line_inactive = theme.cr_border_inactive;

	m_cr_text_active = theme.cr_text;
	m_cr_text_current = accent_deep;
	m_cr_text_inactive = theme.cr_text_dim;

	if (invalidate && GetSafeHwnd())
		Invalidate();
}

Gdiplus::Color CSCStepCtrl::get_thumb_color(int index)
{
	if (index < 0 || index >= (int)m_step.size())
		return m_cr_thumb_inactive;

	//개별 지정(set_thumb_color)이 있으면 진행 상태보다 우선한다.
	if (m_step[index].cr_thumb.GetValue() != Gdiplus::Color::Transparent)
		return m_step[index].cr_thumb;

	if (index > m_pos)
		return m_cr_thumb_inactive;

	if (index == m_pos)
		return m_cr_thumb_current;

	return m_cr_thumb_active;
}

void CSCStepCtrl::reconstruct_font()
{
	m_font.DeleteObject();
	BOOL bCreated = m_font.CreateFontIndirect(&m_lf);

	ASSERT(bCreated);
}

void CSCStepCtrl::set_thumb_style(int index, int style)
{
	if (index < 0)
	{
		for (auto& step : m_step)
		{
			step.thumb_style = style;
		}
	}
	else
	{
		m_step[index].thumb_style = style;
	}

	Invalidate();
}

//각 스텝에 텍스트 지정
void CSCStepCtrl::set_text(int index, CString text, Gdiplus::Color cr)
{
	if (index >= (int)m_step.size())
		return;

	m_step[index].text = text;

	if (cr.GetValue() != Gdiplus::Color::Transparent)
		m_step[index].cr_text = cr;
}

//thumb와 text의 색상을 모두 변경한다.
void CSCStepCtrl::set_step_color(int index, Gdiplus::Color cr_active, Gdiplus::Color cr_current)
{
	set_thumb_color(index, cr_active, cr_current);
	set_text_color(index, cr_active, cr_current);
}

void CSCStepCtrl::set_thumb_color(int index, Gdiplus::Color cr_active, Gdiplus::Color cr_current)
{
	if (index >= (int)m_step.size())
		return;

	if (index < 0)
	{
		m_cr_thumb_active = cr_active;

		if (cr_current.GetValue() != Gdiplus::Color::Transparent)
			m_cr_thumb_current = cr_current;

		for (int i = 0; i < m_step.size(); i++)
		{
			m_step[i].cr_thumb = cr_active;
		}
	}
	else
	{
		m_step[index].cr_thumb = cr_active;
	}
}

void CSCStepCtrl::set_text_color(int index, Gdiplus::Color cr_active, Gdiplus::Color cr_current)
{
	if (index >= (int)m_step.size())
		return;

	if (index < 0)
	{
		m_cr_text_active = cr_active;

		//20260729 by claude. m_cr_thumb_current 에 넣던 복붙 오류 수정 — 텍스트 setter 이므로 텍스트 색이어야 한다.
		if (cr_current.GetValue() != Gdiplus::Color::Transparent)
			m_cr_text_current = cr_current;

		for (int i = 0; i < m_step.size(); i++)
		{
			m_step[i].cr_text = cr_active;
		}
	}
	else
	{
		m_step[index].cr_text = cr_active;
	}
}

//특정 step의 thumb와 text의 색상을 리셋시키고 기본색을 사용하게 한다.
//index == -1이면 모든 스텝 리셋
void  CSCStepCtrl::reset_step_color(int index)
{
	if (index >= 0 && index < (int)m_step.size())
	{
		m_step[index].cr_text = Gdiplus::Color::Transparent;
		m_step[index].cr_thumb = Gdiplus::Color::Transparent;
	}
	else
	{
		for (int i = 0; i < (int)m_step.size(); i++)
		{
			m_step[i].cr_text = Gdiplus::Color::Transparent;
			m_step[i].cr_thumb = Gdiplus::Color::Transparent;
		}
	}

	Invalidate();
}

//각 스텝의 thumb 크기
void CSCStepCtrl::set_thumb_size(int size)
{
	m_thumb_size = size;

	for (auto& step : m_step)
		step.r.SetRectEmpty();

	Invalidate();
}

void CSCStepCtrl::OnSize(UINT nType, int cx, int cy)
{
	CStatic::OnSize(nType, cx, cy);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	if (!m_hWnd)
		return;

	for (auto& step : m_step)
		step.r.SetRectEmpty();

	Invalidate();
}

void CSCStepCtrl::PreSubclassWindow()
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	CWnd* pWnd = GetParent();
	CFont* font = NULL;

	if (pWnd)
		font = pWnd->GetFont();

	if (font == NULL)
		GetObject(GetStockObject(SYSTEM_FONT), sizeof(m_lf), &m_lf);
	else
		font->GetObject(sizeof(m_lf), &m_lf);

	reconstruct_font();

	CStatic::PreSubclassWindow();
}

BOOL CSCStepCtrl::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

	return CStatic::PreTranslateMessage(pMsg);
}
