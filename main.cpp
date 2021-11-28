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
	printf("MinWebServer 1.1\n\n      Server status: ");
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
	int mode; // 3 = OTHER, 2 = HEAD, 1 = GET, 0 = POST
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
	//res.mode = firstl[0] == "GET";
	if (firstl[0] == "POST")
		res.mode = 0;
	else if (firstl[0] == "GET")
		res.mode = 1;
	else if (firstl[0] == "HEAD")
		res.mode = 2;
	else
		res.mode = 3;
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

char cwd[512],cwtemp[1024],cwtemp1[1024];

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

bool typeMatch(string given, string sending) {
	vector<string> gtypes = splitLines(given.c_str(), ',',false,' '), st = splitLines(sending.c_str(), '/', true,' ');
	if (st.size() != 2) return false;
	for (auto i = gtypes.begin(); i != gtypes.end(); i++) {
		vector<string> gs = splitLines(i->c_str(), '/', true);
		if (gs.size() != 2) continue;
		if (gs[0] == st[0] || gs[0] == "*") {
			if (gs[1] == st[1] || gs[1] == "*")
				return true;
		}
	}
	return false;
}

int getSize(string filename) {
	FILE *f = fopen(filename.c_str(), "r");
	if (f == NULL) return 0;
	fseek(f, 0, SEEK_END);
	int res = ftell(f);
	fclose(f);
	return res;
}

char not_found[] = "<html><head><title>Page not found - 404</title></head><body><h1>404 Not found</h1><p>Requested page not found on this server.</p><hr /><p>MinServer 1.0</p></body></html>";
char not_supported[] = "<html><head><title>Not Implemented - 501</title></head><body><h1>Not Implemented</h1><p>Request not implemented by server.</p><hr /><p>MinServer 1.0</p></body></html>";

SOCKET s;

vector<string> defiles = {"index.html","index.htm"};
map<string, string> ctypes = { {".apk", "application/vnd.android"},  {".html","text/html"}, {".htm", "text/html"}, {".ico","image/ico"}, {".jpg", "image/jpg"}, {".jpeg", "image/jpeg"}, {".png", "image/apng"}, {".txt","text/plain"}, {".css", "text/css"}, {".js", "application/x-javascript"}, {".mp3", "audio/mpeg"}, {".wav", "audio/wav"}, {".mp4", "video/mpeg"} };

string defaultType = "text/plain";

void close1(void) {
	printState("Cleaning", FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN);
	closesocket(s);

	WSACleanup();
	printState("Closed", FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN);
	hold();
}

//#define VERBOSE_BEGIN true

string tp; // for debug

#ifndef VERBOSE_BEGIN

#define VERBOSE_BEGIN false

#endif

bool verbose = VERBOSE_BEGIN;

#define printf_verbose(...) if (verbose) printf(__VA_ARGS__)

int main(int argc, char* argv[]) {
	int rcv_bufsz = 4096;
	atexit(close1);
	GetCurrentDirectory(512, cwd);
	hnd = GetStdHandle(STD_OUTPUT_HANDLE);
	printState("Preparing", FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN);

	if (argc > 1) {
		for (int i = 1; i < argc; i++) {
			string argthis = argv[i];
			if (argthis == "--verbose" || argthis == "-v")
				verbose = true;
			if (argthis == "--buffer" || argthis == "-b") {
				rcv_bufsz = atoi(argv[i + 1]);
				i++;
			}
		}
	}

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
	printf("Preparing buffer with %d bytes...\n", rcv_bufsz);
	char *buf = new char[rcv_bufsz];
	printState("Listening", FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	printf("\n\nDocument tree available under %s\nDefault file name are ",cwd);
	// Show default file names
	for (auto i = defiles.begin(); i != defiles.end(); i++)
		printf("%s ", i->c_str());
	// end
	printf("\nInstalled types are ");
	for (auto i = ctypes.begin(); i != ctypes.end(); i++)
		printf("%s ", i->second.c_str());
	printf("\n\n");
	while (true) {
		//hold(); // currently
		memset(buf, 0, sizeof(buf));
		// by this way let's try this.
		DWORD test;
		// end
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
			printf_verbose("Resolved data: \n");
			printf_verbose("%s\n", buf);
			printf_verbose("== END ==\n");
			httpReq res = resolve(buf);
			SetConsoleTextAttribute(hnd, FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE);
			printf_verbose("Receive: Resolved %s %s request for %s\n", res.protocol.c_str(), res.mode ? "GET" : "POST", res.path.c_str());
			if (!res.mode) {
				// Try another receive
				// Always??
				int total_length = atoi(res.settings["Content-Length"].c_str());
				printf("Post Receive: Getting length %d\n", total_length);
				char post_buf[4096];
				printf_verbose("Post Receive: Start segements: ");
				for (int i = 0; i <= total_length; i += 4096) {
					memset(post_buf, 0, sizeof(post_buf));
					int r2 = recv(sr, post_buf, sizeof(post_buf), 0);
					printf_verbose("Post Receive: Received another %d byte(s): \n%s\n", r2, post_buf);
				}
				printf("Post Receive: End of post receive\n");
			}
			printf_verbose("Receive: External Parameters: \n");
			for (auto it = res.settings.begin(); it != res.settings.end(); it++) {
				printf_verbose("%s = %s\n", it->first.c_str(), it->second.c_str());
			}
			for (size_t i = 0; i < res.path.length(); i++)
				if (res.path[i] == '/')
					res.path[i] = '\\'; // to windows standard
			httpRst rst;
			//string sending = "";
			char *sending;
			rst.protocol = res.protocol;
			rst.rcode_info = "OK";
			rst.rcode = 200;
			rst.settings["Connection"] = "keep-alive";
			// also content-length then
			printf("\n");
			sprintf(cwtemp1, "%s%s", cwd, res.path.c_str());
			sprintf(cwtemp, "%s", cwtemp1);
			printf("Send: Resolved path: %s\n", cwtemp);
;			//if (!FilePathExists(cwtemp)) {
				// May redirect to index
			for (auto i = defiles.begin(); i != defiles.end() && !FilePathExists(cwtemp); i++) {
				sprintf(cwtemp, "%s\\%s", cwtemp1, i->c_str());
				printf_verbose("Send: Viewing directory. Trying index page: %s\n",cwtemp);
			}
			//rst.settings["Content-Type"] = "text/html";
			// read as ctypes
			tp = defaultType;

			// 1. Get extension
			string exte = cwtemp;
			for (size_t i = strlen(cwtemp) - 1; i >= 0; i--) {
				if (cwtemp[i] == '.') {
					exte = exte.substr(i);
					break;
				}
			}

			// 2. Get type
			if (!ctypes.count(exte)) {
				SetConsoleTextAttribute(hnd, FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN);
				printf("Warning: unknown file type: %s. Symbolled as %s.\n", exte.c_str(), defaultType.c_str());
			}
			else {
				tp = ctypes[exte];
			}
			SetConsoleTextAttribute(hnd, FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE);
			printf("Send: Confirming content type %s\n", tp.c_str());
			rst.settings["Content-Type"] = tp;

			if (res.settings.count("Accept") && !typeMatch(res.settings["Accept"], tp)) {
				SetConsoleTextAttribute(hnd, FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN);
				printf("Warning: file type not match: given %s with sending %s\n", res.settings["Accept"].c_str(), tp.c_str());
			}
			SetConsoleTextAttribute(hnd, FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE);

			int cws = 0; // File length
			// end
			if (!FilePathExists(cwtemp)) {
				rst.rcode = 404;
				rst.rcode_info = "Not found";
				SetConsoleTextAttribute(hnd, FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN);
				sending = not_found;
				printf("Warning: attempt to get a not exist page\n");
			}
			else if (!res.mode || res.mode == 3) {
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
				cws = getSize(cwtemp);
				FILE *rs = fopen(cwtemp, "rb"); // not 'r'
				fseek(rs, 0, SEEK_SET); // to head
				sending = new char[cws+10];
				memset(sending, 0, sizeof(sending));
				fread(sending, sizeof(char), cws, rs);
				sending[cws] = '\0';
				fclose(rs);
				//if (sending.length()) sending.pop_back(); // removing EOF
				if (strlen(sending) == 0) {
					SetConsoleTextAttribute(hnd, FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN);
					printf("Warning: attempt to send an empty page\n");
				}
				else {
#ifdef _DEBUG
					// Debugging, output file to test
					FILE *dt = fopen("tempraw", "wb");
					fwrite(sending, sizeof(char), cws, dt);
					fclose(dt);
#endif
				}
			}
			SetConsoleTextAttribute(hnd, FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE);
			rst.settings["Content-Length"] = to_string(cws);
			//string ec = (encode(rst) + "\r\n" + sending);
			//const char *ecl = ec.c_str();
			string ers = encode(rst);
			char *ecl = new char[cws+ers.length()+20]; // Line feeds and spaces
			//sprintf(ecl, "%s\r\n%s", ers.c_str(), sending); // Actual problem here - May can't use sprintf
			memset(ecl, 0, sizeof(ecl));
			strcat(ecl, ers.c_str());
			strcat(ecl, "\r\n");
			//strcat(ecl, sending);
			if (res.mode != 2) memcpy(ecl + strlen(ecl), sending, cws+1);
			printf("Send: Sending ...\n");
#ifdef _DEBUG
			FILE *dt = fopen("respraw", "wb");
			fwrite(ecl, sizeof(char), cws + ers.length() + 2, dt);
			fclose(dt);
#endif
			if (send(sr, ecl, cws+ers.length()+2, 0) == SOCKET_ERROR) {
				SetConsoleTextAttribute(hnd, FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN);
				printf("Warning: failed to send\n");
			}
			else {
				printf("Send: Connection completed.\n");
			}
			closesocket(sr);
		}
		//hold(); // currently
		//char tmp[] = "HTTP/1.1 200 OK\nconnection: close\n\n";
		//send(sr, tmp, sizeof(tmp), 0);//currently
	}

	close1();
	return 0;
}