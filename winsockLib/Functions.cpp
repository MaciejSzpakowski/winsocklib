#include "Private.h"

namespace wsl
{
	void ReceiveThread(IntClient* client)
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
			client->receiveBufferMutex.lock();
			for (int i = 0; i < len; i++)
				client->receiveBuffer.push_back(tempBuffer[i]);
			client->receiveBufferMutex.unlock();
		}
	}

	void AcceptThread(IntServer* server)
	{
		while (server->acceptingClients)
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
			IntClient* newClient = new IntClient(socket, address, server);			
			server->clientsMutex.lock();
			server->clients.push_back(newClient);
			server->clientsMutex.unlock();
		}
	}

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

	SocketException* NewSocketExceptionPtr(const char* _callStack, BaseSocket* socket, int code)
	{
		SocketException* result = new SocketException;
		result->callStack = _callStack;
		if (code != -1)
			WSASetLastError(code);
		result->message = GetLastWinsockErrorMessage((PDWORD)&(result->code));
		if (socket != nullptr)
		{
			result->socket.id = socket->id;
			result->socket.ip = socket->GetIP();
			result->socket.name = socket->name;
		}
		else
			result->socket.id = -1;
		return result;
	}

	void LoadWinsock(int major, int minor, LPWSADATA wsadata)
	{
		//initialize winsock library
		int res = WSAStartup(MAKEWORD(major, minor), wsadata);
		if(res != 0)
		{
			SocketException* se = NewSocketExceptionPtr("", nullptr, res);
			std::stringstream ss;
			ss << "Failed to load winsock library, reason: " << se->message;
			MessageBox(0, ss.str().c_str(), "Winsock error", MB_ICONERROR | MB_TASKMODAL);
			delete se;
		}
	}

	void LoadWinsock()
	{
		//initialize winsock library
		WSADATA wsadata;
		int res = WSAStartup(MAKEWORD(2, 2), &wsadata);
		if (res != 0)
		{
			SocketException* se = NewSocketExceptionPtr("", nullptr, res);
			std::stringstream ss;
			ss << "Failed to load winsock library, reason: " << se->message;
			MessageBox(0, ss.str().c_str(), "Winsock error", MB_ICONERROR | MB_TASKMODAL);
			delete se;
		}
	}

	void UnloadWinsock()
	{
		int res = WSACleanup();
		if (res != 0)
		{
			SocketException* se = NewSocketExceptionPtr("", nullptr, res);
			std::stringstream ss;
			ss << "Failed to clean up winsock library, reason: " << se->message;
			MessageBox(0, ss.str().c_str(), "Winsock error", MB_ICONERROR | MB_TASKMODAL);
			delete se;
		}
	}	
}