// CircularProgressCtrl.cpp: 구현 파일
//

#include "CircularProgressCtrl.h"
#include "../../Common/MemoryDC.h"
#include <gdiplus.h>

// CCircularProgressCtrl

IMPLEMENT_DYNAMIC(CCircularProgressCtrl, CStatic)

CCircularProgressCtrl::CCircularProgressCtrl()
{

}

CCircularProgressCtrl::~CCircularProgressCtrl()
{
}


BEGIN_MESSAGE_MAP(CCircularProgressCtrl, CStatic)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_TIMER()
END_MESSAGE_MAP()



// CCircularProgressCtrl 메시지 처리기

void CCircularProgressCtrl::OnPaint()
{
	CPaintDC dc1(this); // device context for painting
					   // TODO: 여기에 메시지 처리기 코드를 추가합니다.
					   // 그리기 메시지에 대해서는 CProgressCtrl::OnPaint()을(를) 호출하지 마십시오.

	CRect rc;
	GetClientRect(rc);

	CMemoryDC dc(&dc1, &rc);
	Gdiplus::Graphics g(dc.GetSafeHdc());
	
	g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
	g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

	CRect r = rc;

	//정사각형이 아니면 w, h 중에 작은 값으로 정사각형을 만들고 중앙에 표시되야 한다.
	if (r.Width() < r.Height())
	{
		int gap_half = (r.Height() - r.Width()) / 2.0;
		r.top = gap_half;
		r.bottom = r.top + r.Width();
	}
	else if (r.Width() > r.Height())
	{
		int gap_half = (r.Width() - r.Height()) / 2.0;
		r.left = gap_half;
		r.right = r.left + r.Height();
	}

	//정사각형으로 만든 후 m_size_ratio를 적용시킨다.
	//축소시킬 크기를 계산하고 그 1/2을 DeflateRect()해준다.
	int deflate = ((float)r.Width() * (1.0f - m_size_ratio)) / 2.0f;
	r.DeflateRect(deflate, deflate);

	//thickness/2 만큼 줄여줘야 영역에 정확히 내접한다.(PenAlignmentCenter 센터라인을 따라 그린다)
	r.DeflateRect(m_line_width / 2.0f, m_line_width / 2.0f);

	double pos = ((double)m_pos / (double)(m_upper - m_lower));

	Gdiplus::Color	cr_gray;
	Gdiplus::Color	cr_pen;

	cr_gray.SetFromCOLORREF(RGB(232, 232, 232));
	cr_pen.SetFromCOLORREF(RGB(24, 156, 247));

	Gdiplus::Pen	pen_gray(cr_gray, m_line_width);
	Gdiplus::Pen	pen(cr_pen, m_line_width);
	
	pen.SetLineCap(Gdiplus::LineCapRound, Gdiplus::LineCapRound, Gdiplus::DashCapRound);
	pen.SetAlignment(Gdiplus::PenAlignmentCenter);

	g.DrawArc(&pen_gray, Gdiplus::Rect(r.left, r.top, r.Width(), r.Height()), 0, 360.0);
	g.DrawArc(&pen, Gdiplus::Rect(r.left, r.top, r.Width(), r.Height()), 0, (Gdiplus::REAL)pos * 360.0);

	Gdiplus::FontFamily fontFamily((WCHAR*)(const WCHAR*)CStringW(_T("맑은 고딕")));
	Gdiplus::Font font(&fontFamily, 20.0f, Gdiplus::FontStyleBold);
	Gdiplus::SolidBrush brush((m_pos == m_lower) ? cr_gray : cr_pen);

	Gdiplus::StringFormat sf;
	sf.SetAlignment(Gdiplus::StringAlignmentCenter);
	sf.SetLineAlignment(Gdiplus::StringAlignmentCenter);

	CString str;
	str.Format(_T("%.0f%%"), pos * 100.0);
	g.DrawString(str, -1, &font, Gdiplus::RectF(r.left, r.top, r.Width(), r.Height()), &sf, &brush);

	//for test 크기 확인용
	//dc.DrawFocusRect(rc);
}


BOOL CCircularProgressCtrl::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	return FALSE;
	return CStatic::OnEraseBkgnd(pDC);
}

void CCircularProgressCtrl::SetPos(int64_t pos, bool ani)
{
	if (ani)
	{
		m_ani_pos_increase = (pos > m_pos);
		m_ani_pos = m_pos;
		m_ani_pos_final = pos;
		m_pos = pos;
		SetTimer(timer_ani_setpos, 100, NULL);
	}
	else
	{
		m_pos = pos;
		Invalidate();
	}
}


void CCircularProgressCtrl::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (nIDEvent == timer_ani_setpos)
	{
		Invalidate();

		if (m_ani_pos_increase)
		{
			m_pos++;
			if (m_pos >= m_ani_pos_final)
			{
				KillTimer(timer_ani_setpos);
				return;
			}
		}
		else
		{
			m_pos--;
			if (m_pos <= m_ani_pos_final)
			{
				KillTimer(timer_ani_setpos);
				return;
			}
		}
	}

	CStatic::OnTimer(nIDEvent);
}

//rc 영역의 ratio 비율만큼 작게 그린다. 1.0이면 100%로 꽉차게 그린다. 최소값은 10%
//크기에 따라 글자크기도 자동 조정되어야 한다.
void CCircularProgressCtrl::set_size_ratio(float ratio)
{
	m_size_ratio = ratio;
	Invalidate();
}
