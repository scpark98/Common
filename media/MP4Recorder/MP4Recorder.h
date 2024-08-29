#pragma once

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
}

#pragma comment (lib, "libavcodec.a")
#pragma comment (lib, "libavformat.a")
#pragma comment (lib, "libavutil.a")
#pragma comment (lib, "libswresample.a")
#pragma comment (lib, "libswscale.a")
#pragma comment (lib, "libx264.lib")
#pragma comment (lib, "secur32.lib")

#include <afxmt.h>

#ifdef av_err2str
#undef av_err2str
#include <string>
av_always_inline std::string av_err2string(int errnum) {
	char str[AV_ERROR_MAX_STRING_SIZE];
	return av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, errnum);
}
#define av_err2str(err) av_err2string(err).c_str()
#endif

class CMP4RecorderParam
{
public:
	CString filepath = _T("");
	int width = 0;
	int height = 0;
	int encWidth = 0;
	int encHeight = 0;
	int fps = 15;
	int quality = 80; // 0:low quality, 100:best quality
};

class CMP4Recorder
{
public:
	CMP4Recorder();
	virtual ~CMP4Recorder();

	bool Start(CMP4RecorderParam* param);
	void Stop();
	void Pause();
	void Resume();

	void WriteVideoSample(BYTE* pData, int nSize, int nCapWidth, int nCapHeight);

private:
	AVFormatContext* m_pFormatCtx;
	AVStream* m_pVideoStream;
	AVCodecContext* m_pCodecCtx;
	AVPicture* m_pAVPictureEnc;
	AVFrame* m_pAVFrameEnc;
	uint8_t* m_pBufferEnc;
	int m_nBufferSize;
	int m_nFrameCounter;

	bool m_bStartRecord;
	bool m_bPauseRecord;
	bool m_bVideoWriting;
	bool m_bAudioWriting;

	CCriticalSection m_csVideoEncoder;
	CCriticalSection m_csAudioEncoder;
	int m_nEncWidth;
	int m_nEncHeight;
	int m_nSAMPLERATE;
	int m_nQuantizer;
	int m_nTimeScale;
	int m_unFPS;
	int m_nVideoFrameType;
	//DWORD m_dwLastVideoTimestamp;
	//DWORD m_dwLastAudioTimestamp;
	DWORD m_dwStartTime;

	__int64 m_nFrameCount;
};
