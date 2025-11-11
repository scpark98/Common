#pragma once

#include <afxwin.h>

class CSCGroupRect : public CRect
{
public:
	void	add(int left, int top, int right, int bottom);
	void	addwh(int left, int top, int width, int height);

	void	add(CRect r);
	void	add(CSCGroupRect r);

	//CSCGroupRect	m_r;
};