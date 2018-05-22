#include "stdafx.h"
#include "ms_nc_requestcompletion.h"
#include "ms_data.h"
#include "ms_memory.h"
#include "ms_nc_request.h"
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

extern struct tcp_keepalive alive_in;
extern struct tcp_keepalive alive_out;
extern unsigned long ulBytesReturn;

int CheckNCData(BUFFER_OBJ* bobj)
{
	TCHAR* pHeader = NULL;
	if (NULL == (pHeader = _tcsstr(bobj->data, _T("\r\n\r\n"))))
	{
		return 0;
	}
	else
	{
		int HeaderLen = (int)(pHeader - bobj->data);
		HeaderLen += (int)_tcslen(_T("\r\n\r\n"));
		int len = (int)_tcslen(_T("Content-Length: "));
		TCHAR* pContentLength = StrStrI(bobj->data, _T("Content-Length: "));
		if (NULL == pContentLength)
			return 0;
		TCHAR* pContentLengthEnd = _tcsstr(pContentLength, _T("\r\n"));
		if (NULL == pContentLengthEnd)
			return 0;
		int nLengthLen = (int)(pContentLengthEnd - pContentLength) - len;
		TCHAR Length[8] = { 0 };
		memcpy_s(Length, sizeof(Length), pContentLength + len, nLengthLen);
		len = _tstoi(Length);
		_tprintf_s("Content-Length: %d\n", len);
		if ((HeaderLen + len) > (int)bobj->dwRecvedCount)
			return 0;
		else if ((HeaderLen + len) == (int)bobj->dwRecvedCount)
			return HeaderLen;
		else
		{
			CMCloseSocket(bobj);
			return 0;
		}
	}
	return 0;
}

void Nc_AcceptCompletionFailed(void* _lsock, void* _bobj)
{
	LISTEN_SOCK* pLSock = (LISTEN_SOCK*)_lsock;
	BUFFER_OBJ* pBObj = (BUFFER_OBJ*)_bobj;
	BUFFER_SOCK* pBSock = (BUFFER_SOCK*)pBObj->pRelateSock;

	pLSock->DeleteFromPendingMap(pBSock);

	CMCloseSocket(pBSock);
	freeBSock(pBSock);
	freeBObj(pBObj);
}

void Nc_AcceptCompletionSuccess(DWORD dwTranstion, void* _lsock, void* _bobj)
{
	if (dwTranstion <= 0)
		return Nc_AcceptCompletionFailed(_lsock, _bobj);

	LISTEN_SOCK* pLSock = (LISTEN_SOCK*)_lsock;
	BUFFER_OBJ* pBObj = (BUFFER_OBJ*)_bobj;
	BUFFER_SOCK* pBSock = (BUFFER_SOCK*)pBObj->pRelateSock;

	pLSock->DeleteFromPendingMap(pBSock);

	WSAIoctl(pBSock->sock, SIO_KEEPALIVE_VALS, &alive_in, sizeof(alive_in),
		&alive_out, sizeof(alive_out), &ulBytesReturn, NULL, NULL);
	if (NULL == CreateIoCompletionPort((HANDLE)pBSock->sock, hComport, (ULONG_PTR)pBSock, 0))
	{
		CMCloseSocket(pBSock);
		freeBSock(pBSock);
		freeBObj(pBObj);
		return;
	}

	pBObj->dwRecvedCount += dwTranstion;

	SOCKADDR* localAddr,
		*remoteAddr;
	localAddr = NULL;
	remoteAddr = NULL;
	int localAddrlen,
		remoteAddrlen;

	lpfnGetAcceptExSockaddrs(pBObj->data, pBObj->datalen - ((sizeof(sockaddr_in) + 16) * 2),
		sizeof(sockaddr_in) + 16,
		sizeof(sockaddr_in) + 16,
		&localAddr, &localAddrlen,
		&remoteAddr, &remoteAddrlen);

	int Headerlen = CheckNCData(pBObj);
	if (0 == Headerlen)
	{
		pBObj->SetIoRequestFunction(NC_RecvZeroCompFailed, NC_RecvZeroCompSuccess);
		if (!Nc_PostZeroRecv(pBSock, pBObj))
		{
			CMCloseSocket(pBSock);
			freeBSock(pBSock);
			freeBObj(pBObj);
			return;
		}
	}
	else
	{
		//TCHAR* pResponData = Utf8ConvertAnsi(bobj->data + Headerlen, bobj->dwRecvedCount - Headerlen);
		//doNcResponse(bobj); // 返回推送反馈报文
		//doNcData(pResponData);
	}
}

void NC_RecvZeroCompFailed(void* _sobj, void* _bobj)
{
	BUFFER_SOCK* pBSock = (BUFFER_SOCK*)_sobj;
	BUFFER_OBJ* pBObj = (BUFFER_OBJ*)_bobj;

#ifdef _DEBUG
	DWORD dwTranstion = 0;
	DWORD dwFlags = 0;
	if (!WSAGetOverlappedResult(pBSock->sock, &pBObj->ol, &dwTranstion, FALSE, &dwFlags))
		_tprintf(_T("函数:%s ErrorCode = %d\n"), __FUNCTION__, WSAGetLastError());
#endif

	CMCloseSocket(pBSock);
	freeBSock(pBSock);
	freeBObj(pBObj);
}

void NC_RecvZeroCompSuccess(DWORD dwTransion, void* _sobj, void* _bobj)
{
	BUFFER_SOCK* pBSock = (BUFFER_SOCK*)_sobj;
	BUFFER_OBJ* pBObj = (BUFFER_OBJ*)_bobj;

	pBObj->SetIoRequestFunction(NC_RecvCompFailed, NC_RecvCompSuccess);
	if (!Nc_PostRecv(pBSock, pBObj))
	{
		CMCloseSocket(pBSock);
		freeBSock(pBSock);
		freeBObj(pBObj);
		return;
	}
}

void NC_RecvCompFailed(void* _sobj, void* _bobj)
{
	BUFFER_SOCK* pBSock = (BUFFER_SOCK*)_sobj;
	BUFFER_OBJ* pBObj = (BUFFER_OBJ*)_bobj;

#ifdef _DEBUG
	DWORD dwTranstion = 0;
	DWORD dwFlags = 0;
	if (!WSAGetOverlappedResult(pBSock->sock, &pBObj->ol, &dwTranstion, FALSE, &dwFlags))
		_tprintf(_T("函数:%s ErrorCode = %d\n"), __FUNCTION__, WSAGetLastError());
#endif

	CMCloseSocket(pBSock);
	freeBSock(pBSock);
	freeBObj(pBObj);
}

void NC_RecvCompSuccess(DWORD dwTransion, void* _sobj, void* _bobj)
{
	if (dwTransion <= 0)
		return NC_RecvCompFailed(_sobj, _bobj);

	BUFFER_SOCK* pBSock = (BUFFER_SOCK*)_sobj;
	BUFFER_OBJ* pBObj = (BUFFER_OBJ*)_bobj;

	pBObj->dwRecvedCount += dwTransion;

	int Headerlen = CheckNCData(pBObj);
	if (0 == Headerlen)
	{
		pBObj->SetIoRequestFunction(NC_RecvZeroCompFailed, NC_RecvZeroCompSuccess);
		if (!Nc_PostZeroRecv(pBSock, pBObj))
		{
			CMCloseSocket(pBSock);
			freeBSock(pBSock);
			freeBObj(pBObj);
			return;
		}
	}
	else
	{
		//TCHAR* pResponData = Utf8ConvertAnsi(bobj->data + Headerlen, bobj->dwRecvedCount - Headerlen);
		//doNcResponse(bobj); // 返回推送反馈报文
		//doNcData(pResponData);
	}
}

void NC_SendCompFailed(void* _sobj, void* _bobj)
{
	BUFFER_SOCK* pBSock = (BUFFER_SOCK*)_sobj;
	BUFFER_OBJ* pBObj = (BUFFER_OBJ*)_bobj;

#ifdef _DEBUG
	DWORD dwTranstion = 0;
	DWORD dwFlags = 0;
	if (!WSAGetOverlappedResult(pBSock->sock, &pBObj->ol, &dwTranstion, FALSE, &dwFlags))
		_tprintf(_T("函数:%s ErrorCode = %d\n"), __FUNCTION__, WSAGetLastError());
#endif

	CMCloseSocket(pBSock);
	freeBSock(pBSock);
	freeBObj(pBObj);
}

void NC_SendCompSuccess(DWORD dwTransion, void* _sobj, void* _bobj)
{
	if (dwTransion <= 0)
		return NC_SendCompFailed(_sobj, _bobj);

	BUFFER_SOCK* pBSock = (BUFFER_SOCK*)_sobj;
	BUFFER_OBJ* pBObj = (BUFFER_OBJ*)_bobj;

	pBObj->dwSendedCount += dwTransion;
	if (pBObj->dwSendedCount < pBObj->dwRecvedCount)
	{
		if (!Nc_PostSend(pBSock, pBObj))
		{
			CMCloseSocket(pBSock);
			freeBSock(pBSock);
			freeBObj(pBObj);
			return;
		}
		return;
	}

	CMCloseSocket(pBSock);
	freeBSock(pBSock);
	freeBObj(pBObj);
	return;

	pBObj->dwRecvedCount = 0;
	pBObj->dwSendedCount = 0;
	pBObj->SetIoRequestFunction(NC_RecvZeroCompFailed, NC_RecvZeroCompSuccess);
	if (!Nc_PostZeroRecv(pBSock, pBObj))
	{
		CMCloseSocket(pBSock);
		freeBSock(pBSock);
		freeBObj(pBObj);
		return;
	}
}