#pragma once

#include <afxwin.h>
#include <deque>
#include <gdiplus.h>

class CSCRect : public Gdiplus::RectF
{
	CSCRect(Gdiplus::RectF r, CString label = _T(""))
		: Gdiplus::RectF(r)
	{
	}

	CString	m_label;
};

class CSCGroupRect
{
public:
	CSCGroupRect() {}
	CSCGroupRect(float left, float top, float right, float bottom);

	~CSCGroupRect() {}

	void	add(float left, float top, float right, float bottom);
	void	addwh(float left, float top, float width, float height);

	void	add(CRect r);
	void	add(Gdiplus::RectF r);
	//void	add(CSCGroupRect r);

	void	clear()
	{
		m_r.clear();
		m_bound = Gdiplus::RectF();
	}

	void	recalc_bound()
	{
		m_bound = Gdiplus::RectF();

		if (m_r.size() == 0)
			return;

		for (size_t i = 0; i < m_r.size(); i++)
		{
			Gdiplus::RectF::Union(m_bound, m_bound, m_r[i]);
		}
	}

	std::deque<Gdiplus::RectF>	m_r;

protected:
	Gdiplus::RectF m_bound;
};