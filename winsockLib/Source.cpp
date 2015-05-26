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

	SocketException::SocketException()
	{
		code = 0;
		callStack = "";
		message = "";
		socket = nullptr;
	}

	//formats socket exception to throw
	SocketException::SocketException(string _callstack, Socket* socket)
	{
		callStack = _callstack;
		message = GetLastWinsockErrorMessage(&code);
		if (socket != nullptr)
			socket = new Socket(*socket);
	}

	SocketException::~SocketException()
	{
		delete socket;
	}

	Winsock::Winsock(int major, int minor)
	{
		//initialize winsock library
		if (WSAStartup(MAKEWORD(major, minor), &wsadata) != 0)
		{
			throw SocketException("WSAStartup() in Winsock::Winsock()", nullptr);
		}
	}

	Winsock::Winsock()
	{
		//initialize winsock library
		if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0)
			throw SocketException("WSAStartup() in Winsock::Winsock()", nullptr);
	}

	Winsock::~Winsock()
	{
		if (WSACleanup() != 0)
			throw SocketException("WSACleanup() in Winsock::~Winsock()", nullptr);
	}	

	void ReceiveThread(Client* client)
	{
		while (client->connected)
		{
			byte tempBuffer[100];
			int len = recv(client->handle, (char*)tempBuffer, 100, NULL);
			if (len == SOCKET_ERROR)
			{
				client->SetLastException("recv() in ReceiveThread()", client);
				break;
			}
			client->receiveBufferMutex->lock();
			for (int i = 0; i < len; i++)
				client->receiveBuffer.push_back(tempBuffer[i]);
			client->receiveBufferMutex->unlock();
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
				server->SetLastException("accept() in AcceptThread()", server);
				break;
			}
			Client* newClient = new Client;
			newClient->server = server;
			newClient->valid = true;
			newClient->lastSocketException.code = 0;
			newClient->connected = true;
			newClient->handle = socket;
			newClient->address = address;
			newClient->receiveBufferMutex = new mutex;
			newClient->receiveThread = new thread(ReceiveThread, newClient);			
			server->clientsMutex->lock();			
			server->clients.push_back(newClient);
			server->clientsMutex->unlock();
		}
	}

	/* * * * * * * * * * * * */
	/* * * S O C K E T * * * */
	/* * * * * * * * * * * * */

	Socket::Socket()
	{
		valid = false;
	}

	Socket::Socket(const Socket& s)
	{
		id = s.id;
		address = s.address;
		name = s.name;
		valid = 0;
		handle = 0;
	}

	string Socket::GetName()
	{
		return name;
	}

	void Socket::SetName(string str)
	{
		name = str;
	}

	long long Socket::GetId()
	{
		return id;
	}

	void Socket::SetId(int val)
	{
		id = val;
	}

	string Socket::GetIP()
	{
		char ip[100];
		inet_ntop(AF_INET, &(address.sin_addr), ip, 100);
		return string(ip);
	}

	void Socket::ClearLastException()
	{
		lastSocketException.code = 0;
	}

	void Socket::SetLastException(string callStack, Socket* socket)
	{
		lastSocketException = SocketException(callStack, socket);
	}

	void Socket::CheckForException()
	{
		if (lastSocketException.code != 0)
			throw lastSocketException;
	}

	/* * * * * * * * * * * * */
	/* * * C L I E N T * * * */
	/* * * * * * * * * * * * */

	Client::Client(string ip, unsigned short port)
	{
		server = nullptr;
		valid = true;		
		receiveThread = nullptr;
		receiveBufferMutex = nullptr;
		name = "";
		connected = false;
		ClearLastException();
		//create socket, AF_INET,SOCK_STREAM,NULL for tcp/ip v4, AF_INET6 for ip v6
		handle = socket(AF_INET, SOCK_STREAM, NULL);
		if (handle == INVALID_SOCKET)
			throw SocketException("socket() in Client::Client()", this);
		id = (long long)handle;
		SecureZeroMemory(&address, sizeof(address));
		address.sin_family = AF_INET;
		//this is how to create ip address from string
		inet_pton(AF_INET, ip.c_str(), &(address.sin_addr));
		//port
		address.sin_port = htons(port);
	}

	Client::Client()
	{
		server = nullptr;
		connected = false;
		valid = false;
		receiveBufferMutex = nullptr;
		receiveThread = nullptr;
	}

	void Client::Connect()
	{
		if (!valid)
			return;
		//connect
		int result = connect(handle, (sockaddr*)&(address), sizeof(sockaddr_in));
		if (result == SOCKET_ERROR)
			throw SocketException("connect() in Client::Client()", this);
		//start receiving thread
		connected = true;
		receiveBufferMutex = new mutex;
		receiveThread = new thread(ReceiveThread, this);
		CheckForException();
	}	

	void Client::Disconnect()
	{
		if (!valid || !connected)
			return;
		connected = false;
		if (closesocket(handle) == SOCKET_ERROR)
			throw SocketException("closesocket() in Client::Disconnect()", this);
		receiveThread->join();
		handle = INVALID_SOCKET;
		ClearLastException();
		if (receiveThread)
			delete receiveThread;
		receiveThread = nullptr;
	}

	vector<byte> Client::GetNextMessage()
	{
		try
		{
			CheckForException();
		}
		catch (SocketException se)
		{
			//10054 means it was disconnected
			if (se.code == 10054)
			{
				Disconnect();
				throw se;
			}
			else
			{
				throw se;
			}
		}
		vector<byte> empty;
		receiveBufferMutex->lock();
		if (receiveBuffer.size() < 2)
		{
			receiveBufferMutex->unlock();
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
			receiveBufferMutex->unlock();
			return empty;
		}
		vector<byte> msg(receiveBuffer.begin() + 2, receiveBuffer.end());
		receiveBuffer.erase(receiveBuffer.begin(), receiveBuffer.begin() + 2 + len);
		receiveBufferMutex->unlock();
		return msg;
	}

	unsigned int Client::GetNextMsgLen()
	{
		CheckForException();
		receiveBufferMutex->lock();
		if (receiveBuffer.size() < 2)
		{
			receiveBufferMutex->unlock();
			return 0;
		}
		//get msg len from buffer, should be at the beginning
		byte lenRaw[2];
		lenRaw[0] = receiveBuffer[0];
		lenRaw[1] = receiveBuffer[1];
		receiveBufferMutex->unlock();
		unsigned short len = *(unsigned short*)lenRaw;
		return (unsigned int)len;
	}

	void Client::GetNextMessage(byte* dst)
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

	void Client::Send(byte* msg, unsigned short len)
	{
		if (!valid || !connected)
			return;
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
				throw SocketException("send() in Server::Send()", this);
			}
			index += sent;
		}
		delete[] smsg;
	}

	Client::~Client()
	{
		Disconnect();
		delete receiveThread;
		delete receiveBufferMutex;
		receiveThread = nullptr;
		receiveBufferMutex = nullptr;
	}

	/* * * * * * * * * * * * */
	/* * S E R V E R * * * * */
	/* * * * * * * * * * * * */
	
	ServerMessage::ServerMessage()
	{
		empty = true;
	}

	Server::Server()
	{
		acceptThread = nullptr;
		clientsMutex = nullptr;
		valid = false;
	}

	Server::Server(unsigned short port)
	{
		acceptThread = nullptr;
		clientsMutex = nullptr;
		//initialize
		valid = true;
		running = false;
		ClearLastException();
		name = "";
		SecureZeroMemory(&address, sizeof(address));
		clientsMutex = new mutex;	
		//new socket
		handle = socket(AF_INET, SOCK_STREAM, NULL);
		if (handle == INVALID_SOCKET)
			throw SocketException("socket() in Server::Server()", this);
		id = (long long)handle;
		//address struct
		address.sin_port = htons(port);
		address.sin_addr.s_addr = htonl(INADDR_ANY);
		address.sin_family = AF_INET;
		sockaddr* paddress = (sockaddr*)&address;
		if (bind((SOCKET)handle, paddress, (int)sizeof(sockaddr)) == SOCKET_ERROR)
			throw SocketException("bind() in Server::Server()", this);
	}

	void Server::Start(int maxQueue)
	{
		running = true;
		acceptingClients = true;
		//start listening
		if (listen(handle, maxQueue) == SOCKET_ERROR)
			throw SocketException("listen() in Server::Start()", this);
		//start accepting
		acceptThread = new thread(wsl::AcceptThread, this);
		CheckForException();
	}

	void Server::Stop()
	{
		if (!valid || !running)
			return;
		DisconnectAll();
		valid = false;
		running = false;
		acceptingClients = false;		
		if (closesocket(handle) == SOCKET_ERROR)
			throw SocketException("closesocket() in Server::Stop()", this);
		acceptThread->join();
		delete acceptThread;
		delete clientsMutex;
		acceptThread = nullptr;
		clientsMutex = nullptr;
	}

	void Server::Disconnect(Client* c)
	{
		if (!valid || !running)
			return;
		int index = 0;
		clientsMutex->lock();
		//find it in vector
		for (int i = 0; i < (int)clients.size(); i++)
		{
			if (clients[i] == c)
			{
				index = i;
				break;
			}
		}
		//close and delete
		if (closesocket(c->handle) == SOCKET_ERROR)
		{
			clientsMutex->unlock();
			throw SocketException("closesocket() in Server::Disconnect()", clients[index]);
		}
		c->receiveThread->join();
		c->connected = false;
		delete c;
		//remove from vector
		clients.erase(clients.begin() + index);
		clientsMutex->unlock();
	}

	void Server::DisconnectAll()
	{
		if (!valid || !running)
			return;
		clientsMutex->lock();
		for (int i = 0; i < (int)clients.size(); i++)
		{
			if (closesocket(clients[i]->handle) == SOCKET_ERROR)
			{
				clientsMutex->unlock();
				throw SocketException("closesocket() in Server::DisconnectAll()", clients[i]);
			}
			delete clients[i];
		}
		clients.clear();
		clientsMutex->unlock();
	}

	ServerMessage Server::GetNextMessage()
	{
		CheckForException();
		/////////////////yes new messages = return first one (least recent)
		if (messages.size() > 0)
		{
			ServerMessage result = messages[0];
			messages.erase(messages.begin());
			return result;
		}

		/////////////////no new messages = get messages from clients
		ServerMessage empty;		
		//cases to return empty message
		clientsMutex->lock();
		if (!valid || !running || clients.size() == 0)
		{
			clientsMutex->unlock();
			return empty;
		}
		//pool all connected clients
		try
		{
			for (auto c : clients)
			{
				vector<byte> msg = c->GetNextMessage();
				if (msg.size() != 0)
				{
					ServerMessage newMsg;
					newMsg.empty = false;
					newMsg.msg = msg;
					newMsg.sender.address = c->address;
					newMsg.sender.id = c->id;
					newMsg.sender.name = c->name;
					messages.push_back(newMsg);
				}
			}
		}
		catch (SocketException se)
		{
			clientsMutex->unlock();
			throw se;
		}
		clientsMutex->unlock();
		//try again
		if (messages.size() > 0)
		{
			ServerMessage result = messages[0];
			messages.erase(messages.begin());
			return result;
		}
		else
			return empty;
	}

	void Server::SendAll(byte* msg, unsigned short len)
	{
		for (auto c : clients)
			c->Send(msg, len);
	}

	Server::~Server()
	{
		Stop();		
	}
}
