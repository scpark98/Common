//Sound.cpp ���� 

#include "StdAfx.h"
#include "DxSound.h"
#include "../Functions.h"
#include <MATH.H>

#pragma comment(lib,"winmm.lib")  
#pragma comment(lib,"dsound.lib")  


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CDxSound::CDxSound()
{
	m_lpDSound		= NULL;
	m_lpDSBuffer	= NULL;
	m_bPlaying		= FALSE;
	m_bValid		= FALSE;
}

CDxSound::~CDxSound()
{
	if ( m_lpDSound )
	{
		m_lpDSound->Release();
		m_lpDSound = NULL;
	}
}

//�Լ��� : CreateDirectSound() 
//����   : DirectSound ��ü�� �����ϰ� ���·����� �����Ѵ�.
BOOL CDxSound::CreateDirectSound(HWND hWnd)
{
	if ( m_lpDSound )
		DeleteDirectSound();

	//���̷�Ʈ ���� ��ü ���� 
    if( DirectSoundCreate8(NULL, &m_lpDSound, NULL) != DS_OK )
        return FALSE;
	
    //���¼��� ����- DSSCL_NORMAL�� ���� 
    if( m_lpDSound->SetCooperativeLevel( hWnd, DSSCL_NORMAL ) != DS_OK )
        return FALSE;
	
	
    return TRUE;
}


//�Լ��� : DeleteDirectSound() 
//����   : DirectSound ��ü�� �����Ѵ�.
void CDxSound::DeleteDirectSound()
{
	if ( m_lpDSound )
	{
		m_lpDSound->Release();
		m_lpDSound = NULL;
	}
}


//�Լ��� : LoadWave() 
//����   : ���Ϸ� ���� wav������ �о� �޸𸮿� �ε��Ѵ�.
BOOL CDxSound::Open( HWND hWnd, CString sFileName, BOOL bEncrypt/* = TRUE*/ )
{
	if ( bEncrypt )
		FileEncryption( sFileName, 0 );
	
	HMMIO			hmmio;              //wave������ �ڵ� 
	MMCKINFO		ckInRIFF, ckIn;  //�θ� ûũ , �ڽ� ûũ 
	PCMWAVEFORMAT	pcmWaveFormat;
	WAVEFORMATEX*	pWaveFormat = NULL;      

	BYTE*   pData = NULL;
    VOID* pBuff1 = NULL;  //���� ������ ù��° �����ּ�  
    VOID* pBuff2 = NULL;  //���� ������ �ι�° �����ּ� 
    DWORD dwLength;      //ù��° ����ũ��        
    DWORD dwLength2;     //�ι�° ����ũ�� 
	
	m_bValid = FALSE;

	if ( !CreateDirectSound( hWnd ) )
		goto OpenFail;

	//���̺� ������ ����, MMIO �ڵ��� ��´�.
	hmmio = mmioOpen((TCHAR*)(LPCTSTR)sFileName, NULL, MMIO_ALLOCBUF|MMIO_READ);
    
	if ( hmmio == NULL )
		goto OpenFail;
		//return FALSE;
    
	//������ ���� ûũ�̸��� ����ϰ�, ���� ��ġ�� RIFFûũ���� WAVEûũ�� 
	
	//ã�� ��������.
	ckInRIFF.fccType = mmioFOURCC('W', 'A', 'V', 'E');
	if( (mmioDescend(hmmio, &ckInRIFF, NULL, MMIO_FINDRIFF)) != 0 )
	{
		
		mmioClose(hmmio, 0);  //�����ϸ� �����ִ� ���̺������� �ݰ� ����(�� ���ش�.)
		goto OpenFail;
		//return FALSE;
	}
	//������ ���� ûũ�̸��� ����ϰ�, ���� ��ġ�� WAVEûũ���� fmt ûũ�� ã�� ��������.
	//����: ��� ûũ�� 4���� �����ڵ带 ���� ������ t ������ ���鹮�ڰ� �ִ�.
	ckIn.ckid = mmioFOURCC('f', 'm', 't', ' '); 
	if( mmioDescend(hmmio, &ckIn, &ckInRIFF, MMIO_FINDCHUNK) != 0)
	{
		
		mmioClose(hmmio, 0);//�����ϸ� �����ִ� ���̺������� �ݰ� ����(�� ���ش�.)
		goto OpenFail;
		//return FALSE; 
	}
	
	//fmt ûũ���� wav���� ����(Format)�� �о� ���δ�.
	if( mmioRead(hmmio, (HPSTR) &pcmWaveFormat, sizeof(pcmWaveFormat))
		
		!= sizeof(pcmWaveFormat) )
	{
		
		mmioClose(hmmio, 0);//�����ϸ� �����ִ� ���̺������� �ݰ� ����(�� ���ش�.)
		goto OpenFail;
		//return FALSE;
	}
	
	//WAVEFORMATEX�� �޸𸮿� �Ҵ� 
	pWaveFormat = new WAVEFORMATEX;
	
	//PCMWAVEFORMAT�κ��� �����Ѵ�.
    memcpy( pWaveFormat, &pcmWaveFormat, sizeof(pcmWaveFormat) );
    pWaveFormat->cbSize = 0;
	//pWaveFormat->nAvgBytesPerSec
	
	
	
	//fmt Chunk ���� �θ�ûũ�� WAVE Chunk�� �ö󰣴�.
	if( mmioAscend(hmmio, &ckIn, 0) )
	{
		
		mmioClose(hmmio, 0);//�����ϸ� �����ִ� ���̺������� �ݰ� ����(�� ���ش�.)
		goto OpenFail;
		//return FALSE;
	}
	
	//������ ���� ûũ�̸��� ����ϰ�, ���� ��ġ�� WAVEûũ���� data ûũ�� 
	
	//ã�� ��������.
	ckIn.ckid = mmioFOURCC('d', 'a', 't', 'a');
	if( mmioDescend(hmmio, &ckIn, &ckInRIFF, MMIO_FINDCHUNK) != 0 )
	{
		
		mmioClose(hmmio, 0);//�����ϸ� �����ִ� ���̺������� �ݰ� ����(�� ���ش�.)
		goto OpenFail;
		//return FALSE;
	}
	
	//data chunk ������ ��ŭ �޸� �Ҵ�
	pData = new BYTE[ckIn.cksize] ;
	
	m_dDuration = (double)(ckIn.cksize) / (double)(pWaveFormat->nAvgBytesPerSec);
	
	
	//data chunk�� �ִ� ������ wave data�� �о� ���δ�. 
	mmioRead(hmmio, (LPSTR)pData, ckIn.cksize);
	
	//������� ������ wav�����б⿡ ������ ���̹Ƿ�, �����ִ� wav������ �ݴ´�. 
	mmioClose(hmmio, 0);
	
	
	
	// DSBUFFERDESC ����ü ������ ä���.
    DSBUFFERDESC dsbd;
    ZeroMemory( &dsbd, sizeof(DSBUFFERDESC) );
    dsbd.dwSize        = sizeof(DSBUFFERDESC);
    dsbd.dwFlags       = DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLDEFAULT | DSBCAPS_STATIC | DSBCAPS_LOCSOFTWARE ;
	
    dsbd.dwBufferBytes = ckIn.cksize;
	
    dsbd.lpwfxFormat   = pWaveFormat;
    
	//���� ������ ����
    if( m_lpDSound->CreateSoundBuffer( &dsbd, &m_lpDSBuffer, NULL ) != DS_OK )
		goto OpenFail;
		//return FALSE;
    
	
	
	//���� ���ۿ� ������ wav�����͸� �����ϱ� ���� ���� �Ǵ�.
    if( (m_lpDSBuffer)->Lock( 0, dsbd.dwBufferBytes, &pBuff1, &dwLength, &pBuff2, &dwLength2, 0L ) != DS_OK )
    {
		
        (m_lpDSBuffer)->Release();
		
        (m_lpDSBuffer) = NULL;
		
        goto OpenFail;
		//return FALSE;
		
	}
	
    
	
	memcpy( pBuff1, pData, dwLength );                     //������ ù��° ������ ���� 
	
	memcpy( pBuff2, (pData+dwLength), dwLength2); //������ �ι�° ������ ����
	
    //��� ���¸� Ǯ���ش�.
    (m_lpDSBuffer)->Unlock(pBuff1, dwLength, pBuff2, dwLength2 );
	pBuff1 = pBuff2= NULL;
	
	//�Ҵ�� �޸𸮸� ����
	delete[] pData;
	delete pWaveFormat;

	if ( bEncrypt )
		FileEncryption( sFileName );

	m_bValid = TRUE;
    return m_bValid;
	
OpenFail : 
	if ( bEncrypt )
		FileEncryption( sFileName );

	if ( m_lpDSound )
		DeleteDirectSound();

	delete[] pData;
	delete pWaveFormat;

	m_bValid = FALSE;
	return m_bValid;
}


//�Լ��� : Play()
//����   : �ش� ���带 �÷��� �Ѵ�.
void CDxSound::Play(BOOL Loop)
{
	//���۰� ��������� ���� 
	if ( m_lpDSBuffer == NULL || !m_bValid )
		return;
	
	Stop();
	
	//����� �����ϸ� ���� 
	if( !m_lpDSBuffer->Play( 0, 0, (Loop) ? 1 :0 ) )  return;
	
	m_bPlaying = TRUE; 
}


//�Լ��� : Stop()
//����   : �ش� ���带 �����.
void CDxSound::Stop()
{
	//���۰� ��������� ���� 
    if(m_lpDSBuffer == NULL || !m_bValid )
		return;    
	
    m_lpDSBuffer->Stop();  //���� 
	m_bPlaying = FALSE;
	m_lpDSBuffer->SetCurrentPosition(0L); //ó����ġ��
}

LONG CDxSound::GetVolume()
{
	LONG volume;

	if ( !m_lpDSBuffer )
		return 0;

	if ( m_lpDSBuffer->GetVolume( &volume ) != DS_OK )
		return 0;

	if ( DSBVOLUME_MAX == DSBVOLUME_MIN )
		return 0;

	volume = min((int)(pow(10, (volume - DSBVOLUME_MAX / 2000.0)) * 255.0), 255);

	return volume;
}

//�Լ��� : SetVolume()
//����   : �ش� ������ ������ �����Ѵ�.(100�̸� �ִ����, 0�̸� ����)
BOOL CDxSound::SetVolume(LONG lVolume)
{
	if (!m_lpDSBuffer || !m_bValid )
		return FALSE;

	if (lVolume <= 0) lVolume = DSBVOLUME_MIN;
	else if (lVolume >= 255) lVolume = DSBVOLUME_MAX;
	else lVolume = max(DSBVOLUME_MIN, (long)(DSBVOLUME_MAX + 2000.0*log10(lVolume/255.0)));
	if (m_lpDSBuffer->SetVolume(lVolume) != DS_OK) return FALSE;
	return TRUE;
}

//�Լ��� : SetVolume()
//����   : ���׷��� �д�����(������ -10000~10000)
BOOL CDxSound::SetPan(LONG lPan)
{
	if( m_lpDSBuffer->SetPan(lPan) != DS_OK )
		return FALSE;

    return TRUE;
}
