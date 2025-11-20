#include <stdio.h>
#include <iostream>
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment (lib, "msimg32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32.lib")

#include <Windows.h>
#include <tchar.h>
#include <ctime>
#include <math.h>
#include <list>
#include "resource.h"
#include <mmsystem.h>
#include <process.h>
#include <queue>
#include <mutex>

#define SERVERPORT 9000
#define BUFSIZE    512

using namespace std;

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
		(char*)&lpMsgBuf, 0, 0);
	printf("[%s] %s\n", msg, (char*)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

HINSTANCE g_hInst;
LPCTSTR lpszClass = L"Window Class Name";
LPCTSTR lpszWindowName = L"windows program";

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

struct CardData {
	int id = 0;
};

struct CardUI {
	int id = 0; // 카드 종류
	int x;
	int y;
	bool select = false;
	bool drag = false;
	bool on = false;
};

struct BossData {
	int defence = 0;
	int stage = 0;
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

struct BossPresentation {
	int x = 0;
	int y = 0;
	int animCount = 0;

	int attackTime = 0;
	int healTime = 0;
	int defUpTime = 0;
	int defDownTime = 0;

	bool defUp = false;
	bool defDown = false;
	bool heal = false;
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

struct InputData {
	bool WPress;
	bool APress;
	bool SPress;
	bool DPress;
	bool SpacePress;
};
InputData inputdata;

char MapData[5][5] = {};
constexpr float MapMoveMargin = 100;
constexpr float MapCenterX = 225;
constexpr float MapCenterY = 250;
constexpr int PlayerW = 150;
constexpr int PlayerH = 200;

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
};

struct PlayerPresentation {
	int x = 225;
	int y = 250; // chaX, chaY
	int animCount = 0;    // chaCount

	//animation values
	struct EffectAnimData {
		bool powerUp = false;
		bool powerDown = false;
		bool defUp = false;
		bool defDown = false;
		bool manaUp = false;
		bool manaDown = false;
		bool holiShild = false;
		bool tekai = false;
		bool hurt = false;
		bool decresehp = false; // 플레이어 피격
		bool myheal = false; // 플레이어 힐

		int tekaicount = 0;
		int hurtcount = 0;
		int holiShildcount = 0;
		int powerUpTime = 0;
		int powerDownTime = 0;
		int defUpTime = 0;
		int defDownTime = 0;
		int manaUpTime = 0;
		int manaDownTime = 0;
		int decresehptime = 0;
		int healTime = 0;
	};
	EffectAnimData effect_anim_data;

	// 상태 타이머
	int attackTime = 0;

	int onepunchingcount = 0;
	int onepunchingcounttime = 0;

	void UpdataPosition(const Pos& serverPos) {
		/*pos.x += dx;
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
		}*/

		x = MapCenterX + serverPos.x * MapMoveMargin;
		y = MapCenterY - serverPos.y * MapMoveMargin;
	}
};

struct GameState;

struct ThrowCard {
	static constexpr int monsterPos = -2147483648;
	static constexpr int playerPos[3] = { -2147483647, -2147483646, -2147483645 };
	Pos start_p; // -max, -max : monster position.
	float maxTime;
	float flowTime;
	Pos end_p; // -max, -max : monster position.
	//Pos GetStartPos(GameState* state) {
	//	Pos p = start_p;
	//	if (start_p.x == monsterPos || start_p.y == monsterPos) {
	//		p.x = state->boss.x;
	//		p.y = state->boss.y;
	//	}
	//	return p;
	//}
	//Pos GetEndPos(GameState* state) {
	//	Pos p = end_p;
	//	if (end_p.x == monsterPos || end_p.y == monsterPos) {
	//		p.x = state->boss.x;
	//		p.y = state->boss.y;
	//	}
	//	return p;
	//}

	int cardID = 0;

	void Update(float deltaTime) {
		flowTime += deltaTime;
	}

	ThrowCard() {

	}

	ThrowCard(Pos sp, Pos ep, float maxtime) :
		start_p{ sp }, end_p{ ep }, maxTime{ maxtime }, flowTime{ 0 }
	{
	}

	/*static ThrowCard CreateMonsterToPlayer_ThrowCard(Player& p, float maxTime) {
		return ThrowCard(Pos(monsterPos, monsterPos), Pos(p.x, p.y), maxTime);
	}

	static ThrowCard CreatePlayerToMonster_ThrowCard(Player& p, float maxTime) {
		return ThrowCard(Pos(p.x, p.y), Pos(monsterPos, monsterPos), maxTime);
	}

	static ThrowCard CreatePlayerToPlayer_ThrowCard(Player& p1, Player& p2, float maxTime) {
		return ThrowCard(Pos(p1.x, p1.y), Pos(p2.x, p2.y), maxTime);
	}*/

	static ThrowCard CreateFollowCard(Pos startp, int fpos, float maxTime) {
		return ThrowCard(startp, Pos(fpos, fpos), maxTime);
	}
};

struct PresentationState {
public:
	bool StartScreen = true;
	bool descrtion = false;
	bool endon = false; // 턴 종료 버튼 활성화
	bool start = false; // 시작 버튼 활성화
	bool end = false; // 종료 버튼

	PlayerPresentation players[3];

	static constexpr int clientindex = 0;
	PlayerPresentation* player = &player[clientindex]; // client's player

	BossPresentation boss;
	CardUI hand[5];

	bool boomswitch = false, Sniper = false, One = false, sword = false;
	bool quake = false;
	bool droww = false; // 카드 뽑기
	bool trunendd = false; // 턴 종료
	bool dehp = false; // 적 피격
	bool startstart = false, endend = false, pdeath = false; // 자막
	bool enermytouch = false;

	int boomcount = 0, snipercount = 0, Onecount = 0, swordcount = 0;
	int backgroundX = 0, backgroundY = 0, backgroundtime = 0, quaketime = 0;
	int damage = 0, dehptime = 0; // 적 피격
	int dedamge = 0, defensedown = 0; // 플레이어 피격
	int healenergy = 0, plusattack = 0, minusattack = 0, defenseup = 0, enermydeff = 0;
	int enermydeffdown = 0, killmana = 0, healmana = 0;
	int startstarttime = 0, endendtime = 0, pdeathtime = 0;

	// TCHAR 버퍼 (UI 텍스트)
	TCHAR def[5], atk[5], Mana[10], nowstagestr[10], enerdef[4];
	TCHAR dam[3], eheal[3], dedam[3], mheal[3], powup[3], powdown[3], defu[3], defd[3];
	TCHAR enerdf[3], enerdfdown[3], mad[3], mau[3];

	float animTimer = 0;

	// ThrowCard
	list<ThrowCard> throwCardList;
	void CardUpdate(float deltaTime, const GameState& gameState);
	void RenderThrowingCards(HDC hdc) const;
	void CardThrow(ThrowCard tc);
public:
	void Initialize();
	void Update(float deltaTime, const GameState& gameState);
	void UpdateBuffsAndTimers(float deltaTime, const GameState& gameState);
	void InitializeBattleVisuals(int bossID);
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

template<size_t N>
using BitmapArray = HBITMAP[N];

class AssetManager {
public:
	HBITMAP hBitBackground, hBitMenu, hBitMenu2, hBitMenu3;
	HBITMAP hBitTemp, hBitTemp2, hBitReturn, hBitwood, hBitstone, hBitShild;
	HBITMAP bossicon, playbutton, hBitplayerdeath, hBitattack;
	BitmapArray<2> endbutton, hBitstop;
	BitmapArray<15> CARD;
	BitmapArray<8> Icon;
	BitmapArray<3> Cha;
	BitmapArray<8> hBitBoom;
	BitmapArray<5> hBitSniper, hBitOne, hBitsword, hBittekai, hBitholiShild;
	BitmapArray<3> hBithurt;
	BitmapArray<3> Boss1, Boss2, Boss3, Boss4, Boss5, Boss6;
	BitmapArray<5> Boss7;
	BitmapArray<3> Boss8, Boss9, Boss10, Boss11, Boss12, Boss13, Boss14, Boss15;
	BitmapArray<6> Boss16, Boss17;

	HBITMAP hBit;

	int backWidth, backHeight, chaWidth, chaHeight;
	int menuWidth, menuHeight;
	int tempWidth, tempHeight;
	int tempWidth2, tempHeight2;
	int returnWidth, returnHeight;
	int woodWidth, woodHeight;
	int stoneWidth, stoneHeight;
	int cardWidth, cardHeight;
	int bossiconWidth, bossiconHeight;
	int stage4iconWidth, stage4iconHeight;
	int stage3iconWidth, stage3iconHeight;
	int stage21iconWidth, stage21iconHeight;
	int stage22iconWidth, stage22iconHeight;
	int stage1iconWidth, stage1iconHeight;
	int ShildWidth, ShildHeight;
	int enermy1Width, enermy1Height;
	int enermy2Width, enermy2Height;
	int enermy3Width, enermy3Height;
	int enermy4Width, enermy4Height;
	int enermy5Width, enermy5Height;
	int enermy6Width, enermy6Height;
	int enermy7Width, enermy7Height;
	int enermy8Width, enermy8Height;
	int enermy9Width, enermy9Height;
	int enermy10Width, enermy10Height;
	int playWidth, playHeight;
	int stopWidth, stopHeight;
	int attackWidth, attackHeight;
	int endbuttonWidth, endbuttonHeight;
	int pdWidth, pdHeight;
	int BoomWidth, BoomHeight;
	int sniperWidth, sniperHeight;
	int OneWidth, OneHeight;
	int swordWidth, swordHeight;
	int tekaiWidth, tekaiHeight;
	int hurtWidth, hurtHeight;
	int holiShildWidth, holiShildHeight;

public:
	void LoadAssets(HINSTANCE hInst, HDC hDC, RECT rectView) {
		hBitBackground = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP1));
		hBitMenu = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP2));
		hBitMenu2 = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP12)); // 게임 설명
		hBitMenu3 = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP13)); // 게임 종료
		hBitTemp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP3));
		hBitTemp2 = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP23)); // 메인화면 
		hBitReturn = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP5)); // 되돌아가기 이미지
		hBitwood = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP11)); // 나무판자 이미지
		hBitstone = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP24)); // 돌 메뉴 이미지
		CARD[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP6)); // 임시 카드1
		CARD[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP7)); // 임시 카드2
		CARD[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP8)); // 임시 카드3
		CARD[3] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP9)); // 임시 카드4
		CARD[4] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP10)); // 임시 카드5
		CARD[5] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP32)); // 임시 카드6
		CARD[6] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP33)); // 임시 카드7
		CARD[7] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP34)); // 임시 카드8
		CARD[8] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP35)); // 임시 카드9
		CARD[9] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP36)); // 임시 카드10
		CARD[10] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP37)); // 임시 카드11
		CARD[11] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP38)); // 임시 카드12
		CARD[12] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP39)); // 임시 카드13
		CARD[13] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP40)); // 임시 카드14
		CARD[14] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP41)); // 임시 카드15
		bossicon = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP14)); // 보스 아이콘
		Icon[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP15)); // 4-1 아이콘
		Icon[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP16)); // 4-2 아이콘
		Icon[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP17)); // 3-1 아이콘
		Icon[3] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP18)); // 3-2 아이콘
		Icon[4] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP19)); // 3-3 아이콘
		Icon[5] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP20)); // 2-1 아이콘
		Icon[6] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP21)); // 2-2 아이콘
		Icon[7] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP22)); // 1-1 아이콘
		Boss1[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP25)); // 슬라임
		Boss1[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP26)); // 슬라임2
		Boss1[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP27)); // 슬라임3
		hBitShild = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP28)); // 쉴드 아이콘
		playbutton = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP29)); // 플레이 아이콘
		hBitstop[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP30)); // 일시정지 아이콘
		hBitstop[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP31)); // 일시정지 아이콘2
		hBitattack = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP42)); // 공격 아이콘
		Cha[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP43)); // 주인공1
		Cha[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP44)); // 주인공2
		Cha[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP45)); // 주인공3
		endbutton[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP46)); // end 버튼
		endbutton[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP47)); // end 버튼2
		hBitBoom[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP48)); // 폭발1
		hBitBoom[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP49)); // 폭발2
		hBitBoom[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP50)); // 폭발3
		hBitBoom[3] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP51)); // 폭발4
		hBitBoom[4] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP52)); // 폭발5
		hBitBoom[5] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP53)); // 폭발6
		hBitBoom[6] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP54)); // 폭발7
		hBitBoom[7] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP55)); // 폭발8
		hBitplayerdeath = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP56)); // player 죽는 모습
		Boss2[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP57)); // 기사
		Boss2[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP58)); // 기사2
		Boss2[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP59)); // 기사3
		Boss3[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP60)); // 주술사
		Boss3[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP61)); // 주술사2
		Boss3[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP62)); // 주술사3
		Boss4[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP66)); // 거북 
		Boss4[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP67)); // 거북 2
		Boss4[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP68)); // 거북 3
		Boss5[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP63)); // 거북 괴물모드
		Boss5[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP64)); // 거북 괴물모드2
		Boss5[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP65)); // 거북 괴물모드3
		Boss6[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP69)); // 개1
		Boss6[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP70)); // 개2
		Boss6[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP71)); // 개3
		Boss7[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP72)); // 두더지1
		Boss7[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP73)); // 두더지2
		Boss7[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP74)); // 두더지3
		Boss7[3] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP75)); // 두더지4
		Boss7[4] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP76)); // 두더지5
		Boss8[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP77)); // 두더지 땅속 1
		Boss8[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP78)); // 두더지 땅속 2
		Boss8[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP79)); // 두더지 땅속 3
		Boss9[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP80)); // 두더지 공격 1
		Boss9[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP81)); // 두더지 공격 2
		Boss9[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP82)); // 두더지 공격 3
		Boss10[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP83)); // 마트료시카 1-1
		Boss10[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP84)); // 마트료시카 1-2
		Boss10[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP85)); // 마트료시카 1-3
		Boss11[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP86)); // 마트료시카 2-1
		Boss11[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP87)); // 마트료시카 2-2
		Boss11[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP88)); // 마트료시카 2-3
		Boss12[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP89)); // 마트료시카 3-1
		Boss12[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP90)); // 마트료시카 3-2
		Boss12[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP91)); // 마트료시카 3-3
		Boss13[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP92)); // 마트료시카 4-1
		Boss13[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP93)); // 마트료시카 4-2
		Boss13[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP94)); // 마트료시카 4-3
		Boss14[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP95)); // 관 1
		Boss14[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP96)); // 관 2
		Boss14[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP97)); // 관 3
		Boss15[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP98)); // 관 공격
		Boss15[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP99)); // 관 공격
		Boss15[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP100)); // 관 공격
		Boss16[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP101)); // boss 1-1
		Boss16[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP102)); // boss 1-2
		Boss16[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP103)); // boss 1-3
		Boss16[3] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP104)); // boss 1-4
		Boss16[4] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP105)); // boss 1-5
		Boss16[5] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP106)); // boss 1-6
		Boss17[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP107)); // boss 2-1
		Boss17[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP108)); // boss 2-2
		Boss17[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP109)); // boss 2-3
		Boss17[3] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP110)); // boss 2-4
		Boss17[4] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP111)); // boss 2-5
		Boss17[5] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP112)); // boss 2-6
		hBitSniper[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP113)); // 정조준1
		hBitSniper[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP114)); // 정조준2
		hBitSniper[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP115)); // 정조준3
		hBitSniper[3] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP116)); // 정조준4
		hBitSniper[4] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP117)); // 정조준5
		hBitOne[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP118)); // 일격1
		hBitOne[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP119)); // 일격2
		hBitOne[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP120)); // 일격3
		hBitOne[3] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP121)); // 일격4
		hBitOne[4] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP122)); // 일격5
		hBitsword[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP123)); // 평타1
		hBitsword[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP124)); // 평타2
		hBitsword[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP125)); // 평타3
		hBitsword[3] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP126)); // 평타4
		hBitsword[4] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP127)); // 평타5
		hBittekai[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP128)); // 방어1
		hBittekai[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP129)); // 방어2
		hBittekai[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP130)); // 방어3
		hBittekai[3] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP131)); // 방어4
		hBittekai[4] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP132)); // 방어5
		hBithurt[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP133)); // 피격1
		hBithurt[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP134)); // 피격2
		hBithurt[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP135)); // 피격3
		hBitholiShild[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP136)); // 절대방어1
		hBitholiShild[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP137)); // 절대방어2
		hBitholiShild[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP138)); // 절대방어3
		hBitholiShild[3] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP139)); // 절대방어4
		hBitholiShild[4] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP140)); // 절대방어5

		hBit = CreateCompatibleBitmap(hDC, rectView.right, rectView.bottom);

		BITMAP bmp;
		// 메뉴 배경 이미지 크기
		GetObject(hBitBackground, sizeof(BITMAP), &bmp);
		backWidth = bmp.bmWidth;
		backHeight = bmp.bmHeight;
		// 메뉴 칸들 이미지 크기
		GetObject(hBitMenu, sizeof(BITMAP), &bmp);
		menuWidth = bmp.bmWidth;
		menuHeight = bmp.bmHeight;
		// 지도 이미지 크기
		GetObject(hBitTemp, sizeof(BITMAP), &bmp);
		tempWidth = bmp.bmWidth;
		tempHeight = bmp.bmHeight;
		// 임시 게임화면 크기
		GetObject(hBitTemp2, sizeof(BITMAP), &bmp);
		tempWidth2 = bmp.bmWidth;
		tempHeight2 = bmp.bmHeight;
		// 되돌아가기 메뉴 이미지
		GetObject(hBitReturn, sizeof(BITMAP), &bmp);
		returnWidth = bmp.bmWidth;
		returnHeight = bmp.bmHeight;
		// 카드 이미지
		GetObject(CARD[0], sizeof(BITMAP), &bmp);
		cardWidth = bmp.bmWidth;
		cardHeight = bmp.bmHeight;
		// 나무판자 이미지
		GetObject(hBitwood, sizeof(BITMAP), &bmp);
		woodWidth = bmp.bmWidth;
		woodHeight = bmp.bmHeight;
		// 돌 판자 이미지
		GetObject(hBitstone, sizeof(BITMAP), &bmp);
		stoneWidth = bmp.bmWidth;
		stoneHeight = bmp.bmHeight;
		// 보스 스테이지 아이콘
		GetObject(bossicon, sizeof(BITMAP), &bmp);
		bossiconWidth = bmp.bmWidth;
		bossiconHeight = bmp.bmHeight;
		// 4 스테이지 아이콘
		GetObject(Icon[0], sizeof(BITMAP), &bmp);
		stage4iconWidth = bmp.bmWidth;
		stage4iconHeight = bmp.bmHeight;
		// 3 스테이지 아이콘
		GetObject(Icon[2], sizeof(BITMAP), &bmp);
		stage3iconWidth = bmp.bmWidth;
		stage3iconHeight = bmp.bmHeight;
		// 2-1 스테이지 아이콘
		GetObject(Icon[5], sizeof(BITMAP), &bmp);
		stage21iconWidth = bmp.bmWidth;
		stage21iconHeight = bmp.bmHeight;
		// 2-2 스테이지 아이콘
		GetObject(Icon[6], sizeof(BITMAP), &bmp);
		stage22iconWidth = bmp.bmWidth;
		stage22iconHeight = bmp.bmHeight;
		// 1-1 스테이지 아이콘
		GetObject(Icon[7], sizeof(BITMAP), &bmp);
		stage1iconWidth = bmp.bmWidth;
		stage1iconHeight = bmp.bmHeight;
		// 1-1 슬라임
		GetObject(Boss1[0], sizeof(BITMAP), &bmp);
		enermy1Width = bmp.bmWidth;
		enermy1Height = bmp.bmHeight;
		// 2-1 기사
		GetObject(Boss2[0], sizeof(BITMAP), &bmp);
		enermy2Width = bmp.bmWidth;
		enermy2Height = bmp.bmHeight;
		// 2-2 주술사
		GetObject(Boss3[0], sizeof(BITMAP), &bmp);
		enermy3Width = bmp.bmWidth;
		enermy3Height = bmp.bmHeight;
		// 3-1 거북이
		GetObject(Boss5[0], sizeof(BITMAP), &bmp);
		enermy4Width = bmp.bmWidth;
		enermy4Height = bmp.bmHeight;
		// 3-2 개
		GetObject(Boss6[0], sizeof(BITMAP), &bmp);
		enermy5Width = bmp.bmWidth;
		enermy5Height = bmp.bmHeight;
		// 3-3 두더지
		GetObject(Boss7[0], sizeof(BITMAP), &bmp);
		enermy6Width = bmp.bmWidth;
		enermy6Height = bmp.bmHeight;
		// 4-1 마트료시카
		GetObject(Boss10[0], sizeof(BITMAP), &bmp);
		enermy7Width = bmp.bmWidth;
		enermy7Height = bmp.bmHeight;
		// 4-2 관
		GetObject(Boss14[0], sizeof(BITMAP), &bmp);
		enermy8Width = bmp.bmWidth;
		enermy8Height = bmp.bmHeight;
		// boss 1
		GetObject(Boss16[0], sizeof(BITMAP), &bmp);
		enermy9Width = bmp.bmWidth;
		enermy9Height = bmp.bmHeight;
		// boss 2
		GetObject(Boss17[0], sizeof(BITMAP), &bmp);
		enermy10Width = bmp.bmWidth;
		enermy10Height = bmp.bmHeight;
		// 쉴드 아이콘
		GetObject(hBitShild, sizeof(BITMAP), &bmp);
		ShildWidth = bmp.bmWidth;
		ShildHeight = bmp.bmHeight;
		GetObject(playbutton, sizeof(BITMAP), &bmp);
		playWidth = bmp.bmWidth;
		playHeight = bmp.bmHeight;
		//일시정지
		GetObject(hBitstop[0], sizeof(BITMAP), &bmp);
		stopWidth = bmp.bmWidth;
		stopHeight = bmp.bmHeight;
		//공격 아이콘
		GetObject(hBitattack, sizeof(BITMAP), &bmp);
		attackWidth = bmp.bmWidth;
		attackHeight = bmp.bmHeight;
		//주인공
		GetObject(Cha[0], sizeof(BITMAP), &bmp);
		chaWidth = bmp.bmWidth;
		chaHeight = bmp.bmHeight;
		//턴 엔드 버튼
		GetObject(endbutton[0], sizeof(BITMAP), &bmp);
		endbuttonWidth = bmp.bmWidth;
		endbuttonHeight = bmp.bmHeight;
		//폭발 이펙트
		GetObject(hBitBoom[0], sizeof(BITMAP), &bmp);
		BoomWidth = bmp.bmWidth;
		BoomHeight = bmp.bmHeight;
		//플레이 죽는 모습
		GetObject(hBitplayerdeath, sizeof(BITMAP), &bmp);
		pdWidth = bmp.bmWidth;
		pdHeight = bmp.bmHeight;
		//정조준 이펙트
		GetObject(hBitSniper[0], sizeof(BITMAP), &bmp);
		sniperWidth = bmp.bmWidth;
		sniperHeight = bmp.bmHeight;
		//일격 이펙트
		GetObject(hBitOne[0], sizeof(BITMAP), &bmp);
		OneWidth = bmp.bmWidth;
		OneHeight = bmp.bmHeight;
		//공격 이펙트
		GetObject(hBitsword[0], sizeof(BITMAP), &bmp);
		swordWidth = bmp.bmWidth;
		swordHeight = bmp.bmHeight;
		//텟카이 이펙트
		GetObject(hBittekai[0], sizeof(BITMAP), &bmp);
		tekaiWidth = bmp.bmWidth;
		tekaiHeight = bmp.bmHeight;
		//피격 이펙트
		GetObject(hBithurt[0], sizeof(BITMAP), &bmp);
		hurtWidth = bmp.bmWidth;
		hurtHeight = bmp.bmHeight;
		//절대 방어 이펙트
		GetObject(hBitholiShild[0], sizeof(BITMAP), &bmp);
		holiShildWidth = bmp.bmWidth;
		holiShildHeight = bmp.bmHeight;
	}

	void ReleaseAssets() {
		DeleteObject(hBitBackground);
		DeleteObject(hBitMenu);
		DeleteObject(hBitMenu2);
		DeleteObject(hBitMenu3);
		DeleteObject(hBitTemp);
		DeleteObject(hBitTemp2);
		DeleteObject(hBitReturn);
		DeleteObject(hBitwood);
		DeleteObject(hBitstone);
		for (int i = 0; i < 15; ++i) DeleteObject(CARD[i]);
		DeleteObject(bossicon);
		for (int i = 0; i < 8; ++i) DeleteObject(Icon[i]);
		for (int i = 0; i < 3; ++i) DeleteObject(Boss1[i]);
		DeleteObject(hBitShild);
		DeleteObject(playbutton);
		for (int i = 0; i < 2; ++i) DeleteObject(hBitstop[i]);
		DeleteObject(hBitattack);
		for (int i = 0; i < 3; ++i) DeleteObject(Cha[i]);
		for (int i = 0; i < 2; ++i) DeleteObject(endbutton[i]);
		for (int i = 0; i < 8; ++i) DeleteObject(hBitBoom[i]);
		DeleteObject(hBitplayerdeath);
		for (int i = 0; i < 3; ++i) DeleteObject(Boss2[i]);
		for (int i = 0; i < 3; ++i) DeleteObject(Boss3[i]);
		for (int i = 0; i < 3; ++i) DeleteObject(Boss4[i]);
		for (int i = 0; i < 3; ++i) DeleteObject(Boss5[i]);
		for (int i = 0; i < 3; ++i) DeleteObject(Boss6[i]);
		for (int i = 0; i < 5; ++i) DeleteObject(Boss7[i]);
		for (int i = 0; i < 3; ++i) DeleteObject(Boss8[i]);
		for (int i = 0; i < 3; ++i) DeleteObject(Boss9[i]);
		for (int i = 0; i < 3; ++i) DeleteObject(Boss10[i]);
		for (int i = 0; i < 3; ++i) DeleteObject(Boss11[i]);
		for (int i = 0; i < 3; ++i) DeleteObject(Boss12[i]);
		for (int i = 0; i < 3; ++i) DeleteObject(Boss13[i]);
		for (int i = 0; i < 3; ++i) DeleteObject(Boss14[i]);
		for (int i = 0; i < 3; ++i) DeleteObject(Boss15[i]);
		for (int i = 0; i < 6; ++i) DeleteObject(Boss16[i]);
		for (int i = 0; i < 6; ++i) DeleteObject(Boss17[i]);
		for (int i = 0; i < 5; ++i) DeleteObject(hBitSniper[i]);
		for (int i = 0; i < 5; ++i) DeleteObject(hBitOne[i]);
		for (int i = 0; i < 5; ++i) DeleteObject(hBitsword[i]);
		for (int i = 0; i < 5; ++i) DeleteObject(hBittekai[i]);
		for (int i = 0; i < 3; ++i) DeleteObject(hBithurt[i]);
		for (int i = 0; i < 5; ++i) DeleteObject(hBitholiShild[i]);
	}
};

class ClientLogic {
public:
	bool ConnectToServerAndStart(HWND hWnd);
	void HandleChar(WPARAM wParam, HWND hWnd);
	void HandleMouseMove(const GameState& state, PresentationState& pState, int x, int y, HWND hWnd);
	void HandleLButtonDown(const GameState& state, PresentationState& pState, int x, int y, HWND hWnd);
	void HandleLButtonUp(const GameState& state, PresentationState& pState, int x, int y, HWND hWnd);
	void OnServerEvent(PresentationState& pState /*eventPacket*/);
	void PlayCard(int cardID, int enemyID, short gridX, short gridY);
};

class Renderer {
public:
	void Render(HDC hdc, RECT rt, const GameState& state, const PresentationState& pState, const AssetManager& assets);

private:
	void DrawBackground(HDC hdc, HDC imgDC, const GameState& state, const PresentationState& pState, const AssetManager& assets);
	void DrawStartScreenUI(HDC hdc, HDC imgDC, const PresentationState& pState, const AssetManager& assets);
	void DrawPvPScreen(HDC hdc, HDC imgDC, const GameState& state, const PresentationState& pState, const AssetManager& assets);
	void DrawPVEScreen(HDC hdc, HDC imgDC, const GameState& state, const PresentationState& pState, const AssetManager& assets);
	void DrawHUD(HDC hdc, const GameState& state, const PresentationState& pState);
	void DrawHand(HDC hdc, HDC imgDC, const GameState& state, const PresentationState& pState, const AssetManager& assets);
	void DrawCharacters(HDC hdc, HDC imgDC, const GameState& state, const PresentationState& pState, const AssetManager& assets);
	void DrawEffects(HDC hdc, HDC imgDC, const GameState& state, const PresentationState& pState, const AssetManager& assets);

	void HPBar(HDC hDC, int x, int y, int hp);
	void ClearCross(HDC hDC, int x, int y, int r);
};

class Game {
public:
	void OnCreate(HWND hWnd, HINSTANCE hInst);
	void OnDestroy();
	void OnTimer();
	void OnPaint(HDC hdc, PAINTSTRUCT& ps, RECT& rt);
	void OnChar(WPARAM wParam);
	void OnMouseMove(int x, int y);
	void OnLButtonDown(int x, int y);
	void OnLButtonUp(int x, int y);
	void OnKeyDown(WPARAM wParam);
	void OnKeyUp(WPARAM wParam);
	void UpdateStateFromServer(const GameState& newState);

	HWND m_hWnd;
	AssetManager m_Assets;  // 자원 관리
	GameState m_State;    // 모든 서버 공유데이터
	PresentationState m_PState; // 클라 전용 데이터
	//GameLogic m_Logic;    // 서버용 규칙
	ClientLogic m_CLogic; // 클라용 규칙
	Renderer m_Renderer;  // 모든 그리기
};

static Game g_Game;

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

struct OP {
	int ptype;

	struct OP_KEY {
		char key;
	};
	struct OP_PLAYCARD {
		int cardID;
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
		op_playcard.cardID = n.op_playcard.cardID;
		op_playcard.enemyID = n.op_playcard.enemyID;
		op_playcard.pos_x = n.op_playcard.pos_x;
		op_playcard.pos_y = n.op_playcard.pos_y;
	}
};

SOCKET sock;
HANDLE g_hSendThread;
HANDLE g_hRecvThread;

HANDLE g_hMutexGameState;
HANDLE g_hMutexSendQueue;

std::queue< std::vector<char> > g_SendQueue;

HWND g_hWnd;
int g_myPlayerIndex = -1;

void participateInMatch(bool ispvp) {
	char buff[2] = { 2, ispvp };
	int n = send(sock, buff, 2, 0);
	if (n == SOCKET_ERROR) {
		cout << "전송을 실패했습니다." << endl;
		return;
	}
}

unsigned __stdcall Send_Thread(void* arg)
{
	SOCKET sock = (SOCKET)arg;
	std::vector<char> packetToSend;
	bool bHasPacket = false;

	while (true)
	{
		bHasPacket = false;
		WaitForSingleObject(g_hMutexSendQueue, INFINITE);

		if (!g_SendQueue.empty()) {
			packetToSend = g_SendQueue.front();
			g_SendQueue.pop();
			bHasPacket = true;
		}

		ReleaseMutex(g_hMutexSendQueue);

		if (bHasPacket) {
			int retval = send(sock, packetToSend.data(), packetToSend.size(), 0); // 임시
			// 프로토콜에 따른 값 보내기

			if (retval == SOCKET_ERROR) {
				break;
			}
		}
		else {
			Sleep(10); // 스레드 휴먼 : 큐 비어있을때 CPU 점유 방지
		}
	}
	return 0;
}

unsigned __stdcall Recv_Thread(void* arg)
{
	SOCKET sock = (SOCKET)arg;
	char ptype;
	int retval;

	while (true)
	{
		retval = recv(sock, &ptype, 1, MSG_WAITALL);

		if (retval == SOCKET_ERROR || retval == 0) {
			MessageBox(g_hWnd, L"서버와의 연결이 끊겼습니다.", L"접속 종료", MB_OK);
			PostMessage(g_hWnd, WM_CLOSE, 0, 0);
			break;
		}

		WaitForSingleObject(g_hMutexGameState, INFINITE);

		if (ptype >= 0 && ptype < 96) {
			int playerIndex = ptype / PLAYER_SYNC_STRIDE; // 0, 1, 2
			int baseType = ptype % PLAYER_SYNC_STRIDE;

			PlayerData& targetPlayer = g_Game.m_State.players[playerIndex];

			switch (baseType)
			{
			case SYNC_HP:
				recv(sock, (char*)&targetPlayer.hp, sizeof(int), MSG_WAITALL);
				break;
			case SYNC_MAX_MANA:
				recv(sock, (char*)&targetPlayer.maxMana, sizeof(float), MSG_WAITALL);
				break;
			case SYNC_MANA:
				recv(sock, (char*)&targetPlayer.mana, sizeof(float), MSG_WAITALL);
				break;
			case SYNC_DEFFENCE:
				recv(sock, (char*)&targetPlayer.defence, sizeof(int), MSG_WAITALL);
				break;
			case SYNC_ATTACK:
				recv(sock, (char*)&targetPlayer.attack, sizeof(int), MSG_WAITALL);
				break;
			case SYNC_POS:
				recv(sock, (char*)&targetPlayer.pos, sizeof(Pos), MSG_WAITALL);
				break;
			case SYNC_HAND_SLOT_0:
				recv(sock, (char*)&targetPlayer.hand[0], sizeof(CardData), MSG_WAITALL);
				break;
			case SYNC_HAND_SLOT_1:
				recv(sock, (char*)&targetPlayer.hand[0], sizeof(CardData), MSG_WAITALL);
				break;
			case SYNC_HAND_SLOT_2:
				recv(sock, (char*)&targetPlayer.hand[0], sizeof(CardData), MSG_WAITALL);
				break;
			case SYNC_HAND_SLOT_3:
				recv(sock, (char*)&targetPlayer.hand[0], sizeof(CardData), MSG_WAITALL);
				break;
			case SYNC_HAND_SLOT_4:
				recv(sock, (char*)&targetPlayer.hand[0], sizeof(CardData), MSG_WAITALL);
				break;
			case SYNC_PLAYER_DEATH:
				recv(sock, (char*)&targetPlayer.playerdeath, sizeof(bool), MSG_WAITALL);
				break;
			case SYNC_CUTTING:
				recv(sock, (char*)&targetPlayer.cutting, sizeof(bool), MSG_WAITALL);
				break;
			case SYNC_INVINCIBLE:
				recv(sock, (char*)&targetPlayer.invincible, sizeof(bool), MSG_WAITALL);
				break;
			case SYNC_IS_CASTING_ONEPUNCH:
				recv(sock, (char*)&targetPlayer.isCastingOnePunch, sizeof(bool), MSG_WAITALL);
				break;
			case SYNC_ONEPUNCH_TIMER:
				recv(sock, (char*)&targetPlayer.onePunchCastTimer, sizeof(float), MSG_WAITALL);
				break;
			case SYNC_ONEPUNCH_DAMAGE:
				recv(sock, (char*)&targetPlayer.onePunchStoredDamage, sizeof(int), MSG_WAITALL);
				break;
			case SYNC_PARING_MOMENT:
				recv(sock, (char*)&targetPlayer.ParingMoment, sizeof(bool), MSG_WAITALL);
				break;
			}
		}
		else {
			switch ((ServerToClient_ProtocolType)ptype)
			{
			case STC_PT_InitGame: // 99번 게임 초기화 값
			{
				GameState initialState;
				int clientIndex;

				recv(sock, (char*)&clientIndex, sizeof(int), MSG_WAITALL);
				recv(sock, (char*)&initialState, sizeof(GameState), MSG_WAITALL);

				g_myPlayerIndex = clientIndex;
				g_Game.UpdateStateFromServer(initialState); 
				break;
			}
			case STC_Sync_MapData: // 116번
				recv(sock, (char*)g_Game.m_State.mapData, sizeof(g_Game.m_State.mapData), MSG_WAITALL);
				break;
			}
		}

		ReleaseMutex(g_hMutexGameState);

		InvalidateRect(g_hWnd, NULL, FALSE);
	}
	return 0;
}

char* SERVERIP = (char*)"127.0.0.1";

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		return 1;
	}

	HWND hWnd;
	MSG Message;
	WNDCLASSEX WndClass;
	g_hInst = hInstance;

	WndClass.cbSize = sizeof(WndClass);
	WndClass.style = CS_HREDRAW | CS_VREDRAW;
	WndClass.lpfnWndProc = (WNDPROC)WndProc;
	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hInstance = hInstance;
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hCursor = LoadCursor(NULL, IDC_HAND);
	WndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	WndClass.lpszMenuName = NULL;
	WndClass.lpszClassName = lpszClass;
	WndClass.hIconSm = LoadIcon(NULL, IDI_QUESTION);
	RegisterClassEx(&WndClass);

	hWnd = CreateWindow(lpszClass, lpszWindowName, WS_OVERLAPPEDWINDOW, 100, 50, 1200, 800, NULL, (HMENU)NULL, hInstance, NULL);
	g_hWnd = hWnd;

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	while (GetMessage(&Message, 0, 0, 0)) {
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}

	if (g_hSendThread) CloseHandle(g_hSendThread);
	if (g_hRecvThread) CloseHandle(g_hRecvThread);
	if (g_hMutexGameState) CloseHandle(g_hMutexGameState);
	if (g_hMutexSendQueue) CloseHandle(g_hMutexSendQueue);

	if (sock != INVALID_SOCKET) closesocket(sock);
	WSACleanup();

	return Message.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HDC hDC;
	PAINTSTRUCT ps;
	RECT rt;

	switch (uMsg) {
	case WM_CREATE:
		g_Game.OnCreate(hWnd, g_hInst);
		break;

	case WM_TIMER:
		g_Game.OnTimer();
		break;

	case WM_PAINT: // 모든 그리기는 여기서
		GetClientRect(hWnd, &rt);
		hDC = BeginPaint(hWnd, &ps);
		g_Game.OnPaint(hDC, ps, rt);
		EndPaint(hWnd, &ps);
		break;

	case WM_KEYDOWN:
		g_Game.OnKeyDown(wParam);
		break;
	case WM_KEYUP:
		g_Game.OnKeyUp(wParam);
		break;
	case WM_CHAR:
		g_Game.OnChar(wParam);
		break;
	case WM_MOUSEMOVE:
		g_Game.OnMouseMove(LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_LBUTTONDOWN:
		g_Game.OnLButtonDown(LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_LBUTTONUP:
		g_Game.OnLButtonUp(LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_DESTROY:
		g_Game.OnDestroy();
		break;

	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return 0;
}

void Game::OnCreate(HWND hWnd, HINSTANCE hInst)
{
	m_hWnd = hWnd;

	HDC hDC = GetDC(hWnd);
	RECT rectView;
	GetClientRect(hWnd, &rectView);

	m_Assets.LoadAssets(hInst, hDC, rectView);
	ReleaseDC(hWnd, hDC);

	m_PState.Initialize();

	SetTimer(m_hWnd, 1, 17, NULL);
	PlaySound(TEXT("Lost Forest1.wav"), NULL, SND_FILENAME | SND_ASYNC);
}

void Game::OnDestroy()
{
	m_Assets.ReleaseAssets();

	KillTimer(m_hWnd, 1);
	PlaySound(NULL, 0, 0);
	PostQuitMessage(0);
}

void Game::OnTimer()
{
	float deltaTime = 0.017f;
	m_PState.Update(deltaTime, m_State);

	InvalidateRect(m_hWnd, NULL, FALSE);
}

void Game::OnPaint(HDC hdc, PAINTSTRUCT& ps, RECT& rt)
{
	m_Renderer.Render(hdc, rt, m_State, m_PState, m_Assets);
}

void Game::OnChar(WPARAM wParam)
{
	m_CLogic.HandleChar(wParam, m_hWnd);
}

void Game::OnMouseMove(int x, int y)
{
	m_CLogic.HandleMouseMove(m_State, m_PState, x, y, m_hWnd);
}

void Game::OnLButtonDown(int x, int y)
{
	m_CLogic.HandleLButtonDown(m_State, m_PState, x, y, m_hWnd);
}

void Game::OnLButtonUp(int x, int y)
{
	m_CLogic.HandleLButtonUp(m_State, m_PState, x, y, m_hWnd);
}

void Game::OnKeyDown(WPARAM wParam)
{
	constexpr float MoveMargin = 60;
	if (wParam == 'W' && inputdata.WPress == false) {
		//m_State.player->Move(0, 1);
		inputdata.WPress = true;
		char buf[2] = { 0, 'w' };
		send(sock, buf, 2, 0);
	}
	else if (wParam == 'A' && inputdata.APress == false) {
		//m_State.player->Move(-1, 0);
		inputdata.APress = true;
		char buf[2] = { 0, 'a' };
		send(sock, buf, 2, 0);
	}
	else if (wParam == 'S' && inputdata.SPress == false) {
		//m_State.player->Move(0, -1);
		inputdata.SPress = true;
		char buf[2] = { 0, 's' };
		send(sock, buf, 2, 0);
	}
	else if (wParam == 'D' && inputdata.DPress == false) {
		//m_State.player->Move(1, 0);
		inputdata.DPress = true;
		char buf[2] = { 0, 'd' };
		send(sock, buf, 2, 0);
	}
	else if (wParam == VK_SPACE && inputdata.SpacePress == false) {
		inputdata.SpacePress = true;
		//m_State.player->ParingMoment = true;
		char buf[2] = { 0, VK_SPACE };
		send(sock, buf, 2, 0);
	}
}

void Game::OnKeyUp(WPARAM wParam)
{
	if (wParam == 'W') {
		inputdata.WPress = false;
	}
	else if (wParam == 'A') {
		inputdata.APress = false;
	}
	else if (wParam == 'S') {
		inputdata.SPress = false;
	}
	else if (wParam == 'D') {
		inputdata.DPress = false;
	}
	else if (wParam == VK_SPACE) {
		inputdata.SpacePress = false;
	}
}

void Game::UpdateStateFromServer(const GameState& newState)
{
	for (int i = 0; i < GameState::playerCount; ++i) {
		m_State.players[i] = newState.players[i];
	}

	m_State.boss = newState.boss;
	memcpy(m_State.mapData, newState.mapData, sizeof(m_State.mapData));

	m_State.PvEMode = newState.PvEMode;
	m_State.GameClear = newState.GameClear;
	m_State.tempstop = newState.tempstop;
	m_State.dontattackcard = newState.dontattackcard;

}

bool ClientLogic::ConnectToServerAndStart(HWND hWnd)
{
	if (sock != 0 && sock != INVALID_SOCKET) {
		return true;
	}

	int retval;
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		MessageBox(hWnd, L"소켓 생성 실패", L"오류", MB_OK);
		return false;
	}

	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	inet_pton(AF_INET, SERVERIP, &serveraddr.sin_addr);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = connect(sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	
	if (retval == SOCKET_ERROR) {
		MessageBox(hWnd, L"서버 접속 실패", L"오류", MB_OK);
		closesocket(sock);
		sock = INVALID_SOCKET;
		return false;
	}

	g_hMutexGameState = CreateMutex(NULL, FALSE, NULL);
	g_hMutexSendQueue = CreateMutex(NULL, FALSE, NULL);
	if (g_hMutexGameState == NULL || g_hMutexSendQueue == NULL) {
		MessageBox(hWnd, L"뮤텍스 생성 실패", L"오류", MB_OK);
		return 0;
	}

	unsigned int sendThreadID, recvThreadID;
	// Send_Thread
	g_hSendThread = (HANDLE)_beginthreadex(NULL, 0, Send_Thread, (void*)sock, 0, &sendThreadID);
	// Recv_Thread
	g_hRecvThread = (HANDLE)_beginthreadex(NULL, 0, Recv_Thread, (void*)sock, 0, &recvThreadID);
	if (g_hSendThread == 0 || g_hRecvThread == 0) {
		MessageBox(hWnd, L"스레드 생성에 실패했습니다.", L"오류", MB_OK);
		return 0;
	}

	return true;
}

void ClientLogic::HandleChar(WPARAM wParam, HWND hWnd)
{
	if (wParam == 'q') { // 종료
		PostQuitMessage(0);
	}
}

void ClientLogic::HandleMouseMove(const GameState& state, PresentationState& pState, int x, int y, HWND hWnd)
{
	if (pState.StartScreen) {
		if (x >= 900 && x <= 1150 && y >= 450 && y <= 500) pState.start = true;
		else pState.start = false;

		if (x >= 900 && x <= 1150 && y >= 530 && y <= 580) pState.descrtion = true;
		else pState.descrtion = false;

		if (x >= 900 && x <= 1150 && y >= 610 && y <= 660) pState.end = true;
		else pState.end = false;
	}
	else {
		if (state.PvEMode) { // 레이드 
			bool isDragging = false;
			for (int i = 0; i < 5; ++i) if (pState.hand[i].drag) isDragging = true;

			if (isDragging) {
				if (x >= 900 - 70 && x <= 900 + 70 && y >= 400 - 60 && y <= 400 + 60) pState.enermytouch = true;
				else pState.enermytouch = false;
			}
			else pState.enermytouch = false;

			if (x >= 1050 && x <= 1120 && y >= 600 && y <= 655) pState.endon = true;
			else pState.endon = false;

			for (int i = 0; i < 5; ++i) {
				if (x >= pState.hand[i].x - 70 && x <= pState.hand[i].x + 70 && y >= pState.hand[i].y - 100 && y <= pState.hand[i].y + 100) {
					pState.hand[i].select = true;
				}
				else pState.hand[i].select = false;

				if (pState.hand[i].drag) {
					pState.hand[i].x = x;
					pState.hand[i].y = y;
				}
			}
		}
		else { // pvp
			bool isDragging = false;
			for (int i = 0; i < 5; ++i) if (pState.hand[i].drag) isDragging = true;

			if (isDragging) {
				if (x >= 900 - 70 && x <= 900 + 70 && y >= 400 - 60 && y <= 400 + 60) pState.enermytouch = true;
				else pState.enermytouch = false;
			}
			else pState.enermytouch = false;

			if (x >= 1050 && x <= 1120 && y >= 600 && y <= 655) pState.endon = true;
			else pState.endon = false;

			for (int i = 0; i < 5; ++i) {
				if (x >= pState.hand[i].x - 70 && x <= pState.hand[i].x + 70 && y >= pState.hand[i].y - 100 && y <= pState.hand[i].y + 100) {
					pState.hand[i].select = true;
				}
				else pState.hand[i].select = false;

				if (pState.hand[i].drag) {
					pState.hand[i].x = x;
					pState.hand[i].y = y;
				}
			}
		}
	}
}

void ClientLogic::HandleLButtonDown(const GameState& state, PresentationState& pState, int x, int y, HWND hWnd)
{
	if (pState.StartScreen) {
		bool connectSucess = false;
		if (x >= 900 && x <= 1150 && y >= 450 && y <= 500) { // pvp
			connectSucess = ConnectToServerAndStart(hWnd);
			if (connectSucess) {
				participateInMatch(true);
				pState.StartScreen = false;
			}
			else {
				pState.StartScreen = true;
			}
			
		}
		if (x >= 900 && x <= 1150 && y >= 530 && y <= 580) { // 레이드
			connectSucess = ConnectToServerAndStart(hWnd);
			if (connectSucess) {
				participateInMatch(false);
				pState.StartScreen = false;
			}
			else {
				pState.StartScreen = true;
			}
		}
		if (x >= 900 && x <= 1150 && y >= 610 && y <= 660) PostQuitMessage(0); // 게임종료
	}
	else {
		if (g_myPlayerIndex < 0 || g_myPlayerIndex >= GameState::playerCount) {
			return; // 아직 매칭 중이므로 클릭 무시
		}
		const PlayerData& myPlayer = state.players[g_myPlayerIndex];

		if (state.PvEMode) { // 레이드
			if (x >= 175 && x <= 225 && y >= 15 && y <= 65) { //일시정지
				//state.tempstop = !state.tempstop;
				//if (state.tempstop) KillTimer(hWnd, 1); 
				//else SetTimer(hWnd, 1, 100, NULL);
			}
			if (x >= 1050 && x <= 1120 && y >= 600 && y <= 655)
				//state.player->mana = 0; // 턴 종료

			for (int i = 0; i < 5; ++i) {
				if (pState.hand[i].on && x >= pState.hand[i].x - 70 && x <= pState.hand[i].x + 70 && y >= pState.hand[i].y - 100 && y <= pState.hand[i].y + 100 && !(myPlayer.isCastingOnePunch)) {
					pState.hand[i].drag = true;
				}
				else pState.hand[i].drag = false;
			}
			if (x >= 10 && x <= 70 && y >= 10 && y <= 70) { // 시작화면
				pState.StartScreen = true;
			}
		}
		else { // pvp
			if (x >= 175 && x <= 225 && y >= 15 && y <= 65) { //일시정지
				/*state.tempstop = !state.tempstop;
				if (state.tempstop) KillTimer(hWnd, 1);
				else SetTimer(hWnd, 1, 100, NULL);*/
			}
			if (x >= 1050 && x <= 1120 && y >= 600 && y <= 655)
				//state.player->mana = 0; // 턴 종료

			for (int i = 0; i < 5; ++i) {
				if (pState.hand[i].on && x >= pState.hand[i].x - 70 && x <= pState.hand[i].x + 70 && y >= pState.hand[i].y - 100 && y <= pState.hand[i].y + 100 && !(myPlayer.isCastingOnePunch)) {
					pState.hand[i].drag = true;
				}
				else pState.hand[i].drag = false;
			}
			if (x >= 10 && x <= 70 && y >= 10 && y <= 70) {
				pState.StartScreen = true; // 시작 화면으로
			}
		}
	}
}

void ClientLogic::HandleLButtonUp(const GameState& state, PresentationState& pState, int x, int y, HWND hWnd)
{
	if (state.PvEMode) { // 레이드
		for (int i = 0; i < 5; ++i) {
			if (pState.hand[i].drag) {
				if (i == 0) { pState.hand[i].x = 300; pState.hand[i].y = 700; }
				else if (i == 1) { pState.hand[i].x = 450; pState.hand[i].y = 700; }
				else if (i == 2) { pState.hand[i].x = 600; pState.hand[i].y = 700; }
				else if (i == 3) { pState.hand[i].x = 750; pState.hand[i].y = 700; }
				else if (i == 4) { pState.hand[i].x = 900; pState.hand[i].y = 700; }

				if (pState.enermytouch) { // 카드 발동
					//PlayCard(pState.hand[i].id, )
				}
				pState.hand[i].drag = false;
			}
		}
	}
	else { // pvp
		for (int i = 0; i < 5; ++i) {
			if (pState.hand[i].drag) {
				if (i == 0) { pState.hand[i].x = 300; pState.hand[i].y = 700; }
				else if (i == 1) { pState.hand[i].x = 450; pState.hand[i].y = 700; }
				else if (i == 2) { pState.hand[i].x = 600; pState.hand[i].y = 700; }
				else if (i == 3) { pState.hand[i].x = 750; pState.hand[i].y = 700; }
				else if (i == 4) { pState.hand[i].x = 900; pState.hand[i].y = 700; }

				if (pState.enermytouch) { // 카드 발동
					//PlayCard(pState.hand[i].id, )
				}
				pState.hand[i].drag = false;
			}
		}
	}
}

void ClientLogic::OnServerEvent(PresentationState& pState)
{
	/*
	if (event.type == "CardUsed") {
            
            // 
            if (cardID == 0 || cardID == 1 || cardID == 2 || cardID == 4 || cardID == 5 || cardID == 10 || cardID == 11 || cardID == 13)
            {
                // 
                const PlayerPresentation& p = pState.players[playerID];
                ThrowCard tc = ThrowCard(Pos(p.x, p.y), Pos(ThrowCard::monsterPos, ThrowCard::monsterPos), 0.75f);
                tc.cardID = cardID;
                pState.CardThrow(tc); // 
            }
            
            pState.killmana = event.manaCost;
            pState.players[playerID].effect_anim_data.manaDown = true;
            pState.players[playerID].effect_anim_data.manaDownTime = 0;
        }
        
        // 
        if (event.type == "PlayCardFailed") {
            int cardHandIndex = event.cardHandIndex;
            CardUI& card = pState.hand[cardHandIndex];
            
            // 
            if (cardHandIndex == 0) { card.x = 300; card.y = 700; }
            // ... ( )
        }

        // 
        // 
        if (event.type == "Sword") {
            pState.sword = true;
            pState.swordcount = 0;
        }
        if (event.type == "Quake") {
            pState.quake = true;
            pState.quaketime = 0;
        }
	if (event.type == "StartBattle") {
            // 1. 
            pState.startstart = true;
            pState.startstarttime = 0;
            
            // 2. 
            pState.boomswitch = false;
            pState.boomcount = 0;
            
            // 3. 
            // 
            pState.InitializeBattleVisuals(gameState.boss.id);
        }
		if (event.type == "BossDefUp") {
			int defAmount = event.amount;

			pState.boss.defUp = true;      
			pState.boss.defUpTime = 0;   
			pState.enermydeff = defAmount; 
		}
	if (eventPacket.type == "BossDehp") {
		pState.dehp = true;

		pState.damage = eventPacket.hpDamage;
		pState.enermydeffdown = eventPacket.defenseDamage;

		pState.dehptime = 0;

		if (pState.enermydeffdown > 0) {
			pState.boss.defDown = true;
			pState.boss.defDownTime = 0;
		}
	}
	if (event.type == "PlayerHurt") {
			int playerID = event.playerID;
			int hpLost = event.hpDamage;
			int defLost = event.defDamage;

			PlayerPresentation& p = pState.players[playerID];
			PlayerPresentation::EffectAnimData& ead = p.effect_anim_data;

			ead.hurt = true; 

			pState.dedamge = hpLost; 
			pState.defensedown = defLost; 

			if (hpLost > 0) {
				ead.decresehp = true; 
				ead.decresehptime = 0;
			}
			if (defLost > 0) {
				ead.defDown = true; 
				ead.defDownTime = 0;
			}
		}
	if (event.type == "BossBoom") {
            boomswitch = true;
            boomcount = 0;
            quake = true;
            quaketime = 0;
        }
        if (event.type == "PlayerDied") {
            pdeath = true;
            pdeathtime = 0;
        }
        if (event.type == "DrawCards") {
            droww = true;
        }
        if (event.type == "OnePunch") {
            One = true;
            Onecount = 0;
        }
	if (event.type == "BossHeal") {
            pState.boss.heal = true;
            pState.boss.healEnergy = event.healAmount; // 
            pState.boss.healTime = 0; // 
            
            // 
            // 
            // 
            pState.boss.visual_hp = gameState.boss.hp - event.healAmount;
            if (pState.boss.visual_hp < 0) pState.boss.visual_hp = 0;
        }
	*/

}

void ClientLogic::PlayCard(int cardID, int enemyID, short gridX, short gridY)
{
	OP packet;
	packet.ptype = CTS_PT_PlayCard;
	packet.op_playcard.cardID = cardID;
	packet.op_playcard.enemyID = enemyID;
	packet.op_playcard.pos_x = gridX;
	packet.op_playcard.pos_y = gridY;

	std::vector<char> buffer(13);
	buffer[0] = 1;
	memcpy(&buffer[1], &cardID, sizeof(int));
	memcpy(&buffer[5], &enemyID, sizeof(int));
	memcpy(&buffer[9], &gridX, sizeof(short));
	memcpy(&buffer[11], &gridY, sizeof(short));

	WaitForSingleObject(g_hMutexSendQueue, INFINITE);
	g_SendQueue.push(buffer);
	ReleaseMutex(g_hMutexSendQueue);
}

void PresentationState::Initialize()
{
	StartScreen = true;

	// 카드 위치/ID 초기화
	hand[0].x = 300;
	hand[0].y = 900;
	hand[1].x = 450;
	hand[1].y = 900;
	hand[2].x = 600;
	hand[2].y = 900;
	hand[3].x = 750;
	hand[3].y = 900;
	hand[4].x = 900;
	hand[4].y = 900;

	players[0].x = MapCenterX;
	players[0].y = MapCenterY;
}

void PresentationState::Update(float deltaTime, const GameState& gameState)
{
	for (int i = 0; i < GameState::playerCount; ++i) {
		const PlayerData& serverPlayer = gameState.players[i];
		PlayerPresentation& visualPlayer = this->players[i];

		visualPlayer.UpdataPosition(serverPlayer.pos);

		if (serverPlayer.invincible && !visualPlayer.effect_anim_data.holiShild) {
			visualPlayer.effect_anim_data.holiShild = true;
			visualPlayer.effect_anim_data.holiShildcount = 0;
		}
		else if (!serverPlayer.invincible) {
			visualPlayer.effect_anim_data.holiShild = false;
		}
	}

	CardUpdate(deltaTime, gameState);

	if (droww) { // 카드 덱 뽑기 모션
		for (int i = 0; i < 5; ++i) {
			hand[i].y -= 5;
			hand[i].on = true;
			if (hand[i].y <= 700) {
				hand[i].y = 700;
				droww = false;
			}
		}
	}

	if (trunendd) { // 카드 덱이 넣기 모션
		for (int i = 0; i < 5; ++i) {
			hand[i].y += 5;
			if (hand[i].y >= 900) {
				hand[i].y = 900;
				trunendd = false;
			}
		}
	}
	//animation
	animTimer += deltaTime;
	if (animTimer >= 0.1f) { // 프레임 전환 속도 (숫자가 작을수록 빠름)
		animTimer -= 0.1f;

		UpdateBuffsAndTimers(deltaTime, gameState);

		for (int i = 0; i < GameState::playerCount; ++i) {
			PlayerPresentation& p = players[i];
			p.animCount++;
			if (p.animCount >= 3) { // 플레이어 이미지 개수
				p.animCount = 0;
			}
		}

		// 보스 모션
		if (!gameState.boss.death) {
			boss.animCount++;
			if (boss.animCount >= 3) { // 적 이미지 개수
				boss.animCount = 0;
			}
		}
	}
}

void PresentationState::UpdateBuffsAndTimers(float deltaTime, const GameState& gameState)
{
	// 적 방어 업
	if (boss.defUp) { 
		boss.defUpTime++;
		if (boss.defUpTime == 7) {
			boss.defUpTime = 0;
			boss.defUp = false;
		}
	}
	// 적 방어 다운
	if (boss.defDown) { 
		boss.defDownTime++;
		if (boss.defDownTime == 7) {
			boss.defDownTime = 0;
			boss.defDown = false;
		}
	}
	// 적 hp바 감소 모션
	if (dehp) { 
		if (damage > 0) {
			if (damage >= 2) {
				damage -= 2;
			}
			else {
				damage = 0;
			}
			dehptime++;
		}
		else if (enermydeffdown > 0) {
			if (enermydeffdown >= 2) {
				enermydeffdown -= 2;
			}
			else {
				enermydeffdown = 0;
			}
			dehptime++;
		}
		else {
			dehptime = 0;
			dehp = false;
		}
	}
	// 시작 자막
	if (startstart) { 
		startstarttime++;
		if (startstarttime == 15) {
			startstarttime = 0;
			startstart = false;
		}
	}
	// 스테이지 클 자막
	if (endend) { 
		endendtime++;
		if (endendtime >= 15) {
			endendtime = 0;
			endend = false;
		}
	}
	// 게임 오버 자막
	if (pdeath) { 
		pdeathtime++;
		if (pdeathtime >= 15) {
			pdeathtime = 0;
			pdeath = false;
		}
		//StartScreen = true;
	}
	//지진
	if (quake) { 
		quaketime++;
		if (backgroundtime == 0 || backgroundtime == 2) {
			backgroundX -= 15;
			backgroundY -= 15;
		}
		else if (backgroundtime == 1 || backgroundtime == 3) {
			backgroundX += 15;
			backgroundY += 15;
		}
		backgroundtime++;
		backgroundtime %= 4;
		if (quaketime >= 4) {
			quaketime = 0;
			quake = false;
			backgroundX = 0;
			backgroundY = 0;
		}
	}
	// 특이 보스 공격모션 애니
	/*if (state.boss.bossmode3) {
		state.boss.animCount++;
		if (state.boss.animCount >= 9) {
			state.boss.bossmode3 = false;
			state.dontattackcard = false;
			state.boss.bossmode2 = false;
			state.boss.animCount = 0;
		}
	}*/
	// 폭발 애니메이션
	if (boomswitch) {
		boomcount++;
		if (boomcount >= 8) {
			boomswitch = false;
			boomcount = 0;
			// *** 스테이지 클리어 처리 ***
			endend = true; // 클리어 자막 시작
		}
	}

	if (Sniper) {
		snipercount++;
		if (snipercount >= 5) { Sniper = false; snipercount = 0; }
	}
	if (One) {
		Onecount++;
		if (Onecount >= 5) { One = false; Onecount = 0; }
	}
	if (sword) {
		swordcount++;
		if (swordcount >= 5) { sword = false; swordcount = 0; }
	}

	for (int i = 0; i < GameState::playerCount; ++i) {
		//player effect
		PlayerPresentation& p = players[i];
		PlayerPresentation::EffectAnimData& ead = p.effect_anim_data;
		if (ead.manaDown) { //행동력 다운 효과
			ead.manaDownTime += 1;
			if (ead.manaDownTime >= 7) {
				ead.manaDownTime = 0;
				ead.manaDown = false;
			}
		}
		if (ead.manaUp) { //행동력 업 효과
			ead.manaUpTime++;
			if (ead.manaUpTime == 7) {
				ead.manaUpTime = 0;
				ead.manaUp = false;
			}
		}
		if (ead.defUp) { // 플레이어 방어력 업
			ead.defUpTime++;
			if (ead.defUpTime == 7) {
				ead.defUpTime = 0;
				ead.defUp = false;
			}
		}
		if (ead.defDown) { // 플레이어 방어력 다운
			ead.defDownTime++;
			if (ead.defDownTime == 7) {
				ead.defDownTime = 0;
				ead.defDown = false;
			}
		}

		if (ead.powerUp) { // 주인공 공격력 추가 업
			ead.powerUpTime++;
			if (ead.powerUpTime == 7) {
				ead.powerUpTime = 0;
				ead.powerUp = false;
			}
		}
		if (ead.powerDown) { // 주인공 공격력 추가 업
			ead.powerDownTime++;
			if (ead.powerDownTime == 7) {
				ead.powerDownTime = 0;
				ead.powerDown = false;
			}
		}

		if (ead.decresehp) { // 주인공 hp 감소 효과
			ead.decresehptime++;
			if (ead.decresehptime == 7) {
				ead.decresehptime = 0;
				ead.decresehp = false;
			}
		}
		if (ead.myheal) { // hp 회복 효과
			ead.healTime++;
			if (ead.healTime == 7) {
				ead.healTime = 0;
				ead.myheal = false;
			}
		}

		if (p.attackTime > 0) { // 주인공 공격 모션
			p.attackTime++;
			if (p.attackTime == 2) {
				p.x += 100;
			}
			else if (p.attackTime == 3) {
				p.x -= 100;
			}
			else if (p.attackTime >= 4) {
				p.attackTime = 0;
			}
		}

		if (ead.tekai) {
			ead.tekaicount++;
			if (ead.tekaicount >= 5) { ead.tekai = false; ead.tekaicount = 0; }
		}
		if (ead.hurt) {
			ead.hurtcount++;
			if (ead.hurtcount >= 3) { ead.hurt = false; ead.hurtcount = 0; }
		}
		if (ead.holiShild) { // 굳건한 태세는 계속 반복
			ead.holiShildcount++;
			ead.holiShildcount %= 5;
		}
		else {
			ead.holiShildcount = 0; // 꺼지면 리셋
		}
	}
	// boss hp바 회복 모션
	if (boss.heal) { 
		boss.healTime += 10;
		// hp 바
		if (boss.healTime >= gameState.boss.healEnergy) {
			boss.healTime = 0;
			boss.heal = false;
		}
	}

	// boss 공격 모션
	if (boss.attackTime > 0) {
		boss.attackTime++;

		if (boss.attackTime == 2) {
			boss.x -= 100; // 
		}
		else if (boss.attackTime == 3) {
			boss.x += 100; // 
		}
		else if (boss.attackTime >= 22) {
			boss.attackTime = 0; // 
		}
	}
}

void PresentationState::InitializeBattleVisuals(int bossID)
{
	for (int i = 0; i < 5; ++i) {
		hand[i].x = 300 + 150 * i;
		hand[i].y = 900;
	}

	if (bossID == 0) {
		boss.x = 825;
		boss.y = 350;
	}
	else if (bossID == 1) {
		boss.x = 825;
		boss.y = 250;
	}
	else if (bossID == 2) {
		boss.x = 825;
		boss.y = 250;
	}
	else if (bossID == 3) {
		boss.x = 755;
		boss.y = 180;
	}
	else if (bossID == 4) {
		boss.x = 820;
		boss.y = 200;
	}
	else if (bossID == 5) {
		boss.x = 825;
		boss.y = 250;
	}
	else if (bossID == 6) {
		boss.x = 755;
		boss.y = 180;
	}
	else if (bossID == 7) {
		boss.x = 755;
		boss.y = 180;
	}
	else if (bossID == 8) {
		boss.x = 755;
		boss.y = 220;
	}
}

void PresentationState::CardUpdate(float deltaTime, const GameState& gameState)
{
	//constexpr float ParingEpsilon = 0.1f;
	//constexpr float CardThrowingTimeDecreaseRate = 0.95f;

	//list<ThrowCard>::iterator EraseArr[128] = {};
	//int up = 0;

	//for (list<ThrowCard>::iterator iter = throwCardList.begin(); iter != throwCardList.end(); ++iter) {
	//	ThrowCard& tc = *iter;
	//	tc.Update(deltaTime);
	//	if (fabsf(tc.flowTime - tc.maxTime) <= ParingEpsilon) {
	//		if (tc.end_p.x != ThrowCard::monsterPos) {
	//			for (int i = 0; i < gameState.playerCount; ++i) {
	//				players[i].ParingMoment = true;
	//				if ((tc.end_p.x == players[i].x && tc.end_p.y == players[i].y) || tc.end_p.x == ThrowCard::playerPos[i]) {
	//					if (players[i].ParingMoment) {
	//						tc = ThrowCard::CreateFollowCard(Pos(players[i].x, players[i].y), ThrowCard::monsterPos, tc.maxTime * CardThrowingTimeDecreaseRate);
	//						break;
	//					}
	//					else {
	//						g_Game.m_Logic.PlayCardLogic(*this, tc.cardID, i, true);
	//						EraseArr[up] = iter;
	//						up += 1;
	//					}
	//				}
	//			}
	//		}
	//		else if (tc.end_p.x == ThrowCard::monsterPos) {
	//			int r = rand() % 2;
	//			if (r == 0) {
	//				tc = ThrowCard::CreateFollowCard(Pos(ThrowCard::monsterPos, ThrowCard::monsterPos), ThrowCard::playerPos[0], tc.maxTime * CardThrowingTimeDecreaseRate);
	//			}
	//			else {
	//				g_Game.m_Logic.PlayCardLogic(*this, tc.cardID, 0);
	//				EraseArr[up] = iter;
	//				up += 1;
	//			}
	//		}
	//	}
	//	else if (tc.flowTime - tc.maxTime > ParingEpsilon) {
	//		for (int i = 0; i < playerCount; ++i) {
	//			if (tc.end_p.x == players[i].x && tc.end_p.y == players[i].y) {
	//				g_Game.m_Logic.ApplyDamageToPlayer(*this, 10, 0); // 데미지 10
	//			}

	//			if (tc.end_p.x == ThrowCard::playerPos[i]) {
	//				g_Game.m_Logic.PlayCardLogic(*this, tc.cardID, i, true);
	//			}
	//		}

	//		if (tc.end_p.x == ThrowCard::monsterPos) {
	//			g_Game.m_Logic.PlayCardLogic(*this, tc.cardID, 0);
	//		}

	//		//push
	//		EraseArr[up] = iter;
	//		up += 1;
	//	}
	//}

	//for (int i = 0; i < up; ++i) {
	//	throwCardList.erase(EraseArr[i]);
	//}
	//up = 0;

	//for (int i = 0; i < 3; ++i) {
	//	players[i].ParingMoment = false;
	//}
}

void PresentationState::RenderThrowingCards(HDC hdc) const
{
	/*for (auto iter = throwCardList.begin(); iter != throwCardList.end(); ++iter) {
		Pos startp, endp;

		startp = iter->start_p;
		if (iter->start_p.x == iter->playerPos[0]) startp = Pos(players[0].x, players[0].y);
		if (iter->start_p.x == iter->playerPos[1]) startp = Pos(players[1].x, players[1].y);
		if (iter->start_p.x == iter->playerPos[2]) startp = Pos(players[2].x, players[2].y);
		if (iter->start_p.x == iter->monsterPos) startp = Pos(boss.x, boss.y);

		endp = iter->end_p;
		if (iter->end_p.x == iter->playerPos[0]) endp = Pos(players[0].x, players[0].y);
		if (iter->end_p.x == iter->playerPos[1]) endp = Pos(players[1].x, players[1].y);
		if (iter->end_p.x == iter->playerPos[2]) endp = Pos(players[2].x, players[2].y);
		if (iter->end_p.x == iter->monsterPos) endp = Pos(boss.x, boss.y);

		float rate = (iter->flowTime / iter->maxTime);

		Pos rp;
		rp.x = startp.x + (endp.x - startp.x) * rate;
		rp.y = startp.y + (endp.y - startp.y) * rate;

		constexpr float margin = 30.0f;
		RECT cellRect;
		SetRect(&cellRect, (int)rp.x - margin, (int)rp.y - margin, (int)rp.x + margin, (int)rp.y + margin);
		FillRect(hdc, &cellRect, hBlueBrush);
	}*/
}

void PresentationState::CardThrow(ThrowCard tc)
{
	throwCardList.push_back(tc);
}

void Renderer::Render(HDC hdc, RECT rt, const GameState& state, const PresentationState& pState, const AssetManager& assets) {
	HDC mDC = CreateCompatibleDC(hdc);
	HBITMAP hBitmap = CreateCompatibleBitmap(hdc, rt.right, rt.bottom);
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(mDC, hBitmap);

	HDC imgDC = CreateCompatibleDC(mDC);

	DrawBackground(mDC, imgDC, state, pState, assets);

	if (pState.StartScreen) {
		DrawStartScreenUI(mDC, imgDC, pState, assets);
	}
	else {
		if (state.PvEMode) {
			DrawPVEScreen(mDC, imgDC, state, pState, assets);
		}
		else {
			DrawPvPScreen(mDC, imgDC, state, pState, assets);
		}
	}

	DrawHUD(mDC, state, pState);

	BitBlt(hdc, 0, 0, rt.right, rt.bottom, mDC, 0, 0, SRCCOPY);

	SelectObject(mDC, hOldBitmap);
	DeleteObject(hBitmap);
	DeleteDC(imgDC);
	DeleteDC(mDC);
}

void Renderer::DrawBackground(HDC hdc, HDC imgDC, const GameState& state, const PresentationState& pState, const AssetManager& assets) {
	HBITMAP hOldImg;

	if (pState.StartScreen) {
		hOldImg = (HBITMAP)SelectObject(imgDC, assets.hBitBackground);
		StretchBlt(hdc, 0, 0, 1180, 750, imgDC, 0, 0, assets.backWidth, assets.backHeight, SRCCOPY);
	}
	else {
		hOldImg = (HBITMAP)SelectObject(imgDC, assets.hBitTemp2);
		StretchBlt(hdc, pState.backgroundX, pState.backgroundY, 1185, 765, imgDC, 0, 0, assets.tempWidth2, assets.tempHeight2, SRCCOPY);
	}
	SelectObject(imgDC, hOldImg);
}

void Renderer::DrawStartScreenUI(HDC hdc, HDC imgDC, const PresentationState& pState, const AssetManager& assets) {
	HBITMAP hOldImg = (HBITMAP)SelectObject(imgDC, assets.hBitMenu);

	if (pState.start) StretchBlt(hdc, 900, 450, 250, 50, imgDC, 0, 0, assets.menuWidth, assets.menuHeight, NOTSRCCOPY);
	else StretchBlt(hdc, 900, 450, 250, 50, imgDC, 0, 0, assets.menuWidth, assets.menuHeight, SRCCOPY);

	SelectObject(imgDC, assets.hBitMenu2);
	if (pState.descrtion) StretchBlt(hdc, 900, 530, 250, 50, imgDC, 0, 0, assets.menuWidth, assets.menuHeight, NOTSRCCOPY);
	else StretchBlt(hdc, 900, 530, 250, 50, imgDC, 0, 0, assets.menuWidth, assets.menuHeight, SRCCOPY);

	SelectObject(imgDC, assets.hBitMenu3);
	if (pState.end) StretchBlt(hdc, 900, 610, 250, 50, imgDC, 0, 0, assets.menuWidth, assets.menuHeight, NOTSRCCOPY);
	else StretchBlt(hdc, 900, 610, 250, 50, imgDC, 0, 0, assets.menuWidth, assets.menuHeight, SRCCOPY);

	SelectObject(imgDC, hOldImg);
}

HPEN hRedPen = CreatePen(PS_SOLID, 3, RGB(255, 0, 0));
HBRUSH hBlueBrush = CreateSolidBrush(RGB(0, 0, 255));

void Renderer::DrawPvPScreen(HDC hdc, HDC imgDC, const GameState& state, const PresentationState& pState, const AssetManager& assets) {
	HBITMAP hOldImg;

	hOldImg = (HBITMAP)SelectObject(imgDC, assets.hBitstone);
	TransparentBlt(hdc, 100, -50, 785, 150, imgDC, 0, 0, assets.stoneWidth, assets.stoneHeight, RGB(0, 0, 0));
	SelectObject(imgDC, hOldImg);

	hOldImg = (HBITMAP)SelectObject(imgDC, assets.hBitShild);
	TransparentBlt(hdc, 500, 20, 50, 50, imgDC, 0, 0, assets.ShildWidth, assets.ShildHeight, RGB(255, 255, 255));

	SelectObject(imgDC, assets.hBitattack);
	TransparentBlt(hdc, 630, 20, 50, 50, imgDC, 0, 0, assets.attackWidth, assets.attackHeight, RGB(255, 255, 255));

	SelectObject(imgDC, assets.hBitReturn);
	TransparentBlt(hdc, 10, 10, 70, 70, imgDC, 0, 0, assets.returnWidth, assets.returnHeight, RGB(255, 255, 255));

	if (state.tempstop) SelectObject(imgDC, assets.hBitstop[1]);
	else SelectObject(imgDC, assets.hBitstop[0]);
	TransparentBlt(hdc, 160, 10, 70, 70, imgDC, 0, 0, assets.stopWidth, assets.stopHeight, RGB(255, 255, 255));

	if (pState.endon) SelectObject(imgDC, assets.endbutton[1]);
	else SelectObject(imgDC, assets.endbutton[0]);
	TransparentBlt(hdc, 1050, 600, 70, 55, imgDC, 0, 0, assets.endbuttonWidth, assets.endbuttonHeight, RGB(255, 255, 255));

	SelectObject(imgDC, hOldImg);

	//Draw Map
	constexpr float rateW = 0.4f;
	constexpr float rateH = 0.4f;
	float offsetX = PlayerW * rateW;
	float offsetY = PlayerH * rateH;
	// 5x5 맵
	SelectObject(hdc, hRedPen);
	for (int i = 0; i <= 5; ++i) {
		float lineOffset = (i - (5.0f / 2.0f)) * MapMoveMargin;
		MoveToEx(hdc, MapCenterX + lineOffset + offsetX, MapCenterY - (5.0f / 2.0f) * MapMoveMargin + offsetY, NULL);
		LineTo(hdc, MapCenterX + lineOffset + offsetX, MapCenterY + (5.0f / 2.0f) * MapMoveMargin + offsetY);
		MoveToEx(hdc, MapCenterX - (5.0f / 2.0f) * MapMoveMargin + offsetX, MapCenterY + lineOffset + offsetY, NULL);
		LineTo(hdc, MapCenterX + (5.0f / 2.0f) * MapMoveMargin + offsetX, MapCenterY + lineOffset + offsetY);
	}

	DrawHand(hdc, imgDC, state, pState, assets);
	DrawCharacters(hdc, imgDC, state, pState, assets);
	DrawEffects(hdc, imgDC, state, pState, assets);
}

void Renderer::DrawPVEScreen(HDC hdc, HDC imgDC, const GameState& state, const PresentationState& pState, const AssetManager& assets) {
	HBITMAP hOldImg;

	hOldImg = (HBITMAP)SelectObject(imgDC, assets.hBitstone);
	TransparentBlt(hdc, 100, -50, 785, 150, imgDC, 0, 0, assets.stoneWidth, assets.stoneHeight, RGB(0, 0, 0));
	SelectObject(imgDC, hOldImg);

	hOldImg = (HBITMAP)SelectObject(imgDC, assets.hBitShild);
	TransparentBlt(hdc, 500, 20, 50, 50, imgDC, 0, 0, assets.ShildWidth, assets.ShildHeight, RGB(255, 255, 255));
	if (!state.boss.death) {
		TransparentBlt(hdc, pState.boss.x, pState.boss.y - 100, 50, 50, imgDC, 0, 0, assets.ShildWidth, assets.ShildHeight, RGB(255, 255, 255));
	}

	SelectObject(imgDC, assets.hBitattack);
	TransparentBlt(hdc, 630, 20, 50, 50, imgDC, 0, 0, assets.attackWidth, assets.attackHeight, RGB(255, 255, 255));

	SelectObject(imgDC, assets.hBitReturn);
	TransparentBlt(hdc, 10, 10, 70, 70, imgDC, 0, 0, assets.returnWidth, assets.returnHeight, RGB(255, 255, 255));

	if (state.tempstop) SelectObject(imgDC, assets.hBitstop[1]);
	else SelectObject(imgDC, assets.hBitstop[0]);
	TransparentBlt(hdc, 160, 10, 70, 70, imgDC, 0, 0, assets.stopWidth, assets.stopHeight, RGB(255, 255, 255));

	if (pState.endon) SelectObject(imgDC, assets.endbutton[1]);
	else SelectObject(imgDC, assets.endbutton[0]);
	TransparentBlt(hdc, 1050, 600, 70, 55, imgDC, 0, 0, assets.endbuttonWidth, assets.endbuttonHeight, RGB(255, 255, 255));

	SelectObject(imgDC, hOldImg);

	// Map
	HBRUSH hBlueBrush = CreateSolidBrush(RGB(0, 0, 255));
	HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBlueBrush);
	constexpr float rateW = 0.4f;
	constexpr float rateH = 0.4f;
	float offsetX = PlayerW * rateW;
	float offsetY = PlayerH * rateH;
	// 데미지 영역 미리 알려주고
	for (int y = 0; y < 5; ++y) {
		for (int x = 0; x < 5; ++x) {
			if (state.mapData[y][x] == 3) {
				float left = MapCenterX + (x - (5.0f / 2.0f)) * MapMoveMargin + offsetX;
				float top = MapCenterY + (y - (5.0f / 2.0f)) * MapMoveMargin + offsetY;
				float right = left + MapMoveMargin;
				float bottom = top + MapMoveMargin;
				RECT cellRect;
				SetRect(&cellRect, (int)left, (int)top, (int)right, (int)bottom);
				FillRect(hdc, &cellRect, hBlueBrush);
			}
		}
	}
	SelectObject(hdc, hOldBrush);
	DeleteObject(hBlueBrush);
	// 5x5 맵
	SelectObject(hdc, hRedPen);
	for (int i = 0; i <= 5; ++i) {
		float lineOffset = (i - (5.0f / 2.0f)) * MapMoveMargin;
		MoveToEx(hdc, MapCenterX + lineOffset + offsetX, MapCenterY - (5.0f / 2.0f) * MapMoveMargin + offsetY, NULL);
		LineTo(hdc, MapCenterX + lineOffset + offsetX, MapCenterY + (5.0f / 2.0f) * MapMoveMargin + offsetY);
		MoveToEx(hdc, MapCenterX - (5.0f / 2.0f) * MapMoveMargin + offsetX, MapCenterY + lineOffset + offsetY, NULL);
		LineTo(hdc, MapCenterX + (5.0f / 2.0f) * MapMoveMargin + offsetX, MapCenterY + lineOffset + offsetY);
	}

	pState.RenderThrowingCards(hdc);

	DrawHand(hdc, imgDC, state, pState, assets);
	DrawCharacters(hdc, imgDC, state, pState, assets);
	DrawEffects(hdc, imgDC, state, pState, assets);
}

void Renderer::DrawHUD(HDC hdc, const GameState& state, const PresentationState& pState) {
	HFONT hFont, hOldFont;
	HBRUSH hBrush, oldBrush;
	HPEN hPen, oldPen;

	if (!pState.StartScreen) {
		if (g_myPlayerIndex < 0 || g_myPlayerIndex >= GameState::playerCount)
		{
			hFont = CreateFont(100, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
				DEFAULT_PITCH | FF_SWISS, L"Arial");
			hOldFont = (HFONT)SelectObject(hdc, hFont);
			SetBkMode(hdc, TRANSPARENT);
			SetTextColor(hdc, RGB(255, 255, 255));
			TextOut(hdc, 400, 350, L"매칭 중...", 6);
			SelectObject(hdc, hOldFont);
			DeleteObject(hFont);

			return;
		}

		const PlayerData& myPlayer = state.players[g_myPlayerIndex];
		const PlayerPresentation& myPState = pState.players[g_myPlayerIndex];

		if (pState.startstart) {
			hFont = CreateFont(200, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
				DEFAULT_PITCH | FF_SWISS, L"Arial");
			hOldFont = (HFONT)SelectObject(hdc, hFont);
			SetBkMode(hdc, TRANSPARENT);
			TextOut(hdc, 200, 300, pState.nowstagestr, lstrlen(pState.nowstagestr));
			SelectObject(hdc, hOldFont);
			DeleteObject(hFont);
		}
		if (pState.endend) {
			hFont = CreateFont(200, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
				DEFAULT_PITCH | FF_SWISS, L"Arial");
			hOldFont = (HFONT)SelectObject(hdc, hFont);
			SetBkMode(hdc, TRANSPARENT);
			TextOut(hdc, 60, 300, L"STAGE Clear", 11);
			SelectObject(hdc, hOldFont);
			DeleteObject(hFont);
		}
		if (pState.pdeath) {
			hFont = CreateFont(200, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
				DEFAULT_PITCH | FF_SWISS, L"Arial");
			hOldFont = (HFONT)SelectObject(hdc, hFont);
			SetBkMode(hdc, TRANSPARENT);
			TextOut(hdc, 80, 300, L"Game Over", 9);
			SelectObject(hdc, hOldFont);
			DeleteObject(hFont);
		}

		hFont = CreateFont(40, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
			DEFAULT_PITCH | FF_SWISS, L"Arial");
		hOldFont = (HFONT)SelectObject(hdc, hFont);
		SetBkMode(hdc, TRANSPARENT);

		// HP
		SetTextColor(hdc, RGB(0, 0, 0));
		TextOut(hdc, 300, 25, L"HP", 2);
		HPBar(hdc, 410, 50, myPlayer.hp);
		TCHAR tempBuffer[32];
		if (myPState.effect_anim_data.decresehp) {
			SetTextColor(hdc, RGB(200, 33, 33));
			wsprintf(tempBuffer, L"-%d", pState.dedamge);
			TextOut(hdc, 400, 18, tempBuffer, lstrlen(tempBuffer));
		}
		if (myPState.effect_anim_data.myheal) {
			SetTextColor(hdc, RGB(33, 200, 33));
			wsprintf(tempBuffer, L"+%d", pState.healenergy);
			TextOut(hdc, 400, 18, tempBuffer, lstrlen(tempBuffer));
		}

		// Defense
		SetTextColor(hdc, RGB(0, 33, 255));
		wsprintf(tempBuffer, L"%d", myPlayer.defence);
		TextOut(hdc, 580, 25, tempBuffer, lstrlen(tempBuffer));

		if (myPState.effect_anim_data.defUp) {
			SetTextColor(hdc, RGB(33, 33, 33));
			wsprintf(tempBuffer, L"+%d", pState.defenseup);
			TextOut(hdc, 620, 18, tempBuffer, lstrlen(tempBuffer));
		}
		if (myPState.effect_anim_data.defDown) {
			SetTextColor(hdc, RGB(33, 33, 33));
			wsprintf(tempBuffer, L"-%d", pState.defensedown);
			TextOut(hdc, 620, 18, tempBuffer, lstrlen(tempBuffer));
		}

		// Attack
		SetTextColor(hdc, RGB(255, 33, 0));
		wsprintf(tempBuffer, L"%d", myPlayer.attack);
		TextOut(hdc, 700, 25, tempBuffer, lstrlen(tempBuffer));

		if (myPState.effect_anim_data.powerUp) {
			SetTextColor(hdc, RGB(33, 33, 33));
			wsprintf(tempBuffer, L"+%d", pState.plusattack);
			TextOut(hdc, 740, 18, tempBuffer, lstrlen(tempBuffer));
		}
		if (myPState.effect_anim_data.powerDown) {
			SetTextColor(hdc, RGB(33, 33, 33));
			wsprintf(tempBuffer, L"-%d", pState.minusattack);
			TextOut(hdc, 740, 18, tempBuffer, lstrlen(tempBuffer));
		}
		SelectObject(hdc, hOldFont);
		DeleteObject(hFont);

		hBrush = CreateSolidBrush(RGB(0, 174, 251));
		oldBrush = (HBRUSH)SelectObject(hdc, hBrush);
		POINT point[5] = { {100,550}, {100 - 60,550 + 50}, {100 - 60 + 20,550 + 40 + 90}, {100 + 60 - 20, 550 + 40 + 90}, {100 + 60, 550 + 50} };
		Polygon(hdc, point, 5);
		SelectObject(hdc, oldBrush);
		DeleteObject(hBrush);

		hFont = CreateFont(40, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
		hOldFont = (HFONT)SelectObject(hdc, hFont);
		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, RGB(33, 33, 33));

		//wsprintf(tempBuffer, L"%d / %d", state.player->mana, state.player->maxMana);
		_stprintf_s(tempBuffer, sizeof(tempBuffer) / sizeof(TCHAR), L"%.2f", myPlayer.mana);
		TextOut(hdc, 65, 600, tempBuffer, lstrlen(tempBuffer));

		if (myPState.effect_anim_data.manaUp) {
			SetTextColor(hdc, RGB(150, 150, 150));
			wsprintf(tempBuffer, L"+%d", pState.healmana);
			TextOut(hdc, 145, 580, tempBuffer, lstrlen(tempBuffer));
		}
		if (myPState.effect_anim_data.manaDown) {
			SetTextColor(hdc, RGB(150, 150, 150));
			wsprintf(tempBuffer, L"-%d", pState.killmana);
			TextOut(hdc, 145, 610, tempBuffer, lstrlen(tempBuffer));
		}
		SelectObject(hdc, hOldFont);
		DeleteObject(hFont);

		if (state.PvEMode) {
			if (!state.boss.death) {
				HPBar(hdc, pState.boss.x + 75, pState.boss.y - 30, state.boss.hp);

				hFont = CreateFont(40, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
				hOldFont = (HFONT)SelectObject(hdc, hFont);
				SetBkMode(hdc, TRANSPARENT);
				SetTextColor(hdc, RGB(0, 33, 255));
				wsprintf(tempBuffer, L"%d", state.boss.defence);
				TextOut(hdc, pState.boss.x + 75, pState.boss.y - 95, tempBuffer, lstrlen(tempBuffer));

				if (pState.boss.defUp) {
					SetTextColor(hdc, RGB(150, 133, 133));
					wsprintf(tempBuffer, L"+%d", pState.enermydeff);
					TextOut(hdc, pState.boss.x + 115, pState.boss.y - 102, tempBuffer, lstrlen(tempBuffer));
				}
				if (pState.boss.defDown) {
					SetTextColor(hdc, RGB(150, 133, 133));
					wsprintf(tempBuffer, L"-%d", pState.enermydeffdown);
					TextOut(hdc, pState.boss.x + 115, pState.boss.y - 110, tempBuffer, lstrlen(tempBuffer));
				}
				// 적 HP 효과 (데미지/힐 텍스트)
				if (pState.dehp) {
					SetTextColor(hdc, RGB(200, 33, 33));
					wsprintf(tempBuffer, L"-%d", pState.damage);
					TextOut(hdc, pState.boss.x + 115, pState.boss.y - 100, tempBuffer, lstrlen(tempBuffer));
				}
				if (pState.boss.heal) {
					SetTextColor(hdc, RGB(33, 200, 33));
					wsprintf(tempBuffer, L"+%d", state.boss.healEnergy);
					TextOut(hdc, pState.boss.x + 115, pState.boss.y - 100, tempBuffer, lstrlen(tempBuffer));
				}
				SelectObject(hdc, hOldFont);
				DeleteObject(hFont);

				if (pState.enermytouch) {
					hPen = CreatePen(PS_SOLID, 3, RGB(255, 0, 0));
					oldPen = (HPEN)SelectObject(hdc, hPen);
					HBRUSH myBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
					HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, myBrush);
					Rectangle(hdc, pState.boss.x - 70, pState.boss.y - 60, pState.boss.x + 70, pState.boss.y + 60);
					SelectObject(hdc, oldBrush);
					SelectObject(hdc, oldPen);
					DeleteObject(hPen);
				}
			}
		}
	}
}

void Renderer::DrawHand(HDC hdc, HDC imgDC, const GameState& state, const PresentationState& pState, const AssetManager& assets) {
	for (int i = 0; i < 5; ++i) {
		const CardUI& card = pState.hand[i];
		if (card.on) {
			HBITMAP hOldCard = (HBITMAP)SelectObject(imgDC, assets.CARD[card.id]);
			if (card.select) {
				TransparentBlt(hdc, card.x - 85, card.y - 200, 168, 240, imgDC, 0, 0, assets.cardWidth, assets.cardHeight, RGB(100, 100, 100));
			}
			else {
				TransparentBlt(hdc, card.x - 70, card.y - 100, 140, 200, imgDC, 0, 0, assets.cardWidth, assets.cardHeight, RGB(100, 100, 100));
			}
			SelectObject(imgDC, hOldCard);
		}
	}
}

void Renderer::DrawCharacters(HDC hdc, HDC imgDC, const GameState& state, const PresentationState& pState, const AssetManager& assets) {
	HBITMAP hOldImg = NULL;

	for (int i = 0; i < GameState::playerCount; ++i) {
		const PlayerPresentation& p = pState.players[i];
		const PlayerData& pd = state.players[i];
		// 주인공
		if (pd.playerdeath) {
			hOldImg = (HBITMAP)SelectObject(imgDC, assets.hBitplayerdeath);
			TransparentBlt(hdc, p.x, p.y + 100, 170, 90, imgDC, 0, 0, assets.pdWidth, assets.pdHeight, RGB(100, 100, 100));
		}
		else {
			hOldImg = (HBITMAP)SelectObject(imgDC, assets.Cha[p.animCount]);
			TransparentBlt(hdc, p.x, p.y, PlayerW * 0.75f, PlayerH * 0.75f, imgDC, 0, 0, assets.chaWidth, assets.chaHeight, RGB(100, 100, 100));
		}
	}

	// 적
	if (state.PvEMode) {
		if (!state.boss.death) {
			if (state.boss.id == 0) {
				SelectObject(imgDC, assets.Boss1[pState.boss.animCount % 3]);
				TransparentBlt(hdc, pState.boss.x, pState.boss.y, 150, 100, imgDC, 0, 0, assets.enermy1Width, assets.enermy1Height, RGB(100, 100, 100));
			}
			else if (state.boss.id == 1) {
				SelectObject(imgDC, assets.Boss2[pState.boss.animCount % 3]);
				TransparentBlt(hdc, pState.boss.x, pState.boss.y, 150, 200, imgDC, 0, 0, assets.enermy2Width, assets.enermy2Height, RGB(100, 100, 100));
			}
			else if (state.boss.id == 2) {
				SelectObject(imgDC, assets.Boss3[pState.boss.animCount % 3]);
				TransparentBlt(hdc, pState.boss.x, pState.boss.y, 150, 200, imgDC, 0, 0, assets.enermy3Width, assets.enermy3Height, RGB(100, 100, 100));
			}
			else if (state.boss.id == 3) {
				if (state.boss.bossmode2) {
					SelectObject(imgDC, assets.Boss5[pState.boss.animCount % 3]);
				}
				else {
					SelectObject(imgDC, assets.Boss4[pState.boss.animCount % 3]);
				}
				TransparentBlt(hdc, pState.boss.x, pState.boss.y, 250, 300, imgDC, 0, 0, assets.enermy4Width, assets.enermy4Height, RGB(100, 100, 100));
			}
			else if (state.boss.id == 4) {
				SelectObject(imgDC, assets.Boss6[pState.boss.animCount % 3]);
				TransparentBlt(hdc, pState.boss.x, pState.boss.y, 200, 250, imgDC, 0, 0, assets.enermy5Width, assets.enermy5Height, RGB(100, 100, 100));
			}
			else if (state.boss.id == 5) { // 두더지
				if (state.boss.bossmode2) {
					if (state.boss.bossmode3) { // 공격 모션 
						int frameIndex = pState.boss.animCount / 3; // 0,0,0, 1,1,1, 2,2,2
						if (frameIndex > 2) frameIndex = 2;
						SelectObject(imgDC, assets.Boss9[frameIndex]);
					}
					else { // 땅 속
						SelectObject(imgDC, assets.Boss8[pState.boss.animCount % 3]);
					}
				}
				else { // 지상
					SelectObject(imgDC, assets.Boss7[pState.boss.animCount % 5]);
				}
				TransparentBlt(hdc, pState.boss.x, pState.boss.y, 400, 250, imgDC, 0, 0, assets.enermy6Width, assets.enermy6Height, RGB(255, 255, 255));
			}
			else if (state.boss.id == 6) { // 마트료시카
				if (state.boss.boss_statck == 0) SelectObject(imgDC, assets.Boss10[pState.boss.animCount % 3]);
				else if (state.boss.boss_statck == 1) SelectObject(imgDC, assets.Boss11[pState.boss.animCount % 3]);
				else if (state.boss.boss_statck == 2) SelectObject(imgDC, assets.Boss12[pState.boss.animCount % 3]);
				else SelectObject(imgDC, assets.Boss13[pState.boss.animCount % 3]);
				TransparentBlt(hdc, pState.boss.x, pState.boss.y, 250, 300, imgDC, 0, 0, assets.enermy7Width, assets.enermy7Height, RGB(255, 255, 255));
			}
			else if (state.boss.id == 7) { // 관
				if (state.boss.bossmode3) {
					SelectObject(imgDC, assets.Boss15[pState.boss.animCount % 3]);
				}
				else {
					SelectObject(imgDC, assets.Boss14[pState.boss.animCount % 3]);
				}
				TransparentBlt(hdc, pState.boss.x, pState.boss.y, 250, 300, imgDC, 0, 0, assets.enermy8Width, assets.enermy8Height, RGB(255, 255, 255));
			}
			else if (state.boss.id == 8) {
				if (state.boss.bossAwakening) {
					SelectObject(imgDC, assets.Boss17[pState.boss.animCount % 6]);
					TransparentBlt(hdc, pState.boss.x - 100, pState.boss.y - 50, 600, 400, imgDC, 0, 0, assets.enermy10Width, assets.enermy10Height, RGB(88, 88, 88));
				}
				else {
					SelectObject(imgDC, assets.Boss16[pState.boss.animCount % 6]);
					TransparentBlt(hdc, pState.boss.x, pState.boss.y, 275, 150, imgDC, 0, 0, assets.enermy9Width, assets.enermy9Height, RGB(88, 88, 88));
				}
			}
		}
	}

	if (hOldImg) {
		SelectObject(imgDC, hOldImg);
	}
}

void Renderer::DrawEffects(HDC hdc, HDC imgDC, const GameState& state, const PresentationState& pState, const AssetManager& assets) {
	HBITMAP hOldImg = NULL;
	// 폭발 이펙트
	if (pState.boomswitch && !state.boss.death) {
		hOldImg = (HBITMAP)SelectObject(imgDC, assets.hBitBoom[pState.boomcount]);
		TransparentBlt(hdc, pState.boss.x, pState.boss.y, 150, 150, imgDC, 0, 0, assets.BoomWidth, assets.BoomHeight, RGB(255, 255, 255));
	}
	// 정조준 이펙트
	if (pState.Sniper) {
		hOldImg = (HBITMAP)SelectObject(imgDC, assets.hBitSniper[pState.snipercount]);
		TransparentBlt(hdc, pState.boss.x - 50, pState.boss.y - 100, 200, 200, imgDC, 0, 0, assets.sniperWidth, assets.sniperHeight, RGB(255, 255, 255));
	}
	// 일격 이펙트p
	if (pState.One) {
		hOldImg = (HBITMAP)SelectObject(imgDC, assets.hBitOne[pState.Onecount]);
		TransparentBlt(hdc, pState.boss.x - 300, pState.boss.y - 100, 600, 200, imgDC, 0, 0, assets.OneWidth, assets.OneHeight, RGB(255, 255, 255));
	}
	// 기본 공격 이펙트
	if (pState.sword) {
		hOldImg = (HBITMAP)SelectObject(imgDC, assets.hBitsword[pState.swordcount]);
		TransparentBlt(hdc, pState.boss.x - 200, pState.boss.y - 180, 500, 500, imgDC, 0, 0, assets.swordWidth, assets.swordHeight, RGB(255, 255, 255));
	}

	//$Chang
	for (int i = 0; i < GameState::playerCount; ++i) {
		const PlayerPresentation& p = pState.players[i];
		// 방어 이펙트
		if (p.effect_anim_data.tekai) {
			hOldImg = (HBITMAP)SelectObject(imgDC, assets.hBittekai[p.effect_anim_data.tekaicount]);
			TransparentBlt(hdc, p.x - 150, p.y - 100, 300, 300, imgDC, 0, 0, assets.tekaiWidth, assets.tekaiHeight, RGB(255, 255, 255));
		}
		// 피격 이펙트
		/*if (p.effect_anim_data.hurt) {
			hOldImg = (HBITMAP)SelectObject(imgDC, assets.hBithurt[p.effect_anim_data.hurtcount]);
			TransparentBlt(hdc, p.x - 150, p.y - 100, 300, 300, imgDC, 0, 0, assets.hurtWidth, assets.hurtHeight, RGB(255, 255, 255));
		}*/
		// 굳건한 태세 이펙트
		if (p.effect_anim_data.holiShild) {
			hOldImg = (HBITMAP)SelectObject(imgDC, assets.hBitholiShild[p.effect_anim_data.holiShildcount]);
			TransparentBlt(hdc, p.x + 20, p.y - 50, 300, 300, imgDC, 0, 0, assets.holiShildWidth, assets.holiShildHeight, RGB(255, 255, 255));
		}
		// 피격 이펙트
		for (int i = 0; i < GameState::GRID_SIZE; i++) {
			for (int j = 0; j < GameState::GRID_SIZE; j++) {
				if (p.effect_anim_data.hurt && state.mapData[i][j] == 2)
				{
					constexpr float rateW = 0.4f;
					constexpr float rateH = 0.8f;
					constexpr float originOffsetX = MapCenterX - 2 * MapMoveMargin + PlayerW * rateW;
					constexpr float originOffsetY = MapCenterX - 2 * MapMoveMargin + PlayerH * rateH;

					int tileCenterX = originOffsetX + j * MapMoveMargin;
					int tileCenterY = originOffsetY + i * MapMoveMargin;

					// 피격 이펙트 (보스 공격 위치에 그리기)
					hOldImg = (HBITMAP)SelectObject(imgDC, assets.hBithurt[p.effect_anim_data.hurtcount]);
					TransparentBlt(hdc, tileCenterX - 150, tileCenterY - 150, 300, 300, imgDC, 0, 0, assets.hurtWidth, assets.hurtHeight, RGB(255, 255, 255));
				}
			}
		}
	}

	if (hOldImg) {
		SelectObject(imgDC, hOldImg);
	}
}

void Renderer::HPBar(HDC hDC, int x, int y, int hp) {
	Rectangle(hDC, x - 50, y - 10, x + 50, y + 10);
	HBRUSH hBrush, oldBrush;
	if (hp <= 20) hBrush = CreateSolidBrush(RGB(255, 33, 0));
	else if (hp <= 50) hBrush = CreateSolidBrush(RGB(255, 255, 0));
	else hBrush = CreateSolidBrush(RGB(33, 255, 0));

	oldBrush = (HBRUSH)SelectObject(hDC, hBrush);
	if (hp > 0) {
		Rectangle(hDC, x - 50, y - 10, x - 50 + hp, y + 10);
	}
	SelectObject(hDC, oldBrush);
	DeleteObject(hBrush);
}

void Renderer::ClearCross(HDC hDC, int x, int y, int r) {
	/*hPen = CreatePen(PS_SOLID, 3, RGB(255, 0, 0));
	oldPen = (HPEN)*/SelectObject(hDC, hRedPen);
	MoveToEx(hDC, x - r, y - r, NULL);
	LineTo(hDC, x + r, y + r);
	MoveToEx(hDC, x + r, y - r, NULL);
	LineTo(hDC, x - r, y + r);
	/*SelectObject(hDC, oldPen);
	DeleteObject(hPen);*/
}
