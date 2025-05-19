/*
* 20250519 scpark
* ASee, TesseractOcrDlg 등과 같이 이미지를 디스플레이하고 +, -키로 확대 축소, 드래그로 이동, 선택영역 처리 등
* 이러한 용도의 전용 클래스 필요성에 의해 제작함.
* parent의 child로 동적 생성하며 마우스, 키보드까지 모두 처리하기 위해 CStatic이 아닌 CDialog로 제작함.
*/

#pragma once
#include "afxdialogex.h"

#include "../../GdiplusBitmap.h"

// CSCImageDlg 대화 상자

class CSCImageDlg : public CDialog
{
	DECLARE_DYNAMIC(CSCImageDlg)

public:
	CSCImageDlg(CWnd* parent = nullptr);   // 표준 생성자입니다.
	virtual ~CSCImageDlg();

	bool			create(CWnd* parent, int x = 0, int y = 0, int cx = 100, int cy = 100);

	//레지스트리에 저장된 recent file 정보가 있다면 로딩한다.
	bool			load();

	//외부 파일 로딩
	bool			load(CString sFile);

	//리소스의 JPG or PNG 파일 로딩
	bool			load(CString sType, UINT id);

	//png일 경우는 sType을 생략할 수 있다.
	bool			load(UINT id);

	CString			get_filename() { return m_filename; }

	bool			get_show_filename() { return true; }
	bool			get_show_pixel() { return m_show_pixel; }

	bool			copy_to_clipbard();

	CRect			get_image_roi();

	//mode : 1(zoom in), -1(zoom out), 0(reset)
	void			zoom(int mode);

	void			fit2ctrl(bool fit);
	bool			get_fit2ctrl() { return m_fit2ctrl; }

	CRect			get_displayed_rect() { return m_displayed; }

	CPoint			get_offset() { return m_offset; }
	void			set_offset(int offset_x, int offset_y) {};
	int				get_offset_size() { return m_offset_size; }

	//이미지 부드럽게 보정
	void			set_smooth_interpolation(bool use = true);

	CGdiplusBitmap	m_img;

protected:
	CWnd*			m_parent = NULL;
	CString			m_filename;
	CRect			m_displayed;

	Gdiplus::InterpolationMode m_interplationMode = Gdiplus::InterpolationModeHighQualityBicubic;

//확대, 축소 배율을 사용하지 않고 창 크기에 맞춤
	bool			m_fit2ctrl = true;

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

//픽셀값을 dc에 그릴지, CSCStatic으로 할지...
	bool			m_show_pixel = true;
	CRect			m_r_pixel;
	Gdiplus::Color	m_cr_pixel = Gdiplus::Color::Black;
	//CSCStatic		m_static_pixel;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
};
