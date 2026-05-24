#include "SCVideoTimeScale.h"
#include "../../log/SCLog/SCLog.h"
#include <strsafe.h>

#pragma comment(lib, "strmiids.lib")


//자체 enum (BaseClasses 의존 X) — CSCAudioTransformFilter 의 동등 클래스와 같은 패턴.
class CSCVideoEnumPins : public IEnumPins
{
	std::atomic<LONG>	m_ref;
	IPin*				m_pPin1;
	IPin*				m_pPin2;
	int					m_pos;
public:
	CSCVideoEnumPins(IPin* p1, IPin* p2) : m_ref(0), m_pPin1(p1), m_pPin2(p2), m_pos(0) {}
	virtual ~CSCVideoEnumPins() {}

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
		CSCVideoEnumPins* p = new CSCVideoEnumPins(m_pPin1, m_pPin2);
		p->m_pos = m_pos; p->AddRef(); *ppEnum = p;
		return S_OK;
	}
};


class CSCVideoEnumMediaTypes : public IEnumMediaTypes
{
	std::atomic<LONG>	m_ref;
public:
	CSCVideoEnumMediaTypes() : m_ref(0) {}
	virtual ~CSCVideoEnumMediaTypes() {}

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
		CSCVideoEnumMediaTypes* p = new CSCVideoEnumMediaTypes();
		p->AddRef(); *ppEnum = p;
		return S_OK;
	}
};


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


//================================================================================
// CSCVideoTimeScale
//================================================================================

CSCVideoTimeScale::CSCVideoTimeScale()
	: m_ref(0), m_state(State_Stopped), m_pGraph(NULL), m_pClock(NULL),
	  m_pInputPin(NULL), m_pOutputPin(NULL)
{
	StringCchCopyW(m_name, 64, L"SC Video Time-Scale");
	m_pInputPin  = new CSCVideoTimeScaleInputPin(this);  m_pInputPin->AddRef();
	m_pOutputPin = new CSCVideoTimeScaleOutputPin(this); m_pOutputPin->AddRef();
}

CSCVideoTimeScale::~CSCVideoTimeScale()
{
	if (m_pInputPin)  { m_pInputPin->Release();  m_pInputPin = NULL; }
	if (m_pOutputPin) { m_pOutputPin->Release(); m_pOutputPin = NULL; }
	if (m_pClock)     { m_pClock->Release();     m_pClock = NULL; }
}

STDMETHODIMP CSCVideoTimeScale::QueryInterface(REFIID iid, void** ppv)
{
	if (!ppv) return E_POINTER;
	if (iid == IID_IUnknown || iid == IID_IPersist || iid == IID_IMediaFilter || iid == IID_IBaseFilter)
	{
		*ppv = static_cast<IBaseFilter*>(this); AddRef();
		return S_OK;
	}
	//IMediaSeeking 미구현 — graph 의 SetRate broadcast 시 우리 응답 안 함. set_rate 로만 적용.
	*ppv = NULL;
	return E_NOINTERFACE;
}
STDMETHODIMP_(ULONG) CSCVideoTimeScale::AddRef() { return ++m_ref; }
STDMETHODIMP_(ULONG) CSCVideoTimeScale::Release() { LONG n = --m_ref; if (n == 0) delete this; return n; }

STDMETHODIMP CSCVideoTimeScale::GetClassID(CLSID* pClsID)
{
	if (!pClsID) return E_POINTER;
	memset(pClsID, 0, sizeof(CLSID));
	return S_OK;
}

STDMETHODIMP CSCVideoTimeScale::Stop()  { m_state = State_Stopped; return S_OK; }
STDMETHODIMP CSCVideoTimeScale::Pause() { m_state = State_Paused;  return S_OK; }
STDMETHODIMP CSCVideoTimeScale::Run(REFERENCE_TIME) { m_state = State_Running; return S_OK; }
STDMETHODIMP CSCVideoTimeScale::GetState(DWORD, FILTER_STATE* pState)
{
	if (!pState) return E_POINTER;
	*pState = m_state; return S_OK;
}
STDMETHODIMP CSCVideoTimeScale::SetSyncSource(IReferenceClock* pClock)
{
	if (m_pClock) m_pClock->Release();
	m_pClock = pClock;
	if (m_pClock) m_pClock->AddRef();
	return S_OK;
}
STDMETHODIMP CSCVideoTimeScale::GetSyncSource(IReferenceClock** ppClock)
{
	if (!ppClock) return E_POINTER;
	*ppClock = m_pClock;
	if (m_pClock) m_pClock->AddRef();
	return S_OK;
}

STDMETHODIMP CSCVideoTimeScale::EnumPins(IEnumPins** ppEnum)
{
	if (!ppEnum) return E_POINTER;
	CSCVideoEnumPins* p = new CSCVideoEnumPins(m_pInputPin, m_pOutputPin);
	p->AddRef(); *ppEnum = p;
	return S_OK;
}
STDMETHODIMP CSCVideoTimeScale::FindPin(LPCWSTR Id, IPin** ppPin)
{
	if (!ppPin || !Id) return E_POINTER;
	if (wcscmp(Id, L"In")  == 0) { m_pInputPin->AddRef();  *ppPin = m_pInputPin;  return S_OK; }
	if (wcscmp(Id, L"Out") == 0) { m_pOutputPin->AddRef(); *ppPin = m_pOutputPin; return S_OK; }
	*ppPin = NULL;
	return VFW_E_NOT_FOUND;
}
STDMETHODIMP CSCVideoTimeScale::QueryFilterInfo(FILTER_INFO* pInfo)
{
	if (!pInfo) return E_POINTER;
	StringCchCopyW(pInfo->achName, sizeof(pInfo->achName)/sizeof(WCHAR), m_name);
	pInfo->pGraph = m_pGraph;
	if (m_pGraph) m_pGraph->AddRef();
	return S_OK;
}
STDMETHODIMP CSCVideoTimeScale::JoinFilterGraph(IFilterGraph* pGraph, LPCWSTR pName)
{
	m_pGraph = pGraph;
	if (pName) StringCchCopyW(m_name, 64, pName);
	return S_OK;
}
STDMETHODIMP CSCVideoTimeScale::QueryVendorInfo(LPWSTR*) { return E_NOTIMPL; }


//================================================================================
// CSCVideoTimeScaleInputPin
//================================================================================

CSCVideoTimeScaleInputPin::CSCVideoTimeScaleInputPin(CSCVideoTimeScale* pFilter)
	: m_ref(0), m_pFilter(pFilter), m_pConnected(NULL), m_mt_set(false), m_pAllocator(NULL)
{
	memset(&m_mt, 0, sizeof(m_mt));
}

CSCVideoTimeScaleInputPin::~CSCVideoTimeScaleInputPin()
{
	if (m_pConnected) m_pConnected->Release();
	if (m_pAllocator) m_pAllocator->Release();
	free_media_type();
}

void CSCVideoTimeScaleInputPin::free_media_type()
{
	if (!m_mt_set) return;
	if (m_mt.cbFormat && m_mt.pbFormat) { CoTaskMemFree(m_mt.pbFormat); m_mt.pbFormat = NULL; m_mt.cbFormat = 0; }
	if (m_mt.pUnk) { m_mt.pUnk->Release(); m_mt.pUnk = NULL; }
	m_mt_set = false;
}

STDMETHODIMP CSCVideoTimeScaleInputPin::QueryInterface(REFIID iid, void** ppv)
{
	if (!ppv) return E_POINTER;
	if (iid == IID_IUnknown || iid == IID_IPin) { *ppv = static_cast<IPin*>(this); AddRef(); return S_OK; }
	if (iid == IID_IMemInputPin) { *ppv = static_cast<IMemInputPin*>(this); AddRef(); return S_OK; }
	*ppv = NULL;
	return E_NOINTERFACE;
}
STDMETHODIMP_(ULONG) CSCVideoTimeScaleInputPin::AddRef() { return ++m_ref; }
STDMETHODIMP_(ULONG) CSCVideoTimeScaleInputPin::Release() { LONG n = --m_ref; if (n == 0) delete this; return n; }

STDMETHODIMP CSCVideoTimeScaleInputPin::Connect(IPin*, const AM_MEDIA_TYPE*) { return E_UNEXPECTED; }	//input only

STDMETHODIMP CSCVideoTimeScaleInputPin::ReceiveConnection(IPin* pConnector, const AM_MEDIA_TYPE* pmt)
{
	if (!pConnector || !pmt) return E_POINTER;
	if (m_pConnected) return VFW_E_ALREADY_CONNECTED;

	//video 만 accept.
	if (!IsEqualGUID(pmt->majortype, MEDIATYPE_Video))
		return VFW_E_TYPE_NOT_ACCEPTED;

	free_media_type();
	copy_media_type(&m_mt, pmt);
	m_mt_set = true;

	m_pConnected = pConnector;
	m_pConnected->AddRef();
	logWrite(_T("[VideoTS] ReceiveConnection major.D1=%08lX sub.D1=%08lX cbFmt=%lu"),
		pmt->majortype.Data1, pmt->subtype.Data1, pmt->cbFormat);
	return S_OK;
}

STDMETHODIMP CSCVideoTimeScaleInputPin::Disconnect()
{
	if (!m_pConnected) return S_FALSE;
	m_pConnected->Release(); m_pConnected = NULL;
	free_media_type();
	return S_OK;
}

STDMETHODIMP CSCVideoTimeScaleInputPin::ConnectedTo(IPin** ppPin)
{
	if (!ppPin) return E_POINTER;
	*ppPin = m_pConnected;
	if (m_pConnected) { m_pConnected->AddRef(); return S_OK; }
	return VFW_E_NOT_CONNECTED;
}

STDMETHODIMP CSCVideoTimeScaleInputPin::ConnectionMediaType(AM_MEDIA_TYPE* pmt)
{
	if (!pmt) return E_POINTER;
	if (!m_mt_set) return VFW_E_NOT_CONNECTED;
	copy_media_type(pmt, &m_mt);
	return S_OK;
}

STDMETHODIMP CSCVideoTimeScaleInputPin::QueryPinInfo(PIN_INFO* pInfo)
{
	if (!pInfo) return E_POINTER;
	StringCchCopyW(pInfo->achName, sizeof(pInfo->achName)/sizeof(WCHAR), L"In");
	pInfo->dir = PINDIR_INPUT;
	pInfo->pFilter = m_pFilter;
	if (m_pFilter) m_pFilter->AddRef();
	return S_OK;
}
STDMETHODIMP CSCVideoTimeScaleInputPin::QueryDirection(PIN_DIRECTION* pPinDir)
{
	if (!pPinDir) return E_POINTER; *pPinDir = PINDIR_INPUT; return S_OK;
}
STDMETHODIMP CSCVideoTimeScaleInputPin::QueryId(LPWSTR* Id)
{
	if (!Id) return E_POINTER;
	*Id = (LPWSTR)CoTaskMemAlloc(sizeof(WCHAR) * 4);
	if (!*Id) return E_OUTOFMEMORY;
	StringCchCopyW(*Id, 4, L"In");
	return S_OK;
}
STDMETHODIMP CSCVideoTimeScaleInputPin::QueryAccept(const AM_MEDIA_TYPE* pmt)
{
	if (!pmt) return E_POINTER;
	return IsEqualGUID(pmt->majortype, MEDIATYPE_Video) ? S_OK : S_FALSE;
}
STDMETHODIMP CSCVideoTimeScaleInputPin::EnumMediaTypes(IEnumMediaTypes** ppEnum)
{
	if (!ppEnum) return E_POINTER;
	CSCVideoEnumMediaTypes* p = new CSCVideoEnumMediaTypes();
	p->AddRef(); *ppEnum = p;
	return S_OK;
}
STDMETHODIMP CSCVideoTimeScaleInputPin::QueryInternalConnections(IPin** apPin, ULONG* nPin)
{
	if (!nPin) return E_POINTER;
	if (!apPin) { *nPin = 1; return S_OK; }
	if (*nPin == 0) { *nPin = 1; return E_NOT_SUFFICIENT_BUFFER; }
	apPin[0] = m_pFilter ? (IPin*)m_pFilter->get_output_pin() : NULL;
	if (apPin[0]) apPin[0]->AddRef();
	*nPin = 1;
	return S_OK;
}

STDMETHODIMP CSCVideoTimeScaleInputPin::EndOfStream()
{
	if (m_pFilter && m_pFilter->get_output_pin())
		return m_pFilter->get_output_pin()->deliver_end_of_stream();
	return S_OK;
}
STDMETHODIMP CSCVideoTimeScaleInputPin::BeginFlush()
{
	if (m_pFilter && m_pFilter->get_output_pin())
		return m_pFilter->get_output_pin()->deliver_begin_flush();
	return S_OK;
}
STDMETHODIMP CSCVideoTimeScaleInputPin::EndFlush()
{
	if (m_pFilter && m_pFilter->get_output_pin())
		return m_pFilter->get_output_pin()->deliver_end_flush();
	return S_OK;
}
STDMETHODIMP CSCVideoTimeScaleInputPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	//segment 의 tStart/tStop 도 1/rate scale — sample.tStart 와 일관.
	double rate = m_pFilter ? m_pFilter->get_rate() : 1.0;
	if (rate > 0.0 && rate != 1.0)
	{
		tStart = (REFERENCE_TIME)((double)tStart / rate);
		tStop  = (REFERENCE_TIME)((double)tStop  / rate);
	}
	if (m_pFilter && m_pFilter->get_output_pin())
		return m_pFilter->get_output_pin()->deliver_new_segment(tStart, tStop, dRate);
	return S_OK;
}

STDMETHODIMP CSCVideoTimeScaleInputPin::GetAllocator(IMemAllocator** ppAllocator)
{
	if (!ppAllocator) return E_POINTER;
	//우리는 자체 allocator 안 제공 — upstream 의 allocator 사용 (downstream 에 forward).
	*ppAllocator = NULL;
	return VFW_E_NO_ALLOCATOR;
}
STDMETHODIMP CSCVideoTimeScaleInputPin::NotifyAllocator(IMemAllocator* pAllocator, BOOL)
{
	if (m_pAllocator) m_pAllocator->Release();
	m_pAllocator = pAllocator;
	if (m_pAllocator) m_pAllocator->AddRef();
	return S_OK;
}
STDMETHODIMP CSCVideoTimeScaleInputPin::GetAllocatorRequirements(ALLOCATOR_PROPERTIES*) { return E_NOTIMPL; }

STDMETHODIMP CSCVideoTimeScaleInputPin::Receive(IMediaSample* pSample)
{
	if (!pSample) return E_POINTER;
	if (!m_pFilter || m_pFilter->get_state() == State_Stopped) return VFW_E_WRONG_STATE;

	//timestamp scale — pSample 의 tStart/tStop 을 1/rate 로. PCM data 변경 X (zero-copy pass-through).
	double rate = m_pFilter->get_rate();
	if (rate > 0.0 && rate != 1.0)
	{
		REFERENCE_TIME tStart = 0, tStop = 0;
		if (SUCCEEDED(pSample->GetTime(&tStart, &tStop)))
		{
			tStart = (REFERENCE_TIME)((double)tStart / rate);
			tStop  = (REFERENCE_TIME)((double)tStop  / rate);
			pSample->SetTime(&tStart, &tStop);
		}
	}

	if (m_pFilter->get_output_pin())
		return m_pFilter->get_output_pin()->deliver_sample(pSample);

	return E_FAIL;
}
STDMETHODIMP CSCVideoTimeScaleInputPin::ReceiveMultiple(IMediaSample** pSamples, long nSamples, long* nSamplesProcessed)
{
	if (!pSamples || !nSamplesProcessed) return E_POINTER;
	*nSamplesProcessed = 0;
	for (long i = 0; i < nSamples; ++i)
	{
		HRESULT hr = Receive(pSamples[i]);
		if (FAILED(hr)) return hr;
		(*nSamplesProcessed)++;
	}
	return S_OK;
}
STDMETHODIMP CSCVideoTimeScaleInputPin::ReceiveCanBlock() { return S_FALSE; }	//우리는 block 안 함


//================================================================================
// CSCVideoTimeScaleOutputPin
//================================================================================

CSCVideoTimeScaleOutputPin::CSCVideoTimeScaleOutputPin(CSCVideoTimeScale* pFilter)
	: m_ref(0), m_pFilter(pFilter), m_pConnected(NULL), m_pDownstreamInput(NULL), m_mt_set(false)
{
	memset(&m_mt, 0, sizeof(m_mt));
}

CSCVideoTimeScaleOutputPin::~CSCVideoTimeScaleOutputPin()
{
	if (m_pConnected) m_pConnected->Release();
	if (m_pDownstreamInput) m_pDownstreamInput->Release();
	free_media_type();
}

void CSCVideoTimeScaleOutputPin::free_media_type()
{
	if (!m_mt_set) return;
	if (m_mt.cbFormat && m_mt.pbFormat) { CoTaskMemFree(m_mt.pbFormat); m_mt.pbFormat = NULL; m_mt.cbFormat = 0; }
	if (m_mt.pUnk) { m_mt.pUnk->Release(); m_mt.pUnk = NULL; }
	m_mt_set = false;
}

STDMETHODIMP CSCVideoTimeScaleOutputPin::QueryInterface(REFIID iid, void** ppv)
{
	if (!ppv) return E_POINTER;
	if (iid == IID_IUnknown || iid == IID_IPin) { *ppv = static_cast<IPin*>(this); AddRef(); return S_OK; }
	*ppv = NULL;
	return E_NOINTERFACE;
}
STDMETHODIMP_(ULONG) CSCVideoTimeScaleOutputPin::AddRef() { return ++m_ref; }
STDMETHODIMP_(ULONG) CSCVideoTimeScaleOutputPin::Release() { LONG n = --m_ref; if (n == 0) delete this; return n; }

STDMETHODIMP CSCVideoTimeScaleOutputPin::Connect(IPin* pReceivePin, const AM_MEDIA_TYPE* pmt)
{
	if (!pReceivePin) return E_POINTER;
	if (m_pConnected) return VFW_E_ALREADY_CONNECTED;

	//upstream connect 의 media type 사용 — pmt 가 NULL 이면 input pin 의 mt 사용.
	AM_MEDIA_TYPE use_mt;
	memset(&use_mt, 0, sizeof(use_mt));
	bool use_mt_alloc = false;

	const AM_MEDIA_TYPE* p_use = pmt;
	if (!p_use && m_pFilter && m_pFilter->get_input_pin())
	{
		if (SUCCEEDED(m_pFilter->get_input_pin()->ConnectionMediaType(&use_mt)))
		{
			p_use = &use_mt;
			use_mt_alloc = true;
		}
	}
	if (!p_use) return VFW_E_TYPE_NOT_ACCEPTED;

	HRESULT hr = pReceivePin->ReceiveConnection(static_cast<IPin*>(this), p_use);
	if (SUCCEEDED(hr))
	{
		m_pConnected = pReceivePin;
		m_pConnected->AddRef();

		free_media_type();
		copy_media_type(&m_mt, p_use);
		m_mt_set = true;

		//downstream 의 IMemInputPin cache — fast Receive.
		if (m_pDownstreamInput) { m_pDownstreamInput->Release(); m_pDownstreamInput = NULL; }
		pReceivePin->QueryInterface(IID_IMemInputPin, (void**)&m_pDownstreamInput);

		//downstream 의 allocator 를 input pin 에 forward — upstream 이 downstream allocator 직접 사용.
		if (m_pDownstreamInput)
		{
			IMemAllocator* pAlloc = NULL;
			if (SUCCEEDED(m_pDownstreamInput->GetAllocator(&pAlloc)) && pAlloc)
			{
				if (m_pFilter && m_pFilter->get_input_pin())
					m_pFilter->get_input_pin()->NotifyAllocator(pAlloc, FALSE);
				m_pDownstreamInput->NotifyAllocator(pAlloc, FALSE);
				pAlloc->Release();
			}
		}
	}

	if (use_mt_alloc)
	{
		if (use_mt.cbFormat && use_mt.pbFormat) CoTaskMemFree(use_mt.pbFormat);
		if (use_mt.pUnk) use_mt.pUnk->Release();
	}

	return hr;
}
STDMETHODIMP CSCVideoTimeScaleOutputPin::ReceiveConnection(IPin*, const AM_MEDIA_TYPE*) { return E_UNEXPECTED; }	//output only

STDMETHODIMP CSCVideoTimeScaleOutputPin::Disconnect()
{
	if (!m_pConnected) return S_FALSE;
	m_pConnected->Release(); m_pConnected = NULL;
	if (m_pDownstreamInput) { m_pDownstreamInput->Release(); m_pDownstreamInput = NULL; }
	free_media_type();
	return S_OK;
}

STDMETHODIMP CSCVideoTimeScaleOutputPin::ConnectedTo(IPin** ppPin)
{
	if (!ppPin) return E_POINTER;
	*ppPin = m_pConnected;
	if (m_pConnected) { m_pConnected->AddRef(); return S_OK; }
	return VFW_E_NOT_CONNECTED;
}

STDMETHODIMP CSCVideoTimeScaleOutputPin::ConnectionMediaType(AM_MEDIA_TYPE* pmt)
{
	if (!pmt) return E_POINTER;
	if (!m_mt_set) return VFW_E_NOT_CONNECTED;
	copy_media_type(pmt, &m_mt);
	return S_OK;
}

STDMETHODIMP CSCVideoTimeScaleOutputPin::QueryPinInfo(PIN_INFO* pInfo)
{
	if (!pInfo) return E_POINTER;
	StringCchCopyW(pInfo->achName, sizeof(pInfo->achName)/sizeof(WCHAR), L"Out");
	pInfo->dir = PINDIR_OUTPUT;
	pInfo->pFilter = m_pFilter;
	if (m_pFilter) m_pFilter->AddRef();
	return S_OK;
}
STDMETHODIMP CSCVideoTimeScaleOutputPin::QueryDirection(PIN_DIRECTION* pPinDir)
{
	if (!pPinDir) return E_POINTER; *pPinDir = PINDIR_OUTPUT; return S_OK;
}
STDMETHODIMP CSCVideoTimeScaleOutputPin::QueryId(LPWSTR* Id)
{
	if (!Id) return E_POINTER;
	*Id = (LPWSTR)CoTaskMemAlloc(sizeof(WCHAR) * 4);
	if (!*Id) return E_OUTOFMEMORY;
	StringCchCopyW(*Id, 4, L"Out");
	return S_OK;
}
STDMETHODIMP CSCVideoTimeScaleOutputPin::QueryAccept(const AM_MEDIA_TYPE* pmt)
{
	if (!pmt) return E_POINTER;
	return IsEqualGUID(pmt->majortype, MEDIATYPE_Video) ? S_OK : S_FALSE;
}
STDMETHODIMP CSCVideoTimeScaleOutputPin::EnumMediaTypes(IEnumMediaTypes** ppEnum)
{
	if (!ppEnum) return E_POINTER;
	CSCVideoEnumMediaTypes* p = new CSCVideoEnumMediaTypes();
	p->AddRef(); *ppEnum = p;
	return S_OK;
}
STDMETHODIMP CSCVideoTimeScaleOutputPin::QueryInternalConnections(IPin** apPin, ULONG* nPin)
{
	if (!nPin) return E_POINTER;
	if (!apPin) { *nPin = 1; return S_OK; }
	if (*nPin == 0) { *nPin = 1; return E_NOT_SUFFICIENT_BUFFER; }
	apPin[0] = m_pFilter ? (IPin*)m_pFilter->get_input_pin() : NULL;
	if (apPin[0]) apPin[0]->AddRef();
	*nPin = 1;
	return S_OK;
}

//upstream → input pin 으로만. output pin 에서 직접 호출되는 케이스 없음.
STDMETHODIMP CSCVideoTimeScaleOutputPin::EndOfStream()
{
	return E_UNEXPECTED;
}
STDMETHODIMP CSCVideoTimeScaleOutputPin::BeginFlush()
{
	return E_UNEXPECTED;
}
STDMETHODIMP CSCVideoTimeScaleOutputPin::EndFlush()
{
	return E_UNEXPECTED;
}
STDMETHODIMP CSCVideoTimeScaleOutputPin::NewSegment(REFERENCE_TIME, REFERENCE_TIME, double)
{
	return E_UNEXPECTED;
}

HRESULT CSCVideoTimeScaleOutputPin::deliver_sample(IMediaSample* pSample)
{
	return m_pDownstreamInput ? m_pDownstreamInput->Receive(pSample) : E_FAIL;
}
HRESULT CSCVideoTimeScaleOutputPin::deliver_end_of_stream()
{
	return m_pConnected ? m_pConnected->EndOfStream() : S_OK;
}
HRESULT CSCVideoTimeScaleOutputPin::deliver_begin_flush()
{
	return m_pConnected ? m_pConnected->BeginFlush() : S_OK;
}
HRESULT CSCVideoTimeScaleOutputPin::deliver_end_flush()
{
	return m_pConnected ? m_pConnected->EndFlush() : S_OK;
}
HRESULT CSCVideoTimeScaleOutputPin::deliver_new_segment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	return m_pConnected ? m_pConnected->NewSegment(tStart, tStop, dRate) : S_OK;
}
