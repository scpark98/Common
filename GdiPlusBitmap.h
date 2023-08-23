#pragma once

/*
scpark.
���� CGdiPlusBitmap �� CGdiPlusBitmapResource�� CGdiplusBitmap�̶�� �ϳ��� Ŭ������ ��ġ��
Gdiplus���� �����ϴ� �پ��� �̹��� ȿ���� �߰���.

- 20221217. animatedGif �߰�
	//�ٸ� �̹��� ���˰��� �޸� ���� �����忡�� parent�� DC�� ���÷��� ��.

	//����� include �� ������� ����
	#include "../../Common/GdiplusBitmap.h"
	...
	CGdiplusBitmap m_gif;

	//�ܺ� ���� �ε� ���
	m_gif.load(_T("D:\\media\\test_image\\test.gif"));

	//���ҽ� �ε� ���. �ݵ�� .gif�� .bin�� ���� �ٸ� �̸����� ���� �� ���ҽ� �߰��� ��.
	m_gif.load(_T("GIF"), UINT(IDR_GIF_CAT_LOADING));

	//�ε� �� parent�� DC �� ��ǥ�� �Ѱ��ָ� �ڵ� �����.
	m_gif.set_animation(m_hWnd);// , 0, 0, 150, 130, true);

	//save_gif_frames()�� �̿��Ͽ� �� �������� ���� ����.

	=> Ư�� �����Ӹ� ǥ��, ���� ǥ�� ����� ���Ǽ��� ����� �� CStatic�� ��ӹ޾� ������ ����
*/

//gdiplus 1.1�� ����ϱ� ���� pch.h, framework.h� �� define�� �߰������� ������� �ʾҴ�.
//�ᱹ ���⿡ �߰��ϴ� �ذ��.
#ifdef GDIPVER
#undef GDIPVER
#endif

#define GDIPVER 0x0110

#include <afxwin.h>
#include <gdiplus.h>
#include <stdint.h>	//for uint8_t in vs2015
#include <algorithm>
#include <vector>

using namespace Gdiplus;

static const UINT Message_CGdiplusBitmap = ::RegisterWindowMessage(_T("MessageString_CGdiplusBitmap"));

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
	uint8_t* data = NULL;

	enum CGdiplusBitmapMsgs
	{
		message_gif_frame_changed = 0,
	};

public:
	CGdiplusBitmap();
	CGdiplusBitmap(Bitmap* src);
	CGdiplusBitmap(HBITMAP hBitmap);
	CGdiplusBitmap(IStream* pStream);
	CGdiplusBitmap(CString pFile, bool show_error = false);
	CGdiplusBitmap(CGdiplusBitmap* src);
	CGdiplusBitmap(CString sType, UINT id, bool show_error = false);
	CGdiplusBitmap(int cx, int cy, Gdiplus::PixelFormat format = PixelFormat32bppARGB, Gdiplus::Color cr = Gdiplus::Color::Transparent);

	//not tested.
	IStream* CreateStreamOnFile(LPCTSTR pszPathName);
	IStream* CreateStreamOnResource(LPCTSTR lpName, LPCTSTR lpType);
	IWICBitmapSource* LoadBitmapFromStream(IStream* ipImageStream);



	virtual ~CGdiplusBitmap();

	void	create(int cx, int cy, Gdiplus::PixelFormat format = PixelFormat32bppARGB, Gdiplus::Color cr = Gdiplus::Color::Transparent);
	void	release();

	//CGdiplusBitmap�� CGdiplusBitmapResource �� ���� Ŭ������ �־����� ������.
	bool	load(CString sFile, bool show_error = false);
	bool	load(CString sType, UINT id, bool show_error = false);
	//png�� ���� sType�� ������ �� �ִ�.
	bool	load(UINT id, bool show_error = false);

	Gdiplus::Bitmap* CreateARGBBitmapFromDIB(const DIBSECTION& dib);


	operator Gdiplus::Bitmap*() const { return m_pBitmap; }

	//�⺻�����δ� �̹��� raw data�� �������� �ʴ´�.
	//cv::Mat�� dataó�� raw data�� �ʿ��� ��쿡 �� �Լ��� ȣ���ϸ� ����� ����������.
	bool	get_raw_data();
	//data ���� ������ �� �ٽ� �̹����� ����
	bool	set_raw_data();

	bool	empty();
	bool	valid();
	int		channels();
	CSize	size() { return CSize(width, height); }

	//targetRect�� �ָ� ��� ������ ������ �����Ͽ� �׸���.
	//targetRect�� NULL�̸� 0,0�� �̹��� ũ���� �׸���.
	CRect	draw(Gdiplus::Graphics* g, CRect targetRect);
	CRect	draw(Gdiplus::Graphics* g, int dx = 0, int dy = 0, int dw = 0, int dh = 0);
	CRect	draw(Gdiplus::Graphics* g, CGdiplusBitmap mask, CRect targetRect);

	//Gdiplus::Bitmap::Clone�� shallow copy�̹Ƿ� ������ ���縦 ���ؼ��� deep_copy�� ����ؾ� �Ѵ�.
	void	clone(CGdiplusBitmap* dst);
	void	deep_copy(CGdiplusBitmap* dst);
	void	rotate(Gdiplus::RotateFlipType type);
	//ȸ����Ű�� w, h�� �޶����Ƿ� �̹����� ũ�⸦ ��������߸� �ϴ� ��쵵 �ִ�.
	//�׷� ���� auto_resize�� true�� �ְ� ���ʿ��� ����� ������ ����
	//���ʿ��� ����� ������ �����Ͽ� �̹��� ũ�⸦ fit�ϰ� ���ϼ��� �ִ�.
	void	rotate(float degree, bool auto_resize = false, Gdiplus::Color remove_back_color = Gdiplus::Color::Transparent);
	//�¿��Ī
	void	mirror();
	//���ϴ�Ī
	void	flip();

	void	draw_text(int x, int y, CString text, int font_size, int thick,
						CString font_name = _T("���� ���"),
						Gdiplus::Color crOutline = Gdiplus::Color::White,
						Gdiplus::Color crFill = Gdiplus::Color::Black,
						UINT align = DT_LEFT | DT_TOP);

	//InterpolationModeNearestNeighbor		: ���� ȭ�Ҹ� ���� ���������� �Ϻ� ȭ�Ҵ� �����. �׷��� �� ��ģ ����
	//InterpolationModeHighQualityBilinear	: �ε巴�� resize������ �ణ �ѿ��� ����
	//InterpolationModeHighQualityBicubic	: �ӵ��� �������� �ְ� ǰ�� ���
	void resize(int cx, int cy, Gdiplus::InterpolationMode mode = Gdiplus::InterpolationModeHighQualityBicubic);
	void resize(float fx, float fy, Gdiplus::InterpolationMode mode = Gdiplus::InterpolationModeHighQualityBicubic);
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
	//Ư�� ��ġ�� �����̳� Ư�������� ���ο� �������� �����Ѵ�.
	void replace_color(int tx, int ty, Gdiplus::Color dst);
	void replace_color(Gdiplus::Color src, Gdiplus::Color dst);

	//���� �̹����� �������� ���̹Ƿ� ��� ������ ���̴�.
	//������ �����ϴ� ���� �����̳� ���� ������ �ʿ��ϴ�.
	void add_rgb(int red, int green, int blue);
	void add_rgb_loop(int red, int green, int blue, COLORREF crExcept);

	//hue : -180 ~ 180, sat : -100 ~ 100, light : -100 ~ 100
	void apply_effect_hsl(int hue, int sat = 0, int light = 0);
	void apply_effect_rgba(float r, float g, float b, float a = 1.0);
	void apply_effect_blur(float radius, BOOL expandEdge);

	void round_shadow_rect(int w, int h, float radius);

	//factor(0.01~0.99)		: �������� ������ ������� ����. 0.0�̸� blur ����.
	//position(0.01~0.99)	: �ٱ����� �������� ������� ����. 0.0�̸� blur ����.
	//�Ϲ����� �׵θ� �� ȿ���� 0.95f, 0.10f�� �ٰ�
	void round_corner(float radius, float factor = 0.0f, float position = 0.0f);

	//src�� src_bgra_index�� �ش��ϴ� ä�ΰ���(bgra���� n�� ä�� �ε���)
	//dst�� dst_bgra_index�� �ش��ϴ� ä�ΰ����� ����
	//ex. src�� ����ũ �̹�����, dst�� ���� �̹����� �����ϰ� index�� 3, 3���� �ϸ�
	//src�� alpha���� dst�� alpha������ �����Ѵ�.
	//�̸� �̿��Ͽ� mask overlay�� Ȱ���� �� �ִ�.
	void replace_channel(Bitmap* src, Bitmap* dst, int src_bgra_index, int dst_bgra_index);
	void replace_channel(Bitmap* src, int src_bgra_index, int dst_bgra_index);
	void replace_channel(CString type, UINT srcID, int src_bgra_index, int dst_bgra_index);
	void replace_channel(CString src_file, int src_bgra_index, int dst_bgra_index);
	PathGradientBrush* createFluffyBrush(GraphicsPath* gp, float* blendFactors, float* blendPositions, INT count, INT* in_out_count);



	//ColorMatrix�� �̿��Ͽ� ������ ����̹����� ���� �� ������
	//�װ� 3ä���� ������� �̹����̹Ƿ� 1ä�� 256 gray�̹����� �ƴϴ�.
	//���� resource���� �о���� �̹����� ����� ��ȯ���� �ʴ� ���� ����(1/3�� �ε��Ǵ� ����)
	void convert2gray();
	//�̱���
	void cvtColor(PixelFormat old_format, PixelFormat new_format);
	//���� �̹����� 32bit�� �����Ѵ�.
	void cvtColor32ARGB();

	bool save(CString filepath);
	bool copy_to_clipbard();

	int width = 0;
	int height = 0;
	int channel = 0;
	int stride = 0;

	//animatedGif
	UINT	m_frame_count;
	UINT	m_frame_index;
	PropertyItem* m_pPropertyItem = NULL;
	bool	is_animated_gif() { return (m_frame_count > 1); }
	int		get_frame_count() { return m_frame_count; }
	//parenthWnd ���� ������ ������ ǥ��. ����ȿ���� �������� �ʴ´�.
	//parent�� ������� �����ϰ� ǥ���� ���� CImageShapeWnd�� ���.
	void	set_animation(HWND parenthWnd, int x = 0, int y = 0, int w = 0, int h = 0, bool start = true);
	void	back_color(COLORREF cr) { m_crBack.SetFromCOLORREF(cr); }
	void	back_color(Gdiplus::Color cr) { m_crBack = cr; }
	void	start_animation();
	void	pause_animation();
	void	stop_animation();
	//gif ������ �̹������� ������ ������ ����
	bool	save_gif_frames(CString folder);
	//gif ������ �̹������� �����ؼ� ���� display�� �� �ִ�.
	//��, ��� �̹����� �����Ͽ� �޸𸮿� �ε��ϹǷ� �޸� ��뷮�� ������ �� �ִ�.
	void	get_gif_frames(std::vector<Gdiplus::Bitmap*>& dqBitmap, std::vector<long> &dqDelay);

	//�� ����ð��� ms������ �����Ѵ�.
	int		get_total_duration();

	int		ani_width() { return m_aniWidth; }
	int		ani_height() { return m_aniHeight; }
	int		ani_width(int dpwidth) { m_aniWidth = dpwidth; return m_aniWidth; }
	int		ani_height(int dpheight) { m_aniHeight = dpheight; return m_aniHeight; }

	void	save_multi_image();// std::vector<Gdiplus::Bitmap*>& dqBitmap);

protected:
	CString			m_filename = _T("untitled");

	void	resolution();
	Bitmap* GetImageFromResource(CString lpType, UINT id);

	//animatedGif
	bool			m_bIsInitialized;
	bool			m_paused = false;
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
