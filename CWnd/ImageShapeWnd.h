#pragma once

/*
* png �Ǵ� animatedGif�� ���÷����ϴ� �������� ����.
* aniGif�� ���� �� ������ �̹��� ������� â�� ����� ����ȴ�.
* ShapeDlg���� Ŭ������ ��ӹ޾Ƽ� ���� ���� ������
* �Ź� IDD_DIALOG ���� ���̾�α� ���ø��� �ʿ��ϰ� �ش� Ŭ���� ���ϵ� �߰��ȴ�.
* �� Ŭ������ �̹��� ���ϸ����� �� ����� �˾� �����츦 �����Ͽ� ����ϴ� ���� �����̴�.
* ��� �������� ���� �� ó���غ����� �޸� ��뷮�� �ʹ� Ŀ���� SelectActiveFrame()�� �̿��ϵ��� ����.
* 
* ���� ������
* OnTimer(), OnLButtonDown()�� ���� �̺�Ʈ �ڵ鷯 �Լ��� ȣ����� �ʴ´�.
* �޽��� �ݹ� �Լ��� ImageShapeWndMessageProc()������ ó���ǹǷ�
* �̸� CImageShapeWnd�� SendMessage()�Ͽ� ó���Ѵ�.
*/

#include <afxwin.h>
#include <vector>
#include "../../Common/GdiPlusBitmap.h"
// CImageShapeWnd

class CImageShapeWnd : public CWnd
{
	DECLARE_DYNAMIC(CImageShapeWnd)

public:
	CImageShapeWnd();
	~CImageShapeWnd();

	static CImageShapeWnd* m_pInstatnce;

	//static LRESULT CALLBACK DummyMessageProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	//{
	//	CImageShapeWnd* dummy = reinterpret_cast<CImageShapeWnd*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	//	if (dummy)
	//		return dummy->ImageShapeWndMessageProc(hWnd, msg, wParam, lParam);
	//	return ::DefWindowProc(hWnd, msg, wParam, lParam);
	//}

	static LRESULT CALLBACK CImageShapeWnd::s_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	//static LRESULT CALLBACK ImageShapeWndMessageProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	//{
	//	//m_frame_index = 5;
	//	return 0;
	//}

	bool create(HWND parentHwnd, CString file, bool auto_play = true, int w = 0, int h = 0, double fx = 0.0, double fy = 0.0);
	bool create(HWND parentHwnd, CString type, UINT id, bool auto_play = false, int w = 0, int h = 0, double fx = 0.0, double fy = 0.0);
	bool create(HWND parentHwnd);
	bool load(CString file, bool auto_play = false);
	bool load(CString type, UINT id, bool auto_play = false);
	void resize(int w, int h, double fx = 0.0, double fy = 0.0);
	void play();
	void repeat(bool repeat) { m_repeat = repeat; }
	//pos : int�� ���� ������ �ε���, double�̸� �� ������ % ��ġ
	void set_pos(int pos);
	void set_pos(double pos);

	void alpha(int alpha) { m_alpha = alpha; }
	//�̹����� tx, ty��ġ�� ������ crNew�� ��ü�Ѵ�.(�� ���� �ƴ� �̹��� ��ü ���)
	//CGdiplusBitmap������ ������ �̹����� ���� ���� ������ ��ü�� �� ������
	//gif�� ���� 
	void replace_color(int tx, int ty, Gdiplus::Color crNew = Gdiplus::Color::Transparent);

	CGdiplusBitmap	m_img;
	int m_alpha = 255;
	int m_frame_index;
	//std::vector<CGdiplusBitmap*> m_gif_frames;
	//std::vector<long> m_gif_delay;

	bool m_use_replace_color = false;

protected:

	enum TIMER
	{
		timer_test = 0,
	};

	BOOL RegisterWindowClass();
	void render(Gdiplus::Bitmap *img);

	bool m_run_thread_animate = false;
	bool m_repeat = true;
	void thread_animate();

	Gdiplus::Color m_old_color;
	Gdiplus::Color m_new_color;

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	virtual void PreSubclassWindow();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
//	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
};

static LRESULT CALLBACK ImageShapeWndMessageProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
