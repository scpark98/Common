#pragma once

/* scpark
- gdi+를 이용한 png 투명 버튼 클래스
- 단색 또는 배경 이미지 선택 가능.
- 배경 이미지의 포인터를 넘겨받아 버튼의 배경으로 이용함.
  따라서 배경은 (0,0)에서 시작되고 resize되지 않은 배경인 경우에만 가능함.
- 사용 방법에 따라 일반 push button, toggle button(as a checkbox) 등으로 사용할 수 있음.

[usage]
- stdafx.h
	#include <gdiplus.h>
	using namespace Gdiplus;

- App.h
	ULONG_PTR	m_gdiplusToken;
	and add ExitInstance() virtual function using class wizard.

- in App.cpp

	in InitInstance() function, add below lines before calling main dlg.

	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);


	in ExitInstance() function, add below.
	
	Gdiplus::GdiplusShutdown(m_gdiplusToken);
*/

//버튼 자동 정렬 정의이며 특정 좌표값 기준이 아닌
//전체 배경 이미지(클라이언트 영역)을 기준으로 한 정렬이다.
#define ANCHOR_NONE			0x00000000
#define ANCHOR_LEFT			0x00000001
#define ANCHOR_TOP			0x00000010
#define ANCHOR_RIGHT		0x00000100
#define ANCHOR_BOTTOM		0x00001000
#define ANCHOR_HCENTER		0x00010000
#define ANCHOR_VCENTER		0x00100000
#define ANCHOR_CENTER		ANCHOR_HCENTER | ANCHOR_VCENTER

#define MAX_BUTTON_IMAGE	10


// CGdiButton

class CGdiButton : public CButton
{
	DECLARE_DYNAMIC(CGdiButton)

public:
	CGdiButton();
	virtual ~CGdiButton();

	//enum 

	//pBack은 버튼의 배경이 되는 그림으로 parent 창의 배경이미지에 사용된 Bitmap*이다.
	//그 배경에서 버튼의 배경을 추출하여 DrawItem에서 배경+전경 이미지를 그려주게 된다.
	//단, parent창에서 배경 그림은 반드시 원래 크기와 비율대로 그려져야 한다.
	//버튼의 위치 좌표에 해당하는 배경 그림을 잘라와서 사용하므로 배경도 1:1로 뿌려져야 한다.
	
	//GetParent()->UpdateWindow()를 이용하여 뿌려주는 방식은 배경 데이터를 명시하지 않아도 투명하게 뿌려지는 장점은 있으나
	//UpdateWindow로 인해 이미지를 변경하는 이벤트가 발생하면 깜빡이는 단점이 존재한다.

	//ex. ANCHOR_RIGHT 옵션을 주면 dx 좌표값과는 무관하게 부모창의 오른쪽에 버튼이 위치하게 된다.

	//한 버튼은 최대 10장의 이미지를 담을 수 있으며 0번이 normal상태 이미지, 그리고 checked 상태로 시작된다.
	bool		AddImage( CString sfile, Bitmap* pBack = NULL, COLORREF crBack = 0, bool bFitToImage = true, int dx = -1, int dy = -1, UINT nAnchor = ANCHOR_NONE, bool bAsStatic = false, bool bShowError = true );
	void		SelectImage( int nIndex );

	//gray 이미지를 자동 생성해서 추가해준다. (어떤 이미지를 선택인 경우는 컬러로, 비선택인 경우는 흑백으로 표시하는 용도로 유용하다)
	//여러장의 이미지를 표현하는 경우에는 사용하지 않으며 보통 1장의 이미지 버튼인 경우 disable 또는 unchecked를 표현하기 위해 사용된다.
	//따라서 normal(checked)는 0번, gray(unchecked)는 1번 인덱스를 갖는다.
	bool		AddGrayImage( bool bSelectGray = false );

	Bitmap*		Load( CString sfile );
	void		ReleaseAll();

	void		SetBackImage( Bitmap* pBack );		//배경을 설정, 변경할 경우 사용
	void		SetBackColor( COLORREF crBack );	//배경색을 설정, 변경할 경우 사용, 기존 배경 그림이 있었을 경우는 배경 그림은 해제됨.



//checkbox or radio button style
	//BS_PUSHBUTTON 이었으나 동적으로 변경할 수 있다. CButton::SetButtonStyle()을 override한 것이므로 주의해야 한다.
	void		SetButtonStyle( UINT nStyle ) { m_nButtonStyle = nStyle; }

	//1번 이미지는 true, 0번 이미지는 false (checkbox나 radiobutton과 같은 용도로 사용할 수 있다)
	bool		GetCheck();
	//true이면 1번 이미지를 표시해준다. 만약 1번이 없다면 normal 이미지만 가진 경우이므로
	//AddGrayImage로 회색 이미지를 1번에 추가해주고 0번과 1번을 바꿔준다.
	void		SetCheck( bool bCkeck );
	void		Toggle();

	//m_bAsStatic이 true일 경우 hover와 down에는 반응을 하지 않는다.
	void		SetAsStatic( bool bAsStatic = true ) { m_bAsStatic = bAsStatic; }


	//버튼의 크기, 위치 변경
	void		SetAnchor( UINT nAnchor ) { m_nAnchor = nAnchor; }	//정렬 방식 설정
	void		SetAnchorMargin( int x, int y ) { m_nAnchorMarginX = x; m_nAnchorMarginY = y; }
	void		ReAlign();	//parent의 크기가 변하면 설정된 align값에 따라 위치를 재조정해준다.
	void		Offset( int x, int y );
	void		Inflate( int cx, int cy );
	void		Inflate( int l, int t, int r, int b );

	//포커스 사각형 관련
	void		ShowFocusRect( bool bShow = true ) { m_bShowFocusRect = bShow; Invalidate(); }
	void		SetFocusRectColor( COLORREF crFocus ) { m_crFocusRect = crFocus; Invalidate(); }
	void		SetFocusRectWidth( int nWidth ) { m_nFocusRectWidth = nWidth; Invalidate(); }

	//blink
	void		SetBlinkTime( int nTime0 = 400, int nTime1 = 1200 );	//nTime0:hidden, nTime1:shown
	void		SetBlink( BOOL bBlink = TRUE );

protected:
	UINT		m_nButtonStyle;

	Bitmap*		m_pImage[MAX_BUTTON_IMAGE];		//한 버튼에 추가된 이미지들
	int			m_nImages;						//담고 있는 이미지의 수
	int			m_nIndex;						//현재 표시하고자 하는 이미지 인덱스

	Bitmap*		m_pBack;				//버튼의 배경 이미지, NULL이면 m_crBack이 배경색
	Bitmap*		m_pBackOrigin;
	COLORREF	m_crBack;				//버튼의 배경색


	int			m_width;
	int			m_height;

	UINT		m_nAnchor;
	int			m_nAnchorMarginX;
	int			m_nAnchorMarginY;

	bool		m_bToggleButton;		//기본 이미지를 표시할지, 부가 이미지를 표시할지
	bool		m_bAsStatic;			//단순 이미지 표시 용도로 사용되고 클릭해도 변화가 없다. 기본값 false.
	bool		m_bIsHovering;
	bool		m_bIsTracking;
	bool		m_bIsPressed;
	bool		m_bHasFocus;
	bool		m_bShowFocusRect;		//포커스 사각형 표시 여부(기본값 false)
	COLORREF	m_crFocusRect;			//색상
	int			m_nFocusRectWidth;		//두께
	bool		m_bFitToImage;			//true이면 이미지 크기대로 컨트롤 크기 변경, false이면 컨트롤 크기로 이미지 사이즈 변경. 기본값=true

	BOOL		m_bBlink;
	BOOL		m_bBlinkStatus;
	int			m_nBlinkTime0;		//blink type is Show/Hide, time0 = shown duration, time1 = hidden duration in millisecond.
	int			m_nBlinkTime1;


	void		ResizeControl( int& dx, int& dy );
	void		SafeRelease( Bitmap** pBitmap );


protected:
	DECLARE_MESSAGE_MAP()
	virtual void PreSubclassWindow();
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void DrawItem(LPDRAWITEMSTRUCT /*lpDrawItemStruct*/);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseHover(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
};


