/*
Mininum Web Server.
by steambird 2021.
*/

#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <map>
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

vector<string> splitLines(const char *data, char spl = '\n', bool firstonly = false, char filter = '\r') {
	size_t len = strlen(data);
	string tmp = "";
	vector<string> res;
	bool flag = false;
	for (size_t i = 0; i < len; i++) {
		if (data[i] == spl && !(firstonly && flag)) {
			flag = true;
			res.push_back(tmp);
			tmp = "";
		}
		else if (data[i] != filter) {
			tmp += data[i];
		}
	}
	if (tmp.length()) res.push_back(tmp);
	return res;
}

struct httpReq {
	int mode; // 1 = GET, 0 = POST
	string path; // Path getting
	string protocol; // Usually 'HTTP/1.1'
	map<string, string> settings; // Request settings
};

struct httpRst {
	string protocol;
	int rcode;
	string rcode_info;
	map<string, string> settings; // Request settings
};

httpReq resolve(char *data) {
	vector<string> rd = splitLines(data);
	vector<string> firstl = splitLines(rd[0].c_str(), ' ');
	httpReq res;
	res.mode = firstl[0] == "GET";
	res.path = firstl[1];
	res.protocol = firstl[2];
	for (size_t i = 1; i < rd.size(); i++) {
		vector<string> tr = splitLines(rd[i].c_str(), ':', true);
		if (tr.size() < 2) {
			SetConsoleTextAttribute(hnd, FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN);
			printf("Warning: Error during resolving: Cannot resolve a setting\n");
			continue;
		}
		res.settings[tr[0]] = tr[1];
	}
	return res;
}

char cwd[512],cwtemp[1024];

string encode(httpRst data, string endline = "\r\n") {
	string r = data.protocol + " " + to_string(data.rcode) + " " + data.rcode_info + endline;
	for (auto i = data.settings.begin(); i != data.settings.end(); i++) {
		r += i->first + ": " + i->second + endline;
	}
	return r;
}


BOOL FindFirstFileExists(LPCTSTR lpPath, DWORD dwFilter)
{
	WIN32_FIND_DATA fd;
	HANDLE hFind = FindFirstFile(lpPath, &fd);
	BOOL bFilter = (FALSE == dwFilter) ? TRUE : fd.dwFileAttributes & dwFilter;
	BOOL RetValue = ((hFind != INVALID_HANDLE_VALUE) && bFilter) ? TRUE : FALSE;
	FindClose(hFind);
	return RetValue;
}

// Is file exists?
BOOL FilePathExists(LPCTSTR lpPath)
{
	return FindFirstFileExists(lpPath, FALSE) && (!FindFirstFileExists(lpPath, FILE_ATTRIBUTE_DIRECTORY));
}
/*
https ://blog.csdn.net/goodnew/article/details/8446575
*/

const char not_found[] = "<html><head><title>Page not found - 404</title></head><body><h1>404 Not found</h1><p>Requested page not found on this server.</p><hr /><p>MinServer 1.0</p></body></html>";
const char not_supported[] = "<html><head><title>Not Implemented - 501</title></head><body><h1>Not Implemented</h1><p>Request not implemented by server.</p><hr /><p>MinServer 1.0</p></body></html>";

SOCKET s;

void close1(void) {
	printState("Cleaning", FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN);
	closesocket(s);

	WSACleanup();
	printState("Closed", FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN);
	hold();
}

int main() {
	atexit(close1);
	GetCurrentDirectory(512, cwd);
	hnd = GetStdHandle(STD_OUTPUT_HANDLE);
	printState("Preparing", FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN);

	WSADATA ws;
	if (WSAStartup(MAKEWORD(2, 2), &ws)) {
		panic("Cannot start WSA");
	}

	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
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
	int ra_size = sizeof(ra), ret;
	const int rcv_bufsz = 4096;
	printf("Preparing buffer with %d bytes...\n", rcv_bufsz);
	char *buf = new char[rcv_bufsz];
	printState("Listening", FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	printf("\n\nDocument tree available under %s\nDefault file name is index.html\n\n",cwd);
	while (true) {
		//hold(); // currently
		memset(buf, 0, sizeof(buf));
		sr = accept(s, (SOCKADDR *)&ra, &ra_size);
		if (sr == INVALID_SOCKET) {
			SetConsoleTextAttribute(hnd, FOREGROUND_INTENSITY | FOREGROUND_RED);
			printf("Error: Failed to make connection\n");
		}
		SetConsoleTextAttribute(hnd, FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE);
		printf("Receive: Received a connection from %s\n", inet_ntoa(ra.sin_addr));
		ret = recv(sr, buf, rcv_bufsz, 0);
		if (ret > 0) {
			printf("Receive: Receiving %d byte(s)\n", ret);
			buf[ret] = '\0';
			// Received data in 'buf', resolve it
			printf("Receive: Resolving ...\n");
			httpReq res = resolve(buf);
			SetConsoleTextAttribute(hnd, FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE);
			printf("Receive: Resolved %s %s request for %s\n", res.protocol.c_str(), res.mode ? "GET" : "POST", res.path.c_str());
			printf("Receive: External Parameters: \n");
			for (auto it = res.settings.begin(); it != res.settings.end(); it++) {
				printf("%s = %s\n", it->first.c_str(), it->second.c_str());
			}
			for (size_t i = 0; i < res.path.length(); i++)
				if (res.path[i] == '/')
					res.path[i] = '\\'; // to windows standard
			httpRst rst;
			string sending = "";
			rst.protocol = res.protocol;
			rst.rcode_info = "OK";
			rst.rcode = 200;
			rst.settings["Connection"] = "keep-alive";
			rst.settings["Content-Type"] = "text/html";
			// also content-length then
			printf("\n");
			sprintf(cwtemp, "%s%s", cwd, res.path.c_str());
			printf("Send: Resolved path: %s\n", cwtemp);
;			if (!FilePathExists(cwtemp)) {
				// May redirect to index
				sprintf(cwtemp, "%s\\index.html", cwtemp);
				printf("Send: Viewing directory. Trying index page: %s\n",cwtemp);
			}
			if (!FilePathExists(cwtemp)) {
				rst.rcode = 404;
				rst.rcode_info = "Not found";
				SetConsoleTextAttribute(hnd, FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN);
				sending = not_found;
				printf("Warning: attempt to get a not exist page\n");
			}
			else if (!res.mode) {
				// POST is not supported yet
				rst.rcode = 501;
				rst.rcode_info = "Error";
				SetConsoleTextAttribute(hnd, FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN);
				sending = not_supported;
				printf("Warning: attempt to POST to server, which it's not supported\n");
			}
			else {
				SetConsoleTextAttribute(hnd, FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE);
				printf("Send: Loading resources.\n");
				FILE *rs = fopen(cwtemp, "rb"); // not 'r'
				while (!feof(rs)) sending += fgetc(rs);
				fclose(rs);
				if (sending.length()) sending.pop_back(); // removing EOF
				else {
					SetConsoleTextAttribute(hnd, FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN);
					printf("Warning: attempt to send an empty page\n");
				}
			}
			SetConsoleTextAttribute(hnd, FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE);
			rst.settings["Content-Length"] = to_string(sending.length());
			string ec = (encode(rst) + "\r\n" + sending);
			const char *ecl = ec.c_str();
			printf("Send: Sending: \n%s\n",ecl);
			send(sr, ecl, strlen(ecl), 0);
			closesocket(sr);
			printf("Send: Connection completed.\n");
		}
		//hold(); // currently
		//char tmp[] = "HTTP/1.1 200 OK\nconnection: close\n\n";
		//send(sr, tmp, sizeof(tmp), 0);//currently
	}

	printState("Cleaning", FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN);
	closesocket(s);

	WSACleanup();
	printState("Closed", FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN);
	hold();
	return 0;
}