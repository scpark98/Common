#if !defined(AFX_CONTROLSPLITTER_H__AEB6C69A_7B1E_4CBD_9391_76B6E8B7900D__INCLUDED_)
#define AFX_CONTROLSPLITTER_H__AEB6C69A_7B1E_4CBD_9391_76B6E8B7900D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#define SPF_TOP             0x0010                  /* distance of the control to the top of the window will be constant */
#define SPF_BOTTOM          0x0020                  /* distance of the control to the bottom of the window will be constant */
#define SPF_LEFT            0x0040                  /* distance of the control to the left of the window will be constant */
#define SPF_RIGHT           0x0080                  /* distance of the control to the right of the window will be constant */

#endif // _MSC_VER > 1000
// ControlSplitter.h : ヘッ?? フ?イル
//
#include <afxwin.h>
#include <vector>
#include <GdiPlus.h>
/////////////////////////////////////////////////////////////////////////////
// CControlSplitter ウィンドウ

static UINT Message_CControlSplitter   = ::RegisterWindowMessage(_T("Message_CControlSplitter_String"));

/*
- ｿﾀｸｮﾁ�ｳﾎ ﾄﾚｵ蠢｡ｼｭｴﾂ resource idｸｦ vectorｿ｡ ﾀ�ﾀ衂ﾏｿｩ ﾃｳｸｮﾇﾟﾀｸｳｪ ﾃﾖｼﾒ ﾅｩｱ箏�ﾀｻ ﾁ�ﾁ､ﾇﾒ ﾇﾊｿ莨ｺﾀﾌ ﾀﾖｾ�
  CControlItem ﾅｸﾀﾔﾀｸｷﾎ ﾀ�ﾀ衂ﾘｼｭ ｻ鄙�ﾇﾏｵｵｷﾏ ｼ�ﾁ､ﾇﾔ.

[ｼ�ﾁ､ﾇﾒ ｳｻｿ�]
- ﾇ�ﾀ邏ﾂ ﾃﾖｼﾒ ﾅｩｱ篋ｸ ｼｳﾁ､ ｰ｡ｴﾉﾇﾏﾁ�ｸｸ ﾃﾖｼﾒ ﾀｧﾄ｡ｵ�ｵｵ ﾁ�ﾁ､ﾇﾒ ｼ� ﾀﾖｾ�ｾﾟ ﾇﾑｴﾙ. ｱﾗｷｸﾁ� ｾﾊﾀｸｸ� splitterｿ｡ ﾀﾇﾇﾘ ｾ�ｶｲ ﾄﾁﾆｮｷﾑﾀｺ ｹﾛﾀｸｷﾎ ｳｪｰ｡ｰﾔ ｵﾈｴﾙ.
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
// コンストラクション
public:
	enum SPLITTER_TYPE
	{
		CS_VERT = 1,
		CS_HORZ = 2,
		CS_NONE = 0
	};
	CControlSplitter();

// アトリビュ?ト
public:

// オペレ?ション
public:

// オ?バ?ライド
	// ClassWizard は仮想関数のオ?バ?ライドを生成します。
	//{{AFX_VIRTUAL(CControlSplitter)
	//}}AFX_VIRTUAL

// インプリメンテ?ション
public:
	void set_type(UINT nType, bool draw_dots = true, Gdiplus::Color cr_dots = Gdiplus::Color::DimGray);
	virtual ~CControlSplitter();

	//CS_VERTﾀﾌｰ� AddToBottomOrRightCtrls()ｷﾎ ﾃﾟｰ｡ﾇﾒ ｶｧ SPF_LEFTｸｸ ﾁﾖｸ� ｱﾗ ﾄﾁﾆｮｷﾑﾀｺ widthｸｦ ﾀｯﾁ�ﾇﾑ ﾃ､ splitterｿ｡ ｺﾙｾ� ﾀﾌｵｿﾇﾑｴﾙ.
	//right ﾀｧﾄ｡ｸｦ ｰ�ﾁ､ﾇﾑ ﾃ､ leftｸｸ splitterｿ｡ ｵ�ｶ� ﾀﾌｵｿﾇﾏｷﾁｸ� "SPF_LEFT | SPF_RIGHT"ｸｦ ﾁﾘｴﾙ.
	void AddToBottomOrRightCtrls(CControlItem ctrl);
	void AddToBottomOrRightCtrls(UINT id, int min_cx = 0, int min_cy = 0, UINT flag = SPF_TOP | SPF_LEFT | SPF_RIGHT | SPF_BOTTOM);

	void AddToTopOrLeftCtrls(CControlItem ctrl);
	void AddToTopOrLeftCtrls(UINT id, int min_cx = 0, int min_cy = 0, UINT flag = SPF_TOP | SPF_LEFT | SPF_RIGHT | SPF_BOTTOM);

	//ｾ�ｶｲ ﾄﾁﾆｮｷﾑﾀｺ ｽｺﾇﾃｸｮﾅﾍｿﾍ ﾇﾔｲｲ ｿ�ﾁ�ｿｩｾﾟ ﾇﾑｴﾙ. ｾ酊ﾊ ｸ�ｵﾎ ﾁ狎ﾟﾇﾑｴﾙ.
	void AddToBoth(CControlItem ctr);
	void AddToBoth(UINT id, int min_cx = 0, int min_cy = 0, UINT flag = SPF_TOP | SPF_LEFT | SPF_RIGHT | SPF_BOTTOM);


	//ﾄﾁﾆｮｷﾑﾀｻ hideｽﾃﾅｰｸ� ﾁｦｾ�ｸｦ ﾇﾒ ｼ� ｾ�ﾀｸｹﾇｷﾎ show ｽﾃﾅｰｵﾇ ｹ隹貊�ﾀｸｷﾎ ﾄ･ﾇﾘﾁ狎ﾟ UIｿ｡ｼｭ ｰｨﾃ� ｼ� ﾀﾖｴﾙ.
	void set_back_color(Gdiplus::Color cr) { m_cr_back = cr; Invalidate(); }

	// 生成されたメッセ?ジ ?ップ関数
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

	//ｿﾀｸｮﾁ�ｳﾎ ｼﾒｽｺｺﾎﾅﾍ ﾁｸﾀ酩ﾏｴ� ｺｯｼ�ｷﾎｼｭ ﾀﾌｵｿｽﾃﾅｳ ｼ� ﾀﾖｴﾂ ｿｵｿｪﾀﾇ ﾅｩｱ簑ﾇ ﾀﾇｹﾌｰ｡ ｾﾆｴﾑ ｿ�ﾁ�ﾀﾏ ｼ� ﾀﾖｴﾂ ｹ�ﾀｧｸｦ ﾀﾇｹﾌﾇﾏｸ� ﾀﾌ ｰｪﾀｺ parentｰ｡ resizeｵﾉｶｧｿ｡ｵｵ ｺｯｰ豬ﾈｴﾙ.
	CRect		m_rectMax;

	//ﾄﾁﾆｮｷﾑﾀｻ hideｽﾃﾅｰｸ� ﾁｦｾ�ｸｦ ﾇﾒ ｼ� ｾ�ﾀｸｹﾇｷﾎ show ｽﾃﾅｰｵﾇ ｹ隹貊�ﾀｸｷﾎ ﾄ･ﾇﾘﾁ狎ﾟ UIｿ｡ｼｭ ｰｨﾃ� ｼ� ﾀﾖｴﾙ.
	Gdiplus::Color	m_cr_back;

	//ｽｺﾇﾃｸｮﾅﾍﾀﾇ dot ﾇ･ｽﾃ
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
// Microsoft Visual C++ は前行の直前に追加の宣言を?入します。

#endif // !defined(AFX_CONTROLSPLITTER_H__AEB6C69A_7B1E_4CBD_9391_76B6E8B7900D__INCLUDED_)
