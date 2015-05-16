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

	//formats message for throw
	string FormatErrorMessage(const char* message)
	{
		DWORD errorCode;
		string errorMessage = GetLastWinsockErrorMessage(&errorCode);
		stringstream result;
		result << "Code: " << errorCode << message << std::endl << errorMessage;
		return result.str();
	}

	Winsock::Winsock(int major, int minor)
	{
		//initialize winsock library
		if (WSAStartup(MAKEWORD(major, minor), &wsadata) != 0)
			throw FormatErrorMessage("WSAStartup() in Winsock::Winsock()");
	}

	Winsock::Winsock()
	{
		//initialize winsock library
		if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0)
			throw FormatErrorMessage("WSAStartup() in Winsock::Winsock()");
	}

	Winsock::~Winsock()
	{
		if (WSACleanup() != 0)
			throw FormatErrorMessage("WSACleanup() in Winsock::~Winsock()");
	}

	string Socket::GetName(){ return name; }

	void Socket::SetName(string str){ name = str; }

	void Server::Disconnect()
	{

	}

	Server::Server(unsigned short port)
	{
		//new socket
		handle = socket(AF_INET, SOCK_STREAM, NULL);
		if (handle == INVALID_SOCKET)
			throw FormatErrorMessage("socket() in Server::Server()");
		//address struct
		address.sin_port = htons(port);
		address.sin_addr.s_addr = htonl(INADDR_ANY);
		address.sin_family = AF_INET;
		sockaddr* paddress = (sockaddr*)&address;
		if (bind((SOCKET)handle, paddress, (int)sizeof(sockaddr)) == SOCKET_ERROR)
			throw FormatErrorMessage("bind() in Server::Server()");
	}

	//starts listening and accepting clients
	void Server::Start(int maxQueue)
	{
		acceptingClients = true;
		//start listening
		if (listen(handle, maxQueue) == SOCKET_ERROR)
			throw FormatErrorMessage("listen() in Server::Start()");
		//start accepting
		acceptThread = thread(wsl::AcceptThread, this);
	}

	Server::~Server()
	{
		Disconnect();
	}

	void Client::Send(byte* msg, unsigned short len)
	{
		int index = 0;
		//first 2 bytes is msg length as short
		byte* smsg = new byte[len + 2];
		memcpy(smsg, &len, 2);
		memcpy(smsg + 2, msg, len);
		//send function isnt guaranteed to send everything
		//compare return value with msg len and resend if necessary
		while (index < len)
		{
			smsg += index;
			int sent = send(handle, (const char*)smsg, len - index, 0);
			if (sent == SOCKET_ERROR)
			{
				delete[] smsg;
				throw FormatErrorMessage("send() in Server::Send()");
			}
			index += sent;
		}
		delete[] smsg;
	}

	void ReceiveThread(Client* client)
	{
		while (client->receiving)
		{
			byte tempBuffer[100];
			int len = recv(client->handle, (char*)tempBuffer, 100, NULL);
			if (len == SOCKET_ERROR)
				throw FormatErrorMessage("recv() in ReceiveThread()");
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
				throw FormatErrorMessage("accept() in AcceptThread()");
				continue;
			}
			Client* newClient = new Client(socket, address);
			server->clientsMutex.lock();
			newClient->receiveThread = thread(ReceiveThread,newClient);
			server->clients.push_back(newClient);
			server->clientsMutex.unlock();
		}
	}

	Client::Client(string ip, unsigned short port)
	{
		//create socket, AF_INET,SOCK_STREAM,NULL for tcp/ip v4, AF_INET6 for ip v6
		handle = socket(AF_INET, SOCK_STREAM, NULL);
		if (handle == INVALID_SOCKET)
			FormatErrorMessage("socket() in Client::Client()");
		address.sin_family = AF_INET;
		//this is how to create ip address from string
		inet_pton(AF_INET, ip.c_str(), &(address.sin_addr));
		//port
		address.sin_port = htons(port);		
	}

	void Client::Connect()
	{
		//connect
		if (connect(handle, (sockaddr*)&(address), sizeof(sockaddr_in)) != SOCKET_ERROR)
			FormatErrorMessage("connect() in Client::Client()");
		//start receiving thread
		receiveThread = thread(ReceiveThread, this);
	}

	vector<byte> Client::GetNextMessage()
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
		if (receiveBuffer.size() < len + 2)
		{
			receiveBufferMutex.unlock();
			return empty;
		}
		vector<byte> msg(receiveBuffer.begin() + 2, receiveBuffer.end());
		receiveBuffer.erase(receiveBuffer.begin(), receiveBuffer.begin() + 2 + len);
		receiveBufferMutex.unlock();
		return msg;
	}
}
