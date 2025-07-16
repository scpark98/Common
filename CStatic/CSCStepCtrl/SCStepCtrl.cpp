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
	//draw_rectangle(g, rc, Gdiplus::Color::Red, m_cr_back);

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

		Gdiplus::Color cr_thumb = m_cr_thumb_active;
		Gdiplus::Color cr_text = m_cr_text_active;
		Gdiplus::Color cr_line = m_cr_line_active;

		if (i > m_pos)
		{
			cr_thumb = m_cr_thumb_inactive;
			cr_text = m_cr_text_inactive;
			cr_line = m_cr_line_inactive;
		}
		else if (i == m_pos)
		{
			cr_thumb = m_cr_thumb_current;
			cr_text = m_cr_text_current;
		}

		//thumb_style, thumb 색상, text의 색상을 별도로 지정한 위치라면 그 설정대로 그려준다.
		if (m_step[i].thumb_style != thumb_style_none)
			thumb_style = m_step[i].thumb_style;

		if (m_step[i].cr_text.GetValue() != Gdiplus::Color::Transparent)
			cr_text = m_step[i].cr_text;

		if (m_step[i].cr_thumb.GetValue() != Gdiplus::Color::Transparent)
			cr_thumb = m_step[i].cr_thumb;


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
			g.FillRectangle(&Gdiplus::SolidBrush(cr_thumb), CRect2GpRect(m_step[i].r));
		else
			g.FillEllipse(&Gdiplus::SolidBrush(cr_thumb), CRect2GpRect(m_step[i].r));

		//pos 미만은 파란색 원에 체크 표시를
		if (i < m_pos)
		{
			draw_line(&dc, m_step[i].r.CenterPoint().x - 3, m_step[i].r.CenterPoint().y - 0, m_step[i].r.CenterPoint().x - 1, m_step[i].r.CenterPoint().y + 2, Gdiplus::Color::White, 1.0f);
			draw_line(&dc, m_step[i].r.CenterPoint().x - 1, m_step[i].r.CenterPoint().y + 2, m_step[i].r.CenterPoint().x + 3, m_step[i].r.CenterPoint().y - 2, Gdiplus::Color::White, 1.0f);
		}
		else if (i == m_pos)
		{
			CRect rthumb_small = m_step[i].r;

			rthumb_small.DeflateRect(2, 2);
			if (thumb_style == thumb_style_rect)
				g.FillRectangle(&Gdiplus::SolidBrush(Gdiplus::Color::White), CRect2GpRect(rthumb_small));
			else
				g.FillEllipse(&Gdiplus::SolidBrush(Gdiplus::Color::White), CRect2GpRect(rthumb_small));

			rthumb_small.DeflateRect(2, 2);
			if (thumb_style == thumb_style_rect)
				//g.FillRectangle(&Gdiplus::SolidBrush(cr_thumb), CRect2GpRect(rthumb_small));
				draw_rectangle(g, rthumb_small, Gdiplus::Color::Transparent, cr_thumb);
			else if (thumb_style == thumb_style_diamond)
				;// draw_diamond(g, rthumb_small, Gdiplus::Color::Transparent, cr_thumb);
			else if (thumb_style == thumb_style_circle)
				g.FillEllipse(&Gdiplus::SolidBrush(cr_thumb), CRect2GpRect(rthumb_small));
				//draw_ellipse(&dc,)
		}
		else
		{
			CRect rthumb_small = m_step[i].r;

			rthumb_small.DeflateRect(1, 1);
			if (thumb_style == thumb_style_rect)
				g.FillRectangle(&Gdiplus::SolidBrush(Gdiplus::Color::White), CRect2GpRect(rthumb_small));
			else
				g.FillEllipse(&Gdiplus::SolidBrush(Gdiplus::Color::White), CRect2GpRect(rthumb_small));
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
			//draw_rectangle(g, rtext, Gdiplus::Color::Red);	//text 영역 확인용
#endif
			dc.SelectObject(pOldFont);
		}

		//각 스텝 사이의 라인을 그려준다.
		if (i > 0)
		{
			Gdiplus::Pen pen_line(cr_thumb, 1.7f);

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

		if (cr_current.GetValue() != Gdiplus::Color::Transparent)
			m_cr_thumb_current = cr_current;

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
