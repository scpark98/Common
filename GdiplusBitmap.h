#pragma once

/*
scpark.
���� CGdiPlusBitmap �� CGdiPlusBitmapResource�� CGdiplusBitmap�̶�� �ϳ��� Ŭ������ ��ġ��
Gdiplus���� �����ϴ� �پ��� �̹��� ȿ���� �߰���.

* Gdiplus�� �ڵ� �ʱ�ȭ, �����ϱ� ���� CGdiplusDummyForInitialization Ŭ������ �߰��ϰ�
  GdiplusBitmap.cpp�� �� ������ static �ν��Ͻ��� ������.


- 20221217. animatedGif �߰�
	//�ٸ� �̹��� ���˰��� �޸� ���� �����忡�� parent�� DC�� ���÷��� ��.

	//����� include �� ������� ����
	#include "../../Common/GdiplusBitmap.h"
	...
	CGdiplusBitmap m_gif;

	//�ܺ� ���� �ε� ���
	m_gif.load(_T("D:\\media\\test_image\\test.gif"));

	//���ҽ� �ε� ���. �ݵ�� .gif�� .bin�� ���� �ٸ� �̸����� ���� �� ���ҽ� �߰��� ��.
	//���ҽ� ID�տ� UINT�� ����ؾ� load()�Լ��� ��ȣ���� ��������.
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

static const UINT Message_CGdiplusBitmap = ::RegisterWindowMessage(_T("MessageString_CGdiplusBitmap"));

//Gdiplus �ʱ�ȭ ������ �ڵ����� �ϱ� ���� CGdiplusDummyForInitialization Ŭ������ �����ϰ�
//GdiplusBitmap.cpp�� �� ������ static �ν��Ͻ��� ������.
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

	//CTreeCtrl, CListCtrl��� ���õ� �׸� ��ü�� �̹����� ����(drag�ÿ� ���)
	void	create_drag_image(CWnd* pWnd);

	void	release();

	//CGdiplusBitmap�� CGdiplusBitmapResource �� ���� Ŭ������ �־����� ������.
	bool	load(CString sFile, bool show_error = false);
	//���� ���ÿ� ���� ���ҽ� ID�տ� UINT�� ����ؾ� load()�Լ��� ��ȣ���� ��������.
	//m_gif.load(_T("GIF"), UINT(IDR_GIF_CAT_LOADING));
	bool	load(CString sType, UINT id, bool show_error = false);
	//png�� ���� sType�� ������ �� �ִ�.
	bool	load(UINT id, bool show_error = false);

	Gdiplus::Bitmap* CreateARGBBitmapFromDIB(const DIBSECTION& dib);


	operator Gdiplus::Bitmap*() const { return m_pBitmap; }

//image pixel data�� ����ϰ��� �� ���
	//�⺻�����δ� �̹��� raw data�� �������� �ʴ´�.
	//cv::Mat�� dataó�� raw data�� �ʿ��� ��쿡 �� �Լ��� ȣ���ϸ� ����� ����������.
	bool		get_raw_data();
	//data ���� ������ �� �ٽ� �̹����� �����Ű���� �ݵ�� set_raw_data()�� ȣ���ؾ� �����ȴ�.
	bool		set_raw_data();
	uint8_t*	data = NULL;


	//m_pBitmap�� ��ȿ�ϰ�, width, height ��� 0���� Ŀ�� �Ѵ�.
	bool	is_empty();
	bool	is_valid();
	int		channels();
	CSize	size() { return CSize(width, height); }

	//targetRect�� �ָ� ��� ������ ������ �����Ͽ� �׸���.
	//targetRect�� NULL�̸� 0,0�� �̹��� ũ���� �׸���.
	//draw�ÿ� CDC�� �ѱ���� Gdiplus::Graphics�� �ѱ���� ��������� Gdiplus::Graphics�� �����ڵ尡 �����ä�� ����ϴ� ��찡 �����Ƿ�
	//Gdiplus::Graphics�� �ѱ�°� �´ٰ� �Ǵ���.
	//draw_mode = draw_mode_zoom(maintain ratio, resize to fit),
	//draw_mode = draw_mode_stretch,
	//draw_mode = draw_mode_origin(as image original 1:1 size),
	CRect	draw(Gdiplus::Graphics& g, CRect targetRect, int draw_mode = draw_mode_zoom);
	CRect	draw(Gdiplus::Graphics& g, int dx = 0, int dy = 0, int dw = 0, int dh = 0);
	CRect	draw(Gdiplus::Graphics& g, CGdiplusBitmap mask, CRect targetRect);

	//bmp �̹����� ���� �̹����� targetRect�� �׸���.
	CRect	draw(CGdiplusBitmap *bmp, CRect* targetRect = NULL);

	//�� ���ʴ� 3state button�� ���ʿ� �����ϰ� ������.
	//BST_UNCHECKED, BST_CHECKED, BST_INDETERMINATE
	enum GDIP_DRAW_MODE
	{
		draw_mode_origin = 0,	//targetRect�� �߾ӿ� ���� ũ��� �׸�
		draw_mode_stretch,		//targetRect�� ������ �׸�(non ratio)
		draw_mode_zoom,			//targetRect�� ratio�� �����Ͽ� �׸�(���� �Ǵ� ���ο� ����)
	};

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

	//�̹����� ���� �ؽ�Ʈ�� �߰�
	CRect		draw_text(	int x, int y, int w, int h,
							CString text,
							float font_size,
							int font_style = Gdiplus::FontStyleRegular,
							int shadow_depth = 0,
							float thickness = 0.0f,
							CString font_name = _T("���� ���"),
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
							CString font_name = _T("���� ���"),
							Gdiplus::Color cr_text = Gdiplus::Color::Black,
							Gdiplus::Color cr_stroke = Gdiplus::Color::LightGray,
							Gdiplus::Color cr_shadow = Gdiplus::Color::DarkGray,
							Gdiplus::Color cr_back = Gdiplus::Color::Transparent,
							UINT align = DT_CENTER | DT_VCENTER);

	//InterpolationModeNearestNeighbor		: ���� ȭ�Ҹ� ���� ���������� �Ϻ� ȭ�Ҵ� �����. �׷��� �� ��ģ ����
	//InterpolationModeHighQualityBilinear	: �ε巴�� resize������ �ణ �ѿ��� ����
	//InterpolationModeHighQualityBicubic	: �ӵ��� �������� �ְ� ǰ�� ���
	//cx �Ǵ� cy�� 0�� ���� ����/���� ������ �����Ͽ� resize�ȴ�.
	void	resize(int cx, int cy, Gdiplus::InterpolationMode mode = Gdiplus::InterpolationModeHighQualityBicubic);
	void	resize(float fx, float fy, Gdiplus::InterpolationMode mode = Gdiplus::InterpolationModeHighQualityBicubic);

	//�̹��� ĵ���� ũ�⸦ �����Ѵ�. ���� ������ cr_fill�� ä���. cr_fill�� ������ �ƴ� ��� ������ ��.
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

	//Ư�� ��ġ�� �����̳� Ư�������� ���ο� �������� �����Ѵ�.
	void	replace_color(int tx, int ty, Gdiplus::Color dst);
	void	replace_color(Gdiplus::Color src, Gdiplus::Color dst = Gdiplus::Color::Transparent);

	//���� png�� ������ �����Ѵ�. undo�� �������� �ʴ´�.
	void	set_back_color(Gdiplus::Color cr_back);

	//������ ������ ä���.
	void	clear(Gdiplus::Color cr);

	//���� �̹����� �������� ���̹Ƿ� ��� ������ ���̴�.
	//������ �����ϴ� ���� �����̳� ���� ������ �ʿ��ϴ�.
	void	add_rgb(int red, int green, int blue);
	//void add_rgb_loop(int red, int green, int blue, COLORREF crExcept);

//����
	void	adjust_bright(int bright);
	//contrast = 255�̸� ������ ����.
	void	adjust_contrast(int contrast);

	//hue : -180 ~ 180, sat : -100 ~ 100, light : -100 ~ 100
	void	adjust_hsl(int hue, int sat = 0, int light = 0);
	void	adjust_rgba(float r, float g, float b, float a = 1.0);

//ȿ��
	//https://github.com/bfraboni/FastGaussianBlur
	//sigma = Gaussian standard deviation (float). Should be positive.
	//order: optional filter order [1: box, 2: bilinear, 3: biquadratic, 4. bicubic, ..., 10]. should be positive. Default is 3
	//border: optional treatment of image boundaries[mirror, extend, crop, wrap].Default is mirror.
	void	blur(float sigma = 10.0f, int order = 1/*, int border = 2*/);
	//Gdiplus���� �����ϴ� GaussianBlur ��������� radius�� ���� ���� �¿�θ� ������� ���� ������ �ִ�.
	void	gdip_blur(float radius, BOOL expandEdge);

	//round shadow rect �̹����� ����
	void	round_shadow_rect(int w, int h, float radius, float blur_sigma = 5.0f, Gdiplus::Color cr_shadow = Gdiplus::Color::Gray);

	void	transformBits();


	//ȸ�������� ���游 �� �� ���� �ȼ������� ������� �ʴ´�.
//weight�� 1.0f�̸� original gray�̹����� ��������� 1.0f���� ũ�� ������� ������ ��ο���.
	void	gray(float weight = 1.0f);
	//rgb ��� -1.0f�̸� black&white�� �ǰ� red�� 1.0f��� red&white�� ���� ȿ���� ����ȴ�.
	void	black_and_white(float red = -1.0f, float green = -1.0f, float blue = -1.0f);
	void	negative();
	void	sepia();
	void	polaroid();
	void	rgb_to_bgr();

	//factor(0.01~0.99)		: �������� ������ ������� ����. 0.0�̸� blur ����.
	//position(0.01~0.99)	: �ٱ����� �������� ������� ����. 0.0�̸� blur ����.
	//�Ϲ����� �׵θ� �� ȿ���� 0.95f, 0.10f�� �ٰ�
	//4���� �ڳʸ� ���� ó�� �Ұ������� �ɼ����� �� �� �ִ�.
	void	round_corner(float radius, float factor = 0.0f, float position = 0.0f, bool tl = true, bool tr = true, bool br = true, bool bl = true);

	//src�� src_bgra_index�� �ش��ϴ� ä�ΰ���(bgra���� n�� ä�� �ε���)
	//dst�� dst_bgra_index�� �ش��ϴ� ä�ΰ����� ����
	//ex. src�� ����ũ �̹�����, dst�� ���� �̹����� �����ϰ� index�� 3, 3���� �ϸ�
	//src�� alpha���� dst�� alpha������ �����Ѵ�.
	//�̸� �̿��Ͽ� mask overlay�� Ȱ���� �� �ִ�.
	void replace_channel(Gdiplus::Bitmap* src, Gdiplus::Bitmap* dst, int src_bgra_index, int dst_bgra_index);
	void replace_channel(Gdiplus::Bitmap* src, int src_bgra_index, int dst_bgra_index);
	void replace_channel(CString type, UINT srcID, int src_bgra_index, int dst_bgra_index);
	void replace_channel(CString src_file, int src_bgra_index, int dst_bgra_index);
	Gdiplus::PathGradientBrush* createFluffyBrush(Gdiplus::GraphicsPath* gp, float* blendFactors, float* blendPositions, INT count, INT* in_out_count);



	//ColorMatrix�� �̿��Ͽ� ������ ����̹����� ���� �� ������
	//�װ� 3ä���� ������� �̹����̹Ƿ� 1ä�� 256 gray�̹����� �ƴϴ�.
	//���� resource���� �о���� �̹����� ����� ��ȯ���� �ʴ� ���� ����(1/3�� �ε��Ǵ� ����)
	void convert2gray();
	//�̱���
	void cvtColor(Gdiplus::PixelFormat old_format, Gdiplus::PixelFormat new_format);
	//���� �̹����� 32bit�� �����Ѵ�.
	void cvtColor32ARGB();

	bool save(LPCTSTR filepath, ...);
	bool copy_to_clipbard();
	bool paste_from_clipboard();

	int width = 0;
	int height = 0;
	int channel = 0;
	int stride = 0;

//animated Gif ����
	int		m_frame_count;
	int		m_frame_index;
	Gdiplus::PropertyItem* m_pPropertyItem = NULL;
	bool	is_animated_gif() { return (is_valid() && (m_frame_count > 1)); }
	int		get_frame_count() { return m_frame_count; }
	//parenthWnd ���� ������ ������ ǥ��. ����ȿ���� �������� �ʴ´�.
	//parent�� ������� �����ϰ� ǥ���� ���� CImageShapeWnd�� ���.
	void	set_animation(HWND parenthWnd, int x = 0, int y = 0, int w = 0, int h = 0, bool auto_play = true);
	//void	set_animation(HWND parenthWnd, int x, int y, float ratio = 1.0f, bool start = true);
	//ratio�� �����Ͽ� r�ȿ� ǥ���Ѵ�.
	void	set_animation(HWND parenthWnd, CRect r, bool start = true);
	void	move(int x = 0, int y = 0, int w = 0, int h = 0);
	void	move(CRect r);
	void	gif_back_color(COLORREF cr) { m_cr_back.SetFromCOLORREF(cr); }
	void	gif_back_color(Gdiplus::Color cr) { m_cr_back = cr; }
	void	play_animation();
	//pos��ġ�� �̵��� �� �Ͻ������Ѵ�. -1�̸� pause <-> play�� ����Ѵ�.
	void	pause_animation(int pos = 0);
	//animation thread�� ����ǰ� ȭ�鿡�� �� �̻� ǥ�õ��� �ʴ´�. ���� �״�� ���߱� ���Ѵٸ� pause_animation()�� ȣ���Ѵ�.
	void	stop_animation();
	void	goto_frame(int pos, bool pause = false);			//���� ���������� �̵�
	void	goto_frame_percent(int pos, bool pause = false);	//���� % ��ġ�� ���������� �̵�
	void	set_mirror(bool is_mirror = true) { m_is_gif_mirror = is_mirror; }

	//gif ������ �̹������� ������ ������ ����
	bool	save_gif_frames(CString folder);
	//gif ������ �̹������� �����ؼ� ���� display�� �� �ִ�.
	//��, ��� �̹����� �����Ͽ� �޸𸮿� �ε��ϹǷ� �޸� ��뷮�� ������ �� �ִ�.
	void	get_gif_frames(std::vector<Gdiplus::Bitmap*>& dqBitmap, std::vector<long> &dqDelay);

	//�� ����ð��� ms������ �����Ѵ�.
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

	//ani gif ǥ�� ������
	HWND			m_target_hwnd;

	//ani ���� x ��ǥ
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
