// SCImage2dDlg.cpp: 구현 파일
//

//#include "afxdialogex.h"
#include "SCImage2dDlg.h"

#include <thread>
#include "../../Functions.h"
#include "../../MemoryDC.h"
#include "../../AutoFont.h"

// CSCImage2dDlg 대화 상자

IMPLEMENT_DYNAMIC(CSCImage2dDlg, CDialog)

CSCImage2dDlg::CSCImage2dDlg(CWnd* parent)
{
	m_img.resize(m_buffer_max);
}

CSCImage2dDlg::~CSCImage2dDlg()
{
	stop();
}

bool CSCImage2dDlg::create(CWnd* parent, int x, int y, int cx, int cy)
{
	m_parent = parent;

	LONG_PTR dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN;

	WNDCLASS wc = {};
	::GetClassInfo(AfxGetInstanceHandle(), _T("#32770"), &wc);
	wc.lpszClassName = _T("CSCImage2dDlg");
	AfxRegisterClass(&wc);

	bool res = CreateEx(NULL, wc.lpszClassName, _T("CSCImage2dDlg"), (DWORD)dwStyle, CRect(x, y, x + cx, y + cy), parent, 0);
	if (!res)
		return false;

	dwStyle = GetWindowLongPtr(m_hWnd, GWL_STYLE);
	dwStyle &= ~(WS_CAPTION);
	SetWindowLongPtr(m_hWnd, GWL_STYLE, dwStyle);

	CRect r_pixel = make_rect(x + 8, y + cy - 8 - PIXEL_INFO_CY, PIXEL_INFO_CX, PIXEL_INFO_CY);
	m_static_pixel.create(_T("A 0\nR 0\nG 0\nB 0"), WS_CHILD | SS_CENTER | SS_CENTERIMAGE, r_pixel, this);
	m_static_pixel.sunken();
	m_static_pixel.set_font_name(_T("Consolas"));
	m_static_pixel.set_font_size(10);

	res = m_slider_gif.Create(WS_CHILD, make_rect(x + 8, y + cy - 8 - GIF_SLIDER_HEIGHT, GIF_SLIDER_WIDTH, GIF_SLIDER_HEIGHT), this, 0);
	m_slider_gif.set_style(CSCSliderCtrl::style_progress);
	m_slider_gif.set_track_height(GIF_SLIDER_HEIGHT);
	m_slider_gif.set_font_name(_T("Arial"));
	m_slider_gif.set_font_size(8);
	m_slider_gif.set_inactive_color(gGRAY(192));
	m_slider_gif.draw_progress_border();
	m_slider_gif.set_progress_border_color(Gdiplus::Color::DimGray);
	m_slider_gif.set_use_slide();

	//m_br_zigzag = CSCGdiplusBitmap::get_zigzag_pattern(32, m_cr_zigzag_back, m_cr_zigzag_fore);

	m_thumb.create(this);
	m_thumb.set_color_theme(CSCColorTheme::color_theme_dark_gray);
	m_thumb.ShowWindow(m_show_thumb ? SW_SHOW : SW_HIDE);


	m_d2dc.init(m_hWnd);

	HRESULT hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&m_WriteFactory));
	if (SUCCEEDED(hr))
	{
		m_WriteFactory->CreateTextFormat(_T("맑은 고딕"), nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 12.0f, _T("ko-kr"), &m_WriteFormat);
		m_WriteFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		m_WriteFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
	}

	m_image_roi = get_profile_value(_T("setting\\CSCImage2dDlg"), _T("image roi"), Gdiplus::RectF());

	m_d2dc.get_d2dc()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Ivory), &m_brush);
	m_d2dc.get_d2dc()->CreateSolidColorBrush(D2D1::ColorF(1.f, 1.f, 1.f, 0.2f), &m_brush_pixel_guide);

	set_show_pixel(AfxGetApp()->GetProfileInt(_T("setting\\CSCImage2dDlg"), _T("show pixel"), false));
	set_show_pixel_pos(AfxGetApp()->GetProfileInt(_T("setting\\CSCImage2dDlg"), _T("show pixel_pos"), true));
	set_show_info(AfxGetApp()->GetProfileInt(_T("setting\\CSCImage2dDlg"), _T("show info"), false));
	m_fit2ctrl = AfxGetApp()->GetProfileInt(_T("setting\\CSCImage2dDlg"), _T("fit to ctrl"), true);
	m_show_cursor_guide_line = AfxGetApp()->GetProfileInt(_T("setting\\CSCImage2dDlg"), _T("show cursor guide line"), false);

	m_interpolation_mode = (D2D1_BITMAP_INTERPOLATION_MODE)AfxGetApp()->GetProfileInt(_T("setting\\CSCImage2dDlg"), _T("interpolation mode"), D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);

	if (!m_fit2ctrl)
		zoom(get_profile_value(_T("setting\\CSCImage2dDlg"), _T("zoom ratio"), 1.0));


	return true;
}

void CSCImage2dDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSCImage2dDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	//ON_WM_MOUSEWHEEL()
	ON_WM_MOUSEHOVER()
	ON_WM_MOUSELEAVE()
	ON_WM_SETCURSOR()
	ON_WM_WINDOWPOSCHANGED()
	ON_REGISTERED_MESSAGE(Message_CSCD2Image, &CSCImage2dDlg::on_message_from_CSCD2Image)
	ON_REGISTERED_MESSAGE(Message_CSCSliderCtrl, &CSCImage2dDlg::on_message_from_CSCSliderCtrl)
	ON_REGISTERED_MESSAGE(Message_CSCThumbCtrl, &CSCImage2dDlg::on_message_CSCThumbCtrl)
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CSCImage2dDlg 메시지 처리기

void CSCImage2dDlg::OnPaint()
{
	//if (m_skip_repaint)
	//{
	//	CDialog::OnPaint();
	//	return;
	//}

	//TRACE(_T("CSCImage2dDlg::OnPaint. %ld\n"), GetTickCount64());

	CPaintDC dc(this); // device context for painting
	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	// 그리기 메시지에 대해서는 CStatic::OnPaint()을(를) 호출하지 마십시오.
	CRect rc;
	CString str;

	GetClientRect(rc);

	ID2D1DeviceContext* d2dc = m_d2dc.get_d2dc();

	D2D1_SIZE_F sz_dc = m_d2dc.get_size();

	d2dc->BeginDraw();
	d2dc->SetTransform(D2D1::Matrix3x2F::Identity());

	//black으로 칠한 후
	d2dc->Clear(D2D1::ColorF(0.125f, 0.125f, 0.125f));


	if (m_img.size() == 0 || m_img[0].is_empty())
	{
		dc.SelectClipRgn(NULL);

		CString msg;

		m_WriteFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
		m_WriteFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

		if (m_img[0].get_filename().IsEmpty())
		{
			msg = _T("Drag&Drop으로 이미지 파일을 열 수 있습니다.");
		}
		else if (!PathFileExists(m_img[0].get_filename()))
		{
			msg.Format(_T("%s\n\n위 파일이 존재하지 않습니다."), m_img[0].get_filename());
		}
		else
		{
			msg.Format(_T("%s\n파일이 손상되었거나 지원하지 않는 형식입니다."), m_img[0].get_filename());
		}
		//dc.DrawText(msg, rText, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_CALCRECT);
		//DrawShadowText(dc.GetSafeHdc(), msg, -1, rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE, indianred, black, 2, 1);

		m_brush->SetColor(D2D1::ColorF(D2D1::ColorF::Ivory));
		d2dc->DrawText(msg, msg.GetLength(), m_WriteFormat, CRect_to_d2Rect(rc), m_brush);

		HRESULT hr = d2dc->EndDraw();

		if (SUCCEEDED(hr))
			hr = m_d2dc.get_swapchain()->Present(0, 0);

		return;
	}

	Gdiplus::Graphics g(dc.GetSafeHdc());

	//확대 모드에 따른 이미지가 표시될 실제 영역을 구한다.
	if (m_fit2ctrl)
	{
		m_r_display = m_img[0].calc_rect(rc);// m_img.draw(g, rc);
		m_zoom = (double)m_r_display.Width() / (double)m_img[0].get_width();
	}
	else
	{
		//크게 확대해서 offset을 옮겨 그려주는 방식과
		//확대할 부분을 잘라 확대해서 그려주는 방식은
		//이론적으로는 후자가 더 효율적이지만 속도에 큰 차이가 느껴지진 않는다.
#if 0
		int nw = (double)rc.Width() / m_zoom;
		int nh = (double)rc.Height() / m_zoom;

		TRACE(_T("offset %d, %d\n"), m_offset.x, m_offset.y);
		Clamp(m_offset.x, (long)(rc.Width() - nw), (long)0);
		Clamp(m_offset.y, (long)(rc.Height() - nh), (long)0);
		TRACE(_T("offset %d, %d\n"), m_offset.x, m_offset.y);

		CSCGdiplusBitmap img;
		m_img.deep_copy(&img);
		img.sub_image(-m_offset.x, -m_offset.y, nw, nh);
		//g.DrawImage(img, 0, 0, rc.Width(), rc.Height());
		img.draw(&dc, 0, 0, rc.Width(), rc.Height());
#else
		//nw | nh가 rc보다 작으면 센터에 오도록 그려준다.
		int nw = m_img[0].get_width() * m_zoom;
		int nh = m_img[0].get_height() * m_zoom;

		Clamp(m_offset.x, (long)(rc.Width() - nw), (long)0);
		Clamp(m_offset.y, (long)(rc.Height() - nh), (long)0);

		int sx = m_offset.x;
		int sy = m_offset.y;

		if (nw < rc.Width())
		{
			sx = (rc.Width() - nw) / 2;
		}
		if (nh < rc.Height())
		{
			sy = (rc.Height() - nh) / 2;
		}

		m_r_display = make_rect(sx, sy, nw, nh);
#endif
	}

	//실제 이미지를 그려준다. 투명 이미지인 경우는 배경에 지그재그 패턴을 그려준 후에 실제 이미지를 그려준다.
	if (m_img[0].get_channel() == 4 || (m_img[0].get_channel() == 1 && get_part(m_img[0].get_filename(), fn_ext) == _T("gif")))
		d2dc->FillRectangle(CRect_to_d2Rect(m_r_display), m_d2dc.get_zigzag_brush().Get());

	m_img[0].draw(d2dc, CRect_to_d2Rect(m_r_display));

	//이미지 정보 표시
	if (m_show_info)// && m_parent->IsZoomed())
	{
		CString info_str;
		info_str.Format(_T("%s\n확대 배율 : %.0f%%\n표시 크기 : %d x %d"), m_info_str, m_zoom * 100.0, m_r_display.Width(), m_r_display.Height());

		//촬영된 사진이라면 exif 정보도 붙여서 출력해준다.
		if (m_img[0].get_exif_str().GetLength())
			info_str.Format(_T("%s\n\n%s"), info_str, m_img[0].get_exif_str());

		CRect rText = rc;
		rText.DeflateRect(8, 8);

		//draw_text()를 사용하면 간편할수는 있으나 이 함수안에서는 OnPaint()가 호출될 때마다 계속 객체를 생성, 해제하는 액션을 수행하므로 부담을 줄 수 있다.
		//기존처럼 이미 만들어진 인스턴스들에 대해 속성만 변경한 후 사용하는 것이 더 경제적일 수 있다.
		//대신 draw_text에서는 라인 간격도 조정 가능하다.
		draw_text(d2dc, rText, info_str, _T("맑은 고딕"), 14.0f, DWRITE_FONT_WEIGHT_NORMAL, Gdiplus::Color::White, Gdiplus::Color::Black, DT_LEFT | DT_TOP);
	}

	//dc에 그릴 경우 d2devicecontext가 그려지고 dc에 그려지므로 깜빡임이 발생한다.
	//반드시 CDC로 그려야 하는 경우가 아니라면 d2dc에 그려야 한다.
	if (m_show_pixel_pos && m_pixel_pos.X >= 0.0f && m_pixel_pos.Y >= 0.0f)
	{
		CString pixel_pos;
		pixel_pos.Format(_T("(%.f, %.f)"), m_pixel_pos.X, m_pixel_pos.Y);

		CRect rText = rc;
		rText.DeflateRect(8, 8);
		rText.top = rText.bottom - 14;

		//shadow 파라미터까지 추가된 함수 이용 코드
		//draw_text(d2dc, rText, pixel_pos, 12.0f, DWRITE_FONT_WEIGHT_NORMAL, Gdiplus::Color::White, Gdiplus::Color::Red, DT_RIGHT | DT_TOP);

		m_WriteFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
		m_WriteFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

		//shadow나 stroke 효과 적용방법을 아직 모르므로 우선 검은색으로 그리고 전경색으로 그린다.
		rText.OffsetRect(1, 1);
		m_brush->SetColor(D2D1::ColorF(D2D1::ColorF::Black));
		d2dc->DrawText(pixel_pos, pixel_pos.GetLength(), m_WriteFormat, CRect_to_d2Rect(rText), m_brush);

		rText.OffsetRect(-1, -1);
		m_brush->SetColor(D2D1::ColorF(D2D1::ColorF::Ivory));
		d2dc->DrawText(pixel_pos, pixel_pos.GetLength(), m_WriteFormat, CRect_to_d2Rect(rText), m_brush);
	}


	//roi를 그리거나 위치, 크기를 조정할 때는 오로지 m_screen_roi만 신경쓴다.
	if (!m_image_roi.IsEmptyArea() && m_screen_roi.IsEmptyArea())
		get_screen_coord_from_real_coord(m_r_display, m_img[0].get_width(), m_image_roi, &m_screen_roi);

	CRect screen_roi = gpRectF_to_CRect(m_screen_roi);
	screen_roi.InflateRect(0, 0, 2, 2);	//이렇게 2씩 늘려줘야 roi의 right, bottom이 정확히 픽셀과 일치되게 표시된다.
	screen_roi.NormalizeRect();

	//ID2D1SolidColorBrush *br_red = NULL;
	//d2dc->CreateSolidColorBrush(D2D1::ColorF(1.0f, 0.25f, 0.0f, 0.8f), &br_red);

	//ID2D1SolidColorBrush* br_roi;
	//d2dc->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.12f, 1.0f, 0.25f), &br_roi);

	if (m_lbutton_down)// && m_drawing_roi)
	{
		//d2dc->DrawRectangle(convert(screen_roi), br_red);
		//d2dc->FillRectangle(convert(screen_roi), br_roi);
		//draw_rect(g, screen_roi, Gdiplus::Color::Red, Gdiplus::Color(64, 0, 64, 255), 1);
		draw_rect(d2dc, screen_roi, Gdiplus::Color(0, 120, 212), Gdiplus::Color(64, 0, 32, 255));
	}
	else if (m_image_roi.Width >= 5.0f && m_image_roi.Height >= 5.0f)
	{
		//이미지 확대 축소 등에 의해 m_r_display가 변경되면 그에 따라 m_screen_roi도 다시 계산해줘야 한다.
		get_screen_coord_from_real_coord(m_r_display, m_img[0].get_width(), m_image_roi, &m_screen_roi);
		m_screen_roi.Offset(m_offset.x, m_offset.y);
		screen_roi = gpRectF_to_CRect(m_screen_roi);
		screen_roi.InflateRect(0, 0, 2, 2);	//이렇게 2씩 늘려줘야 roi의 right, bottom이 정확히 픽셀과 일치되게 표시된다.
		//이미 offset 변경에 의한 보정은 get_screen_coord_from_real_coord()에서 해준다.
		//screen_roi.OffsetRect(m_offset);
		//draw_rect(&dc, screen_roi, red, NULL_BRUSH, 1, PS_DASH, R2_XORPEN);
		//draw_rect(g, screen_roi, Gdiplus::Color::Red, Gdiplus::Color(64, 0, 64, 255), 1);
		//d2dc->FillRectangle(convert(screen_roi), br_roi);
		draw_rect(d2dc, screen_roi, Gdiplus::Color(0, 120, 212), Gdiplus::Color(64, 0, 32, 255));
	}

	if (!m_screen_roi.IsEmptyArea())
	{
		Gdiplus::RectF image_roi;
		get_real_coord_from_screen_coord(m_r_display, m_img[0].get_width(), CRect_to_gpRectF(screen_roi), &image_roi);
		get_resizable_handle(screen_roi, m_roi_handle, 4);

		//image_roi 역시 normalize_rect을 해줘야 한다. 그렇지 않으면 뒤집어 그릴 경우 x1,y1이 x2, y2보다 큰 좌표로 표시된다.
		normalize_rect(image_roi);

		//9군데 조절 핸들을 그려준다.
		for (int i = 1; i < RECT_RESIZE_HANDLE_COUNT; i++)
			draw_rect(d2dc, m_roi_handle[i], Gdiplus::Color::Red, Gdiplus::Color(128, 255, 32, 0));

		if (m_show_roi_info)
		{
			str.Format(_T("%.0f, %.0f"), image_roi.X, image_roi.Y);
			CSize sz = dc.GetTextExtent(str);
			m_brush->SetColor(D2D1::ColorF(D2D1::ColorF::Ivory));

			//lt
			m_WriteFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
			m_WriteFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
			d2dc->DrawText(str, str.GetLength(), m_WriteFormat,
				D2D1::RectF(screen_roi.left + 4, screen_roi.top + 2, screen_roi.left + 4 + sz.cx + 2, screen_roi.top + 2 + sz.cy), m_brush);

			//rb
			m_WriteFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
			m_WriteFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);
			str.Format(_T("%.0f, %.0f"), image_roi.X + image_roi.Width, image_roi.Y + image_roi.Height);
			sz = dc.GetTextExtent(str);
			d2dc->DrawText(str, str.GetLength(), m_WriteFormat,
				D2D1::RectF(screen_roi.left + screen_roi.Width() - 2 - sz.cx, screen_roi.top + screen_roi.Height() - sz.cy - 2, screen_roi.left + screen_roi.Width() - 2, screen_roi.top + screen_roi.Height()), m_brush);

			//size
			m_WriteFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
			m_WriteFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
			str.Format(_T("%.0f x %.0f"), image_roi.Width, image_roi.Height);
			sz = dc.GetTextExtent(str);
			d2dc->DrawText(str, str.GetLength(), m_WriteFormat,
				D2D1::RectF(screen_roi.CenterPoint().x - sz.cx / 2, screen_roi.CenterPoint().y + 2, screen_roi.CenterPoint().x + sz.cx / 2, screen_roi.CenterPoint().y + 2 + sz.cy), m_brush);
			/*
			CAutoFont af(_T("Arial"));
			af.SetHeight(14);

			CFont* pOldFont = dc.SelectObject(&af);

			CSize sz = dc.GetTextExtent(_T("A"));
			dc.SetBkMode(TRANSPARENT);

			//roi 영역 정보를 표시한다. 비록 image_roi는 변환식에 의해 소수점이지만 실제 이미지 픽셀은 정수로 표현해준다.
			//start
			dc.SetTextColor(yellow);
			str.Format(_T("%.0f, %.0f"), image_roi.X, image_roi.Y);
			sz = dc.GetTextExtent(str);
			DrawShadowText(dc.GetSafeHdc(), str, str.GetLength(),
				CRect(screen_roi.left + 4, screen_roi.top + 2, screen_roi.left + 4 + sz.cx + 2, screen_roi.top + 2 + sz.cy),
				DT_NOCLIP | DT_LEFT | DT_TOP, white, black, 2, 1);

			//end
			str.Format(_T("%.0f, %.0f"), image_roi.X + image_roi.Width, image_roi.Y + image_roi.Height);
			sz = dc.GetTextExtent(str);
			DrawShadowText(dc.GetSafeHdc(), str, str.GetLength(),
				CRect(screen_roi.left + screen_roi.Width() - 2 - sz.cx, screen_roi.top + screen_roi.Height() - sz.cy - 2, screen_roi.left + screen_roi.Width() - 2, screen_roi.top + screen_roi.Height()),
				DT_NOCLIP | DT_RIGHT | DT_BOTTOM, white, black, 2, 1);

			//size and distance
			dc.SetTextColor(green);
			str.Format(_T("%.0f x %.0f"), image_roi.Width, image_roi.Height);
			sz = dc.GetTextExtent(str);
			DrawShadowText(dc.GetSafeHdc(), str, str.GetLength(),
				CRect(screen_roi.CenterPoint().x - sz.cx / 2, screen_roi.CenterPoint().y + 2, screen_roi.CenterPoint().x + sz.cx / 2, screen_roi.CenterPoint().y + 2 + sz.cy),
				DT_NOCLIP | DT_CENTER | DT_TOP, yellow, black, 2, 1);

			dc.SelectObject(pOldFont);
			*/
		}
	}

	//for test line thickness and antialias
	/*
	d2dc->DrawLine(
		D2D1::Point2F(0.0f, 10.0f),
		D2D1::Point2F(200, 10),
		m_brush,
		1.0f
	);

	d2dc->DrawLine(
		D2D1::Point2F(0.0f, 20.0f),
		D2D1::Point2F(200, 20),
		m_brush,
		2.0f
	);

	d2dc->DrawLine(
		D2D1::Point2F(0.0f, 0.0f),
		D2D1::Point2F(sz_dc.width, sz_dc.height),
		m_brush,
		2.0f
	);
	*/

	HRESULT hr = d2dc->EndDraw();

	if (SUCCEEDED(hr))
		hr = m_d2dc.get_swapchain()->Present(0, 0);

	//원래 d2dc를 이용해서 그렸으나 cursor guide line을 R2_XORPEN으로 그려야 시각적으로 구분되므로
	//GDI DC를 이용해서 그려준다.
	if (m_show_cursor_guide_line && m_pixel_pos.X >= 0.0f && m_pixel_pos.Y >= 0.0f)
	{
		float x, y;
		get_screen_coord_from_real_coord(m_r_display, m_img[0].get_width(), (float)m_pixel_pos.X, (float)m_pixel_pos.Y, &x, &y);
		draw_line(&dc, m_r_display.left, y, m_r_display.right, y, lightgray, 1, PS_SOLID, R2_XORPEN);
		draw_line(&dc, x, m_r_display.top, x, m_r_display.bottom, lightgray, 1, PS_SOLID, R2_XORPEN);
	}
}

//이 컨트롤에서 자체 처리할 메시지와 반드시 parent에서 처리할 메시지를 명확히 해야 한다.
//여기서 바로 처리할 수 밖에 없는 메시지라면 모를까 모두 여기서 처리하면
//범용성이 떨어질 수 있다.
BOOL CSCImage2dDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

	//parent에서 처리해야 하는 메시지 일 경우 return FALSE;를 해줘야 한다.
	switch (pMsg->message)
	{
		case WM_SYSKEYDOWN :
		case WM_MBUTTONDOWN :
			return FALSE;
		/*
		case WM_LBUTTONDOWN:
			if (m_fit2ctrl)
			{
				return FALSE;
			}
			return TRUE;
		case WM_LBUTTONUP:
			if (m_fit2ctrl)
			{
				return FALSE;
			}
			return TRUE;
		*/
	}

	if (pMsg->message == WM_KEYDOWN)
	{
		bool is_ctrl_pressed = IsCtrlPressed();
		double interval = (IsCtrlPressed() ? 5.0 : 1.0);

		TRACE(_T("%s, %d\n"), __function__, pMsg->wParam);

		switch (pMsg->wParam)
		{
			case VK_RETURN :
			case VK_ESCAPE :
				m_thumb.stop_loading();
				return FALSE;
			case VK_SPACE :
 				if (m_img[0].is_animated_image())
				{
					pause(-1);
					return TRUE;
				}
				break;
			case 'B':
				set_view_mode(view_mode_toggle);
				return TRUE;
				/*
			case VK_ADD :
				if (IsShiftPressed() && !m_image_roi.IsEmptyArea())
				{
					m_image_roi.Inflate(Gdiplus::PointF(interval, interval));
					//get_screen_coord_from_real_coord(m_r_display, m_img.width, m_image_roi, &m_screen_roi);
					rerender();
					return TRUE;
				}
				zoom(1);
				return TRUE;
			case VK_SUBTRACT :
				if (IsShiftPressed() && !m_image_roi.IsEmptyArea())
				{
					m_image_roi.Inflate(Gdiplus::PointF(-interval, -interval));
					//get_screen_coord_from_real_coord(m_r_display, m_img.width, m_image_roi, &m_screen_roi);
					rerender();
					return TRUE;
				}
				zoom(-1);
				return TRUE;
			*/
			case VK_UP :
				if (IsShiftPressed() && !m_image_roi.IsEmptyArea())
				{
					m_image_roi.Y -= interval;
					rerender();
					return TRUE;
				}
				return FALSE;
			case VK_DOWN:
				if (IsShiftPressed() && !m_image_roi.IsEmptyArea())
				{
					m_image_roi.Y += interval;
					rerender();
					return TRUE;
				}
				return FALSE;
			case VK_LEFT:
				if (IsShiftPressed() && !m_image_roi.IsEmptyArea())
				{
					m_image_roi.X -= interval;
					rerender();
					return TRUE;
				}
				//창에 맞게 크기조절이 아니고 확대/축소되고 이미지 표시 영역이 rc를 벗어났다면 스크롤 처리
				else if (!m_fit2ctrl && !rect_in_rect(get_rc(m_hWnd), m_r_display))
				{
					scroll(is_ctrl_pressed ? interval * 8 : 8, 0);
					return true;
				}
				else if (m_img.size() > 0 && m_img[0].is_animated_image())
				{
					m_img[0].step(-1);
				}
				else
				{
					display_image(-2);
				}
				break;
			case VK_RIGHT:
				if (IsShiftPressed() && !m_image_roi.IsEmptyArea())
				{
					m_image_roi.X += interval;
					rerender();
					return TRUE;
				}
				//창에 맞게 크기조절이 아니고 확대/축소되고 이미지 표시 영역이 rc를 벗어났다면 스크롤 처리
				else if (!m_fit2ctrl && !rect_in_rect(get_rc(m_hWnd), m_r_display))
				{
					scroll(is_ctrl_pressed ? interval * -8 : -8, 0);
					return true;
				}
				else if (m_img.size() > 0 && m_img[0].is_animated_image())
				{
					m_img[0].step(1);
				}
				else
				{
					display_image(-2);
				}
				break;
		}

		return FALSE;
	}

	return CDialog::PreTranslateMessage(pMsg);
}

BOOL CSCImage2dDlg::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	return FALSE;
	return CDialog::OnEraseBkgnd(pDC);
}

//레지스트리에 저장된 recent file 정보가 있다면 로딩한다.
bool CSCImage2dDlg::load()
{
	CString recent = AfxGetApp()->GetProfileString(_T("setting\\SCImage2dDlg"), _T("recent file"), _T(""));
	return load(recent);
}

bool CSCImage2dDlg::load(CString sFile, bool load_thumbs)
{
	m_pixel_pos.X = -1.0f;
	m_pixel_pos.Y = -1.0f;

	AfxGetApp()->WriteProfileString(_T("setting\\CSCImage2dDlg"), _T("recent file"), sFile);

	//m_mutex.lock();
	if (m_img.size() < m_buffer_max)
		m_img.resize(m_buffer_max);
	//m_mutex.unlock();

	HRESULT res = m_img[0].load(m_d2dc.get_WICFactory(), m_d2dc.get_d2dc(), sFile);

	if (m_img[0].is_animated_image())
	{
		m_img[0].set_parent(m_hWnd);

		CRect rc;
		GetClientRect(rc);
		m_slider_gif.set_range(0, m_img[0].get_frame_count() - 1);
		m_slider_gif.set_pos(0);
		m_slider_gif.MoveWindow(rc.left + 8, rc.bottom - 8 - GIF_SLIDER_HEIGHT, GIF_SLIDER_WIDTH, GIF_SLIDER_HEIGHT);
		m_slider_gif.ShowWindow(SW_SHOW);
		
		//원래 CSCGdiplusBitmap은 animated gif를 로딩하면 set_animation() 함수를 호출하여 자체 재생되는 기능을 포함한다.
		//하지만 CSCImage2dDlg에서는 roi 설정, 다른 child ctrl들과의 충돌등이 있으므로 이 클래스에서 직접 재생한다.
		//m_img[0].set_gif_play_itself(false);

		//std::thread t(&CSCImage2dDlg::thread_gif_animation, this);
		//t.detach();
	}
	else
	{
		m_slider_gif.ShowWindow(SW_HIDE);
		rerender();
	}

	if (m_thumb.size() == 0 || load_thumbs)
		m_thumb.set_path(get_part(sFile, fn_folder));

	return (res == S_OK);
}

bool CSCImage2dDlg::load(CString sType, UINT id)
{

	m_filename.Format(_T("Resource Image(id:%d)"), id);

	m_pixel_pos.X = -1.0f;
	m_pixel_pos.Y = -1.0f;

	bool res = m_img[0].load(m_d2dc.get_WICFactory(), m_d2dc.get_d2dc(), id, sType);
	rerender();

	//::SendMessage(GetParent()->GetSafeHwnd(), Message_CImageStatic, (WPARAM)&CImageStaticMessage(this, message_loading_completed), 0);

	return res;
}

//png일 경우는 sType을 생략할 수 있다.
bool CSCImage2dDlg::load(UINT id)
{
	return load(_T("PNG"), id);
}

CString CSCImage2dDlg::get_filename(bool fullpath)
{
	if (m_index < 0 || m_index >= m_files.size() || m_img.size() == 0)
		return _T("");

	return m_img[0].get_filename(fullpath);
}

void CSCImage2dDlg::release()
{
	//for (auto img : m_img)
	//	img.release();

	m_thumb.release_thumb(-1);
	rerender();
}

void CSCImage2dDlg::set_show_info(bool show)
{
	m_show_info = show;
	AfxGetApp()->WriteProfileInt(_T("setting\\CSCImage2dDlg"), _T("show info"), show);
	rerender();
}

void CSCImage2dDlg::set_show_pixel(bool show)
{
	m_show_pixel = show;
	AfxGetApp()->WriteProfileInt(_T("setting\\CSCImage2dDlg"), _T("show pixel"), m_show_pixel);
}

//1:show, 0:hide, -1:toggle
void CSCImage2dDlg::set_show_cursor_guide_line(int show)
{
	if (show == -1)
		m_show_cursor_guide_line = !m_show_cursor_guide_line;
	else
		m_show_cursor_guide_line = (show == 1);

	AfxGetApp()->WriteProfileInt(_T("setting\\CSCImage2dDlg"), _T("show cursor guide line"), m_show_cursor_guide_line);
}

void CSCImage2dDlg::set_show_pixel_pos(bool show)
{
	m_show_pixel_pos = show;
	AfxGetApp()->WriteProfileInt(_T("setting\\CSCImage2dDlg"), _T("show pixel pos"), m_show_pixel_pos);
	rerender();
}

void CSCImage2dDlg::rerender()
{
	if (m_img.size() == 0)
		return;

	D2D1_SIZE_F sz = m_d2dc.get_size();
	m_d2dc.on_size_changed(sz.width, sz.height);
	Invalidate();
}

bool CSCImage2dDlg::copy_to_clipboard(int type)
{
	D2D1_RECT_U r;
	D2D1_SIZE_F sz = m_img[0].get_size();
	D2D1_POINT_2U pt{ 0, 0 };

	if ((type == copy_auto) && (m_image_roi.IsEmptyArea() == false))
	{
		r = D2D1::RectU(m_image_roi.X, m_image_roi.Y, m_image_roi.GetRight(), m_image_roi.GetBottom());
	}
	else
	{
		r = D2D1::RectU(0, 0, sz.width, sz.height);
	}
	
	//ID2D1Bitmap1* img;
	CSCD2Image img(m_img[0].get_WICFactory2(), m_d2dc.get_d2dc(), r.right - r.left, r.bottom - r.top);

	//CSCD2Image img;
	m_img[0].get_sub_img(r, &img);
	img.save(_T("d:\\sub.png"));

	return true;
}

bool CSCImage2dDlg::paste_from_clipboard()
{
	/*
	stop_gif();

	if (m_img[0].paste_from_clipboard())
	{
		m_slider_gif.ShowWindow(SW_HIDE);
		m_filename = _T("image from clipboard");
		set_alt_info(_T(""));
		set_view_mode(view_mode_image);
		rerender();
		return true;
	}
	*/
	return false;
}

Gdiplus::RectF CSCImage2dDlg::get_image_roi()
{
	//if (m_image_roi.IsEmptyArea())
	//	return Gdiplus::RectF(0, 0, m_img.width, m_img.height);

	return m_image_roi;
}

void CSCImage2dDlg::set_image_roi(Gdiplus::RectF roi)
{
	m_image_roi = roi;
	get_screen_coord_from_real_coord(m_r_display, m_img[0].get_width(), m_image_roi, &m_screen_roi);
	rerender();
}

D2D1_BITMAP_INTERPOLATION_MODE CSCImage2dDlg::get_interpolation_mode()
{
	return m_interpolation_mode;
}

//이미지 부드럽게 보정
void CSCImage2dDlg::set_interpolation_mode(D2D1_BITMAP_INTERPOLATION_MODE mode)
{
	m_interpolation_mode = mode;
	m_img[0].set_interpolation_mode(m_interpolation_mode);
	AfxGetApp()->WriteProfileInt(_T("setting\\CSCImage2dDlg"), _T("interpolation mode"), mode);
	rerender();
}

void CSCImage2dDlg::rotate(Gdiplus::RotateFlipType type)
{
	if (m_img[0].is_empty())
		return;

	//m_img[0].rotate(type);
	rerender();
}

void CSCImage2dDlg::fit2ctrl(bool fit, bool invalidate)
{
	m_fit2ctrl = fit;
	AfxGetApp()->WriteProfileInt(_T("setting\\CSCImage2dDlg"), _T("fit to ctrl"), m_fit2ctrl);
	write_profile_value(_T("setting\\CSCImage2dDlg"), _T("zoom ratio"), m_zoom);

	if (invalidate)
		rerender();
}

//mode : 1(zoom in), -1(zoom out), 0(reset)
void CSCImage2dDlg::zoom(int mode)
{
	if (mode > 0)
		m_zoom += 0.2;
	else if (mode < 0)
		m_zoom -= 0.2;
	else
		m_zoom = 1.0;

	Clamp(m_zoom, 0.2, 40.0);

	//m_fit2ctrl의 값을 직접 변경하지 말고 fit2ctrl() 함수를 호출해야만
	//registry에 설정이 저장된다.
	//m_fit2ctrl = false;
	fit2ctrl(false, false);

	//get_screen_coord_from_real_coord(m_r_display, m_img.width, m_image_roi, &m_screen_roi);
	//Wait(1);
	rerender();
}

void CSCImage2dDlg::zoom(double ratio)
{
	m_zoom = ratio;
	Clamp(m_zoom, 0.2, 40.0);
	fit2ctrl(false, false);
	rerender();
}

void CSCImage2dDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	if (m_static_pixel.m_hWnd == NULL)
		return;

	CRect rc;
	GetClientRect(rc);

	m_thumb.MoveWindow(rc);
	m_slider_gif.MoveWindow(rc.left + 8, rc.bottom - 8 - GIF_SLIDER_HEIGHT, GIF_SLIDER_WIDTH, GIF_SLIDER_HEIGHT);

	//m_img[0].on_resize(m_d2dc.get_d2dc(), m_d2dc.get_swapchain(), cx, cy);
	m_d2dc.on_size_changed(cx, cy);

	m_r_pixel_pos = rc;
	m_r_pixel_pos.DeflateRect(4, 4);
	m_r_pixel_pos.left = m_r_pixel_pos.right - 100;
	m_r_pixel_pos.top = m_r_pixel_pos.bottom - 16;

	Invalidate();

	if (m_show_pixel)
	{
		CRect r_pixel = make_rect(rc.left + 8, rc.bottom - 8 - PIXEL_INFO_CY, PIXEL_INFO_CX, PIXEL_INFO_CY);
		m_static_pixel.MoveWindow(r_pixel);
	}
}

void CSCImage2dDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	Clamp(point.x, m_r_display.left, m_r_display.right);
	Clamp(point.y, m_r_display.top, m_r_display.bottom);

	//if (m_r_display.PtInRect(point) == false)
	//	return;

	m_lbutton_down = true;
	m_ptClicked = point;
	SetCapture();

	//roi 설정중
	if (IsCtrlPressed())
	{
		m_drawing_roi = true;
		m_screen_roi = Gdiplus::RectF(point.x, point.y, 0, 0);
		
		//실제 이미지에서 1픽셀 단위로 선택되어야 하므로 float이 아닌 int로 변경한 후
		//해당 픽셀의 크기를 다시 screen_roi로 넣어줘야 픽셀 단위로 선택된다.
		Gdiplus::RectF img_roi;
		get_real_coord_from_screen_coord(m_r_display, m_img[0].get_width(), m_screen_roi, &img_roi);
		img_roi.X = (int)img_roi.X;
		img_roi.Y = (int)img_roi.Y;
		get_screen_coord_from_real_coord(m_r_display, m_img[0].get_width(), img_roi, &m_screen_roi);

		return;
	}
	//roi의 크기를 변경중
	else if (m_handle_index >= 0)
	{
		m_drawing_roi = false;
	}
	else if (m_fit2ctrl)
	{
		//::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCImage2dDlg, (WPARAM)&CSCImage2dDlgMessage(this, message_lbuttondown), 0);
		::DefWindowProc(GetParent()->GetSafeHwnd(), WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(point.x, point.y));
		//GetParent()->PostMessage(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(point.x, point.y));
	}

	CDialog::OnLButtonDown(nFlags, point);
}

void CSCImage2dDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (m_lbutton_down)
	{
		ReleaseCapture();
		m_lbutton_down = false;

		if (m_drawing_roi)
		{
			m_drawing_roi = false;

			m_screen_roi.Width = point.x - m_screen_roi.X;
			m_screen_roi.Height = point.y - m_screen_roi.Y;

			//실제 이미지에서 1픽셀 단위로 선택되어야 하므로 float이 아닌 int로 변경한 후
			//해당 픽셀의 크기를 다시 screen_roi로 넣어줘야 픽셀 단위로 선택된다.
			Gdiplus::RectF img_roi;
			get_real_coord_from_screen_coord(m_r_display, m_img[0].get_width(), m_screen_roi, &img_roi);
			img_roi.X = ROUND(img_roi.X, 0);
			img_roi.Y = ROUND(img_roi.Y, 0);
			img_roi.Width = ROUND(img_roi.Width, 0);
			img_roi.Height = ROUND(img_roi.Height, 0);
			get_screen_coord_from_real_coord(m_r_display, m_img[0].get_width(), img_roi, &m_screen_roi);

			//5x5 미만일 경우는 roi를 그리지 않는다.
			if (fabs(img_roi.Width) < 5.0 || fabs(img_roi.Height) < 5.0)
			{
				m_screen_roi = Gdiplus::RectF();
				m_image_roi = Gdiplus::RectF();
				write_profile_value(_T("setting\\CSCImage2dDlg"), _T("image roi"), m_image_roi);
				rerender();
				return;
			}

			//roi가 모두 그려지면, 또는 이동, 크기조정이 완료되면 m_image_roi로 변환해준다.
			normalize_rect(m_screen_roi);
			get_real_coord_from_screen_coord(m_r_display, m_img[0].get_width(), m_screen_roi, &m_image_roi);
			write_profile_value(_T("setting\\CSCImage2dDlg"), _T("image roi"), m_image_roi);
			TRACE(_T("roi completed.\n"));
			rerender();
		}
	}

	CDialog::OnLButtonUp(nFlags, point);
}

void CSCImage2dDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	TRACKMOUSEEVENT tme;
	tme.cbSize = sizeof(tme);
	tme.hwndTrack = m_hWnd;
	tme.dwFlags = TME_LEAVE | TME_HOVER;
	tme.dwHoverTime = 1;
	_TrackMouseEvent(&tme);

	if (m_lbutton_down)
	{
		Clamp(point.x, m_r_display.left, m_r_display.right);
		Clamp(point.y, m_r_display.top, m_r_display.bottom);

		//roi 설정중인 경우
		if (m_drawing_roi)
		{
			m_static_pixel.ShowWindow(SW_HIDE);

			m_screen_roi.Width = point.x - m_screen_roi.X;
			m_screen_roi.Height = point.y - m_screen_roi.Y;

			//실제 이미지에서 1픽셀 단위로 선택되어야 하므로 float이 아닌 int로 변경한 후
			//해당 픽셀의 크기를 다시 screen_roi로 넣어줘야 픽셀 단위로 선택된다.
			Gdiplus::RectF img_roi;
			get_real_coord_from_screen_coord(m_r_display, m_img[0].get_width(), m_screen_roi, &img_roi);
			img_roi.Width = ROUND(img_roi.Width, 0);
			img_roi.Height = ROUND(img_roi.Height, 0);
			get_screen_coord_from_real_coord(m_r_display, m_img[0].get_width(), img_roi, &m_screen_roi);
			//Ctrl키를 눌러서 roi가 그려지는 동안에는 이미지 확대/축소에 무관하게
			//마우스 위치가 그대로 그려져야 한다.
			//InvalidateRect(GpRectF2CRect(m_screen_roi));
			rerender();
		}
		//그려진 roi의 크기, 위치를 조절하는 경우
		else if (m_handle_index >= 0)
		{
			TRACE(_T("m_handle_index = %d\n"), m_handle_index);
			switch (m_handle_index)
			{
				case corner_inside :
					m_screen_roi.X = point.x - m_screen_roi.Width / 2;
					m_screen_roi.Y = point.y - m_screen_roi.Height / 2;
					adjust_rect_range(m_screen_roi, CRect_to_gpRectF(m_r_display));
					break;
				case corner_left :
					set_left(m_screen_roi, point.x);
					break;
				case corner_right:
					m_screen_roi.Width = point.x - m_screen_roi.X;
					break;
				case corner_top:
					set_top(m_screen_roi, point.y);
					break;
				case corner_bottom:
					m_screen_roi.Height = point.y - m_screen_roi.Y;
					break;
				case corner_topleft:
					set_top(m_screen_roi, point.y);
					set_left(m_screen_roi, point.x);
					break;
				case corner_topright :
					set_top(m_screen_roi, point.y);
					m_screen_roi.Width = point.x - m_screen_roi.X;
					break;
				case corner_bottomleft :
					m_screen_roi.Height = point.y - m_screen_roi.Y;
					set_left(m_screen_roi, point.x);
					break;
				case corner_bottomright :
					m_screen_roi.Height = point.y - m_screen_roi.Y;
					m_screen_roi.Width = point.x - m_screen_roi.X;
					break;
			}

			Gdiplus::RectF img_roi;
			get_real_coord_from_screen_coord(m_r_display, m_img[0].get_width(), m_screen_roi, &img_roi);
			img_roi.X = ROUND(img_roi.X, 0);
			img_roi.Y = ROUND(img_roi.Y, 0);
			img_roi.Width = ROUND(img_roi.Width, 0);
			img_roi.Height = ROUND(img_roi.Height, 0);
			get_screen_coord_from_real_coord(m_r_display, m_img[0].get_width(), img_roi, &m_screen_roi);
			rerender();
		}
		//이미지 이동을 위한 단순 드래그인 경우
		else
		{
			m_offset.x += (point.x - m_ptClicked.x);
			m_offset.y += (point.y - m_ptClicked.y);

			//int nw = m_img.width * m_zoom;
			//int nh = m_img.height * m_zoom;

			//CRect rc;
			//GetClientRect(rc);
			//Clamp(m_offset.x, (long)(rc.Width() - nw), (long)0);
			//Clamp(m_offset.y, (long)(rc.Height() - nh), (long)0);

			//확대한 이미지를 마우스로 이동해서 offset이 변경됐다면 roi 위치도 같이 이동시켜줘야 한다.
			if (m_screen_roi.IsEmptyArea() == false)
			{
				//m_screen_roi.X += m_offset.x;
				//m_screen_roi.Y += m_offset.y;
				//m_screen_roi.X += (point.x - m_ptClicked.x);
				//m_screen_roi.Y += (point.y - m_ptClicked.y);
				//m_screen_roi.Offset(m_offset.x, m_offset.y);
			}
			
			//Gdiplus::RectF img_roi;
			//get_real_coord_from_screen_coord(m_r_display, m_img[0].width, m_screen_roi, &img_roi);
			//img_roi.X = ROUND(img_roi.X, 0);
			//img_roi.Y = ROUND(img_roi.Y, 0);
			//img_roi.Width = ROUND(img_roi.Width, 0);
			//img_roi.Height = ROUND(img_roi.Height, 0);
			//get_screen_coord_from_real_coord(m_r_display, m_img[0].width, img_roi, &m_screen_roi);
			
			rerender();
			m_ptClicked = point;
		}
	}
	else if (m_show_pixel)
	{
		CPoint pt;
		get_real_coord_from_screen_coord(m_r_display, m_img[0].get_width(), point, &pt);
		if (pt.x < 0 || pt.x >= m_img[0].get_width() || pt.y < 0 || pt.y >= m_img[0].get_height())
		{
			if (m_static_pixel.IsWindowVisible())
				m_static_pixel.ShowWindow(SW_HIDE);
		}
		else
		{
			if (m_static_pixel.IsWindowVisible() == false)
				m_static_pixel.ShowWindow(SW_SHOW);

			m_cr_pixel = m_img[0].get_pixel(pt.x, pt.y);

			//픽셀 정보 표시창을 커서	위치에 맞춰서 이동시킨다. rc를 벗어나지 않도록 보정까지 한다.
			CRect r = make_rect(point.x + 24, point.y, PIXEL_INFO_CX, PIXEL_INFO_CY);

			CRect rc;
			GetClientRect(rc);

			adjust_rect_range(r, rc, true);
			m_static_pixel.MoveWindow(r);

			CString str;

			str.Format(_T("(%d,%d)\nA %3d\nR %3d\nG %3d\nB %3d"), pt.x, pt.y, m_cr_pixel.GetA(), m_cr_pixel.GetR(), m_cr_pixel.GetG(), m_cr_pixel.GetB());

			{
				m_static_pixel.set_back_color(m_cr_pixel.GetA() == 0 ? Gdiplus::Color::White : m_cr_pixel);
				m_static_pixel.set_text_color(m_cr_pixel.GetA() == 0 ? Gdiplus::Color::DimGray : get_distinct_color(m_cr_pixel));
				m_static_pixel.set_text(str);
				m_cr_pixel_old = m_cr_pixel;
			}
		}
	}

	if (m_show_pixel_pos)
	{
		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(tme);
		tme.hwndTrack = m_hWnd;
		tme.dwFlags = TME_LEAVE | TME_HOVER;
		tme.dwHoverTime = 1;
		_TrackMouseEvent(&tme);

		float x, y;
		get_real_coord_from_screen_coord(m_r_display, m_img[0].get_width(), (float)point.x, (float)point.y, &x, &y);

		if (x < 0.0f || x >= m_img[0].get_width() || y < 0.0f || y >= m_img[0].get_height())
			m_pixel_pos = Gdiplus::PointF(-1.0f, -1.0f);
		else
			m_pixel_pos = Gdiplus::PointF(x, y);

		if (m_r_pixel_pos.IsRectEmpty())
		{
			CRect rc;
			GetClientRect(rc);
			m_r_pixel_pos = rc;
			m_r_pixel_pos.DeflateRect(4, 4);
			m_r_pixel_pos.left = m_r_pixel_pos.right - 100;
			m_r_pixel_pos.top = m_r_pixel_pos.bottom - 16;
		}

		//InvalidateRect(m_r_pixel_pos, FALSE);
		rerender();
	}

	::SendMessage(m_parent->m_hWnd, WM_MOUSEMOVE, 0, MAKELONG(point.x, point.y));

	CDialog::OnMouseMove(nFlags, point);
}

void CSCImage2dDlg::OnMouseHover(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	CDialog::OnMouseHover(nFlags, point);
}

void CSCImage2dDlg::OnMouseLeave()
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	m_static_pixel.ShowWindow(SW_HIDE);

	CDialog::OnMouseLeave();
}

BOOL CSCImage2dDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (m_show_thumb)
		return CDialog::OnSetCursor(pWnd, nHitTest, message);

	//roi를 그리기 위해 Ctrl키를 누른 상태라면 cross 커서로
	if (IsCtrlPressed())
	{
		//SetCursor(AfxGetApp()->LoadStandardCursor(IDC_CROSS));
		SetCursor(m_cursor_cross);
		return TRUE;
	}

	//roi가 그려진 상태라면 roi의 move, resize 위치에 맞는 커서로 변경한다.
	if (m_screen_roi.IsEmptyArea() == false)
	{
		CPoint pt;
		CRect rc;

		GetClientRect(rc);

		GetCursorPos(&pt);
		ScreenToClient(&pt);

		m_handle_index = -1;

		for (int i = 0; i < ROI_RECT_HANDLE_COUNT; i++)
		{
			if (m_roi_handle[i].PtInRect(pt))
			{
				m_handle_index = i;
				break;
			}
		}



		if (m_handle_index != -1)
		{
			//마우스 커서 모양을 변경한다.
			switch (m_handle_index)
			{
				case corner_inside :
					SetCursor(AfxGetApp()->LoadStandardCursor(IDC_SIZEALL));
					break;
				case corner_left :
				case corner_right :
					SetCursor(AfxGetApp()->LoadStandardCursor(IDC_SIZEWE));
					break;
				case corner_top :
				case corner_bottom :
					SetCursor(AfxGetApp()->LoadStandardCursor(IDC_SIZENS));
					break;
				case corner_topleft :
				case corner_bottomright :
					SetCursor(AfxGetApp()->LoadStandardCursor(IDC_SIZENWSE));
					break;
				case corner_topright :
				case corner_bottomleft :
					SetCursor(AfxGetApp()->LoadStandardCursor(IDC_SIZENESW));
					break;
			}

			return TRUE;
		}
		else
		{
			SetCursor(m_cursor_cross);
			return TRUE;
		}
	}

	CPoint pt;
	GetCursorPos(&pt);

	CPoint client_pt = pt;
	ScreenToClient(&client_pt);

	//CSCImage2dDlg이고 이미지 표시 영역에 한해
	if (WindowFromPoint(pt) == this && m_r_display.PtInRect(client_pt))
	{
		//픽셀 정보 표시라면 스포이트를
		if (m_show_pixel && m_cursor_dropper)
			SetCursor(m_cursor_dropper);
		//그렇지 않다면 십자 커서를 표시한다.
		else
			//SetCursor(AfxGetApp()->LoadStandardCursor(IDC_CROSS));
			SetCursor(m_cursor_cross);
		return TRUE;
	}

	//기타 다른 컨트롤 또는 이미지 영역밖은 기본 커서로 표시한다.
	return CDialog::OnSetCursor(pWnd, nHitTest, message);
}

void CSCImage2dDlg::set_cross_cursor(UINT nID)
{
	m_cursor_cross = AfxGetApp()->LoadCursor(nID);
}

//dropper(spuit) cursor 지정
void CSCImage2dDlg::set_dropper_cursor(UINT nID)
{
	m_cursor_dropper = AfxGetApp()->LoadCursor(nID);
}

void CSCImage2dDlg::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CDialog::OnWindowPosChanged(lpwndpos);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
}

LRESULT CSCImage2dDlg::on_message_from_CSCD2Image(WPARAM wParam, LPARAM lParam)
{
	CSCD2ImageMessage* msg = (CSCD2ImageMessage*)wParam;
	if (!msg)
		return 0;

	if (msg->message == CSCD2Image::message_frame_changed)
	{
		//TRACE(_T("%d / %d\n"), msg->frame_index, msg->total_frames);
		m_slider_gif.set_pos(msg->frame_index);
		rerender();
	}

	return 0;
}

LRESULT CSCImage2dDlg::on_message_from_CSCSliderCtrl(WPARAM wParam, LPARAM lParam)
{
	CSCSliderCtrlMsg* msg = (CSCSliderCtrlMsg*)wParam;
	if (msg->msg == CSCSliderCtrlMsg::msg_thumb_move)
	{
		TRACE(_T("%ld, pos = %d\n"), GetTickCount(), msg->pos);
		goto_frame(msg->pos, true);
	}
	return 0;
}

//pos위치로 이동한 후 일시정지한다. -1이면 pause <-> play를 토글한다.
void CSCImage2dDlg::pause(int pos)
{
	if (m_img.size() == 0)
		return;

	m_img[0].pause(pos);
}

void CSCImage2dDlg::stop()
{
	if (m_img.size() == 0)
		return;

	m_img[0].stop();
}

void CSCImage2dDlg::goto_frame(int pos, bool pause)
{
	if (m_img.size() == 0)
		return;

	m_img[0].goto_frame(pos, pause);
}

//지정 % 위치의 프레임으로 이동
void CSCImage2dDlg::goto_frame_percent(int pos, bool pause)
{
	double dpos = ((double)pos / (double)m_img[0].get_frame_count()) * 100.0;
	goto_frame((int)dpos, pause);
}

/*
void CSCImage2dDlg::thread_gif_animation()
{
	if (m_img[0].get_frame_count() < 2)
		return;

	m_frame_index = 0;
	m_run_thread_animation = true;
	m_thread_animation_terminated = false;

	long t0 = clock();
	long t1 = t0;

	while (m_run_thread_animation)
	{
		GUID   pageGuid = Gdiplus::FrameDimensionTime;

		if (m_paused)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		if (!m_run_thread_animation)
			break;

		trace(m_frame_index);
		m_slider_gif.set_pos(m_frame_index + 1);

		m_frame_index++;
		if (m_frame_index >= m_img[0].get_frame_count())
			m_frame_index = 0;

		m_img[0].m_pBitmap->SelectActiveFrame(&pageGuid, m_frame_index);

		if (!m_run_thread_animation)
			break;

		Invalidate();

		//delay는 해당 프레임에 설정된 delay를 그대로 주면 안되고
		//전 프레임부터 현 프레임까지 재생하는데 걸린 시간은 빼줘야 한다.
		t1 = clock();
		long display_delay = t1 - t0;
		long delay = ((long*)(m_img[0].m_frame_delay->value))[m_frame_index] * 10;

		if (delay > display_delay)
			std::this_thread::sleep_for(std::chrono::milliseconds((delay - display_delay)));
		t0 = clock();
	}

	m_run_thread_animation = false;
	m_thread_animation_terminated = true;
}
*/

void CSCImage2dDlg::set_zigzag_color(Gdiplus::Color cr_back, Gdiplus::Color cr_fore)
{
	//m_br_zigzag.release();
	//m_br_zigzag = CSCGdiplusBitmap::get_zigzag_pattern(32, cr_back, cr_fore);
	rerender();
}

LRESULT CSCImage2dDlg::on_message_CSCThumbCtrl(WPARAM wParam, LPARAM lParam)
{
	CString str;
	CSCThumbCtrlMessage* msg = (CSCThumbCtrlMessage*)wParam;

	if (msg->msg == CSCThumbCtrl::message_thumb_lbutton_dbclicked)
	{
		m_show_thumb = false;
		m_thumb.ShowWindow(m_show_thumb ? SW_SHOW : SW_HIDE);

		//thumb를 ldbclick하면 해당 이미지 파일을 직접 여는게 아니라 CASeeDlg에서 해당 이미지의 인덱스를 display시켜야 한다.
		goto_index(msg->index);
		//load(m_thumb.m_thumb[msg->index].full_path, false);
		//::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCImageDlg, (WPARAM)&CSCImageDlgMessage(this, message_load_image, msg->index), 0);
	}
	else if (msg->msg == CSCThumbCtrl::message_thumb_loading_completed)
	{
		//TRACE(_T("loading completed. %ldms\n"), m_thumb.get_loading_elapsed());
	}
	else if (msg->msg == CSCThumbCtrl::message_thumb_rename)
	{
		CString folder = get_part(m_thumb.m_thumb[msg->index].full_path, fn_folder);
		CString ext = get_part(m_thumb.m_thumb[msg->index].full_path, fn_ext);
		CString oldName;
		CString newName;
		bool filename_error = false;

		if (m_thumb.m_thumb[msg->index].title.FindOneOf(FILENAME_NOT_ALLOWED_CHAR) >= 0)
		{
			str.Format(_T("파일 이름에는 다음 문자를 사용할 수 없습니다.\n\n\\ : * ? \" < > |"));
			filename_error = true;
		}

		oldName.Format(_T("%s\\%s"), folder, m_thumb.get_old_title());
		newName.Format(_T("%s\\%s"), folder, m_thumb.m_thumb[msg->index].title);

		//확장자를 표시하고 있지 않았다면 title에 확장자를 붙여서 rename해야 한다.
		if (!m_thumb.get_show_file_extension())
		{
			oldName = oldName + _T(".") + ext;
			newName = newName + _T(".") + ext;
		}

		if (PathFileExists(newName))
		{
			str.Format(_T("%s\n\n동일한 이름의 파일이 이미 존재합니다."), newName);
			filename_error = true;
		}

		if (filename_error)
		{
			AfxMessageBox(str, MB_ICONEXCLAMATION);
			m_thumb.set_title(msg->index, m_thumb.get_old_title());
			return 0;
		}

		//str.Format(_T("%s\n=>\n%s 로 변경합니다."), oldName, newName);
		//int res = AfxMessageBox(str, MB_ICONQUESTION | MB_OKCANCEL);
		//if (res == IDOK)
		{
			if (MoveFile(oldName, newName))
			{
				m_thumb.select_item(-1, false, false);
				str = m_thumb.m_thumb[msg->index].title;
				m_thumb.sort_by_title();
				Wait(50);
				int index = m_thumb.find_by_title(str);
				m_thumb.select_item(index);
				return 0;
			}
			else
			{
				str.Format(_T("%s\n\n위 파일 이름으로 변경하지 못했습니다."), newName);
				AfxMessageBox(str, MB_ICONEXCLAMATION);
			}
		}

		m_thumb.set_title(msg->index, m_thumb.get_old_title());
	}
	else if (msg->msg == CSCThumbCtrl::message_thumb_reload)
	{
		//CString recent_folder = AfxGetApp()->GetProfileString(_T("setting\\CSCImage2dDlg"), _T("recent folder"), _T(""));
		//m_thumb.set_path(recent_folder);
	}

	return 0;
}

void CSCImage2dDlg::set_view_mode(int view_mode)
{
	if (view_mode == view_mode_toggle)
		m_show_thumb = !m_show_thumb;
	else if (view_mode == view_mode_image)
		m_show_thumb = false;
	else if (view_mode == view_mode_thumb)
		m_show_thumb = true;
	else
		return;

	m_thumb.select_item(m_filename);
	m_static_pixel.ShowWindow(m_show_thumb || !m_show_pixel ? SW_HIDE : SW_SHOW);
	m_slider_gif.ShowWindow(m_show_thumb || !m_img[0].is_animated_image() ? SW_HIDE : SW_SHOW);
	m_thumb.ShowWindow(m_show_thumb ? SW_SHOW : SW_HIDE);

	if (m_show_thumb)
		m_thumb.SetFocus();
}

void CSCImage2dDlg::display_image(CString filepath, bool scan_folder)
{
	m_mutex.lock();
	m_files.clear();
	m_img.clear();
	m_img.resize(m_buffer_max);
	m_mutex.unlock();

	CString path = filepath;

	if (PathFileExists(filepath))
	{
		if (!PathIsDirectory(filepath))
			path = get_part(filepath, fn_folder);
	}
	else
	{
		rerender();
		return;
	}

	m_files = find_all_files(path, _T("*"), FILE_EXTENSION_IMAGE, _T(""), false);
	int index = find_index(m_files, filepath);

	if (m_files.size() > 0 && index < 0)
		index = 0;

	display_image(index, true);
}

//index : -1(next), -2(previous)
void CSCImage2dDlg::display_image(int index, bool scan_folder)
{
	::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCImage2dDlg, (WPARAM)&CSCImage2dDlgMessage(this, message_hide_message), 0);

	if (m_files.size() == 0)
		return;

	//animated gif를 재생중이었다면 stop시켜야 한다.
	//그렇지 않으면 m_img.clear(), m_img.pop_front()에서 에러가 발생한다.
	stop();

	//next image
	if (index == -1)
	{
		if (m_index >= m_files.size() - 1)
		{
			//show_message(_T("맨 마지막 이미지"));
			::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCImage2dDlg, (WPARAM)&CSCImage2dDlgMessage(this, message_last_image), 0);
			return;
		}
		m_index++;

		//foward / backward가 토글되면 버퍼링 된 이미지까지 모두 비워주고 다시 채워준다.
		m_mutex.lock();

		if (!m_is_forward)
		{
			m_is_forward = true;
			m_img.clear();
		}
		else
		{
			m_img.pop_front();
		}

		m_mutex.unlock();
	}
	//previous image
	else if (index == -2)
	{
		if (m_index == 0)
		{
			//show_message(_T("맨 처음 이미지"));
			::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCImage2dDlg, (WPARAM)&CSCImage2dDlgMessage(this, message_first_image), 0);
			return;
		}
		m_index--;

		//foward / backward가 토글되면 버퍼링 된 이미지까지 모두 비워주고 다시 채워준다.
		m_mutex.lock();

		if (m_is_forward)
		{
			m_is_forward = false;
			m_img.clear();
		}
		else
		{
			m_img.pop_front();
		}

		m_mutex.unlock();
	}
	else
	{
		m_index = index;
	}

	//m_notice.ShowWindow(SW_HIDE);

	CString folder = get_part(m_files[m_index], fn_folder);
	CString recent_folder = AfxGetApp()->GetProfileString(_T("setting\\CSCImage2dDlg"), _T("recent folder"), _T(""));
	AfxGetApp()->WriteProfileString(_T("setting\\CSCImage2dDlg"), _T("recent folder"), folder);

	//cur image가 아직 load되지 않았다면 파일을 로딩하지만
	//미리 버퍼링 된 이미지가 있다면 화면만 갱신하면 된다.
	//단, animated gif인 경우는 바로 재생까지 시켜줘야 하므로 load()를 다시 호출해줘야 한다.
	m_mutex.lock();
	if (m_img.size() == 0 || m_img[0].is_empty() || m_img[0].is_animated_image())
		load(m_files[m_index], folder != recent_folder);
	m_mutex.unlock();

	m_filename = m_img[0].get_filename();
	if (!m_img[0].is_animated_image())
		m_slider_gif.ShowWindow(SW_HIDE);

	build_image_info_str();

	CRect rc;
	GetClientRect(rc);
	//m_img[0].on_resize(m_d2dc.get_d2dc(), m_d2dc.get_swapchain(), rc.Width(), rc.Height());

	m_img[0].set_interpolation_mode(m_interpolation_mode);
	rerender();

	if (scan_folder)
	{
		CString cur_file = m_files[m_index];

		m_files.clear();

		m_files = find_all_files(get_part(cur_file, fn_folder), _T("*"), FILE_EXTENSION_IMAGE, _T(""), false, true);
		m_index = find_index(m_files, cur_file);
	}

	AfxGetApp()->WriteProfileString(_T("setting\\CSCImage2dDlg"), _T("recent file"), m_filename);
	add_registry_str(AfxGetApp(), _T("setting\\CSCImage2dDlg\\recent folders"), folder);

	//현재 이미지를 시작으로 forward or backward 버퍼링을 시작한다.
	//버퍼링 중이었다면 버퍼링을 중지시킨 후 해야 한다.
	//단, 빠른 전환시에는 문제될 수 있으므로 일정 시간후에 thread를 구동한다.
	if (m_buffer_max > 1)
		SetTimer(timer_thread_buffering, 100, NULL);

	::SendMessage(GetParent()->GetSafeHwnd(), Message_CSCImage2dDlg, (WPARAM)&CSCImage2dDlgMessage(this, message_image_changed), 0);
}

void CSCImage2dDlg::build_image_info_str()
{
	CString ratio_str;
	float ratio = m_img[0].get_ratio();

	//if (m_image_from_clipboard)
	//	filename = _T("paste image from clipboard");
	//else
	//	filename = get_part(m_img[0].get_filename(), fn_name);

	if (m_img[0].get_width() == m_img[0].get_height())
		ratio_str = _T("1 : 1");
	else if ((int)(ratio * 1000) % 1000 == 0)
		ratio_str.Format(_T("%.0f : 1"), ratio);
	else
		ratio_str.Format(_T("%.3f : 1"), ratio);

	m_info_str.Format(_T("파일 이름 : %s\n파일 크기: %s (%s)\n수정 날짜: %s\n이미지 정보: %.0fx%.0fx%d (%s)\n이미지 비율: %s"),
		m_filename + m_alt_info,
		get_file_size_str(m_img[0].get_filename()),		//KB 단위
		get_file_size_str(m_img[0].get_filename(), 0),	//byte 단위
		get_datetime_str(GetFileLastModifiedTime(m_img[0].get_filename())),
		m_img[0].get_width(), m_img[0].get_height(),
		m_img[0].get_channel() * 8,
		m_img[0].get_pixel_format_str(),
		ratio_str);

	////촬영된 사진이라면 exif 정보도 붙여서 출력해준다.
	//if (m_img[0].get_exif_str().GetLength())
	//{
	//	m_info_str.Format(_T("%s\n\n%s"), m_info_str, m_img[0].get_exif_str());
	//}
}

void CSCImage2dDlg::goto_index(int index)
{
	m_mutex.lock();
	m_img.clear();

	if (index == -1 || index == get_image_count() - 1)
	{
		index = get_image_count() - 1;
		m_is_forward = false;
	}
	else
	{
		m_is_forward = true;
	}

	m_mutex.unlock();

	display_image(index);
}

//테스트 목적으로 파일을 저장하여 확인할 경우는 주의해야 한다.
//ASee 프로젝트에서는 CDirWatcher에 의해 현재 이미지가 속한 폴더를 모니터링하고 있으므로
//계속 refresh하게 되어 thread_buffering()에서 계속 오류가 발생한 적이 있다.
void CSCImage2dDlg::thread_buffering()
{
	TRACE(_T("thread_buffering start...\n"));

	int cur_idx = m_index;
	m_thread_buffering = true;

	m_mutex.lock();
	if (m_img.size() != 5)
		m_img.resize(m_buffer_max);
	m_mutex.unlock();

	while (m_thread_buffering)
	{
		if (m_is_forward)
		{
			cur_idx++;

			//맨 마지막 파일까지 버퍼링되었다면 중지.
			if (cur_idx == m_files.size())
				break;

			//버퍼링 된 갯수가 최대라면 중지.
			if (cur_idx - m_index >= m_buffer_max)
				break;

			m_mutex.lock();
			//현재 버퍼에 이미지가 비어있다면 로딩.
			if (((cur_idx - m_index) < m_img.size()) && m_img[cur_idx - m_index].is_empty())
			{
				m_img[cur_idx - m_index].load(m_d2dc.get_WICFactory(), m_d2dc.get_d2dc(), m_files[cur_idx]);
				TRACE(_T("m_img[%d] buffered.\n"), cur_idx - m_index);
			}
			m_mutex.unlock();
		}
		else
		{
			cur_idx--;

			//맨 처음 파일까지 버퍼링되었다면 중지.
			if (cur_idx < 0)
				break;

			//버퍼링 된 갯수가 최대라면 중지.
			if (m_index - cur_idx >= m_buffer_max)
				break;

			m_mutex.lock();
			//현재 버퍼에 이미지가 비어있다면 로딩.
			if (((m_index - cur_idx) < m_img.size()) && m_img[m_index - cur_idx].is_empty())
			{
				m_img[m_index - cur_idx].load(m_d2dc.get_WICFactory(), m_d2dc.get_d2dc(), m_files[cur_idx]);
				TRACE(_T("m_img[%d] buffered.\n"), m_index - cur_idx);
			}
			m_mutex.unlock();
		}
	}

	m_thread_buffering = false;
	TRACE(_T("thread_buffering terminated.\n"));
}

//현재 파일을 비롯해서 폴더를 다시 검사한다.
void CSCImage2dDlg::reload_image()
{
	//현재 파일의 인덱스와 파일명을 기억해두고
	int old_index = m_index;
	CString sfile = m_files[m_index];

	m_files.clear();
	m_img.clear();
	m_img.resize(m_buffer_max);

	//현재 폴더의 이미지 파일들을 다시 검색하고
	m_files = find_all_files(get_part(sfile, fn_folder), _T("*"), FILE_EXTENSION_IMAGE, _T(""), false);

	//검색된 파일들에서 현재 이미지의 인덱스를 찾고
	int index = find_dqstring(m_files, sfile);

	if (m_files.size() == 0)
	{
		rerender();
		return;
	}

	if (index < 0)
	{
		if (old_index >= 0 && old_index < m_files.size())
			index = old_index;
		else
			index = 0;
	}
	else if (index >= m_files.size())
	{
		index = m_files.size() - 1;
	}

	display_image(index, false);
}

HRESULT CSCImage2dDlg::save(CString filepath, float quality)
{
	if (m_img[0].is_empty())
		return S_FALSE;

	return m_img[0].save(filepath, quality);
}

CSize CSCImage2dDlg::get_img_size()
{
	if (m_img[0].is_empty())
		return CSize();

	return CSize(m_img[0].get_width(), m_img[0].get_height());
}

int CSCImage2dDlg::get_channel()
{
	if (m_img.size() == 0 || m_img[0].is_empty())
		return 0;

	return m_img[0].get_channel();
}

//start : 1(start), 0(stop), -1(toggle)
void CSCImage2dDlg::start_slide_show(int start)
{
	if (start == -1)
		m_slide_show = !m_slide_show;
	else
		m_slide_show = start;

	if (m_slide_show)
	{
		if (m_slide_show_interval < 0)
		{
			SetTimer(timer_slide_show, -m_slide_show_interval, NULL);
		}
		else
		{
			SetTimer(timer_slide_show, m_slide_show_interval * 1000, NULL);
		}
	}
	else
	{
		KillTimer(timer_slide_show);
	}
}
void CSCImage2dDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (nIDEvent == timer_thread_buffering)
	{
		KillTimer(timer_thread_buffering);
		std::thread t(&CSCImage2dDlg::thread_buffering, this);
		t.detach();
	}

	CDialog::OnTimer(nIDEvent);
}

void CSCImage2dDlg::scroll(int offset_x, int offset_y)
{
	m_offset.x += offset_x;
	m_offset.y += offset_y;
	rerender();
}

//exif
double CSCImage2dDlg::get_gps_latitude()
{
	if (m_img.size() == 0 || !m_img[0].is_valid() || m_img[0].get_exif().gps_latitude == 0.0f)
		return 0.0;

	return m_img[0].get_exif().gps_latitude;
}

double CSCImage2dDlg::get_gps_longitude()
{
	if (m_img.size() == 0 || !m_img[0].is_valid() || m_img[0].get_exif().gps_longitude)
		return 0.0;

	return m_img[0].get_exif().gps_longitude;
}

