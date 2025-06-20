//
//	The MIT License
//
//	Copyright (c) 2010 James E Beveridge
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//	THE SOFTWARE.


//	This sample code is for my blog entry titled, "Understanding ReadDirectoryChangesW"
//	http://qualapps.blogspot.com/2010/05/understanding-readdirectorychangesw.html
//	See ReadMe.txt for overview information.

//#include "stdafx.h"
#include "ReadDirectoryChanges.h"
#include "ReadDirectoryChangesPrivate.h"

using namespace ReadDirectoryChangesPrivate;

///////////////////////////////////////////////////////////////////////////
// CReadDirectoryChanges

CReadDirectoryChanges::CReadDirectoryChanges(int nMaxCount)
    : m_Notifications(nMaxCount)
{
    m_pServer = std::make_unique<ReadDirectoryChangesPrivate::CReadChangesServer>(CReadChangesServer(this));
}

CReadDirectoryChanges::~CReadDirectoryChanges()
{
    Terminate();
}

void CReadDirectoryChanges::Init()
{
    //
    // Kick off the worker thread, which will be
    // managed by CReadChangesServer.
    //
    m_Thread = std::thread([this]() { m_pServer->Run(); });
}

void CReadDirectoryChanges::Terminate()
{
    if (m_Thread.joinable())
    {
        ::QueueUserAPC(CReadChangesServer::TerminateProc, GetThreadHandle(), (ULONG_PTR)m_pServer.get());
        m_Thread.join();
    }
}

void CReadDirectoryChanges::AddDirectory(LPCTSTR szDirectory, bool bWatchSubtree, DWORD dwNotifyFilter, DWORD dwBufferSize)
{
    //scpark add. ����͸� �����带 �����Ű�� ����� ã�ƾ� �ϴµ� �켱 ""�� �Ѿ���� watching thread�� �����Ű���� �Ѵ�.
    if (_tcslen(szDirectory) == 0)
    {
        Terminate();
        return;
    }

    if (!m_Thread.joinable())
        Init();

    CReadChangesRequest* pRequest = new CReadChangesRequest(m_pServer.get(), szDirectory, bWatchSubtree, dwNotifyFilter, dwBufferSize);
    QueueUserAPC(CReadChangesServer::AddDirectoryProc, GetThreadHandle(), (ULONG_PTR)pRequest);
}

//scpark add
void CReadDirectoryChanges::stop_watching_directory(LPCTSTR wszDirectory)
{
    m_pServer.get()->stop(wszDirectory);
}

int CReadDirectoryChanges::get_watching_count()
{
    return m_pServer.get()->get_watching_count();
}

bool CReadDirectoryChanges::is_watching(LPCTSTR wszDirectory)
{
    return m_pServer.get()->is_watching(wszDirectory);
}



void CReadDirectoryChanges::Push(DWORD dwAction, std::wstring& wstrFilename)
{
    m_Notifications.push(TDirectoryChangeNotification(dwAction, wstrFilename));
}

bool  CReadDirectoryChanges::Pop(DWORD& dwAction, std::wstring& wstrFilename)
{
    TDirectoryChangeNotification pair;
    if (!m_Notifications.pop(pair))
        return false;

    dwAction = pair.first;
    wstrFilename = pair.second;

    return true;
}

bool CReadDirectoryChanges::CheckOverflow()
{
    bool b = m_Notifications.overflow();
    if (b)
        m_Notifications.clear();
    return b;
}


HANDLE CReadDirectoryChanges::GetThreadHandle()
{
    return reinterpret_cast<HANDLE>(m_Thread.native_handle());
}
