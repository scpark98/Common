[NH프로젝트에서 김근호 부장 작성]

- Multibyte, static으로 빌드됨.
- dll로 작성되서 그런지 한 카메라를 2개 이상의 webview에서 동시 접근 가능하다.
- CWebView2Obj의 instance로 CWebView2Wnd의 멤버에 접근하는 구조로 다소 번거롭다.

- framework.h에 다음 정의 추가

#ifdef LIVEWEBEXT_EXPORTS
#define LIVEWEBEXT_API __declspec(dllexport)
#else
#define LIVEWEBEXT_API __declspec(dllimport)
#endif
