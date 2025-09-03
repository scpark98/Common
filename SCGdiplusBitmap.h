#pragma once

/*
scpark.
���� CSCGdiPlusBitmap �� CGdiPlusBitmapResource�� CSCGdiplusBitmap�̶�� �ϳ��� Ŭ������ ��ġ��
Gdiplus���� �����ϴ� �پ��� �̹��� ȿ���� �߰���.

* Gdiplus�� �ڵ� �ʱ�ȭ, �����ϱ� ���� CGdiplusDummyForInitialization Ŭ������ �߰��ϰ�
  GdiplusBitmap.cpp�� �� ������ static �ν��Ͻ��� ������.


- 20221217. animatedGif �߰�
	//�ٸ� �̹��� ���˰��� �޸� ���� �����忡�� parent�� DC�� ���÷��� ��.
	//������ ������ ������ parent DC�� ���÷����ϸ鼭 parent�� �ٸ� child��� ������ �߻��ϹǷ�
	//CSCGdiplusBitmap���� ���÷��� �� ��
	//parent���� ���� ���÷��� �� ���� �ɼ����� ó���ؾ� �Ѵ�.

	//����� include �� ������� ����
	#include "Common/GdiplusBitmap.h"
	...
	CSCGdiplusBitmap m_gif;

	//�ܺ� ���� �ε� ���
	m_gif.load(_T("D:\\media\\test_image\\test.gif"));

	//���ҽ� �ε� ���. �ݵ�� .gif�� .bin�� ���� �ٸ� �̸����� ���� �� ���ҽ� �߰��� ��.
	//���ҽ� ID�տ� UINT�� ����ؾ� load()�Լ��� ��ȣ���� ��������.
	m_gif.load(_T("GIF"), UINT(IDR_GIF_CAT_LOADING));

	//�ε� �� parent�� DC �� ��ǥ�� �Ѱ��ָ� �ڵ� �����.
	m_gif.set_animation(m_hWnd);// , 0, 0, 150, 130, true);

	//save_gif_frames()�� �̿��Ͽ� �� �������� ���� ����.

	=> Ư�� �����Ӹ� ǥ��, ���� ǥ�� ����� ���Ǽ��� ����� �� CStatic�� ��ӹ޾� ������ ����

[20250902]
- ȸ���� �̹����� ��� �̸� �����Ͽ� �ڵ� ȸ���ǵ��� ����.
- �� �� Gdiplus::PropertyItem*�� �������Ƿ� �̸� 

[20250609]
- animated gif ������ �и�
	CSCGdiplusBitmap���� ���� gif�� ����ϸ� �������� parent�� �ٸ� child��� ���� �߻� �� ������ ���� �̸� �ɼ����� ó����.
	load("test.gif", true)�� ȣ���ϸ� CSCGdiplusBitmap���� �ڵ� ����ǰ�
	load("test.gif", false)�� ȣ���ϸ� load�ϴ� �����쿡�� ���� ����ϰ� �ȴ�.

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
#include <memory>
#include <cmath>

static const UINT Message_CSCGdiplusBitmap = ::RegisterWindowMessage(_T("MessageString_CSCGdiplusBitmap"));

//������ ���� TOP, LEFT�� 0�̹Ƿ� ���� ���ϸ� �ȵǰ� CENTER, RIGHT���� �˻��� �� �ƴϸ� LEFT��� �Ǵ��ؾ� �Ѵ�.
//align = ALIGN_LEFT | ALIGN_BOTTOM �� �ָ� 8�� �Ǵµ� �̷��� ȣ���ϸ�
//if (align & ALIGN_LEFT) //�� �� ����� 0�� �ǹǷ� ������� �ʴ´�.
//=> CSCGdiplusBitmap::canvas_size()�� ȣ���� �� �̹��� ��ġ�� ������ �� ����ϱ� ���� ����������
//�̹� afxbutton.h�� ���ǵǾ� �ߺ��ȴ�. DrawText()���� ���Ǵ� align�� �׳� �̿��Ѵ�.
//#define ALIGN_TOP        0x00000000
//#define ALIGN_LEFT       0x00000000
//#define ALIGN_CENTER     0x00000001
//#define ALIGN_RIGHT      0x00000002
//#define ALIGN_VCENTER    0x00000004
//#define ALIGN_BOTTOM     0x00000008

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
		res.Format(_T("ī�޶� ������: %s\nī�޶� �𵨸�: %s\n����Ʈ����: %s\n�Կ� �ð�: %s\n�÷���: %s\n���� �Ÿ�: %.1f mm\n35mm ȯ��: %.1f\n")\
			_T("���� �ð� : 1/%d sec\n���� ����: %.2f EV\n������ ��: f/%.1f\nISO ����: %d\nȸ�� ����: %s\nGPS ����: N %s, E %s, %.0fm"),
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
	//sType�� "png", "jpg", "gif"�� ��ǥ������ �����ϸ� "tiff", "webp" �� �ٸ� ���˵� ���������� �켱 ������.
	//"ico" ������ ũ�� �Ķ���͵� �߿��ϹǷ� load_icon()�� ����ϵ��� �Ѵ�.
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

	//CTreeCtrl, CListCtrl��� ���õ� �׸� ��ü�� �̹����� ����(drag�ÿ� ���)
	void			create_drag_image(CWnd* pWnd);

	void			release();

	//CSCGdiplusBitmap�� CSCGdiplusBitmapResource �� ���� Ŭ������ �־����� ������.
	//�ܺ� �̹��� ���� �ε�
	bool			load(CString sfile);
	//���� ���ÿ� ���� ���ҽ� ID�տ� UINT�� ����ؾ� load()�Լ��� ��ȣ���� ��������.
	//m_gif.load(_T("GIF"), UINT(IDR_GIF_CAT_LOADING));
	bool			load(CString sType, UINT id);
	//png�� ���� sType�� ������ �� �ִ�.
	bool			load(UINT id);
	bool			load_icon(UINT id, int size = 32);
	bool			load_webp(CString sfile);

	CString			get_filename(bool fullpath = true);

	//animated gif�� load�� ��� ����� CSCGdiplusBitmap���� ��ü������ �ϴ���(�⺻��),
	//load�ϴ� ���� Ŭ�������� ���� thread�� ����ϴ��ĸ� ����
	//�Ϲ������δ� Ư�� dlg���� gif�� �ε��Ͽ� CSCGdiplusBitmap ��ü���� �ٷ� ����ϵ��� �ϸ� ��������
	//ASee.exe�� ���� roi ����, ��Ÿ child ctrl���� �����Ͽ� �׵���� ������ ������ �� ���� CSCGdiplusBitmap���� �����Ű�� �ʰ�
	//CSCImageDlg���� ���� ������Ѿ� �ϴ� ��쵵 �ִ�.
	void			set_gif_play_itself(bool play_itself = true) { m_gif_play_in_this = play_itself; }
	bool			is_gif_play_itself() { return m_gif_play_in_this; }

	Gdiplus::Bitmap* CreateARGBBitmapFromDIB(const DIBSECTION& dib);


	operator Gdiplus::Bitmap*() const { return m_pBitmap; }

//image pixel data�� ����ϰ��� �� ���
	//�⺻�����δ� �̹��� raw data�� �������� �ʴ´�.
	//cv::Mat�� dataó�� raw data�� �ʿ��� ��쿡 �� �Լ��� ȣ���ϸ� ����� ����������.
	bool			get_raw_data();
	//data ���� ������ �� �ٽ� �̹����� �����Ű���� �ݵ�� set_raw_data()�� ȣ���ؾ� �����ȴ�.
	bool			set_raw_data();
	uint8_t*		data = NULL;

//palette
	Gdiplus::ColorPalette* m_palette = NULL;
	bool			get_palette();
	//8bit indexed �̹����� �ȷ�Ʈ ���� ����. ���� �ε� �� 1ȸ�� �ؾ� �Ѵ�.
	bool			m_palette_adjusted = false;

	//m_pBitmap�� ��ȿ�ϰ�, width, height ��� 0���� Ŀ�� �Ѵ�.
	bool			is_empty();
	bool			is_valid();
	int				get_channel();
	CSize			size() { return CSize(width, height); }

	//�̹��� ������ ���� width/height���� �ݴ� ������ ���� ���� wh = false�� �ش�.
	float			get_ratio(bool wh = true);

	//alpha �ȼ��� ������ �̹������� �Ǻ�.
	//-1 : ���� �Ǻ����� ���� �����̹Ƿ� �Ǻ� ����
	// 0 : 3ä���̰ų� 4ä���� ��� �ȼ��� alpha = 255�� ���
	// 1 : �� ���̶� alpha < 255�� ���
	int				has_alpha_pixel();

	//PixelFormat24bppRGB�� ���� ���ǵ� ���� ���ڿ��� �����ϸ� simple = true�� ���� "RGB (24bit)"�� ���� �����Ѵ�.
	//fmt�� �־����� ������ ���� �̹����� PixelFormat�� ���Ͽ� ����� �����Ѵ�.
	//�ѹ� ���� �Ŀ��� m_pixel_format_str ������ ����ǰ� �̸� ���������� �ٽ� ���ؾ� �� ���� reset = true�� ȣ���Ѵ�.
	CString			get_pixel_format_str(Gdiplus::PixelFormat fmt = -1, bool simple = true, bool reset = false);


	//�� ���� �����ϴ� ������� ���� �귯���� �����Ѵ�.
	//sz_tile : 32�� �ָ� 16x16 ũ���� Ÿ�Ϸ� �ݺ��Ǵ� ������� ������ �����ȴ�. ��, 16x16 zigzag �� �� 4���� 32x32 ũ���� Ÿ�Ϸ� �ݺ���.
	static std::unique_ptr<Gdiplus::TextureBrush> get_zigzag_pattern(int sz_tile = 32, Gdiplus::Color cr_back = Gdiplus::Color::White, Gdiplus::Color cr_fore = Gdiplus::Color(200, 200, 200));
	static Gdiplus::Color m_cr_zigzag_back;
	static Gdiplus::Color m_cr_zigzag_fore;

	//targetRect�� �ָ� ��� ������ ������ �����Ͽ� �׸���.
	//targetRect�� NULL�̸� 0,0�� �̹��� ũ���� �׸���.
	//draw�ÿ� CDC�� �ѱ���� Gdiplus::Graphics�� �ѱ���� ��������� Gdiplus::Graphics�� �����ڵ尡 �����ä�� ����ϴ� ��찡 �����Ƿ�
	//Gdiplus::Graphics�� �ѱ�°� �´ٰ� �Ǵ���.
	//��Ȥ Gdiplus::Graphics�� ������� �ʰ� ������ dc���� �׸��� ��쵵 �����ϹǷ� ��� �����ؾ� �Ѵ�.
	//draw_mode = draw_mode_zoom(maintain ratio, resize to fit),
	//draw_mode = draw_mode_stretch,
	//draw_mode = draw_mode_origin(as image original 1:1 size),
	CRect			draw(Gdiplus::Graphics& g, CRect targetRect, int draw_mode = draw_mode_zoom);
	CRect			draw(Gdiplus::Graphics& g, int dx = 0, int dy = 0, int dw = 0, int dh = 0);
	CRect			draw(Gdiplus::Graphics& g, CSCGdiplusBitmap mask, CRect targetRect);
	CRect			draw(CDC* pDC, CRect targetRect, int draw_mode = draw_mode_zoom);
	CRect			draw(CDC* pDC, int dx = 0, int dy = 0, int dw = 0, int dh = 0);

	//CSCGdiplusBitmap �̹����� ���� �̹����� targetRect�� �׸���.
	CRect			draw(CSCGdiplusBitmap *img, CRect* targetRect = NULL);

	//�׸��� �׸��� �ʰ� ǥ�õ� ���� ������ ��´�.
	CRect			calc_rect(CRect targetRect, int draw_mode = draw_mode_zoom);

	//�� ���ʴ� 3state button�� ���ʿ� �����ϰ� ������.
	//BST_UNCHECKED, BST_CHECKED, BST_INDETERMINATE
	enum GDIP_DRAW_MODE
	{
		draw_mode_origin = 0,	//targetRect�� �߾ӿ� ���� ũ��� �׸�
		draw_mode_stretch,		//targetRect�� ������ �׸�(non ratio)
		draw_mode_zoom,			//targetRect�� ratio�� �����Ͽ� �׸�(���� �Ǵ� ���ο� ����)
	};

	//Gdiplus::Bitmap::Clone�� shallow copy�̹Ƿ� ������ ���縦 ���ؼ��� deep_copy�� ����ؾ� �Ѵ�.
	void			clone(CSCGdiplusBitmap* dst);
	void			deep_copy(CSCGdiplusBitmap* dst);
	static void		deep_copy(Gdiplus::Bitmap** dst, Gdiplus::Bitmap* src);
	void			rotate(Gdiplus::RotateFlipType type);
	//ȸ����Ű�� w, h�� �޶����Ƿ� �̹����� ũ�⸦ ��������߸� �ϴ� ��쵵 �ִ�.
	//�׷� ���� auto_resize�� true�� �ְ� ���ʿ��� ����� ������ ����
	//���ʿ��� ����� ������ �����Ͽ� �̹��� ũ�⸦ fit�ϰ� ���ϼ��� �ִ�.
	void			rotate(float degree, bool auto_resize = false, Gdiplus::Color remove_back_color = Gdiplus::Color::Transparent);
	//�¿��Ī
	void			mirror();
	//���ϴ�Ī
	void			flip();

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
	enum INTERPOLATION_TYPE
	{
		interpolation_none = Gdiplus::InterpolationModeNearestNeighbor,
		interpolation_bilinear = Gdiplus::InterpolationModeHighQualityBilinear,
		interpolation_bicubic = Gdiplus::InterpolationModeHighQualityBicubic,
		interpolation_lanczos,
	};

	//cx �Ǵ� cy�� 0�� ���� ����/���� ������ �����Ͽ� resize�ȴ�.
	void			resize(int cx, int cy, Gdiplus::InterpolationMode mode = Gdiplus::InterpolationModeHighQualityBicubic);
	void			resize(float fx, float fy, Gdiplus::InterpolationMode mode = Gdiplus::InterpolationModeHighQualityBicubic);

	//�̹��� ĵ���� ũ�⸦ �����Ѵ�. ���� ������ cr_fill�� ä���. cr_fill�� ������ �ƴ� ��� ������ ��.
	//align ������� DT_CENTER | DT_VCENTER�� ���� DrawText()���� ����ϴ� align ������� ���� ����Ѵ�.
	void			canvas_size(int cx, int cy, int align = DT_CENTER | DT_VCENTER, Gdiplus::Color cr_fill = Gdiplus::Color::Transparent);

	//���� png�� l, t, r, b�� ������ ���� ũ�⸦ ���Ѵ�.
	CRect			get_transparent_rect();

	//���� �̹����� �Ϻκ��� �� ��ü �̹����� �缳���Ѵ�.
	//��, ���� �̹����� ���� ��ü���� sub_image()�� ȣ���ϸ� �� ��ü�� crop�� �̹����� ����ȴ�.
	//���� A�̹����� r ������ B�̹��� �Ҵ��ϰ��� �� ���� ���� �� �ܰ踦 ��ģ��.
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

	//Gdiplus::Bitmap::GetPixel()�̹Ƿ� data�� �������� �ʰ� ���� �� ������ �ӵ��� ����
	Gdiplus::Color	get_color(int x, int y);

	//data�� �����Ͽ� �ּҰ����� ���ϹǷ� �ӵ��� ����. get_raw_data()�� ȣ���Ͽ� data�� ������ ��쿡�� ����� ��!
	Gdiplus::Color	get_pixel(int x, int y);
	void			set_pixel(int x, int y, Gdiplus::Color cr);


	//Ư�� ��ġ�� �����̳� Ư�������� ���ο� �������� �����Ѵ�.
	void			replace_color(int tx, int ty, Gdiplus::Color dst);
	void			replace_color(Gdiplus::Color src, Gdiplus::Color dst = Gdiplus::Color::Transparent);

	//���� png�� ������ �����Ѵ�. undo�� �������� �ʴ´�.
	void			set_back_color(Gdiplus::Color cr_back);

	//������ ������ ä���.
	void			clear(Gdiplus::Color cr);

	//���� �̹����� �������� ���̹Ƿ� ��� ������ ���̴�.
	//������ �����ϴ� ���� �����̳� ���� ������ �ʿ��ϴ�.
	void			add_rgb(int red, int green, int blue);
	//void add_rgb_loop(int red, int green, int blue, COLORREF crExcept);

//����
	//bright, constrast ���� ������ ��� ������ ��������, ���� �̹����� ���������� ���� ��å�� ���ؾ� �Ѵ�.
	//1.������ ������ ���
	//	- m_origin�� �׻� �����ϹǷ� �޸𸮰� ����� �� �ִ�.
	//	- �� ó�� �̹��� ������ �Ͼ�� ��쿡�� �����Ҵ����ָ� ���� �ּ�ȭ �� �� �ִ�.
	//	- bright, contrast, hsl, rgba �� adjust�� ���� ��쿡�� �̷��� ó���� �ϵ��� �Ѵ�.
	//2.���� �̹����� ������ ���
	//	- adjust_bright(255)�� �ϰ� adjust_bright(-255)�� �ص� ������� ���ƿ� �� ����. ������ ���� 1ȸ�����θ� ���� �����ϴ�.
	//bright = 0% ~ 400% ������ percentage���̸� 100%�� ������ �����ϴ�.
	int				adjust_bright(int bright, bool adjust_others = true);
	//contrast = 0% ~ 400% ������ percentage���̸� 100%�� ������ �����ϴ�.
	int				adjust_contrast(int contrast, bool adjust_others = true);

	int				get_bright() { return m_bright; }
	int				get_contrast() { return m_contrast; }

	int				increase_bright(int interval);
	int				increase_contrast(int interval);

	void			reset_adjust();

	//hue : -180 ~ 180, sat : -100 ~ 100, light : -100 ~ 100
	void			adjust_hsl(int hue, int sat = 0, int light = 0);
	void			adjust_rgba(float r, float g, float b, float a = 1.0);

//ȿ��
	//https://github.com/bfraboni/FastGaussianBlur
	//sigma = Gaussian standard deviation (float). Should be positive.
	//order: optional filter order [1: box, 2: bilinear, 3: biquadratic, 4. bicubic, ..., 10]. should be positive. Default is 3
	//border: optional treatment of image boundaries[mirror, extend, crop, wrap].Default is mirror.
	void			blur(float sigma = 10.0f, int order = 1/*, int border = 2*/);
	//Gdiplus���� �����ϴ� GaussianBlur ��������� radius�� ���� ���� �¿�θ� ������� ���� ������ �ִ�.
	void			gdip_blur(float radius, BOOL expandEdge);

	//round shadow rect �̹����� ����
	void			round_shadow_rect(int w, int h, float radius, float blur_sigma = 5.0f, Gdiplus::Color cr_shadow = Gdiplus::Color::Gray);

	void			transformBits();


	//ȸ�������� ���游 �� �� ���� �ȼ������� ������� �ʴ´�.
//weight�� 1.0f�̸� original gray�̹����� ��������� 1.0f���� ũ�� ������� ������ ��ο���.
	void			gray(float weight = 1.0f);
	//rgb ��� -1.0f�̸� black&white�� �ǰ� red�� 1.0f��� red&white�� ���� ȿ���� ����ȴ�.
	void			black_and_white(float red = -1.0f, float green = -1.0f, float blue = -1.0f);
	void			negative();
	void			sepia();
	void			polaroid();
	void			rgb_to_bgr();

	//factor(0.01~0.99)		: �������� ������ ������� ����. 0.0�̸� blur ����.
	//position(0.01~0.99)	: �ٱ����� �������� ������� ����. 0.0�̸� blur ����.
	//�Ϲ����� �׵θ� �� ȿ���� 0.95f, 0.10f�� �ٰ�
	//4���� �ڳʸ� ���� ó�� �Ұ������� �ɼ����� �� �� �ִ�.
	void			round_corner(float radius, float factor = 0.0f, float position = 0.0f, bool tl = true, bool tr = true, bool br = true, bool bl = true);

	//src�� src_bgra_index�� �ش��ϴ� ä�ΰ���(bgra���� n�� ä�� �ε���)
	//dst�� dst_bgra_index�� �ش��ϴ� ä�ΰ����� ����
	//ex. src�� ����ũ �̹�����, dst�� ���� �̹����� �����ϰ� index�� 3, 3���� �ϸ�
	//src�� alpha���� dst�� alpha������ �����Ѵ�.
	//�̸� �̿��Ͽ� mask overlay�� Ȱ���� �� �ִ�.
	void			replace_channel(Gdiplus::Bitmap* src, Gdiplus::Bitmap* dst, int src_bgra_index, int dst_bgra_index);
	void			replace_channel(Gdiplus::Bitmap* src, int src_bgra_index, int dst_bgra_index);
	void			replace_channel(CString type, UINT srcID, int src_bgra_index, int dst_bgra_index);
	void			replace_channel(CString src_file, int src_bgra_index, int dst_bgra_index);
	Gdiplus::PathGradientBrush* createFluffyBrush(Gdiplus::GraphicsPath* gp, float* blendFactors, float* blendPositions, INT count, INT* in_out_count);


	//��� �׸��� �̹����� �����Ѵ�.
	void			create_back_shadow_image(CSCGdiplusBitmap* shadow, float sigma = 10.0f, int type = 0, int depth = 10);

	//ColorMatrix�� �̿��Ͽ� ������ ����̹����� ���� �� ������
	//�װ� 3ä���� ������� �̹����̹Ƿ� 1ä�� 256 gray�̹����� �ƴϴ�.
	//���� resource���� �о���� �̹����� ����� ��ȯ���� �ʴ� ���� ����(1/3�� �ε��Ǵ� ����)
	void			convert2gray();

	//new_format�� PixelFormat32bppARGB �� ���� Ÿ������ ȣ���Ѵ�.
	void			convert(Gdiplus::PixelFormat new_format);

	//printf()ó�� save(_T("D:\\test_%d.png"), i);�� ���� ����� �� �ִ�.
	bool			savef(LPCTSTR filepath, ...);
	bool			save(Gdiplus::Bitmap* bitmap, CString filename, int quality = 100);
	//�׽�Ʈ �������� ������ �����Ͽ� Ȯ���� ���� �����ؾ� �Ѵ�.
	//ASee ������Ʈ������ CDirWatcher�� ���� ���� �̹����� ���� ������ ����͸��ϰ� �����Ƿ�
	//��� refresh�ϰ� �Ǿ� thread_buffering()���� ��� ������ �߻��� ���� �ִ�.
	bool			save(CString filename, int quality = 100);
	bool			copy_to_clipbard();
	bool			paste_from_clipboard();

	int				width = 0;
	int				height = 0;
	int				channel = 0;
	int				stride = 0;

//animated Gif ����
	int				m_frame_count;
	int				m_frame_index;
	//PropertyItem ���� (orientation, GpsLongitude, GpsAltitude, GpsSpeed, ImageDescription, Orientation...)
	//https://www.devhut.net/getting-a-specific-image-property-using-the-gdi-api/
	Gdiplus::PropertyItem* m_property_item = NULL;
	//m_frame_delay�� m_property_item�� �����ִµ� ���� �߰������� frame_delay�� ������ ���ؼ��� ������ malloc�� �ؾ��ϴ� ���ϴ�.
	//�ϴ� �ش� �׸��� ���� ������ ���ؼ� ����Ѵ�.
	Gdiplus::PropertyItem* m_frame_delay = NULL;

	//ī�޶�� �Կ��� ������ ���� exif ������ �ʿ�� �� �� �ִ�.
	//property�� ������ �������� �ٸ����� �ֿ� ������ �̸� ���ؼ� �ʿ��� ��� ��½�Ų��.
	//Common\image_processing\exif�� �̿��ؼ� ���� ������ �� ������ dependency�� �߻��ϹǷ�
	//m_property_item�� �̿��ؼ� ������ �����ϴ� ���� �������ϴ�.


	bool			is_animated_gif() { return (is_valid() && (m_frame_count > 1)); }
	int				get_frame_count() { return m_frame_count; }
	//parenthWnd ���� ������ ������ ǥ��. ����ȿ���� �������� �ʴ´�.
	//parent�� ������� �����ϰ� ǥ���� ���� CImageShapeWnd�� ���.
	void			set_animation(HWND parenthWnd, int x = 0, int y = 0, int w = 0, int h = 0, bool auto_play = true);
	//void	set_animation(HWND parenthWnd, int x, int y, float ratio = 1.0f, bool start = true);
	//ratio�� �����Ͽ� r�ȿ� ǥ���Ѵ�.
	void			set_animation(HWND parenthWnd, CRect r, bool start = true);
	void			move_gif(int x = 0, int y = 0, int w = 0, int h = 0);
	void			move_gif(CRect r);
	void			set_gif_back_color(COLORREF cr) { m_cr_back.SetFromCOLORREF(cr); }
	void			set_gif_back_color(Gdiplus::Color cr) { m_cr_back = cr; }
	void			play_gif();
	//pos��ġ�� �̵��� �� �Ͻ������Ѵ�. -1�̸� pause <-> play�� ����Ѵ�.
	void			pause_gif(int pos = 0);
	//animation thread�� ����ǰ� ȭ�鿡�� �� �̻� ǥ�õ��� �ʴ´�. ���� �״�� ���߱� ���Ѵٸ� pause_animation()�� ȣ���Ѵ�.
	void			stop_gif();
	void			goto_frame(int pos, bool pause = false);			//���� ���������� �̵�
	void			goto_frame_percent(int pos, bool pause = false);	//���� % ��ġ�� ���������� �̵�
	void			set_gif_mirror(bool is_mirror = true) { m_is_gif_mirror = is_mirror; }

	//gif ������ �̹������� ������ ������ ����
	bool			save_gif_frames(CString folder);
	//gif ������ �̹������� �����ؼ� ���� display�� �� �ִ�.
	//��, ��� �̹����� �����Ͽ� �޸𸮿� �ε��ϹǷ� �޸� ��뷮�� ������ �� �ִ�.
	void			get_gif_frames(std::vector<Gdiplus::Bitmap*>& dqBitmap, std::vector<long> &dqDelay);

	//�� ����ð��� ms������ �����Ѵ�.
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


	//-1 : ���� �Ǻ����� ���� �����̹Ƿ� �Ǻ� ����
	// 0 : 3ä���̰ų� 4ä���� ��� �ȼ��� alpha = 255�� ���
	// 1 : �� ���̶� alpha < 255�� ���
	int				m_has_alpha_pixel = -1;	
	void			resolution();
	Gdiplus::Bitmap* GetImageFromResource(CString lpType, UINT id);

	CString			m_pixel_format_str;

	CSCEXIFInfo		m_exif_info;

	Gdiplus::Bitmap*m_pOrigin = NULL;
	int				m_bright = 100;		//100%�� �⺻��
	int				m_contrast = 100;	//100%�� �⺻��

//animatedGif
	bool			m_paused = false;
	bool			m_gif_play_in_this = true;	//gif ����ڵ带 �� Ŭ�������� ��ü ������ ��, parent���� ���� ������ ��

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
