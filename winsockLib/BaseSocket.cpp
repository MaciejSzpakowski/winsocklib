#include "Private.h"

namespace wsl
{
	string BaseSocket::GetIP()
	{
		char ip[100];
		inet_ntop(AF_INET, &(address.sin_addr), ip, 100);
		return string(ip);
	}

	void BaseSocket::AppendException(const char* callStack, BaseSocket* socket)
	{
		notifications.push_back(NewSocketException(callStack, socket, -1));
	}

	void BaseSocket::AppendNotification(Notification notification)
	{
		notifications.push_back(notification);
	}

	bool BaseSocket::GetLastNotification(Notification& notification)
	{
		acceptThreadMutex.lock();
		if (notifications.size() == 0)
		{
			acceptThreadMutex.unlock();
			return false;
		}
		notification = notifications.back();
		notifications.pop_back();
		acceptThreadMutex.unlock();
		return true;
	}
}