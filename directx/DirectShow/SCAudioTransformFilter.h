#pragma once

//CSCAudioTransformFilter — DirectShow audio in-place transform filter base.
//
//설계 목적
//  ① BaseClasses (strmbase) 의존 없이 동작 (XP SP3 호환).
//  ② Audio gain / normalize / EQ / fade 등 PCM sample 단위 DSP 의 공용 base.
//  ③ In-place transform — input pin 의 IMemAllocator 를 output 도 공유. sample buffer 직접 modify.
//
//사용
//  파생 클래스가 process_sample() override 하여 buf[len] 의 PCM sample 을 in-place 수정한다.
//  WAVEFORMATEX 로 sample format (16-bit PCM / 32-bit PCM / 32-bit float / channel 수 등) 식별.
//
//그래프 통합
//  CoCreateInstance 등록 안 된 자체 객체이므로 IGraphBuilder::AddFilter 후 ConnectDirect 로 양 핀 연결.

#include <Afxwin.h>
#include <dshow.h>
#include <mmreg.h>	//WAVEFORMATEXTENSIBLE
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>

class CSCAudioTransformInputPin;
class CSCAudioTransformOutputPin;

//IMediaSeeking pass-through — graph IMediaSeeking::SetRate 가 chain 의 모든 filter 의
//IMediaSeeking::SetRate 호출 시 우리 transform filter 도 응답해야 NewSegment 가 결정적으로
//chain 따라 propagate. 자체 state 는 없고 upstream connected pin 의 IMediaSeeking 으로 모두 위임.
class CSCAudioTransformFilter : public IBaseFilter, public IMediaSeeking
{
public:
	CSCAudioTransformFilter(LPCWSTR friendly_name);
	virtual ~CSCAudioTransformFilter();

	//파생이 override — buf 의 sample 을 in-place 수정.
	//buf : sample buffer 시작 주소
	//len : buffer 의 byte 길이
	//wfx : sample format (cached from connection negotiation)
	virtual void	process_sample(BYTE* buf, long len, const WAVEFORMATEX* wfx) = 0;

	//audio delay (ms 단위, +양수 = audio 늦게/지연, -음수 = audio 빠르게).
	//Receive 시점에 IMediaSample 의 start/stop time 을 shift 한다 — base 에서 일괄 처리.
	//graph 의 audio renderer 가 DirectSound 처럼 IExFilterConfig 미지원이어도 동작.
	void			set_delay_ms(int ms) { m_delay_100ns.store((LONGLONG)ms * 10000LL); }
	int				get_delay_ms() const { return (int)(m_delay_100ns.load() / 10000LL); }
	LONGLONG		get_delay_ns()  const { return m_delay_100ns.load(); }	//Receive 가 timestamp shift 시 호출

//IUnknown
	STDMETHODIMP			QueryInterface(REFIID iid, void** ppv) override;
	STDMETHODIMP_(ULONG)	AddRef() override;
	STDMETHODIMP_(ULONG)	Release() override;

//IPersist
	STDMETHODIMP	GetClassID(CLSID* pClsID) override;

//IMediaFilter
	STDMETHODIMP	Stop() override;
	STDMETHODIMP	Pause() override;
	STDMETHODIMP	Run(REFERENCE_TIME tStart) override;
	STDMETHODIMP	GetState(DWORD dwMSecs, FILTER_STATE* pState) override;
	STDMETHODIMP	SetSyncSource(IReferenceClock* pClock) override;
	STDMETHODIMP	GetSyncSource(IReferenceClock** ppClock) override;

//IBaseFilter
	STDMETHODIMP	EnumPins(IEnumPins** ppEnum) override;
	STDMETHODIMP	FindPin(LPCWSTR Id, IPin** ppPin) override;
	STDMETHODIMP	QueryFilterInfo(FILTER_INFO* pInfo) override;
	STDMETHODIMP	JoinFilterGraph(IFilterGraph* pGraph, LPCWSTR pName) override;
	STDMETHODIMP	QueryVendorInfo(LPWSTR* pVendorInfo) override;

//IMediaSeeking — upstream connected pin / filter 의 IMediaSeeking 으로 모두 위임.
//transform filter 가 seeking state 를 자체 보유하지 않으므로 pass-through 만이 안전.
	STDMETHODIMP	GetCapabilities(DWORD* pCapabilities) override;
	STDMETHODIMP	CheckCapabilities(DWORD* pCapabilities) override;
	STDMETHODIMP	IsFormatSupported(const GUID* pFormat) override;
	STDMETHODIMP	QueryPreferredFormat(GUID* pFormat) override;
	STDMETHODIMP	GetTimeFormat(GUID* pFormat) override;
	STDMETHODIMP	IsUsingTimeFormat(const GUID* pFormat) override;
	STDMETHODIMP	SetTimeFormat(const GUID* pFormat) override;
	STDMETHODIMP	GetDuration(LONGLONG* pDuration) override;
	STDMETHODIMP	GetStopPosition(LONGLONG* pStop) override;
	STDMETHODIMP	GetCurrentPosition(LONGLONG* pCurrent) override;
	STDMETHODIMP	ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat,
		LONGLONG Source, const GUID* pSourceFormat) override;
	STDMETHODIMP	SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags,
		LONGLONG* pStop, DWORD dwStopFlags) override;
	STDMETHODIMP	GetPositions(LONGLONG* pCurrent, LONGLONG* pStop) override;
	STDMETHODIMP	GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest) override;
	STDMETHODIMP	SetRate(double dRate) override;
	STDMETHODIMP	GetRate(double* pdRate) override;
	STDMETHODIMP	GetPreroll(LONGLONG* pllPreroll) override;

//Internal — pin 에서 호출.
	IFilterGraph*			get_graph() { return m_pGraph; }
	FILTER_STATE			get_state() { return m_state; }
	bool					is_flushing() { return m_flushing.load(); }
	const WAVEFORMATEX*		get_wave_format() { return m_wfx_set ? (const WAVEFORMATEX*)&m_wfx_ext : NULL; }
	//WAVEFORMATEXTENSIBLE 의 SubFormat 을 포함해 전체 cbFmt byte 까지 복사. WFEX subtype 검사를 위해 필수.
	void					set_wave_format(const BYTE* fmt, ULONG cbFmt);
	void					clear_wave_format() { m_wfx_set = false; }
	CSCAudioTransformInputPin*	get_input_pin()  { return m_pInputPin; }
	CSCAudioTransformOutputPin* get_output_pin() { return m_pOutputPin; }

//Async worker — Receive 가 sample 을 queue 에 push 후 즉시 return,
//worker thread 가 dequeue → process_sample → deliver_sample. LAV emit thread 와 DSound
//consume thread 의 직렬 chain 을 분리 → graph rate 변경 시 audio renderer 가 rate 적용 보장.
	//Queue 에 sample 을 push. sample 은 AddRef 된 상태로 전달 — worker 가 dequeue 후 Release.
	void					enqueue_sample(IMediaSample* pSample);
	//NewSegment 도 sample 흐름과 같은 순서로 처리되어야 — queue 에 sentinel 로 enqueue.
	void					enqueue_new_segment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
	//EOS / Flush 등 흐름 제어.
	void					enqueue_end_of_stream();
	void					begin_flush();	//queue clear + worker drain
	void					end_flush();
	//worker thread 시작/종료. Run 시 start, Stop / destructor 에서 stop.
	void					start_worker();
	void					stop_worker();

protected:
	std::atomic<LONG>		m_ref;
	FILTER_STATE			m_state;
	IFilterGraph*			m_pGraph;	//weak (DirectShow convention)
	IReferenceClock*		m_pClock;
	WCHAR					m_name[64];

	CSCAudioTransformInputPin*	m_pInputPin;
	CSCAudioTransformOutputPin*	m_pOutputPin;

	WAVEFORMATEXTENSIBLE	m_wfx_ext;	//40 byte 까지 저장. WAVEFORMATEX 만 와도 18 byte 만 채움.
	bool					m_wfx_set;

	std::atomic<LONGLONG>	m_delay_100ns;	//timestamp shift (100-ns units, REFERENCE_TIME 단위)

	//Async worker queue.
	//queue 의 element 는 sample (or sentinel 표시). sentinel 은 NewSegment / EOS / Flush mark.
	enum work_kind { kind_sample, kind_new_segment, kind_eos };
	struct work_item
	{
		work_kind		kind;
		IMediaSample*	sample;		//kind_sample 시 AddRef 된 상태로 보유
		REFERENCE_TIME	ns_start;	//kind_new_segment 인자
		REFERENCE_TIME	ns_stop;
		double			ns_rate;
	};
	std::thread					m_worker;
	std::mutex					m_queue_mutex;
	std::condition_variable		m_queue_cv;
	std::deque<work_item>		m_queue;
	std::atomic<bool>			m_worker_run;	//worker loop 종료 신호
	std::atomic<bool>			m_flushing;		//BeginFlush ↔ EndFlush 사이엔 queue 비우고 신규 sample drop
	void					worker_main();
	//virtual — variable-length output 이 필요한 파생 (예: CSCAudioTimeStretch) 이 override.
	//default impl: in-place process_sample 호출 후 input sample 그대로 deliver. delay shift 도 처리.
	//override 시 output sample 새로 할당 + 자체 deliver. 기존 in-place 파생 (Gain/Compressor 등) 은 override 안 함 → default 동작 유지.
	virtual void			process_one(const work_item& w);
	void					clear_queue();		//queue clear + sample release
};


//Input pin — splitter/upstream 으로부터 sample 을 받는다.
class CSCAudioTransformInputPin : public IPin, public IMemInputPin
{
public:
	CSCAudioTransformInputPin(CSCAudioTransformFilter* pFilter);
	virtual ~CSCAudioTransformInputPin();

//IUnknown
	STDMETHODIMP			QueryInterface(REFIID iid, void** ppv) override;
	STDMETHODIMP_(ULONG)	AddRef() override;
	STDMETHODIMP_(ULONG)	Release() override;

//IPin
	STDMETHODIMP	Connect(IPin* pReceivePin, const AM_MEDIA_TYPE* pmt) override;
	STDMETHODIMP	ReceiveConnection(IPin* pConnector, const AM_MEDIA_TYPE* pmt) override;
	STDMETHODIMP	Disconnect() override;
	STDMETHODIMP	ConnectedTo(IPin** ppPin) override;
	STDMETHODIMP	ConnectionMediaType(AM_MEDIA_TYPE* pmt) override;
	STDMETHODIMP	QueryPinInfo(PIN_INFO* pInfo) override;
	STDMETHODIMP	QueryDirection(PIN_DIRECTION* pPinDir) override;
	STDMETHODIMP	QueryId(LPWSTR* Id) override;
	STDMETHODIMP	QueryAccept(const AM_MEDIA_TYPE* pmt) override;
	STDMETHODIMP	EnumMediaTypes(IEnumMediaTypes** ppEnum) override;
	STDMETHODIMP	QueryInternalConnections(IPin** apPin, ULONG* nPin) override;
	STDMETHODIMP	EndOfStream() override;
	STDMETHODIMP	BeginFlush() override;
	STDMETHODIMP	EndFlush() override;
	STDMETHODIMP	NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate) override;

//IMemInputPin
	STDMETHODIMP	GetAllocator(IMemAllocator** ppAllocator) override;
	STDMETHODIMP	NotifyAllocator(IMemAllocator* pAllocator, BOOL bReadOnly) override;
	STDMETHODIMP	GetAllocatorRequirements(ALLOCATOR_PROPERTIES* pProps) override;
	STDMETHODIMP	Receive(IMediaSample* pSample) override;
	STDMETHODIMP	ReceiveMultiple(IMediaSample** pSamples, long nSamples, long* nSamplesProcessed) override;
	STDMETHODIMP	ReceiveCanBlock() override;

	IMemAllocator*	get_allocator() { return m_pAllocator; }

	//async path 용 — Receive 가 upstream sample 을 자체 allocator 의 새 sample 으로 복사 후
	//queue 에 push. 자체 allocator 가 충분한 buffer pool 제공 (LAV ↔ DSound 분리).
	IMemAllocator*	get_or_create_internal_allocator();

protected:
	std::atomic<LONG>			m_ref;
	CSCAudioTransformFilter*	m_pFilter;	//weak — filter outlives pin
	IPin*						m_pConnected;
	AM_MEDIA_TYPE				m_mt;
	bool						m_mt_set;
	IMemAllocator*				m_pAllocator;
	IMemAllocator*				m_pInternalAllocator;	//async copy 용. lazy create.

	void	free_media_type();
};


//Output pin — downstream (audio renderer 등) 으로 transform 결과를 전달.
//In-place transform 이라 input pin 의 sample 을 그대로 downstream 에게 forward.
class CSCAudioTransformOutputPin : public IPin
{
public:
	CSCAudioTransformOutputPin(CSCAudioTransformFilter* pFilter);
	virtual ~CSCAudioTransformOutputPin();

//IUnknown
	STDMETHODIMP			QueryInterface(REFIID iid, void** ppv) override;
	STDMETHODIMP_(ULONG)	AddRef() override;
	STDMETHODIMP_(ULONG)	Release() override;

//IPin
	STDMETHODIMP	Connect(IPin* pReceivePin, const AM_MEDIA_TYPE* pmt) override;
	STDMETHODIMP	ReceiveConnection(IPin* pConnector, const AM_MEDIA_TYPE* pmt) override;
	STDMETHODIMP	Disconnect() override;
	STDMETHODIMP	ConnectedTo(IPin** ppPin) override;
	STDMETHODIMP	ConnectionMediaType(AM_MEDIA_TYPE* pmt) override;
	STDMETHODIMP	QueryPinInfo(PIN_INFO* pInfo) override;
	STDMETHODIMP	QueryDirection(PIN_DIRECTION* pPinDir) override;
	STDMETHODIMP	QueryId(LPWSTR* Id) override;
	STDMETHODIMP	QueryAccept(const AM_MEDIA_TYPE* pmt) override;
	STDMETHODIMP	EnumMediaTypes(IEnumMediaTypes** ppEnum) override;
	STDMETHODIMP	QueryInternalConnections(IPin** apPin, ULONG* nPin) override;
	STDMETHODIMP	EndOfStream() override;
	STDMETHODIMP	BeginFlush() override;
	STDMETHODIMP	EndFlush() override;
	STDMETHODIMP	NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate) override;

	//transform filter 가 input sample 을 처리한 후 호출 — downstream 으로 forward.
	HRESULT			deliver_sample(IMediaSample* pSample);
	HRESULT			deliver_end_of_stream();
	HRESULT			deliver_begin_flush();
	HRESULT			deliver_end_flush();
	HRESULT			deliver_new_segment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

	IMemInputPin*	get_downstream_input() { return m_pDownstreamInput; }

protected:
	std::atomic<LONG>			m_ref;
	CSCAudioTransformFilter*	m_pFilter;	//weak
	IPin*						m_pConnected;
	IMemInputPin*				m_pDownstreamInput;	//cached for fast Receive call
	AM_MEDIA_TYPE				m_mt;
	bool						m_mt_set;

	void	free_media_type();
};
