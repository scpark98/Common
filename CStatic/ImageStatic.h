#pragma once


#include "../GdiplusBitmap.h"

// CImageStatic
static const UINT Message_CImageStatic = ::RegisterWindowMessage(_T("MessageString_CImageStatic"));

class CImageStaticMessage
{
public:
	CImageStaticMessage(CWnd* _this, int _message, CWnd* _pTarget = NULL)
	{
		pThis = _this;
		message = _message;
		pTarget = _pTarget;
	}

	CWnd* pThis = NULL;
	CWnd* pTarget = NULL;
	int	message;
};

class CImageStatic : public CStatic
{
	DECLARE_DYNAMIC(CImageStatic)

public:
	CImageStatic();
	virtual ~CImageStatic();

	enum CImageStaticMsgs
	{
		message_loading_completed = 0,
	};

	CGdiplusBitmap	m_img;

	//������Ʈ���� ����� recent file ������ �ִٸ� �ε��Ѵ�.
	bool		load();
	bool		load(CString sFile, bool show_error = false);
	bool		load(CString sType, UINT id, bool show_error = false);
	//png�� ���� sType�� ������ �� �ִ�.
	bool		load(UINT id, bool show_error = false);

	CString		get_filename() { return m_filename; }

	bool		copy_to_clipbard();
	//bool		paste_from_clipboard();
	CRect		get_image_roi();

	void		zoom(int mode);
	void		fit2ctrl(bool fit);

protected:

	CString		m_filename;
	CRect		m_displayed;

	Gdiplus::InterpolationMode m_interplationMode = Gdiplus::InterpolationModeHighQualityBicubic;

	//Ȯ��, ��� ������ ������� �ʰ� â ũ�⿡ ����
	bool		m_fit2ctrl = true;
	double		m_zoom = 1.0;
	//mode : 1(zoom in), -1(zoom out), 0(reset)
	CPoint		m_offset = CPoint(0, 0);
	int			m_offset_size = 8;

	//���콺 �巡��
	bool		m_lbutton_down = false;
	CPoint		m_ptClicked = CPoint(0, 0);
	HCURSOR		m_hCursor;

	//roi ����(���ÿ���)
	//roi�� screen�������θ� �����ϸ� �̹��� scroll, resize ���� �Ҷ����� �׻� ��������� �ϹǷ�
	//roi�� ������ ������ image_roi�� ����س��� ��ġ, ũ�� ����ÿ��� image_roi�� screen_roi�� �����Ͽ� ǥ���Ѵ�.
	CRect		m_image_roi;	//������ ���� ROI
	CRect		m_screen_roi;	//���÷��̵ǰ� �ִ� ȭ����� ROI


protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnDropFiles(HDROP hDropInfo);
	virtual void PreSubclassWindow();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};


