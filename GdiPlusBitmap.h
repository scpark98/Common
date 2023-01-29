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

	//CGdiplusBitmap�� CGdiplusBitmapResource �� ���� Ŭ������ �־����� ������.
	bool	load(CString sFile, bool show_error = false);
	void	load(CString sType, UINT id, bool show_error = false);

	Gdiplus::Bitmap* CreateARGBBitmapFromDIB(const DIBSECTION& dib);


	operator Gdiplus::Bitmap*() const { return m_pBitmap; }

	//�⺻�����δ� �̹��� raw data�� �������� �ʴ´�.
	//cv::Mat�� dataó�� raw data�� �ʿ��� ��쿡 �� �Լ��� ȣ���ϸ� ����� ����������.
	bool	get_raw_data();

	bool	empty();
	bool	valid();
	int		channels();
	CSize	size() { return CSize(width, height); }

	//targetRect�� �ָ� ��� ������ ������ �����Ͽ� �׸���.
	//targetRect�� NULL�̸� 0,0�� �̹��� ũ���� �׸���.
	CRect	draw(CDC* pDC, CRect targetRect);
	CRect	draw(CDC* pDC, int dx = 0, int dy = 0, int dw = 0, int dh = 0);
	CRect	draw(CDC* pDC, CGdiplusBitmap mask, CRect targetRect);

	//Gdiplus::Bitmap::Clone�� shallow copy�̹Ƿ� ������ ���縦 ���ؼ��� deep_copy�� ����ؾ� �Ѵ�.
	void	clone(CGdiplusBitmap* dst);
	void	deep_copy(CGdiplusBitmap* dst);
	void	rotate(Gdiplus::RotateFlipType type);
	//�¿��Ī
	void	mirror();
	//���ϴ�Ī
	void	flip();

	//ȸ����Ű�� w, h�� �޶����Ƿ� �̹����� ũ�⸦ ��������߸� �ϴ� ��쵵 �ִ�.
	//�׷� ���� auto_resize�� true�� �ְ� ���ʿ��� ����� ������ ����
	//���ʿ��� ����� ������ �����Ͽ� �̹��� ũ�⸦ fit�ϰ� ���ϼ��� �ִ�.
	void rotate(float degree, bool auto_resize = false, Gdiplus::Color remove_back_color = Gdiplus::Color::Transparent);

	//InterpolationModeNearestNeighbor		: ���� ȭ�Ҹ� ���� ���������� �Ϻ� ȭ�Ҵ� �����. �׷��� �� ��ģ ����
	//InterpolationModeHighQualityBilinear	: �ε巴�� resize������ �ణ �ѿ��� ����
	//InterpolationModeHighQualityBicubic	: �ӵ��� �������� �ְ� ǰ�� ���
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

	//ColorMatrix�� �̿��Ͽ� ������ ����̹����� ���� �� ������
	//�װ� 3ä���� ������� �̹����̹Ƿ� 1ä�� 256 gray�̹����� �ƴϴ�.
	//���� resource���� �о���� �̹����� ����� ��ȯ���� �ʴ� ���� ����(1/3�� �ε��Ǵ� ����)
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
	//gif ������ �̹������� ������ ������ ����
	bool	save_gif_frames(CString folder);
	//gif ������ �̹������� �����ؼ� ���� display�� �� �ִ�.
	void	get_gif_frames(std::vector<CGdiplusBitmap*>& dqImage, std::vector<long> &dqDelay);

	//�� ����ð��� ms������ �����Ѵ�.
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
