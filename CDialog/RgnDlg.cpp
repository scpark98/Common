// RGNDlg.cpp : implementation file
//

#include "stdafx.h"
#include "RGNDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRGNDlg dialog


CRGNDlg::CRGNDlg(UINT nIDTemplate,CWnd* pParent,CString rgnfile)
		: CDialog(nIDTemplate, pParent)
{
	//{{AFX_DATA_INIT(CRGNDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_bTrackMove = FALSE;
	m_rgnfile = rgnfile;	
}
CRGNDlg::~CRGNDlg()
{	
}



void CRGNDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRGNDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRGNDlg, CDialog)
	//{{AFX_MSG_MAP(CRGNDlg)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRGNDlg message handlers



BOOL CRGNDlg::OnInitDialog() 
{
	
	
	
	HRSRC hResInfo;
    HGLOBAL hGlobal;
    hResInfo = FindResource(AfxGetInstanceHandle(), m_rgnfile,"RGN");
    hGlobal = LoadResource(NULL,hResInfo); 
    if (hGlobal) {
     BYTE *rgndata = (BYTE FAR*)LockResource(hGlobal);       	 
	 if (rgndata) {
 	  HRGN rgn;	  
	  m_dialogrgn = ExtCreateRegion(NULL,sizeof(RGNDATAHEADER) + (sizeof(RECT) * ((RGNDATA*)rgndata)->rdh.nCount),(RGNDATA*)rgndata);
	  VERIFY(m_dialogrgn!=NULL); // if you want more comprehensive checking - feel free!
	  rgn = ExtCreateRegion(NULL, sizeof(RGNDATAHEADER) + (sizeof(RECT) * ((RGNDATA*)rgndata)->rdh.nCount),(RGNDATA*)rgndata);
      VERIFY(rgn!=NULL);  // if you want more comprehensive checking - feel free!
	  ::SetWindowRgn(m_hWnd, rgn, TRUE);
	  ::UnlockResource(hGlobal);
	 }
	}
    
    if(hGlobal) ::FreeResource(hGlobal);


 

	CDialog::OnInitDialog();	
	
	return TRUE; 
}

BOOL CRGNDlg::OnEraseBkgnd(CDC* pDC) 
{
	return TRUE;

}

void CRGNDlg::InvertTracker(CPoint point)
{

	//just invert outline - slow for complex regions
	HRGN rgn1;
	rgn1 = CreateRectRgn(0, 0,1,1);
	CBrush b;
	b.CreateSolidBrush(RGB(0,0,255));
	CombineRgn(rgn1,m_dialogrgn,NULL,RGN_COPY);
	OffsetRgn(rgn1,point.x-m_ptMouse.x,point.y-m_ptMouse.y);
	SetROP2(m_dc.m_hDC,R2_NOT);	
	FrameRgn(m_dc.m_hDC,rgn1,(HBRUSH)b.m_hObject,2,2);
	b.DeleteObject();	
	DeleteObject(rgn1);	
	
	//otherwise invert entire region - fast
	/*HRGN rgn1;
	rgn1 = CreateRectRgn(0, 0,1,1);
	CBrush b;
	b.CreateSolidBrush(RGB(0,0,255));
	CombineRgn(rgn1,m_dialogrgn,NULL,RGN_COPY);
	OffsetRgn(rgn1,point.x-m_ptMouse.x,point.y-m_ptMouse.y);
	SetROP2(m_dc.m_hDC,R2_NOT);	
	FrameRgn(m_dc.m_hDC,rgn1,(HBRUSH)b.m_hObject,2,2);
	b.DeleteObject();	
	DeleteObject(rgn1);		
	*/

}


void CRGNDlg::OnLButtonDown(UINT nFlags, CPoint point) 
{
/*
   m_dc.Attach(::GetDC(NULL));
   m_bTrackMove = TRUE;
   m_ptMouse = point;
   SetCapture();
   ClientToScreen(&point);
   InvertTracker(point);
   m_ptLast = point;
*/
}

void CRGNDlg::OnLButtonUp(UINT nFlags, CPoint point) 
{
	if (m_bTrackMove)
	{
		m_bTrackMove=FALSE;
		ReleaseCapture();
		InvertTracker(m_ptLast);
		::ReleaseDC(NULL,m_dc.Detach());		
		ClientToScreen(&point);
		SetWindowPos(NULL, point.x-m_ptMouse.x, point.y-m_ptMouse.y,0,0,
				SWP_NOZORDER|SWP_NOSIZE);
		ShowWindow(SW_SHOW);
	}
	else CDialog::OnLButtonUp(nFlags, point);
}

void CRGNDlg::OnMouseMove(UINT nFlags, CPoint point) 
{
	if (m_bTrackMove)
	{
		ClientToScreen(&point);
		InvertTracker(m_ptLast);
		m_ptLast = point;
		InvertTracker(m_ptLast);
	}
	else  CDialog::OnMouseMove(nFlags, point);
}

void CRGNDlg::OnPaint() 
{
   CPaintDC dc(this); // device context for painting
}


