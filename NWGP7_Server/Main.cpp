#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <mutex>
using namespace std;
#pragma comment(lib, "ws2_32.lib")

#define SERVERPORT 9000
#define BUFSIZE    512

void err_quit(const char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(char*)&lpMsgBuf, 0, NULL);
	MessageBoxA(NULL, (const char*)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

void err_display(const char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(char*)&lpMsgBuf, 0, NULL);
	printf("[%s] %s\n", msg, (char*)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

struct TryMatch {
	int client_id;
	bool is_pvp;
};
mutex matchMutex;
vector<TryMatch> MatchingArr;
thread_local int threadID;

struct ClientLocalParam {
	int thread_id;
	SOCKET sock;
};
DWORD WINAPI ProcessClient(LPVOID arg) 
{
	int retval;
	ClientLocalParam* param = (ClientLocalParam*)arg;
	threadID = param->thread_id;
	SOCKET client_sock = (SOCKET)param->sock;
	struct sockaddr_in clientaddr;
	char addr[INET_ADDRSTRLEN];
	int addrlen;
	bool isParticipant = false;

	char buff[BUFSIZE] = {};

	addrlen = sizeof(clientaddr);
	getpeername(client_sock, (struct sockaddr*)&clientaddr, &addrlen);
	inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));

	while (1) {
		int recv_siz = recv(client_sock, buff, BUFSIZE, 0);
		if (recv_siz > 0) {
			switch(buff[0]) {
			case 0: // 사용자 키보드 입력 정보를 서버에 알리는 프로토콜
				break;
			case 1: // 사용한 카드의 정보를 서버에 알리는 프로토콜
				break;
			case 2: // 클라이언트가 게임에 참여한다는걸 알리는 프로토콜
			{
				if (isParticipant == false) {
					TryMatch tm;
					tm.client_id = threadID;
					tm.is_pvp = buff[1];
					matchMutex.lock();
					MatchingArr.push_back(tm);
					matchMutex.unlock();
					printf("%d client participant in match!\n", threadID);
					isParticipant = true;
				}
				break;
			}
			}
		}
		else if(recv_siz == 0) {
			break;
		}
	}

	closesocket(client_sock);
	printf("[NWGP7 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n", addr, ntohs(clientaddr.sin_port));
	return 0;
}

DWORD WINAPI ProcessMatching(LPVOID arg) {
	while (1) {
		Sleep(500);
		// 매치 만들기
		matchMutex.lock();
		int pvp_stack = 0;
		TryMatch* pvp_clientsid[3] = {};

		int raid_stack = 0;
		TryMatch* raid_clientsid[3] = {};

		bool matching_pvp = false;
		bool matching_raid = false;
		for (int i = 0; i < MatchingArr.size(); ++i) {
			TryMatch& tm = MatchingArr[i];
			if (tm.client_id >= 0) {
				if (tm.is_pvp) {
					pvp_clientsid[pvp_stack] = &tm;
					pvp_stack += 1;
					if (pvp_stack == 3) {
						matching_pvp = true;
						break;
					}
				}
				else {
					raid_clientsid[raid_stack] = &tm;
					raid_stack += 1;
					if (raid_stack == 3) {
						matching_raid = true;
						break;
					}
				}
			}
			else {
				MatchingArr.erase(MatchingArr.begin() + i);
				i -= 1;
			}
		}

		if (matching_pvp) {
			// 방을 만들어 클라이언트에게 게임을 제공한다.

			printf("matching! client %d, %d, %d", raid_clientsid[0]->client_id, raid_clientsid[1]->client_id, raid_clientsid[2]->client_id);
			for (int i = 0; i < 3; ++i) {
				pvp_clientsid[i]->client_id = -1;
			}
		}

		if (matching_raid) {
			// 방을 만들어 클라이언트에게 게임을 제공한다.

			printf("matching! client %d, %d, %d", raid_clientsid[0]->client_id, raid_clientsid[1]->client_id, raid_clientsid[2]->client_id);
			for (int i = 0; i < 3; ++i) {
				raid_clientsid[i]->client_id = -1;
			}
		}
		matchMutex.unlock();
	}
}

int main(int argc, char* argv[]) 
{
	HANDLE hMatchingThread;
	hMatchingThread = CreateThread(NULL, 0, ProcessMatching, (LPVOID)0, 0, NULL);

	int retval;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		return 1;
	}

	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) {
		err_quit("socket()");
	}

	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR)
		err_quit("bind()");

	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR)
		err_quit("listen()");

	SOCKET client_sock;
	struct sockaddr_in clientaddr;
	int addrlen;
	HANDLE hThread[128] = {};
	int thread_id_up = 0;
	while (1) {
		// 새로 접속하는 클라이언트 처리
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}

		char addr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
		printf("\n[NWGP7 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n", addr, ntohs(clientaddr.sin_port));

		ClientLocalParam* param = new ClientLocalParam();
		param->sock = client_sock;
		param->thread_id = thread_id_up;
		hThread[thread_id_up] = CreateThread(NULL, 0, ProcessClient, (LPVOID)param, 0, NULL);

		if (hThread[thread_id_up] == NULL) {
			closesocket(client_sock);
			CloseHandle(hThread[thread_id_up]);
		}
		else thread_id_up += 1;
	}

	for (int i = 0; i < thread_id_up;++i) {
		CloseHandle(hThread[i]);
	}

	WSACleanup();
	CloseHandle(hMatchingThread);
}