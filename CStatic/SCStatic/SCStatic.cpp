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

	m_cr_text.SetFromCOLORREF(::GetSysColor(COLOR_BTNTEXT));
	m_cr_back.SetFromCOLORREF(::GetSysColor(COLOR_BTNFACE));

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
	if (m_tooltip)
	{
		m_tooltip->DestroyWindow();
		delete m_tooltip;
	}

	if (m_hIcon)
		DestroyIcon(m_hIcon);

	for (int i = 0; i < m_header_images.size(); i++)
	{
		CSCGdiplusBitmap* img = m_header_images[i];
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
	//}}AFX_MSG_MAP
ON_WM_WINDOWPOSCHANGED()
ON_WM_SIZE()
ON_WM_LBUTTONDOWN()
ON_WM_SETCURSOR()
END_MESSAGE_MAP()

BOOL CSCStatic::create(LPCTSTR lpszText, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
{
	m_text = lpszText;
	BOOL res = CStatic::Create(lpszText, dwStyle, rect, pParentWnd, nID);
	reconstruct_font();

	return res;
}

void CSCStatic::PreSubclassWindow() 
{
	// TODO: Add your specialized code here and/or call the base class

	CStatic::PreSubclassWindow();

	//���� �����ÿ��� �̹� m_text�� ������ ���� �����ÿ��� ���� ���;� �Ѵ�.
	if (m_text.IsEmpty())
		CStatic::GetWindowText(m_text);

	//modified the style to avoid text overlap when press tab 
	ModifyStyle(0, BS_ICON);

	m_halign = get_halign();
	m_valign = get_valign();

	//Resource Editor���� �� ��Ʈ���� ����ϴ� dlg�� ����� ��Ʈ�� �⺻���� ����ؾ� �Ѵ�.
	//��, �������� ������ Ŭ�������� �� Ŭ������ ����ϰų�
	//���� MainWnd�� �������� ���� ���¿����� �� �ڵ带 ���� �� �����Ƿ�
	//pWnd = NULL�� �� �ִ�.
	CWnd* pWnd = GetParent();

	if (pWnd == NULL)
		pWnd = AfxGetApp()->GetMainWnd();

	CFont* font = NULL;

	if (pWnd)
		font = pWnd->GetFont();

	if (font == NULL)
	{
		GetObject(GetStockObject(SYSTEM_FONT), sizeof(m_lf), &m_lf);
		//SYSTEM_FONT�� ������ m_lf.lfWidth�� 8�� ���õǴµ�
		//���� font_size, font_name�� �����ص� �� ���� �״���̹Ƿ�
		//���η� �о��� �۲÷� ǥ�õȴ�.
		//���� m_lf.lfWidth�� 0���� �ʱ�ȭ����߸� lfHeight�� ���� �ڵ����� ������ �۲÷� �ùٸ��� ǥ�õȴ�.
		//�ǵ������� ���̸� �������� �ʴ� �� �� ���� 0���� �����ؾ� �Ѵ�.
		m_lf.lfWidth = 0; // 0 means default width
	}
	else
	{
		font->GetObject(sizeof(m_lf), &m_lf);
	}

	//CStatic���� tooltip, mouse event���� ó���Ϸ��� �ݵ�� SS_NOTIFY ��Ÿ���� �߰��ؾ� �Ѵ�.
	//ModifyStyle(0, SS_NOTIFY);

	reconstruct_font();
}

void CSCStatic::prepare_tooltip()
{
	if (m_tooltip)
	{
		m_tooltip->DestroyWindow();
		delete m_tooltip;
	}

	m_tooltip = new CToolTipCtrl();

	try
	{
		BOOL b = m_tooltip->Create(this, TTS_ALWAYSTIP | TTS_NOPREFIX | TTS_NOANIMATE);
	}
	catch (CException*)
	{
		CString str = get_error_str(GetLastError());
	}

	//m_tooltip->SetDelayTime(TTDT_AUTOPOP, -1);
	//m_tooltip->SetDelayTime(TTDT_INITIAL, 0);
	//m_tooltip->SetDelayTime(TTDT_RESHOW, 0);
	m_tooltip->SetMaxTipWidth(400);
	m_tooltip->AddTool(this, _T(""));
	m_tooltip->Activate(TRUE);
	EnableToolTips(TRUE);
	EnableTrackingToolTips(TRUE);

	//TOOLINFO ti;
	//ti.cbSize = TTTOOLINFOW_V2_SIZE;// sizeof(TOOLINFO);
	//ti.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
	//ti.rect.left = ti.rect.top = ti.rect.bottom = ti.rect.right = 0;
	//ti.hwnd = GetParent()->GetSafeHwnd();
	//ti.uId = (UINT)GetSafeHwnd();
	//ti.hinst = AfxGetInstanceHandle();
	//ti.lpszText = (LPTSTR)_T("skldfjkl");

	//SendMessage(TTM_ADDTOOL, 0, (LPARAM)&ti);
	//SendMessage(TTM_TRACKACTIVATE, TRUE, (LPARAM)&ti);

	//EnableTrackingToolTips(TRUE);
	//m_tooltip->Activate(true);
}

void CSCStatic::set_tooltip_text(CString text)
{
	m_tooltip_text = text;

	if (!text.IsEmpty())
		m_use_tooltip = true;

	if (!m_tooltip)
		prepare_tooltip();

	m_tooltip->UpdateTipText(m_tooltip_text, this);
	m_tooltip->AddTool(this, m_tooltip_text);
}

void CSCStatic::reconstruct_font()
{
	m_font.DeleteObject();

	//TRACE(_T("m_lf.lfQuality = %d\n"), m_lf.lfQuality);// ANTIALIASED_QUALITY
	BOOL bCreated = m_font.CreateFontIndirect(&m_lf);

	Invalidate();
}

void CSCStatic::set_font(CFont* font)
{
	font->GetLogFont(&m_lf);
	reconstruct_font();
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
	Gdiplus::GraphicsPath	roundPath;

	g.SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);
	g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);

	if (m_round > 0)
		get_round_rect_path(&roundPath, CRect2GpRect(rc), m_round);

	//���� ����̸� ��浵 ��ĥ�ϰ� ���ڵ� ���� ���� ��µȴ�.
	dc.SetBkMode(TRANSPARENT);

	if (m_round > 0 || m_transparent)
	{
		dc.FillSolidRect(rc, m_cr_parent_back.ToCOLORREF());
		/*
		CRect Rect;
		GetWindowRect(&Rect);
		CWnd* pParent = GetParent();
		ASSERT(pParent);
		pParent->ScreenToClient(&Rect);  //convert our corrdinates to our parents
		//copy what's on the parents at this point
		/*
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
		*/
	}
	
	if (m_img_back.is_valid() && !m_img_back.is_animated_gif())
	{
		// ��� �׸��� �׸���.
		m_img_back.draw(g, rc, CSCGdiplusBitmap::draw_mode_stretch);
		//m_img_back.save(_T("d:\\copy.png"));
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
				//draw_gradient_rect(&dc, rc, m_cr_back, m_crGradient, m_bVertical);
			}
		}
		else
		{
			if (m_transparent || m_round > 0)
			{
				Gdiplus::SolidBrush brush(m_cr_back);
				g.FillPath(&brush, &roundPath);
			}
			else
			{
				dc.FillSolidRect(rc, m_cr_back.ToCOLORREF());
			}

			if (m_bSunken)
			{
				draw_sunken_rect(&dc, rc, true, GRAY128, GRAY192);
			}
		}
	}


	if (m_text.IsEmpty() && m_hIcon == NULL && m_header_images.size() == 0)
		return;
	

	CFont *pOldFont=dc.SelectObject(&m_font);
	DWORD dwStyle = GetStyle();
	DWORD dwText = DT_NOCLIP;// | DT_WORDBREAK;
	
	if (m_dwStyle == 0)
	{
		MAP_STYLE(SS_LEFT,			DT_LEFT						);
		MAP_STYLE(SS_RIGHT,			DT_RIGHT					);
		MAP_STYLE(SS_CENTER,		DT_CENTER					);
		//MAP_STYLE(SS_CENTERIMAGE,	DT_VCENTER | DT_SINGLELINE	);
		MAP_STYLE(SS_NOPREFIX,		DT_NOPREFIX					);
		MAP_STYLE(SS_WORDELLIPSIS,	DT_WORD_ELLIPSIS			);
		MAP_STYLE(SS_ENDELLIPSIS,	DT_END_ELLIPSIS				);
		MAP_STYLE(SS_PATHELLIPSIS,	DT_PATH_ELLIPSIS			);

		//SS_LEFTNOWORDWRAP ��Ÿ���� DT_WORDBREAK�� �Բ� ����ϸ� �ȵȴ�.
		//��, SS_LEFTNOWORDWRAP�̸� �ؽ�Ʈ�� �ڵ����� SS_LEFT�� �ǰ� �ڵ� �ٹٲ� ���� �ʴ´�.
		//�ٸ� SS_CENTERIMAGE���� ���ԵǸ� ��Ÿ���� �ʹ� ���ѵǹǷ� SS_CENTERIMAGE�� üũ ��󿡼� ���ܽ�Ų��.
		NMAP_STYLE(	SS_LEFTNOWORDWRAP |
					//SS_CENTERIMAGE |
					SS_WORDELLIPSIS |
					SS_ENDELLIPSIS |
					SS_PATHELLIPSIS,	DT_WORDBREAK);
	}

	//�ؽ�Ʈ ��� ��ƾ
	CString sSpace = _T("");

	//���ڿ� �տ� ���� �ֱ� �ɼ��� ���� ��� ���� �߰�
	if (!m_text.IsEmpty())
	{
		for (int i = 0; i < m_nPrefixSpace; i++)
			sSpace = _T(" ") + sSpace;
	}

	//�ؽ�Ʈ�� ��� ũ�⸦ ���Ѵ�.
	CSize szText;

	//20250820 scpark. DT_CALCRECT �ÿ��� dwText�� �ݿ����ִ� ���� ����������
	//�������� �پ��� ��Ÿ���� �����ϱ� ���ؼ��� dwText�� �����ϰ� DT_CALCRECT�ؾ߸� ���� �����ϴ�.
	dc.DrawText(sSpace + m_text, &m_rect_text, /*dwText | */DT_CALCRECT);
	m_text_extent = m_rect_text.Width();
	//TRACE(_T("width = %d (%s)\n"), m_rect_text.Width(), m_text);
	szText.cx = m_rect_text.Width();// + m_nOutlineWidth * 2;
	szText.cy = m_rect_text.Height();// + m_nOutlineWidth * 2;

	m_rect_text = rc;

	//������ �Ǵ� ��� �̹����� �ؽ�Ʈ ������ �� ũ������ �̹����� ����, ũ�� ������ ���� ���� ������ �����ǵ��� �����Ǿ�� �Ѵ�.
	int gap = 4;

	//�������� ������ �������� �׷��ش�.
	if (m_hIcon != NULL)
	{
		gap = 4;

		CRect rIcon = m_rect_text;
		CSize szIcon = m_sz_icon;

		//�������� �ʺ�ŭ �ؽ�Ʈ�� �з��� ��µȴ�.
		if (dwStyle & SS_CENTER)
		{
			if (m_text.IsEmpty())
				rIcon.left = rc.CenterPoint().x - m_sz_icon.cx / 2;// (rc.Width() - szText.cx - m_szIcon.cx) / 2;
			else
				rIcon.left = (rc.Width() - (m_sz_icon.cx + gap + szText.cx)) / 2;// -m_szIcon.cx / 2 - 2;

			if (m_image_left_align_fix)
			{
				rIcon.left = 8;
				m_rect_text.left = rc.CenterPoint().x - szText.cx / 2;
			}
			else
			{
				m_rect_text.left = rIcon.left + gap + m_sz_icon.cx;
			}
		}
		else if (dwStyle & SS_RIGHT)
		{
			if (m_image_left_align_fix)
			{
				rIcon.left = 8;
				m_rect_text.left = rc.right - gap - szText.cx;
			}
			else
			{
				rIcon.left = rc.right - szText.cx - m_sz_icon.cx - 2;
				m_rect_text.left = rIcon.left + m_sz_icon.cx + 2;
			}
		}
		else
		{
			if (m_text.IsEmpty())
			{
				rIcon.left = 0;
			}
			else
			{
				rIcon.left = 2;
				m_rect_text.left = rIcon.left + m_sz_icon.cx + gap;
			}
		}

		if (dwStyle & SS_CENTERIMAGE)
		{
			rIcon.top = rc.top + (rc.Height() - m_sz_icon.cy) / 2;
			m_rect_text.top = (rc.Height() - szText.cy) / 2;
		}
		else
		{
			if (m_text.IsEmpty())
			{
				rIcon.top = 0;
			}
			else
			{
				rIcon.top = szText.cy / 2 - m_sz_icon.cy / 2;

				//top �����ε� �������� Ŀ�� ����� ����ٸ� �̹���, �ؽ�Ʈ ��� �Ʒ��� �����ش�.
				if (rIcon.top < 0)
				{
					m_rect_text.top -= rIcon.top;
					rIcon.top = 0;
				}
			}
		}

		if (!m_bBlinkStatus)
			::DrawIconEx(dc.GetSafeHdc(), rIcon.left, rIcon.top, m_hIcon, m_sz_icon.cx, m_sz_icon.cy, 0, NULL, DI_NORMAL);
	}
	else if (m_header_images.size() > 0)
	{
		CRect rImg = m_rect_text;
		CSize szImg(m_header_images[m_header_image_index]->width, m_header_images[m_header_image_index]->height);

		//���� align�� �ؽ�Ʈ�� �߾� �����̸� "image + header" �� �� �߾ӿ�, LEFT�̸� �� �� ���ʺ��� ���ĵ�����
		//m_image_left_align_fix�� true�� ������ ��쿡�� �̹����� ���ʿ� �����ϰ� �ؽ�Ʈ�� ������ align���� ���ĵȴ�.
		//Ư���� �̷��� ��찡 ����Ǵ� ���� �ٷ� CSCMessageBox�̴�.
		//AfxMessageBox() �Ǵ� �׿� ������ �޽����ڽ����� ���� �������� �׻� ���ʿ� ǥ�õǰ� �ؽ�Ʈ�� �����ܰ� ������� ������ ���Ĺ������ ǥ�õȴ�.

		//header image�� �ʺ�ŭ �ؽ�Ʈ�� �з��� ��µȴ�.
		if (dwStyle & SS_CENTER)
		{
			if (m_text.IsEmpty())
				rImg.left = rc.CenterPoint().x - szImg.cx / 2;
			else
				rImg.left = (rc.Width() - (szImg.cx + gap + szText.cx)) / 2;

			if (m_image_left_align_fix)
			{
				rImg.left = 8;
				m_rect_text.left = rc.CenterPoint().x - szText.cx / 2;
			}
			else
			{
				m_rect_text.left = rImg.left + gap + m_header_images[m_header_image_index]->width;
			}
		}
		else if (dwStyle & SS_RIGHT)
		{
			if (m_image_left_align_fix)
			{
				rImg.left = 8;
				m_rect_text.left = rc.right - gap - szText.cx;
			}
			else
			{
				rImg.left = rc.right - szText.cx - szImg.cx - 2;
				m_rect_text.left = rImg.left + szImg.cx + 2;
			}
		}
		else
		{
			if (m_text.IsEmpty())
			{
				rImg.left = 0;
			}
			else
			{
				rImg.left = 2;
				m_rect_text.left = rImg.left + szImg.cx + gap;
			}
		}

		if (dwStyle & SS_CENTERIMAGE)
		{
			rImg.top = rc.top + (rc.Height() - szImg.cy) / 2;
			m_rect_text.top = (rc.Height() - szText.cy) / 2;
		}
		else
		{
			//m_rect_text.top = 0;

			if (m_text.IsEmpty())
			{
				rImg.top = 0;
			}
			else
			{
				rImg.top = szText.cy / 2 - szImg.cy / 2;

				//top �����ε� �������� Ŀ�� ����� ����ٸ� �̹���, �ؽ�Ʈ ��� �Ʒ��� �����ش�.
				if (rImg.top < 0)
				{
					m_rect_text.top -= rImg.top;
					rImg.top = 0;
				}
			}
		}

		m_header_images[0]->draw(g, rImg.left, rImg.top);
	}
	else
	{
		//�� �ڵ尡 �־ center image�� false�ε��� ���� �߾ӿ� ǥ�õȴ�. �� �� �ڵ尡 �ִ��� Ȯ������!
		//m_rect_text.top = (rc.Height() - szText.cy) / 2;
		//m_rect_text.bottom = m_rect_text.top + szText.cy;
	}

	if (!m_text.IsEmpty())
	{
		if (IsWindowEnabled())
		{
			if (m_link_url.IsEmpty())
				dc.SetTextColor(m_cr_text.ToCOLORREF());
			else
				dc.SetTextColor(m_cr_link.ToCOLORREF());
		}
		else
		{
			dc.SetTextColor(::GetSysColor(COLOR_GRAYTEXT));
		}

		//SS_CENTERIMAGE �� ��� DT_VCENTER�� �ݵ�� DT_SINGLELINE�� �����ؾ� �Ѵ�.
		//������ �׷� ��� multiline�� ǥ���� �� �����Ƿ�
		//SS_CENTERIMAGE �� ���� ���� �����ġ�� ���� �������ش�.
		if (dwStyle & SS_CENTERIMAGE)
		{
			m_rect_text.top = (rc.Height() - szText.cy) / 2;
			m_rect_text.bottom = m_rect_text.top + szText.cy;
		}
		else
		{
		}

		//�������� ���� ��쿡�� �������� ��ǥ�� �����ǰ� �ؽ�Ʈ�� ��ǥ ����
		//���� ��Ŀ� ���� �ڵ� �����ȴ�. ���� ���� �÷��� ���� �����ϵ��� �ؾ� ���� ǥ�õȴ�.
		//�̴� ���� ���� �����ܰ� �ؽ�Ʈ�� ���� ��ǥ�� ��� ��ƾ�� ��ü������ �����ؾ� �� ���ϴ�.
		if (m_hIcon || (m_header_images.size() > 0))
		{
			dwText &= ~(DT_CENTER);
			dwText &= ~(DT_RIGHT);
			dwText &= ~(DT_VCENTER);
		}

		if (!m_bBlinkStatus)
		{
			//CStatic�� �⺻������ WORDBREAK(WORDWRAP) ��Ÿ���� ������� ������
			//�� CSCStatic�� CMessageBox���� ����ϴ� WORDBREAK �ɼ��� ����Ǿ�߸� �Ѵ�.
			//m_use_word_wrap = true;�� ���� �ɼ��� �߰��� ���� ������ �� �� �Ϲ����� ������� �����ؾ� �Ѵ�.
			//DT_WORDBREAK�� �⺻ ���ܽ��״� ������ CStatic�� �⺻ WORDBREAK ��Ÿ���� ������� �ʱ� �����̱⵵ �ϰ�
			//m_rect_text�� ��Ȯ�� ����ϱ� ���Ե� �־����� m_rect_text�� ���� �ؽ�Ʈ�� ��µǴ� �������� ���õǴ�����
			//�ٽ� Ȯ���� �ʿ��ϴ�.
			dc.DrawText(sSpace + m_text, m_rect_text, dwText);// | DT_WORDBREAK);
			//DrawShadowText(dc.GetSafeHdc(), sSpace + m_text, CString(sSpace + m_text).GetLength(), m_rect_text,
			//	DT_CENTER | DT_TOP | DT_NOCLIP, m_cr_text.ToCOLORREF(), 0, 2, 1);
		}
	}

	if (m_draw_border)
	{
		//TRACE(_T("draw_border\n"));
		if (m_round > 0)
			draw_round_rect(&g, CRect2GpRect(rc), m_gcr_border, Gdiplus::Color::Transparent, m_round, m_border_thick);
		else
			draw_rectangle(g, rc, m_gcr_border);
	}

	//TRACE(_T("m_rect_text = %s\n"), get_rect_info_string(m_rect_text));
	// Select old font
	dc.SelectObject(pOldFont);
}

void CSCStatic::set_transparent(bool transparent, Gdiplus::Color cr_parent_back)
{
	m_transparent = transparent;

	if (cr_parent_back.GetValue() != Gdiplus::Color::Transparent)
		m_cr_parent_back = cr_parent_back;

	Invalidate();
}

CRect CSCStatic::set_text(CString sText, Gdiplus::Color cr_text_color /*-1*/)
{
	m_text = sText;
	//CStatic::SetWindowText(sText);

	//-1�̸� �⺻ ������ ���ڻ� ���
 	if (cr_text_color.GetValue() != Gdiplus::Color::Transparent)
 		m_cr_text = cr_text_color;
	
	//�ݺ����ȿ��� �̸� ȣ���� ��� Invalidate()�����δ� �ؽ�Ʈ�� �ٷ� ������� �ʱ⵵ �Ѵ�.

	//�����϶� update_surface()�� ��� ������ ���ŵȴ�.
	//=>dlg���� clip sibling�� ���� ����� �޶�����.
	//if (m_transparent)
		update_surface();
	//else
		Invalidate(false);

	return m_rect_text;
}

void CSCStatic::set_textf(LPCTSTR format, ...)
{
	va_list args;
	va_start(args, format);

	CString text;

	text.FormatV(format, args);

	set_text(text);
}

DWORD CSCStatic::get_text_align()
{
	DWORD dwText = DT_NOCLIP;
	DWORD dwStyle = GetStyle();

	MAP_STYLE(SS_LEFT, DT_LEFT);
	MAP_STYLE(SS_RIGHT, DT_RIGHT);
	MAP_STYLE(SS_CENTER, DT_CENTER);
	//MAP_STYLE(SS_CENTERIMAGE,	DT_VCENTER | DT_SINGLELINE	);
	MAP_STYLE(SS_NOPREFIX, DT_NOPREFIX);
	MAP_STYLE(SS_WORDELLIPSIS, DT_WORD_ELLIPSIS);
	MAP_STYLE(SS_ENDELLIPSIS, DT_END_ELLIPSIS);
	MAP_STYLE(SS_PATHELLIPSIS, DT_PATH_ELLIPSIS);

	NMAP_STYLE(SS_LEFTNOWORDWRAP |
		//SS_CENTERIMAGE |
		SS_WORDELLIPSIS |
		SS_ENDELLIPSIS |
		SS_PATHELLIPSIS, DT_WORDBREAK);

	if (dwStyle & SS_CENTERIMAGE)
		dwText |= (DT_VCENTER | DT_SINGLELINE);
	else
		dwText |= DT_TOP;

	return dwText;
}

int CSCStatic::get_halign()
{
	DWORD dwStyle = GetStyle();
	if (dwStyle & SS_CENTER)
		return DT_CENTER;
	else if (dwStyle & DT_RIGHT)
		return DT_RIGHT;

	return DT_LEFT;
}

int CSCStatic::get_valign()
{
	DWORD dwStyle = GetStyle();
	if (dwStyle & SS_CENTERIMAGE)
		return DT_VCENTER;

	return DT_TOP;
}

/*
void CSCStatic::set_halign()
{

}

void CSCStatic::set_valign()
{

}
*/

void CSCStatic::set_back_image(CString type, UINT nIDBack, Gdiplus::Color cr_back)
{
	m_img_back.load(type, nIDBack);
	m_cr_back = cr_back;

	//m_transparent = true;
	m_img_back.set_back_color(m_cr_back);

	if (m_img_back.is_animated_gif())
		m_img_back.set_animation(m_hWnd, CRect(), false);
}

void CSCStatic::set_back_image(CSCGdiplusBitmap& img, Gdiplus::Color cr_back)
{
	img.deep_copy(&m_img_back);
	m_cr_back = cr_back;
	//m_transparent = true;
	m_img_back.set_back_color(m_cr_back);

	if (m_img_back.is_animated_gif())
		m_img_back.set_animation(m_hWnd, CRect(), false);
}

//��� �̹����� �¿��Ī�ϴµ� ���� animated gif��� �����ó�� ���۽�ų �� �ִ�.
void CSCStatic::set_back_image_mirror(bool is_mirror)
{
	if (m_img_back.is_animated_gif())
		m_img_back.set_gif_mirror(is_mirror);
	else
		m_img_back.mirror();
}

void CSCStatic::play_gif()
{
	if (!m_img_back.is_animated_gif())
		return;

	m_img_back.play_gif();
}

//pos��ġ�� �̵��� �� �Ͻ������Ѵ�. -1�̸� pause <-> play�� ����Ѵ�.
void CSCStatic::pause_gif(int pos)
{
	if (!m_img_back.is_animated_gif())
		return;

	m_img_back.pause_gif(pos);
}

//animation thread�� ����ǰ� ȭ�鿡�� �� �̻� ǥ�õ��� �ʴ´�. ���� �״�� ���߱� ���Ѵٸ� pause_animation()�� ȣ���Ѵ�.
void CSCStatic::stop_gif()
{
	if (!m_img_back.is_animated_gif())
		return;

	m_img_back.stop_gif();
}


//��� �̹��� ũ�⿡ �°� ��Ʈ���� resize�Ѵ�.
void CSCStatic::fit_to_back_image(bool fit_to_image)
{
	m_fit_to_back_image = fit_to_image;

	if (m_img_back.is_empty())
		return;

	if (fit_to_image)
	{
		SetWindowPos(NULL, 0, 0, m_img_back.width, m_img_back.height, SWP_NOMOVE | SWP_NOZORDER);
	}
	else
	{
		CRect rc;
		GetClientRect(rc);

		if (m_img_back.is_animated_gif())
		{
			m_img_back.set_ani_width(rc.Width());
			m_img_back.set_ani_height(rc.Height());
		}
		else
		{
			m_img_back.resize(rc.Width(), rc.Height());
		}
	}
}

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
		Gdiplus::Color cr = m_crGradient[m_crGradient.size()-1];
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

	CWnd* parent = GetParent();
	if (parent == NULL)
		return;

	parent->ScreenToClient(&rc);
	parent->InvalidateRect(rc, false);
	parent->UpdateWindow();
}

BOOL CSCStatic::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class

	//�� �ڵ带 �־���� disabled������ ������ �����ϴµ�
	//�� �ڵ带 ��Ʈ�� Ŭ������ �־��൵ �ҿ����.
	//�� �ڵ�� main�� �־�߸� disable �����϶��� �� ǥ�õȴ�.
	if (m_use_tooltip && m_tooltip && m_tooltip->m_hWnd)
	{
		//msg�� ���� �����ؼ� ������� �ʰ� *pMsg�� �״�� �̿��ϸ� �̻��� ������ �߻��Ѵ�.
		MSG msg = *pMsg;
		msg.hwnd = (HWND)m_tooltip->SendMessage(TTM_WINDOWFROMPOINT, 0, (LPARAM) & (msg.pt));

		CPoint pt = msg.pt;

		if (msg.message >= WM_MOUSEFIRST && msg.message <= WM_MOUSELAST)
			::ScreenToClient(msg.hwnd, &pt);

		msg.lParam = MAKELONG(pt.x, pt.y);

		// relay mouse event before deleting old tool 
		m_tooltip->SendMessage(TTM_RELAYEVENT, 0, (LPARAM)&msg);
	}

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

void CSCStatic::set_icon(HICON hIcon, int nSize, bool left_align_fix)
{
	ICONINFO	iconInfo;

	m_hIcon = hIcon;

	::ZeroMemory(&iconInfo, sizeof(ICONINFO));
	BOOL bRetValue = ::GetIconInfo(m_hIcon, &iconInfo);

	if (bRetValue == false)
		return;

	m_sz_icon.cx = nSize;//(DWORD)(iconInfo.xHotspot * 2);
	m_sz_icon.cy = nSize;//(DWORD)(iconInfo.yHotspot * 2);

	m_image_left_align_fix = left_align_fix;

	//GetIconInfo �Լ��� hbmMask�� hbmColor ��Ʈ���� �����Ͽ� �����ϹǷ�
	//hbmMask�� hbmColor ��Ʈ���� �������־�� ��. �׷��� ������ GDI��ü�� ��� �þ.
	::DeleteObject(iconInfo.hbmMask);
	::DeleteObject(iconInfo.hbmColor);


	CRect	rc, rParentRect;

	GetWindowRect(rc);

	//��Ʈ���� ũ�Ⱑ �������� ũ�⺸�� �۴ٸ� ��Ʈ���� ũ�⸦ �������� ũ��� �����ش�?
	//��, �ؽ�Ʈ�� ������ ��쿡�� �߰����� �ڵ��� �ʿ��ϴ�.
	//if (m_hIcon && rc.Width() < nSize)
	//	SetWindowPos(NULL, rc.left, rc.top, nSize, rc.Height(), SWP_NOZORDER | SWP_NOMOVE);
	//if (m_hIcon && rc.Height() < nSize)
	//	SetWindowPos(NULL, rc.left, rc.top, rc.Width(), nSize, SWP_NOZORDER | SWP_NOMOVE);

	Invalidate();
}

void CSCStatic::set_margin(CRect margin)
{
	m_margin = margin;
}

void CSCStatic::set_icon(UINT nIDResource, int nSize, bool left_align_fix)
{
	if (m_hWnd == NULL)
		return;

	if (m_hIcon)
		::DestroyIcon(m_hIcon);

	//HINSTANCE hInstResource = AfxFindResourceHandle(MAKEINTRESOURCE(nIDResource), RT_GROUP_ICON);
	//m_hIcon = (HICON)::LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(nIDResource), IMAGE_ICON, nSize, nSize, 0);
	//m_hIcon = (HICON)(::LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(nIDResource), IMAGE_ICON, nSize, nSize, LR_DEFAULTCOLOR | LR_LOADTRANSPARENT));
	m_hIcon = load_icon(AfxGetInstanceHandle(), nIDResource, nSize, nSize);

	if (m_hIcon == NULL)
		return;

	set_icon(m_hIcon, nSize, left_align_fix);
}

void CSCStatic::set_round(int round, Gdiplus::Color gcr_border, Gdiplus::Color	gcr_parent_back)
{
	if (round < 0)
		round = 0;

	m_round = round;

	if (gcr_border.GetValue() != Gdiplus::Color::Transparent)
	{
		m_draw_border = true;
		m_gcr_border = gcr_border;
	}

	if (gcr_parent_back.GetValue() != Gdiplus::Color::Transparent)
	{
		m_cr_parent_back = gcr_parent_back;
	}

	if (m_round > 0)
		set_transparent(true, m_cr_parent_back);
	else
		Invalidate();
}

void CSCStatic::set_font_name(const CString& strFont, BYTE byCharSet)
{
	m_lf.lfCharSet = byCharSet;

	_tcscpy_s(m_lf.lfFaceName, _countof(m_lf.lfFaceName), strFont);
	reconstruct_font();
	update_surface();
}

void CSCStatic::set_font_size(int nSize)
{
	m_lf.lfHeight = get_pixel_size_from_font_size(m_hWnd, nSize);
	reconstruct_font();
	update_surface();
}

void CSCStatic::enlarge_font_size(bool enlarge)
{
	int font_size = get_font_size_from_pixel_size(m_hWnd, m_lf.lfHeight);
	enlarge ? font_size++ : font_size--;

	if (font_size < 4)
		font_size = 4;
	if (font_size > 40)
		font_size = 40;

	m_lf.lfHeight = get_pixel_size_from_font_size(GetParent()->GetSafeHwnd(), font_size);

	reconstruct_font();
}

void CSCStatic::get_auto_font_size(CWnd* pWnd, CRect r, CString text, LOGFONT *lf)
{
	CClientDC dc(this);
	double lower = 0.8;
	double upper = 0.9;
	int resize_count = 0;

	CFont font, *pOldFont;

	while (resize_count++ < 50)
	{
		font.DeleteObject();
		font.CreateFontIndirect(lf);

		pOldFont = (CFont*)dc.SelectObject(&font);

		CSize sz = dc.GetTextExtent(text);
		//TRACE(_T("sz.cx = %d, rc.width * %.1f = %.1f ~ rc.width * %.1f = %.1f\n"), sz.cx, lower, r.Width() * lower, upper, r.Width() * upper);
		if (sz.cx > r.Width() * upper)
		{
			if (lf->lfHeight > -12)
				break;

			lf->lfHeight++;
			//TRACE(_T("lf->lfHeight = %d\n"), lf->lfHeight);
			dc.SelectObject(pOldFont);
		}
		else if (sz.cx < r.Width() * lower)
		{
			lf->lfHeight--;
			//TRACE(_T("lf->lfHeight = %d\n"), lf->lfHeight);
			dc.SelectObject(pOldFont);
		}
		else
		{
			break;
		}
	}

	font.DeleteObject();
}

void CSCStatic::set_auto_font_size(bool auto_font_size)
{
	CRect rc;

	GetClientRect(rc);

	m_auto_font_size = auto_font_size;

	if (auto_font_size)
	{
		get_auto_font_size(this, rc, m_text, &m_lf);
		reconstruct_font();
	}
	else
	{
		set_font_size(m_nFontSize);
	}
}

void CSCStatic::set_font_bold(int weight)
{
	m_lf.lfWeight = weight;
	reconstruct_font();
	update_surface();
}

void CSCStatic::set_font_underline(bool bSet)
{
	m_lf.lfUnderline = bSet;
	reconstruct_font();
	update_surface();
}

void CSCStatic::set_font_italic(bool italic)
{
	m_lf.lfItalic = italic;
	reconstruct_font();
	update_surface();
}

void CSCStatic::set_font_antialiased(bool bAntiAliased)
{
	m_bFontAntiAliased = bAntiAliased;
	m_lf.lfQuality = (m_bFontAntiAliased ? ANTIALIASED_QUALITY : DEFAULT_QUALITY);
	reconstruct_font();
	update_surface();
}

void CSCStatic::set_text_color(Gdiplus::Color cr_text)
{
	if (cr_text.GetValue() != Gdiplus::Color::Transparent)
		m_cr_text = cr_text;

	update_surface();
	Invalidate();
}

void CSCStatic::set_back_color(Gdiplus::Color cr_back)
{
	if (cr_back.GetValue() != Gdiplus::Color::Transparent)
		m_cr_back = cr_back;

	if (m_round <= 0)
		m_transparent = false;

	Invalidate();
}

void CSCStatic::set_gradient(bool bGradient)
{
	m_bGradient = bGradient;

	if (m_bGradient)
		m_transparent = false;
}

void CSCStatic::set_gradient_color(Gdiplus::Color crGradient)
{
	set_gradient();
	m_crGradient.clear();
	m_crGradient.push_back(crGradient);
}

void CSCStatic::set_gradient_color(int idx, Gdiplus::Color crGradient)
{
	set_gradient();
	std::deque<Gdiplus::Color>::iterator it;

	if (idx >= m_crGradient.size())
	{
		int loop = idx -  m_crGradient.size() + 1;
		for (int i = 0; i < loop; i++)
			m_crGradient.push_back(crGradient);
	}
	else
	{
		m_crGradient[idx] = crGradient;
	}
}
/*
void CSCStatic::set_gradient_color(int count, ...)
{
	set_gradient();
	m_crGradient.clear();

	va_list arglist;
	va_start(arglist, count);
	
	for (int i = 0; i < count; i++)
		m_crGradient.push_back(va_arg(arglist, DWORD));
	
	va_end(arglist);
}
*/
void CSCStatic::add_gradient_color(Gdiplus::Color crGradient)
{
	set_gradient();
	m_crGradient.push_back(crGradient);
}

void CSCStatic::insert_gradient_color(int idx, Gdiplus::Color crGradient)
{
	set_gradient();
	std::deque<Gdiplus::Color>::iterator it;

	if (idx >= m_crGradient.size())
	{
		int loop = idx -  m_crGradient.size();
		for (int i = 0; i < loop; i++)
			m_crGradient.push_back(crGradient);
	}

	it = m_crGradient.begin();
	m_crGradient.insert(it + idx, crGradient);
}

void CSCStatic::set_vertical_gradient(bool bVertical)
{
	set_gradient();
	m_bVertical = bVertical;
	Invalidate();
}

//this function will be used only if msimg32.dll library is not available
void CSCStatic::draw_gradient_rect(CDC *pDC, CRect r, Gdiplus::Color cLeft, Gdiplus::Color cRight, BOOL a_bVertical)
{
	CRect stepR;					// rectangle for color's band
	Gdiplus::Color color;			// color for the bands
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
		color = Gdiplus::Color(255, ((cRight.GetR() - cLeft.GetR()) * ((float)iOnBand) / 255.0f + cLeft.GetR()),
			(cRight.GetG() - cLeft.GetG()) * ((float)iOnBand) / 255.0f + cLeft.GetG(),
			(cRight.GetB() - cLeft.GetB()) * ((float)iOnBand) / 255.0f + cLeft.GetB());
		// fill current band
		pDC->FillSolidRect(stepR, color.ToCOLORREF());
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

void CSCStatic::add_header_image(UINT id, bool left_align_fix)
{
	CSCGdiplusBitmap* img = new CSCGdiplusBitmap(_T("PNG"), (UINT)id);
	m_header_images.push_back(img);
	m_image_left_align_fix = left_align_fix;
	//RedrawWindow();
	//UpdateWindow();
	//Invalidate();
}


void CSCStatic::OnSize(UINT nType, int cx, int cy)
{
	CStatic::OnSize(nType, cx, cy);

	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰��մϴ�.
	if (m_hWnd)
	{
		CRect rc;

		GetClientRect(rc);

		if (m_auto_font_size)
		{
			get_auto_font_size(this, rc, m_text, &m_lf);
			reconstruct_font();
		}
		else if (m_img_back.is_valid() && !m_fit_to_back_image)
		{
			if (m_img_back.is_animated_gif())
			{
				m_img_back.set_ani_width(rc.Width());
				m_img_back.set_ani_height(rc.Height());
			}
			else
			{
				m_img_back.resize(rc.Width(), rc.Height());
			}
		}

		Invalidate();
	}
}

void CSCStatic::set_color(Gdiplus::Color cr_text, Gdiplus::Color cr_back)
{
	m_cr_text = cr_text;

	if (cr_back.GetValue() != Gdiplus::Color::Transparent)
	{
		m_cr_back = cr_back;
		m_transparent = false;
	}

	Invalidate();
}


void CSCStatic::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	if (!m_link_url.IsEmpty())
	{
		ShellExecute(NULL, _T("open"), m_link_url, 0, 0, SW_SHOWNORMAL);
	}

	CStatic::OnLButtonDown(nFlags, point);
}


BOOL CSCStatic::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	if (!m_link_url.IsEmpty())
	{
		::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_HAND));
		return TRUE;
	}

	return CStatic::OnSetCursor(pWnd, nHitTest, message);
}

//CStatic�� Ŭ�� �̺�Ʈ�� ó���Ϸ��� SS_NOTIFY �Ӽ� �ʼ��̹Ƿ� �ڵ� �����ǵ��� ������.
void CSCStatic::set_link(CString url, Gdiplus::Color cr_link)
{
	ModifyStyle(0, SS_NOTIFY);
	m_link_url = url;
	m_cr_link = cr_link;
}

//align ������� ���� �ؽ�Ʈ�� �ʺ� �˰��� �� ��� ���.
CSize CSCStatic::get_text_extent()
{
	CClientDC dc(this);
	CFont* pOldFont = dc.SelectObject(&m_font);
	CSize sz = dc.GetTextExtent(m_text);
	return sz;
}

//parent���� ���� �� static�� ��ġ�� ����.
CRect CSCStatic::get_rect()
{
	CWnd* parent = GetParent();

	CRect r;
	GetWindowRect(r);
	parent->ScreenToClient(r);

	return r;
}
