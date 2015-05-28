========================================================================
    STATIC LIBRARY : winsockLib Project Overview
========================================================================
This is a simple TCP/IPv4 library. With this library, it is easy to
start server and connect clients to it. On server side, every client is
handled in separete thread so this library migth not be good if you want
to connect maybe more than couple dozens of clients.

How to use library.

Set up:
* Link library statically e.g. #pragma comment(lib,"library path")
* Include public header "Public.h"
* Use wsl namespace
* load winsock library by calling: wsl::LoadWinsock();

Server:

//start server
wsl::Server server;
server.Init(50000);
server.Start();
   
//receive messages
vector<byte> message;
message = server.GetNextMessage();

//send messages
unsigned char msg[] = "Hello there";
int len = strlen(msg);
server.SendAll(msg,len);

//stop when you are done
server.Stop();

Client:

//start client
wsl::Client client;
client.Init("192.168.0.101",50000);
client.Connect();

//receive messages
vector<byte> message;
message = server.GetNextMessage();

//send messages
unsigned char msg[] = "Hello there";
int len = strlen(msg);
client.Send(msg,len);

//stop when you are done
client.Disconnect();

All functions: