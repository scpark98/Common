#pragma once

/*
* https://github.com/scpark98/Test_ImageStatic.git

  단순히 이미지(파일 또는 리소스)를 CStatic에 표시하는 목적 뿐 만 아니라
  fit2ctrl(true)를 호출하여 이미지 표시 크기를 컨트롤에 맞춰 표시할 수도 있고
  이미지 크기를 1:1로 표시하고 pan, zoom 등의 기능도 지원한다.
  
  만약 현재 이미지 및 동일 경로의 다른 이미지 파일들까지 browse, thumb view, roi 등의 기능이 필요하다면(ex: ASee)
  이 클래스보다는 CSCImageDlg를 사용하는 것이 좋다.
*/

#include "../../SCGdiplusBitmap.h"

#define ZOOM_MIN_RATIO	0.2
#define ZOOM_MAX_RATIO	5.0

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

	CSCGdiplusBitmap	m_img;

	//레지스트리에 저장된 recent file 정보가 있다면 로딩한다.
	bool			load();

	//외부 파일 로딩
	bool			load(CString sFile);

	//리소스의 JPG or PNG 파일 로딩
	bool			load(CString sType, UINT id);

	//png일 경우는 sType을 생략할 수 있다.
	bool			load(UINT id);

	void			set_image(CSCGdiplusBitmap& img);

	CString			get_filename() { return m_filename; }

	bool			copy_to_clipbard();
	//bool		paste_from_clipboard();
	CRect			get_image_roi();

	void			zoom(int mode, float interval = 0.1f);
	double			get_zoom_min() { return ZOOM_MIN_RATIO; }
	double			get_zoom_max() { return ZOOM_MAX_RATIO; }
	double			get_zoom_ratio() { return m_zoom; }
	void			set_zoom_ratio(double zoom);

	void			fit2ctrl(bool fit);
	bool			get_fit2ctrl() { return m_fit2ctrl; }

	CRect			get_displayed_rect() { return m_displayed; }

	CPoint			get_offset() { return m_offset; }
	void			set_offset(int offset_x, int offset_y) {};
	int				get_offset_size() { return m_offset_size; }

	//이미지 부드럽게 보정
	void			set_smooth_interpolation(bool use = true);

	void			set_back_color(Gdiplus::Color cr_back);
protected:

	CString			m_filename;
	CRect			m_displayed;

	//투명 png일 경우 배경에 표시할 zigzag 패턴 브러시
	std::unique_ptr<Gdiplus::TextureBrush> m_br_zigzag;
	Gdiplus::Color	m_cr_zigzag_back = Gdiplus::Color::White;
	Gdiplus::Color	m_cr_zigzag_fore = Gdiplus::Color(200, 200, 200);

	Gdiplus::Color	m_cr_back = Gdiplus::Color::Transparent;

	Gdiplus::InterpolationMode m_interplationMode = Gdiplus::InterpolationModeHighQualityBicubic;

//확대, 축소 배율을 사용하지 않고 창 크기에 맞춤
	bool			m_fit2ctrl = false;

//확대 및 이동
	double			m_zoom = 1.0;
	//mode : 1(zoom in), -1(zoom out), 0(reset)
	CPoint			m_offset = CPoint(0, 0);
	int				m_offset_size = 8;

//마우스 드래그
	bool			m_lbutton_down = false;
	CPoint			m_ptClicked = CPoint(0, 0);
	HCURSOR			m_hCursor;

	//roi 관련(선택영역)
	//roi를 screen기준으로만 저장하면 이미지 scroll, resize 등을 할때마다 항상 보정해줘야 하므로
	//roi가 설정된 순간에 image_roi를 계산해서 저장해 놓고
	//위치, 크기 변경시에 image_roi를 screen_roi로 변경하여 표시한다.
	CRect			m_image_roi;	//영상의 실제 ROI
	CRect			m_screen_roi;	//디스플레이되고 있는 화면상의 ROI

	bool			m_show_pixel = true;
	CRect			m_r_pixel;
	Gdiplus::Color	m_cr_pixel = Gdiplus::Color::Black;

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


