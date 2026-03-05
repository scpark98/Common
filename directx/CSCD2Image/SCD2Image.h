/*
- 기존 Gdiplus를 이용한 이미지 표시 및 그리기는 이미지가 크면 클수록 속도가 크게 저하된다.
  Direct2D를 이용해서 이미지를 표시하기 위해 제작함.
  단, Gdiplus를 이용한 CSCGdiplusBitmap은 이미지 정보를 이 클래스에서 저장하고 parent의 CDC에 그렸으나
  CSCD2Image는 CDC에 그리는 것이 아닌 ID2D1DeviceContext에 그려야 하므로
  CSCD2Context를 추가하고 m_d2dc.init(m_hWnd);을 호출하여 그려질 대상 윈도우의 ID2D1DeviceContext를 생성한 후
  그 d2dc에 그려야하므로 CSCD2Image와 CSCD3Context는 함께 사용되어야 한다.

  in .h
	CSCD2Context				m_d2dc;
	CSCD2Image					m_d2back;

  in .cpp
    m_d2dc.init(m_hWnd);
	m_d2back.load(m_d2dc.get_WICFactory(), m_d2dc.get_d2dc(), IDR_JPG_LOBBY, _T("JPG"));
	m_d2back.load(m_d2dc.get_WICFactory(), m_d2dc.get_d2dc(), IDR_PNG_IMAGE);
	or
	m_d2back.load(m_d2dc.get_WICFactory(), m_d2dc.get_d2dc(), _T("d:\\back.jpg"));


[animated gif, webp 등 여러 프레임을 담은 이미지 처리]
- load가 끝나고 total_frame > 1이면 play()가 자동 호출되고
  각 프레임 delay마다 m_frame_index를 증가시키는 thread가 구동되어
  parent window에게 메시지를 보내서 Invalidate()을 통해 화면이 갱신된다.
  m_d2dc를 알고 있으므로 이 클래스에서 직접 렌더할 수도 있지만 그럴 경우
  parent에서 배경을 그리므로 배경위에 gif가 자연스럽게 렌더시키기 애매해진다.

	m_d2gif.load(m_d2dc.get_WICFactory(), m_d2dc.get_d2dc(), IDR_GIF_NHQV06, _T("GIF"));
	m_d2gif.set_parent(m_hWnd);	//frame_changed 메시지를 받기 위해 parent를 꼭 지정해줘야 한다.
*/

#pragma once

#include <afxwin.h>
//#include <d2d1.h>
#include <d2d1_1.h>
#include <dxgi1_2.h>
#include <d2d1helper.h>
//#include <d2d1effects.h>
#include <d2d1effects_2.h>
#include <d3d11.h>
#include <wincodec.h>
#include <wrl/client.h>

#include "../../Functions.h"

using namespace Microsoft::WRL;

static const UINT Message_CSCD2Image = ::RegisterWindowMessage(_T("MessageString_CSCD2Image"));

enum
{
	DM_UNDEFINED = 0,
	DM_NONE = 1,
	DM_BACKGROUND = 2,
	DM_PREVIOUS = 3
};

enum class eSCD2Image_DRAW_MODE
{
	draw_mode_original = 0,		//target의 left, top에 원본 크기로 그림
	draw_mode_original_center,	//target의 중앙에 원본 크기로 그림
	draw_mode_stretch,			//target에 꽉차게 그림(non ratio)
	draw_mode_zoom,				//target에 ratio를 유지하여 그림(가로 또는 세로에 꽉참)
};


class CSCD2Image;

class CSCD2ImageMessage
{
public:
	CSCD2ImageMessage(CSCD2Image* _this, int _message, int _frame_index, int _total_frames)
		: pThis(_this), message(_message), frame_index(_frame_index), total_frames(_total_frames)
	{
	}

	CSCD2Image* pThis = nullptr;
	int		message;
	int		frame_index = -1;
	int		total_frames = -1;
};

class CSCD2Image
{
public:
	CSCD2Image();
	CSCD2Image(const CSCD2Image&) = delete;  // 복사 금지
	CSCD2Image& operator=(const CSCD2Image&) = delete;  // 복사 할당 금지

	CSCD2Image(CSCD2Image&& other) noexcept;  // 이동 생성자
	CSCD2Image& operator=(CSCD2Image&& other) noexcept;  // 이동 할당 연산자

	CSCD2Image(IWICImagingFactory2* WICfactory, ID2D1DeviceContext* d2context, int width, int height)
	{
		create(WICfactory, d2context, width, height);
	}
	~CSCD2Image();

	enum ENUM_CSCD2ImageMessages
	{
		message_image_modified = 0,
		message_frame_changed,
	};

	HRESULT					create(IWICImagingFactory2* WICfactory, ID2D1DeviceContext* d2context, int width, int height);
	void					release();

	//load external image
	HRESULT					load(IWICImagingFactory2* WICfactory, ID2D1DeviceContext* d2context, CString path, bool auto_play = true);

	//load resource image
	HRESULT					load(IWICImagingFactory2* WICfactory, ID2D1DeviceContext* d2context, UINT resource_id, CString type = _T("PNG"), bool auto_play = true);

	//load from raw data
	HRESULT					load(IWICImagingFactory2* WICfactory, ID2D1DeviceContext* d2context, void* data, int width, int height, int channel);

	//must call when the parent window was resized
	//HRESULT					on_resize(ID2D1DeviceContext* d2context, IDXGISwapChain* swapchain, int cx, int cy);

	IWICImagingFactory2*	get_WICFactory2() { return m_pWICFactory;}
	ID2D1DeviceContext*		get_d2dc() { return m_d2dc;}
	ID2D1Bitmap1*			get_cur_img() { return m_img[m_frame_index].Get(); }
	ID2D1Bitmap1*			get_frame_img(int index);
	HRESULT					set_frame_img(ID2D1Bitmap1* img);

	std::deque<ComPtr<ID2D1Bitmap1>>* get_img_list() { return &m_img; };
	std::deque<int>*		get_frame_delay_list() { return &m_frame_delay; }
	bool					set_frame_delay(int index, int delay_ms);

	//get original image demension
	float					get_width();
	float					get_height();
	D2D1_SIZE_F				get_size();
	float					get_ratio();
	int						get_channel() { return m_channel; }
	
	Gdiplus::Color			get_pixel(int x, int y);

	//PixelFormat24bppRGB과 같이 정의된 값을 문자열로 리턴하며 simple = true일 경우는 "RGB (24bit)"와 같이 리턴한다.
	//fmt가 주어지지 않으면 현재 이미지의 PixelFormat을 구하여 결과를 리턴한다.
	//한번 구한 후에는 m_pixel_format_str 변수에 저장되고 이를 리턴하지만 다시 구해야 할 경우는 reset = true로 호출한다.
	CString					get_pixel_format_str(WICPixelFormatGUID *pf = NULL, bool simple = true, bool reset = false);

	//alpha pixel들의 count를 구한다. (m_alpha_pixel_count < 0 || recount = true)이면 새로 계산한다. 그렇지 않으면 이미 구해놓은 정보를 리턴한다.
	//현재는 gif, webp라도 0번 이미지에 대한 m_data를 대상으로 한다.
	int						get_alpha_pixel_count(bool recount = false);


	//테두리 픽셀들을 이용하여 배경색을 자동 감지한다. index가 -1이면 index = m_frame_index
	Gdiplus::Color			detect_back_color(int index = -1);

	//index 위치의 이미지에서 배경 색상을 transparent 색상으로 변경한다.
	//내부적으로는 make_back_transparent()를 호출하지만
	//set_back_transparency()는 현재 이미지의 원본을 보존하면서 투명하게 만들고자 하는 경우에 사용한다. make_back_transparent()는 현재 이미지 자체를 변경한다.
	void					set_back_transparency(int index, int inner_threshold = 30, int outer_threshold = 120, Gdiplus::Color cr_back = Gdiplus::Color::Transparent);

	//cr_back을 Transparent가 아닌 다른 색으로 주면 해당 색상을 투명 배경색으로 처리한다.
	//cr_back을 Red로 주면 실제 배경이 Red가 아니어도 Red에 가까운 색상은 투명하게 처리한다.
	//inner_threshold는 완전히 투명한 색상과의 최대 허용 오차, (default 30)
	//outer_threshold는 완전히 불투명한 색상과의 최소 허용 오차이다. (default 120)
	//inner_threshold와 outer_threshold 사이의 색상은 반투명으로 처리한다.
	//index = -1이면 모든 프레임에 대해 적용한다.
	//inner_threshold: 이 거리 이내의 픽셀은 완전 투명 (기본값 30)
	//outer_threshold: 이 거리 이상의 픽셀은 원본 유지 (기본값 120)
	void					make_back_transparent(int index, int inner_threshold = 30, int outer_threshold = 120, Gdiplus::Color cr_back = Gdiplus::Color::Transparent);


	//index 위치의 이미지가 nullptr이 아니고, width, height 모두 0보다 커야 한다.
	bool					is_empty(int index = 0);
	bool					is_valid(int index = 0);

	CString					get_filename(bool fullpath = true);

	int						get_interpolation_mode() { return m_interpolation_mode; }
	void					set_interpolation_mode(int mode);


	//dx, dy 좌표에 dw, dh 크기로 그려준다.
	//dw 또는 dh 중 1개가 1 이상이라면 나머지 1개는 비율에 맞게 자동 조정된다.
	//둘 다 0이하이면 원본 크기로 그려준다.
	//dw, dh가 모두 0이면 dwr, dhr 비율로 조정한 후 그려준다.
	//dwr = 0이면 dhr을 기준으로, dhr=0이면 dwr기준으로 비율을 유지해서 그려준다.
	D2D1_RECT_F				draw(ID2D1DeviceContext* d2dc, int dx = 0, int dy = 0, int dw = 0, int dh = 0, float dwr = 0.f, float dhr = 0.f);

	//pt 좌표에 그려준다.
	D2D1_RECT_F				draw(ID2D1DeviceContext* d2dc, D2D1_POINT_2F pt = D2D1::Point2F());

	//d2dc 캔버스에 정해진 draw mode로 그려준다.
	D2D1_RECT_F				draw(ID2D1DeviceContext* d2dc, eSCD2Image_DRAW_MODE draw_mode = eSCD2Image_DRAW_MODE::draw_mode_zoom);
	D2D1_RECT_F				draw(ID2D1DeviceContext* d2dc, D2D1_RECT_F target, eSCD2Image_DRAW_MODE draw_mode = eSCD2Image_DRAW_MODE::draw_mode_zoom);

	//그림을 그리지 않고 표시될 영역 정보만 얻는다.
	CRect					calc_rect(CRect targetRect, eSCD2Image_DRAW_MODE draw_mode = eSCD2Image_DRAW_MODE::draw_mode_zoom);


	//data멤버에 픽셀 데이터를 가리키도록 한다. 구현중...
	void					get_raw_data();

	void					blend(ID2D1DeviceContext* d2dc, ID2D1Bitmap* src, ID2D1Bitmap* blend_img, int dx, int dy, int sx, int sy, int sw, int sh);
	//void					copy(ID2D1DeviceContext* d2dc, ID2D1Bitmap* src, ID2D1Bitmap* dst);

	//반드시 new 또는 정적으로 메모리가 할당된 상태의 dst를 넘겨줘야 한다.
	//null의 dst를 넘기고 copy()에서 new로 할당해도 되지만 new와 delete이 서로 다른 thread에서 수행될 수 없으므로 문제가 될 수 있다.
	void					copy(CSCD2Image* dst);
	//m_img의 특정 인덱스 이미지를 dst에 복사한다.
	//반드시 new 또는 정적으로 메모리가 할당된 상태의 dst를 넘겨줘야 한다.
	//null의 dst를 넘기고 copy()에서 new로 할당해도 되지만 new와 delete이 서로 다른 thread에서 수행될 수 없으므로 문제가 될 수 있다.
	HRESULT					copy(int src_index, ID2D1Bitmap1* dst);

	HRESULT					get_sub_img(CRect r, CSCD2Image *dest);
	HRESULT					get_sub_img(D2D1_RECT_U r, CSCD2Image* dest);

	void					restore_original_image();
	void					blur_effect(float dev = 3.0f);

	//quality = 0.0f(lowest quality) ~ 1.0f(best quality)
	//현재는 jpg만 품질옵션이 적용된다.
	HRESULT					save(CString path, float quality = 1.0f);
	//quality = 0.0f(lowest quality) ~ 1.0f(best quality)
	HRESULT					save(ID2D1Bitmap* img, float quality, LPCTSTR path, ...);
	HRESULT					save_webp(LPCTSTR path, ...);
	HRESULT					save_gif(LPCTSTR path, ...);

	//PBGRA 포맷의 픽셀 데이터를 RGBA 포맷으로 변환한다. un-premultiply와 동시에 B↔R 채널을 교환합니다:
	void					convert_PBGRA_to_RGBA(byte* pixels, int width, int height, int stride);

	// un-premultiply만 수행, 채널 순서(BGRA)는 유지
	void					convert_PBGRA_to_BGRA(byte* pixels, int width, int height, int stride);

	//index 위치의 이미지를 클립보드로 복사한다. 0보다 작으면 현재 프레임 이미지를 클립보드로 복사한다.
	bool					copy_to_clipboard(int index = -1);
	//클립보드에 이미지가 있다면 클립보드의 이미지를 index 위치에 붙여넣는다. 0보다 작으면 현재 프레임 이미지에 붙여넣는다.
	bool					paste_from_clipboard(int index = -1);

//animated gif
	bool					is_animated_image() { return (m_img.size() > 1); }
	int						get_cur_frame_index() { return m_frame_index; }
	int						get_frame_count() { return m_img.size(); }
	int						get_frame_delay(int index);
	void					set_parent(HWND hWnd) { m_parent = hWnd; }
	void					play();
	void					pause(int pos = -1);
	bool					stop();
	//n개의 이미지로 구성된 gif와 같은 이미지일 경우 특정 프레임 인덱스로 이동
	int						goto_frame(int index, bool pause = false);
	//n개의 이미지로 구성된 gif와 같은 이미지일 경우 interval 만큼 프레임 이동
	void					step(int interval = 1);

//exif
	CSCEXIFInfo				get_exif() { return m_exif_info; }
	CString					get_exif_str();
	void					apply_orientation_transform();  // EXIF Orientation 적용
	WICBitmapTransformOptions ExifOrientationToWicTransform(USHORT orientation);

	bool					read_webp_frame_delay(const CString& path);


	HRESULT					extract_raw_data_from_bitmap(ID2D1DeviceContext* d2dc, ID2D1Bitmap1* src, uint8_t** data);

protected:
	CString					m_filename;// = _T("untitled");

	//load or draw시에 파라미터로 받아서 참조하여 사용하기 위해 선언했을 뿐이고 이 클래스에서 직접 생성하는 것이 아님.
	IWICImagingFactory2*	m_pWICFactory = nullptr;
	ID2D1DeviceContext*		m_d2dc = nullptr;

	//대부분은 이미지가 1장이지만 animated gif, jfif, webp 등은 n개의 이미지로 구성되므로 deque로 선언한다.
	std::deque<ComPtr<ID2D1Bitmap1>> m_img = std::deque<ComPtr<ID2D1Bitmap1>>{ nullptr, };
	ComPtr<ID2D1Bitmap1>	m_img_origin;

	Gdiplus::Color			m_auto_detected_back_color = Gdiplus::Color::Transparent; //배경 제거 시 투명하게 만들 색상. 기본값은 투명색이지만 배경 제거 시 자동으로 탐지된 색상으로 변경된다.
	//배경 제거 시 슬라이드로 파라미터를 조정하며 제거할 수 있는데 이미 제거를 시도한 이미지를 대상으로 또 제거시도하는 것은 의미없다.
	//슬라이드로 제거 시도할 때 원본 이미지를 복원한 후 파라미터를 적용시켜야 한다.
	//thread_animation() 등의 함수에서 표시되는 프레임이 변경될 때마다 이 변수는 리셋되어야 한다.
	ComPtr<ID2D1Bitmap1>	m_img_origin_for_back_transparency;

	uint8_t*				m_data = nullptr;
	UINT					m_stride = 0;
	int						m_channel = 0;

	int						m_frame_index = 0;

	CString					m_pixel_format_str;
	int						m_alpha_pixel_count = -1;

	//촬영된 이미지의 경우 exif 정보를 추출한다.
	CSCEXIFInfo				m_exif_info;
	HRESULT					extract_exif_info(IWICBitmapDecoder* pDecoder);


	HRESULT					load(IWICImagingFactory2* WICfactory, ID2D1DeviceContext* d2context, IWICBitmapDecoder* pDecoder, bool auto_play = true);

	D2D1_BITMAP_INTERPOLATION_MODE m_interpolation_mode = D2D1_BITMAP_INTERPOLATION_MODE_LINEAR;

//animated gif
	HWND					m_parent = NULL; //animation이 진행될 때 parent에게 메시지를 보내기 위해 필요
	std::deque<int>			m_frame_delay; //in milliseconds
	bool					m_run_thread_animation = false;
	bool					m_thread_animation_terminated = true;
	bool					m_ani_paused = false;
	bool					m_ani_mirror = false;

	void					thread_animation();
};
