#pragma once

/*
* - 프로젝트 속성에서 Common/webView2/include, lib 경로 및 lib 파일 설정.
* 
* - framework.h (or stdafx.h or app.h)에 아래 define 추가.
*	
		#ifdef LIVEWEBEXT_EXPORTS
		#define LIVEWEBEXT_API __declspec(dllexport)
		#else
		#define LIVEWEBEXT_API __declspec(dllimport)
		#endif

  - App.cpp의 InitInstance() 시작 시 AfxOleInit(); 호출
		AfxOleInit();

  - dlg에 웹을 표시할 컨트롤(CStatic) 추가 및 변수 선언(m_static_web)
  
  - dlg의 h에 필요 코드 추가
		#include "WebView2Helper.h"
		...
		CWebView2Helper m_Web;

  - dlg의 cpp에서 WebView2Helper 웹을 표시할 컨트롤과 URL 세팅
		m_Web.SetWebView(&m_static_web, "https://www.naver.com");

  - dlg의 cpp에서 OnSize() 핸들러에 다음 코드 추가
		if (!m_static_web.m_hWnd)
			return;

		CRect rc;
		GetClientRect(rc);
		m_static_web.MoveWindow(rc);
		m_Web.SetBounds(rc.Width(), rc.Height());

  * 현재 이 라이브러리는 프로젝트 속성->매니페스트 도구->입력 및 출력에서
    DPI 인식 항목의 기본 세팅값인 "높은 DPI 인식" 이외의 설정으로 변경하면 충돌이 발생한다.
	이 라이브러리를 사용하는 2개 이상의 앱이 서로 다른 세팅일 경우 충돌이 발생하여
	경고 메시지와 함께 WebView2의 Initialize가 실패한다.
*/

class CWebView2Obj;

class LIVEWEBEXT_API CWebView2Helper
{
public:
	CWebView2Helper();
	virtual ~CWebView2Helper();

	void SetWebView(CWnd* pWnd, CString strURI);
	void Navigate(CString strURL);

	void SetZoom(float fZoom);

	void SetBounds(LONG const width, LONG const height);

	HRESULT ExecuteScript(CString strJavaScript);
	HRESULT PostWebMessageAsJson(CString strWebMessageAsJson);

	//wil::com_ptr<ICoreWebView2> GetWebView2();

protected:
	CWebView2Obj* m_pWebView2Obj;
};

