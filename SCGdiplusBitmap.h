#pragma once

/*
scpark.
기존 CSCGdiPlusBitmap 및 CGdiPlusBitmapResource를 CSCGdiplusBitmap이라는 하나의 클래스로 합치고
Gdiplus에서 제공하는 다양한 이미지 효과를 추가함.

* Gdiplus를 자동 초기화, 해제하기 위해 CGdiplusDummyForInitialization 클래스를 추가하고
  GdiplusBitmap.cpp의 맨 위에서 static 인스턴스를 선언함.


- 20221217. animatedGif 추가
	//다른 이미지 포맷과는 달리 내부 쓰레드에서 parent의 DC에 디스플레이 함.
	//간편한 장점도 있으나 parent DC에 디스플레이하면서 parent의 다른 child들과 간섭이 발생하므로
	//CSCGdiplusBitmap에서 디스플레이 할 지
	//parent에서 직접 디스플레이 할 지를 옵션으로 처리해야 한다.

	//헤더에 include 및 멤버변수 선언
	#include "Common/GdiplusBitmap.h"
	...
	CSCGdiplusBitmap m_gif;

	//외부 파일 로딩 방법
	m_gif.load(_T("D:\\media\\test_image\\test.gif"));

	//리소스 로딩 방법. 반드시 .gif를 .bin과 같이 다른 이름으로 변경 후 리소스 추가할 것.
	//리소스 ID앞에 UINT로 명시해야 load()함수의 모호함이 없어진다.
	m_gif.load(_T("GIF"), UINT(IDR_GIF_CAT_LOADING));

	//로딩 후 parent의 DC 및 좌표를 넘겨주면 자동 재생됨.
	m_gif.set_animation(m_hWnd);// , 0, 0, 150, 130, true);

	//save_gif_frames()를 이용하여 각 프레임을 저장 가능.

	=> 특정 프레임만 표시, 투명 표시 등등의 편의성을 고려할 때 CStatic을 상속받아 변경할 예정

[20250902]
- 회전된 이미지인 경우 이를 감지하여 자동 회전되도록 수정.
- 그 외 Gdiplus::PropertyItem*을 얻어왔으므로 이를 

[20250609]
- animated gif 재생방식 분리
	CSCGdiplusBitmap에서 직접 gif를 재생하면 편리하지만 parent의 다른 child들과 간섭 발생 등 단점이 많아 이를 옵션으로 처리함.
	load("test.gif", true)로 호출하면 CSCGdiplusBitmap에서 자동 재생되고
	load("test.gif", false)로 호출하면 load하는 윈도우에서 직접 재생하게 된다.

*/

//gdiplus 1.1을 사용하기 위해 pch.h, framework.h등에 이 define을 추가했으나 적용되지 않았다.
//결국 여기에 추가하니 해결됨.
#ifdef GDIPVER
#undef GDIPVER
#endif
#define GDIPVER 0x0110

#include <afxwin.h>
#include <gdiplus.h>
#include <stdint.h>	//for uint8_t in vs2015
#include <algorithm>
#include <vector>
#include <memory>
#include <cmath>

static const UINT Message_CSCGdiplusBitmap = ::RegisterWindowMessage(_T("MessageString_CSCGdiplusBitmap"));

//주의할 점은 TOP, LEFT는 0이므로 먼저 비교하면 안되고 CENTER, RIGHT인지 검사한 후 아니면 LEFT라고 판단해야 한다.
//align = ALIGN_LEFT | ALIGN_BOTTOM 을 주면 8이 되는데 이렇게 호출하면
//if (align & ALIGN_LEFT) //이 비교 결과는 0이 되므로 실행되지 않는다.
//=> CSCGdiplusBitmap::canvas_size()를 호출할 때 이미지 위치를 지정할 때 사용하기 위해 정의했으나
//이미 afxbutton.h에 정의되어 중복된다. DrawText()에서 사용되는 align을 그냥 이용한다.
//#define ALIGN_TOP        0x00000000
//#define ALIGN_LEFT       0x00000000
//#define ALIGN_CENTER     0x00000001
//#define ALIGN_RIGHT      0x00000002
//#define ALIGN_VCENTER    0x00000004
//#define ALIGN_BOTTOM     0x00000008

//Gdiplus 초기화 과정을 자동으로 하기 위해 CGdiplusDummyForInitialization 클래스를 선언하고
//GdiplusBitmap.cpp의 맨 위에서 static 인스턴스를 선언함.
class CGdiplusDummyForInitialization
{
public:
	CGdiplusDummyForInitialization()
	{
		Gdiplus::GdiplusStartupInput gdiplusStartupInput;

		if (Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL) != Gdiplus::Ok)
		{
			AfxMessageBox(TEXT("ERROR:Falied to initalize GDI+ library"));
		}
	}

	~CGdiplusDummyForInitialization()
	{
		Gdiplus::GdiplusShutdown(gdiplusToken);
	}

protected:
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
};

//https://exiv2.org/tags.html
class CSCEXIFInfo
{
public:
	CString camera_make;
	CString camera_model;
	CString software;
	CString image_description;
	CString image_copyright;
	CString original_datetime;
	double exposure_time = 1.0;
	double exposure_bias = 0.0;
	double f_number = 0.0;				//"
	unsigned short iso_speed = 0;
	char flash = 0;
	double focal_length = 0.0;
	double focal_length_in_35mm = 0.0;
	double gps_altitude;
	CString gps_latitude;
	CString gps_longitude;
	CString rotated_str;

	CString get_exif_str()
	{
		CString res;
		res.Format(_T("카메라 제조사: %s\n카메라 모델명: %s\n소프트웨어: %s\n촬영 시각: %s\n플래시: %s\n초점 거리: %.1f mm\n35mm 환산: %.1f\n")\
			_T("노출 시간 : 1/%d sec\n노출 보정: %.2f EV\n조리개 값: f/%.1f\nISO 감도: %d\n회전 정보: %s\nGPS 정보: N %s, E %s, %.0fm"),
			camera_make,
			camera_model,
			software,
			original_datetime,
			flash ? _T("on") : _T("off"),
			focal_length,
			focal_length_in_35mm,
			(unsigned)round(1.0 / exposure_time),
			exposure_bias,
			f_number,
			iso_speed,
			rotated_str,
			gps_latitude,
			gps_longitude,
			gps_altitude);
		return res;
	}
};

class CSCGdiplusBitmapMessage
{
public:
	CSCGdiplusBitmapMessage(Gdiplus::Bitmap* _this, int _message, int _frame_index, int _total_frames)
	{
		pThis = _this;
		message = _message;
		frame_index = _frame_index;
		total_frames = _total_frames;
	}

	Gdiplus::Bitmap* pThis = NULL;
	int		message;
	int		frame_index = -1;
	int		total_frames = -1;
};

class CSCGdiplusBitmap
{
public:
	Gdiplus::Bitmap* m_pBitmap = NULL;

	enum CSCGdiplusBitmapMsgs
	{
		message_image_modified = 0,
		message_gif_frame_changed,
	};

public:
	CSCGdiplusBitmap();
	CSCGdiplusBitmap(Gdiplus::Bitmap* src);
	CSCGdiplusBitmap(HBITMAP hBitmap);
	CSCGdiplusBitmap(IStream* pStream);
	CSCGdiplusBitmap(CString pFile);
	CSCGdiplusBitmap(CSCGdiplusBitmap* src);
	//sType은 "png", "jpg", "gif"를 대표적으로 지원하며 "tiff", "webp" 등 다른 포맷도 지원하지만 우선 배제함.
	//"ico" 파일은 크기 파라미터도 중요하므로 load_icon()을 사용하도록 한다.
	CSCGdiplusBitmap(CString sType, UINT id);
	CSCGdiplusBitmap(int cx, int cy, Gdiplus::PixelFormat format = PixelFormat32bppARGB, Gdiplus::Color cr = Gdiplus::Color::Transparent);

	//not tested.
	IStream* CreateStreamOnFile(LPCTSTR pszPathName);
	IStream* CreateStreamOnResource(LPCTSTR lpName, LPCTSTR lpType);
	IWICBitmapSource* LoadBitmapFromStream(IStream* ipImageStream);

	Gdiplus::Graphics* get_graphics() { Gdiplus::Graphics g(m_pBitmap); return &g; }

	virtual ~CSCGdiplusBitmap();

	bool			m_referenced_variable = false;

	void			create(int cx, int cy, Gdiplus::PixelFormat format = PixelFormat32bppARGB, Gdiplus::Color cr = Gdiplus::Color::Transparent);

	//CTreeCtrl, CListCtrl등에서 선택된 항목 자체를 이미지로 리턴(drag시에 사용)
	void			create_drag_image(CWnd* pWnd);

	void			release();

	//CSCGdiplusBitmap과 CSCGdiplusBitmapResource 두 개의 클래스가 있었으나 통합함.
	//외부 이미지 파일 로딩
	bool			load(CString sfile);
	//다음 예시와 같이 리소스 ID앞에 UINT로 명시해야 load()함수의 모호함이 없어진다.
	//m_gif.load(_T("GIF"), UINT(IDR_GIF_CAT_LOADING));
	bool			load(CString sType, UINT id);
	//png일 경우는 sType을 생략할 수 있다.
	bool			load(UINT id);
	bool			load_icon(UINT id, int size = 32);
	bool			load_webp(CString sfile);

	CString			get_filename(bool fullpath = true);

	//animated gif를 load한 경우 재생을 CSCGdiplusBitmap에서 자체적으로 하느냐(기본값),
	//load하는 메인 클래스에서 직접 thread로 재생하느냐를 설정
	//일반적으로는 특정 dlg에서 gif를 로딩하여 CSCGdiplusBitmap 자체에서 바로 재생하도록 하면 편하지만
	//ASee.exe와 같이 roi 설정, 기타 child ctrl들이 존재하여 그들과의 간섭이 문제가 될 경우는 CSCGdiplusBitmap에서 재생시키지 않고
	//CSCImageDlg에서 직접 재생시켜야 하는 경우도 있다.
	void			set_gif_play_itself(bool play_itself = true) { m_gif_play_in_this = play_itself; }
	bool			is_gif_play_itself() { return m_gif_play_in_this; }

	Gdiplus::Bitmap* CreateARGBBitmapFromDIB(const DIBSECTION& dib);


	operator Gdiplus::Bitmap*() const { return m_pBitmap; }

//image pixel data를 사용하고자 할 경우
	//기본적으로는 이미지 raw data를 추출하진 않는다.
	//cv::Mat의 data처럼 raw data가 필요한 경우에 이 함수를 호출하면 사용이 가능해진다.
	bool			get_raw_data();
	//data 값을 변경한 후 다시 이미지에 적용시키려면 반드시 set_raw_data()를 호출해야 유지된다.
	bool			set_raw_data();
	uint8_t*		data = NULL;

//palette
	Gdiplus::ColorPalette* m_palette = NULL;
	bool			get_palette();
	//8bit indexed 이미지의 팔레트 보정 여부. 최초 로딩 후 1회만 해야 한다.
	bool			m_palette_adjusted = false;

	//m_pBitmap이 유효하고, width, height 모두 0보다 커야 한다.
	bool			is_empty();
	bool			is_valid();
	int				get_channel();
	CSize			size() { return CSize(width, height); }

	//이미지 비율은 보통 width/height지만 반대 비율을 원할 경우는 wh = false로 준다.
	float			get_ratio(bool wh = true);

	//alpha 픽셀을 포함한 이미지인지 판별.
	//-1 : 아직 판별되지 않은 상태이므로 판별 시작
	// 0 : 3채널이거나 4채널의 모든 픽셀의 alpha = 255인 경우
	// 1 : 한 점이라도 alpha < 255인 경우
	int				has_alpha_pixel();

	//PixelFormat24bppRGB과 같이 정의된 값을 문자열로 리턴하며 simple = true일 경우는 "RGB (24bit)"와 같이 리턴한다.
	//fmt가 주어지지 않으면 현재 이미지의 PixelFormat을 구하여 결과를 리턴한다.
	//한번 구한 후에는 m_pixel_format_str 변수에 저장되고 이를 리턴하지만 다시 구해야 할 경우는 reset = true로 호출한다.
	CString			get_pixel_format_str(Gdiplus::PixelFormat fmt = -1, bool simple = true, bool reset = false);


	//두 색이 교차하는 지그재그 패턴 브러쉬를 생성한다.
	//sz_tile : 32로 주면 16x16 크기의 타일로 반복되는 지그재그 패턴이 생성된다. 즉, 16x16 zigzag 한 셀 4개가 32x32 크기의 타일로 반복됨.
	static std::unique_ptr<Gdiplus::TextureBrush> get_zigzag_pattern(int sz_tile = 32, Gdiplus::Color cr_back = Gdiplus::Color::White, Gdiplus::Color cr_fore = Gdiplus::Color(200, 200, 200));
	static Gdiplus::Color m_cr_zigzag_back;
	static Gdiplus::Color m_cr_zigzag_fore;

	//targetRect를 주면 대상 영역에 비율을 유지하여 그린다.
	//targetRect가 NULL이면 0,0에 이미지 크기대로 그린다.
	//draw시에 CDC를 넘기느냐 Gdiplus::Graphics를 넘기느냐 고민했으나 Gdiplus::Graphics에 설정코드가 적용된채로 사용하는 경우가 많으므로
	//Gdiplus::Graphics를 넘기는게 맞다고 판단함.
	//간혹 Gdiplus::Graphics를 사용하지 않고 간단히 dc에도 그리는 경우도 존재하므로 모두 지원해야 한다.
	//draw_mode = draw_mode_zoom(maintain ratio, resize to fit),
	//draw_mode = draw_mode_stretch,
	//draw_mode = draw_mode_origin(as image original 1:1 size),
	CRect			draw(Gdiplus::Graphics& g, CRect targetRect, int draw_mode = draw_mode_zoom);
	CRect			draw(Gdiplus::Graphics& g, int dx = 0, int dy = 0, int dw = 0, int dh = 0);
	CRect			draw(Gdiplus::Graphics& g, CSCGdiplusBitmap mask, CRect targetRect);
	CRect			draw(CDC* pDC, CRect targetRect, int draw_mode = draw_mode_zoom);
	CRect			draw(CDC* pDC, int dx = 0, int dy = 0, int dw = 0, int dh = 0);

	//CSCGdiplusBitmap 이미지를 현재 이미지의 targetRect에 그린다.
	CRect			draw(CSCGdiplusBitmap *img, CRect* targetRect = NULL);

	//그림을 그리지 않고 표시될 영역 정보만 얻는다.
	CRect			calc_rect(CRect targetRect, int draw_mode = draw_mode_zoom);

	//이 차례는 3state button의 차례와 동일하게 정의함.
	//BST_UNCHECKED, BST_CHECKED, BST_INDETERMINATE
	enum GDIP_DRAW_MODE
	{
		draw_mode_origin = 0,	//targetRect의 중앙에 원본 크기로 그림
		draw_mode_stretch,		//targetRect에 꽉차게 그림(non ratio)
		draw_mode_zoom,			//targetRect에 ratio를 유지하여 그림(가로 또는 세로에 꽉참)
	};

	//Gdiplus::Bitmap::Clone은 shallow copy이므로 완전한 복사를 위해서는 deep_copy를 사용해야 한다.
	void			clone(CSCGdiplusBitmap* dst);
	void			deep_copy(CSCGdiplusBitmap* dst);
	static void		deep_copy(Gdiplus::Bitmap** dst, Gdiplus::Bitmap* src);
	void			rotate(Gdiplus::RotateFlipType type);
	//회전시키면 w, h가 달라지므로 이미지의 크기를 보정해줘야만 하는 경우도 있다.
	//그럴 경우는 auto_resize를 true로 주고 불필요한 배경이 생겼을 경우는
	//불필요한 배경의 색상을 지정하여 이미지 크기를 fit하게 줄일수도 있다.
	void			rotate(float degree, bool auto_resize = false, Gdiplus::Color remove_back_color = Gdiplus::Color::Transparent);
	//좌우대칭
	void			mirror();
	//상하대칭
	void			flip();

	//이미지에 직접 텍스트를 추가
	CRect		draw_text(	int x, int y, int w, int h,
							CString text,
							float font_size,
							int font_style = Gdiplus::FontStyleRegular,
							int shadow_depth = 0,
							float thickness = 0.0f,
							CString font_name = _T("맑은 고딕"),
							Gdiplus::Color cr_text = Gdiplus::Color::Black,
							Gdiplus::Color cr_stroke = Gdiplus::Color::LightGray,
							Gdiplus::Color cr_shadow = Gdiplus::Color::DarkGray,
							Gdiplus::Color cr_back = Gdiplus::Color::Transparent,
							UINT align = DT_CENTER | DT_VCENTER);

	CRect		draw_text(	CRect rTarget,
							CString text,
							float font_size,
							int font_style = Gdiplus::FontStyleRegular,
							int shadow_depth = 0,
							float thickness = 0.0f,
							CString font_name = _T("맑은 고딕"),
							Gdiplus::Color cr_text = Gdiplus::Color::Black,
							Gdiplus::Color cr_stroke = Gdiplus::Color::LightGray,
							Gdiplus::Color cr_shadow = Gdiplus::Color::DarkGray,
							Gdiplus::Color cr_back = Gdiplus::Color::Transparent,
							UINT align = DT_CENTER | DT_VCENTER);

	//InterpolationModeNearestNeighbor		: 원본 화소를 거의 유지하지만 일부 화소는 사라짐. 그래서 더 거친 느낌
	//InterpolationModeHighQualityBilinear	: 부드럽게 resize되지만 약간 뿌옇게 변함
	//InterpolationModeHighQualityBicubic	: 속도는 느리지만 최고 품질 모드
	enum INTERPOLATION_TYPE
	{
		interpolation_none = Gdiplus::InterpolationModeNearestNeighbor,
		interpolation_bilinear = Gdiplus::InterpolationModeHighQualityBilinear,
		interpolation_bicubic = Gdiplus::InterpolationModeHighQualityBicubic,
		interpolation_lanczos,
	};

	//cx 또는 cy가 0인 경우는 가로/세로 비율을 유지하여 resize된다.
	void			resize(int cx, int cy, Gdiplus::InterpolationMode mode = Gdiplus::InterpolationModeHighQualityBicubic);
	void			resize(float fx, float fy, Gdiplus::InterpolationMode mode = Gdiplus::InterpolationModeHighQualityBicubic);

	//이미지 캔버스 크기를 조정한다. 남는 공간은 cr_fill로 채운다. cr_fill이 투명이 아닌 경우 주의할 것.
	//align 상수값은 DT_CENTER | DT_VCENTER와 같이 DrawText()에서 사용하는 align 상수값을 빌려 사용한다.
	void			canvas_size(int cx, int cy, int align = DT_CENTER | DT_VCENTER, Gdiplus::Color cr_fill = Gdiplus::Color::Transparent);

	//투명 png의 l, t, r, b의 투명한 영역 크기를 구한다.
	CRect			get_transparent_rect();

	//현재 이미지의 일부분을 이 객체 이미지로 재설정한다.
	//즉, 원본 이미지를 가진 객체에서 sub_image()를 호출하면 이 객체는 crop된 이미지로 변경된다.
	//따라서 A이미지의 r 영역을 B이미지 할당하고자 할 경우는 다음 두 단계를 거친다.
	//A.deep_copy(&B);
	//B.sub_image(r);
	void			sub_image(float x, float y, float w, float h);
	void			sub_image(CRect r);
	void			sub_image(Gdiplus::Rect r);
	void			sub_image(Gdiplus::RectF r);
	void			fit_to_image(Gdiplus::Color remove_back_color = Gdiplus::Color::Transparent);
	void			set_colorkey(Gdiplus::Color low, Gdiplus::Color high);
	bool			is_equal(Gdiplus::Color cr0, Gdiplus::Color cr1, int channel = 3);

	void			set_matrix(Gdiplus::ColorMatrix *colorMatrix, Gdiplus::ColorMatrix *grayMatrix = NULL);
	//0(transparent) ~ 255(opaque)
	void			set_alpha(int alpha);

	//Gdiplus::Bitmap::GetPixel()이므로 data를 추출하지 않고도 구할 수 있으나 속도가 느림
	Gdiplus::Color	get_color(int x, int y);

	//data를 추출하여 주소값으로 구하므로 속도가 빠름. get_raw_data()를 호출하여 data를 추출한 경우에만 사용할 것!
	Gdiplus::Color	get_pixel(int x, int y);
	void			set_pixel(int x, int y, Gdiplus::Color cr);


	//특정 위치의 색상이나 특정색상을 새로운 색상으로 변경한다.
	void			replace_color(int tx, int ty, Gdiplus::Color dst);
	void			replace_color(Gdiplus::Color src, Gdiplus::Color dst = Gdiplus::Color::Transparent);

	//투명 png의 배경색을 변경한다. undo는 지원되지 않는다.
	void			set_back_color(Gdiplus::Color cr_back);

	//지정된 색으로 채운다.
	void			clear(Gdiplus::Color cr);

	//현재 이미지에 더해지는 것이므로 계속 누적될 것이다.
	//원본에 적용하는 것이 정석이나 구조 수정이 필요하다.
	void			add_rgb(int red, int green, int blue);
	//void add_rgb_loop(int red, int green, int blue, COLORREF crExcept);

//조정
	//bright, constrast 등을 변경할 경우 원본에 적용할지, 현재 이미지에 적용할지에 대한 정책을 정해야 한다.
	//1.원본에 적용할 경우
	//	- m_origin을 항상 유지하므로 메모리가 낭비될 수 있다.
	//	- 맨 처음 이미지 변경이 일어나는 경우에만 동적할당해주면 낭비를 최소화 할 수 있다.
	//	- bright, contrast, hsl, rgba 등 adjust가 붙은 경우에만 이러한 처리를 하도록 한다.
	//2.현재 이미지에 적용할 경우
	//	- adjust_bright(255)를 하고 adjust_bright(-255)를 해도 원래대로 돌아올 수 없다. 원본에 대해 1회성으로만 적용 가능하다.
	//bright = 0% ~ 400% 범위의 percentage값이며 100%가 원본과 동일하다.
	int				adjust_bright(int bright, bool adjust_others = true);
	//contrast = 0% ~ 400% 범위의 percentage값이며 100%가 원본과 동일하다.
	int				adjust_contrast(int contrast, bool adjust_others = true);

	int				get_bright() { return m_bright; }
	int				get_contrast() { return m_contrast; }

	int				increase_bright(int interval);
	int				increase_contrast(int interval);

	void			reset_adjust();

	//hue : -180 ~ 180, sat : -100 ~ 100, light : -100 ~ 100
	void			adjust_hsl(int hue, int sat = 0, int light = 0);
	void			adjust_rgba(float r, float g, float b, float a = 1.0);

//효과
	//https://github.com/bfraboni/FastGaussianBlur
	//sigma = Gaussian standard deviation (float). Should be positive.
	//order: optional filter order [1: box, 2: bilinear, 3: biquadratic, 4. bicubic, ..., 10]. should be positive. Default is 3
	//border: optional treatment of image boundaries[mirror, extend, crop, wrap].Default is mirror.
	void			blur(float sigma = 10.0f, int order = 1/*, int border = 2*/);
	//Gdiplus에서 제공하는 GaussianBlur 방식이지만 radius가 작을 경우는 좌우로만 흐려지는 등의 문제가 있다.
	void			gdip_blur(float radius, BOOL expandEdge);

	//round shadow rect 이미지로 생성
	void			round_shadow_rect(int w, int h, float radius, float blur_sigma = 5.0f, Gdiplus::Color cr_shadow = Gdiplus::Color::Gray);

	void			transformBits();


	//회색톤으로 변경만 할 뿐 실제 픽셀포맷은 변경되지 않는다.
//weight가 1.0f이면 original gray이미지로 변경되지만 1.0f보다 크면 밝아지고 작으면 어두워짐.
	void			gray(float weight = 1.0f);
	//rgb 모두 -1.0f이면 black&white가 되고 red만 1.0f라면 red&white와 같이 효과가 적용된다.
	void			black_and_white(float red = -1.0f, float green = -1.0f, float blue = -1.0f);
	void			negative();
	void			sepia();
	void			polaroid();
	void			rgb_to_bgr();

	//factor(0.01~0.99)		: 중점부터 밖으로 까매지는 정도. 0.0이면 blur 없음.
	//position(0.01~0.99)	: 바깥에서 중점으로 밝아지는 정도. 0.0이면 blur 없음.
	//일반적인 테두리 블러 효과는 0.95f, 0.10f로 줄것
	//4개의 코너를 라운드 처리 할것인지도 옵션으로 줄 수 있다.
	void			round_corner(float radius, float factor = 0.0f, float position = 0.0f, bool tl = true, bool tr = true, bool br = true, bool bl = true);

	//src의 src_bgra_index에 해당하는 채널값을(bgra중의 n번 채널 인덱스)
	//dst의 dst_bgra_index에 해당하는 채널값으로 변경
	//ex. src에 마스크 이미지를, dst에 원본 이미지를 지정하고 index를 3, 3으로 하면
	//src의 alpha값을 dst의 alpha값으로 변경한다.
	//이를 이용하여 mask overlay로 활용할 수 있다.
	void			replace_channel(Gdiplus::Bitmap* src, Gdiplus::Bitmap* dst, int src_bgra_index, int dst_bgra_index);
	void			replace_channel(Gdiplus::Bitmap* src, int src_bgra_index, int dst_bgra_index);
	void			replace_channel(CString type, UINT srcID, int src_bgra_index, int dst_bgra_index);
	void			replace_channel(CString src_file, int src_bgra_index, int dst_bgra_index);
	Gdiplus::PathGradientBrush* createFluffyBrush(Gdiplus::GraphicsPath* gp, float* blendFactors, float* blendPositions, INT count, INT* in_out_count);


	//배경 그림자 이미지를 생성한다.
	void			create_back_shadow_image(CSCGdiplusBitmap* shadow, float sigma = 10.0f, int type = 0, int depth = 10);

	//ColorMatrix를 이용하여 간단히 흑백이미지를 만들 수 있지만
	//그건 3채널의 흑백톤의 이미지이므로 1채널 256 gray이미지가 아니다.
	//현재 resource에서 읽어들인 이미지는 제대로 변환되지 않는 문제 있음(1/3만 로딩되는 현상)
	void			convert2gray();

	//new_format은 PixelFormat32bppARGB 과 같은 타입으로 호출한다.
	void			convert(Gdiplus::PixelFormat new_format);

	//printf()처럼 save(_T("D:\\test_%d.png"), i);와 같이 사용할 수 있다.
	bool			savef(LPCTSTR filepath, ...);
	bool			save(Gdiplus::Bitmap* bitmap, CString filename, int quality = 100);
	//테스트 목적으로 파일을 저장하여 확인할 경우는 주의해야 한다.
	//ASee 프로젝트에서는 CDirWatcher에 의해 현재 이미지가 속한 폴더를 모니터링하고 있으므로
	//계속 refresh하게 되어 thread_buffering()에서 계속 오류가 발생한 적이 있다.
	bool			save(CString filename, int quality = 100);
	bool			copy_to_clipbard();
	bool			paste_from_clipboard();

	int				width = 0;
	int				height = 0;
	int				channel = 0;
	int				stride = 0;

//animated Gif 관련
	int				m_frame_count;
	int				m_frame_index;
	//PropertyItem 정보 (orientation, GpsLongitude, GpsAltitude, GpsSpeed, ImageDescription, Orientation...)
	//https://www.devhut.net/getting-a-specific-image-property-using-the-gdi-api/
	Gdiplus::PropertyItem* m_property_item = NULL;
	//m_frame_delay도 m_property_item에 속해있는데 뭔가 추가적으로 frame_delay를 얻어오기 위해서는 별도로 malloc을 해야하는 듯하다.
	//일단 해당 항목은 별도 변수로 구해서 사용한다.
	Gdiplus::PropertyItem* m_frame_delay = NULL;

	//카메라로 촬영된 사진의 경우는 exif 정보를 필요로 할 수 있다.
	//property의 갯수는 사진마다 다르지만 주요 정보만 미리 정해서 필요할 경우 출력시킨다.
	//Common\image_processing\exif를 이용해서 쉽게 추출할 수 있으나 dependency가 발생하므로
	//m_property_item을 이용해서 간단히 추출하는 것이 좋을듯하다.


	bool			is_animated_gif() { return (is_valid() && (m_frame_count > 1)); }
	int				get_frame_count() { return m_frame_count; }
	//parenthWnd 내의 지정된 영역에 표시. 투명효과는 지원되지 않는다.
	//parent에 관계없이 투명하게 표시할 경우는 CImageShapeWnd를 사용.
	void			set_animation(HWND parenthWnd, int x = 0, int y = 0, int w = 0, int h = 0, bool auto_play = true);
	//void	set_animation(HWND parenthWnd, int x, int y, float ratio = 1.0f, bool start = true);
	//ratio를 유지하여 r안에 표시한다.
	void			set_animation(HWND parenthWnd, CRect r, bool start = true);
	void			move_gif(int x = 0, int y = 0, int w = 0, int h = 0);
	void			move_gif(CRect r);
	void			set_gif_back_color(COLORREF cr) { m_cr_back.SetFromCOLORREF(cr); }
	void			set_gif_back_color(Gdiplus::Color cr) { m_cr_back = cr; }
	void			play_gif();
	//pos위치로 이동한 후 일시정지한다. -1이면 pause <-> play를 토글한다.
	void			pause_gif(int pos = 0);
	//animation thread가 종료되고 화면에도 더 이상 표시되지 않는다. 만약 그대로 멈추길 원한다면 pause_animation()을 호출한다.
	void			stop_gif();
	void			goto_frame(int pos, bool pause = false);			//지정 프레임으로 이동
	void			goto_frame_percent(int pos, bool pause = false);	//지정 % 위치의 프레임으로 이동
	void			set_gif_mirror(bool is_mirror = true) { m_is_gif_mirror = is_mirror; }

	//gif 프레임 이미지들을 지정된 폴더에 저장
	bool			save_gif_frames(CString folder);
	//gif 프레임 이미지들을 추출해서 직접 display할 수 있다.
	//단, 모든 이미지를 추출하여 메모리에 로드하므로 메모리 사용량이 문제될 수 있다.
	void			get_gif_frames(std::vector<Gdiplus::Bitmap*>& dqBitmap, std::vector<long> &dqDelay);

	//총 재생시간을 ms단위로 리턴한다.
	int				get_total_duration();

	int				get_ani_width() { return m_ani_width; }
	int				get_ani_height() { return m_ani_height; }
	int				set_ani_width(int dpwidth) { m_ani_width = dpwidth; return m_ani_width; }
	int				set_ani_height(int dpheight) { m_ani_height = dpheight; return m_ani_height; }

	void			save_multi_image();// std::vector<Gdiplus::Bitmap*>& dqBitmap);

//exif
	CString			get_exif_str();

protected:
	CString			m_filename = _T("untitled");


	//-1 : 아직 판별되지 않은 상태이므로 판별 시작
	// 0 : 3채널이거나 4채널의 모든 픽셀의 alpha = 255인 경우
	// 1 : 한 점이라도 alpha < 255인 경우
	int				m_has_alpha_pixel = -1;	
	void			resolution();
	Gdiplus::Bitmap* GetImageFromResource(CString lpType, UINT id);

	CString			m_pixel_format_str;

	CSCEXIFInfo		m_exif_info;

	Gdiplus::Bitmap*m_pOrigin = NULL;
	int				m_bright = 100;		//100%가 기본값
	int				m_contrast = 100;	//100%가 기본값

//animatedGif
	bool			m_paused = false;
	bool			m_gif_play_in_this = true;	//gif 재생코드를 이 클래스에서 자체 실행할 지, parent에서 직접 실행할 지

	//ani gif 표시 윈도우
	HWND			m_target_hwnd;

	//ani 시작 x 좌표
	int				m_ani_sx;
	int				m_ani_sy;
	int				m_ani_width;
	int				m_ani_height;
	Gdiplus::Color	m_cr_back = Gdiplus::Color::Transparent;
	bool			m_run_thread_animation = false;
	bool			m_is_gif_mirror = false;

	void			check_animate_gif();
	void			thread_gif_animation();
	void			goto_gif_frame(int frame);
};
