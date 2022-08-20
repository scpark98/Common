#if !defined(AFX_GRAPHICOBJECT_H__644C37DB_0C57_44AA_B067_6D324D763A8F__INCLUDED_)
#define AFX_GRAPHICOBJECT_H__644C37DB_0C57_44AA_B067_6D324D763A8F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GraphicObject.h : header file
//



/////////////////////////////////////////////////////////////////////////////
// CGraphicObject command target

class CGraphicObject
{
public:
	CGraphicObject();
	virtual ~CGraphicObject();

public:
	virtual UINT GetWidth() = 0;
	virtual UINT GetHeight() = 0;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GRAPHICOBJECT_H__644C37DB_0C57_44AA_B067_6D324D763A8F__INCLUDED_)
