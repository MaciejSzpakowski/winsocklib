//WinSock2.h include must be before windows.h
#include <Ws2tcpip.h>
#include <WinSock2.h>
#include <cmath>
#include <windows.h>
#include <string>
#include <sstream>
#include <vector>
#include <mutex>
#include <thread>

namespace wsl
{
	using std::string;
	using std::vector;
	using std::thread;
	using std::mutex;
	using std::stringstream;

	class Socket;
	class Client;
	class Server;
	//main windows socket object
	class Winsock
	{
	private:
		WSADATA wsadata;
	public:
		//winsock object constructor
		//version: major,minor
		Winsock(int major, int minor);
		//winsock object constructor
		//version: 2.2
		Winsock();

		~Winsock();
	};

	struct SocketException
	{
		DWORD code;
		string callStack;
		string message;
	};

	class Socket
	{
	protected:
		string name;		
		vector<byte> sendBuffer;		
		sockaddr_in address;
		SOCKET handle;		
	public:
		string Socket::GetName(){
			return name; }

		void Socket::SetName(string str){
			name = str; }

		string GetIP(){
			char ip[100];
			inet_ntop(AF_INET, &(address.sin_addr), ip, 100);
			return string(ip);	}
	};

	class Client : public Socket
	{
	private:
		mutex receiveBufferMutex;
		vector<byte> receiveBuffer;
		thread receiveThread;
		bool connected;
		friend void AcceptThread(Server* server);
		friend void ReceiveThread(Client* client);
		friend class Server;
		//constructor for client used by server
		Client(SOCKET s, sockaddr_in sa)
		{
			connected = false;
			handle = s;
			address = sa;
		}
	public:		
		//constructor for actual client program
		Client(string ip, unsigned short port);
		//default construtor
		Client(){
			connected = false; }
		//used to initialize stack obj since assignment is not allowed
		void Init(string ip, unsigned short port);
		//connect to server
		void Connect();
		//disconnect from server
		void Disconnect();

		bool IsConnected(){
			return connected;	}

		vector<byte> GetNextMessage();

		//the most generic send method
		//msg: pointer to whatever bytes you want to send
		//len: length in bytes
		void Send(byte* msg, unsigned short len);

		void operator=(const Client &rhs)
		{
			string msg = "(Client)a = (Client)b\nCannot assign to Client\nUse Init instead";
			MessageBox(0, msg.c_str(), 0, MB_ICONEXCLAMATION);
			exit(0);
		}

		~Client(){}
	};

	class Server : public Socket
	{
	private:
		mutex clientsMutex;
		vector<Client*> clients;
		thread acceptThread;
		bool acceptingClients;
		bool running;
		friend void AcceptThread(Server* server);
	public:
		//server constructor
		//throws exception if socket() or bind() fails
		Server(unsigned short port);
		//default constructor
		Server(){
			running = false; }
		//used to initialize stack obj since assignment is not allowed
		void Init(unsigned short port);

		//starts listening and accepting clients
		//maxQueue: max number of clients waiting for connection
		void Start(int maxQueue);

		//starts listening and accepting clients
		void Start(){
			Start(100);	}

		//disconnect all clients and stop accepting new clients
		void Stop();

		//disconnect client, server will keep running and accepting new clients
		void Disconnect(Client* c);

		//disconnect all clients, server will keep running and accepting new clients
		void DisconnectAll();

		bool IsAccepting(){
			return acceptingClients; }		

		bool IsRunning(){
			return running;	}

		//send message to all clients
		void SendAll(byte* msg, unsigned short len)
		{
			for (auto c : clients)
				c->Send(msg, len);
		}

		vector<Client*> GetClients(){
			return clients;	}

		void operator=(const Server &rhs)
		{
			string msg = "(Server)a = (Server)b\nCannot assign to Server\nUse Init instead";
			MessageBox(0, msg.c_str(), 0, MB_ICONEXCLAMATION);
			exit(0);
		}

		~Server();
	};
}
