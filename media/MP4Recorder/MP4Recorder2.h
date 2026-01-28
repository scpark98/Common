#pragma once

/* sdh 제공 라이브러리 이용(x86만 지원함)
* 프로젝트 폴더에 ffmpeg 폴더가 존재해야 하는데 용량때문에 git에 포함될 수 없으므로 별도로 복사해와야 함.
* ffmpeg/include, ffmpeg/lib를 속성에 추가.
* 일반->대상 플랫폼 버전을 8.1로 해야 함.
* 플랫폼 도구 집합을 "Visual Studio 2015"로 해야 Release모드에서도 빌드됨.(Debug 모드에서는 2019로도 빌드됨)
* 링커->명령줄 항목에 /FORCE:MULTIPLE 추가해야 중복된 코드에 대한 링크에러가 발생하지 않음.
* 링커->모든 옵션->이미지에 안전한 예외 처리기 포함->아니요(/SAFESEH:NO)로 설정해야 SAFESH 관련 링크 에러가 발생하지 않음.
* 

*/

//#include <Windows.h>
#include <afxmt.h>
#include <tchar.h>
#include "MP4Recorder.h"

#define inline _inline
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "inttypes.h"
#include "stdint.h"
#include "x264.h"
};

#define MP4V2_USE_STATIC_LIB
#include <mp4v2/mp4v2.h>
//#include <faac/faac.h>

#pragma warning (disable:4996)

#pragma comment (lib, "libavcodec.a")
#pragma comment (lib, "libavutil.a")
#pragma comment (lib, "libswscale.a")
#pragma comment (lib, "libx264.lib")

//#pragma comment (lib, "legacy_stdio_definitions.lib")
//#pragma comment (lib, "libiconv.a")
//#pragma comment (lib, "libgcc.a")
//#pragma comment (lib, "libmingwex.a")

#ifdef _DEBUG
#pragma comment(lib, "libmp4v2D.lib")
#else
#pragma comment(lib, "libmp4v2.lib")
#endif


//#define inline _inline
//extern "C"
//{
//#include "libavcodec/avcodec.h"
//#include "libswscale/swscale.h"
//};
//
//FILE _iob[] = { *stdin, *stdout, *stderr };
//extern "C" FILE * __cdecl __iob_func(void) { return _iob; }


typedef void *MP4FileHandle;

#define WMV_VIDEO_FRAME_TYPE_BMP 0

class CMP4Recorder2
{
public:

	CMP4Recorder2();
	virtual ~CMP4Recorder2();

	bool Start(CString file, int width, int height, int fps = 15, int quality = 80);
	bool Start(CMP4RecorderParam* param);
	void Stop();
	void Pause();
	void Resume();

	void WriteVideoSample(BYTE* pData, int nSize, int nCapWidth, int nCapHeight, double fTimeStamp);

private:

	MP4FileHandle		m_hMP4File;
	MP4TrackId			m_nVTrackId;
	MP4TrackId			m_nATrackId;
	bool				m_bStartRecord;
	bool				m_bPauseRecord;
	bool				m_bVideoWriting;
	bool				m_bAudioWriting;

	AVPicture*			m_pAVPictureEnc;
	AVFrame*			m_pAVFrameEnc;
	uint8_t*			m_pBufferEnc;
	x264_t*				m_pEncX264;
	int					m_nBufferSize;

	CCriticalSection	m_csVideoEncoder;
	CCriticalSection	m_csAudioEncoder;
	int					m_nEncWidth;
	int					m_nEncHeight;
	int					m_nSAMPLERATE;
	int					m_nQuantizer;
	int					m_nTimeScale;
	int					m_unFPS;
	int					m_nVideoFrameType;
	DWORD				m_dwLastVideoTimestamp;
	DWORD				m_dwLastAudioTimestamp;
};
