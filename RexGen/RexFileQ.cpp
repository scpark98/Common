#include "StdAfx.h"
#include "RexFileQ.h"

const int TOTAL_SEGMENT = 16;
const int IO_SLEEP = 10;

RexFileQ::RexFileQ(void)
{
	m_bRunthread = true;
	m_iCount = 0;
}


RexFileQ::~RexFileQ(void)
{
	m_bRunthread = false;

	while(m_image_mem_q.empty() == false)
	{
		delete m_image_mem_q.front();
		m_image_mem_q.pop();
	}
	while(m_image_file_q.empty() == false)
	{
		delete m_image_file_q.front();
		m_image_file_q.pop();
	}

	Sleep(10);
}

int RexFileQ::Init(const char* fq_path, bool clear, void (*p_func)(REXFILEQ_DATA*), int max_mq, int max_fq)
{
	strcpy_s(m_sFileQPath, fq_path);
	m_pProc_func = p_func;
	m_iMaxMQ = max_mq;
	m_iMaxFQ  = max_fq;

	//Check FQ folder
	DWORD ftyp = GetFileAttributesA(fq_path);	

	if((ftyp & FILE_ATTRIBUTE_DIRECTORY) == 0)
		return -1; //is not a folder

	if (ftyp == INVALID_FILE_ATTRIBUTES) // path doesn't exist
	{//so make it!
		if(CreateDirectory(fq_path, NULL) == FALSE)
			return -2;
	}
	//Clean FQ folder
	if(clear)
	{
		CFileFind hFile;
		CString t_path = fq_path;
		BOOL bFound = hFile.FindFile( t_path + _T("\\*.*") );
		CString name;

		int count = 0;
		while ( bFound )   
		{
			bFound = hFile.FindNextFile();
			if( (!hFile.IsDots()) && ( !hFile.IsDirectory() ) )		
			{
				name = hFile.GetFileName();				
				DeleteFile(GetFullPath(name));
			}
		}
	}

	m_bRunthread = true;
	m_pFileSaveThread = ::AfxBeginThread(RexFileQ::FileSaveThread, this, THREAD_PRIORITY_NORMAL, 0, 0, NULL);		

	if(p_func)
		m_pProcThread = ::AfxBeginThread(RexFileQ::ProcThread, this, THREAD_PRIORITY_NORMAL, 0, 0, NULL);

	return 0;
}

int RexFileQ::PushData(REXFILEQ_DATA* data)
{
	m_csImage.Lock();	
	m_iCount++;
	if(m_iCount > 99999999)
		m_iCount = 0;

	CString org_name = data->m_sFname;
	sprintf_s(data->m_sFname, "%08d_%s", m_iCount, org_name);	
	
	if((int)m_image_mem_q.size() < m_iMaxMQ || m_iMaxMQ == 0)
		m_image_mem_q.push(data);
	m_csImage.Unlock();

	return 0;
}

REXFILEQ_DATA* RexFileQ::PopData()
{
	REXFILEQ_DATA* ret = NULL;
	//Wait until memQ be empty
	bool bWait = true;
	while(bWait)
	{
		m_csImage.Lock();
		bWait = !m_image_mem_q.empty();
		m_csImage.Unlock();
		Sleep(1);
	}	
	////////////////////////////////////////
	m_csFile.Lock();
	if(m_image_file_q.empty() == false)
	{
		ret = m_image_file_q.front();
		m_image_file_q.pop();
	}
	if(ret)
	{
		CString fname = GetFullPath(ret->m_sFname);	
		try
		{
			FILE *fp = fopen(fname.GetBuffer(), "rb");
			fseek(fp, 0, SEEK_END);
			int file_size = ftell(fp);
			fseek(fp, 0, SEEK_SET);

			int width, height;

			fread(&width, 1, sizeof(int), fp);
			fread(&height, 1, sizeof(int), fp);
			
			const int total_size = file_size - sizeof(int)*2;

			byte *pData = new byte[total_size];
			const int segment_size = total_size / TOTAL_SEGMENT;
			int t_pos = 0;

			for(int i=0; i<TOTAL_SEGMENT; i++)
			{
				if(i < TOTAL_SEGMENT-1)
				{
					fread(pData + t_pos, 1, segment_size, fp);
					t_pos += segment_size;
					Sleep(IO_SLEEP);
				}
				else
					fread(pData + t_pos, 1, total_size - t_pos, fp);
			}

			ret->m_pImage = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 3);
			memcpy(ret->m_pImage->imageData, pData, total_size);
			fclose(fp);
			//
			DeleteFile(fname);
			//return as new name
			CString new_name = ret->GetRemovedSerialFName();
			strcpy(ret->m_sFname, new_name.GetBuffer());
			//cout << "new_name : " << ret->m_sFname  << endl;
		}
		catch (...)
		{

		}
	}
	m_csFile.Unlock();
	return ret;
}

REXFILEQ_DATA* RexFileQ::GetHead(CString *fname) 
{
	//Wait until memQ be empty
	bool bWait = true;
	while(bWait)
	{
		m_csImage.Lock();
		bWait = !m_image_mem_q.empty();
		m_csImage.Unlock();
		Sleep(1);
	}

	REXFILEQ_DATA* ret = NULL;
	m_csFile.Lock();
	if(m_image_file_q.empty() == false)
		ret = m_image_file_q.front();
	m_csFile.Unlock();
	if(ret && fname)
	{
		*fname = ret->GetRemovedSerialFName();		
	}
	return ret;
}

CString RexFileQ::GetFullPath(CString fname)
{
	CString ret = m_sFileQPath;
	ret += "\\" + fname;	
	return ret;
}

UINT RexFileQ::FileSaveThread(LPVOID dParameter)
{
	RexFileQ* pMyQ = (RexFileQ*)dParameter;

	while(pMyQ->m_bRunthread)
	{
		REXFILEQ_DATA* data = NULL;
		
		pMyQ->m_csImage.Lock();		
		if(pMyQ->m_image_mem_q.empty() == false)
		{
			data = pMyQ->m_image_mem_q.front();
			pMyQ->m_image_mem_q.pop();
		}
		pMyQ->m_csImage.Unlock();
		
		if(pMyQ->m_bRunthread == false)
			break;
		
		if(data)
		{
			//Now saving
			pMyQ->m_csFile.Lock();
			if((int)pMyQ->m_image_file_q.size() < pMyQ->m_iMaxFQ || pMyQ->m_iMaxFQ == 0)
			{	
				try
				{
					FILE *fp = fopen(pMyQ->GetFullPath(data->m_sFname).GetBuffer(), "wb");

					fwrite(&data->m_pImage->width, 1, sizeof(int), fp);
					fwrite(&data->m_pImage->height, 1, sizeof(int), fp);

					const int total_size = (((data->m_pImage->height)+3)/4*4) * (((data->m_pImage->width)+3)/4*4) * 3;
					const int segment_size = total_size / TOTAL_SEGMENT;

					int t_pos = 0;
					for(int i=0; i<TOTAL_SEGMENT; i++)
					{
						if(i < TOTAL_SEGMENT-1)
						{
							fwrite(data->m_pImage->imageData + t_pos, 1, segment_size, fp);
							t_pos += segment_size;
							Sleep(IO_SLEEP);
						}
						else
							fwrite(data->m_pImage->imageData + t_pos, 1, total_size - t_pos, fp);
					}
					fclose(fp);

					//cvSaveImage(pMyQ->GetFullPath(data->m_sFname).GetBuffer(), data->m_pImage);
					pMyQ->m_image_file_q.push(new REXFILEQ_DATA(NULL, data->m_sFname, data->m_total));
				}
				catch (...)
				{
										
				}				
			}
			pMyQ->m_csFile.Unlock();
			delete data;
		}
		Sleep(IO_SLEEP);
	}
	return 0;
}

UINT RexFileQ::ProcThread(LPVOID dParameter)
{
	RexFileQ* pMyQ = (RexFileQ*)dParameter;

	while(pMyQ->m_bRunthread)
	{
		REXFILEQ_DATA* data = NULL;

		pMyQ->m_csFile.Lock();
		////////////////////////////////////////
		if(pMyQ->m_image_file_q.empty() == false)
		{
			data = pMyQ->m_image_file_q.front();
			pMyQ->m_image_file_q.pop();
		}

		if(data)
		{
			CString fname = pMyQ->GetFullPath(data->m_sFname);	

			FILE *fp = fopen(fname.GetBuffer(), "rb");
			fseek(fp, 0, SEEK_END);
			int file_size = ftell(fp);
			fseek(fp, 0, SEEK_SET);

			int width, height;

			fread(&width, 1, sizeof(int), fp);
			fread(&height, 1, sizeof(int), fp);

			const int total_size = file_size - sizeof(int)*2;

			byte *pData = new byte[total_size];
			const int segment_size = total_size / TOTAL_SEGMENT;
			int t_pos = 0;

			for(int i=0; i<TOTAL_SEGMENT; i++)
			{
				if(i < TOTAL_SEGMENT-1)
				{
					fread(pData + t_pos, 1, segment_size, fp);
					t_pos += segment_size;
					Sleep(IO_SLEEP);
				}
				else
					fread(pData + t_pos, 1, total_size - t_pos, fp);
			}

			data->m_pImage = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 3);
			memcpy(data->m_pImage->imageData, pData, total_size);

			fclose(fp);
			//data->m_pImage = cvLoadImage(fname);
			DeleteFile(fname);
		}
		pMyQ->m_csFile.Unlock();

		if(data)
		{
			pMyQ->m_pProc_func(data);
			delete data;
		}

		Sleep(IO_SLEEP);
	}

	return 0;
}