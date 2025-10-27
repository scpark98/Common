/*
- ���� Gdiplus�� �̿��� �̹��� ǥ�� �� �׸���� �̹����� ũ�� Ŭ���� �ӵ��� ũ�� ���ϵȴ�.
  Direct2D�� �̿��ؼ� �̹����� ǥ���ϱ� ���� ������.
  ��, Gdiplus�� �̿��� CSCGdiplusBitmap�� �̹��� ������ �� Ŭ�������� �����ϰ� parent�� CDC�� �׷�����
  CSCD2Image�� CDC�� �׸��� ���� �ƴ� ID2D1DeviceContext�� �׷����ϹǷ� CSCD2Context�� �������
  ID2D1DeviceContext�� �׷����ϹǷ� CSCD2Image�� CSCD3Context�� �Բ� ���Ǿ�� �Ѵ�.

[animated gif, webp �� ���� �������� ���� �̹��� ó��]
- play()�� ȣ��Ǹ� �� ������ delay���� m_frame_index�� ������Ű�� thread�� �����ǰ�
  parent window���� �޽����� ������ Invalidate()�� ���� ȭ���� ���ŵȴ�.
  �� ����� mfc control�� ���� ���� �̹��� ǥ�� ����� �ۿ����� ��������
  ȭ��� mfc control�� �ִٸ� �������� ���� �߻��� �� �ִ�.
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
	float					get_width() { return m_width; }
	float					get_height() { return m_height; }
	D2D1_SIZE_F				get_size() { return D2D1::SizeF(m_width, m_height); }
	float					get_ratio();

	//m_pBitmap�� ��ȿ�ϰ�, width, height ��� 0���� Ŀ�� �Ѵ�.
	bool					is_empty(int index = 0);
	bool					is_valid(int index = 0);

	CString					get_filename(bool fullpath = true);


	//dx, dy ��ǥ�� dw, dh ũ��� �׷��ش�.
	//dw �Ǵ� dh �� 1���� 1 �̻��̶�� ������ 1���� ������ �°� �ڵ� �����ȴ�.
	//�� �� 0�����̸� ���� ũ��� �׷��ش�.
	D2D1_RECT_F				draw(ID2D1DeviceContext* d2dc, int dx = 0, int dy = 0, int dw = 0, int dh = 0);

	//pt ��ǥ�� �׷��ش�.
	D2D1_RECT_F				draw(ID2D1DeviceContext* d2dc, D2D1_POINT_2F pt = D2D1::Point2F());

	//d2dc ĵ������ ������ draw mode�� �׷��ش�.
	D2D1_RECT_F				draw(ID2D1DeviceContext* d2dc, eSCD2Image_DRAW_MODE draw_mode = eSCD2Image_DRAW_MODE::draw_mode_zoom);
	D2D1_RECT_F				draw(ID2D1DeviceContext* d2dc, D2D1_RECT_F target, eSCD2Image_DRAW_MODE draw_mode = eSCD2Image_DRAW_MODE::draw_mode_zoom);

	//�׸��� �׸��� �ʰ� ǥ�õ� ���� ������ ��´�.
	CRect					calc_rect(CRect targetRect, eSCD2Image_DRAW_MODE draw_mode = eSCD2Image_DRAW_MODE::draw_mode_zoom);

	//Functions.h�� �ִ� �Ϲ� �Լ��� ������ ����� �����ϴ� �Լ��̸� ���� D2Functions�� ���� �и��ؾ� �Ѵ�.
	CRect					convert(D2D1_RECT_F d2r);
	D2D1_RECT_F				convert(CRect r);
	D2D1_RECT_F				get_ratio_rect(D2D1_RECT_F target, float ratio, int attach = attach_hcenter | attach_vcenter, bool stretch = true);
	D2D1_RECT_F				get_ratio_rect(D2D1_RECT_F target, float width, float height, int attach = attach_hcenter | attach_vcenter, bool stretch = true);

	//data����� �ȼ� �����͸� ����Ű���� �Ѵ�. ������...
	void					get_raw_data();

	void					blend(ID2D1DeviceContext* d2dc, ID2D1Bitmap* src, ID2D1Bitmap* blend_img, int dx, int dy, int sx, int sy, int sw, int sh);
	void					copy(ID2D1DeviceContext* d2dc, ID2D1Bitmap* src, ID2D1Bitmap* dst);
	void					save(CString path);
	void					save(ID2D1Bitmap* img, LPCTSTR path, ...);

//animated gif
	bool					is_animated_image() { return (m_img.size() > 1); }
	int						get_frame_count() { return m_img.size(); }
	void					set_parent(HWND hWnd) { m_parent = hWnd; }
	void					play();
	void					pause(int pos = 0);
	void					stop();
	//n���� �̹����� ������ gif�� ���� �̹����� ��� ������ �̵�
	int						goto_frame(int index);

protected:
	CString					m_filename;// = _T("untitled");

	//load or draw�ÿ� �Ķ���ͷ� �޾Ƽ� �����Ͽ� ����ϱ� ���� �������� ���̰� �� Ŭ�������� ���� �����ϴ� ���� �ƴ�.
	IWICImagingFactory2*	m_pWICFactory = NULL;
	ID2D1DeviceContext*		m_d2dc = NULL;

	//��κ��� �̹����� 1�������� animated gif, jfif, webp ���� n���� �̹����� �����ǹǷ� deque�� ó���Ѵ�.
	std::deque<ComPtr<ID2D1Bitmap>>		m_img = std::deque<ComPtr<ID2D1Bitmap>>{ NULL, };
	uint8_t*				data = NULL;
	int						m_frame_index = 0;
	float					m_width = 0.0f;
	float					m_height = 0.0f;
	HRESULT					load(IWICImagingFactory2* WICfactory, ID2D1DeviceContext* d2context, IWICBitmapDecoder* pDecoder);

//animated gif
	HWND					m_parent = NULL; //animation�� ����� �� parent���� �޽����� ������ ���� �ʿ�
	std::deque<int>			m_frame_delay; //in milliseconds
	bool					m_run_thread_animation = false;
	bool					m_ani_paused = false;
	bool					m_ani_mirror = false;

	void					thread_animation();
};
