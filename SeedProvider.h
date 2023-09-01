#ifndef _CRYPTOPROVIDER_SEEDPROVIDER_H_
#define _CRYPTOPROVIDER_SEEDPROVIDER_H_

#pragma once

#include <afxwin.h>
#include <string>
#include <WTypes.h>

/* usage. ////////////////////////////////////////////////////
SeedProvider	m_sp;
m_sp.SetHeader( "K-RIVER" );
m_sp.SetKey( "&!^@&#*$^&*!&@#^" );
//string encryption
char str[1024] = "void CTest_EncryptDlg::OnSysCommand(UINT nID, LPARAM lParam)\0";
m_sp.Encrypt( str, true );
m_sp.Decrypt( str, false );
//file encryption
m_sp.EncryptFileWithHeader( "d:\\untitled.png", false, "d:\\untitled_enc.png" );
m_sp.DecryptFileWithHeader( "d:\\untitled_enc.png", false, "d:\\untitled_dec.png" );
*/

//scpark
//기존 코드에 최대 8바이트의 헤더(file signature)를 추가하여 암호화 된 파일인지 복호화 된 파일인지 검사할 수 있도록 변경함.
//이미 암호화 된 파일을 다시 암호화하는 오류를 방지함.
//raw data가 아닌 파일을 직접 암호화, 복호화 하도록 함수 추가
//1:1 변환이라서 byte수가 중요하므로 char 타입을 절대 TCHAR 타입으로의 시도는 하지말것!!
#define MAX_HEADER_SIZE		16
#define MAX_KEY_SIZE		16


class SeedProvider
{
public:
	SeedProvider(void);
	~SeedProvider(void);

	void	SetShowError(bool bShow) { m_bShowError = bShow; }

private:
	char	m_header[MAX_HEADER_SIZE + 1];	//max 8 characters file signature
	char	m_key[MAX_KEY_SIZE + 1];		//key characters for encryption
	int		m_nLastError;
	bool	m_bShowError;					//default = true

public:
	enum SP_ERROR_CODE
	{
		SP_ERROR_NO_ERROR = 1,			//정상 완료
		SP_ERROR_FILE_NOT_FOUND,
		SP_ERROR_OPEN_FAIL,				//파일을 열 수 없는 경우
		SP_ERROR_WRITE_FAIL,			//파일을 새로 만들 수 없는 경우
		SP_ERROR_NO_HEADER,				//필수 헤더 정보가 입력되지 않은 경우. SetHeader를 통해 반드시 설정해야 함.
		SP_ERROR_INVALID_HEADER,		//복호화 하려는 파일의 헤더가 맞지 않는 경우
		SP_ERROR_NO_KEY,				//필수 키 정보가 입력되지 않은 경우. SetKey를 통해 반드시 설정해야 함.
		SP_ERROR_INVALID_KEY,			//필수 키 정보가 잘못 입력된 경우. 그 길이를 16으로 fix했다.
		SP_ERROR_ALREADY_ENCRYPTED,		//이미 암호화 된 파일을 암호화 하려는 경우
		SP_ERROR_ENCRYPT_FAIL,			//암호화 실패
		SP_ERROR_DECRYPT_FAIL,			//복호화 실패
		SP_ERROR_INVALID_PARAM,			//파라미터가 잘못된 경우(bOverwrite = false인 경우 sDestFile = ""이면 안됨)
	};

	void SetHeader(char* header);
	void SetKey(char* key);

	bool IsEncryptedHeader(char* header);

	//순수 데이터를 암호화, 복호화 함.
	void Encrypt(char* str, int size, bool bEncrypt);
	void Encrypt(CString& str, bool encrypt);

	//파일을 암호화 함. overwrite = true이면 sDstFile이 무시되고 sSrcFile에 덮어쓴다.
	int	EncryptFileWithHeader(CString src_file, bool overwrite, CString dst_file = _T(""));
	//파일을 복호화 함. overwrite = true이면 sDestFile이 무시되고 sSrcFile에 덮어쓴다.
	int DecryptFileWithHeader(CString src_file, bool overwrite, CString dst_file = _T(""));
	//해당 파일을 복호화하고 덮어쓴다.
	int DecryptFileWithHeader(char* sfile);

	CString	GetLastErrorString();
};

#endif //_CRYPTOPROVIDER_SEEDPROVIDER_H_
