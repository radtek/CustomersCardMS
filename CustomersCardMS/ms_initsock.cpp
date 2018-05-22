#include "stdafx.h"
#include "ms_data.h"
#include "ms_initsock.h"

#define NP_THE_ONE_INSTANCE _T("Global\\np_The_One_Instance_Event_CustomersCardMS")

GUID GuidAcceptEx = WSAID_ACCEPTEX,
GuidGetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS,
GuidConnectEx = WSAID_CONNECTEX;

HANDLE hUniqueInstance = NULL;
LPFN_ACCEPTEX lpfnAccpetEx = NULL;
LPFN_GETACCEPTEXSOCKADDRS lpfnGetAcceptExSockaddrs = NULL;
LPFN_CONNECTEX lpfnConnectEx = NULL;

LISTEN_SOCK* LSock_Array[WSA_MAXIMUM_WAIT_EVENTS];
HANDLE hEvent_Array[WSA_MAXIMUM_WAIT_EVENTS];
DWORD g_dwListenCount = 0;

bool UniqueInstance()
{
	hUniqueInstance = ::OpenEvent(EVENT_ALL_ACCESS, false, NP_THE_ONE_INSTANCE);
	if (NULL != hUniqueInstance)
		return false;

	hUniqueInstance = ::CreateEvent(NULL, false, false, NP_THE_ONE_INSTANCE);
	if (NULL == hUniqueInstance)
		return false;

	return true;
}

bool GetExtensionFunctionPointer()
{
	SOCKET sTemp = INVALID_SOCKET;
	sTemp = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == sTemp)
		return false;

	DWORD dwBytes = 0;
	int rtErr = 0;

	rtErr = WSAIoctl(sTemp, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidGetAcceptExSockaddrs, sizeof(GuidGetAcceptExSockaddrs),
		&lpfnGetAcceptExSockaddrs, sizeof(lpfnGetAcceptExSockaddrs),
		&dwBytes, NULL, NULL);
	if (SOCKET_ERROR == rtErr)
		return false;

	rtErr = WSAIoctl(sTemp, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidAcceptEx, sizeof(GuidAcceptEx),
		&lpfnAccpetEx, sizeof(lpfnAccpetEx),
		&dwBytes, NULL, NULL);
	if (SOCKET_ERROR == rtErr)
		return false;

	rtErr = WSAIoctl(sTemp, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidConnectEx, sizeof(GuidConnectEx),
		&lpfnConnectEx, sizeof(lpfnConnectEx),
		&dwBytes, NULL, NULL);
	if (SOCKET_ERROR == rtErr)
		return false;

	closesocket(sTemp);
	sTemp = INVALID_SOCKET;

	return true;
}

bool InitWinsock2()
{
	WSADATA wsadata;
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsadata))
		return false;

	return GetExtensionFunctionPointer();
}

bool InitListenSock(LISTEN_SOCK* lsock, USHORT nPort)
{
	if (NULL == lsock)
		return false;

	lsock->Init();
	lsock->hPostAcceptexEvent = WSACreateEvent();
	if (NULL == lsock->hPostAcceptexEvent)
		return false;

	lsock->sListenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == lsock->sListenSock)
		return false;

	if (NULL == CreateIoCompletionPort((HANDLE)lsock->sListenSock, hComport, (ULONG_PTR)lsock, 0))
	{
		closesocket(lsock->sListenSock);
		lsock->sListenSock = INVALID_SOCKET;
		return false;
	}

	LSock_Array[g_dwListenCount] = lsock;
	hEvent_Array[g_dwListenCount++] = lsock->hPostAcceptexEvent;

	if (SOCKET_ERROR == WSAEventSelect(lsock->sListenSock, lsock->hPostAcceptexEvent, FD_ACCEPT))
	{
		closesocket(lsock->sListenSock);
		lsock->sListenSock = INVALID_SOCKET;
		return false;
	}

	struct sockaddr_in laddr;
	memset(&laddr, 0x00, sizeof(laddr));
	laddr.sin_family = AF_INET;
	laddr.sin_port = htons(nPort);
	laddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (SOCKET_ERROR == bind(lsock->sListenSock, (SOCKADDR*)&laddr, sizeof(laddr)))
	{
		closesocket(lsock->sListenSock);
		lsock->sListenSock = INVALID_SOCKET;
		return false;
	}

	if (SOCKET_ERROR == listen(lsock->sListenSock, SOMAXCONN))
	{
		closesocket(lsock->sListenSock);
		lsock->sListenSock = INVALID_SOCKET;
		return false;
	}

	return true;
}