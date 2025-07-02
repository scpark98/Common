/*
* 20250519 scpark
* ASee, TesseractOcrDlg 등과 같이 이미지를 디스플레이하고 +, -키로 확대 축소, 드래그로 이동, 선택영역 처리 등
* 이러한 용도의 전용 클래스 필요성에 의해 제작함.
* create()을 이용해서 parent의 child로 동적 생성하며
* 마우스, 키보드까지 모두 처리하기 위해 CStatic이 아닌 CDialog로 제작함.
* 
* [이미지 로딩 관련]
* - 한 이미지를 로딩하고 표시하면서 그 이미지가 포함된 폴더내의 모든 이미지 목록을 유지는 것은
*   이 클래스에서? 메인에서?
*   ASee, TesseractOcrDlg와 같은 프로젝트라면 이 클래스에서 목록을 가져도 문제없지만
*   범용적으로는 이 클래스에서 목록을 가질 경우 목록 변경과 같은 이벤트마다
*   항상 메인에 이를 메시지로 통보해야 하므로 범용성을 잃을 수 있다.
*   우선 이 클래스에서는 하나의 이미지를 표시하는 범위로만 구현한다.
* 
* [animated gif 관련]
* - CGdiplusBitmap에 구현된 thread_gif()를 이용하면 간단하고 편하지만
*   이 함수에서 화면 갱신까지 처리되므로 CSCImageDlg에서 그린 roi 정보 등이 표시되지 않고 CSCImageDlg 내의 다른 컨트롤이 깜빡이는 현상도 발생한다.
*   CSCImageDlg에서 thread_gif()를 추가하고 직접 재생하도록 수정함.
*/

#pragma once
#include "afxdialogex.h"

#include "../../GdiplusBitmap.h"
#include "../../CStatic/SCStatic/SCStatic.h"
#include "../../CSliderCtrl/SCSliderCtrl/SCSliderCtrl.h"
//#include "../../CToolTipCtrl/RichToolTipCtrl.h"
//#include "../../CToolTipCtrl/XInfoTip.h"

#define PIXEL_INFO_CX		80
#define PIXEL_INFO_CY		80

#define GIF_SLIDER_WIDTH	120
#define GIF_SLIDER_HEIGHT	14

#define MAX_ROI_RECT_HANDLE	9		//roi의 크기 조정 및 이동을 위한 조정 핸들 개수

// CSCImageDlg 대화 상자

class CSCImageDlg : public CDialog
{
	DECLARE_DYNAMIC(CSCImageDlg)

public:
	CSCImageDlg(CWnd* parent = nullptr);   // 표준 생성자입니다.
	virtual ~CSCImageDlg();

	//원칙적으로는 protected 멤버변수로 선언해야 하나 CGdiplusBitmap의 method를 이용할 경우
	//CSCImageDlg에도 매번 관련 멤버함수를 추가해야 하는 번거로움이 있다.
	CGdiplusBitmap	m_img;

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
	void			set_filename(CString filename) { m_filename = filename; }

	bool			get_show_info() { return m_show_info; }
	void			set_show_info(bool show);
	void			set_alt_info(CString alt_info) { m_alt_info = alt_info; Invalidate(); }

	bool			get_show_pixel() { return m_show_pixel; }
	void			set_show_pixel(bool show);

	bool			copy_to_clipbard();
	bool			paste_from_clipboard();

	Gdiplus::RectF	get_image_roi();
	void			set_image_roi(Gdiplus::RectF roi = Gdiplus::RectF()) { m_image_roi = roi; Invalidate(); }
	bool			get_show_roi_info() { return m_show_roi_info; }
	void			set_show_roi_info(bool show) { m_show_roi_info = show; Invalidate(); }

	//mode : 1(zoom in), -1(zoom out), 0(reset)
	void			zoom(int mode);
	void			zoom(double ratio);
	double			get_zoom_ratio() { return m_zoom; }

	void			rotate(Gdiplus::RotateFlipType type) { m_img.rotate(type); Invalidate(); }

	//true일 경우 이 컨트롤의 크기에 맞춰서 이미지를 stretch한다.
	void			fit2ctrl(bool fit);
	bool			get_fit2ctrl() { return m_fit2ctrl; }

	CRect			get_displayed_rect() { return m_r_display; }

	CPoint			get_offset() { return m_offset; }
	void			set_offset(int offset_x, int offset_y) { m_offset = CPoint(offset_x, offset_y); };
	int				get_offset_size() { return m_offset_size; }

	//dropper(spuit) cursor 지정
	void			set_dropper_cursor(UINT nID);

	int				get_smooth_interpolation();
	void			set_smooth_interpolation(int type);

	LRESULT			on_message_from_GdiplusBitmap(WPARAM wParam, LPARAM lParam);

	void			set_zigzag_color(Gdiplus::Color cr_back, Gdiplus::Color cr_fore);

//animated gif 관련 public member
	void			play_gif();
	//pos위치로 이동한 후 일시정지한다. -1이면 pause <-> play를 토글한다.
	void			pause_gif(int pos = 0);
	//animation thread가 종료되고 화면에도 더 이상 표시되지 않는다. 만약 그대로 멈추길 원한다면 pause_animation()을 호출한다.
	void			stop_gif();
	void			goto_frame(int pos, bool pause = false);			//지정 프레임으로 이동
	void			goto_frame_percent(int pos, bool pause = false);	//지정 % 위치의 프레임으로 이동


protected:
	CWnd*			m_parent = NULL;
	CString			m_filename;
	CRect			m_r_display;
	
	HCURSOR			m_cursor_dropper;

	//투명 png일 경우 배경에 표시할 zigzag 패턴 브러시
	std::unique_ptr<Gdiplus::TextureBrush> m_br_zigzag;
	Gdiplus::Color	m_cr_zigzag_back = Gdiplus::Color::White;
	Gdiplus::Color	m_cr_zigzag_fore = Gdiplus::Color(200, 200, 200);

	Gdiplus::InterpolationMode m_interplationMode = (Gdiplus::InterpolationMode)CGdiplusBitmap::interpolation_bicubic;// Gdiplus::InterpolationModeHighQualityBicubic;

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

//이미지 정보 표시
	bool			m_show_info = true;
	//파일명 뒤에 인덱스 정보등의 추가 정보를 표시하고자 할 경우 설정한다.
	CString			m_alt_info;

	//roi 관련(선택영역)
	//roi를 screen기준으로만 저장하면 이미지 scroll, resize 등을 할때마다 항상 보정해줘야 하므로
	//roi가 설정된 순간에 image_roi를 계산해서 저장해 놓고
	//위치, 크기 변경시에 image_roi를 screen_roi로 변경하여 표시한다.
	Gdiplus::RectF	m_image_roi;					//영상의 실제 ROI
	Gdiplus::RectF	m_screen_roi;					//디스플레이되고 있는 화면상의 ROI
	bool			m_show_roi_info = true;			//roi 좌표 정보 표시 여부

	int				m_handle_index = -1;			//이동 및 크기 조정을 위해 마우스가 위치하거나 클릭된 핸들 인덱스
	CRect			m_roi_handle[MAX_ROI_RECT_HANDLE];	//사각형 개체의 이동 및 크기 조정을 위한 작은 사각형 9개

//픽셀값을 dc에 그릴지, CSCStatic으로 할지?
	//dc에 그리면 처음엔 심플하지만 font, draw style(multiline vcenter)등 CSCStatic 구현 시 했던 번거로움이 그대로 발생한다.
	//CSCStatic으로 하면 생성 등 처음엔 복잡하지만 결국 더 편한 방법이 된다.
	bool			m_show_pixel = false;
	//CRect			m_r_pixel;
	Gdiplus::Color	m_cr_pixel = Gdiplus::Color::Black;
	Gdiplus::Color	m_cr_pixel_old = Gdiplus::Color::Black;
	CSCStatic		m_static_pixel;

//animated gif
	bool			m_paused = false;
	int				m_frame_index;
	CRect			m_r_gif;
	bool			m_run_thread_animation = false;
	bool			m_thread_animation_terminated = true;
	bool			m_is_gif_mirror = false;
	void			thread_gif_animation();

	CSCSliderCtrl	m_slider_gif;
	LRESULT			on_message_from_CSCSliderCtrl(WPARAM wParam, LPARAM lParam);

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
	//afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnMouseHover(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
};
