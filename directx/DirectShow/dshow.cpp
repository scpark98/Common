#include "dshow.h"
#include "SCAudioGain.h"
#include "SCAudioCompressor.h"
#include "../../log/SCLog/SCLog.h"
#include <gdiplus.h>


#pragma comment(lib, "strmiids.lib")

/*
#include "TextDesigner/OutlineText.h"

#ifdef _DEBUG
#pragma comment(lib, "d:/1.project/Common/DirectShow/TextDesigner/TextDesignerd.lib")
//상대 경로는 안먹히는 듯하다.
//#pragma comment(lib, "./TextDesigner/TextDesignerd.lib")
#else
#pragma comment(lib, "d:/1.project/Common/DirectShow/TextDesigner/TextDesigner.lib")
#endif
*/

#define REGISTER_FILTERGRAPH

DWORD     g_dwGraphRegister=0;

//MEDIATYPE_Subtitle 은 Windows SDK 의 uuids.h 에 없고 LAV/Haali/MPC splitter SDK 들이 공통으로 사용하는 GUID.
//{e487eb08-6b26-4be9-9dd3-4434f6db9dfe}
#ifndef MEDIATYPE_Subtitle
static const GUID MEDIATYPE_Subtitle =
{ 0xe487eb08, 0x6b26, 0x4be9, { 0x9d, 0xd3, 0x44, 0x34, 0xf6, 0xdb, 0x9d, 0xfe } };
#endif

//Custom IBaseFilter sink — Microsoft SampleGrabber 가 video/audio majortype 만 받기 때문에
//자막 stream 을 직접 받을 수 없어 자체 sink filter 를 작성한다.
//단일 input pin (IPin + IMemInputPin) + 단순 IEnumPins / IEnumMediaTypes stub.

class CSubtitleEnumPins : public IEnumPins
{
	LONG m_ref;
	IPin* m_pPin;
	int m_pos;
public:
	CSubtitleEnumPins(IPin* pPin) : m_ref(0), m_pPin(pPin), m_pos(0) { if (pPin) pPin->AddRef(); }
	~CSubtitleEnumPins() { if (m_pPin) m_pPin->Release(); }

	STDMETHODIMP QueryInterface(REFIID iid, void** ppv) override {
		if (iid == IID_IUnknown || iid == IID_IEnumPins) { *ppv = static_cast<IEnumPins*>(this); AddRef(); return S_OK; }
		*ppv = NULL; return E_NOINTERFACE;
	}
	STDMETHODIMP_(ULONG) AddRef() override { return InterlockedIncrement(&m_ref); }
	STDMETHODIMP_(ULONG) Release() override { ULONG n = InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }

	STDMETHODIMP Next(ULONG cPins, IPin** ppPins, ULONG* pcFetched) override {
		ULONG fetched = 0;
		if (m_pos == 0 && cPins >= 1 && m_pPin) {
			m_pPin->AddRef();
			ppPins[0] = m_pPin;
			fetched = 1;
			m_pos = 1;
		}
		if (pcFetched) *pcFetched = fetched;
		return (fetched == cPins) ? S_OK : S_FALSE;
	}
	STDMETHODIMP Skip(ULONG cPins) override { m_pos += cPins; return S_OK; }
	STDMETHODIMP Reset() override { m_pos = 0; return S_OK; }
	STDMETHODIMP Clone(IEnumPins** ppEnum) override {
		if (!ppEnum) return E_POINTER;
		CSubtitleEnumPins* p = new CSubtitleEnumPins(m_pPin);
		p->m_pos = m_pos;
		p->AddRef();
		*ppEnum = p;
		return S_OK;
	}
};

class CSubtitleEnumMediaTypes : public IEnumMediaTypes
{
	LONG m_ref;
public:
	CSubtitleEnumMediaTypes() : m_ref(0) {}

	STDMETHODIMP QueryInterface(REFIID iid, void** ppv) override {
		if (iid == IID_IUnknown || iid == IID_IEnumMediaTypes) { *ppv = static_cast<IEnumMediaTypes*>(this); AddRef(); return S_OK; }
		*ppv = NULL; return E_NOINTERFACE;
	}
	STDMETHODIMP_(ULONG) AddRef() override { return InterlockedIncrement(&m_ref); }
	STDMETHODIMP_(ULONG) Release() override { ULONG n = InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }

	STDMETHODIMP Next(ULONG, AM_MEDIA_TYPE**, ULONG* pcFetched) override { if (pcFetched) *pcFetched = 0; return S_FALSE; }
	STDMETHODIMP Skip(ULONG) override { return S_OK; }
	STDMETHODIMP Reset() override { return S_OK; }
	STDMETHODIMP Clone(IEnumMediaTypes** ppEnum) override {
		if (!ppEnum) return E_POINTER;
		CSubtitleEnumMediaTypes* p = new CSubtitleEnumMediaTypes();
		p->AddRef();
		*ppEnum = p;
		return S_OK;
	}
};

class CSubtitleSinkPin : public IPin, public IMemInputPin
{
	LONG m_ref;
	IBaseFilter* m_pFilter;	//weak — filter 가 pin 보다 먼저 destruct 안 됨
	IPin* m_pConnected;
	AM_MEDIA_TYPE m_mt;
	bool m_mt_set;
	HWND m_target;
	UINT m_msg;
	IMemAllocator* m_pAllocator;
	REFERENCE_TIME m_segment_start;	//NewSegment.tStart — Receive 의 sample timestamp 가 segment-relative 라 add 해야 absolute stream time
public:
	CSubtitleSinkPin(IBaseFilter* pFilter, HWND target, UINT msg)
		: m_ref(0), m_pFilter(pFilter), m_pConnected(NULL), m_mt_set(false),
		  m_target(target), m_msg(msg), m_pAllocator(NULL), m_segment_start(0) {
		ZeroMemory(&m_mt, sizeof(m_mt));
	}
	~CSubtitleSinkPin() {
		if (m_pConnected) m_pConnected->Release();
		if (m_pAllocator) m_pAllocator->Release();
		if (m_mt_set) {
			if (m_mt.cbFormat && m_mt.pbFormat) CoTaskMemFree(m_mt.pbFormat);
			if (m_mt.pUnk) m_mt.pUnk->Release();
		}
	}

	GUID get_subtype() {
		if (m_mt_set) return m_mt.subtype;
		GUID g; ZeroMemory(&g, sizeof(g)); return g;
	}

	//IUnknown
	STDMETHODIMP QueryInterface(REFIID iid, void** ppv) override {
		if (iid == IID_IUnknown || iid == IID_IPin) { *ppv = static_cast<IPin*>(this); AddRef(); return S_OK; }
		if (iid == IID_IMemInputPin) { *ppv = static_cast<IMemInputPin*>(this); AddRef(); return S_OK; }
		*ppv = NULL; return E_NOINTERFACE;
	}
	STDMETHODIMP_(ULONG) AddRef() override { return InterlockedIncrement(&m_ref); }
	STDMETHODIMP_(ULONG) Release() override { ULONG n = InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }

	//IPin
	STDMETHODIMP Connect(IPin*, const AM_MEDIA_TYPE*) override { return E_UNEXPECTED; }
	STDMETHODIMP ReceiveConnection(IPin* pConnector, const AM_MEDIA_TYPE* pmt) override {
		if (!pConnector || !pmt) return E_POINTER;
		if (m_pConnected) return VFW_E_ALREADY_CONNECTED;
		if (!IsEqualGUID(pmt->majortype, MEDIATYPE_Subtitle) && pmt->majortype.Data1 != 0xE487EB08)
			return VFW_E_TYPE_NOT_ACCEPTED;
		m_mt = *pmt;
		if (pmt->cbFormat && pmt->pbFormat) {
			m_mt.pbFormat = (BYTE*)CoTaskMemAlloc(pmt->cbFormat);
			if (m_mt.pbFormat) memcpy(m_mt.pbFormat, pmt->pbFormat, pmt->cbFormat);
		}
		if (pmt->pUnk) pmt->pUnk->AddRef();
		m_mt_set = true;
		m_pConnected = pConnector;
		m_pConnected->AddRef();
		return S_OK;
	}
	STDMETHODIMP Disconnect() override {
		if (!m_pConnected) return S_FALSE;
		m_pConnected->Release(); m_pConnected = NULL;
		if (m_mt_set) {
			if (m_mt.cbFormat && m_mt.pbFormat) CoTaskMemFree(m_mt.pbFormat);
			if (m_mt.pUnk) m_mt.pUnk->Release();
			ZeroMemory(&m_mt, sizeof(m_mt));
			m_mt_set = false;
		}
		return S_OK;
	}
	STDMETHODIMP ConnectedTo(IPin** ppPin) override {
		if (!ppPin) return E_POINTER;
		if (!m_pConnected) { *ppPin = NULL; return VFW_E_NOT_CONNECTED; }
		m_pConnected->AddRef(); *ppPin = m_pConnected;
		return S_OK;
	}
	STDMETHODIMP ConnectionMediaType(AM_MEDIA_TYPE* pmt) override {
		if (!pmt) return E_POINTER;
		if (!m_pConnected) return VFW_E_NOT_CONNECTED;
		*pmt = m_mt;
		if (m_mt.cbFormat && m_mt.pbFormat) {
			pmt->pbFormat = (BYTE*)CoTaskMemAlloc(m_mt.cbFormat);
			if (pmt->pbFormat) memcpy(pmt->pbFormat, m_mt.pbFormat, m_mt.cbFormat);
		}
		if (m_mt.pUnk) m_mt.pUnk->AddRef();
		return S_OK;
	}
	STDMETHODIMP QueryPinInfo(PIN_INFO* pInfo) override {
		if (!pInfo) return E_POINTER;
		pInfo->pFilter = m_pFilter;
		if (m_pFilter) m_pFilter->AddRef();
		pInfo->dir = PINDIR_INPUT;
		wcscpy_s(pInfo->achName, L"In");
		return S_OK;
	}
	STDMETHODIMP QueryDirection(PIN_DIRECTION* pPinDir) override {
		if (!pPinDir) return E_POINTER;
		*pPinDir = PINDIR_INPUT; return S_OK;
	}
	STDMETHODIMP QueryId(LPWSTR* Id) override {
		if (!Id) return E_POINTER;
		*Id = (LPWSTR)CoTaskMemAlloc(sizeof(WCHAR) * 3);
		if (!*Id) return E_OUTOFMEMORY;
		wcscpy_s(*Id, 3, L"In");
		return S_OK;
	}
	STDMETHODIMP QueryAccept(const AM_MEDIA_TYPE* pmt) override {
		if (!pmt) return E_POINTER;
		if (IsEqualGUID(pmt->majortype, MEDIATYPE_Subtitle) || pmt->majortype.Data1 == 0xE487EB08)
			return S_OK;
		return S_FALSE;
	}
	STDMETHODIMP EnumMediaTypes(IEnumMediaTypes** ppEnum) override {
		if (!ppEnum) return E_POINTER;
		CSubtitleEnumMediaTypes* p = new CSubtitleEnumMediaTypes();
		p->AddRef();
		*ppEnum = p;
		return S_OK;
	}
	STDMETHODIMP QueryInternalConnections(IPin**, ULONG* pcCount) override {
		if (pcCount) *pcCount = 0; return E_NOTIMPL;
	}
	STDMETHODIMP EndOfStream() override { return S_OK; }
	STDMETHODIMP BeginFlush() override {
		if (m_pAllocator) m_pAllocator->Decommit();
		return S_OK;
	}
	STDMETHODIMP EndFlush() override {
		if (m_pAllocator) m_pAllocator->Commit();
		return S_OK;
	}
	STDMETHODIMP NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME, double) override {
		m_segment_start = tStart;
		return S_OK;
	}

	//IMemInputPin
	STDMETHODIMP GetAllocator(IMemAllocator** ppAllocator) override {
		if (!ppAllocator) return E_POINTER;
		if (!m_pAllocator) {
			HRESULT hr = CoCreateInstance(CLSID_MemoryAllocator, NULL, CLSCTX_INPROC_SERVER,
				IID_IMemAllocator, (void**)&m_pAllocator);
			if (FAILED(hr)) return hr;
		}
		m_pAllocator->AddRef();
		*ppAllocator = m_pAllocator;
		return S_OK;
	}
	STDMETHODIMP NotifyAllocator(IMemAllocator* pAllocator, BOOL) override {
		if (m_pAllocator) m_pAllocator->Release();
		m_pAllocator = pAllocator;
		if (m_pAllocator) m_pAllocator->AddRef();
		return S_OK;
	}
	STDMETHODIMP GetAllocatorRequirements(ALLOCATOR_PROPERTIES*) override { return E_NOTIMPL; }
	STDMETHODIMP Receive(IMediaSample* pSample) override {
		if (!pSample) return E_POINTER;
		if (!::IsWindow(m_target)) return S_OK;

		BYTE* pBuf = NULL;
		pSample->GetPointer(&pBuf);
		long len = pSample->GetActualDataLength();
		if (!pBuf || len <= 0) return S_OK;

		REFERENCE_TIME rtStart = 0, rtEnd = 0;
		pSample->GetTime(&rtStart, &rtEnd);

		//sample timestamp 는 NewSegment.tStart 기준 segment-relative offset 이므로
		//absolute stream time 으로 변환하기 위해 m_segment_start 를 add 한다.
		REFERENCE_TIME absStart = rtStart + m_segment_start;
		REFERENCE_TIME absEnd   = rtEnd   + m_segment_start;

		if (absStart < 0 || absEnd <= absStart)
			return S_OK;

		rtStart = absStart;
		rtEnd   = absEnd;

		CDShow::SubtitlePacket* pkt = new CDShow::SubtitlePacket;
		pkt->start_ms = (int)(rtStart / 10000);
		pkt->end_ms   = (int)(rtEnd   / 10000);
		pkt->data.assign(pBuf, pBuf + len);

		if (!::PostMessage(m_target, m_msg, 0, (LPARAM)pkt))
			delete pkt;
		return S_OK;
	}
	STDMETHODIMP ReceiveMultiple(IMediaSample** pSamples, long nSamples, long* nSamplesProcessed) override {
		long n = 0;
		for (long i = 0; i < nSamples; i++) {
			HRESULT hr = Receive(pSamples[i]);
			if (FAILED(hr)) break;
			n++;
		}
		if (nSamplesProcessed) *nSamplesProcessed = n;
		return S_OK;
	}
	STDMETHODIMP ReceiveCanBlock() override { return S_FALSE; }
};

class CSubtitleSinkFilter : public IBaseFilter
{
	LONG m_ref;
	FILTER_STATE m_state;
	IFilterGraph* m_pGraph;	//weak (DirectShow convention)
	IReferenceClock* m_pClock;
	WCHAR m_name[64];
	CSubtitleSinkPin* m_pPin;
public:
	CSubtitleSinkFilter(HWND target, UINT msg)
		: m_ref(0), m_state(State_Stopped), m_pGraph(NULL), m_pClock(NULL) {
		m_name[0] = 0;
		m_pPin = new CSubtitleSinkPin(this, target, msg);
		m_pPin->AddRef();
	}
	~CSubtitleSinkFilter() {
		if (m_pPin) m_pPin->Release();
		if (m_pClock) m_pClock->Release();
	}

	GUID get_subtype() { return m_pPin ? m_pPin->get_subtype() : GUID_NULL; }

	//IUnknown
	STDMETHODIMP QueryInterface(REFIID iid, void** ppv) override {
		if (iid == IID_IUnknown || iid == IID_IPersist || iid == IID_IMediaFilter || iid == IID_IBaseFilter) {
			*ppv = static_cast<IBaseFilter*>(this);
			AddRef();
			return S_OK;
		}
		*ppv = NULL; return E_NOINTERFACE;
	}
	STDMETHODIMP_(ULONG) AddRef() override { return InterlockedIncrement(&m_ref); }
	STDMETHODIMP_(ULONG) Release() override { ULONG n = InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }

	//IPersist
	STDMETHODIMP GetClassID(CLSID* pClsID) override {
		if (!pClsID) return E_POINTER;
		ZeroMemory(pClsID, sizeof(CLSID));
		return S_OK;
	}

	//IMediaFilter
	STDMETHODIMP Stop() override { m_state = State_Stopped; return S_OK; }
	STDMETHODIMP Pause() override { m_state = State_Paused; return S_OK; }
	STDMETHODIMP Run(REFERENCE_TIME) override { m_state = State_Running; return S_OK; }
	STDMETHODIMP GetState(DWORD, FILTER_STATE* pState) override {
		if (!pState) return E_POINTER;
		*pState = m_state; return S_OK;
	}
	STDMETHODIMP SetSyncSource(IReferenceClock* pClock) override {
		if (m_pClock) m_pClock->Release();
		m_pClock = pClock;
		if (m_pClock) m_pClock->AddRef();
		return S_OK;
	}
	STDMETHODIMP GetSyncSource(IReferenceClock** ppClock) override {
		if (!ppClock) return E_POINTER;
		*ppClock = m_pClock;
		if (m_pClock) m_pClock->AddRef();
		return S_OK;
	}

	//IBaseFilter
	STDMETHODIMP EnumPins(IEnumPins** ppEnum) override {
		if (!ppEnum) return E_POINTER;
		CSubtitleEnumPins* p = new CSubtitleEnumPins(m_pPin);
		p->AddRef(); *ppEnum = p;
		return S_OK;
	}
	STDMETHODIMP FindPin(LPCWSTR Id, IPin** ppPin) override {
		if (!ppPin) return E_POINTER;
		if (Id && wcscmp(Id, L"In") == 0) {
			m_pPin->AddRef(); *ppPin = m_pPin; return S_OK;
		}
		*ppPin = NULL; return VFW_E_NOT_FOUND;
	}
	STDMETHODIMP QueryFilterInfo(FILTER_INFO* pInfo) override {
		if (!pInfo) return E_POINTER;
		wcscpy_s(pInfo->achName, m_name);
		pInfo->pGraph = m_pGraph;
		if (m_pGraph) m_pGraph->AddRef();
		return S_OK;
	}
	STDMETHODIMP JoinFilterGraph(IFilterGraph* pGraph, LPCWSTR pName) override {
		m_pGraph = pGraph;	//weak ref
		if (pName) wcscpy_s(m_name, pName);
		else m_name[0] = 0;
		return S_OK;
	}
	STDMETHODIMP QueryVendorInfo(LPWSTR*) override { return E_NOTIMPL; }
};

static IPin* find_unconnected_pin(IBaseFilter* filter, PIN_DIRECTION dir_want)
{
	IEnumPins* pEnum = NULL;
	if (!filter || FAILED(filter->EnumPins(&pEnum)) || !pEnum) return NULL;
	IPin* pPin = NULL;
	IPin* pFound = NULL;
	while (pEnum->Next(1, &pPin, NULL) == S_OK)
	{
		PIN_DIRECTION dir;
		pPin->QueryDirection(&dir);
		if (dir == dir_want)
		{
			IPin* pOther = NULL;
			pPin->ConnectedTo(&pOther);
			if (pOther) { pOther->Release(); pPin->Release(); continue; }
			pFound = pPin;
			break;
		}
		pPin->Release();
	}
	pEnum->Release();
	return pFound;
}

void CDShow::setup_subtitle_grabber(HWND target, UINT msg)
{
	teardown_subtitle_grabber();

	if (!m_pGB || !m_pSplitter || !target) return;

	//graph state 보호 — running/paused 시 새 필터 추가/연결 실패 가능. stop 후 setup 후 복원.
	CComQIPtr<IMediaControl> pMC(m_pGB);
	OAFilterState saved_state = State_Stopped;
	if (pMC) { pMC->GetState(0, &saved_state); pMC->Stop(); }

	//splitter 의 자막 출력 핀 찾기
	IPin* pSubPin = NULL;
	IEnumPins* pEnum = NULL;
	m_pSplitter->EnumPins(&pEnum);
	if (pEnum)
	{
		IPin* pPin = NULL;
		while (pEnum->Next(1, &pPin, NULL) == S_OK)
		{
			PIN_DIRECTION dir;
			pPin->QueryDirection(&dir);
			if (dir == PINDIR_OUTPUT)
			{
				IEnumMediaTypes* pEnumMT = NULL;
				pPin->EnumMediaTypes(&pEnumMT);
				if (pEnumMT)
				{
					AM_MEDIA_TYPE* pmt = NULL;
					if (pEnumMT->Next(1, &pmt, NULL) == S_OK)
					{
						//IsEqualGUID + Data1 fallback (E487EB08 은 자막 majortype 의 unique signature)
						if (IsEqualGUID(pmt->majortype, MEDIATYPE_Subtitle) ||
							pmt->majortype.Data1 == 0xE487EB08)
						{
							IPin* pConnected = NULL;
							pPin->ConnectedTo(&pConnected);
							if (pConnected) { m_pGB->Disconnect(pPin); m_pGB->Disconnect(pConnected); pConnected->Release(); }
							pSubPin = pPin;
							pSubPin->AddRef();
							m_subtitle_subtype = pmt->subtype;
						}
						if (pmt->cbFormat && pmt->pbFormat) CoTaskMemFree(pmt->pbFormat);
						if (pmt->pUnk) pmt->pUnk->Release();
						CoTaskMemFree(pmt);
					}
					pEnumMT->Release();
				}
			}
			pPin->Release();
			if (pSubPin) break;
		}
		pEnum->Release();
	}

	if (!pSubPin)
	{
		if (pMC && (saved_state == State_Running || saved_state == State_Paused)) pMC->Run();
		return;
	}

	//Custom sink filter 생성 + 그래프에 추가
	CSubtitleSinkFilter* pSink = new CSubtitleSinkFilter(target, msg);
	pSink->AddRef();

	m_pGB->AddFilter(pSink, L"Subtitle Sink");

	//splitter 자막 핀 → sink input 핀 직접 연결 (intermediate filter 사용 안 함)
	IPin* pSinkIn = NULL;
	pSink->FindPin(L"In", &pSinkIn);
	if (pSinkIn)
	{
		m_pGB->ConnectDirect(pSubPin, pSinkIn, NULL);
		pSinkIn->Release();
	}

	pSubPin->Release();

	m_pSubtitleGrabber = pSink;	//IBaseFilter* alias
	m_pSubtitleNullRenderer = NULL;
	m_pSubtitleGrabberCB = pSink;	//CSubtitleSinkFilter* (subtype 조회용 별칭)

	//graph state 복원
	if (pMC && (saved_state == State_Running || saved_state == State_Paused)) pMC->Run();
}

GUID CDShow::get_subtitle_format()
{
	if (m_pSubtitleGrabberCB)
	{
		GUID g = ((CSubtitleSinkFilter*)m_pSubtitleGrabberCB)->get_subtype();
		if (g.Data1 != 0)
			return g;
	}
	return m_subtitle_subtype;
}

void CDShow::teardown_subtitle_grabber()
{
	//m_pSubtitleGrabber 와 m_pSubtitleGrabberCB 는 같은 sink 객체의 다른 alias
	//(IBaseFilter* / CSubtitleSinkFilter*). Release 는 한 번만.
	if (m_pGB && m_pSubtitleGrabber)
		m_pGB->RemoveFilter(m_pSubtitleGrabber);
	if (m_pSubtitleGrabber) { m_pSubtitleGrabber->Release(); m_pSubtitleGrabber = NULL; }
	m_pSubtitleNullRenderer = NULL;
	m_pSubtitleGrabberCB = NULL;	//alias 였으므로 별도 release 안 함
}


//Audio gain filter — graph 의 audio renderer 직전에 끼워 PCM sample 직접 amplify.
//graph 빌드 후 (load_media 끝) 호출. graph 가 stopped 인 상태에서 reconnect.
void CDShow::setup_audio_filter_chain() { setup_audio_gain_filter(); }
void CDShow::teardown_audio_filter_chain() { teardown_audio_gain_filter(); }
void CDShow::set_audio_compressor_makeup_db(float db)
{
	if (!m_pAudioCompressorFilter) return;
	((CSCAudioCompressor*)m_pAudioCompressorFilter)->set_makeup_db(db);
	logWrite(_T("[set_audio_compressor_makeup_db] db=%.2f"), db);
}

void CDShow::setup_audio_gain_filter()
{
	logWrite(_T("[AudioGain] setup entry gb=%p"), m_pGB);
	teardown_audio_gain_filter();

	if (!m_pGB) return;

	//graph state 보호
	CComQIPtr<IMediaControl> pMC(m_pGB);
	OAFilterState saved_state = State_Stopped;
	if (pMC) { pMC->GetState(0, &saved_state); pMC->Stop(); }

	//audio renderer 찾기 — IBaseFilter::EnumPins 로 Audio renderer 의 input pin 검사.
	//기준: input pin 만 가진 (output pin 없는) filter + connected media major == MEDIATYPE_Audio.
	IBaseFilter*	pRenderer = NULL;
	IPin*			pRendererIn = NULL;
	IPin*			pUpstreamOut = NULL;

	IEnumFilters* pEnumF = NULL;
	if (SUCCEEDED(m_pGB->EnumFilters(&pEnumF)) && pEnumF)
	{
		IBaseFilter* pF = NULL;
		while (pEnumF->Next(1, &pF, NULL) == S_OK)
		{
			IEnumPins* pEnumP = NULL;
			if (SUCCEEDED(pF->EnumPins(&pEnumP)) && pEnumP)
			{
				bool has_input = false;
				bool has_output = false;
				IPin* pAudioIn = NULL;
				IPin* p = NULL;
				while (pEnumP->Next(1, &p, NULL) == S_OK)
				{
					PIN_DIRECTION dir;
					p->QueryDirection(&dir);
					if (dir == PINDIR_OUTPUT) has_output = true;
					else if (dir == PINDIR_INPUT)
					{
						has_input = true;
						AM_MEDIA_TYPE mt;
						memset(&mt, 0, sizeof(mt));
						if (SUCCEEDED(p->ConnectionMediaType(&mt)))
						{
							if (IsEqualGUID(mt.majortype, MEDIATYPE_Audio))
							{
								if (pAudioIn) pAudioIn->Release();
								pAudioIn = p;
								pAudioIn->AddRef();
							}
							if (mt.cbFormat && mt.pbFormat) CoTaskMemFree(mt.pbFormat);
							if (mt.pUnk) mt.pUnk->Release();
						}
					}
					p->Release();
				}
				pEnumP->Release();

				if (has_input && !has_output && pAudioIn)
				{
					pRenderer = pF;	pRenderer->AddRef();
					pRendererIn = pAudioIn;	//ref 그대로 owner
					pAudioIn = NULL;
				}
				if (pAudioIn) pAudioIn->Release();
			}
			pF->Release();
			if (pRenderer) break;
		}
		pEnumF->Release();
	}

	if (!pRenderer || !pRendererIn)
	{
		logWrite(_T("[AudioGain] no audio renderer found"));
		if (pRenderer) pRenderer->Release();
		if (pRendererIn) pRendererIn->Release();
		if (pMC && (saved_state == State_Running || saved_state == State_Paused)) pMC->Run();
		return;
	}
	logWrite(_T("[AudioGain] audio renderer found pRenderer=%p pRendererIn=%p"), pRenderer, pRendererIn);

	//audio renderer input 의 connected upstream output pin 가져옴.
	pRendererIn->ConnectedTo(&pUpstreamOut);

	if (!pUpstreamOut)
	{
		logWrite(_T("[AudioGain] upstream pin not connected"));
		pRenderer->Release();
		pRendererIn->Release();
		if (pMC && (saved_state == State_Running || saved_state == State_Paused)) pMC->Run();
		return;
	}

	//기존 connection 의 media type 을 보존 (disconnect 후 EnumMediaTypes 만으론 재협상 불안정).
	AM_MEDIA_TYPE saved_mt;
	memset(&saved_mt, 0, sizeof(saved_mt));
	HRESULT hr_mt = pRendererIn->ConnectionMediaType(&saved_mt);
	bool has_saved_mt = SUCCEEDED(hr_mt);
	logWrite(_T("[AudioGain] saved mt hr=0x%08lX major.D1=%08lX sub.D1=%08lX cbFmt=%lu"),
		hr_mt, saved_mt.majortype.Data1, saved_mt.subtype.Data1, saved_mt.cbFormat);

	//기존 connection 끊기.
	m_pGB->Disconnect(pRendererIn);
	m_pGB->Disconnect(pUpstreamOut);

	//gain filter 생성 + graph 추가.
	CSCAudioGain* pGain = new CSCAudioGain();
	pGain->AddRef();

	HRESULT hr_add = m_pGB->AddFilter(pGain, L"SC Audio Gain");
	logWrite(_T("[AudioGain] AddFilter hr=0x%08lX"), hr_add);
	if (FAILED(hr_add))
	{
		m_pGB->ConnectDirect(pUpstreamOut, pRendererIn, has_saved_mt ? &saved_mt : NULL);
		pGain->Release();
		if (has_saved_mt) { if (saved_mt.cbFormat && saved_mt.pbFormat) CoTaskMemFree(saved_mt.pbFormat); if (saved_mt.pUnk) saved_mt.pUnk->Release(); }
		pUpstreamOut->Release();
		pRenderer->Release();
		pRendererIn->Release();
		if (pMC && (saved_state == State_Running || saved_state == State_Paused)) pMC->Run();
		return;
	}

	IPin* pGainIn = NULL;
	IPin* pGainOut = NULL;
	pGain->FindPin(L"In",  &pGainIn);
	pGain->FindPin(L"Out", &pGainOut);

	HRESULT hr1 = (pGainIn  ? m_pGB->ConnectDirect(pUpstreamOut, pGainIn,  has_saved_mt ? &saved_mt : NULL) : E_FAIL);
	HRESULT hr2 = (pGainOut ? m_pGB->ConnectDirect(pGainOut, pRendererIn, has_saved_mt ? &saved_mt : NULL) : E_FAIL);
	logWrite(_T("[AudioGain] Connect upstream->gain hr=0x%08lX  gain->renderer hr=0x%08lX"), hr1, hr2);

	if (pGainIn) pGainIn->Release();
	if (pGainOut) pGainOut->Release();

	if (FAILED(hr1) || FAILED(hr2))
	{
		logWrite(_T("[AudioGain] connect failed - restoring original chain"));
		m_pGB->RemoveFilter(pGain);
		pGain->Release();
		m_pGB->ConnectDirect(pUpstreamOut, pRendererIn, has_saved_mt ? &saved_mt : NULL);
	}
	else
	{
		logWrite(_T("[AudioGain] gain filter inserted into graph"));
		m_pAudioGainFilter = pGain;

		//Compressor 도 chain 에 추가: upstream → gain → compressor → renderer.
		//현재 gain → renderer 의 link 를 끊고 그 사이에 compressor 끼움.
		IPin* pGainOutPin = NULL;
		pGain->FindPin(L"Out", &pGainOutPin);
		if (pGainOutPin)
		{
			AM_MEDIA_TYPE comp_mt;
			memset(&comp_mt, 0, sizeof(comp_mt));
			HRESULT hr_cmt = pGainOutPin->ConnectionMediaType(&comp_mt);
			bool has_comp_mt = SUCCEEDED(hr_cmt);

			m_pGB->Disconnect(pGainOutPin);
			m_pGB->Disconnect(pRendererIn);

			CSCAudioCompressor* pComp = new CSCAudioCompressor();
			pComp->AddRef();
			//-24 dB threshold, 4:1 ratio — 음악·음성 통상 setting. attack/release 는 default(30/200ms).
			pComp->set_threshold_db(-24.0f);
			pComp->set_ratio(4.0f);
			pComp->set_makeup_db(0.0f);	//set_volume 에서 동적 조절

			HRESULT hr_addc = m_pGB->AddFilter(pComp, L"SC Audio Compressor");
			logWrite(_T("[AudioComp] AddFilter hr=0x%08lX"), hr_addc);

			IPin* pCompIn = NULL, * pCompOut = NULL;
			pComp->FindPin(L"In",  &pCompIn);
			pComp->FindPin(L"Out", &pCompOut);

			HRESULT hr_c1 = (pCompIn  ? m_pGB->ConnectDirect(pGainOutPin, pCompIn,  has_comp_mt ? &comp_mt : NULL) : E_FAIL);
			HRESULT hr_c2 = (pCompOut ? m_pGB->ConnectDirect(pCompOut, pRendererIn, has_comp_mt ? &comp_mt : NULL) : E_FAIL);
			logWrite(_T("[AudioComp] Connect gain->comp hr=0x%08lX  comp->renderer hr=0x%08lX"), hr_c1, hr_c2);

			if (pCompIn) pCompIn->Release();
			if (pCompOut) pCompOut->Release();

			if (FAILED(hr_c1) || FAILED(hr_c2))
			{
				logWrite(_T("[AudioComp] connect failed - reverting (compressor 없이 gain 만 사용)"));
				m_pGB->RemoveFilter(pComp);
				pComp->Release();
				m_pGB->ConnectDirect(pGainOutPin, pRendererIn, has_comp_mt ? &comp_mt : NULL);
			}
			else
			{
				logWrite(_T("[AudioComp] compressor inserted into chain (gain->comp->renderer)"));
				m_pAudioCompressorFilter = pComp;
			}

			if (has_comp_mt)
			{
				if (comp_mt.cbFormat && comp_mt.pbFormat) CoTaskMemFree(comp_mt.pbFormat);
				if (comp_mt.pUnk) comp_mt.pUnk->Release();
			}
			pGainOutPin->Release();
		}
	}

	if (has_saved_mt)
	{
		if (saved_mt.cbFormat && saved_mt.pbFormat) CoTaskMemFree(saved_mt.pbFormat);
		if (saved_mt.pUnk) saved_mt.pUnk->Release();
	}

	pUpstreamOut->Release();
	pRenderer->Release();
	pRendererIn->Release();

	if (pMC && (saved_state == State_Running || saved_state == State_Paused)) pMC->Run();
}

void CDShow::teardown_audio_gain_filter()
{
	if (m_pAudioCompressorFilter)
	{
		CSCAudioCompressor* pComp = (CSCAudioCompressor*)m_pAudioCompressorFilter;
		if (m_pGB) m_pGB->RemoveFilter(pComp);
		pComp->Release();
		m_pAudioCompressorFilter = NULL;
	}

	if (m_pAudioGainFilter)
	{
		CSCAudioGain* pGain = (CSCAudioGain*)m_pAudioGainFilter;
		if (m_pGB) m_pGB->RemoveFilter(pGain);
		pGain->Release();
		m_pAudioGainFilter = NULL;
	}
}

void CDShow::set_audio_gain_db(float db)
{
	logWrite(_T("[set_audio_gain_db] db=%.2f filter=%p"), db, m_pAudioGainFilter);
	if (!m_pAudioGainFilter) return;
	CSCAudioGain* pGain = (CSCAudioGain*)m_pAudioGainFilter;
	pGain->set_gain_db(db);
}

float CDShow::get_audio_gain_db()
{
	if (!m_pAudioGainFilter) return 0.0f;
	CSCAudioGain* pGain = (CSCAudioGain*)m_pAudioGainFilter;
	return pGain->get_gain_db();
}

#ifndef JIF
#define JIF(x) if (FAILED(hr=(x))) {return hr;}
#endif

#define SAFE_RELEASE(pObject) { if (pObject!=NULL) { pObject->Release(); pObject=NULL; } }
const float EDGE_BUFFER  = 0.04f;  // Pixel buffer between bitmap and window edge

CDShow::CDShow()
{
	if (FAILED(CoInitialize(NULL)))  //COM을 사용하기 위해서 초가화(디쇼는 컴으로 되어있다)
	{
		AfxMessageBox(_T("CoInitiallize Error"));
		return;
	}

	m_pGB = NULL;
	m_VMR = NULL;
	pSource = NULL;
	m_SourceBase = NULL;
	m_pFileSource = NULL;
	m_pSplitter = NULL;

	m_pParentDC = NULL;
	m_pMemDC[0] = NULL;
	m_pMemDC[1] = NULL;
	m_pBitmap[0] = NULL;
	m_pBitmap[1] = NULL;

	m_pSubtitleGrabber = NULL;
	m_pSubtitleNullRenderer = NULL;
	m_pSubtitleGrabberCB = NULL;
	memset(&m_subtitle_subtype, 0, sizeof(m_subtitle_subtype));

	m_pAudioGainFilter = NULL;
	m_pAudioCompressorFilter = NULL;

	m_use_dvs = false;
	m_use_mpcvr = false;
	m_has_subtitle = false;
	m_subtitle_file.Empty();
	m_hDirectVobSubWnd = NULL;
	m_subtitle_sync = 0;
	m_media_filename.Empty();

	close_media();

	m_volume_mute = AfxGetApp()->GetProfileInt(_T("setting"), _T("volume mute"), false);
	m_volume = AfxGetApp()->GetProfileInt(_T("setting"), _T("volume"), 50);
	m_default_interval = AfxGetApp()->GetProfileInt(_T("setting"), _T("default track interval"), 5);
	m_control_interval = AfxGetApp()->GetProfileInt(_T("setting"), _T("control track interval"), 30);

	m_show_subtitle = AfxGetApp()->GetProfileInt(_T("subtitle"), _T("show"), true);

	m_subCfg <<= AfxGetApp()->GetProfileString(_T("subtitle"), _T("setting"), _T(""));

	//옛 schema (colors[4] + alpha[4] + alpha_link) 로 저장된 값을 새 schema (cr[4] ARGB) 로 deserialize 하면
	//cr 의 alpha 비트가 모두 0 으로 읽혀 자막이 완전 투명. 그 경우 옛 garbage 데이터로 판정해
	//default 로 reset 하고 즉시 새 schema 로 저장 — 옛 키 데이터 덮어쓰기.
	bool legacy = true;
	for (int i = 0; i < 4; i++)
		if (m_subCfg.cr[i].GetA() != 0) { legacy = false; break; }
	if (legacy)
	{
		m_subCfg.set_default();
		CString style;
		AfxGetApp()->WriteProfileString(_T("subtitle"), _T("setting"), style <<= m_subCfg);
	}

	/*
	memset(&m_subCfg, 0, sizeof(m_subCfg));
	m_subCfg.lf = (LOGFONT*)new LOGFONT;
	memset(m_subCfg.lf, 0, sizeof(LOGFONT));

	m_subCfg.pos_x = AfxGetApp()->GetProfileInt(_T("subtitle"), _T("pos x"), 50);
	m_subCfg.pos_y = AfxGetApp()->GetProfileInt(_T("subtitle"), _T("pos y"), 90);
	m_subCfg.line_spacing = AfxGetApp()->GetProfileInt(_T("subtitle"), _T("line spacing"), 120);
	_tcscpy(m_subCfg.lf->lfFaceName, AfxGetApp()->GetProfileString(_T("subtitle"), _T("face name"), _T("맑은 고딕")));
	m_subCfg.fontSize = AfxGetApp()->GetProfileInt(_T("subtitle"), _T("font size"), 40);
	m_subCfg.lf->lfHeight = get_logical_size_from_font_size(::GetDC(NULL), m_subCfg.fontSize);
	m_subCfg.lf->lfItalic = AfxGetApp()->GetProfileInt(_T("subtitle"), _T("font italic"), false);
	m_subCfg.lf->lfWeight = AfxGetApp()->GetProfileInt(_T("subtitle"), _T("font weight"), 0);

	m_subCfg.outlineWidthX = AfxGetApp()->GetProfileInt(_T("subtitle"), _T("width outline"), 3);
	m_subCfg.outlineWidthY = AfxGetApp()->GetProfileInt(_T("subtitle"), _T("width outline"), 3);
	m_subCfg.shadowDepthX = AfxGetApp()->GetProfileInt(_T("subtitle"), _T("depth shadow"), 3);
	m_subCfg.shadowDepthY = AfxGetApp()->GetProfileInt(_T("subtitle"), _T("depth shadow"), 3);

	m_subCfg.colors[0] = AfxGetApp()->GetProfileInt(_T("subtitle"), _T("color text"), RGB(255, 243, 212));
	m_subCfg.colors[2] = AfxGetApp()->GetProfileInt(_T("subtitle"), _T("color outline"), RGB(0, 0, 1));
	m_subCfg.colors[3] = AfxGetApp()->GetProfileInt(_T("subtitle"), _T("color shadow"), RGB(32, 32, 32));

	m_subCfg.alpha[0] = AfxGetApp()->GetProfileInt(_T("subtitle"), _T("alpha text"), 255);
	m_subCfg.alpha[2] = AfxGetApp()->GetProfileInt(_T("subtitle"), _T("alpha outline"), 255);
	m_subCfg.alpha[3] = AfxGetApp()->GetProfileInt(_T("subtitle"), _T("alpha shadow"), 255);
	*/
}

CDShow::~CDShow()
{
	close_media();

	CoUninitialize();
}


void CDShow::close_media()
{
	teardown_subtitle_grabber();
	teardown_audio_gain_filter();

	if (m_pGB)
	{
		CComQIPtr<IMediaControl> pMC(m_pGB);
		if (pMC != NULL)
			pMC->Stop();

		hide_cursor(false);

		if (g_dwGraphRegister != 0)
			RemoveGraphFromRot(g_dwGraphRegister);
	}

	SAFE_RELEASE(m_VMR);
	SAFE_RELEASE(pSource);
	SAFE_RELEASE(m_pSplitter)
	SAFE_RELEASE(m_pFileSource);
	SAFE_RELEASE(m_SourceBase);
	SAFE_RELEASE(m_pGB);

	m_pMP.Release();
	m_pMS.Release();
	m_pMC.Release();
	m_pME.Release();

	m_pVMRWC.Release();
	m_pVMRFC.Release();
	m_pVMRMC.Release();
	m_pVMRMB.Release();
	m_pVDC.Release();
	m_use_mpcvr = false;

	m_has_subtitle = false;
	m_subtitle_file.Empty();
	m_show_subtitle = true;
	m_hDirectVobSubWnd = NULL;
	m_subtitle_sync = 0;
	m_media_filename.Empty();

	m_video_stream.clear();
	m_audio_stream.clear();
	m_subtitle_stream.clear();
	m_video_stream_index = 0;
	m_audio_stream_index = 0;
	m_subtitle_stream_index = -1;

	m_mirror = m_flip = false;

	if (m_pParentDC)
	{
		m_pParent->ReleaseDC(m_pParentDC);
		m_pParentDC = NULL;
	}

	if (m_pMemDC[0])
	{
		m_pMemDC[0]->DeleteDC();
		delete m_pMemDC[0];
		m_pMemDC[0] = NULL;
	}
	if (m_pMemDC[1])
	{
		m_pMemDC[1]->DeleteDC();
		delete m_pMemDC[1];
		m_pMemDC[1] = NULL;
	}

	if (m_pBitmap[0])
	{
		m_pBitmap[0]->DeleteObject();
		delete m_pBitmap[0];
		m_pBitmap[0] = NULL;
	}
	if (m_pBitmap[1])
	{
		m_pBitmap[1]->DeleteObject();
		delete m_pBitmap[1];
		m_pBitmap[1] = NULL;
	}
}

//#include "C:\\1.projects\\mpc-hc-develop\\src\\filters\\transform\\VSFilter\\IDirectVobSub.h"
//#include "C:\\1.projects\\VSFilterMod-master\\src\\vsfilter\\IDirectVobSub.h"
//#pragma comment( lib, "C:\\1.projects\\VSFilterMod-master\\bin\\Win32\\VSFilter\\Debug (MOD)\\VSFilterMod.lib" )

int CDShow::load_media(CString sfile, CWnd* pParent, bool auto_render)
{
	if (PathFileExists(sfile) == false)
		return 0;

	m_media_info_string.Empty();
	m_media_info_string = m_media_info.Option(__T("Info_Version"), __T("0.7.13;MediaInfoDLL_Example_MSVC;0.7.13")).c_str();
	/*
	if (m_media_info_string.Find(_T("Unable")) == 0)
	{
		AfxMessageBox(_T("이 프로그램은 다음 코덱을 필요로 합니다.\n-MediaInfo\n-AC3Filter\n-LAV Splitter\n\n스타 코덱 라이트 버전(32비트 버전) 설치를 권장합니다."));
		return 0;
	}
	*/

	m_pParent = pParent;

	HRESULT hr;
	WCHAR wFileName[MAX_PATH],wFileNameSMI[MAX_PATH];
	CString subTitle = sfile.Left(sfile.GetLength() - 3) + _T("smi");

#ifndef UNICODE
	MultiByteToWideChar(CP_ACP,0,szFileName.GetBuffer(0),-1,wFileName,MAX_PATH);
	MultiByteToWideChar(CP_ACP,0,TwFileNameSMI,-1,wFileNameSMI,MAX_PATH);
#else
	lstrcpy(wFileName,sfile);
	lstrcpy(wFileNameSMI,subTitle);
#endif

	if (m_pGB != NULL)
	{
		close_media();
	}

	m_media_filename = sfile;

	CoCreateInstance(CLSID_FilterGraph,NULL,CLSCTX_INPROC_SERVER,IID_IGraphBuilder,(void **)&m_pGB) ;

	//VMR9 으로 revert — EVR 의 SetVideoPosition 보간이 resize 마다 비디오가 y=0 으로 잠깐 점프하는 transient 유발.
	//VMR9 은 SetVideoPosition 즉시 적용되어 transient 없음. 깜빡임 fix 는 m_in_size_move 패턴 (drag 중 검정 fill).
	CoCreateInstance(CLSID_VideoMixingRenderer9, NULL, CLSCTX_INPROC,
		IID_IBaseFilter, (LPVOID*)&m_VMR);
	if (m_VMR != NULL)
	{
		m_pGB->AddFilter(m_VMR, L"Video Mixing Renderer 9");
	}

	m_VMR->QueryInterface(IID_PPV_ARGS(&m_pVMRFC));
	if (m_pVMRFC != NULL)
		m_pVMRFC->SetRenderingMode(VMR9Mode_Windowless);

	m_VMR->QueryInterface(IID_PPV_ARGS(&m_pVMRWC));
	if (m_pVMRWC != NULL)
	{
		m_pVMRWC->SetVideoClippingWindow(pParent->m_hWnd);
		m_pVMRWC->SetAspectRatioMode(VMR_ARMODE_LETTER_BOX);
		//letterbox 바 색을 명시적으로 검정. default 가 미정이라 비디오 aspect 와 rect aspect 가 다를 때
		//좌/우 또는 상/하 바에 흰색·미초기화 픽셀이 깜빡임으로 보일 수 있음.
		m_pVMRWC->SetBorderColor(RGB(0, 0, 0));
		//SetVideoPosition 은 호출자(open_media) 가 chrome strip 뺀 정확한 비디오 영역으로 직접 호출.
	}

	m_VMR->QueryInterface(IID_PPV_ARGS(&m_pVMRMB));
	m_VMR->QueryInterface(IID_PPV_ARGS(&m_pVMRMC));

	CComQIPtr<IVideoWindow> pVW(m_pGB);
	if (pVW != NULL)
	{
		hr = pVW->put_Owner((OAHWND)pParent->m_hWnd);
		hr = pVW->put_WindowStyle(WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	}

	hide_cursor(true);

	m_pGB->QueryInterface(IID_PPV_ARGS(&m_pMC));

	hr = m_pGB->QueryInterface(IID_PPV_ARGS(&m_pME));
	if (m_pME != NULL)
	{
		hr = m_pME->SetNotifyWindow((OAHWND)pParent->m_hWnd, WM_GRAPHNOTIFY, 0);
	}

	if (m_VMR != NULL)
	{
		//RenderFileToVMR9(m_pGB, wFileName, pSource);
		if (!auto_render)
		{
			IBaseFilter	*pBaseFilter = NULL;
			//CComQIPtr<IDirectVobSub> pDVS;

			if (m_use_dvs)
			{
				hr = FindFilter(_T("DirectVobSub (auto-loading version)"), CLSID_LegacyAmFilterCategory, &pBaseFilter );
				hr = m_pGB->AddFilter(pBaseFilter, _T("DirectVobSub (auto-loading version)"));
			}

			hr = FindFilter(_T("AC3Filter"), CLSID_LegacyAmFilterCategory, &pBaseFilter );
			hr = m_pGB->AddFilter(pBaseFilter, _T("AC3Filter"));

			m_media_info.Open(wFileName);
			m_media_info.Option(__T("Complete"));
			m_media_info_string = m_media_info_string + _T("\r\n\r\n") + m_media_info.Inform().c_str();
			m_frame_rate = get_frame_rate();
			if (m_frame_rate <= 1.0)
			{
				m_frame_rate = 29.97;
				//AfxMessageBox(_T("fail to get Frame rate."));
			}

			if (is_windows_media())
			{
				hr = FindFilter(_T("WM ASF Reader"), CLSID_LegacyAmFilterCategory, &m_SourceBase );
				hr = m_pGB->AddFilter(m_SourceBase, _T("WM ASF Reader"));

				hr = m_SourceBase->QueryInterface( IID_IFileSourceFilter, (void **) &m_pFileSource );
				hr = m_pFileSource->Load(wFileName, NULL);

				hr = FindFilter(_T("WMVideo Decoder DMO"), CLSID_LegacyAmFilterCategory, &pBaseFilter );
				hr = m_pGB->AddFilter(pBaseFilter, _T("WMVideo Decoder DMO"));

				hr = FindFilter(_T("WMAudio Decoder DMO"), CLSID_LegacyAmFilterCategory, &pBaseFilter );
				hr = m_pGB->AddFilter(pBaseFilter, _T("WMAudio Decoder DMO"));
			}
			else
			{
				hr = FindFilter(_T("File Source (Async.)"), CLSID_LegacyAmFilterCategory, &m_SourceBase );
				hr = m_pGB->AddFilter(m_SourceBase, _T("File Source (Async.)"));

				hr = m_SourceBase->QueryInterface( IID_IFileSourceFilter, (void **) &m_pFileSource );
				hr = m_pFileSource->Load(wFileName, NULL);
				ShowFilterPropertyPage(_T("File Source (Async.)"));

				//CComQIPtr<ISpecifyPropertyPages> pSpecify(m_SourceBase);
				//ISpecifyPropertyPages *pSpecify = NULL;

				// Discover if this filter contains a property page
				//hr = m_SourceBase->QueryInterface(IID_ISpecifyPropertyPages, (void **)&pSpecify);


				//LAV Video Decoder 의 HW 가속 톤매핑 활성화 — D3D11 디코더 + GPU 톤매핑으로
				//BT.2020+PQ → BT.709+sRGB 변환을 LAV 가 직접 수행. 그 후 VMR9 에는 SDR frame 만 전달.
				//VMR9 자체는 HDR 미지원이라 LAV 측에서 미리 변환하지 않으면 워시드 아웃이 된다.
				HKEY hLavKey = NULL;
				if (::RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\LAV\\Video"),
						0, NULL, 0, KEY_SET_VALUE, NULL, &hLavKey, NULL) == ERROR_SUCCESS)
				{
					DWORD hwaccel = 2;	//0=SW, 1=DXVA2-native, 2=DXVA2-copyback, 3=D3D11
					//madVR 와의 호환성을 위해 copyback 모드. D3D11/native 모드는 madVR D3D11 device 와 충돌해 access violation 가능.
					::RegSetValueEx(hLavKey, _T("HWAccel"), 0, REG_DWORD,
						(const BYTE*)&hwaccel, sizeof(hwaccel));
					DWORD hw_tonemap = 1;
					::RegSetValueEx(hLavKey, _T("HWAccelTonemap"), 0, REG_DWORD,
						(const BYTE*)&hw_tonemap, sizeof(hw_tonemap));
					::RegCloseKey(hLavKey);
				}

				hr = FindFilter(_T("LAV Splitter"), CLSID_LegacyAmFilterCategory, &m_pSplitter );
				hr = m_pGB->AddFilter(m_pSplitter, _T("LAV Splitter"));

				hr = FindFilter(_T("LAV Video Decoder"), CLSID_LegacyAmFilterCategory, &pBaseFilter );
				hr = m_pGB->AddFilter(pBaseFilter, _T("LAV Video Decoder"));

				hr = FindFilter(_T("LAV Audio Decoder"), CLSID_LegacyAmFilterCategory, &pBaseFilter );
				hr = m_pGB->AddFilter(pBaseFilter, _T("LAV Audio Decoder"));


				// Attempt to load this file
			}

			if (hr != S_OK)
			{
				close_media();
				return 0;
			}
		}
	}
	
	if (m_pGB != NULL)
	{
		//RenderFile을 호출하면 추가된 필터들끼리 알아서 연결될 줄 알았으나
		//추가된 필터들이 연결은 되지만 또 다른 파일 소스 필터가 추가되어 재생된다.
		//따라서 m_SourceBase의 OUT_PIN을 찾아서 수동으로 렌더시켜준다.
		//hr = m_pGB->RenderFile(wFileName, NULL);
		hr = RenderOutputPins(m_pGB, m_SourceBase);

		if (is_windows_media())
			analyze_stream(m_SourceBase);
		else
			analyze_stream(m_pSplitter);
	}
	
#ifdef REGISTER_FILTERGRAPH
	hr = AddGraphToRot(m_pGB, &g_dwGraphRegister);
	if (FAILED(hr))
	{
		TRACE(TEXT("Failed to register filter graph with ROT!  hr=0x%x"), hr);
		g_dwGraphRegister = 0;
	}
#endif

	
	m_pGB->QueryInterface(IID_PPV_ARGS(&m_pMP));
	if (m_pMP != NULL)
	{
		REFTIME t;
		m_pMP->get_Duration(&t);
		m_duration = t;
	}
	
	m_pGB->QueryInterface(IID_PPV_ARGS(&m_pMS));
	if (m_pMS != NULL)
	{
		LONGLONG t;
		m_pMS->GetDuration(&t);
		m_duration = t/10000.0;
	}

	if (m_pVMRWC != NULL && is_media_video())
	{
		m_pVMRWC->GetNativeVideoSize(&m_video_size.cx, &m_video_size.cy, NULL, NULL);
	}
	else if (m_pVDC != NULL && is_media_video())
	{
		SIZE sz = {0,};
		if (SUCCEEDED(m_pVDC->GetNativeVideoSize(&sz, NULL)) && sz.cx > 0 && sz.cy > 0)
			m_video_size = CSize(sz.cx, sz.cy);
		else
			m_video_size = CSize(640, 368);
	}
	else if (is_media_video())
	{
		CComQIPtr<IBasicVideo> pBV(m_pGB);
		long w = 0, h = 0;
		if (pBV && SUCCEEDED(pBV->GetVideoSize(&w, &h)) && w > 0 && h > 0)
			m_video_size = CSize(w, h);
		else
			m_video_size = CSize(640, 368);
	}
	else
	{
		m_video_size = CSize(640, 368);
	}

	if (is_media_video())
		prepare_AlphaBitmap();

	//audio gain filter 끼움 — graph 의 audio renderer 직전. graph 빌드 끝난 후 한 번.
	setup_audio_gain_filter();

	//graph 의 현재 connection 상태를 .grf 파일로 export — GraphEdit/GraphStudioNext 로 시각화 검증.
	{
		TCHAR exe_path[MAX_PATH] = {0};
		GetModuleFileName(NULL, exe_path, MAX_PATH);
		CString grf_path = exe_path;
		int slash = grf_path.ReverseFind(_T('\\'));
		if (slash >= 0) grf_path = grf_path.Left(slash);
		grf_path += _T("\\Log");
		CreateDirectory(grf_path, NULL);
		grf_path += _T("\\last_graph.grf");
		HRESULT hr_save = save_filter_graph(m_pGB, grf_path);
		logWrite(_T("[graph] save_filter_graph hr=0x%08lX path=%s"), hr_save, grf_path.GetString());
	}

	//graph 구조 dump — 모든 filter / pin / connection 을 로그로 출력.
	{
		IEnumFilters* pEnumF = NULL;
		if (SUCCEEDED(m_pGB->EnumFilters(&pEnumF)) && pEnumF)
		{
			IBaseFilter* pF = NULL;
			int fidx = 0;
			while (pEnumF->Next(1, &pF, NULL) == S_OK)
			{
				FILTER_INFO fi; memset(&fi, 0, sizeof(fi));
				pF->QueryFilterInfo(&fi);
				if (fi.pGraph) fi.pGraph->Release();
				logWrite(_T("[graph] filter[%d] %s"), fidx, fi.achName);

				IEnumPins* pEnumP = NULL;
				if (SUCCEEDED(pF->EnumPins(&pEnumP)) && pEnumP)
				{
					IPin* pP = NULL;
					int pidx = 0;
					while (pEnumP->Next(1, &pP, NULL) == S_OK)
					{
						PIN_DIRECTION dir;
						pP->QueryDirection(&dir);
						PIN_INFO pi; memset(&pi, 0, sizeof(pi));
						pP->QueryPinInfo(&pi);
						if (pi.pFilter) pi.pFilter->Release();

						IPin* pConn = NULL;
						pP->ConnectedTo(&pConn);
						if (pConn)
						{
							PIN_INFO ci; memset(&ci, 0, sizeof(ci));
							pConn->QueryPinInfo(&ci);
							FILTER_INFO cfi; memset(&cfi, 0, sizeof(cfi));
							if (ci.pFilter)
							{
								ci.pFilter->QueryFilterInfo(&cfi);
								if (cfi.pGraph) cfi.pGraph->Release();
							}
							AM_MEDIA_TYPE mt; memset(&mt, 0, sizeof(mt));
							pP->ConnectionMediaType(&mt);
							logWrite(_T("[graph]   pin[%d] %s '%s' -> '%s'.'%s'  major.D1=%08lX"),
								pidx, dir == PINDIR_INPUT ? _T("IN ") : _T("OUT"),
								pi.achName, cfi.achName, ci.achName, mt.majortype.Data1);
							if (mt.cbFormat && mt.pbFormat) CoTaskMemFree(mt.pbFormat);
							if (mt.pUnk) mt.pUnk->Release();
							if (ci.pFilter) ci.pFilter->Release();
							pConn->Release();
						}
						else
						{
							logWrite(_T("[graph]   pin[%d] %s '%s' -> (disconnected)"),
								pidx, dir == PINDIR_INPUT ? _T("IN ") : _T("OUT"), pi.achName);
						}
						pP->Release();
						pidx++;
					}
					pEnumP->Release();
				}
				pF->Release();
				fidx++;
			}
			pEnumF->Release();
		}
	}

	return 1;
}

double CDShow::get_frame_rate()
{
	int pos0 = m_media_info_string.Find(_T("Frame rate"));
	int pos1 = m_media_info_string.Find(_T("FPS"));

	CString str = m_media_info_string.Mid(pos0, pos1 - pos0);
	pos0 = str.ReverseFind(':') + 2;
	pos1 = str.Find(_T(" ("));
	if (pos1 < pos0)
		str = str.Mid(pos0);
	else
		str = str.Mid(pos0, pos1 - pos0);
	//AfxMessageBox(str);

	return _ttof(str);
}

bool CDShow::is_media_video()
{
	return (m_video_stream.size() > 0);
}

bool CDShow::is_windows_media()
{
	int pos0 = m_media_info_string.Find(_T("Format"));
	int pos1;

	if (pos0 < 0)
		return false;

	pos0 = m_media_info_string.Find(_T(": "), pos0) + 2;
	pos1 = m_media_info_string.Find(_T("\r\n"), pos0);

	CString str = m_media_info_string.Mid(pos0, pos1 - pos0);

	if (str == _T("Windows Media"))
		return true;

	return false;
}

void CDShow::analyze_stream(IBaseFilter *pBaseFilter)
{
	HRESULT			hr;
	DWORD			dwCount = 0;
	AM_MEDIA_TYPE	*pmt;
	DWORD			dwFlags;
	LCID			lcid;
	DWORD			dwGroup;
	WCHAR			*pzsName;

	m_video_stream_index = 0;
	m_audio_stream_index = 0;
	m_subtitle_stream_index = -1;
	m_video_stream.clear();
	m_audio_stream.clear();
	m_subtitle_stream.clear();

	if (!pBaseFilter)
		return;

	//stream 분석
	CComQIPtr<IAMStreamSelect> pStreamSelect(pBaseFilter);
	if (!pStreamSelect)
		return;

	pStreamSelect->Count(&dwCount);

	for (int i = 0; i < dwCount; i++)
	{
		hr = pStreamSelect->Info(i, &pmt, &dwFlags, &lcid, &dwGroup, &pzsName, NULL, NULL);
		if (pmt == NULL)
		{
			TRACE(_T("pmt is null\n"));
			continue;
		}

		//LAV Splitter 는 video=group0, audio=group1, subtitle=group2 로 분류한다.
		//일부 splitter 는 자막의 majortype 으로 MEDIATYPE_Subtitle 가 아닌 다른 GUID 를 쓰므로
		//dwGroup 검사를 OR 로 같이 둔다.
		if (pmt->majortype == MEDIATYPE_Video || dwGroup == 0)
		{
			m_video_stream.push_back(CMediaStream(pzsName, i));
		}
		else if (pmt->majortype == MEDIATYPE_Audio || dwGroup == 1)
		{
			m_audio_stream.push_back(CMediaStream(pzsName, i));
		}
		else if (pmt->majortype == MEDIATYPE_Subtitle || dwGroup == 2)
		{
			m_subtitle_stream.push_back(CMediaStream(pzsName, i));
			if (dwFlags & (AMSTREAMSELECTINFO_ENABLED | AMSTREAMSELECTINFO_EXCLUSIVE))
				m_subtitle_stream_index = (int)m_subtitle_stream.size() - 1;
		}

		if (pmt->formattype == FORMAT_VideoInfo)
		{
			VIDEOINFOHEADER *vih = (VIDEOINFOHEADER *)pmt->pbFormat;
		}
		else if (pmt->formattype == FORMAT_VideoInfo2)
		{
			VIDEOINFOHEADER *vih = (VIDEOINFOHEADER *)pmt->pbFormat;
		}
	}

	//if (m_nVideoStream > 1 && nVideoCh > 0)
		//pStreamSelect->Enable(m_nVideoStream + 1, AMSTREAMSELECTENABLE_ENABLE);
	//pStreamSelect->Release();
}

//----------------------------------------------------------------------------
//  VerifyVMR9
// 
//  Verifies that VMR9 COM objects exist on the system and that the VMR9
//  can be instantiated.
//
//  Returns: FALSE if the VMR9 can't be created
//----------------------------------------------------------------------------

BOOL CDShow::VerifyVMR9(void)
{
	HRESULT hr;

	// Verify that the VMR exists on this system
	IBaseFilter* pBF = NULL;
	hr = CoCreateInstance(CLSID_VideoMixingRenderer9, NULL,
		CLSCTX_INPROC,
		IID_IBaseFilter,
		(LPVOID *)&pBF);
	if(SUCCEEDED(hr))
	{
		pBF->Release();
		return TRUE;
	}
	else
	{
		MessageBox(NULL, 
			TEXT("This application requires the Video Mixing Renderer, which is present\r\n")
			TEXT("only on DirectX 9 systems with hardware video acceleration enabled.\r\n\r\n")

			TEXT("The Video Mixing Renderer (VMR9) is not enabled when viewing a \r\n")
			TEXT("remote Windows XP machine through a Remote Desktop session.\r\n")
			TEXT("You can run VMR-enabled applications only on your local machine.\r\n\r\n")

			TEXT("To verify that hardware acceleration is enabled on a Windows XP\r\n")
			TEXT("system, follow these steps:\r\n")
			TEXT("-----------------------------------------------------------------------\r\n")
			TEXT(" - Open 'Display Properties' in the Control Panel\r\n")
			TEXT(" - Click the 'Settings' tab\r\n")
			TEXT(" - Click on the 'Advanced' button at the bottom of the page\r\n")
			TEXT(" - Click on the 'Troubleshooting' tab in the window that appears\r\n")
			TEXT(" - Verify that the 'Hardware Acceleration' slider is at the rightmost position\r\n")

			TEXT("\r\nThis sample will now exit."),

			TEXT("Video Mixing Renderer (VMR9) capabilities are required"), MB_OK);

		return FALSE;
	}
}


HRESULT CDShow::GetUnconnectedPin(
	IGraphBuilder *pGB,
	IBaseFilter *pFilter,   // Pointer to the filter.
	PIN_DIRECTION PinDir,   // Direction of the pin to find.
	IPin **ppPin)           // Receives a pointer to the pin.
{

	IEnumPins *pEnum = 0;
	IPin *pPin = 0;
	bool ren = FALSE;
	CComPtr <IFilterGraph2> pFG;

	if (!ppPin)
		return E_POINTER;
	*ppPin = 0;

	// Get a pin enumerator
	HRESULT hr = pFilter->EnumPins(&pEnum);
	if (FAILED(hr))
		return hr;

	JIF(pGB->QueryInterface(IID_IFilterGraph2, (void **)&pFG));
	// Look for the first unconnected pin
	while (pEnum->Next(1, &pPin, NULL) == S_OK)
	{
		PIN_DIRECTION ThisPinDir;

		pPin->QueryDirection(&ThisPinDir);
		if (ThisPinDir == PinDir)
		{
			IPin *pTmp = 0;

			hr = pPin->ConnectedTo(&pTmp);
			if (SUCCEEDED(hr))  // Already connected, not the pin we want.
			{
				pTmp->Release();
			}
			else  // Unconnected, this is the pin we want.
			{
				//			pEnum->Release();
				//			*ppPin = pPin;
				hr = pFG->RenderEx(pPin, AM_RENDEREX_RENDERTOEXISTINGRENDERERS, NULL);
				if(FAILED(hr))
				{
					pGB->Render(pPin);
				}
				//				return S_OK;
			}
		}
		pPin->Release();
	}

	// Release the enumerator
	pEnum->Release();

	// Did not find a matching pin
	return S_OK;
}
HRESULT CDShow::GetUnConnectPin( IGraphBuilder *pGB,
	IBaseFilter *pFilter,   // Pointer to the filter.
	PIN_DIRECTION PinDir,   // Direction of the pin to find.
	IPin **ppPin,int &num)           // Receives a pointer to the pin.)
{
	IEnumPins *EnumPin=0;
	IPin *pin=0;
	HRESULT hr;
	int count =0;
	hr=pFilter->EnumPins(&EnumPin);
	if(FAILED(hr))
	{
		return E_FAIL;
	}

	while(EnumPin->Next(1,&pin,NULL) == S_OK)
	{
		PIN_DIRECTION ThisPinDir;   //인핀인지 아웃 핀인지 판별하기위한 변수 
		pin->QueryDirection(&ThisPinDir);
		IPin *TPin=0;
		if (ThisPinDir == PinDir)  // 판별하기 
		{
			hr = pin->ConnectedTo(&TPin);
			if (FAILED(hr))
			{
				ppPin[count] = pin;
				count++;
			}else
			{

			}
		}

	}
	return S_OK;


}

HRESULT CDShow::RenderFileToVMR9(IGraphBuilder *pGB, WCHAR *wFileName, 
	IBaseFilter *pRenderer, BOOL bRenderAudio)
{
	HRESULT hr=S_OK;
	CComPtr <IPin> pOutputPin;
	CComPtr <IBaseFilter> pSource;
	CComPtr <IBaseFilter> pAudioRenderer;
	CComPtr <IFilterGraph2> pFG;

	if (SUCCEEDED(CoCreateInstance(CLSID_DSoundRender, NULL, CLSCTX_INPROC_SERVER, 
		IID_IBaseFilter, (void **)&pAudioRenderer)))
	{
		// The audio renderer was successfully created, so add it to the graph
		JIF(pGB->AddFilter(pAudioRenderer, L"Audio Renderer"));
	}


	if (FAILED(hr = pGB->AddSourceFilter(wFileName, L"SOURCE", &pSource)))
	{
		USES_CONVERSION;
		TCHAR szMsg[MAX_PATH + 128];

		wsprintf(szMsg, TEXT("Failed to add the source filter to the graph!  hr=0x%x\r\n\r\n")
			TEXT("Filename: %s\0"), hr, W2T(wFileName));
		MessageBox(NULL, szMsg, TEXT("Failed to render file to VMR9"), MB_OK | MB_ICONERROR);
		return hr;
	}

	// Get the interface for the first unconnected output pin
	JIF(GetUnconnectedPin(pGB,pSource, PINDIR_OUTPUT, &pOutputPin));

	// Render audio if requested (defaults to TRUE)

	/*	if (bRenderAudio)
	{
	// Because we will be rendering with the RENDERTOEXISTINGRENDERERS flag,
	// we need to create an audio renderer and add it to the graph.  
	// Create an instance of the DirectSound renderer (for each media file).
	//
	// Note that if the system has no sound card (or if the card is disabled),
	// then creating the DirectShow renderer will fail.  In that case,
	// handle the failure quietly.
	if (SUCCEEDED(CoCreateInstance(CLSID_DSoundRender, NULL, CLSCTX_INPROC_SERVER, 
	IID_IBaseFilter, (void **)&pAudioRenderer)))
	{
	// The audio renderer was successfully created, so add it to the graph
	JIF(pGB->AddFilter(pAudioRenderer, L"Audio Renderer"));
	}
	}

	// Get an IFilterGraph2 interface to assist in building the
	// multifile graph with the non-default VMR9 renderer
	JIF(pGB->QueryInterface(IID_IFilterGraph2, (void **)&pFG));

	// Render the output pin, using the VMR9 as the specified renderer.  This is 
	// necessary in case the GraphBuilder needs to insert a Color Space convertor,
	// or if multiple filters insist on using multiple allocators.
	// The audio renderer will also be used, if the media file contains audio.
	JIF(pFG->RenderEx(pOutputPin, AM_RENDEREX_RENDERTOEXISTINGRENDERERS, NULL));

	// If this media file does not contain an audio stream, then the 
	// audio renderer that we created will be unconnected.  If left in the 
	// graph, it could interfere with rate changes and timing.
	// Therefore, if the audio renderer is unconnected, remove it from the graph.
	*/	if (pAudioRenderer != NULL)
	{
		int num;
		IPin *pUnconnectedPin[10]={0,};


		// Is the audio renderer's input pin connected?
		HRESULT hrPin = GetUnConnectPin(pGB,pAudioRenderer, PINDIR_INPUT, pUnconnectedPin,num);

		// If there is an unconnected pin, then remove the unused filter
		if (num == 0) 
		{
			// Release the returned IPin interface
			// Remove the audio renderer from the graph
			hrPin = pGB->RemoveFilter(pAudioRenderer);
		}
	}

	return hr;
}

void CDShow::play(int state)
{
	CComQIPtr<IMediaControl> pMC(m_pGB);
	if (pMC != NULL)
	{
		if (state == -1)
		{
			if (m_play_state == State_Paused || m_play_state == State_Stopped)
				m_play_state = State_Running;
			else if (m_play_state == State_Running)
				m_play_state = State_Paused;
			else
				return;
		}
		else
		{
			m_play_state = state;
		}

		if (m_play_state == State_Stopped)
			pMC->Stop();
		else if (m_play_state == State_Paused)
			pMC->Pause();
		else if (m_play_state == State_Running)
			pMC->Run();
	}
}

void CDShow::set_video_position(CRect r)
{
	if (!m_pGB)
		return;

	if (m_pVMRWC)
	{
		m_pVMRWC->SetVideoPosition(NULL, &r);
	}
	else if (m_pVDC)
	{
		RECT rc = r;
		m_pVDC->SetVideoPosition(NULL, &rc);
	}
	else
	{
		CComQIPtr<IVideoWindow> pVW(m_pGB);
		if (pVW)
			pVW->SetWindowPosition(r.left, r.top, r.Width(), r.Height());
	}

	//paused/stopped 상태에서는 새 프레임이 들어오지 않아 위치 변경 후 stale 픽셀이 남음.
	//RepaintVideo 로 backbuffer 의 마지막 프레임을 새 영역에 강제로 다시 그림.
	if (m_pMC)
	{
		OAFilterState state = State_Running;
		if (SUCCEEDED(m_pMC->GetState(0, &state)) && state != State_Running)
		{
			if (m_pVMRWC && m_pParent && m_pParent->GetSafeHwnd())
			{
				HDC hdc = ::GetDC(m_pParent->GetSafeHwnd());
				m_pVMRWC->RepaintVideo(m_pParent->GetSafeHwnd(), hdc);
				::ReleaseDC(m_pParent->GetSafeHwnd(), hdc);
			}
			else if (m_pVDC)
			{
				m_pVDC->RepaintVideo();
			}
		}
	}
}

double CDShow::get_track_pos()
{
	if (!m_pGB || !m_pMP)
		return 0.0;

	REFTIME pos;

	m_pMP->get_CurrentPosition(&pos);
	return pos * 1000.0;
}

void CDShow::set_track_pos(double pos)
{
	if (!m_pGB || !m_pMS)
		return;

	HRESULT hr;

	/*
	if (false)
	{
		CComQIPtr<IMediaPosition> pMP(m_pGB);
		if (pMP != NULL)
		{
			pMP->put_CurrentPosition(pos / 1000.0);
		}
	}	
	else
	*/
	{
		//CComQIPtr<IMediaSeeking> pMC(m_pGB);
		//hr = pMC->IsFormatSupported(&TIME_FORMAT_FRAME);
		LONGLONG lPos = (LONGLONG)(pos * 10000.0);
		//HRESULT hr = pMC->SetPositions(&lPos, AM_SEEKING_SeekToKeyFrame, NULL, AM_SEEKING_AbsolutePositioning);
		hr = m_pMS->SetPositions(&lPos, AM_SEEKING_AbsolutePositioning | AM_SEEKING_SeekToKeyFrame, 0, 0);

		//set_track_pos에서도 msg_track_moved를 보내면
		//메인에 많은 혼란이 온다.
		//메시지는 보내지 말고 메인에서 set_track_pos한 후 필요한 조치를 취하도록 하자.
		//::PostMessage(m_pParent->m_hWnd, MESSAGE_DSHOW_MEDIA, msg_track_moved, (LPARAM)pos);
	}
}

void CDShow::move_track(bool forward, int interval)
{
	if (!m_pGB || !m_pMP)
		return;

	if (interval < 1)
		interval = m_default_interval;

	if (IsCtrlPressed())
		interval = m_control_interval;

	REFTIME pos;
	m_pMP->get_CurrentPosition(&pos);
	pos = pos + (forward ? interval : -interval);
	m_pMP->put_CurrentPosition(pos < 0 ? 0 : pos);

	//forward 를 wParam 의 상위 word 에 (msg_track_moved 와 함께) 인코딩하지 않고 wParam 에는 msg, lParam 에 interval 그대로 (signed int).
	//forward 부호는 lParam 의 signed bit 로 표현 — backward 면 음수.
	::PostMessage(m_pParent->m_hWnd, MESSAGE_DSHOW_MEDIA, msg_track_moved, (LPARAM)(forward ? interval : -interval));
}

void CDShow::step_frame(bool forward)
{
	if (!m_pGB || !m_pMP)
		return;

	if (get_play_state() == State_Stopped)
		return;

	if (get_play_state() == State_Running)
		play(State_Paused);

	REFTIME pos;
	double interval = 1.0 / m_frame_rate;
	m_pMP->get_CurrentPosition(&pos);
	m_pMP->put_CurrentPosition(pos + (forward ? interval : -interval));
}

void CDShow::select_stream(bool video, int index)
{
	if (!m_pSplitter)
		return;

	//stream 분석
	CComQIPtr<IAMStreamSelect> pStreamSelect(m_pSplitter);
	if (!pStreamSelect)
		return;

	if (video)
		m_video_stream_index = index;
	else
		m_audio_stream_index = index;

	if (video)
		pStreamSelect->Enable(m_video_stream[index].get_index(), AMSTREAMSELECTENABLE_ENABLE);
	else
		pStreamSelect->Enable(m_audio_stream[index].get_index(), AMSTREAMSELECTENABLE_ENABLE);
}

void CDShow::select_subtitle_stream(int index)
{
	if (!m_pSplitter)
		return;

	CComQIPtr<IAMStreamSelect> pStreamSelect(m_pSplitter);
	if (!pStreamSelect)
		return;

	//index < 0 = 임베디드 자막 stream 전체 비활성화 (외부 자막 우선용).
	//splitter 의 default active stream 도 disable 해야 하므로 m_subtitle_stream_index 가 -1 이어도 전체 loop.
	if (index < 0)
	{
		for (size_t k = 0; k < m_subtitle_stream.size(); k++)
			pStreamSelect->Enable(m_subtitle_stream[k].get_index(), 0);
		m_subtitle_stream_index = -1;
		return;
	}
	if (index >= (int)m_subtitle_stream.size())
		return;

	pStreamSelect->Enable(m_subtitle_stream[index].get_index(), AMSTREAMSELECTENABLE_ENABLE);
	m_subtitle_stream_index = index;
}

void CDShow::reselect_current_subtitle_stream()
{
	if (!m_pSplitter || m_subtitle_stream_index < 0)
		return;
	if (m_subtitle_stream_index >= (int)m_subtitle_stream.size())
		return;

	CComQIPtr<IAMStreamSelect> pStreamSelect(m_pSplitter);
	if (!pStreamSelect)
		return;

	int abs_idx = m_subtitle_stream[m_subtitle_stream_index].get_index();
	pStreamSelect->Enable(abs_idx, 0);
	pStreamSelect->Enable(abs_idx, AMSTREAMSELECTENABLE_ENABLE);
}

double CDShow::get_playback_rate()
{
	if (!m_pGB || !m_pMP)
		return 1.0;

	double rate;
	m_pMP->get_Rate(&rate);
	return rate;
}

void CDShow::set_playback_rate(double rate)
{
	if (!m_pGB || !m_pMP)
		return;

	m_pMP->put_Rate(rate);
}

#include <atlimage.h>
bool CDShow::capture_frame(CString sfile)
{
	if (!is_media_opened() || !m_VMR || !m_pVMRWC)
		return false;

	HRESULT hr;
	BYTE *lpDib = NULL;
	CImage img;

	hr = m_pVMRWC->GetCurrentImage(&lpDib);

	if (SUCCEEDED(hr))
	{
		BITMAPINFOHEADER  *pBMI = (BITMAPINFOHEADER*) lpDib;

		/*
		BITMAPFILEHEADER	hdr;
		DWORD				dwWritten = 0;
			
		int nColors = 0;
		if (pBMI->biBitCount <= 8)
		{
			nColors = (1 << pBMI->biBitCount);
		}

		// Prepare FileHeader
		hdr.bfType = 0x4d42;        // 0x42 = "B" 0x4d = "M" 
		hdr.bfReserved1 = 0; 
		hdr.bfReserved2 = 0; 

		// Compute the size of the entire file. 
		hdr.bfSize = (DWORD) (sizeof(BITMAPFILEHEADER) + 
			pBMI->biSize + pBMI->biClrUsed
			* sizeof(RGBQUAD) + pBMI->biSizeImage); 

		// Compute the offset to the array of color indices. 
		hdr.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER) + 
			pBMI->biSize + pBMI->biClrUsed 
			* sizeof (RGBQUAD); 

		// Create the .BMP file. 
		HANDLE hf = CreateFile(sfile, 
			GENERIC_READ | GENERIC_WRITE, 
			(DWORD) 0, 
			NULL, 
			CREATE_ALWAYS, 
			FILE_ATTRIBUTE_NORMAL, 
			(HANDLE) NULL); 
		if (hf == INVALID_HANDLE_VALUE) 
		{
			//sMessage = "Cannot create file";
			//bResult = FALSE;
			goto CLEANUP;
		}

		// Copy the BITMAPFILEHEADER into the .BMP file. 
		if (!WriteFile(hf, (LPVOID) &hdr, sizeof(BITMAPFILEHEADER), 
			(LPDWORD) &dwWritten,  NULL)) 
		{
			//sMessage = "Cannot write to file";
			//bResult = FALSE;
			goto CLEANUP;
		}

		// Copy the BITMAPINFOHEADER and RGBQUAD array into the file. 
		if (!WriteFile(hf, (LPVOID) pBMI, sizeof(BITMAPINFOHEADER) 
			+ pBMI->biClrUsed * sizeof (RGBQUAD), 
			(LPDWORD) &dwWritten, ( NULL)) )
		{
			//sMessage = "Cannot write to file";
			//bResult = FALSE;
			goto CLEANUP;
		}
		*/

		// Copy the array of color indices into the .BMP file. 
		LPBYTE hp = (LPBYTE)pBMI + (pBMI->biSize);// + nColors * sizeof(RGBQUAD)); 
		/*
		if (!WriteFile(hf, (LPSTR) hp, (int) pBMI->biSizeImage, (LPDWORD) &dwWritten,NULL)) 
		{
			//sMessage = "Cannot write to file";
			//bResult = FALSE;
			goto CLEANUP;
		}
		*/
		img.Create(pBMI->biWidth, pBMI->biHeight, pBMI->biBitCount);
		BITMAPINFO bmInfo;
		bmInfo.bmiHeader = *pBMI;
		HDC dc = img.GetDC();
		SetDIBitsToDevice(dc, 0, 0, pBMI->biWidth, pBMI->biHeight, 0, 0, 0, pBMI->biHeight, hp, &bmInfo, DIB_RGB_COLORS);
		hr = img.Save(sfile);
		img.ReleaseDC();

		if (get_play_state() == State_Paused)
			step_frame(true);

		CoTaskMemFree(lpDib);

		if (hr == S_OK)
			return true;
	}

	return false;
}

DWORD CDShow::get_aspect_ratio_mode()
{
	if (!m_pGB || !m_pVMRWC)
		return 2;

	DWORD ratio_mode;
	m_pVMRWC->GetAspectRatioMode(&ratio_mode);
	return ratio_mode;
}

void CDShow::set_aspect_ratio_mode(int mode)
{
	if (!m_pVMRWC)
		return;

	DWORD ratio_mode;
	m_pVMRWC->GetAspectRatioMode(&ratio_mode);
	if (mode == -1)
	{
		if (ratio_mode == VMR_ARMODE_LETTER_BOX)
			ratio_mode = VMR_ARMODE_NONE;
		else
			ratio_mode = VMR_ARMODE_LETTER_BOX;
	}
	else if (mode == 0)
	{
		ratio_mode = VMR_ARMODE_NONE;
	}
	else if (mode == 1)
	{
		ratio_mode = VMR_ARMODE_LETTER_BOX;
	}

	m_pVMRWC->SetAspectRatioMode(ratio_mode);
}

void CDShow::show_subtitle_property_page()
{
	if (!m_use_dvs)
		return;

	ShowFilterPropertyPage(_T("DirectVobSub (auto-loading version)"));
}

void CDShow::DirectVobSub_function(WPARAM wParam, LPARAM lParam)
{
	if (!m_use_dvs)
		return;

	if (!m_hDirectVobSubWnd)
	{
		m_hDirectVobSubWnd = FindWindow(0, _T("DVSWND"));
		if (!m_hDirectVobSubWnd)
			return;
	}

	//SendMessage(m_hDirectVobSubWnd, WM_DVSMESSAGE, msg_parent_hwnd, (LPARAM)(m_pParent->GetSafeHwnd()));
	PostMessage(m_hDirectVobSubWnd, WM_DVSMESSAGE, wParam, lParam);

	if (wParam == msg_parent_hwnd)
	{
		//맨 처음 미디어가 재생되면 메인의 m_hWnd가 전달되는데
		//이 때 자막 싱크값도 디폴트 싱크값으로 리셋시킨다.
		m_subtitle_sync = 0;
		PostMessage(m_hDirectVobSubWnd, WM_DVSMESSAGE, msg_put_SubtitleTiming, m_subtitle_sync);

		//자막의 x, y placement 설정값도 요청하여 endorphin에서 받아서
		//이 클래스의 변수에 넣어주자.
		//PostMessage(m_hDirectVobSubWnd, WM_DVSMESSAGE, msg_get_Placement, 0);

		//우선 dvs을 사용하지 않기 위해 고정 위치에 표시한다.
		PostMessage(m_hDirectVobSubWnd, WM_DVSMESSAGE, msg_put_Placement, MAKEWORD(50, 30));
	}
	else if (wParam == msg_put_FileName)
	{
		m_has_subtitle = true;
		m_subtitle_file = (LPCTSTR)lParam;
	}
}

void CDShow::set_video_pan_scan(DWORD dwStreamID, int mode, float dx, float dy)
{
	if (!m_pVMRMC)
		return;

	VMR9NormalizedRect rc;
	m_pVMRMC->GetOutputRect(dwStreamID, &rc);

	if (mode == pan_scan_origin)
	{
		rc.left = 0.0f;
		rc.right = 1.0f;
		rc.top = 0.0f;
		rc.bottom = 1.0f;
	}
	else if (mode == pan_scan_size)
	{
		rc.left -= dx;
		rc.right += dx;
		rc.top -= dy;
		rc.bottom += dy;
	}
	else if (mode == pan_scan_move)
	{
		rc.left += dx;
		rc.right += dx;
		rc.top += dy;
		rc.bottom += dy;
	}
	else if (mode == pan_scan_mirror)
	{
		m_mirror = !m_mirror;
		SWAP(rc.left, rc.right);
	}
	else if (mode == pan_scan_flip)
	{
		m_flip = !m_flip;
		SWAP(rc.top, rc.bottom);
	}

	m_pVMRMC->SetOutputRect(0, &rc);
}

//ProcAmp 변경 후 paused/stopped 상태에서도 즉시 시각 반영시키기 위한 redraw.
//RepaintVideo 만으론 mixer 가 현재 backbuffer 를 단순 복사할 뿐 새 ProcAmp 로 *재합성* 안 함.
//paused 일 땐 같은 위치로 seek 해 mixer 가 frame 을 디코더에서 다시 받아 새 ProcAmp 적용 후 push.
//PotPlayer/MPC 가 paused 시 화질 변경을 즉시 반영하는 정공.
//호출처의 HDC 로 즉시 backbuffer 의 가장 최근 frame 다시 그림. resize 중 OS erase 가 stale 픽셀
//남기는 깜빡임 영역을 비디오로 메우기 위해 OnEraseBkgnd 에서 사용.
void CDShow::repaint_video(HDC hdc)
{
	if (hdc == NULL)
		return;
	if (m_pVMRWC && m_pParent && m_pParent->GetSafeHwnd())
		m_pVMRWC->RepaintVideo(m_pParent->GetSafeHwnd(), hdc);
	else if (m_pVDC)
		m_pVDC->RepaintVideo();
}


void CDShow::repaint_for_procamp_change()
{
	if (m_pMC && m_pMS)
	{
		OAFilterState state = State_Stopped;
		if (SUCCEEDED(m_pMC->GetState(0, &state)) && state == State_Paused)
		{
			LONGLONG cur = 0;
			if (SUCCEEDED(m_pMS->GetCurrentPosition(&cur)))
				m_pMS->SetPositions(&cur, AM_SEEKING_AbsolutePositioning,
					NULL, AM_SEEKING_NoPositioning);
		}
	}

	if (m_pVMRWC && m_pParent && m_pParent->GetSafeHwnd())
	{
		HDC hdc = ::GetDC(m_pParent->GetSafeHwnd());
		m_pVMRWC->RepaintVideo(m_pParent->GetSafeHwnd(), hdc);
		::ReleaseDC(m_pParent->GetSafeHwnd(), hdc);
	}
}

int CDShow::adjust_video(int dwStreamID, int target, bool up)
{
	if (!m_pVMRMC)
		return -1;

	//enum VIDEO_ADJUST 와 1:1 매핑. contrast=0, bright=1, hue=2, saturation=3.
	const DWORD flag_table[4] = {
		ProcAmpControl9_Contrast,
		ProcAmpControl9_Brightness,
		ProcAmpControl9_Hue,
		ProcAmpControl9_Saturation,
	};

	const DWORD all_flags = ProcAmpControl9_Contrast | ProcAmpControl9_Brightness |
	                        ProcAmpControl9_Hue | ProcAmpControl9_Saturation;

	//target == -1 : 4채널 모두 driver default 로 복원.
	if (target == -1)
	{
		VMR9ProcAmpControl ctrl = {0,};
		ctrl.dwSize = sizeof(ctrl);
		ctrl.dwFlags = all_flags;
		if (FAILED(m_pVMRMC->GetProcAmpControl(dwStreamID, &ctrl)))
			return -1;

		for (int i = 0; i < 4; ++i)
		{
			VMR9ProcAmpControlRange range;
			range.dwSize = sizeof(range);
			range.dwProperty = (VMR9ProcAmpControlFlags)flag_table[i];
			if (FAILED(m_pVMRMC->GetProcAmpControlRange(dwStreamID, &range)))
				continue;
			switch (flag_table[i])
			{
			case ProcAmpControl9_Contrast:   ctrl.Contrast   = range.DefaultValue; break;
			case ProcAmpControl9_Brightness: ctrl.Brightness = range.DefaultValue; break;
			case ProcAmpControl9_Hue:        ctrl.Hue        = range.DefaultValue; break;
			case ProcAmpControl9_Saturation: ctrl.Saturation = range.DefaultValue; break;
			}
		}
		ctrl.dwFlags = all_flags;
		m_pVMRMC->SetProcAmpControl(dwStreamID, &ctrl);
		repaint_for_procamp_change();
		return 0;
	}

	if (target < 0 || target > 3)
		return -1;

	const DWORD flag = flag_table[target];

	VMR9ProcAmpControlRange range;
	range.dwSize = sizeof(range);
	range.dwProperty = (VMR9ProcAmpControlFlags)flag;
	if (FAILED(m_pVMRMC->GetProcAmpControlRange(dwStreamID, &range)))
		return -1;

	VMR9ProcAmpControl ctrl = {0,};
	ctrl.dwSize = sizeof(ctrl);
	ctrl.dwFlags = all_flags;
	if (FAILED(m_pVMRMC->GetProcAmpControl(dwStreamID, &ctrl)))
		return -1;

	const float step = (range.MaxValue - range.MinValue) / 100.0f;	//1% per click (PotPlayer 호환)
	float* pVal = NULL;
	switch (flag)
	{
	case ProcAmpControl9_Contrast:   pVal = &ctrl.Contrast; break;
	case ProcAmpControl9_Brightness: pVal = &ctrl.Brightness; break;
	case ProcAmpControl9_Hue:        pVal = &ctrl.Hue; break;
	case ProcAmpControl9_Saturation: pVal = &ctrl.Saturation; break;
	}
	if (!pVal)
		return -1;

	*pVal += up ? step : -step;
	if (*pVal < range.MinValue) *pVal = range.MinValue;
	if (*pVal > range.MaxValue) *pVal = range.MaxValue;

	//SetProcAmpControl 은 dwFlags 에 set 된 비트만 반영. 다른 채널값은 GetProcAmpControl 로 미리 채워둠.
	ctrl.dwFlags = flag;
	if (FAILED(m_pVMRMC->SetProcAmpControl(dwStreamID, &ctrl)))
		return -1;

	repaint_for_procamp_change();

	//현재 값을 0~100 % 로 변환해 반환 (OSD 표시용).
	return (int)(((*pVal - range.MinValue) / (range.MaxValue - range.MinValue)) * 100.0f + 0.5f);
}

int CDShow::set_video_adjust(int dwStreamID, int target, int percent)
{
	if (!m_pVMRMC || target < 0 || target > 3)
		return -1;
	if (percent < 0)   percent = 0;
	if (percent > 100) percent = 100;

	const DWORD flag_table[4] = {
		ProcAmpControl9_Contrast,
		ProcAmpControl9_Brightness,
		ProcAmpControl9_Hue,
		ProcAmpControl9_Saturation,
	};
	const DWORD flag = flag_table[target];

	VMR9ProcAmpControlRange range;
	range.dwSize = sizeof(range);
	range.dwProperty = (VMR9ProcAmpControlFlags)flag;
	if (FAILED(m_pVMRMC->GetProcAmpControlRange(dwStreamID, &range)))
		return -1;

	const DWORD all_flags = ProcAmpControl9_Contrast | ProcAmpControl9_Brightness |
	                        ProcAmpControl9_Hue | ProcAmpControl9_Saturation;
	VMR9ProcAmpControl ctrl = {0,};
	ctrl.dwSize = sizeof(ctrl);
	ctrl.dwFlags = all_flags;
	if (FAILED(m_pVMRMC->GetProcAmpControl(dwStreamID, &ctrl)))
		return -1;

	float val = range.MinValue + (range.MaxValue - range.MinValue) * (percent / 100.0f);
	switch (flag)
	{
	case ProcAmpControl9_Contrast:   ctrl.Contrast   = val; break;
	case ProcAmpControl9_Brightness: ctrl.Brightness = val; break;
	case ProcAmpControl9_Hue:        ctrl.Hue        = val; break;
	case ProcAmpControl9_Saturation: ctrl.Saturation = val; break;
	}
	ctrl.dwFlags = flag;
	if (FAILED(m_pVMRMC->SetProcAmpControl(dwStreamID, &ctrl)))
		return -1;
	repaint_for_procamp_change();
	return percent;
}

void CDShow::volume_up(bool up, int interval)
{
	if (up)
	{
		if (m_volume < 30)
			interval = 2;
		m_volume += interval;
	}
	else
	{
		if (m_volume <= 30)
			interval = 2;
		m_volume -= interval;
	}

	if (m_volume < 0)
		m_volume = 0;
	else if (m_volume > VOLUME_MAX)
		m_volume = VOLUME_MAX;

	set_volume(m_volume);
}

void CDShow::set_volume(int volume, bool reset_mute)
{
	if (volume < 0)
	{
		m_volume_mute = !m_volume_mute;
	}
	else
	{
		m_volume = volume;
		if (reset_mute)
			m_volume_mute = false;

		if (m_volume < 0)
			m_volume = 0;
		else if (m_volume > VOLUME_MAX)
			m_volume = VOLUME_MAX;

		AfxGetApp()->WriteProfileInt(_T("setting"), _T("volume"), m_volume);
	}

	AfxGetApp()->WriteProfileInt(_T("setting"), _T("volume mute"), m_volume_mute);

	if (m_pGB == NULL)
		return;

	CComQIPtr<IBasicAudio> pBA(m_pGB);
	if (!pBA)
		return;

	//Volume 매핑 — piece-wise 곡선:
	//  v=0~10  : -25 dB → -10 dB (가파른 attenuation, 작은 영역의 미세 조정)
	//  v=10~100: -10 dB → +30 dB (점진 boost, 큰 영역의 PotPlayer 비교 음량)
	//   0  : mute
	//   1  : -23.5 dB (거의 안 들림 — 작은 음량 표현)
	//   2  : -22 dB
	//   5  : -17.5 dB
	//  10  : -10 dB (분기점, 작음)
	//  20  : -5.56 dB
	//  33  : ≈ 0 dB (IBasicAudio max — boost 분기)
	//  50  : +7.78 dB
	// 100  : +30 dB
	//전 영역 boost 는 작은 영역 표현 불가 / 전 영역 attenuation 은 큰 영역 부족 — piece-wise 가 균형.
	const int v = m_volume_mute ? 0 : m_volume;
	logWrite(_T("[set_volume] v=%d gain=%p comp=%p"), v, m_pAudioGainFilter, m_pAudioCompressorFilter);

	//gain filter 는 chain 에 있지만 항상 0 dB (passthrough). amplification 은 compressor makeup 으로.
	set_audio_gain_db(0.0f);
	if (v <= 0)
	{
		pBA->put_Volume(-10000);
		set_audio_compressor_makeup_db(0.0f);
	}
	else
	{
		double total_db;
		if (v <= 10)
			total_db = -25.0 + (double)v * 15.0 / 10.0;			//v=1: -23.5, v=10: -10
		else
			total_db = -10.0 + (double)(v - 10) * 40.0 / 90.0;	//v=10: -10, v=100: +30

		if (total_db <= 0.0)
		{
			long centibels = (long)(total_db * 100.0);
			if (centibels < -10000) centibels = -10000;
			pBA->put_Volume(centibels);
			set_audio_compressor_makeup_db(0.0f);
		}
		else
		{
			pBA->put_Volume(0);
			set_audio_compressor_makeup_db((float)total_db);
		}
	}
}

#ifdef REGISTER_FILTERGRAPH
HRESULT CDShow::AddGraphToRot(IUnknown *pUnkGraph, DWORD *pdwRegister) 
{
	IMoniker * pMoniker;
	IRunningObjectTable *pROT;

	if (FAILED(GetRunningObjectTable(0, &pROT))) 
	{
		return E_FAIL;
	}

	WCHAR wsz[256];
	wsprintfW(wsz, L"FilterGraph %08x pid %08x", (DWORD_PTR)pUnkGraph, 
		GetCurrentProcessId());

	HRESULT hr = CreateItemMoniker(L"!", wsz, &pMoniker);

	if (SUCCEEDED(hr)) 
	{
		// Use the ROTFLAGS_REGISTRATIONKEEPSALIVE to ensure a strong reference
		// to the object.  Using this flag will cause the object to remain
		// registered until it is explicitly revoked with the Revoke() method.
		//
		// Not using this flag means that if GraphEdit remotely connects
		// to this graph and then GraphEdit exits, this object registration 
		// will be deleted, causing future attempts by GraphEdit to fail until
		// this application is restarted or until the graph is registered again.
		hr = pROT->Register(ROTFLAGS_REGISTRATIONKEEPSALIVE, pUnkGraph, 
			pMoniker, pdwRegister);

		pMoniker->Release();
	}

	pROT->Release();
	return hr;
}

void CDShow::RemoveGraphFromRot(DWORD pdwRegister)
{
	IRunningObjectTable *pROT;

	if (SUCCEEDED(GetRunningObjectTable(0, &pROT))) 
	{
		pROT->Revoke(pdwRegister);
		pROT->Release();
	}
}
#endif

HRESULT CDShow::HandleGraphEvent(WPARAM wParam,LPARAM lparam)
{
	if (m_pME == NULL)
		return E_ABORT;

	long lEvent = 0;
	LONG_PTR lParam1 = 0, lParam2 = 0, msTimeOut = 0;

	while (SUCCEEDED(m_pME->GetEvent(&lEvent, &lParam1, &lParam2, msTimeOut)))
	{
		switch(lEvent)
		{
		case EC_OPENING_FILE :
			TRACE(_T("EC_OPENING_FILE...\n"));
			AfxMessageBox(_T("EC_OPENING_FILE"));
			break;
		case EC_BUFFERING_DATA :
			TRACE(_T("EC_BUFFERING_DATA...\n"));
			AfxMessageBox(_T("EC_BUFFERING_DATA"));
			break;
		case EC_USERABORT : // 사용자가 강제로 끝낼때
			//OnUserStop(wParam,lparam);
			break;
		case EC_COMPLETE: // 재생이 완벽하게 끝날을때
		{
			::SendMessage(m_pParent->m_hWnd, MESSAGE_DSHOW_MEDIA, msg_ec_complete, 0);
			/*
			CComQIPtr<IMediaPosition> pMP(m_pGB);
			if (pMP != NULL)
			{
				pMP->put_CurrentPosition(0);
				CComQIPtr<IMediaControl > pMC(m_pGB);
				if (pMC != NULL)
				{
					pMC->Pause();
				}

			}
			*/
		}
		break;
		case EC_OLE_EVENT : //필터에서 TEXT문자열을 읽었을때 어플에게 보낸는 이벤트
			//BlendAppicationText();
			//TRACE(_T("%s, %s\n"), lParam1, lParam2);
			//TextSmiDisplay(lParam1,lParam2);
			/*
			if (text != NULL)
			{
				delete[] text;
				text = NULL;
			}
			if (content != NULL)
			{
				delete[] content;
				content = NULL;
			}
			*/
			break;
		}

		m_pME->FreeEventParams(lEvent, lParam1, lParam2);
	}

	return NOERROR;
}

void CDShow::prepare_AlphaBitmap()
{
	if (!m_pVMRWC)
		return;

	m_pParentDC = m_pParent->GetDC();

	float fZoom = 1.0f;

	for (int i = 0; i < 2; i++)
	{
		m_pMemDC[i] = new CDC();
		m_pMemDC[i]->CreateCompatibleDC(m_pParentDC);

		m_pBitmap[i] = new CBitmap();
		m_pBitmap[i]->CreateCompatibleBitmap(m_pParentDC, m_video_size.cx * fZoom, m_video_size.cy * fZoom);
		m_pMemDC[i]->SelectObject(m_pBitmap[i]);
		m_pMemDC[i]->SetBkMode(TRANSPARENT);
	}

	m_crColorKey = RGB(0, 31, 1);

	m_buf_index = 0;
	RECT reText;
	SetRect(&reText, 0, 0, m_video_size.cx * fZoom, m_video_size.cy * fZoom);

	ZeroMemory(&m_AlphaBitmap,sizeof(m_AlphaBitmap));
	m_AlphaBitmap.dwFlags = VMRBITMAP_HDC;
	m_AlphaBitmap.hdc = m_pMemDC[m_buf_index]->GetSafeHdc();        // 나타낼 메모리 
	m_AlphaBitmap.rDest.left = 0.0f;//EDGE_BUFFER;
	m_AlphaBitmap.rDest.right = 1.0f;// - EDGE_BUFFER;
	m_AlphaBitmap.rDest.top = 0.00f;
	m_AlphaBitmap.rDest.bottom = 1.0f;// -EDGE_BUFFER;
	m_AlphaBitmap.rSrc = reText;
	m_AlphaBitmap.fAlpha = 1.0f;//TRANSPARENCY_VALUE;
	m_AlphaBitmap.clrSrcKey = m_crColorKey;
	m_AlphaBitmap.dwFlags |= VMRBITMAP_SRCCOLORKEY;

	m_pVMRMB->SetAlphaBitmap(&m_AlphaBitmap);

	m_osd_text.Empty();
	m_cur_subtitle.reset();
}

HRESULT CDShow::update_osd_subtitle()
{
	if (!m_pVMRWC)
		return E_FAIL;

	int i;
	int sx = m_video_size.cx * m_subCfg.pos_x / 100.0;
	int sy = m_video_size.cy * m_subCfg.pos_y / 100.0;// -56;
	HRESULT hr = S_OK;

	Trace(_T("m_video_size = %d, %d"), m_video_size.cx, m_video_size.cy);
	m_pMemDC[m_buf_index]->SetTextAlign(TA_CENTER);
	m_pMemDC[m_buf_index]->SetTextCharacterExtra(m_subCfg.char_spacing);
	m_pMemDC[m_buf_index]->FillSolidRect(0, 0, m_video_size.cx, m_video_size.cy, m_crColorKey);

	Gdiplus::Graphics gr(m_pMemDC[m_buf_index]->GetSafeHdc());
	Gdiplus::StringFormat strFormat;
	Gdiplus::GraphicsPath path_outline;
	Gdiplus::GraphicsPath *path_text;
	Gdiplus::GraphicsPath *path_shadow;
	Gdiplus::Color color;

	gr.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
	gr.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
	gr.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);
	//gr.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

	if (m_osd_text.IsEmpty() == false)
	{
		Gdiplus::FontFamily osd_font(L"맑은 고딕");

		//osd display
		path_outline.AddString(m_osd_text, _tcslen(m_osd_text), &osd_font, Gdiplus::FontStyleRegular, 48,
			Gdiplus::Point(10, 10), &strFormat );

		path_text = path_outline.Clone();

		//외곽선
		Gdiplus::Pen pen(Gdiplus::Color(0, 0, 0), 8);
		pen.SetLineJoin(Gdiplus::LineJoinRound);

		//외곽선의 두께를 늘려준 다음
		path_outline.Widen(&pen);
		gr.FillPath(&Gdiplus::SolidBrush(Gdiplus::Color(212, 0, 0, 0)), &path_outline);

		color.SetFromCOLORREF(m_osd_color);
		Gdiplus::SolidBrush brush(color);
		gr.FillPath(&brush, path_text);

		path_outline.Reset();
		path_text->Reset();
	}


	if (m_show_subtitle && m_cur_subtitle.is_valid())
	{
		Gdiplus::FontFamily font(m_subCfg.lf->lfFaceName);
#if 0
		strFormat.SetAlignment(Gdiplus::StringAlignmentCenter);

		//라인 간격은 기본 120%를 기본으로 하되
		//사용자가 그 기본값에서 -50% ~ +50%를 조정할 수 있다.
		//조정 화면에서는 그 조정 범위만 표시되도록 하고
		//실제 적용하는 여기에서 120을 더해서 라인 간격을 계산한다.
		int line_spacing = -(double)m_subCfg.lf->lfHeight * (double)(m_subCfg.line_spacing + 120) / 100.0;

		for (i = m_cur_subtitle.sentences.size() - 1; i >= 0; i--)
		{
			TextDesigner::OutlineText td;

			Gdiplus::Color color = m_subCfg.cr[0];
			if (m_cur_subtitle.sentences[i].color.IsEmpty() == false)
			{
				COLORREF crText = get_color(m_cur_subtitle.sentences[i].color);
				color = Gdiplus::Color(m_subCfg.cr[0].GetA(), GetRValue(crText), GetGValue(crText), GetBValue(crText));
			}

			td.TextOutline(color, m_subCfg.cr[2], m_subCfg.outline_widthX * 2.5);
			//text.TextGlow(Gdiplus::Color(255, 255, 255), Gdiplus::Color(32, 255, 0, 0), 16);

			td.EnableShadow(true);

			td.Shadow(m_subCfg.cr[3],
									m_subCfg.shadow_depthX,
									Gdiplus::Point(m_subCfg.shadow_depthX, m_subCfg.shadow_depthY));
			/*
			td.DiffusedShadow(Gdiplus::Color(m_subCfg.alpha[3],
									GetRValue(m_subCfg.colors[3]),
									GetGValue(m_subCfg.colors[3]),
									GetBValue(m_subCfg.colors[3])),
									m_subCfg.shadow_depthX * 2,
									Gdiplus::Point(m_subCfg.shadow_depthX, m_subCfg.shadow_depthY));
			*/
			
			//이걸 쓰면 자간 조절이 안되고
			if (false)
			{
				td.DrawString(&gr, &font, (m_subCfg.lf->lfWeight == 0 ? Gdiplus::FontStyleRegular : Gdiplus::FontStyleBold),
					m_subCfg.lf->lfHeight * -1, m_cur_subtitle.sentences[i].sentence,
					Gdiplus::Point(sx, sy), &strFormat);
			}
			//이걸 쓰면 폰트 Weight가 적용이 안된다.
			//td에서 FW_BOLD만 bold로 처리되기 때문이었다.
			else
			{
				td.GdiDrawString(&gr, m_subCfg.lf, m_cur_subtitle.sentences[i].sentence,
					Gdiplus::Point(sx, sy));
			}

			sy -= line_spacing;
		}
#else
		//Gdiplus::Graphics gr(m_pMemDC[m_buf_index]->GetSafeHdc());
		//Gdiplus::StringFormat strFormat;
		//Gdiplus::GraphicsPath path_outline;
		//Gdiplus::GraphicsPath *path_text;
		//Gdiplus::GraphicsPath *path_shadow;
		Gdiplus::Matrix m;
		m.Translate(m_subCfg.shadow_depthX, m_subCfg.shadow_depthY);

		//gr.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
		//gr.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
		//gr.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);
		//gr.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

		strFormat.SetAlignment(Gdiplus::StringAlignmentCenter);

		//라인 간격은 기본 120%를 기본으로 하되
		//사용자가 그 기본값에서 -50% ~ +50%를 조정할 수 있다.
		//조정 화면에서는 그 조정 범위만 표시되도록 하고
		//실제 적용하는 여기에서 120을 더해서 라인 간격을 계산한다.
		//int line_spacing = -m_subCfg.lf->lfHeight * ((double)m_subCfg.line_spacing / 100.0);
		int line_spacing = -(double)m_subCfg.lf->lfHeight * (double)(m_subCfg.line_spacing + 120) / 100.0;

		for (i = m_cur_subtitle.sentences.size() - 1; i >= 0; i--)
		{
			path_outline.AddString(m_cur_subtitle.sentences[i].sentence,
							_tcslen(m_cur_subtitle.sentences[i].sentence),
							&font,
							(m_subCfg.lf->lfWeight == 500 ? Gdiplus::FontStyleBold : Gdiplus::FontStyleRegular),
							m_subCfg.lf->lfHeight * -1,
							Gdiplus::Point(m_video_size.cx / 2, sy), &strFormat );

			path_text = path_outline.Clone();

			//외곽선
			Gdiplus::Pen pen(Gdiplus::Color(128, 0, 0, 0), m_subCfg.outline_widthX * 2);
			pen.SetLineJoin(Gdiplus::LineJoinRound);

			//외곽선의 두께를 늘려준 다음
			path_outline.Widen(&pen);

			//그 값을 복사해서 이동시켜서 칠해주면 그림자가 된다.
			path_shadow = path_outline.Clone();
			path_shadow->Transform(&m);
			gr.FillPath(&Gdiplus::SolidBrush(Gdiplus::Color(/*m_subCfg.alphaShadow*/128, 16, 16, 16)), path_shadow);

			//그림자 위에 외곽선을 그려주고
			gr.FillPath(&Gdiplus::SolidBrush(Gdiplus::Color(/*m_subCfg.alphaOutline*/128, 0, 0, 0)), &path_outline);


			//마지막으로 실제 글자 출력
			Gdiplus::SolidBrush brush(m_subCfg.cr[0]);
			if (m_cur_subtitle.sentences[i].color.IsEmpty() == false)
			{
				Gdiplus::Color crText = get_color(m_cur_subtitle.sentences[i].color);
				brush.SetColor(crText);
			}
			gr.FillPath(&brush, path_text);

			path_outline.Reset();
			path_shadow->Reset();
			path_text->Reset();

			sy -= line_spacing;
		}
#endif
	}

	float fZoom = 1.0f;
	RECT reText;
	SetRect(&reText, 0, 0, m_video_size.cx * fZoom, m_video_size.cy * fZoom);

	ZeroMemory(&m_AlphaBitmap,sizeof(m_AlphaBitmap));
	m_AlphaBitmap.dwFlags = VMRBITMAP_HDC;
	m_AlphaBitmap.hdc = m_pMemDC[m_buf_index]->GetSafeHdc();        // 나타낼 메모리 
	m_AlphaBitmap.rDest.left = 0.0f;// + 10;//X_EDGE_BUFFER;
	m_AlphaBitmap.rDest.right = 1.0f;//textWidthRatioX;// + 10;//X_EDGE_BUFFER;
	m_AlphaBitmap.rDest.top = 0.0f;//(float)(cy - 100) / (float)cy - 0.05;
	m_AlphaBitmap.rDest.bottom = 1.0f;// -EDGE_BUFFER;
	m_AlphaBitmap.rSrc = reText;
	m_AlphaBitmap.clrSrcKey = m_crColorKey;
	m_AlphaBitmap.dwFlags |= VMRBITMAP_SRCCOLORKEY;
	//m_AlphaBitmap.fAlpha = (float)m_subCfg.alpha[0];
	m_AlphaBitmap.fAlpha = 0.5f;// (float)m_subCfg.alpha[0] / 255.0;
	m_pVMRMB->SetAlphaBitmap(&m_AlphaBitmap);

	//이걸 해주지 않으면 일시정지인 경우 OSD가 제대로 표시되지 않는다.
	//repaint는 먹히지 않아서 트랙 이동 방법을 사용했다.
	if (m_play_state == State_Paused)
	{
		//pVMRWC->RepaintVideo(m_pParent->GetSafeHwnd(), ::GetDC(m_pParent->GetSafeHwnd()));
		double d = get_track_pos();
		set_track_pos(d);
	}

	return hr;
}

void CDShow::prepare_next_subtitle(CString text, COLORREF crText)
{
	int index = !m_buf_index;

	m_pMemDC[index]->FillSolidRect(0, 0, m_video_size.cx, m_video_size.cy, m_crColorKey);

	Gdiplus::Graphics gr(m_pMemDC[index]->GetSafeHdc());
	Gdiplus::FontFamily osd_font(L"맑은 고딕");
	Gdiplus::StringFormat strFormat;
	Gdiplus::GraphicsPath path;

	//USES_CONVERSION;
	gr.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
	gr.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);

	//osd display
	path.AddString(text, _tcslen(text), &osd_font, Gdiplus::FontStyleRegular, 48,
		Gdiplus::Point(10, 10), &strFormat );
	for (int i = 0; i < 10; i++ )
	{
		Gdiplus::Pen pen(Gdiplus::Color(32, 4, 7, 4), i);
		pen.SetLineJoin(Gdiplus::LineJoinRound);
		gr.DrawPath(&pen, &path);
	}

	Gdiplus::SolidBrush brush(Gdiplus::Color(GetRValue(crText), GetGValue(crText), GetBValue(crText)));
	gr.FillPath(&brush, &path);
	path.Reset();
}

void CDShow::show_next_subtitle()
{

}

void CDShow::set_osd_text(CString text, COLORREF cr)
{
	m_osd_text = text;
	m_osd_color = cr;
	update_osd_subtitle();
}

void CDShow::set_subtitle_text(CCaption caption)
{
	if (m_cur_subtitle.start == caption.start)
		return;
	m_cur_subtitle = caption;
	update_osd_subtitle();
}

int CDShow::subtitle_font_enlarge(int enlarge)
{
	if (enlarge == 0)
		m_subCfg.font_size = 40;
	else
		m_subCfg.font_size += ((enlarge > 0) ? 1 : -1);

	Clamp(m_subCfg.font_size, 10, 100);

	m_subCfg.lf->lfHeight = get_pixel_size_from_font_size(m_pParent->m_hWnd, m_subCfg.font_size);
	update_osd_subtitle();

	return m_subCfg.font_size;
}

void CDShow::EnumFilters()
{
	// Reset filter number
	m_nFilter = 0;

	if ( m_pGB == NULL )
		return;

	HRESULT hr;
	IEnumFilters *pEnum = NULL;
	IBaseFilter *pFilter = NULL;
	ULONG cFetched;

	// Get filter enumerator
	hr = m_pGB->EnumFilters(&pEnum);
	if ( FAILED(hr) )
		return;

	// Enumerate all filters in the graph
	while ( pEnum->Next(1, &pFilter, &cFetched) == S_OK )
	{
		FILTER_INFO FilterInfo;
		TCHAR szName[512];

		hr = pFilter->QueryFilterInfo(&FilterInfo);
		if (FAILED(hr))
		{
			//m_ListFilters.AddString(TEXT("<ERROR>"));
			//			AfxMessageBox( "Error" );
		}
		else
		{
			// Add the filter name to the filters listbox
			USES_CONVERSION;

			lstrcpy(szName, W2T(FilterInfo.achName));

			CString sCodecName = szName;
			if ( m_media_filename.Find(sCodecName) < 0 )
			{
				m_sFilter[m_nFilter] = szName;

				ISpecifyPropertyPages *pSpecify;

				// Discover if this filter contains a property page
				hr = pFilter->QueryInterface(IID_ISpecifyPropertyPages, (void **)&pSpecify);
				if (SUCCEEDED(hr)) 
				{
					pSpecify->Release();
					m_bFilter[m_nFilter] = TRUE;
				}
				else
					m_bFilter[m_nFilter] = FALSE;

				m_nFilter++;
				if ( m_nFilter == 10 )
					return;
			}

			FilterInfo.pGraph->Release();
		}       
		pFilter->Release();
	}
	pEnum->Release();
}

IBaseFilter* CDShow::FindFilterByNameInGraph( CString sFilter )
{
	USES_CONVERSION;

	HRESULT hr;
	IEnumFilters *pEnum = NULL;
	IBaseFilter *pFilter = NULL;
	ULONG cFetched;
	BOOL bFound = FALSE;

	// Get filter enumerator
	hr = m_pGB->EnumFilters(&pEnum);
	if (FAILED(hr))
		return NULL;

	// Enumerate all filters in the graph
	while((pEnum->Next(1, &pFilter, &cFetched) == S_OK) && (!bFound))
	{
		FILTER_INFO FilterInfo;
		TCHAR szName[256];

		hr = pFilter->QueryFilterInfo(&FilterInfo);
		if (FAILED(hr))
		{
			pFilter->Release();
			pEnum->Release();
			return NULL;
		}

		// Compare this filter's name with the one we want
		lstrcpy(szName, W2T(FilterInfo.achName));
		if (! lstrcmp(szName, sFilter))
		{
			bFound = TRUE;
		}

		FilterInfo.pGraph->Release();

		// If we found the right filter, don't release its interface.
		// The caller will use it and release it later.
		if (!bFound)
			pFilter->Release();
		else
			break;
	}
	pEnum->Release();

	return (bFound ? pFilter : NULL);
}

void CDShow::ShowFilterPropertyPage(CString sFilterName) 
{
	HRESULT hr;
	IBaseFilter *pFilter = NULL;
	TCHAR szNameToFind[128];
	ISpecifyPropertyPages *pSpecify;

	_stprintf_s(szNameToFind, _T("%s"), sFilterName);
	// Read the current list box name and find it in the graph
	pFilter = FindFilterByNameInGraph(szNameToFind);
	if (!pFilter)
		return;

	// Discover if this filter contains a property page
	hr = pFilter->QueryInterface(IID_ISpecifyPropertyPages, (void **)&pSpecify);
	if (SUCCEEDED(hr)) 
	{
		do 
		{
			FILTER_INFO FilterInfo;
			hr = pFilter->QueryFilterInfo(&FilterInfo);
			if (FAILED(hr))
				break;

			CAUUID caGUID;
			hr = pSpecify->GetPages(&caGUID);
			if (FAILED(hr))
				break;

			pSpecify->Release();

			// Display the filter's property page
			OleCreatePropertyFrame(
				m_pParent->m_hWnd,                 // Parent window
				0,                      // x (Reserved)
				0,                      // y (Reserved)
				FilterInfo.achName,     // Caption for the dialog box
				1,                      // Number of filters
				(IUnknown **)&pFilter,  // Pointer to the filter 
				caGUID.cElems,          // Number of property pages
				caGUID.pElems,          // Pointer to property page CLSIDs
				0,                      // Locale identifier
				0,                      // Reserved
				NULL                    // Reserved
			);
			CoTaskMemFree(caGUID.pElems);
			FilterInfo.pGraph->Release(); 

		} while(0);
	}

	pFilter->Release();
}

HRESULT CDShow::CreateFilter(REFCLSID clsid, IBaseFilter **ppFilter)
{
	HRESULT hr;

	hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER,
		IID_IBaseFilter,
		(void **) ppFilter);

	if(FAILED(hr))
	{
		//Msg(_T("CreateFilter: Failed to create filter!  hr=0x%x\n"), hr);
		if (ppFilter)
			*ppFilter = NULL;
		return hr;
	}

	return S_OK;
}

HRESULT CDShow::RenderOutputPins(IGraphBuilder *pGB, IBaseFilter *pFilter)
{
	/*
	CComPtr <IEnumPins>     pEnum;
	CComPtr <IPin>          pPin;
	CComQIPtr<IFilterGraph2, &IID_IFilterGraph2> pFilterGraph2(pGB);
	HRESULT hr =S_OK;

	hr = pFilter->EnumPins(&pEnum);
	if (SUCCEEDED(hr))
	{
	while(pEnum->Next(1, &pPin, 0) == S_OK)
	{
	PIN_DIRECTION PinDirThis;
	hr = pPin->QueryDirection(&PinDirThis);

	if(SUCCEEDED(hr))
	{
	if (PINDIR_OUTPUT == PinDirThis)
	{
	CComPtr<IPin> pConnectedPin;

	hr = pPin->ConnectedTo(&pConnectedPin);

	if(VFW_E_NOT_CONNECTED == hr)
	{
	if (!pConnectedPin)
	{
	hr = pFilterGraph2->RenderEx(pPin, AM_RENDEREX_RENDERTOEXISTINGRENDERERS, NULL);

	}
	}
	}
	}
	pPin.Release();
	}
	}
	return hr;  
	*/

	HRESULT         hr = S_OK;
	IEnumPins *     pEnumPin = NULL;
	IPin *          pConnectedPin = NULL, * pPin = NULL;
	PIN_DIRECTION   PinDirection;
	ULONG           ulFetched;
	BOOL			bAtLeastOnePinRendered = FALSE;

	// Enumerate all pins on the filter
	hr = pFilter->EnumPins(&pEnumPin);

	if(SUCCEEDED(hr))
	{
		// Step through every pin, looking for the output pins
		while (S_OK == (hr = pEnumPin->Next(1L, &pPin, &ulFetched)))
		{
			// Is this pin connected?  We're not interested in connected pins.
			hr = pPin->ConnectedTo(&pConnectedPin);
			if (pConnectedPin)
			{
				pConnectedPin->Release();
				pConnectedPin = NULL;
			}

			// If this pin is not connected, render it.
			if (VFW_E_NOT_CONNECTED == hr)
			{
				hr = pPin->QueryDirection(&PinDirection);
				if ((S_OK == hr) && (PinDirection == PINDIR_OUTPUT))
				{
					hr = pGB->Render(pPin);
				}
			}
			pPin->Release();

			// scpark
			// 출력핀들을 렌더할 때 하나의 핀이라도 렌더된다면 렌더시키자.
			// 어떤 미디어는 두 개의 핀을 가지지만 하나만 렌더되는 미디어도 있다.
			// 따라서 첫번째 핀 렌더가 실패했어도 두번째 핀렌더가 성공한다면
			// 렌더는 이루어진것으로 판단한다.
			if ( SUCCEEDED(hr) )
				bAtLeastOnePinRendered = TRUE;
			// If there was an error, stop enumerating
			if (FAILED(hr))
				break;
		}
	}

	// Release pin enumerator
	pEnumPin->Release();

	if ( bAtLeastOnePinRendered )
		return 0;

	return hr;

}

void CDShow::FindAudioRenderer()
{
	m_nAudioFilter = 0;

	HRESULT hr;    
	IEnumMoniker *pEnumCat = NULL;

	// Instantiate the system device enumerator
	ICreateDevEnum	*m_pSysDevEnum = NULL;

	hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, 
		CLSCTX_INPROC, IID_ICreateDevEnum, 
		(void **)&m_pSysDevEnum);
	if FAILED(hr)
	{
		CoUninitialize();
		return;
	}

	const CLSID *clsid;
	clsid = &CLSID_AudioRendererCategory;

	// If the CLSID wasn't allocated earlier, then fail
	if ( !clsid )
		return;

	//
	// WARNING!
	//
	// Some third-party filters throw an exception (int 3) during enumeration
	// on Debug builds, often due to heap corruption in RtlFreeHeap().
	// Sometimes it is done intentionally to prevent reverse engineering.
	// 
	// This is not an issue on Release builds.
	//

	// Enumerate all filters of the selected category  
	hr = m_pSysDevEnum->CreateClassEnumerator(*clsid, &pEnumCat, 0);
	ASSERT(SUCCEEDED(hr));
	if FAILED(hr)
		return;

	// Enumerate all filters using the category enumerator
	hr = EnumFilters(pEnumCat);

	SAFE_RELEASE(pEnumCat);

	if ( m_nAudioFilter > 0 )
	{
		CString		sIndex;
		for ( int i = 0; i < m_nAudioFilter; i++ )
		{
			sIndex.Format(_T("%d"), i );
			AfxGetApp()->WriteProfileString(_T("Config\\Sound"), sIndex, m_sAudioFilter[i] );
		}

		AfxGetApp()->WriteProfileInt(_T("Config\\Sound"), _T("nCount"), m_nAudioFilter );
	}
}

HRESULT CDShow::EnumFilters(IEnumMoniker *pEnumCat)
{
	HRESULT hr=S_OK;
	IMoniker *pMoniker;
	ULONG cFetched;
	VARIANT varName={0};

	m_nAudioFilter = 0;

	// If there are no filters of a requested type, show default string
	if ( !pEnumCat )
	{
		return S_FALSE;
	}

	// Enumerate all items associated with the moniker
	while(pEnumCat->Next(1, &pMoniker, &cFetched) == S_OK)
	{
		IPropertyBag *pPropBag;
		ASSERT(pMoniker);

		// Associate moniker with a file
		hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, 
			(void **)&pPropBag);
		ASSERT(SUCCEEDED(hr));
		ASSERT(pPropBag);
		if (FAILED(hr))
			continue;

		// Read filter name from property bag
		varName.vt = VT_BSTR;
		hr = pPropBag->Read(L"FriendlyName", &varName, 0);
		if (FAILED(hr))
			continue;

		// Get filter name (converting BSTR name to a CString)
		CString str(varName.bstrVal);
		SysFreeString(varName.bstrVal);

		// Read filter's CLSID from property bag.  This CLSID string will be
		// converted to a binary CLSID and passed to AddFilter(), which will
		// add the filter's name to the listbox and its CLSID to the listbox
		// item's DataPtr item.  When the user clicks on a filter name in
		// the listbox, we'll read the stored CLSID, convert it to a string,
		// and use it to find the filter's filename in the registry.
		VARIANT varFilterClsid;
		varFilterClsid.vt = VT_BSTR;

		// Read CLSID string from property bag
		hr = pPropBag->Read(L"CLSID", &varFilterClsid, 0);
		if(SUCCEEDED(hr))
		{
			CLSID clsidFilter;

			// Add filter name and CLSID to listbox
			if(CLSIDFromString(varFilterClsid.bstrVal, &clsidFilter) == S_OK)
			{
				//AddFilter(str, &clsidFilter);
				m_sAudioFilter[m_nAudioFilter++] = str;
			}

			SysFreeString(varFilterClsid.bstrVal);
		}

		// Cleanup interfaces
		SAFE_RELEASE(pPropBag);
		SAFE_RELEASE(pMoniker);
	}

	return S_OK;
}

HRESULT CDShow::FindFilter(CString compFiterName, REFCLSID clsID, IBaseFilter **ppSrcFilter)
{
	HRESULT hr=S_OK;
	IBaseFilter * pSrc = NULL;
	IMoniker* pMoniker =NULL;
	ULONG cFetched;
	VARIANT varName={0};

	if (!ppSrcFilter)
		return E_POINTER;

	// Create the system device enumerator
	CComPtr <ICreateDevEnum> pDevEnum =NULL;

	hr = CoCreateInstance (CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC,
		IID_ICreateDevEnum, (void **) &pDevEnum);
	if (FAILED(hr))
	{
		return hr;
	}

	// Create an enumerator for the video capture devices
	CComPtr <IEnumMoniker> pClassEnum = NULL;

	hr = pDevEnum->CreateClassEnumerator( clsID, &pClassEnum, 0 );

	if (FAILED(hr))
	{
		return hr;
	}

	// If there are no enumerators for the requested type, then 
	// CreateClassEnumerator will succeed, but pClassEnum will be NULL.
	if (pClassEnum == NULL)
	{
		return E_FAIL;
	}

	// Use the first video capture device on the device list.
	// Note that if the Next() call succeeds but there are no monikers,
	// it will return S_FALSE (which is not a failure).  Therefore, we
	// check that the return code is S_OK instead of using SUCCEEDED() macro.
	while (S_OK == (pClassEnum->Next (1, &pMoniker, &cFetched)))
	{
		IPropertyBag *pPropBag;
		ASSERT(pMoniker);

		// Associate moniker with a file
		hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, 
			(void **)&pPropBag);
		ASSERT(SUCCEEDED(hr));
		ASSERT(pPropBag);
		if (FAILED(hr))
			continue;

		// Read filter name from property bag
		varName.vt = VT_BSTR;
		hr = pPropBag->Read(L"FriendlyName", &varName, 0);
		if (FAILED(hr))
			continue;

		// Get filter name (converting BSTR name to a CString)
		CString str(varName.bstrVal);
		SysFreeString(varName.bstrVal);

		//TRACE( "%s\n", str );

		if(str.Compare(compFiterName) == 0)
		{
			// Bind Moniker to a filter object
			hr = pMoniker->BindToObject(0,0,IID_IBaseFilter, (void**)&pSrc);
			if (FAILED(hr))
			{
				return hr;
			}
			SAFE_RELEASE(pPropBag);
			*ppSrcFilter = pSrc;
			return hr;
		}

		// Cleanup interfaces
		SAFE_RELEASE(pPropBag);
		SAFE_RELEASE(pMoniker);
	}

	// Copy the found filter pointer to the output parameter.
	// Do NOT Release() the reference, since it will still be used
	// by the calling function.


	return hr;
}

void CDShow::hide_cursor(bool hide)
{
	CComQIPtr<IVideoWindow> pVW(m_pGB);
	if (pVW != NULL)
	{
		pVW->HideCursor(hide ? OATRUE : OAFALSE);
	}
}

HRESULT CDShow::save_filter_graph(IGraphBuilder *pGraph, CString sfile)
{
	const WCHAR wszStreamName[] = L"ActiveMovieGraph";
	HRESULT hr;

	if (pGraph == NULL)
		pGraph = m_pGB;

	IStorage *pStorage = NULL;
	hr = StgCreateDocfile(
		sfile,
		STGM_CREATE | STGM_TRANSACTED | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
		0, &pStorage);
	if(FAILED(hr))
	{
		return hr;
	}

	IStream *pStream;
	hr = pStorage->CreateStream(
		wszStreamName,
		STGM_WRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
		0, 0, &pStream);
	if (FAILED(hr))
	{
		pStorage->Release();   
		return hr;
	}

	IPersistStream *pPersist = NULL;
	pGraph->QueryInterface(IID_IPersistStream, (void**) &pPersist);
	hr = pPersist->Save(pStream, TRUE);
	pStream->Release();
	pPersist->Release();
	if (SUCCEEDED(hr))
	{
		hr = pStorage->Commit(STGC_DEFAULT);
	}
	pStorage->Release();
	return hr;
}

//-1, 1이면 500ms단위로 빠르게, 느리게 변경하고 그 이외의 값이면 해당 값을 싱크 속도로 적용한다.
//-1:faster, 1:slower, 0:origin	//per 500ms
void CDShow::subtitle_sync(int sync)
{
	if (sync == 1)
		m_subtitle_sync += 500;
	else if (sync == 0)
		m_subtitle_sync = 0;
	else if (sync == -1)
		m_subtitle_sync -= 500;
	else
		m_subtitle_sync = sync;

	if (m_use_dvs)
		DirectVobSub_function(msg_put_SubtitleTiming, m_subtitle_sync);
}

void CDShow::subtitle_placement(int dir)
{
	if (dir == dir_default)
	{
		m_subCfg.pos_x = 50;
		m_subCfg.pos_y = 90;
	}
	else if (dir == dir_left)
	{
		m_subCfg.pos_x--;
	}
	else if (dir == dir_right)
	{
		m_subCfg.pos_x++;
	}
	else if (dir == dir_up)
	{
		m_subCfg.pos_y--;
	}
	else if (dir == dir_down)
	{
		m_subCfg.pos_y++;
	}
	else
	{
		AfxMessageBox(_T("unknown subtitle placement direction"));
		return;
	}

	Clamp(m_subCfg.pos_x, 0, 100);
	Clamp(m_subCfg.pos_y, 0, 100);

	if (m_use_dvs)
		DirectVobSub_function(msg_put_Placement, (LPARAM)(MAKEWORD(m_subCfg.pos_x, m_subCfg.pos_y)));

	TRACE(_T("subtitle pos = %d%%, %d%%\n"), m_subCfg.pos_x, m_subCfg.pos_y);
	update_osd_subtitle();
}

void CDShow::subtitle_placement(int x, int y)
{
	Clamp(x, 0, 100);
	Clamp(y, 10, 100);
	m_subCfg.pos_x = x;
	m_subCfg.pos_y = y;

	if (m_use_dvs)
		DirectVobSub_function(msg_put_Placement, (LPARAM)(MAKEWORD(m_subCfg.pos_x, m_subCfg.pos_y)));

	TRACE(_T("subtitle pos = %d%%, %d%%\n"), m_subCfg.pos_x, m_subCfg.pos_y);
	update_osd_subtitle();
}

void CDShow::save_sub_cfg()
{
	CString style;
	AfxGetApp()->WriteProfileString(_T("subtitle"), _T("setting"), style <<= m_subCfg);
}
