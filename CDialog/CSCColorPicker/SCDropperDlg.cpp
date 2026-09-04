// SCDropperDlg.cpp
#include "SCDropperDlg.h"
#include "../../Functions.h"
#include "../../cursor_helpers.h"
#include "../../data_structure/SCParagraph/SCParagraph.h"

IMPLEMENT_DYNAMIC(CSCDropperDlg, CDialog)

BEGIN_MESSAGE_MAP(CSCDropperDlg, CDialog)
	ON_WM_TIMER()
	ON_WM_LBUTTONDOWN()
	ON_WM_KEYDOWN()
	ON_WM_MOUSEWHEEL()
	ON_WM_MBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
END_MESSAGE_MAP()

// --- 레지스트리 키 경로 (CSCColorPicker와 동일 그룹) ---
static LPCTSTR kRegSection = _T("setting\\color picker\\dropper");

//커서 좌표 / 색상 정보 문자열
static const float	kInfoFontSize = 14.0f;
static const int	kInfoOffsetY = 32;	//커서 중심에서 아래로 이만큼 떨어진 자리가 기본 위치

//조합키 안내 — 돋보기 원 바깥(위/아래 띠)에 그린다.
//"휠 배율" 처럼 줄이면 처음 보는 사람은 무엇이 키고 무엇이 동작인지 알 수 없으므로
//"휠 : 배율 조정" 형태로 풀어 쓰고 두 줄로 나눈다. 범례이므로 두 줄 모두 왼쪽 정렬.
//키 이름은 <b> 없이 색만으로 구분한다 — GDI+ 는 Regular / Bold 두 단계뿐이라
//9pt 에서 Bold 를 얹으면 글자가 뭉개진다. 중간 굵기는 face 이름(Segoe UI Semibold 등)으로만 가능한데
//굴림에는 그런 face 가 없다.
static const int	kHintBandHeight = 24;
static const int	kHintPadX = 10;
static const int	kHintPadY = 5;
static const Gdiplus::Color kHintPlateColor = Gdiplus::Color(20, 20, 20);

static LPCTSTR		kHintText =
	_T("<cr=#FFD54F>휠</cr> : 배율 조정      ")
	_T("<cr=#FFD54F>Ctrl+휠</cr> : 창 크기 조정      ")
	_T("<cr=#FFD54F>Shift</cr> : 큰 폭으로 조정")
	_T("<br>")
	_T("<cr=#FFD54F>방향키</cr> : 1픽셀 이동      ")
	_T("<cr=#FFD54F>좌클릭</cr> : 색 선택      ")
	_T("<cr=#FFD54F>휠클릭</cr> : 정보 표시      ")
	_T("<cr=#FFD54F>ESC</cr> : 취소");

//문자열을 실제로 그려질 크기로 잰다. draw_text 는 thickness > 0 이면 emSize = dpi * font_size / 72 로
//글자 path 를 만들므로 (thickness == 0 인 DrawString 경로의 /96 과 다르다) 여기서도 /72 + UnitPixel 로 맞춘다.
static CRect measure_label(Gdiplus::Graphics& g, CString text, float font_size)
{
	Gdiplus::FontFamily family(L"Arial");
	Gdiplus::Font font(&family, g.GetDpiY() * font_size / 72.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
	Gdiplus::RectF rt = measure_string(&g, font, text);

	return CRect(0, 0, (int)rt.Width + 5, (int)rt.Height + 3);
}

//draw_text 의 그림자는 rTarget 을 (0,0) 기준 비트맵에 0.4 배로 그린 뒤 되늘리는 방식이라
//rTarget.left/top 이 0 이 아니면 그림자가 엉뚱한 곳에 놓이거나 아예 잘려나간다.
//원점에서 시작하는 rect + 우하단 정렬로 넘겨야 문자열도 그림자도 r 자리에 그려진다.
static void draw_label(Gdiplus::Graphics& g, CRect r, CString text, float font_size,
	Gdiplus::Color cr_text, Gdiplus::Color cr_shadow)
{
	draw_text(g, CRect(0, 0, r.right, r.bottom), text, font_size, Gdiplus::FontStyleBold, 2, 1.0f,
		_T("Arial"), cr_text, cr_shadow, cr_shadow, Gdiplus::Color::Transparent, DT_RIGHT | DT_BOTTOM);
}

//안내 문구 폰트. 맑은 고딕은 Vista+ 에만 있으므로 XP 에서는 굴림으로 떨어진다.
//굴림은 작은 크기에 내장 비트맵을 갖고 획이 픽셀 그리드에 맞도록 설계돼 AA 를 켜도 걸릴 곳이 거의 없다 —
//또렷하지만 딱딱하다. 맑은 고딕은 AA 를 전제로 만들어진 폰트라 같은 크기에서 훨씬 부드럽게 나온다.
//있는 환경에서까지 굴림을 쓸 이유가 없어 런타임에 고른다.
static void set_hint_font(CSCTextProperty& prop)
{
	//GDI+ 는 wide 전용이라 LPCWSTR 로 둔다. CStringW 임시 객체를 생성자에 바로 넘기면
	//컴파일러가 "CStringW candidates[i] 를 받는 함수 선언" 으로 읽어 C2131 이 난다 (most vexing parse).
	LPCWSTR candidates[] = { L"맑은 고딕", L"Malgun Gothic", L"굴림" };

	for (int i = 0; i < _countof(candidates); ++i)
	{
		Gdiplus::FontFamily family(candidates[i]);
		if (family.IsAvailable())
		{
			_tcscpy_s(prop.name, CString(candidates[i]));
			return;
		}
	}
}

//창 좌표 rect 를 화면에 실제로 보이는 영역 안으로 민다.
//오른쪽보다 왼쪽을 나중에 맞춰, 문자열이 보이는 영역보다 넓을 때 앞부분이 살아남게 한다.
static void clamp_into(CRect& r, const CRect& visible)
{
	if (r.right > visible.right)
		r.OffsetRect(visible.right - r.right, 0);
	if (r.left < visible.left)
		r.OffsetRect(visible.left - r.left, 0);
	if (r.bottom > visible.bottom)
		r.OffsetRect(0, visible.bottom - r.bottom);
	if (r.top < visible.top)
		r.OffsetRect(0, visible.top - r.top);
}

CSCDropperDlg::CSCDropperDlg(CWnd* pParent)
{
}

CSCDropperDlg::~CSCDropperDlg()
{
	release_screen();

	delete m_hint_bitmap;
	m_hint_bitmap = nullptr;
}

void CSCDropperDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

void CSCDropperDlg::PostNcDestroy()
{
	CDialog::PostNcDestroy();
}

// --- 설정 저장/복원 ---
void CSCDropperDlg::load_settings()
{
	CWinApp* app = AfxGetApp();
	if (!app) return;

	int val = app->GetProfileInt(kRegSection, _T("wnd_size"), kWndSizeDefault);
	m_wnd_size = max(kWndSizeMin, min(kWndSizeMax, val));

	val = app->GetProfileInt(kRegSection, _T("sample"), kSampleDefault);
	if (val % 2 == 0) val++;
	const int sample_max = (m_wnd_size % 2 == 0) ? m_wnd_size - 1 : m_wnd_size;
	m_sample = max(kSampleMin, min(sample_max, val));
}

void CSCDropperDlg::save_settings()
{
	CWinApp* app = AfxGetApp();
	if (!app) return;

	app->WriteProfileInt(kRegSection, _T("sample"), m_sample);
	app->WriteProfileInt(kRegSection, _T("wnd_size"), m_wnd_size);
}

void CSCDropperDlg::build_hint_bitmap()
{
	if (m_hint_bitmap)
		return;

	CSCTextProperty prop;
	set_hint_font(prop);
	prop.size = 9.0f;
	prop.cr_text = Gdiplus::Color(235, 235, 235);

	std::deque<std::deque<CSCParagraph>> para;
	CString text = kHintText;
	CSCParagraph::build_paragraph_str(text, para, &prop);

	CClientDC dc(this);
	CSCParagraph::calc_text_rect(CRect(0, 0, 0, 0), &dc, para, DT_NOCLIP);

	//calc_text_rect 는 라인 박스를 여백 없이 붙여 쌓는다 (CSS 의 line-height: normal).
	//1.2 = 그 위에 박스 높이의 20% 를 여백으로 더한 것 — CSS line-height: 1.2 와 같다.
	CSCParagraph::set_line_spacing(para, 1.2f);

	CRect bounds = CSCParagraph::get_bounding_rect(para);

	for (auto& line : para)
	{
		for (auto& run : line)
			run.r.OffsetRect(-bounds.left + kHintPadX, -bounds.top + kHintPadY);
	}

	const int bw = bounds.Width() + kHintPadX * 2;
	const int bh = bounds.Height() + kHintPadY * 2;

	//ClearType(서브픽셀 AA)은 작은 글씨를 또렷하게 만드는 유일한 수단인데, GDI+ 는 알파 채널이 있는
	//대상에는 걸어주지 않는다 (레이어드 윈도우에 합성하려면 최종 비트맵은 알파가 있어야 한다).
	//그래서 글자는 불투명 비트맵에 ClearType 으로 그리고, 라운드 모서리 알파만 따로 만들어 합성한다.
	//이 때문에 띠 자체는 완전 불투명이 된다 — 반투명으로 두면 ClearType 이 계산해 둔 색 프린지가
	//바탕과 어긋나 오히려 지저분해진다.
	Gdiplus::Bitmap text_bmp(bw, bh, PixelFormat32bppRGB);
	{
		Gdiplus::Graphics gt(&text_bmp);
		gt.Clear(kHintPlateColor);
		gt.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
		gt.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

		CSCParagraph::draw_text(gt, para, 0, true);
	}

	m_hint_bitmap = new Gdiplus::Bitmap(bw, bh, PixelFormat32bppPARGB);
	{
		//흰색으로 채운 라운드 사각형 = 알파 마스크. 모서리는 AA 로 부드럽게 떨어진다.
		Gdiplus::Graphics gm(m_hint_bitmap);
		gm.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

		draw_round_rect(&gm, Gdiplus::Rect(0, 0, bw, bh),
			Gdiplus::Color::Transparent, Gdiplus::Color::White, 6);
	}

	//마스크의 알파를 그대로 두고, 색만 ClearType 렌더 결과로 갈아끼운다 (PARGB 라 알파를 곱해 넣는다).
	Gdiplus::Rect rc_lock(0, 0, bw, bh);
	Gdiplus::BitmapData bd_text = {};
	Gdiplus::BitmapData bd_out = {};

	text_bmp.LockBits(&rc_lock, Gdiplus::ImageLockModeRead, PixelFormat32bppRGB, &bd_text);
	m_hint_bitmap->LockBits(&rc_lock, Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeWrite, PixelFormat32bppPARGB, &bd_out);

	for (int y = 0; y < bh; ++y)
	{
		const BYTE* src = (const BYTE*)bd_text.Scan0 + y * bd_text.Stride;
		BYTE* dst = (BYTE*)bd_out.Scan0 + y * bd_out.Stride;

		for (int x = 0; x < bw * 4; x += 4)
		{
			const int a = dst[x + 3];
			dst[x + 0] = (BYTE)(src[x + 0] * a / 255);
			dst[x + 1] = (BYTE)(src[x + 1] * a / 255);
			dst[x + 2] = (BYTE)(src[x + 2] * a / 255);
		}
	}

	m_hint_bitmap->UnlockBits(&bd_out);
	text_bmp.UnlockBits(&bd_text);
}

// --- 전체 가상 데스크톱 1회 캡처 ---
void CSCDropperDlg::capture_screen()
{
	const int vx = ::GetSystemMetrics(SM_XVIRTUALSCREEN);
	const int vy = ::GetSystemMetrics(SM_YVIRTUALSCREEN);
	const int vw = ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
	const int vh = ::GetSystemMetrics(SM_CYVIRTUALSCREEN);

	m_screen_origin = CPoint(vx, vy);

	HDC hDesktopDC = ::GetDC(nullptr);
	m_hScreenDC = ::CreateCompatibleDC(hDesktopDC);
	m_hScreenBmp = ::CreateCompatibleBitmap(hDesktopDC, vw, vh);
	m_hOldScreenBmp = (HBITMAP)::SelectObject(m_hScreenDC, m_hScreenBmp);
	::BitBlt(m_hScreenDC, 0, 0, vw, vh, hDesktopDC, vx, vy, SRCCOPY);
	::ReleaseDC(nullptr, hDesktopDC);
}

void CSCDropperDlg::release_screen()
{
	if (m_hScreenDC)
	{
		::SelectObject(m_hScreenDC, m_hOldScreenBmp);
		::DeleteDC(m_hScreenDC);
		m_hScreenDC = nullptr;
	}
	if (m_hScreenBmp)
	{
		::DeleteObject(m_hScreenBmp);
		m_hScreenBmp = nullptr;
	}
}

// --- 생성 ---
bool CSCDropperDlg::create(CWnd* parent)
{
	load_settings();
	capture_screen();

	bool res = CreateEx(
		WS_EX_TOPMOST | WS_EX_LAYERED,
		AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW),
		nullptr,
		WS_POPUP | WS_VISIBLE,
		CRect(0, 0, m_wnd_size, m_wnd_size),
		parent, 0);

	if (!res)
	{
		release_screen();
		return false;
	}

	//20260904 by claude. 전역 단축키로 띄우면 이 프로세스가 포그라운드가 아니라 창을 만들어도 키 입력이
	//직전 앱으로 간다 — ESC 취소 / 방향키 이동 / 휠 조작이 모두 먹지 않는다.
	//::SetForegroundWindow 는 그 상태에서 조용히 실패하므로 AttachThreadInput 을 쓰는 Common 함수로 활성화한다.
	SetForegroundWindowForce(m_hWnd);

	SetTimer(kTimerID, 16, nullptr);
	SetCursor(get_thin_cross_cursor());
	return true;
}

// --- 타이머 -> 표시 갱신 ---
void CSCDropperDlg::OnTimer(UINT_PTR /*nIDEvent*/)
{
	update_display();
}

void CSCDropperDlg::update_display()
{
	if (!m_hScreenDC)
		return;

	CPoint cursor;
	GetCursorPos(&cursor);

	//20260904 by claude. 모니터가 바뀔 때만 다시 찾는다 — get_monitor_index / get_monitor_rect 는
	//호출할 때마다 EnumDisplayMonitors 를 돌리므로 16ms 타이머에서 매번 부르면 낭비다.
	if (!m_monitor_rect.PtInRect(cursor))
		m_monitor_rect = get_monitor_rect(get_monitor_index(cursor.x, cursor.y));

	const int half = m_sample / 2;
	const int srcX = cursor.x - m_screen_origin.x - half;
	const int srcY = cursor.y - m_screen_origin.y - half;

	BITMAPINFOHEADER bih = { sizeof(bih), m_sample, -m_sample, 1, 32, BI_RGB };
	BYTE* pCapBits = nullptr;
	HBITMAP hCapDib = ::CreateDIBSection(nullptr, (BITMAPINFO*)&bih, DIB_RGB_COLORS, (void**)&pCapBits, nullptr, 0);
	HDC hCapDC = ::CreateCompatibleDC(nullptr);
	HBITMAP hOldCap = (HBITMAP)::SelectObject(hCapDC, hCapDib);

	::BitBlt(hCapDC, 0, 0, m_sample, m_sample, m_hScreenDC, srcX, srcY, SRCCOPY);

	m_center_color = ::GetPixel(m_hScreenDC, cursor.x - m_screen_origin.x, cursor.y - m_screen_origin.y);

	::SelectObject(hCapDC, hOldCap);
	::DeleteDC(hCapDC);

	for (int i = 0, n = m_sample * m_sample; i < n; ++i)
		pCapBits[i * 4 + 3] = 0xFF;

	const int ws = m_wnd_size;

	Gdiplus::Bitmap content(ws, ws, PixelFormat32bppPARGB);
	{
		Gdiplus::Bitmap capBmp(m_sample, m_sample, m_sample * 4, PixelFormat32bppPARGB, pCapBits);

		Gdiplus::Graphics cg(&content);
		cg.SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor);
		cg.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
		cg.DrawImage(&capBmp, 0, 0, ws, ws);
	}
	::DeleteObject(hCapDib);

	{
		Gdiplus::Graphics cg(&content);
		cg.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

		const float cs = (float)ws / m_sample;

		if (cs > 4.0f)
		{
			Gdiplus::Pen gridPen(Gdiplus::Color(80, 0, 0, 0), 1.f);
			for (int i = 0; i <= m_sample; ++i)
			{
				float p = i * cs;
				cg.DrawLine(&gridPen, p, 0.f, p, (float)ws);
				cg.DrawLine(&gridPen, 0.f, p, (float)ws, p);
			}
		}

		const float cx = half * cs;
		const float cy = half * cs;
		Gdiplus::Pen centerPen(Gdiplus::Color(220, 255, 255, 255), 1.5f);
		cg.DrawRectangle(&centerPen, cx, cy, cs, cs);
	}

	//20260904 by claude. 캔버스는 돋보기 원보다 크다 — 원 위아래로 조합키 안내 띠를 둘 자리를 둔다.
	//원은 항상 커서 중심에 오고, 안내는 화면에 남는 쪽(아래가 기본, 모니터 하단이면 위) 띠에 그린다.
	//창을 넓혀도 클릭은 위치와 무관하게 중앙 픽셀을 선택하므로 동작은 그대로다.
	int cw = ws;
	int band = 0;
	CRect rhint(0, 0, 0, 0);
	CString rgb;

	if (m_show_info)
	{
		rgb.Format(_T("%dx%d (%s)"), cursor.x - m_screen_origin.x, cursor.y - m_screen_origin.y, get_color_str(m_center_color));

		build_hint_bitmap();
		rhint.SetRect(0, 0, (int)m_hint_bitmap->GetWidth(), (int)m_hint_bitmap->GetHeight());

		band = max(kHintBandHeight, rhint.Height() + 4);
		if (cw < rhint.Width())
			cw = rhint.Width();
	}

	const int ch = ws + band * 2;
	const int ox = (cw - ws) / 2;	//캔버스 안에서 돋보기 원의 좌상단
	const int oy = band;

	Gdiplus::Bitmap canvas(cw, ch, PixelFormat32bppPARGB);
	{
		Gdiplus::Graphics g(&canvas);
		g.SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);
		//g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
		//g.SetTextRenderingHint(Gdiplus::TextRenderingHint::TextRenderingHintAntiAliasGridFit);

		//TextureBrush 는 캔버스 원점을 기준으로 타일링하므로, 원을 옮겨 그리면 브러시도 같이 옮겨야 한다.
		Gdiplus::TextureBrush tb(&content);
		tb.TranslateTransform((float)ox, (float)oy);
		g.FillEllipse(&tb,
			Gdiplus::RectF((float)ox, (float)oy, (float)ws, (float)ws));

		Gdiplus::Pen borderPen(Gdiplus::Color(200, 60, 60, 60), 2.5f);
		g.DrawEllipse(&borderPen,
			ox + 1.5f, oy + 1.5f,
			(float)(ws - 3), (float)(ws - 3));

		if (m_show_info)
		{
			//창은 커서를 중심으로 뜨므로 커서가 모니터 가장자리에 있으면 창의 일부가 화면 밖이다.
			//창 ∩ 모니터 = 실제로 보이는 영역이고, 문자열을 그 안으로 옮겨야 읽을 수 있다.
			const CPoint org(cursor.x - cw / 2, cursor.y - oy - ws / 2);
			CRect rc_visible(org.x, org.y, org.x + cw, org.y + ch);
			rc_visible.IntersectRect(rc_visible, m_monitor_rect);
			rc_visible.OffsetRect(-org.x, -org.y);

			CRect rinfo = measure_label(g, rgb, kInfoFontSize);
			rinfo.OffsetRect(ox + (ws - rinfo.Width()) / 2, oy + ws / 2 + kInfoOffsetY - rinfo.Height() / 2);

			//커서 아래가 잘리면 위로 뒤집는다. 남는 어긋남과 좌우는 밀어서 맞춘다.
			if (rinfo.bottom > rc_visible.bottom)
				rinfo.OffsetRect(0, -2 * kInfoOffsetY);
			clamp_into(rinfo, rc_visible);

			Gdiplus::Color cr_text;
			cr_text.SetFromCOLORREF(m_center_color);
			draw_label(g, rinfo, rgb, kInfoFontSize, cr_text, get_distinct_bw_color(cr_text));

			//안내 띠. 원 아래가 기본, 화면 밖이면 원 위로.
			rhint.OffsetRect((cw - rhint.Width()) / 2, oy + ws + (band - rhint.Height()) / 2);
			if (rhint.bottom > rc_visible.bottom)
				rhint.OffsetRect(0, -(ws + band));
			clamp_into(rhint, rc_visible);

			g.DrawImage(m_hint_bitmap, rhint.left, rhint.top);
		}
	}

	HBITMAP hBmp = nullptr;
	canvas.GetHBITMAP(Gdiplus::Color(0, 0, 0, 0), &hBmp);

	HDC hDC = ::GetDC(nullptr);
	HDC hMemDC = ::CreateCompatibleDC(hDC);
	HBITMAP hOld = (HBITMAP)::SelectObject(hMemDC, hBmp);

	BLENDFUNCTION blend = {};
	blend.BlendOp = AC_SRC_OVER;
	blend.SourceConstantAlpha = 255;
	blend.AlphaFormat = AC_SRC_ALPHA;

	SIZE  sz = { cw, ch };
	POINT ptSrc = { 0, 0 };
	POINT ptDst = { cursor.x - cw / 2, cursor.y - oy - ws / 2 };

	::UpdateLayeredWindow(m_hWnd, hDC, &ptDst, &sz,
		hMemDC, &ptSrc, 0, &blend, ULW_ALPHA);

	::SelectObject(hMemDC, hOld);
	::DeleteDC(hMemDC);
	::ReleaseDC(nullptr, hDC);
	::DeleteObject(hBmp);
}

// --- 입력 처리 ---
void CSCDropperDlg::OnLButtonDown(UINT /*nFlags*/, CPoint /*point*/)
{
	m_picked_color = Gdiplus::Color(255,
		GetRValue(m_center_color),
		GetGValue(m_center_color),
		GetBValue(m_center_color));
	m_picked = true;

	copy_to_clipboard(m_hWnd, get_color_str(m_center_color));

	save_settings();
	KillTimer(kTimerID);
	DestroyWindow();
}

void CSCDropperDlg::OnKeyDown(UINT nChar, UINT /*nRepCnt*/, UINT /*nFlags*/)
{
	if (nChar == VK_ESCAPE)
	{
		save_settings();
		KillTimer(kTimerID);
		DestroyWindow();
		return;
	}

	if (nChar == VK_LEFT || nChar == VK_RIGHT || nChar == VK_UP || nChar == VK_DOWN)
	{
		const int step = (IsShiftPressed() ? 8 : 1);
		CPoint pt;
		GetCursorPos(&pt);
		switch (nChar)
		{
			case VK_LEFT:
				pt.x -= step;
				break;
			case VK_RIGHT:
				pt.x += step;
				break;
			case VK_UP:
				pt.y -= step;
				break;
			case VK_DOWN:
				pt.y += step;
				break;
		}

		const int vx = ::GetSystemMetrics(SM_XVIRTUALSCREEN);
		const int vy = ::GetSystemMetrics(SM_YVIRTUALSCREEN);
		const int vw = ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
		const int vh = ::GetSystemMetrics(SM_CYVIRTUALSCREEN);
		pt.x = max(vx, min(vx + vw - 1, pt.x));
		pt.y = max(vy, min(vy + vh - 1, pt.y));

		SetCursorPos(pt.x, pt.y);
		update_display();
	}
}

BOOL CSCDropperDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint /*pt*/)
{
	if (nFlags & MK_CONTROL)
	{
		if (zDelta > 0)
			m_wnd_size = min(kWndSizeMax, m_wnd_size + kWndSizeStep);
		else
			m_wnd_size = max(kWndSizeMin, m_wnd_size - kWndSizeStep);
	}
	else
	{
		const int step = (IsShiftPressed() ? 10 : 2);
		if (zDelta > 0)
			m_sample = m_sample - step;
		else
			m_sample = m_sample + step;
	}

	// 홀수 보정 + 동적 범위 클램핑
	if (m_sample % 2 == 0)
		m_sample--;
	const int sample_max = (m_wnd_size % 2 == 0) ? m_wnd_size - 1 : m_wnd_size;
	m_sample = max(kSampleMin, min(sample_max, m_sample));

	update_display();
	return TRUE;
}
void CSCDropperDlg::OnMButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	m_show_info = !m_show_info;
	update_display();

	CDialog::OnMButtonUp(nFlags, point);
}

void CSCDropperDlg::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	CDialog::OnRButtonDown(nFlags, point);
}

BOOL CSCDropperDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

	return CDialog::PreTranslateMessage(pMsg);
}

void CSCDropperDlg::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	m_picked = false;

	save_settings();
	KillTimer(kTimerID);
	DestroyWindow();

	CDialog::OnRButtonUp(nFlags, point);
}
