#pragma once
#include <list>
#include <tbb\concurrent_hash_map.h>

typedef void(*FPSuccess)(DWORD trans, void* key, void* obj);
typedef void(*FPFailed)(void* key, void* obj);
typedef bool(*FPAcceptEx)(void* lsock);

typedef struct _listen_sock	LISTEN_SOCK; // 监听对象

typedef struct _recv_sock	RECV_SOCK; // 接收客户端发送数据
typedef struct _recv_obj	RECV_OBJ; // 辅助处理客户端数据

typedef struct _buffer_scok	BUFFER_SOCK; // 连接管理对象
typedef struct _buffer_obj	BUFFER_OBJ; // 接收发送数据

typedef struct _key_value
{
	int nKey;
	SOCKET sock;
}KEY_VALUE;


struct _listen_sock
{
	SOCKET sListenSock;
	FPAcceptEx fpAcceptex;
	HANDLE hPostAcceptexEvent;
	tbb::concurrent_hash_map<int, void*> acceptPendingMap;
	
	void Init()
	{
		if (INVALID_SOCKET != sListenSock)
		{
			closesocket(sListenSock);
			sListenSock = INVALID_SOCKET;
		}
		fpAcceptex = NULL;
		hPostAcceptexEvent = NULL;
		acceptPendingMap.clear();
	}

	void InsertIntoPendingMap(void* _sock)
	{
		tbb::concurrent_hash_map<int, void*>::accessor a;
		acceptPendingMap.insert(a, ((KEY_VALUE*)_sock)->nKey);
		a->second = _sock;
	}

	void DeleteFromPendingMap(void* _sock)
	{
		tbb::concurrent_hash_map<int, void*>::accessor a;
		if (acceptPendingMap.find(a, ((KEY_VALUE*)_sock)->nKey))
			acceptPendingMap.erase(a);
	}
};

struct _recv_sock
{
	int nKey;
	SOCKET sock;
	std::list<BUFFER_OBJ*>* lstSend;
	CRITICAL_SECTION cs;
	WSABUF wsabuf[2];
	DWORD dwBufsize;
	DWORD dwWritepos;
	DWORD dwReadpos;
	DWORD dwRightsize;
	DWORD dwLeftsize;
	bool bFull;
	bool bEmpty;
	byte sum;
	volatile long nRef;
	TCHAR buf[1];

	void Init(DWORD _Bufsize)
	{
		nKey = 0;
		if (INVALID_SOCKET != sock)
		{
			closesocket(sock);
			sock = INVALID_SOCKET;
		}
		dwBufsize = _Bufsize;
		dwWritepos = 0;
		dwReadpos = 0;
		dwRightsize = 0;
		dwLeftsize = 0;
		bFull = false;
		bEmpty = true;
		sum = 0;
		nRef = 1;
		InitializeCriticalSection(&cs);
		lstSend->clear();
	}

	void Clear()
	{
		if (NULL != lstSend)
			delete lstSend;
	}

	void InitRLsize()
	{
		if (bFull)
		{
			dwRightsize = 0;
			dwLeftsize = 0;
		}
		else if (dwReadpos <= dwWritepos)
		{
			dwRightsize = dwBufsize - dwWritepos;
			dwLeftsize = dwReadpos;
		}
		else
		{
			dwRightsize = dwReadpos - dwWritepos;
			dwLeftsize = 0;
		}
	}

	void InitWSABUFS()
	{
		InitRLsize();
		wsabuf[0].buf = buf + dwWritepos;
		wsabuf[0].len = dwRightsize;
		wsabuf[1].buf = buf;
		wsabuf[1].len = dwLeftsize;
	}

	void InitWRpos(DWORD dwTranstion)
	{
		if (dwTranstion <= 0)
			return;

		if (bFull)
			return;

		dwWritepos = (dwTranstion > dwRightsize) ? (dwTranstion - dwRightsize) : (dwWritepos + dwTranstion);
		bFull = (dwWritepos == dwReadpos);
		bEmpty = false;
	}

	byte csum(unsigned char *addr, int count)
	{
		for (int i = 0; i< count; i++)
		{
			sum += (byte)addr[i];
		}
		return sum;
	}

	DWORD GetCmdDataLength()
	{
		if (bEmpty)
			return 0;

		DWORD dwNum = 0;
		sum = 0;
		if (dwWritepos > dwReadpos)
		{
			DWORD nDataNum = dwWritepos - dwReadpos;
			if (nDataNum < 6)
				return 0;

			if ((UCHAR)buf[dwReadpos] != 0xfb || (UCHAR)buf[dwReadpos + 1] != 0xfc)//  没有数据开始标志
			{
				closesocket(sock);
				sock = INVALID_SOCKET;
				return 0;
			}

			DWORD dwFrameLen = *(INT*)(buf + dwReadpos + 2);
			if ((dwFrameLen + 8) > nDataNum)
				return 0;

			byte nSum = buf[dwReadpos + 6 + dwFrameLen];
			if (nSum != csum((unsigned char*)buf + dwReadpos + 6, dwFrameLen))
			{
				closesocket(sock);
				sock = INVALID_SOCKET;
				return 0;
			}

			if (0x0d != buf[dwReadpos + dwFrameLen + 7])
			{
				closesocket(sock);
				sock = INVALID_SOCKET;
				return 0;
			}

			dwReadpos += 6;

			return dwFrameLen + 2;
		}
		else
		{
			DWORD dwLeft = dwBufsize - dwReadpos;
			if (dwLeft < 6)
			{
				char subchar[6];
				memcpy(subchar, buf + dwReadpos, dwLeft);
				memcpy(subchar + dwLeft, buf, 6 - dwLeft);

				if ((UCHAR)subchar[0] != 0xfb || (UCHAR)subchar[1] != 0xfc)//  没有数据开始标志
				{
					closesocket(sock);
					sock = INVALID_SOCKET;
					return 0;
				}

				DWORD dwFrameLen = *(INT*)(subchar + 2);
				if ((dwFrameLen + 8) > (dwLeft + dwWritepos)) // 消息太长了
				{
					if (bFull)
					{
						closesocket(sock);
						sock = INVALID_SOCKET;
					}
					return 0;
				}

				DWORD dwIndex = (dwReadpos + 6 - dwBufsize);
				byte nSum = buf[dwIndex + dwFrameLen];
				if (nSum != csum((unsigned char*)buf + dwIndex, dwFrameLen))
				{
					closesocket(sock);
					sock = INVALID_SOCKET;
					return 0;
				}

				if (0x0d != buf[dwIndex + dwFrameLen + 1])
				{
					closesocket(sock);
					sock = INVALID_SOCKET;
					return 0;
				}

				dwReadpos = (dwReadpos + 6) > dwBufsize ? (dwReadpos + 6 - dwBufsize) : (dwReadpos + 6);

				return dwFrameLen + 2;
			}
			else
			{
				if ((UCHAR)buf[dwReadpos] != 0xfb || (UCHAR)buf[dwReadpos + 1] != 0xfc)//  没有数据开始标志
				{
					closesocket(sock);
					sock = INVALID_SOCKET;
					return 0;
				}

				DWORD dwFrameLen = *(INT*)(buf + dwReadpos + 2);
				if ((dwFrameLen + 8) > (dwLeft + dwWritepos)) // 消息太长了
				{
					if (bFull)
					{
						closesocket(sock);
						sock = INVALID_SOCKET;
					}
					return 0;
				}

				byte nSum = buf[(dwReadpos + 6 + dwFrameLen) >= dwBufsize ? (dwReadpos + 6 + dwFrameLen - dwBufsize) : (dwReadpos + 6 + dwFrameLen)];
				if ((dwFrameLen + 6) > dwLeft)
				{
					csum((unsigned char*)buf + dwReadpos + 6, dwLeft - 6);
					csum((unsigned char*)buf, dwFrameLen - dwLeft + 6);
					if (nSum != sum)
					{
						closesocket(sock);
						sock = INVALID_SOCKET;
						return 0;
					}
				}
				else
				{
					if (nSum != csum((unsigned char*)buf + dwReadpos + 6, dwFrameLen))
					{
						closesocket(sock);
						sock = INVALID_SOCKET;
						return 0;
					}
				}

				if (0x0d != buf[(dwReadpos + dwFrameLen + 7) >= dwBufsize ? (dwReadpos + dwFrameLen + 7 - dwBufsize) : (dwReadpos + dwFrameLen + 7)])
				{
					closesocket(sock);
					sock = INVALID_SOCKET;
					return 0;
				}

				dwReadpos = (dwReadpos + 6) > dwBufsize ? (dwReadpos + 6 - dwBufsize) : (dwReadpos + 6);

				return dwFrameLen + 2;
			}
		}
		return 0;
	}

	int Read(TCHAR* _buf, DWORD dwNum)
	{
		if (dwNum <= 0)
			return 0;

		if (bEmpty)
			return 0;

		bFull = false;
		if (dwReadpos < dwWritepos)
		{
			memcpy(_buf, buf + dwReadpos, dwNum);
			dwReadpos += dwNum;
			bEmpty = (dwReadpos == dwWritepos);
			return dwNum;
		}
		else if (dwReadpos >= dwWritepos)
		{
			DWORD leftcount = dwBufsize - dwReadpos;
			if (leftcount > dwNum)
			{
				memcpy(_buf, buf + dwReadpos, dwNum);
				dwReadpos += dwNum;
				bEmpty = (dwReadpos == dwWritepos);
				return dwNum;
			}
			else
			{
				memcpy(_buf, buf + dwReadpos, leftcount);
				dwReadpos = dwNum - leftcount;
				memcpy(_buf + leftcount, buf, dwReadpos);
				bEmpty = (dwReadpos == dwWritepos);
				return leftcount + dwReadpos;
			}
		}

		return 0;
	}

	void AddRef()
	{
		InterlockedIncrement(&nRef);
	}

	void DecRef()
	{
		InterlockedDecrement(&nRef);
	}

	bool CheckSend(BUFFER_OBJ* data)
	{
		EnterCriticalSection(&cs);
		if (lstSend->empty())
		{
			lstSend->push_front(data);
			LeaveCriticalSection(&cs);
			return true;
		}
		else
		{
			lstSend->push_front(data);
			LeaveCriticalSection(&cs);
			return false;
		}
	}

	BUFFER_OBJ* GetNextData()
	{
		BUFFER_OBJ* data = NULL;;
		EnterCriticalSection(&cs);
		lstSend->pop_back();
		if (!lstSend->empty())
			data = lstSend->back();
		LeaveCriticalSection(&cs);
		return data;
	}
};
#define RECV_SOCK_SIZE (sizeof(RECV_SOCK) - sizeof(TCHAR))

struct _recv_obj
{
	WSAOVERLAPPED ol;
	FPSuccess fpSuccess;
	FPFailed fpFailed;
	void* pRelateSock;

	void Init()
	{
		memset(&ol, 0x00, sizeof(ol));
		fpSuccess = NULL;
		fpFailed = NULL;
		pRelateSock = NULL;
	}

	void SetIoRequestFunction(FPFailed _fpFailed, FPSuccess _fpSuccess)
	{
		fpSuccess = _fpSuccess;
		fpFailed = _fpFailed;
	}
};
#define RECV_OBJ_SIZE sizeof(RECV_OBJ)

struct _buffer_scok
{
	int nKey;
	SOCKET sock;
	ADDRINFOT* sAddrInfo;
	
	void Init()
	{
		nKey = 0;
		if (INVALID_SOCKET != sock)
		{
			closesocket(sock);
			sock = INVALID_SOCKET;
		}

		if (NULL != sAddrInfo)
		{
			FreeAddrInfo(sAddrInfo);
			sAddrInfo = NULL;
		}
	}
};
#define BUFER_SOCK_SIZE sizeof(_buffer_scok)

struct _buffer_obj
{
	WSAOVERLAPPED ol;
	FPSuccess fpSuccess;
	FPFailed fpFailed;
	void* pRelateSock;
	WSABUF wsaBuf;
	DWORD dwRecvedCount;
	DWORD dwSendedCount;
	int datalen;
	TCHAR data[1];

	void Init(DWORD dwSize)
	{
		memset(&ol, 0x00, sizeof(ol));
		fpSuccess = NULL;
		fpFailed = NULL;
		pRelateSock = NULL;
		dwRecvedCount = 0;
		dwSendedCount = 0;
		datalen = dwSize;
	}

	void SetIoRequestFunction(FPFailed _fpFailed, FPSuccess _fpSuccess)
	{
		fpSuccess = _fpSuccess;
		fpFailed = _fpFailed;
	}
};
#define BUFFER_OBJ_SIZE (sizeof(BUFFER_OBJ) - sizeof(TCHAR))

extern LPFN_ACCEPTEX lpfnAccpetEx;
extern LPFN_GETACCEPTEXSOCKADDRS lpfnGetAcceptExSockaddrs;
extern LPFN_CONNECTEX lpfnConnectEx;
int GetRand();
void CMCloseSocket(void* client_sock);