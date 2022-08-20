#include "stdafx.h"
#include "SeedProvider.h"

SeedProvider::SeedProvider()
{
	memset( m_header, 0, MAX_HEADER_SIZE + 1 );
	memset( m_key, 0, MAX_KEY_SIZE + 1 );

	m_bShowError = true;
}

SeedProvider::~SeedProvider()
{
}

void SeedProvider::SetHeader( char *header )
{
	int len = strlen(header);
	if ( strlen(header) > MAX_HEADER_SIZE )
	{
		TCHAR str[128];
		_stprintf( str, _T("header(%s) size(%d) is over MAX_HEADER_SIZE(%d)"), header, strlen(header), MAX_HEADER_SIZE );
		if ( m_bShowError )
			AfxMessageBox( str, MB_ICONEXCLAMATION );
		return;
	}

	memcpy( m_header, header, strlen(header) );
	m_nLastError = SP_ERROR_NO_ERROR;
}

void SeedProvider::SetKey( char *key )
{
	if ( strlen(key) > MAX_KEY_SIZE )
	{
		TCHAR str[128];
		_stprintf(str, _T("key(%s) size(%d) is over MAX_KEY_SIZE(%d)"), key, strlen(key), MAX_KEY_SIZE );
		if ( m_bShowError )
			AfxMessageBox( str, MB_ICONEXCLAMATION );
		return;
	}

	memcpy( m_key, key, strlen(key) );
}

bool SeedProvider::IsEncryptedHeader( char *header )
{
	if ( strlen(header) != strlen(m_header) )
		return false;

	return !strncmp( header, m_header, strlen(m_header) );
}


//���� �����͸� ��ȣȭ, ��ȣȭ ��.
void SeedProvider::Encrypt( char* str, int size, bool bEncrypt )
{
	unsigned int j = 0;

	for ( int i = 0; i < size; i++ )
	{
		if ( bEncrypt )
			str[i] += m_key[j++];
		else
			str[i] -= m_key[j++];
		
		if ( j == strlen(m_key) )
		{
			j = 0;
		}
	}
}

void SeedProvider::Encrypt(CString &str, bool encrypt)
{
	unsigned int j = 0;

	for ( int i = 0; i < str.GetLength(); i++ )
	{
		if ( encrypt )
			str.SetAt(i, str[i] + m_key[j++]);
		else
			str.SetAt(i, str[i] - m_key[j++]);

		if ( j == strlen(m_key) )
		{
			j = 0;
		}
	}
	TRACE(_T("%s\n", str));
}

//������ ��ȣȭ, ��ȣȭ ��.
int	SeedProvider::EncryptFileWithHeader( CString sFile, bool bOverwrite, CString sDestFile )
{
	if ( PathFileExists( sFile ) == false )
	{
		m_nLastError = SP_ERROR_FILE_NOT_FOUND;
		if ( m_bShowError )
			AfxMessageBox( GetLastErrorString() );
		return m_nLastError;
	}

	if ( strlen(m_header) == 0 )
	{
		m_nLastError = SP_ERROR_INVALID_HEADER;
		if ( m_bShowError )
			AfxMessageBox( GetLastErrorString() );
		return m_nLastError;
	}
	
	if ( strlen(m_key) == 0 )
	{
		m_nLastError = SP_ERROR_NO_KEY;
		if ( m_bShowError )
			AfxMessageBox( GetLastErrorString() );
		return m_nLastError;
	}

	if ( !bOverwrite && sDestFile == "" )
	{
		m_nLastError = SP_ERROR_INVALID_PARAM;
		if ( m_bShowError )
		{
			AfxMessageBox( GetLastErrorString() );
			AfxMessageBox( "If bOverwrite option is set to false, sDestFile must not be empty string!" );
		}
		return m_nLastError;
	}

	char		header[MAX_HEADER_SIZE];
	ULONGLONG	filesize;
	LPVOID		pdata = NULL;

	//������ �������� ���� ��� ���⿡�� �ý��� ���� �޽����� ��µȴ�.
	//���� �տ��� �̸� ���� ������ üũ�ϰ� �޽��� ��µ� �ɼ����� �޾ƾ� �Ѵ�.
	CFile inFile( sFile, CFile::modeRead|CFile::typeBinary );

	filesize = inFile.GetLength();
	pdata	= new byte[ filesize ];

	inFile.Read( pdata, filesize );
	inFile.Close();

	memcpy( header, pdata, strlen(m_header) );

	if ( IsEncryptedHeader( header) )
	{
		delete [] pdata;

		m_nLastError = SP_ERROR_ALREADY_ENCRYPTED;
		TRACE( "%s\n", GetLastErrorString() );
		if ( m_bShowError )
			AfxMessageBox( GetLastErrorString() );
		return m_nLastError;
	}

	Encrypt( (char *)pdata, filesize, true );

	if ( bOverwrite )
		sDestFile = sFile;

	CFile outFile( sDestFile, CFile::modeCreate|CFile::modeWrite|CFile::typeBinary );
	outFile.Write( m_header, strlen(m_header) );
	outFile.Write( pdata, filesize );
	outFile.Close();

	/*
	FILE *fp = fopen( sDestFile, "wb" );
	fwrite( m_header, 1, strlen(m_header), fp );
	fwrite( pdata, 1, filesize, fp );
	fclose( fp );
	*/

	delete [] pdata;

	m_nLastError = SP_ERROR_NO_ERROR;
	return m_nLastError;
}

int SeedProvider::DecryptFileWithHeader( CString sFile, bool bOverwrite, CString sDestFile )
{
	if ( PathFileExists( sFile ) == false )
	{
		m_nLastError = SP_ERROR_FILE_NOT_FOUND;
		if ( m_bShowError )
			AfxMessageBox( GetLastErrorString() );
		return m_nLastError;
	}

	if ( strlen(m_header) == 0 )
	{
		m_nLastError = SP_ERROR_INVALID_HEADER;
		if ( m_bShowError )
			AfxMessageBox( GetLastErrorString() );
		return m_nLastError;
	}
	
	if ( strlen(m_key) == 0 )
	{
		m_nLastError = SP_ERROR_NO_KEY;
		if ( m_bShowError )
			AfxMessageBox( GetLastErrorString() );
		return m_nLastError;
	}

	if ( !bOverwrite && sDestFile == "" )
	{
		AfxMessageBox( "If bOverwrite option is set to false, sDestFile must not be empty string!" );
		m_nLastError = SP_ERROR_INVALID_PARAM;
		if ( m_bShowError )
			AfxMessageBox( GetLastErrorString() );
		return m_nLastError;
	}

	char		header[MAX_HEADER_SIZE+1] = {0,};
	CFile		inFile( sFile, CFile::modeRead|CFile::typeBinary );
	ULONGLONG	filesize;

	filesize = inFile.GetLength();

	inFile.Read( header, strlen(m_header) );

	if ( IsEncryptedHeader( header ) == false )
	{
		inFile.Close();

		m_nLastError = SP_ERROR_INVALID_HEADER;
		TRACE( "%s\n", GetLastErrorString() );
		if ( m_bShowError )
			AfxMessageBox( GetLastErrorString() );
		return m_nLastError;
	}

	LPVOID pdata = NULL;

	filesize = filesize - strlen(m_header);
	pdata = new byte[filesize];
	inFile.Read( pdata, filesize );

	inFile.Close();

	Encrypt( (char *)pdata, filesize, false );


	if ( bOverwrite )
		sDestFile = sFile;

	CFile outFile( sDestFile, CFile::modeCreate|CFile::modeWrite|CFile::typeBinary );
	outFile.Write( pdata, filesize );
	outFile.Close();

	delete [] pdata;

	m_nLastError = SP_ERROR_NO_ERROR;
	return m_nLastError;
}

//�ش� ������ ��ȣȭ�ϰ� �����.
//NeoDAS �������� ����ϱ� ���� header�� key�� �������� ����ϰ�
//C code�θ� �ۼ���.
int SeedProvider::DecryptFileWithHeader(char *sfile)
{
	char header[17] = "verasys_can_info";
	char key[17] = "����ý�ĵ������";

	char		str[MAX_HEADER_SIZE];
	FILE		*fp = fopen(sfile, "rb");
	uint32_t	filesize;

	struct stat st;
	stat(sfile, &st);
	filesize = st.st_size;

	fread(str, 1, 16, fp);

	if (IsEncryptedHeader(str) == false)
	{
		fclose(fp);
		printf("invalid header or not encrypted file.");
		return 0;
	}

	char *pdata = NULL;

	filesize = filesize - 16;
	pdata = new char[filesize];
	fread(pdata, 1, filesize, fp);
	fclose(fp);

	uint32_t i, j = 0;

	for (i = 0; i < filesize; i++)
	{
		pdata[i] -= key[j++];

		if (j == strlen(key))
			j = 0;
	}

	fp = fopen(sfile, "wb");
	fwrite(pdata, 1, filesize, fp);
	fclose(fp);

	delete[] pdata;
	return 1;
}

CString SeedProvider::GetLastErrorString()
{
	switch( m_nLastError )
	{
		case SP_ERROR_NO_ERROR			:	return _T("no error.");
		case SP_ERROR_FILE_NOT_FOUND	:	return _T("file not found.");
		case SP_ERROR_OPEN_FAIL			:	return _T("can't open file.");
		case SP_ERROR_WRITE_FAIL		:	return _T("can't write file.");
		case SP_ERROR_NO_HEADER			:	return _T("no header string info.");
		case SP_ERROR_INVALID_HEADER	:	return _T("invalid file header or not encrypted file.");
		case SP_ERROR_NO_KEY			:	return _T("no key string info.");
		case SP_ERROR_INVALID_KEY		:	return _T("invalid key string. It's must be 16 characters.");
		case SP_ERROR_ALREADY_ENCRYPTED :	return _T("already encrypted file.");
		case SP_ERROR_ENCRYPT_FAIL		:	return _T("encrypt failed.");
		case SP_ERROR_DECRYPT_FAIL		:	return _T("decrypt failed.");
		case SP_ERROR_INVALID_PARAM		:	return _T("bOverwrite = false, then sDestFile parameter cannot be empty string.");
	}

	return _T("unknow error.");
}

