#ifndef CLASSES_H
#define CLASSES_H
//WinSock2.h include must be before windows.h
#include <Ws2tcpip.h>
#include <WinSock2.h>
#include <windows.h>
#include <string>
#include <sstream>
#include <vector>
#include <mutex>
#include <thread>
#pragma comment(lib, "ws2_32.lib")
#include "Public.h"

namespace wsl
{
	using std::string;
	using std::vector;
	using std::thread;
	using std::mutex;
	
	//prototypes
	struct IntClient;
	struct IntServer;
	struct BaseSocket;
	void ReceiveThread(IntClient* client);
	void AcceptThread(IntServer* server);
	string GetLastWinsockErrorMessage(PDWORD errorCode);
	SocketException* NewSocketExceptionPtr(const char* _callStack, BaseSocket* socket, int code);
	
	struct BaseSocket
	{
		bool valid;
		long long id;
		SocketException* lastSocketException;
		string name;
		sockaddr_in address;
		SOCKET handle;

		string GetIP();

		void ClearLastException();

		void SetLastException(const char* callStack, BaseSocket* socket);
	};

	struct IntClient : public BaseSocket
	{
		IntServer* server; //used for server side client connection
		mutex receiveBufferMutex;
		vector<byte> receiveBuffer;
		thread receiveThread;
		bool connected;

		//constructor for actual client program
		IntClient(string ip, unsigned short port);

		//constructor for server client
		IntClient(SOCKET socket, sockaddr_in _address, IntServer* _server);

		//connect to server
		void Connect();

		//disconnect from server
		void Disconnect();

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

		~IntClient();
	};

	struct IntServer : public BaseSocket
	{
		vector<ServerMessage> messages;
		mutex clientsMutex;
		vector<IntClient*> clients;
		thread acceptThread;
		bool acceptingClients;
		bool running;

		//server constructor
		IntServer(unsigned short port);

		//starts listening and accepting clients
		//maxQueue: max number of clients waiting for connection
		void Start(int maxQueue);

		//disconnect all clients and stop accepting new clients
		void Stop();

		//disconnect client, server will keep running and accepting new clients
		void Disconnect(IntClient* c);

		//disconnect all clients, server will keep running and accepting new clients
		void DisconnectAll();

		ServerMessage GetNextMessage();

		//send message to all clients
		void SendAll(byte* msg, unsigned short len);

		~IntServer();
	};
}

#endif
