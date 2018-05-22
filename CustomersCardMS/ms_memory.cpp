#include "stdafx.h"
#include "ms_data.h"
#include "ms_memory.h"
#include <vector>

std::vector<RECV_SOCK*>		v_RSock;
CRITICAL_SECTION			cs_RSock;
#define RSOCK_SIZE_MAX		500

std::vector<RECV_OBJ*>		v_RObj;
CRITICAL_SECTION			cs_RObj;
#define ROBJ_SIZE_MAX		500

std::vector<BUFFER_SOCK*>	v_BSock;
CRITICAL_SECTION			cs_BSock;
#define BSOCK_SIZE_MAX		500

std::vector<BUFFER_OBJ*>	v_BObj;
CRITICAL_SECTION			cs_BObj;
#define BOBJ_SIZE_MAX		500

void InitMemory()
{
	v_RSock.clear();
	InitializeCriticalSection(&cs_RSock);
	v_RObj.clear();
	InitializeCriticalSection(&cs_RObj);
	v_BSock.clear();
	InitializeCriticalSection(&cs_BSock);
	v_BObj.clear();
	InitializeCriticalSection(&cs_BObj);
}

RECV_SOCK* allocRSock(DWORD dwSize)
{
	RECV_SOCK* pRSock = NULL;
	
	EnterCriticalSection(&cs_RSock);
	if (v_RSock.empty())
	{
		pRSock = (RECV_SOCK*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize);
		pRSock->lstSend = new std::list<BUFFER_OBJ*>;
	}
	else
	{
		pRSock = v_RSock.back();
		v_RSock.pop_back();
	}
	LeaveCriticalSection(&cs_RSock);

	if (pRSock)
		pRSock->Init(dwSize - RECV_SOCK_SIZE);

	return pRSock;
}

void freeRSock(RECV_SOCK* pRSock)
{
	EnterCriticalSection(&cs_RSock);
	if (v_RSock.size() < RSOCK_SIZE_MAX)
		v_RSock.push_back(pRSock);
	else
	{
		pRSock->Clear();
		HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, pRSock);
	}
	LeaveCriticalSection(&cs_RSock);
}

RECV_OBJ* allocRObj()
{
	RECV_OBJ* pRObj = NULL;
	EnterCriticalSection(&cs_RObj);
	if (v_RObj.empty())
		pRObj = (RECV_OBJ*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, RECV_OBJ_SIZE);
	else
	{
		pRObj = v_RObj.back();
		v_RObj.pop_back();
	}
	LeaveCriticalSection(&cs_RObj);

	if (pRObj)
		pRObj->Init();

	return pRObj;
}

void freeRObj(RECV_OBJ* pRObj)
{
	EnterCriticalSection(&cs_RObj);
	if (v_RObj.size() < ROBJ_SIZE_MAX)
		v_RObj.push_back(pRObj);
	else
		HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, pRObj);
	LeaveCriticalSection(&cs_RObj);
}

BUFFER_SOCK* allocBSock()
{
	BUFFER_SOCK* pBSock = NULL;
	EnterCriticalSection(&cs_BSock);
	if (v_BSock.empty())
		pBSock = (BUFFER_SOCK*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, BUFER_SOCK_SIZE);
	else
	{
		pBSock = v_BSock.back();
		v_BSock.pop_back();
	}
	LeaveCriticalSection(&cs_BSock);

	if (pBSock)
		pBSock->Init();

	return pBSock;
}

void freeBSock(BUFFER_SOCK* pBSock)
{
	EnterCriticalSection(&cs_BSock);
	if (v_BSock.size() < BSOCK_SIZE_MAX)
		v_BSock.push_back(pBSock);
	else
		HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, pBSock);
	LeaveCriticalSection(&cs_BSock);
}

BUFFER_OBJ* allocBObj(DWORD dwSize)
{
	BUFFER_OBJ* pBObj = NULL;
	EnterCriticalSection(&cs_BObj);
	if (v_BObj.empty())
		pBObj = (BUFFER_OBJ*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize);
	else
	{
		pBObj = v_BObj.back();
		v_BObj.pop_back();
	}
	LeaveCriticalSection(&cs_BObj);

	if (pBObj)
		pBObj->Init(dwSize - BUFFER_OBJ_SIZE);

	return pBObj;

}
void freeBObj(BUFFER_OBJ* pBObj)
{
	EnterCriticalSection(&cs_BObj);
	if (v_BObj.size() < BOBJ_SIZE_MAX)
		v_BObj.push_back(pBObj);
	else
		HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, pBObj);
	LeaveCriticalSection(&cs_BObj);
}