#include "header.h"
//need to link this lib for winsock
#pragma comment(lib, "ws2_32.lib")

namespace wsl
{	
	//returns error code as formatted message in string
	//out errorCode: error code
	string GetLastWinsockErrorMessage(PDWORD errorCode)
	{
		DWORD err = WSAGetLastError();
		*errorCode = err;
		char str[300];
		SecureZeroMemory(str, 300);
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0,
			err, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
			str, 300, 0);
		return string(str);
	}

	//formats socket exception to throw
	SocketException NewSocketException(string callstack)
	{
		SocketException se;
		se.callStack = callstack;
		se.message = GetLastWinsockErrorMessage(&se.code);
		return se;
	}

	Winsock::Winsock(int major, int minor)
	{
		//initialize winsock library
		if (WSAStartup(MAKEWORD(major, minor), &wsadata) != 0)
			throw NewSocketException("WSAStartup() in Winsock::Winsock()");
	}

	Winsock::Winsock()
	{
		//initialize winsock library
		if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0)
			throw NewSocketException("WSAStartup() in Winsock::Winsock()");
	}

	Winsock::~Winsock()
	{
		if (WSACleanup() != 0)
			throw NewSocketException("WSACleanup() in Winsock::~Winsock()");
	}	

	void ReceiveThread(Client* client)
	{
		while (client->connected)
		{
			byte tempBuffer[100];
			int len = recv(client->handle, (char*)tempBuffer, 100, NULL);
			if (len == SOCKET_ERROR)
			{
				SocketException se = NewSocketException("recv() in ReceiveThread()");
				client->SetLastException(se);
				break;
			}
			client->receiveBufferMutex.lock();
			for (int i = 0; i < len; i++)
				client->receiveBuffer.push_back(tempBuffer[i]);
			client->receiveBufferMutex.unlock();
		}
	}

	void AcceptThread(Server* server)
	{
		while (server->IsAccepting())
		{
			int i1 = sizeof(sockaddr_in);
			sockaddr_in address;
			SOCKET socket = accept(server->handle, (sockaddr*)&(address), &i1);
			if (socket == INVALID_SOCKET)
			{
				server->Stop();
				SocketException se = NewSocketException("accept() in AcceptThread()");
				server->SetLastException(se);
				break;
			}
			Client* newClient = new Client;
			newClient->lastSocketException.code = 0;
			newClient->connected = false;
			newClient->handle = socket;
			newClient->address = address;
			server->clientsMutex.lock();
			newClient->receiveThread = thread(ReceiveThread,newClient);
			server->clients.push_back(newClient);
			server->clientsMutex.unlock();
		}
	}

	/* * * * * * * * * * * * */
	/* * * C L I E N T * * * */
	/* * * * * * * * * * * * */
	Client::Client(string ip, unsigned short port)
	{
		connected = false;
		//create socket, AF_INET,SOCK_STREAM,NULL for tcp/ip v4, AF_INET6 for ip v6
		handle = socket(AF_INET, SOCK_STREAM, NULL);
		if (handle == INVALID_SOCKET)
			throw NewSocketException("socket() in Client::Client()");
		id = (long long)handle;
		address.sin_family = AF_INET;
		//this is how to create ip address from string
		inet_pton(AF_INET, ip.c_str(), &(address.sin_addr));
		//port
		address.sin_port = htons(port);		
	}

	void Client::Init(string ip, unsigned short port)
	{
		Disconnect();
		name = "";
		receiveBuffer.clear();
		receiveBuffer.shrink_to_fit();
		connected = false;
		//create socket, AF_INET,SOCK_STREAM,NULL for tcp/ip v4, AF_INET6 for ip v6
		handle = socket(AF_INET, SOCK_STREAM, NULL);
		if (handle == INVALID_SOCKET)
			throw NewSocketException("socket() in Client::Client()");
		id = (long long)handle;
		SecureZeroMemory(&address, sizeof(address));
		address.sin_family = AF_INET;
		//this is how to create ip address from string
		inet_pton(AF_INET, ip.c_str(), &(address.sin_addr));
		//port
		address.sin_port = htons(port);
	}

	void Client::Connect()
	{		
		//connect
		int result = connect(handle, (sockaddr*)&(address), sizeof(sockaddr_in));
		if (result == SOCKET_ERROR)
			throw NewSocketException("connect() in Client::Client()");
		//start receiving thread
		connected = true;
		receiveThread = thread(ReceiveThread, this);
		CheckForException();
	}	

	void Client::Disconnect()
	{
		if (!connected)
			return;
		connected = false;
		if (closesocket(handle) == SOCKET_ERROR)
			throw NewSocketException("closesocket() in Client::Disconnect()");
		handle = INVALID_SOCKET;
		ClearLastException();
	}

	vector<byte> Client::GetNextMessage()
	{
		CheckForException();
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

	unsigned int Client::GetNextMsgLen()
	{
		CheckForException();
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

	void Client::GetNextMessage(byte* dst)
	{
		CheckForException();
		receiveBufferMutex.lock();
		if (receiveBuffer.size() < 2)
		{
			receiveBufferMutex.unlock();
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
			receiveBufferMutex.unlock();
			return;
		}
		byte* it = receiveBuffer.data() + 2;
		for (int i = 0; i < len; i++)
			*(dst++) = *(it++);
		receiveBuffer.erase(receiveBuffer.begin(), receiveBuffer.begin() + 2 + len);
		receiveBufferMutex.unlock();
	}

	void Client::Send(byte* msg, unsigned short len)
	{
		CheckForException();
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
				throw NewSocketException("send() in Server::Send()");
			}
			index += sent;
		}
		delete[] smsg;
	}

	/* * * * * * * * * * * * */
	/* * S E R V E R * * * * */
	/* * * * * * * * * * * * */
	Server::Server(unsigned short port)
	{
		running = false;
		Init(port);
	}

	void Server::Init(unsigned short port)
	{
		Stop();
		ClearLastException();
		name = "";
		SecureZeroMemory(&address, sizeof(address));
		//new socket
		handle = socket(AF_INET, SOCK_STREAM, NULL);
		if (handle == INVALID_SOCKET)
			throw NewSocketException("socket() in Server::Server()");
		id = (long long)handle;
		//address struct
		address.sin_port = htons(port);
		address.sin_addr.s_addr = htonl(INADDR_ANY);
		address.sin_family = AF_INET;
		sockaddr* paddress = (sockaddr*)&address;
		if (bind((SOCKET)handle, paddress, (int)sizeof(sockaddr)) == SOCKET_ERROR)
			throw NewSocketException("bind() in Server::Server()");
	}

	void Server::Start(int maxQueue)
	{
		running = true;
		acceptingClients = true;
		//start listening
		if (listen(handle, maxQueue) == SOCKET_ERROR)
			throw NewSocketException("listen() in Server::Start()");
		//start accepting
		acceptThread = thread(wsl::AcceptThread, this);
		CheckForException();
	}

	void Server::Stop()
	{
		if (!running)
			return;
		running = false;
		acceptingClients = false;
		DisconnectAll();
		if (closesocket(handle) == SOCKET_ERROR)
			throw NewSocketException("closesocket() in Server::Stop()");
		acceptThread.join();
		acceptThread = 0;
	}

	void Server::Disconnect(Client* c)
	{
		int index = 0;
		for (int i = 0; i < (int)clients.size(); i++)
		{
			if (clients[i] == c)
			{
				index = i;
				break;
			}
		}
		if (closesocket(clients[index]->handle) == SOCKET_ERROR)
			throw NewSocketException("closesocket() in Server::Disconnect()");
		delete clients[index];
		clients.erase(clients.begin() + index);
	}

	void Server::DisconnectAll()
	{
		for (int i = 0; i < (int)clients.size(); i++)
		{
			if (closesocket(clients[i]->handle) == SOCKET_ERROR)
				throw NewSocketException("closesocket() in Server::Disconnect()");
			delete clients[i];
		}
		clients.clear();
	}	

	Server::~Server()
	{
		Stop();
	}
}
