// TransparentStatic.cpp : implementation file
//

#include "SCStatic.h"

#include "../../Functions.h"
#include "../../MemoryDC.h"

// CSCStatic

IMPLEMENT_DYNAMIC(CSCStatic, CStatic)
CSCStatic::CSCStatic()
{
	m_transparent	= false;

	m_sText			= "";
	m_crText		= ::GetSysColor(COLOR_BTNTEXT);
	m_crBack		= ::GetSysColor(COLOR_BTNFACE);

	m_bGradient		= false;
	m_bSunken		= false;
	m_bVertical		= false;
	m_start_marquee = 0;
	/*
	hinst_msimg32 = LoadLibrary(_T("msimg32.dll"));
	if (hinst_msimg32)
	{
		m_bCanDoGradientFill = true;
		dllfunc_GradientFill = ((LPFNDLLFUNC1) GetProcAddress(hinst_msimg32, "GradientFill"));
	}
	*/

	m_sFontName		= _T("");
	m_nFontSize		= 0;
	m_nFontWidth	= 0;
	m_bFontBold		= false;
	m_bFontUnderline= false;

	m_nOutlineWidth	= 0;
	m_crOutline		= 0;//RGB(255, 255, 255);

	m_bBlink		= FALSE;
	m_bBlinkStatus	= FALSE;
	m_nBlinkTime0	= 400;
	m_nBlinkTime1	= 1200;

	m_dwStyle		= 0;
	m_nPrefixSpace	= 0;

	m_hIcon			= NULL;
	m_header_image_index = 0;
}

CSCStatic::~CSCStatic()
{
	if (m_hIcon)
		DestroyIcon(m_hIcon);

	for (int i = 0; i < m_header_images.size(); i++)
	{
		CGdiplusBitmap* img = m_header_images[i];
		delete img;
	}

	//functions.h�� gradient_fill�� ���ؼ� ����� �� �ڵ��� ����ϴ� ���α׷��̶��
	//����� �� ������������.
	//�ϴ� �� Ŭ�������� ����ϹǷ� �� Ŭ������ �Ҹ��ڿ��� ���� �Լ��� ȣ�����ְ� ������
	//���� �۷ι� ������ ���� ���ۿ��� �������� �ִ�.
	safe_release_gradient_rect_handle();
}

BEGIN_MESSAGE_MAP(CSCStatic, CStatic)
	//{{AFX_MSG_MAP(CSCStatic)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_TIMER()
 	ON_MESSAGE(WM_SETTEXT,OnSetText)
// 	ON_WM_CTLCOLOR_REFLECT()
	//}}AFX_MSG_MAP
//	ON_MESSAGE(WM_SETTEXT,OnSetText)
//	ON_WM_CTLCOLOR_REFLECT()
ON_WM_WINDOWPOSCHANGED()
END_MESSAGE_MAP()


void CSCStatic::PreSubclassWindow() 
{
	// TODO: Add your specialized code here and/or call the base class

	CStatic::PreSubclassWindow();

	//modified the style to avoid text overlap when press tab 
	ModifyStyle(0, BS_ICON);

	// Get Defalut Font 
	CFont* cf = GetFont();

	if (cf != NULL)
	{
		cf->GetObject(sizeof(m_lf), &m_lf);
	}
	else
	{
		GetObject(GetStockObject(SYSTEM_FONT), sizeof(m_lf), &m_lf);
	}

	reconstruct_font();
}

void CSCStatic::reconstruct_font()
{
	m_font.DeleteObject();
	BOOL bCreated = m_font.CreateFontIndirect(&m_lf);

	ASSERT(bCreated);
}

// CSCStatic message handlers

void CSCStatic::OnPaint()
{
	CPaintDC dc1(this); // device context for painting

	// Where to draw text
	CRect rc;

	GetClientRect(rc);

	CMemoryDC	dc(&dc1, &rc);
	Gdiplus::Graphics g(dc.GetSafeHdc());

	if (m_ImageBack.m_hImageList != NULL)
	{
		// ��� �׸��� ĥ�Ѵ�.
		for (int i = rc.left; i < rc.right; i += 2)
			m_ImageBack.Draw(&dc, 0, CPoint(i, rc.top), ILD_TRANSPARENT);
	}

	//���� ����̸� ��浵 ��ĥ�ϰ� ���ڵ� ���� ���� ��µȴ�.
	dc.SetBkMode(TRANSPARENT);

	if (m_transparent)
	{
		CRect Rect;
		GetWindowRect(&Rect);
		CWnd* pParent = GetParent();
		ASSERT(pParent);
		pParent->ScreenToClient(&Rect);  //convert our corrdinates to our parents
		//copy what's on the parents at this point
		CDC* pDC = pParent->GetDC();
		CDC MemDC;
		CBitmap bmp;
		MemDC.CreateCompatibleDC(pDC);
		bmp.CreateCompatibleBitmap(pDC, Rect.Width(), Rect.Height());
		CBitmap* pOldBmp = MemDC.SelectObject(&bmp);
		MemDC.BitBlt(0, 0, Rect.Width(), Rect.Height(), pDC, Rect.left, Rect.top, SRCCOPY);
		dc.BitBlt(0, 0, Rect.Width(), Rect.Height(), &MemDC, 0, 0, SRCCOPY);
		MemDC.SelectObject(pOldBmp);
		pParent->ReleaseDC(pDC);
		MemDC.DeleteDC();

	}
	else
	{
		if (m_bGradient)
		{
			if (true)// m_bCanDoGradientFill) //msimg32.dll library is loaded
			{
				gradient_rect(&dc, rc, m_crGradient, m_bVertical);
			}
			else
			{
				//msimg32.dll is not available. Let's use our own code to display gradient background.
				//This code is very simple and produces worse gradient that function from the library - but works!
				//draw_gradient_rect(&dc, rc, m_crBack, m_crGradient, m_bVertical);
			}
		}
		else
		{
			dc.FillSolidRect(rc, m_crBack);
			if (m_bSunken)
			{
				int sunken_depth = 12;
				DrawLinePt(&dc, vertex(rc, 1, true), vertex(rc, 2, true), get_color(m_crBack, sunken_depth));
				DrawLinePt(&dc, vertex(rc, 2, true), vertex(rc, 3, true), get_color(m_crBack, sunken_depth));
				DrawLinePt(&dc, vertex(rc, 1, true), vertex(rc, 0, true), get_color(m_crBack, -sunken_depth));
				DrawLinePt(&dc, vertex(rc, 0, true), vertex(rc, 3, true), get_color(m_crBack, -sunken_depth));
			}
		}
	}

	if (m_sText == "")
		GetWindowText(m_sText);
	

	if (m_sText == "" && m_hIcon == NULL && m_header_images.size() == 0)
		return;
	

	CFont *pOldFont=dc.SelectObject(&m_font);
	DWORD dwStyle = GetStyle();
	DWORD dwText = 0;
	
	if (m_dwStyle == 0)
	{
		MAP_STYLE(SS_LEFT,			DT_LEFT						);
		MAP_STYLE(SS_RIGHT,			DT_RIGHT					);
		MAP_STYLE(SS_CENTER,		DT_CENTER					);
		MAP_STYLE(SS_CENTERIMAGE,	DT_VCENTER | DT_SINGLELINE	);
		MAP_STYLE(SS_NOPREFIX,		DT_NOPREFIX					);
		MAP_STYLE(SS_WORDELLIPSIS,	DT_WORD_ELLIPSIS			);
		MAP_STYLE(SS_ENDELLIPSIS,	DT_END_ELLIPSIS				);
		MAP_STYLE(SS_PATHELLIPSIS,	DT_PATH_ELLIPSIS			);



		NMAP_STYLE(	SS_LEFTNOWORDWRAP |
					SS_CENTERIMAGE |
					SS_WORDELLIPSIS |
					SS_ENDELLIPSIS |
					SS_PATHELLIPSIS,	DT_WORDBREAK);

	}

	//�ؽ�Ʈ ��� ��ƾ
	CString sSpace = _T("");

	//���ڿ� �տ� ���� �ֱ� �ɼ��� ���� ��� ���� �߰�
	if (m_sText != "")
	{
		for (int i = 0; i < m_nPrefixSpace; i++)
			sSpace = _T(" ") + sSpace;
	}

	//�ؽ�Ʈ�� ��� ũ�⸦ ���Ѵ�.
	CRect rText;
	CSize szText;

	dc.DrawText(sSpace + m_sText, &rText, DT_CALCRECT);	
	szText.cx = rText.Width();// + m_nOutlineWidth * 2;
	szText.cy = rText.Height();// + m_nOutlineWidth * 2;

	rText = rc;


	//�������� ������ �������� �׷��ش�.
	if (m_hIcon != NULL)
	{
		CRect rIcon = rText;
		CSize szImg = m_szIcon;

		//�������� �ʺ�ŭ �ؽ�Ʈ�� �з��� ��µȴ�.
		if (dwStyle & SS_CENTER)
		{
			if (m_sText != "")
				rIcon.left = (rc.Width() - szText.cx - szImg.cx) / 2 - szImg.cx / 2 - 2;
			else
				rIcon.left = (rc.Width() - szText.cx - szImg.cx) / 2;

			rText.left = rIcon.left + szImg.cx + 2;
		}
		else if (dwStyle & SS_RIGHT)
		{
			rIcon.left = rc.right - szText.cx - szImg.cx - 2 - m_nOutlineWidth * 2;
			rText.left = rIcon.left + szImg.cx + 2 + m_nOutlineWidth;
		}
		else
		{
			if (m_sText != "")
			{
				rIcon.left = 2;
				rText.left = 2 + szImg.cx + 2;
			}
			else
			{
				rIcon.left = 0;
			}
		}

		if (dwStyle & SS_CENTERIMAGE)
		{
			rIcon.top = rc.top + (rc.Height() - szImg.cy) / 2;
			rText.top = (rc.Height() - szText.cy) / 2;
		}
		else
		{
			if (m_sText != "")
				rIcon.top = szText.cy / 2 - szImg.cy / 2;
			else
				rIcon.top = 0;
			
			rText.top = 0;
		}

		if (!m_bBlinkStatus)
			::DrawIconEx(dc.GetSafeHdc(), rIcon.left, rIcon.top, m_hIcon, szImg.cx, szImg.cy, 0, NULL, DI_NORMAL);
	}
	else if (m_header_images.size() > 0)
	{
		CRect rImg = rText;
		CSize szImg(m_header_images[m_header_image_index]->width, m_header_images[m_header_image_index]->height);

		//�������� �ʺ�ŭ �ؽ�Ʈ�� �з��� ��µȴ�.
		if (dwStyle & SS_CENTER)
		{
			if (m_sText != "")
				rImg.left = (rc.Width() - szText.cx - szImg.cx) / 2 - szImg.cx / 2 - 2;
			else
				rImg.left = (rc.Width() - szText.cx - szImg.cx) / 2;

			rText.left = rImg.left + m_header_images[m_header_image_index]->width + 2;
		}
		else if (dwStyle & SS_RIGHT)
		{
			rImg.left = rc.right - szText.cx - szImg.cx - 2 - m_nOutlineWidth * 2;
			rText.left = rImg.left + szImg.cx + 2 + m_nOutlineWidth;
		}
		else
		{
			if (m_sText != "")
			{
				rImg.left = 2;
				rText.left = 2 + szImg.cx + 2;
			}
			else
			{
				rImg.left = 0;
			}
		}

		if (dwStyle & SS_CENTERIMAGE)
		{
			rImg.top = rc.top + (rc.Height() - szImg.cy) / 2;
			rText.top = (rc.Height() - szText.cy) / 2;
		}
		else
		{
			if (m_sText != "")
				rImg.top = szText.cy / 2 - szImg.cy / 2;
			else
				rImg.top = 0;

			rText.top = 0;
		}

		m_header_images[0]->draw(g, rImg.left, rImg.top);
	}
	else
	{
		//�� �ڵ尡 �־ center image�� false�ε��� ���� �߾ӿ� ǥ�õȴ�. �� �� �ڵ尡 �ִ��� Ȯ������!
		//rText.top = (rc.Height() - szText.cy) / 2;
		//rText.bottom = rText.top + szText.cy;
	}

	if (m_sText != "")
	{
		if (IsWindowEnabled())
			dc.SetTextColor(m_crText);
		else
			dc.SetTextColor(::GetSysColor(COLOR_GRAYTEXT));


		// Draw the text

		//line space������ �ణ �Ʒ� ��µǹǷ� �̸� �������ش�.
		if (dwStyle & SS_CENTERIMAGE)
		{
			//rText.OffsetRect(0, -3);

			// 			if (rText.top < 0)
			// 				rText.OffsetRect(0, -rText.top);
		}
		else
		{
		}

		//�������� ���� ��쿡�� �������� ��ǥ�� �����ǰ� �ؽ�Ʈ�� ��ǥ ����
		//���� ��Ŀ� ���� �ڵ� �����ȴ�. ���� ���� �÷��� ���� �����ϵ��� �ؾ� ���� ǥ�õȴ�.
		//�̴� ���� ���� �����ܰ� �ؽ�Ʈ�� ���� ��ǥ�� ��� ��ƾ�� ��ü������ �����ؾ� �� ���ϴ�.
		if (m_hIcon)
		{
			dwText &= ~(DT_CENTER);
			dwText &= ~(DT_RIGHT);
			dwText &= ~(DT_VCENTER);
		}

		if (!m_bBlinkStatus)
		{
			if (m_nOutlineWidth > 0)
			{
				dc.SetTextColor(m_crOutline);

				for (int x = -m_nOutlineWidth; x <= m_nOutlineWidth; ++x)
				{
					for (int y = -m_nOutlineWidth; y <= m_nOutlineWidth; ++y)
					{
						//dc.TextOut(10 + x, 10 + y, str, str.GetLength());
						CRect	rOffset = rText;
						rOffset.OffsetRect(x, y);
						dc.DrawText(sSpace + m_sText, rOffset, dwText);
					}
				}

				dc.SetTextColor(m_crText);
			}

			dc.DrawText(sSpace + m_sText, rText, dwText);
		}
	}

	// Select old font
	dc.SelectObject(pOldFont);
}

void CSCStatic::set_text(CString sText, COLORREF cTextColor /*= RGB(0,0,0)*/)
{
	m_sText = sText;

 	if (cTextColor > 0)
 		m_crText = cTextColor;
	
	//�ݺ����ȿ��� �̸� ȣ���� ��� Invalidate()�����δ� �ؽ�Ʈ�� �ٷ� ������� �ʱ⵵ �Ѵ�.

	//�����϶� update_surface()�� ��� ������ ���ŵȴ�.
	//=>dlg���� clip sibling�� ���� ����� �޶�����.
	if (m_transparent)
		update_surface();
	else
		Invalidate();
}

void CSCStatic::set_back_image(UINT nIDBack)
{
	CBitmap	Bitmap;

	Bitmap.LoadBitmap(nIDBack);
	m_ImageBack.Create(2, 23, ILC_COLORDDB|ILC_MASK, 1, 0);
	m_ImageBack.Add(&Bitmap, RGB(0,255,0));
	m_ImageBack.SetBkColor(CLR_NONE);
	Bitmap.DeleteObject();
}

// CSCStatic message handlers

LRESULT CSCStatic::OnSetText(WPARAM wParam,LPARAM lParam)
{
	update_surface();

	return 0;
}
/*
HBRUSH CSCStatic::CtlColor(CDC* pDC, UINT nCtlColor)
{
   pDC->SetBkMode(TRANSPARENT);
   return (HBRUSH)GetStockObject(NULL_BRUSH);
}
*/


BOOL CSCStatic::OnEraseBkgnd(CDC* pDC) 
{
	// TODO: Add your message handler code here and/or call default
	return FALSE;
	
	//CDialog::OnEraseBkgnd(pDC);
}


void CSCStatic::set_blink_time(int nTime0 /*= 500*/, int nTime1 /*= 500*/)
{
	KillTimer(TIMER_BLINK);

	m_nBlinkTime0		= nTime0;
	m_nBlinkTime1		= nTime1;

	set_blink(m_bBlink);
}

void CSCStatic::set_blink(BOOL bBlink /*= TRUE*/)
{
	m_bBlink = bBlink;
	m_bBlinkStatus = FALSE;

	if (m_bBlink)
	{
		SetTimer(TIMER_BLINK, m_nBlinkTime0, NULL);
	}
	else
	{
		m_bBlink = false;
		KillTimer(TIMER_BLINK);
		update_surface();
	}
}



// �� �ٸ� ���
// CStaticEx2 message handlers
/*
LRESULT CSCStatic::Onset_text(WPARAM wParam,LPARAM lParam)
{
   LRESULT Result = Default();
   Invalidate();
   UpdateWindow();
   return Result;
}

HBRUSH CSCStatic::CtlColor(CDC* pDC, UINT nCtlColor)
{
   pDC->SetBkMode(TRANSPARENT);
   return (HBRUSH)GetStockObject(NULL_BRUSH);
}

BOOL CSCStatic::OnEraseBkgnd(CDC* pDC)
{
   if (m_Bmp.GetSafeHandle() == NULL)
   {
      CRect Rect;
      GetWindowRect(&Rect);
      CWnd *pParent = GetParent();
      ASSERT(pParent);
      pParent->ScreenToClient(&Rect);  //convert our corrdinates to our parents
      
      //copy what's on the parents at this point
      CDC *pDC = pParent->GetDC();
      CDC MemDC;
      MemDC.CreateCompatibleDC(pDC);
      m_Bmp.CreateCompatibleBitmap(pDC,Rect.Width(),Rect.Height());
      CBitmap *pOldBmp = MemDC.SelectObject(&m_Bmp);
      MemDC.BitBlt(0,0,Rect.Width(),Rect.Height(),pDC,Rect.left,Rect.top,SRCCOPY);
      MemDC.SelectObject(pOldBmp);
      pParent->ReleaseDC(pDC);
   }
   else //copy what we copied off the parent the first time back onto the parent
   {
      CRect Rect;
      GetClientRect(Rect);
      CDC MemDC;
      MemDC.CreateCompatibleDC(pDC);
      CBitmap *pOldBmp = MemDC.SelectObject(&m_Bmp);
      pDC->BitBlt(0,0,Rect.Width(),Rect.Height(),&MemDC,0,0,SRCCOPY);
      MemDC.SelectObject(pOldBmp);
   }

   return TRUE;
}
*/

void CSCStatic::OnTimer(UINT_PTR nIDEvent) 
{
	//TRACE("Static blink\n");
	if (nIDEvent == TIMER_BLINK)
	{
		m_bBlinkStatus = !m_bBlinkStatus;

		if (IsWindowEnabled() == false)
			m_bBlinkStatus = false;

		update_surface();

		if (IsWindowEnabled() == false)
			return;

		KillTimer(TIMER_BLINK);

		if (m_bBlinkStatus)
			SetTimer(TIMER_BLINK, m_nBlinkTime1, NULL);
		else
			SetTimer(TIMER_BLINK, m_nBlinkTime0, NULL);
	}
	else if (nIDEvent == TIMER_MARQUEE)
	{
		COLORREF cr = m_crGradient[m_crGradient.size()-1];
		m_crGradient.insert(m_crGradient.begin(), cr);
		m_crGradient.pop_back();
		Invalidate();
	}

	CStatic::OnTimer(nIDEvent);
}

void CSCStatic::update_surface()
{
	CRect rc;

	GetWindowRect(&rc);
	//RedrawWindow();

	GetParent()->ScreenToClient(&rc);
	GetParent()->InvalidateRect(rc, false);
	GetParent()->UpdateWindow();
}

BOOL CSCStatic::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
/*
	if (pMsg->message == WM_KEYDOWN)
	{
		TRACE("key in TransparentStatic control = %d\n", pMsg->wParam);
		::PostMessage(GetParent()->m_hWnd, WM_KEYDOWN, pMsg->wParam, pMsg->lParam);
		return TRUE;
	}
*/

	return CStatic::PreTranslateMessage(pMsg);
}

void CSCStatic::set_icon(UINT nIDResource, int nSize /*= 16*/)
{
	if  (m_hWnd == NULL)
		return;

	if (m_hIcon)
		::DestroyIcon(m_hIcon);

	//HINSTANCE hInstResource = AfxFindResourceHandle(MAKEINTRESOURCE(nIDResource), RT_GROUP_ICON);
	//m_hIcon = (HICON)::LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(nIDResource), IMAGE_ICON, nSize, nSize, 0);
	//m_hIcon = (HICON)(::LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(nIDResource), IMAGE_ICON, nSize, nSize, LR_DEFAULTCOLOR | LR_LOADTRANSPARENT));
	m_hIcon = load_icon(AfxGetInstanceHandle(), nIDResource, nSize, nSize);

	if (m_hIcon == NULL)
		return;

	ICONINFO	iconInfo;

	::ZeroMemory(&iconInfo, sizeof(ICONINFO));
	BOOL bRetValue = ::GetIconInfo(m_hIcon, &iconInfo);

	if (bRetValue == false)
		return;

	m_szIcon.cx = nSize;//(DWORD)(iconInfo.xHotspot * 2);
	m_szIcon.cy = nSize;//(DWORD)(iconInfo.yHotspot * 2);


	//GetIconInfo �Լ��� hbmMask�� hbmColor ��Ʈ���� �����Ͽ� �����ϹǷ�
	//hbmMask�� hbmColor ��Ʈ���� �������־�� ��. �׷��� ������ GDI��ü�� ��� �þ.
 	::DeleteObject(iconInfo.hbmMask);
 	::DeleteObject(iconInfo.hbmColor);


	CRect	rc, rParentRect;

	GetWindowRect(rc);

	//��Ʈ���� ũ�Ⱑ �������� ũ�⺸�� �۴ٸ� ��Ʈ���� ũ�⸦ �������� ũ��� �����ش�.
	//��, �ؽ�Ʈ�� ������ ��쿡�� �߰����� �ڵ��� �ʿ��ϴ�.
	if (m_hIcon && rc.Width() < nSize)
		SetWindowPos(NULL, rc.left, rc.top, nSize, rc.Height(), SWP_NOZORDER | SWP_NOMOVE);
	if (m_hIcon && rc.Height() < nSize)
		SetWindowPos(NULL, rc.left, rc.top, rc.Width(), nSize, SWP_NOZORDER | SWP_NOMOVE);

	Invalidate();
}

CSCStatic& CSCStatic::set_font_name(const CString& strFont, BYTE byCharSet)
{
	m_lf.lfCharSet = byCharSet;

	_tcscpy_s(m_lf.lfFaceName, _countof(m_lf.lfFaceName), strFont);
	reconstruct_font();
	update_surface();

	return *this;
}

CSCStatic& CSCStatic::set_font_size(int nSize)
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

CSCStatic& CSCStatic::set_font_bold(bool bBold)
{
	m_lf.lfWeight = bBold ? FW_BOLD : FW_NORMAL;
	reconstruct_font();
	update_surface();

	return *this;
}

CSCStatic& CSCStatic::set_font_underline(bool bSet)
{
	m_lf.lfUnderline = bSet;
	reconstruct_font();
	update_surface();

	return *this;
}

CSCStatic& CSCStatic::set_font_antialiased(bool bAntiAliased)
{
	m_bFontAntiAliased = bAntiAliased;
	m_lf.lfQuality = (m_bFontAntiAliased ? ANTIALIASED_QUALITY : DEFAULT_QUALITY);
	reconstruct_font();
	update_surface();

	return *this;
}

void CSCStatic::set_text_color(COLORREF crTextColor)
{
	m_crText = crTextColor;
	update_surface();
	Invalidate();
}

CSCStatic& CSCStatic::set_gradient(bool bGradient)
{
	m_bGradient = bGradient;

	if (m_bGradient)
		m_transparent = false;

	return *this;
}

CSCStatic& CSCStatic::set_gradient_color(COLORREF crGradient)
{
	set_gradient();
	m_crGradient.clear();
	m_crGradient.push_back(crGradient);
	return *this;
}

CSCStatic& CSCStatic::set_gradient_color(int idx, COLORREF crGradient)
{
	set_gradient();
	std::deque<COLORREF>::iterator it;

	if (idx >= m_crGradient.size())
	{
		int loop = idx -  m_crGradient.size() + 1;
		for (int i = 0; i < loop; i++)
			m_crGradient.push_back(crGradient);
		return *this;
	}
	else
	{
		m_crGradient[idx] = crGradient;
	}

	return *this;
}
/*
CSCStatic& CSCStatic::set_gradient_color(int count, ...)
{
	set_gradient();
	m_crGradient.clear();

	va_list arglist;
	va_start(arglist, count);
	
	for (int i = 0; i < count; i++)
		m_crGradient.push_back(va_arg(arglist, DWORD));
	
	va_end(arglist);
	
	return *this;
}
*/
CSCStatic& CSCStatic::add_gradient_color(COLORREF crGradient)
{
	set_gradient();
	m_crGradient.push_back(crGradient);
	return *this;
}

CSCStatic& CSCStatic::insert_gradient_color(int idx, COLORREF crGradient)
{
	set_gradient();
	std::deque<COLORREF>::iterator it;

	if (idx >= m_crGradient.size())
	{
		int loop = idx -  m_crGradient.size();
		for (int i = 0; i < loop; i++)
			m_crGradient.push_back(crGradient);
		return *this;
	}

	it = m_crGradient.begin();
	m_crGradient.insert(it + idx, crGradient);
	return *this;
}

CSCStatic& CSCStatic::set_vertical_gradient(bool bVertical)
{
	set_gradient();
	m_bVertical = bVertical;
	Invalidate();
	return *this;
}

//this function will be used only if msimg32.dll library is not available
void CSCStatic::draw_gradient_rect(CDC *pDC, CRect r, COLORREF cLeft, COLORREF cRight, BOOL a_bVertical)
{
	CRect stepR;					// rectangle for color's band
	COLORREF color;				// color for the bands
	float fStep;
	
	if(a_bVertical)
		fStep = ((float)r.Height())/255.0f;	
	else
		fStep = ((float)r.Width())/255.0f;	// width of color's band
	
	for (int iOnBand = 0; iOnBand < 255; iOnBand++) 
	{
		// set current band
		if(a_bVertical)
		{
			SetRect(&stepR,
				r.left, 
				r.top+(int)(iOnBand * fStep),
				r.right, 
				r.top+(int)((iOnBand+1)* fStep));	
		}
		else
		{
			SetRect(&stepR,
				r.left+(int)(iOnBand * fStep), 
				r.top,
				r.left+(int)((iOnBand+1)* fStep), 
				r.bottom);	
		}

		// set current color
		color = RGB((GetRValue(cRight)-GetRValue(cLeft))*((float)iOnBand)/255.0f+GetRValue(cLeft),
			(GetGValue(cRight)-GetGValue(cLeft))*((float)iOnBand)/255.0f+GetGValue(cLeft),
			(GetBValue(cRight)-GetBValue(cLeft))*((float)iOnBand)/255.0f+GetBValue(cLeft));
		// fill current band
		pDC->FillSolidRect(stepR,color);
	}
}
/*
void CSCStatic::start_marquee(int start)
{
	if (start == -1)
		m_start_marquee = !m_start_marquee;
	else
		m_start_marquee = start;

	if (m_start_marquee)
		SetTimer(TIMER_MARQUEE, 100, NULL);
	else
		KillTimer(TIMER_MARQUEE);
}
*/

void CSCStatic::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CStatic::OnWindowPosChanged(lpwndpos);

	// TODO: Add your message handler code here
	Invalidate();
}

void CSCStatic::add_header_image(UINT id)
{
	CGdiplusBitmap* img = new CGdiplusBitmap(_T("PNG"), (UINT)id);
	m_header_images.push_back(img);

	//RedrawWindow();
	//UpdateWindow();
	//Invalidate();
}
