#pragma once

/* CVideoWnd
[ 2017 08 09 ��������]
- ROI ���� ���� �� ����, �̵�, ũ�� ������ Shift, Ctrl�� �̿�������
  ���� App���� Shift, Ctrl�� �̿��ϴ� ��찡 ���� ������
  ���� ROI ���� ����� �˾��޴� ���� �׸����� ǥ���Ѵ�.
  * ROI ����, ROI ũ�� ����, ROI �̵�, ROI ����

[�⺻ ���]
- opencv�� �̿��� ���� ���� �Ǵ� ķ���� ���÷��̿� ������
- ������ ������ ����Ҽ���, �̹��� �����͸� ���޹޾� �̹����� ���÷��� �� ���� �ִ�.
- ������ ���� drag&drop ����.
- ROI ����, ����, ROI�� �̵� �� ũ������ ����.
- ctrl + drag�� ���� ������ ũ��� �밢�� ���� ǥ��.
- copy, paste ��� �߰�.
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
	int				m_nVideoWndID;			//�� �ۿ��� 2�� �̻��� CVideoWnd�� ����� ��� �޽��� ������ �� ����ID�� �̿���.
	void			SetVideoWndID( int nID ) { m_nVideoWndID = nID; }

	//����� Open�ÿ� ���Ե����� Open���� �ʰ� Ŭ������ �̹����� �ٿ��������� �����Ƿ� ������ �Ŀ� �� ���� ä�����־�� �Ѵ�.
	//�Ǵ� SetParentWnd()�� ���� �������ش�.
	HWND			m_hParentWnd;
	void			SetParentWnd( HWND hParent ) { m_hParentWnd = hParent; }

	bool			m_bAutoPlayWhenDrop;	//default = true
	void			SetAutoPlayWhenDrop( bool play ) { m_bAutoPlayWhenDrop = play; }

	//popup menu ����
	bool			m_bUsePopupMenu;
	bool			m_bPopupMenu;			//�˾��޴��� ���ִ� ��������...���콺 Ŀ�� ������ �ʿ�.
	bool			m_bSaveRecentFrame;		//������ ��� ������ ��ġ ���� ����. default = false
	int				m_nSaveIndex;
	bool			m_bImageFromClipboard;	//���� m_mat�� video frame image���� Ŭ�����忡�� �ٿ��ֱ� �� �̹�������...m_nSaveIndex�� ������ �ش�.
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
		id_menu_save_recent_frame,			//�ֱ� ��� ��ġ�� ������Ʈ���� ����ų ������. ���߿� ����ϸ� �� ��ġ���� ���.
		id_menu_current_vanishing_point,	//���� ������ �ҽ��� ��ǥ ������ ǥ�����ش�.
		id_menu_show_vanishing_point,		//���� ������ �ҽ��� ��ǥ ������ ǥ�����ش�.
		id_menu_get_vanishing_point,		//ȭ��� 4���� Ŭ���Ͽ� vp�� ����
		id_menu_set_vanishing_point,		//vp��ǥ�� ���� �Է���
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

	bool			m_bSendPlayWndInfo;		//��������� �������� ���� ���� �޽����� parent�� ��������...�⺻��=false
	void			SendPlayWndInfo( bool bUse = true ) { m_bSendPlayWndInfo = bUse; }

	bool			m_bUseCrossCursor;		//���� Ŀ���� �������..default = false
	HCURSOR			m_hCursor;
	void			UseCrossCursor( bool bUse = true ) { m_bUseCrossCursor = bUse; }

	bool			m_bUsePatternBackground;//pattern�� ����� ������� solid ������ �������. default = false
	void			UsePatternBackground( bool bUse ) { m_bUsePatternBackground = bUse; }
	COLORREF		m_crBackground;
	unsigned char	m_Bitmap_Pattern[BMPX * BMPX];	//non-image ��濡 ǥ���� ȸ�� ���� ���� ��Ʈ�� ������


	//infotext ����
	CString			m_sInfoText[4];							//lt, rt, lb, rb 4���� �ؽ�Ʈ ���� ��¿�
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

//���� ���� �� ���� ���� ����
	int				m_nFileType;
	int				m_cam_index;
	int				GetFileType() { return m_nFileType; }
	CString			m_sVideoFileName;
	CString			GetVideoFileName() { return m_sVideoFileName; }
	void			SetVideoFileName( CString sName ) { m_sVideoFileName = sName; SetWindowText(sName); }	//���� �̹����� ������ ���� �ʾ����� ������ Ÿ��Ʋ�� ������ ��� �ʿ�

	//bin ������ �� stereo video ����
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
	uint8_t			*m_yuv_origin;		//stereo ���󿡼� left ������ ���� yuv ������
	int				m_yuv_format;
	void			set_bin_size( int w, int h ) { m_nBinWidth = w; m_nBinHeight = h; }
	void			stereo_video( bool stereo ) { m_bStereoBin = stereo; }

	//sfile = "" �϶� bRunFileDialog = true�̸� ���Ͽ��� â�� ����� �Է¹޴´�. �̶� �⺻ ������ sRecentFile�� �ִ� ������ �ȴ�.
	bool			OpenVideoFile( CString sfile, bool bRunFileDialog = true, CString sRecentFile = _T(""), bool bPlay = true );
	bool			open_cam(int index, bool auto_start = true);
	void			CloseVideoFile();
	//���������� ���� ������ ������ ��쿡 ���ϸ� ������ ����ǹǷ� �̸� �̿��� ������ ������ ���� �������� üũ�Ѵ�.
	bool			IsVideoFileOpened() { return (m_sVideoFileName != ""); }

	VideoCapture	m_capture;
	Mat				m_matOrigin;								//�������� ������ ������
	Mat				m_mat;										//ó���� ���� ��� �Ǵ� �߶��� ���� ����
	Mat				GetNonFilteredMat();						//resize ratio�� roi�� ����� �ҽ� ������ ����.

	CSize			m_szImageSizeOrigin;						//���� ������ ũ��
	CSize			m_szImageSize;								//ó���� ���� resize�� ũ��. roi�� ũ�Ⱑ �ƴ� Ȯ��, ��ҵ� ������ ũ����.
	CSize			GetImageSizeOrigin();
	CSize			GetImageSize();
	double			GetImageRatio();							//cols / rows

	//���� ������ ���� Ȯ�� �Ǵ� ����ؼ� ó�������� ����(percentage). default = 1.0
	double			m_dResizeRatio;
	int				m_nInterpolation;							//opencv resize policy. cv default = INTER_LINEAR
	int				GetInterpolationMode() { return m_nInterpolation; }
	void			SetInterpolationMode( int mode ) { m_nInterpolation = mode; }
	double			GetResizeRatio() { return m_dResizeRatio; }
	void			SetResizeRatio( double dRatio );			

	bool			ReadFrameFromBinFile();						//bin ���Ͽ��� ���� ������ ������ ��� m_mat�� �־��ش�.
	void			Clear() { m_mat.release(); Invalidate(); }	//���� ���� �̹����� m_mat�� release�ϰ� ȭ�� ����.

	//��� ����
	bool			m_do_not_image_processing = false;
	bool			m_bRepeat;									//�ݺ� ��� ����. �⺻�� false.
	void			Repeat( bool bRepeat = true ) { m_bRepeat = bRepeat; }
	void			Play( int nState );
	int				m_nPlayState;
	int				GetPlayState() { return m_nPlayState; }
	bool			m_bReverse;
	void			SetReversePlay( bool bReverse ) { m_bReverse = bReverse; }
	void			ToggleReversePlay() { m_bReverse = !m_bReverse; }

	//��� fps ����
	double			m_VideoFPS;	//������ ���ڵ��� ���� FPS
	double			GetVideoFPS() { return m_VideoFPS; }

	//ó���ӵ� ����
	//CVideoWnd ���ο��� FPS�� ����ؼ� ��������(true),
	//���� ����ó�� ��ƾ���� ������ ����ؼ� FPS�� ��������(false)...�⺻��=true;
	//���ο��� ó���ϸ� �������� ������ �� �̹��� ó�� ������ ��ġ�� ���� �������� �����ϱ� �������� �ð��� ����ϹǷ�
	//���÷��̳� �ٸ� �ΰ����� �ڵ���� ����ð��� ��� ���Եȴ�.
	//���� ���� �̹��� ó�� �������� fps�� ���ϰ��� �� ��쿡�� �� ���� false�� �����ϰ�
	//���� �̹��� ó�� ��ƾ���� fps�� ���ؼ� ����Ѵ�.
protected:
	double			m_dCurrentFPS;
	double			m_dCurrentProcessingTime;
	double			m_dTime0, m_dTime1;
	bool			m_bUseOwnFPS;

public:
	void			UseOwnFPS( bool bUse ) { m_bUseOwnFPS = bUse; }
	double			GetCurrentFPS() { return m_dCurrentFPS; }
	double			GetCurrentProcessingTime() { return m_dCurrentProcessingTime; }
	//���� fps�� infoIndex ��ġ�� ǥ���Ѵ�. infoIndex = -1�̸� fps�� �����Ѵ�.
	double			calcFPS(double t0, double t1, int infoIndex = 1);

//������ ����
	//�̹��� ���ϵ��� ������ �� ������ó�� ó���� ��� true. �⺻�� = false.
	//����ϴ� ������Ʈ�� ������ �°� true �Ǵ� false�� �������ش�.
	bool			m_bUseImageArray;
	std::deque<CString>	m_dqImageFiles;		//�̹��� ���� ���
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

	//�������� �ƴ� �̹��� ����Ʈ ������ �˰� �ִٸ� 
	void			GotoFrame(int nFrame);
	void			GotoNextFrame(bool bNext, int nStep = 1, int newState = NO_CHANGE );
	void			GotoNextFrameAr(CStringArray *ar, int *nCurrent, bool bNext = true, int nStep = 1, int newState = NO_CHANGE);

	//mat�� m_mat�� �����Ͽ� ȭ�鿡 ǥ���Ѵ�. �� ���� NULL�� ���� �׳� ���� m_mat�� ���Ž����ִ� Invalidate()������ ȣ���Ѵ�.
	//filter ����� ���ð��� ����Ǹ� �̹��� ������ ���� updateMainMat�� ȣ���ϴµ�
	//�̹� m_mat�� �̹� filter�� ����� �̹��� ������ ���
	//�������� ����� �ɼǰ��� ���� �����ϱ� ���ؼ��� m_mat�� reload�ؼ� ����ó�� �Լ��� �����ؾ� �Ѵ�.
	void			updateMainMat( Mat* mat = NULL, bool bReload = false, bool bSendToParent = true );


	//ROI ����
	//opencv������ �̹����� roi�� �����ϸ� �ش� ������ ���� ���길 ����� �� ���� mat�� ũ�Ⱑ ����Ǵ� ���� �ƴ�����
	//�� CVideoWnd������ roi, zoom�� ���� m_mat�� �������� ���ϰ� �ȴ�.
	//��, roi, zoom�� ���� ȭ�鿡 �������� ���� ������ �׻� m_mat �̹����̴�.
	CRect			m_rImageROI;			//������ ���� ROI
	CRect			m_rScreenROI;			//���÷��̵ǰ� �ִ� ȭ����� ROI
	CRect			m_rDisplayedImageRect;	//������ â�� �ѷ��� ������ ȭ��ũ��(roi���� �� �� �� ���� ũ��. zoom�Ŀ� polygon�� ������ǥ ���� �ʿ�.
	CRect			GetDisplayedImageRect();
	bool			m_bLButtonDown;
	void			RecalcScreenROI();		//���� �������� ũ�Ⱑ ����Ǹ� ������ screen ROI�� ũ�⵵ �޶�����. ���� ��� ����.
	CRect			GetImageROI() { return m_rImageROI; }
	void			SetImageROI( CRect roi );
	bool			CheckROIValidation( CRect &roi );
	void			CheckScreenROIValidation();	//roi ��ǥ�� ��ȿ�� �˻� �� ����
	bool			IsROIExist();

	//ROI ���� ����
	bool			m_bROI_Set;
	bool			m_bROI_Move;
	int				m_nROIResizeDirection;	//roi �̵� �� ũ�������� ���� Ŀ�� ��ġ �Ǻ�. -1:out of range, 0:left, 1:top, 2:right, 3:bottom, 4:all
	int				m_nPlayState4ROIMove;	//roi �̵� ������ �÷��� ���°� ���.
	bool			m_bLButtonDown4ROIMove;
	CPoint			m_ptLButtonDown4ROIMove;

	//�ҽ��� ����(ȭ�鿡 4���� ���� �� �ҽ��� ��ǥ�� ����Ѵ�)
	bool			m_bGetVanishingPoint;
	bool			m_bShowVanishingPoint;			//default = false
	int				m_nVanishingPointCount;
	CPoint			m_ptVanishingPointClicked[4];
	//m_ptVanishingPoint�� ������ resize�� roi�� ���� �ڵ� ��ȯ�ȴ�.
	//��, FHD �������� vp�� 800, 400�̾��ٸ� 1/2�� ������ ��ҵǸ�
	//vp�� �ڵ����� 400, 200���� ����ȴ�.
	//���� roi�� ����Ǹ� �ű⿡ �°� �ڵ� ����ʿ� �����ؾ� �Ѵ�.
	CPoint			m_ptVanishingPoint;				//default = CPoint(0, 0)
	CPoint			GetVanishingPoint() { return m_ptVanishingPoint; }
	void			SetVanishingPoint( int vx, int vy ) { m_ptVanishingPoint = CPoint(vx, vy); }
	void			ShowVanishingPoint( bool bShow = true ) { m_bShowVanishingPoint = bShow; }

	//�̹��� ġ�� üũ ���
	bool			m_bMeasureSize;			//�̹��� Ư�� ������ ũ�� �� �Ÿ� ���� ���

	//CRect			
	void			ConvertCoordinateImage2Screen( CRect &r );
	void			ConvertCoordinateScreen2Image( CRect &r );

	//Ʈ���� ����
	bool			m_bTrackBarDown;

	//ũ�� �� ���� ǥ�� ��� ����(Ctrl + drag)
	CRect			m_rSizeInfo;			//

	COLORREF		m_crBorder;				//-1 : no border
	int				m_nBorderWidth;
	void			SetBorder( COLORREF crBorder = 0, int width = 1 ) { m_crBorder = crBorder; m_nBorderWidth = width; }

	//�������� ���ӵ� ���� ����� �����Ͽ� "���� ���� ���" ��ɿ� ����Ѵ�.
	bool			m_auto_next_video;		//���� ���� ����� �ڵ����� ����...(default = false)
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


