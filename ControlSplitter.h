#if !defined(AFX_CONTROLSPLITTER_H__AEB6C69A_7B1E_4CBD_9391_76B6E8B7900D__INCLUDED_)
#define AFX_CONTROLSPLITTER_H__AEB6C69A_7B1E_4CBD_9391_76B6E8B7900D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#define SPF_TOP             0x0010                  /* distance of the control to the top of the window will be constant */
#define SPF_BOTTOM          0x0020                  /* distance of the control to the bottom of the window will be constant */
#define SPF_LEFT            0x0040                  /* distance of the control to the left of the window will be constant */
#define SPF_RIGHT           0x0080                  /* distance of the control to the right of the window will be constant */

#endif // _MSC_VER > 1000
// ControlSplitter.h : �w�b?? �t?�C��
//
#include <afxwin.h>
#include <vector>
#include <GdiPlus.h>
/////////////////////////////////////////////////////////////////////////////
// CControlSplitter �E�B���h�E

static UINT Message_CControlSplitter   = ::RegisterWindowMessage(_T("Message_CControlSplitter_String"));

/*
- �������� �ڵ忡���� resource id�� vector�� �����Ͽ� ó�������� �ּ� ũ����� ������ �ʿ伺�� �־�
  CControlItem Ÿ������ �����ؼ� ����ϵ��� ������.

[������ ����]
- ����� �ּ� ũ�⸸ ���� ���������� �ּ� ��ġ� ������ �� �־�� �Ѵ�. �׷��� ������ splitter�� ���� � ��Ʈ���� ������ ������ �ȴ�.
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
// �R���X�g���N�V����
public:
	enum SPLITTER_TYPE
	{
		CS_VERT = 1,
		CS_HORZ = 2,
		CS_NONE = 0
	};
	CControlSplitter();

// �A�g���r��?�g
public:

// �I�y��?�V����
public:

// �I?�o?���C�h
	// ClassWizard �͉��z�֐��̃I?�o?���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CControlSplitter)
	//}}AFX_VIRTUAL

// �C���v�������e?�V����
public:
	void set_type(UINT nType, bool draw_dots = true, Gdiplus::Color cr_dots = Gdiplus::Color::DimGray);
	virtual ~CControlSplitter();

	//CS_VERT�̰� AddToBottomOrRightCtrls()�� �߰��� �� SPF_LEFT�� �ָ� �� ��Ʈ���� width�� ������ ä splitter�� �پ� �̵��Ѵ�.
	//right�� ������ ä left�� splitter�� ���� �̵��Ϸ��� "SPF_LEFT | SPF_RIGHT"�� �ش�.
	//width�� ������ ä �̵���Ű���� SPF_LEFT�� �ش�.
	void AddToBottomOrRightCtrls(CControlItem ctrl);
	void AddToBottomOrRightCtrls(UINT id, int min_cx = 0, int min_cy = 0, UINT flag = SPF_TOP | SPF_LEFT | SPF_RIGHT | SPF_BOTTOM);

	void AddToTopOrLeftCtrls(CControlItem ctrl);
	void AddToTopOrLeftCtrls(UINT id, int min_cx = 0, int min_cy = 0, UINT flag = SPF_TOP | SPF_LEFT | SPF_RIGHT | SPF_BOTTOM);

	//� ��Ʈ���� ���ø��Ϳ� �Բ� �������� �Ѵ�. ���� ��� ����Ѵ�.
	void AddToBoth(CControlItem ctr);
	void AddToBoth(UINT id, int min_cx = 0, int min_cy = 0, UINT flag = SPF_TOP | SPF_LEFT | SPF_RIGHT | SPF_BOTTOM);


	//��Ʈ���� hide��Ű�� ��� �� �� �����Ƿ� show ��Ű�� �������� ĥ����� UI���� ���� �� �ִ�.
	void set_back_color(Gdiplus::Color cr) { m_cr_back = cr; Invalidate(); }

	// �������ꂽ���b�Z?�W ?�b�v�֐�
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

	//�������� �ҽ����� �����ϴ� �����μ� �̵���ų �� �ִ� ������ ũ���� �ǹ̰� �ƴ� ������ �� �ִ� ������ �ǹ��ϸ� �� ���� parent�� resize�ɶ����� ����ȴ�.
	CRect		m_rectMax;

	//��Ʈ���� hide��Ű�� ��� �� �� �����Ƿ� show ��Ű�� �������� ĥ����� UI���� ���� �� �ִ�.
	Gdiplus::Color	m_cr_back;

	//���ø����� dot ǥ��
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
// Microsoft Visual C++ �͑O�s�̒��O�ɒǉ��̐錾��?�����܂��B

#endif // !defined(AFX_CONTROLSPLITTER_H__AEB6C69A_7B1E_4CBD_9391_76B6E8B7900D__INCLUDED_)
