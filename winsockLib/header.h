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
		Socket* socket;
		SocketException();
		SocketException(string callstack, Socket* socket);
		~SocketException();
	};

	class Socket
	{
	protected:
		bool valid;
		long long id;
		SocketException lastSocketException;
		string name;
		sockaddr_in address;
		SOCKET handle;
	public:
		//default constructor, no initialization
		Socket();
		//copy construtor to initialize invalid socket just to get ip, name and id
		Socket(const Socket& s);

		string GetName();

		void SetName(string str);

		long long GetId();

		//not necessary, all sockets are guaranteed to have a unique id
		void SetId(int val);

		string GetIP();

		void ClearLastException();

		void SetLastException(string callStack, Socket* socket);

		//to be called in catching thread
		void CheckForException();
	};

	class Client : public Socket
	{
	private:
		Server* server; //used for server side client connection
		mutex* receiveBufferMutex;
		vector<byte> receiveBuffer;
		thread* receiveThread;
		bool connected;
		friend void AcceptThread(Server* server);
		friend void ReceiveThread(Client* client);
		friend class Server;		
	public:
		//constructor for actual client program
		Client(string ip, unsigned short port);
		//default construtor, creates invalid client so it has to be reassigned later
		Client();
		//connect to server
		void Connect();
		//disconnect from server
		void Disconnect();

		bool IsConnected()
		{
			return connected;
		}

		//easy to use, returns message as vector of bytes
		//bytes can be accessed directly by calling data() on the vector
		//it consumes the msg
		vector<byte> GetNextMessage();

		//returns length in bytes of the next message in receive buffer
		//0 means that buffer is empty
		unsigned int GetNextMsgLen();

		//copy raw bytes to provided buffer (probably faster than returning msg as vector)
		//it will overflow buffer, make sure it's large enough by calling GetNextMsgLen()
		//it consumes the msg
		void GetNextMessage(byte* dst);

		//the most generic send method
		//msg: pointer to whatever bytes you want to send
		//len: length in bytes
		void Send(byte* msg, unsigned short len);

		~Client();
	};

	struct ServerMessage
	{
		bool empty;
		Client sender;
		vector<byte> msg;
		ServerMessage();
	};

	class Server : public Socket
	{
	private:
		vector<ServerMessage> messages;
		mutex* clientsMutex;
		vector<Client*> clients;
		thread* acceptThread;
		bool acceptingClients;
		bool running;
		friend void AcceptThread(Server* server);
	public:
		//server constructor
		Server(unsigned short port);
		//default constructor, creates invalid server so it must be reassign
		Server();

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

		bool IsAccepting()
		{
			return acceptingClients;
		}

		ServerMessage GetNextMessage();

		bool IsRunning()
		{
			return running;	
		}

		//send message to all clients
		void SendAll(byte* msg, unsigned short len);
		
		~Server();
	};
}
