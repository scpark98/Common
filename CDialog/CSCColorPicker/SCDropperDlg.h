// SCDropperDlg.h
#pragma once
#include <afxwin.h>
#include <gdiplus.h>

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
		kSampleMin = 3,             // zoom in limit (odd)
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