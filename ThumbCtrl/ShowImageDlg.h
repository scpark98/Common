#pragma once

#ifndef DIB_WIDTHBYTES
#define		DIB_WIDTHBYTES(bits)	(((bits) + 31) / 32 * 4)	//bits is not width, but (width * bitCount)
#endif

#define		MAKE4WIDTH(width)		( (width + 3) & ~3 )		//width�� 4�� ����� ������ش�.

// CShowImageDlg ��ȭ �����Դϴ�.

class CShowImageDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CShowImageDlg)

public:
	CShowImageDlg(CWnd* pParent = NULL);   // ǥ�� �������Դϴ�.
	virtual ~CShowImageDlg();

// ��ȭ ���� �������Դϴ�.
	//enum { IDD = IDD_SHOW_IMAGE };
	void		SetImageData( BYTE* pImage, int w, int h, int ch = 3, CString sTitle = _T("") );

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �����Դϴ�.

	BITMAPINFO	m_bmpInfo;

	//image data
	BYTE*		m_pImage;
	int			m_nWidth;
	int			m_nHeight;
	int			m_nChannel;

	double		m_dZoom;

	CPoint		m_ptStart;
	bool		m_bDrag;
	CPoint		m_ptDrag;

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnDestroy();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
};
