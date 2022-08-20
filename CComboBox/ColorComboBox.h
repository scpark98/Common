#pragma once


// CColorComboBox

class CColorComboBox : public CComboBox
{
	DECLARE_DYNAMIC(CColorComboBox)

public:
	CColorComboBox();
	virtual ~CColorComboBox();

	COLORREF		m_crText;
	COLORREF		m_crBack;
	COLORREF		m_crHighlightText;
	COLORREF		m_crHighlightTextBack;

	void			SetTextColor( COLORREF crText ) { m_crText = crText; RedrawWindow(); }
	void			SetBackColor( COLORREF crBack ) { m_crBack = crBack; RedrawWindow(); }
	void			SetHighlightTextColor( COLORREF cr ) { m_crHighlightText = cr; RedrawWindow(); }
	void			SetHighlightTextBackColor( COLORREF cr ) { m_crHighlightTextBack = cr; RedrawWindow(); }

protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual void DrawItem(LPDRAWITEMSTRUCT /*lpDrawItemStruct*/);
	virtual void MeasureItem(LPMEASUREITEMSTRUCT /*lpMeasureItemStruct*/);
	virtual int CompareItem(LPCOMPAREITEMSTRUCT /*lpCompareItemStruct*/);
	//afx_msg void OnNcPaint();
	//afx_msg void OnPaint();
	//afx_msg void OnEditUpdate();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
};


