#pragma once

/* sdh ���� ���̺귯�� �̿�(x86�� ������)
* ������Ʈ ������ ffmpeg ������ �����ؾ� �ϴµ� �뷮������ git�� ���Ե� �� �����Ƿ� ������ �����ؿ;� ��.
* ffmpeg/include, ffmpeg/lib�� �Ӽ��� �߰�.
* �Ϲ�->��� �÷��� ������ 8.1�� �ؾ� ��.
* �÷��� ���� ������ "Visual Studio 2015"�� �ؾ� Release��忡���� �����.(Debug ��忡���� 2019�ε� �����)
* ��Ŀ->����� �׸� /FORCE:MULTIPLE �߰��ؾ� �ߺ��� �ڵ忡 ���� ��ũ������ �߻����� ����.
* ��Ŀ->��� �ɼ�->�̹����� ������ ���� ó���� ����->�ƴϿ�(/SAFESEH:NO)�� �����ؾ� SAFESH ���� ��ũ ������ �߻����� ����.
* 

*/

#include <Windows.h>
#include <tchar.h>

#define inline _inline
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "inttypes.h"
#include "stdint.h"
#include "x264_142.h"
};

#define MP4V2_USE_STATIC_LIB
#include <mp4v2/mp4v2.h>
//#include <faac/faac.h>

#pragma warning (disable:4996)

#pragma comment (lib, "legacy_stdio_definitions.lib")

#pragma comment (lib, "libavcodec.a")
#pragma comment (lib, "libavutil.a")
#pragma comment (lib, "libswscale.a")
#pragma comment (lib, "libiconv.a")
#pragma comment (lib, "libx264_142.a")
#pragma comment (lib, "libgcc.a")
#pragma comment (lib, "libmingwex.a")

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

class CMP4RecorderParam
{
public:
	LPCTSTR filepath = _T("");
	int width = 0;
	int height = 0;
	int fps = 15;
	int quality = 80;	//0:low quality, 100:best quality
};

class CMP4Recorder
{
public:

	CMP4Recorder();
	virtual ~CMP4Recorder();

	bool Start(LPCTSTR file, int width, int height, int fps = 15, int quality = 80);
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

	//CCriticalSection	m_csVideoEncoder;
	//CCriticalSection	m_csAudioEncoder;
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
