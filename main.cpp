/*
Mininum Web Server.
by steambird 2021.
*/

#define _CRT_SECURE_NO_WARNINGS
#include <cstdio>
#include <cstdlib>
#include <string>
#include <WinSock2.h>
#include <Windows.h>
using namespace std;

#pragma comment(lib, "ws2_32.lib")

HANDLE hnd;

inline void hold(void) {
	while (1);
}

void printState(string state, WORD color) {
	system("cls");
	SetConsoleTextAttribute(hnd, FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	printf("MinWebServer 1.0\n\n      Server status: ");
	SetConsoleTextAttribute(hnd, color);
	printf("%s\n", state.c_str());
}

void panic(string info) {
	printState("Fail", FOREGROUND_INTENSITY | FOREGROUND_RED);
	printf("\nError information:\n\n%s\n",info.c_str());
	hold();
}

bool dym_bind(int port, SOCKET *soc) {
	sockaddr_in sa;
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port); // set host
	sa.sin_addr.S_un.S_addr = INADDR_ANY;
	if (bind(*soc, (LPSOCKADDR)&sa, sizeof(sa)) == SOCKET_ERROR) {
		return false;
	}
	else {
		return true;
	}
}

int main() {
	hnd = GetStdHandle(STD_OUTPUT_HANDLE);
	printState("Preparing", FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN);

	WSADATA ws;
	if (WSAStartup(MAKEWORD(2, 2), &ws)) {
		panic("Cannot start WSA");
	}

	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET) {
		panic("Cannot start socket");
	}
	printf("\n\n");

	int sc = 80;
	while (!dym_bind(sc, &s)) {
		printf("Cannot bind port %d.\nWhat port would you like to bind? ",sc);
		scanf("%d", &sc);
	}

	if (listen(s, 5) == SOCKET_ERROR) {
		panic("Cannot listen");
	}

	SOCKET sr;
	sockaddr_in ra;
	int ra_size = sizeof(ra);
	const int rcv_bufsz = 4096;
	printf("Preparing buffer with %d bytes...\n", rcv_bufsz);
	char *buf = new char[rcv_bufsz];
	while (true) {
		printState("Listening", FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		hold(); // currently
	}

	printState("Cleaning", FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN);
	closesocket(s);

	WSACleanup();
	printState("Closed", FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN);
	hold();
	return 0;
}