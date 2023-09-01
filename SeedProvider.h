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
//���� �ڵ忡 �ִ� 8����Ʈ�� ���(file signature)�� �߰��Ͽ� ��ȣȭ �� �������� ��ȣȭ �� �������� �˻��� �� �ֵ��� ������.
//�̹� ��ȣȭ �� ������ �ٽ� ��ȣȭ�ϴ� ������ ������.
//raw data�� �ƴ� ������ ���� ��ȣȭ, ��ȣȭ �ϵ��� �Լ� �߰�
//1:1 ��ȯ�̶� byte���� �߿��ϹǷ� char Ÿ���� ���� TCHAR Ÿ�������� �õ��� ��������!!
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
		SP_ERROR_NO_ERROR = 1,			//���� �Ϸ�
		SP_ERROR_FILE_NOT_FOUND,
		SP_ERROR_OPEN_FAIL,				//������ �� �� ���� ���
		SP_ERROR_WRITE_FAIL,			//������ ���� ���� �� ���� ���
		SP_ERROR_NO_HEADER,				//�ʼ� ��� ������ �Էµ��� ���� ���. SetHeader�� ���� �ݵ�� �����ؾ� ��.
		SP_ERROR_INVALID_HEADER,		//��ȣȭ �Ϸ��� ������ ����� ���� �ʴ� ���
		SP_ERROR_NO_KEY,				//�ʼ� Ű ������ �Էµ��� ���� ���. SetKey�� ���� �ݵ�� �����ؾ� ��.
		SP_ERROR_INVALID_KEY,			//�ʼ� Ű ������ �߸� �Էµ� ���. �� ���̸� 16���� fix�ߴ�.
		SP_ERROR_ALREADY_ENCRYPTED,		//�̹� ��ȣȭ �� ������ ��ȣȭ �Ϸ��� ���
		SP_ERROR_ENCRYPT_FAIL,			//��ȣȭ ����
		SP_ERROR_DECRYPT_FAIL,			//��ȣȭ ����
		SP_ERROR_INVALID_PARAM,			//�Ķ���Ͱ� �߸��� ���(bOverwrite = false�� ��� sDestFile = ""�̸� �ȵ�)
	};

	void SetHeader(char* header);
	void SetKey(char* key);

	bool IsEncryptedHeader(char* header);

	//���� �����͸� ��ȣȭ, ��ȣȭ ��.
	void Encrypt(char* str, int size, bool bEncrypt);
	void Encrypt(CString& str, bool encrypt);

	//������ ��ȣȭ ��. overwrite = true�̸� sDstFile�� ���õǰ� sSrcFile�� �����.
	int	EncryptFileWithHeader(CString src_file, bool overwrite, CString dst_file = _T(""));
	//������ ��ȣȭ ��. overwrite = true�̸� sDestFile�� ���õǰ� sSrcFile�� �����.
	int DecryptFileWithHeader(CString src_file, bool overwrite, CString dst_file = _T(""));
	//�ش� ������ ��ȣȭ�ϰ� �����.
	int DecryptFileWithHeader(char* sfile);

	CString	GetLastErrorString();
};

#endif //_CRYPTOPROVIDER_SEEDPROVIDER_H_
