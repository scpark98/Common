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
	  m_pInputPin(NULL), m_pOutputPin(NULL), m_wfx_set(false)
{
	memset(&m_wfx_ext, 0, sizeof(m_wfx_ext));
	if (friendly_name) StringCchCopyW(m_name, 64, friendly_name);
	else m_name[0] = 0;

	m_pInputPin  = new CSCAudioTransformInputPin(this);  m_pInputPin->AddRef();
	m_pOutputPin = new CSCAudioTransformOutputPin(this); m_pOutputPin->AddRef();
}

CSCAudioTransformFilter::~CSCAudioTransformFilter()
{
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

STDMETHODIMP CSCAudioTransformFilter::Stop()  { m_state = State_Stopped; return S_OK; }
STDMETHODIMP CSCAudioTransformFilter::Pause() { m_state = State_Paused;  return S_OK; }
STDMETHODIMP CSCAudioTransformFilter::Run(REFERENCE_TIME) { m_state = State_Running; return S_OK; }
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
// CSCAudioTransformInputPin
//================================================================================

CSCAudioTransformInputPin::CSCAudioTransformInputPin(CSCAudioTransformFilter* pFilter)
	: m_ref(0), m_pFilter(pFilter), m_pConnected(NULL), m_mt_set(false), m_pAllocator(NULL)
{
	memset(&m_mt, 0, sizeof(m_mt));
}

CSCAudioTransformInputPin::~CSCAudioTransformInputPin()
{
	if (m_pConnected) m_pConnected->Release();
	if (m_pAllocator) m_pAllocator->Release();
	free_media_type();
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
		logWrite(_T("[AudioFilter] ReceiveConnection wfx tag=0x%04X ch=%d sample_rate=%lu bps=%d cbSize=%d cbFmt=%lu"),
			wfx->wFormatTag, wfx->nChannels, wfx->nSamplesPerSec, wfx->wBitsPerSample, wfx->cbSize, pmt->cbFormat);
		if (wfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE && pmt->cbFormat >= sizeof(WAVEFORMATEXTENSIBLE))
		{
			const WAVEFORMATEXTENSIBLE* wfex = (const WAVEFORMATEXTENSIBLE*)pmt->pbFormat;
			logWrite(_T("[AudioFilter] WFEX SubFormat.D1=%08lX validBits=%d ChMask=0x%lX"),
				wfex->SubFormat.Data1, wfex->Samples.wValidBitsPerSample, wfex->dwChannelMask);
		}
	}
	else
	{
		logWrite(_T("[AudioFilter] ReceiveConnection no WAVEFORMATEX (formattype.D1=%08lX cbFmt=%lu)"),
			pmt->formattype.Data1, pmt->cbFormat);
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

STDMETHODIMP CSCAudioTransformInputPin::EndOfStream()
{
	return m_pFilter->get_output_pin()->deliver_end_of_stream();
}
STDMETHODIMP CSCAudioTransformInputPin::BeginFlush()
{
	if (m_pAllocator) m_pAllocator->Decommit();
	return m_pFilter->get_output_pin()->deliver_begin_flush();
}
STDMETHODIMP CSCAudioTransformInputPin::EndFlush()
{
	if (m_pAllocator) m_pAllocator->Commit();
	return m_pFilter->get_output_pin()->deliver_end_flush();
}
STDMETHODIMP CSCAudioTransformInputPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	return m_pFilter->get_output_pin()->deliver_new_segment(tStart, tStop, dRate);
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

STDMETHODIMP CSCAudioTransformInputPin::Receive(IMediaSample* pSample)
{
	if (!pSample) return E_POINTER;

	BYTE* pBuf = NULL;
	pSample->GetPointer(&pBuf);
	long len = pSample->GetActualDataLength();

	//파생이 sample buffer 를 in-place 수정.
	if (pBuf && len > 0)
	{
		const WAVEFORMATEX* pwfx = m_pFilter->get_wave_format();
		static int s_trace_count = 0;
		if (s_trace_count < 3)
		{
			logWrite(_T("[AudioFilter] Receive #%d len=%ld wfx=%p tag=0x%04X bps=%d"),
				s_trace_count, len, pwfx, pwfx ? pwfx->wFormatTag : 0, pwfx ? pwfx->wBitsPerSample : 0);
			s_trace_count++;
		}
		m_pFilter->process_sample(pBuf, len, pwfx);
	}

	//downstream 으로 forward (같은 IMediaSample 객체 그대로).
	return m_pFilter->get_output_pin()->deliver_sample(pSample);
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
	//downstream 의 ReceiveCanBlock 결과 반환.
	IMemInputPin* pDown = m_pFilter->get_output_pin()->get_downstream_input();
	return pDown ? pDown->ReceiveCanBlock() : S_FALSE;
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
