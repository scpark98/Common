#pragma once

//CSCVideoTimeScale — DirectShow video transform filter. video sample 의 tStart/tStop 만
//1/rate 로 scale. PCM data pass-through (zero-copy).
//
//설계 목적 — LAV path 의 재생속도 변경 시 audio 의 TimeStretch 와 동일 비율로 video sample
//timestamp 도 scale. graph SetRate 가 LAV Video Decoder 의 timestamp scale 동작이 *atempo 의
//양 기반 timestamp 진행 비율* 과 *비대칭* 인 케이스 (특히 rate < 1.0) 에서 video 가 audio 보다
//느려지는 sync 문제 해결.
//
//audio (TimeStretch) 와 video (TimeScale) 가 같은 rate 로 timestamp 진행 → graph clock 의
//master 가 어디든 양쪽 일관.
//
//architecture — CSCAudioTransformFilter 와 별도 IBaseFilter 직접 구현. audio base 의 worker
//thread / queue / WAVEFORMATEX 처리 / internal allocator copy 가 video 에 부적합 (큰 buffer
//copy 비효율, frame 단위 동기 처리가 자연). Receive 가 sync: input sample 의 SetTime 만 호출
//후 그대로 downstream Deliver — zero-copy.

#include <Afxwin.h>
#include <dshow.h>
#include <atomic>

class CSCVideoTimeScaleInputPin;
class CSCVideoTimeScaleOutputPin;

class CSCVideoTimeScale : public IBaseFilter
{
public:
	CSCVideoTimeScale();
	virtual ~CSCVideoTimeScale();

	//rate 설정 — thread-safe (atomic). rate>0. 0 이하면 1.0 으로 처리. clamp 없음 (atempo 와 달리 video timestamp
	//는 단순 scale 이라 모든 양수 rate 처리 가능).
	void	set_rate(double rate) { if (rate <= 0.0) rate = 1.0; m_rate.store(rate); }
	double	get_rate() const { return m_rate.load(); }

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

//Internal — pin 에서 호출.
	IFilterGraph*			get_graph() { return m_pGraph; }
	FILTER_STATE			get_state() { return m_state; }
	CSCVideoTimeScaleInputPin*  get_input_pin()  { return m_pInputPin; }
	CSCVideoTimeScaleOutputPin* get_output_pin() { return m_pOutputPin; }

protected:
	std::atomic<LONG>	m_ref;
	FILTER_STATE		m_state;
	IFilterGraph*		m_pGraph;	//weak (DirectShow convention)
	IReferenceClock*	m_pClock;
	WCHAR				m_name[64];

	CSCVideoTimeScaleInputPin*	m_pInputPin;
	CSCVideoTimeScaleOutputPin*	m_pOutputPin;

	std::atomic<double>	m_rate{ 1.0 };
};


//Input pin — upstream (video decoder) 으로부터 sample 받음.
class CSCVideoTimeScaleInputPin : public IPin, public IMemInputPin
{
public:
	CSCVideoTimeScaleInputPin(CSCVideoTimeScale* pFilter);
	virtual ~CSCVideoTimeScaleInputPin();

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

protected:
	std::atomic<LONG>			m_ref;
	CSCVideoTimeScale*			m_pFilter;	//weak — filter outlives pin
	IPin*						m_pConnected;
	AM_MEDIA_TYPE				m_mt;
	bool						m_mt_set;
	IMemAllocator*				m_pAllocator;

	void	free_media_type();
};


//Output pin — downstream (video renderer) 으로 sample forward.
class CSCVideoTimeScaleOutputPin : public IPin
{
public:
	CSCVideoTimeScaleOutputPin(CSCVideoTimeScale* pFilter);
	virtual ~CSCVideoTimeScaleOutputPin();

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

	//Input pin 이 sample 처리 후 downstream 으로 forward 시 호출.
	HRESULT			deliver_sample(IMediaSample* pSample);
	HRESULT			deliver_end_of_stream();
	HRESULT			deliver_begin_flush();
	HRESULT			deliver_end_flush();
	HRESULT			deliver_new_segment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

	IMemInputPin*	get_downstream_input() { return m_pDownstreamInput; }

protected:
	std::atomic<LONG>			m_ref;
	CSCVideoTimeScale*			m_pFilter;	//weak
	IPin*						m_pConnected;
	IMemInputPin*				m_pDownstreamInput;	//cached for fast Receive call
	AM_MEDIA_TYPE				m_mt;
	bool						m_mt_set;

	void	free_media_type();
};
