#include "../winsocklib/header.h"
//hardcoded for debug
#pragma comment(lib, "..\\debug\\winsockLib.lib")

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

wsl::Winsock* winsock;
wsl::Server* server;
wsl::Client* client;

void buttonClick(HANDLE button);
std::string getEditText(HWND edit);

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CLOSE:
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
	//load library
	winsock = new wsl::Winsock();
	server = nullptr;
	client = nullptr;

	const char className[] = "myWindowClass";
	label = labels;
	WNDCLASSEX wc;
	MSG Msg;
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

	// Step 2: Creating the Window
	mainWindow = CreateWindowEx(
		WS_EX_CLIENTEDGE,
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

	connectButton = CreateWindowEx(NULL, "Button", "Connect", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 
		10, 10, 100, 20, mainWindow, NULL, hInstance, NULL);
	startServerButton = CreateWindowEx(NULL, "Button", "Start Server", WS_TABSTOP | WS_VISIBLE | WS_CHILD ,
		120, 10, 100, 20, mainWindow, NULL, hInstance, NULL);
	disconnectButton = CreateWindowEx(NULL, "Button", "Disconnect", WS_TABSTOP | WS_VISIBLE | WS_CHILD,
		230, 10, 100, 20, mainWindow, NULL, hInstance, NULL);
	*(label++) = CreateWindowEx(NULL, "Static", "IP",  WS_VISIBLE | WS_CHILD ,
		10, 40, 100, 20, mainWindow, NULL, hInstance, NULL);
	*(label++) = CreateWindowEx(NULL, "Static", "Port",  WS_VISIBLE | WS_CHILD,
		230, 40, 100, 20, mainWindow, NULL, hInstance, NULL);
	ipEdit = CreateWindowEx(WS_EX_CLIENTEDGE, "Edit", "", WS_TABSTOP | WS_VISIBLE | WS_CHILD,
		10, 70, 210, 20, mainWindow, NULL, hInstance, NULL);
	portEdit = CreateWindowEx(WS_EX_CLIENTEDGE, "Edit", "", WS_TABSTOP | WS_VISIBLE | WS_CHILD,
		230, 70, 100, 20, mainWindow, NULL, hInstance, NULL);
	*(label++) = CreateWindowEx(NULL, "Static", "Output", WS_VISIBLE | WS_CHILD,
		10, 100, 100, 20, mainWindow, NULL, hInstance, NULL);
	outputEdit = CreateWindowEx(WS_EX_CLIENTEDGE, "Edit", "", WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_VSCROLL |
		ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
		10, 130, 320, 200, mainWindow, NULL, hInstance, NULL);
	*(label++) = CreateWindowEx(NULL, "Static", "Input", WS_VISIBLE | WS_CHILD,
		10, 340, 100, 20, mainWindow, NULL, hInstance, NULL);
	inputEdit = CreateWindowEx(WS_EX_CLIENTEDGE, "Edit", "", WS_TABSTOP | WS_VISIBLE | WS_CHILD,
		10, 370, 210, 20, mainWindow, NULL, hInstance, NULL);
	sendButton = CreateWindowEx(NULL, "Button", "Send", WS_TABSTOP | WS_VISIBLE | WS_CHILD,
		230, 370, 100, 20, mainWindow, NULL, hInstance, NULL);

	ShowWindow(mainWindow, nCmdShow);
	UpdateWindow(mainWindow);

	while (GetMessage(&Msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

	//release library
	delete winsock;

	return Msg.wParam;
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

void buttonClick(HANDLE button)
{
	if (button == startServerButton)
	{
		int port = getPort();
		if (port == -1)
			return;

		server = new wsl::Server(port);
		server->Start(10);
		EnableWindow(startServerButton, 0);
		UpdateWindow(startServerButton);
		EnableWindow(connectButton, 0);
		UpdateWindow(connectButton);
	}
	else if (button == connectButton)
	{
		int port = getPort();
		if (port == -1)
			return;
		std::string ip = getEditText(ipEdit);
		try
		{
			client = new wsl::Client(ip, port);
			client->Connect();
		}
		catch (std::string str)
		{
			MessageBox(0, str.c_str(), 0, 0);
		}
		EnableWindow(connectButton, 0);
		UpdateWindow(connectButton);
		EnableWindow(startServerButton, 0);
		UpdateWindow(startServerButton);
	}
	else if (button == sendButton)
	{
		std::string msg = getEditText(outputEdit);
		server->SendAll((byte*)msg.c_str(), msg.length());
	}
	else if (button == disconnectButton)
	{
		if (server != nullptr)
		{
			delete server;
			server = nullptr;
		}
		EnableWindow(connectButton, 1);
		UpdateWindow(connectButton);
		EnableWindow(startServerButton, 1);
		UpdateWindow(startServerButton);
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
