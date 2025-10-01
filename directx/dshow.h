#pragma once

#include <Afxwin.h>
#include <Afxdisp.h>

#include <dshow.h>          // ��� ����
#include <D3d9.h>
#include <vmr9.h>

#include "../Functions.h"
#include "../subtitle/Subtitle.h"
#include "../subtitle/subtitle_setting.h"

#define USE_DIRECT_VOB_SUB	false

#ifdef MEDIAINFO_LIBRARY
#include "MediaInfo/MediaInfo.h" //Staticly-loaded library (.lib or .a or .so)
#define MediaInfoNameSpace MediaInfoLib;
#else //MEDIAINFO_LIBRARY
#include "MediaInfoDLL/MediaInfoDLL.h" //Dynamicly-loaded library (.dll or .so)
#define MediaInfoNameSpace MediaInfoDLL;
#endif //MEDIAINFO_LIBRARY
#include <iostream>
#include <iomanip>
using namespace MediaInfoNameSpace;


#define VOLUME_MAX 100
#define MESSAGE_DSHOW_MEDIA		WM_USER + 23
#define WM_GRAPHNOTIFY			WM_USER + 24

//#if USE_DIRECT_VOB_SUB
//IDirectVobSub�� �����͸� ���ͼ� ���� �޼ҵ带 ȣ���Ͽ� �����ؾ� ������
//���� VSFilter�ҽ��� ������ �غ������� �� �� ���� ������ �߻��Ͽ� ����.
//���� ��ӵ� �޽����� �̿��Ͽ� ����ϵ��� �ϴ� ������.
static UINT WM_DVSPREVSUB = RegisterWindowMessage(TEXT("WM_DVSPREVSUB"));
static UINT WM_DVSNEXTSUB = RegisterWindowMessage(TEXT("WM_DVSNEXTSUB"));
static UINT WM_DVSHIDESUB = RegisterWindowMessage(TEXT("WM_DVSHIDESUB"));
static UINT WM_DVSSHOWSUB = RegisterWindowMessage(TEXT("WM_DVSSHOWSUB"));
static UINT WM_DVSSHOWHIDESUB = RegisterWindowMessage(TEXT("WM_DVSSHOWHIDESUB"));
static UINT s_uTaskbarRestart = RegisterWindowMessage(TEXT("TaskbarCreated"));
static UINT WM_NOTIFYICON = RegisterWindowMessage(TEXT("MYWM_NOTIFYICON"));
static UINT WM_DVSMESSAGE = RegisterWindowMessage(TEXT("WM_DVSMESSAGE"));
//#endif

//�� Ÿ������ video, audio�� deque�� �����ؼ� �������. �ʿ��ϸ� textŸ�Ե� �����Ѵ�.
class CMediaStream
{
public:
	CMediaStream(CString _stream_name, int _index)
	{
		stream_name = _stream_name;
		index = _index;
	}

	int		get_index() { return index; }
	CString	get_stream_name() { return stream_name; }

protected:
	int index;				//�̵���� AM_MEDIA_TYPE�� �߿��� ���� ���° ��Ʈ������ ����ص־� ���߿� �̰ɷ� ���� ��� ����.
	CString stream_name;
};


class CDShow
{
public:
	CDShow();
	~CDShow();

	enum CDShow_Message
	{
		msg_ec_complete = 0,
		msg_track_moved,
	};

	bool			m_use_dvs;

	CSubtitleSetting m_subCfg;
	void			save_sub_cfg();

	MediaInfo		m_media_info;
	CString			get_media_info_string() { return m_media_info_string; }
	double			get_frame_rate();

	int				load_media(CString sfile, CWnd* pParent, bool auto_render = false);
	void			close_media();
	bool			is_media_opened() { return (m_pGB != NULL); }
	CString			get_media_filename() { return m_media_filename; }
	bool			is_media_video();
	bool			is_windows_media();

	int				m_play_state;
	void			play(int state);
	int				get_play_state() { return m_play_state; }

	//ms������ ����ð��� ���´�.
	double			get_media_duration() { return m_duration; }

//Ʈ�� �̵�
	int				m_default_interval;
	int				m_control_interval;
	void			set_video_position(CRect r);
	double			get_track_pos();
	void			set_track_pos(double pos);
	void			move_track(bool forward, int interval = -1);	//unit:sec
	void			step_frame(bool forward);
	bool			capture_frame(CString sfile);

	std::deque<CMediaStream> m_video_stream;
	std::deque<CMediaStream> m_audio_stream;
	int				get_video_stream_count() { return m_video_stream.size(); }
	int				get_audio_stream_count() { return m_audio_stream.size(); }
	int				get_video_stream_current() { return m_video_stream_index; }
	int				get_audio_stream_current() { return m_audio_stream_index; }
	CString			get_video_stream(int index) { return m_video_stream[index].get_stream_name(); }
	CString			get_audio_stream(int index) { return m_audio_stream[index].get_stream_name(); }
	void			select_stream(bool video, int index);

	double			get_playback_rate();
	void			set_playback_rate(double rate);

	CSize			get_video_size() { return m_video_size; }
	DWORD			get_aspect_ratio_mode();

	//mode : -1(toggle), 0(none), 1(ratio mode)
	void			set_aspect_ratio_mode(int mode);

	bool			get_video_mirror() { return m_mirror; }
	bool			get_video_flip() { return m_flip; }
	
	enum PAN_SCAN_MODE
	{
		pan_scan_origin = 0,
		pan_scan_size,
		pan_scan_move,
		pan_scan_mirror,
		pan_scan_flip,
	};
	void			set_video_pan_scan(DWORD dwStreamID, int mode, float dx, float dy);

	enum VIDEO_ADJUST
	{
		adjust_contrast = 0,
		adjust_bright,
		adjust_hue,
		adjust_saturation,
	};
	int				adjust_video(int dwStreamID, int target, bool up);

	int				get_volume() { return m_volume; }
	//volume�� -1�̸� mute�� ó���ϰ� m_volume���� �������� �ʴ´�.
	void			set_volume(int volume, bool reset_mute = true);
	bool			get_volume_mute() { return m_volume_mute; }
	void			volume_up(bool up, int interval = 5);
	void			hide_cursor(bool hide = true);

	HRESULT			HandleGraphEvent(WPARAM wParam,LPARAM lparam);
	HRESULT			save_filter_graph(IGraphBuilder *pGraph, CString sfile);


//�ڸ� ����
	bool			m_has_subtitle;		//�ڸ� ������ �ִ���
	CString			m_subtitle_file;	//�ڸ� ���ϸ�
	HWND			m_hDirectVobSubWnd;
	bool			m_show_subtitle;
	void			show_subtitle_property_page();
	void			DirectVobSub_function(WPARAM wParam, LPARAM lParam);
	enum DVS_messages
	{
		msg_parent_hwnd = 0,
		msg_get_FileName,
		msg_put_FileName,
		msg_get_HideSubtitles,
		msg_put_HideSubtitles,	//0:hide, 1:show, -1:toggle
		msg_get_Placement,
		msg_put_Placement,
		msg_get_OSD,
		msg_put_OSD,
		msg_get_SubtitleTiming,
		msg_put_SubtitleTiming,
	};
	int				m_subtitle_sync;	//ms
	//-1, 1�̸� 500ms������ ������, ������ �����ϰ� �� �̿��� ���̸� �ش� ���� ��ũ �ӵ��� �����Ѵ�.
	//-1:faster, 1:slower, 0:origin	//per 500ms
	void			subtitle_sync(int sync);

	//�ڸ� ��ġ �̵�
	enum SUBTITLE_POS_DIRECTION
	{
		dir_default = -1,
		dir_left,
		dir_up,
		dir_right,
		dir_down,
	};
	void			subtitle_placement(int dir);
	void			subtitle_placement(int x, int y);


	CDC*		m_pParentDC;
	int			m_buf_index;
	CDC*		m_pMemDC[2];
	CBitmap*	m_pBitmap[2];
	VMR9AlphaBitmap m_AlphaBitmap;
	COLORREF	m_crColorKey;

	HRESULT		update_osd_subtitle();
	void		prepare_AlphaBitmap();
	void		prepare_next_subtitle(CString text, COLORREF crText);
	void		show_next_subtitle();

	//osd��
	LOGFONT		m_lfOsd;
	CString		m_osd_text;
	COLORREF	m_osd_color;
	void		set_osd_text(CString text, COLORREF cr = RGB(234, 212, 198));

	//subtitle��
	CCaption	m_cur_subtitle;
	void		set_subtitle_text(CCaption caption);
	int			subtitle_font_enlarge(int enlarge);		//1:larger, -1:smaller, 0:default
	int			subtitle_font_bold(bool bold);

	void ShowFilterPropertyPage(CString sFilterName);

protected:
	CWnd*			m_pParent;
	CString			m_media_filename;
	IGraphBuilder*	m_pGB;
	IBaseFilter*	m_VMR;
	IBaseFilter*	pSource;
	IBaseFilter*	m_pSplitter;
	IBaseFilter*	m_SourceBase;
	IFileSourceFilter	*m_pFileSource;

	CComQIPtr<IMediaPosition> m_pMP;
	CComQIPtr<IMediaSeeking> m_pMS;
	CComQIPtr<IMediaControl> m_pMC;
	CComQIPtr<IMediaEventEx> m_pME;


	//VMR����
	CComQIPtr<IVMRWindowlessControl9> m_pVMRWC;
	CComQIPtr<IVMRFilterConfig9> m_pVMRFC;
	CComQIPtr<IVMRMixerControl9> m_pVMRMC;
	CComQIPtr<IVMRMixerBitmap9> m_pVMRMB;


	//�� ��� �ð�(ms)
	double			m_duration;

	double			m_frame_rate;
	CString			m_media_info_string;
	CSize			m_video_size;

	bool			m_volume_mute;
	int				m_volume;

	bool			m_mirror;
	bool			m_flip;

	BOOL VerifyVMR9(void);
	HRESULT GetUnconnectedPin(
		IGraphBuilder *pGB,
		IBaseFilter *pFilter,   // Pointer to the filter.
		PIN_DIRECTION PinDir,   // Direction of the pin to find.
		IPin **ppPin) ;          // Receives a pointer to the pin.

	HRESULT GetUnConnectPin( IGraphBuilder *pGB,
		IBaseFilter *pFilter,   // Pointer to the filter.
		PIN_DIRECTION PinDir,   // Direction of the pin to find.
		IPin **ppPin,int &num);           // Receives a pointer to the pin.)

	HRESULT RenderFileToVMR9(IGraphBuilder *pGB, WCHAR *wFileName, 
		IBaseFilter *pRenderer, BOOL bRenderAudio=TRUE);

	HRESULT AddGraphToRot(IUnknown *pUnkGraph, DWORD *pdwRegister);
	void RemoveGraphFromRot(DWORD pdwRegister);

	int						m_video_stream_index;
	int						m_audio_stream_index;
	void					analyze_stream(IBaseFilter *pBaseFilter);

	int						m_nFilter;				// ���� ������ ��
	BOOL					m_bFilter[10];			// ��������� ������ ��������...
	CString					m_sFilter[10];			// ���� �̸���
	int						m_nAudioFilter;
	CString					m_sAudioFilter[50];
	void					FindAudioRenderer();

	void EnumFilters();
	IBaseFilter* FindFilterByNameInGraph( CString sFilter );
	HRESULT CreateFilter(REFCLSID clsid, IBaseFilter **ppFilter);
	HRESULT RenderOutputPins(IGraphBuilder *pGB, IBaseFilter *pFilter);
	HRESULT EnumFilters(IEnumMoniker *pEnumCat);
	HRESULT FindFilter(CString compFiterName, REFCLSID clsID, IBaseFilter **ppSrcFilter);
};
