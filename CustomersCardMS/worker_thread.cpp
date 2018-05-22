#include "stdafx.h"
#include "ms_data.h"

typedef struct _obj_value
{
	WSAOVERLAPPED ol;
	FPSuccess fpSuccess;
	FPFailed fpFailed;
}OBJ_VALUE;

unsigned int _stdcall worker_thread(PVOID pVoid)
{
	::CoInitialize(NULL);
	ULONG_PTR key;
	OBJ_VALUE* bobj;
	LPOVERLAPPED lpol;
	DWORD dwTranstion;
	BOOL bSuccess;

	while (true)
	{
		bSuccess = GetQueuedCompletionStatus(hComport, &dwTranstion, &key, &lpol, INFINITE);
		if (NULL == lpol)
		{
			printf("如果不是认为设定NULL为退出信号，那么就是发生重大错误，直接退出\n");
			return 0;
		}

		bobj = CONTAINING_RECORD(lpol, OBJ_VALUE, ol);

		if (!bSuccess)
			bobj->fpFailed((void*)key, bobj);
		else
			bobj->fpSuccess(dwTranstion, (void*)key, bobj);
	}

	return 0;
}