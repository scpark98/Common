#include "MP4Recorder.h"
#include <mmsystem.h>
#include "AutoSync.h"

int calculateCRF(int qualityScore) {
	if (qualityScore < 1) qualityScore = 1;
	if (qualityScore > 100) qualityScore = 100;
	int retValue = static_cast<int>(51 * (1 - (qualityScore - 1) / 99.0));
	if (retValue < 23) retValue = 23;
	return retValue;
}

char* WCHARToUTF8(const WCHAR* pwstrSrc)
{
	TCHAR TempNull[8] = _T("\0");
	if (pwstrSrc == NULL) pwstrSrc = TempNull;

	int nLen = (wcslen(pwstrSrc) + 1) * sizeof(WCHAR);
	char* pstr = new char[nLen + 1];
	WideCharToMultiByte(CP_UTF8, 0, pwstrSrc, -1, pstr, nLen, NULL, NULL);

	return pstr;
}

CMP4Recorder::CMP4Recorder()
	: m_pFormatCtx(nullptr)
	, m_pVideoStream(nullptr)
	, m_pCodecCtx(nullptr)
	, m_pAVPictureEnc(nullptr)
	, m_pAVFrameEnc(nullptr)
	, m_pBufferEnc(nullptr)
	, m_nFrameCounter(0)
	, m_bStartRecord(false)
	, m_bPauseRecord(false)
	, m_bVideoWriting(false)
	, m_bAudioWriting(false)
	, m_nBufferSize(0)
	, m_nFrameCount(0)
	, m_dwStartTime(0)
{
	av_register_all();
	avcodec_register_all();
}

CMP4Recorder::~CMP4Recorder()
{
	Stop();
}

bool CMP4Recorder::Start(CMP4RecorderParam* param)
{
	m_nEncWidth = param->encWidth > 0 ? param->encWidth : param->width;
	m_nEncHeight = param->encHeight > 0 ? param->encHeight : param->height;
	m_unFPS = param->fps;
	m_nQuantizer = calculateCRF(param->quality);

	char* cfilename = WCHARToUTF8(param->filepath);
	std::string sFileName = cfilename;
	delete[] cfilename;

	// Allocate the format context
	//avformat_alloc_output_context2(&m_pFormatCtx, nullptr, "mpegts", sFileName.data());
	avformat_alloc_output_context2(&m_pFormatCtx, nullptr, "mp4", sFileName.data());
	if (!m_pFormatCtx) {
		return false;
	}

	// Add video stream
	// Find the H.264 video encoder
	AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (!codec) {
		//fprintf(stderr, "Codec not found\n");
		return false;
	}

	m_pVideoStream = avformat_new_stream(m_pFormatCtx, codec);
	if (!m_pVideoStream) {
		TRACE("Could not allocate video stream\n");
		return false;
	}

	m_pCodecCtx = avcodec_alloc_context3(codec);
	if (!m_pCodecCtx) {
		TRACE("Could not allocate video codec context\n");
		return false;
	}

	m_pCodecCtx->codec_id = AV_CODEC_ID_H264;
	//m_pCodecCtx->bit_rate = 400000;  // Set a suitable bit rate
	m_pCodecCtx->width = m_nEncWidth;
	m_pCodecCtx->height = m_nEncHeight;
	m_pCodecCtx->time_base = { 1, m_unFPS };
	m_pCodecCtx->framerate = { m_unFPS, 1 };
	//m_pCodecCtx->gop_size = 10; // Set the group of pictures size
	//m_pCodecCtx->max_b_frames = 1;
	m_pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

	if (m_pFormatCtx->oformat->flags & AVFMT_GLOBALHEADER)
		m_pCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	// Optional: Set H.264 preset and tune options
	AVDictionary* opt = nullptr;
	char szValue[128];
	av_dict_set(&opt, "preset", "veryfast", 0);
	av_dict_set(&opt, "tune", "zerolatency", 0);
	av_dict_set(&opt, "profile", "high", 0);
	sprintf(szValue, "%d", m_nQuantizer);
	av_dict_set(&opt, "crf", szValue, 0);
	sprintf(szValue, "trellis=0:merange=0:keyint=%d:min-keyint=%d", 1*m_unFPS, 1*m_unFPS);
	av_dict_set(&opt, "x264-params", szValue, 0);
	// Open the codec
	int ret = avcodec_open2(m_pCodecCtx, codec, &opt);
	av_dict_free(&opt);
	if (ret < 0) {
		TRACE("Could not open codec: %s\n", av_err2str(ret));
		return false;
	}

	m_pAVFrameEnc = av_frame_alloc();
	m_nBufferSize = avpicture_get_size(AV_PIX_FMT_YUV420P, m_nEncWidth+32, m_nEncHeight+32);
	m_pBufferEnc = new uint8_t[m_nBufferSize];
	avpicture_fill((AVPicture*)m_pAVFrameEnc, m_pBufferEnc, AV_PIX_FMT_YUV420P, m_nEncWidth, m_nEncHeight);
	m_pAVPictureEnc = (AVPicture*)m_pAVFrameEnc;

	// Set the codec parameters to the stream
	ret = avcodec_parameters_from_context(m_pVideoStream->codecpar, m_pCodecCtx);
	if (ret < 0) {
		TRACE("Could not copy codec parameters: %s\n", av_err2str(ret));
		return false;
	}

	// Open the output file with the specified movflags
	ret = avio_open(&m_pFormatCtx->pb, sFileName.data(), AVIO_FLAG_WRITE);
	if (ret < 0) {
		return false;
	}

	// Set movflags for fragmentation
	AVDictionary* mux_opts = nullptr;
	av_dict_set(&mux_opts, "movflags", "+frag_keyframe+empty_moov+default_base_moof+separate_moof", 0);
	// Write the stream header
	ret = avformat_write_header(m_pFormatCtx, &mux_opts);
	av_dict_free(&mux_opts);
	if (ret < 0) {
		TRACE("Error occurred when opening output file: %s\n", av_err2str(ret));
		return false;
	}

	m_bStartRecord = true;

	return true;
}

void CMP4Recorder::Stop()
{
	m_bStartRecord = false;

	for (int i = 0; i < 1000; i++)
	{
		if (!m_bVideoWriting && !m_bAudioWriting)
		{
			break;
		}
		else Sleep(1);
	}

	if (m_pCodecCtx)
	{
		// Flush the encoder
		avcodec_send_frame(m_pCodecCtx, nullptr); // Send a NULL frame to flush the encoder
		AVPacket pkt;
		av_init_packet(&pkt);
		pkt.data = nullptr;
		pkt.size = 0;

		while (true) {
			int ret = avcodec_receive_packet(m_pCodecCtx, &pkt);
			if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
				break; // No more packets to flush
			if (ret < 0) {
				TRACE("Error during flushing the encoder: %s\n", av_err2str(ret));
				break;
			}

			// Rescale packet timestamps from codec to stream timebase
			av_packet_rescale_ts(&pkt, m_pCodecCtx->time_base, m_pVideoStream->time_base);
			pkt.stream_index = m_pVideoStream->index;

			// Write the packet to the output file
			ret = av_interleaved_write_frame(m_pFormatCtx, &pkt);
			if (ret < 0) {
				TRACE("Error writing packet during flush: %s\n", av_err2str(ret));
				break;
			}

			av_packet_unref(&pkt); // Free the packet
		}
	}

	if (m_pFormatCtx != NULL) {
		av_write_trailer(m_pFormatCtx);
	}

	if (m_pVideoStream) {
		if (m_pVideoStream->codec) {
			avcodec_close(m_pVideoStream->codec);
		}
	}

	if (m_pFormatCtx)
	{
		// Close the output file
		if (!(m_pFormatCtx->oformat->flags & AVFMT_NOFILE)) {
			avio_closep(&m_pFormatCtx->pb);
		}
		avformat_free_context(m_pFormatCtx);
		m_pFormatCtx = NULL;
	}

	if (m_pBufferEnc != NULL)
	{
		delete[]m_pBufferEnc;
		m_pBufferEnc = NULL;
	}

	if (m_pCodecCtx != NULL) {
		avcodec_free_context(&m_pCodecCtx);
		m_pCodecCtx = NULL;
	}

	if (m_pAVFrameEnc)
	{
		av_frame_free(&m_pAVFrameEnc);
		m_pAVFrameEnc = NULL;
	}

	m_nFrameCount = 0;
	m_dwStartTime = timeGetTime();
}

void CMP4Recorder::Pause()
{
	m_bPauseRecord = true;
}

void CMP4Recorder::Resume()
{
	m_bPauseRecord = false;
}

void CMP4Recorder::WriteVideoSample(BYTE* pData, int nSize, int nCapWidth, int nCapHeight)
{
	if (!m_bStartRecord || m_bPauseRecord)
		return;
	m_bVideoWriting = true;

	static AVPicture	avPictureCap;
	avpicture_fill(&avPictureCap, pData, AV_PIX_FMT_RGB32, nCapWidth, nCapHeight);

	// Create the SwsContext for the conversion7
	SwsContext* pSwsConvertCtx = sws_getContext(nCapWidth, nCapHeight, AV_PIX_FMT_RGB32,
		m_nEncWidth, m_nEncHeight, AV_PIX_FMT_YUV420P, SWS_SPLINE, nullptr, nullptr, nullptr);
	if (pSwsConvertCtx)
	{
		int ret = sws_scale(pSwsConvertCtx, avPictureCap.data, avPictureCap.linesize, 0, nCapHeight,
			m_pAVPictureEnc->data, m_pAVPictureEnc->linesize);
		sws_freeContext(pSwsConvertCtx);
	}

	DWORD currentTime = timeGetTime();
	m_pAVFrameEnc->pts = (currentTime - m_dwStartTime) * m_unFPS / 1000;

	// Send the frame to the encoder
	int ret = avcodec_send_frame(m_pCodecCtx, m_pAVFrameEnc);
	while (ret >= 0) {
		AVPacket pkt;
		av_init_packet(&pkt);
		pkt.data = nullptr;
		pkt.size = 0;

		ret = avcodec_receive_packet(m_pCodecCtx, &pkt);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
			break; // 패킷이 없거나 스트림 끝
		}
		else if (ret < 0) {
			TRACE("패킷을 인코딩하는 중 오류 발생: %s\n", av_err2str(ret));
			break;
		}

		av_packet_rescale_ts(&pkt, m_pCodecCtx->time_base, m_pVideoStream->time_base);
		pkt.stream_index = m_pVideoStream->index;

		ret = av_interleaved_write_frame(m_pFormatCtx, &pkt);
		if (ret < 0) {
			TRACE("패킷을 파일에 쓰는 중 오류 발생: %s\n", av_err2str(ret));
			break;
		}

		av_packet_unref(&pkt);
	}

	m_bVideoWriting = false;
}
