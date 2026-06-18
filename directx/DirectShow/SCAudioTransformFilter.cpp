#include "SCAudioTransformFilter.h"
#include "../../log/SCLog/SCLog.h"
#include <strsafe.h>

#pragma comment(lib, "strmiids.lib")


//자체 enum (BaseClasses 의존 X)
class CSCAudioEnumPins : public IEnumPins
{
	std::atomic<LONG>	m_ref;
	IPin*				m_pPin1;
	IPin*				m_pPin2;
	int					m_pos;
public:
	CSCAudioEnumPins(IPin* p1, IPin* p2) : m_ref(0), m_pPin1(p1), m_pPin2(p2), m_pos(0) {}
	virtual ~CSCAudioEnumPins() {}

	STDMETHODIMP QueryInterface(REFIID iid, void** ppv) override
	{
		if (iid == IID_IUnknown || iid == IID_IEnumPins) { *ppv = static_cast<IEnumPins*>(this); AddRef(); return S_OK; }
		*ppv = NULL; return E_NOINTERFACE;
	}
	STDMETHODIMP_(ULONG) AddRef() override { return ++m_ref; }
	STDMETHODIMP_(ULONG) Release() override { LONG n = --m_ref; if (n == 0) delete this; return n; }

	STDMETHODIMP Next(ULONG cPins, IPin** ppPins, ULONG* pcFetched) override
	{
		ULONG fetched = 0;
		while (fetched < cPins)
		{
			IPin* p = NULL;
			if (m_pos == 0) p = m_pPin1;
			else if (m_pos == 1) p = m_pPin2;
			else break;
			m_pos++;
			if (p) { p->AddRef(); ppPins[fetched++] = p; }
		}
		if (pcFetched) *pcFetched = fetched;
		return (fetched == cPins) ? S_OK : S_FALSE;
	}
	STDMETHODIMP Skip(ULONG cPins) override { m_pos += (int)cPins; return S_OK; }
	STDMETHODIMP Reset() override { m_pos = 0; return S_OK; }
	STDMETHODIMP Clone(IEnumPins** ppEnum) override
	{
		if (!ppEnum) return E_POINTER;
		CSCAudioEnumPins* p = new CSCAudioEnumPins(m_pPin1, m_pPin2);
		p->m_pos = m_pos; p->AddRef(); *ppEnum = p;
		return S_OK;
	}
};


class CSCAudioEnumMediaTypes : public IEnumMediaTypes
{
	std::atomic<LONG>	m_ref;
public:
	CSCAudioEnumMediaTypes() : m_ref(0) {}
	virtual ~CSCAudioEnumMediaTypes() {}

	STDMETHODIMP QueryInterface(REFIID iid, void** ppv) override
	{
		if (iid == IID_IUnknown || iid == IID_IEnumMediaTypes) { *ppv = static_cast<IEnumMediaTypes*>(this); AddRef(); return S_OK; }
		*ppv = NULL; return E_NOINTERFACE;
	}
	STDMETHODIMP_(ULONG) AddRef() override { return ++m_ref; }
	STDMETHODIMP_(ULONG) Release() override { LONG n = --m_ref; if (n == 0) delete this; return n; }

	STDMETHODIMP Next(ULONG, AM_MEDIA_TYPE**, ULONG* pcFetched) override { if (pcFetched) *pcFetched = 0; return S_FALSE; }
	STDMETHODIMP Skip(ULONG) override { return S_OK; }
	STDMETHODIMP Reset() override { return S_OK; }
	STDMETHODIMP Clone(IEnumMediaTypes** ppEnum) override
	{
		if (!ppEnum) return E_POINTER;
		CSCAudioEnumMediaTypes* p = new CSCAudioEnumMediaTypes();
		p->AddRef(); *ppEnum = p;
		return S_OK;
	}
};


//Media type 비교 — major/sub/format 일치 검사.
static bool is_same_media_type(const AM_MEDIA_TYPE* a, const AM_MEDIA_TYPE* b)
{
	if (!a || !b) return false;
	return IsEqualGUID(a->majortype, b->majortype) && IsEqualGUID(a->subtype, b->subtype);
}

//Media type 깊은 복사.
static void copy_media_type(AM_MEDIA_TYPE* dst, const AM_MEDIA_TYPE* src)
{
	*dst = *src;
	if (src->cbFormat && src->pbFormat)
	{
		dst->pbFormat = (BYTE*)CoTaskMemAlloc(src->cbFormat);
		if (dst->pbFormat) memcpy(dst->pbFormat, src->pbFormat, src->cbFormat);
	}
	if (src->pUnk) src->pUnk->AddRef();
}

static void release_media_type(AM_MEDIA_TYPE* mt)
{
	if (!mt) return;
	if (mt->cbFormat && mt->pbFormat) { CoTaskMemFree(mt->pbFormat); mt->pbFormat = NULL; mt->cbFormat = 0; }
	if (mt->pUnk) { mt->pUnk->Release(); mt->pUnk = NULL; }
}


//================================================================================
// CSCAudioTransformFilter
//================================================================================

CSCAudioTransformFilter::CSCAudioTransformFilter(LPCWSTR friendly_name)
	: m_ref(0), m_state(State_Stopped), m_pGraph(NULL), m_pClock(NULL),
	  m_pInputPin(NULL), m_pOutputPin(NULL), m_wfx_set(false), m_delay_100ns(0),
	  m_worker_run(false), m_flushing(false)
{
	memset(&m_wfx_ext, 0, sizeof(m_wfx_ext));
	if (friendly_name) StringCchCopyW(m_name, 64, friendly_name);
	else m_name[0] = 0;

	m_pInputPin  = new CSCAudioTransformInputPin(this);  m_pInputPin->AddRef();
	m_pOutputPin = new CSCAudioTransformOutputPin(this); m_pOutputPin->AddRef();
}

CSCAudioTransformFilter::~CSCAudioTransformFilter()
{
	stop_worker();		//worker join + queue drain
	if (m_pInputPin)  { m_pInputPin->Release();  m_pInputPin = NULL; }
	if (m_pOutputPin) { m_pOutputPin->Release(); m_pOutputPin = NULL; }
	if (m_pClock)     { m_pClock->Release();     m_pClock = NULL; }
}

void CSCAudioTransformFilter::set_wave_format(const BYTE* fmt, ULONG cbFmt)
{
	if (!fmt || cbFmt == 0) return;
	memset(&m_wfx_ext, 0, sizeof(m_wfx_ext));
	ULONG copy_size = (cbFmt < sizeof(WAVEFORMATEXTENSIBLE)) ? cbFmt : (ULONG)sizeof(WAVEFORMATEXTENSIBLE);
	memcpy(&m_wfx_ext, fmt, copy_size);
	m_wfx_set = true;
}

STDMETHODIMP CSCAudioTransformFilter::QueryInterface(REFIID iid, void** ppv)
{
	if (!ppv) return E_POINTER;
	if (iid == IID_IUnknown || iid == IID_IPersist || iid == IID_IMediaFilter || iid == IID_IBaseFilter)
	{
		*ppv = static_cast<IBaseFilter*>(this); AddRef();
		return S_OK;
	}
	if (iid == IID_IMediaSeeking)
	{
		*ppv = static_cast<IMediaSeeking*>(this); AddRef();
		return S_OK;
	}
	*ppv = NULL;
	return E_NOINTERFACE;
}
STDMETHODIMP_(ULONG) CSCAudioTransformFilter::AddRef() { return ++m_ref; }
STDMETHODIMP_(ULONG) CSCAudioTransformFilter::Release() { LONG n = --m_ref; if (n == 0) delete this; return n; }

STDMETHODIMP CSCAudioTransformFilter::GetClassID(CLSID* pClsID)
{
	if (!pClsID) return E_POINTER;
	memset(pClsID, 0, sizeof(CLSID));
	return S_OK;
}

STDMETHODIMP CSCAudioTransformFilter::Stop()  { m_state = State_Stopped; stop_worker(); return S_OK; }
STDMETHODIMP CSCAudioTransformFilter::Pause() { m_state = State_Paused; start_worker(); return S_OK; }
STDMETHODIMP CSCAudioTransformFilter::Run(REFERENCE_TIME) { m_state = State_Running; start_worker(); return S_OK; }
STDMETHODIMP CSCAudioTransformFilter::GetState(DWORD, FILTER_STATE* pState)
{
	if (!pState) return E_POINTER;
	*pState = m_state; return S_OK;
}
STDMETHODIMP CSCAudioTransformFilter::SetSyncSource(IReferenceClock* pClock)
{
	if (m_pClock) m_pClock->Release();
	m_pClock = pClock;
	if (m_pClock) m_pClock->AddRef();
	return S_OK;
}
STDMETHODIMP CSCAudioTransformFilter::GetSyncSource(IReferenceClock** ppClock)
{
	if (!ppClock) return E_POINTER;
	*ppClock = m_pClock;
	if (m_pClock) m_pClock->AddRef();
	return S_OK;
}

STDMETHODIMP CSCAudioTransformFilter::EnumPins(IEnumPins** ppEnum)
{
	if (!ppEnum) return E_POINTER;
	CSCAudioEnumPins* p = new CSCAudioEnumPins(m_pInputPin, m_pOutputPin);
	p->AddRef(); *ppEnum = p;
	return S_OK;
}
STDMETHODIMP CSCAudioTransformFilter::FindPin(LPCWSTR Id, IPin** ppPin)
{
	if (!ppPin || !Id) return E_POINTER;
	if (wcscmp(Id, L"In")  == 0) { m_pInputPin->AddRef();  *ppPin = m_pInputPin;  return S_OK; }
	if (wcscmp(Id, L"Out") == 0) { m_pOutputPin->AddRef(); *ppPin = m_pOutputPin; return S_OK; }
	*ppPin = NULL;
	return VFW_E_NOT_FOUND;
}
STDMETHODIMP CSCAudioTransformFilter::QueryFilterInfo(FILTER_INFO* pInfo)
{
	if (!pInfo) return E_POINTER;
	StringCchCopyW(pInfo->achName, sizeof(pInfo->achName)/sizeof(WCHAR), m_name);
	pInfo->pGraph = m_pGraph;
	if (m_pGraph) m_pGraph->AddRef();
	return S_OK;
}
STDMETHODIMP CSCAudioTransformFilter::JoinFilterGraph(IFilterGraph* pGraph, LPCWSTR pName)
{
	m_pGraph = pGraph;	//weak
	if (pName) StringCchCopyW(m_name, 64, pName);
	return S_OK;
}
STDMETHODIMP CSCAudioTransformFilter::QueryVendorInfo(LPWSTR*) { return E_NOTIMPL; }


//================================================================================
// CSCAudioTransformFilter — IMediaSeeking pass-through
//
// graph IMediaSeeking::SetRate 의 default 구현은 *모든 filter 의 IMediaSeeking::SetRate*
// 를 호출하며, 응답한 filter 가 자기 NewSegment 발행으로 chain rate propagation 보장.
// 우리 transform 이 IMediaSeeking 미응답이면 SC chain 의 일부 단계 NewSegment 가 환경
// 의존적으로 누락되어 audio renderer 의 실재 rate 적용 fail. 모든 메서드를 upstream pin /
// filter 의 IMediaSeeking 으로 위임 — CPosPassThru 표준 패턴.
//================================================================================

//helper — input pin 의 connected upstream 의 IMediaSeeking 얻기. 우선 pin 자체 QI,
//안 되면 그 pin 이 속한 filter 의 IMediaSeeking. caller 가 release. NULL 가능.
static IMediaSeeking* get_upstream_seeking(CSCAudioTransformInputPin* pInputPin)
{
	if (!pInputPin) return NULL;

	IPin* pConnected = NULL;
	pInputPin->ConnectedTo(&pConnected);
	if (!pConnected) return NULL;

	IMediaSeeking* pMS = NULL;
	HRESULT hr = pConnected->QueryInterface(IID_IMediaSeeking, (void**)&pMS);
	if (FAILED(hr) || !pMS)
	{
		pMS = NULL;
		PIN_INFO info = { 0 };
		if (SUCCEEDED(pConnected->QueryPinInfo(&info)) && info.pFilter)
		{
			info.pFilter->QueryInterface(IID_IMediaSeeking, (void**)&pMS);
			info.pFilter->Release();
		}
	}
	pConnected->Release();
	return pMS;
}

//각 IMediaSeeking 메서드 macro — upstream IMediaSeeking 얻고 동일 호출 후 release.
//얻기 실패 시 E_NOTIMPL (DirectShow convention).
#define SC_SEEK_DELEGATE_0(method) \
	IMediaSeeking* pMS = get_upstream_seeking(m_pInputPin); \
	if (!pMS) return E_NOTIMPL; \
	HRESULT hr = pMS->method(); \
	pMS->Release(); \
	return hr;

#define SC_SEEK_DELEGATE_1(method, a1) \
	IMediaSeeking* pMS = get_upstream_seeking(m_pInputPin); \
	if (!pMS) return E_NOTIMPL; \
	HRESULT hr = pMS->method(a1); \
	pMS->Release(); \
	return hr;

#define SC_SEEK_DELEGATE_2(method, a1, a2) \
	IMediaSeeking* pMS = get_upstream_seeking(m_pInputPin); \
	if (!pMS) return E_NOTIMPL; \
	HRESULT hr = pMS->method(a1, a2); \
	pMS->Release(); \
	return hr;

#define SC_SEEK_DELEGATE_4(method, a1, a2, a3, a4) \
	IMediaSeeking* pMS = get_upstream_seeking(m_pInputPin); \
	if (!pMS) return E_NOTIMPL; \
	HRESULT hr = pMS->method(a1, a2, a3, a4); \
	pMS->Release(); \
	return hr;

STDMETHODIMP CSCAudioTransformFilter::GetCapabilities(DWORD* pCapabilities)
{
	SC_SEEK_DELEGATE_1(GetCapabilities, pCapabilities)
}
STDMETHODIMP CSCAudioTransformFilter::CheckCapabilities(DWORD* pCapabilities)
{
	SC_SEEK_DELEGATE_1(CheckCapabilities, pCapabilities)
}
STDMETHODIMP CSCAudioTransformFilter::IsFormatSupported(const GUID* pFormat)
{
	SC_SEEK_DELEGATE_1(IsFormatSupported, pFormat)
}
STDMETHODIMP CSCAudioTransformFilter::QueryPreferredFormat(GUID* pFormat)
{
	SC_SEEK_DELEGATE_1(QueryPreferredFormat, pFormat)
}
STDMETHODIMP CSCAudioTransformFilter::GetTimeFormat(GUID* pFormat)
{
	SC_SEEK_DELEGATE_1(GetTimeFormat, pFormat)
}
STDMETHODIMP CSCAudioTransformFilter::IsUsingTimeFormat(const GUID* pFormat)
{
	SC_SEEK_DELEGATE_1(IsUsingTimeFormat, pFormat)
}
STDMETHODIMP CSCAudioTransformFilter::SetTimeFormat(const GUID* pFormat)
{
	SC_SEEK_DELEGATE_1(SetTimeFormat, pFormat)
}
STDMETHODIMP CSCAudioTransformFilter::GetDuration(LONGLONG* pDuration)
{
	SC_SEEK_DELEGATE_1(GetDuration, pDuration)
}
STDMETHODIMP CSCAudioTransformFilter::GetStopPosition(LONGLONG* pStop)
{
	SC_SEEK_DELEGATE_1(GetStopPosition, pStop)
}
STDMETHODIMP CSCAudioTransformFilter::GetCurrentPosition(LONGLONG* pCurrent)
{
	SC_SEEK_DELEGATE_1(GetCurrentPosition, pCurrent)
}
STDMETHODIMP CSCAudioTransformFilter::ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat,
	LONGLONG Source, const GUID* pSourceFormat)
{
	SC_SEEK_DELEGATE_4(ConvertTimeFormat, pTarget, pTargetFormat, Source, pSourceFormat)
}
STDMETHODIMP CSCAudioTransformFilter::SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags,
	LONGLONG* pStop, DWORD dwStopFlags)
{
	SC_SEEK_DELEGATE_4(SetPositions, pCurrent, dwCurrentFlags, pStop, dwStopFlags)
}
STDMETHODIMP CSCAudioTransformFilter::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop)
{
	SC_SEEK_DELEGATE_2(GetPositions, pCurrent, pStop)
}
STDMETHODIMP CSCAudioTransformFilter::GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest)
{
	SC_SEEK_DELEGATE_2(GetAvailable, pEarliest, pLatest)
}
STDMETHODIMP CSCAudioTransformFilter::SetRate(double dRate)
{
	SC_SEEK_DELEGATE_1(SetRate, dRate)
}
STDMETHODIMP CSCAudioTransformFilter::GetRate(double* pdRate)
{
	SC_SEEK_DELEGATE_1(GetRate, pdRate)
}
STDMETHODIMP CSCAudioTransformFilter::GetPreroll(LONGLONG* pllPreroll)
{
	SC_SEEK_DELEGATE_1(GetPreroll, pllPreroll)
}

#undef SC_SEEK_DELEGATE_0
#undef SC_SEEK_DELEGATE_1
#undef SC_SEEK_DELEGATE_2
#undef SC_SEEK_DELEGATE_4


//================================================================================
// CSCAudioTransformFilter — Async worker thread + queue
//
// LAV→SC→DSound 직렬 sample push 가 LAV emit rate 를 throttle 해 graph rate 변경이
// audio renderer 에 반영되지 못하는 문제 해결. Receive 는 sample copy + queue push 후
// 즉시 return, worker thread 가 dequeue → process_sample → downstream Receive. LAV thread
// 와 DSound thread 사이의 직접 backpressure 단절 → 양쪽 모두 자기 속도로 동작.
//================================================================================

void CSCAudioTransformFilter::start_worker()
{
	if (m_worker_run.load())
		return;	//already running
	m_worker_run.store(true);
	m_flushing.store(false);
	m_worker = std::thread(&CSCAudioTransformFilter::worker_main, this);
}

void CSCAudioTransformFilter::stop_worker()
{
	if (!m_worker_run.load() && !m_worker.joinable())
		return;
	m_worker_run.store(false);
	{
		std::lock_guard<std::mutex> lock(m_queue_mutex);
		m_queue_cv.notify_all();
	}
	if (m_worker.joinable())
		m_worker.join();
	clear_queue();		//남은 sample release
}

void CSCAudioTransformFilter::clear_queue()
{
	std::lock_guard<std::mutex> lock(m_queue_mutex);
	for (auto& w : m_queue)
	{
		if (w.kind == kind_sample && w.sample)
			w.sample->Release();
	}
	m_queue.clear();
}

void CSCAudioTransformFilter::enqueue_sample(IMediaSample* pSample)
{
	if (!pSample) return;
	if (m_flushing.load())
	{
		//flush 중엔 신규 sample drop. caller 가 AddRef 해서 넘긴 ref 해제.
		pSample->Release();
		return;
	}
	work_item w;
	w.kind = kind_sample;
	w.sample = pSample;	//caller 가 AddRef 한 상태로 전달 — worker 가 dequeue 후 Release
	w.ns_start = 0; w.ns_stop = 0; w.ns_rate = 1.0;
	{
		std::lock_guard<std::mutex> lock(m_queue_mutex);
		m_queue.push_back(w);
	}
	m_queue_cv.notify_one();
}

void CSCAudioTransformFilter::enqueue_new_segment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	if (m_flushing.load())
		return;
	work_item w;
	w.kind = kind_new_segment;
	w.sample = NULL;
	w.ns_start = tStart; w.ns_stop = tStop; w.ns_rate = dRate;
	{
		std::lock_guard<std::mutex> lock(m_queue_mutex);
		m_queue.push_back(w);
	}
	m_queue_cv.notify_one();
}

void CSCAudioTransformFilter::enqueue_end_of_stream()
{
	if (m_flushing.load())
		return;
	work_item w;
	w.kind = kind_eos;
	w.sample = NULL;
	w.ns_start = 0; w.ns_stop = 0; w.ns_rate = 1.0;
	{
		std::lock_guard<std::mutex> lock(m_queue_mutex);
		m_queue.push_back(w);
	}
	m_queue_cv.notify_one();
}

void CSCAudioTransformFilter::begin_flush()
{
	m_flushing.store(true);
	clear_queue();		//queue 비우기
	{
		std::lock_guard<std::mutex> lock(m_queue_mutex);
		m_queue_cv.notify_all();	//worker 가 wait 중이면 깨움
	}
}

void CSCAudioTransformFilter::end_flush()
{
	m_flushing.store(false);
}

void CSCAudioTransformFilter::worker_main()
{
	for (;;)
	{
		work_item w;
		bool got = false;
		{
			std::unique_lock<std::mutex> lock(m_queue_mutex);
			m_queue_cv.wait(lock, [this] {
				return !m_queue.empty() || !m_worker_run.load();
			});
			if (!m_worker_run.load() && m_queue.empty())
				break;
			if (!m_queue.empty())
			{
				w = m_queue.front();
				m_queue.pop_front();
				got = true;
			}
		}
		if (got)
			process_one(w);
	}
}

void CSCAudioTransformFilter::process_one(const work_item& w)
{
	if (w.kind == kind_sample && w.sample)
	{
		//in-place process + delay shift + downstream deliver.
		BYTE* pBuf = NULL;
		w.sample->GetPointer(&pBuf);
		long len = w.sample->GetActualDataLength();
		if (pBuf && len > 0)
		{
			const WAVEFORMATEX* pwfx = get_wave_format();
			process_sample(pBuf, len, pwfx);
		}

		//audio delay — 음수(audio 당김)만 동작. 양수(audio 밀기)는 DSound 가 미래 timestamp 를 무시해 무효.
		//양수를 content-delay(무음 prefill FIFO)로 시도했으나 하류에서 흡수돼 효과 없음 (2026-05-30 검증). 음수만 유지.
		LONGLONG delay = m_delay_100ns.load();
		if (delay != 0)
		{
			REFERENCE_TIME tStart = 0, tStop = 0;
			if (SUCCEEDED(w.sample->GetTime(&tStart, &tStop)))
			{
				tStart += delay;
				tStop  += delay;
				w.sample->SetTime(&tStart, &tStop);
			}
		}

		if (m_pOutputPin)
			m_pOutputPin->deliver_sample(w.sample);
		w.sample->Release();	//enqueue 시 AddRef 된 ref 해제
	}
	else if (w.kind == kind_new_segment)
	{
		if (m_pOutputPin)
			m_pOutputPin->deliver_new_segment(w.ns_start, w.ns_stop, w.ns_rate);
	}
	else if (w.kind == kind_eos)
	{
		if (m_pOutputPin)
			m_pOutputPin->deliver_end_of_stream();
	}
}


//================================================================================
// CSCAudioTransformInputPin
//================================================================================

CSCAudioTransformInputPin::CSCAudioTransformInputPin(CSCAudioTransformFilter* pFilter)
	: m_ref(0), m_pFilter(pFilter), m_pConnected(NULL), m_mt_set(false),
	  m_pAllocator(NULL), m_pInternalAllocator(NULL)
{
	memset(&m_mt, 0, sizeof(m_mt));
}

CSCAudioTransformInputPin::~CSCAudioTransformInputPin()
{
	if (m_pConnected) m_pConnected->Release();
	if (m_pAllocator) m_pAllocator->Release();
	if (m_pInternalAllocator)
	{
		m_pInternalAllocator->Decommit();
		m_pInternalAllocator->Release();
	}
	free_media_type();
}

IMemAllocator* CSCAudioTransformInputPin::get_or_create_internal_allocator()
{
	if (m_pInternalAllocator)
		return m_pInternalAllocator;

	HRESULT hr = CoCreateInstance(CLSID_MemoryAllocator, NULL, CLSCTX_INPROC_SERVER,
		IID_IMemAllocator, (void**)&m_pInternalAllocator);
	if (FAILED(hr) || !m_pInternalAllocator)
		return NULL;

	//async queue path 의 sample copy 용 — LAV ↔ DSound 의 buffer pool 과 분리.
	//cBuffers 충분히 (16) — worker 가 dequeue 후 downstream 에 forward 되기까지의 in-flight.
	//cbBuffer 는 first Receive 시 upstream sample size 로 동적 갱신 가능하나 일단 64KB.
	ALLOCATOR_PROPERTIES props = { 0 };
	props.cBuffers = 16;
	props.cbBuffer = 65536;
	props.cbAlign  = 1;
	props.cbPrefix = 0;
	ALLOCATOR_PROPERTIES actual = { 0 };
	m_pInternalAllocator->SetProperties(&props, &actual);
	m_pInternalAllocator->Commit();
	return m_pInternalAllocator;
}

void CSCAudioTransformInputPin::free_media_type()
{
	if (m_mt_set)
	{
		release_media_type(&m_mt);
		memset(&m_mt, 0, sizeof(m_mt));
		m_mt_set = false;
	}
}

STDMETHODIMP CSCAudioTransformInputPin::QueryInterface(REFIID iid, void** ppv)
{
	if (!ppv) return E_POINTER;
	if (iid == IID_IUnknown || iid == IID_IPin)        { *ppv = static_cast<IPin*>(this);         AddRef(); return S_OK; }
	if (iid == IID_IMemInputPin)                       { *ppv = static_cast<IMemInputPin*>(this); AddRef(); return S_OK; }
	*ppv = NULL;
	return E_NOINTERFACE;
}
STDMETHODIMP_(ULONG) CSCAudioTransformInputPin::AddRef()  { return ++m_ref; }
STDMETHODIMP_(ULONG) CSCAudioTransformInputPin::Release() { LONG n = --m_ref; if (n == 0) delete this; return n; }

STDMETHODIMP CSCAudioTransformInputPin::Connect(IPin*, const AM_MEDIA_TYPE*) { return E_UNEXPECTED; }	//input only

STDMETHODIMP CSCAudioTransformInputPin::ReceiveConnection(IPin* pConnector, const AM_MEDIA_TYPE* pmt)
{
	if (!pConnector || !pmt) return E_POINTER;
	if (m_pConnected) return VFW_E_ALREADY_CONNECTED;

	//audio 만 accept.
	if (!IsEqualGUID(pmt->majortype, MEDIATYPE_Audio))
		return VFW_E_TYPE_NOT_ACCEPTED;

	//WAVEFORMATEX / WAVEFORMATEXTENSIBLE 추출 — SubFormat 까지 보존하기 위해 전체 cbFormat 복사.
	if (pmt->formattype == FORMAT_WaveFormatEx && pmt->cbFormat >= sizeof(WAVEFORMATEX) && pmt->pbFormat)
	{
		m_pFilter->set_wave_format(pmt->pbFormat, pmt->cbFormat);
		const WAVEFORMATEX* wfx = (const WAVEFORMATEX*)pmt->pbFormat;
		//logWrite(_T("[AudioFilter] ReceiveConnection wfx tag=0x%04X ch=%d sample_rate=%lu bps=%d cbSize=%d cbFmt=%lu"),
			//wfx->wFormatTag, wfx->nChannels, wfx->nSamplesPerSec, wfx->wBitsPerSample, wfx->cbSize, pmt->cbFormat);
		if (wfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE && pmt->cbFormat >= sizeof(WAVEFORMATEXTENSIBLE))
		{
			const WAVEFORMATEXTENSIBLE* wfex = (const WAVEFORMATEXTENSIBLE*)pmt->pbFormat;
			//logWrite(_T("[AudioFilter] WFEX SubFormat.D1=%08lX validBits=%d ChMask=0x%lX"),
				//wfex->SubFormat.Data1, wfex->Samples.wValidBitsPerSample, wfex->dwChannelMask);
		}
	}
	else
	{
		//logWrite(_T("[AudioFilter] ReceiveConnection no WAVEFORMATEX (formattype.D1=%08lX cbFmt=%lu)"),
			//pmt->formattype.Data1, pmt->cbFormat);
	}

	free_media_type();
	copy_media_type(&m_mt, pmt);
	m_mt_set = true;

	m_pConnected = pConnector;
	m_pConnected->AddRef();
	return S_OK;
}

STDMETHODIMP CSCAudioTransformInputPin::Disconnect()
{
	if (!m_pConnected) return S_FALSE;
	m_pConnected->Release(); m_pConnected = NULL;
	free_media_type();
	m_pFilter->clear_wave_format();
	return S_OK;
}

STDMETHODIMP CSCAudioTransformInputPin::ConnectedTo(IPin** ppPin)
{
	if (!ppPin) return E_POINTER;
	if (!m_pConnected) { *ppPin = NULL; return VFW_E_NOT_CONNECTED; }
	m_pConnected->AddRef(); *ppPin = m_pConnected;
	return S_OK;
}

STDMETHODIMP CSCAudioTransformInputPin::ConnectionMediaType(AM_MEDIA_TYPE* pmt)
{
	if (!pmt) return E_POINTER;
	if (!m_pConnected) return VFW_E_NOT_CONNECTED;
	copy_media_type(pmt, &m_mt);
	return S_OK;
}

STDMETHODIMP CSCAudioTransformInputPin::QueryPinInfo(PIN_INFO* pInfo)
{
	if (!pInfo) return E_POINTER;
	pInfo->pFilter = m_pFilter;
	if (m_pFilter) m_pFilter->AddRef();
	pInfo->dir = PINDIR_INPUT;
	StringCchCopyW(pInfo->achName, sizeof(pInfo->achName)/sizeof(WCHAR), L"In");
	return S_OK;
}
STDMETHODIMP CSCAudioTransformInputPin::QueryDirection(PIN_DIRECTION* pPinDir) { if (!pPinDir) return E_POINTER; *pPinDir = PINDIR_INPUT; return S_OK; }
STDMETHODIMP CSCAudioTransformInputPin::QueryId(LPWSTR* Id)
{
	if (!Id) return E_POINTER;
	*Id = (LPWSTR)CoTaskMemAlloc(sizeof(WCHAR) * 3);
	if (!*Id) return E_OUTOFMEMORY;
	wcscpy_s(*Id, 3, L"In");
	return S_OK;
}
STDMETHODIMP CSCAudioTransformInputPin::QueryAccept(const AM_MEDIA_TYPE* pmt)
{
	if (!pmt) return E_POINTER;
	return IsEqualGUID(pmt->majortype, MEDIATYPE_Audio) ? S_OK : S_FALSE;
}
STDMETHODIMP CSCAudioTransformInputPin::EnumMediaTypes(IEnumMediaTypes** ppEnum)
{
	if (!ppEnum) return E_POINTER;
	CSCAudioEnumMediaTypes* p = new CSCAudioEnumMediaTypes();
	p->AddRef(); *ppEnum = p;
	return S_OK;
}
STDMETHODIMP CSCAudioTransformInputPin::QueryInternalConnections(IPin** apPin, ULONG* nPin)
{
	//in-place: input ↔ output 1:1 internal connection 노출.
	if (!nPin) return E_POINTER;
	IPin* pOut = (IPin*)m_pFilter->get_output_pin();
	if (apPin && *nPin >= 1 && pOut) { pOut->AddRef(); apPin[0] = pOut; *nPin = 1; return S_OK; }
	*nPin = 1;
	return S_FALSE;
}

//Async path — EOS / Flush / NewSegment 모두 worker queue 로 라우팅해 sample 흐름과
//순서 보존. 직접 downstream 호출하면 worker 가 queue 에 holding 한 sample 보다 먼저 도착해
//순서 깨짐 (예: EOS 가 마지막 sample 보다 먼저 도착해 audio 꼬리 잘림).
STDMETHODIMP CSCAudioTransformInputPin::EndOfStream()
{
	m_pFilter->enqueue_end_of_stream();
	return S_OK;
}
STDMETHODIMP CSCAudioTransformInputPin::BeginFlush()
{
	//Decommit/Commit cycle 제거 — 빠른 연속 seek 시 outstanding sample 의 Release 와 race 로 allocator state stuck
	//→ 새 GetBuffer fail → audio 영구 silence 회귀. m_flushing flag + clear_queue 만으로 충분.
	m_pFilter->begin_flush();		//queue clear + 신규 sample drop
	return m_pFilter->get_output_pin()->deliver_begin_flush();
}
STDMETHODIMP CSCAudioTransformInputPin::EndFlush()
{
	m_pFilter->end_flush();
	return m_pFilter->get_output_pin()->deliver_end_flush();
}
STDMETHODIMP CSCAudioTransformInputPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	m_pFilter->enqueue_new_segment(tStart, tStop, dRate);
	return S_OK;
}

STDMETHODIMP CSCAudioTransformInputPin::GetAllocator(IMemAllocator** ppAllocator)
{
	if (!ppAllocator) return E_POINTER;
	if (!m_pAllocator)
	{
		HRESULT hr = CoCreateInstance(CLSID_MemoryAllocator, NULL, CLSCTX_INPROC_SERVER,
			IID_IMemAllocator, (void**)&m_pAllocator);
		if (FAILED(hr)) return hr;
	}
	m_pAllocator->AddRef();
	*ppAllocator = m_pAllocator;
	return S_OK;
}
STDMETHODIMP CSCAudioTransformInputPin::NotifyAllocator(IMemAllocator* pAllocator, BOOL)
{
	if (m_pAllocator) m_pAllocator->Release();
	m_pAllocator = pAllocator;
	if (m_pAllocator) m_pAllocator->AddRef();
	return S_OK;
}
STDMETHODIMP CSCAudioTransformInputPin::GetAllocatorRequirements(ALLOCATOR_PROPERTIES*) { return E_NOTIMPL; }

//Async Receive — 자체 IMemAllocator 의 새 sample 으로 upstream sample 데이터를 복사한 후
//queue 에 push, 즉시 return. upstream (LAV) 의 buffer 는 그 즉시 해제되어 LAV 가 자기
//thread 에서 다음 sample produce 가능. worker thread 가 dequeue → process_sample → deliver.
//이로써 LAV emit rate 와 DSound consume rate 의 직접 backpressure 가 끊겨, graph rate 변경
//시에도 DSound 가 자기 audio device clock 에 따라 sample timestamp 정상 적용.
STDMETHODIMP CSCAudioTransformInputPin::Receive(IMediaSample* pSample)
{
	if (!pSample) return E_POINTER;
	if (m_pFilter->get_state() == State_Stopped) return E_UNEXPECTED;

	//자체 allocator 에서 새 sample 가져옴.
	IMemAllocator* pAlloc = get_or_create_internal_allocator();
	if (!pAlloc)
		return E_FAIL;

	//blocking GetBuffer(...,0) 금지 — 16-buffer 가 다 in-flight 면 ~1버퍼 주기(~5.9ms) 블록한다. 이게 소스의
	//Deliver→이 Receive 를 잡아, seek 시 on_seek_flush 의 Stop() 이 그만큼 기다리게 해 오디오 끊김의 원인이 됐다.
	//AM_GBF_NOWAIT 폴링 + m_flushing 체크로 in-progress 블록을 ~1ms 안에 abort. allocator Decommit(=silence 회귀)·
	//렌더러(=영상 stutter 회귀) 를 안 건드리고, flush 중엔 기존 enqueue_sample 과 동일하게 sample drop.
	IMediaSample* pOur = NULL;
	HRESULT hr;
	for (;;)
	{
		hr = pAlloc->GetBuffer(&pOur, NULL, NULL, AM_GBF_NOWAIT);
		if (SUCCEEDED(hr) && pOur)
			break;
		if (m_pFilter->is_flushing())
			return S_OK;
		if (m_pFilter->get_state() == State_Stopped)
			return E_UNEXPECTED;
		Sleep(1);
	}

	BYTE* pInBuf = NULL;
	pSample->GetPointer(&pInBuf);
	long len = pSample->GetActualDataLength();

	BYTE* pOutBuf = NULL;
	pOur->GetPointer(&pOutBuf);
	long maxLen = pOur->GetSize();
	if (len > maxLen)
		len = maxLen;
	if (pInBuf && pOutBuf && len > 0)
		memcpy(pOutBuf, pInBuf, len);
	pOur->SetActualDataLength(len);

	//timestamp / flags 복사 — sync point / discontinuity / preroll 보존.
	REFERENCE_TIME tStart = 0, tStop = 0;
	if (SUCCEEDED(pSample->GetTime(&tStart, &tStop)))
		pOur->SetTime(&tStart, &tStop);

	LONGLONG mStart = 0, mStop = 0;
	if (SUCCEEDED(pSample->GetMediaTime(&mStart, &mStop)))
		pOur->SetMediaTime(&mStart, &mStop);

	pOur->SetSyncPoint(pSample->IsSyncPoint() == S_OK ? TRUE : FALSE);
	pOur->SetDiscontinuity(pSample->IsDiscontinuity() == S_OK ? TRUE : FALSE);
	pOur->SetPreroll(pSample->IsPreroll() == S_OK ? TRUE : FALSE);

	//queue 에 push — worker 가 dequeue 후 process + deliver + Release.
	m_pFilter->enqueue_sample(pOur);
	return S_OK;
}
STDMETHODIMP CSCAudioTransformInputPin::ReceiveMultiple(IMediaSample** pSamples, long nSamples, long* nSamplesProcessed)
{
	long n = 0;
	for (long i = 0; i < nSamples; i++)
	{
		HRESULT hr = Receive(pSamples[i]);
		if (FAILED(hr)) break;
		n++;
	}
	if (nSamplesProcessed) *nSamplesProcessed = n;
	return S_OK;
}
STDMETHODIMP CSCAudioTransformInputPin::ReceiveCanBlock()
{
	//async path — 우리 Receive 는 GetBuffer 외에는 block 안 함. allocator 의 buffer 가
	//고갈되면 GetBuffer 가 block 가능하나 cBuffers=16 이라 보통 충분. S_FALSE 반환으로
	//upstream 에게 "block 안 한다" 안내 → upstream 자기 thread 흐름 결정에 도움.
	return S_FALSE;
}


//================================================================================
// CSCAudioTransformOutputPin
//================================================================================

CSCAudioTransformOutputPin::CSCAudioTransformOutputPin(CSCAudioTransformFilter* pFilter)
	: m_ref(0), m_pFilter(pFilter), m_pConnected(NULL), m_pDownstreamInput(NULL), m_mt_set(false)
{
	memset(&m_mt, 0, sizeof(m_mt));
}

CSCAudioTransformOutputPin::~CSCAudioTransformOutputPin()
{
	if (m_pDownstreamInput) m_pDownstreamInput->Release();
	if (m_pConnected) m_pConnected->Release();
	free_media_type();
}

void CSCAudioTransformOutputPin::free_media_type()
{
	if (m_mt_set)
	{
		release_media_type(&m_mt);
		memset(&m_mt, 0, sizeof(m_mt));
		m_mt_set = false;
	}
}

STDMETHODIMP CSCAudioTransformOutputPin::QueryInterface(REFIID iid, void** ppv)
{
	if (!ppv) return E_POINTER;
	if (iid == IID_IUnknown || iid == IID_IPin) { *ppv = static_cast<IPin*>(this); AddRef(); return S_OK; }
	*ppv = NULL;
	return E_NOINTERFACE;
}
STDMETHODIMP_(ULONG) CSCAudioTransformOutputPin::AddRef()  { return ++m_ref; }
STDMETHODIMP_(ULONG) CSCAudioTransformOutputPin::Release() { LONG n = --m_ref; if (n == 0) delete this; return n; }

STDMETHODIMP CSCAudioTransformOutputPin::Connect(IPin* pReceivePin, const AM_MEDIA_TYPE* pmt)
{
	if (!pReceivePin) return E_POINTER;
	if (m_pConnected) return VFW_E_ALREADY_CONNECTED;

	//input pin 의 connected media type 으로 propose (in-place: 같은 type pass-through).
	CSCAudioTransformInputPin* pIn = m_pFilter->get_input_pin();
	AM_MEDIA_TYPE mt_propose;
	memset(&mt_propose, 0, sizeof(mt_propose));
	if (pmt && pmt->cbFormat > 0)
		copy_media_type(&mt_propose, pmt);
	else
	{
		AM_MEDIA_TYPE mt_in;
		memset(&mt_in, 0, sizeof(mt_in));
		HRESULT hr_mt = pIn->ConnectionMediaType(&mt_in);
		if (FAILED(hr_mt))
		{
			release_media_type(&mt_in);
			return VFW_E_NOT_CONNECTED;
		}
		mt_propose = mt_in;	//ownership transfer
	}

	HRESULT hr = pReceivePin->ReceiveConnection(this, &mt_propose);
	if (FAILED(hr))
	{
		release_media_type(&mt_propose);
		return hr;
	}

	free_media_type();
	m_mt = mt_propose;
	m_mt_set = true;

	m_pConnected = pReceivePin;
	m_pConnected->AddRef();

	//downstream input pin 의 IMemInputPin 캐시.
	if (m_pDownstreamInput) { m_pDownstreamInput->Release(); m_pDownstreamInput = NULL; }
	pReceivePin->QueryInterface(IID_IMemInputPin, (void**)&m_pDownstreamInput);

	//in-place transform — input pin 의 allocator 를 downstream 에게 알려준다.
	if (m_pDownstreamInput)
	{
		IMemAllocator* pAlloc = pIn->get_allocator();
		if (pAlloc)
			m_pDownstreamInput->NotifyAllocator(pAlloc, FALSE);
	}

	return S_OK;
}

STDMETHODIMP CSCAudioTransformOutputPin::ReceiveConnection(IPin*, const AM_MEDIA_TYPE*) { return E_UNEXPECTED; }	//output only

STDMETHODIMP CSCAudioTransformOutputPin::Disconnect()
{
	if (!m_pConnected) return S_FALSE;
	if (m_pDownstreamInput) { m_pDownstreamInput->Release(); m_pDownstreamInput = NULL; }
	m_pConnected->Release(); m_pConnected = NULL;
	free_media_type();
	return S_OK;
}

STDMETHODIMP CSCAudioTransformOutputPin::ConnectedTo(IPin** ppPin)
{
	if (!ppPin) return E_POINTER;
	if (!m_pConnected) { *ppPin = NULL; return VFW_E_NOT_CONNECTED; }
	m_pConnected->AddRef(); *ppPin = m_pConnected;
	return S_OK;
}
STDMETHODIMP CSCAudioTransformOutputPin::ConnectionMediaType(AM_MEDIA_TYPE* pmt)
{
	if (!pmt) return E_POINTER;
	if (!m_pConnected) return VFW_E_NOT_CONNECTED;
	copy_media_type(pmt, &m_mt);
	return S_OK;
}
STDMETHODIMP CSCAudioTransformOutputPin::QueryPinInfo(PIN_INFO* pInfo)
{
	if (!pInfo) return E_POINTER;
	pInfo->pFilter = m_pFilter;
	if (m_pFilter) m_pFilter->AddRef();
	pInfo->dir = PINDIR_OUTPUT;
	StringCchCopyW(pInfo->achName, sizeof(pInfo->achName)/sizeof(WCHAR), L"Out");
	return S_OK;
}
STDMETHODIMP CSCAudioTransformOutputPin::QueryDirection(PIN_DIRECTION* pPinDir) { if (!pPinDir) return E_POINTER; *pPinDir = PINDIR_OUTPUT; return S_OK; }
STDMETHODIMP CSCAudioTransformOutputPin::QueryId(LPWSTR* Id)
{
	if (!Id) return E_POINTER;
	*Id = (LPWSTR)CoTaskMemAlloc(sizeof(WCHAR) * 4);
	if (!*Id) return E_OUTOFMEMORY;
	wcscpy_s(*Id, 4, L"Out");
	return S_OK;
}
STDMETHODIMP CSCAudioTransformOutputPin::QueryAccept(const AM_MEDIA_TYPE* pmt)
{
	if (!pmt) return E_POINTER;
	return IsEqualGUID(pmt->majortype, MEDIATYPE_Audio) ? S_OK : S_FALSE;
}
STDMETHODIMP CSCAudioTransformOutputPin::EnumMediaTypes(IEnumMediaTypes** ppEnum)
{
	if (!ppEnum) return E_POINTER;
	CSCAudioEnumMediaTypes* p = new CSCAudioEnumMediaTypes();
	p->AddRef(); *ppEnum = p;
	return S_OK;
}
STDMETHODIMP CSCAudioTransformOutputPin::QueryInternalConnections(IPin** apPin, ULONG* nPin)
{
	if (!nPin) return E_POINTER;
	IPin* pIn = (IPin*)m_pFilter->get_input_pin();
	if (apPin && *nPin >= 1 && pIn) { pIn->AddRef(); apPin[0] = pIn; *nPin = 1; return S_OK; }
	*nPin = 1;
	return S_FALSE;
}
STDMETHODIMP CSCAudioTransformOutputPin::EndOfStream() { return E_UNEXPECTED; }	//upstream → input pin 으로만
STDMETHODIMP CSCAudioTransformOutputPin::BeginFlush()  { return E_UNEXPECTED; }
STDMETHODIMP CSCAudioTransformOutputPin::EndFlush()    { return E_UNEXPECTED; }
STDMETHODIMP CSCAudioTransformOutputPin::NewSegment(REFERENCE_TIME, REFERENCE_TIME, double) { return E_UNEXPECTED; }

HRESULT CSCAudioTransformOutputPin::deliver_sample(IMediaSample* pSample)
{
	return m_pDownstreamInput ? m_pDownstreamInput->Receive(pSample) : S_OK;
}
HRESULT CSCAudioTransformOutputPin::deliver_end_of_stream()
{
	return m_pConnected ? m_pConnected->EndOfStream() : S_OK;
}
HRESULT CSCAudioTransformOutputPin::deliver_begin_flush()
{
	return m_pConnected ? m_pConnected->BeginFlush() : S_OK;
}
HRESULT CSCAudioTransformOutputPin::deliver_end_flush()
{
	return m_pConnected ? m_pConnected->EndFlush() : S_OK;
}
HRESULT CSCAudioTransformOutputPin::deliver_new_segment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	return m_pConnected ? m_pConnected->NewSegment(tStart, tStop, dRate) : S_OK;
}
