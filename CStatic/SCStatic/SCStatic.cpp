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

	//functions.h에 gradient_fill을 위해서 선언된 이 핸들을 사용하는 프로그램이라면
	//종료될 때 해제시켜주자.
	//일단 이 클래스에서 사용하므로 이 클래스의 소멸자에서 해제 함수를 호출해주고 있으나
	//공통 글로벌 변수라서 뭔가 부작용이 있을수도 있다.
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

	//동적 생성시에는 이미 m_text에 들어가지만 정적 생성시에는 직접 얻어와야 한다.
	if (m_text.IsEmpty())
		CStatic::GetWindowText(m_text);

	//modified the style to avoid text overlap when press tab 
	ModifyStyle(0, BS_ICON);

	m_halign = get_halign();
	m_valign = get_valign();

	//Resource Editor에서 이 컨트롤을 사용하는 dlg에 적용된 폰트를 기본으로 사용해야 한다.
	//단, 동적으로 생성된 클래스에서 이 클래스를 사용하거나
	//아직 MainWnd가 생성되지 않은 상태에서도 이 코드를 만날 수 있으므로
	//pWnd = NULL일 수 있다.
	CWnd* pWnd = GetParent();

	if (pWnd == NULL)
		pWnd = AfxGetApp()->GetMainWnd();

	CFont* font = NULL;

	if (pWnd)
		font = pWnd->GetFont();

	if (font == NULL)
	{
		GetObject(GetStockObject(SYSTEM_FONT), sizeof(m_lf), &m_lf);
		//SYSTEM_FONT를 얻어오면 m_lf.lfWidth가 8로 세팅되는데
		//이후 font_size, font_name을 변경해도 이 값이 그대로이므로
		//가로로 넓어진 글꼴로 표시된다.
		//따라서 m_lf.lfWidth를 0으로 초기화해줘야만 lfHeight에 따라 자동으로 조절된 글꼴로 올바르게 표시된다.
		//의도적으로 넓이를 지정하지 않는 한 이 값은 0으로 설정해야 한다.
		m_lf.lfWidth = 0; // 0 means default width
	}
	else
	{
		font->GetObject(sizeof(m_lf), &m_lf);
	}

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
	m_tooltip->SetMaxTipWidth(240);
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
	//CStatic에서 tooltip, mouse event등을 처리하려면 반드시 SS_NOTIFY 스타일을 추가해야 한다.
	ModifyStyle(0, SS_NOTIFY);

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

	//투명 모드이면 배경도 안칠하고 글자도 배경색 없이 출력된다.
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
		// 배경 그림을 그린다.
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

		//SS_LEFTNOWORDWRAP 스타일은 DT_WORDBREAK와 함께 사용하면 안된다.
		//즉, SS_LEFTNOWORDWRAP이면 텍스트는 자동으로 SS_LEFT가 되고 자동 줄바꿈 되지 않는다.
		//다만 SS_CENTERIMAGE까지 포함되면 스타일이 너무 제한되므로 SS_CENTERIMAGE는 체크 대상에서 제외시킨다.
		NMAP_STYLE(	SS_LEFTNOWORDWRAP |
					//SS_CENTERIMAGE |
					SS_WORDELLIPSIS |
					SS_ENDELLIPSIS |
					SS_PATHELLIPSIS,	DT_WORDBREAK);
	}

	//텍스트 출력 루틴
	CString sSpace = _T("");

	//문자열 앞에 공백 넣기 옵션이 있을 경우 공백 추가
	if (!m_text.IsEmpty())
	{
		for (int i = 0; i < m_nPrefixSpace; i++)
			sSpace = _T(" ") + sSpace;
	}

	//텍스트의 출력 크기를 구한다.
	CSize szText;

	//20250820 scpark. DT_CALCRECT 시에도 dwText를 반영해주는 것이 정석이지만
	//여러가지 다양한 스타일을 지원하기 위해서는 dwText를 제외하고 DT_CALCRECT해야만 지원 가능하다.
	dc.DrawText(sSpace + m_text, &m_rect_text, /*dwText | */DT_CALCRECT);
	m_text_extent = m_rect_text.Width();
	//TRACE(_T("width = %d (%s)\n"), m_rect_text.Width(), m_text);
	szText.cx = m_rect_text.Width();// + m_nOutlineWidth * 2;
	szText.cy = m_rect_text.Height();// + m_nOutlineWidth * 2;

	m_rect_text = rc;

	//아이콘 또는 헤더 이미지와 텍스트 사이의 갭 크기지만 이미지의 여백, 크기 정도에 따라 갭은 비율로 조정되도록 수정되어야 한다.
	int gap = 4;

	//아이콘이 있으면 아이콘을 그려준다.
	if (m_hIcon != NULL)
	{
		gap = 4;

		CRect rIcon = m_rect_text;
		CSize szIcon = m_sz_icon;

		//아이콘의 너비만큼 텍스트는 밀려서 출력된다.
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

				//top 정렬인데 아이콘이 커서 상단을 벗어난다면 이미지, 텍스트 모두 아래로 내려준다.
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

		//보통 align은 텍스트가 중앙 정렬이면 "image + header" 둘 다 중앙에, LEFT이면 둘 다 왼쪽부터 정렬되지만
		//m_image_left_align_fix가 true로 설정된 경우에는 이미지만 왼쪽에 정렬하고 텍스트는 지정된 align으로 정렬된다.
		//특별이 이러한 경우가 적용되는 곳은 바로 CSCMessageBox이다.
		//AfxMessageBox() 또는 그와 유사한 메시지박스들을 보면 아이콘은 항상 왼쪽에 표시되고 텍스트는 아이콘과 관계없이 지정된 정렬방식으로 표시된다.

		//header image의 너비만큼 텍스트는 밀려서 출력된다.
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

				//top 정렬인데 아이콘이 커서 상단을 벗어난다면 이미지, 텍스트 모두 아래로 내려준다.
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
		//이 코드가 있어서 center image가 false인데도 세로 중앙에 표시된다. 왜 이 코드가 있는지 확인하자!
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

		//SS_CENTERIMAGE 일 경우 DT_VCENTER는 반드시 DT_SINGLELINE을 동반해야 한다.
		//하지만 그럴 경우 multiline을 표현할 수 없으므로
		//SS_CENTERIMAGE 일 경우는 세로 출력위치를 직접 보정해준다.
		if (dwStyle & SS_CENTERIMAGE)
		{
			m_rect_text.top = (rc.Height() - szText.cy) / 2;
			m_rect_text.bottom = m_rect_text.top + szText.cy;
		}
		else
		{
		}

		//아이콘이 있을 경우에는 아이콘의 좌표가 결정되고 텍스트의 좌표 또한
		//정렬 방식에 따라 자동 결정된다. 따라서 정렬 플래그 값을 무시하도록 해야 정상 표시된다.
		//이는 차후 위의 아이콘과 텍스트에 대한 좌표값 계산 루틴을 전체적으로 수정해야 할 듯하다.
		if (m_hIcon || (m_header_images.size() > 0))
		{
			dwText &= ~(DT_CENTER);
			dwText &= ~(DT_RIGHT);
			dwText &= ~(DT_VCENTER);
		}

		if (!m_bBlinkStatus)
		{
			//CStatic은 기본적으로 WORDBREAK(WORDWRAP) 스타일이 적용되지 않으나
			//이 CSCStatic을 CMessageBox에서 사용하니 WORDBREAK 옵션이 적용되어야만 한다.
			//m_use_word_wrap = true;과 같은 옵션을 추가할 수도 있으나 좀 더 일반적인 방법으로 변경해야 한다.
			//DT_WORDBREAK를 기본 제외시켰던 이유는 CStatic이 기본 WORDBREAK 스타일을 사용하지 않기 때문이기도 하고
			//m_rect_text를 정확히 계산하기 위함도 있었으나 m_rect_text이 실제 텍스트가 출력되는 영역으로 세팅되는지는
			//다시 확인이 필요하다.
			dc.DrawText(sSpace + m_text, m_rect_text, dwText);// | DT_WORDBREAK);
			//DrawShadowText(dc.GetSafeHdc(), sSpace + m_text, CString(sSpace + m_text).GetLength(), m_rect_text,
			//	DT_CENTER | DT_TOP | DT_NOCLIP, m_cr_text.ToCOLORREF(), 0, 2, 1);
		}
	}

	if (m_draw_border)
	{
		//TRACE(_T("draw_border\n"));
		if (m_round > 0)
			draw_round_rect(&g, CRect2GpRect(rc), m_cr_border, Gdiplus::Color::Transparent, m_round, m_border_thick);
		else
			draw_rect(g, rc, m_cr_border);
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

CRect CSCStatic::set_text(CString text, Gdiplus::Color cr_text_color /*-1*/)
{
	m_text = text;

	//-1이면 기본 설정된 글자색 사용
 	if (cr_text_color.GetValue() != Gdiplus::Color::Transparent)
 		m_cr_text = cr_text_color;
	
	//반복문안에서 이를 호출할 경우 Invalidate()만으로는 텍스트가 바로 변경되지 않기도 한다.

	//투명일때 update_surface()를 써야 온전히 갱신된다.
	//=>dlg에서 clip sibling에 따라 결과가 달라진다.
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
		m_img_back.set_animation(m_hWnd, CRect(), true);
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

//배경 이미지를 좌우대칭하는데 만약 animated gif라면 역재생처럼 동작시킬 수 있다.
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

//pos위치로 이동한 후 일시정지한다. -1이면 pause <-> play를 토글한다.
void CSCStatic::pause_gif(int pos)
{
	if (!m_img_back.is_animated_gif())
		return;

	m_img_back.pause_gif(pos);
}

//animation thread가 종료되고 화면에도 더 이상 표시되지 않는다. 만약 그대로 멈추길 원한다면 pause_animation()을 호출한다.
void CSCStatic::stop_gif()
{
	if (!m_img_back.is_animated_gif())
		return;

	m_img_back.stop_gif();
}


//배경 이미지 크기에 맞게 컨트롤을 resize한다.
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



// 또 다른 방법
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

	//이 코드를 넣어줘야 disabled에서도 툴팁이 동작하는데
	//이 코드를 컨트롤 클래스에 넣어줘도 소용없다.
	//이 코드는 main에 있어야만 disable 상태일때도 잘 표시된다.
	if (m_use_tooltip && m_tooltip && m_tooltip->m_hWnd)
	{
		//msg를 따로 선언해서 사용하지 않고 *pMsg를 그대로 이용하면 이상한 현상이 발생한다.
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

	//GetIconInfo 함수는 hbmMask와 hbmColor 비트맵을 생성하여 리턴하므로
	//hbmMask와 hbmColor 비트맵을 해제해주어야 함. 그렇지 않으면 GDI개체가 계속 늘어남.
	::DeleteObject(iconInfo.hbmMask);
	::DeleteObject(iconInfo.hbmColor);


	CRect	rc, rParentRect;

	GetWindowRect(rc);

	//컨트롤의 크기가 아이콘의 크기보다 작다면 컨트롤의 크기를 아이콘의 크기로 맞춰준다?
	//단, 텍스트가 존재할 경우에는 추가적인 코딩이 필요하다.
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

void CSCStatic::set_round(int round, Gdiplus::Color cr_border, Gdiplus::Color cr_parent_back)
{
	if (round < 0)
		round = 0;

	m_round = round;

	if (cr_border.GetValue() != Gdiplus::Color::Transparent)
	{
		m_draw_border = true;
		m_cr_border = cr_border;
	}

	if (cr_parent_back.GetValue() != Gdiplus::Color::Transparent)
	{
		m_cr_parent_back = cr_parent_back;
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

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
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
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (!m_link_url.IsEmpty())
	{
		ShellExecute(NULL, _T("open"), m_link_url, 0, 0, SW_SHOWNORMAL);
	}

	CStatic::OnLButtonDown(nFlags, point);
}


BOOL CSCStatic::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (!m_link_url.IsEmpty())
	{
		::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_HAND));
		return TRUE;
	}

	return CStatic::OnSetCursor(pWnd, nHitTest, message);
}

//CStatic의 클릭 이벤트를 처리하려면 SS_NOTIFY 속성 필수이므로 자동 설정되도록 수정함.
void CSCStatic::set_link(CString url, Gdiplus::Color cr_link)
{
	ModifyStyle(0, SS_NOTIFY);
	m_link_url = url;
	m_cr_link = cr_link;
}

//align 관계없이 현재 텍스트의 너비를 알고자 할 경우 사용.
CSize CSCStatic::get_text_extent()
{
	CClientDC dc(this);
	CFont* pOldFont = dc.SelectObject(&m_font);
	CSize sz = dc.GetTextExtent(m_text);
	return sz;
}

//parent에서 현재 이 static의 위치를 리턴.
CRect CSCStatic::get_rect()
{
	CWnd* parent = GetParent();

	CRect r;
	GetWindowRect(r);
	parent->ScreenToClient(r);

	return r;
}
