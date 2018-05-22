#include "stdafx.h"
#include "ms_data.h"
#include "ms_nc_request.h"
#include "ms_memory.h"

extern void Nc_AcceptCompletionFailed(void* _lsock, void* _bobj);
extern void Nc_AcceptCompletionSuccess(DWORD dwTranstion, void* _lsock, void* _bobj);

bool Nc_PostAcceptEx(void* _lsock)
{
	LISTEN_SOCK* pLSock = (LISTEN_SOCK*)_lsock;
	DWORD dwBytes = 0;
	BUFFER_OBJ* pBObj = allocBObj(g_dwPagesize);
	if (NULL == pBObj)
		return false;

	BUFFER_SOCK* pBSock = allocBSock();
	if (NULL == pBSock)
	{
		freeBObj(pBObj);
		return false;
	}

	pBSock->sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == pBSock->sock)
	{
		freeBObj(pBObj);
		freeBSock(pBSock);
		return false;
	}

	pBObj->pRelateSock = pBSock;
	pBObj->SetIoRequestFunction(Nc_AcceptCompletionFailed, Nc_AcceptCompletionSuccess);
	// ÉèÖÃ»Øµô
	pBSock->nKey = GetRand();
	pLSock->InsertIntoPendingMap(pBSock);

	if (!lpfnAccpetEx(pLSock->sListenSock, pBSock->sock, pBObj->data, pBObj->datalen - ((sizeof(sockaddr_in) + 16) * 2),
		sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, &dwBytes, &pBObj->ol))
	{
		if (WSA_IO_PENDING != WSAGetLastError())
		{
			pLSock->DeleteFromPendingMap(pBSock);
			CMCloseSocket(pBSock);
			freeBObj(pBObj);
			freeBSock(pBSock);
			return false;
		}
	}

	return true;
}

BOOL Nc_PostZeroRecv(BUFFER_SOCK* pBSock, BUFFER_OBJ* pBObj)
{
	DWORD dwBytes = 0,
		dwFlags = 0;

	int err = 0;

	pBObj->wsaBuf.buf = NULL;
	pBObj->wsaBuf.len = 0;

	err = WSARecv(pBSock->sock, &pBObj->wsaBuf, 1, &dwBytes, &dwFlags, &pBObj->ol, NULL);
	if (SOCKET_ERROR == err && WSA_IO_PENDING != WSAGetLastError())
		return FALSE;

	return TRUE;
}

BOOL Nc_PostRecv(BUFFER_SOCK* pBSock, BUFFER_OBJ* pBObj)
{
	DWORD dwBytes = 0,
		dwFlags = 0;

	int err = 0;

	pBObj->wsaBuf.buf = pBObj->data + pBObj->dwRecvedCount;
	pBObj->wsaBuf.len = pBObj->datalen - pBObj->dwRecvedCount;

	err = WSARecv(pBSock->sock, &pBObj->wsaBuf, 1, &dwBytes, &dwFlags, &pBObj->ol, NULL);
	if (SOCKET_ERROR == err && WSA_IO_PENDING != WSAGetLastError())
		return FALSE;

	return TRUE;
}

BOOL Nc_PostSend(BUFFER_SOCK* pBSock, BUFFER_OBJ* pBObj)
{
	DWORD dwBytes = 0;
	int err = 0;

	pBObj->wsaBuf.buf = pBObj->data + pBObj->dwSendedCount;
	pBObj->wsaBuf.len = pBObj->dwRecvedCount - pBObj->dwSendedCount;

	err = WSASend(pBSock->sock, &pBObj->wsaBuf, 1, &dwBytes, 0, &pBObj->ol, NULL);
	if (SOCKET_ERROR == err && WSA_IO_PENDING != WSAGetLastError())
		return FALSE;

	return TRUE;
}