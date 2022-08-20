//Sound.cpp 파일 

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

//함수명 : CreateDirectSound() 
//설명   : DirectSound 객체를 생성하고 협력레벨을 설정한다.
BOOL CDxSound::CreateDirectSound(HWND hWnd)
{
	if ( m_lpDSound )
		DeleteDirectSound();

	//다이렉트 사운드 개체 생성 
    if( DirectSoundCreate8(NULL, &m_lpDSound, NULL) != DS_OK )
        return FALSE;
	
    //협력수준 설정- DSSCL_NORMAL로 설정 
    if( m_lpDSound->SetCooperativeLevel( hWnd, DSSCL_NORMAL ) != DS_OK )
        return FALSE;
	
	
    return TRUE;
}


//함수명 : DeleteDirectSound() 
//설명   : DirectSound 객체를 해제한다.
void CDxSound::DeleteDirectSound()
{
	if ( m_lpDSound )
	{
		m_lpDSound->Release();
		m_lpDSound = NULL;
	}
}


//함수명 : LoadWave() 
//설명   : 파일로 부터 wav파일을 읽어 메모리에 로드한다.
BOOL CDxSound::Open( HWND hWnd, CString sFileName, BOOL bEncrypt/* = TRUE*/ )
{
	if ( bEncrypt )
		FileEncryption( sFileName, 0 );
	
	HMMIO			hmmio;              //wave파일의 핸들 
	MMCKINFO		ckInRIFF, ckIn;  //부모 청크 , 자식 청크 
	PCMWAVEFORMAT	pcmWaveFormat;
	WAVEFORMATEX*	pWaveFormat = NULL;      

	BYTE*   pData = NULL;
    VOID* pBuff1 = NULL;  //사운드 버퍼의 첫번째 영역주소  
    VOID* pBuff2 = NULL;  //사운드 버퍼의 두번째 영역주소 
    DWORD dwLength;      //첫번째 영역크기        
    DWORD dwLength2;     //두번째 영역크기 
	
	m_bValid = FALSE;

	if ( !CreateDirectSound( hWnd ) )
		goto OpenFail;

	//웨이브 파일을 열어, MMIO 핸들을 얻는다.
	hmmio = mmioOpen((TCHAR*)(LPCTSTR)sFileName, NULL, MMIO_ALLOCBUF|MMIO_READ);
    
	if ( hmmio == NULL )
		goto OpenFail;
		//return FALSE;
    
	//내려갈 하위 청크이름을 등록하고, 현재 위치인 RIFF청크에서 WAVE청크를 
	
	//찾아 내려간다.
	ckInRIFF.fccType = mmioFOURCC('W', 'A', 'V', 'E');
	if( (mmioDescend(hmmio, &ckInRIFF, NULL, MMIO_FINDRIFF)) != 0 )
	{
		
		mmioClose(hmmio, 0);  //실패하면 열려있는 웨이브파일을 닫고 리턴(꼭 해준다.)
		goto OpenFail;
		//return FALSE;
	}
	//내려갈 하위 청크이름을 등록하고, 현재 위치인 WAVE청크에서 fmt 청크를 찾아 내려간다.
	//주의: 모든 청크는 4개의 문자코드를 갖기 때문에 t 다음에 공백문자가 있다.
	ckIn.ckid = mmioFOURCC('f', 'm', 't', ' '); 
	if( mmioDescend(hmmio, &ckIn, &ckInRIFF, MMIO_FINDCHUNK) != 0)
	{
		
		mmioClose(hmmio, 0);//실패하면 열려있는 웨이브파일을 닫고 리턴(꼭 해준다.)
		goto OpenFail;
		//return FALSE; 
	}
	
	//fmt 청크에서 wav파일 포맷(Format)을 읽어 들인다.
	if( mmioRead(hmmio, (HPSTR) &pcmWaveFormat, sizeof(pcmWaveFormat))
		
		!= sizeof(pcmWaveFormat) )
	{
		
		mmioClose(hmmio, 0);//실패하면 열려있는 웨이브파일을 닫고 리턴(꼭 해준다.)
		goto OpenFail;
		//return FALSE;
	}
	
	//WAVEFORMATEX를 메모리에 할당 
	pWaveFormat = new WAVEFORMATEX;
	
	//PCMWAVEFORMAT로부터 복사한다.
    memcpy( pWaveFormat, &pcmWaveFormat, sizeof(pcmWaveFormat) );
    pWaveFormat->cbSize = 0;
	//pWaveFormat->nAvgBytesPerSec
	
	
	
	//fmt Chunk 에서 부모청크인 WAVE Chunk로 올라간다.
	if( mmioAscend(hmmio, &ckIn, 0) )
	{
		
		mmioClose(hmmio, 0);//실패하면 열려있는 웨이브파일을 닫고 리턴(꼭 해준다.)
		goto OpenFail;
		//return FALSE;
	}
	
	//내려갈 하위 청크이름을 등록하고, 현재 위치인 WAVE청크에서 data 청크를 
	
	//찾아 내려간다.
	ckIn.ckid = mmioFOURCC('d', 'a', 't', 'a');
	if( mmioDescend(hmmio, &ckIn, &ckInRIFF, MMIO_FINDCHUNK) != 0 )
	{
		
		mmioClose(hmmio, 0);//실패하면 열려있는 웨이브파일을 닫고 리턴(꼭 해준다.)
		goto OpenFail;
		//return FALSE;
	}
	
	//data chunk 사이즈 만큼 메모리 할당
	pData = new BYTE[ckIn.cksize] ;
	
	m_dDuration = (double)(ckIn.cksize) / (double)(pWaveFormat->nAvgBytesPerSec);
	
	
	//data chunk에 있는 순수한 wave data를 읽어 들인다. 
	mmioRead(hmmio, (LPSTR)pData, ckIn.cksize);
	
	//여기까지 왔으면 wav파일읽기에 성공한 것이므로, 열려있는 wav파일을 닫는다. 
	mmioClose(hmmio, 0);
	
	
	
	// DSBUFFERDESC 구조체 정보를 채운다.
    DSBUFFERDESC dsbd;
    ZeroMemory( &dsbd, sizeof(DSBUFFERDESC) );
    dsbd.dwSize        = sizeof(DSBUFFERDESC);
    dsbd.dwFlags       = DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLDEFAULT | DSBCAPS_STATIC | DSBCAPS_LOCSOFTWARE ;
	
    dsbd.dwBufferBytes = ckIn.cksize;
	
    dsbd.lpwfxFormat   = pWaveFormat;
    
	//사운드 버퍼의 생성
    if( m_lpDSound->CreateSoundBuffer( &dsbd, &m_lpDSBuffer, NULL ) != DS_OK )
		goto OpenFail;
		//return FALSE;
    
	
	
	//사운드 버퍼에 순수한 wav데이터를 복사하기 위해 락을 건다.
    if( (m_lpDSBuffer)->Lock( 0, dsbd.dwBufferBytes, &pBuff1, &dwLength, &pBuff2, &dwLength2, 0L ) != DS_OK )
    {
		
        (m_lpDSBuffer)->Release();
		
        (m_lpDSBuffer) = NULL;
		
        goto OpenFail;
		//return FALSE;
		
	}
	
    
	
	memcpy( pBuff1, pData, dwLength );                     //버퍼의 첫번째 영역을 복사 
	
	memcpy( pBuff2, (pData+dwLength), dwLength2); //버퍼의 두번째 영역을 복사
	
    //잠금 상태를 풀어준다.
    (m_lpDSBuffer)->Unlock(pBuff1, dwLength, pBuff2, dwLength2 );
	pBuff1 = pBuff2= NULL;
	
	//할당된 메모리를 해제
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


//함수명 : Play()
//설명   : 해당 사운드를 플레이 한다.
void CDxSound::Play(BOOL Loop)
{
	//버퍼가 비어있으면 종료 
	if ( m_lpDSBuffer == NULL || !m_bValid )
		return;
	
	Stop();
	
	//재생중 실패하면 종료 
	if( !m_lpDSBuffer->Play( 0, 0, (Loop) ? 1 :0 ) )  return;
	
	m_bPlaying = TRUE; 
}


//함수명 : Stop()
//설명   : 해당 사운드를 멈춘다.
void CDxSound::Stop()
{
	//버퍼가 비어있으면 종료 
    if(m_lpDSBuffer == NULL || !m_bValid )
		return;    
	
    m_lpDSBuffer->Stop();  //멈춤 
	m_bPlaying = FALSE;
	m_lpDSBuffer->SetCurrentPosition(0L); //처음위치로
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

//함수명 : SetVolume()
//설명   : 해당 사운드의 볼륨을 조절한다.(100이면 최대출력, 0이면 무음)
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

//함수명 : SetVolume()
//설명   : 스테레오 패닝조절(범위는 -10000~10000)
BOOL CDxSound::SetPan(LONG lPan)
{
	if( m_lpDSBuffer->SetPan(lPan) != DS_OK )
		return FALSE;

    return TRUE;
}
