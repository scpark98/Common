////////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <SDKDDKVer.h>
#include <afxcontrolbars.h>
////////////////////////////////////////////////////////////////////////////////////
class CPropGridSlider;

class CPropSlider : public CSliderCtrl
{
public:
	CPropSlider(CPropGridSlider *parent_prop) : CSliderCtrl()
	{
		m_iPrevPos = -1; 
		m_pParentProp = parent_prop;
	}
	
protected:
	CPropGridSlider *m_pParentProp;
	int m_iPrevPos;
	void OnUpdateValue();

public:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnNMReleasedcapture(NMHDR *pNMHDR, LRESULT *pResult);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
};

class CPropGridSlider : public CMFCPropertyGridProperty
{
	// OnEdit -> CreateInPlaceEdit -> OnClickValue
	// OnPosSizeChanged -> OnDrawValue
public:

protected:
	CPropSlider *m_pSlider;
	int m_min, m_max;
	int m_step;
	int m_iEditCtrlWidth;

public:
	CPropGridSlider(CString name, int value, CString tip, int min, int max, int step, DWORD dwData, int edit_ctrl_width=-2 ) :
		CMFCPropertyGridProperty(name, (_variant_t)(value), tip, dwData) {
		m_min = min;
		m_max = max;
		m_step = step;
		m_pSlider = NULL;
		m_iEditCtrlWidth = edit_ctrl_width;
	}

	virtual ~CPropGridSlider()
	{
		if (m_pSlider)
			delete m_pSlider;
	}

	void set_range(int min, int max)
	{
		m_min = min;
		m_max = max;
		SetSliderPos();
	}

	void OnSliderPosChanged();
	void SetSliderPos();

//virtuals:
	virtual BOOL OnEndEdit();
	virtual void OnDrawValue(CDC* pDC, CRect rect);
	virtual CWnd* CreateInPlaceEdit(CRect rectEdit, BOOL& bDefaultFormat);
	virtual void SetValue(const COleVariant& varValue) {
		CMFCPropertyGridProperty::SetValue(varValue);
		SetSliderPos();
	}
};

////////////////////////////////////////////////////////////////////////////////////

