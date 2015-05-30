#include <Windows.h>
#include <sstream>

//hardcoded
#include "../winsocklib/Public.h"
#pragma comment(lib, "../debug/winsockLib.lib")

#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")  

HWND mainWindow;
HWND startServerButton;
HWND connectButton;
HWND sendButton;
HWND disconnectButton;
HWND outputEdit;
HWND inputEdit;
HWND ipEdit;
HWND portEdit;
HWND labels[10];
HWND* label;
std::string outputString;

wsl::Notification notification;
wsl::Server server;
wsl::Client client;

void buttonClick(HANDLE button);
std::string getEditText(HWND edit);
void writeOutput(std::string str);
void displayErrorMessage(wsl::Notification se);
void readMessages();

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CLOSE:

		/* * * * stop server or client */
		if (server.IsRunning())
			server.Stop();
		if (client.IsConnected())
			client.Disconnect();
		/* * * * * * * * * * * * * * * */

		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_COMMAND:
		buttonClick((HWND)lParam);
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	/* * * * load library* * * * * */
	wsl::LoadWinsock();
	/* * * * * * * * * * * * * * * */

	outputString = "";
	const char className[] = "myWindowClass";
	label = labels;
	WNDCLASSEX wc;
	MSG Msg;
	ZeroMemory(&Msg, sizeof(Msg));
	ZeroMemory(&wc, sizeof(wc));
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	wc.lpszClassName = className;
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassEx(&wc))
	{
		MessageBox(NULL, "Window Registration Failed!", "Error!",
			MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	mainWindow = CreateWindowEx(WS_EX_CONTROLPARENT|WS_EX_CLIENTEDGE,
		className,
		"Server/Client test",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 400, 500,
		NULL, NULL, hInstance, NULL);

	if (mainWindow == NULL)
	{
		MessageBox(NULL, "Window Creation Failed!", "Error!",
			MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	connectButton = CreateWindowEx(NULL, "Button", "Connect Client", WS_GROUP | WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		10, 10, 100, 20, mainWindow, NULL, hInstance, NULL);
	startServerButton = CreateWindowEx(NULL, "Button", "Start Server", WS_TABSTOP | WS_VISIBLE | WS_CHILD ,
		120, 10, 100, 20, mainWindow, NULL, hInstance, NULL);
	disconnectButton = CreateWindowEx(NULL, "Button", "Disconnect", WS_TABSTOP | WS_VISIBLE | WS_CHILD,
		230, 10, 100, 20, mainWindow, NULL, hInstance, NULL);
	EnableWindow(disconnectButton, 0);
	UpdateWindow(disconnectButton);
	*(label++) = CreateWindowEx(NULL, "Static", "IP",  WS_VISIBLE | WS_CHILD ,
		10, 51, 100, 20, mainWindow, NULL, hInstance, NULL);
	*(label++) = CreateWindowEx(NULL, "Static", "Port",  WS_VISIBLE | WS_CHILD,
		230, 51, 100, 20, mainWindow, NULL, hInstance, NULL);
	ipEdit = CreateWindowEx(WS_EX_CLIENTEDGE, "Edit", "", WS_TABSTOP | WS_VISIBLE | WS_CHILD,
		10, 70, 210, 20, mainWindow, NULL, hInstance, NULL);
	SetWindowText(ipEdit, "127.0.0.1");
	portEdit = CreateWindowEx(WS_EX_CLIENTEDGE, "Edit", "", WS_TABSTOP | WS_VISIBLE | WS_CHILD,
		230, 70, 100, 20, mainWindow, NULL, hInstance, NULL);
	SetWindowText(portEdit, "50000");
	*(label++) = CreateWindowEx(NULL, "Static", "Output", WS_VISIBLE | WS_CHILD,
		10, 100, 100, 20, mainWindow, NULL, hInstance, NULL);
	outputEdit = CreateWindowEx(WS_EX_CLIENTEDGE, "Edit", "",  WS_VISIBLE | WS_CHILD | WS_VSCROLL |
		ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
		10, 130, 320, 200, mainWindow, NULL, hInstance, NULL);
	*(label++) = CreateWindowEx(NULL, "Static", "Input", WS_VISIBLE | WS_CHILD,
		10, 340, 100, 20, mainWindow, NULL, hInstance, NULL);
	inputEdit = CreateWindowEx(WS_EX_CLIENTEDGE, "Edit", "", WS_TABSTOP | WS_VISIBLE | WS_CHILD,
		10, 370, 210, 20, mainWindow, NULL, hInstance, NULL);
	sendButton = CreateWindowEx(NULL, "Button", "Send", WS_TABSTOP | WS_VISIBLE | WS_CHILD,
		230, 370, 100, 20, mainWindow, NULL, hInstance, NULL);

	HFONT hFontDef = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	HWND* it = &startServerButton;
	for (int i = 0; i < 18;i++,it++)
		SendMessage(*it, WM_SETFONT, (WPARAM)hFontDef, MAKELPARAM(TRUE, 0));

	ShowWindow(mainWindow, nCmdShow);
	UpdateWindow(mainWindow);
	while (WM_QUIT != Msg.message)
	{
		if (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
		{
			if (IsDialogMessage(mainWindow, &Msg)){}
			else
			{
				TranslateMessage(&Msg);
				DispatchMessage(&Msg);
			}
		}
		else
			readMessages();
	}

	/* * * * release library* * * * * */
	wsl::UnloadWinsock();
	/* * * * * * * * * * * * * * * */

	return Msg.wParam;
}

void readMessages()
{
	std::string newMessage = "";

	/* * * * read messages * * * * */
	//if current application is server...
	if (server.IsRunning())
	{
		//if there is a new message...
		wsl::ServerMessage msg;
		if (server.GetNextMessage(&msg))
			newMessage += msg.sender.ip + ": " + std::string(msg.msg.begin(), msg.msg.end());
		if (server.GetLastNotification(notification))
			displayErrorMessage(notification);
	}
	//if current application is client
	if (client.IsConnected())
	{
		//if there is a new message from server...
		std::vector<byte> msg;
		if (client.GetNextMessage(msg))
			newMessage += "Server: " + std::string(msg.begin(), msg.end());
		if (client.GetLastNotification(notification))
			displayErrorMessage(notification);
	}
	/* * * * * * * * * * * * * * * */

	if (newMessage != "")
		writeOutput(newMessage);
}

int getPort()
{
	int port = -1;
	int result = sscanf_s(getEditText(portEdit).c_str(), "%i", &port);
	if (result == 0 || port < 0 || port > 0xffff)
	{
		MessageBox(0, "Invalid port", "Error", MB_ICONEXCLAMATION);
		return -1;
	}
	return port;
}

void disableButton(HWND button)
{
	EnableWindow(button, 0);
	UpdateWindow(button);
}

void enableButton(HWND button)
{
	EnableWindow(button, 1);
	UpdateWindow(button);
}

void displayErrorMessage(wsl::Notification se)
{
	std::stringstream ssmsg;
	ssmsg << se.callStack << "\n\n";
	ssmsg << se.message << "\n\n";
	if (se.socket.id != NO_SENDER)
	{
		ssmsg << "Socket id: " << se.socket.id << "\n";
		ssmsg << "Socket ip: " << se.socket.ip << "\n";
		ssmsg << "Socket name: " << se.socket.name << "\n\n";
	}
	ssmsg << "Press YES to abort";
	std::stringstream sscode;
	sscode << "Code: " << se.code;
	if (MessageBox(0, ssmsg.str().c_str(), sscode.str().c_str(), MB_ICONERROR | MB_TASKMODAL | MB_YESNO | MB_DEFBUTTON2) == IDYES)
		abort();
}

void buttonClick(HANDLE button)
{
	if (button == startServerButton)
	{
		int port = getPort();
		if (port == -1)
			return;

		/* * * * start server* * * * * */
		server.Init(port);		
		server.Start();
		if (server.GetLastNotification(notification))
			displayErrorMessage(notification);
		/* * * * * * * * * * * * * * * */

		if (server.IsRunning())
		{
			writeOutput("Server started");
			SetWindowText(disconnectButton, "Stop Server");
			disableButton(startServerButton);
			disableButton(connectButton);
			enableButton(disconnectButton);
		}
	}
	else if (button == connectButton)
	{
		int port = getPort();
		if (port == -1)
			return;
		std::string ip = getEditText(ipEdit);

		/* * * * * connect client* * * */
		client.Init(ip, port);
		client.Connect();
		if (client.GetLastNotification(notification))
			displayErrorMessage(notification);
		/* * * * * * * * * * * * * * * */

		if (client.IsConnected())
		{
			SetWindowText(disconnectButton, "Disconnect client");
			writeOutput("Client connected");
			disableButton(connectButton);
			disableButton(startServerButton);
			enableButton(disconnectButton);
		}
	}
	else if (button == sendButton)
	{
		std::string msg = getEditText(inputEdit);

		/* * * *send message * * * * * */		
		if (server.IsRunning())
		{
			server.SendAll((byte*)msg.c_str(), msg.length());
			if (server.GetLastNotification(notification))
				displayErrorMessage(notification);
		}
		else if (client.IsConnected())
		{
			client.Send((byte*)msg.c_str(), msg.length());
			if (client.GetLastNotification(notification))
				displayErrorMessage(notification);
		}
		/* * * * * * * * * * * * * * * */

		writeOutput("Message sent");
	}
	else if (button == disconnectButton)
	{
		/* * * * stop server or client */		
		if (server.IsRunning())
		{
			server.Stop();
			writeOutput("Server stopped");
			if (server.GetLastNotification(notification))
				displayErrorMessage(notification);
		}
		if (client.IsConnected())
		{
			client.Disconnect();
			writeOutput("Client disconnected");
			if (client.GetLastNotification(notification))
				displayErrorMessage(notification);
		}
		/* * * * * * * * * * * * * * * */

		enableButton(connectButton);
		enableButton(startServerButton);
		disableButton(disconnectButton);
	}
}

std::string getEditText(HWND edit)
{
	int len = GetWindowTextLength(edit);
	char* buffer = new char[len+1];
	GetWindowText(edit, buffer, len+1);
	std::string result = buffer;
	delete[] buffer;
	return result;
}

void writeOutput(std::string str)
{
	outputString += str;
	outputString += "\r\n";
	SetWindowText(outputEdit, outputString.c_str());
}