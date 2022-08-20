#pragma once


// CShapeDialog dialog

class CShapeDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CShapeDialog)

public:
	CShapeDialog(UINT nIDTemplate, CWnd* pParent = NULL);   // standard constructor
	virtual ~CShapeDialog();

	//dlg의 shape이 될 png파일의 경로와 dlg의 투명도 값을 받는다.
	//dlg_alpha : 0(transparent) ~ 255(opaque)
	void		set_image(LPCTSTR shape_image_file, byte dlg_alpha = 255);

protected:
	byte		m_alpha;
	Image*		m_pImage;
	Image*		LoadImage(LPCTSTR lpszFile);
	
	void		Render();


// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SHAPEDIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
};
