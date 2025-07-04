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

#include <vector>

class CReadDirectoryChanges;

namespace ReadDirectoryChangesPrivate
{

    class CReadChangesServer;

    ///////////////////////////////////////////////////////////////////////////

    // All functions in CReadChangesRequest run in the context of the worker thread.
    // One instance of this object is created for each call to AddDirectory().
    class CReadChangesRequest
    {
    public:
        CReadChangesRequest(CReadChangesServer* pServer, LPCTSTR sz, bool b, DWORD dw, DWORD size);

        ~CReadChangesRequest();

        bool OpenDirectory();

        void BeginRead();

        // The dwSize is the actual number of bytes sent to the APC.
        void BackupBuffer(DWORD dwSize)
        {
            // We could just swap back and forth between the two
            // buffers, but this code is easier to understand and debug.
            memcpy(&m_BackupBuffer[0], &m_Buffer[0], dwSize);
        }

        void ProcessNotification();

        void RequestTermination()
        {
            ::CancelIo(m_hDirectory);
            ::CloseHandle(m_hDirectory);
            m_hDirectory = nullptr;
        }

        CReadChangesServer* const m_pServer;
        std::wstring get_directory_name() { return m_wstrDirectory; }

    protected:

        static VOID CALLBACK NotificationCompletion(
            DWORD dwErrorCode,							// completion code
            DWORD dwNumberOfBytesTransfered,			// number of bytes transferred
            LPOVERLAPPED lpOverlapped);					// I/O information buffer

        // Parameters from the caller for ReadDirectoryChangesW().
        const DWORD		m_dwFilterFlags;
        const bool		m_bIncludeChildren;

        std::wstring	m_wstrDirectory;

        // Result of calling CreateFile().
        HANDLE		    m_hDirectory{};

        // Required parameter for ReadDirectoryChangesW().
        OVERLAPPED	    m_Overlapped{};

        // Data buffer for the request.
        // Since the memory is allocated by malloc, it will always
        // be aligned as required by ReadDirectoryChangesW().
        std::vector<BYTE> m_Buffer;

        // Double buffer strategy so that we can issue a new read
        // request before we process the current buffer.
        std::vector<BYTE> m_BackupBuffer;
    };

    ///////////////////////////////////////////////////////////////////////////

    // All functions in CReadChangesServer run in the context of the worker thread.
    // One instance of this object is allocated for each instance of CReadDirectoryChanges.
    // This class is responsible for thread startup, orderly thread shutdown, and shimming
    // the various C++ member functions with C-style Win32 functions.
    class CReadChangesServer
    {
    public:
        CReadChangesServer(CReadDirectoryChanges* pParent) : m_pBase(pParent)
        {}

        // This helper function is not needed if you are using std::thread.
        // For example:
        // std::thread thread = std::thread([pServer]() {pServer->Run();});
        static unsigned int WINAPI ThreadStartProc(LPVOID arg)
        {
            CReadChangesServer* pServer = (CReadChangesServer*)arg;
            pServer->Run();
            return 0;
        }

        // Called by QueueUserAPC to start orderly shutdown.
        static void CALLBACK TerminateProc(__in  ULONG_PTR arg)
        {
            CReadChangesServer* pServer = (CReadChangesServer*)arg;
            pServer->RequestTermination();
        }

        // Called by QueueUserAPC to add another directory.
        static void CALLBACK AddDirectoryProc(__in  ULONG_PTR arg)
        {
            CReadChangesRequest* pRequest = (CReadChangesRequest*)arg;
            pRequest->m_pServer->AddDirectory(pRequest);
        }

        void stop(std::wstring dir)
        {
            StopDirectory(dir);
		}

        int get_watching_count()
        {
            return m_pBlocks.size();
		}

        bool is_watching(std::wstring dir)
        {
            for (int i = 0; i < m_pBlocks.size(); i++)
            {
                if (m_pBlocks[i]->get_directory_name() == dir)
					return true;
            }
            //for (const auto& block : m_pBlocks)
            //{
            //    if (block->get_directory_name() == dir)
            //        return true;
            //}
            return false;
		}

        void Run()
        {
            while (m_nOutstandingRequests || !m_bTerminate)
            {
                DWORD rc = ::SleepEx(INFINITE, true);
            }
        }

        CReadDirectoryChanges* const m_pBase;

        volatile DWORD m_nOutstandingRequests{};

    protected:

        void AddDirectory(CReadChangesRequest* pBlock)
        {
            if (pBlock->OpenDirectory())
            {
                ::InterlockedIncrement(&pBlock->m_pServer->m_nOutstandingRequests);
                m_pBlocks.push_back(pBlock);
                pBlock->BeginRead();
            }
            else
                delete pBlock;
        }

        //scpark add
        void StopDirectory(std::wstring dir)
        {
            for (DWORD i = 0; i < m_pBlocks.size(); ++i)
            {
                // Each Request object will delete itself.
                std::wstring dir_name = m_pBlocks[i]->get_directory_name();
                if (dir_name == dir)
                {
                    m_pBlocks[i]->RequestTermination();
                    ::InterlockedDecrement(&m_nOutstandingRequests);
                    delete m_pBlocks[i];
                    m_pBlocks.erase(m_pBlocks.begin() + i);
                    --i; // Adjust index after removal.
				}
            }
        }

        void RequestTermination()
        {
            m_bTerminate = true;

            for (DWORD i = 0; i < m_pBlocks.size(); ++i)
            {
                // Each Request object will delete itself.
                m_pBlocks[i]->RequestTermination();
            }

            m_pBlocks.clear();
        }

        std::vector<CReadChangesRequest*> m_pBlocks;

        bool m_bTerminate{};
    };

}
