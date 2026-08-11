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
	ON_WM_LBUTTONDOWN()
	ON_WM_TIMER()
	ON_MESSAGE(Message_CSCVideoWndD2, &CSCVideoWndD2::on_message_CSCVideoWndD2)
	ON_REGISTERED_MESSAGE(Message_CSCSliderCtrl, &CSCVideoWndD2::on_message_CSCSliderCtrl)
END_MESSAGE_MAP()

static D2D1_COLOR_F to_d2color(Gdiplus::Color color)
{
	return D2D1::ColorF(color.GetR() / 255.0f, color.GetG() / 255.0f, color.GetB() / 255.0f, color.GetA() / 255.0f);
}

//20260811 by claude. 프레임의 표시 시각(ms). 시각을 못 구하면 음수.
//ffi::CDecoder 는 best_effort_timestamp 를 채우지 않고 pts 만 넘긴다(ffi_decoder.cpp:1476, 1722).
//다른 디코더로 바뀌어도 동작하도록 best_effort 를 폴백으로 둔다.
double CSCVideoWndD2::frame_position_ms(const AVFrame* frame) const
{
	if (frame == nullptr || m_decoder == nullptr)
		return -1.0;

	int64_t timestamp = (frame->pts != AV_NOPTS_VALUE) ? frame->pts : frame->best_effort_timestamp;

	if (timestamp == AV_NOPTS_VALUE)
		return -1.0;

	return timestamp * av_q2d(m_decoder->video_time_base()) * 1000.0;
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

	create_slider();

	CWnd::PreSubclassWindow();
}

void CSCVideoWndD2::create_slider()
{
	if (GetSafeHwnd() == NULL)
		return;

	CRect rc;
	GetClientRect(rc);

	//열기 전에는 표시할 구간이 없으므로 WS_VISIBLE 없이 만든다. open() 에서 보인다.
	m_slider.Create(WS_CHILD, CRect(0, rc.Height() - slider_height, rc.Width(), rc.Height() - 20), this, 0);

	//썸 없이 경과 구간만 칠하는 스타일. 4px 두께에서 썸은 그릴 자리가 없다.
	m_slider.set_style(CSCSliderCtrl::style_progress);
	m_slider.set_track_height((float)slider_height);
	m_slider.set_track_color(Gdiplus::Color::RoyalBlue, Gdiplus::Color(255, 64, 64, 64));
	m_slider.set_use_slide();

	//기본값이 text_style_value 라 "위치 / 전체" 가 트랙 위에 그려진다. 4px 높이에 글자를 얹을 자리가 없다.
	m_slider.set_text_style(CSCSliderCtrl::text_style_none);

	//20260811 by claude. 슬라이더는 컨트롤 전체를 배경색으로 칠한 뒤(SCSliderCtrl.cpp:156) 그 위에
	//GDI+ 로 트랙을 채운다. AntiAlias 가 켜져 있어 채움의 가장자리 한 행이 반투명으로 합성되므로,
	//배경이 테마 회색이면 그 줄이 비쳐 트랙 위쪽에 회색 선처럼 보인다.
	//이 컨트롤은 비디오 창의 자식이므로 테마색이 아니라 그 창의 배경색을 써야 한다.
	m_slider.set_back_color(m_cr_back);
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

bool CSCVideoWndD2::create_frame_bitmap()
{
	//해상도 크기의 스트리밍 텍스처를 한 번만 만든다. 이후에는 픽셀만 교체한다.
	//D2D1_BITMAP_OPTIONS_TARGET 은 주지 않는다 — 렌더타깃 비트맵은 CopyFromMemory 경로가 느리다.
	D2D1_BITMAP_PROPERTIES1 properties =
		D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_NONE,
			D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE));

	m_bitmap.Reset();

	HRESULT hr = m_d2context.get_d2dc()->CreateBitmap(D2D1::SizeU(m_width, m_height), nullptr, 0, properties, m_bitmap.GetAddressOf());

	if (FAILED(hr))
	{
		logWriteE(_T("CreateBitmap failed. hr = 0x%08X, %d x %d"), hr, m_width, m_height);
		return false;
	}

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

	//20260811 by claude. 키프레임 단위 seek 을 끈다. 이 컨트롤은 미디어 재생기가 아니라 영상처리·검출
	//도구의 표시부라, 탐색 속도보다 "요청한 위치의 그 프레임" 이 정확히 나오는 것이 중요하다.
	//켜져 있으면 GOP 가 긴 파일에서 방향키 이동이 같은 키프레임으로 되돌아와 제자리를 맴돈다.
	m_decoder->set_seek_keyframe_mode(false);

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

	if (!create_frame_bitmap())
	{
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

	if (m_slider.GetSafeHwnd())
	{
		m_slider.set_range(0, (int)m_duration_ms);
		m_slider.set_pos((int)m_position_ms.load());
		m_slider.ShowWindow(SW_SHOW);
	}

	m_thread_stop = false;
	m_playing = true;
	m_pacer_thread = std::thread(&CSCVideoWndD2::pacer_thread_proc, this);

	return true;
}

bool CSCVideoWndD2::open_image(const cv::Mat &image)
{
	close();

	if (image.empty())
		return false;

	if (!init_d2d())
		return false;

	m_width = image.cols;
	m_height = image.rows;

	//시간축이 없다는 뜻으로 0 을 남긴다. is_image() 와 함께 호출측이 UI 를 구분하는 근거가 된다.
	m_fps = 0.0;
	m_duration_ms = 0.0;
	m_position_ms = 0.0;
	m_hw_accel.Empty();

	if (!create_frame_bitmap())
		return false;

	{
		std::lock_guard<std::mutex> lock(m_bgra_mutex);

		//디코더가 sws_scale 로 채워주는 자리를 여기서는 색공간 변환 한 번으로 대신한다.
		//이후 get_frame() / get_frame_bgr() / render() 는 동영상과 같은 버퍼를 본다.
		if (image.channels() == 4)
			m_bgra = image.clone();
		else
			cv::cvtColor(image, m_bgra, (image.channels() == 1) ? cv::COLOR_GRAY2BGRA : cv::COLOR_BGR2BGRA);
	}

	m_bgra_dirty = true;
	m_frame_count = 1;
	m_play_fps = 0.0;

	logWriteI(_T("image opened : %d x %d x %dch"), m_width, m_height, image.channels());

	//동영상이라면 pacer 스레드가 프레임마다 하는 일 — 렌더 + 부모 통지 — 을 한 번만 한다.
	//부모는 "프레임이 갱신됐다" 는 같은 코드로 반응하면 된다.
	if (!m_render_pending.exchange(true))
		PostMessage(Message_CSCVideoWndD2, (WPARAM)m_frame_count.load());

	return true;
}

void CSCVideoWndD2::seek(double pos_ms)
{
	if (m_decoder == nullptr)
		return;

	m_decoder->seek(pos_ms);
	m_position_ms = pos_ms;

	//pacer 가 이 위치 이전 프레임을 흘려버리도록 목표를 남긴다.
	m_seek_target_ms = pos_ms;

	//프레임 스텝 앵커는 여기서 끊는다. 임의 위치로 뛴 뒤에는 이전 앵커가 의미 없다.
	//step_frame() 은 이 호출 직후 앵커를 자기 목표값으로 다시 세운다.
	m_step_anchor_ms = -1.0;

	//일시정지 중이어도 이동한 위치의 화면은 바로 보여야 한다.
	m_render_one = true;
}

void CSCVideoWndD2::seek_relative(double delta_ms)
{
	if (m_decoder == nullptr)
		return;

	double pos = m_position_ms.load() + delta_ms;

	if (pos < 0.0)
		pos = 0.0;

	//끝에 정확히 붙으면 EOF 로 빠져 반복 재생이 즉시 돌아버린다. 조금 앞으로.
	if (m_duration_ms > 0.0 && pos > m_duration_ms - 200.0)
		pos = m_duration_ms - 200.0;

	seek(pos);
}

bool CSCVideoWndD2::handle_key(UINT key)
{
	if (m_decoder == nullptr)
		return false;

	if (key == VK_SPACE)
	{
		toggle_play();
		return true;
	}

	//20260811 by claude. D / F 로 한 프레임씩 이동. 검출 결과를 프레임 단위로 확인하려면 필수다.
	//프레임 이동은 정지 상태에서만 의미가 있으므로 재생 중이면 먼저 멈춘다.
	if (key == 'D' || key == 'F')
	{
		m_playing = false;

		step_frame((key == 'D') ? -1 : 1);

		return true;
	}

	if (key != VK_LEFT && key != VK_RIGHT)
		return false;

	//Ctrl 조합이면 큰 폭으로.
	double step = (GetKeyState(VK_CONTROL) & 0x8000) ? m_seek_step_large_ms : m_seek_step_ms;

	seek_relative((key == VK_LEFT) ? -step : step);

	return true;
}

void CSCVideoWndD2::step_frame(int count)
{
	if (m_decoder == nullptr || m_fps <= 0.0)
		return;

	double interval_ms = 1000.0 / m_fps;

	//앵커가 무효면(외부 seek·재생 직후) 현재 표시 위치에서 새로 시작한다.
	if (m_step_anchor_ms < 0.0)
		m_step_anchor_ms = m_position_ms.load();

	double target = m_step_anchor_ms + interval_ms * count;

	if (target < 0.0)
		target = 0.0;

	if (m_duration_ms > 0.0 && target > m_duration_ms - interval_ms)
		target = m_duration_ms - interval_ms;

	//seek() 가 앵커를 무효화하므로 그 뒤에 다시 세운다. 다음 스텝은 실제 착지 pts 가 아니라
	//이 값에서 이어가야 오차가 쌓이지 않는다.
	seek(target);

	m_step_anchor_ms = target;
}

BOOL CSCVideoWndD2::PreTranslateMessage(MSG* pMsg)
{
	//이 창이 포커스를 가진 경우의 경로. 부모 다이얼로그가 방향키를 먼저 가져가는
	//경우를 대비해 handle_key() 를 public 으로 열어두었다.
	if (pMsg->message == WM_KEYDOWN && handle_key((UINT)pMsg->wParam))
		return TRUE;

	return CWnd::PreTranslateMessage(pMsg);
}

void CSCVideoWndD2::toggle_play()
{
	if (m_decoder == nullptr)
		return;

	m_playing = !m_playing.load();
}

void CSCVideoWndD2::OnLButtonDown(UINT nFlags, CPoint point)
{
	//클릭하면 포커스를 가져와 이후 방향키가 이 창으로 온다.
	SetFocus();

	CWnd::OnLButtonDown(nFlags, point);
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
	m_render_one = false;

	//20260809 by claude. 크기를 0 으로 되돌려야 is_opened() / calc_image_rect() 가
	//닫힌 상태를 알아본다. 정지 이미지는 디코더가 없어서 이 값이 유일한 판단 근거다.
	m_width = 0;
	m_height = 0;
	m_fps = 0.0;
	m_duration_ms = 0.0;
	m_position_ms = 0.0;
	m_frame_count = 0;
	m_play_fps = 0.0;
	m_hw_accel.Empty();

	m_drag_grabbed = false;
	m_drag_pending_pos = -1;
	m_drag_last_seek_tick = 0;
	m_step_anchor_ms = -1.0;
	m_seek_target_ms = -1.0;

	if (m_slider.GetSafeHwnd())
	{
		KillTimer(timer_drag_throttle);
		m_slider.set_pos(0);
		m_slider.ShowWindow(SW_HIDE);
	}
}

void CSCVideoWndD2::play()
{
	//정지 이미지에는 재생 개념이 없다. pacer 스레드가 없어 상태만 어긋난다.
	if (m_decoder == nullptr)
		return;

	//재생을 재개하면 프레임 스텝 앵커는 의미를 잃는다. 다음 스텝은 멈춘 그 위치에서 새로 잡는다.
	m_step_anchor_ms = -1.0;

	m_playing = true;
}

void CSCVideoWndD2::pause()
{
	if (m_decoder == nullptr)
		return;

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

		//일시정지 중이라도 탐색 직후 한 장은 그려야 한다.
		bool render_one = m_render_one.load();

		if (!m_playing && !render_one)
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

		//20260811 by claude. seek 요청 위치 이전 프레임을 흘려버린다.
		//CDecoder 의 정확 모드가 보장하는 것은 "target 이하 keyframe 착지" 까지고, 거기서 target 까지
		//forward skip 하는 일은 호출측 몫이다(ffi_decoder.cpp:1320 주석의 source filter 역할).
		//이게 없으면 ±1 프레임 이동이 같은 keyframe 을 계속 돌려줘 화면이 전혀 바뀌지 않는다.
		//타이머 대기를 타지 않고 여기서 연속으로 버린다 — 한 프레임씩 대기하면 GOP 길이만큼 지연된다.
		double seek_target = m_seek_target_ms.load();

		if (seek_target >= 0.0)
		{
			//경계에서 반올림으로 한 프레임 더 버리지 않도록 반 프레임 여유를 둔다.
			double tolerance = (m_fps > 0.0) ? (500.0 / m_fps) : 0.0;
			int skipped = 0;

			while (frame != nullptr && frame_position_ms(frame) < seek_target - tolerance)
			{
				//pts 가 어긋난 파일에서 무한히 버리지 않도록 상한을 둔다.
				if (++skipped > seek_skip_limit)
					break;

				av_frame_free(&frame);
				frame = m_decoder->pop_video_frame();
			}

			//큐가 아직 안 찼으면 target 을 유지한 채 다음 틱에 이어서 버린다.
			if (frame == nullptr)
				continue;

			m_seek_target_ms = -1.0;
		}

		//20260811 by claude. ffi::CDecoder 는 best_effort_timestamp 를 채우지 않는다. 하드웨어 프레임을
		//CPU 로 옮길 때 sw_frame->pts = frame->pts 만 복사한다(ffi_decoder.cpp:1476, 1722).
		//그래서 best_effort_timestamp 만 보면 항상 AV_NOPTS_VALUE 라 재생 위치가 0 에서 멈춘다.
		//pts 를 우선 쓰고, 다른 디코더로 바뀌어도 동작하도록 best_effort 를 폴백으로 둔다.
		double frame_ms = frame_position_ms(frame);

		if (frame_ms >= 0.0)
			m_position_ms = frame_ms;

		bool converted = convert_to_bgra(frame);
		av_frame_free(&frame);

		if (!converted)
			continue;

		m_bgra_dirty = true;
		m_frame_count++;

		//한 장을 실제로 표시했으니 해제한다. seek 후 큐가 빌 동안은 위에서 continue 되어
		//플래그가 유지되므로, 프레임이 준비되는 즉시 정확히 한 장만 그려진다.
		if (render_one)
			m_render_one = false;

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

	//마우스로 잡고 있는 동안에는 재생 위치로 덮어쓰지 않는다. 서로 밀어내면 트랙이 튄다.
	//드래그 상태는 컨트롤이 이미 알고 있으므로 따로 들고 있지 않는다.
	if (m_slider.GetSafeHwnd() && !m_slider.is_lbutton_down() && m_duration_ms > 0.0)
		m_slider.set_pos((int)m_position_ms.load());

	//20260811 by claude. 트랙 추적용 진단 로그. 동작 확인 후 제거한다. 30 프레임마다 한 줄.
	if ((wParam % 30) == 0 && m_slider.GetSafeHwnd())
	{
		CRect rc_slider;
		m_slider.GetWindowRect(&rc_slider);
		ScreenToClient(&rc_slider);

		CRect rc_client;
		GetClientRect(&rc_client);

		D2D1_RECT_F image_rect = calc_image_rect();

		logWriteI(_T("[frame] frame=%d position=%.0f GetPos=%d lbutton=%d grabbed=%d | client=%dx%d slider=(%d,%d,%d,%d) image=(%.0f,%.0f,%.0f,%.0f)"),
			(int)wParam, m_position_ms.load(), m_slider.GetPos(),
			(int)m_slider.is_lbutton_down(), (int)m_drag_grabbed,
			rc_client.Width(), rc_client.Height(),
			rc_slider.left, rc_slider.top, rc_slider.right, rc_slider.bottom,
			image_rect.left, image_rect.top, image_rect.right, image_rect.bottom);
	}

	CWnd* parent = GetParent();
	if (parent && parent->GetSafeHwnd())
		parent->PostMessage(Message_CSCVideoWndD2, wParam, 0);

	return 0;
}

LRESULT CSCVideoWndD2::on_message_CSCSliderCtrl(WPARAM wParam, LPARAM)
{
	CSCSliderCtrlMsg* msg = (CSCSliderCtrlMsg*)wParam;

	if (msg == nullptr)
		return 0;

	//20260811 by claude. 트랙 조작 추적용 진단 로그. 동작 확인 후 제거한다.
	logWriteI(_T("[slider] msg=%d pos=%d grabbed=%d pending=%d lbutton=%d position=%.0f"),
		msg->msg, msg->pos, (int)m_drag_grabbed, m_drag_pending_pos,
		(int)m_slider.is_lbutton_down(), m_position_ms.load());

	//20260811 by claude. 조작 처리 방식은 Endorphin2 의 트랙(CControlDlg + CEndorphin2Dlg)을 따랐다.
	switch (msg->msg)
	{
	//grab 은 "잡았다" 가 아니라 PotPlayer 식으로 "클릭한 위치" 를 실어 온다(SCSliderCtrl.cpp:1186).
	//여기서 seek 하지 않으면 트랙 클릭이 통째로 무시된다.
	case CSCSliderCtrlMsg::msg_thumb_grab:
		m_drag_grabbed = true;
		seek_throttled(msg->pos);
		break;

	case CSCSliderCtrlMsg::msg_thumb_move:
		seek_throttled(msg->pos);
		break;

	//놓을 때 무조건 seek 하면 안 된다. throttle 이 이미 마지막 위치로 보냈고 그 뒤로 영상이 앞으로
	//흘렀는데 release 지점으로 다시 보내면 재생이 뒤로 되돌아간다.
	//- grab 없이 release 만 온 경우: 그 위치로 보낸다.
	//- 아직 못 보낸 위치가 남아 있으면: 그 위치로 보낸다.
	//- 둘 다 아니면 보내지 않는다.
	case CSCSliderCtrlMsg::msg_thumb_release:
		{
			KillTimer(timer_drag_throttle);

			int seek_pos = -1;

			if (!m_drag_grabbed)
				seek_pos = msg->pos;
			else if (m_drag_pending_pos >= 0)
				seek_pos = m_drag_pending_pos;

			if (seek_pos >= 0)
			{
				seek((double)seek_pos);
				m_drag_last_seek_tick = GetTickCount();
			}

			m_drag_pending_pos = -1;
			m_drag_grabbed = false;
		}
		break;
	}

	return 0;
}

void CSCVideoWndD2::seek_throttled(int pos_ms)
{
	DWORD now = GetTickCount();
	DWORD elapsed = now - m_drag_last_seek_tick;

	if (m_drag_last_seek_tick == 0 || elapsed >= drag_throttle_ms)
	{
		seek((double)pos_ms);
		m_drag_last_seek_tick = now;
		m_drag_pending_pos = -1;
		KillTimer(timer_drag_throttle);
	}
	else
	{
		//throttle 창 안이면 발화를 미룬다. 남은 시간 뒤에 마지막 위치 하나만 보낸다.
		m_drag_pending_pos = pos_ms;
		SetTimer(timer_drag_throttle, drag_throttle_ms - elapsed, NULL);
	}
}

void CSCVideoWndD2::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == timer_drag_throttle)
	{
		KillTimer(timer_drag_throttle);

		if (m_drag_pending_pos >= 0)
		{
			seek((double)m_drag_pending_pos);
			m_drag_last_seek_tick = GetTickCount();
			m_drag_pending_pos = -1;
		}

		return;
	}

	CWnd::OnTimer(nIDEvent);
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

		if (SUCCEEDED(m_d2context.handle_device_lost()) && m_width > 0 && m_height > 0 && create_frame_bitmap())
		{
			m_bgra_dirty = true;

			//20260809 by claude. 복구된 비트맵을 실제로 그릴 기회를 만든다. 재생 중이라면
			//다음 프레임에 그려지겠지만 일시정지·정지 이미지는 다음 프레임이 오지 않는다.
			Invalidate(FALSE);
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

	if (m_slider.GetSafeHwnd() && cx > 0 && cy > 0)
		m_slider.MoveWindow(0, cy - slider_height, cx, slider_height);

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
