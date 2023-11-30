// Filename: ScrollHelper.h
// S.Chan, 01 Jul 2005
//https://www.codeproject.com/Articles/10902/Add-Scrolling-to-a-CWnd-or-CDialog-using-a-C-Helpe?fid=195884&df=90&mpp=25&sort=Position&spc=Relaxed&prof=True&view=Normal&fr=26#xx0xx

/*
* CWnd, CDialog등에 자동으로 스크롤바를 추가해주므로 편리하게 스크롤 기능을 이용할 수 있으나
* CStatic에서는 스크롤바가 표시는 되지만 disable상태가 되어 반응이 없다. notify를 true해도 안됨.
*/

#ifndef SCROLL_HELPER_INCLUDED
#define SCROLL_HELPER_INCLUDED

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <afxwin.h>

class CScrollHelper
{
public:
    CScrollHelper();
    ~CScrollHelper();

    // Attach/detach a CWnd or CDialog.
    void   AttachWnd(CWnd* pWnd);
    void   DetachWnd();

    // Set/get the virtual display size. When the dialog or window
    // size is smaller than the display size, then that is when
    // scrollbars will appear. Set either the display width or display
    // height to zero if you don't want to enable the scrollbar in the
    // corresponding direction.
    void   SetDisplaySize(int displayWidth, int displayHeight);
    const CSize& GetDisplaySize() const;

    // Get current scroll position. This is needed if you are scrolling
    // a custom CWnd which implements its own drawing in OnPaint().
    const CSize& GetScrollPos() const;

    // Get current page size. Useful for debugging purposes.
    const CSize& GetPageSize() const;

    // Scroll back to top, left, or top-left corner of the window.
    void   ScrollToOrigin(bool scrollLeft, bool scrollTop);

    // Message handling.
    void   OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    void   OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    BOOL   OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    void   OnSize(UINT nType, int cx, int cy);

private:
    int    Get32BitScrollPos(int bar, CScrollBar* pScrollBar);
    void   UpdateScrollInfo();
    void   UpdateScrollBar(int bar, int windowSize, int displaySize,
                           LONG& pageSize, LONG& scrollPos, LONG& deltaPos);

    CWnd*  m_attachWnd;
    CSize  m_pageSize;
    CSize  m_displaySize;
    CSize  m_scrollPos;
};

#endif // SCROLL_HELPER_INCLUDED

// END
