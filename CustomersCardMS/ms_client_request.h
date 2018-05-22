#pragma once

bool Client_PostAcceptEx(void* _lsock);

bool Client_PostZeroRecv(RECV_SOCK *pRSock, RECV_OBJ* pRObj);

bool Client_PostRecv(RECV_SOCK *pRSock, RECV_OBJ* pRObj);

bool CLient_PostSend(RECV_SOCK *pRSock, BUFFER_OBJ* pBObj);