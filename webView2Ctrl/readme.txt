[NuGet package version]
Microsoft.Web.WebView2 : 1.0.1293.44
Microsoft.Windows.ImplementationLibrary : 1.0.220914.1

* NH프로젝트 STM에 설치된 버전과의 호환을 위해 임시 낮춰서 작업 중이므로
  일부 코드들은 임시 주석 처리함.

-ICoreWebView2가 아닌 ICoreWebView2_15로 생성하므로 현재 제공되는 WebView2의 대부분의 API가 바로 접근 가능하다.
 (물론 ICoreWebView2로 생성한 후에도 아래와 같이 이후 버전에서 추가된 API를 사용할 수 있다)
  wil::com_ptr<ICoreWebView2_4> wvWnd4 = m_webView.try_query<ICoreWebView2_4>();
	=>그냥 ICoreWebView2로 생성하고 상위 버전이 필요한 경우는 위와 같이 try_query를 사용하여 사용중임.
  
-dll이 아닌 코드 레벨로 webView2를 사용하므로 한 대의 카메라는 하나의 webView에서만 접근 가능하다.
	=>하나의 exe안에서는 여러개의 웹뷰에서 한 대의 카메라의 동시 접근, 사용이 가능하다.
	  김근호 부장이 dll로 만든 웹뷰는 서로 다른 exe라도 한 대의 카메라를 동시 사용할 수 있다.
