#include "Private.h"

namespace wsl
{
	IntServer::IntServer(unsigned short port)
	{
		//initialize
		valid = true;
		running = false;
		name = "";
		SecureZeroMemory(&address, sizeof(address));
		//new socket
		handle = socket(AF_INET, SOCK_STREAM, NULL);
		if (handle == INVALID_SOCKET)
		{
			AppendException("socket() in Server::Init()", this);
			valid = false;
		}
		else
		{
			id = (long long)handle;
			//address struct
			address.sin_port = htons(port);
			address.sin_addr.s_addr = htonl(INADDR_ANY);
			address.sin_family = AF_INET;
			sockaddr* paddress = (sockaddr*)&address;
			if (bind((SOCKET)handle, paddress, (int)sizeof(sockaddr)) == SOCKET_ERROR)
			{
				valid = false;
				AppendException("bind() in Server::Init()", this);
			}
		}
	}

	void IntServer::Start(int maxQueue)
	{
		if (!valid)
			return;		
		//start listening
		if (listen(handle, maxQueue) == SOCKET_ERROR)
		{
			AppendException("listen() in Server::Start()", this);
			return;
		}
		running = true;
		acceptingClients = true;
		//start accepting
		acceptThread = thread(wsl::AcceptThread, this);
	}

	void IntServer::Stop()
	{
		if (!valid || !running)
			return;
		DisconnectAll();
		valid = false;
		running = false;
		acceptingClients = false;
		if (closesocket(handle) == SOCKET_ERROR)
			AppendException("closesocket() in Server::Stop()", this);
		acceptThread.join();
	}

	void IntServer::Disconnect(IntClient* c)
	{
		if (!valid || !running)
			return;
		int index = -1;
		clientsMutex.lock();
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
			AppendException("closesocket() in Server::Disconnect()", clients[index]);
		c->receiveThread.join();
		c->connected = false;
		delete c;
		//remove from vector
		clients.erase(clients.begin() + index);
		clientsMutex.unlock();
	}

	void IntServer::DisconnectAll()
	{
		if (!valid || !running)
			return;
		clientsMutex.lock();
		for (int i = 0; i < (int)clients.size(); i++)
		{
			if (closesocket(clients[i]->handle) == SOCKET_ERROR)
				AppendException("closesocket() in Server::DisconnectAll()", clients[i]);
			delete clients[i];
		}
		clients.clear();
		clientsMutex.unlock();
	}

	bool IntServer::GetNextMessage(ServerMessage* msg)
	{
		/////////////////yes new messages = return first one (least recent)
		if (messages.size() > 0)
		{
			*msg = messages[0];
			messages.erase(messages.begin());
			return true;
		}

		/////////////////no new messages = get messages from clients
		ServerMessage empty;
		//cases to return empty message
		clientsMutex.lock();
		if (!valid || !running || clients.size() == 0)
		{
			clientsMutex.unlock();
			return false;
		}
		//pool all connected clients
		for (auto c : clients)
		{
			vector<byte> vmsg;
			if (c->GetNextMessage(vmsg))
			{
				ServerMessage newMsg;
				newMsg.msg = vmsg;
				newMsg.sender.ip = c->GetIP();
				newMsg.sender.id = c->id;
				newMsg.sender.name = c->name;
				messages.push_back(newMsg);
			}
		}
		clientsMutex.unlock();

		//try again
		if (messages.size() > 0)
		{
			*msg = messages[0];
			messages.erase(messages.begin());
			return true;
		}
		else
			return false;
	}

	void IntServer::SendAll(byte* msg, unsigned short len)
	{
		for (auto c : clients)
			c->Send(msg, len);
	}

	IntServer::~IntServer()
	{
		Stop();
	}
}