#pragma once

/*
* https://github.com/scpark98/Test_ImageStatic.git

  �ܼ��� �̹���(���� �Ǵ� ���ҽ�)�� CStatic�� ǥ���ϴ� ���� �� �� �ƴ϶�
  fit2ctrl(true)�� ȣ���Ͽ� �̹��� ǥ�� ũ�⸦ ��Ʈ�ѿ� ���� ǥ���� ���� �ְ�
  �̹��� ũ�⸦ 1:1�� ǥ���ϰ� pan, zoom ���� ��ɵ� �����Ѵ�.
  
  ���� ���� �̹��� �� ���� ����� �ٸ� �̹��� ���ϵ���� browse, thumb view, roi ���� ����� �ʿ��ϴٸ�(ex: ASee)
  �� Ŭ�������ٴ� CSCImageDlg�� ����ϴ� ���� ����.
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

	//������Ʈ���� ����� recent file ������ �ִٸ� �ε��Ѵ�.
	bool			load();

	//�ܺ� ���� �ε�
	bool			load(CString sFile);

	//���ҽ��� JPG or PNG ���� �ε�
	bool			load(CString sType, UINT id);

	//png�� ���� sType�� ������ �� �ִ�.
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

	//�̹��� �ε巴�� ����
	void			set_smooth_interpolation(bool use = true);

	void			set_back_color(Gdiplus::Color cr_back);
protected:

	CString			m_filename;
	CRect			m_displayed;

	//���� png�� ��� ��濡 ǥ���� zigzag ���� �귯��
	std::unique_ptr<Gdiplus::TextureBrush> m_br_zigzag;
	Gdiplus::Color	m_cr_zigzag_back = Gdiplus::Color::White;
	Gdiplus::Color	m_cr_zigzag_fore = Gdiplus::Color(200, 200, 200);

	Gdiplus::Color	m_cr_back = Gdiplus::Color::Transparent;

	Gdiplus::InterpolationMode m_interplationMode = Gdiplus::InterpolationModeHighQualityBicubic;

//Ȯ��, ��� ������ ������� �ʰ� â ũ�⿡ ����
	bool			m_fit2ctrl = false;

//Ȯ�� �� �̵�
	double			m_zoom = 1.0;
	//mode : 1(zoom in), -1(zoom out), 0(reset)
	CPoint			m_offset = CPoint(0, 0);
	int				m_offset_size = 8;

//���콺 �巡��
	bool			m_lbutton_down = false;
	CPoint			m_ptClicked = CPoint(0, 0);
	HCURSOR			m_hCursor;

	//roi ����(���ÿ���)
	//roi�� screen�������θ� �����ϸ� �̹��� scroll, resize ���� �Ҷ����� �׻� ��������� �ϹǷ�
	//roi�� ������ ������ image_roi�� ����ؼ� ������ ����
	//��ġ, ũ�� ����ÿ� image_roi�� screen_roi�� �����Ͽ� ǥ���Ѵ�.
	CRect			m_image_roi;	//������ ���� ROI
	CRect			m_screen_roi;	//���÷��̵ǰ� �ִ� ȭ����� ROI

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


