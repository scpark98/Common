// XPGroupBox.cpp : implementation file
//

#include "XPGroupBox.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CXPGroupBox Version 1.0
//
//
// Thanks to:	NT ALMOND's class CLable.   
//
// Name                     Date        Version   Comments
// Jack Jin					2003-12-03    1.0       Origin
/////////////////////////////////////////////////////////////////////////////


IMPLEMENT_DYNAMIC(CXPGroupBox,CButton);

/////////////////////////////////////////////////////////////////////////////
// CXPGroupBox

//////////////////////////////////////////////////////////////////////////
//
// Function:		CXPGroupBox::CXPGroupBox
//
// Description:		Default contructor
//
// Name                     Date        Version Comments
// Jack Jin					2003-12-03    1.0     Origin
//
//////////////////////////////////////////////////////////////////////////
CXPGroupBox::CXPGroupBox()
{
}

//////////////////////////////////////////////////////////////////////////
//
// Function:		CXPGroupBox::~CXPGroupBox
//
// Description:		Default destructor
// Note:	Thanks John A. Johnson 
//
// Name                     Date        Version Comments
// Jack Jin					2003-12-03    1.0     Origin
// Jack Jin                 2003-12-03    1.01    fix paint problem with help of John A. Johnson   
//////////////////////////////////////////////////////////////////////////
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

//////////////////////////////////////////////////////////////////////////
//
// Function:		CXPGroupBox::OnPaint
//
// Description:		Handles all the drawing code for the label
//
// Name                     Date        Version Comments
// Jack Jin					2003-12-03    1.0     Origin
//////////////////////////////////////////////////////////////////////////
void CXPGroupBox::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	// TODO: Add your message handler code here
   	CRect	rectClient;
	GetClientRect(rectClient);
	
	// Defalte Rect
	//rectClient.DeflateRect(1,1);
	
	// Get Text Rect 
	CSize sizeText;
	CRect rectText, rectFrame;
	CRect rectContent;
	
	CFont *pOldFont = dc.SelectObject(&m_font);
  		
	// get Text if need
	if (m_strTitle.IsEmpty())
	{ 
        GetWindowText(m_strTitle);
		if (!m_strTitle.IsEmpty())
			m_strTitle = _T("  ") + m_strTitle + _T("  ");	//좌우 여백 추가
	}
	
	if (!m_strTitle.IsEmpty())
	{
		sizeText = dc.GetTextExtent(m_strTitle);
		sizeText.cy++;
	}
	else
	{
		sizeText.cx = 0;
		sizeText.cy = 0;
	}
	
	if (m_nType == XPGB_FRAME) // Frame style
	{
		// Calculate Text Rect 
		switch(m_dwAlignment)
		{
		case SS_LEFT:	
			rectText.top = rectClient.top;
			rectText.left = rectClient.left + 10;
			
			rectText.bottom = rectText.top + sizeText.cy;
			rectText.right = rectText.left + sizeText.cx;
			break;
		case SS_CENTER:	
			rectText.top = rectClient.top;
			rectText.left = rectClient.left + (rectClient.Width() - sizeText.cx) / 2 ;
			
			rectText.bottom = rectText.top + sizeText.cy;
			rectText.right = rectText.left + sizeText.cx;
			break;
		case SS_RIGHT	:
			rectText.top = rectClient.top;
			rectText.right = rectClient.right - 10 ;
			
			rectText.bottom = rectText.top + sizeText.cy;
			rectText.left = rectText.right - sizeText.cx;
			break;
		}
		
		//  Calculate Frame rect
		rectFrame.left = rectClient.left;
		rectFrame.top = rectClient.top + sizeText.cy / 2;
		
		rectFrame.right = rectClient.right;
		rectFrame.bottom = rectFrame.top + rectClient.Height() - sizeText.cy / 2; 
		
		//dc.FillSolidRect(rectClient, m_crBase);

		// Draw Frame border
		CPen penFrame;
		CBrush brushBKFrame(m_crTitleBack);
		
		penFrame.CreatePen(PS_SOLID, 1, m_clrBorder);
		
		CPen* pOldPen = dc.SelectObject(&penFrame);
		CBrush* pOldBrush = (CBrush*)dc.SelectStockObject(NULL_BRUSH);
		
		dc.RoundRect(rectFrame, CPoint(10,10)); 
		
		dc.SelectObject(pOldPen);
		dc.SelectObject(pOldBrush); 
		
		dc.IntersectClipRect(rectText);
		dc.FillSolidRect(rectText, m_crTitleBack);
	}
	else  // Windows Style
	{
		// Calculate Title size
		m_rectTitle.top = rectClient.top;
		m_rectTitle.left = rectClient.left;
		
		m_rectTitle.right = rectClient.right;
		m_rectTitle.bottom = rectClient.top + sizeText.cy + 4;
		
		//dc.FillSolidRect(rectClient, m_crBase);

		// Draw Title round rect
		CPen penFrame;
		CBrush brushBKTitle(m_crTitleBack);
		CBrush brushBKContent(m_crClientBack);
		

		penFrame.CreatePen(PS_SOLID, 1, m_clrBorder);
		
		CPen* pOldPen = dc.SelectObject(&penFrame);
		//CBrush* pOldBrush = dc.SelectObject(&brushBKTitle);
		CBrush* pOldBrush = (CBrush*)dc.SelectStockObject(NULL_BRUSH);
		
		dc.RoundRect(rectClient, CPoint(10, 10)); 
		
		dc.SelectObject(pOldBrush); 
		
		// Draw content area
		rectContent.left = rectClient.left;
		rectContent.top = rectClient.top + sizeText.cy + 4;
		
		rectContent.right = rectClient.right;
		rectContent.bottom = rectContent.top + rectClient.Height() - sizeText.cy - 4; 
		
		//pOldBrush = dc.SelectObject(&brushBKContent); 
		pOldBrush = (CBrush*)dc.SelectStockObject(NULL_BRUSH);
		
		dc.Rectangle(rectContent);  
		
		dc.SelectObject(pOldPen);
		dc.SelectObject(pOldBrush); 
		
		
		// Calculate Text Rect 
		switch(m_dwAlignment)
		{
		case SS_LEFT:	
			rectText.top = m_rectTitle.top + 2;
			rectText.left = m_rectTitle.left + 2;
			
			rectText.bottom = rectText.top + sizeText.cy;
			rectText.right = rectText.left + sizeText.cx ;
			break;
		case	SS_CENTER:	
			rectText.top = m_rectTitle.top + 2;
			rectText.left = m_rectTitle.left + (m_rectTitle.Width() - sizeText.cx) / 2;
			
			rectText.bottom = rectText.top + sizeText.cy;
			rectText.right = rectText.left + sizeText.cx;
			break;
		case	SS_RIGHT	:
			rectText.top = m_rectTitle.top + 2;
			rectText.right = rectClient.right - 2;
			
			rectText.bottom = rectText.top + sizeText.cy;
			rectText.left = rectText.right - sizeText.cx;
			break;
		}
		
		
	}
    
	COLORREF clrOldText = dc.SetTextColor(m_crTitleText);
	UINT nMode = dc.SetBkMode(TRANSPARENT);
	
	dc.DrawText(m_strTitle, &rectText, DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_NOCLIP); //DT_END_ELLIPSIS);
	
	// restore DC
	dc.SetBkMode(nMode);
    dc.SetTextColor(clrOldText);
    dc.SelectObject(pOldFont);
}

//////////////////////////////////////////////////////////////////////////
//
// Function:		CXPGroupBox::Create
//
// Description:		Modify the groupbox windows style 
//
// Name                     Date        Version Comments
// Jack Jin					2003-12-03    1.0     Origin
//////////////////////////////////////////////////////////////////////////
BOOL CXPGroupBox::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext) 
{
	// TODO: Add your specialized code here and/or call the base class
	
	dwStyle |= BS_ICON;

	return CWnd::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);
}

//////////////////////////////////////////////////////////////////////////
//
// Function:		CXPGroupBox::PreCreateWindow
//
// Description:		Modify the groupbox windows style 
//
// Name                     Date        Version Comments
// Jack Jin					2003-12-03    1.0     Origin
//////////////////////////////////////////////////////////////////////////
BOOL CXPGroupBox::PreCreateWindow(CREATESTRUCT& cs) 
{
	// TODO: Add your specialized code here and/or call the base class
	cs.style |= BS_ICON;
	return CButton::PreCreateWindow(cs);
}

//////////////////////////////////////////////////////////////////////////
//
// Function:		CXPGroupBox::PreSubclassWindow
//
// Description:		Modify the groupbox windows style 
//
//
// Name                     Date        Version Comments
// Jack Jin					2003-12-03    1.0     Origin
//////////////////////////////////////////////////////////////////////////

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

	ReconstructFont();


	DWORD dwAlign = GetStyle();
	if ((dwAlign & BS_CENTER) == BS_CENTER)
		m_dwAlignment = SS_CENTER;
	else if (dwAlign & BS_LEFT)
		m_dwAlignment = SS_LEFT;
	else if (dwAlign & BS_RIGHT)
		m_dwAlignment = SS_RIGHT;
}

//////////////////////////////////////////////////////////////////////////
//
// Function:		CXPGroupBox::ReconstructFont
//
// Description:		Construct font
//
//
// Name                     Date        Version Comments
// Jack Jin					2003-12-03    1.0     Origin
//////////////////////////////////////////////////////////////////////////
void CXPGroupBox::ReconstructFont()
{
	m_font.DeleteObject();
	BOOL bCreated = m_font.CreateFontIndirect(&m_lf);

	ASSERT(bCreated);
}

//////////////////////////////////////////////////////////////////////////
//
// Function:		CXPGroupBox::ReconstructFont
//
// Description:		Construct font
//
//
// Name                     Date        Version Comments
// Jack Jin					2003-12-03    1.0     Origin
//////////////////////////////////////////////////////////////////////////
void CXPGroupBox::UpdateSurface()
{
	CRect (rc);
	GetWindowRect(rc);
	RedrawWindow();

	GetParent()->ScreenToClient(rc);
	GetParent()->InvalidateRect(rc,false);
	GetParent()->UpdateWindow();
}

//////////////////////////////////////////////////////////////////////////
//
// Function:		CXPGroupBox::set_XPGroup_style
//
// Description:		Set Groupbox style
//
// INPUTS:          one of the XPGB_FRAME, XPGB_WINDOW 
// 
// RETURNS:         Reference to 'this' object
//
//
// Name                     Date        Version Comments
// Jack Jin					2003-12-03    1.0     Origin
//////////////////////////////////////////////////////////////////////////
CXPGroupBox& CXPGroupBox::set_XPGroup_style(XPGroupBoxStyle style)
{
   m_nType = style;
   UpdateSurface();
   return *this;
}

//////////////////////////////////////////////////////////////////////////
//
// Function:		CXPGroupBox::SetFont
//
// Description:		Sets font with LOGFONT structure
//
// INPUTS:          LOGFONT structure
// 
// RETURNS:         Reference to 'this' object
//
//
// Name                     Date        Version Comments
// Jack Jin					2003-12-03    1.0     Origin
//////////////////////////////////////////////////////////////////////////
CXPGroupBox& CXPGroupBox::SetFont(LOGFONT lf)
{
	CopyMemory(&m_lf, &lf, sizeof(m_lf));
	ReconstructFont();
	UpdateSurface();
	return *this;
}

//////////////////////////////////////////////////////////////////////////
//
// Function:		CXPGroupBox::set_font_bold
//
// Description:		Sets Font Style
//
// INPUTS:          TRUE of FALSE
// 
// RETURNS:         Reference to 'this' object
//
//
// Name                     Date        Version Comments
// Jack Jin					2003-12-03    1.0     Origin
//////////////////////////////////////////////////////////////////////////
CXPGroupBox& CXPGroupBox::set_font_bold(bool bold)
{
	m_lf.lfWeight = bold ? FW_BOLD : FW_NORMAL;
	ReconstructFont();
	UpdateSurface();
	return *this;
}

//////////////////////////////////////////////////////////////////////////
//
// Function:		CXPGroupBox::set_font_name
//
// Description:		Sets the fonts face name
//
// INPUTS:          String containing font name
// 
// RETURNS:         Reference to 'this' object
//
//
// Name                     Date        Version Comments
// Jack Jin					2003-12-03    1.0     Origin
//////////////////////////////////////////////////////////////////////////
CXPGroupBox& CXPGroupBox::set_font_name(const CString& font_name, BYTE byCharSet)
{
	m_lf.lfCharSet = byCharSet;

	_tcscpy_s(m_lf.lfFaceName, _countof(m_lf.lfFaceName), font_name);
	ReconstructFont();
	UpdateSurface();

	return *this;
}

//////////////////////////////////////////////////////////////////////////
//
// Function:		CXPGroupBox::set_font_underline
//
// Description:		Sets font underline attribue
//
// INPUTS:          True of false
// 
// RETURNS:         Reference to 'this' object
//
//
// Name                     Date        Version Comments
// Jack Jin					2003-12-03    1.0     Origin
//////////////////////////////////////////////////////////////////////////
CXPGroupBox& CXPGroupBox::set_font_underline(bool underline)
{
	m_lf.lfUnderline = underline;
	ReconstructFont();
	UpdateSurface();

	return *this;
}

//////////////////////////////////////////////////////////////////////////
//
// Function:		CXPGroupBox::set_font_italic
//
// Description:		Sets font italic attribue
//
// INPUTS:          True of false
// 
// RETURNS:         Reference to 'this' object
//
//
// Name                     Date        Version Comments
// Jack Jin					2003-12-03    1.0     Origin
//////////////////////////////////////////////////////////////////////////
CXPGroupBox& CXPGroupBox::set_font_italic(bool italic)
{
	m_lf.lfItalic = italic;
	ReconstructFont();
	UpdateSurface();

	return *this;	
}

//////////////////////////////////////////////////////////////////////////
//
// Function:		CXPGroupBox::SetFontSize
//
// Description:		Sets font size attribue
//
// INPUTS:          True of false
// 
// RETURNS:         Reference to 'this' object
//
//
// Name                     Date        Version Comments
// Jack Jin					2003-12-03    1.0     Origin
//////////////////////////////////////////////////////////////////////////
CXPGroupBox& CXPGroupBox::SetFontSize(int nSize)
{
	CFont cf;
	LOGFONT lf;

	cf.CreatePointFont(nSize * 10, m_lf.lfFaceName);
	cf.GetLogFont(&lf);

	m_lf.lfHeight = lf.lfHeight;
	m_lf.lfWidth  = lf.lfWidth;

	ReconstructFont();
	UpdateSurface();

	return *this;
}

//clrBase : 이 컨트롤이 그려질 parent의 배경색
CXPGroupBox& CXPGroupBox::set_base_color(COLORREF cr_base)
{
	m_crBase = cr_base;

	return *this;
}

//////////////////////////////////////////////////////////////////////////
//
// Function:		CXPGroupBox::set_border_color
//
// Description:		Set groupbox border Color
//
// INPUTS:          COLORREF clrBorder
// 
// RETURNS:         Reference to 'this' object
//
//
// Name                     Date        Version Comments
// Jack Jin					2003-12-03    1.0     Origin
//////////////////////////////////////////////////////////////////////////
CXPGroupBox& CXPGroupBox::set_border_color(COLORREF cr_border)
{
	m_clrBorder = cr_border;
	UpdateSurface();
	return *this;
}

//////////////////////////////////////////////////////////////////////////
//
// Function:		CXPGroupBox::set_title_text_color
//
// Description:		Set groupbox title color
//
// INPUTS:          COLORREF clrText
// 
// RETURNS:         Reference to 'this' object
//
//
// Name                     Date        Version Comments
// Jack Jin					2003-12-03    1.0     Origin
//////////////////////////////////////////////////////////////////////////
CXPGroupBox& CXPGroupBox::set_title_text_color(COLORREF cr_text)
{
	m_crTitleText = cr_text;
	UpdateSurface();
	return *this;
}

//////////////////////////////////////////////////////////////////////////
//
// Function:		CXPGroupBox::set_back_color
//
// Description:		Set groupbox backgroup color when group style is XPGB_FRAME
//
// INPUTS:          COLORREF clrBKClient
// 
// RETURNS:         Reference to 'this' object
//
//
// Name                     Date        Version Comments
// Jack Jin					2003-12-03    1.0     Origin
//////////////////////////////////////////////////////////////////////////
CXPGroupBox& CXPGroupBox::set_back_color(COLORREF cr_back_client)
{
	m_crTitleBack = cr_back_client;
	m_crClientBack = cr_back_client;
	UpdateSurface();
	return *this;
}

//////////////////////////////////////////////////////////////////////////
//
// Function:		CXPGroupBox::set_back_color
//
// Description:		Set groupbox backgroup color when group style is XPGB_WINDOW
//
// INPUTS:          COLORREF clrBKTilte,  COLORREF clrBKClient
// 
// RETURNS:         Reference to 'this' object
//
//
// Name                     Date        Version Comments
// Jack Jin					2003-12-03    1.0     Origin
//////////////////////////////////////////////////////////////////////////
CXPGroupBox& CXPGroupBox::set_back_color(COLORREF cr_back_title,  COLORREF cr_back_client)
{
	m_crTitleBack = cr_back_title;
	m_crClientBack = cr_back_client;
	UpdateSurface();
	return *this;
}

//////////////////////////////////////////////////////////////////////////
//
// Function:		CXPGroupBox::SetText
//
// Description:		Set groupbox title
//
// INPUTS:          Text to use
// 
// RETURNS:         Reference to 'this' object
//
//
// Name                     Date        Version Comments
// Jack Jin					2003-12-03    1.0     Origin
//////////////////////////////////////////////////////////////////////////
CXPGroupBox& CXPGroupBox::set_text(LPCTSTR lpszText)
{
	if(IsWindow(this->GetSafeHwnd())) 
	{
		m_strTitle = lpszText;
		m_strTitle = _T(" ") + m_strTitle + _T(" ");
		InvalidateRect(m_rectTitle, false);
	}
	
	return *this;
}


//////////////////////////////////////////////////////////////////////////
//
// Function:		CXPGroupBox::SetAlignment
//
// Description:		Set Alignment for title
//
// INPUTS:          Text to use
// 
// RETURNS:         Reference to 'this' object
//
//
// Name                     Date        Version Comments
// Jack Jin					2003-12-03    1.0     Origin
//////////////////////////////////////////////////////////////////////////
CXPGroupBox& CXPGroupBox::SetAlignment(DWORD dwType)
{
	switch(dwType)
	{
	default	:	ASSERT(false);
	case SS_LEFT:	
		m_dwAlignment =  SS_LEFT;
		break;
	case	SS_CENTER:	
		m_dwAlignment = SS_CENTER;
		break;
	case	SS_RIGHT	:
		m_dwAlignment = SS_RIGHT;
		break;
	}
	UpdateSurface();
	
	return *this;
}


