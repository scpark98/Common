//Sound.h 파일 

#if !defined(_DXSOUND_H__INCLUDED_)
#define _DXSOUND_H__INCLUDED_

#include <afxwin.h>
//#include <windows.h>
#include <mmsystem.h>	// 이 헤더파일은 windows.h에 정의한 매크로영향을 받기 
						// 때문에 반드시 windows.h를 포함된 다음에 인클루드 해 줘야 한다.
#include <dsound.h>

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef DSBCAPS_CTRLDEFAULT  //6.0이후 부터 없어졌으므로 define해줌 
#define DSBCAPS_CTRLDEFAULT  (DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME)
#endif 

//입력값을 dB(decibel)로 바꿔주는 매크로 함수
#define DSVOLUME_TO_DB(volume)   ((DWORD)(-30*(100-volume)))  

/////////////////////////////////////////////////////////////////////////////
// CDxSound

class CDxSound 
{
public:
	CDxSound();
	~CDxSound();
	
	LPDIRECTSOUND8		m_lpDSound;		//다이렉트 사운드개체
	LPDIRECTSOUNDBUFFER	m_lpDSBuffer;

	double				m_dDuration;
	BOOL                m_bPlaying;

	BOOL CreateDirectSound(HWND hWnd);
	BOOL Open( HWND hWnd, CString sFileName, BOOL bEncrypt = FALSE );
	LONG GetVolume();
	BOOL SetVolume(LONG lVolume);
	BOOL SetPan(LONG lPan);

	void DeleteDirectSound();
	void Play(BOOL Loop = FALSE);
	void Stop();
	BOOL IsValid() { return m_bValid; }

protected:
	BOOL	m_bValid;
};

#endif
