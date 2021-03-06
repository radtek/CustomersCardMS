// CustomersCardMS.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "ms_data.h"
#include "ms_initsock.h"
#include "ms_memory.h"

extern LISTEN_SOCK* LSock_Array[];
extern HANDLE hEvent_Array[];
extern DWORD g_dwListenCount;

extern unsigned int _stdcall worker_thread(PVOID pVoid);
extern bool Client_PostAcceptEx(void* _lsock);
extern bool Nc_PostAcceptEx(void* lsock);

int main()
{
	if (!UniqueInstance())
	{
		_tprintf_s(_T("已经有实力在运行了\n"));
		return -1;
	}

	if (!InitWinsock2())
	{
		_tprintf(_T("winsock2初始化失败\n"));
		return -1;
	}

	InitMemory();

	SYSTEM_INFO sys;
	GetSystemInfo(&sys);
	DWORD dwCpunum = sys.dwNumberOfProcessors * 2;
	g_dwPagesize = sys.dwPageSize;

	hComport = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, (ULONG_PTR)0, 0);
	if (NULL == hComport)
	{
		_tprintf(_T("创建完成端口失败, errCode = %d\n"), WSAGetLastError());
		return -1;
	}

	for (DWORD i = 0; i < dwCpunum; i++)
	{
		HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, worker_thread, NULL, 0, NULL);
		if (NULL != hThread)
		{
			CloseHandle(hThread);
		}
	}

	LISTEN_SOCK* client_listen_sock = new LISTEN_SOCK;
	if (NULL == client_listen_sock)
		return -1;
	if (!InitListenSock(client_listen_sock, 7099))
	{
		delete client_listen_sock;
		return -1;
	}
	client_listen_sock->fpAcceptex = Client_PostAcceptEx;
	for (int i = 0; i < 10; i++)
	{
		Client_PostAcceptEx(client_listen_sock);
	}

	LISTEN_SOCK* api_listen_sock = new LISTEN_SOCK;
	if (NULL == api_listen_sock)
		return -1;
	if (!InitListenSock(api_listen_sock, 80))
	{
		delete api_listen_sock;
		return -1;
	}
	api_listen_sock->fpAcceptex = Nc_PostAcceptEx;
	for (int i = 0; i < 10; i++)
	{
		api_listen_sock->fpAcceptex(api_listen_sock);
	}

	while (true)
	{
		int rt = WSAWaitForMultipleEvents(g_dwListenCount, hEvent_Array, FALSE, INFINITE, FALSE);
		if (WSA_WAIT_FAILED == rt)
		{
			_tprintf(_T("带来异常，推出\n"));
			return -1;
		}

		for (DWORD nIndex = 0; nIndex < g_dwListenCount; nIndex++)
		{
			rt = WaitForSingleObject(hEvent_Array[nIndex], 0);
			if (WSA_WAIT_TIMEOUT == rt)
				continue;
			else if (WSA_WAIT_FAILED == rt)
			{
				_tprintf(_T("带来异常，推出\n"));
				return -1;
			}

			WSAResetEvent(hEvent_Array[nIndex]);
			for (size_t i = 0; i < 10; i++)
			{
				LSock_Array[nIndex]->fpAcceptex(LSock_Array[nIndex]);
			}

			if (LSock_Array[nIndex]->acceptPendingMap.size() > 100)
			{
				int nError = 0;
				int optlen,
					optval;
				optlen = sizeof(optval);

				for (tbb::concurrent_hash_map<int, void*>::iterator i = LSock_Array[nIndex]->acceptPendingMap.begin(); i != LSock_Array[nIndex]->acceptPendingMap.end(); ++i)
				{
					nError = getsockopt(((KEY_VALUE*)(i->second))->sock, SOL_SOCKET, SO_CONNECT_TIME, (char*)&optval, &optlen);
					if (SOCKET_ERROR == nError)
					{
						_tprintf(_T("getsockopt failed error: %d\n"), WSAGetLastError());
						continue;
					}

					if (optval != 0xffffffff && optval > 60)
					{
						closesocket(((KEY_VALUE*)(i->second))->sock);
						((KEY_VALUE*)(i->second))->sock = INVALID_SOCKET;
					}
				}
			}
		}
	}

    return 0;
}

