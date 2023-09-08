#include "MP4Recorder.h"
#include <mmsystem.h>
#include "AutoSync.h"

#define MAX_MP4_VIDEO_WIDTH		1920
#define MAX_MP4_VIDEO_HEIGHT	1080

char* WCHARToUTF8(const WCHAR* pwstrSrc)
{
	TCHAR TempNull[8] = _T("\0");
	if (pwstrSrc == NULL) pwstrSrc = TempNull; 

	int nLen = (wcslen(pwstrSrc) + 1) * sizeof(WCHAR);
	char* pstr = new char[nLen + 1];
	WideCharToMultiByte( CP_UTF8, 0, pwstrSrc, -1, pstr, nLen, NULL, NULL );

	return pstr;
}

CMP4Recorder::CMP4Recorder()
:	m_hMP4File(NULL)
,	m_nVTrackId(0)
,	m_nATrackId(0)
,	m_pAVPictureEnc(NULL)
,	m_pAVFrameEnc(NULL)
,	m_pBufferEnc(NULL)
,	m_pEncX264(NULL)
,	m_nSAMPLERATE(16000)
,	m_unFPS(15)
,	m_nQuantizer(30)
,	m_nBufferSize(0)
,	m_nTimeScale(90000)
,	m_nVideoFrameType(WMV_VIDEO_FRAME_TYPE_BMP)
,	m_bStartRecord(false)
,	m_bPauseRecord(false)
,	m_bVideoWriting(false)
,	m_bAudioWriting(false)
,	m_dwLastVideoTimestamp(0)
,	m_dwLastAudioTimestamp(0)
{
}

CMP4Recorder::~CMP4Recorder()
{
	Stop();
}

bool CMP4Recorder::Start(CString file, int width, int height, int fps, int quality)
{
	CMP4RecorderParam param;
	param.filepath = file;
	param.width = width;
	param.height = height;
	param.fps = fps;
	param.quality = quality;

	return Start(&param);
}

bool CMP4Recorder::Start(CMP4RecorderParam* param)
{
	bool bResult = true;

	if (param->quality < 0)
		param->quality = 0;
	else if (param->quality > 100)
		param->quality = 100;

	if (param->width <= 0 || param->height <= 0)
		return false;

	m_nEncWidth			= param->width;
	m_nEncHeight		= param->height;
	m_unFPS				= param->fps;
	m_nQuantizer		= 28 + (int)((float)(100 - param->quality) / 10); //높을수로 화질저하

	char* cfilename = WCHARToUTF8(param->filepath);
	m_hMP4File = MP4Create(cfilename);
	delete [] cfilename;

	if (!MP4_IS_VALID_FILE_HANDLE(m_hMP4File)) return false;

	if (true)//pProfile->nStreams & WMV_STREAM_VIDEO_ONLY)
	{
		try
		{
			x264_param_t params;
			//x264_param_default(&params);
			x264_param_default_preset(&params, "veryfast", "zerolatency");
			//x264_param_default_preset(&params, "show", "film");

			params.analyse.b_weighted_bipred = 0;
			params.analyse.i_weighted_pred 	= X264_WEIGHTP_NONE;

			params.i_threads				= 1;
			params.i_bframe					= 0;
			params.i_width					= m_nEncWidth;
			params.i_height					= m_nEncHeight;	
			params.i_fps_num				= m_unFPS;
//			params.i_keyint_max				= m_unFPS*2;
//			params.i_keyint_min				= 0;

			params.b_repeat_headers			= 0;
			params.b_annexb					= 0;
			params.b_vfr_input				= 0;

			params.rc.i_rc_method			= X264_RC_CRF;
			params.rc.i_qp_constant			= m_nQuantizer;
			params.rc.f_rf_constant			= m_nQuantizer;

			params.b_deblocking_filter		= 0;
			params.b_cabac					= 0;
			params.i_frame_reference		= 1;
			params.i_log_level				= 0;	//X264_LOG_ERROR X264_LOG_WARNING X264_LOG_INFO X264_LOG_DEBUG X264_LOG_NONE;

			params.analyse.i_subpel_refine	= 1;
			params.analyse.i_trellis		= 0; 
			params.analyse.i_me_range		= 0;
			params.analyse.intra			= 0;	//X264_ANALYSE_I4x4 | X264_ANALYSE_I8x8; 
			params.analyse.inter			= 0;	//X264_ANALYSE_I4x4 | X264_ANALYSE_I8x8;// | X264_ANALYSE_PSUB16x16 | X264_ANALYSE_BSUB16x16; 

			m_pEncX264 = x264_encoder_open(&params);
			if (m_pEncX264 == NULL) 
			{
				bResult = false;
				goto ERROREXIT;
			}
		}
		catch (...)
		{
			bResult = false;
			goto ERROREXIT;
		}

		x264_nal_t *nal;
		int i_nal;
		x264_encoder_headers(m_pEncX264, &nal, &i_nal);
		uint8_t *sps = nal[0].p_payload;

		MP4SetTimeScale(m_hMP4File, m_nTimeScale);
		m_nVTrackId = MP4AddH264VideoTrack(m_hMP4File, m_nTimeScale, m_nTimeScale/m_unFPS, m_nEncWidth, m_nEncHeight, sps[5], sps[6], sps[7], 3);
		if (m_nVTrackId == MP4_INVALID_TRACK_ID)
		{
			bResult = false;
			goto ERROREXIT;
		}

		MP4SetVideoProfileLevel(m_hMP4File, 0x7f);

		uint8_t profile_idc, profile_compat, level_idc;
		profile_idc = nal[0].p_payload[5];
		profile_compat = nal[0].p_payload[6];
		level_idc = nal[0].p_payload[7];
		MP4SetTrackIntegerProperty(m_hMP4File, m_nVTrackId, "mdia.minf.stbl.stsd.avc1.avcC.AVCProfileIndication", profile_idc);
		MP4SetTrackIntegerProperty(m_hMP4File, m_nVTrackId, "mdia.minf.stbl.stsd.avc1.avcC.profile_compatibility", profile_compat);
		MP4SetTrackIntegerProperty(m_hMP4File, m_nVTrackId, "mdia.minf.stbl.stsd.avc1.avcC.AVCLevelIndication", level_idc);
		MP4AddH264SequenceParameterSet(m_hMP4File, m_nVTrackId, nal[0].p_payload + 4, nal[0].i_payload - 4);
		MP4AddH264PictureParameterSet(m_hMP4File, m_nVTrackId, nal[1].p_payload + 4, nal[1].i_payload - 4);

		m_pAVFrameEnc = av_frame_alloc();
		m_nBufferSize = avpicture_get_size(AV_PIX_FMT_YUV420P, m_nEncWidth, m_nEncHeight);
		m_pBufferEnc = new uint8_t[m_nBufferSize];
		avpicture_fill((AVPicture*)m_pAVFrameEnc, m_pBufferEnc, AV_PIX_FMT_YUV420P, m_nEncWidth, m_nEncHeight);
		m_pAVPictureEnc = (AVPicture*)m_pAVFrameEnc;
	}

	m_dwLastVideoTimestamp = m_dwLastAudioTimestamp = timeGetTime();
	m_bStartRecord = true;
	m_bPauseRecord = false;

ERROREXIT:

	if (bResult != true) Stop();

	return bResult;
}

void CMP4Recorder::Stop()
{
	m_bStartRecord = false;

	try
	{
		for (int i = 0; i < 1000; i++)
		{
			if (!m_bVideoWriting && !m_bAudioWriting)
			{
				if (m_hMP4File != NULL)
				{
					MP4Close(m_hMP4File);
					m_hMP4File = NULL;
				}
				break;
			}
			else Sleep(1);
		}
	}
	catch (...) { m_hMP4File = NULL; }

	try
	{
		if (m_pBufferEnc != NULL)
		{
			delete []m_pBufferEnc;
			m_pBufferEnc = NULL;
		}
	}
	catch (...) { m_pBufferEnc = NULL; }

	try
	{
		if (m_pAVFrameEnc) 
		{
			av_frame_free(&m_pAVFrameEnc);
			m_pAVFrameEnc = NULL;
		}
	}
	catch (...) { m_pAVFrameEnc = NULL; }


	try
	{
		if (m_pEncX264 != NULL)
		{
			x264_encoder_close(m_pEncX264);
			m_pEncX264 = NULL;
		}
	}
	catch (...) { m_pEncX264 = NULL; }
}

void CMP4Recorder::Pause()
{
	m_bPauseRecord = true;
}

void CMP4Recorder::Resume()
{
	m_dwLastVideoTimestamp = m_dwLastAudioTimestamp = timeGetTime();
	m_bPauseRecord = false;
}

void CMP4Recorder::WriteVideoSample(BYTE* lpData, int nSize, int nCapWidth, int nCapHeight, double fTimeStamp)
{
	if (m_bStartRecord != true || m_bPauseRecord || m_hMP4File == NULL || m_nVTrackId <= 0) return;
	m_bVideoWriting = true;

	try
	{
		CAutoSync	autoSync(m_csVideoEncoder);

		if (m_nVideoFrameType == WMV_VIDEO_FRAME_TYPE_BMP || nCapWidth != m_nEncWidth || nCapHeight != m_nEncHeight)
		{
			static AVPicture	avPictureCap;
			avpicture_fill(&avPictureCap, lpData, 
				(m_nVideoFrameType == WMV_VIDEO_FRAME_TYPE_BMP) ? AV_PIX_FMT_RGB32 : AV_PIX_FMT_YUV420P, nCapWidth, nCapHeight);

			//if (m_nVideoFrameType == WMV_VIDEO_FRAME_TYPE_BMP)
			//{
			//	avPictureCap.data[0] += avPictureCap.linesize[0]*(nCapHeight-1);
			//	avPictureCap.linesize[0] = -avPictureCap.linesize[0];
			//}

			int flags = SWS_FAST_BILINEAR;
			if (m_nEncHeight <= 720)
				flags = SWS_BILINEAR;
			if (nCapWidth == m_nEncWidth && nCapHeight == m_nEncHeight)
				flags = SWS_FAST_BILINEAR;

			SwsContext *pSwsConvertCtx = sws_getContext(nCapWidth, nCapHeight, 
				(m_nVideoFrameType == WMV_VIDEO_FRAME_TYPE_BMP) ? AV_PIX_FMT_RGB32 : AV_PIX_FMT_YUV420P,
				m_nEncWidth, m_nEncHeight, AV_PIX_FMT_YUV420P, flags, NULL, NULL, NULL);
			if (pSwsConvertCtx)
			{
				int nRet = sws_scale(pSwsConvertCtx, avPictureCap.data, avPictureCap.linesize, 0, nCapHeight,
					m_pAVPictureEnc->data, m_pAVPictureEnc->linesize);
				sws_freeContext(pSwsConvertCtx);
			}
		}
		else
		{
			avpicture_fill(m_pAVPictureEnc, lpData, AV_PIX_FMT_YUV420P, nCapWidth, nCapHeight);
		}
		
		int i_nal;
		x264_nal_t *nal;

		x264_picture_t pic = {0};
		pic.img.i_csp = X264_CSP_I420;
		pic.img.i_plane = 3;
		pic.img.i_stride[0] = m_pAVPictureEnc->linesize[0];
		pic.img.i_stride[1] = m_pAVPictureEnc->linesize[1];
		pic.img.i_stride[2] = m_pAVPictureEnc->linesize[2];
		pic.img.i_stride[3] = NULL;
		pic.img.plane[0] = m_pAVPictureEnc->data[0];
		pic.img.plane[1] = m_pAVPictureEnc->data[1];
		pic.img.plane[2] = m_pAVPictureEnc->data[2];
		pic.img.plane[3] = 0;

		if (m_pEncX264)
		{
			int nOut = x264_encoder_encode(m_pEncX264, &nal, &i_nal, &pic, &pic);
			if (nOut > 0) 
			{
				bool bIsKeyFrame = (pic.i_type == X264_TYPE_IDR);
				MP4Duration	nDuration = MP4_INVALID_DURATION;
				if (m_dwLastVideoTimestamp > 0) nDuration = (int)(90000.0 / (1000.0 / (double)(timeGetTime() - m_dwLastVideoTimestamp)));
				MP4WriteSample(m_hMP4File, m_nVTrackId, nal->p_payload, nOut, nDuration, 0, bIsKeyFrame);
				m_dwLastVideoTimestamp = timeGetTime();
			}
		}
	}
	catch (...) { }

	m_bVideoWriting = false;
}
