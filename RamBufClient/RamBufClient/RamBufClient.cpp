#include <cstdio>
#include<iostream>
#include<string>
#include <WS2tcpip.h>
#include<WinSock2.h>
#pragma comment(lib,"ws2_32.lib")
using namespace std;
const int PORT = 8000;
#define MaxBufSize 256

#define _CRT_SECURE_NO_WARINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

int main() {
	WSADATA wsd;
	WSAStartup(MAKEWORD(2, 2), &wsd);
	SOCKET SocketClient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SOCKADDR_IN  ClientAddr;

	ClientAddr.sin_family = AF_INET;
	//ClientAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	inet_pton(AF_INET, "127.0.0.1", (void*)&ClientAddr.sin_addr.S_un.S_addr);
	ClientAddr.sin_port = htons(PORT);
	
	int n = 0;
	n = connect(SocketClient, (struct sockaddr*)&ClientAddr, sizeof(ClientAddr));
	if (n == SOCKET_ERROR) {
		cout << "连接失败" << endl;
		return -1;
	}
	cout << "已经连接到服务器，可以向服务器发送消息了！" << endl;
	char info[256], SendBuff[MaxBufSize], RecvBuff[MaxBufSize];//缓冲区，发送缓冲区，接收缓冲区
	while (1) {
		cout << "请输入要发送的信息,按回车结束发送：" << endl;
		gets_s(info);//输入数据id(端口号),数据内容
		if (info[0] == '\0')
			break;
		strcpy(SendBuff, info);
		memset(info, 0, sizeof(info));
		int k = 0;
		k = send(SocketClient, SendBuff, sizeof(SendBuff), 0);
		memset(SendBuff, 0, sizeof(SendBuff));
		if (k < 0) {
			cout << "发送失败" << endl;
		}
		Sleep(500);
		int n = 0;
		n = recv(SocketClient, RecvBuff, sizeof(RecvBuff), 0);
		if (n > 0) {
			cout << "接收到来自服务器的消息为：" << RecvBuff << endl;
		}
		memset(RecvBuff, 0, sizeof(RecvBuff));
	}
	closesocket(SocketClient);
	WSACleanup();
	return 0;



}
