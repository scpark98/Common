- framework.h에 다음 정의 추가

#ifdef LIVEWEBEXT_EXPORTS
#define LIVEWEBEXT_API __declspec(dllexport)
#else
#define LIVEWEBEXT_API __declspec(dllimport)
#endif
