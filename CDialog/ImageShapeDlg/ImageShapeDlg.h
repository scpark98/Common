#pragma once

#include <afxwin.h>
#include <afxdialogex.h>

/*
* png, gif 등의 이미지 또는 CGdiplusBitmap 이미지 모양대로 dlg를 표시한다.
* 위 목적의 dlg를 생성한 후 CDialogEx 대신 CImageShapeDlg를 상속받는다.
* class CPopupDlg : public CImageShapeDlg
* {
*		...
* }
* 
* => 불필요한 중간 dlg를 생략하고 create으로 직접 생성해서 사용하자!
* 
* //CPopupDlg::OnInitDialog()에서 이미지를 지정한다.
* load(_T("Z:\\내 드라이브\\media\\test_image\\progress\\checking.gif"));
* 
* 또는 CGdiplusBitmap 이미지를 생성한 후 render(Gdiplus::Bitmap* img)함수를 호출하여
* 이미지를 설정한다.
*/

#include "../../GdiplusBitmap.h"

// CImageShapeDlg 대화 상자

class CImageShapeDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CImageShapeDlg)

public:
	CImageShapeDlg();   // 표준 생성자입니다.
	virtual ~CImageShapeDlg();

	bool			create(CWnd* parent, int left, int top, int right, int bottom);
	
	//set_image()로 CGdiplusBitmap를 받는 경우는 반드시 deep_copy를 해야하지만
	//load()를 통해서 직접 로딩하여 m_img에 넣을 경우는 불필요하다.
	void			set_image(CGdiplusBitmap* img, bool deep_copy = true);
	bool			load(UINT id);
	bool			load(CString sType, UINT id);
	bool			load(CString sFile);

	//alpha = 0 ~ 255
	void			alpha(int alpha);

	//animated gif인 경우
	enum GIF_PLAY_STATE
	{
		state_stop = -1,
		state_pause,
		state_play,
		state_toggle,
	};

	int				m_gif_state = state_stop;
	int				m_gif_index = 0;

	void			gif_thread();
	void			gif_play(int new_state = state_play);
	void			gif_pause();
	void			gif_stop();
	void			gif_goto(int pos, bool pause = false);

protected:
	CWnd*			m_parent = NULL;

	CGdiplusBitmap	m_img;
	int				m_alpha = 255;
	void			render(Gdiplus::Bitmap* img);


protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
};
