#include "stdafx.h"
#include "ms_client_requestcompletion.h"
#include "ms_data.h"
#include "ms_memory.h"
#include "ms_client_request.h"
#include "ms_cmd.h"

struct tcp_keepalive alive_in = { TRUE, 1000 * 10, 1000 };
struct tcp_keepalive alive_out = { 0 };
unsigned long ulBytesReturn = 0;

void Client_AcceptCompletionFailed(void* _lsock, void* _bobj)
{
	LISTEN_SOCK* pLSock = (LISTEN_SOCK*)_lsock;
	RECV_OBJ* pRObj = (RECV_OBJ*)_bobj;
	RECV_SOCK* pRSock = (RECV_SOCK*)pRObj->pRelateSock;

	pLSock->DeleteFromPendingMap(pRSock);

	CMCloseSocket(pRSock);

	freeRObj(pRObj);
	freeRSock(pRSock);
}

void Client_AcceptCompletionSuccess(DWORD dwTranstion, void* _lsock, void* _bobj)
{
	if (dwTranstion <= 0)
		return Client_AcceptCompletionFailed(_lsock, _bobj);

	LISTEN_SOCK* pLSock = (LISTEN_SOCK*)_lsock;
	RECV_OBJ* pRObj = (RECV_OBJ*)_bobj;
	RECV_SOCK* pRSock = (RECV_SOCK*)pRObj->pRelateSock;

	pLSock->DeleteFromPendingMap(pRSock);

	WSAIoctl(pRSock->sock, SIO_KEEPALIVE_VALS, &alive_in, sizeof(alive_in),
		&alive_out, sizeof(alive_out), &ulBytesReturn, NULL, NULL);

	if (NULL == CreateIoCompletionPort((HANDLE)pRSock->sock, hComport, (ULONG_PTR)pRSock, 0))
	{
		_tprintf(_T("客户端socket绑定完成端口失败, errCode = %d\n"), WSAGetLastError());

		CMCloseSocket(pRSock);

		freeRObj(pRObj);
		freeRSock(pRSock);
		return;
	}

	SOCKADDR* localAddr,
		*remoteAddr;
	localAddr = NULL;
	remoteAddr = NULL;
	int localAddrlen,
		remoteAddrlen;

	lpfnGetAcceptExSockaddrs(pRSock->buf, pRSock->dwBufsize - ((sizeof(sockaddr_in) + 16) * 2),
		sizeof(sockaddr_in) + 16,
		sizeof(sockaddr_in) + 16,
		&localAddr, &localAddrlen,
		&remoteAddr, &remoteAddrlen);

	pRSock->InitWRpos(dwTranstion);
	DWORD len = pRSock->GetCmdDataLength();
	while (len)
	{
		BUFFER_OBJ* pBObj = allocBObj(g_dwPagesize);
		pRSock->Read(pBObj->data, len);
		pBObj->dwRecvedCount = len;
		pBObj->pRelateSock = pRSock;
		pRSock->AddRef();
		pBObj->SetIoRequestFunction(Client_CmdCompletionFailed, Client_CmdCompletionSuccess);
		if (0 == PostQueuedCompletionStatus(hComport, len, (ULONG_PTR)pBObj, &pBObj->ol))
		{
			pRSock->DecRef();
			freeBObj(pBObj);
		}
		len = pRSock->GetCmdDataLength();
	}

	pRObj->SetIoRequestFunction(Client_ZeroRecvCompletionFailed, Client_ZeroRecvCompletionSuccess);
	if (!Client_PostZeroRecv(pRSock, pRObj))
	{
		if (0 == InterlockedDecrement(&pRSock->nRef))
		{
			CMCloseSocket(pRSock);
			freeRSock(pRSock);
		}
		freeRObj(pRObj);
	}
}

void Client_ZeroRecvCompletionFailed(void* _csock, void* _bobj)
{
	RECV_SOCK* pRSock = (RECV_SOCK*)_csock;
	RECV_OBJ* pRObj = (RECV_OBJ*)_bobj;
	if (0 == InterlockedDecrement(&pRSock->nRef))
	{
		CMCloseSocket(pRSock);
		freeRSock(pRSock);
	}
	freeRObj(pRObj);
}

void Client_ZeroRecvCompletionSuccess(DWORD dwTranstion, void* _csock, void* _bobj)
{
	RECV_SOCK* pRSock = (RECV_SOCK*)_csock;
	RECV_OBJ* pRObj = (RECV_OBJ*)_bobj;

	pRObj->SetIoRequestFunction(Client_RecvCompletionFailed, Client_RecvCompletionSuccess);
	if (!Client_PostRecv(pRSock, pRObj))
	{
		if (0 == InterlockedDecrement(&pRSock->nRef))
		{
			CMCloseSocket(pRSock);
			freeRSock(pRSock);
		}
		freeRObj(pRObj);
	}
}

void Client_RecvCompletionFailed(void* _csock, void* _bobj)
{
	RECV_SOCK* pRSock = (RECV_SOCK*)_csock;
	RECV_OBJ* pRObj = (RECV_OBJ*)_bobj;

	if (0 == InterlockedDecrement(&pRSock->nRef))
	{
		CMCloseSocket(pRSock);
		freeRSock(pRSock);
	}
	freeRObj(pRObj);
}

void Client_RecvCompletionSuccess(DWORD dwTranstion, void* _csock, void* _bobj)
{
	if (dwTranstion <= 0)
		return Client_RecvCompletionFailed(_csock, _bobj);

	RECV_SOCK* pRSock = (RECV_SOCK*)_csock;
	RECV_OBJ* pRObj = (RECV_OBJ*)_bobj;

	pRSock->InitWRpos(dwTranstion);
	DWORD len = pRSock->GetCmdDataLength();
	while (len)
	{
		BUFFER_OBJ* pBObj = allocBObj(g_dwPagesize);
		pRSock->Read(pBObj->data, len);
		pBObj->dwRecvedCount = len;
		pBObj->pRelateSock = pRSock;
		pRSock->AddRef();
		pBObj->SetIoRequestFunction(Client_CmdCompletionFailed, Client_CmdCompletionSuccess);
		if (0 == PostQueuedCompletionStatus(hComport, len, (ULONG_PTR)pBObj, &pBObj->ol))
		{
			freeBObj(pBObj);
			if (0 == InterlockedDecrement(&pRSock->nRef))
			{
				CMCloseSocket(pRSock);
				freeRSock(pRSock);
				return;
			}
		}
		len = pRSock->GetCmdDataLength();
	}

	pRObj->SetIoRequestFunction(Client_ZeroRecvCompletionFailed, Client_ZeroRecvCompletionSuccess);
	if (!Client_PostZeroRecv(pRSock, pRObj))
	{
		if (0 == InterlockedDecrement(&pRSock->nRef))
		{
			CMCloseSocket(pRSock);
			freeRSock(pRSock);
		}
		freeRObj(pRObj);
	}
}

void Client_SendCompletionFailed(void* _sobj, void* _bobj)
{
	RECV_SOCK* pRSock = (RECV_SOCK*)_sobj;
	BUFFER_OBJ* pBObj = (BUFFER_OBJ*)_bobj;

	BUFFER_OBJ* obj = pRSock->GetNextData();
	freeBObj(pBObj);
	if (obj)
	{
		InterlockedDecrement(&pRSock->nRef);
		obj->SetIoRequestFunction(Client_SendCompletionFailed, Client_SendCompletionSuccess);
		if (!CLient_PostSend(pRSock, obj))
		{
			Client_SendCompletionFailed(pRSock, obj);
		}
	}
	else
	{
		if (0 == InterlockedDecrement(&pRSock->nRef))
		{
			CMCloseSocket(pRSock);
			freeRSock(pRSock);
		}
	}
}
void Client_SendCompletionSuccess(DWORD dwTranstion, void* _sobj, void* _bobj)
{
	if (dwTranstion <= 0)
		return Client_SendCompletionFailed(_sobj, _bobj);

	RECV_SOCK* pRSock = (RECV_SOCK*)_sobj;
	BUFFER_OBJ* pBObj = (BUFFER_OBJ*)_bobj;

	pBObj->dwSendedCount += dwTranstion;
	if (pBObj->dwSendedCount < pBObj->dwRecvedCount)
	{
		if (!CLient_PostSend(pRSock, pBObj))
		{
			Client_SendCompletionFailed(pRSock, pBObj);
			return;
		}
		return;
	}

	BUFFER_OBJ* obj = pRSock->GetNextData();
	freeBObj(pBObj);
	if (obj)
	{
		InterlockedDecrement(&pRSock->nRef);
		obj->SetIoRequestFunction(Client_SendCompletionFailed, Client_SendCompletionSuccess);
		if (!CLient_PostSend(pRSock, obj))
		{
			Client_SendCompletionFailed(pRSock, obj);
		}
	}
	else
	{
		if (0 == InterlockedDecrement(&pRSock->nRef))
		{
			CMCloseSocket(pRSock);
			freeRSock(pRSock);
		}
	}
}