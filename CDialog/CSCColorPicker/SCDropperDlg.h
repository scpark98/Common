// SCDropperDlg.h
#pragma once
#include <afxwin.h>
#include <gdiplus.h>

/*
모니터 전체 영역을 캡처한 후 커서를 중심으로 n x n 크기 이미지를 원형으로 그려서 픽셀을 선택하는 목적.
CSCColorPicker에서 스포이드 클릭 시 이 창이 열리고, 픽셀을 클릭하면 해당 색상이 선택되어 CSCColorPicker로 전달된다.	
CSCShapeDlg와 유사한 구조이므로 CSCShapeDlg를 상속받아 구현하려 했으나 claude가 권장하지 않음.
이유는 CSCShapeDlg에 있는 코드 중 20%만 중복되고 불필요한 외부 파일들과 코드들이 80%임.
권장하는 방식은 CSCLayeredDlg로 일부 기능을 분리하고 이를 상속받아 두 클래스를 구현하는 방식도 있으나 파일들이 역시 추가되고
우선 보류한다.

- 호출 전에 parent를 감추거나 메뉴를 통해 실행할 때 parent를 숨기거나 메뉴가 사라진 후 캡처해야 하므로 약간의 딜레이를 주고 실행해야 한다.
- 처음에는 실시간으로 화면을 캡처하여 처리하려 했으나 문제가 있어 우선 이 창이 실행되기 전 캡처한 이미지를 대상으로 처리한다.
- 좌클릭 : 픽셀 선택 및 선택된 색상을 get_picked_color()로 반환
- 우클릭 : 창 취소
- 휠클릭 : 좌표 및 색상정보 표시/숨김

[실제 호출 방법]
void CASeeDlg::OnMenuMagnify()
{
	//메뉴가 완전히 사라진 후 돋보기가 표시되어야 하므로 약간의 딜레이가 필요하다.
	Wait(500);

	CSCDropperDlg dlg;
	dlg.create(this);

	MSG msg = {};
	bool quit_posted = false;
	msg = {};

	//create으로 띠운 후 키보드 또는 마우스 처리를 위해 자체적으로 메시지 루프를 돌린다.(DoModal()이 아니므로)
	while (dlg.GetSafeHwnd() != NULL)
	{
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				quit_posted = true;
				break;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if (quit_posted) break;
		if (dlg.GetSafeHwnd() != NULL)
			WaitMessage();
	}

	if (quit_posted)
		PostQuitMessage(static_cast<int>(msg.wParam));
}
*/

class CSCDropperDlg : public CDialog
{
	DECLARE_DYNAMIC(CSCDropperDlg)

public:
	enum {
		kWndSizeDefault = 180,      // default circle diameter
		kWndSizeMin = 100,           // Ctrl+wheel min
		kWndSizeMax = 1000,          // Ctrl+wheel max
		kWndSizeStep = 20,          // Ctrl+wheel step
		kTimerID = 1,
		kSampleMin = 9,             // zoom in limit (odd). 현재 dropper 창에 kSampleMin x kSampleMin 개의 셀이 표시된다.
		kSampleDefault = 15         // default sample count (odd)
	};

private:
	Gdiplus::Color  m_picked_color;
	bool            m_picked = false;
	COLORREF        m_center_color = 0;
	int             m_sample = kSampleDefault;
	int             m_wnd_size = kWndSizeDefault;
	bool			m_show_info = true;

	// 1회 캡처된 전체 화면 이미지
	HDC             m_hScreenDC = nullptr;
	HBITMAP         m_hScreenBmp = nullptr;
	HBITMAP         m_hOldScreenBmp = nullptr;
	CPoint          m_screen_origin;

	void            capture_screen();
	void            release_screen();
	void            update_display();
	void            load_settings();
	void            save_settings();

public:
	CSCDropperDlg(CWnd* pParent = nullptr);
	virtual ~CSCDropperDlg();

	bool            create(CWnd* parent);
	Gdiplus::Color  get_picked_color() const { return m_picked_color; }
	bool            is_picked()        const { return m_picked; }

protected:
	virtual void    DoDataExchange(CDataExchange* pDX) override;
	virtual void    PostNcDestroy() override;

	DECLARE_MESSAGE_MAP()
	afx_msg void    OnTimer(UINT_PTR nIDEvent);
	afx_msg void    OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void    OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg BOOL    OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
public:
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
};