#pragma once

#include <afxwin.h>
#include <deque>
#include <gdiplus.h>

class CSCUIElement
{
public:
	CSCUIElement() {}
	CSCUIElement(float left, float top, float right, float bottom, CString label = _T(""));
	CSCUIElement(Gdiplus::RectF r, CString label = _T(""));
	CSCUIElement(CString label);

	~CSCUIElement() {}

	void	set(float left, float top, float right, float bottom, CString label = _T(""));
	void	setwh(float left, float top, float width, float height, CString label = _T(""));

	void	set(CRect r, CString label = _T(""));
	void	set(Gdiplus::RectF r, CString label = _T(""));


	Gdiplus::RectF	get_rect() const { return m_r; }
	void			set_rect(Gdiplus::RectF r) { m_r = r; }

	CString			get_label() const { return m_label; }
	void			set_label(CString label) { m_label = label; }

protected:
	Gdiplus::RectF	m_r;
	CString			m_label;
};
