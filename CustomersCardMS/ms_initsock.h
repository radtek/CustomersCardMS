#pragma once

bool UniqueInstance();

bool InitWinsock2();

bool InitListenSock(LISTEN_SOCK* lsock, USHORT nPort);