#if !defined(AFX_SCSTATIC_INCLUDED_)
#define AFX_SCSTATIC_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#pragma once

#include <afxwin.h>
#include <deque>
/*
//scpark

[gradient usage]
m_static1.SetGradient();
m_static1.SetGradientColor(RED);
//m_static1.SetVerticalGradient();
//m_static1.SetGradientColor(4, RED, GREEN, YELLOW, BLUE);

[������ ����]
- ����� �ִ� �ۿ����� ������ �� ��������� �⺻ ��Ÿ���� dlg������ ȭ�� ������ �� ���� �ʴ´�.
  �켱 �⺻ ��Ÿ���� dlg������ ������ ����������.
- ���ũ�⺸�� rc�� ������ Ű����� �Ѵ�.

[2014-11-27]
- blink�ÿ� �����ܵ� �Բ� ������� �ʴ� ���� ����
- blink�� false�� �� ��� ������ ���·� blink�� ����Ǵ� ���� ����

*/

// CSCStatic

typedef UINT (CALLBACK* LPFNDLLFUNC1)(HDC,CONST PTRIVERTEX,DWORD,CONST PVOID,DWORD,DWORD);

class CSCStatic : public CStatic
{
	DECLARE_DYNAMIC(CSCStatic)

public:
	CSCStatic();
	~CSCStatic();
	
	void		SetTransparent(bool bTransparent = true) { m_bTransparent = bTransparent; Invalidate(); }
	void		SetText(CString sText, COLORREF crTextColor = 0);
	CString		GetText() { return m_sText; }
	void		SetWindowText(CString sText) { SetText(sText); }
	COLORREF	get_back_color() { return m_crBack; }

	//���ڻ��� ������ ���
	void		SetTextColor(COLORREF crTextColor);
	//������ �����ϸ� ���� �ɼ��� off��.
	void		SetBackColor(COLORREF cBackColor) { m_crBack = cBackColor; m_bTransparent = false; Invalidate(); }
	//SetColor�� ����ϸ� ���� �ɼ��� off��.
	void		SetColor(COLORREF cTextColor, COLORREF cBackColor = ::GetSysColor(COLOR_3DFACE))
				{
					m_crText = cTextColor;
					m_crBack = cBackColor;
					m_bTransparent = false;
					Invalidate(FALSE);
				}

	//gradient
	//m_crGradient deque�� ������� �ְ� n����Ͽ� gradient fill�� �ؼ� ǥ���Ѵ�.
	CSCStatic&		SetGradient(bool bGradient = true);
	CSCStatic&		SetGradientColor(COLORREF crGradient);			//���~crGradient 2�÷��� ǥ����.
	CSCStatic&		SetGradientColor(int idx, COLORREF crGradient);	//idx�� ������ crGradient�� ����. idx�� ������ idx���� crGradient�� ä��.
	CSCStatic&		SetGradientColor(int count, ...);
	CSCStatic&		AddGradientColor(COLORREF crGradient);			//���� �߰�
	CSCStatic&		InsertGradientColor(int idx, COLORREF crGradient);//idx��° ���� �߰�
	CSCStatic&		SetVerticalGradient(bool bVertical = true);

	void		sunken(bool sunken = true) { m_bSunken = sunken; Invalidate(); }


//marqueeȿ���� ���ؼ� ������ �׽�Ʈ�� �������� �ε巴�� ǥ���ϱ� ���ؼ���
//rc�� width �Ǵ� height ũ�⿡ ������ ������ m_crGradient�� �ڵ����� ä���� �Ŀ� �������Ѵ�.
//�ʿ伺�� �ֳ� �; �ϴ� �����Ѵ�.
	//start = 1:start, 0:stop, -1:toggle
	bool			m_start_marquee;
	//void			start_marquee(int start);

	void			SetBackImage(UINT nIDBack);

	void			SetRoundHead(bool bRound);

	//font
	virtual CSCStatic& SetFontName(const CString& strFont, BYTE byCharSet = DEFAULT_CHARSET);
	virtual CSCStatic& SetFontSize(int nFontSize);
	virtual CSCStatic& SetFontBold(bool bFontBold = true);
	virtual CSCStatic& SetFontUnderline(bool bSet);
	virtual CSCStatic& SetFontAntiAliased(bool bAntiAliased = true);

	void		SetFontWidth(int nFontWidth) { m_nFontWidth = nFontWidth; Invalidate(); }
	bool		IsBold() { return m_bFontBold; }

	void		SetOutlineWidth(int nWidth) { m_nOutlineWidth = nWidth; Invalidate(); }
	void		SetOutline(int nWidth, COLORREF crOutline) { m_nOutlineWidth = nWidth; m_crOutline = crOutline; Invalidate(); }

	void		SetBlinkTime(int nTime0 = 400, int nTime1 = 1200);
	void		SetBlink(BOOL bBlink = TRUE);
	void		SetTextAlignment(DWORD dwStyle) { m_dwStyle = dwStyle; }
	void		SetPrefixSpace(int nSpace = 1) { m_nPrefixSpace = nSpace; Invalidate(); }

	void		SetIcon(UINT nIDResource, int nSize = 16);

protected:
	enum ENUM_TIMER
	{
		TIMER_BLINK = 0,
		TIMER_MARQUEE,
	};

	CBitmap		m_Bmp;
	CString		m_sText;
	CString		m_sFontName;
	int			m_nFontSize;
	int			m_nFontWidth;
	bool		m_bFontBold;
	bool		m_bFontUnderline;
	bool		m_bFontAntiAliased;

	//gradient background fill
	std::deque<COLORREF> m_crGradient;
	bool		m_bGradient;		//gradient background?
	bool		m_bVertical;		//vertical gradient?


	//this function will be used only if msimg32.dll library is not available
	void		DrawGradientRect(CDC *pDC, CRect r, COLORREF cLeft, COLORREF cRight, BOOL a_bVertical);


 	int			m_nOutlineWidth;	//�β��� 1 �̻��̸� �ܰ������� �׷�����.
 	COLORREF	m_crOutline;		//�ܰ��� ����
	bool		m_bSunken;			//default = false;

	COLORREF	m_crText;
	COLORREF	m_crBack;
	bool		m_bTransparent = true;		//default = true

	BOOL		m_bBlink;
	BOOL		m_bBlinkStatus;
	int			m_nBlinkTime0;		//blink type is Show/Hide, time0 = shown duration, time1 = hidden duration in millisecond.
	int			m_nBlinkTime1;

	DWORD		m_dwStyle;
	int			m_nPrefixSpace;
	CImageList	m_ImageBack;


	LOGFONT		m_lf;
	CFont		m_font;
	void		UpdateSurface();
	void		ReconstructFont();


	HICON		m_hIcon;
	CSize		m_szIcon;


protected:
	//{{AFX_MSG(CSCStatic)
	afx_msg void OnPaint();
 	afx_msg LRESULT OnSetText(WPARAM,LPARAM);
	
// 	afx_msg HBRUSH CtlColor(CDC* /*pDC*/, UINT /*nCtlColor*/);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void PreSubclassWindow();

	//}}AFX_MSG
//	LRESULT		OnSetText(WPARAM wParam,LPARAM lParam);
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
};
#endif