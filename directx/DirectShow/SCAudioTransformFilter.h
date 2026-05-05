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

class CSCAudioTransformInputPin;
class CSCAudioTransformOutputPin;

class CSCAudioTransformFilter : public IBaseFilter
{
public:
	CSCAudioTransformFilter(LPCWSTR friendly_name);
	virtual ~CSCAudioTransformFilter();

	//파생이 override — buf 의 sample 을 in-place 수정.
	//buf : sample buffer 시작 주소
	//len : buffer 의 byte 길이
	//wfx : sample format (cached from connection negotiation)
	virtual void	process_sample(BYTE* buf, long len, const WAVEFORMATEX* wfx) = 0;

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
	const WAVEFORMATEX*		get_wave_format() { return m_wfx_set ? (const WAVEFORMATEX*)&m_wfx_ext : NULL; }
	//WAVEFORMATEXTENSIBLE 의 SubFormat 을 포함해 전체 cbFmt byte 까지 복사. WFEX subtype 검사를 위해 필수.
	void					set_wave_format(const BYTE* fmt, ULONG cbFmt);
	void					clear_wave_format() { m_wfx_set = false; }
	CSCAudioTransformInputPin*	get_input_pin()  { return m_pInputPin; }
	CSCAudioTransformOutputPin* get_output_pin() { return m_pOutputPin; }

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

protected:
	std::atomic<LONG>			m_ref;
	CSCAudioTransformFilter*	m_pFilter;	//weak — filter outlives pin
	IPin*						m_pConnected;
	AM_MEDIA_TYPE				m_mt;
	bool						m_mt_set;
	IMemAllocator*				m_pAllocator;

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
