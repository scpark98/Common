#pragma once

/*
* CSCVideoWndD2 — Direct2D 비디오 재생 윈도우.
*
* 기존 CVideoWnd 대비 달라진 점:
* - 디코딩: OpenCV VideoCapture → ffi::CDecoder (FFmpeg + D3D11VA/DXVA2 하드웨어 디코딩, 워커 스레드)
* - 렌더링: GDI StretchDIBits(HALFTONE, CPU 리샘플) → D2D DrawBitmap (GPU 샘플러)
*           창을 키워도 비용이 거의 늘지 않는다. GDI 는 목적지 픽셀 수에 비례해 CPU 가 소모된다.
* - 프레임 페이싱: WM_TIMER → 고해상도 waitable timer 를 쓰는 pacer 스레드
*           WM_TIMER 는 해상도 15.6ms 에 큐가 빌 때만 합성되는 최저 우선순위 메시지라 안정적 페이싱이 불가능하다.
*
* [사용법]
*   //대화상자 멤버
*   CSCVideoWndD2 m_video;
*
*   //OnInitDialog — 리소스의 static 컨트롤 위치에 붙이거나 직접 Create
*   m_video.Create(NULL, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN, rect, this, id);
*   m_video.set_overlay_callback([this](ID2D1DeviceContext* d2dc, const D2D1_RECT_F& r) { ... });
*   m_video.open(_T("d:\\sample.mp4"));
*
*   //검출 등 프레임이 필요한 작업은 별도 스레드에서
*   cv::Mat bgra;
*   if (m_video.get_frame(bgra)) { ... }
*
* [의존]
*   Common/directx/CSCD2Context/SCD2Context.{h,cpp}
*   Common/ffmpeg/internal/ffmpeg_internal.{h,cpp}, ffi_decoder.{h,cpp}
*   Common/ffmpeg/include, Common/ffmpeg/lib, 실행 폴더에 av*.dll / sw*.dll
*   OpenCV (프레임 전달 타입으로 cv::Mat 사용)
*/

#include <afxwin.h>
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

#include <opencv2/opencv.hpp>

#include "../CSCD2Context/SCD2Context.h"

//ffmpeg 헤더를 이 헤더에 노출하지 않기 위한 전방 선언.
struct AVFrame;
struct SwsContext;

namespace ffi { class CDecoder; }

//프레임을 한 장 렌더링할 때마다 부모에게 통지. wParam = 재생된 프레임 수.
#define Message_CSCVideoWndD2		(WM_APP + 3200)

class CSCVideoWndD2 : public CWnd
{
	DECLARE_DYNAMIC(CSCVideoWndD2)

public:
	CSCVideoWndD2();
	virtual ~CSCVideoWndD2();

	//프레임을 그린 직후 호출된다. image_rect 는 프레임이 실제로 그려진 화면 영역(DIP).
	//원본 좌표 → 화면 좌표 변환에 쓴다. UI 스레드에서 호출된다.
	typedef std::function<void(ID2D1DeviceContext* d2dc, const D2D1_RECT_F& image_rect)> overlay_func;
	void			set_overlay_callback(overlay_func func) { m_overlay = func; }

	//더블클릭 콜백. point 는 클라이언트 좌표, image_rect 는 프레임이 그려진 영역(DIP).
	//true 를 반환하면 그것으로 끝내고, false 면 기본 동작(재생/일시정지 토글)을 수행한다.
	//"얼굴을 더블클릭하면 등록, 빈 곳이면 토글" 같은 분기를 호출측이 결정할 수 있게 한다.
	typedef std::function<bool(CPoint point, const D2D1_RECT_F& image_rect)> dblclick_func;
	void			set_dblclick_callback(dblclick_func func) { m_dblclick = func; }

	//현재 프레임이 그려진 화면 영역(DIP). 원본 좌표 ↔ 화면 좌표 변환에 쓴다.
	D2D1_RECT_F		get_image_rect() { return calc_image_rect(); }

	//start_ms > 0 이면 그 위치부터 재생을 시작한다.
	bool			open(CString path, double start_ms = 0.0);
	void			close();
	bool			is_opened() const { return m_decoder != nullptr; }

	//비동기 seek. UI 스레드를 막지 않는다. 직전 프레임과 검출 결과는 무효가 되므로 호출측이 정리한다.
	void			seek(double pos_ms);

	void			play();
	void			pause();
	void			toggle_play();
	bool			is_playing() const { return m_playing.load(); }

	//현재 재생 위치(ms). 프레임의 pts 기준.
	double			get_position_ms() const { return m_position_ms.load(); }

	//끝까지 재생하면 처음으로 되돌린다. default = true.
	void			set_repeat(bool repeat) { m_repeat = repeat; }
	bool			get_repeat() const { return m_repeat.load(); }

	//최근 디코드된 프레임의 BGRA 사본. 워커 스레드에서 호출해도 된다. 프레임이 없으면 false.
	bool			get_frame(cv::Mat &bgra);

	//BGRA 사본을 만들지 않고 곧바로 3채널 BGR 로 변환해 받는다.
	//get_frame() 은 8MB 복사 후 다시 변환하지만 이쪽은 변환 한 번으로 끝난다.
	//대부분의 추론 모델이 BGR 입력이라 실질적으로 이쪽을 쓰게 된다.
	bool			get_frame_bgr(cv::Mat &bgr);

	int				get_video_width()  const { return m_width; }
	int				get_video_height() const { return m_height; }
	double			get_video_fps()	   const { return m_fps; }
	double			get_duration_ms()  const { return m_duration_ms; }

	//실측치. play_fps = 실제 프레임 표시율, render_ms = 마지막 렌더 1회 소요시간.
	double			get_play_fps()	 const { return m_play_fps.load(); }
	double			get_render_ms()	 const { return m_render_ms.load(); }
	int				get_frame_count() const { return m_frame_count.load(); }

	//"D3D11VA" / "DXVA2" / "CUDA". 하드웨어 디코딩을 못 쓰면 빈 문자열.
	CString			get_hw_accel_name() const { return m_hw_accel; }

	//D2D 디바이스가 새로 만들어질 때마다 증가한다. overlay 콜백이 브러시 같은
	//디바이스 의존 자원을 캐시한다면 이 값이 바뀌었을 때 다시 만들어야 한다.
	int				get_device_generation() const { return m_device_generation; }

	void			set_back_color(Gdiplus::Color color) { m_cr_back = color; }

protected:
	afx_msg void	OnPaint();
	afx_msg void	OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL	OnEraseBkgnd(CDC* pDC);
	afx_msg void	OnDestroy();
	afx_msg void	OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg LRESULT on_message_CSCVideoWndD2(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

	virtual void	PreSubclassWindow();

private:
	bool			init_d2d();
	void			pacer_thread_proc();

	//디코드된 AVFrame(대개 NV12) → BGRA. sws_getCachedContext 라 포맷이 바뀌어도 자동 대응.
	bool			convert_to_bgra(AVFrame* frame);

	void			render();
	D2D1_RECT_F		calc_image_rect();

	CSCD2Context			m_d2context;
	ComPtr<ID2D1Bitmap1>	m_bitmap;		//동영상 해상도 크기의 스트리밍 텍스처. 한 번만 만들고 픽셀만 교체.
	bool					m_d2d_ready = false;
	int						m_device_generation = 0;

	std::unique_ptr<ffi::CDecoder>	m_decoder;
	SwsContext*				m_sws = nullptr;

	int						m_width = 0;
	int						m_height = 0;
	double					m_fps = 0.0;
	double					m_duration_ms = 0.0;
	CString					m_hw_accel;

	//pacer 스레드가 write, UI 스레드가 read(업로드). 30fps 기준 경합 시간이 짧아 mutex 로 충분하다.
	cv::Mat					m_bgra;
	std::mutex				m_bgra_mutex;
	std::atomic<bool>		m_bgra_dirty{ false };	//UI 스레드가 아직 업로드 안 한 새 프레임이 있는지.

	std::thread				m_pacer_thread;
	std::atomic<bool>		m_thread_stop{ true };
	std::atomic<bool>		m_playing{ false };
	std::atomic<bool>		m_repeat{ true };

	//UI 스레드가 밀릴 때 렌더 메시지가 큐에 쌓이지 않도록 하는 게이트.
	std::atomic<bool>		m_render_pending{ false };

	std::atomic<double>		m_position_ms{ 0.0 };
	std::atomic<double>		m_play_fps{ 0.0 };
	std::atomic<double>		m_render_ms{ 0.0 };
	std::atomic<int>		m_frame_count{ 0 };

	overlay_func			m_overlay;
	dblclick_func			m_dblclick;
	Gdiplus::Color			m_cr_back = Gdiplus::Color(255, 24, 24, 24);
};
