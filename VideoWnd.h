#pragma once

/* CVideoWnd
[ 2017 08 09 수정사항]
- ROI 관련 설정 및 해제, 이동, 크기 조정에 Shift, Ctrl을 이용했으나
  메인 App에서 Shift, Ctrl을 이용하는 경우가 많기 때문에
  위의 ROI 관련 기능을 팝업메뉴 선택 항목으로 표현한다.
  * ROI 설정, ROI 크기 조정, ROI 이동, ROI 해제

[기본 기능]
- opencv를 이용한 비디오 영상 또는 캠영상 디스플레이용 윈도우
- 동영상 파일을 재생할수도, 이미지 데이터를 전달받아 이미지만 디스플레이 할 수도 있다.
- 동영상 파일 drag&drop 열기.
- ROI 설정, 해제, ROI의 이동 및 크기조정 가능.
- ctrl + drag로 임의 영역의 크기와 대각선 길이 표시.
- copy, paste 기능 추가.
*/

#include <afxwin.h>
#include "Functions.h"
#include "OpenCVFunctions.h"

#define BMPX	16

#define NO_CHANGE				-2
#define STOP					-1
#define PAUSE					0
#define	PLAY					1
#define PLAY_TOGGLE				2

enum VIDEO_WND_MESSAGES
{
	message_video_wnd_video_opened = WM_USER + 10001,
	message_video_wnd_video_closing,					//before normal video closing or app exit.
	message_video_wnd_mouse_event,
	message_video_wnd_track_changing,					//before track change
	message_video_wnd_track_changed,					//after track changed
	message_video_wnd_play_state_changed,
	message_video_wnd_roi_changed,
	message_video_wnd_vanishingpoint_changed,
	message_video_wnd_end_of_video,						//end of track.(not stop playing)
};

class CVideoWnd : public CWnd
{
	DECLARE_DYNAMIC(CVideoWnd)
protected:
	BOOL			RegisterWindowClass();

public:
	int				m_nVideoWndID;			//한 앱에서 2개 이상의 CVideoWnd를 사용할 경우 메시지 전달할 때 구분ID로 이용함.
	void			SetVideoWndID( int nID ) { m_nVideoWndID = nID; }

	//현재는 Open시에 대입되지만 Open하지 않고 클립보드 이미지를 붙여넣을수도 있으므로 생성된 후에 이 값이 채워져있어야 한다.
	//또는 SetParentWnd()로 직접 대입해준다.
	HWND			m_hParentWnd;
	void			SetParentWnd( HWND hParent ) { m_hParentWnd = hParent; }

	bool			m_bAutoPlayWhenDrop;	//default = true
	void			SetAutoPlayWhenDrop( bool play ) { m_bAutoPlayWhenDrop = play; }

	//popup menu 관련
	bool			m_bUsePopupMenu;
	bool			m_bPopupMenu;			//팝업메뉴가 떠있는 상태인지...마우스 커서 때문에 필요.
	bool			m_bSaveRecentFrame;		//마지막 재생 프레임 위치 저장 여부. default = false
	int				m_nSaveIndex;
	bool			m_bImageFromClipboard;	//현재 m_mat이 video frame image인지 클립보드에서 붙여넣기 한 이미지인지...m_nSaveIndex에 영향을 준다.
	void			UsePopupMenu( bool bUse ) { m_bUsePopupMenu = bUse; }
	void			OnExitMenuLoop( BOOL bIsTrackPopupMenu );
	void			OnPopupMenu( UINT nID );

	enum POPUP_MENU_ITEM
	{
		id_menu_image_info = 1,
		id_menu_file_open,
		id_menu_file_open_sequence,

		id_menu_do_not_image_processing,

		id_menu_yuv_stereo,
		id_menu_yuv_size,
		id_menu_yuv422_uyvy,
		id_menu_yuv422_yuyv,
		id_menu_yuv420_nv12,
		id_menu_yuv420_yv12,	//I420

		id_menu_resize_ratio,
		id_menu_roi_set,
		id_menu_roi_reset,
		id_menu_roi_move,
		id_menu_goto_frame,
		id_menu_repeat,
		id_menu_auto_next_video,
		id_menu_save_recent_frame,			//최근 재생 위치를 레지스트리에 기억시킬 것인지. 나중에 재생하면 그 위치부터 재생.
		id_menu_current_vanishing_point,	//현재 설정된 소실점 좌표 정보를 표시해준다.
		id_menu_show_vanishing_point,		//현재 설정된 소실점 좌표 정보를 표시해준다.
		id_menu_get_vanishing_point,		//화면상에 4점을 클릭하여 vp를 구함
		id_menu_set_vanishing_point,		//vp좌표를 직접 입력함
		id_menu_measure_size,
		id_menu_copy_to_clipboard,
		id_menu_paste_from_clipboard,
		id_menu_save_as_original_size,
		id_menu_save_as_displayed_size,
	};
	void			OpenSequenceImages( CString sfile );
	void			OnMenuROIReset();
	void			CopyToClipboard();
	void			CopyVideonameAndFrameToClipboard();
	void			PasteFromClipboard();
	void			SaveAsOriginalSize();
	void			SaveAsDisplayedSize();


	CToolTipCtrl	m_ToolTip;
	CPoint			m_ptMousePosOld;

	bool			m_bSendPlayWndInfo;		//재생정보와 제어정보 등의 변경 메시지를 parent로 전송할지...기본값=false
	void			SendPlayWndInfo( bool bUse = true ) { m_bSendPlayWndInfo = bUse; }

	bool			m_bUseCrossCursor;		//십자 커서를 사용할지..default = false
	HCURSOR			m_hCursor;
	void			UseCrossCursor( bool bUse = true ) { m_bUseCrossCursor = bUse; }

	bool			m_bUsePatternBackground;//pattern의 배경을 사용할지 solid 배경색을 사용할지. default = false
	void			UsePatternBackground( bool bUse ) { m_bUsePatternBackground = bUse; }
	COLORREF		m_crBackground;
	unsigned char	m_Bitmap_Pattern[BMPX * BMPX];	//non-image 배경에 표시할 회색 격자 패턴 비트맵 데이터


	//infotext 관련
	CString			m_sInfoText[4];							//lt, rt, lb, rb 4곳에 텍스트 정보 출력용
	CRect			m_rInfoText[4];
	COLORREF		m_crInfoText[4];
	bool			m_bShowInfoText[4];						//show or not

	void			SetShowInfoText( int idx, bool bShow );
	CString			GetInfoText( int idx ) { return m_sInfoText[idx]; }
	void			SetInfoText( int idx, CString sInfoText, COLORREF cr = -1 ) { m_sInfoText[idx] = sInfoText; DisplayInfoText(idx); m_crInfoText[idx] = cr; }
	void			DisplayInfoText( int idx );

	CString			m_sInfoTextFont;	//default = "System";
	void			SetInfoTextFont( CString sFont ) { m_sInfoTextFont = sFont; }

	int				m_nInfoTextFize;	//default = 10 (System font does not affected)
	void			SetinfoTextSize( int size ) { m_nInfoTextFize = size; }

	COLORREF		m_crTextColor;		//default info text color
	COLORREF		m_crTextBkColor;	//default info text background color
	void			SetTextColor( COLORREF crText ) { m_crTextColor = crText; Invalidate(); }
	void			SetTextBkColor( COLORREF crTextBk ) { m_crTextBkColor = crTextBk; Invalidate(); }


	bool			m_bDrawFocusRect;
	void			DrawFocusRect( bool bDraw = true ) { m_bDrawFocusRect = bDraw; }

//비디오 파일 및 메인 영상 관련
	int				m_nFileType;
	int				m_cam_index;
	int				GetFileType() { return m_nFileType; }
	CString			m_sVideoFileName;
	CString			GetVideoFileName() { return m_sVideoFileName; }
	void			SetVideoFileName( CString sName ) { m_sVideoFileName = sName; SetWindowText(sName); }	//실제 이미지나 비디오를 열지 않았지만 가상의 타이틀을 설정할 경우 필요

	//bin 동영상 및 stereo video 관련
	CFile			m_FileBin;
	int				m_nBinWidth;
	int				m_nBinHeight;
	bool			m_bStereoBin;		//default = false
	bool			m_bMandoRearCam;	//default = false
	int				m_nHeaderSize;		//custom header info size. default = 0. mando rear cam = 1000
	int				m_nTailSize;		//custom tail info size. default = 0. mando rear cam = 11288
	Mat				m_matOrigin2;		//stereo right image original
	Mat				m_mat2;				//stereo right image resized
	uint8_t			*imgYUV;
	uint8_t			*m_yuv_origin;		//stereo 영상에서 left 영상의 원본 yuv 데이터
	int				m_yuv_format;
	void			set_bin_size( int w, int h ) { m_nBinWidth = w; m_nBinHeight = h; }
	void			stereo_video( bool stereo ) { m_bStereoBin = stereo; }

	//sfile = "" 일때 bRunFileDialog = true이면 파일열기 창을 띠워서 입력받는다. 이때 기본 폴더는 sRecentFile이 있는 폴더가 된다.
	bool			OpenVideoFile( CString sfile, bool bRunFileDialog = true, CString sRecentFile = _T(""), bool bPlay = true );
	bool			open_cam(int index, bool auto_start = true);
	void			CloseVideoFile();
	//정상적으로 열린 동영상 파일일 경우에 파일명 변수에 저장되므로 이를 이용해 동영상 파일이 열린 상태인지 체크한다.
	bool			IsVideoFileOpened() { return (m_sVideoFileName != ""); }

	VideoCapture	m_capture;
	Mat				m_matOrigin;								//비디오에서 추출한 원영상
	Mat				m_mat;										//처리를 위해 축소 또는 잘라진 메인 영상
	Mat				GetNonFilteredMat();						//resize ratio와 roi만 적용된 소스 영상을 리턴.

	CSize			m_szImageSizeOrigin;						//원본 영상의 크기
	CSize			m_szImageSize;								//처리를 위해 resize된 크기. roi의 크기가 아닌 확대, 축소된 영상의 크기임.
	CSize			GetImageSizeOrigin();
	CSize			GetImageSize();
	double			GetImageRatio();							//cols / rows

	//원본 영상을 몇배로 확대 또는 축소해서 처리할지의 비율(percentage). default = 1.0
	double			m_dResizeRatio;
	int				m_nInterpolation;							//opencv resize policy. cv default = INTER_LINEAR
	int				GetInterpolationMode() { return m_nInterpolation; }
	void			SetInterpolationMode( int mode ) { m_nInterpolation = mode; }
	double			GetResizeRatio() { return m_dResizeRatio; }
	void			SetResizeRatio( double dRatio );			

	bool			ReadFrameFromBinFile();						//bin 파일에서 현재 프레임 영상을 얻어 m_mat에 넣어준다.
	void			Clear() { m_mat.release(); Invalidate(); }	//현재 영상 이미지인 m_mat을 release하고 화면 갱신.

	//재생 관련
	bool			m_do_not_image_processing = false;
	bool			m_bRepeat;									//반복 재생 여부. 기본값 false.
	void			Repeat( bool bRepeat = true ) { m_bRepeat = bRepeat; }
	void			Play( int nState );
	int				m_nPlayState;
	int				GetPlayState() { return m_nPlayState; }
	bool			m_bReverse;
	void			SetReversePlay( bool bReverse ) { m_bReverse = bReverse; }
	void			ToggleReversePlay() { m_bReverse = !m_bReverse; }

	//재생 fps 관련
	double			m_VideoFPS;	//동영상 인코딩된 실제 FPS
	double			GetVideoFPS() { return m_VideoFPS; }

	//처리속도 관련
	//CVideoWnd 내부에서 FPS를 계산해서 보여줄지(true),
	//실제 영상처리 루틴에서 별도로 계산해서 FPS를 보여줄지(false)...기본값=true;
	//내부에서 처리하면 프레임을 추출한 후 이미지 처리 과정을 거치고 다음 프레임을 추출하기 전까지의 시간을 계산하므로
	//디스플레이나 다른 부가적인 코드들의 수행시간도 모두 포함된다.
	//따라서 순수 이미지 처리 과정만의 fps를 구하고자 할 경우에는 이 값을 false로 세팅하고
	//실제 이미지 처리 루틴에서 fps를 구해서 사용한다.
protected:
	double			m_dCurrentFPS;
	double			m_dCurrentProcessingTime;
	double			m_dTime0, m_dTime1;
	bool			m_bUseOwnFPS;

public:
	void			UseOwnFPS( bool bUse ) { m_bUseOwnFPS = bUse; }
	double			GetCurrentFPS() { return m_dCurrentFPS; }
	double			GetCurrentProcessingTime() { return m_dCurrentProcessingTime; }
	//계산된 fps를 infoIndex 위치에 표시한다. infoIndex = -1이면 fps만 리턴한다.
	double			calcFPS(double t0, double t1, int infoIndex = 1);

//프레임 관련
	//이미지 파일들을 동영상 각 프레임처럼 처리할 경우 true. 기본값 = false.
	//사용하는 프로젝트의 목적에 맞게 true 또는 false로 변경해준다.
	bool			m_bUseImageArray;
	std::deque<CString>	m_dqImageFiles;		//이미지 파일 목록
	void			UseImageArray(bool bUse = true) { m_bUseImageArray = bUse; }
	void			BuildImageArray(CString sfile, CString sExts);

	int				m_nCurrentFrame;
	unsigned __int64 m_nTotalFrame;
	int				GetCurrentFrame()
	{
			return m_nCurrentFrame;
	}
	int				GetTotalFrame() { return (int)m_nTotalFrame; }
	void			SendTrackChanged();		//send TRACK_CHANGED message and display frame info

	//동영상이 아닌 이미지 리스트 정보를 알고 있다면 
	void			GotoFrame(int nFrame);
	void			GotoNextFrame(bool bNext, int nStep = 1, int newState = NO_CHANGE );
	void			GotoNextFrameAr(CStringArray *ar, int *nCurrent, bool bNext = true, int nStep = 1, int newState = NO_CHANGE);

	//mat을 m_mat에 복사하여 화면에 표시한다. 이 값이 NULL일 경우는 그냥 현재 m_mat을 갱신시켜주는 Invalidate()용으로 호출한다.
	//filter 적용시 세팅값이 변경되면 이미지 갱신을 위해 updateMainMat을 호출하는데
	//이미 m_mat이 이미 filter가 적용된 이미지 상태인 경우
	//정지영상에 변경된 옵션값을 새로 적용하기 위해서는 m_mat을 reload해서 영상처리 함수를 적용해야 한다.
	void			updateMainMat( Mat* mat = NULL, bool bReload = false, bool bSendToParent = true );


	//ROI 관련
	//opencv에서는 이미지에 roi를 설정하면 해당 영역에 대한 연산만 수행될 뿐 실제 mat의 크기가 변경되는 것은 아니지만
	//이 CVideoWnd에서는 roi, zoom에 따라 m_mat이 동적으로 변하게 된다.
	//즉, roi, zoom에 따라 화면에 보여지는 현재 영상이 항상 m_mat 이미지이다.
	CRect			m_rImageROI;			//영상의 실제 ROI
	CRect			m_rScreenROI;			//디스플레이되고 있는 화면상의 ROI
	CRect			m_rDisplayedImageRect;	//윈도우 창에 뿌려진 영상의 화면크기(roi설정 후 줌 된 영상 크기. zoom후에 polygon의 실제좌표 계산시 필요.
	CRect			GetDisplayedImageRect();
	bool			m_bLButtonDown;
	void			RecalcScreenROI();		//비디오 윈도우의 크기가 변경되면 설정된 screen ROI의 크기도 달라진다. 현재 사용 안함.
	CRect			GetImageROI() { return m_rImageROI; }
	void			SetImageROI( CRect roi );
	bool			CheckROIValidation( CRect &roi );
	void			CheckScreenROIValidation();	//roi 좌표값 유효성 검사 및 보정
	bool			IsROIExist();

	//ROI 변경 관련
	bool			m_bROI_Set;
	bool			m_bROI_Move;
	int				m_nROIResizeDirection;	//roi 이동 및 크기조정을 위한 커서 위치 판별. -1:out of range, 0:left, 1:top, 2:right, 3:bottom, 4:all
	int				m_nPlayState4ROIMove;	//roi 이동 시점의 플레이 상태값 백업.
	bool			m_bLButtonDown4ROIMove;
	CPoint			m_ptLButtonDown4ROIMove;

	//소실점 관련(화면에 4개의 점을 찍어서 소실점 좌표를 계산한다)
	bool			m_bGetVanishingPoint;
	bool			m_bShowVanishingPoint;			//default = false
	int				m_nVanishingPointCount;
	CPoint			m_ptVanishingPointClicked[4];
	//m_ptVanishingPoint는 비디오의 resize와 roi에 따라 자동 변환된다.
	//즉, FHD 원본에서 vp가 800, 400이었다면 1/2로 영상이 축소되면
	//vp는 자동으로 400, 200으로 변경된다.
	//또한 roi가 변경되면 거기에 맞게 자동 변경됨에 주의해야 한다.
	CPoint			m_ptVanishingPoint;				//default = CPoint(0, 0)
	CPoint			GetVanishingPoint() { return m_ptVanishingPoint; }
	void			SetVanishingPoint( int vx, int vy ) { m_ptVanishingPoint = CPoint(vx, vy); }
	void			ShowVanishingPoint( bool bShow = true ) { m_bShowVanishingPoint = bShow; }

	//이미지 치수 체크 모드
	bool			m_bMeasureSize;			//이미지 특정 영역의 크기 및 거리 측정 기능

	//CRect			
	void			ConvertCoordinateImage2Screen( CRect &r );
	void			ConvertCoordinateScreen2Image( CRect &r );

	//트랙바 관련
	bool			m_bTrackBarDown;

	//크기 및 길이 표시 기능 관련(Ctrl + drag)
	CRect			m_rSizeInfo;			//

	COLORREF		m_crBorder;				//-1 : no border
	int				m_nBorderWidth;
	void			SetBorder( COLORREF crBorder = 0, int width = 1 ) { m_crBorder = crBorder; m_nBorderWidth = width; }

	//폴더내의 연속된 파일 목록을 생성하여 "다음 파일 재생" 기능에 사용한다.
	bool			m_auto_next_video;		//다음 파일 재생을 자동으로 할지...(default = false)
	std::deque<CString> m_dqFileList;
	void			build_file_list();
	void			open_next_video( bool next = true );

public:
	CVideoWnd();
	virtual ~CVideoWnd();

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseHover(UINT nFlags, CPoint point);
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void PreSubclassWindow();
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg void OnActivateApp(BOOL bActive, DWORD dwThreadID);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
};


