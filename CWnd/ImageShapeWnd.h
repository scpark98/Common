#pragma once

/*
* png 또는 animatedGif를 디스플레이하는 목적으로 제작.
* aniGif인 경우는 각 프레임 이미지 모양으로 창의 모습이 변경된다.
* ShapeDlg등의 클래스를 상속받아서 만들 수도 있으나
* 매번 IDD_DIALOG 등의 다이얼로그 템플릿이 필요하고 해당 클래스 파일도 추가된다.
* 이 클래스는 이미지 파일만으로 그 모양의 팝업 윈도우를 생성하여 사용하는 것이 목적이다.
* 모든 프레임을 구한 후 처리해봤으나 메모리 사용량이 너무 커져서 SelectActiveFrame()을 이용하도록 수정.
* 
* 현재 문제점
* OnTimer(), OnLButtonDown()과 같은 이벤트 핸들러 함수가 호출되지 않는다.
* 메시지 콜백 함수인 ImageShapeWndMessageProc()에서는 처리되므로
* 이를 CImageShapeWnd에 SendMessage()하여 처리한다.
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
	//pos : int면 실제 프레임 인덱스, double이면 총 구간의 % 위치
	void set_pos(int pos);
	void set_pos(double pos);

	void alpha(int alpha) { m_alpha = alpha; }
	//이미지의 tx, ty위치의 색상을 crNew로 대체한다.(한 점이 아닌 이미지 전체 대상)
	//CGdiplusBitmap에서는 한장의 이미지에 대해 여러 색상을 교체할 수 있으나
	//gif인 경우는 
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
