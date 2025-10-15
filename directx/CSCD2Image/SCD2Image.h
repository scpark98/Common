/*
[animated gif, webp 등 여러 프레임을 담은 이미지 처리]
- play()가 호출되면 각 프레임 delay마다 m_frame_index를 증가시키는 thread가 구동되고
  parent window에게 메시지를 보내서 Invalidate()을 통해 화면이 갱신된다.
  이 방법은 mfc control이 없고 순수 이미지 표시 방식의 앱에서는 괜찮지만
  화면상에 mfc control이 있다면 깜빡임이 많이 발생할 수 있다.
*/

#pragma once

#include <afxwin.h>
//#include <d2d1.h>
#include <d2d1_1.h>
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

class CSCD2Image
{
public:
	CSCD2Image();
	~CSCD2Image();

	//load external image
	HRESULT					load(IWICImagingFactory2* WICfactory, ID2D1DeviceContext* d2context, CString path);

	//load resource image
	HRESULT					load(IWICImagingFactory2* WICfactory, ID2D1DeviceContext* d2context, UINT resource_id, CString type = _T("PNG"));

	//must call when the parent window was resized
	HRESULT					on_resize(ID2D1DeviceContext* d2context, IDXGISwapChain* swapchain, int cx, int cy);

	ID2D1Bitmap*			get() { return m_img[m_frame_index].Get(); }

	//get original image demension
	D2D1_SIZE_F				get_size() { return D2D1::SizeF(m_width, m_height); }


	//dx, dy 좌표에 dw, dh 크기로 그려준다.
	//dw 또는 dh 중 1개가 1 이상이라면 나머지 1개는 비율에 맞게 자동 조정된다.
	//둘 다 0이하이면 원본 크기로 그려준다.
	D2D1_RECT_F				draw(ID2D1DeviceContext* d2dc, int dx = 0, int dy = 0, int dw = 0, int dh = 0);

	//pt 좌표에 그려준다.
	D2D1_RECT_F				draw(ID2D1DeviceContext* d2dc, D2D1_POINT_2F pt = D2D1::Point2F());

	//d2dc 캔버스에 정해진 draw mode로 그려준다.
	D2D1_RECT_F				draw(ID2D1DeviceContext* d2dc, eSCD2Image_DRAW_MODE draw_mode = eSCD2Image_DRAW_MODE::draw_mode_zoom);
	D2D1_RECT_F				draw(ID2D1DeviceContext* d2dc, D2D1_RECT_F target, eSCD2Image_DRAW_MODE draw_mode = eSCD2Image_DRAW_MODE::draw_mode_zoom);

	//Functions.h에 있는 일반 함수와 동일한 기능을 수행하는 함수이며 추후 D2Functions로 별도 분리해야 한다.
	D2D1_RECT_F				get_ratio_rect(D2D1_RECT_F target, float ratio, int attach = attach_hcenter | attach_vcenter, bool stretch = true);
	D2D1_RECT_F				get_ratio_rect(D2D1_RECT_F target, float width, float height, int attach = attach_hcenter | attach_vcenter, bool stretch = true);

	//data멤버에 픽셀 데이터를 가리키도록 한다. 구현중...
	void					get_raw_data();

	void					blend(ID2D1DeviceContext* d2dc, ID2D1Bitmap* src, ID2D1Bitmap* blend_img, int dx, int dy, int sx, int sy, int sw, int sh);
	void					copy(ID2D1DeviceContext* d2dc, ID2D1Bitmap* src, ID2D1Bitmap* dst);
	void					save(CString path);
	void					save(ID2D1Bitmap* img, LPCTSTR path, ...);

	//n개의 이미지로 구성된 gif와 같은 이미지일 경우 프레임 이동
	int						goto_frame(int index);

//animated gif
	void					set_parent(HWND hWnd) { m_parent = hWnd; }
	void					play();
	void					pause(int pos = 0);
	void					stop();

protected:
	IWICImagingFactory2*	m_pWICFactory = NULL;
	ID2D1DeviceContext*		m_d2dc = NULL;

	//대부분은 이미지가 1장이지만 animated gif, jfif, webp 등은 n개의 이미지로 구성되므로 deque로 처리한다.
	std::deque<ComPtr<ID2D1Bitmap>>		m_img;
	uint8_t*				data = NULL;
	int						m_frame_index = 0;
	float					m_width;
	float					m_height;
	HRESULT					load(IWICImagingFactory2* WICfactory, ID2D1DeviceContext* d2context, IWICBitmapDecoder* pDecoder);

//animated gif
	HWND					m_parent = NULL; //animation이 진행될 때 parent에게 메시지를 보내기 위해 필요
	std::deque<int>			m_frame_delay; //in milliseconds
	bool					m_run_thread_animation = false;
	bool					m_ani_paused = false;
	bool					m_ani_mirror = false;

	void					thread_animation();
};
