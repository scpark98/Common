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
		return false;
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

bool CSCShapeDlg::set_text(CString str)
{
	m_text_setting.text = str;
	return set_text();
}

bool CSCShapeDlg::set_text(CSCShapeDlgTextSetting* setting)
{
	if (setting != NULL)
		memcpy(&m_text_setting, setting, sizeof(CSCShapeDlgTextSetting));

	return set_text(m_parent,
					m_text_setting.text,
					m_text_setting.font_size,
					m_text_setting.font_style,
					m_text_setting.shadow_depth,
					m_text_setting.thickness,
					m_text_setting.font_name,
					m_text_setting.cr_text,
					m_text_setting.cr_stroke,
					m_text_setting.cr_shadow,
					m_text_setting.cr_back
	);
}

//gdiplus를 이용한 text 출력용 dlg 생성
bool CSCShapeDlg::set_text(CWnd* parent,
						CString text,
						float font_size,
						int font_style,
						int shadow_depth/* = 2*/,
						float thickness/* = 2*/,
						CString font_name/* = _T("")*/,
						Gdiplus::Color cr_text/* = Gdiplus::Color::Black*/,
						Gdiplus::Color cr_stroke/* = Gdiplus::Color::DarkGray*/,
						Gdiplus::Color cr_shadow/* = Gdiplus::Color::HotPink*/,
						Gdiplus::Color cr_back/* = Gdiplus::Color(1, 0, 0, 0)*/)
{
	bool res = false;

	if (parent == NULL)
		m_parent = parent = AfxGetApp()->GetMainWnd();
	else
		m_parent = parent;

	if (font_name.IsEmpty())
	{
		CFont* font = m_parent->GetFont();
		LOGFONT lf;

		memset(&lf, 0, sizeof(LOGFONT));

		if (font != NULL)
			font->GetObject(sizeof(lf), &lf);
		else
			GetObject(GetStockObject(SYSTEM_FONT), sizeof(lf), &lf);

		m_text_setting.font_name = font_name = lf.lfFaceName;
	}

	m_text_setting = CSCShapeDlgTextSetting(text, font_size, font_style, shadow_depth, thickness, font_name, cr_text, cr_stroke, cr_shadow, cr_back);

	//텍스트 출력 크기를 얻어오고
	CRect r;
	
	r = draw_text(NULL, CRect(), text,
		font_size, font_style, shadow_depth, thickness, font_name,
		cr_text, cr_stroke, cr_shadow, DT_LEFT | DT_TOP);

	CGdiplusBitmap	img(r.Width(), r.Height(), PixelFormat32bppARGB, cr_back);

	//TRACE(_T("img rect = (%d,%d)\n"), img.width, img.height);

	//해당 캔버스에
	Gdiplus::Graphics g(img.m_pBitmap);

	//글자를 출력하고
	r = draw_text(&g, r, text,
		font_size, font_style, shadow_depth, thickness, font_name,
		cr_text, cr_stroke, cr_shadow, DT_LEFT | DT_TOP);

	if (m_hWnd)
	{
		res = true;
		//SetWindowPos(NULL, 0, 0, r.Width(), r.Height(), SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	}
	else
	{
		res = create(parent, 0, 0, r.Width(), r.Height());
	}

#ifdef _DEBUG
	img.save(_T("d:\\SCShapeDlg.png"));
#endif

	if (!res)
		return res;

	set_image(parent, &img);

	return res;
}

void CSCShapeDlg::set_image(CWnd* parent, CGdiplusBitmap* img, bool deep_copy)
{
	//현재 이미지가 animateGif이고 play중이라면 일단 멈춘다.
	if (m_img.is_animated_gif() && m_gif_state == state_play)
	{
		TRACE(_T("stop current gif...\n"));
		m_gif_state = state_stop;
		Wait(1000);
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

//alpha = 0 ~ 255
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
		TRACE(_T("image width or height is invalid.\n"));
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
	ASSERT(hbmpMem != NULL);
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
	GUID   pageGuid = Gdiplus::FrameDimensionTime;
	m_gif_index = 0;

	while (m_gif_state != state_stop)
	{
		TRACE(_T("gif_thread\n"));

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
		TRACE(_T("%3d = %ld ms\n"), m_gif_index, delay);
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
void CSCShapeDlg::fade_in(int delay_ms, int hide_after_ms, bool fadeout)
{
	std::thread t(&CSCShapeDlg::thread_fadeinout, this, true, delay_ms, hide_after_ms, fadeout);
	t.detach();
}

void CSCShapeDlg::fade_out()
{
	std::thread t(&CSCShapeDlg::thread_fadeinout, this, false, 0, 0, false);
	t.detach();
}

void CSCShapeDlg::thread_fadeinout(bool fadein, int delay_ms, int hide_after_ms, bool fadeout)
{
	set_alpha(fadein ? 0 : 255);
	ShowWindow(SW_SHOW);

	int _alpha = m_alpha;

	if (delay_ms <= 0)
		delay_ms = 50;

	while (true)
	{
		_alpha += (fadein ? 5 : -5);

		if (_alpha < 0 || _alpha > 255)
			break;
		
		set_alpha(_alpha);

		std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
	}

	if (hide_after_ms > 0)
		std::this_thread::sleep_for(std::chrono::milliseconds(hide_after_ms));
	else
		return;

	if (fadeout)
	{
		set_alpha(255);
		int _alpha = m_alpha;

		if (delay_ms <= 0)
			delay_ms = 50;

		while (true)
		{
			_alpha -= 5;

			if (_alpha < 0 || _alpha > 255)
				break;

			set_alpha(_alpha);

			std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
		}

		ShowWindow(SW_HIDE);
		set_alpha(255);
	}
	else
	{
		ShowWindow(SW_HIDE);
	}
}
#endif

void CSCShapeDlg::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CDialogEx::OnWindowPosChanged(lpwndpos);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
}

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
