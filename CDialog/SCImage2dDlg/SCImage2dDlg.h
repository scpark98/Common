/*
[수정할 내용]
- ROI를 그린 후 확대된 이미지를 드래그 할 때 ROI 또한 같이 이동되어야 한다.

[수정한 내용]
* 20250902
- 회전된 사진의 경우 이를 감지하여 자동 회전하도록 CSCGdiplusBitmap 수정.
  단, 이럴 경우는 자동 회전되었음을 알도록 우측 하단에 회전된 정보 아이콘을 표시해줘야 한다.
- 카메라 촬영 정보 관련 exif 정보 표시, 위도경도 정보를 이용한 지도 열기 추가할 것

* 20250519 scpark
- ASee, TesseractOcrDlg 등과 같이 이미지를 디스플레이하고 +, -키로 확대 축소, 드래그로 이동, 선택영역 처리 등
  이러한 용도의 전용 클래스 필요성에 의해 제작함.
  create()을 이용해서 parent의 child로 동적 생성하며
  마우스, 키보드까지 모두 처리하기 위해 CStatic이 아닌 CDialog로 제작함.
	
* [이미지 로딩 관련]
- 한 이미지를 로딩하고 표시하면서 그 이미지가 포함된 폴더내의 모든 이미지 목록을 유지는 것은
  이 클래스에서? 메인에서?
  ASee, TesseractOcrDlg와 같은 프로젝트라면 이 클래스에서 목록을 가져도 문제없지만
  범용적으로는 이 클래스에서 목록을 가질 경우 목록 변경과 같은 이벤트마다
  항상 메인에 이를 메시지로 통보해야 하므로 범용성을 잃을 수 있다.
  우선 이 클래스에서는 하나의 이미지를 표시하는 범위로만 구현한다.

[animated gif 관련]
- CSCGdiplusBitmap에 구현된 thread_gif()를 이용하면 간단하고 편하지만
  이 함수에서 화면 갱신까지 처리되므로 CSCImage2dDlg에서 그린 roi 정보 등이 표시되지 않고 CSCImage2dDlg 내의 다른 컨트롤이 깜빡이는 현상도 발생한다.
  CSCImage2dDlg에서 thread_gif()를 추가하고 직접 재생하도록 수정함.
*/

#pragma once
#include "afxdialogex.h"

#include <mutex>
//#include "Common/SCGdiplusBitmap.h"
#include "Common/ThumbCtrl/SCThumbCtrl.h"
#include "Common/CStatic/SCStatic/SCStatic.h"
#include "Common/CSliderCtrl/SCSliderCtrl/SCSliderCtrl.h"

#include "Common/directx/CSCD2Context/SCD2Context.h"
#include "Common/directx/CSCD2Image/SCD2Image.h"

#include <dwrite.h>
#pragma comment(lib, "dwrite")

#define PIXEL_INFO_CX			80
#define PIXEL_INFO_CY			80

#define GIF_SLIDER_WIDTH		120
#define GIF_SLIDER_HEIGHT		12

#define ROI_RECT_HANDLE_COUNT	9		//roi의 크기 조정 및 이동을 위한 조정 핸들 개수

// CSCImage2dDlg 대화 상자

static const UINT Message_CSCImage2dDlg = ::RegisterWindowMessage(_T("MessageString_CSCImage2dDlg"));

class CSCImage2dDlgMessage
{
public:
	CSCImage2dDlgMessage(CWnd* _pThis, int _msg, int _index = -1)
		: pThis(_pThis), msg(_msg), index(_index)
	{
	}

	CWnd*	pThis = NULL;
	int		msg;
	int		index;
};


class CSCImage2dDlg : public CDialog
{
	DECLARE_DYNAMIC(CSCImage2dDlg)

public:
	CSCImage2dDlg(CWnd* parent = nullptr);   // 표준 생성자입니다.
	virtual ~CSCImage2dDlg();

	bool			create(CWnd* parent, int x = 0, int y = 0, int cx = 100, int cy = 100);

	enum ENUM_VIEW_MODE
	{
		view_mode_toggle = -1,
		view_mode_image,
		view_mode_thumb,
	};
	void			set_view_mode(int view_mode = view_mode_toggle);

	bool			m_show_thumb = false;
	CSCThumbCtrl	m_thumb;
	LRESULT			on_message_CSCThumbCtrl(WPARAM wParam, LPARAM lParam);

	enum ENUM_CSCImage2dDlgMessage
	{
		message_image_changed = 0,	//현재 표시되는 이미지가 변경된 경우
		message_hide_message,
		message_first_image,		//맨 처음 이미지 표시 중 이전 이미지를 표시하려 한 경우
		message_last_image,
		message_lbuttondown,
	};

	//레지스트리에 저장된 recent file 정보가 있다면 로딩한다.
	bool			load();

	//외부 파일 로딩
	bool			load(CString sFile, bool load_thumbs = true);

	//리소스의 JPG or PNG 파일 로딩
	bool			load(CString sType, UINT id);

	//png일 경우는 sType을 생략할 수 있다.
	bool			load(UINT id);

	CString			get_filename(bool fullpath = true);

	void			release();

	std::deque<CString> m_files;
	int				m_index;
	void			display_image(int index, bool scan_folder = false);
	void			display_image(CString filepath, bool scan_folder = false);
	void			build_image_info_str();

	//index = 0 (goto first image), index = -1 (goto last image)
	void			goto_index(int index);
	int				get_image_count() { return m_files.size(); }
	int				get_cur_index() { return m_index; }

	//quality = 0.0f(lowest quality) ~ 1.0f(best quality)
	HRESULT			save(CString filepath, float quality);

	//현재 파일을 비롯해서 폴더를 다시 검사한다.
	void			reload_image();

//슬라이드 쇼 관련
	bool			m_slide_show = false;
	bool			m_slide_show_repeat = false;
	//단위 = 초
	int				m_slide_show_interval = 3;
	//start : 1(start), 0(stop), -1(toggle)
	void			start_slide_show(int start = 1);

	CSize			get_img_size();
	bool			get_show_info() { return m_show_info; }
	void			set_show_info(bool show);
	void			set_alt_info(CString alt_info) { m_alt_info = alt_info; Invalidate(); }

	int				get_channel();

	bool			get_show_pixel_pos() { return m_show_pixel_pos; }
	void			set_show_pixel_pos(bool show);

	bool			get_show_pixel() { return m_show_pixel; }
	void			set_show_pixel(bool show);

	bool			get_show_cursor_guide_line() { return m_show_cursor_guide_line; }
	//1:show, 0:hide, -1:toggle
	void			set_show_cursor_guide_line(int show);

	//d2dc에 Direct2D를 그리고 CDC를 이용해서 부득이하게 그려야 할 경우
	//OnPaint()에서 swapchain()후에 dc를 이용하여 그림을 그리도록 했는데
	//OnSize()에서는 잘 그려지지만 OnMouseMove()에서는 Invalidate()만 하니 깜빡이며 사라진다.
	//OnSize()와 OnMouseMove()에서의 차이는
	//m_d2back.on_resize(m_d2dc.get_d2dc(), m_d2dc.get_swapchain(), sz.width, sz.height);와 같이
	//배경 이미지에 대한 on_resize() 호출이 차이가 있다.
	//따라서 부득이하게 dc를 이용해서 그림을 그리는 코드가 OnPaint()에 포함되어 있다면
	//기본 함수인 Invalidate() 대신 이 함수를 호출하여 on_resize() 호출후에 Invalidate()를 호출하도록 해야 한다.
	void			rerender();

	enum ENUM_COPY_TO_CLIPBOARD
	{
		copy_auto = 0,		//roi가 있으면 roi 영역을, 없다면 이미지 전체를 클립보드로 복사
		copy_whole_image,	//roi 여부와 관계없이 이미지 전체를 클립보드로 복사
		copy_photo_exif,	//사진의 메타정보 텍스트를 클립보드로 복사
	};
	bool			copy_to_clipboard(int type = copy_auto);
	bool			paste_from_clipboard();

	Gdiplus::RectF	get_image_roi();
	void			set_image_roi(Gdiplus::RectF roi = Gdiplus::RectF());
	bool			get_show_roi_info() { return m_show_roi_info; }
	void			set_show_roi_info(bool show) { m_show_roi_info = show; Invalidate(); }

	//마우스를 클릭하여 이미지 오프셋을 변경하거나, roi를 그리거나 변경할때는
	//parent에서 마우스 액션을 무시해야하므로 추가.
	bool			is_lbutton_down() { return m_lbutton_down; }

	//mode : 1(zoom in), -1(zoom out), 0(reset)
	void			zoom(int mode);
	void			zoom(double ratio);
	double			get_zoom_ratio() { return m_zoom; }

	void			rotate(Gdiplus::RotateFlipType type);

	//true일 경우 이 컨트롤의 크기에 맞춰서 이미지를 stretch한다.
	void			fit2ctrl(bool fit, bool invalidate = true);
	bool			get_fit2ctrl() { return m_fit2ctrl; }

	CRect			get_displayed_rect() { return m_r_display; }

	CPoint			get_offset() { return m_offset; }
	void			set_offset(int offset_x, int offset_y) { m_offset = CPoint(offset_x, offset_y); };
	int				get_offset_size() { return m_offset_size; }
	void			scroll(int offset_x, int offset_y);

	//dropper(spuit) cursor 지정
	void			set_dropper_cursor(UINT nID);

	D2D1_BITMAP_INTERPOLATION_MODE	get_interpolation_mode();
	void			set_interpolation_mode(D2D1_BITMAP_INTERPOLATION_MODE mode);

	LRESULT			on_message_from_CSCD2Image(WPARAM wParam, LPARAM lParam);

	void			set_zigzag_color(Gdiplus::Color cr_back, Gdiplus::Color cr_fore);

//animated gif 관련 public member
	void			play();
	//pos위치로 이동한 후 일시정지한다. -1이면 pause <-> play를 토글한다.
	void			pause(int pos = 0);
	//animation thread가 종료되고 화면에도 더 이상 표시되지 않는다. 만약 그대로 멈추길 원한다면 pause_animation()을 호출한다.
	void			stop();
	void			goto_frame(int pos, bool pause = false);			//지정 프레임으로 이동
	void			goto_frame_percent(int pos, bool pause = false);	//지정 % 위치의 프레임으로 이동

//exif
	double			get_gps_latitude();
	double			get_gps_longitude();

	void			set_cross_cursor(UINT nID);

protected:
	enum TIMER_ID
	{
		timer_slide_show = 0,
		timer_thread_buffering,
	};

	std::mutex				m_mutex;

	CSCD2Context			m_d2dc;
	ID2D1SolidColorBrush*	m_brush;
	IDWriteFactory*			m_WriteFactory;
	IDWriteTextFormat*		m_WriteFormat;

	//n개의 이미지를 버퍼링하기 위해 deque로 생성(-2, -1, 0, 1, 2)
	//0이 cur? n개 미만인 경우는 어떻게 저장할 지, 어떤 index가 현재 표시중인 이미지인지 정하기 애매해진다.
	//대개 image viewer는 순방향 또는 역방향으로 계속 보는 편이 많으므로
	//아래와 같이 forward/backward가 전환되면 순방향, 역방향의 이미지들을 버퍼링하는 방식으로 한다.
	//forward면 0(cur), 1, 2, 3, 4
	//backward면 0(cur), -1, -2, -3, -4
	//이미지만 미리 버퍼링 해 놓을 경우 표시는 빨라지지만 그 이미지 파일명, 파일속성등은? m_files에서 해당 파일 인덱스를 찾아서 처리해도 될 듯함.
	std::deque<CSCD2Image>	m_img;
	int				m_buffer_max = 1;		//buffering 관련 test가 아니라면 debugging에 방해되므로 임시로 1로 변경. default=5
	bool			m_is_forward = true;

	bool			m_thread_buffering = false;
	void			thread_buffering();

	CWnd*			m_parent = NULL;
	CString			m_filename;						//실제 파일경로 또는 클립보드에서 또는 리소스 이미지인지에 따라 저장되는 문자열이 다르다.
	//bool			m_image_from_clipboard = false;	//클립보드에서 붙여넣은 이미지인지(파일명 표시 목적)
	CRect			m_r_display;
	
	HCURSOR			m_cursor_cross = NULL;
	HCURSOR			m_cursor_dropper = NULL;

	D2D1_BITMAP_INTERPOLATION_MODE	m_interpolation_mode = D2D1_BITMAP_INTERPOLATION_MODE_LINEAR;


	//투명 png일 경우 배경에 표시할 zigzag 패턴 브러시
	//std::unique_ptr<Gdiplus::TextureBrush> m_br_zigzag;
	//Gdiplus::Color	m_cr_zigzag_back = Gdiplus::Color::White;
	//Gdiplus::Color	m_cr_zigzag_fore = Gdiplus::Color(200, 200, 200);
	//Gdiplus::Color		m_cr_zigzag_back = Gdiplus::Color::White;
	//Gdiplus::Color		m_cr_zigzag_fore = Gdiplus::Color(200, 200, 200);

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
	//파일명부터 exif 정보까지...tab을 누르면 표시되는 정보를 저장해두고
	//클립보드에 복사 명령 시 활용한다.
	CString			m_info_str;

	//roi 관련(선택영역)
	//roi를 screen기준으로만 저장하면 이미지 scroll, resize 등을 할때마다 항상 보정해줘야 하므로
	//roi가 설정된 순간에 image_roi를 계산해서 저장해 놓고
	//위치, 크기 변경시에 image_roi를 screen_roi로 변경하여 표시한다.
	Gdiplus::RectF	m_image_roi;					//영상의 실제 ROI
	Gdiplus::RectF	m_screen_roi;					//디스플레이되고 있는 화면상의 ROI
	bool			m_drawing_roi = false;			//roi를 그리는 중인지
	bool			m_show_roi_info = true;			//roi 좌표 정보 표시 여부

	int				m_handle_index = -1;			//이동 및 크기 조정을 위해 마우스가 위치하거나 클릭된 핸들 인덱스
	CRect			m_roi_handle[ROI_RECT_HANDLE_COUNT];	//사각형 개체의 이동 및 크기 조정을 위한 작은 사각형 9개

//픽셀값을 dc에 그릴지, CSCStatic으로 할지?
	//dc에 그리면 처음엔 심플하지만 font, draw style(multiline vcenter)등 CSCStatic 구현 시 했던 번거로움이 그대로 발생한다.
	//CSCStatic으로 하면 생성 등 처음엔 복잡하지만 결국 더 편한 방법이 된다.
	bool			m_show_pixel = false;
	bool			m_show_pixel_pos = true;
	Gdiplus::PointF	m_pixel_pos;
	CRect			m_r_pixel_pos;
	ID2D1SolidColorBrush* m_brush_pixel_guide = NULL;

	bool			m_show_cursor_guide_line = false;

	//CRect			m_r_pixel;
	Gdiplus::Color	m_cr_pixel = Gdiplus::Color::Black;
	Gdiplus::Color	m_cr_pixel_old = Gdiplus::Color::Black;
	CSCStatic		m_static_pixel;

//animated gif
	//bool			m_paused = false;
	//int				m_frame_index;
	//CRect			m_r_gif;
	//bool			m_run_thread_animation = false;
	//bool			m_thread_animation_terminated = true;
	//bool			m_is_gif_mirror = false;
	//void			thread_gif_animation();

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
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};
