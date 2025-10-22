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
#include <GdiPlus.h>
/////////////////////////////////////////////////////////////////////////////
// CControlSplitter 긂귻깛긤긂

static UINT Message_CControlSplitter   = ::RegisterWindowMessage(_T("Message_CControlSplitter_String"));

/*
- 오리지널 코드에서는 resource id를 vector에 저장하여 처리했으나 최소 크기등을 지정할 필요성이 있어
  CControlItem 타입으로 저장해서 사용하도록 수정함.

[수정할 내용]
- 현재는 최소 크기만 설정 가능하지만 최소 위치등도 지정할 수 있어야 한다. 그렇지 않으면 splitter에 의해 어떤 컨트롤은 밖으로 나가게 된다.
*/

class CControlItem
{
public :
	CControlItem(CWnd* _pWnd, int _min_cx = 0, int _min_cy = 0, UINT _flag = SPF_TOP | SPF_LEFT | SPF_RIGHT | SPF_BOTTOM)
	{
		pWnd = _pWnd;
		min_cx = _min_cx;
		min_cy = _min_cy;
		flag = _flag;
	}

	CWnd*	pWnd = NULL;
	int		min_cx = 0;
	int		min_cy = 0;
	UINT	flag = SPF_TOP | SPF_LEFT | SPF_RIGHT | SPF_BOTTOM;
};

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
	void set_type(UINT nType, bool draw_dots = true, Gdiplus::Color cr_dots = Gdiplus::Color::DimGray);
	virtual ~CControlSplitter();

	//CS_VERT이고 AddToBottomOrRightCtrls()로 추가할 때 SPF_LEFT만 주면 그 컨트롤은 width를 유지한 채 splitter에 붙어 이동한다.
	//right를 고정한 채 left만 splitter에 따라 이동하려면 "SPF_LEFT | SPF_RIGHT"를 준다.
	//width를 고정한 채 이동시키려면 SPF_LEFT만 준다.
	void AddToBottomOrRightCtrls(CControlItem ctrl);
	void AddToBottomOrRightCtrls(UINT id, int min_cx = 0, int min_cy = 0, UINT flag = SPF_TOP | SPF_LEFT | SPF_RIGHT | SPF_BOTTOM);

	void AddToTopOrLeftCtrls(CControlItem ctrl);
	void AddToTopOrLeftCtrls(UINT id, int min_cx = 0, int min_cy = 0, UINT flag = SPF_TOP | SPF_LEFT | SPF_RIGHT | SPF_BOTTOM);

	//어떤 컨트롤은 스플리터와 함께 움직여야 한다. 양쪽 모두 줘야한다.
	void AddToBoth(CControlItem ctr);
	void AddToBoth(UINT id, int min_cx = 0, int min_cy = 0, UINT flag = SPF_TOP | SPF_LEFT | SPF_RIGHT | SPF_BOTTOM);


	//컨트롤을 hide시키면 제어를 할 수 없으므로 show 시키되 배경색으로 칠해줘야 UI에서 감출 수 있다.
	void set_back_color(Gdiplus::Color cr) { m_cr_back = cr; Invalidate(); }

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
	unsigned int m_type;
	std::vector<CControlItem> m_vtTopLeftControls;
	std::vector<CControlItem> m_vtBottomRightControls;
	CWnd * m_pOldDragCapture;
	CPoint m_ptStartDrag,m_ptStartPos;  
    bool m_bDragging;
	HCURSOR m_hCursor;

	//오리지널 소스부터 존재하던 변수로서 이동시킬 수 있는 영역의 크기의 의미가 아닌 움직일 수 있는 범위를 의미하며 이 값은 parent가 resize될때에도 변경된다.
	CRect		m_rectMax;

	//컨트롤을 hide시키면 제어를 할 수 없으므로 show 시키되 배경색으로 칠해줘야 UI에서 감출 수 있다.
	Gdiplus::Color	m_cr_back;

	//스플리터의 dot 표시
	bool			m_draw_dots = false;
	Gdiplus::Color	m_cr_dots = Gdiplus::Color::DimGray;
	void			draw_dots(CDC* pDC, CRect const* rect, bool horz, int number, int size, Gdiplus::Color cr);

public:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ 궼멟뛱궻뮳멟궸믁돿궻먬뙻귩?볺궢귏궥갃

#endif // !defined(AFX_CONTROLSPLITTER_H__AEB6C69A_7B1E_4CBD_9391_76B6E8B7900D__INCLUDED_)
