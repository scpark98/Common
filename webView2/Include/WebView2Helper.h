#pragma once

/*
* - ������Ʈ �Ӽ����� Common/webView2/include, lib ���� ���� �߰�.
* 
* - framework.h (or stdafx.h or app.h)�� �Ʒ� define �߰�.
*	
		#ifdef LIVEWEBEXT_EXPORTS
		#define LIVEWEBEXT_API __declspec(dllexport)
		#else
		#define LIVEWEBEXT_API __declspec(dllimport)
		#endif

  - App.cpp�� InitInstance() ���� �� AfxOleInit(); ȣ��
		AfxOleInit();

  - dlg�� ���� ǥ���� ��Ʈ��(CStatic) �߰� �� ���� ����(m_static_web)
  
  - dlg�� h�� �ʿ� �ڵ� �߰�
		#include "WebView2Helper.h"
		...
		CWebView2Helper m_Web;

  - dlg�� cpp���� WebView2Helper ���� ǥ���� ��Ʈ�Ѱ� URL ����
		m_Web.SetWebView(&m_static_web, "https://www.naver.com");

  - dlg�� cpp���� OnSize() �ڵ鷯�� ���� �ڵ� �߰�
		if (!m_static_web.m_hWnd)
			return;

		CRect rc;
		GetClientRect(rc);
		m_static_web.MoveWindow(rc);
		m_Web.SetBounds(rc.Width(), rc.Height());
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

