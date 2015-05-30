#ifndef PUBLIC_H
#define PUBLIC_H
#include <string>
#include <vector>

namespace wsl
{
	using std::string;
	using std::vector;
	
	#define NO_SENDER -1

	struct IntServer;
	struct IntClient;
	//loads dll required by winsock
	//major and minor: version
	//wsadata: pointer to receiving wsadata struct, can be NULL
	void LoadWinsock(int major, int minor, LPWSADATA wsadata);
	//loads dll required by winsock, version 2.2
	void LoadWinsock();
	//releases dll code
	void UnloadWinsock();

	struct RawSocket
	{
		long long id;
		string name;
		string ip;
	};

	struct Notification
	{
		int code;
		string callStack;
		string message;
		RawSocket socket;
	};

	struct ServerMessage
	{
		RawSocket sender;
		vector<byte> msg;
		ServerMessage();
	};

	class Server
	{
	private:
		IntServer* server;
	public:

		//default constructor, does nothing
		Server();

		//server constructor
		void Init(unsigned short port);

		//starts listening and accepting clients
		//maxQueue: max number of clients waiting for connection
		void Start(int maxQueue);

		//starts listening and accepting clients
		void Start();

		//disconnect all clients and stop accepting new clients
		void Stop();

		//disconnect client, server will keep running and accepting new clients
		void Disconnect(IntClient* c);

		//disconnect all clients, server will keep running and accepting new clients
		void DisconnectAll();

		//returns true if new message has been written to *msg
		//returns false if there was no message
		bool GetNextMessage(ServerMessage* msg);

		//send message to all clients
		void SendAll(byte* msg, unsigned short len);

		//get and remove last notification
		//returns true if there was a notification to get, false otherwise
		bool GetLastNotification(Notification& notification);

		~Server();
		
		//getters and setters

		string GetIP();
		//no need to use this one, all ids are unique
		void SetId(long long id);
		long long GetId();
		void SetName(string name);
		string GetName();
		bool IsRunning();
		bool IsAccepting();
	};

	class Client
	{
	private:
		IntClient* client;
	public:
		//defult constructor, does nothing
		Client();

		//ip: server address
		//port: server port
		void Init(string ip, unsigned short port);

		//connect to server
		void Connect();

		//disconnect from server
		void Disconnect();

		//returns true if new message has been written to msg
		//returns false if there was no new message
		//bytes can be accessed directly by calling data() on the vector
		//it consumes the msg
		bool GetNextMessage(vector<byte>& msg);

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

		//get and remove last notification
		//returns true if there was a notification to get, false otherwise
		bool GetLastNotification(Notification& notification);

		~Client();

		//getters and setters

		string GetIP();
		//no need to use this one, all ids are unique
		void SetId(long long id);
		long long GetId();
		void SetName(string name);
		string GetName();
		bool IsConnected();
	};
}
#endif
