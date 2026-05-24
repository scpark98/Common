#include "dshow.h"
#include "SCAudioGain.h"
#include "SCAudioCompressor.h"
#include "SCAudioTimeStretch.h"
#include "SCVideoTimeScale.h"
#include "../../log/SCLog/SCLog.h"
#include <gdiplus.h>
#include <cmath>
#include <dxgi.h>		//IDXGIFactory1 / IDXGIAdapter1 — get_video_memory_mb() 에서 사용.


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

//MPC-BE include/FilterInterfaces.h 의 IExFilterConfig 정의 (필요한 메서드만 추출).
//MPCVR 의 runtime config — "lessRedraws" 등으로 resize 매끄러움 개선.
interface __declspec(uuid("37CBDF10-D65E-4E5A-8F37-40E0C8EA1695")) IExFilterConfig : public IUnknown
{
	STDMETHOD(Flt_GetBool  )(LPCSTR field, bool    *value) PURE;
	STDMETHOD(Flt_GetInt   )(LPCSTR field, int     *value) PURE;
	STDMETHOD(Flt_GetInt64 )(LPCSTR field, __int64 *value) PURE;
	STDMETHOD(Flt_GetDouble)(LPCSTR field, double  *value) PURE;
	STDMETHOD(Flt_GetString)(LPCSTR field, LPWSTR  *value, unsigned *chars) PURE;
	STDMETHOD(Flt_GetBin   )(LPCSTR field, LPVOID  *value, unsigned *size ) PURE;
	STDMETHOD(Flt_SetBool  )(LPCSTR field, bool    value) PURE;
	STDMETHOD(Flt_SetInt   )(LPCSTR field, int     value) PURE;
	STDMETHOD(Flt_SetInt64 )(LPCSTR field, __int64 value) PURE;
	STDMETHOD(Flt_SetDouble)(LPCSTR field, double  value) PURE;
	STDMETHOD(Flt_SetString)(LPCSTR field, LPWSTR  value, int chars) PURE;
	STDMETHOD(Flt_SetBin   )(LPCSTR field, LPVOID  value, int size ) PURE;
};

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

		static int s_count = 0;
		if (++s_count <= 20)
			logWrite(_T("[sub_sink] Receive #%d len=%ld rt=%lld..%lld"),
				s_count, len, (long long)rtStart, (long long)rtEnd);

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

	if (!m_pGB || !m_pSplitter || !target)
	{
		logWrite(_T("[sub_grab] precondition fail: pGB=%p pSplitter=%p target=%p"),
			(void*)m_pGB, (void*)m_pSplitter, (void*)target);
		return;
	}

	//graph state 보호 — running/paused 시 새 필터 추가/연결 실패 가능. stop 후 setup 후 복원.
	CComQIPtr<IMediaControl> pMC(m_pGB);
	OAFilterState saved_state = State_Stopped;
	if (pMC) { pMC->GetState(0, &saved_state); pMC->Stop(); }

	//splitter 의 자막 출력 핀 찾기
	IPin* pSubPin = NULL;
	IEnumPins* pEnum = NULL;
	m_pSplitter->EnumPins(&pEnum);
	int pin_seen = 0, pin_out = 0, pin_sub = 0;
	if (pEnum)
	{
		IPin* pPin = NULL;
		while (pEnum->Next(1, &pPin, NULL) == S_OK)
		{
			pin_seen++;
			PIN_DIRECTION dir;
			pPin->QueryDirection(&dir);
			if (dir == PINDIR_OUTPUT)
			{
				pin_out++;
				PIN_INFO pi; memset(&pi, 0, sizeof(pi));
				pPin->QueryPinInfo(&pi);
				if (pi.pFilter) pi.pFilter->Release();

				IEnumMediaTypes* pEnumMT = NULL;
				pPin->EnumMediaTypes(&pEnumMT);
				if (pEnumMT)
				{
					AM_MEDIA_TYPE* pmt = NULL;
					if (pEnumMT->Next(1, &pmt, NULL) == S_OK)
					{
						logWrite(_T("[sub_grab] splitter pin '%s' major.D1=%08lX sub.D1=%08lX"),
							pi.achName, pmt->majortype.Data1, pmt->subtype.Data1);
						//IsEqualGUID + Data1 fallback (E487EB08 은 자막 majortype 의 unique signature)
						if (IsEqualGUID(pmt->majortype, MEDIATYPE_Subtitle) ||
							pmt->majortype.Data1 == 0xE487EB08)
						{
							pin_sub++;
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
	logWrite(_T("[sub_grab] splitter pin scan: seen=%d out=%d subtitle=%d → pSubPin=%p"),
		pin_seen, pin_out, pin_sub, (void*)pSubPin);

	if (!pSubPin)
	{
		if (pMC) { if (saved_state == State_Running) pMC->Run(); else if (saved_state == State_Paused) pMC->Pause(); }
		return;
	}

	//Custom sink filter 생성 + 그래프에 추가
	CSubtitleSinkFilter* pSink = new CSubtitleSinkFilter(target, msg);
	pSink->AddRef();

	m_pGB->AddFilter(pSink, L"Subtitle Sink");

	//splitter 자막 핀 → sink input 핀 직접 연결 (intermediate filter 사용 안 함)
	IPin* pSinkIn = NULL;
	pSink->FindPin(L"In", &pSinkIn);
	HRESULT hr_conn = E_FAIL;
	if (pSinkIn)
	{
		hr_conn = m_pGB->ConnectDirect(pSubPin, pSinkIn, NULL);
		pSinkIn->Release();
	}
	logWrite(_T("[sub_grab] ConnectDirect sink hr=0x%08x"), hr_conn);

	pSubPin->Release();

	m_pSubtitleGrabber = pSink;	//IBaseFilter* alias
	m_pSubtitleNullRenderer = NULL;
	m_pSubtitleGrabberCB = pSink;	//CSubtitleSinkFilter* (subtype 조회용 별칭)

	//graph state 복원
	if (pMC) { if (saved_state == State_Running) pMC->Run(); else if (saved_state == State_Paused) pMC->Pause(); }
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
//Video TimeScale 도 LAV path 한정 동시 setup — audio TimeStretch 와 동일 rate 적용을 위해.
void CDShow::setup_audio_filter_chain()
{
	setup_audio_gain_filter();
	setup_video_time_scale_filter();
}
void CDShow::teardown_audio_filter_chain()
{
	teardown_video_time_scale_filter();
	teardown_audio_gain_filter();
}
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
		if (pMC) { if (saved_state == State_Running) pMC->Run(); else if (saved_state == State_Paused) pMC->Pause(); }
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
		if (pMC) { if (saved_state == State_Running) pMC->Run(); else if (saved_state == State_Paused) pMC->Pause(); }
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
		if (pMC) { if (saved_state == State_Running) pMC->Run(); else if (saved_state == State_Paused) pMC->Pause(); }
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

		//AGC default 적용 — registry "setting\audio_agc_enabled" + "audio_agc_target_db" 에서 읽고
		//CSCAudioGain 에 set. 없으면 default (enabled=true, target=-20 dB).
		bool agc_on = AfxGetApp()->GetProfileInt(_T("setting"), _T("audio_agc_enabled"), 1) != 0;
		int target_int = AfxGetApp()->GetProfileInt(_T("setting"), _T("audio_agc_target_db"), -20);
		pGain->set_agc_target_db((float)target_int);
		pGain->set_agc_enabled(agc_on);
		pGain->reset_agc();
		logWrite(_T("[AudioGain] AGC enabled=%d target=%d dB"), (int)agc_on, target_int);

		//audio_sync 복원 — chain teardown/setup 토글 시 이전 m_audio_sync 가 0 이 아니면
		//새 chain 에 set_delay_ms 적용. rate=1.0 복귀 후 audio_sync 유실 방지.
		if (m_audio_sync != 0)
		{
			pGain->set_delay_ms(m_audio_sync);
			logWrite(_T("[AudioGain] audio_sync %d ms re-applied after chain setup"), m_audio_sync);
		}

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

		//CSCAudioTimeStretch (atempo pitch-preserving) — LAV path 에서만 chain 에 추가.
		//internal path 는 CFFiAudioStream::FillBuffer 안에서 atempo 처리 — 중복 적용 방지.
		//chain 위치: 마지막 tail filter (compressor 또는 gain) → time-stretch → renderer.
		//compressor 실패해 tail 이 gain 인 경우는 일단 cover 안 함 (compressor 실패 자체가 드문 경로).
		if (m_pAudioCompressorFilter && !m_pFFiSource)
		{
			CSCAudioCompressor* pCompTail = (CSCAudioCompressor*)m_pAudioCompressorFilter;
			IPin* pTailOut = NULL;
			pCompTail->FindPin(L"Out", &pTailOut);
			if (pTailOut)
			{
				AM_MEDIA_TYPE ts_mt;
				memset(&ts_mt, 0, sizeof(ts_mt));
				HRESULT hr_tsmt = pTailOut->ConnectionMediaType(&ts_mt);
				bool has_ts_mt = SUCCEEDED(hr_tsmt);

				m_pGB->Disconnect(pTailOut);
				m_pGB->Disconnect(pRendererIn);

				CSCAudioTimeStretch* pTS = new CSCAudioTimeStretch();
				pTS->AddRef();

				HRESULT hr_addts = m_pGB->AddFilter(pTS, L"SC Audio Time-Stretch");
				logWrite(_T("[AudioTS] AddFilter hr=0x%08lX"), hr_addts);

				IPin* pTsIn = NULL, *pTsOut = NULL;
				pTS->FindPin(L"In",  &pTsIn);
				pTS->FindPin(L"Out", &pTsOut);

				HRESULT hr_t1 = (pTsIn  ? m_pGB->ConnectDirect(pTailOut, pTsIn,  has_ts_mt ? &ts_mt : NULL) : E_FAIL);
				HRESULT hr_t2 = (pTsOut ? m_pGB->ConnectDirect(pTsOut, pRendererIn, has_ts_mt ? &ts_mt : NULL) : E_FAIL);
				logWrite(_T("[AudioTS] Connect comp->ts hr=0x%08lX  ts->renderer hr=0x%08lX"), hr_t1, hr_t2);

				if (pTsIn) pTsIn->Release();
				if (pTsOut) pTsOut->Release();

				if (FAILED(hr_t1) || FAILED(hr_t2))
				{
					logWrite(_T("[AudioTS] connect failed - reverting (time-stretch 없이 compressor 까지만 사용)"));
					m_pGB->RemoveFilter(pTS);
					pTS->Release();
					m_pGB->ConnectDirect(pTailOut, pRendererIn, has_ts_mt ? &ts_mt : NULL);
				}
				else
				{
					logWrite(_T("[AudioTS] time-stretch inserted into chain (comp->ts->renderer)"));
					m_pAudioTimeStretchFilter = pTS;
				}

				if (has_ts_mt)
				{
					if (ts_mt.cbFormat && ts_mt.pbFormat) CoTaskMemFree(ts_mt.pbFormat);
					if (ts_mt.pUnk) ts_mt.pUnk->Release();
				}
				pTailOut->Release();
			}
		}
	}

	if (has_saved_mt)
	{
		if (saved_mt.cbFormat && saved_mt.pbFormat) CoTaskMemFree(saved_mt.pbFormat);
		if (saved_mt.pUnk) saved_mt.pUnk->Release();
	}

	//audio renderer 의 IReferenceClock 을 graph master 로 명시. graph 의 default 가 audio renderer 인 게 보통이지만
	//chain 변경 (filter add/remove) 후 graph 가 재선택 — 다른 filter 가 master 가 되면 audio 의 sample timing 진행 속도가
	//graph clock 에 반영 안 됨 → rate < 1.0 시 video 가 audio 보다 느림 같은 비대칭. audio renderer 강제로 안정.
	if (pRenderer)
	{
		CComQIPtr<IReferenceClock> pClock(pRenderer);
		if (pClock)
		{
			CComQIPtr<IMediaFilter> pMF(m_pGB);
			if (pMF)
			{
				HRESULT hr_sync = pMF->SetSyncSource(pClock);
				logWrite(_T("[AudioGain] SetSyncSource(audio renderer clock) hr=0x%08lX"), hr_sync);
			}
			else
			{
				logWrite(_T("[AudioGain] graph IMediaFilter QI fail — SetSyncSource skipped"));
			}
		}
		else
		{
			logWrite(_T("[AudioGain] audio renderer IReferenceClock not exposed — SetSyncSource skipped"));
		}
	}

	pUpstreamOut->Release();
	pRenderer->Release();
	pRendererIn->Release();

	if (pMC) { if (saved_state == State_Running) pMC->Run(); else if (saved_state == State_Paused) pMC->Pause(); }
}

void CDShow::teardown_audio_gain_filter()
{
	if (!m_pGB || (!m_pAudioGainFilter && !m_pAudioCompressorFilter && !m_pAudioTimeStretchFilter))
		return;

	//graph state 보존 — Stop 후 chain 제거, 마지막에 복원.
	CComQIPtr<IMediaControl> pMC(m_pGB);
	OAFilterState saved_state = State_Stopped;
	if (pMC) { pMC->GetState(0, &saved_state); pMC->Stop(); }

	//chain: upstream(LAV Audio out) → Gain → Compressor → TimeStretch → downstream(DSound in).
	//재연결 위한 양끝 pin + media type 보존.
	IPin* pUpstreamOut = NULL;
	IPin* pDownstreamIn = NULL;
	AM_MEDIA_TYPE saved_mt;
	memset(&saved_mt, 0, sizeof(saved_mt));
	bool has_mt = false;

	if (m_pAudioGainFilter)
	{
		CSCAudioGain* pGain = (CSCAudioGain*)m_pAudioGainFilter;
		IPin* pGainIn = NULL;
		pGain->FindPin(L"In", &pGainIn);
		if (pGainIn) { pGainIn->ConnectedTo(&pUpstreamOut); pGainIn->Release(); }
	}

	//tail 우선순위: TimeStretch > Compressor > Gain.
	IBaseFilter* pTailFilter = NULL;
	if (m_pAudioTimeStretchFilter)
		pTailFilter = (IBaseFilter*)(CSCAudioTimeStretch*)m_pAudioTimeStretchFilter;
	else if (m_pAudioCompressorFilter)
		pTailFilter = (IBaseFilter*)(CSCAudioCompressor*)m_pAudioCompressorFilter;
	else if (m_pAudioGainFilter)
		pTailFilter = (IBaseFilter*)(CSCAudioGain*)m_pAudioGainFilter;
	if (pTailFilter)
	{
		IPin* pTailOut = NULL;
		pTailFilter->FindPin(L"Out", &pTailOut);
		if (pTailOut)
		{
			pTailOut->ConnectedTo(&pDownstreamIn);
			if (pDownstreamIn)
			{
				AM_MEDIA_TYPE mt;
				memset(&mt, 0, sizeof(mt));
				if (SUCCEEDED(pDownstreamIn->ConnectionMediaType(&mt)))
				{
					saved_mt = mt;
					has_mt = true;
				}
			}
			pTailOut->Release();
		}
	}

	//SC filter 들 RemoveFilter — graph 가 자동 disconnect. 순서: chain tail (TimeStretch) → Compressor → Gain.
	if (m_pAudioTimeStretchFilter)
	{
		CSCAudioTimeStretch* pTS = (CSCAudioTimeStretch*)m_pAudioTimeStretchFilter;
		m_pGB->RemoveFilter(pTS);
		pTS->Release();
		m_pAudioTimeStretchFilter = NULL;
	}
	if (m_pAudioCompressorFilter)
	{
		CSCAudioCompressor* pComp = (CSCAudioCompressor*)m_pAudioCompressorFilter;
		m_pGB->RemoveFilter(pComp);
		pComp->Release();
		m_pAudioCompressorFilter = NULL;
	}
	if (m_pAudioGainFilter)
	{
		CSCAudioGain* pGain = (CSCAudioGain*)m_pAudioGainFilter;
		m_pGB->RemoveFilter(pGain);
		pGain->Release();
		m_pAudioGainFilter = NULL;
	}

	//upstream ↔ downstream 직결 재연결 — 이게 누락되면 audio path 가 끊겨 graph broken.
	HRESULT hr_reconnect = E_FAIL;
	if (pUpstreamOut && pDownstreamIn)
	{
		hr_reconnect = m_pGB->ConnectDirect(pUpstreamOut, pDownstreamIn, has_mt ? &saved_mt : NULL);
		logWrite(_T("[AudioGain] teardown direct reconnect hr=0x%08x"), hr_reconnect);
	}
	if (pUpstreamOut) pUpstreamOut->Release();
	if (pDownstreamIn) pDownstreamIn->Release();
	if (has_mt)
	{
		if (saved_mt.cbFormat && saved_mt.pbFormat) CoTaskMemFree(saved_mt.pbFormat);
		if (saved_mt.pUnk) saved_mt.pUnk->Release();
	}

	//graph state 복원.
	if (pMC)
	{
		if (saved_state == State_Running) pMC->Run();
		else if (saved_state == State_Paused) pMC->Pause();
	}
}


void CDShow::setup_video_time_scale_filter()
{
	teardown_video_time_scale_filter();

	if (!m_pGB) return;
	//internal path (m_pFFiSource != NULL) 에서는 CFFiVideoStream::FillBuffer 가 이미 rate scaling — skip.
	if (m_pFFiSource) return;

	//graph state 보호.
	CComQIPtr<IMediaControl> pMC(m_pGB);
	OAFilterState saved_state = State_Stopped;
	if (pMC) { pMC->GetState(0, &saved_state); pMC->Stop(); }

	//video renderer 찾기 — input pin 만 가진 + connected media major = MEDIATYPE_Video.
	IBaseFilter* pRenderer = NULL;
	IPin*        pRendererIn = NULL;
	IPin*        pUpstreamOut = NULL;

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
				IPin* pVideoIn = NULL;
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
							if (IsEqualGUID(mt.majortype, MEDIATYPE_Video))
							{
								if (pVideoIn) pVideoIn->Release();
								pVideoIn = p;
								pVideoIn->AddRef();
							}
							if (mt.cbFormat && mt.pbFormat) CoTaskMemFree(mt.pbFormat);
							if (mt.pUnk) mt.pUnk->Release();
						}
					}
					p->Release();
				}
				pEnumP->Release();

				if (has_input && !has_output && pVideoIn)
				{
					pRenderer = pF; pRenderer->AddRef();
					pRendererIn = pVideoIn;
					pVideoIn = NULL;
				}
				if (pVideoIn) pVideoIn->Release();
			}
			pF->Release();
			if (pRenderer) break;
		}
		pEnumF->Release();
	}

	if (!pRenderer || !pRendererIn)
	{
		logWrite(_T("[VideoTS] no video renderer found"));
		if (pRenderer) pRenderer->Release();
		if (pRendererIn) pRendererIn->Release();
		if (pMC) { if (saved_state == State_Running) pMC->Run(); else if (saved_state == State_Paused) pMC->Pause(); }
		return;
	}
	logWrite(_T("[VideoTS] video renderer found pRenderer=%p pRendererIn=%p"), pRenderer, pRendererIn);

	pRendererIn->ConnectedTo(&pUpstreamOut);
	if (!pUpstreamOut)
	{
		logWrite(_T("[VideoTS] upstream pin not connected"));
		pRenderer->Release(); pRendererIn->Release();
		if (pMC) { if (saved_state == State_Running) pMC->Run(); else if (saved_state == State_Paused) pMC->Pause(); }
		return;
	}

	AM_MEDIA_TYPE saved_mt;
	memset(&saved_mt, 0, sizeof(saved_mt));
	HRESULT hr_mt = pRendererIn->ConnectionMediaType(&saved_mt);
	bool has_saved_mt = SUCCEEDED(hr_mt);

	m_pGB->Disconnect(pRendererIn);
	m_pGB->Disconnect(pUpstreamOut);

	CSCVideoTimeScale* pTS = new CSCVideoTimeScale();
	pTS->AddRef();

	HRESULT hr_add = m_pGB->AddFilter(pTS, L"SC Video Time-Scale");
	logWrite(_T("[VideoTS] AddFilter hr=0x%08lX"), hr_add);
	if (FAILED(hr_add))
	{
		m_pGB->ConnectDirect(pUpstreamOut, pRendererIn, has_saved_mt ? &saved_mt : NULL);
		pTS->Release();
	}
	else
	{
		IPin* pTsIn = NULL, *pTsOut = NULL;
		pTS->FindPin(L"In",  &pTsIn);
		pTS->FindPin(L"Out", &pTsOut);

		HRESULT hr_t1 = (pTsIn  ? m_pGB->ConnectDirect(pUpstreamOut, pTsIn,  has_saved_mt ? &saved_mt : NULL) : E_FAIL);
		HRESULT hr_t2 = (pTsOut ? m_pGB->ConnectDirect(pTsOut, pRendererIn, has_saved_mt ? &saved_mt : NULL) : E_FAIL);
		logWrite(_T("[VideoTS] Connect upstream->ts hr=0x%08lX  ts->renderer hr=0x%08lX"), hr_t1, hr_t2);

		if (pTsIn) pTsIn->Release();
		if (pTsOut) pTsOut->Release();

		if (FAILED(hr_t1) || FAILED(hr_t2))
		{
			logWrite(_T("[VideoTS] connect failed - restoring original"));
			m_pGB->RemoveFilter(pTS);
			pTS->Release();
			m_pGB->ConnectDirect(pUpstreamOut, pRendererIn, has_saved_mt ? &saved_mt : NULL);
		}
		else
		{
			logWrite(_T("[VideoTS] video time-scale inserted into chain"));
			m_pVideoTimeScaleFilter = pTS;
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

	if (pMC) { if (saved_state == State_Running) pMC->Run(); else if (saved_state == State_Paused) pMC->Pause(); }
}

void CDShow::teardown_video_time_scale_filter()
{
	if (!m_pGB || !m_pVideoTimeScaleFilter) return;

	CComQIPtr<IMediaControl> pMC(m_pGB);
	OAFilterState saved_state = State_Stopped;
	if (pMC) { pMC->GetState(0, &saved_state); pMC->Stop(); }

	CSCVideoTimeScale* pTS = (CSCVideoTimeScale*)m_pVideoTimeScaleFilter;

	IPin* pTsIn = NULL, *pTsOut = NULL;
	pTS->FindPin(L"In",  &pTsIn);
	pTS->FindPin(L"Out", &pTsOut);

	IPin* pUpstreamOut = NULL;
	IPin* pDownstreamIn = NULL;
	AM_MEDIA_TYPE saved_mt;
	memset(&saved_mt, 0, sizeof(saved_mt));
	bool has_mt = false;

	if (pTsIn)  pTsIn->ConnectedTo(&pUpstreamOut);
	if (pTsOut)
	{
		pTsOut->ConnectedTo(&pDownstreamIn);
		if (pDownstreamIn && SUCCEEDED(pDownstreamIn->ConnectionMediaType(&saved_mt)))
			has_mt = true;
	}
	if (pTsIn) pTsIn->Release();
	if (pTsOut) pTsOut->Release();

	m_pGB->RemoveFilter(pTS);
	pTS->Release();
	m_pVideoTimeScaleFilter = NULL;

	if (pUpstreamOut && pDownstreamIn)
	{
		HRESULT hr_r = m_pGB->ConnectDirect(pUpstreamOut, pDownstreamIn, has_mt ? &saved_mt : NULL);
		logWrite(_T("[VideoTS] teardown direct reconnect hr=0x%08x"), hr_r);
	}
	if (pUpstreamOut) pUpstreamOut->Release();
	if (pDownstreamIn) pDownstreamIn->Release();
	if (has_mt)
	{
		if (saved_mt.cbFormat && saved_mt.pbFormat) CoTaskMemFree(saved_mt.pbFormat);
		if (saved_mt.pUnk) saved_mt.pUnk->Release();
	}

	if (pMC)
	{
		if (saved_state == State_Running) pMC->Run();
		else if (saved_state == State_Paused) pMC->Pause();
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

//AGC (Automatic Gain Control) — 미디어별 인코딩 레벨 차이를 자동 보정. CSCAudioGain 의 AGC 모드 토글.
void CDShow::set_audio_agc_enabled(bool e)
{
	if (!m_pAudioGainFilter) return;
	CSCAudioGain* pGain = (CSCAudioGain*)m_pAudioGainFilter;
	pGain->set_agc_enabled(e);
	if (e) pGain->reset_agc();		//enable 직후 새로 측정 시작.
}

bool CDShow::get_audio_agc_enabled()
{
	if (!m_pAudioGainFilter) return false;
	CSCAudioGain* pGain = (CSCAudioGain*)m_pAudioGainFilter;
	return pGain->get_agc_enabled();
}

//AGC 목표 RMS dB — default -20 dB. 더 작은 값 (예: -16) 은 평균 인지 음량 더 크게.
void CDShow::set_audio_agc_target_db(float db)
{
	if (!m_pAudioGainFilter) return;
	CSCAudioGain* pGain = (CSCAudioGain*)m_pAudioGainFilter;
	pGain->set_agc_target_db(db);
}

float CDShow::get_audio_agc_target_db()
{
	if (!m_pAudioGainFilter) return -20.0f;
	CSCAudioGain* pGain = (CSCAudioGain*)m_pAudioGainFilter;
	return pGain->get_agc_target_db();
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
	m_pAudioTimeStretchFilter = NULL;
	m_pVideoTimeScaleFilter = NULL;

	m_use_dvs = false;
	m_use_mpcvr = false;
	m_has_subtitle = false;
	m_subtitle_file.Empty();
	m_hDirectVobSubWnd = NULL;
	m_subtitle_sync = 0;
	m_audio_sync = 0;
	m_user_playback_rate.store(1.0);
	m_media_filename.Empty();

	close_media();

	m_volume_mute = AfxGetApp()->GetProfileInt(_T("setting"), _T("volume mute"), false);
	m_volume = AfxGetApp()->GetProfileInt(_T("setting"), _T("volume"), 50);
	m_default_interval = AfxGetApp()->GetProfileInt(_T("setting"), _T("default track interval"), 5);
	m_control_interval = AfxGetApp()->GetProfileInt(_T("setting"), _T("control track interval"), 30);

	logWrite(_T("[reg] CDShow.ctor: GetProfileInt subtitle/show ENTER"));
	m_show_subtitle = AfxGetApp()->GetProfileInt(_T("subtitle"), _T("show"), true);
	logWrite(_T("[reg] CDShow.ctor: GetProfileInt subtitle/show OK = %d"), m_show_subtitle);

	logWrite(_T("[reg] CDShow.ctor: GetProfileString subtitle/setting ENTER"));
	m_subCfg <<= AfxGetApp()->GetProfileString(_T("subtitle"), _T("setting"), _T(""));
	logWrite(_T("[reg] CDShow.ctor: subtitle/setting deserialize OK is_sane=%d"), (int)m_subCfg.is_sane());

	//<<= 가 deserialize 후 is_sane() 으로 자체 검증, 부적합하면 set_default() 호출.
	//garbage 잔여 (옛 schema, 부분 쓰기, 손상 토큰) 가 registry 에 남아있으면 매 실행마다 reset 되므로
	//현재 sane 한 값을 새 schema 로 즉시 저장 — 다음 실행에서 garbage 를 다시 읽지 않도록.
	{
		CString style;
		AfxGetApp()->WriteProfileString(_T("subtitle"), _T("setting"), style <<= m_subCfg);
	}
	logWrite(_T("[reg] CDShow.ctor: subtitle/setting writeback OK"));

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
	teardown_audio_filter_chain();	//video time-scale + audio gain 둘 다 해제 (이전 teardown_audio_gain_filter 만 호출 시 video filter leak).

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
	SAFE_RELEASE(m_pSplitter)
	SAFE_RELEASE(m_pFileSource);
	SAFE_RELEASE(m_SourceBase);
	SAFE_RELEASE(m_pGB);

	//internal FFmpeg source cache reset — 이전 미디어가 internal path 였고 다음 미디어가 LAV path 인 경우
	//stale m_pFFiSource 로 audio_sync 의 분기가 잘못 발화. CSCAudioGain delay 적용 skip → sync 안 맞음.
	m_pFFiSource = nullptr;

	m_pMP.Release();
	m_pMS.Release();
	m_pMC.Release();
	m_pME.Release();

	m_pVMRWC.Release();
	m_pVMRFC.Release();
	m_pVMRMC.Release();
	m_pVMRMB.Release();
	m_pVDC.Release();
	m_pVP.Release();
	m_pMixerControl.Release();
	m_use_mpcvr = false;

	m_has_subtitle = false;
	m_subtitle_file.Empty();
	m_show_subtitle = true;
	m_hDirectVobSubWnd = NULL;
	m_subtitle_sync = 0;
	m_audio_sync = 0;
	m_user_playback_rate.store(1.0);
	m_media_filename.Empty();

	m_video_stream.clear();
	m_audio_stream.clear();
	m_subtitle_stream.clear();
	m_video_stream_index = 0;
	m_audio_stream_index = 0;
	m_subtitle_stream_index = -1;

	m_mirror = m_flip = false;
	m_panscan_left = 0.0f; m_panscan_top = 0.0f; m_panscan_right = 1.0f; m_panscan_bottom = 1.0f;
	m_last_video_position_rect.SetRectEmpty();

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

	//Internal FFmpeg 경로 — LAV Splitter+Decoder 의 SetPositions 동기 블로킹 회피.
	//HW 가속 가능 codec (h.264/h.265/vp9/av1/mpeg2) 만 internal path. 그 외 (mpeg4/vc1/wmv 등 SW 한정) 는 LAV fallback.
	if (m_use_internal_ffmpeg)
	{
		int ret = load_media_internal_ffmpeg(sfile, pParent);
		logWrite(_T("[load/internal] result=%d"), ret);
		if (ret == 1)
			return ret;

		//ret = -1 (HW 미지원) 또는 0 (실패) → graph 정리 후 LAV path 진행.
		close_media();
		m_use_internal_ffmpeg = false;	//fallback 후 query API 들이 LAV 분기로 가도록 flag reset.
										//(이전엔 true 채로 남아 get_video_codec_name 등 모든 query 가 internal 분기 →
										//m_pFFiSource=nullptr → empty 반환 → UI 의 "Video Codec:" 등 비어 보이는 회귀.)
		CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&m_pGB);
		m_media_filename = sfile;   //close_media() 가 reset 한 filename 재세팅 — LAV path 의 OSD/registry 가 caller file 인식 필요.
		logWrite(_T("[load/internal] falling back to LAV path"));
		//아래 LAV path 로 흐름 계속.
	}

	//Renderer 우선순위: MPCVR > EVR > VMR9. MPC-BE FGFilter.cpp 의 통합 패턴 참고.
	//{71F080AA-8661-4093-B15E-4F6903E77D0A} = MPC Video Renderer (Aleksoid 0.7+).
	static const GUID CLSID_MPCVR_NEW =
		{ 0x71F080AA, 0x8661, 0x4093, { 0xB1, 0x5E, 0x4F, 0x69, 0x03, 0xE7, 0x7D, 0x0A } };
	HRESULT hr_mpcvr = CoCreateInstance(CLSID_MPCVR_NEW, NULL, CLSCTX_INPROC,
		IID_IBaseFilter, (LPVOID*)&m_VMR);
	if (SUCCEEDED(hr_mpcvr) && m_VMR != NULL)
	{
		m_pGB->AddFilter(m_VMR, L"MPC Video Renderer");
		m_use_mpcvr = true;
		logWrite(_T("[renderer] MPC Video Renderer (MPCVR) selected"));

		//MPC-BE 방식 — pin 연결 전 put_Owner 만 호출. put_WindowStyle/put_Visible/put_MessageDrain 은 호출 안 함
		//(E_NOTIMPL 반환 + 내부 상태 변경 가능성 — 직전 시도에서 화면 깨짐 원인).
		CComQIPtr<IVideoWindow> pVW(m_VMR);
		if (pVW)
		{
			HRESULT hr_owner = pVW->put_Owner((OAHWND)pParent->m_hWnd);
			logWrite(_T("[renderer] MPCVR put_Owner=0x%08x"), hr_owner);
		}

		//IExFilterConfig 로 resize 동작 최적화. MPC-BE 가 권장하는 lessRedraws 활성화.
		CComQIPtr<IExFilterConfig> pCfg(m_VMR);
		if (pCfg)
		{
			HRESULT hr_lr = pCfg->Flt_SetBool("lessRedraws", true);
			HRESULT hr_dc = pCfg->Flt_SetBool("d3dFullscreenControl", false);
			logWrite(_T("[renderer] MPCVR cfg lessRedraws=0x%08x d3dFullscreenControl=0x%08x"), hr_lr, hr_dc);
		}
	}
	else
	{
		logWrite(_T("[renderer] MPCVR CoCreate failed hr=0x%08x — try EVR"), hr_mpcvr);
		m_VMR = NULL;
		HRESULT hr_evr = CoCreateInstance(CLSID_EnhancedVideoRenderer, NULL, CLSCTX_INPROC,
			IID_IBaseFilter, (LPVOID*)&m_VMR);
		if (SUCCEEDED(hr_evr) && m_VMR != NULL)
		{
			m_pGB->AddFilter(m_VMR, L"Enhanced Video Renderer");
			logWrite(_T("[renderer] EVR selected"));
		}
		else
		{
			m_VMR = NULL;
			CoCreateInstance(CLSID_VideoMixingRenderer9, NULL, CLSCTX_INPROC,
				IID_IBaseFilter, (LPVOID*)&m_VMR);
			if (m_VMR != NULL)
			{
				m_pGB->AddFilter(m_VMR, L"Video Mixing Renderer 9");
				logWrite(_T("[renderer] VMR9 selected"));
			}
		}
	}

	//EVR 경로 — IMFVideoDisplayControl 로 windowless 제어. MPCVR 는 위에서 IVideoWindow + IBasicVideo 로 처리됨.
	if (m_VMR != NULL && !m_use_mpcvr)
	{
		CComQIPtr<IMFGetService> pGetService(m_VMR);
		if (pGetService)
		{
			pGetService->GetService(MR_VIDEO_RENDER_SERVICE, IID_PPV_ARGS(&m_pVDC));
			if (m_pVDC)
			{
				m_pVDC->SetVideoWindow(pParent->m_hWnd);
				m_pVDC->SetAspectRatioMode(MFVideoARMode_PreservePicture);
				m_pVDC->SetBorderColor(RGB(0, 0, 0));
			}
			//ProcAmp 채널 제어 — graph build 후에야 stream 생성되므로 여기서 QI 는 fail 가능.
			//실제 사용 시점 (adjust_video) 에 lazy 로 다시 시도하되, 가능하면 미리 잡아둠.
			//MPCVR 는 GetService(VP) 가 succeed 하지만 lifecycle 차이로 close_media 시 Release 가 access violation 유발 — skip.
			pGetService->GetService(MR_VIDEO_MIXER_SERVICE, IID_PPV_ARGS(&m_pVP));

			//EVR mixer control — pan&scan output rect. VMR9 의 IVMRMixerControl9 등가.
			pGetService->GetService(MR_VIDEO_MIXER_SERVICE, IID_PPV_ARGS(&m_pMixerControl));
		}
	}

	//VMR9 경로 — EVR QI 가 fail 했거나 fallback 로 VMR9 인스턴스화된 경우.
	if (m_pVDC == NULL && m_VMR != NULL)
	{
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
		}

		m_VMR->QueryInterface(IID_PPV_ARGS(&m_pVMRMB));
		m_VMR->QueryInterface(IID_PPV_ARGS(&m_pVMRMC));

		if (m_pVMRMC != NULL)
		{
			DWORD prefs = 0;
			if (SUCCEEDED(m_pVMRMC->GetMixingPrefs(&prefs)))
			{
				prefs &= ~MixerPref_FilteringMask;
				prefs |= MixerPref_BiLinearFiltering;
				prefs &= ~MixerPref_RenderTargetMask;
				prefs |= MixerPref_RenderTargetYUV;
				if (FAILED(m_pVMRMC->SetMixingPrefs(prefs)))
				{
					prefs &= ~MixerPref_RenderTargetMask;
					prefs |= MixerPref_RenderTargetRGB;
					m_pVMRMC->SetMixingPrefs(prefs);
				}
			}
		}
	}

	//VMR9 windowless 전용 — EVR 은 IMFVideoDisplayControl::SetVideoWindow 로 이미 처리.
	//EVR 모드에서 IVideoWindow::put_Owner 호출 시 windowless 셋업이 깨질 수 있어 skip.
	if (m_pVMRWC != NULL)
	{
		CComQIPtr<IVideoWindow> pVW(m_pGB);
		if (pVW != NULL)
		{
			hr = pVW->put_Owner((OAHWND)pParent->m_hWnd);
			hr = pVW->put_WindowStyle(WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
		}
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
				logWrite(_T("[wmv] is_windows_media=true — building graph"));

				//WM ASF Reader 는 source 로 유지 (ASF 파싱 잘 함). decoder 만 LAV 로 교체 — MPC-BE 패턴.
				//기존 WMVideo/WMAudio Decoder DMO 는 seek 후 video frame 생성을 못 하는 알려진 결함 → LAV 의
				//FFmpeg 기반 decoder 가 WMV1/2/3/VC-1/WMA 모두 처리 + 빠른 flush + DXVA2 가속.
				HRESULT hr_find = FindFilter(_T("WM ASF Reader"), CLSID_LegacyAmFilterCategory, &m_SourceBase );
				HRESULT hr_add = m_SourceBase ? m_pGB->AddFilter(m_SourceBase, _T("WM ASF Reader")) : E_FAIL;
				logWrite(_T("[wmv] WM ASF Reader find=0x%08x add=0x%08x ptr=%p"), hr_find, hr_add, m_SourceBase);

				if (m_SourceBase)
				{
					hr = m_SourceBase->QueryInterface( IID_IFileSourceFilter, (void **) &m_pFileSource );
					if (m_pFileSource)
					{
						HRESULT hr_load = m_pFileSource->Load(wFileName, NULL);
						logWrite(_T("[wmv] WM ASF Reader Load hr=0x%08x"), hr_load);
					}
				}

				IBaseFilter* pLavV = NULL;
				HRESULT hr_lav_v_find = FindFilter(_T("LAV Video Decoder"), CLSID_LegacyAmFilterCategory, &pLavV);
				HRESULT hr_lav_v_add = pLavV ? m_pGB->AddFilter(pLavV, _T("LAV Video Decoder")) : E_FAIL;
				logWrite(_T("[wmv] LAV Video Decoder find=0x%08x add=0x%08x ptr=%p"), hr_lav_v_find, hr_lav_v_add, pLavV);

				IBaseFilter* pLavA = NULL;
				HRESULT hr_lav_a_find = FindFilter(_T("LAV Audio Decoder"), CLSID_LegacyAmFilterCategory, &pLavA);
				HRESULT hr_lav_a_add = pLavA ? m_pGB->AddFilter(pLavA, _T("LAV Audio Decoder")) : E_FAIL;
				logWrite(_T("[wmv] LAV Audio Decoder find=0x%08x add=0x%08x ptr=%p"), hr_lav_a_find, hr_lav_a_add, pLavA);

				//LAV Video Decoder 가 없으면 fallback 으로 WMVideo Decoder DMO 추가 (느리지만 재생은 됨).
				if (!pLavV)
				{
					IBaseFilter* pDmo = NULL;
					HRESULT hr_dmo_find = FindFilter(_T("WMVideo Decoder DMO"), CLSID_LegacyAmFilterCategory, &pDmo);
					HRESULT hr_dmo_add = pDmo ? m_pGB->AddFilter(pDmo, _T("WMVideo Decoder DMO")) : E_FAIL;
					logWrite(_T("[wmv] fallback WMVideo Decoder DMO find=0x%08x add=0x%08x"), hr_dmo_find, hr_dmo_add);
				}
				if (!pLavA)
				{
					IBaseFilter* pDmo = NULL;
					HRESULT hr_dmo_find = FindFilter(_T("WMAudio Decoder DMO"), CLSID_LegacyAmFilterCategory, &pDmo);
					HRESULT hr_dmo_add = pDmo ? m_pGB->AddFilter(pDmo, _T("WMAudio Decoder DMO")) : E_FAIL;
					logWrite(_T("[wmv] fallback WMAudio Decoder DMO find=0x%08x add=0x%08x"), hr_dmo_find, hr_dmo_add);
				}
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
				//BT.2020+PQ → BT.709+sRGB 변환을 LAV 가 직접 수행. 그 후 우리 렌더러 (EVR/MPCVR/VMR9) 에는 SDR frame 만 전달.
				//현재 사용 렌더러 모두 HDR 미지원이라 LAV 측에서 미리 변환하지 않으면 워시드 아웃.
				//HWAccel 은 사용자 설정 보존 — 이미 0(SW)이 아니거나 정의돼 있으면 건드리지 않음.
				//HWAccelTonemap 만 우리 SDR-only 출력을 위해 강제.
				HKEY hLavKey = NULL;
				if (::RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\LAV\\Video"),
						0, NULL, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, NULL, &hLavKey, NULL) == ERROR_SUCCESS)
				{
					DWORD hwaccel_existing = 0;
					DWORD cb = sizeof(hwaccel_existing);
					LONG q = ::RegQueryValueEx(hLavKey, _T("HWAccel"), NULL, NULL,
						(LPBYTE)&hwaccel_existing, &cb);
					//사용자가 SW 모드 (0) 거나 키 자체 없을 때만 GPU 가속 enable. 기존 1/2/3 은 그대로 둠.
					if (q != ERROR_SUCCESS || hwaccel_existing == 0)
					{
						DWORD hwaccel = 2;	//DXVA2-copyback — 모든 렌더러와 호환되는 가장 안전한 GPU 가속.
						::RegSetValueEx(hLavKey, _T("HWAccel"), 0, REG_DWORD,
							(const BYTE*)&hwaccel, sizeof(hwaccel));
					}
					DWORD hw_tonemap = 1;
					::RegSetValueEx(hLavKey, _T("HWAccelTonemap"), 0, REG_DWORD,
						(const BYTE*)&hw_tonemap, sizeof(hw_tonemap));
					::RegCloseKey(hLavKey);
				}

				//LAV Video Decoder 의 format-specific enable — 일부 사용자 환경에서 wmv3/vc1 등이 default disabled.
				//disabled 면 LAV Video 의 input pin 이 해당 subtype reject → graph builder 가 WMVideo Decoder DMO 로 fallback
				//→ DMO 의 seek-flush 결함으로 WMV seek 후 영상 freeze. 명시적 enable 로 LAV 가 처리하게 보장.
				HKEY hLavFmt = NULL;
				if (::RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\LAV\\Video\\Formats"),
						0, NULL, 0, KEY_SET_VALUE, NULL, &hLavFmt, NULL) == ERROR_SUCCESS)
				{
					DWORD on = 1;
					::RegSetValueEx(hLavFmt, _T("wmv1"), 0, REG_DWORD, (const BYTE*)&on, sizeof(on));
					::RegSetValueEx(hLavFmt, _T("wmv2"), 0, REG_DWORD, (const BYTE*)&on, sizeof(on));
					::RegSetValueEx(hLavFmt, _T("wmv3"), 0, REG_DWORD, (const BYTE*)&on, sizeof(on));
					::RegSetValueEx(hLavFmt, _T("vc1"),  0, REG_DWORD, (const BYTE*)&on, sizeof(on));
					::RegCloseKey(hLavFmt);
				}

				//LAV Audio Decoder 의 format-specific enable — wma 계열.
				HKEY hLavAFmt = NULL;
				if (::RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\LAV\\Audio\\Formats"),
						0, NULL, 0, KEY_SET_VALUE, NULL, &hLavAFmt, NULL) == ERROR_SUCCESS)
				{
					DWORD on = 1;
					::RegSetValueEx(hLavAFmt, _T("wmav1"),    0, REG_DWORD, (const BYTE*)&on, sizeof(on));
					::RegSetValueEx(hLavAFmt, _T("wmav2"),    0, REG_DWORD, (const BYTE*)&on, sizeof(on));
					::RegSetValueEx(hLavAFmt, _T("wmavoice"), 0, REG_DWORD, (const BYTE*)&on, sizeof(on));
					::RegSetValueEx(hLavAFmt, _T("wmapro"),   0, REG_DWORD, (const BYTE*)&on, sizeof(on));
					::RegSetValueEx(hLavAFmt, _T("wmalossless"), 0, REG_DWORD, (const BYTE*)&on, sizeof(on));
					::RegCloseKey(hLavAFmt);
				}

				//LAV Splitter 손상 미디어 대응 설정 — registry 에 set 후 CoCreateInstance 시점에 LAV 가 read.
				//- GenerateMissingCueTime: Matroska 의 cue table 누락/손상 시 LAV 가 자체 cue 생성 (linear scan 한 번).
				//  → 이후 seek 요청에 빠르고 정확한 keyframe 응답 가능. 손상 미디어 freeze 완화 핵심.
				//- MaxQueueSizeMem / MaxQueueSize: splitter 내부 버퍼 확대 → seek 직후 decoder 가 버퍼링한 데이터로 빨리 회복.
				HKEY hLavSpKey = NULL;
				if (::RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\LAV\\Splitter"),
						0, NULL, 0, KEY_SET_VALUE, NULL, &hLavSpKey, NULL) == ERROR_SUCCESS)
				{
					DWORD gen_cue = 1;
					::RegSetValueEx(hLavSpKey, _T("GenerateMissingCueTime"), 0, REG_DWORD,
						(const BYTE*)&gen_cue, sizeof(gen_cue));
					DWORD max_mem = 256;	//MB. default 64.
					::RegSetValueEx(hLavSpKey, _T("MaxQueueSizeMem"), 0, REG_DWORD,
						(const BYTE*)&max_mem, sizeof(max_mem));
					DWORD max_pkts = 1000;	//packets per stream. default 350.
					::RegSetValueEx(hLavSpKey, _T("MaxQueueSize"), 0, REG_DWORD,
						(const BYTE*)&max_pkts, sizeof(max_pkts));
					::RegCloseKey(hLavSpKey);
				}

				hr = FindFilter(_T("LAV Splitter"), CLSID_LegacyAmFilterCategory, &m_pSplitter );
				hr = m_pGB->AddFilter(m_pSplitter, _T("LAV Splitter"));

				//File Source → LAV Splitter 를 명시적으로 Connect — 이후 RenderOutputPins 시 graph builder 가 이미
				//연결된 LAV 를 따라 진행하고 더 높은 merit 의 Haali Media Splitter (AR) 등을 자동 채택해 video pin
				//없이 audio 만 빠지는 케이스 차단.
				if (m_SourceBase && m_pSplitter)
				{
					CComPtr<IPin> pSrcOut, pSplIn;
					CComPtr<IEnumPins> pEnumS;
					if (SUCCEEDED(m_SourceBase->EnumPins(&pEnumS)))
					{
						CComPtr<IPin> p;
						while (pEnumS->Next(1, &p, NULL) == S_OK)
						{
							PIN_DIRECTION dir;
							if (SUCCEEDED(p->QueryDirection(&dir)) && dir == PINDIR_OUTPUT)
							{
								pSrcOut = p;
								break;
							}
							p.Release();
						}
					}
					CComPtr<IEnumPins> pEnumD;
					if (SUCCEEDED(m_pSplitter->EnumPins(&pEnumD)))
					{
						CComPtr<IPin> p;
						while (pEnumD->Next(1, &p, NULL) == S_OK)
						{
							PIN_DIRECTION dir;
							if (SUCCEEDED(p->QueryDirection(&dir)) && dir == PINDIR_INPUT)
							{
								pSplIn = p;
								break;
							}
							p.Release();
						}
					}
					if (pSrcOut && pSplIn)
					{
						HRESULT hr_conn = m_pGB->Connect(pSrcOut, pSplIn);
						logWrite(_T("[splitter] explicit Connect File Source -> LAV Splitter hr=0x%08x"), hr_conn);
					}
				}

				IBaseFilter* pLavVideo = NULL;
				hr = FindFilter(_T("LAV Video Decoder"), CLSID_LegacyAmFilterCategory, &pLavVideo);
				if (pLavVideo)
					m_pGB->AddFilter(pLavVideo, _T("LAV Video Decoder"));

				IBaseFilter* pLavAudio = NULL;
				hr = FindFilter(_T("LAV Audio Decoder"), CLSID_LegacyAmFilterCategory, &pLavAudio);
				if (pLavAudio)
					m_pGB->AddFilter(pLavAudio, _T("LAV Audio Decoder"));

				//Splitter 의 named output pin 을 LAV decoder 의 input 에 명시적 Connect — graph builder 가 자동으로
				//WMVideo/WMAudio Decoder DMO 를 채택하지 않도록 강제. WMV/ASF 같은 포맷에서 DMO 의 seek-flush
				//결함을 우회. RenderOutputPins(splitter) 가 이미 connected pin 을 skip 하므로 안전.
				auto connect_splitter_to_lav = [&](LPCWSTR splitterPinName, IBaseFilter* pLavDec) -> HRESULT
				{
					if (!m_pSplitter || !pLavDec)
						return E_FAIL;

					CComPtr<IPin> pSpOut;
					{
						CComPtr<IEnumPins> pEnum;
						if (FAILED(m_pSplitter->EnumPins(&pEnum)))
							return E_FAIL;
						CComPtr<IPin> p;
						while (pEnum->Next(1, &p, NULL) == S_OK)
						{
							PIN_DIRECTION dir;
							PIN_INFO info = {};
							p->QueryDirection(&dir);
							p->QueryPinInfo(&info);
							if (info.pFilter)
								info.pFilter->Release();
							if (dir == PINDIR_OUTPUT && wcscmp(info.achName, splitterPinName) == 0)
							{
								pSpOut = p;
								break;
							}
							p.Release();
						}
					}
					if (!pSpOut)
						return E_FAIL;

					CComPtr<IPin> pLavIn;
					{
						CComPtr<IEnumPins> pEnum;
						if (FAILED(pLavDec->EnumPins(&pEnum)))
							return E_FAIL;
						CComPtr<IPin> p;
						while (pEnum->Next(1, &p, NULL) == S_OK)
						{
							PIN_DIRECTION dir;
							p->QueryDirection(&dir);
							if (dir == PINDIR_INPUT)
							{
								pLavIn = p;
								break;
							}
							p.Release();
						}
					}
					if (!pLavIn)
						return E_FAIL;

					return m_pGB->Connect(pSpOut, pLavIn);
				};

				HRESULT hr_v = connect_splitter_to_lav(L"Video", pLavVideo);
				HRESULT hr_a = connect_splitter_to_lav(L"Audio", pLavAudio);
				logWrite(_T("[splitter->lav] Video hr=0x%08x  Audio hr=0x%08x"), hr_v, hr_a);

				//LAV 가 WMV3/WMA 등을 reject 하는 환경 fallback — MPC Video Decoder 시도 (MPC-BE 설치 시 사용 가능).
				//그것도 실패면 default RenderOutputPins(splitter) 가 registry 에서 (느린) DMO 자동 선택.
				IBaseFilter* pMpcVideo = NULL;
				if (FAILED(hr_v))
				{
					if (pLavVideo) { m_pGB->RemoveFilter(pLavVideo); pLavVideo->Release(); pLavVideo = NULL; }
					if (SUCCEEDED(FindFilter(_T("MPC Video Decoder"), CLSID_LegacyAmFilterCategory, &pMpcVideo)) && pMpcVideo)
					{
						m_pGB->AddFilter(pMpcVideo, _T("MPC Video Decoder"));
						hr_v = connect_splitter_to_lav(L"Video", pMpcVideo);
						logWrite(_T("[splitter->mpc] Video hr=0x%08x"), hr_v);
					}
				}
				IBaseFilter* pMpcAudio = NULL;
				if (FAILED(hr_a))
				{
					if (pLavAudio) { m_pGB->RemoveFilter(pLavAudio); pLavAudio->Release(); pLavAudio = NULL; }
					if (SUCCEEDED(FindFilter(_T("MPC Audio Decoder"), CLSID_LegacyAmFilterCategory, &pMpcAudio)) && pMpcAudio)
					{
						m_pGB->AddFilter(pMpcAudio, _T("MPC Audio Decoder"));
						hr_a = connect_splitter_to_lav(L"Audio", pMpcAudio);
						logWrite(_T("[splitter->mpc] Audio hr=0x%08x"), hr_a);
					}
				}

				//Splitter→Decoder 가 직접 연결되면서 RenderOutputPins(splitter) 가 splitter pin 을 skip → decoder 의
				//OUTPUT pin 까지 chain 이 안 내려감. decoder 의 output 을 명시적 render 해 renderer / audio chain 까지 완성.
				IBaseFilter* pVDec = pLavVideo ? pLavVideo : pMpcVideo;
				IBaseFilter* pADec = pLavAudio ? pLavAudio : pMpcAudio;
				if (SUCCEEDED(hr_v) && pVDec)
				{
					HRESULT hr_rv = RenderOutputPins(m_pGB, pVDec);
					logWrite(_T("[decoder->renderer] Video render hr=0x%08x"), hr_rv);
				}
				if (SUCCEEDED(hr_a) && pADec)
				{
					HRESULT hr_ra = RenderOutputPins(m_pGB, pADec);
					logWrite(_T("[decoder->renderer] Audio render hr=0x%08x"), hr_ra);
				}

				if (pLavVideo) pLavVideo->Release();
				if (pLavAudio) pLavAudio->Release();
				if (pMpcVideo) pMpcVideo->Release();
				if (pMpcAudio) pMpcAudio->Release();

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

		//File Source 의 output 이 explicit Connect 로 LAV Splitter 에 이미 붙어있는 경우 RenderOutputPins 는
		//그 pin 을 already-connected 로 skip — splitter 의 video/audio output 까지 내려가지 않음.
		//splitter 의 output pin 들도 render 해 chain 완성.
		if (m_pSplitter)
			RenderOutputPins(m_pGB, m_pSplitter);

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

	//audio gain filter + video time-scale filter 끼움 — graph 의 audio/video renderer 직전. graph 빌드 끝난 후 한 번.
	setup_audio_filter_chain();

	//Graph reference clock 을 audio renderer (default) 가 아닌 SystemClock 으로 변경 —
	//  audio renderer 가 clock master 면 seek 후 audio buffer preroll (~50-200ms) 동안 clock 이 정지 → video 도 wait → 사용자 체감 delay.
	//  SystemClock 은 시스템 시간 기반 free-running 이라 buffer fill 과 무관하게 항상 advance → seek 후 first frame 도착 즉시 표시.
	//  Trade-off: audio renderer 가 system clock 에 sync (자체 sample rate 와 미세 차이 보정 — drop/dup sample 가능, 보통 imperceptible).
	//  진단에서 SetSyncSource(NULL) 시 video 가 1배속보다 빠르게 free-run 됨을 확인 → clock 이 video pace limiter 임을 증명.
	//  NULL 대신 SystemClock 으로 video pace 유지 + audio buffer wait 우회.
	{
		CComPtr<IReferenceClock> pSysClock;
		HRESULT hr_sc = CoCreateInstance(CLSID_SystemClock, NULL, CLSCTX_INPROC_SERVER,
			IID_IReferenceClock, (void**)&pSysClock);
		if (SUCCEEDED(hr_sc) && pSysClock)
		{
			CComQIPtr<IMediaFilter> pMF(m_pGB);
			if (pMF)
			{
				HRESULT hr_set = pMF->SetSyncSource(pSysClock);
				logWrite(_T("[clock] graph sync source = SystemClock hr=0x%08x"), hr_set);
			}
		}
		else
		{
			logWrite(_T("[clock] SystemClock CoCreateInstance failed hr=0x%08x — keep default audio clock"), hr_sc);
		}
	}

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

#include "../../ffmpeg/internal/ffi_source_filter.h"

//---- UI 표시용 codec/format info — graph + decoder context 직접 query.
//internal FFmpeg path: m_pFFiSource->decoder() 의 AVCodecParameters 정보.
//LAV path: 현재는 보유 정보 (m_video_size / m_frame_rate / m_video_stream 이름) 만 — 추가 정밀 query 는 Phase 다음.

//graph filter enumerate helper — 정의는 아래쪽. forward 선언.
static IBaseFilter* find_renderer_by_majortype(IFilterGraph* pGraph, REFGUID target_majortype);
static IBaseFilter* get_upstream_filter(IBaseFilter* pDownstream);
static CString filter_name(IBaseFilter* pFilter);

static ffi::CDecoder* ffi_decoder_or_null(void* pFFiSource)
{
	if (!pFFiSource)
		return nullptr;
	return &((ffi::CFFiSource*)pFFiSource)->decoder();
}

CString CDShow::get_video_codec_name()
{
	if (m_use_internal_ffmpeg)
	{
		if (auto* d = ffi_decoder_or_null(m_pFFiSource))
			return CString(d->video_codec_name().c_str());
	}
	return _T("");
}

CString CDShow::get_video_fourcc()
{
	if (m_use_internal_ffmpeg)
	{
		if (auto* d = ffi_decoder_or_null(m_pFFiSource))
			return CString(d->video_fourcc().c_str());
	}
	return _T("");
}

int CDShow::get_video_width()
{
	if (m_use_internal_ffmpeg)
	{
		if (auto* d = ffi_decoder_or_null(m_pFFiSource))
			return d->video_width();
	}
	return m_video_size.cx;
}

int CDShow::get_video_height()
{
	if (m_use_internal_ffmpeg)
	{
		if (auto* d = ffi_decoder_or_null(m_pFFiSource))
			return d->video_height();
	}
	return m_video_size.cy;
}

int CDShow::get_video_bit_depth()
{
	if (m_use_internal_ffmpeg)
	{
		if (auto* d = ffi_decoder_or_null(m_pFFiSource))
			return d->video_bit_depth();
	}
	return 0;
}

int64_t CDShow::get_video_bit_rate()
{
	if (m_use_internal_ffmpeg)
	{
		if (auto* d = ffi_decoder_or_null(m_pFFiSource))
			return d->video_bit_rate();
	}
	return 0;
}

CString CDShow::get_video_aspect_ratio()
{
	if (m_use_internal_ffmpeg)
	{
		if (auto* d = ffi_decoder_or_null(m_pFFiSource))
			return CString(d->video_aspect_ratio().c_str());
	}
	//LAV fallback — width:height 의 simplified ratio.
	int w = m_video_size.cx, h = m_video_size.cy;
	if (w <= 0 || h <= 0)
		return _T("");
	int a = w, b = h;
	while (b) { int t = a % b; a = b; b = t; }
	if (a > 0) { w /= a; h /= a; }
	CString out;
	out.Format(_T("%d:%d"), w, h);
	return out;
}

double CDShow::get_video_fps()
{
	if (m_use_internal_ffmpeg)
	{
		if (auto* d = ffi_decoder_or_null(m_pFFiSource))
			return d->frame_rate();
	}
	return m_frame_rate;
}

CString CDShow::get_audio_codec_name()
{
	if (m_use_internal_ffmpeg)
	{
		if (auto* d = ffi_decoder_or_null(m_pFFiSource))
			return CString(d->audio_codec_name().c_str());
	}
	return _T("");
}

int CDShow::get_audio_sample_rate()
{
	if (m_use_internal_ffmpeg)
	{
		if (auto* d = ffi_decoder_or_null(m_pFFiSource))
			return d->audio_sample_rate();
	}
	return 0;
}

int CDShow::get_audio_channels()
{
	if (m_use_internal_ffmpeg)
	{
		if (auto* d = ffi_decoder_or_null(m_pFFiSource))
			return d->audio_channels();
	}
	return 0;
}

int CDShow::get_audio_bit_depth()
{
	if (m_use_internal_ffmpeg)
	{
		if (auto* d = ffi_decoder_or_null(m_pFFiSource))
			return d->audio_bit_depth();
	}
	return 0;
}

int64_t CDShow::get_audio_bit_rate()
{
	if (m_use_internal_ffmpeg)
	{
		if (auto* d = ffi_decoder_or_null(m_pFFiSource))
			return d->audio_bit_rate();
	}
	return 0;
}

CString CDShow::get_video_pixel_format()
{
	if (m_use_internal_ffmpeg)
	{
		if (auto* d = ffi_decoder_or_null(m_pFFiSource))
			return CString(d->video_pixel_format_name().c_str());
	}
	return _T("");
}

CString CDShow::get_video_hw_accel_name()
{
	if (m_use_internal_ffmpeg)
	{
		if (auto* d = ffi_decoder_or_null(m_pFFiSource))
			return CString(d->video_hw_accel_name().c_str());
	}
	return _T("");
}

CString CDShow::get_audio_channel_layout()
{
	if (m_use_internal_ffmpeg)
	{
		if (auto* d = ffi_decoder_or_null(m_pFFiSource))
			return CString(d->audio_channel_layout_name().c_str());
	}
	return _T("");
}

CString CDShow::get_video_decoder_label()
{
	if (m_use_internal_ffmpeg)
	{
		auto* d = ffi_decoder_or_null(m_pFFiSource);
		if (!d)
			return _T("");
		CString codec(d->video_codec_name().c_str());
		CString hw(d->video_hw_accel_name().c_str());
		CString out = _T("내장 FFmpeg 디코더");
		if (!codec.IsEmpty())
		{
			out += _T(" (");
			out += codec;
			if (!hw.IsEmpty())
			{
				out += _T(", ");
				out += hw;
			}
			out += _T(")");
		}
		return out;
	}
	//LAV path — m_VMR 의 upstream filter (= video decoder).
	if (!m_VMR)
		return _T("");
	IBaseFilter* pDecoder = get_upstream_filter(m_VMR);
	if (!pDecoder)
		return _T("");
	CString name = filter_name(pDecoder);
	pDecoder->Release();
	return name;
}

CString CDShow::get_video_renderer_label()
{
	if (!m_VMR)
		return _T("");
	FILTER_INFO info = { 0 };
	HRESULT hr = m_VMR->QueryFilterInfo(&info);
	if (FAILED(hr))
		return _T("");
	CString name(info.achName);
	if (info.pGraph)
		info.pGraph->Release();
	return name;
}

//graph 의 filter 중 *output pin 없음 + input pin 의 ConnectionMediaType.majortype == target* 인 filter (renderer).
//caller 가 Release.
static IBaseFilter* find_renderer_by_majortype(IFilterGraph* pGraph, REFGUID target_majortype)
{
	if (!pGraph)
		return NULL;

	IEnumFilters* pEnum = NULL;
	if (FAILED(pGraph->EnumFilters(&pEnum)))
		return NULL;

	IBaseFilter* result = NULL;
	IBaseFilter* pF = NULL;
	ULONG fetched = 0;
	while (!result && pEnum->Next(1, &pF, &fetched) == S_OK && fetched == 1)
	{
		IEnumPins* pPinEnum = NULL;
		bool has_output = false;
		bool has_target_input = false;
		if (SUCCEEDED(pF->EnumPins(&pPinEnum)))
		{
			IPin* pPin = NULL;
			ULONG pf = 0;
			while (!has_output && pPinEnum->Next(1, &pPin, &pf) == S_OK && pf == 1)
			{
				PIN_DIRECTION dir;
				pPin->QueryDirection(&dir);
				if (dir == PINDIR_OUTPUT)
				{
					has_output = true;
				}
				else if (dir == PINDIR_INPUT)
				{
					AM_MEDIA_TYPE mt = { 0 };
					if (SUCCEEDED(pPin->ConnectionMediaType(&mt)))
					{
						if (mt.majortype == target_majortype)
							has_target_input = true;
						if (mt.cbFormat != 0) { CoTaskMemFree(mt.pbFormat); mt.pbFormat = NULL; mt.cbFormat = 0; }
						if (mt.pUnk) { mt.pUnk->Release(); mt.pUnk = NULL; }
					}
				}
				pPin->Release();
			}
			pPinEnum->Release();
		}
		if (!has_output && has_target_input)
		{
			result = pF;	//ref 유지 — caller 가 Release.
		}
		else
		{
			pF->Release();
		}
	}
	pEnum->Release();
	return result;
}

//filter 의 *input pin 의 ConnectedTo* upstream filter 반환. caller 가 Release.
static IBaseFilter* get_upstream_filter(IBaseFilter* pDownstream)
{
	if (!pDownstream)
		return NULL;

	IEnumPins* pPinEnum = NULL;
	if (FAILED(pDownstream->EnumPins(&pPinEnum)))
		return NULL;

	IBaseFilter* result = NULL;
	IPin* pPin = NULL;
	ULONG pf = 0;
	while (!result && pPinEnum->Next(1, &pPin, &pf) == S_OK && pf == 1)
	{
		PIN_DIRECTION dir;
		pPin->QueryDirection(&dir);
		if (dir == PINDIR_INPUT)
		{
			IPin* pUpstream = NULL;
			if (SUCCEEDED(pPin->ConnectedTo(&pUpstream)) && pUpstream)
			{
				PIN_INFO pi = { 0 };
				if (SUCCEEDED(pUpstream->QueryPinInfo(&pi)))
				{
					result = pi.pFilter;	//ref 유지 — caller Release.
				}
				pUpstream->Release();
			}
		}
		pPin->Release();
	}
	pPinEnum->Release();
	return result;
}

static CString filter_name(IBaseFilter* pFilter)
{
	if (!pFilter)
		return _T("");
	FILTER_INFO info = { 0 };
	if (FAILED(pFilter->QueryFilterInfo(&info)))
		return _T("");
	CString name(info.achName);
	if (info.pGraph)
		info.pGraph->Release();
	return name;
}

CString CDShow::get_audio_renderer_label()
{
	IBaseFilter* pRenderer = find_renderer_by_majortype(m_pGB, MEDIATYPE_Audio);
	if (!pRenderer)
		return _T("");
	CString name = filter_name(pRenderer);
	pRenderer->Release();
	return name;
}

CString CDShow::get_audio_decoder_label()
{
	if (m_use_internal_ffmpeg)
	{
		auto* d = ffi_decoder_or_null(m_pFFiSource);
		if (!d)
			return _T("");
		CString codec(d->audio_codec_name().c_str());
		CString out = _T("내장 FFmpeg 디코더");
		if (!codec.IsEmpty())
		{
			out += _T(" (");
			out += codec;
			out += _T(")");
		}
		return out;
	}
	//LAV path — audio renderer 의 upstream filter (= audio decoder).
	IBaseFilter* pRenderer = find_renderer_by_majortype(m_pGB, MEDIATYPE_Audio);
	if (!pRenderer)
		return _T("");
	IBaseFilter* pDecoder = get_upstream_filter(pRenderer);
	pRenderer->Release();
	if (!pDecoder)
		return _T("");
	CString name = filter_name(pDecoder);
	pDecoder->Release();
	return name;
}

bool CDShow::get_frame_stats(FrameStats& out)
{
	out = FrameStats();
	if (!m_VMR)
		return false;

	IQualProp* pQP = NULL;
	if (FAILED(m_VMR->QueryInterface(IID_IQualProp, (void**)&pQP)) || !pQP)
		return false;

	int v;
	if (SUCCEEDED(pQP->get_FramesDrawn(&v)))                out.frames_drawn = v;
	if (SUCCEEDED(pQP->get_FramesDroppedInRenderer(&v)))    out.frames_dropped = v;
	if (SUCCEEDED(pQP->get_AvgFrameRate(&v)))               out.avg_frame_rate_x100 = v;
	if (SUCCEEDED(pQP->get_AvgSyncOffset(&v)))              out.avg_sync_offset_ms = v;
	if (SUCCEEDED(pQP->get_DevSyncOffset(&v)))              out.dev_sync_offset_ms = v;
	if (SUCCEEDED(pQP->get_Jitter(&v)))                     out.jitter_ms = v;

	pQP->Release();
	return true;
}

int CDShow::get_refresh_rate_hz()
{
	DEVMODE dm = { 0 };
	dm.dmSize = sizeof(dm);
	if (!EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm))
		return 0;
	return (int)dm.dmDisplayFrequency;
}

CString CDShow::get_video_output_subtype()
{
	if (!m_VMR)
		return _T("");

	IEnumPins* pPinEnum = NULL;
	if (FAILED(m_VMR->EnumPins(&pPinEnum)))
		return _T("");

	CString result;
	IPin* pPin = NULL;
	ULONG pf = 0;
	while (result.IsEmpty() && pPinEnum->Next(1, &pPin, &pf) == S_OK && pf == 1)
	{
		PIN_DIRECTION dir;
		pPin->QueryDirection(&dir);
		if (dir == PINDIR_INPUT)
		{
			AM_MEDIA_TYPE mt = { 0 };
			if (SUCCEEDED(pPin->ConnectionMediaType(&mt)))
			{
				//subtype GUID 의 첫 4-char (Data1) 가 fourcc 인 경우 그것 사용.
				DWORD d1 = mt.subtype.Data1;
				char buf[5] = { 0 };
				for (int i = 0; i < 4; i++)
				{
					unsigned char c = (d1 >> (i * 8)) & 0xFF;
					if (c >= 0x20 && c < 0x7F)
						buf[i] = (char)c;
					else
					{
						buf[0] = 0;
						break;
					}
				}
				if (buf[0])
				{
					for (int i = 0; i < 4 && buf[i]; i++)
						result += (TCHAR)(buf[i] >= 'a' && buf[i] <= 'z' ? buf[i] - 32 : buf[i]);
				}
				if (mt.cbFormat != 0) { CoTaskMemFree(mt.pbFormat); mt.pbFormat = NULL; mt.cbFormat = 0; }
				if (mt.pUnk) { mt.pUnk->Release(); mt.pUnk = NULL; }
			}
		}
		pPin->Release();
	}
	pPinEnum->Release();
	return result;
}

CSize CDShow::get_display_size()
{
	CSize sz(0, 0);
	if (!m_VMR)
		return sz;
	IBaseFilter* pVMR = m_VMR;
	//VMR 의 video window 가 자체 HWND 보유 시 (IVideoWindow). 부재 시 m_pParent 의 client.
	IVideoWindow* pVW = NULL;
	if (SUCCEEDED(pVMR->QueryInterface(IID_IVideoWindow, (void**)&pVW)) && pVW)
	{
		long w = 0, h = 0;
		pVW->get_Width(&w);
		pVW->get_Height(&h);
		pVW->Release();
		if (w > 0 && h > 0)
		{
			sz.cx = (int)w;
			sz.cy = (int)h;
			return sz;
		}
	}
	//fallback: parent client.
	if (m_pParent && m_pParent->GetSafeHwnd())
	{
		CRect rc;
		m_pParent->GetClientRect(&rc);
		sz.cx = rc.Width();
		sz.cy = rc.Height();
	}
	return sz;
}

CString CDShow::get_display_adapter_name()
{
	DISPLAY_DEVICE dd = { 0 };
	dd.cb = sizeof(dd);
	if (!EnumDisplayDevices(NULL, 0, &dd, 0))
		return _T("");
	return CString(dd.DeviceString);
}

int CDShow::get_video_memory_mb()
{
	//IDXGIFactory → IDXGIAdapter[0] → GetDesc().DedicatedVideoMemory.
	//d3d11.lib / dxgi.lib link 필요 — dshow.cpp 가 이미 D3D11 사용 (internal path) 라 link 가능.
	typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY)(REFIID, void**);
	static PFN_CREATE_DXGI_FACTORY pfn = NULL;
	if (!pfn)
	{
		HMODULE hMod = LoadLibrary(_T("dxgi.dll"));
		if (hMod)
			pfn = (PFN_CREATE_DXGI_FACTORY)GetProcAddress(hMod, "CreateDXGIFactory1");
	}
	if (!pfn)
		return 0;

	IDXGIFactory1* pFactory = NULL;
	if (FAILED(pfn(__uuidof(IDXGIFactory1), (void**)&pFactory)) || !pFactory)
		return 0;

	IDXGIAdapter1* pAdapter = NULL;
	int total_mb = 0;
	if (SUCCEEDED(pFactory->EnumAdapters1(0, &pAdapter)) && pAdapter)
	{
		DXGI_ADAPTER_DESC1 desc = { 0 };
		if (SUCCEEDED(pAdapter->GetDesc1(&desc)))
			total_mb = (int)(desc.DedicatedVideoMemory / (1024 * 1024));
		pAdapter->Release();
	}
	pFactory->Release();
	return total_mb;
}

int CDShow::load_media_internal_ffmpeg(CString sfile, CWnd* pParent)
{
	//Phase 3c — LAV Splitter+Decoder skip, CFFiSource 단일 filter 로 demux+decode.
	//Renderer 는 기존 우선순위 (MPCVR > EVR > VMR9) 그대로 사용.
	//Audio path 는 Phase 4 — 현재 video only.

	//=== 1) Renderer 추가 (기존 load_media 의 renderer 셋업 코드 압축 사본) ===
	static const GUID CLSID_MPCVR_NEW =
		{ 0x71F080AA, 0x8661, 0x4093, { 0xB1, 0x5E, 0x4F, 0x69, 0x03, 0xE7, 0x7D, 0x0A } };
	HRESULT hr_mpcvr = CoCreateInstance(CLSID_MPCVR_NEW, NULL, CLSCTX_INPROC,
		IID_IBaseFilter, (LPVOID*)&m_VMR);
	if (SUCCEEDED(hr_mpcvr) && m_VMR != NULL)
	{
		m_pGB->AddFilter(m_VMR, L"MPC Video Renderer");
		m_use_mpcvr = true;
		logWrite(_T("[internal] MPC Video Renderer selected"));
		CComQIPtr<IVideoWindow> pVW(m_VMR);
		if (pVW && pParent)
			pVW->put_Owner((OAHWND)pParent->m_hWnd);
		CComQIPtr<IExFilterConfig> pCfg(m_VMR);
		if (pCfg)
		{
			pCfg->Flt_SetBool("lessRedraws", true);
			pCfg->Flt_SetBool("d3dFullscreenControl", false);
		}
	}
	else
	{
		HRESULT hr_evr = CoCreateInstance(CLSID_EnhancedVideoRenderer, NULL, CLSCTX_INPROC,
			IID_IBaseFilter, (LPVOID*)&m_VMR);
		if (SUCCEEDED(hr_evr) && m_VMR != NULL)
		{
			m_pGB->AddFilter(m_VMR, L"Enhanced Video Renderer");
			logWrite(_T("[internal] EVR selected"));
		}
		else
		{
			CoCreateInstance(CLSID_VideoMixingRenderer9, NULL, CLSCTX_INPROC,
				IID_IBaseFilter, (LPVOID*)&m_VMR);
			if (m_VMR != NULL)
			{
				m_pGB->AddFilter(m_VMR, L"Video Mixing Renderer 9");
				logWrite(_T("[internal] VMR9 selected"));
			}
		}
	}

	//=== 2) CFFiSource 생성 + open_file + graph add ===
	HRESULT hr_src = S_OK;
	ffi::CFFiSource* pFFi = new ffi::CFFiSource(NULL, &hr_src);
	if (!pFFi || FAILED(hr_src))
	{
		logWrite(_T("[internal] CFFiSource ctor fail hr=0x%08x"), hr_src);
		if (pFFi) delete pFFi;
		return 0;
	}
	pFFi->AddRef();

	if (m_pending_internal_audio_track >= 0)
	{
		pFFi->decoder().set_initial_audio_track(m_pending_internal_audio_track);
		logWrite(_T("[internal] forcing audio track %d for open"), m_pending_internal_audio_track);
		m_pending_internal_audio_track = -1;
	}

	HRESULT hr = pFFi->open_file(sfile);
	if (FAILED(hr))
	{
		logWrite(_T("[internal] CFFiSource.open_file fail hr=0x%08x"), hr);
		pFFi->Release();
		return 0;
	}

	//HW 가속 미지원 codec — 기본은 LAV fallback (MPEG4/VC1/WMV 등은 SW keyframe walk 가 5-10초 freeze).
	//단 frame-independent codec (MJPEG 등) 은 seek freeze 없음 + LAV path 가 일부 컨테이너에서 비정상 EOS 유발
	//(예: MP4 in MJPEG + no audio) → internal SW decode 로 진행.
	if (!pFFi->decoder().has_hw_accel())
	{
		std::wstring codec = pFFi->decoder().video_codec_name();
		const bool safe_sw = (codec == L"MJPEG");	//필요 시 화이트리스트 확장.
		if (!safe_sw)
		{
			logWrite(_T("[internal] no HW accel for codec=%s — fallback to LAV path"), codec.c_str());
			pFFi->Release();
			m_pFFiSource = nullptr;
			return -1;   //caller 가 LAV path 진행 신호.
		}
		logWrite(_T("[internal] no HW accel for codec=%s — internal SW decode (frame-independent, freeze 무관)"), codec.c_str());
	}

	m_pFFiSource = pFFi;   //caching for later (duration / video size accessor).

	IBaseFilter* pBF = NULL;
	pFFi->NonDelegatingQueryInterface(IID_IBaseFilter, (void**)&pBF);
	if (!pBF)
	{
		pFFi->Release();
		m_pFFiSource = nullptr;
		return 0;
	}
	m_pGB->AddFilter(pBF, L"FFmpeg Internal Source");

	//=== 3) output pins → renderer connect (Render 가 자동 처리, video + audio 모두) ===
	IEnumPins* pEnumP = NULL;
	if (SUCCEEDED(pBF->EnumPins(&pEnumP)) && pEnumP)
	{
		IPin* pPin = NULL;
		int pidx = 0;
		while (pEnumP->Next(1, &pPin, NULL) == S_OK)
		{
			PIN_DIRECTION dir;
			if (SUCCEEDED(pPin->QueryDirection(&dir)) && dir == PINDIR_OUTPUT)
			{
				hr = m_pGB->Render(pPin);
				logWrite(_T("[internal] Render(pin[%d]) hr=0x%08x"), pidx, hr);
			}
			pPin->Release();
			++pidx;
		}
		pEnumP->Release();
	}
	pBF->Release();

	//=== 4) control 인터페이스 QI ===
	m_pGB->QueryInterface(IID_IMediaPosition, (void**)&m_pMP);
	m_pGB->QueryInterface(IID_IMediaSeeking,  (void**)&m_pMS);
	m_pGB->QueryInterface(IID_IMediaControl,  (void**)&m_pMC);

	//IMediaEventEx — graph event 통지 (EC_COMPLETE / EC_VIDEO_SIZE_CHANGED 등). PreTranslateMessage → HandleGraphEvent 가 사용.
	//LAV path 와 동일 — 누락 시 60fps 등 event 빈도 높은 미디어에서 HandleGraphEvent 의 m_pME 접근 = NULL assert.
	HRESULT hr_me = m_pGB->QueryInterface(IID_PPV_ARGS(&m_pME));
	if (m_pME != NULL && pParent != NULL)
		m_pME->SetNotifyWindow((OAHWND)pParent->m_hWnd, WM_GRAPHNOTIFY, 0);
	logWrite(_T("[internal] IMediaEventEx hr=0x%08x me=%p"), hr_me, (void*)m_pME);

	//=== 5) media info ===
	m_duration   = pFFi->decoder().duration_ms();
	m_video_size = CSize(pFFi->decoder().video_width(), pFFi->decoder().video_height());
	m_frame_rate = pFFi->decoder().frame_rate();
	logWrite(_T("[internal] open OK %dx%d fps=%.2f duration=%.0fms"),
		m_video_size.cx, m_video_size.cy, m_frame_rate, m_duration);

	//stream list — Endorphin2 의 open_media corruption check 통과용.
	m_video_stream.clear();
	m_video_stream.push_back(CMediaStream(L"Video", 0));
	m_video_stream_index = 0;

	m_audio_stream.clear();
	if (pFFi->decoder().has_audio())
	{
		int n = pFFi->decoder().audio_track_count();
		for (int i = 0; i < n; i++)
		{
			const std::wstring& nm = pFFi->decoder().audio_track_name(i);
			m_audio_stream.push_back(CMediaStream(nm.c_str(), i));
		}
		m_audio_stream_index = pFFi->decoder().audio_track_current();
		if (m_audio_stream_index < 0)
			m_audio_stream_index = 0;
	}

	//subtitle metadata enumeration — internal path 는 자막 디코딩/렌더링 미지원. 메뉴 표시용으로만
	//m_subtitle_stream 을 채워둠. 사용자가 트랙 선택하면 Endorphin2 가 close+reopen 으로 LAV path 전환.
	m_subtitle_stream.clear();
	{
		int n = pFFi->decoder().subtitle_track_count();
		for (int i = 0; i < n; i++)
		{
			const std::wstring& nm = pFFi->decoder().subtitle_track_name(i);
			m_subtitle_stream.push_back(CMediaStream(nm.c_str(), i));
		}
		m_subtitle_stream_index = -1;   //internal path 에선 어떤 트랙도 active 아님.
	}

	//SC Audio chain (Gain / Compressor / audio_sync) 삽입 — LAV path 와 동일 코드 재사용.
	//internal path 는 video time-scale filter 미적용 (CFFiVideoStream::FillBuffer 가 이미 rate scaling)
	//— setup_video_time_scale_filter 내부에서 m_pFFiSource 체크로 자동 skip.
	if (pFFi->decoder().has_audio())
	{
		setup_audio_filter_chain();
		logWrite(_T("[internal] SC Audio chain setup"));
	}

	//새 renderer 인스턴스의 surface position 설정 — OnSize 가 fire 안 되는 미디어 전환 (mkv→mp4 등) 케이스에서
	//MPCVR 의 SetWindowPosition / SetDestinationPosition 이 한 번도 안 호출되어 surface 표시 안 되는 결함 fix.
	//set_video_position 이 m_video_size 의 새 aspect 로 letterbox 재계산. 이전 rect 가 없으면 parent client rect 사용.
	if (pParent && pParent->GetSafeHwnd())
	{
		CRect r = m_last_video_position_rect;
		if (r.IsRectEmpty())
			pParent->GetClientRect(&r);
		set_video_position(r);
		logWrite(_T("[internal] set_video_position(%d,%d,%d,%d) after open"),
			r.left, r.top, r.right, r.bottom);
	}

	//graph 가 ref 보유 — 우리 소유권 해제.
	pFFi->Release();
	return 1;
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
			//LAV 의 가상 stream ("No subtitles" / "Forced Subtitles") 은 실제 자막 트랙 아님 — 메뉴에서 제외.
			//PotPlayer 등 메이저 player 도 이를 노출 안 함.
			CString name_check(pzsName ? pzsName : L"");
			if (name_check.Find(_T("No subtitles")) >= 0 ||
				name_check.Find(_T("Forced Subtitles")) >= 0)
			{
				logWrite(_T("[analyze] subtitle stream[%d] '%s' skipped (virtual)"), i, name_check.GetString());
			}
			else
			{
				m_subtitle_stream.push_back(CMediaStream(pzsName, i));
				if (dwFlags & (AMSTREAMSELECTINFO_ENABLED | AMSTREAMSELECTINFO_EXCLUSIVE))
					m_subtitle_stream_index = (int)m_subtitle_stream.size() - 1;
			}
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

		//MPCVR — graph Run 후에야 put_MessageDrain 이 succeed. parent 로 마우스 forward 해 영상 클릭/드래그가 main dlg 의
		//OnLButtonDown 등에 도달하도록 (창 이동 / 더블클릭 fullscreen 등 기존 동작 유지).
		if (m_use_mpcvr && m_VMR && m_pParent && m_pParent->GetSafeHwnd() &&
			(m_play_state == State_Running || m_play_state == State_Paused))
		{
			CComQIPtr<IVideoWindow> pVW(m_VMR);
			if (pVW)
				pVW->put_MessageDrain((OAHWND)m_pParent->m_hWnd);
		}
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
		//pnrcSource 를 full normalized rect 로 명시 — NULL 일 때 EVR 이 dst 변경에 smooth 보간 적용,
		//양쪽 다 명시하면 EVR 이 즉시 적용하는 것으로 알려짐 (PotPlayer/MPC 우회 트릭).
		MFVideoNormalizedRect src = { 0.0f, 0.0f, 1.0f, 1.0f };
		m_pVDC->SetVideoPosition(&src, &rc);
		m_pVDC->RepaintVideo();
	}
	else
	{
		//MPCVR / legacy IVideoWindow path. MPC-BE FGFilter::SetPosition 패턴:
		//IBasicVideo::SetDefaultSourcePosition + SetDestinationPosition 으로 video rect (MPCVR window 의 client 좌표),
		//IVideoWindow::SetWindowPosition 으로 window rect (parent 의 client 좌표). 둘 다 필요.
		if (m_use_mpcvr)
		{
			//Aspect-preserve 계산 — MPCVR 의 IBasicVideo 는 raw stretch 라 호출자가 letterbox/pillarbox sub-rect 를 직접 산출해야 함
			//(MPC-BE 의 SetPosition(RECT w, RECT v) 에서 caller 가 v 를 미리 계산해 넘기는 패턴).
			const int win_w = r.Width();
			const int win_h = r.Height();
			int vid_w = win_w, vid_h = win_h, vid_x = 0, vid_y = 0;
			if (m_video_size.cx > 0 && m_video_size.cy > 0 && win_w > 0 && win_h > 0)
			{
				const float src_ar = (float)m_video_size.cx / (float)m_video_size.cy;
				const float win_ar = (float)win_w / (float)win_h;
				if (win_ar > src_ar)
				{
					vid_h = win_h;
					vid_w = (int)(win_h * src_ar + 0.5f);
					vid_x = (win_w - vid_w) / 2;
				}
				else
				{
					vid_w = win_w;
					vid_h = (int)(win_w / src_ar + 0.5f);
					vid_y = (win_h - vid_h) / 2;
				}
			}

			//panscan 누적 state 적용 — base (vid_x, vid_y, vid_w, vid_h) 위에 normalized rect 변환.
			//mirror/flip (값 swap 으로 음수 width/height 가 되는 경우) 은 IBasicVideo 가 reject 가능 — Phase 2.
			int final_x = vid_x + (int)(m_panscan_left * vid_w + 0.5f);
			int final_y = vid_y + (int)(m_panscan_top * vid_h + 0.5f);
			int final_w = (int)((m_panscan_right - m_panscan_left) * vid_w + 0.5f);
			int final_h = (int)((m_panscan_bottom - m_panscan_top) * vid_h + 0.5f);

			CComQIPtr<IBasicVideo> pBV(m_VMR);
			if (pBV)
			{
				pBV->SetDefaultSourcePosition();
				pBV->SetDestinationPosition(final_x, final_y, final_w, final_h);
			}
			CComQIPtr<IVideoWindow> pVW(m_VMR);
			if (pVW)
				pVW->SetWindowPosition(r.left, r.top, win_w, win_h);

			//panscan refresh 위해 base rect 기억.
			m_last_video_position_rect = r;
		}
		else
		{
			CComQIPtr<IVideoWindow> pVW(m_pGB);
			if (pVW)
				pVW->SetWindowPosition(r.left, r.top, r.Width(), r.Height());
		}
	}

	//Paused 상태에서 resize 시 새 프레임이 안 들어와 stale 픽셀이 남음 → renderer 별 강제 redraw.
	//VMR9 paused: RepaintVideo / EVR paused: 위에서 이미 처리 / MPCVR paused: cmd_redraw via IExFilterConfig.
	//가드: hr==S_OK 만 통과 (VFW_S_STATE_INTERMEDIATE 차단) + state==State_Paused 한정.
	if (m_pMC)
	{
		OAFilterState state = State_Running;
		HRESULT hr = m_pMC->GetState(0, &state);
		if (hr == S_OK && state == State_Paused)
		{
			if (m_pVMRWC && m_pParent && m_pParent->GetSafeHwnd())
			{
				HDC hdc = ::GetDC(m_pParent->GetSafeHwnd());
				m_pVMRWC->RepaintVideo(m_pParent->GetSafeHwnd(), hdc);
				::ReleaseDC(m_pParent->GetSafeHwnd(), hdc);
			}
			else if (m_use_mpcvr && m_VMR)
			{
				CComQIPtr<IExFilterConfig> pCfg(m_VMR);
				if (pCfg)
					pCfg->Flt_SetBool("cmd_redraw", true);
			}
		}
	}
}

double CDShow::get_track_pos()
{
	if (!m_pGB || !m_pMP)
		return 0.0;

	//IMediaPosition::get_CurrentPosition 은 graph state 가 transition 중일 때 무한 block 가능.
	//IMediaControl::GetState(0, ...) 로 timeout=0 즉시 query. VFW_S_STATE_INTERMEDIATE 면
	//graph 가 transition 중이므로 query 안 하고 cached pos 반환 — UI thread freeze 회피.
	if (m_pMC)
	{
		OAFilterState fs = State_Stopped;
		HRESULT hr_s = m_pMC->GetState(0, &fs);
		if (hr_s == VFW_S_STATE_INTERMEDIATE)
			return m_last_track_pos_ms;
	}

	REFTIME pos;
	m_pMP->get_CurrentPosition(&pos);
	m_last_track_pos_ms = pos * 1000.0;
	return m_last_track_pos_ms;
}

void CDShow::set_track_pos(double pos, bool seek_to_keyframe)
{
	if (!m_pGB || !m_pMS)
		return;

	LARGE_INTEGER qpc_freq, qpc_t0, qpc_t1, qpc_t2, qpc_t3;
	::QueryPerformanceFrequency(&qpc_freq);
	::QueryPerformanceCounter(&qpc_t0);

	//[diag] graph clock + 현재 position 진단 — seek 후 graph clock 이 새 위치로 advance 되는 시점이 visible frame 의 proxy.
	REFERENCE_TIME pre_pos = 0;
	m_pMS->GetCurrentPosition(&pre_pos);

	HRESULT hr;
	{
		LONGLONG lPos = (LONGLONG)(pos * 10000.0);
		//AM_SEEKING_SeekToKeyFrame: LAV 는 internally 처리 (redundant) 하지만 WMVideo Decoder DMO 등
		//Microsoft DMO 는 keyframe snap 없으면 frame 생성 못 하는 경우 있음. drag 경로(false) 외엔 활성.
		//AM_SEEKING_NoFlush: graph 가 seek 시 internal buffer flush 안 함 → target 이 이미 디코드된 영역 안이면
		//재디코드 없이 즉시 hit. 영역 밖이면 무효이지만 무해 (LAV/EVR 는 NoFlush 정상 처리).
		DWORD flags = AM_SEEKING_AbsolutePositioning | AM_SEEKING_NoFlush;
		if (seek_to_keyframe)
			flags |= AM_SEEKING_SeekToKeyFrame;
		hr = m_pMS->SetPositions(&lPos, flags, NULL, AM_SEEKING_NoPositioning);
		::QueryPerformanceCounter(&qpc_t1);

		REFERENCE_TIME post_pos = 0;
		m_pMS->GetCurrentPosition(&post_pos);

		logWrite(_T("[seek/t0] SetPositions pos=%.0fms hr=0x%08x flags=0x%08x setpos_us=%lld pre_pos=%.0fms post_pos=%.0fms"),
			pos, hr, flags,
			(long long)((qpc_t1.QuadPart - qpc_t0.QuadPart) * 1000000LL / qpc_freq.QuadPart),
			(double)pre_pos / 10000.0, (double)post_pos / 10000.0);
	}

	//seek target cache — get_track_pos 가 INTERMEDIATE transition 중에 cache 반환 (B→A→B 깜빡임 회피).
	m_last_track_pos_ms = pos;

	//Post-seek refresh — paused 상태의 stale frame 제거 + WMV running 상태 DMO flush 강제.
	if (m_pMC)
	{
		OAFilterState fs;
		if (SUCCEEDED(m_pMC->GetState(0, &fs)))
		{
			if (fs == State_Paused)
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
				else if (m_use_mpcvr && m_VMR)
				{
					CComQIPtr<IExFilterConfig> pCfg(m_VMR);
					if (pCfg)
						pCfg->Flt_SetBool("cmd_redraw", true);
				}
			}
			else if (fs == State_Running)
			{
				::QueryPerformanceCounter(&qpc_t2);
				if (m_use_mpcvr && m_VMR)
				{
					CComQIPtr<IExFilterConfig> pCfg(m_VMR);
					if (pCfg)
						pCfg->Flt_SetBool("cmd_redraw", true);
				}
				else
				{
					m_pMC->Pause();
					OAFilterState fs_paused;
					m_pMC->GetState(100, &fs_paused);
					m_pMC->Run();
				}
				::QueryPerformanceCounter(&qpc_t3);

				//[diag] cmd_redraw / Pause-Run 후 graph clock 이 new pos 부근으로 advance 될 때까지 polling.
				//advance 가 빨리 시작되면 video 가 곧 표시됨. 길면 audio buffer wait 등으로 clock stuck.
				LONGLONG advance_us = -1;
				REFERENCE_TIME tgt = (REFERENCE_TIME)(pos * 10000.0);
				for (int i = 0; i < 50; i++)   //최대 500ms (10ms × 50)
				{
					REFERENCE_TIME cur = 0;
					if (FAILED(m_pMS->GetCurrentPosition(&cur)))
						break;
					if (cur >= tgt - 10000000)   //tgt -1s 범위 도달 = clock 이 새 위치로 advance 시작
					{
						LARGE_INTEGER now;
						::QueryPerformanceCounter(&now);
						advance_us = (now.QuadPart - qpc_t0.QuadPart) * 1000000LL / qpc_freq.QuadPart;
						break;
					}
					::Sleep(10);
				}

				logWrite(_T("[seek/t1] redraw_us=%lld total_advance_us=%lld (T0→clock@target)"),
					(long long)((qpc_t3.QuadPart - qpc_t2.QuadPart) * 1000000LL / qpc_freq.QuadPart),
					(long long)advance_us);
			}
		}
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

	//set_track_pos 가 SetPositions(absolute) + m_last_track_pos_ms cache 갱신 + paused 분기 RepaintVideo /
	//MPCVR cmd_redraw + WMV DMO flush 까지 위임. legacy put_CurrentPosition 만 호출하던 시점 paused 상태에서
	//IMediaPosition graph 가 INTERMEDIATE state 로 머물면서 get_track_pos 가 stale cache 반환 → 컨트롤바
	//시각 stuck, 자막 매칭 실패 누적 회귀 (2026-05-12 좌우 화살표 회귀 fix).
	//base 위치는 get_track_pos() 사용 — INTERMEDIATE state 도 cache 로 처리하므로 누적 오차 없음.
	double new_pos_ms = get_track_pos() + (forward ? interval : -interval) * 1000.0;
	if (new_pos_ms < 0)
		new_pos_ms = 0;
	set_track_pos(new_pos_ms);

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

	//forward step: IVideoFrameStep (MPC-VR / EVR 지원) — graph clock 진행 없이 *다음 sample 1 개* 표시.
	//seek-based step 의 부작용 (sparse keyframe 미디어에서 av_seek_frame BACKWARD 가 forward fallback → 수초 jump) 회피.
	if (forward)
	{
		CComQIPtr<IVideoFrameStep> pFrameStep(m_pGB);
		if (pFrameStep && pFrameStep->CanStep(0, NULL) == S_OK)
		{
			HRESULT hr = pFrameStep->Step(1, NULL);
			if (SUCCEEDED(hr))
				return;
		}
	}

	//backward step 또는 IVideoFrameStep 미지원: seek-based fallback.
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
	{
		logWrite(_T("[sub_sel] no splitter — skip (internal path or pre-load)"));
		return;
	}

	CComQIPtr<IAMStreamSelect> pStreamSelect(m_pSplitter);
	if (!pStreamSelect)
	{
		logWrite(_T("[sub_sel] splitter has no IAMStreamSelect — skip"));
		return;
	}

	//index < 0 = 임베디드 자막 stream 전체 비활성화 (외부 자막 우선용).
	//splitter 의 default active stream 도 disable 해야 하므로 m_subtitle_stream_index 가 -1 이어도 전체 loop.
	if (index < 0)
	{
		for (size_t k = 0; k < m_subtitle_stream.size(); k++)
		{
			HRESULT hr = pStreamSelect->Enable(m_subtitle_stream[k].get_index(), 0);
			logWrite(_T("[sub_sel] disable subtitle[%zu] stream_idx=%d hr=0x%08x"),
				k, m_subtitle_stream[k].get_index(), hr);
		}
		m_subtitle_stream_index = -1;
		return;
	}
	if (index >= (int)m_subtitle_stream.size())
	{
		logWrite(_T("[sub_sel] index=%d out of range (size=%zu)"), index, m_subtitle_stream.size());
		return;
	}

	HRESULT hr = pStreamSelect->Enable(m_subtitle_stream[index].get_index(), AMSTREAMSELECTENABLE_ENABLE);
	logWrite(_T("[sub_sel] enable subtitle[%d] stream_idx=%d hr=0x%08x"),
		index, m_subtitle_stream[index].get_index(), hr);
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
	//user-visible rate — set_playback_rate 가 store 한 값. graph m_pMS->GetRate 무관.
	//LAV path 에서 m_pMS->SetRate 안 호출 (LAV chipmunk 회피) → graph rate 항상 1.0.
	//그러나 사용자에게는 우리가 atempo 로 적용한 rate 가 진짜 재생속도 — 그 값 반환.
	return m_user_playback_rate.load();
}

void CDShow::set_playback_rate(double rate)
{
	//rate clamp [0.5, 2.0] — atempo 의 valid range. 그 외는 audio/video 의 *동일 rate 동기 보장 X* (atempo
	//hard clamp 0.5/2.0 → audio 가 그 한계 고정 + video 는 그 외 rate scale → sync 어긋남).
	//사용자 명시 — 0.4 이하 시 audio 가 0.5 와 동일 재생되어 비교적 일치 깨지므로 chord clamp 도입.
	if (rate < 0.5) rate = 0.5;
	if (rate > 2.0) rate = 2.0;

	//user-visible rate 갱신 — get_playback_rate 가 이 값 반환 (graph SetRate 안 호출하는 LAV path 도 OSD 정상).
	m_user_playback_rate.store(rate);

	if (!m_pGB)
		return;

	//atempo time-stretch path — chain teardown / graph SetRate 모두 불필요.
	//  (1) LAV path: CSCAudioTimeStretch 가 chain 에 들어가 있어 set_rate(rate) 만 호출하면
	//      audio PCM 양 1/rate 줄어 DSound 가 빨리 처리 → graph clock 가속 → video 도 따라.
	//      pitch 유지 (atempo). graph 의 IMediaSeeking::SetRate 는 호출 안 함 — LAV File Source 가
	//      sample timestamp scale + audio data 양 줄임 동작 (chipmunk) 을 막아야 함.
	//  (2) internal path: CFFiSource->set_playback_rate(rate) — CFFiSeeking::ChangeRate 통해
	//      CFFiAudioStream::FillBuffer 의 atempo 가 처리. video FillBuffer 의 rtStart scaling 도 함께.
	//  (3) fallback: 둘 다 없으면 graph SetRate 직접 (구버전 호환).
	HRESULT hr = S_OK;
	if (m_pAudioTimeStretchFilter)
	{
		//LAV path — audio TimeStretch + video TimeScale 둘 다 적용. 동일 rate 보장.
		//graph SetRate 안 호출 — LAV 가 받으면 audio/video 의 timestamp/data 양 변경 → 우리 filter 와 이중 적용 위험
		//(rate < 1.0 시 video 가 1/rate^2 로 너무 느림 / audio 가 chipmunk 가능).
		((CSCAudioTimeStretch*)m_pAudioTimeStretchFilter)->set_rate(rate);
		if (m_pVideoTimeScaleFilter)
			((CSCVideoTimeScale*)m_pVideoTimeScaleFilter)->set_rate(rate);
		logWrite(_T("[playback_rate] LAV path → CSCAudioTimeStretch + CSCVideoTimeScale set_rate(%.3f)"), rate);
	}
	else if (m_pFFiSource)
	{
		((ffi::CFFiSource*)m_pFFiSource)->set_playback_rate(rate);
		//internal path 의 ChangeRate 가 호출되도록 graph SetRate 도 트리거 (CFFiSeeking 의 SetRate 라우팅).
		if (m_pMS)
			hr = m_pMS->SetRate(rate);
		logWrite(_T("[playback_rate] internal path → CFFiSource->set_playback_rate(%.3f) + graph SetRate hr=0x%08x"), rate, hr);
	}
	else
	{
		if (m_pMS)
			hr = m_pMS->SetRate(rate);
		if (FAILED(hr) && m_pMP)
			hr = m_pMP->put_Rate(rate);
		logWrite(_T("[playback_rate] fallback graph SetRate %.3f hr=0x%08x"), rate, hr);
	}

	//self-seek — chain 전체 flush (DSound / MPC-VR queue 의 old timestamp sample 제거) + 새 NewSegment 발행
	//→ 우리 두 filter 가 새 rate 의 timeline 으로 재시작. 사용자 수동 트랙 이동 시 sync 맞춰지던 효과의 자동화.
	//부작용: video 1~2 frame 끊김 + audio 짧은 silence (수동 seek 와 동일). sync 정확 보장과 trade-off.
	flush_audio_buffer();
}

#include <atlimage.h>
#include "../../SCGdiplusBitmap.h"

//현재 video frame 의 packed DIB (BITMAPINFOHEADER + pixel rows) 를 얻는다.
//성공 시 *out_dib 는 CoTaskMemAlloc 된 메모리 — 호출자가 CoTaskMemFree 책임.
//paused 상태에서 GetCurrentImage 가 frame 을 blank 시키는 VMR9 동작을 step_frame 으로 보정.
//EVR 은 BMI 와 pixel bits 를 별도 인자로 돌려주므로 VMR9 packed 포맷에 맞춰 합쳐서 반환.
//MPCVR 은 IVMRWindowlessControl9 / IMFVideoDisplayControl 둘 다 노출 안 함 — IBasicVideo::GetCurrentImage 사용.
static bool get_current_image_dib(IVMRWindowlessControl9* pVMRWC, IMFVideoDisplayControl* pVDC, IBaseFilter* pMPCVR, BYTE** out_dib)
{
	*out_dib = NULL;

	if (pVMRWC)
	{
		HRESULT hr = pVMRWC->GetCurrentImage(out_dib);
		return SUCCEEDED(hr) && *out_dib;
	}

	if (pVDC)
	{
		BITMAPINFOHEADER bih = {};
		bih.biSize = sizeof(BITMAPINFOHEADER);
		BYTE* dib = NULL;
		DWORD cb = 0;
		LONGLONG ts = 0;
		HRESULT hr = pVDC->GetCurrentImage(&bih, &dib, &cb, &ts);
		if (FAILED(hr) || !dib)
			return false;

		BYTE* combined = (BYTE*)CoTaskMemAlloc(sizeof(BITMAPINFOHEADER) + cb);
		if (!combined)
		{
			CoTaskMemFree(dib);
			return false;
		}
		memcpy(combined, &bih, sizeof(BITMAPINFOHEADER));
		memcpy(combined + sizeof(BITMAPINFOHEADER), dib, cb);
		CoTaskMemFree(dib);
		*out_dib = combined;
		return true;
	}

	if (pMPCVR)
	{
		CComQIPtr<IBasicVideo> pBV(pMPCVR);
		if (!pBV)
			return false;

		LONG cb = 0;
		HRESULT hr = pBV->GetCurrentImage(&cb, NULL);
		if (FAILED(hr) || cb <= 0)
			return false;

		BYTE* dib = (BYTE*)CoTaskMemAlloc(cb);
		if (!dib)
			return false;

		hr = pBV->GetCurrentImage(&cb, (LONG*)dib);
		if (FAILED(hr))
		{
			CoTaskMemFree(dib);
			return false;
		}
		*out_dib = dib;
		return true;
	}

	return false;
}

//EVR GetCurrentImage 는 display rect (letterbox/pillarbox bars 포함) 으로 반환.
//Bars 를 crop 하고 native 사이즈로 resize 해 window 와 무관한 일관 출력.
static void normalize_captured_to_native(CSCGdiplusBitmap& bmp, int native_w, int native_h)
{
	if (!bmp.m_pBitmap || native_w <= 0 || native_h <= 0)
		return;

	const int cap_w = bmp.m_pBitmap->GetWidth();
	const int cap_h = bmp.m_pBitmap->GetHeight();
	if (cap_w <= 0 || cap_h <= 0)
		return;

	const float native_ar = (float)native_w / (float)native_h;
	const float cap_ar = (float)cap_w / (float)cap_h;

	if (fabs(cap_ar - native_ar) > 0.005f)
	{
		CRect r;
		if (cap_ar > native_ar)
		{
			//pillarbox — 좌/우 bars
			int video_w = (int)(cap_h * native_ar + 0.5f);
			r.SetRect((cap_w - video_w) / 2, 0, (cap_w - video_w) / 2 + video_w, cap_h);
		}
		else
		{
			//letterbox — 상/하 bars
			int video_h = (int)(cap_w / native_ar + 0.5f);
			r.SetRect(0, (cap_h - video_h) / 2, cap_w, (cap_h - video_h) / 2 + video_h);
		}
		bmp.sub_image(r);
	}

	if (bmp.m_pBitmap->GetWidth() != (UINT)native_w || bmp.m_pBitmap->GetHeight() != (UINT)native_h)
		bmp.resize(native_w, native_h);
}

bool CDShow::capture_frame(CString sfile)
{
	if (!is_media_opened() || !m_VMR || (!m_pVMRWC && !m_pVDC && !m_use_mpcvr))
		return false;

	BYTE* lpDib = NULL;
	IBaseFilter* pMPCVR = m_use_mpcvr ? m_VMR : NULL;
	if (!get_current_image_dib(m_pVMRWC, m_pVDC, pMPCVR, &lpDib))
		return false;

	BITMAPINFOHEADER* pBMI = (BITMAPINFOHEADER*)lpDib;
	LPBYTE pixels = (LPBYTE)pBMI + pBMI->biSize;

	BITMAPINFO bmInfo = {};
	bmInfo.bmiHeader = *pBMI;

	CSCGdiplusBitmap bmp;
	bool ok = bmp.create_from_dib(&bmInfo, pixels);
	if (ok)
		normalize_captured_to_native(bmp, m_video_size.cx, m_video_size.cy);
	bool saved = ok && bmp.save(sfile);

	if (get_play_state() == State_Paused)
		step_frame(true);

	CoTaskMemFree(lpDib);
	return saved;
}

bool CDShow::capture_frame(CSCGdiplusBitmap& out)
{
	if (!is_media_opened() || !m_VMR || (!m_pVMRWC && !m_pVDC && !m_use_mpcvr))
		return false;

	BYTE* lpDib = NULL;
	IBaseFilter* pMPCVR = m_use_mpcvr ? m_VMR : NULL;
	if (!get_current_image_dib(m_pVMRWC, m_pVDC, pMPCVR, &lpDib))
		return false;

	BITMAPINFOHEADER* pBMI = (BITMAPINFOHEADER*)lpDib;
	LPBYTE pixels = (LPBYTE)pBMI + pBMI->biSize;

	BITMAPINFO bmInfo = {};
	bmInfo.bmiHeader = *pBMI;

	bool ok = out.create_from_dib(&bmInfo, pixels);
	if (ok)
		normalize_captured_to_native(out, m_video_size.cx, m_video_size.cy);

	if (get_play_state() == State_Paused)
		step_frame(true);

	CoTaskMemFree(lpDib);
	return ok;
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
	//VMR9 path — IVMRMixerControl9::SetOutputRect 사용.
	if (m_pVMRMC)
	{
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
		return;
	}

	//EVR path — IMFVideoMixerControl::SetStreamOutputRect 사용. VMR9NormalizedRect 와 같은 normalized 좌표 모델.
	if (m_pMixerControl)
	{
		MFVideoNormalizedRect rc;
		m_pMixerControl->GetStreamOutputRect(dwStreamID, &rc);

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

		m_pMixerControl->SetStreamOutputRect(dwStreamID, &rc);
		return;
	}

	//MPCVR path — m_panscan_* 누적 state 갱신 후 set_video_position 재호출해 IBasicVideo::SetDestinationPosition 적용.
	//mirror/flip 의 swap (값 좌우/상하 뒤집기) 은 음수 width/height 유발해 IBasicVideo 가 reject — Phase 2 (IExFilterConfig flip/rotation 우회).
	if (m_use_mpcvr)
	{
		if (mode == pan_scan_origin)
		{
			m_panscan_left = 0.0f; m_panscan_top = 0.0f;
			m_panscan_right = 1.0f; m_panscan_bottom = 1.0f;
		}
		else if (mode == pan_scan_size)
		{
			m_panscan_left -= dx; m_panscan_right += dx;
			m_panscan_top -= dy; m_panscan_bottom += dy;
		}
		else if (mode == pan_scan_move)
		{
			m_panscan_left += dx; m_panscan_right += dx;
			m_panscan_top += dy; m_panscan_bottom += dy;
		}
		else if (mode == pan_scan_mirror || mode == pan_scan_flip)
		{
			if (mode == pan_scan_mirror)
				m_mirror = !m_mirror;
			else
				m_flip = !m_flip;

			//MPCVR 의 IExFilterConfig 는 "rotation" (0/90/180/270) + "flip" (bool, 가로 반전) 만 제공.
			//VMR9/EVR 경로의 m_mirror (좌우) / m_flip (상하) 두 독립 토글을 MPCVR 모델로 매핑:
			//  m_mirror=0,m_flip=0 → rot=0,   flip=0  (정상)
			//  m_mirror=1,m_flip=0 → rot=0,   flip=1  (좌우만)
			//  m_mirror=0,m_flip=1 → rot=180, flip=1  (상하만 = 180회전 + 가로반전 = 결과적으로 상하 뒤집기)
			//  m_mirror=1,m_flip=1 → rot=180, flip=0  (180회전 = 좌우+상하 동시)
			int mpcvr_rot = m_flip ? 180 : 0;
			bool mpcvr_flip = (m_mirror != m_flip);

			CComQIPtr<IExFilterConfig> pCfg(m_VMR);
			if (pCfg)
			{
				pCfg->Flt_SetInt("rotation", mpcvr_rot);
				pCfg->Flt_SetBool("flip", mpcvr_flip);
				pCfg->Flt_SetBool("cmd_redraw", true);
			}
			return;
		}

		//마지막 set_video_position rect 로 재호출 — panscan 적용된 final dest 로 SetDestinationPosition.
		if (!m_last_video_position_rect.IsRectEmpty())
			set_video_position(m_last_video_position_rect);
	}
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
	else if (m_pVDC)
	{
		m_pVDC->RepaintVideo();
	}
}

//target index (contrast=0, bright=1, hue=2, saturation=3) → DXVA2 ProcAmp flag.
static DWORD target_to_dxva2(int target)
{
	switch (target)
	{
	case 0: return DXVA2_ProcAmp_Contrast;
	case 1: return DXVA2_ProcAmp_Brightness;
	case 2: return DXVA2_ProcAmp_Hue;
	case 3: return DXVA2_ProcAmp_Saturation;
	}
	return 0;
}

//graph build 후에야 EVR mixer service 가 안정 — open_media 시점 QI 가 fail 했어도 사용 시 lazy retry.
static bool ensure_evr_proc_amp(IBaseFilter* pVMR, CComPtr<IMFVideoProcessor>& pVP)
{
	if (pVP) return true;
	if (!pVMR) return false;
	CComQIPtr<IMFGetService> pGS(pVMR);
	if (!pGS) return false;
	return SUCCEEDED(pGS->GetService(MR_VIDEO_MIXER_SERVICE, IID_PPV_ARGS(&pVP))) && pVP;
}

int CDShow::adjust_video(int dwStreamID, int target, bool up)
{
	//MPCVR 분기 — IMFVideoProcessor (DXVA2 ProcAmp) 로 제어. IExFilterConfig 의 "video_*" 키는
	//Aleksoid MPCVR 에서 E_INVALIDARG (검증됨). m_pVP 캐싱 시 close_media 시 access violation —
	//매 호출마다 lazy QI + 로컬 변수로만 사용.
	if (m_use_mpcvr && m_VMR)
	{
		CComPtr<IMFVideoProcessor> pVP;
		CComQIPtr<IMFGetService> pGS(m_VMR);
		if (pGS)
			pGS->GetService(MR_VIDEO_MIXER_SERVICE, IID_PPV_ARGS(&pVP));

		if (pVP)
		{
			const DWORD all_flags = DXVA2_ProcAmp_Brightness | DXVA2_ProcAmp_Contrast |
			                        DXVA2_ProcAmp_Hue | DXVA2_ProcAmp_Saturation;

			if (target == -1)
			{
				DXVA2_ProcAmpValues vals = {};
				if (FAILED(pVP->GetProcAmpValues(all_flags, &vals)))
					return -1;
				for (int i = 0; i < 4; ++i)
				{
					DWORD f = target_to_dxva2(i);
					DXVA2_ValueRange r = {};
					if (FAILED(pVP->GetProcAmpRange(f, &r)))
						continue;
					switch (f)
					{
					case DXVA2_ProcAmp_Brightness: vals.Brightness = r.DefaultValue; break;
					case DXVA2_ProcAmp_Contrast:   vals.Contrast   = r.DefaultValue; break;
					case DXVA2_ProcAmp_Hue:        vals.Hue        = r.DefaultValue; break;
					case DXVA2_ProcAmp_Saturation: vals.Saturation = r.DefaultValue; break;
					}
				}
				if (FAILED(pVP->SetProcAmpValues(all_flags, &vals)))
					return -1;
				repaint_for_procamp_change();
				return 0;
			}

			if (target < 0 || target > 3)
				return -1;

			DWORD flag = target_to_dxva2(target);
			DXVA2_ValueRange range = {};
			if (FAILED(pVP->GetProcAmpRange(flag, &range)))
				return -1;

			DXVA2_ProcAmpValues vals = {};
			if (FAILED(pVP->GetProcAmpValues(all_flags, &vals)))
				return -1;

			const float minv = DXVA2FixedToFloat(range.MinValue);
			const float maxv = DXVA2FixedToFloat(range.MaxValue);
			DXVA2_Fixed32* pVal = NULL;
			switch (flag)
			{
			case DXVA2_ProcAmp_Brightness: pVal = &vals.Brightness; break;
			case DXVA2_ProcAmp_Contrast:   pVal = &vals.Contrast; break;
			case DXVA2_ProcAmp_Hue:        pVal = &vals.Hue; break;
			case DXVA2_ProcAmp_Saturation: pVal = &vals.Saturation; break;
			}
			if (!pVal)
				return -1;

			float current = DXVA2FixedToFloat(*pVal);
			const float step = (maxv - minv) / 100.0f;
			current += up ? step : -step;
			if (current < minv) current = minv;
			if (current > maxv) current = maxv;
			*pVal = DXVA2FloatToFixed(current);

			if (FAILED(pVP->SetProcAmpValues(flag, &vals)))
				return -1;

			repaint_for_procamp_change();
			return (int)(((current - minv) / (maxv - minv)) * 100.0f + 0.5);
		}
	}

	//EVR 분기 — m_pVMRMC NULL 일 때 IMFVideoProcessor 로 ProcAmp 제어.
	//MPCVR 는 GetService(VP) 가 succeed 하지만 lifecycle 차이로 close_media 시 m_pVP release 가 access violation — skip.
	if (!m_pVMRMC && !m_use_mpcvr && ensure_evr_proc_amp(m_VMR, m_pVP))
	{
		const DWORD all_flags = DXVA2_ProcAmp_Brightness | DXVA2_ProcAmp_Contrast |
		                        DXVA2_ProcAmp_Hue | DXVA2_ProcAmp_Saturation;

		if (target == -1)
		{
			DXVA2_ProcAmpValues vals = {};
			if (FAILED(m_pVP->GetProcAmpValues(all_flags, &vals)))
				return -1;
			for (int i = 0; i < 4; ++i)
			{
				DWORD f = target_to_dxva2(i);
				DXVA2_ValueRange r = {};
				if (FAILED(m_pVP->GetProcAmpRange(f, &r)))
					continue;
				switch (f)
				{
				case DXVA2_ProcAmp_Brightness: vals.Brightness = r.DefaultValue; break;
				case DXVA2_ProcAmp_Contrast:   vals.Contrast   = r.DefaultValue; break;
				case DXVA2_ProcAmp_Hue:        vals.Hue        = r.DefaultValue; break;
				case DXVA2_ProcAmp_Saturation: vals.Saturation = r.DefaultValue; break;
				}
			}
			if (FAILED(m_pVP->SetProcAmpValues(all_flags, &vals)))
				return -1;
			repaint_for_procamp_change();
			return 0;
		}

		if (target < 0 || target > 3)
			return -1;

		DWORD flag = target_to_dxva2(target);
		DXVA2_ValueRange range = {};
		if (FAILED(m_pVP->GetProcAmpRange(flag, &range)))
			return -1;

		DXVA2_ProcAmpValues vals = {};
		if (FAILED(m_pVP->GetProcAmpValues(all_flags, &vals)))
			return -1;

		const float minv = DXVA2FixedToFloat(range.MinValue);
		const float maxv = DXVA2FixedToFloat(range.MaxValue);
		DXVA2_Fixed32* pVal = NULL;
		switch (flag)
		{
		case DXVA2_ProcAmp_Brightness: pVal = &vals.Brightness; break;
		case DXVA2_ProcAmp_Contrast:   pVal = &vals.Contrast; break;
		case DXVA2_ProcAmp_Hue:        pVal = &vals.Hue; break;
		case DXVA2_ProcAmp_Saturation: pVal = &vals.Saturation; break;
		}
		if (!pVal)
			return -1;

		float current = DXVA2FixedToFloat(*pVal);
		const float step = (maxv - minv) / 100.0f;
		current += up ? step : -step;
		if (current < minv) current = minv;
		if (current > maxv) current = maxv;
		*pVal = DXVA2FloatToFixed(current);

		if (FAILED(m_pVP->SetProcAmpValues(flag, &vals)))
			return -1;

		repaint_for_procamp_change();
		return (int)(((current - minv) / (maxv - minv)) * 100.0f + 0.5f);
	}

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
	if (target < 0 || target > 3)
		return -1;
	if (percent < 0)   percent = 0;
	if (percent > 100) percent = 100;

	//MPCVR 분기 — adjust_video 와 동일하게 IMFVideoProcessor lazy QI + 로컬 사용.
	if (m_use_mpcvr && m_VMR)
	{
		CComPtr<IMFVideoProcessor> pVP;
		CComQIPtr<IMFGetService> pGS(m_VMR);
		if (pGS)
			pGS->GetService(MR_VIDEO_MIXER_SERVICE, IID_PPV_ARGS(&pVP));
		if (pVP)
		{
			DWORD flag = target_to_dxva2(target);
			DXVA2_ValueRange range = {};
			if (FAILED(pVP->GetProcAmpRange(flag, &range)))
				return -1;

			const DWORD all_flags = DXVA2_ProcAmp_Brightness | DXVA2_ProcAmp_Contrast |
			                        DXVA2_ProcAmp_Hue | DXVA2_ProcAmp_Saturation;
			DXVA2_ProcAmpValues vals = {};
			if (FAILED(pVP->GetProcAmpValues(all_flags, &vals)))
				return -1;

			const float minv = DXVA2FixedToFloat(range.MinValue);
			const float maxv = DXVA2FixedToFloat(range.MaxValue);
			const float val = minv + (maxv - minv) * (percent / 100.0f);
			const DXVA2_Fixed32 fixed = DXVA2FloatToFixed(val);
			switch (flag)
			{
			case DXVA2_ProcAmp_Brightness: vals.Brightness = fixed; break;
			case DXVA2_ProcAmp_Contrast:   vals.Contrast   = fixed; break;
			case DXVA2_ProcAmp_Hue:        vals.Hue        = fixed; break;
			case DXVA2_ProcAmp_Saturation: vals.Saturation = fixed; break;
			}
			if (FAILED(pVP->SetProcAmpValues(flag, &vals)))
				return -1;
			repaint_for_procamp_change();
			return percent;
		}
	}

	//EVR 분기 — MPCVR 모드는 lifecycle 충돌로 m_pVP skip (위 adjust_video 와 동일).
	if (!m_pVMRMC && !m_use_mpcvr && ensure_evr_proc_amp(m_VMR, m_pVP))
	{
		DWORD flag = target_to_dxva2(target);
		DXVA2_ValueRange range = {};
		if (FAILED(m_pVP->GetProcAmpRange(flag, &range)))
			return -1;

		const DWORD all_flags = DXVA2_ProcAmp_Brightness | DXVA2_ProcAmp_Contrast |
		                        DXVA2_ProcAmp_Hue | DXVA2_ProcAmp_Saturation;
		DXVA2_ProcAmpValues vals = {};
		if (FAILED(m_pVP->GetProcAmpValues(all_flags, &vals)))
			return -1;

		const float minv = DXVA2FixedToFloat(range.MinValue);
		const float maxv = DXVA2FixedToFloat(range.MaxValue);
		const float val = minv + (maxv - minv) * (percent / 100.0f);
		const DXVA2_Fixed32 fixed = DXVA2FloatToFixed(val);
		switch (flag)
		{
		case DXVA2_ProcAmp_Brightness: vals.Brightness = fixed; break;
		case DXVA2_ProcAmp_Contrast:   vals.Contrast   = fixed; break;
		case DXVA2_ProcAmp_Hue:        vals.Hue        = fixed; break;
		case DXVA2_ProcAmp_Saturation: vals.Saturation = fixed; break;
		}
		if (FAILED(m_pVP->SetProcAmpValues(flag, &vals)))
			return -1;
		repaint_for_procamp_change();
		return percent;
	}

	if (!m_pVMRMC)
		return -1;

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

	//Volume 매핑 — 헤더 코멘트 의도 ("80% = 0 dB, 100% = +18 dB") 그대로 복원 + 인간 청각의 log 특성 반영:
	//   v=0       : mute
	//   1≤v<80   : db = 33 * log10(v / 80)        (순수 log 곡선, v=80 에서 0 dB unity)
	//  80≤v≤100  : db = (v - 80) * 18 / 20        (선형 boost 0 → +18 dB)
	//
	//   v=1  : -62.8 dB   거의 mute
	//   v=2  : -52.9 dB   미세 (사용자 요구 "거의 안 들림")
	//   v=5  : -39.7 dB
	//   v=10 : -29.8 dB
	//   v=20 : -19.9 dB
	//   v=30 : -14.1 dB   (이전 -1.1 dB 가 "꽤 크다" 였던 영역)
	//   v=40 :  -9.95 dB
	//   v=50 :  -6.7 dB
	//   v=70 :  -1.9 dB
	//   v=80 :   0  dB   unity
	//   v=100:  +18 dB   max boost
	//
	//IBasicAudio 는 -100~0 dB 범위만 attenuation 가능. boost (>0 dB) 는 compressor makeup_db 로 처리.
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
		if (v < 80)
			total_db = 33.0 * std::log10((double)v / 80.0);		//attenuation 영역, v=80 에서 0 dB
		else
			total_db = (double)(v - 80) * 18.0 / 20.0;			//boost 영역, v=100 에서 +18 dB

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
			//EC_COMPLETE 의 정확한 trigger 진단 — current position / duration / lParam (HRESULT) / sender filter.
			LONGLONG cur_ns = 0, dur_ns = 0;
			CComQIPtr<IMediaSeeking> pMS(m_pGB);
			if (pMS)
			{
				pMS->GetCurrentPosition(&cur_ns);
				pMS->GetDuration(&dur_ns);
			}
			CString sender_name;
			if (lParam2)
			{
				IBaseFilter* pSender = (IBaseFilter*)lParam2;
				FILTER_INFO fi = { 0 };
				if (SUCCEEDED(pSender->QueryFilterInfo(&fi)))
				{
					sender_name = fi.achName;
					if (fi.pGraph) fi.pGraph->Release();
				}
			}
			logWrite(_T("[EC_COMPLETE] hr=0x%08lX cur=%lldms dur=%lldms sender='%s'"),
				(LONG)lParam1, cur_ns / 10000, dur_ns / 10000, sender_name.GetString());
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

	m_crColorKey = RGB(0, 31, 1);

	for (int i = 0; i < 2; i++)
	{
		m_pMemDC[i] = new CDC();
		m_pMemDC[i]->CreateCompatibleDC(m_pParentDC);

		m_pBitmap[i] = new CBitmap();
		m_pBitmap[i]->CreateCompatibleBitmap(m_pParentDC, m_video_size.cx * fZoom, m_video_size.cy * fZoom);
		m_pMemDC[i]->SelectObject(m_pBitmap[i]);
		m_pMemDC[i]->SetBkMode(TRANSPARENT);

		//VMR9 mixer 가 alpha bitmap 을 colorkey 기반 투명도로 합성. 새로 생성한 비트맵의 픽셀이
		//undefined (보통 black) 상태면 mixer 는 이를 불투명으로 보고 영상 위 덮어 영상이 안 보임.
		//전체를 colorkey 색으로 채워 mixer 가 fully transparent 로 인식하도록 초기화.
		m_pMemDC[i]->FillSolidRect(0, 0, m_video_size.cx * fZoom, m_video_size.cy * fZoom, m_crColorKey);
	}

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

	//OSD 텍스트와 자막 모두 비어있으면 alpha bitmap 을 그릴 일이 없음 → SetAlphaBitmap 호출 자체 skip.
	//일부 환경 (특정 GPU/드라이버) 에서 graph Running 중 SetAlphaBitmap 호출이 VMR9 mixer 내부 lock 으로
	//무한 block 되는 현상 (UI thread freeze, CPU 0~5%) 회피.
	//Endorphin2 는 OSD/자막을 모두 별도 CSCShapeDlg popup 으로 그리므로 dshow 의 alpha bitmap path 는 미사용.
	if (m_osd_text.IsEmpty() && !(m_show_subtitle && m_cur_subtitle.is_valid()))
		return S_OK;

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

	Clamp(m_subCfg.font_size, 2, 100);

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
	m_nAudioFilter = 0;

	if (!pEnumCat)
		return S_FALSE;

	while (true)
	{
		CComPtr<IMoniker> pMoniker;
		ULONG cFetched = 0;
		if (pEnumCat->Next(1, &pMoniker, &cFetched) != S_OK)
			break;

		CComPtr<IPropertyBag> pPropBag;
		HRESULT hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPropBag);
		if (FAILED(hr))
			continue;

		VARIANT varName = {};
		varName.vt = VT_BSTR;
		hr = pPropBag->Read(L"FriendlyName", &varName, 0);
		if (FAILED(hr))
		{
			VariantClear(&varName);
			continue;
		}
		CString str(varName.bstrVal);
		VariantClear(&varName);

		VARIANT varFilterClsid = {};
		varFilterClsid.vt = VT_BSTR;
		hr = pPropBag->Read(L"CLSID", &varFilterClsid, 0);
		if (SUCCEEDED(hr))
		{
			CLSID clsidFilter;
			if (CLSIDFromString(varFilterClsid.bstrVal, &clsidFilter) == S_OK)
				m_sAudioFilter[m_nAudioFilter++] = str;
		}
		VariantClear(&varFilterClsid);
	}

	return S_OK;
}

HRESULT CDShow::FindFilter(CString compFiterName, REFCLSID clsID, IBaseFilter **ppSrcFilter)
{
	if (!ppSrcFilter)
		return E_POINTER;

	HRESULT hr = S_OK;

	CComPtr<ICreateDevEnum> pDevEnum;
	hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC,
		IID_ICreateDevEnum, (void**)&pDevEnum);
	if (FAILED(hr))
		return hr;

	CComPtr<IEnumMoniker> pClassEnum;
	hr = pDevEnum->CreateClassEnumerator(clsID, &pClassEnum, 0);
	if (FAILED(hr))
		return hr;
	if (pClassEnum == NULL)
		return E_FAIL;

	//Loop 안의 pMoniker / pPropBag / varName 은 매 iteration 끝/continue 시 자동 정리되어야 함.
	//CComPtr 로 감싸 자동 release. VARIANT 는 VariantClear 로 일관 정리.
	while (true)
	{
		CComPtr<IMoniker> pMoniker;
		ULONG cFetched = 0;
		if (pClassEnum->Next(1, &pMoniker, &cFetched) != S_OK)
			break;

		CComPtr<IPropertyBag> pPropBag;
		hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPropBag);
		if (FAILED(hr))
			continue;

		VARIANT varName = {};
		varName.vt = VT_BSTR;
		hr = pPropBag->Read(L"FriendlyName", &varName, 0);
		if (FAILED(hr))
		{
			VariantClear(&varName);
			continue;
		}

		CString str(varName.bstrVal);
		VariantClear(&varName);

		if (str.Compare(compFiterName) == 0)
		{
			IBaseFilter* pSrc = NULL;
			hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&pSrc);
			if (FAILED(hr))
				return hr;
			*ppSrcFilter = pSrc;
			return hr;
		}
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

void CDShow::audio_sync(int sync)
{
	if (sync == 1)
		m_audio_sync += 50;
	else if (sync == 0)
		m_audio_sync = 0;
	else if (sync == -1)
		m_audio_sync -= 50;
	else
		m_audio_sync = sync;

	//MPC convention — IExFilterConfig::Flt_SetInt("audio_delay", ms) 를 지원하는 모든 graph filter
	//(MPC Audio Renderer / SaneAR / MPC-BE 의 audio renderer 등) 에 새 delay 값 전파.
	//지원 안 하는 renderer 면 silent no-op — m_audio_sync 값만 저장.
	if (!m_pGB)
		return;

	//internal FFmpeg path 인 경우 — CSCAudioGain 의 sample.rtStart shift 가 audio.rtStart 가 segment-local 0 부터인
	//우리 path 에서 negative 결과 → DSound 즉시 표시 → delay 무효. 대신 CFFiSource 의 audio anchor offset 에 적용.
	//audio first emit 의 미디어 시점 = video first emit 시점 + delay → 정상 sync.
	if (m_pFFiSource)
	{
		((ffi::CFFiSource*)m_pFFiSource)->set_audio_sync_delay_ms(m_audio_sync);
		logWrite(_T("[AudioSync] internal path: anchor offset = %d ms"), m_audio_sync);
		flush_audio_buffer();
		return;
	}

	CComPtr<IEnumFilters> pEnum;
	if (FAILED(m_pGB->EnumFilters(&pEnum)) || !pEnum)
		return;

	//1차 — audio chain 에 끼워진 자체 transform filter (CSCAudioGain) 에 직접 setter.
	//graph 의 audio renderer 가 DirectSound 처럼 IExFilterConfig 미지원이어도 timestamp shift 가 작동.
	if (m_pAudioGainFilter)
		((CSCAudioGain*)m_pAudioGainFilter)->set_delay_ms(m_audio_sync);

	//2차 — MPC convention IExFilterConfig 지원 filter 가 있으면 그쪽도 호출 (MPC Audio Renderer / SaneAR 등).
	IBaseFilter* pF = NULL;
	ULONG fetched = 0;
	while (pEnum->Next(1, &pF, &fetched) == S_OK)
	{
		CComQIPtr<IExFilterConfig> pCfg(pF);
		if (pCfg)
			pCfg->Flt_SetInt("audio_delay", m_audio_sync);
		pF->Release();
	}

	flush_audio_buffer();
}

void CDShow::flush_audio_buffer()
{
	if (!m_pMS)
		return;
	LONGLONG cur = 0;
	if (FAILED(m_pMS->GetCurrentPosition(&cur)))
		return;
	m_pMS->SetPositions(&cur, AM_SEEKING_AbsolutePositioning,
						NULL, AM_SEEKING_NoPositioning);
}

CString CDShow::get_audio_delay_status()
{
	if (!m_pGB)
		return _T("graph 없음");

	CComPtr<IEnumFilters> pEnum;
	if (FAILED(m_pGB->EnumFilters(&pEnum)) || !pEnum)
		return _T("EnumFilters fail");

	CString supported;
	CString all_filters;
	IBaseFilter* pF = NULL;
	ULONG fetched = 0;
	while (pEnum->Next(1, &pF, &fetched) == S_OK)
	{
		FILTER_INFO fi = {};
		if (SUCCEEDED(pF->QueryFilterInfo(&fi)))
		{
			if (!all_filters.IsEmpty())
				all_filters += _T(", ");
			all_filters += fi.achName;

			CComQIPtr<IExFilterConfig> pCfg(pF);
			if (pCfg)
			{
				if (!supported.IsEmpty())
					supported += _T(", ");
				supported += fi.achName;
			}
			if (fi.pGraph)
				fi.pGraph->Release();
		}
		pF->Release();
	}

	logWrite(_T("[audio_delay_diag] all=%s | supports=%s | gain_chain=%s"),
		all_filters.GetString(),
		supported.IsEmpty() ? _T("(none)") : supported.GetString(),
		m_pAudioGainFilter ? _T("yes") : _T("no"));

	//지원 시 empty 반환 — caller (OSD) 가 별도 표시 안 함.
	//미지원 시만 문자열 반환 — caller 가 붉은 계열로 강조.
	if (m_pAudioGainFilter)
		return _T("");   //CSCAudioGain chain 이 timestamp shift 로 delay 동작 — renderer 무관.

	if (!supported.IsEmpty())
		return _T("");   //renderer 가 IExFilterConfig::Flt_SetInt("audio_delay") 지원.

	return _T("오디오 싱크 미지원");
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
