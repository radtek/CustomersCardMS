#include "stdafx.h"
#include "ms_data.h"
#include "ms_client_request.h"
#include "ms_memory.h"

extern void Client_AcceptCompletionFailed(void* _lsock, void* _bobj);
extern void Client_AcceptCompletionSuccess(DWORD dwTranstion, void* _lsock, void* _bobj);

bool Client_PostAcceptEx(void* _lsock)
{
	LISTEN_SOCK* pLSock = (LISTEN_SOCK*)_lsock;
	DWORD dwBytes = 0;

	RECV_OBJ* pRObj = allocRObj();
	if (NULL == pRObj)
		return false;

	RECV_SOCK* pRSock = allocRSock(g_dwPagesize * 8);
	if (NULL == pRSock)
	{
		freeRObj(pRObj);
		return false;
	}

	pRSock->sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == pRSock->sock)
	{
		freeRObj(pRObj);
		freeRSock(pRSock);
		return false;
	}

	pRObj->pRelateSock = pRSock;
	pRObj->SetIoRequestFunction(Client_AcceptCompletionFailed, Client_AcceptCompletionSuccess);

	pRSock->nKey = GetRand();
	pLSock->InsertIntoPendingMap(pRSock);

	if (!lpfnAccpetEx(pLSock->sListenSock, pRSock->sock, pRSock->buf, pRSock->dwBufsize - ((sizeof(sockaddr_in) + 16) * 2),
		sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, &dwBytes, &pRObj->ol))
	{
		if (WSA_IO_PENDING != WSAGetLastError())
		{
			pLSock->DeleteFromPendingMap(pRSock);
			CMCloseSocket(pRSock);
			freeRObj(pRObj);
			freeRSock(pRSock);
			return false;
		}
	}

	return true;
}

bool Client_PostZeroRecv(RECV_SOCK *pRSock, RECV_OBJ* pRObj)
{
	DWORD dwBytes = 0,
		dwFlags = 0;

	int err = 0;

	pRSock->wsabuf[0].buf = NULL;
	pRSock->wsabuf[0].len = 0;
	pRSock->wsabuf[1].buf = NULL;
	pRSock->wsabuf[1].len = 0;

	err = WSARecv(pRSock->sock, pRSock->wsabuf, 2, &dwBytes, &dwFlags, &pRObj->ol, NULL);
	if (SOCKET_ERROR == err && WSA_IO_PENDING != WSAGetLastError())
		return FALSE;

	return TRUE;
}

bool Client_PostRecv(RECV_SOCK *pRSock, RECV_OBJ* pRObj)
{
	DWORD dwBytes = 0,
		dwFlags = 0;

	int err = 0;

	pRSock->InitWSABUFS();

	err = WSARecv(pRSock->sock, pRSock->wsabuf, 2, &dwBytes, &dwFlags, &pRObj->ol, NULL);
	if (SOCKET_ERROR == err && WSA_IO_PENDING != WSAGetLastError())
		return FALSE;

	return TRUE;
}

bool CLient_PostSend(RECV_SOCK *pRSock, BUFFER_OBJ* pBObj)
{
	DWORD dwBytes = 0;
	int err = 0;

	pBObj->wsaBuf.buf = pBObj->data + pBObj->dwSendedCount;
	pBObj->wsaBuf.len = pBObj->dwRecvedCount - pBObj->dwSendedCount;

	err = WSASend(pRSock->sock, &pBObj->wsaBuf, 1, &dwBytes, 0, &pBObj->ol, NULL);
	if (SOCKET_ERROR == err && WSA_IO_PENDING != WSAGetLastError())
		return FALSE;

	return TRUE;
}