#pragma once

/*
scpark.
기존 CGdiPlusBitmap 및 CGdiPlusBitmapResource를 CGdiplusBitmap이라는 하나의 클래스로 합치고
Gdiplus에서 제공하는 다양한 이미지 효과를 추가함.

- 20221217. animatedGif 추가
	//다른 이미지 포맷과는 달리 내부 쓰레드에서 parent의 DC에 디스플레이 함.

	//헤더에 include 및 멤버변수 선언
	#include "../../Common/GdiplusBitmap.h"
	...
	CGdiplusBitmap m_gif;

	//외부 파일 로딩 방법
	m_gif.load(_T("D:\\media\\test_image\\test.gif"));

	//리소스 로딩 방법. 반드시 .gif를 .bin과 같이 다른 이름으로 변경 후 리소스 추가할 것.
	m_gif.load(_T("GIF"), UINT(IDR_GIF_CAT_LOADING));

	//로딩 후 parent의 DC 및 좌표를 넘겨주면 자동 재생됨.
	m_gif.set_animation(m_hWnd);// , 0, 0, 150, 130, true);

	//save_gif_frames()를 이용하여 각 프레임을 저장 가능.

	=> 특정 프레임만 표시, 투명 표시 등등의 편의성을 고려할 때 CStatic을 상속받아 변경할 예정
*/

//gdiplus 1.1을 사용하기 위해 pch.h, framework.h등에 이 define을 추가했으나 적용되지 않았다.
//결국 여기에 추가하니 해결됨.
#define GDIPVER 0x0110

#include <afxwin.h>
#include <gdiplus.h>
#include <stdint.h>	//for uint8_t in vs2015
#include <algorithm>
#include <vector>

using namespace Gdiplus;

enum CGdiplusBitmap_Message
{
	msg_gif_frame_changed = WM_USER + 761,
};

class CGdiplusBitmap
{
public:
	Gdiplus::Bitmap* m_pBitmap = NULL;
	uint8_t* data = NULL;

public:
	CGdiplusBitmap();
	CGdiplusBitmap(Bitmap* src);
	CGdiplusBitmap(HBITMAP hBitmap);
	CGdiplusBitmap(IStream* pStream);
	CGdiplusBitmap(CString pFile, bool show_error = false);
	CGdiplusBitmap(CGdiplusBitmap* src);
	CGdiplusBitmap(CString sType, UINT id, bool show_error = false);

	virtual ~CGdiplusBitmap();

	void	release();

	//CGdiplusBitmap과 CGdiplusBitmapResource 두 개의 클래스가 있었으나 통합함.
	bool	load(CString sFile, bool show_error = false);
	void	load(CString sType, UINT id, bool show_error = false);

	Gdiplus::Bitmap* CreateARGBBitmapFromDIB(const DIBSECTION& dib);


	operator Gdiplus::Bitmap*() const { return m_pBitmap; }

	//기본적으로는 이미지 raw data를 추출하진 않는다.
	//cv::Mat의 data처럼 raw data가 필요한 경우에 이 함수를 호출하면 사용이 가능해진다.
	bool	get_raw_data();

	bool	empty();
	bool	valid();
	int		channels();
	CSize	size() { return CSize(width, height); }

	//targetRect를 주면 대상 영역에 비율을 유지하여 그린다.
	//targetRect가 NULL이면 0,0에 이미지 크기대로 그린다.
	CRect	draw(CDC* pDC, CRect targetRect);
	CRect	draw(CDC* pDC, int dx = 0, int dy = 0, int dw = 0, int dh = 0);
	CRect	draw(CDC* pDC, CGdiplusBitmap mask, CRect targetRect);

	//Gdiplus::Bitmap::Clone은 shallow copy이므로 완전한 복사를 위해서는 deep_copy를 사용해야 한다.
	void	clone(CGdiplusBitmap* dst);
	void	deep_copy(CGdiplusBitmap* dst);
	void	rotate(Gdiplus::RotateFlipType type);
	//좌우대칭
	void	mirror();
	//상하대칭
	void	flip();

	//회전시키면 w, h가 달라지므로 이미지의 크기를 보정해줘야만 하는 경우도 있다.
	//그럴 경우는 auto_resize를 true로 주고 불필요한 배경이 생겼을 경우는
	//불필요한 배경의 색상을 지정하여 이미지 크기를 fit하게 줄일수도 있다.
	void rotate(float degree, bool auto_resize = false, Gdiplus::Color remove_back_color = Gdiplus::Color::Transparent);

	//InterpolationModeNearestNeighbor		: 원본 화소를 거의 유지하지만 일부 화소는 사라짐. 그래서 더 거친 느낌
	//InterpolationModeHighQualityBilinear	: 부드럽게 resize되지만 약간 뿌옇게 변함
	//InterpolationModeHighQualityBicubic	: 속도는 느리지만 최고 품질 모드
	void resize(int cx, int cy, Gdiplus::InterpolationMode = Gdiplus::InterpolationModeHighQualityBicubic);
	void sub_image(int x, int y, int w, int h);
	void sub_image(CRect r);
	void sub_image(Gdiplus::Rect r);
	void fit_to_image(Gdiplus::Color remove_back_color = Gdiplus::Color::Transparent);
	void set_colorkey(Gdiplus::Color low, Gdiplus::Color high);
	bool is_equal(Gdiplus::Color cr0, Gdiplus::Color cr1, int channel = 3);

	void set_matrix(ColorMatrix *colorMatrix, ColorMatrix *grayMatrix = NULL);
	void set_alpha(float alpha);
	void gray();
	void negative();

	//ColorMatrix를 이용하여 간단히 흑백이미지를 만들 수 있지만
	//그건 3채널의 흑백톤의 이미지이므로 1채널 256 gray이미지가 아니다.
	//현재 resource에서 읽어들인 이미지는 제대로 변환되지 않는 문제 있음(1/3만 로딩되는 현상)
	void convert2gray();

	int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);
	bool save(CString filepath);
	bool save(Gdiplus::Bitmap *bitmap, CString filepath);
	bool copy_to_clipbard();

	int width = 0;
	int height = 0;
	int channel = 0;
	int stride = 0;

	//animatedGif
	void	set_animation(HWND hWnd, int x = 0, int y = 0, int w = 0, int h = 0, bool start = true);
	void	back_color(COLORREF cr) { m_crBack.SetFromCOLORREF(cr); }
	void	back_color(Gdiplus::Color cr) { m_crBack = cr; }
	void	start_animation();
	void	pause_animation();
	void	stop_animation();
	//gif 프레임 이미지들을 지정된 폴더에 저장
	bool	save_gif_frames(CString folder);
	//gif 프레임 이미지들을 추출해서 직접 display할 수 있다.
	void	get_gif_frames(std::vector<CGdiplusBitmap*>& dqImage, std::vector<long> &dqDelay);

	//총 재생시간을 ms단위로 리턴한다.
	int		get_total_duration();

	int		ani_width() { return m_aniWidth; }
	int		ani_height() { return m_aniHeight; }
	int		ani_width(int dpwidth) { m_aniWidth = dpwidth; return m_aniWidth; }
	int		ani_height(int dpheight) { m_aniHeight = dpheight; return m_aniHeight; }

protected:
	CString			m_filename = _T("untitled");
	void resolution();
	Bitmap* GetImageFromResource(CString lpType, UINT id);

	//animatedGif
	UINT			m_total_frame;
	UINT			m_frame_index;
	bool			m_bIsInitialized;
	bool			m_paused = false;
	PropertyItem*	m_pPropertyItem = NULL;
	HWND			m_displayHwnd;
	int				m_aniX;
	int				m_aniY;
	int				m_aniWidth;
	int				m_aniHeight;
	Gdiplus::Color	m_crBack = Gdiplus::Color::Transparent;
	bool			m_run_thread_animation = false;

	void check_animate_gif();
	void thread_gif_animation();
};
