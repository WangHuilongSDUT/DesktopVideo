// DesktopVedioSev.cpp: 定义控制台应用程序的入口点。
//

#include <winsock2.h>
#include <stdlib.h>
#include <windows.h>
#include <iostream>


#include "CVideoPlayer.h"
#include "CDVTools.h"

#pragma comment(lib,"ws2_32.lib")
#pragma warning(disable:4996)

#define DEFAULT_BUFLEN 1024
#define DEFAULT_PORT 1997

using namespace std;

CVideoPlayer player;

void RunOneServer()
{
	HANDLE hMutex = ::CreateMutexA(NULL, false, "MutexVideoDesktopSercer");
	if (ERROR_ALREADY_EXISTS == GetLastError())
	{
		CloseHandle(hMutex);
		cout << "Only one instance can run!" << endl;
		exit(-1);
	}
}


void Commander(char *command)
{
	int volume;
	switch (command[0])
	{
	//播放控制
	case 'P':											//播放
		::ShowWindow(CDVTools::GetHandleBeforeDesktopIcon(), SW_SHOW);
		[](char *str) {
			int i = 2;
			while ('\0' != str[i])
			{
				str[i - 2] = str[i++];
			}
			str[i - 2] = str[i];
		}(command);
		player.OpenMedia(command);
		player.Play();
		break;
	case 'A':
		player.Pause();									//暂停
		break;
	case 'S':											//停止
		player.Stop();
		break;
	case 'C':											//快进/退
		if ('F' == command[2])
		{
			player.SetTime(player.GetTime() + 15000);
		}
		if ('B' == command[2])
		{
			player.SetTime(player.GetTime() - 15000);
		}
		break;
	case 'R':											//倍率
		if ('D' == command[2])
		{
			player.SetRate(2.0f);
		}
		if ('N' == command[2])
		{
			player.SetRate(1.0f);
		}
		if ('H' == command[2])
		{
			player.SetRate(0.5f);
		}
		break;
	//音量控制
	case 'V':											//设定音量
		volume=[](char *str)->int {
			int i = 2;
			while ('\0' != str[i])
			{
				str[i - 2] = str[i++];
			}
			str[i - 2] = str[i];
			return atoi(str);
		}(command);
		player.SetVolume(volume);
		break;
	case 'M':											//
		player.Mute(!player.GetMute());
		break;
	case 'B':
		::ShowWindow(CDVTools::GetHandleBeforeDesktopIcon(),SW_HIDE);
	}
}



void SocketLoop()
{
	WSADATA wsaData = {0};
	SOCKET servSock;
	sockaddr_in sockAddr = { 0 };
	SOCKADDR clientAddr = {0};
	SOCKET clientSock=0;
	char strReady[] = "Server Ready";
	int nSize = 0;


	auto err=WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (err != 0) {
		cout<<"WSAStartup failed with error: "<< err<<endl;
		goto End;
	}
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
		cout << "Could not find a usable version of Winsock.dll" << endl;
		goto End;
	}

	servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (servSock == INVALID_SOCKET) {
		cout<<"socket failed with error:"<< WSAGetLastError()<<endl;
		goto End;
	}

	sockAddr.sin_family = PF_INET;
	sockAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	sockAddr.sin_port = htons(DEFAULT_PORT);

	if (SOCKET_ERROR == bind(servSock, (SOCKADDR*)&sockAddr, sizeof(SOCKADDR)))
	{
		cout << "bind function failed with error " << WSAGetLastError() << endl;
		goto End;
	}

	if (SOCKET_ERROR == listen(servSock, SOMAXCONN))
	{
		cout << "listen function failed with error: " << WSAGetLastError() << endl;
		goto End;
	}

	nSize = sizeof(SOCKADDR);
	clientSock = accept(servSock, (SOCKADDR*)&clientAddr, &nSize);
	if (clientSock == INVALID_SOCKET) {
		cout<<"accept failed with error: "<< WSAGetLastError()<<endl;
		goto End;
	}
	if (SOCKET_ERROR == send(clientSock, strReady, sizeof(strReady), 0))
	{
		cout << "send failed with error: %d\n" << WSAGetLastError() << endl;
		goto End;
	}

	while (true)
	{
		char buff[DEFAULT_BUFLEN] = {0};
		int len=recv(clientSock, buff, DEFAULT_BUFLEN, 0);
		if (len == 0)
		{

			closesocket(clientSock);
			closesocket(servSock);
			WSACleanup();
			exit(0);
		}
		if (len < 0)
		{
			cout << "recv failed: " << WSAGetLastError() << endl;
			goto End;
		}
		Commander(buff);

	}

	End:
	closesocket(clientSock);
	closesocket(servSock);
	WSACleanup();
	exit(-1);
}

void RedirectOutput()
{
	if (NULL == freopen("log.txt", "w", stdout))
	{
		MessageBoxA(NULL, "Output Redirect Failed!", "Error:", MB_OK | MB_ICONERROR);
		exit(-1);
	}
}

void ForbbidenHumanStart(int argc, char **argv)
{
	if (argc != 2)
	{
		cout << "Command line is not crrect!" << endl;
		exit(-1);
	}
	if (!strcmp("sstart",argv[1]))
	{
		cout << "start" << endl;
		return;
	}
	cout << "Forbidden human start!" << endl;
	exit(-1);
}

int main(int argc,char **argvs)
{
	//启动前检查
	RedirectOutput();														//输出重定向
	RunOneServer();															//单实例运行
	ForbbidenHumanStart(argc,argvs);										//只允许客户端启动
	player.SetWindow(CDVTools::GetHandleBeforeDesktopIcon());				//播放器设置桌面窗口
	player.SetEndCallback([](const struct libvlc_event_t* p_event, void* p_data) {
		::ShowWindow(CDVTools::GetHandleBeforeDesktopIcon(), SW_HIDE);
	});
	SocketLoop();															//通信
	return 0;
}

