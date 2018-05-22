#include "stdafx.h"
#include "ms_data.h"
#include <random>

std::random_device randdev;
std::default_random_engine randeg(randdev());
std::uniform_int_distribution<int> urand(0);

int GetRand()
{
	return urand(randeg);
}

void CMCloseSocket(void* client_sock)
{

	if (INVALID_SOCKET != ((KEY_VALUE*)client_sock)->sock)
	{
		closesocket(((KEY_VALUE*)client_sock)->sock);
		((KEY_VALUE*)client_sock)->sock = INVALID_SOCKET;
	}
}