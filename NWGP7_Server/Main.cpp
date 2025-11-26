#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <mutex>
#include <queue>
#include <atomic>
using namespace std;
#pragma comment(lib, "ws2_32.lib")

#define SERVERPORT 9000
#define BUFSIZE    512

enum ClientToServer_ProtocolType {
	CTS_PT_KeyInput = 0,
	CTS_PT_PlayCard = 1,
	CTS_PT_Participant = 2,

};

enum PlayerSyncBase {
	SYNC_HP = 0,
	SYNC_MAX_MANA = 1,
	SYNC_MANA = 2,
	SYNC_DEFFENCE = 3,
	SYNC_ATTACK = 4,
	SYNC_POS = 5,

	SYNC_HAND_SLOT_0 = 9,
	SYNC_HAND_SLOT_1 = 10,
	SYNC_HAND_SLOT_2 = 11,
	SYNC_HAND_SLOT_3 = 12,
	SYNC_HAND_SLOT_4 = 13,

	SYNC_PLAYER_DEATH = 14,
	SYNC_CUTTING = 15,
	SYNC_INVINCIBLE = 16,
	SYNC_IS_CASTING_ONEPUNCH = 17,
	SYNC_ONEPUNCH_TIMER = 18,
	SYNC_ONEPUNCH_DAMAGE = 19,
	SYNC_PARING_MOMENT = 20,

};

const int PLAYER_SYNC_STRIDE = 32;

enum ServerToClient_ProtocolType {
	// Game Event
	STC_PT_ThrowCard = 96,
	STC_PT_Effect_Event = 97,
	STC_PT_Effect_Pos = 98,

	// Game Init
	STC_PT_InitGame = 99,

	// boss sync
	STC_Sync_Boss_Defence = 100,
	STC_Sync_Boss_Hp = 101,
	STC_Sync_Boss_Death = 102,
	STC_Sync_Boss_Awakening = 103,
	STC_Sync_Boss_ID = 104,
	
	// MapData
	STC_Sync_MapData = 116,
};

struct STC_OP {
	int ptype;

	struct STC_OP_PlayerPos {
		short posx;
		short posy;
	};

	union {
		STC_OP_PlayerPos op_playerpos;
	};

	void SetPtype_SyncBattleOP(int partID, PlayerSyncBase optype) {
		ptype = partID * 32 + optype;
	}
};

struct OP {
	int ptype;

	struct OP_KEY {
		char playerID;
		char key;
	};
	struct OP_PLAYCARD {
		short playerID;
		short cardindex;
		int enemyID;
		short pos_x;
		short pos_y;
	};
	struct OP_PART {
		bool ispvp;
	};

	union {
		OP_KEY op_key;
		OP_PLAYCARD op_playcard;
		OP_PART op_part;
	};

	OP() {}
	OP(const OP& n) {
		ptype = n.ptype;
		op_playcard.playerID = n.op_playcard.playerID;
		op_playcard.cardindex = n.op_playcard.cardindex;
		op_playcard.enemyID = n.op_playcard.enemyID;
		op_playcard.pos_x = n.op_playcard.pos_x;
		op_playcard.pos_y = n.op_playcard.pos_y;
	}
};

class QNode {
public:
	OP v;
	QNode* next;

	QNode() {
		v.ptype = -1;
		next = nullptr;
	}

	QNode(OP V, QNode* nextptr) {
		v = V;
		next = nextptr;
	}
};

class CoarseGainQueue {
public:
	mutex mut;
	QNode* head;
	QNode* tail;

	CoarseGainQueue() {
		head = new QNode();
		tail = head;
	}

	void enq(OP v) {
		QNode* node = new QNode(v, nullptr);
		mut.lock();
		tail->next = node;
		tail = node;
		mut.unlock();
	}

	OP deq() {
		mut.lock();
		OP r;
		r.ptype = -1;
		if (head->next == nullptr) {
			mut.unlock();
			return r;
		}
		r = head->next->v;
		QNode* qn = head->next;
		delete head;
		head = qn;
		mut.unlock();
		return r;
	}

	void clear() {
		mut.lock();
		QNode* node = head->next;
		while (node != nullptr) {
			QNode* qn = node;
			node = node->next;
			delete qn;
		}
		head->next = nullptr;
		head->v.ptype = -1;
		tail = head;
		mut.unlock();
	}

	void print20() {
		//int cnt = 0;
		//QNode* node = head->next;
		//while (node != nullptr) {
		//	printf("%d ", node->v);
		//	node = node->next;

		//	cnt += 1;
		//	if (cnt > 20) break;
		//}
		//printf("\n");
	}
};

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

struct CardData {
	int id = 0;
};

struct Pos {
	int x;
	int y;

	Pos() {

	}

	Pos(int X, int Y) :
		x{ X }, y{ Y }
	{

	}
};

struct BossData {
	int defence = 0;
	int hp = 100;
	bool death = false;
	bool bossAwakening = false;
	int id = 0; // 보스 종류
	int patturn = 0;
	int healEnergy = 0;
	int DamageStack = 0;
	float attackTimer = 0.0f;

	bool bossmode2 = false;
	bool bossmode3 = false;
	int boss_statck = 0;
	bool nodamageMode = false; // 보스 무적 모드

	int attackState = 0; // 0: 아무것도 아님 1: 경고 2: 데미지공격
	float currentStateDuration = 3.0f; // 현재 상태 몇초 지속
};

struct PlayerData {
	int hp = 100;
	float maxMana = 3;
	float mana = 3;
	int defence = 0;
	int attack = 0;
	Pos pos;
	CardData hand[5];

	// 상태 플래그
	bool playerdeath = false;
	bool cutting = false;
	bool invincible = false;
	bool isCastingOnePunch = false;
	float onePunchCastTimer = 0.0f;
	int onePunchStoredDamage = 0;
	bool ParingMoment = false;
	void* clientData;

	void Move(int dx, int dy);
};

struct GameState {
public:
	// 게임 화면 상태
	bool PvEMode = false;  // true 일떄 레이드 false일때 pvp

	bool GameClear = false;

	// 전투 상태
	static constexpr int playerCount = 3;
	PlayerData players[playerCount]; // players data

	//static constexpr int clientindex = 0;
	//PlayerData* player = &player[clientindex]; // client's player

	BossData boss; // boss data

	bool tempstop = false; // 일시정지
	bool dontattackcard = false;

	// 격자 맵 데이터 및 보스 공격 표시
	static const int GRID_SIZE = 5;       // 고정된 5x5 격자
	int mapData[GRID_SIZE][GRID_SIZE] = { 0 };  // 0=빈칸, 1=이동가능, 2=보스공격

};

struct BattleData;

class GameLogic {
public:
	void Initialize(GameState& state);
	void StartBattle(GameState& state); // 맵에서 전투 시작

	void Update(GameState& state, BattleData& bd);

	void UpdatePvE(GameState& state, float deltaTime, BattleData& bd);
	void UpdatePvP(GameState& state, float deltaTime, BattleData& bd);

	void CheckWinLossConditionsPvP(GameState& state, BattleData& bd);
	void CheckWinLossConditionsPvE(GameState& state, BattleData& bd);

	void AttackWarning(GameState& state, BattleData& bd);
	void AttackOnRandomGreed(GameState& state, int damage, BattleData& bd);
	void MapStateRepare(GameState& state, BattleData& bd);

	// if realtime
	void GameInit(SOCKET sock, const GameState& state, int playerindex, PlayerData& pdata);
	void UpdateBattle_RealTime(GameState& state, float deltaTime, BattleData& bd);
	void ExecuteEnemyAI(GameState& state, float deltaTime, BattleData& bd);
	void UpdateBuffsAndTimers(GameState& state, float deltaTime);

	void PlayCard(GameState& state, int cardIndex, BattleData& bd, int playerIndex = 0); // 카드 사용
	void PlayCardLogic(GameState& state, int cardID, BattleData& bd, int playerindex = 0, bool usedByMonster = false); // 카드 사용

	void CardUpdate(GameState& state, float deltaTime, BattleData& bd);

	void ApplyDamageToPlayer(GameState& state, int damage, int playerindex, BattleData& bd);
	void ApplyDamageToBoss(GameState& state, int playerID, int rawDamage, BattleData& bd);
	void ApplyDefenseToEnemy(GameState& state, int defense, BattleData& bd);
	void HealBoss(GameState& state, int healAmount, BattleData& bd);

	void RecordSTCPacket(BattleData& bd, char type, void* data, int dataSize);

};

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

struct BattleData {
	CoarseGainQueue OPQueue;
	int clientsID[3] = {};
	HANDLE hBattleThread;

	GameState gameState;
	GameLogic gameLogic;

	queue<vector<char>> STCQueue;
	mutex STCQueueMutex;
};

struct ClientData {
	HANDLE hThread;
	ClientLocalParam param;
	BattleData* bd = nullptr;
	int ParticipateID; // player's id in Match. 0, 1, 2
};

ClientData clients[128] = {};
int thread_id_up = 0;
BattleData battles[128] = {};
int battle_id_up = 0;
thread_local ClientData* clientData;

void PlayerData::Move(int dx, int dy) {
	ClientData* client = (ClientData*)clientData;
	pos.x += dx;
	pos.y += dy;
	if (pos.x < -2) {
		pos.x = -2;
	}
	else if (pos.x > 2) {
		pos.x = 2;
	}

	if (pos.y < -2) {
		pos.y = -2;
	}
	else if (pos.y > 2) {
		pos.y = 2;
	}

	STC_OP op;
	op.SetPtype_SyncBattleOP(client->ParticipateID, PlayerSyncBase::SYNC_POS);
	op.op_playerpos.posx = pos.x;
	op.op_playerpos.posy = pos.y;
	//Send To All Clients
	for (int i = 0; i < 3; ++i) {
		int retval = send(clients[client->bd->clientsID[i]].param.sock, (char*)&op, sizeof(STC_OP), 0);
	}
}

DWORD WINAPI ProcessClient(LPVOID arg) 
{
	int retval;
	ClientLocalParam* param = (ClientLocalParam*)arg;
	clientData = &clients[param->thread_id];
	memcpy_s(&clientData->param, sizeof(ClientLocalParam), param, sizeof(ClientLocalParam));
	
	SOCKET client_sock = (SOCKET)clientData->param.sock;
	struct sockaddr_in clientaddr;
	char addr[INET_ADDRSTRLEN];
	int addrlen;
	bool isParticipant = false;

	char buff[BUFSIZE] = {};

	addrlen = sizeof(clientaddr);
	getpeername(client_sock, (struct sockaddr*)&clientaddr, &addrlen);
	inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));

	while (1) {
		OP CurrOP;
		int recv_siz = recv(client_sock, (char*) & CurrOP, sizeof(OP), 0);
		if (recv_siz > 0) {
			if (CurrOP.ptype != 2) {
				clientData->bd->OPQueue.enq(CurrOP);
			}
			else {
				//CTS_PT_Participant;
				if (isParticipant == false) {
					TryMatch tm;
					tm.client_id = clientData->param.thread_id;
					tm.is_pvp = CurrOP.op_part.ispvp;
					matchMutex.lock();
					MatchingArr.push_back(tm);
					matchMutex.unlock();
					printf("%d client participant in match!\n", threadID);
					isParticipant = true;
				}
			}

			//switch(CurrOP.ptype) {
			//case 0: // 사용자 키보드 입력 정보를 서버에 알리는 프로토콜
			//{
			//	OP op;
			//	op.ptype = CTS_PT_KeyInput;
			//	op.op_key.playerID = clientData->ParticipateID;
			//	op.op_key.key = buff[1];
			//	
			//}
			//	break;
			//case 1: // 사용한 카드의 정보를 서버에 알리는 프로토콜
			//{
			//	OP op;
			//	op.ptype = CTS_PT_PlayCard;
			//	op.op_playcard.cardID = *(int*)&buff[1];
			//	op.op_playcard.enemyID = *(int*)&buff[5];
			//	op.op_playcard.pos_x = *(short*)&buff[9];
			//	op.op_playcard.pos_y = *(short*)&buff[11];
			//	clientData->bd->OPQueue.enq(op);
			//}
			//	break;
			//case 2: // 클라이언트가 게임에 참여한다는걸 알리는 프로토콜
			//{
			//	
			//}
			//}
		}
		else if(recv_siz == 0) {
			break;
		}
	}

	closesocket(client_sock);
	printf("[NWGP7 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n", addr, ntohs(clientaddr.sin_port));
	return 0;
}

void ExecuteOP(BattleData& bd) {
	// get op
	OP op = bd.OPQueue.deq();
	while (op.ptype >= 0) {
		// Executing Code
		switch (op.ptype) {
		case CTS_PT_KeyInput:
		{
			printf("player %d key down : %c \n", op.op_key.playerID, op.op_key.key);
			int partID = op.op_key.playerID;
			char key = op.op_key.key;

			if (key == 'W') {
				bd.gameState.players[partID].Move(0, 1);
			}
			else if (key == 'A') {
				bd.gameState.players[partID].Move(-1, 0);
			}
			else if (key == 'S') {
				bd.gameState.players[partID].Move(0, -1);
			}
			else if (key == 'D') {
				bd.gameState.players[partID].Move(1, 0);
			}
			break;
		}
		case CTS_PT_PlayCard:
		{
			printf("player %d Use Card index : %d \n", op.op_playcard.playerID, op.op_playcard.cardindex);
			bd.gameLogic.PlayCard(bd.gameState, op.op_playcard.cardindex, bd, op.op_playcard.playerID);
			break;
		}
		}

		//next op
		op = bd.OPQueue.deq();
	}
}

void TimeBasedUpdate(BattleData& bd, float deltaTime) {
	bd.gameLogic.Update(bd.gameState, bd);
}

DWORD WINAPI ProcessBattle(LPVOID arg) {
	BattleData& bd = *(BattleData*)arg;

	bd.gameLogic.Initialize(bd.gameState);
	bd.gameLogic.StartBattle(bd.gameState);

	int ptype = STC_PT_InitGame; // 게임 초기값 프로토콜

	for (int i = 0; i < GameState::playerCount; ++i) {
		int clientId = bd.clientsID[i];
		if (clientId < 0) continue;
		SOCKET sock = clients[clientId].param.sock;

		vector<char> initPacket(sizeof(int) + sizeof(int) + sizeof(GameState));
		int offset = 0;
		memcpy(initPacket.data() + offset, &ptype, sizeof(int));
		offset += sizeof(int);
		memcpy(initPacket.data() + offset, &i, sizeof(int)); 
		offset += sizeof(int);

		memcpy(initPacket.data() + offset, &bd.gameState, sizeof(GameState));

		send(sock, initPacket.data(), initPacket.size(), 0);
	}

	// battleupdate
	while (1) {
		ExecuteOP(bd);
		
		const float deltaTime = 0.017; // temp delta time;
		TimeBasedUpdate(bd, deltaTime);

		bd.STCQueueMutex.lock();
		while (!bd.STCQueue.empty()) {
			vector<char> packet = bd.STCQueue.front();
			bd.STCQueue.pop();

			for (int i = 0; i < GameState::playerCount; ++i) {
				int clientID = bd.clientsID[i];
				if (clientID < 0) continue;

				SOCKET clientSock = clients[clientID].param.sock;
				send(clientSock, packet.data(), packet.size(), 0);
			}
		}
		bd.STCQueueMutex.unlock();
		
		Sleep(17);//
	}
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
			ClientData& c0 = clients[raid_clientsid[0]->client_id];
			ClientData& c1 = clients[raid_clientsid[1]->client_id];
			ClientData& c2 = clients[raid_clientsid[2]->client_id];

			battles[battle_id_up].clientsID[0] = raid_clientsid[0]->client_id;
			battles[battle_id_up].clientsID[1] = raid_clientsid[1]->client_id;
			battles[battle_id_up].clientsID[2] = raid_clientsid[2]->client_id;

			clients[battles[battle_id_up].clientsID[0]].ParticipateID = 0;
			clients[battles[battle_id_up].clientsID[1]].ParticipateID = 1;
			clients[battles[battle_id_up].clientsID[2]].ParticipateID = 2;

			battles[battle_id_up].gameState.PvEMode = true;
			battles[battle_id_up].OPQueue.clear();
			battles[battle_id_up].hBattleThread = CreateThread(NULL, 0, ProcessBattle, (LPVOID)&battles[battle_id_up], 0, NULL);

			c0.bd = &battles[battle_id_up];
			c1.bd = &battles[battle_id_up];
			c2.bd = &battles[battle_id_up];

			for (int i = 0; i < 3; ++i) {
				battles[battle_id_up].gameState.players[i].clientData = &clients[battles[battle_id_up].clientsID[i]];
			}

			battle_id_up += 1;

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
		clients[thread_id_up].hThread = CreateThread(NULL, 0, ProcessClient, (LPVOID)param, 0, NULL);

		if (clients[thread_id_up].hThread == NULL) {
			closesocket(client_sock);
			CloseHandle(clients[thread_id_up].hThread);
		}
		else thread_id_up += 1;
	}

	for (int i = 0; i < thread_id_up;++i) {
		CloseHandle(clients[i].hThread);
	}

	WSACleanup();
	CloseHandle(hMatchingThread);
}

void GameLogic::Initialize(GameState& state)
{
	srand((unsigned int)time(NULL));

	for (int i = 0; i < 3; ++i) { // 3 플레이어 초기화

		for (int i = 0; i < 5; ++i) {
			state.players[i].hand[i].id = rand() % 15;
		}
		// 주인공 좌표/스탯 초기화
		state.players[i].pos.x = 0;
		state.players[i].pos.y = 0;
		state.players[i].defence = 0;
		state.players[i].hp = 0;
		state.players[i].mana = 0;
		state.players[i].maxMana = 3;
		state.players[i].attack = 0;

		state.players[i].playerdeath = false;
		state.players[i].cutting = false;
		state.players[i].isCastingOnePunch = false;
		state.players[i].ParingMoment = false;
	}
}

void GameLogic::StartBattle(GameState& state)
{
	//$Chang Player Init
	for (int i = 0; i < GameState::playerCount; ++i) {
		PlayerData& p = state.players[i];
		p.mana = p.maxMana;
		p.hp = 100;
		p.pos.x = 0;
		p.pos.y = 0;

		for (int i = 0; i < 5; ++i) {
			p.hand[i].id = rand() % 15;
		}
	}

	if (state.PvEMode) { //레이드
		state.boss.attackTimer = 0.0f;
		state.boss.death = false;
		//state.boss.id = rand() % 9;
		state.boss.id = 0;

		if (state.boss.id == 0) {
			state.boss.hp = 100;
			state.boss.defence = 0;
		}
		else if (state.boss.id == 1) {
			state.boss.hp = 100;
			state.boss.defence = 15;
		}
		else if (state.boss.id == 2) {
			state.boss.hp = 100;
			state.boss.defence = 15;
		}
		else if (state.boss.id == 3) {
			state.boss.hp = 100;
			state.boss.defence = 20;
			state.boss.bossmode2 = false;
			state.boss.boss_statck = 0;
		}
		else if (state.boss.id == 4) {
			state.boss.hp = 100;
			state.boss.defence = 30;
		}
		else if (state.boss.id == 5) {
			state.boss.hp = 100;
			state.boss.defence = 20;
			state.boss.bossmode2 = false;
			state.boss.boss_statck = 0;
		}
		else if (state.boss.id == 6) {
			state.boss.hp = 100;
			state.boss.defence = 20;
			state.boss.boss_statck = 0;
		}
		else if (state.boss.id == 7) {
			state.boss.hp = 100;
			state.boss.defence = 20;
			state.boss.nodamageMode = false;
			state.boss.bossmode3 = false;
			state.boss.boss_statck = 0;
		}
		else if (state.boss.id == 8) {
			state.boss.hp = 100;
			state.boss.defence = 20;
			state.boss.bossAwakening = false;
		}
	}
	else { // pvp

	}
}

void GameLogic::Update(GameState& state, BattleData& bd)
{
	if (state.PvEMode) {
		float deltaTime = 0.017f;
		UpdatePvE(state, deltaTime, bd); // 레이드
	}
	else {
		float deltaTime = 0.017f;
		UpdatePvP(state, deltaTime, bd); // pvp
	}
}

void GameLogic::UpdatePvE(GameState& state, float deltaTime, BattleData& bd)
{
	CheckWinLossConditionsPvE(state, bd);
	UpdateBattle_RealTime(state, deltaTime, bd);
}

void GameLogic::UpdatePvP(GameState& state, float deltaTime, BattleData& bd)
{
	CheckWinLossConditionsPvP(state, bd);
	UpdateBattle_RealTime(state, deltaTime, bd);
}

void GameLogic::CheckWinLossConditionsPvE(GameState& state, BattleData& bd)
{
	if (state.boss.hp <= 0) { // boss 
		if (state.boss.bossAwakening) {
			//state.boomswitch = true; // 폭발 모션 재생
			state.boss.death = true;
			RecordSTCPacket(bd, STC_Sync_Boss_Death, &state.boss.death, sizeof(bool));
		}
		else {
			//state.boss.heal = true; // 체력회복 모션 재생
			state.boss.healEnergy = 100;
			state.boss.bossAwakening = true;
			state.boss.defence = 20;

			//RecordSTCPacket(bd, STC_Sync_Boss_HealEnergy, &state.boss.healEnergy, sizeof(int));
			RecordSTCPacket(bd, STC_Sync_Boss_Awakening, &state.boss.bossAwakening, sizeof(bool));
			RecordSTCPacket(bd, STC_Sync_Boss_Defence, &state.boss.defence, sizeof(int));
		}
	}

	for (int i = 0; i < GameState::playerCount; ++i) {
		if (state.players[0].hp <= 0 && state.players[0].playerdeath == false) {
			state.players[0].playerdeath = true;
			char ptype = (i * PLAYER_SYNC_STRIDE) + SYNC_PLAYER_DEATH;
			RecordSTCPacket(bd, ptype, &state.players[i].playerdeath, sizeof(bool));
			//state.pdeath = true; // 플레이어 죽는 모션 재생
		}
	}
}

void GameLogic::CheckWinLossConditionsPvP(GameState& state, BattleData& bd)
{
	// pvp 모드 승리 패배 조건
}

void GameLogic::AttackWarning(GameState& state, BattleData& bd)
{
	MapStateRepare(state, bd);

	int count = 3;
	for (int i = 0; i < count; ++i) {
		int rx = rand() % GameState::GRID_SIZE;
		int ry = rand() % GameState::GRID_SIZE;

		state.mapData[ry][rx] = 3; // please enum
	}

	char ptype = STC_Sync_MapData;
	RecordSTCPacket(bd, ptype, state.mapData, sizeof(state.mapData));
}

void GameLogic::AttackOnRandomGreed(GameState& state, int damage, BattleData& bd)
{
	for (int i = 0; i < GameState::GRID_SIZE; ++i) {
		for (int j = 0; j < GameState::GRID_SIZE; ++j) {
			if (state.mapData[i][j] == 3) {
				state.mapData[i][j] = 2;
			}
		}
	}

	char ptype = STC_Sync_MapData;
	RecordSTCPacket(bd, ptype, state.mapData, sizeof(state.mapData));

	ApplyDamageToPlayer(state, damage, 0, bd); // 데미지 15
	ApplyDamageToPlayer(state, damage, 1, bd); // 데미지 15
	ApplyDamageToPlayer(state, damage, 2, bd); // 데미지 15
}

void GameLogic::MapStateRepare(GameState& state, BattleData& bd)
{
	for (int i = 0; i < GameState::GRID_SIZE; i++) {
		for (int j = 0; j < GameState::GRID_SIZE; j++) {
			state.mapData[i][j] = 0;
		}
	}

	char ptype = STC_Sync_MapData;
	RecordSTCPacket(bd, ptype, state.mapData, sizeof(state.mapData));
}

void GameLogic::ExecuteEnemyAI(GameState& state, float deltaTime, BattleData& bd)
{
	// 스테이지 1 슬라임 패턴
	if (state.boss.id == 0) { // 데미지 0~5
		if (state.boss.attackState == 0) {
			state.boss.attackState = 1;
			state.boss.currentStateDuration = 3.0f; //경고


			AttackWarning(state, bd);
		}
		else if (state.boss.attackState == 1) {
			state.boss.attackState = 2;
			state.boss.currentStateDuration = 1.0f; //공격
			AttackOnRandomGreed(state, 5, bd);
		}
		else if (state.boss.attackState == 2) {
			state.boss.attackState = 0;
			state.boss.currentStateDuration = 3.0f; // 복구
			MapStateRepare(state, bd);
		}
	}
	//// 스테이지 2-1 기사 50% 공격 50% 방어
	//else if (state.boss.id == 1) { // 데미지 10 / 방어 10
	//	state.boss.patturn = rand() % 2;
	//	if (state.boss.patturn == 0) {
	//		AttackOnRandomGreed(state, 30);
	//	}
	//	else {
	//		ApplyDefenseToEnemy(state, 10);
	//	}
	//}
	//// 스테이지 2-2 주술사 50% 공격 50% 자힐
	//else if (state.boss.id == 2) { // 데미지 10 / 힐 10
	//	state.boss.patturn = rand() % 2;
	//	if (state.boss.patturn == 0) {
	//		HealBoss(state, 30);
	//		state.boss.healEnergy = 10;
	//	}
	//	else {
	//		AttackOnRandomGreed(state, 30);
	//	}
	//}
	//// 스테이지 3-1 거북
	//else if (state.boss.id == 3) {
	//	if (state.boss.bossmode2) { // 빨간 모드
	//		AttackOnRandomGreed(state, 30);
	//	}
	//	else { // 노멀 모드
	//		state.boss.boss_statck++;
	//		if (state.boss.boss_statck == 5) {
	//			state.boss.bossmode2 = true;
	//		}
	//		ApplyDefenseToEnemy(state, 20);
	//	}
	//}
	//// 스테이지 3-2 개
	//else if (state.boss.id == 4) { // 데미지 20 / 방어 20
	//	AttackOnRandomGreed(state, 30);
	//	ApplyDefenseToEnemy(state, 20);
	//}
	//// 스테이지 3-3 두더지
	//else if (state.boss.id == 5) {
	//	if (state.boss.bossmode2) { // 땅 팜
	//		state.boss.boss_statck++;
	//		if (state.boss.boss_statck == 5) {
	//			state.boss.boss_statck = 0;
	//			state.boss.bossmode3 = true; // 공격 애니메이션
	//			AttackOnRandomGreed(state, 30);
	//		}
	//	}
	//	else { // 지상
	//		state.boss.boss_statck++;
	//		if (state.boss.boss_statck == 3) {
	//			state.boss.bossmode2 = true;
	//			state.dontattackcard = true; // 공격 카드 사용 금지
	//		}
	//		AttackOnRandomGreed(state, 30);
	//		ApplyDefenseToEnemy(state, 10);
	//	}
	//}
	//// 스테이지 4-1 마트료시카
	//else if (state.boss.id == 6) { // 공격20 방어 20
	//	AttackOnRandomGreed(state, 30);
	//	ApplyDefenseToEnemy(state, 20);
	//}
	//// 스테이지 4-2 관
	//else if (state.boss.id == 7) { // 공격 20 방 20 3턴마다 무적 다음턴 30뎀
	//	state.boss.boss_statck++;
	//	if (state.boss.boss_statck == 3) {
	//		state.boss.bossmode3 = true;
	//		state.boss.nodamageMode = true; // 무적
	//	}
	//	else if (state.boss.boss_statck == 4) {
	//		state.boss.boss_statck = 0;
	//		state.boss.nodamageMode = false; // 무적 해제
	//		AttackOnRandomGreed(state, 30);
	//		state.boss.bossmode3 = false;
	//	}
	//	else {
	//		AttackOnRandomGreed(state, 30);
	//		ApplyDefenseToEnemy(state, 20);
	//	}
	//}
	//// 보스 스테이지
	//if (state.boss.id == 8) { // 노 각성 공 20 방 20 / 각성 공 30 방 30
	//	if (state.boss.bossAwakening) {
	//		AttackOnRandomGreed(state, 30);
	//		ApplyDefenseToEnemy(state, 30);
	//	}
	//	else {
	//		AttackOnRandomGreed(state, 30);
	//		ApplyDefenseToEnemy(state, 20);
	//	}
	//}
}

void GameLogic::GameInit(SOCKET sock, const GameState& state, int playerindex, PlayerData& pdata)
{
	char buf[64] = {};
	int offset = 0;

	//buf[offset++] = STC_PT_Gameinit;          // type
	buf[offset++] = state.PvEMode ? 1 : 0;
	buf[offset++] = (uint8_t)playerindex;

	//boss info
	*(int32_t*)&buf[offset] = state.boss.id;      offset += 4;
	*(int32_t*)&buf[offset] = state.boss.hp;      offset += 4;
	*(int32_t*)&buf[offset] = state.boss.defence; offset += 4;

	//player info
	for (int i = 0; i < GameState::playerCount; ++i) {
		buf[offset++] = (uint8_t)state.players[i].hp;
		buf[offset++] = (uint8_t)state.players[i].mana;
	}

	//card info
	uint8_t handCount = 5;
	buf[offset++] = handCount;

	for (int i = 0; i < handCount; ++i) {
		int cardId = pdata.hand[i].id;                 // state에서 읽기
		memcpy(&buf[offset], &cardId, sizeof(int)); // buf에 쓰기
		offset += sizeof(int);
	}

	send(sock, buf, offset, 0);

}

void GameLogic::UpdateBattle_RealTime(GameState& state, float deltaTime, BattleData& bd)
{
	//CardUpdate(state, deltaTime);

	//player Update
	for (int i = 0; i < GameState::playerCount; ++i) {
		PlayerData& p = state.players[i];

		//mana regen
		constexpr float manaRegenSpeed = 0.5f;
		p.mana += deltaTime * manaRegenSpeed;
		if (p.mana > p.maxMana) {
			p.mana = p.maxMana;

			char ptypeMana = (i * PLAYER_SYNC_STRIDE) + SYNC_MANA;
			RecordSTCPacket(bd, ptypeMana, &p.mana, sizeof(float));
		}

		//onepuncing?
		if (p.isCastingOnePunch) { // 일격!!

			p.onePunchCastTimer -= deltaTime;
			char ptypeTimer = (i * PLAYER_SYNC_STRIDE) + SYNC_ONEPUNCH_TIMER;
			RecordSTCPacket(bd, ptypeTimer, &p.onePunchCastTimer, sizeof(float));

			if (p.onePunchCastTimer <= 0.0f) {
				p.isCastingOnePunch = false;
				p.onePunchCastTimer = 0.0f;

				char ptypeCast = (i * PLAYER_SYNC_STRIDE) + SYNC_IS_CASTING_ONEPUNCH;
				RecordSTCPacket(bd, ptypeCast, &p.isCastingOnePunch, sizeof(bool));

				char ptypeDmg = (i * PLAYER_SYNC_STRIDE) + SYNC_ONEPUNCH_DAMAGE;
				RecordSTCPacket(bd, ptypeDmg, &p.onePunchStoredDamage, sizeof(int));

				ApplyDamageToBoss(state, i, p.onePunchStoredDamage, bd);
				p.onePunchStoredDamage = 0;

				// 일격 & 지진 이펙트 활성화
			}
		}
	}
	//boss update
	if (state.PvEMode) {
		if (state.boss.hp <= 0 && !state.boss.death) {
			state.boss.death = true; // 보스 죽음
			RecordSTCPacket(bd, STC_Sync_Boss_Death, &state.boss.death, sizeof(bool));
		}
		state.boss.attackTimer += deltaTime;

		constexpr float bossAttackDelay = 3;
		if (state.boss.attackTimer > bossAttackDelay && !state.boss.death) { // 턴 시작
			state.boss.attackTimer = 0;
			// 카드 드로우 애니메이션 draww = true
			ExecuteEnemyAI(state, deltaTime, bd);
		}
	}

}

void GameLogic::UpdateBuffsAndTimers(GameState& state, float deltaTime)
{
	for (int i = 0; i < GameState::playerCount; ++i) {
		PlayerData& p = state.players[i];
	}

}

void GameLogic::PlayCard(GameState& state, int cardIndex, BattleData& bd, int playerIndex) {
	PlayerData& player = state.players[playerIndex];
	CardData& card = player.hand[cardIndex];

	constexpr float cardThrowMaxtime = 0.75f;

	if (state.dontattackcard) {
		// 공격카드 사용 불가
		return;
	}

	bool isused = false;
	int manaCost = 0;
	// Card ID 0: 심장 뽑기 (Cost 2, Attack 70, Heal 10, Attack resets)
	if (card.id == 0 && player.mana >= 2) {
		manaCost = 2;
		isused = true;
	}
	// Card ID 1: 심판 (Cost 3, Attack 90, Attack resets)
	else if (card.id == 1 && player.mana >= 3) {
		manaCost = 3;
		isused = true;
	}
	// Card ID 2: 강타 (Cost 2, Attack 60, Attack resets)
	else if (card.id == 2 && player.mana >= 2) {
		manaCost = 2;
		isused = true;
	}
	// Card ID 3: 자세잡기 (Cost 1, Defense +3, Mana +1)
	else if (card.id == 3 && player.mana >= 1) {
		manaCost = 3;
		isused = true;
	}
	// Card ID 4: 돌진 (Cost 2, Attack 40, Defense +10, Attack resets)
	else if (card.id == 4 && player.mana >= 2) {
		manaCost = 2;
		isused = true;
	}
	// Card ID 5: 대검휘두르기 (Cost 1, Attack 50, Attack resets)
	else if (card.id == 5 && player.mana >= 1) {
		manaCost = 1;
		isused = true;
	}
	// Card ID 6: 바리게이트 (Cost 2, Defense x2)
	else if (card.id == 6 && player.mana >= 2) {
		manaCost = 2;
		isused = true;
	}
	// Card ID 7: 방패 밀쳐내기 (Cost 1, Attack = Defense, Attack resets)
	else if (card.id == 7 && player.mana >= 1) {
		manaCost = 1;
		isused = true;
	}
	// Card ID 8: 굳건한 태세 (Cost 2, Block next attack)
	else if (card.id == 8 && player.mana >= 2) {
		manaCost = 2;
		isused = true;
	}
	// Card ID 9: 방패 세우기 (Cost 1, Defense +5)
	else if (card.id == 9 && player.mana >= 1) {
		manaCost = 1;
		isused = true;
	}
	// Card ID 10: 절단 (Cost 2, Attack 40 (ignores defense), Attack resets)
	else if (card.id == 10 && player.mana >= 2) {
		manaCost = 2;
		isused = true;
	}
	// Card ID 11: 일격 (Cost 3, Attack 140 next turn, Attack resets)
	else if (card.id == 11 && player.mana >= 3) {
		manaCost = 3;
		isused = true;
	}
	// Card ID 12: 고속 이동 (Cost 2, Defense +5, Mana +1)
	else if (card.id == 12 && player.mana >= 2) {
		manaCost = 2;
		isused = true;
	}
	// Card ID 13: 혈류 (Cost 1, HP -10, Attack 60, Attack resets)
	else if (card.id == 13 && player.mana >= 1) {
		manaCost = 1;
		isused = true;
	}
	// Card ID 14: 정조준 (Cost 1, Next Attack +20)
	else if (card.id == 14 && player.mana >= 1) {
		manaCost = 1;
		isused = true;
	}

	if (isused) {
		player.mana -= manaCost;
		float newMana = player.mana;
		char ptypeMana = (playerIndex * PLAYER_SYNC_STRIDE) + SYNC_MANA;
		RecordSTCPacket(bd, ptypeMana, &newMana, sizeof(float));

		PlayCardLogic(state, card.id, bd, playerIndex, false);

		card.id = rand() % 15;
		CardData newCard = card;
		char ptypeCard = (playerIndex * PLAYER_SYNC_STRIDE) + (SYNC_HAND_SLOT_0 + cardIndex);
		RecordSTCPacket(bd, ptypeCard, &newCard, sizeof(CardData));

		// 마나 다운 애니메이션 이벤트
		
		// 카드 사용 애니매이션 이벤트
	}
}

void GameLogic::PlayCardLogic(GameState& state, int cardID, BattleData& bd, int playerindex, bool usedByMonster)
{
	PlayerData& player = state.players[playerindex];
	if (usedByMonster) {
		// Card ID 0: 심장 뽑기 (Cost 2, Attack 70, Heal 10, Attack resets)
		if (cardID == 0) {
			ApplyDamageToPlayer(state, 70, playerindex, bd);
		}
		// Card ID 1: 심판 (Cost 3, Attack 90, Attack resets)
		else if (cardID == 1 && player.mana >= 3) {
			ApplyDamageToPlayer(state, 90, playerindex, bd);
		}
		// Card ID 2: 강타 (Cost 2, Attack 60, Attack resets)
		else if (cardID && player.mana >= 2) {
			ApplyDamageToPlayer(state, 60, playerindex, bd);
		}
		// Card ID 4: 돌진 (Cost 2, Attack 40, Defense +10, Attack resets)
		else if (cardID == 4 && player.mana >= 2) {
			ApplyDamageToPlayer(state, 40, playerindex, bd);
		}
		// Card ID 5: 대검휘두르기 (Cost 1, Attack 50, Attack resets)
		else if (cardID == 5 && player.mana >= 1) {
			ApplyDamageToPlayer(state, 50, playerindex, bd);
		}
		// Card ID 10: 절단 (Cost 2, Attack 40 (ignores defense), Attack resets)
		else if (cardID == 10 && player.mana >= 2) {
			ApplyDamageToPlayer(state, 40, playerindex, bd);
		}
		// Card ID 11: 일격 (Cost 3, Attack 140 next turn, Attack resets)
		else if (cardID == 11 && player.mana >= 3) {
			ApplyDamageToPlayer(state, 140, playerindex, bd);
		}
		// Card ID 13: 혈류 (Cost 1, HP -10, Attack 60, Attack resets)
		else if (cardID == 13 && player.mana >= 1) {
			ApplyDamageToPlayer(state, 60, playerindex, bd);
		}
	}
	else {
		// Card ID 0: 심장 뽑기 (Cost 2, Attack 70, Heal 10, Attack resets)
		if (cardID == 0) {
			int damage = 70;
			if (player.attack != 0) {
				damage += player.attack;
				player.attack = 0;
				// 추가 공격 다운 애니메이션
			}
			player.hp += 10;
			if (player.hp > 100) player.hp = 100;
			char ptypeHP = (playerindex * PLAYER_SYNC_STRIDE) + SYNC_HP;
			RecordSTCPacket(bd, ptypeHP, &player.hp, sizeof(int));

			ApplyDamageToBoss(state, playerindex, damage, bd);
		}
		// Card ID 1: 심판 (Cost 3, Attack 90, Attack resets)
		else if (cardID == 1) {
			int damage = 90;
			if (player.attack != 0) {
				damage += player.attack;
				player.attack = 0;
				// 추가 공격 다운 애니메이션 이펙트
				char ptypeAtk = (playerindex * PLAYER_SYNC_STRIDE) + SYNC_ATTACK;
				RecordSTCPacket(bd, ptypeAtk, &player.attack, sizeof(int));
			}
			ApplyDamageToBoss(state, playerindex, damage, bd);
		}
		// Card ID 2: 강타 (Cost 2, Attack 60, Attack resets)
		else if (cardID == 2) {
			int damage = 60;
			if (player.attack != 0) {
				damage += player.attack;
				player.attack = 0;
				// 추가 공격 다운 애니메이션 이펙트
				char ptypeAtk = (playerindex * PLAYER_SYNC_STRIDE) + SYNC_ATTACK;
				RecordSTCPacket(bd, ptypeAtk, &player.attack, sizeof(int));
			}
			ApplyDamageToBoss(state, playerindex, damage, bd);
		}
		// Card ID 3: 자세잡기 (Cost 1, Defense +3, Mana +1)
		else if (cardID == 3) {
			player.mana += 1.0f;
			player.defence += 3;
			// 마나 업, 방어 업 애니매이션, 테카이 애니매이션 이펙트
			char ptypeMana = (playerindex * PLAYER_SYNC_STRIDE) + SYNC_MANA;
			RecordSTCPacket(bd, ptypeMana, &player.mana, sizeof(float));
			char ptypeDef = (playerindex * PLAYER_SYNC_STRIDE) + SYNC_DEFFENCE;
			RecordSTCPacket(bd, ptypeDef, &player.defence, sizeof(int));
		}
		// Card ID 4: 돌진 (Cost 2, Attack 40, Defense +10, Attack resets)
		else if (cardID == 4) {
			int damage = 60;
			if (player.attack != 0) {
				damage += player.attack;
				player.attack = 0;
				// 추가 공격 다운 애니메이션 이펙트
				char ptypeAtk = (playerindex * PLAYER_SYNC_STRIDE) + SYNC_ATTACK;
				RecordSTCPacket(bd, ptypeAtk, &player.attack, sizeof(int));
			}
			player.defence += 3;
			// 방어 업, 테카이 애니메이션 이펙트
			char ptypeDef = (playerindex * PLAYER_SYNC_STRIDE) + SYNC_DEFFENCE;
			RecordSTCPacket(bd, ptypeDef, &player.defence, sizeof(int));

			ApplyDamageToBoss(state, playerindex, damage, bd);
		}
		// Card ID 5: 대검휘두르기 (Cost 1, Attack 50, Attack resets)
		else if (cardID == 5) {
			int damage = 50;
			if (player.attack != 0) {
				damage += player.attack;
				player.attack = 0;

				// 추가 공격 다운 애니메이션 이펙트
				char ptypeAtk = (playerindex * PLAYER_SYNC_STRIDE) + SYNC_ATTACK;
				RecordSTCPacket(bd, ptypeAtk, &player.attack, sizeof(int));
			}
			ApplyDamageToBoss(state, playerindex, damage, bd);
		}
		// Card ID 6: 바리게이트 (Cost 2, Defense x2)
		else if (cardID == 6) {
			player.defence *= 2;
			// 텟카이 애니메이션, 방어 업 애니메이션 이펙트
			char ptypeDef = (playerindex * PLAYER_SYNC_STRIDE) + SYNC_DEFFENCE;
			RecordSTCPacket(bd, ptypeDef, &player.defence, sizeof(int));
		}
		// Card ID 7: 방패 밀쳐내기 (Cost 1, Attack = Defense, Attack resets)
		else if (cardID == 7) {
			int damage = player.defence;
			if (player.attack != 0) {
				damage += player.attack;
				player.attack = 0;
				// 추가 공격 다운 애니메이션 이펙트
				char ptypeAtk = (playerindex * PLAYER_SYNC_STRIDE) + SYNC_ATTACK;
				RecordSTCPacket(bd, ptypeAtk, &player.attack, sizeof(int));
			}
			ApplyDamageToBoss(state, playerindex, damage, bd);
		}
		// Card ID 8: 굳건한 태세 (Cost 2, Block next attack)
		else if (cardID == 8) {
			player.invincible = true;
			// 홀리 쉴드 애니메이션 이펙트
			char ptypeInv = (playerindex * PLAYER_SYNC_STRIDE) + SYNC_INVINCIBLE;
			RecordSTCPacket(bd, ptypeInv, &player.invincible, sizeof(bool));
		}
		// Card ID 9: 방패 세우기 (Cost 1, Defense +5)
		else if (cardID == 9) {
			player.defence += 5;
			// 방어 업, 텟카이 애니메이션 이펙트
			char ptypeDef = (playerindex * PLAYER_SYNC_STRIDE) + SYNC_DEFFENCE;
			RecordSTCPacket(bd, ptypeDef, &player.defence, sizeof(int));
		}
		// Card ID 10: 절단 (Cost 2, Attack 40 (ignores defense), Attack resets)
		else if (cardID == 10) {
			int damage = 40;
			if (player.attack != 0) {
				damage += player.attack;
				player.attack = 0;
				// 추가 공격력 다운 이펙트
				char ptypeAtk = (playerindex * PLAYER_SYNC_STRIDE) + SYNC_ATTACK;
				RecordSTCPacket(bd, ptypeAtk, &player.attack, sizeof(int));
			}
			player.cutting = true; // 
			char ptypeCut = (playerindex * PLAYER_SYNC_STRIDE) + SYNC_CUTTING;
			RecordSTCPacket(bd, ptypeCut, &player.cutting, sizeof(bool));

			ApplyDamageToBoss(state, playerindex, damage, bd);
		}
		// Card ID 11: 일격 (Cost 3, Attack 140 next turn, Attack resets)
		else if (cardID == 11) {
			int damage = 140;
			if (player.attack != 0) {
				damage += player.attack;
				player.attack = 0;
				// 추가 공격다운 애니메이션 이펙트
				char ptypeAtk = (playerindex * PLAYER_SYNC_STRIDE) + SYNC_ATTACK;
				RecordSTCPacket(bd, ptypeAtk, &player.attack, sizeof(int));
			}
			player.isCastingOnePunch = true;
			player.onePunchCastTimer = 3.0f; // 
			player.onePunchStoredDamage = damage;
			// 일격 애니메이션 이펙트
			char ptypeCast = (playerindex * PLAYER_SYNC_STRIDE) + SYNC_IS_CASTING_ONEPUNCH;
			RecordSTCPacket(bd, ptypeCast, &player.isCastingOnePunch, sizeof(bool));
			char ptypeTimer = (playerindex * PLAYER_SYNC_STRIDE) + SYNC_ONEPUNCH_TIMER;
			RecordSTCPacket(bd, ptypeTimer, &player.onePunchCastTimer, sizeof(float));
			char ptypeDmg = (playerindex * PLAYER_SYNC_STRIDE) + SYNC_ONEPUNCH_DAMAGE;
			RecordSTCPacket(bd, ptypeDmg, &player.onePunchStoredDamage, sizeof(int));
		}
		// Card ID 12: 고속 이동 (Cost 2, Defense +5, Mana +1)
		else if (cardID == 12) {
			player.mana += 1.0f; // 마나 업 애니매이션
			player.defence += 5; // 방어력 업 애니메이션

			char ptypeMana = (playerindex * PLAYER_SYNC_STRIDE) + SYNC_MANA;
			RecordSTCPacket(bd, ptypeMana, &player.mana, sizeof(float));
			char ptypeDef = (playerindex * PLAYER_SYNC_STRIDE) + SYNC_DEFFENCE;
			RecordSTCPacket(bd, ptypeDef, &player.defence, sizeof(int));
		}
		// Card ID 13: 혈류 (Cost 1, HP -10, Attack 60, Attack resets)
		else if (cardID == 13) {
			int damage = 60;
			if (player.attack != 0) {
				damage += player.attack;
				player.attack = 0;
				// 추가 공격 다운 애니메이션
				char ptypeAtk = (playerindex * PLAYER_SYNC_STRIDE) + SYNC_ATTACK;
				RecordSTCPacket(bd, ptypeAtk, &player.attack, sizeof(int));
			}
			player.hp -= 10;

			//피 감소 애니메이션
			char ptypeHP = (playerindex * PLAYER_SYNC_STRIDE) + SYNC_HP;
			RecordSTCPacket(bd, ptypeHP, &player.hp, sizeof(int));


			ApplyDamageToBoss(state, playerindex, damage, bd);
		}
		// Card ID 14: 정조준 (Cost 1, Next Attack +20)
		else if (cardID == 14) {
			player.attack += 20;
			// 공격력 업 애니매이션, 스나이퍼 애니메이션
			char ptypeAtk = (playerindex * PLAYER_SYNC_STRIDE) + SYNC_ATTACK;
			RecordSTCPacket(bd, ptypeAtk, &player.attack, sizeof(int));
		}
	}
}

void GameLogic::CardUpdate(GameState& state, float deltaTime, BattleData& bd)
{

}

void GameLogic::ApplyDamageToPlayer(GameState& state, int damage, int playerindex, BattleData& bd) {
	PlayerData& player = state.players[playerindex];

	int px = player.pos.x + 2;
	int py = 2 - player.pos.y;

	for (int i = 0; i < GameState::GRID_SIZE; ++i) {
		for (int j = 0; j < GameState::GRID_SIZE; ++j) {
			if (state.mapData[i][j] == 2 && (py == i && px == j)) {

				if (player.invincible) { // 무적
					damage = 0;
					player.invincible = false;
					bool newInvincible = false;
					char ptypeInv = (playerindex * PLAYER_SYNC_STRIDE) + SYNC_INVINCIBLE;
					RecordSTCPacket(bd, ptypeInv, &newInvincible, sizeof(bool));
				}
				// 방어력 대비 피 다는거 계산
				int defenseLost = 0;
				int hpLost = 0;
				int initialDefense = player.defence;

				int damageDealt = player.defence - damage;
				player.defence = (damageDealt > 0) ? damageDealt : 0;
				int newDef = player.defence;
				char ptypeDef = (playerindex * PLAYER_SYNC_STRIDE) + SYNC_DEFFENCE;
				RecordSTCPacket(bd, ptypeDef, &newDef, sizeof(int));

				if (damageDealt < 0) {
					hpLost = -damageDealt;
					player.hp -= hpLost;
					if (player.hp < 0) player.hp = 0;

					int newHP = player.hp;
					char ptypeHP = (playerindex * PLAYER_SYNC_STRIDE) + SYNC_HP;
					RecordSTCPacket(bd, ptypeHP, &newHP, sizeof(int));
				}

				defenseLost = initialDefense - player.defence;

				return;
			}
		}
	}


}

void GameLogic::ApplyDamageToBoss(GameState& state, int playerID, int rawDamage, BattleData& bd)
{

	if (state.boss.nodamageMode) {
		rawDamage = 0;
	}

	bool iscutting = false;
	iscutting = state.players[playerID].cutting;

	int actualHpDamage = rawDamage;
	int actualDefenseDamage = 0;

	if (state.boss.defence > 0 && iscutting == false) {
		actualDefenseDamage = rawDamage;
		state.boss.defence -= rawDamage;

		if (state.boss.defence < 0) {
			actualHpDamage = -state.boss.defence;
			state.boss.defence = 0;
			actualDefenseDamage -= actualHpDamage;
		}
		else {
			actualHpDamage = 0;
		}

		// 보스 방어력 감소 이펙트 이벤트

		int newBossDef = state.boss.defence;
		RecordSTCPacket(bd, STC_Sync_Boss_Defence, &newBossDef, sizeof(int));
	}

	state.boss.hp -= actualHpDamage;
	if (state.boss.hp < 0) {
		state.boss.hp = 0;
	}

	// 보스 체력 감소 이펙트 이벤트

	int newBossHP = state.boss.hp;
	RecordSTCPacket(bd, STC_Sync_Boss_Hp, &newBossHP, sizeof(int));

	if (iscutting) {
		state.players[playerID].cutting = false;

		bool newCutting = false;
		char ptypeCut = (playerID * PLAYER_SYNC_STRIDE) + SYNC_CUTTING;
		RecordSTCPacket(bd, ptypeCut, &newCutting, sizeof(bool));
	}
}

void GameLogic::ApplyDefenseToEnemy(GameState& state, int defense, BattleData& bd) {
	state.boss.defence += defense;

	// 보스 방어력 업 애니메이션 이벤트
	int newBossDef = state.boss.defence;
	RecordSTCPacket(bd, STC_Sync_Boss_Defence, &newBossDef, sizeof(int));
}

void GameLogic::HealBoss(GameState& state, int healAmount, BattleData& bd)
{
	state.boss.hp += healAmount;

	if (state.boss.hp > 100) {
		state.boss.hp = 100;
	}

	// 보스 힐 이펙트 이벤트
	int newBossHP = state.boss.hp;
	RecordSTCPacket(bd, STC_Sync_Boss_Hp, &newBossHP, sizeof(int));
}

void GameLogic::RecordSTCPacket(BattleData& bd, char ptype, void* data, int dataSize)
{
	std::vector<char> packet(sizeof(int) + dataSize);
	*(int*)& packet[0] = ptype;
	memcpy(packet.data() + sizeof(int), data, dataSize);

	bd.STCQueueMutex.lock();
	bd.STCQueue.push(packet);
	bd.STCQueueMutex.unlock();
}
