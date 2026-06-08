#pragma once

/*
* ffi::CDecoder — async video decode engine.
*
*  Phase 2 산출물. open → start → seek/pop_frame → stop → close 의 lifecycle.
*  worker thread 가 demux + decode 를 background 에서 수행하고 frame queue (default 5) 에 push.
*  UI thread 는 pop_video_frame() 으로 디코드된 frame 을 받음 (queue 비면 nullptr).
*  seek 는 worker thread 에 비동기 위임 — UI thread 는 SetPositions 같은 동기 blocking 없음.
*
*  Phase 3 부터 audio + renderer 통합 예정.
*/

#include "ffmpeg_internal.h"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace ffi
{
	class CDecoder
	{
	public:
		CDecoder();
		~CDecoder();

		//file open + 비디오 stream 의 decoder context 준비. worker thread 는 아직 시작 안 됨.
		bool open(const wchar_t* utf16_path);

		//worker thread 종료 + format/codec context 해제.
		void close();

		//worker thread 시작. decoding 시작.
		void start();

		//worker thread 종료 (queue 의 frame 들 release).
		void stop();

		//seek 요청 — worker thread 에 비동기 위임. UI thread 는 즉시 반환.
		//worker 가 queue flush + av_seek_frame + codec flush 후 새 위치부터 디코드 재개.
		//segment_rt = graph 가 알린 seek target rt. worker 가 av_seek_frame 후 generation ++ 와 동시에 m_segment_rt 갱신.
		//prev_emit_ms = seek 직전 재생 위치 (keyframe 모드에서 forward/backward 판정 + 전진 보장). -1 = 모름.
		void seek(double pos_ms, int64_t segment_rt = 0, double prev_emit_ms = -1.0);

		//worker 가 seek 를 *처리 완료* 할 때까지 동기 대기 (queue drain + av_seek_frame + codec flush 끝남).
		//그 후 pop_video_frame 으로 받는 frame 은 모두 post-seek. timeout_ms 안에 처리되면 true.
		//정확한 seek latency 측정 / "seek 후 첫 frame" 확보용.
		bool wait_seek_done(int timeout_ms);

		//queue 에서 frame 1개 pop. 비면 nullptr. caller 가 av_frame_free 책임.
		AVFrame* pop_video_frame();
		AVFrame* pop_audio_frame();

		//info
		int		video_width()  const;
		int		video_height() const;
		double	duration_ms()  const;
		double	frame_rate()   const;		 //avg_frame_rate (fps). 0 이면 unknown.
		AVRational video_time_base() const;	 //stream 의 time_base. pts→ms 변환에 사용.
		int		video_pixel_format() const;	 //AVPixelFormat. AVFrame 의 format 과 동일 또는 codec 의 hw_pix_fmt.

		//display info — UI 표시용 (codec name string, fourcc, bit depth, bit rate, aspect ratio).
		std::wstring video_codec_name()		 const;	 //"HEVC" / "H264" / "VP9" 등 — avcodec_get_name() 결과.
		std::wstring video_fourcc()			 const;	 //codec_tag 4-char ("HVC1" / "AVC1" 등). 없으면 빈 문자열.
		int			 video_bit_depth()		 const;	 //PotPlayer 식 색 비트수 = component depth × component 수 (8-bit yuv420p → 24, 10-bit → 30).
		int64_t		 video_bit_rate()		 const;	 //bps. codecpar->bit_rate → MKV "BPS" 태그 → (전체 비트레이트 − 타 stream) 추정 순. 모두 실패 시 0.
		std::wstring video_aspect_ratio()	 const;	 //"16:9" / "1.85:1" 같은 표시용. sample_aspect_ratio + width/height 조합.
		std::wstring video_pixel_format_name() const;//"yuv420p" / "nv12" 등 — av_get_pix_fmt_name(). 없으면 빈 문자열.
		std::wstring video_chroma_location_name() const;//"left"/"center"/"topleft" 등 — av_chroma_location_name(). unspecified/없으면 빈 문자열.
		std::wstring video_hw_accel_name()	 const;	 //HW accel 사용 중이면 "D3D11VA"/"DXVA2"/"CUDA" 등, 아니면 빈 문자열.

		//audio info — Phase 4. has_audio() false 면 audio stream 없음 / 디코더 fail.
		bool	has_audio() const { return m_audio_stream_idx >= 0 && m_audio_ctx != nullptr; }
		int		audio_sample_rate() const;		 //Hz
		int		audio_channels()	const;
		int		audio_sample_format() const;	 //AVSampleFormat enum 값
		AVRational audio_time_base() const;

		std::wstring audio_codec_name()		 const;	 //"AAC" / "AC3" / "DTS" 등.
		int			 audio_bit_depth()		 const;	 //bits_per_coded_sample (0 = unknown).
		int64_t		 audio_bit_rate()		 const;	 //bps. 0 이면 unknown.
		std::wstring audio_channel_layout_name() const; //"stereo"/"5.1"/"7.1" 등 — av_channel_layout_describe(). 없으면 빈 문자열.

		//multi-audio track enumeration. open() 후 audio_track_count() > 1 이면 multi-track 미디어.
		//track_idx 는 0..count-1 의 dense index. AVFormatContext 의 stream index 와 다름 (audio 만 추림).
		int					audio_track_count() const { return (int)m_audio_stream_indices.size(); }
		const std::wstring& audio_track_name(int track_idx) const;
		int					audio_track_current() const { return m_audio_track_current; }

		//open() 전에 set_initial_audio_track(idx) 호출 시 av_find_best_stream 우회하고 그 track 으로 열기.
		//track switch 의 close+reopen 흐름에서 새 선택을 전달하는 채널. open() 이 한 번 consume 후 -1 로 reset.
		void				set_initial_audio_track(int track_idx) { m_initial_audio_track = track_idx; }

		//subtitle stream metadata enumeration. internal path 는 자막 디코딩/렌더링 미지원이라
		//이 API 는 메뉴 표시용 metadata 만 제공. 실제 선택은 LAV path 로 close+reopen 필요.
		int					subtitle_track_count() const { return (int)m_subtitle_track_names.size(); }
		const std::wstring& subtitle_track_name(int track_idx) const;

		bool	is_opened()	   const { return m_fmt != nullptr; }
		bool	is_running()   const { return m_thread.joinable(); }
		bool	has_hw_accel() const { return m_hw_pix_fmt != AV_PIX_FMT_NONE; }
		//영상 packet 에 timestamp(pts/dts)가 있는지 — open() 에서 첫 video packet probe 로 판정.
		//false(일부 AVI 등 no-PTS) 면 internal path 의 PTS 기반 A/V 동기·seek·컨트롤바가 동작 안 해 호출측이 LAV 로 라우팅.
		bool	video_has_pts() const { return m_video_has_pts; }
		//인덱스 없는 미종료 파일(헤더 duration 미상) — video pts/dts 가 비선형/엉터리(예: 121441 frame 인데 last dts=293223).
		//이 경우 video·audio rtStart 를 sample_count 기반으로(garbage pts 우회), 위치는 audio 실시간 pts, seek 는 byte 추정으로
		//전환해야 한다. open() 에서 m_fmt->duration==NOPTS 면 true. *정상 파일은 false → 기존 pts 기반 동작 그대로*.
		bool	unreliable_video_pts() const { return m_unreliable_video_pts; }
		bool	is_eof()	   const { return m_eof.load(); }	//av_read_frame AVERROR_EOF 도달 + queue 비면 stream 끝.

		//queue 의 현재 size — 디버깅용.
		size_t	video_queue_size();
		size_t	audio_queue_size();

		//queue 최대 깊이 (default video 5 / audio 50, PotPlayer 와 비슷).
		void	set_max_video_queue(int n) { m_max_queue = n; }
		void	set_max_audio_queue(int n) { m_max_audio_queue = n; }

		//CFFi*Stream 의 FillBuffer 가 매 frame pop 후 호출. atomic load — race-free segment baseline.
		int64_t segment_rt() const { return m_segment_rt.load(); }

		//video 의 seek 후 *첫 emit frame* 의 pts_rt. audio FillBuffer 가 *anchor* 로 사용 → 그 이전 audio frame skip.
		//이유: MPEG-TS 등에서 av_seek_frame BACKWARD 가 target +3초 forward keyframe 으로 fallback 점프. video 첫 frame 의
		//미디어 시점 ≠ audio 첫 frame 의 미디어 시점. 둘 다 segment-local 0 시점에 시작하면 *영구 audio drift*.
		//audio 가 video anchor 까지 frame skip → 두 stream 이 *같은 미디어 시점* 부터 emit.
		int64_t video_first_emit_pts_rt() const { return m_video_first_emit_pts_rt.load(); }

		//CFFiVideoStream::FillBuffer 가 *pre-target skip 후 실제 emit 한* 첫 frame 의 pts_rt 를 set.
		//worker 의 첫 push frame (keyframe) 과 다를 수 있어 emit 시점 기준 필요.
		void	set_video_first_emit_pts_rt(int64_t pts_rt) { m_video_first_emit_pts_rt.store(pts_rt); }

		//[seek_keyframe_mode] CFFiSource 의 옵션을 worker 에 전달.
		//true: av_seek_frame flag=0 (forward keyframe = target 직후) + margin 없음.
		//false: 기존 동작 (AVSEEK_FLAG_BACKWARD + -1초 margin).
		void	set_seek_keyframe_mode(bool seek_keyframe) { m_seek_keyframe_mode.store(seek_keyframe); }

	private:
		void	worker_loop();

		//avformat 기본 file protocol 대신 custom AVIOContext + Win32 CreateFile (FILE_SHARE_DELETE 포함) 사용 —
		//재생 중 미디어 파일이 외부에서 rename / move 되더라도 source 가 동일 handle 로 계속 read.
		//_wsopen 기반 avformat 기본 path 는 FILE_SHARE_DELETE 미설정 → MoveFile 시 sharing violation.
		static int		avio_read_cb(void* opaque, uint8_t* buf, int buf_size);
		static int64_t	avio_seek_cb(void* opaque, int64_t offset, int whence);

		//HANDLE 의 실 타입 = void*. 헤더에서 windows.h 미포함 (MFC winsock2 충돌 회피).
		//cpp 내에서만 CreateFile/ReadFile/CloseHandle 등 windows API 호출.
		//valid handle 만 저장 — CreateFile 의 INVALID_HANDLE_VALUE 결과는 nullptr 로 normalize.
		void*				m_file_handle = nullptr;
		AVIOContext*		m_avio = nullptr;

		AVFormatContext*	m_fmt = nullptr;
		AVCodecContext*		m_video_ctx = nullptr;
		int					m_video_stream_idx = -1;

		//HW 가속 — D3D11VA / DXVA2. SW keyframe walk decode 의 5-10초 freeze 회피.
		AVBufferRef*		m_hw_device_ctx = nullptr;
		AVPixelFormat		m_hw_pix_fmt = AV_PIX_FMT_NONE;	  //HW frame format (codec 의 get_format 반환).

		bool				m_video_has_pts = true;	  //open() 의 첫 video packet probe 결과. false 면 호출측이 LAV 로 라우팅.

		//audio decode
		AVCodecContext*		m_audio_ctx = nullptr;
		int					m_audio_stream_idx = -1;

		//multi-audio track enumeration — open() 에서 채워짐.
		//m_audio_stream_indices[track_idx] = AVFormatContext::streams[] 의 stream index.
		std::vector<int>		 m_audio_stream_indices;
		std::vector<std::wstring> m_audio_track_names;
		int						 m_audio_track_current = -1;   //선택된 track index (audio_stream_indices 의 인덱스).
		int						 m_initial_audio_track = -1;   //open() 호출 전 set_initial_audio_track 으로 지정. consume 후 -1.

		//subtitle stream metadata — open() 에서 채워짐. internal path 는 디코딩/렌더링 미지원, 메뉴 표시용.
		std::vector<std::wstring> m_subtitle_track_names;

		std::thread				m_thread;
		std::atomic<bool>		m_quit{false};

		//백그라운드 duration 스캔 — 헤더에 길이가 없는 미종료 녹화 파일(AVI 등)용.
		//open() 에서 m_fmt->duration 이 미상이면 worker 를 띄워 전체 packet 을 *디코드 없이* 읽어
		//마지막 video timestamp 로 총 길이를 산출, m_scanned_duration_ms 에 atomic store. duration_ms() 가 우선 반환.
		//정상 파일(헤더에 duration 있음)은 스캔 안 띄움 → 오버헤드 0.
		std::wstring			m_path;					//open() 의 파일 경로 (스캐너가 2nd context 로 다시 열 때 사용).
		std::thread				m_scan_thread;
		std::atomic<bool>		m_scan_quit{false};
		std::atomic<double>		m_scanned_duration_ms{-1.0};	//< 0 = 미산출. > 0 = 산출된 총 길이(ms).
		void					scan_duration_worker();

		//video pts 가 비선형/엉터리라 sample_count 타이밍 + audio 위치 + byte seek 로 전환할지. open 에서 set, worker/pin 이 read.
		//set-once (worker 시작 전 open). 정상 파일은 false → 모든 기존 경로 그대로 (정상 미디어 회귀 방지).
		bool					m_unreliable_video_pts = false;

		//[seek 인덱스] 인덱스 없는 미종료 파일의 byte-seek 정확도용. 스캔(scan_duration_worker)이 (audio시간 ms, 파일 byte 위치)
		//샘플을 ~수초 간격으로 기록. byte-seek 시 시간→byte 를 보간해 *콘텐츠가 차지한 실제 byte 영역* 으로 이동 (linear×filesize
		//는 garbage tail/비균일 bitrate 때문에 overshoot → EOF). 스캔 thread 가 write, worker thread 가 read → mutex 보호.
		std::vector<std::pair<int64_t, int64_t>> m_seek_index;	//(ms, byte). ms 오름차순.
		std::mutex				m_seek_index_mtx;
		//true = 인덱스가 *keyframe* byte (video-only 미종료 파일). byte_for_time_ms 가 보간 대신 target 이하 keyframe 으로 snap.
		//(byte-seek 후 디코드는 keyframe 부터만 가능 → 비keyframe byte 보간은 무의미. audio 인덱스는 false = 보간.)
		bool					m_seek_index_snap = false;
		//시간(ms)에 대응하는 파일 byte 위치를 인덱스에서 보간. 인덱스 비었으면 -1 (호출측이 linear fallback). ms 가 끝 너머면 마지막 byte 로 clamp.
		int64_t					byte_for_time_ms(double ms);

		//seek 요청 — worker 가 다음 iteration 에서 pickup.
		std::mutex				m_mtx_seek;
		std::condition_variable m_cv_seek_done;
		double					m_pending_seek_ms = -1.0;
		double					m_pending_prev_ms = -1.0;  //seek 직전 재생 위치 (keyframe 모드 방향 판정 + 전진 보장용).
		//seek 캡처 시점(seek() 호출 = UI thread, put_CurrentPosition 내부 동기 구간)에 m_seek_keyframe_mode 스냅샷.
		//worker 가 live atomic 을 읽으면, frame step 의 "off→seek→on" 우회가 worker 처리 전에 on 으로 복원돼 race.
		//스냅샷이면 toggle 이 동기 구간만 감싸면 되므로 그 seek 은 결정적으로 캡처된 모드로 처리됨.
		bool					m_pending_kf_mode = true;
		bool					m_seek_processed = true;   //worker 가 seek 끝낸 직후 true. seek() 호출 시 false.

		//Seek generation — frame 마다 tag 부여. seek() 호출 시 ++. pop 시 current generation 만 accept,
		//이전 generation 의 stale frame 은 skip + free. caller flush 와 worker push 의 모든 race 차단.
		std::atomic<int>		m_seek_generation{0};

		//EOF — av_read_frame AVERROR_EOF 도달 시 set, seek 시 clear. FillBuffer 가 EOS 인지 vs 일시 starve 구분.
		std::atomic<bool>		m_eof{false};

		//Segment baseline rt — worker 의 av_seek_frame 처리 후 generation ++ 와 동시에 갱신.
		//FillBuffer 의 rt 계산이 generation tag 와 같은 시점의 segment 사용 → race 없음.
		//CFFiVideoStream / CFFiAudioStream 이 직접 m_segment_rt.load() 접근 가능하도록 public-like accessor 별도 제공.
		std::atomic<int64_t>   m_segment_rt{0};
		int64_t				   m_pending_segment_rt = 0;   //seek() 호출 시 저장. worker 가 av_seek_frame 후 m_segment_rt.store(this).

		//seek 후 *첫 video frame emit* 의 pts_rt — audio FillBuffer 의 anchor.
		//av_seek_frame 후 reset (LLONG_MIN). worker 가 첫 video frame queue push 시 set.
		std::atomic<int64_t>   m_video_first_emit_pts_rt{LLONG_MIN};

		//seek → first frame 시간 측정용.
		bool				   m_first_frame_after_seek = true;
		int64_t				   m_seek_done_qpc = 0;

		//[seek_keyframe_mode] true 면 worker 가 BACKWARD seek 후 forward keyframe (pts>=target) 까지
		//packet 을 *디코드 없이* demux-skip → GOP 디코드 비용 제거 + 항상 target 이상 (절대 안 되돌아옴).
		//UI thread 가 set, worker 가 read.
		std::atomic<bool>		m_seek_keyframe_mode{true};

		//worker 전용 (seek 처리 / decode 루프 모두 worker thread) — keyframe skip 진행 상태.
		//forward seek 일 때만 active. m_kf_skip_min_pts 보다 큰 첫 video keyframe 만날 때까지 packet unref.
		//(min_pts = seek 직전 위치 prev → 결과 keyframe 이 prev 보다 항상 전진 = 제자리 회귀 방지.)
		bool					m_kf_skip_active = false;
		int64_t					m_kf_skip_min_pts	 = 0;	 //video time_base. 이 값 초과 keyframe 부터 디코드.
		int64_t					m_kf_skip_target_pts = 0;	 //EOF fallback (target 으로 backward 재seek) 용.
		//손상 구간 무한 skip 방지 — seek 후 이 개수만큼 video packet 을 skip 해도 유효 keyframe 을 못 만나면
		//(garbage 라 EOF 도 안 나고 keyframe 도 없음) skip 을 포기하고 정상 디코드로 전환 → 워커 hang/ UI freeze 차단.
		int64_t					m_kf_skip_count = 0;
		static const int64_t	kf_skip_limit = 3000;	//정상 GOP(키프레임 간격)보다 충분히 큼. 넘으면 garbage 로 판단.
		//kf_skip 로 손상 구간을 건너뛰는 중 *현재 스캔 위치(ms)*. 위치 query 가 이 값으로 트랙바·시간을 전진시켜
		//"복구 지점까지 빠르게 찾아가는" 모습을 보인다(멈춰 보여 freeze 로 오인되는 것 방지). 미스캔 시 -1.
		std::atomic<int64_t>	m_scan_pos_ms{ -1 };
		//seek 후 첫 video frame 디코드 전까지 true — 이 동안 매 video packet 시각을 m_scan_pos_ms 로 발행(손상 구간 통과 포함).
		//첫 frame 디코드 시 false → 정상 재생 땐 read-ahead packet 으로 트랙바가 오염되지 않음.
		std::atomic<bool>		m_pos_searching{ false };
	public:
		int64_t					kf_skip_scan_pos_ms() const { return m_scan_pos_ms.load(std::memory_order_relaxed); }
		//seek 후 첫 video frame 디코드 전(손상 구간 스캔 포함). audio pin 이 이 동안 video 복구를 대기(garbage 오디오 미전달).
		bool					is_searching() const { return m_pos_searching.load(std::memory_order_relaxed); }
	private:
		//연속 video packet 디코드가 frame 을 못 내는 횟수. frame 받으면 0. 정상은 매 packet 마다 frame → 작게 유지.
		//일정 임계 초과(손상 keyframe 디코드 후 stall 등)면 searching 재진입 → 스캔으로 정상 구간 탐색(시작부 손상 케이스).
		int						m_video_no_frame_count = 0;
		static const int		video_stall_limit = 60;
		//이번 seek 의 스캔이 손상 구간을 거쳤는지(bound bail 또는 stall 재진입). 복구 시 audio decoder 재생성 트리거. seek 마다 리셋.
		bool					m_did_garbage_scan = false;

		//frame queue
		std::mutex				m_mtx_queue;
		std::condition_variable m_cv_queue;		//worker 가 queue 비울 때 ↔ UI 가 pop 할 때 양방향 wake.
		std::deque<AVFrame*>	m_video_queue;
		std::deque<AVFrame*>	m_audio_queue;
		int						m_max_queue = 30;		//seek 후 큐 빨리 채워지도록 5→30. FillBuffer wait 시간 단축.
		int						m_max_audio_queue = 100;
	};
}
