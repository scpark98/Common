// SCDropperDlg.h
#pragma once
#include <afxwin.h>
#include <gdiplus.h>

//모니터 전체 영역을 캡처한 후 커서를 중심으로 n x n 크기 이미지를 원형으로 그려서 픽셀을 선택하는 목적.
//CSCColorPicker에서 스포이드 클릭 시 이 창이 열리고, 픽셀을 클릭하면 해당 색상이 선택되어 CSCColorPicker로 전달된다.	
//CSCShapeDlg와 유사한 구조이므로 CSCShapeDlg를 상속받아 구현하려 했으나 claude가 권장하지 않음.
//이유는 CSCShapeDlg에 있는 코드 중 20%만 중복되고 불필요한 외부 파일들과 코드들이 80%임.
//권장하는 방식은 CSCLayeredDlg로 일부 기능을 분리하고 이를 상속받아 두 클래스를 구현하는 방식도 있으나 파일들이 역시 추가되고
//우선 보류한다.

class CSCDropperDlg : public CDialog
{
	DECLARE_DYNAMIC(CSCDropperDlg)

public:
	enum {
		kWndSizeDefault = 180,      // default circle diameter
		kWndSizeMin = 100,           // Ctrl+wheel min
		kWndSizeMax = 500,          // Ctrl+wheel max
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
};