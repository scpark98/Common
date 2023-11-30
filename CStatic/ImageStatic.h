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

	//레지스트리에 저장된 recent file 정보가 있다면 로딩한다.
	bool		load();
	bool		load(CString sFile, bool show_error = false);
	bool		load(CString sType, UINT id, bool show_error = false);
	//png일 경우는 sType을 생략할 수 있다.
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

	//확대, 축소 배율을 사용하지 않고 창 크기에 맞춤
	bool		m_fit2ctrl = true;
	double		m_zoom = 1.0;
	//mode : 1(zoom in), -1(zoom out), 0(reset)
	CPoint		m_offset = CPoint(0, 0);
	int			m_offset_size = 8;

	//마우스 드래그
	bool		m_lbutton_down = false;
	CPoint		m_ptClicked = CPoint(0, 0);
	HCURSOR		m_hCursor;

	//roi 관련(선택영역)
	//roi를 screen기준으로만 저장하면 이미지 scroll, resize 등을 할때마다 항상 보정해줘야 하므로
	//roi가 설정된 순간에 image_roi를 계산해놓고 위치, 크기 변경시에는 image_roi를 screen_roi로 변경하여 표시한다.
	CRect		m_image_roi;	//영상의 실제 ROI
	CRect		m_screen_roi;	//디스플레이되고 있는 화면상의 ROI


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


