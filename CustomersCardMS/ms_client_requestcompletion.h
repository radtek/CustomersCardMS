#pragma once

void Client_AcceptCompletionFailed(void* _lsock, void* _bobj);
void Client_AcceptCompletionSuccess(DWORD dwTranstion, void* _lsock, void* _bobj);

void Client_ZeroRecvCompletionFailed(void* _csock, void* _bobj);
void Client_ZeroRecvCompletionSuccess(DWORD dwTranstion, void* _csock, void* _bobj);

void Client_RecvCompletionFailed(void* _csock, void* _bobj);
void Client_RecvCompletionSuccess(DWORD dwTranstion, void* _csock, void* _bobj);

void Client_SendCompletionFailed(void* _csock, void* _bobj);
void Client_SendCompletionSuccess(DWORD dwTranstion, void* _csock, void* _bobj);