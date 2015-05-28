#include "Private.h"

namespace wsl
{
	string BaseSocket::GetIP()
	{
		char ip[100];
		inet_ntop(AF_INET, &(address.sin_addr), ip, 100);
		return string(ip);
	}

	void BaseSocket::ClearLastException()
	{
		delete lastSocketException;
		lastSocketException = nullptr;
	}

	void BaseSocket::SetLastException(const char* callStack, BaseSocket* socket)
	{
		delete lastSocketException;
		lastSocketException = NewSocketExceptionPtr(callStack, socket, -1);
	}
}