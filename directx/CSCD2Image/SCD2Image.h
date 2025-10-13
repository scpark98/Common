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

static const UINT Message_CSCD2Image = ::RegisterWindowMessage(_T("MessageString_CSCD2Image"));

enum class eSCD2Image_DRAW_MODE
{
	draw_mode_original = 0,		//target�� left, top�� ���� ũ��� �׸�
	draw_mode_original_center,	//target�� �߾ӿ� ���� ũ��� �׸�
	draw_mode_stretch,			//target�� ������ �׸�(non ratio)
	draw_mode_zoom,				//target�� ratio�� �����Ͽ� �׸�(���� �Ǵ� ���ο� ����)
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


	//dx, dy ��ǥ�� dw, dh ũ��� �׷��ش�.
	//dw �Ǵ� dh �� 1���� 1 �̻��̶�� ������ 1���� ������ �°� �ڵ� �����ȴ�.
	//�� �� 0�����̸� ���� ũ��� �׷��ش�.
	D2D1_RECT_F				draw(ID2D1DeviceContext* d2dc, int dx = 0, int dy = 0, int dw = 0, int dh = 0);

	//pt ��ǥ�� �׷��ش�.
	D2D1_RECT_F				draw(ID2D1DeviceContext* d2dc, D2D1_POINT_2F pt = D2D1::Point2F());

	//d2dc ĵ������ ������ draw mode�� �׷��ش�.
	D2D1_RECT_F				draw(ID2D1DeviceContext* d2dc, eSCD2Image_DRAW_MODE draw_mode = eSCD2Image_DRAW_MODE::draw_mode_zoom);
	D2D1_RECT_F				draw(ID2D1DeviceContext* d2dc, D2D1_RECT_F target, eSCD2Image_DRAW_MODE draw_mode = eSCD2Image_DRAW_MODE::draw_mode_zoom);

	//Functions.h�� �ִ� �Ϲ� �Լ��� ������ ����� �����ϴ� �Լ��̸� ���� D2Functions�� ���� �и��ؾ� �Ѵ�.
	D2D1_RECT_F				get_ratio_rect(D2D1_RECT_F target, float ratio, int attach = attach_hcenter | attach_vcenter, bool stretch = true);
	D2D1_RECT_F				get_ratio_rect(D2D1_RECT_F target, float width, float height, int attach = attach_hcenter | attach_vcenter, bool stretch = true);

	//n���� �̹����� ������ gif�� ���� �̹����� ��� ������ �̵�
	int						goto_frame(int index);

//animated gif
	void					set_parent(HWND hWnd) { m_parent = hWnd; }
	void					play();
	void					pause(int pos = 0);
	void					stop();

protected:
	//��κ��� �̹����� 1�������� animated gif, jfif, webp ���� n���� �̹����� �����ǹǷ� deque�� ó���Ѵ�.
	std::deque<ComPtr<ID2D1Bitmap>>		m_img;
	int						m_frame_index = 0;
	float					m_width;
	float					m_height;
	HRESULT					load(IWICImagingFactory2* WICfactory, ID2D1DeviceContext* d2context, IWICBitmapDecoder* pDecoder);

//animated gif
	HWND					m_parent = NULL; //animation�� ����� �� parent���� �޽����� ������ ���� �ʿ�
	std::deque<int>			m_frame_delay; //in milliseconds
	bool					m_run_thread_animation = false;
	bool					m_ani_paused = false;
	bool					m_ani_mirror = false;

	void					thread_animation();
};
