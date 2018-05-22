#pragma once

void Nc_AcceptCompletionFailed(void* _lsock, void* _bobj);
void Nc_AcceptCompletionSuccess(DWORD dwTranstion, void* _lsock, void* _bobj);

void NC_RecvZeroCompFailed(void* _sobj, void* _bobj);
void NC_RecvZeroCompSuccess(DWORD dwTransion, void* _sobj, void* _bobj);

void NC_RecvCompFailed(void* _sobj, void* _bobj);
void NC_RecvCompSuccess(DWORD dwTransion, void* _sobj, void* _bobj);

void NC_SendCompFailed(void* _sobj, void* _bobj);
void NC_SendCompSuccess(DWORD dwTransion, void* _sobj, void* _bobj);