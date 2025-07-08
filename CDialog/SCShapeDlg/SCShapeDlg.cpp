// SCShapeDlg.cpp: 구현 파일
//

#include "SCShapeDlg.h"
#include "afxdialogex.h"

#include <thread>
#include "../../Functions.h"

// CSCShapeDlg 대화 상자

IMPLEMENT_DYNAMIC(CSCShapeDlg, CDialogEx)

CSCShapeDlg::CSCShapeDlg()
{
}

CSCShapeDlg::~CSCShapeDlg()
{
}

void CSCShapeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSCShapeDlg, CDialogEx)
	ON_WM_LBUTTONDOWN()
	ON_WM_WINDOWPOSCHANGED()
	ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()


// CSCShapeDlg 메시지 처리기


BOOL CSCShapeDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  여기에 추가 초기화 작업을 추가합니다.
	return TRUE;  // return TRUE unless you set the focus to a control
				  // 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}


BOOL CSCShapeDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

	//모든 키입력은 parent에서 처리.
	if (pMsg->message == WM_KEYDOWN)
	{
		//parent의 PreTranslateMessage()에서 해당 키를 처리하는 코드가 있어야 한다.
		//ex. VK_ESCAPE키를 눌렀을 때 mainDlg에서 OnBnClickedCancel()가 자동 호출되리라 예상했으나
		//자동 호출되지 않으므로 처리코드를 추가해야 한다.
		//Traceln(_T(""));
		//::SendMessage(m_parent->m_hWnd, WM_KEYDOWN, pMsg->wParam, pMsg->lParam);
		return FALSE;
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}


void CSCShapeDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	DefWindowProc(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(point.x, point.y));

	CDialogEx::OnLButtonDown(nFlags, point);
}

bool CSCShapeDlg::create(CWnd* parent, int left, int top, int right, int bottom)
{
	m_parent = parent;

	DWORD dwStyle = WS_POPUP;
	//DWORD dwStyle = WS_CHILD | WS_VISIBLE;

	WNDCLASS wc = {};
	::GetClassInfo(AfxGetInstanceHandle(), _T("#32770"), &wc);
	wc.lpszClassName = _T("CSCShapeDlg");
	AfxRegisterClass(&wc);

	bool res = CreateEx(NULL, wc.lpszClassName, _T("SCShapeDlg"), dwStyle, CRect(left, top, right, bottom), parent, 0);

	//TRACE(_T("create. rect = (%d,%d) (%d,%d)\n"), left, top, right, bottom);
	CRect rc;
	GetClientRect(rc);
	//TRACE(_T("rc = %s\n"), get_rect_info_string(rc));

	dwStyle = GetWindowLongPtr(m_hWnd, GWL_STYLE);
	dwStyle &= ~(WS_CAPTION);
	SetWindowLongPtr(m_hWnd, GWL_STYLE, dwStyle);

	dwStyle = GetWindowLong(m_hWnd, GWL_EXSTYLE) | WS_EX_LAYERED;
	SetWindowLongPtr(m_hWnd, GWL_EXSTYLE, dwStyle);

	GetClientRect(rc);
	TRACE(_T("rc = %s\n"), get_rect_info_string(rc));

	return res;
}

CSCShapeDlgTextSetting* CSCShapeDlg::set_text(CString str)
{
	m_text_setting.text = str;
	return set_text();
}

CSCShapeDlgTextSetting* CSCShapeDlg::set_text(CSCShapeDlgTextSetting* setting)
{
	if (setting != NULL)
		memcpy(&m_text_setting, setting, sizeof(CSCShapeDlgTextSetting));

	return set_text(m_parent, m_text_setting.text, m_text_setting.text_prop.size, m_text_setting.text_prop.style, m_text_setting.text_prop.shadow_depth, m_text_setting.text_prop.thickness,
					m_text_setting.text_prop.name, m_text_setting.text_prop.cr_text, m_text_setting.text_prop.cr_stroke, m_text_setting.text_prop.cr_shadow, m_text_setting.text_prop.cr_back);
}

//gdiplus를 이용한 text 출력용 dlg 생성
CSCShapeDlgTextSetting* CSCShapeDlg::set_text(CWnd* parent, CString text, 
						float font_size,
						int font_style,
						float shadow_depth,
						float thickness,
						CString font_name,
						Gdiplus::Color cr_text,
						Gdiplus::Color cr_stroke,
						Gdiplus::Color cr_shadow,
						Gdiplus::Color cr_back)
{
	m_para.clear();

	CSCTextProperty text_prop;

	if (parent == NULL)
		m_parent = parent = AfxGetApp()->GetMainWnd();
	else
		m_parent = parent;

	//font_name이 ""이라면 main의 LOGFONT를 구해서 lfFaceName값으로 채워준다.
	//main이 아닌 GetParent()를 통해 font를 구하면 그 parent가 동적생성된 경우에는 제대로 가져오지 못하므로 mainWnd()를 구해서 가져올 것!
	if (font_name.IsEmpty())
	{
		CFont* font = AfxGetApp()->GetMainWnd()->GetFont();
		LOGFONT lf;

		memset(&lf, 0, sizeof(lf));

		if (font != NULL)
			font->GetObject(sizeof(lf), &lf);
		else
			GetObject(GetStockObject(SYSTEM_FONT), sizeof(lf), &lf);

		font_name = lf.lfFaceName;
	}

	text_prop.name = font_name;
	text_prop.size = font_size;
	text_prop.style = font_style;
	text_prop.cr_text = cr_text;
	text_prop.cr_stroke = cr_stroke;
	text_prop.cr_shadow = cr_shadow;
	text_prop.cr_back = cr_back;
	text_prop.shadow_depth = shadow_depth;
	text_prop.thickness = thickness;

	//아직 윈도우 생성 전이라면 텍스트 출력 크기를 알 수 없으므로 100x100으로 가정한다.
	CRect r(0, 0, 100, 100);

	m_img.release();

	CSCParagraph::build_paragraph_str(text, m_para, &text_prop);
	m_text_setting = CSCShapeDlgTextSetting(text, text_prop);

	if (!m_hWnd)
	{
		bool success = create(parent, 0, 0, r.Width(), r.Height());
		if (!success)
			return &m_text_setting;
	}

	CClientDC dc(this);
	r = CSCParagraph::calc_text_rect(r, &dc, m_para, DT_CENTER | DT_VCENTER);
	r.InflateRect(1, 1);

	m_img.create(r.Width(), r.Height(), PixelFormat32bppARGB, cr_back);
	r = CRect(0, 0, r.Width(), r.Height());
	r = CSCParagraph::calc_text_rect(r, &dc, m_para, DT_CENTER | DT_VCENTER);

	//해당 캔버스에
	Gdiplus::Graphics g(m_img.m_pBitmap);

	g.SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);
	g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
	g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);


	//기본적으로 m_para는 DT_CENTER로 가정하고 계산하므로 DT_CENTER이면 여기서 바로 그려주고
	//DT_LEFT, DT_RIGHT라면 set_text_align()을 한 후 그 함수 안에서 다시 그려준다.
	if (m_text_align == DT_CENTER)
	{
		CSCParagraph::draw_text(g, m_para);
		set_image(parent, &m_img, false);
	}
	else
	{
		set_text_align(m_text_align);
	}

#ifdef _DEBUG
	//m_img.save(_T("d:\\SCShapeDlg.png"));
#endif
	return &m_text_setting;
}

//기본 정렬은 센터정렬로 만들어지지만 DT_LEFT를 주면 모든 라인이 왼쪽 정렬된다. 각 라인마다 따로 정렬을 지정할 수도 있지만 필요성을 따져봐야 한다.
void CSCShapeDlg::set_text_align(int align)
{
	int i, j;

	m_text_align = align;

	if (m_text_align == DT_CENTER)
	{
		//이미 set_text()에서 기본적으로 중앙 정렬하므로 그냥 set_text()를 다시 호출해주면 된다.
		set_text(&m_text_setting);
		return;
	}
	else if (m_text_align == DT_LEFT)
	{
		int min_left = 99999;

		//find min_left
		for (i = 0; i < m_para.size(); i++)
		{
			if (m_para[i][0].r.left < min_left)
				min_left = m_para[i][0].r.left;
		}

		//move all paragraph
		for (i = 0; i < m_para.size(); i++)
		{
			int offset = min_left - m_para[i][0].r.left;
			for (j = 0; j < m_para[i].size(); j++)
			{
				m_para[i][j].r.OffsetRect(offset, 0);
			}
		}
	}
	else if (m_text_align == DT_RIGHT)
	{
		int max_right = 0;

		//find max_right
		for (i = 0; i < m_para.size(); i++)
		{
			if (m_para[i].back().r.right > max_right)
				max_right = m_para[i].back().r.right;
		}

		//move all paragraph
		for (i = 0; i < m_para.size(); i++)
		{
			int offset = max_right - m_para[i].back().r.right;
			for (j = 0; j < m_para[i].size(); j++)
			{
				m_para[i][j].r.OffsetRect(offset, 0);
			}
		}
	}

	//해당 캔버스에 다시 그려준다.
	Gdiplus::Graphics g(m_img.m_pBitmap);
	g.Clear(m_text_setting.text_prop.cr_back);
	CSCParagraph::draw_text(g, m_para);
	set_image(m_parent, &m_img, false);
}

void CSCShapeDlg::set_image(CWnd* parent, CGdiplusBitmap* img, bool deep_copy)
{
	//현재 이미지가 animateGif이고 play중이라면 일단 멈춘다.
	if (m_img.is_animated_gif() && m_gif_state == state_play)
	{
		TRACE(_T("stop current gif...\n"));
		m_gif_state = state_stop;
		Wait(100);
	}

	if (deep_copy)
		img->deep_copy(&m_img);

	m_parent = parent;

	if (!m_hWnd)
	{
		if (!create(m_parent, 0, 0, img->width, img->height))
			return;
	}


	SetWindowPos(NULL, 0, 0, m_img.width, m_img.height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

	//단일 이미지인지 gif인지에 따라 별도 처리
	if (m_img.is_animated_gif())
	{
		gif_play(state_play);
	}
	else
	{
		render(m_img.m_pBitmap);
	}

	set_alpha(255);
	ShowWindow(SW_SHOW);
}

bool CSCShapeDlg::load(CWnd* parent, UINT id)
{
	bool res = m_img.load(id);
	if (res)
		set_image(parent, &m_img, false);
	return res;
}

bool CSCShapeDlg::load(CWnd* parent, CString sType, UINT id)
{
	bool res = m_img.load(sType, id);

	if (res)
		set_image(parent, &m_img, false);

	return res;
}

bool CSCShapeDlg::load(CWnd* parent, CString sFile)
{
	bool res = m_img.load(sFile);
	if (res)
		set_image(parent, &m_img, false);
	return res;
}

//keyboard, mouse 이벤트 처리 여부.
void CSCShapeDlg::use_control(bool use)
{
	ModifyStyleEx(use ? WS_EX_LAYERED | WS_EX_TRANSPARENT : 0, use ? 0 : WS_EX_LAYERED | WS_EX_TRANSPARENT);
	if (use)
		render(m_img.m_pBitmap);
}

//0 : 투명, 255 : 불투명
void CSCShapeDlg::set_alpha(int alpha)
{
	m_alpha = alpha;

	//if (!IsWindowVisible())
	//	ShowWindow(SW_SHOW);

	if (!m_img.is_animated_gif())
		render(m_img.m_pBitmap);
}

void CSCShapeDlg::render(Gdiplus::Bitmap* img)
{
	if (!IsWindow(m_hWnd) || !img)
		return;

	CRect rc;
	GetWindowRect(rc);
	POINT ptSrc = { 0, 0 };
	POINT ptWinPos = { rc.left, rc.top };
	SIZE sz = { img->GetWidth(), img->GetHeight() };
	BLENDFUNCTION stBlend = { AC_SRC_OVER, 0, m_alpha, AC_SRC_ALPHA };

	if (img->GetWidth() == 0 || img->GetHeight() == 0)
	{
		TRACE(_T("image width or height is invalid. m_alpha = %d\n"), m_alpha);
		return;
	}

	HDC hDC = ::GetDC(m_hWnd);
	HDC hdcMemory = ::CreateCompatibleDC(hDC);

	BITMAPINFOHEADER bmih = { 0 };
	int nBytesPerLine = ((sz.cx * 32 + 31) & (~31)) >> 3;
	bmih.biSize = sizeof(BITMAPINFOHEADER);
	bmih.biWidth = sz.cx;
	bmih.biHeight = sz.cy;
	bmih.biPlanes = 1;
	bmih.biBitCount = 32;
	bmih.biCompression = BI_RGB;
	bmih.biClrUsed = 0;
	bmih.biSizeImage = nBytesPerLine * sz.cy;

	PVOID pvBits = NULL;
	HBITMAP hbmpMem = ::CreateDIBSection(NULL, (PBITMAPINFO)&bmih, DIB_RGB_COLORS, &pvBits, NULL, 0);
	if (!hbmpMem)
		return;
	//ASSERT(hbmpMem != NULL);

	memset(pvBits, 0, sz.cx * 4 * sz.cy);
	if (hbmpMem)
	{
		HGDIOBJ hbmpOld = ::SelectObject(hdcMemory, hbmpMem);
		Gdiplus::Graphics g(hdcMemory);

		g.SetPageScale(1.0);
		g.SetPageUnit(Gdiplus::UnitPixel);
		g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

		g.DrawImage(img, 0, 0, sz.cx - 1, sz.cy - 1);
		//Draw customized figures
		//g.DrawRectangle(&Pen(Color(255, 255, 0, 0), 1.0f), Gdiplus::Rect(0, 0, sz.cx - 1, sz.cy - 1));

		::UpdateLayeredWindow(m_hWnd
			, hDC
			, &ptWinPos
			, &sz
			, hdcMemory
			, &ptSrc
			, 0
			, &stBlend
			, ULW_ALPHA
		);

		g.ReleaseHDC(hdcMemory);
		::SelectObject(hdcMemory, hbmpOld);
		::DeleteObject(hbmpMem);
	}

	::DeleteDC(hdcMemory);
	::ReleaseDC(m_hWnd, hDC);
}

//일반 이미지와 animated gif 공통으로 사용한다.
void CSCShapeDlg::gif_play(int new_state)
{
	if (new_state == state_stop)
	{
		m_gif_state = state_stop;
		return;
	}
	else if (new_state == state_toggle)
	{
		if (m_gif_state == state_play)
			m_gif_state = state_pause;
		else if (m_gif_state == state_pause)
			m_gif_state = state_play;
		return;
	}
	else if (new_state == state_pause)
	{
		m_gif_state = state_pause;
		return;
	}

	//기존 gif play중이라면 thread를 종료시킨 후 새로 생성한다.
	if (m_gif_state == state_play)
	{
		m_gif_state = state_stop;
		//Wait(1000);
	}

	m_gif_state = state_play;

	//단일 이미지인지 gif인지에 따라 별도 처리
	std::thread t(&CSCShapeDlg::gif_thread, this);
	t.detach();
}

void CSCShapeDlg::gif_pause()
{
	m_gif_state = state_pause;
}

void CSCShapeDlg::gif_stop()
{
	m_gif_state = state_stop;
}

void CSCShapeDlg::gif_goto(int pos, bool pause)
{

}

void CSCShapeDlg::gif_thread()
{
	TRACE(_T("gif_thread started...\n"));

	GUID   pageGuid = Gdiplus::FrameDimensionTime;
	m_gif_index = 0;

	while (m_gif_state != state_stop)
	{
		if (m_gif_state == state_pause)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
			continue;
		}

		if (!m_img.is_valid() ||
			m_img.m_pBitmap == NULL ||
			m_gif_index > m_img.get_frame_count() ||
			m_img.m_pPropertyItem == NULL)
			break;

		m_img.m_pBitmap->SelectActiveFrame(&pageGuid, m_gif_index);
		render(m_img.m_pBitmap);
		long delay = ((long*)m_img.m_pPropertyItem->value)[m_gif_index] * 10;
		//if (delay < 10)
		//	delay = 10;
		//TRACE(_T("gif_thread %3d = %ld ms\n"), m_gif_index, delay);
		std::this_thread::sleep_for(std::chrono::milliseconds(delay));
		m_gif_index++;

		if (m_gif_index >= m_img.get_frame_count())
		{
			if (true)
				m_gif_index = 0;
			else
				break;
		}
	}

	TRACE(_T("%s terminated."), __function__);
}


void CSCShapeDlg::PreSubclassWindow()
{
	// TODO: Add your specialized code here and/or call the base class

	CDialogEx::PreSubclassWindow();
}

//show상태로 만들고 time후에 hide된다.
void CSCShapeDlg::time_out(int timeout, bool fadein, bool fadeout)
{
	if (timeout <= 0)
	{
		ShowWindow(SW_SHOW);
		return;
	}

	//if (fadein)
	//	fade_in();

	//Wait(timeout * 1000);

	//if (fadeout)
	//	fade_out();
}

#if 1
//set_image(), set_text()를 호출해도 아직 hide상태다.
//ShowWindow()시키거나 fadein()으로 보여지게 한다.
//0 ~ 255까지 25간격으로 alpha를 변경한다.
//hide_after_ms, fadeout 파라미터는 fade_in에서만 사용된다.
void CSCShapeDlg::fade_in(int fade_in_delay_ms, int hide_after_ms, bool fadeout, int fade_out_delay_ms)
{
	if (m_fadeinout_ing)
	{
		TRACE(_T("m_fadeinout_ing = 1. stop...\n"));
		m_fadeinout_ing = false;
		Wait(100);
	}

	std::thread t(&CSCShapeDlg::thread_fadeinout, this, true, fade_in_delay_ms, hide_after_ms, fadeout, fade_out_delay_ms);
	t.detach();
}

void CSCShapeDlg::fade_out(int fade_out_delay_ms)
{
	if (m_fadeinout_ing)
	{
		TRACE(_T("m_fadeinout_ing. stop...\n"));
		m_fadeinout_ing = false;
		Wait(100);
	}

	std::thread t(&CSCShapeDlg::thread_fadeinout, this, false, 0, 0, false, fade_out_delay_ms);
	t.detach();
}

//fade_in(), fade_out() 함수에서 호출하며
//이미 이 thread가 돌고 있는중에 다시 fade_in(), fade_out()이 호출되면
//이 thread는 즉시 중단되고 text를 변경한 후 다시 이 thread가 호출된다.
//이 때 hide시키지 않고 text만 변경시킨다. 그렇지 않으면 사라졌다가 나타나므로 깜빡이게 된다.
void CSCShapeDlg::thread_fadeinout(bool fadein, int fadein_delay_ms, int hide_after_ms, bool fadeout, int fadeout_delay_ms)
{
	m_fadeinout_ing = true;

	set_alpha((fadein && fadein_delay_ms > 0) ? 0 : 255);
	ShowWindow(SW_SHOW);

	int _alpha = m_alpha;

	if (fadein_delay_ms < 0)
		fadein_delay_ms = 10;

	while (m_fadeinout_ing)
	{
		_alpha += (fadein ? 25 : -25);

		if (_alpha < 0 || _alpha > 255)
			break;
		
		set_alpha(_alpha);

		//예를 들어 delay_ms가 10초라고 하면 그 중간에 m_fadeinout_ing이 false가 되어도
		//10초가 모두 지나기 전에는 이 루프를 종료할 수 없게 된다.
		//따라서 짧은 간격으로 딜레이를 수정하고 m_fadeinout_ing = false이면
		//이 thread를 종료시켜줘야 한다.
		int delayed_total = 0;
		while (delayed_total < fadein_delay_ms)
		{
			//TRACE(_T("delayed_total = %d\n"), delayed_total);
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			delayed_total += 10;
			if (!m_fadeinout_ing)
				break;
		}
	}

	if (!m_fadeinout_ing)
		return;

	if (hide_after_ms > 0)
	{
		int hide_after_delayed_total = 0;
		while (hide_after_delayed_total < hide_after_ms)
		{
			//TRACE(_T("hide_after_delayed_total = %d\n"), hide_after_delayed_total);
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			hide_after_delayed_total += 10;
			if (!m_fadeinout_ing)
				break;
		}
	}
	else
	{
		return;
	}

	if (fadeout)
	{
		set_alpha(255);
		int _alpha = m_alpha;

		if (fadeout_delay_ms < 0)
			fadeout_delay_ms = 10;

		while (m_fadeinout_ing)
		{
			_alpha -= 5;

			if (_alpha < 0 || _alpha > 255)
			{
				ShowWindow(SW_HIDE);
				break;
			}

			set_alpha(_alpha);

			int delayed_total = 0;
			while (delayed_total < fadeout_delay_ms)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				delayed_total += 10;
				if (!m_fadeinout_ing)
					break;
			}
		}

		set_alpha(255);
	}

	m_fadeinout_ing = false;
	TRACE(_T("thread_fadeinout stopped.\n"));
}
#endif

void CSCShapeDlg::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CDialogEx::OnWindowPosChanged(lpwndpos);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
}
/*
void CSCShapeDlg::get_logfont(LOGFONT *lf)
{
	memset(lf, 0, sizeof(LOGFONT));
	_tcscpy_s(lf->lfFaceName, m_text_setting.font_name);

	HDC hDC = ::GetDC(m_hWnd);
	LONG size = -MulDiv(m_text_setting.font_size, GetDeviceCaps(hDC, LOGPIXELSY), 72);
	::DeleteDC(hDC);

	lf->lfHeight = size;
	lf->lfWeight = (m_text_setting.font_style & Gdiplus::FontStyleBold) ? FW_BOLD : FW_NORMAL;
	lf->lfItalic = (m_text_setting.font_style & Gdiplus::FontStyleItalic) ? true : false;
	lf->lfStrikeOut = (m_text_setting.font_style & Gdiplus::FontStyleStrikeout) ? true : false;
}
*/

//커서가 위치한 음절을 parent에게 메시지로 전달할 수 있다.
//팟플레이어에서 자막의 어떤 단어에 마우스를 올리면 포커싱되고 클릭하면 웹검색이 열리는 기능처럼 활용할 수 있다.
void CSCShapeDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (!m_send_hover_info)
		return;

	int i, j;
	bool found = false;

	for (i = 0; i < m_para.size(); i++)
	{
		for (j = 0; j < m_para[i].size(); j++)
		{
			if (m_para[i][j].r.PtInRect(point))
			{
				TRACE(_T("i = %d, j = %d, text = %s\n"), i, j, m_para[i][j].text);
				found = true;
				break;
			}
		}
		
		if (found)
			break;
	}

	CDialogEx::OnMouseMove(nFlags, point);
}
