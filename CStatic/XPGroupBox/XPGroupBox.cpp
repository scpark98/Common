// XPGroupBox.cpp : implementation file
//

#include "XPGroupBox.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNAMIC(CXPGroupBox,CButton);

CXPGroupBox::CXPGroupBox()
{
}

CXPGroupBox::~CXPGroupBox()
{
}


BEGIN_MESSAGE_MAP(CXPGroupBox, CButton)
	//{{AFX_MSG_MAP(CXPGroupBox)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()



/////////////////////////////////////////////////////////////////////////////
// CXPGroupBox message handlers

void CXPGroupBox::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	// TODO: Add your message handler code here
   	CRect	rc;
	GetClientRect(rc);
	
	Gdiplus::Graphics g(dc.m_hDC);
	g.SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);

	// Defalte Rect
	//rc.DeflateRect(1,1);
	
	// Get Text Rect 
	CSize sizeText;
	CRect rectText, rectFrame;
	CRect rectContent;
	
	CFont *pOldFont = dc.SelectObject(&m_font);
  		
	// get Text if need
	if (m_title.IsEmpty())
	{ 
        GetWindowText(m_title);
		if (!m_title.IsEmpty())
			m_title = _T("  ") + m_title + _T("  ");	//좌우 여백 추가
	}
	
	if (!m_title.IsEmpty())
	{
		sizeText = dc.GetTextExtent(m_title);
		sizeText.cy++;
	}
	else
	{
		sizeText.cx = 0;
		sizeText.cy = 0;
	}
	
	if (m_type == XPGB_FRAME) // Frame style
	{
		// Calculate Text Rect 
		switch(m_alignment)
		{
		case SS_LEFT:	
			rectText.top = rc.top;
			rectText.left = rc.left + 10;
			
			rectText.bottom = rectText.top + sizeText.cy;
			rectText.right = rectText.left + sizeText.cx;
			break;
		case SS_CENTER:	
			rectText.top = rc.top;
			rectText.left = rc.left + (rc.Width() - sizeText.cx) / 2 ;
			
			rectText.bottom = rectText.top + sizeText.cy;
			rectText.right = rectText.left + sizeText.cx;
			break;
		case SS_RIGHT	:
			rectText.top = rc.top;
			rectText.right = rc.right - 10 ;
			
			rectText.bottom = rectText.top + sizeText.cy;
			rectText.left = rectText.right - sizeText.cx;
			break;
		}
		
		//  Calculate Frame rect
		rectFrame.left = rc.left;
		rectFrame.top = rc.top + sizeText.cy / 2;
		
		rectFrame.right = rc.right;
		rectFrame.bottom = rectFrame.top + rc.Height() - sizeText.cy / 2; 
		
		//dc.FillSolidRect(rc, m_crBase);

		// Draw Frame border
		CPen penFrame;
		CBrush brushBKFrame(m_cr_title_back.ToCOLORREF());
		
		penFrame.CreatePen(PS_SOLID, 1, m_cr_border.ToCOLORREF());
		
		CPen* pOldPen = dc.SelectObject(&penFrame);
		CBrush* pOldBrush = (CBrush*)dc.SelectStockObject(NULL_BRUSH);
		
		dc.RoundRect(rectFrame, CPoint(10,10)); 
		
		dc.SelectObject(pOldPen);
		dc.SelectObject(pOldBrush); 
		
		dc.IntersectClipRect(rectText);
		dc.FillSolidRect(rectText, m_cr_title_back.ToCOLORREF());
	}
	else  // Windows Style
	{
		// Calculate Title size
		m_rt_title.top = rc.top;
		m_rt_title.left = rc.left;
		
		m_rt_title.right = rc.right;
		m_rt_title.bottom = rc.top + sizeText.cy + 4;
		
		//dc.FillSolidRect(rc, m_crBase);

		// Draw Title round rect
		CPen penFrame;
		CBrush brushBKTitle(m_cr_title_back.ToCOLORREF());
		CBrush brushBKContent(m_cr_client_back.ToCOLORREF());
		

		penFrame.CreatePen(PS_SOLID, 1, m_cr_border.ToCOLORREF());
		
		CPen* pOldPen = dc.SelectObject(&penFrame);
		//CBrush* pOldBrush = dc.SelectObject(&brushBKTitle);
		CBrush* pOldBrush = (CBrush*)dc.SelectStockObject(NULL_BRUSH);
		
		dc.RoundRect(rc, CPoint(10, 10)); 
		
		dc.SelectObject(pOldBrush); 
		
		// Draw content area
		rectContent.left = rc.left;
		rectContent.top = rc.top + sizeText.cy + 4;
		
		rectContent.right = rc.right;
		rectContent.bottom = rectContent.top + rc.Height() - sizeText.cy - 4; 
		
		//pOldBrush = dc.SelectObject(&brushBKContent); 
		pOldBrush = (CBrush*)dc.SelectStockObject(NULL_BRUSH);
		
		dc.Rectangle(rectContent);  
		
		dc.SelectObject(pOldPen);
		dc.SelectObject(pOldBrush); 
		
		
		// Calculate Text Rect 
		switch(m_alignment)
		{
		case SS_LEFT:	
			rectText.top = m_rt_title.top + 2;
			rectText.left = m_rt_title.left + 2;
			
			rectText.bottom = rectText.top + sizeText.cy;
			rectText.right = rectText.left + sizeText.cx ;
			break;
		case	SS_CENTER:	
			rectText.top = m_rt_title.top + 2;
			rectText.left = m_rt_title.left + (m_rt_title.Width() - sizeText.cx) / 2;
			
			rectText.bottom = rectText.top + sizeText.cy;
			rectText.right = rectText.left + sizeText.cx;
			break;
		case	SS_RIGHT	:
			rectText.top = m_rt_title.top + 2;
			rectText.right = rc.right - 2;
			
			rectText.bottom = rectText.top + sizeText.cy;
			rectText.left = rectText.right - sizeText.cx;
			break;
		}
		
		
	}
    
	COLORREF clrOldText = dc.SetTextColor(m_cr_title_text.ToCOLORREF());
	UINT nMode = dc.SetBkMode(TRANSPARENT);
	
	dc.DrawText(m_title, &rectText, DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_NOCLIP); //DT_END_ELLIPSIS);
	
	// restore DC
	dc.SetBkMode(nMode);
    dc.SetTextColor(clrOldText);
    dc.SelectObject(pOldFont);
}

BOOL CXPGroupBox::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext) 
{
	// TODO: Add your specialized code here and/or call the base class
	
	dwStyle |= BS_ICON;

	return CWnd::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);
}

BOOL CXPGroupBox::PreCreateWindow(CREATESTRUCT& cs) 
{
	// TODO: Add your specialized code here and/or call the base class
	cs.style |= BS_ICON;
	return CButton::PreCreateWindow(cs);
}

void CXPGroupBox::PreSubclassWindow() 
{
	// TODO: Add your specialized code here and/or call the base class
	
	CButton::PreSubclassWindow();

	//modified the style to avoid text overlap when press tab 
	ModifyStyle(0, BS_ICON);

	// Get Defalut Font 
	CFont* cf = GetFont();
	if(cf !=NULL)
	{
		cf->GetObject(sizeof(m_lf),&m_lf);
	}
	else
	{
		GetObject(GetStockObject(SYSTEM_FONT),sizeof(m_lf),&m_lf);
	}

	reconstruct_font();

	DWORD dwAlign = GetStyle();
	if ((dwAlign & BS_CENTER) == BS_CENTER)
		m_alignment = SS_CENTER;
	else if (dwAlign & BS_LEFT)
		m_alignment = SS_LEFT;
	else if (dwAlign & BS_RIGHT)
		m_alignment = SS_RIGHT;
}

void CXPGroupBox::reconstruct_font()
{
	m_font.DeleteObject();
	BOOL bCreated = m_font.CreateFontIndirect(&m_lf);

	ASSERT(bCreated);
}

void CXPGroupBox::update_surface()
{
	CRect (rc);
	GetWindowRect(rc);
	RedrawWindow();

	GetParent()->ScreenToClient(rc);
	GetParent()->InvalidateRect(rc,false);
	GetParent()->UpdateWindow();
}

CXPGroupBox& CXPGroupBox::set_XPGroup_style(XPGroupBoxStyle style)
{
   m_type = style;
   update_surface();
   return *this;
}

CXPGroupBox& CXPGroupBox::set_font(LOGFONT lf)
{
	CopyMemory(&m_lf, &lf, sizeof(m_lf));
	reconstruct_font();
	update_surface();
	return *this;
}

CXPGroupBox& CXPGroupBox::set_font_bold(bool bold)
{
	m_lf.lfWeight = bold ? FW_BOLD : FW_NORMAL;
	reconstruct_font();
	update_surface();
	return *this;
}

CXPGroupBox& CXPGroupBox::set_font_name(const CString& font_name, BYTE byCharSet)
{
	m_lf.lfCharSet = byCharSet;

	_tcscpy_s(m_lf.lfFaceName, _countof(m_lf.lfFaceName), font_name);
	reconstruct_font();
	update_surface();

	return *this;
}

CXPGroupBox& CXPGroupBox::set_font_underline(bool underline)
{
	m_lf.lfUnderline = underline;
	reconstruct_font();
	update_surface();

	return *this;
}

CXPGroupBox& CXPGroupBox::set_font_italic(bool italic)
{
	m_lf.lfItalic = italic;
	reconstruct_font();
	update_surface();

	return *this;	
}

CXPGroupBox& CXPGroupBox::set_font_size(int nSize)
{
	CFont cf;
	LOGFONT lf;

	cf.CreatePointFont(nSize * 10, m_lf.lfFaceName);
	cf.GetLogFont(&lf);

	m_lf.lfHeight = lf.lfHeight;
	m_lf.lfWidth  = lf.lfWidth;

	reconstruct_font();
	update_surface();

	return *this;
}

//clrBase : 이 컨트롤이 그려질 parent의 배경색
CXPGroupBox& CXPGroupBox::set_base_color(Gdiplus::Color cr_base)
{
	m_crBase = cr_base;

	return *this;
}

CXPGroupBox& CXPGroupBox::set_border_color(Gdiplus::Color cr_border)
{
	m_cr_border = cr_border;
	update_surface();
	return *this;
}

CXPGroupBox& CXPGroupBox::set_title_text_color(Gdiplus::Color cr_text)
{
	m_cr_title_text = cr_text;
	update_surface();
	return *this;
}

CXPGroupBox& CXPGroupBox::set_back_color(Gdiplus::Color cr_back_client)
{
	m_cr_title_back = cr_back_client;
	m_cr_client_back = cr_back_client;
	update_surface();
	return *this;
}

CXPGroupBox& CXPGroupBox::set_back_color(Gdiplus::Color cr_back_title, Gdiplus::Color cr_back_client)
{
	m_cr_title_back = cr_back_title;
	m_cr_client_back = cr_back_client;
	update_surface();
	return *this;
}

CXPGroupBox& CXPGroupBox::set_text(LPCTSTR lpszText)
{
	if(IsWindow(this->GetSafeHwnd())) 
	{
		m_title = lpszText;
		m_title = _T(" ") + m_title + _T(" ");
		InvalidateRect(m_rt_title, false);
	}
	
	return *this;
}


CXPGroupBox& CXPGroupBox::set_alignment(DWORD dwType)
{
	switch(dwType)
	{
		case SS_LEFT :
			m_alignment = SS_LEFT;
			break;
		case SS_CENTER :	
			m_alignment = SS_CENTER;
			break;
		case SS_RIGHT :
			m_alignment = SS_RIGHT;
			break;
		default	:
			ASSERT(false);
	}

	update_surface();
	
	return *this;
}


