#pragma once

/*
scpark.
기존 CGdiPlusBitmap 및 CGdiPlusBitmapResource를 CGdiplusBitmap이라는 하나의 클래스로 합치고
Gdiplus에서 제공하는 다양한 이미지 효과를 추가함.

* Gdiplus를 자동 초기화, 해제하기 위해 CGdiplusDummyForInitialization 클래스를 추가하고
  GdiplusBitmap.cpp의 맨 위에서 static 인스턴스를 선언함.


- 20221217. animatedGif 추가
	//다른 이미지 포맷과는 달리 내부 쓰레드에서 parent의 DC에 디스플레이 함.

	//헤더에 include 및 멤버변수 선언
	#include "../../Common/GdiplusBitmap.h"
	...
	CGdiplusBitmap m_gif;

	//외부 파일 로딩 방법
	m_gif.load(_T("D:\\media\\test_image\\test.gif"));

	//리소스 로딩 방법. 반드시 .gif를 .bin과 같이 다른 이름으로 변경 후 리소스 추가할 것.
	//리소스 ID앞에 UINT로 명시해야 load()함수의 모호함이 없어진다.
	m_gif.load(_T("GIF"), UINT(IDR_GIF_CAT_LOADING));

	//로딩 후 parent의 DC 및 좌표를 넘겨주면 자동 재생됨.
	m_gif.set_animation(m_hWnd);// , 0, 0, 150, 130, true);

	//save_gif_frames()를 이용하여 각 프레임을 저장 가능.

	=> 특정 프레임만 표시, 투명 표시 등등의 편의성을 고려할 때 CStatic을 상속받아 변경할 예정
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

static const UINT Message_CGdiplusBitmap = ::RegisterWindowMessage(_T("MessageString_CGdiplusBitmap"));

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

class CGdiplusBitmapMessage
{
public:
	CGdiplusBitmapMessage(Gdiplus::Bitmap* _this, int _message)
	{
		pThis = _this;
		message = _message;
	}

	Gdiplus::Bitmap* pThis = NULL;
	int		message;
};

class CGdiplusBitmap
{
public:
	Gdiplus::Bitmap* m_pBitmap = NULL;

	enum CGdiplusBitmapMsgs
	{
		message_gif_frame_changed = 0,
	};

public:
	CGdiplusBitmap();
	CGdiplusBitmap(Gdiplus::Bitmap* src);
	CGdiplusBitmap(HBITMAP hBitmap);
	CGdiplusBitmap(IStream* pStream);
	CGdiplusBitmap(CString pFile);
	CGdiplusBitmap(CGdiplusBitmap* src);
	CGdiplusBitmap(CString sType, UINT id);
	CGdiplusBitmap(int cx, int cy, Gdiplus::PixelFormat format = PixelFormat32bppARGB, Gdiplus::Color cr = Gdiplus::Color::Transparent);

	//not tested.
	IStream* CreateStreamOnFile(LPCTSTR pszPathName);
	IStream* CreateStreamOnResource(LPCTSTR lpName, LPCTSTR lpType);
	IWICBitmapSource* LoadBitmapFromStream(IStream* ipImageStream);

	Gdiplus::Graphics* get_graphics() { Gdiplus::Graphics g(m_pBitmap); return &g; }

	virtual ~CGdiplusBitmap();

	void	create(int cx, int cy, Gdiplus::PixelFormat format = PixelFormat32bppARGB, Gdiplus::Color cr = Gdiplus::Color::Transparent);

	//CTreeCtrl, CListCtrl등에서 선택된 항목 자체를 이미지로 리턴(drag시에 사용)
	void	create_drag_image(CWnd* pWnd);

	void	release();

	//CGdiplusBitmap과 CGdiplusBitmapResource 두 개의 클래스가 있었으나 통합함.
	bool	load(CString sFile, bool show_error = false);
	//다음 예시와 같이 리소스 ID앞에 UINT로 명시해야 load()함수의 모호함이 없어진다.
	//m_gif.load(_T("GIF"), UINT(IDR_GIF_CAT_LOADING));
	bool	load(CString sType, UINT id, bool show_error = false);
	//png일 경우는 sType을 생략할 수 있다.
	bool	load(UINT id, bool show_error = false);

	Gdiplus::Bitmap* CreateARGBBitmapFromDIB(const DIBSECTION& dib);


	operator Gdiplus::Bitmap*() const { return m_pBitmap; }

//image pixel data를 사용하고자 할 경우
	//기본적으로는 이미지 raw data를 추출하진 않는다.
	//cv::Mat의 data처럼 raw data가 필요한 경우에 이 함수를 호출하면 사용이 가능해진다.
	bool		get_raw_data();
	//data 값을 변경한 후 다시 이미지에 적용시키려면 반드시 set_raw_data()를 호출해야 유지된다.
	bool		set_raw_data();
	uint8_t*	data = NULL;


	//m_pBitmap이 유효하고, width, height 모두 0보다 커야 한다.
	bool	is_empty();
	bool	is_valid();
	int		channels();
	CSize	size() { return CSize(width, height); }

	//targetRect를 주면 대상 영역에 비율을 유지하여 그린다.
	//targetRect가 NULL이면 0,0에 이미지 크기대로 그린다.
	//draw시에 CDC를 넘기느냐 Gdiplus::Graphics를 넘기느냐 고민했으나 Gdiplus::Graphics에 설정코드가 적용된채로 사용하는 경우가 많으므로
	//Gdiplus::Graphics를 넘기는게 맞다고 판단함.
	//draw_mode = draw_mode_zoom(maintain ratio, resize to fit),
	//draw_mode = draw_mode_stretch,
	//draw_mode = draw_mode_origin(as image original 1:1 size),
	CRect	draw(Gdiplus::Graphics& g, CRect targetRect, int draw_mode = draw_mode_zoom);
	CRect	draw(Gdiplus::Graphics& g, int dx = 0, int dy = 0, int dw = 0, int dh = 0);
	CRect	draw(Gdiplus::Graphics& g, CGdiplusBitmap mask, CRect targetRect);

	//bmp 이미지를 현재 이미지의 targetRect에 그린다.
	CRect	draw(CGdiplusBitmap *bmp, CRect* targetRect = NULL);

	//이 차례는 3state button의 차례와 동일하게 정의함.
	//BST_UNCHECKED, BST_CHECKED, BST_INDETERMINATE
	enum GDIP_DRAW_MODE
	{
		draw_mode_origin = 0,	//targetRect의 중앙에 원본 크기로 그림
		draw_mode_stretch,		//targetRect에 꽉차게 그림(non ratio)
		draw_mode_zoom,			//targetRect에 ratio를 유지하여 그림(가로 또는 세로에 꽉참)
	};

	//Gdiplus::Bitmap::Clone은 shallow copy이므로 완전한 복사를 위해서는 deep_copy를 사용해야 한다.
	void	clone(CGdiplusBitmap* dst);
	void	deep_copy(CGdiplusBitmap* dst);
	void	rotate(Gdiplus::RotateFlipType type);
	//회전시키면 w, h가 달라지므로 이미지의 크기를 보정해줘야만 하는 경우도 있다.
	//그럴 경우는 auto_resize를 true로 주고 불필요한 배경이 생겼을 경우는
	//불필요한 배경의 색상을 지정하여 이미지 크기를 fit하게 줄일수도 있다.
	void	rotate(float degree, bool auto_resize = false, Gdiplus::Color remove_back_color = Gdiplus::Color::Transparent);
	//좌우대칭
	void	mirror();
	//상하대칭
	void	flip();

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
	//cx 또는 cy가 0인 경우는 가로/세로 비율을 유지하여 resize된다.
	void	resize(int cx, int cy, Gdiplus::InterpolationMode mode = Gdiplus::InterpolationModeHighQualityBicubic);
	void	resize(float fx, float fy, Gdiplus::InterpolationMode mode = Gdiplus::InterpolationModeHighQualityBicubic);

	//이미지 캔버스 크기를 조정한다. 남는 공간은 cr_fill로 채운다. cr_fill이 투명이 아닌 경우 주의할 것.
	void	canvas_size(int cx, int cy, Gdiplus::Color cr_fill = Gdiplus::Color::Transparent);

	void	sub_image(int x, int y, int w, int h);
	void	sub_image(CRect r);
	void	sub_image(Gdiplus::Rect r);
	void	fit_to_image(Gdiplus::Color remove_back_color = Gdiplus::Color::Transparent);
	void	set_colorkey(Gdiplus::Color low, Gdiplus::Color high);
	bool	is_equal(Gdiplus::Color cr0, Gdiplus::Color cr1, int channel = 3);

	void	set_matrix(Gdiplus::ColorMatrix *colorMatrix, Gdiplus::ColorMatrix *grayMatrix = NULL);
	//0(transparent) ~ 255(opaque)
	void	set_alpha(int alpha);


	Gdiplus::Color get_color(int x, int y);

	//특정 위치의 색상이나 특정색상을 새로운 색상으로 변경한다.
	void	replace_color(int tx, int ty, Gdiplus::Color dst);
	void	replace_color(Gdiplus::Color src, Gdiplus::Color dst = Gdiplus::Color::Transparent);

	//투명 png의 배경색을 변경한다. undo는 지원되지 않는다.
	void	set_back_color(Gdiplus::Color cr_back);

	//지정된 색으로 채운다.
	void	clear(Gdiplus::Color cr);

	//현재 이미지에 더해지는 것이므로 계속 누적될 것이다.
	//원본에 적용하는 것이 정석이나 구조 수정이 필요하다.
	void	add_rgb(int red, int green, int blue);
	//void add_rgb_loop(int red, int green, int blue, COLORREF crExcept);

//조정
	void	adjust_bright(int bright);
	//contrast = 255이면 원본과 동일.
	void	adjust_contrast(int contrast);

	//hue : -180 ~ 180, sat : -100 ~ 100, light : -100 ~ 100
	void	adjust_hsl(int hue, int sat = 0, int light = 0);
	void	adjust_rgba(float r, float g, float b, float a = 1.0);

//효과
	//https://github.com/bfraboni/FastGaussianBlur
	//sigma = Gaussian standard deviation (float). Should be positive.
	//order: optional filter order [1: box, 2: bilinear, 3: biquadratic, 4. bicubic, ..., 10]. should be positive. Default is 3
	//border: optional treatment of image boundaries[mirror, extend, crop, wrap].Default is mirror.
	void	blur(float sigma = 10.0f, int order = 1/*, int border = 2*/);
	//Gdiplus에서 제공하는 GaussianBlur 방식이지만 radius가 작을 경우는 좌우로만 흐려지는 등의 문제가 있다.
	void	gdip_blur(float radius, BOOL expandEdge);

	//round shadow rect 이미지로 생성
	void	round_shadow_rect(int w, int h, float radius, float blur_sigma = 5.0f, Gdiplus::Color cr_shadow = Gdiplus::Color::Gray);

	void	transformBits();


	//회색톤으로 변경만 할 뿐 실제 픽셀포맷은 변경되지 않는다.
//weight가 1.0f이면 original gray이미지로 변경되지만 1.0f보다 크면 밝아지고 작으면 어두워짐.
	void	gray(float weight = 1.0f);
	//rgb 모두 -1.0f이면 black&white가 되고 red만 1.0f라면 red&white와 같이 효과가 적용된다.
	void	black_and_white(float red = -1.0f, float green = -1.0f, float blue = -1.0f);
	void	negative();
	void	sepia();
	void	polaroid();
	void	rgb_to_bgr();

	//factor(0.01~0.99)		: 중점부터 밖으로 까매지는 정도. 0.0이면 blur 없음.
	//position(0.01~0.99)	: 바깥에서 중점으로 밝아지는 정도. 0.0이면 blur 없음.
	//일반적인 테두리 블러 효과는 0.95f, 0.10f로 줄것
	//4개의 코너를 라운드 처리 할것인지도 옵션으로 줄 수 있다.
	void	round_corner(float radius, float factor = 0.0f, float position = 0.0f, bool tl = true, bool tr = true, bool br = true, bool bl = true);

	//src의 src_bgra_index에 해당하는 채널값을(bgra중의 n번 채널 인덱스)
	//dst의 dst_bgra_index에 해당하는 채널값으로 변경
	//ex. src에 마스크 이미지를, dst에 원본 이미지를 지정하고 index를 3, 3으로 하면
	//src의 alpha값을 dst의 alpha값으로 변경한다.
	//이를 이용하여 mask overlay로 활용할 수 있다.
	void replace_channel(Gdiplus::Bitmap* src, Gdiplus::Bitmap* dst, int src_bgra_index, int dst_bgra_index);
	void replace_channel(Gdiplus::Bitmap* src, int src_bgra_index, int dst_bgra_index);
	void replace_channel(CString type, UINT srcID, int src_bgra_index, int dst_bgra_index);
	void replace_channel(CString src_file, int src_bgra_index, int dst_bgra_index);
	Gdiplus::PathGradientBrush* createFluffyBrush(Gdiplus::GraphicsPath* gp, float* blendFactors, float* blendPositions, INT count, INT* in_out_count);



	//ColorMatrix를 이용하여 간단히 흑백이미지를 만들 수 있지만
	//그건 3채널의 흑백톤의 이미지이므로 1채널 256 gray이미지가 아니다.
	//현재 resource에서 읽어들인 이미지는 제대로 변환되지 않는 문제 있음(1/3만 로딩되는 현상)
	void convert2gray();
	//미구현
	void cvtColor(Gdiplus::PixelFormat old_format, Gdiplus::PixelFormat new_format);
	//현재 이미지를 32bit로 변경한다.
	void cvtColor32ARGB();

	bool save(LPCTSTR filepath, ...);
	bool copy_to_clipbard();
	bool paste_from_clipboard();

	int width = 0;
	int height = 0;
	int channel = 0;
	int stride = 0;

//animated Gif 관련
	int		m_frame_count;
	int		m_frame_index;
	Gdiplus::PropertyItem* m_pPropertyItem = NULL;
	bool	is_animated_gif() { return (is_valid() && (m_frame_count > 1)); }
	int		get_frame_count() { return m_frame_count; }
	//parenthWnd 내의 지정된 영역에 표시. 투명효과는 지원되지 않는다.
	//parent에 관계없이 투명하게 표시할 경우는 CImageShapeWnd를 사용.
	void	set_animation(HWND parenthWnd, int x = 0, int y = 0, int w = 0, int h = 0, bool auto_play = true);
	//void	set_animation(HWND parenthWnd, int x, int y, float ratio = 1.0f, bool start = true);
	//ratio를 유지하여 r안에 표시한다.
	void	set_animation(HWND parenthWnd, CRect r, bool start = true);
	void	move(int x = 0, int y = 0, int w = 0, int h = 0);
	void	move(CRect r);
	void	gif_back_color(COLORREF cr) { m_cr_back.SetFromCOLORREF(cr); }
	void	gif_back_color(Gdiplus::Color cr) { m_cr_back = cr; }
	void	play_animation();
	//pos위치로 이동한 후 일시정지한다. -1이면 pause <-> play를 토글한다.
	void	pause_animation(int pos = 0);
	//animation thread가 종료되고 화면에도 더 이상 표시되지 않는다. 만약 그대로 멈추길 원한다면 pause_animation()을 호출한다.
	void	stop_animation();
	void	goto_frame(int pos, bool pause = false);			//지정 프레임으로 이동
	void	goto_frame_percent(int pos, bool pause = false);	//지정 % 위치의 프레임으로 이동
	void	set_mirror(bool is_mirror = true) { m_is_gif_mirror = is_mirror; }

	//gif 프레임 이미지들을 지정된 폴더에 저장
	bool	save_gif_frames(CString folder);
	//gif 프레임 이미지들을 추출해서 직접 display할 수 있다.
	//단, 모든 이미지를 추출하여 메모리에 로드하므로 메모리 사용량이 문제될 수 있다.
	void	get_gif_frames(std::vector<Gdiplus::Bitmap*>& dqBitmap, std::vector<long> &dqDelay);

	//총 재생시간을 ms단위로 리턴한다.
	int		get_total_duration();

	int		get_ani_width() { return m_ani_width; }
	int		get_ani_height() { return m_ani_height; }
	int		set_ani_width(int dpwidth) { m_ani_width = dpwidth; return m_ani_width; }
	int		set_ani_height(int dpheight) { m_ani_height = dpheight; return m_ani_height; }

	void	save_multi_image();// std::vector<Gdiplus::Bitmap*>& dqBitmap);

protected:
	CString			m_filename = _T("untitled");

	void			resolution();
	Gdiplus::Bitmap* GetImageFromResource(CString lpType, UINT id);

//animatedGif
	bool			m_bIsInitialized;
	bool			m_paused = false;

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
