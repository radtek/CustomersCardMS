#pragma once

bool Nc_PostAcceptEx(void* lsock);

BOOL Nc_PostZeroRecv(BUFFER_SOCK* pBSock, BUFFER_OBJ* pBObj);

BOOL Nc_PostRecv(BUFFER_SOCK* pBSock, BUFFER_OBJ* pBObj);

BOOL Nc_PostSend(BUFFER_SOCK* pBSock, BUFFER_OBJ* pBObj);