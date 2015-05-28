#include "Private.h"

namespace wsl
{
	IntClient::IntClient(string ip, unsigned short port)
	{
		valid = true;
		name = "";
		connected = false;
		lastSocketException = nullptr;
		//create socket, AF_INET,SOCK_STREAM,NULL for tcp/ip v4, AF_INET6 for ip v6
		handle = socket(AF_INET, SOCK_STREAM, NULL);
		if (handle == INVALID_SOCKET)
		{
			valid = false;
			SetLastException("socket() in Client::Client()", this);
		}
		else
			ClearLastException();
		id = (long long)handle;
		SecureZeroMemory(&address, sizeof(address));
		address.sin_family = AF_INET;
		//this is how to create ip address from string
		inet_pton(AF_INET, ip.c_str(), &(address.sin_addr));
		//port
		address.sin_port = htons(port);
	}

	IntClient::IntClient(SOCKET socket, sockaddr_in _address, IntServer* _server)
	{
		server = _server;
		valid = true;
		lastSocketException = nullptr;
		connected = true;
		handle = socket;
		address = _address;
		receiveThread = thread(ReceiveThread, this);
		id = (long long)handle;
		name = "";
	}

	void IntClient::Connect()
	{
		if (!valid)
			return;
		//connect
		int result = connect(handle, (sockaddr*)&(address), sizeof(sockaddr_in));
		if (result == SOCKET_ERROR)
		{
			SetLastException("connect() in Client::Client()", this);
			return;
		}
		else
			ClearLastException();
		//start receiving thread
		connected = true;
		receiveThread = thread(ReceiveThread, this);
	}

	void IntClient::Disconnect()
	{
		if (!valid || !connected)
			return;
		connected = false;
		if (closesocket(handle) == SOCKET_ERROR)
			SetLastException("closesocket() in Client::Disconnect()", this);
		else
			ClearLastException();
		receiveThread.join();
		handle = INVALID_SOCKET;
		ClearLastException();
	}

	vector<byte> IntClient::GetNextMessage()
	{
		vector<byte> empty;
		receiveBufferMutex.lock();
		if (receiveBuffer.size() < 2)
		{
			receiveBufferMutex.unlock();
			return empty;
		}
		//get msg len from buffer, should be at the beginning
		byte lenRaw[2];
		lenRaw[0] = receiveBuffer[0];
		lenRaw[1] = receiveBuffer[1];
		unsigned short len = *(unsigned short*)lenRaw;
		//check if complete msg received
		if ((int)receiveBuffer.size() < len + 2)
		{
			receiveBufferMutex.unlock();
			return empty;
		}
		vector<byte> msg(receiveBuffer.begin() + 2, receiveBuffer.end());
		receiveBuffer.erase(receiveBuffer.begin(), receiveBuffer.begin() + 2 + len);
		receiveBufferMutex.unlock();
		return msg;
	}

	unsigned int IntClient::GetNextMsgLen()
	{
		receiveBufferMutex.lock();
		if (receiveBuffer.size() < 2)
		{
			receiveBufferMutex.unlock();
			return 0;
		}
		//get msg len from buffer, should be at the beginning
		byte lenRaw[2];
		lenRaw[0] = receiveBuffer[0];
		lenRaw[1] = receiveBuffer[1];
		receiveBufferMutex.unlock();
		unsigned short len = *(unsigned short*)lenRaw;
		return (unsigned int)len;
	}

	void IntClient::GetNextMessage(byte* dst)
	{
		/*CheckForException();
		receiveBufferMutex->lock();
		if (receiveBuffer.size() < 2)
		{
		receiveBufferMutex->unlock();
		return;
		}
		//get msg len from buffer, should be at the beginning
		byte lenRaw[2];
		lenRaw[0] = receiveBuffer[0];
		lenRaw[1] = receiveBuffer[1];
		unsigned short len = *(unsigned short*)lenRaw;
		//check if complete msg received
		if ((int)receiveBuffer.size() < len + 2)
		{
		receiveBufferMutex->unlock();
		return;
		}
		byte* it = receiveBuffer.data() + 2;
		for (int i = 0; i < len; i++)
		*(dst++) = *(it++);
		receiveBuffer.erase(receiveBuffer.begin(), receiveBuffer.begin() + 2 + len);
		receiveBufferMutex->unlock();*/
	}

	void IntClient::Send(byte* msg, unsigned short len)
	{
		if (!valid || !connected)
			return;
		int index = 0;
		//first 2 bytes is msg length as short
		byte* smsg = new byte[len + 2];
		memcpy(smsg, &len, 2);
		memcpy(smsg + 2, msg, len);
		//send function isnt guaranteed to send everything
		//compare return value with msg len and resend if necessary
		while (index < len + 2)
		{
			smsg += index;
			int sent = send(handle, (const char*)smsg, len - index + 2, 0);
			if (sent == SOCKET_ERROR)
			{
				delete[] smsg;
				SetLastException("send() in Server::Send()", this);
				return;
			}
			else
				ClearLastException();
			index += sent;
		}
		delete[] smsg;
	}

	IntClient::~IntClient()
	{
		Disconnect();
		ClearLastException();
	}
}