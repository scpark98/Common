//Sound.h ���� 

#if !defined(_DXSOUND_H__INCLUDED_)
#define _DXSOUND_H__INCLUDED_

#include <afxwin.h>
//#include <windows.h>
#include <mmsystem.h>	// �� ��������� windows.h�� ������ ��ũ�ο����� �ޱ� 
						// ������ �ݵ�� windows.h�� ���Ե� ������ ��Ŭ��� �� ��� �Ѵ�.
#include <dsound.h>

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef DSBCAPS_CTRLDEFAULT  //6.0���� ���� ���������Ƿ� define���� 
#define DSBCAPS_CTRLDEFAULT  (DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME)
#endif 

//�Է°��� dB(decibel)�� �ٲ��ִ� ��ũ�� �Լ�
#define DSVOLUME_TO_DB(volume)   ((DWORD)(-30*(100-volume)))  

/////////////////////////////////////////////////////////////////////////////
// CDxSound

class CDxSound 
{
public:
	CDxSound();
	~CDxSound();
	
	LPDIRECTSOUND8		m_lpDSound;		//���̷�Ʈ ���尳ü
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
