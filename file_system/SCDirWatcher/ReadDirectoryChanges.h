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


#pragma once

#include <memory>
#include <string>
#include <thread>
#include "ThreadSafeQueue.h"

using TDirectoryChangeNotification = std::pair<DWORD, std::wstring>;

namespace ReadDirectoryChangesPrivate
{
    class CReadChangesServer;
}

///////////////////////////////////////////////////////////////////////////


/// <summary>
/// Track changes to filesystem directories and report them
/// to the caller via a thread-safe queue.
/// </summary>
/// <remarks>
/// <para>
/// This sample code is based on my blog entry titled, "Understanding ReadDirectoryChangesW"
///	http://qualapps.blogspot.com/2010/05/understanding-readdirectorychangesw.html
/// </para><para>
/// All functions in CReadDirectoryChangesServer run in
/// the context of the calling thread.
/// </para>
/// <example><code>
/// 	CReadDirectoryChanges changes;
/// 	changes.AddDirectory(_T("C:\\"), false, dwNotificationFlags);
///
///		const HANDLE handles[] = { hStopEvent, changes.GetWaitHandle() };
///
///		while (!bTerminate)
///		{
///			::MsgWaitForMultipleObjectsEx(
///				_countof(handles),
///				handles,
///				INFINITE,
///				QS_ALLINPUT,
///				MWMO_INPUTAVAILABLE | MWMO_ALERTABLE);
///			switch (rc)
///			{
///			case WAIT_OBJECT_0 + 0:
///				bTerminate = true;
///				break;
///			case WAIT_OBJECT_0 + 1:
///				// We've received a notification in the queue.
///				{
///					DWORD dwAction;
///					std::wstring wstrFilename;
///					changes.Pop(dwAction, wstrFilename);
///					wprintf(L"%s %s\n", ExplainAction(dwAction), wstrFilename);
///				}
///				break;
///			case WAIT_OBJECT_0 + _countof(handles):
///				// Get and dispatch message
///				break;
///			case WAIT_IO_COMPLETION:
///				// APC complete.No action needed.
///				break;
///			}
///		}
/// </code></example>
/// </remarks>
class CReadDirectoryChanges
{
public:
    CReadDirectoryChanges(int nMaxChanges = 1000);

    // This is a multithreaded object, so don't allow copies.
    CReadDirectoryChanges(const CReadDirectoryChanges&) = delete;
    CReadDirectoryChanges& operator=(const CReadDirectoryChanges&) = delete;

    ~CReadDirectoryChanges();

    void Init();
    void Terminate();

    /// <summary>
    /// Add a new directory to be monitored.
    /// </summary>
    /// <param name="wszDirectory">Directory to monitor.</param>
    /// <param name="bWatchSubtree">True to also monitor subdirectories.</param>
    /// <param name="dwNotifyFilter">The types of file system events to monitor, such as FILE_NOTIFY_CHANGE_ATTRIBUTES.</param>
    /// <param name="dwBufferSize">The size of the buffer used for overlapped I/O.</param>
    /// <remarks>
    /// <para>
    /// This function will make an APC call to the worker thread to issue a new
    /// ReadDirectoryChangesW call for the given directory with the given flags.
    /// </para>
    /// </remarks>
    void AddDirectory(LPCTSTR wszDirectory, bool bWatchSubtree, DWORD dwNotifyFilter, DWORD dwBufferSize = 16384);

    //scpark add
	void    stop_watching_directory(LPCTSTR wszDirectory);
    int     get_watching_count();
    bool    is_watching(LPCTSTR wszDirectory);

    /// <summary>
    /// Return a handle for the Win32 Wait... functions that will be
    /// signaled when there is a queue entry.
    /// </summary>
    HANDLE GetWaitHandle() { return m_Notifications.GetWaitHandle(); }

    bool Pop(DWORD& dwAction, std::wstring& wstrFilename);

    // "Push" is for usage by ReadChangesRequest.  Not intended for external usage.
    void Push(DWORD dwAction, std::wstring& wstrFilename);

    // Check if the queue overflowed. If so, clear it and return true.
    bool CheckOverflow();

protected:
    std::unique_ptr<ReadDirectoryChangesPrivate::CReadChangesServer> m_pServer;

    std::thread m_Thread;

    unsigned int m_dwThreadId{};

    CThreadSafeQueue<TDirectoryChangeNotification> m_Notifications;

    HANDLE GetThreadHandle();
};
