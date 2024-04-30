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
	memset(&m_lf, 0, sizeof(LOGFONT));
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
	//id를 1234로 주면 왜 생성이 안될까...
	//className과 windowName을 CSCMenu 등 다른걸로 주면 생성이 실패하는 이유는??
	bool res = CreateEx(WS_EX_CLIENTEDGE, _T("listbox"), _T("listbox"), dwStyle, CRect(left, top, right, bottom), parent, 0);

	TRACE(_T("create. rect = (%d,%d) (%d,%d)\n"), left, top, right, bottom);
	CRect rc;
	GetClientRect(rc);
	TRACE(_T("rc = %s\n"), get_rect_info_string(rc));

	dwStyle = GetWindowLongPtr(m_hWnd, GWL_STYLE);
	dwStyle &= ~(WS_CAPTION);
	SetWindowLongPtr(m_hWnd, GWL_STYLE, dwStyle);

	dwStyle = GetWindowLong(m_hWnd, GWL_EXSTYLE) | WS_EX_LAYERED;
	SetWindowLongPtr(m_hWnd, GWL_EXSTYLE, dwStyle);

	GetClientRect(rc);
	TRACE(_T("rc = %s\n"), get_rect_info_string(rc));

	return res;
}

bool CSCShapeDlg::set_text(CShapeDlgTextSetting* setting)
{
	if (setting != NULL)
		memcpy(&m_text_setting, setting, sizeof(CShapeDlgTextSetting));

	return set_text(m_parent,
					m_text_setting.text,
					m_text_setting.font_size,
					m_text_setting.font_bold,
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
						bool font_bold,
						int shadow_depth/* = 2*/,
						float thickness/* = 2*/,
						CString font_name/* = _T("")*/,
						Gdiplus::Color cr_text/* = Gdiplus::Color::Black*/,
						Gdiplus::Color cr_stroke/* = Gdiplus::Color::DarkGray*/,
						Gdiplus::Color cr_shadow/* = Gdiplus::Color::HotPink*/,
						Gdiplus::Color cr_back/* = Gdiplus::Color::Transparent*/)
{
	bool res = false;

	if (parent == NULL)
		m_parent = parent = AfxGetMainWnd();
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

	m_text_setting = CShapeDlgTextSetting(text, font_size, font_bold, shadow_depth, thickness, font_name, cr_text, cr_stroke, cr_shadow, cr_back);

	//텍스트 출력 크기를 얻어오고
	CRect r;
	
	r = draw_text(NULL, CRect(), text,
		font_size, font_bold, shadow_depth, thickness, font_name,
		cr_text, cr_stroke, cr_shadow);

	CGdiplusBitmap	img(r.Width(), r.Height(), PixelFormat32bppARGB, cr_back);

	TRACE(_T("img rect = (%d,%d)"), img.width, img.height);

	//해당 캔버스에
	Gdiplus::Graphics g(img);

	//글자를 출력하고
	r = draw_text(&g, r, text,
		font_size, font_bold, shadow_depth, thickness, font_name,
		cr_text, cr_stroke, cr_shadow);

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

	set_image(&img);

	return res;
}

void CSCShapeDlg::set_image(CGdiplusBitmap* img, bool deep_copy)
{
	if (deep_copy)
		img->deep_copy(&m_img);

	SetWindowPos(NULL, 0, 0, m_img.width, m_img.height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

	gif_play();
}

bool CSCShapeDlg::load(UINT id)
{
	m_img.load(id);
	set_image(&m_img, false);
	return false;
}

bool CSCShapeDlg::load(CString sType, UINT id)
{
	m_img.load(sType, id);
	set_image(&m_img, false);
	return false;
}

bool CSCShapeDlg::load(CString sFile)
{
	m_img.load(sFile);
	set_image(&m_img, false);
	return false;
}

//alpha = 0 ~ 255
void CSCShapeDlg::alpha(int alpha)
{
	m_alpha = alpha;

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
		Graphics g(hdcMemory);

		g.SetPageScale(1.0);
		g.SetPageUnit(UnitPixel);
		g.SetSmoothingMode(SmoothingModeAntiAlias);

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
	::DeleteDC(hDC);
}

//animated gif인 경우
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

	m_gif_state = state_play;

	//단일 이미지인지 gif인지에 따라 별도 처리
	if (m_img.is_animated_gif())
	{
		m_gif_state = state_play;
		std::thread t(&CSCShapeDlg::gif_thread, this);
		t.detach();
	}
	else
	{
		render(m_img.m_pBitmap);
	}
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
	GUID   pageGuid = FrameDimensionTime;
	m_gif_index = 0;

	while (m_gif_state != state_stop)
	{
		if (m_gif_state == state_pause)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
			continue;
		}

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

	if (fadein)
		fade_in();

	Wait(timeout * 1000);

	if (fadeout)
		fade_out();
}

//set_image(), set_text()를 호출해도 아직 hide상태다.
//ShowWindow()시키거나 fadein()으로 보여지게 한다.
void CSCShapeDlg::fade_in()
{
	std::thread t(&CSCShapeDlg::thread_fadeinout, this, true);
	t.detach();
}

void CSCShapeDlg::fade_out()
{
	std::thread t(&CSCShapeDlg::thread_fadeinout, this, false);
	t.detach();
}

void CSCShapeDlg::thread_fadeinout(bool fadein)
{
	alpha(fadein ? 0 : 255);
	ShowWindow(SW_SHOW);

	int _alpha = m_alpha;

	while (true)
	{
		_alpha += (fadein ? 5 : -5);

		if (_alpha < 0 || _alpha > 255)
			break;
		
		alpha(_alpha);
	}
}
