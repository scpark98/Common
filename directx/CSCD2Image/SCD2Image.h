#pragma once

#include <afxwin.h>
//#include <d2d1.h>
#include <d2d1_1.h>
#include <d2d1helper.h>
#include <d2d1effects_2.h>
#include <d3d11.h>
#include <wincodec.h>
#include <wrl/client.h>

#include "../../Functions.h"

using namespace Microsoft::WRL;

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


	//dx, dy 좌표에 그려준다.
	D2D1_RECT_F				draw(ID2D1DeviceContext* d2dc, int dx = 0, int dy = 0);

	//pt 좌표에 그려준다.
	D2D1_RECT_F				draw(ID2D1DeviceContext* d2dc, D2D1_POINT_2F pt = D2D1::Point2F());

	//d2dc 캔버스에 정해진 draw mode로 그려준다.
	D2D1_RECT_F				draw(ID2D1DeviceContext* d2dc, eSCD2Image_DRAW_MODE draw_mode = eSCD2Image_DRAW_MODE::draw_mode_zoom);
	D2D1_RECT_F				draw(ID2D1DeviceContext* d2dc, D2D1_RECT_F target, eSCD2Image_DRAW_MODE draw_mode = eSCD2Image_DRAW_MODE::draw_mode_zoom);

	//Functions.h에 있는 일반 함수와 동일한 기능을 수행하는 함수이며 추후 D2Functions로 별도 분리해야 한다.
	D2D1_RECT_F				get_ratio_rect(D2D1_RECT_F target, float ratio, int attach = attach_hcenter | attach_vcenter, bool stretch = true);
	D2D1_RECT_F				get_ratio_rect(D2D1_RECT_F target, float width, float height, int attach = attach_hcenter | attach_vcenter, bool stretch = true);

	//n개의 이미지로 구성된 gif와 같은 이미지일 경우 프레임 이동
	int						goto_frame(int index);

protected:
	//대부분은 이미지가 1장이지만 animated gif, jfif, webp 등은 n개의 이미지로 구성된다.
	std::deque<ComPtr<ID2D1Bitmap>>		m_img;
	int						m_frame_index = 0;
	float					m_width;
	float					m_height;
	HRESULT					load(IWICImagingFactory2* WICfactory, ID2D1DeviceContext* d2context, IWICBitmapDecoder* pDecoder);
};

