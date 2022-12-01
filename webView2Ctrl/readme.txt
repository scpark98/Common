[NuGet package version]
Microsoft.Web.WebView2 : 1.0.1418.22
Microsoft.Windows.ImplementationLibrary : 1.0.220914.1

-ICoreWebView2가 아닌 ICoreWebView2_15로 생성하므로 현재 제공되는 WebView2의 대부분의 API가 바로 접근 가능하다.
 (물론 ICoreWebView2로 생성한 후에도 아래와 같이 이후 버전에서 추가된 API를 사용할 수 있다)
  wil::com_ptr<ICoreWebView2_4> wvWnd4 = m_webView.try_query<ICoreWebView2_4>();
  
-dll이 아닌 코드 레벨로 webView2를 사용하므로 한 대의 카메라는 하나의 webView에서만 접근 가능하다.
