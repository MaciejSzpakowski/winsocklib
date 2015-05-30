#include "Private.h"
#include "Public.h"

namespace wsl
{
	ServerMessage::ServerMessage()
	{
		sender.id = NO_SENDER;
	}

	///////SERVER/////////////

	Server::Server()
	{
		server = nullptr;
	}

	void Server::Init(unsigned short port)
	{
		delete server;
		server = new IntServer(port);
	}

	void Server::Start(int maxQueue)
	{
		if(server != nullptr)
			server->Start(maxQueue);
	}

	void Server::Start()
	{
		if (server != nullptr)
			server->Start(100);
	}

	void Server::Stop()
	{
		if (server != nullptr)
			server->Stop();
	}

	void Server::Disconnect(IntClient* c)
	{
		if (server != nullptr)
			server->Disconnect(c);
	}

	void DisconnectAll();

	bool Server::GetNextMessage(ServerMessage* msg)
	{
		if (server != nullptr)
			return server->GetNextMessage(msg);
		else
			return false;
	}

	void Server::SendAll(byte* msg, unsigned short len)
	{
		if (server != nullptr)
			server->SendAll(msg, len);
	}

	bool Server::GetLastNotification(Notification& notification)
	{
		if (server != nullptr)
			return server->GetLastNotification(notification);
		else
			return false;
	}

	string Server::GetName()
	{
		if (server != nullptr)
			return server->name;
		else
			return "";
	}

	void Server::SetName(string name)
	{
		if (server != nullptr)
			server->name = name;
	}

	long long Server::GetId()
	{
		if (server != nullptr)
			return server->id;
		else
			return -1;
	}

	void Server::SetId(long long id)
	{
		if (server != nullptr)
			server->id = id;
	}

	string Server::GetIP()
	{
		if (server != nullptr)
			return server->GetIP();
		else
			return "";
	}

	bool Server::IsRunning()
	{
		if (server != nullptr)
			return server->acceptingClients;
		else
			return false;
	}

	bool Server::IsAccepting()
	{
		if (server != nullptr)
			return server->acceptingClients;
		else
			return false;
	}

	Server::~Server()
	{
		delete server;
	}

	///////Client/////////////

	Client::Client()
	{
		client = nullptr;
	}

	void Client::Init(string ip, unsigned short port)
	{
		delete client;
		client = new IntClient(ip, port);
	}

	void Client::Connect()
	{
		if (client != nullptr)
			client->Connect();
	}

	void Client::Disconnect()
	{
		if (client != nullptr)
			client->Disconnect();
	}

	bool Client::GetNextMessage(vector<byte>& msg)
	{
		if (client != nullptr)
			return client->GetNextMessage(msg);
		else
			return false;
	}

	unsigned int Client::GetNextMsgLen()
	{
		if (client != nullptr)
			return client->GetNextMsgLen();
		else
			return 0;
	}

	void Client::GetNextMessage(byte* dst)
	{
		if (client != nullptr)
			client->GetNextMessage(dst);
	}

	bool Client::GetLastNotification(Notification& notification)
	{
		if (client != nullptr)
			return client->GetLastNotification(notification);
		else
			return false;
	}

	void Client::Send(byte* msg, unsigned short len)
	{
		if (client != nullptr)
			client->Send(msg, len);
	}

	string Client::GetName()
	{
		if (client != nullptr)
			return client->name;
		else
			return "";
	}

	void Client::SetName(string name)
	{
		if (client != nullptr)
			client->name = name;
	}

	long long Client::GetId()
	{
		if (client != nullptr)
			return client->id;
		else
			return -1;
	}

	void Client::SetId(long long id)
	{
		if (client != nullptr)
			client->id = id;
	}

	string Client::GetIP()
	{
		if (client != nullptr)
			return client->GetIP();
		else
			return "";
	}

	bool Client::IsConnected()
	{
		if (client != nullptr)
			return client->connected;
		else
			return false;
	}

	Client::~Client()
	{
		delete client;
	}
}