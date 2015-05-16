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
		//throws exception if WSAStartup() fails
		Winsock(int major, int minor);
		//winsock object constructor
		//version: 2.2
		//throws exception if WSAStartup() fails
		Winsock();

		//winsock object destructor
		//throws exception if WSACleanup() fails
		~Winsock();
	};

	class Socket
	{
	protected:
		string name;		
		vector<byte> sendBuffer;		
		sockaddr_in address;
		SOCKET handle;		
	public:
		string GetName();
		void SetName(string str);

		string GetIP()
		{
			char ip[100];
			inet_ntop(AF_INET, &(address.sin_addr), ip, 100);
			return string(ip);
		}
	};

	class Client : public Socket
	{
	private:
		mutex receiveBufferMutex;
		vector<byte> receiveBuffer;
		thread receiveThread;
		bool receiving;
		friend void AcceptThread(Server* server);
		friend void ReceiveThread(Client* client);
		friend class Server;
		//constructor for client used by server
		Client(SOCKET s, sockaddr_in sa)
		{
			receiving = true;
			handle = s;
			address = sa;
		}
	public:		
		//constructor for actual client program
		Client(string ip, unsigned short port);
		//connect to server
		void Connect();

		vector<byte> GetNextMessage();

		//the most generic send method
		//msg: pointer to whatever bytes you want to send
		//len: length in bytes
		void Send(byte* msg, unsigned short len);
	};

	class Server : public Socket
	{
	private:
		mutex clientsMutex;
		vector<Client*> clients;
		thread acceptThread;
		bool acceptingClients;
		void Disconnect();
		friend void AcceptThread(Server* server);
	public:
		//server constructor
		//throws exception if socket() or bind() fails
		Server(unsigned short port);

		//starts listening and accepting clients
		void Start(int maxQueue);

		//start or stop accepting clients
		void AcceptClients(bool val)		{
			acceptingClients = val;		}

		bool IsAccepting(){
			return acceptingClients;		}		

		void SendAll(byte* msg, unsigned short len)
		{
			for (auto c : clients)
				c->Send(msg, len);
		}

		~Server();
	};
}
