#if !defined(AFX_CONTROLSPLITTER_H__AEB6C69A_7B1E_4CBD_9391_76B6E8B7900D__INCLUDED_)
#define AFX_CONTROLSPLITTER_H__AEB6C69A_7B1E_4CBD_9391_76B6E8B7900D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#define SPF_TOP             0x0010                  /* distance of the control to the top of the window will be constant */
#define SPF_BOTTOM          0x0020                  /* distance of the control to the bottom of the window will be constant */
#define SPF_LEFT            0x0040                  /* distance of the control to the left of the window will be constant */
#define SPF_RIGHT           0x0080                  /* distance of the control to the right of the window will be constant */

#endif // _MSC_VER > 1000
// ControlSplitter.h : 긶긞?? 긲?귽깑
//
#include <afxwin.h>
#include <vector>
/////////////////////////////////////////////////////////////////////////////
// CControlSplitter 긂귻깛긤긂

//#define MSG_CONTROL_SPLITTER_MOVED	WM_APP + 8653
//#define MSG_CONTROL_SPLITTER_MOVED_MSG		_T("MSG_CONTROL_SPLITTER_MOVED_MSG	")
static UINT MSG_CONTROL_SPLITTER_MOVED   = ::RegisterWindowMessage(_T("MSG_CONTROL_SPLITTER_MOVED_MSG"));

class CControlSplitter : public CButton
{	
// 긓깛긚긣깋긏긘깈깛
public:
	enum SPLITTER_TYPE
	{
		CS_VERT = 1,
		CS_HORZ = 2,
		CS_NONE = 0
	};
	CControlSplitter();

// 귺긣깏긮깄?긣
public:

// 긆긻깒?긘깈깛
public:

// 긆?긫?깋귽긤
	// ClassWizard 궼돹몒듫릶궻긆?긫?깋귽긤귩맯맟궢귏궥갃
	//{{AFX_VIRTUAL(CControlSplitter)
	//}}AFX_VIRTUAL

// 귽깛긵깏긽깛긡?긘깈깛
public:
	//CS_VERT이고 AddToBottomOrRightCtrls()로 추가할 때 SPF_LEFT만 주면 그 컨트롤은 width를 유지한 채 splitter에 붙어 이동한다.
	void AddToBottomOrRightCtrls(UINT nCtrlId, WORD nFlags = SPF_TOP|SPF_LEFT|SPF_RIGHT|SPF_BOTTOM);

	void AddToTopOrLeftCtrls(UINT nCtrlId, WORD nFlags = SPF_TOP|SPF_LEFT|SPF_BOTTOM|SPF_RIGHT);

	//어떤 컨트롤은 스플리터와 함께 움직여야 한다. 양쪽 모두 줘야한다.
	void AddToBoth(UINT nCtrlId, WORD nFlags = SPF_TOP | SPF_LEFT | SPF_BOTTOM | SPF_RIGHT);

	void SetType(UINT nType);
	virtual ~CControlSplitter();

	void SetBkColor(COLORREF crBack) { m_crBack = crBack; Invalidate(); }

	// 맯맟궠귢궫긽긞긜?긙 ?긞긵듫릶
protected:
	//{{AFX_MSG(CControlSplitter)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
private:
	unsigned int m_nType;
	std::vector<DWORD> m_vtTopLeftControls;
	std::vector<DWORD> m_vtBottomRightControls;
	CWnd * m_pOldDragCapture;
	CPoint m_ptStartDrag,m_ptStartPos;  
    bool m_bDragging;
	HCURSOR m_hCursor;
	CRect m_rectMax;

	COLORREF	m_crBack;

public:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ 궼멟뛱궻뮳멟궸믁돿궻먬뙻귩?볺궢귏궥갃

#endif // !defined(AFX_CONTROLSPLITTER_H__AEB6C69A_7B1E_4CBD_9391_76B6E8B7900D__INCLUDED_)
