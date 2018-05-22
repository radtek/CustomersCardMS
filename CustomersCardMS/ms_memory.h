#pragma once

void InitMemory();

RECV_SOCK* allocRSock(DWORD dwSize);
void freeRSock(RECV_SOCK* pRSock);

RECV_OBJ* allocRObj();
void freeRObj(RECV_OBJ* pRObj);

BUFFER_SOCK* allocBSock();
void freeBSock(BUFFER_SOCK* pBSock);

BUFFER_OBJ* allocBObj(DWORD dwSize);
void freeBObj(BUFFER_OBJ* pBObj);