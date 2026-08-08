#include "SCVideoWndD2.h"

//ffmpeg_internal.h 가 avformat/avcodec/avutil/swscale 을 extern "C" 로 묶어 include 하고
//#pragma comment(lib, ...) 로 자동 링크까지 한다. 프로젝트에는 lib 경로만 넣어주면 된다.
#include "../../ffmpeg/internal/ffi_decoder.h"
#include "../../log/SCLog/SCLog.h"

IMPLEMENT_DYNAMIC(CSCVideoWndD2, CWnd)

BEGIN_MESSAGE_MAP(CSCVideoWndD2, CWnd)
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_DESTROY()
	ON_WM_LBUTTONDBLCLK()
	ON_MESSAGE(Message_CSCVideoWndD2, &CSCVideoWndD2::on_message_CSCVideoWndD2)
END_MESSAGE_MAP()

static D2D1_COLOR_F to_d2color(Gdiplus::Color color)
{
	return D2D1::ColorF(color.GetR() / 255.0f, color.GetG() / 255.0f, color.GetB() / 255.0f, color.GetA() / 255.0f);
}

CSCVideoWndD2::CSCVideoWndD2()
{
}

CSCVideoWndD2::~CSCVideoWndD2()
{
	close();
}

void CSCVideoWndD2::PreSubclassWindow()
{
	//D2D 스왑체인이 자식 컨트롤 영역까지 덮어 깜빡이지 않도록.
	ModifyStyle(0, WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

	init_d2d();

	CWnd::PreSubclassWindow();
}

bool CSCVideoWndD2::init_d2d()
{
	if (m_d2d_ready)
		return true;

	if (GetSafeHwnd() == NULL)
		return false;

	//20260808 by claude. 이 컨트롤은 더블클릭을 쓴다. 그런데 WM_LBUTTONDBLCLK 은 윈도우 클래스에
	//CS_DBLCLKS 가 있을 때만 전달된다. 호출측이 어떤 클래스로 만들든 여기서 보장한다.
	DWORD class_style = (DWORD)GetClassLongPtr(GetSafeHwnd(), GCL_STYLE);

	if (!(class_style & CS_DBLCLKS))
		SetClassLongPtr(GetSafeHwnd(), GCL_STYLE, class_style | CS_DBLCLKS);

	if (FAILED(m_d2context.init(GetSafeHwnd())))
	{
		logWriteE(_T("CSCD2Context::init failed"));
		return false;
	}

	m_d2d_ready = true;

	return true;
}

bool CSCVideoWndD2::open(CString path, double start_ms)
{
	close();

	if (!init_d2d())
		return false;

	m_decoder = std::make_unique<ffi::CDecoder>();

	if (!m_decoder->open(path))
	{
		logWriteE(_T("ffi::CDecoder::open failed : %s"), (LPCTSTR)path);
		m_decoder.reset();
		return false;
	}

	m_width = m_decoder->video_width();
	m_height = m_decoder->video_height();
	m_fps = m_decoder->frame_rate();
	m_duration_ms = m_decoder->duration_ms();
	m_hw_accel = m_decoder->video_hw_accel_name().c_str();

	if (m_fps <= 0.0)
		m_fps = 30.0;

	if (m_width <= 0 || m_height <= 0)
	{
		logWriteE(_T("invalid video size %d x %d : %s"), m_width, m_height, (LPCTSTR)path);
		m_decoder.reset();
		return false;
	}

	//동영상 해상도 크기의 스트리밍 텍스처를 한 번만 만든다. 이후에는 픽셀만 교체한다.
	//D2D1_BITMAP_OPTIONS_TARGET 은 주지 않는다 — 렌더타깃 비트맵은 CopyFromMemory 경로가 느리다.
	D2D1_BITMAP_PROPERTIES1 properties =
		D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_NONE,
			D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE));

	m_bitmap.Reset();
	HRESULT hr = m_d2context.get_d2dc()->CreateBitmap(D2D1::SizeU(m_width, m_height), nullptr, 0, properties, m_bitmap.GetAddressOf());
	if (FAILED(hr))
	{
		logWriteE(_T("CreateBitmap failed. hr = 0x%08X, %d x %d"), hr, m_width, m_height);
		m_decoder.reset();
		return false;
	}

	{
		std::lock_guard<std::mutex> lock(m_bgra_mutex);
		m_bgra = cv::Mat(m_height, m_width, CV_8UC4, cv::Scalar(0, 0, 0, 255));
	}

	m_frame_count = 0;
	m_play_fps = 0.0;

	logWriteI(_T("video opened : %s  %d x %d, %.3f fps, %.0f ms, hw = %s"),
		(LPCTSTR)path, m_width, m_height, m_fps, m_duration_ms,
		m_hw_accel.IsEmpty() ? _T("(none)") : (LPCTSTR)m_hw_accel);

	m_decoder->start();

	//worker 가 돌기 시작한 뒤에 seek 해야 요청이 처리된다.
	if (start_ms > 0.0)
	{
		m_decoder->seek(start_ms);
		m_decoder->wait_seek_done(3000);
		m_position_ms = start_ms;
	}

	m_thread_stop = false;
	m_playing = true;
	m_pacer_thread = std::thread(&CSCVideoWndD2::pacer_thread_proc, this);

	return true;
}

void CSCVideoWndD2::seek(double pos_ms)
{
	if (m_decoder == nullptr)
		return;

	m_decoder->seek(pos_ms);
	m_position_ms = pos_ms;
}

void CSCVideoWndD2::toggle_play()
{
	m_playing = !m_playing.load();
}

void CSCVideoWndD2::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	//콜백이 처리했다고 하면 재생 토글은 하지 않는다.
	if (m_dblclick == nullptr || !m_dblclick(point, calc_image_rect()))
		toggle_play();

	CWnd::OnLButtonDblClk(nFlags, point);
}

void CSCVideoWndD2::close()
{
	m_thread_stop = true;
	m_playing = false;

	if (m_pacer_thread.joinable())
		m_pacer_thread.join();

	if (m_decoder)
	{
		m_decoder->stop();
		m_decoder->close();
		m_decoder.reset();
	}

	if (m_sws)
	{
		sws_freeContext(m_sws);
		m_sws = nullptr;
	}

	m_bitmap.Reset();

	{
		std::lock_guard<std::mutex> lock(m_bgra_mutex);
		m_bgra.release();
	}

	m_render_pending = false;
	m_bgra_dirty = false;
}

void CSCVideoWndD2::play()
{
	m_playing = true;
}

void CSCVideoWndD2::pause()
{
	m_playing = false;
}

bool CSCVideoWndD2::get_frame(cv::Mat &bgra)
{
	std::lock_guard<std::mutex> lock(m_bgra_mutex);

	if (m_bgra.empty())
		return false;

	bgra = m_bgra.clone();

	return true;
}

bool CSCVideoWndD2::get_frame_bgr(cv::Mat &bgr)
{
	std::lock_guard<std::mutex> lock(m_bgra_mutex);

	if (m_bgra.empty())
		return false;

	//dst 크기·타입이 같으면 cvtColor 가 버퍼를 재사용하므로 매 호출 할당이 없다.
	cv::cvtColor(m_bgra, bgr, cv::COLOR_BGRA2BGR);

	return true;
}

bool CSCVideoWndD2::convert_to_bgra(AVFrame* frame)
{
	if (frame == nullptr || frame->width <= 0 || frame->height <= 0)
		return false;

	//디코더는 NV12 로 내려주는 것이 보통이지만 코덱/파일에 따라 yuv420p 등도 온다.
	//sws_getCachedContext 가 입력 포맷이 바뀌면 컨텍스트를 알아서 다시 만든다.
	m_sws = sws_getCachedContext(m_sws,
		frame->width, frame->height, (AVPixelFormat)frame->format,
		m_width, m_height, AV_PIX_FMT_BGRA,
		SWS_BILINEAR, nullptr, nullptr, nullptr);

	if (m_sws == nullptr)
		return false;

	std::lock_guard<std::mutex> lock(m_bgra_mutex);

	if (m_bgra.empty())
		return false;

	uint8_t* dst[4] = { m_bgra.data, nullptr, nullptr, nullptr };
	int dst_stride[4] = { (int)m_bgra.step, 0, 0, 0 };

	return sws_scale(m_sws, frame->data, frame->linesize, 0, frame->height, dst, dst_stride) > 0;
}

void CSCVideoWndD2::pacer_thread_proc()
{
	//WM_TIMER(해상도 15.6ms, 최저 우선순위) 대신 고해상도 waitable timer 로 페이싱한다.
	//CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 은 Win10 1803+ 에서만 되므로 실패 시 일반 타이머로 폴백.
	HANDLE timer = CreateWaitableTimerEx(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
	if (timer == nullptr)
		timer = CreateWaitableTimer(nullptr, FALSE, nullptr);

	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);

	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);

	//드리프트가 누적되지 않도록 절대 시각 스케줄을 유지한다.
	double interval_ticks = (double)freq.QuadPart / m_fps;
	double next_tick = (double)now.QuadPart + interval_ticks;

	LARGE_INTEGER fps_base = now;
	int fps_count = 0;

	while (!m_thread_stop)
	{
		QueryPerformanceCounter(&now);

		double remain_ticks = next_tick - (double)now.QuadPart;
		if (remain_ticks > 0)
		{
			LARGE_INTEGER due;
			due.QuadPart = -(LONGLONG)(remain_ticks * 10000000.0 / (double)freq.QuadPart);

			if (timer && SetWaitableTimer(timer, &due, 0, nullptr, nullptr, FALSE))
				WaitForSingleObject(timer, INFINITE);
			else
				Sleep(1);
		}
		else if (remain_ticks < -interval_ticks * 4)
		{
			//4프레임 이상 밀렸으면 따라잡기를 포기하고 기준을 현재로 재설정한다.
			next_tick = (double)now.QuadPart;
		}

		next_tick += interval_ticks;

		if (m_thread_stop)
			break;

		if (!m_playing)
			continue;

		AVFrame* frame = m_decoder ? m_decoder->pop_video_frame() : nullptr;
		if (frame == nullptr)
		{
			//큐가 빈 채로 EOF 면 끝까지 재생한 것 — 처음으로 되돌린다.
			//seek 한 번에 is_eof 가 풀리므로 반복 요청되지 않는다.
			if (m_repeat && m_decoder && m_decoder->is_eof() && m_decoder->video_queue_size() == 0)
				m_decoder->seek(0.0);

			continue;	//디코더가 아직 못 따라옴 — 직전 프레임을 그대로 둔다.
		}

		if (frame->best_effort_timestamp != AV_NOPTS_VALUE && m_decoder)
			m_position_ms = frame->best_effort_timestamp * av_q2d(m_decoder->video_time_base()) * 1000.0;

		bool converted = convert_to_bgra(frame);
		av_frame_free(&frame);

		if (!converted)
			continue;

		m_bgra_dirty = true;
		m_frame_count++;

		if (++fps_count >= 30)
		{
			QueryPerformanceCounter(&now);
			m_play_fps = fps_count * (double)freq.QuadPart / (double)(now.QuadPart - fps_base.QuadPart);
			fps_base = now;
			fps_count = 0;
		}

		//UI 스레드가 밀려도 메시지가 쌓이지 않도록 한 장만 예약한다.
		if (!m_render_pending.exchange(true))
			PostMessage(Message_CSCVideoWndD2, (WPARAM)m_frame_count.load());
	}

	if (timer)
		CloseHandle(timer);
}

LRESULT CSCVideoWndD2::on_message_CSCVideoWndD2(WPARAM wParam, LPARAM)
{
	m_render_pending = false;

	render();

	CWnd* parent = GetParent();
	if (parent && parent->GetSafeHwnd())
		parent->PostMessage(Message_CSCVideoWndD2, wParam, 0);

	return 0;
}

D2D1_RECT_F CSCVideoWndD2::calc_image_rect()
{
	D2D1_SIZE_F size = m_d2context.get_size();

	if (m_width <= 0 || m_height <= 0 || size.width <= 0 || size.height <= 0)
		return D2D1::RectF(0, 0, size.width, size.height);

	//가로세로 비율을 유지한 채 창 안에 최대로 채운다.
	float scale_x = size.width / (float)m_width;
	float scale_y = size.height / (float)m_height;
	float scale = (scale_x < scale_y) ? scale_x : scale_y;

	float w = m_width * scale;
	float h = m_height * scale;
	float x = (size.width - w) / 2.0f;
	float y = (size.height - h) / 2.0f;

	return D2D1::RectF(x, y, x + w, y + h);
}

void CSCVideoWndD2::render()
{
	if (!m_d2d_ready)
		return;

	ID2D1DeviceContext* d2dc = m_d2context.get_d2dc();
	if (d2dc == nullptr)
		return;

	LARGE_INTEGER t0, t1, freq;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&t0);

	//새 프레임이 있을 때만 GPU 로 올린다. 리사이즈/노출로 인한 재그리기는 업로드 없이 다시 그리기만 한다.
	if (m_bitmap && m_bgra_dirty.exchange(false))
	{
		std::lock_guard<std::mutex> lock(m_bgra_mutex);

		if (!m_bgra.empty())
		{
			D2D1_RECT_U rect = D2D1::RectU(0, 0, m_width, m_height);
			m_bitmap->CopyFromMemory(&rect, m_bgra.data, (UINT32)m_bgra.step);
		}
	}

	d2dc->BeginDraw();
	d2dc->SetTransform(D2D1::Matrix3x2F::Identity());
	d2dc->Clear(to_d2color(m_cr_back));

	D2D1_RECT_F image_rect = calc_image_rect();

	if (m_bitmap)
	{
		//확대/축소는 GPU 샘플러가 처리한다. 창 크기가 커져도 비용이 거의 늘지 않는다.
		d2dc->DrawBitmap(m_bitmap.Get(), image_rect, 1.0f, D2D1_INTERPOLATION_MODE_LINEAR);

		if (m_overlay)
			m_overlay(d2dc, image_rect);
	}

	HRESULT hr = d2dc->EndDraw();

	if (SUCCEEDED(hr))
		hr = m_d2context.get_swapchain()->Present(0, 0);

	if (hr == D2DERR_RECREATE_TARGET || hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
	{
		//디바이스 로스트 — 컨텍스트와 디바이스 의존 자원(비트맵)을 모두 다시 만든다.
		logWriteW(_T("D2D device lost. hr = 0x%08X. recreating."), hr);

		m_bitmap.Reset();

		if (SUCCEEDED(m_d2context.handle_device_lost()) && m_width > 0 && m_height > 0)
		{
			D2D1_BITMAP_PROPERTIES1 properties =
				D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_NONE,
					D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE));

			m_d2context.get_d2dc()->CreateBitmap(D2D1::SizeU(m_width, m_height), nullptr, 0, properties, m_bitmap.GetAddressOf());
			m_bgra_dirty = true;
		}

		m_device_generation++;
	}

	QueryPerformanceCounter(&t1);
	m_render_ms = (double)(t1.QuadPart - t0.QuadPart) * 1000.0 / (double)freq.QuadPart;
}

void CSCVideoWndD2::OnPaint()
{
	CPaintDC dc(this);

	render();
}

BOOL CSCVideoWndD2::OnEraseBkgnd(CDC*)
{
	//D2D 가 전체를 덮으므로 GDI 배경 지우기는 깜빡임만 만든다.
	return TRUE;
}

void CSCVideoWndD2::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	if (m_d2d_ready && cx > 0 && cy > 0)
	{
		m_d2context.on_size_changed(cx, cy);
		render();
	}
}

void CSCVideoWndD2::OnDestroy()
{
	close();

	CWnd::OnDestroy();
}
