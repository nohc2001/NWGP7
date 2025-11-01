#pragma comment (lib, "msimg32.lib")
#pragma comment(lib, "winmm.lib")
#include <Windows.h>
#include <tchar.h>
#include <ctime>
#include <math.h>
#include "resource.h"
#include <mmsystem.h>

HINSTANCE g_hInst;
LPCTSTR lpszClass = L"Window Class Name";
LPCTSTR lpszWindowName = L"windows program";

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

struct Card {
	int id = 0; // 카드 종류
	int x;
	int y;
	bool select = false;
	bool drag = false;
	bool on = false;
};

struct Enermy {
	int x = 0;
	int y = 0;
	int defence = 0;
	int stage = 0;
	int hp = 100;
	bool death = false;

	int animCount = 0; 
	int attackTime = 0; 
	int healTime = 0; 
	int defUpTime = 0; 
	int defDownTime = 0; 
	int healEnergy = 0; 
	bool defUp = false; 
	bool defDown = false; 
	bool heal = false; 

	int animTimer = 0;
};

struct Pos {
	int x;
	int y;
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

struct Player {
	int x = 225;
	int y = 250; // chaX, chaY
	int animCount = 0;    // chaCount
	int hp = 100;
	int maxMana = 3;
	int mana = 3;
	int defence = 0;
	int attack = 0;
	Card hand[5];         // card[5]
	Pos pos;

	void Move(int dx, int dy) {
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

		x = MapCenterX + pos.x * MapMoveMargin;
		y = MapCenterY - pos.y * MapMoveMargin;
	}

	// 상태 플래그
	bool playerdeath = false;
	bool powerUp = false;
	bool powerDown = false;
	bool defUp = false;
	bool defDown = false;
	bool manaUp = false;
	bool manaDown = false;
	bool holiShild = false;
	bool cutting = false;
	bool onepunching = false;

	// 상태 타이머
	int attackTime = 0;
	int powerUpTime = 0;
	int powerDownTime = 0;
	int defUpTime = 0;
	int defDownTime = 0;
	int manaUpTime = 0;
	int manaDownTime = 0;
	int healTime = 0;
	int onepunchingcount = 0;
	int onepunchingcounttime = 0;
	int animTimer = 0;
};


class GameState {
public:
	// 게임 화면 상태
	bool StartScreen = true;
	bool MAIN = false;  // true 일떄 레이드 false일때 pvp
	bool descrtion = false;
	bool end = false;
	bool endon = false; // 턴 종료 버튼 활성화
	bool start = false; // 시작 버튼 활성화
	bool GameClear = false;

	// 스테이지별 플래그
	bool boss = false, bossPhase2 = false;
	int bossId = 0; // 보스 종류
	int boss_6_statck = 0;
	bool nodamage = false; // 보스 무적 패턴

	// 전투 상태
	Player player;
	Enermy enemy; // enermyy
	bool isPlayerTurn = true; // turn
	int patturn = 0;

	// 전투 효과/애니메이션 플래그
	bool tempstop = false; // 일시정지
	bool boomswitch = false, Sniper = false, One = false, sword = false, tekai = false, hurt = false;
	bool quake = false;
	bool droww = false; // 카드 뽑기
	bool trunendd = false; // 턴 종료
	bool dehp = false; // 적 피격
	bool decresehp = false; // 플레이어 피격
	bool myheal = false; // 플레이어 힐
	bool startstart = false, endend = false, pdeath = false; // 자막
	bool enermytouch = false;
	bool dontattackcard = false;

	// 전투 효과/애니메이션 타이머 및 값
	int boomcount = 0, snipercount = 0, Onecount = 0, swordcount = 0, tekaicount = 0, hurtcount = 0, holiShildcount = 0;
	int backgroundX = 0, backgroundY = 0, backgroundtime = 0, quaketime = 0;
	int damage = 0, dehptime = 0; // 적 피격
	int dedamge = 0, decresehptime = 0, defensedown = 0; // 플레이어 피격
	int healenergy = 0, plusattack = 0, minusattack = 0, defenseup = 0, enermydeff = 0;
	int enermydeffdown = 0, killmana = 0, healmana = 0;
	int startstarttime = 0, endendtime = 0, pdeathtime = 0;

	// TCHAR 버퍼 (UI 텍스트)
	TCHAR def[5], atk[5], Mana[10], nowstagestr[10], enerdef[4];
	TCHAR dam[3], eheal[3], dedam[3], mheal[3], powup[3], powdown[3], defu[3], defd[3];
	TCHAR enerdf[3], enerdfdown[3], mad[3], mau[3];
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
	BitmapArray<3> Enermy1, Enermy2, Enermy3, Enermy4, Enermy5, Enermy6;
	BitmapArray<5> Enermy7;
	BitmapArray<3> Enermy8, Enermy9, Enermy10, Enermy11, Enermy12, Enermy13, Enermy14, Enermy15;
	BitmapArray<6> Enermy16, Enermy17;

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
		Enermy1[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP25)); // 슬라임
		Enermy1[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP26)); // 슬라임2
		Enermy1[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP27)); // 슬라임3
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
		Enermy2[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP57)); // 기사
		Enermy2[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP58)); // 기사2
		Enermy2[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP59)); // 기사3
		Enermy3[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP60)); // 주술사
		Enermy3[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP61)); // 주술사2
		Enermy3[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP62)); // 주술사3
		Enermy4[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP66)); // 거북 
		Enermy4[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP67)); // 거북 2
		Enermy4[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP68)); // 거북 3
		Enermy5[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP63)); // 거북 괴물모드
		Enermy5[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP64)); // 거북 괴물모드2
		Enermy5[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP65)); // 거북 괴물모드3
		Enermy6[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP69)); // 개1
		Enermy6[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP70)); // 개2
		Enermy6[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP71)); // 개3
		Enermy7[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP72)); // 두더지1
		Enermy7[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP73)); // 두더지2
		Enermy7[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP74)); // 두더지3
		Enermy7[3] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP75)); // 두더지4
		Enermy7[4] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP76)); // 두더지5
		Enermy8[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP77)); // 두더지 땅속 1
		Enermy8[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP78)); // 두더지 땅속 2
		Enermy8[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP79)); // 두더지 땅속 3
		Enermy9[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP80)); // 두더지 공격 1
		Enermy9[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP81)); // 두더지 공격 2
		Enermy9[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP82)); // 두더지 공격 3
		Enermy10[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP83)); // 마트료시카 1-1
		Enermy10[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP84)); // 마트료시카 1-2
		Enermy10[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP85)); // 마트료시카 1-3
		Enermy11[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP86)); // 마트료시카 2-1
		Enermy11[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP87)); // 마트료시카 2-2
		Enermy11[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP88)); // 마트료시카 2-3
		Enermy12[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP89)); // 마트료시카 3-1
		Enermy12[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP90)); // 마트료시카 3-2
		Enermy12[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP91)); // 마트료시카 3-3
		Enermy13[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP92)); // 마트료시카 4-1
		Enermy13[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP93)); // 마트료시카 4-2
		Enermy13[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP94)); // 마트료시카 4-3
		Enermy14[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP95)); // 관 1
		Enermy14[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP96)); // 관 2
		Enermy14[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP97)); // 관 3
		Enermy15[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP98)); // 관 공격
		Enermy15[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP99)); // 관 공격
		Enermy15[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP100)); // 관 공격
		Enermy16[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP101)); // boss 1-1
		Enermy16[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP102)); // boss 1-2
		Enermy16[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP103)); // boss 1-3
		Enermy16[3] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP104)); // boss 1-4
		Enermy16[4] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP105)); // boss 1-5
		Enermy16[5] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP106)); // boss 1-6
		Enermy17[0] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP107)); // boss 2-1
		Enermy17[1] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP108)); // boss 2-2
		Enermy17[2] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP109)); // boss 2-3
		Enermy17[3] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP110)); // boss 2-4
		Enermy17[4] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP111)); // boss 2-5
		Enermy17[5] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP112)); // boss 2-6
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
		GetObject(Enermy1[0], sizeof(BITMAP), &bmp);
		enermy1Width = bmp.bmWidth;
		enermy1Height = bmp.bmHeight;
		// 2-1 기사
		GetObject(Enermy2[0], sizeof(BITMAP), &bmp);
		enermy2Width = bmp.bmWidth;
		enermy2Height = bmp.bmHeight;
		// 2-2 주술사
		GetObject(Enermy3[0], sizeof(BITMAP), &bmp);
		enermy3Width = bmp.bmWidth;
		enermy3Height = bmp.bmHeight;
		// 3-1 거북이
		GetObject(Enermy5[0], sizeof(BITMAP), &bmp);
		enermy4Width = bmp.bmWidth;
		enermy4Height = bmp.bmHeight;
		// 3-2 개
		GetObject(Enermy6[0], sizeof(BITMAP), &bmp);
		enermy5Width = bmp.bmWidth;
		enermy5Height = bmp.bmHeight;
		// 3-3 두더지
		GetObject(Enermy7[0], sizeof(BITMAP), &bmp);
		enermy6Width = bmp.bmWidth;
		enermy6Height = bmp.bmHeight;
		// 4-1 마트료시카
		GetObject(Enermy10[0], sizeof(BITMAP), &bmp);
		enermy7Width = bmp.bmWidth;
		enermy7Height = bmp.bmHeight;
		// 4-2 관
		GetObject(Enermy14[0], sizeof(BITMAP), &bmp);
		enermy8Width = bmp.bmWidth;
		enermy8Height = bmp.bmHeight;
		// boss 1
		GetObject(Enermy16[0], sizeof(BITMAP), &bmp);
		enermy9Width = bmp.bmWidth;
		enermy9Height = bmp.bmHeight;
		// boss 2
		GetObject(Enermy17[0], sizeof(BITMAP), &bmp);
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
		for (int i = 0; i < 3; ++i) DeleteObject(Enermy1[i]);
		DeleteObject(hBitShild);
		DeleteObject(playbutton);
		for (int i = 0; i < 2; ++i) DeleteObject(hBitstop[i]);
		DeleteObject(hBitattack);
		for (int i = 0; i < 3; ++i) DeleteObject(Cha[i]);
		for (int i = 0; i < 2; ++i) DeleteObject(endbutton[i]);
		for (int i = 0; i < 8; ++i) DeleteObject(hBitBoom[i]);
		DeleteObject(hBitplayerdeath);
		for (int i = 0; i < 3; ++i) DeleteObject(Enermy2[i]);
		for (int i = 0; i < 3; ++i) DeleteObject(Enermy3[i]);
		for (int i = 0; i < 3; ++i) DeleteObject(Enermy4[i]);
		for (int i = 0; i < 3; ++i) DeleteObject(Enermy5[i]);
		for (int i = 0; i < 3; ++i) DeleteObject(Enermy6[i]);
		for (int i = 0; i < 5; ++i) DeleteObject(Enermy7[i]);
		for (int i = 0; i < 3; ++i) DeleteObject(Enermy8[i]);
		for (int i = 0; i < 3; ++i) DeleteObject(Enermy9[i]);
		for (int i = 0; i < 3; ++i) DeleteObject(Enermy10[i]);
		for (int i = 0; i < 3; ++i) DeleteObject(Enermy11[i]);
		for (int i = 0; i < 3; ++i) DeleteObject(Enermy12[i]);
		for (int i = 0; i < 3; ++i) DeleteObject(Enermy13[i]);
		for (int i = 0; i < 3; ++i) DeleteObject(Enermy14[i]);
		for (int i = 0; i < 3; ++i) DeleteObject(Enermy15[i]);
		for (int i = 0; i < 6; ++i) DeleteObject(Enermy16[i]);
		for (int i = 0; i < 6; ++i) DeleteObject(Enermy17[i]);
		for (int i = 0; i < 5; ++i) DeleteObject(hBitSniper[i]);
		for (int i = 0; i < 5; ++i) DeleteObject(hBitOne[i]);
		for (int i = 0; i < 5; ++i) DeleteObject(hBitsword[i]);
		for (int i = 0; i < 5; ++i) DeleteObject(hBittekai[i]);
		for (int i = 0; i < 3; ++i) DeleteObject(hBithurt[i]);
		for (int i = 0; i < 5; ++i) DeleteObject(hBitholiShild[i]);
	}
};

class GameLogic {
public:
	void Initialize(GameState& state);

	void Update(GameState& state);

	void HandleChar(GameState& state, WPARAM wParam);
	void HandleMouseMove(GameState& state, int x, int y);
	void HandleLButtonDown(GameState& state, int x, int y, HWND hWnd);
	void HandleLButtonUp(GameState& state, int x, int y);

private:
	void UpdatePvE(GameState& state);
	void UpdatePvP(GameState& state);

	void CheckWinLossConditions(GameState& state);
	void UpdatePlayerTurn(GameState& state);
	void UpdateEnemyTurn(GameState& state);
	void ExecuteEnemyAI(GameState& state);
	void UpdateBuffsAndTimers(GameState& state);

	void StartBattle(GameState& state); // 맵에서 전투 시작
	void PlayCard(GameState& state, int cardIndex); // 카드 사용

	void ApplyDamageToPlayer(GameState& state, int damage);
	void ApplyDefenseToEnemy(GameState& state, int defense);
};

class Renderer {
public:
	void Render(HDC hdc, RECT rt, const GameState& state, const AssetManager& assets);

private:
	void DrawBackground(HDC hdc, HDC imgDC, const GameState& state, const AssetManager& assets);
	void DrawStartScreenUI(HDC hdc, HDC imgDC, const GameState& state, const AssetManager& assets);
	void DrawPvPScreen(HDC hdc, HDC imgDC, const GameState& state, const AssetManager& assets);
	void DrawPVEScreen(HDC hdc, HDC imgDC, const GameState& state, const AssetManager& assets);
	void DrawHUD(HDC hdc, const GameState& state); // 텍스트, HP바

	void DrawHand(HDC hdc, HDC imgDC, const GameState& state, const AssetManager& assets);
	void DrawCharacters(HDC hdc, HDC imgDC, const GameState& state, const AssetManager& assets);
	void DrawEffects(HDC hdc, HDC imgDC, const GameState& state, const AssetManager& assets);

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
private:
	HWND m_hWnd;
	AssetManager m_Assets;  // 자원 관리
	GameState m_State;    // 모든 데이터
	GameLogic m_Logic;    // 모든 규칙/로직
	Renderer m_Renderer;  // 모든 그리기
};

static Game g_Game;

class GameRoom {
public:
	//여기에 함수 추가

private:
	int RoomId;
	GameState State;
	GameLogic Logic;
	Renderer Renderer;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
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
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	while (GetMessage(&Message, 0, 0, 0)) {
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}

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

	m_Logic.Initialize(m_State);

	SetTimer(m_hWnd, 1, 100, NULL);
	PlaySound(TEXT("C:/Users/dong0/Desktop/윈도우 프로그래밍/기말/Project1/Project1/Lost Forest1.wav"), NULL, SND_FILENAME | SND_ASYNC);
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
	m_Logic.Update(m_State);

	InvalidateRect(m_hWnd, NULL, FALSE);
}

void Game::OnPaint(HDC hdc, PAINTSTRUCT& ps, RECT& rt)
{
	m_Renderer.Render(hdc, rt, m_State, m_Assets);
}

void Game::OnChar(WPARAM wParam)
{
	m_Logic.HandleChar(m_State, wParam);
}

void Game::OnMouseMove(int x, int y)
{
	m_Logic.HandleMouseMove(m_State, x, y);
}

void Game::OnLButtonDown(int x, int y)
{
	m_Logic.HandleLButtonDown(m_State, x, y, m_hWnd);
}

void Game::OnLButtonUp(int x, int y)
{
	m_Logic.HandleLButtonUp(m_State, x, y);
}

void Game::OnKeyDown(WPARAM wParam)
{
	constexpr float MoveMargin = 60;
	if (wParam == 'W' && inputdata.WPress == false) {
		m_State.player.Move(0, 1);
		inputdata.WPress = true;
	}
	else if (wParam == 'A' && inputdata.APress == false) {
		m_State.player.Move(-1, 0);
		inputdata.APress = true;
	}
	else if (wParam == 'S' && inputdata.SPress == false) {
		m_State.player.Move(0, -1);
		inputdata.SPress = true;
	}
	else if (wParam == 'D' && inputdata.DPress == false) {
		m_State.player.Move(1, 0);
		inputdata.DPress = true;
	}
	else if (wParam == VK_SPACE && inputdata.SpacePress == false) {
		inputdata.SpacePress = true;
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

void GameLogic::Initialize(GameState& state)
{
	srand((unsigned int)time(NULL));
	state.StartScreen = true;

	// 카드 위치/ID 초기화
	state.player.hand[0].x = 300;
	state.player.hand[0].y = 900;
	state.player.hand[1].x = 450;
	state.player.hand[1].y = 900;
	state.player.hand[2].x = 600;
	state.player.hand[2].y = 900;
	state.player.hand[3].x = 750;
	state.player.hand[3].y = 900;
	state.player.hand[4].x = 900;
	state.player.hand[4].y = 900;

	for (int i = 0; i < 5; ++i) {
		state.player.hand[i].id = rand() % 15;
	}

	// 주인공 좌표/스탯 초기화
	state.player.x = 225;
	state.player.y = 250;
	state.player.defence = 0;

}

void GameLogic::Update(GameState& state)
{
	if (state.StartScreen) {
		// 
	}
	else {
		if (state.MAIN) {
			UpdatePvE(state); // 전투
		}
		else {
			UpdatePvP(state); // 맵 이동
		}
	}
}

void GameLogic::HandleChar(GameState& state, WPARAM wParam)
{
	if (wParam == 'q') { // 종료
		PostQuitMessage(0);
	}
}

void GameLogic::HandleMouseMove(GameState& state, int x, int y)
{
	if (state.StartScreen) {
		if (x >= 900 && x <= 1150 && y >= 450 && y <= 500) state.start = true;
		else state.start = false;

		if (x >= 900 && x <= 1150 && y >= 530 && y <= 580) state.descrtion = true;
		else state.descrtion = false;

		if (x >= 900 && x <= 1150 && y >= 610 && y <= 660) state.end = true;
		else state.end = false;
	}
	else {
		if (state.MAIN) { // 레이드 
			bool isDragging = false;
			for (int i = 0; i < 5; ++i) if (state.player.hand[i].drag) isDragging = true;

			if (isDragging && state.isPlayerTurn) {
				if (x >= 900 - 70 && x <= 900 + 70 && y >= 400 - 60 && y <= 400 + 60) state.enermytouch = true;
				else state.enermytouch = false;
			}
			else state.enermytouch = false;

			if (x >= 1050 && x <= 1120 && y >= 600 && y <= 655 && state.isPlayerTurn) state.endon = true;
			else state.endon = false;

			for (int i = 0; i < 5; ++i) {
				if (x >= state.player.hand[i].x - 70 && x <= state.player.hand[i].x + 70 && y >= state.player.hand[i].y - 100 && y <= state.player.hand[i].y + 100 && state.isPlayerTurn) {
					state.player.hand[i].select = true;
				}
				else state.player.hand[i].select = false;

				if (state.player.hand[i].drag && state.isPlayerTurn) {
					state.player.hand[i].x = x;
					state.player.hand[i].y = y;
				}
			}
		}
		else { // pvp
			bool isDragging = false;
			for (int i = 0; i < 5; ++i) if (state.player.hand[i].drag) isDragging = true;

			if (isDragging && state.isPlayerTurn) {
				if (x >= 900 - 70 && x <= 900 + 70 && y >= 400 - 60 && y <= 400 + 60) state.enermytouch = true;
				else state.enermytouch = false;
			}
			else state.enermytouch = false;

			if (x >= 1050 && x <= 1120 && y >= 600 && y <= 655 && state.isPlayerTurn) state.endon = true;
			else state.endon = false;

			for (int i = 0; i < 5; ++i) {
				if (x >= state.player.hand[i].x - 70 && x <= state.player.hand[i].x + 70 && y >= state.player.hand[i].y - 100 && y <= state.player.hand[i].y + 100 && state.isPlayerTurn) {
					state.player.hand[i].select = true;
				}
				else state.player.hand[i].select = false;

				if (state.player.hand[i].drag && state.isPlayerTurn) {
					state.player.hand[i].x = x;
					state.player.hand[i].y = y;
				}
			}
		}
	}
}

void GameLogic::HandleLButtonDown(GameState& state, int x, int y, HWND hWnd)
{
	if (state.StartScreen) {
		if (x >= 900 && x <= 1150 && y >= 450 && y <= 500) { // pvp
			state.StartScreen = false;
			state.MAIN = false;
			StartBattle(state);
		}
		if (x >= 900 && x <= 1150 && y >= 530 && y <= 580) { // 레이드
			state.StartScreen = false;
			state.MAIN = true;
			StartBattle(state);
		}
		if (x >= 900 && x <= 1150 && y >= 610 && y <= 660) PostQuitMessage(0);
	}
	else {
		if (state.MAIN) { // 레이드
			if (x >= 175 && x <= 225 && y >= 15 && y <= 65) { //일시정지
				state.tempstop = !state.tempstop;
				if (state.tempstop) KillTimer(hWnd, 1);
				else SetTimer(hWnd, 1, 100, NULL);
			}
			if (x >= 1050 && x <= 1120 && y >= 600 && y <= 655 && state.isPlayerTurn) 
				state.player.mana = 0; // 턴 종료 버튼

			for (int i = 0; i < 5; ++i) {
				if (state.player.hand[i].on && x >= state.player.hand[i].x - 70 && x <= state.player.hand[i].x + 70 && y >= state.player.hand[i].y - 100 && y <= state.player.hand[i].y + 100 && state.isPlayerTurn && !state.player.onepunching) {
					state.player.hand[i].drag = true;
				}
				else state.player.hand[i].drag = false;
			}
			if (x >= 10 && x <= 70 && y >= 10 && y <= 70) {
				state.StartScreen = true;
			}
		}
		else { // pvp
			if (x >= 175 && x <= 225 && y >= 15 && y <= 65) { //일시정지
				state.tempstop = !state.tempstop;
				if (state.tempstop) KillTimer(hWnd, 1);
				else SetTimer(hWnd, 1, 100, NULL);
			}
			if (x >= 1050 && x <= 1120 && y >= 600 && y <= 655 && state.isPlayerTurn)
				state.player.mana = 0; // 턴 종료 버튼

			for (int i = 0; i < 5; ++i) {
				if (state.player.hand[i].on && x >= state.player.hand[i].x - 70 && x <= state.player.hand[i].x + 70 && y >= state.player.hand[i].y - 100 && y <= state.player.hand[i].y + 100 && state.isPlayerTurn && !state.player.onepunching) {
					state.player.hand[i].drag = true;
				}
				else state.player.hand[i].drag = false;
			}
			if (x >= 10 && x <= 70 && y >= 10 && y <= 70) {
				state.StartScreen = true; // 시작 화면으로
			}
		}
	}
}

void GameLogic::HandleLButtonUp(GameState& state, int x, int y)
{
	if (state.MAIN) { // 레이드
		for (int i = 0; i < 5; ++i) {
			if (state.player.hand[i].drag) {
				if (i == 0) { state.player.hand[i].x = 300; state.player.hand[i].y = 700; }
				else if (i == 1) { state.player.hand[i].x = 450; state.player.hand[i].y = 700; }
				else if (i == 2) { state.player.hand[i].x = 600; state.player.hand[i].y = 700; }
				else if (i == 3) { state.player.hand[i].x = 750; state.player.hand[i].y = 700; }
				else if (i == 4) { state.player.hand[i].x = 900; state.player.hand[i].y = 700; }

				if (state.enermytouch) { // 카드 발동
					PlayCard(state, i);
				}
				state.player.hand[i].drag = false;
			}
		}
	}
	else { // pvp
		for (int i = 0; i < 5; ++i) {
			if (state.player.hand[i].drag) {
				if (i == 0) { state.player.hand[i].x = 300; state.player.hand[i].y = 700; }
				else if (i == 1) { state.player.hand[i].x = 450; state.player.hand[i].y = 700; }
				else if (i == 2) { state.player.hand[i].x = 600; state.player.hand[i].y = 700; }
				else if (i == 3) { state.player.hand[i].x = 750; state.player.hand[i].y = 700; }
				else if (i == 4) { state.player.hand[i].x = 900; state.player.hand[i].y = 700; }

				if (state.enermytouch) { // 카드 발동
					PlayCard(state, i);
				}
				state.player.hand[i].drag = false;
			}
		}
	}
}

void GameLogic::UpdatePvE(GameState& state)
{
	CheckWinLossConditions(state);
	if (state.isPlayerTurn) UpdatePlayerTurn(state);
	else UpdateEnemyTurn(state);
	UpdateBuffsAndTimers(state);
}

void GameLogic::UpdatePvP(GameState& state)
{
	UpdateBuffsAndTimers(state);
}

void GameLogic::CheckWinLossConditions(GameState& state)
{
	if (state.enemy.hp <= 0 && !state.boss) {
		state.boomswitch = true;
	}
	if (state.boss && state.enemy.hp <= 0) { // boss 스테이지
		if (state.bossPhase2) {
			state.boomswitch = true;
		}
		else {
			state.enemy.heal = true;
			state.enemy.healEnergy = 100;
			state.bossPhase2 = true;
			state.enemy.defence = 20;
		}
	}
	if (state.player.hp <= 0) {
		state.player.playerdeath = true;
		state.pdeath = true;
	}
}

void GameLogic::UpdatePlayerTurn(GameState& state)
{
	if (state.player.mana == 0) {
		state.isPlayerTurn = false;
		state.trunendd = true; // 턴 종료 
	}
	if (state.player.onepunching && state.player.onepunchingcount == 1) { // 일격!!
		state.player.onepunchingcounttime++;
		if (state.player.onepunchingcounttime == 12) {
			state.player.attackTime = 1; // chaAttack = true;
			state.quake = true;
			state.dehp = true;
			state.player.onepunching = false;
			state.player.onepunchingcount = 0;
			state.player.onepunchingcounttime = 0;
		}
		state.One = true;
	}

	state.player.animTimer++;
	if (state.player.animTimer >= 1) { // 프레임 전환 속도 (숫자가 작을수록 빠름)
		state.player.animTimer = 0;
		state.player.animCount++;
		if (state.player.animCount >= 3) { // 플레이어 이미지 개수 (예: 4프레임)
			state.player.animCount = 0;
		}
	}

	if (!state.enemy.death) {
		state.enemy.animTimer++;
		if (state.enemy.animTimer >= 1) {
			state.enemy.animTimer = 0;
			state.enemy.animCount++;
			if (state.enemy.animCount >= 3) { // 적 이미지 개수
				state.enemy.animCount = 0;
			}
		}
	}
}

void GameLogic::UpdateEnemyTurn(GameState& state)
{
	if (state.enemy.hp <= 0) {
		state.enemy.death = true;
	}

	if (state.player.onepunching) {
		state.player.onepunchingcount = 1;
	}
	if (!state.dehp) {
		state.enemy.attackTime++;
	}

	if (state.enemy.attackTime == 22 && !state.enemy.death) { // 턴 시작
		state.isPlayerTurn = true;
		state.droww = true; // 카드 드로우
		state.enemy.attackTime = 0;
		state.player.mana = state.player.maxMana;
		ExecuteEnemyAI(state);
	}

	if (!state.enemy.death) {
		state.enemy.animTimer++;
		if (state.enemy.animTimer >= 1) {
			state.enemy.animTimer = 0;
			state.enemy.animCount++;
			if (state.enemy.animCount >= 3) { // 적 이미지 개수
				state.enemy.animCount = 0;
			}
		}
	}

	state.player.animTimer++;
	if (state.player.animTimer >= 1) { // 프레임 전환 속도 (숫자가 작을수록 빠름)
		state.player.animTimer = 0;
		state.player.animCount++;
		if (state.player.animCount >= 3) { // 플레이어 이미지 개수 (예: 4프레임)
			state.player.animCount = 0;
		}
	}
}

void GameLogic::ExecuteEnemyAI(GameState& state)
{
	// 보스 로직
	if (state.boss) { // 노 각성 공 20 방 20 / 각성 공 30 방 30
		if (state.bossPhase2) {
			ApplyDamageToPlayer(state, 30);
			ApplyDefenseToEnemy(state, 30);
		}
		else {
			ApplyDamageToPlayer(state, 20);
			ApplyDefenseToEnemy(state, 20);
		}
	}
}

void GameLogic::UpdateBuffsAndTimers(GameState& state)
{
	if (state.player.manaDown) { //행동력 다운 효과
		state.player.manaDownTime++;
		if (state.player.manaDownTime == 7) { 
			state.player.manaDownTime = 0; 
			state.player.manaDown = false; 
		}
	}
	if (state.player.manaUp) { //행동력 업 효과
		state.player.manaUpTime++;
		if (state.player.manaUpTime == 7) {
			state.player.manaUpTime = 0;
			state.player.manaUp = false;
		}
	}
	if (state.player.defUp) { // 플레이어 방어력 업
		state.player.defUpTime++;
		if (state.player.defUpTime == 7) {
			state.player.defUpTime = 0;
			state.player.defUp = false;
		}
	}
	if (state.player.defDown) { // 플레이어 방어력 다운
		state.player.defDownTime++;
		if (state.player.defDownTime == 7) {
			state.player.defDownTime = 0;
			state.player.defDown = false;
		}
	}
	if (state.enemy.defUp) { // 적 방어 업
		state.enemy.defUpTime++;
		if (state.enemy.defUpTime == 7) {
			state.enemy.defUpTime = 0;
			state.enemy.defUp = false;
		}
	}
	if (state.enemy.defDown) { // 적 방어 다운
		state.enemy.defDownTime++;
		if (state.enemy.defDownTime == 7) { 
			state.enemy.defDownTime = 0; 
			state.enemy.defDown = false; 
		}
	}
	if (state.player.powerUp) { // 주인공 공격력 추가 업
		state.player.powerUpTime++;
		if (state.player.powerUpTime == 7) { 
			state.player.powerUpTime = 0;
			state.player.powerUp = false; 
		}
	}
	if (state.player.powerDown) { // 주인공 공격력 추가 업
		state.player.powerDownTime++;
		if (state.player.powerDownTime == 7) {
			state.player.powerDownTime = 0;
			state.player.powerDown = false;
		}
	}

	if (state.decresehp) { // 주인공 hp 감소 효과
		state.decresehptime++;
		if (state.decresehptime == 7) {
			state.decresehptime = 0;
			state.decresehp = false;
		}
	}

	if (state.dehp) { // 적 hp바 감소 모션
		if (state.nodamage) {
			state.damage = 0;
		}
		if (state.enemy.defence != 0 && state.player.cutting == false) {
			int temp2 = state.enemy.defence;
			state.enemy.defence = state.enemy.defence - state.damage;
			if (state.enemy.defence < 0) {
				state.damage = -state.enemy.defence;
				state.enemy.defence = 0;
				state.enermydeffdown = temp2;
			}
			else {
				state.enermydeffdown = state.damage;
				state.damage = 0;
			}
			state.enemy.defDown = true;
			state.enemy.defDownTime = 0;
		}

		if (state.damage == 0) {
			state.dehptime = 0;
			state.dehp = false;
		}
		else {
			if (state.damage % 2 == 0) {
				state.dehptime += 2;
				state.enemy.hp -= 2;
			}
			else if (state.damage != 5 && state.damage % 5 == 0) {
				state.dehptime += 5;
				state.enemy.hp -= 5;
			}
			else {
				state.dehptime++;
				state.enemy.hp--;
			}
		}
		if (state.dehptime >= state.damage || state.enemy.hp <= 0) {
			state.dehptime = 0;
			state.dehp = false;
			state.player.cutting = false;
		}
	}

	if (state.myheal) { // hp 회복 효과
		state.player.healTime++;
		if (state.player.healTime == 7) {
			state.player.healTime = 0;
			state.myheal = false;
		}
	}

	if (state.enemy.heal) { // 적 hp바 회복 모션
		if (state.boss) {
			state.enemy.healTime += 20;
			if (state.enemy.hp < 100) { 
				state.enemy.hp += 20;
			}
		}
		else {
			state.enemy.healTime++;
			if (state.enemy.hp < 100) { 
				state.enemy.hp++;
			}
		}
		// (최대 HP 초과 방지)
		if (state.enemy.hp > 100) state.enemy.hp = 100;

		if (state.enemy.healTime >= state.enemy.healEnergy) { 
			state.enemy.healTime = 0;
			state.enemy.heal = false;
		}
	}

	if (state.startstart) { // 시작 자막
		state.startstarttime++;
		if (state.startstarttime == 15) {
			state.startstarttime = 0;
			state.startstart = false;
		}
	}

	if (state.endend) { // 스테이지 클 자막
		state.endendtime++;
		if (state.endendtime >= 15) {
			state.endendtime = 0;
			state.endend = false;
			state.StartScreen = true;
			state.GameClear = true; // 게임 클
		}
	}

	if (state.pdeath) { // 게임 오버 자막
		state.pdeathtime++;
		if (state.pdeathtime >= 15) {
			state.pdeathtime = 0;
			state.pdeath = false; 
			state.player.hp = 100;
			state.player.playerdeath = false;
			state.StartScreen = true;
			lstrcpy(state.nowstagestr, L"STGAE 1");
		}
	}

	if (state.droww) { // 카드 덱 뽑기
		for (int i = 0; i < 5; ++i) {
			state.player.hand[i].y -= 20;
			state.player.hand[i].on = true;
		}
		if (state.player.hand[4].y <= 700) { 
			state.player.hand[4].y = 700;
			state.droww = false;
		}
	}

	if (state.trunendd) { // 카드 덱이 넣기
		for (int i = 0; i < 5; ++i) {
			state.player.hand[i].y += 20;
		}
		if (state.player.hand[4].y >= 900) { 
			state.player.hand[4].y = 900; 
			state.trunendd = false;
		}
	}

	if (state.quake) { //지진
		state.quaketime++;
		if (state.backgroundtime == 0 || state.backgroundtime == 2) {
			state.backgroundX -= 15;
			state.backgroundY -= 15;
		}
		else if (state.backgroundtime == 1 || state.backgroundtime == 3) {
			state.backgroundX += 15;
			state.backgroundY += 15;
		}
		state.backgroundtime++;
		state.backgroundtime %= 4;
		if (state.quaketime >= 4) {
			state.quaketime = 0;
			state.quake = false;
			state.backgroundX = 0;
			state.backgroundY = 0;
		}
	}

	if (state.player.attackTime > 0) { // 주인공 공격 모션
		state.player.attackTime++; 
		if (state.player.attackTime == 2) { 
			state.player.x += 100;
		}
		else if (state.player.attackTime == 3) { 
			state.player.x -= 100;
		}
		else if (state.player.attackTime >= 4) { 
			state.player.attackTime = 0; 
		}
	}

	if (state.enemy.attackTime > 0 && state.enemy.attackTime < 22) { // 적 공격 모션
		if (state.enemy.attackTime == 2) { 
			state.enemy.x -= 100;
		}
		else if (state.enemy.attackTime == 3) { 
			state.enemy.x += 100;
		}
	}

	if (state.boomswitch) {
		state.boomcount++;
		if (state.boomcount >= 8) {
			state.boomswitch = false;
			state.boomcount = 0;
			// *** 스테이지 클리어 처리 ***
			state.endend = true; // 클리어 자막 시작
		}
	}
	if (state.Sniper) {
		state.snipercount++;
		if (state.snipercount >= 5) { state.Sniper = false; state.snipercount = 0; }
	}
	if (state.One) {
		state.Onecount++;
		if (state.Onecount >= 5) { state.One = false; state.Onecount = 0; }
	}
	if (state.sword) {
		state.swordcount++;
		if (state.swordcount >= 5) { state.sword = false; state.swordcount = 0; }
	}
	if (state.tekai) {
		state.tekaicount++;
		if (state.tekaicount >= 5) { state.tekai = false; state.tekaicount = 0; }
	}
	if (state.hurt) {
		state.hurtcount++;
		if (state.hurtcount >= 3) { state.hurt = false; state.hurtcount = 0; }
	}
	if (state.player.holiShild) { // 굳건한 태세는 계속 반복
		state.holiShildcount++;
		state.holiShildcount %= 5;
	}
	else {
		state.holiShildcount = 0; // 꺼지면 리셋
	}
}

void GameLogic::StartBattle(GameState& state)
{
	if (state.MAIN) { // 레이드
		state.startstart = true;
		state.droww = true;
		state.enemy.death = false;
		state.startstarttime = 0;
		state.player.mana = state.player.maxMana;
		state.boomswitch = false;
		state.boomcount = 0;
		state.isPlayerTurn = true;
		state.enemy.attackTime = 0;

		for (int i = 0; i < 5; ++i) {
			state.player.hand[i].id = rand() % 15;
			state.player.hand[i].x = 300 + 150 * i;
			state.player.hand[i].y = 900;
		}

		state.boss = true; // boss 활성화
		state.bossId = rand() % 9;
	}
	else { // PvP
		state.startstart = true;
		state.droww = true;
		state.enemy.death = false;
		state.startstarttime = 0;
		state.player.mana = state.player.maxMana;
		state.boomswitch = false;
		state.boomcount = 0;
		state.isPlayerTurn = true;
		state.enemy.attackTime = 0;

		for (int i = 0; i < 5; ++i) {
			state.player.hand[i].id = rand() % 15;
			state.player.hand[i].x = 300 + 150 * i;
			state.player.hand[i].y = 900;
		}
	}
}

void GameLogic::PlayCard(GameState& state, int i) {
	Card& card = state.player.hand[i];
	Player& player = state.player;

	if (!state.enermytouch || state.dehp || state.dontattackcard) {
		if (i == 0) { card.x = 300; card.y = 700; }
		else if (i == 1) { card.x = 450; card.y = 700; }
		else if (i == 2) { card.x = 600; card.y = 700; }
		else if (i == 3) { card.x = 750; card.y = 700; }
		else if (i == 4) { card.x = 900; card.y = 700; }
		card.drag = false;
		return;
	}

	// Card ID 0: 심장 뽑기 (Cost 2, Attack 70, Heal 10, Attack resets)
	if (card.id == 0 && player.mana >= 2) {
		player.attackTime = 1; 
		state.quake = true;
		if (player.attack == 0) state.damage = 70;
		else {
			state.minusattack = player.attack; 
			state.damage = player.attack + 70;
			player.attack = 0;
			player.powerDown = true; 
			player.powerDownTime = 0; 
		}
		player.mana -= 2;
		state.killmana = 2; 
		player.manaDown = true;
		player.manaDownTime = 0; 
		state.healenergy = 10; 
		state.myheal = true; 
		state.dehp = true; 
		if (player.hp < 90) player.hp += 10;
		else player.hp = 100;
		card.on = false;
		card.id = rand() % 15;
		state.sword = true;
	}
	// Card ID 1: 심판 (Cost 3, Attack 90, Attack resets)
	else if (card.id == 1 && player.mana >= 3) {
		player.attackTime = 1; state.quake = true;
		if (player.attack == 0) state.damage = 90;
		else {
			state.minusattack = player.attack; state.damage = player.attack + 90; player.attack = 0; player.powerDown = true; player.powerDownTime = 0;
		}
		player.mana -= 3; state.killmana = 3; player.manaDown = true; player.manaDownTime = 0;
		state.dehp = true;
		card.on = false; card.id = rand() % 15; state.sword = true;
	}
	// Card ID 2: 강타 (Cost 2, Attack 60, Attack resets)
	else if (card.id == 2 && player.mana >= 2) {
		player.attackTime = 1; state.quake = true;
		player.mana -= 2; state.killmana = 2; player.manaDown = true; player.manaDownTime = 0;
		if (player.attack == 0) state.damage = 60;
		else {
			state.minusattack = player.attack; state.damage = player.attack + 60; player.attack = 0; player.powerDown = true; player.powerDownTime = 0;
		}
		state.dehp = true;
		card.on = false; card.id = rand() % 15; state.sword = true;
	}
	// Card ID 3: 자세잡기 (Cost 1, Defense +3, Mana +1)
	else if (card.id == 3 && player.mana >= 1) {
		player.mana -= 1; 
		state.killmana = 1; player.manaDown = true; player.manaDownTime = 0;
		player.mana++; 
		state.healmana = 1; 
		player.manaUp = true; 
		player.manaUpTime = 0; 
		player.defUp = true; 
		player.defUpTime = 0; 
		state.defenseup = 3; 
		player.defence += 3;
		card.on = false; card.id = rand() % 15; state.tekai = true; 
	}
	// Card ID 4: 돌진 (Cost 2, Attack 40, Defense +10, Attack resets)
	else if (card.id == 4 && player.mana >= 2) {
		player.attackTime = 1; state.quake = true;
		player.mana -= 2; state.killmana = 2; player.manaDown = true; player.manaDownTime = 0;
		if (player.attack == 0) state.damage = 40;
		else {
			state.minusattack = player.attack; state.damage = player.attack + 40; player.attack = 0; player.powerDown = true; player.powerDownTime = 0;
		}
		state.dehp = true;
		player.defUp = true; player.defUpTime = 0; state.defenseup = 10; player.defence += 10;
		card.on = false; card.id = rand() % 15; state.sword = true;
	}
	// Card ID 5: 대검휘두르기 (Cost 1, Attack 50, Attack resets)
	else if (card.id == 5 && player.mana >= 1) {
		player.attackTime = 1; state.quake = true;
		player.mana--; state.killmana = 1; player.manaDown = true; player.manaDownTime = 0;
		if (player.attack == 0) state.damage = 50;
		else {
			state.minusattack = player.attack; state.damage = player.attack + 50; player.attack = 0; player.powerDown = true; player.powerDownTime = 0;
		}
		state.dehp = true;
		card.on = false; card.id = rand() % 15; state.sword = true;
	}
	// Card ID 6: 바리게이트 (Cost 2, Defense x2)
	else if (card.id == 6 && player.mana >= 2) {
		player.mana -= 2; state.killmana = 2; player.manaDown = true; player.manaDownTime = 0;
		player.defUp = true; player.defUpTime = 0; state.defenseup = player.defence; player.defence *= 2;
		card.on = false; card.id = rand() % 15; state.tekai = true;
	}
	// Card ID 7: 방패 밀쳐내기 (Cost 1, Attack = Defense, Attack resets)
	else if (card.id == 7 && player.mana >= 1) {
		player.mana--; state.killmana = 1; player.manaDown = true; player.manaDownTime = 0;
		player.attackTime = 1; state.quake = true;
		if (player.attack == 0) state.damage = player.defence;
		else {
			state.minusattack = player.attack; state.damage = player.attack + player.defence; player.attack = 0; player.powerDown = true; player.powerDownTime = 0;
		}
		state.dehp = true;
		card.on = false; card.id = rand() % 15; state.sword = true;
	}
	// Card ID 8: 굳건한 태세 (Cost 2, Block next attack)
	else if (card.id == 8 && player.mana >= 2) {
		player.mana -= 2; state.killmana = 2; player.manaDown = true; player.manaDownTime = 0;
		player.holiShild = true; 
		card.on = false; card.id = rand() % 15;
	}
	// Card ID 9: 방패 세우기 (Cost 1, Defense +5)
	else if (card.id == 9 && player.mana >= 1) {
		player.mana -= 1; state.killmana = 1; player.manaDown = true; player.manaDownTime = 0;
		player.defUp = true; player.defUpTime = 0; state.defenseup = 5; player.defence += 5;
		card.on = false; card.id = rand() % 15; state.tekai = true;
	}
	// Card ID 10: 절단 (Cost 2, Attack 40 (ignores defense), Attack resets)
	else if (card.id == 10 && player.mana >= 2) {
		player.mana -= 2; state.killmana = 2; player.manaDown = true; player.manaDownTime = 0;
		player.attackTime = 1; state.quake = true;
		if (player.attack == 0) state.damage = 40;
		else {
			state.minusattack = player.attack; state.damage = player.attack + 40; player.attack = 0; player.powerDown = true; player.powerDownTime = 0;
		}
		state.dehp = true;
		player.cutting = true; 
		card.on = false; card.id = rand() % 15; state.sword = true;
	}
	// Card ID 11: 일격 (Cost 3, Attack 140 next turn, Attack resets)
	else if (card.id == 11 && player.mana >= 3) {
		player.mana -= 3; state.killmana = 3; player.manaDown = true; player.manaDownTime = 0;
		if (player.attack == 0) state.damage = 140; 
		else {
			state.minusattack = player.attack; state.damage = player.attack + 140; player.attack = 0; player.powerDown = true; player.powerDownTime = 0;
		}
		player.onepunching = true; 
		player.onepunchingcount = 0; 
		player.onepunchingcounttime = 0; 
		card.on = false; card.id = rand() % 15;
	}
	// Card ID 12: 고속 이동 (Cost 2, Defense +5, Mana +1)
	else if (card.id == 12 && player.mana >= 2) {
		player.mana -= 2; state.killmana = 2; player.manaDown = true; player.manaDownTime = 0;
		player.mana++; state.healmana = 1; player.manaUp = true; player.manaUpTime = 0;
		player.defUp = true; player.defUpTime = 0; state.defenseup = 5; player.defence += 5;
		card.on = false; card.id = rand() % 15; state.tekai = true;
	}
	// Card ID 13: 혈류 (Cost 1, HP -10, Attack 60, Attack resets)
	else if (card.id == 13 && player.mana >= 1) {
		player.mana--; state.killmana = 1; player.manaDown = true; player.manaDownTime = 0;
		player.attackTime = 1; state.quake = true;
		if (player.attack == 0) state.damage = 60;
		else {
			state.minusattack = player.attack; state.damage = player.attack + 60; player.attack = 0; player.powerDown = true; player.powerDownTime = 0;
		}
		state.dehp = true;
		state.dedamge = 10;
		player.hp -= 10;
		state.decresehp = true;
		card.on = false; card.id = rand() % 15; state.sword = true;
	}
	// Card ID 14: 정조준 (Cost 1, Next Attack +20)
	else if (card.id == 14 && player.mana >= 1) {
		player.mana--; state.killmana = 1; player.manaDown = true; player.manaDownTime = 0;
		player.powerUp = true; player.powerUpTime = 0; state.plusattack = 20; player.attack += 20;
		card.on = false; card.id = rand() % 15; state.Sniper = true; 
	}
	else {
		if (i == 0) { card.x = 300; card.y = 700; }
		else if (i == 1) { card.x = 450; card.y = 700; }
		else if (i == 2) { card.x = 600; card.y = 700; }
		else if (i == 3) { card.x = 750; card.y = 700; }
		else if (i == 4) { card.x = 900; card.y = 700; }
	}

	card.drag = false;
	state.enermytouch = false;
}

void GameLogic::ApplyDamageToPlayer(GameState& state, int damage) {
	state.enemy.attackTime = 1;
	state.hurt = true;

	if (state.player.holiShild) {
		damage = 0;
		state.player.holiShild = false;
	}

	state.defensedown = damage;
	int temp = state.player.defence;
	int damageDealt = state.player.defence - damage;

	state.player.defence = (damageDealt > 0) ? damageDealt : 0;

	if (damageDealt < 0) {
		state.dedamge = -damageDealt;
		state.player.hp -= state.dedamge;
		state.decresehp = true;
		if (temp > 0) state.player.defDown = true;
	}
	else {
		state.player.defDown = true;
	}
}

void GameLogic::ApplyDefenseToEnemy(GameState& state, int defense) {
	state.enemy.defUp = true;
	state.enemy.defUpTime = 0; // 타이머 리셋
	state.enermydeff = defense;
	state.enemy.defence += defense;
}

void Renderer::Render(HDC hdc, RECT rt, const GameState& state, const AssetManager& assets) {
	HDC mDC = CreateCompatibleDC(hdc);
	HBITMAP hBitmap = CreateCompatibleBitmap(hdc, rt.right, rt.bottom);
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(mDC, hBitmap);

	HDC imgDC = CreateCompatibleDC(mDC);

	DrawBackground(mDC, imgDC, state, assets);

	if (state.StartScreen) {
		DrawStartScreenUI(mDC, imgDC, state, assets);
	}
	else if (state.MAIN) {
		DrawPVEScreen(mDC, imgDC, state, assets);
	}
	else {
		DrawPvPScreen(mDC, imgDC, state, assets);
	}

	DrawHUD(mDC, state);

	BitBlt(hdc, 0, 0, rt.right, rt.bottom, mDC, 0, 0, SRCCOPY);

	SelectObject(mDC, hOldBitmap);
	DeleteObject(hBitmap);
	DeleteDC(imgDC);
	DeleteDC(mDC);
}

void Renderer::DrawBackground(HDC hdc, HDC imgDC, const GameState& state, const AssetManager& assets) {
	HBITMAP hOldImg;

	if (state.StartScreen) {
		hOldImg = (HBITMAP)SelectObject(imgDC, assets.hBitBackground);
		StretchBlt(hdc, 0, 0, 1180, 750, imgDC, 0, 0, assets.backWidth, assets.backHeight, SRCCOPY);
	}
	else {
		hOldImg = (HBITMAP)SelectObject(imgDC, assets.hBitTemp2);
		StretchBlt(hdc, state.backgroundX, state.backgroundY, 1185, 765, imgDC, 0, 0, assets.tempWidth2, assets.tempHeight2, SRCCOPY);
	}
	SelectObject(imgDC, hOldImg);
}

void Renderer::DrawStartScreenUI(HDC hdc, HDC imgDC, const GameState& state, const AssetManager& assets) {
	HBITMAP hOldImg = (HBITMAP)SelectObject(imgDC, assets.hBitMenu);

	if (state.start) StretchBlt(hdc, 900, 450, 250, 50, imgDC, 0, 0, assets.menuWidth, assets.menuHeight, NOTSRCCOPY);
	else StretchBlt(hdc, 900, 450, 250, 50, imgDC, 0, 0, assets.menuWidth, assets.menuHeight, SRCCOPY);

	SelectObject(imgDC, assets.hBitMenu2);
	if (state.descrtion) StretchBlt(hdc, 900, 530, 250, 50, imgDC, 0, 0, assets.menuWidth, assets.menuHeight, NOTSRCCOPY);
	else StretchBlt(hdc, 900, 530, 250, 50, imgDC, 0, 0, assets.menuWidth, assets.menuHeight, SRCCOPY);

	SelectObject(imgDC, assets.hBitMenu3);
	if (state.end) StretchBlt(hdc, 900, 610, 250, 50, imgDC, 0, 0, assets.menuWidth, assets.menuHeight, NOTSRCCOPY);
	else StretchBlt(hdc, 900, 610, 250, 50, imgDC, 0, 0, assets.menuWidth, assets.menuHeight, SRCCOPY);

	SelectObject(imgDC, hOldImg);
}

HPEN hRedPen = CreatePen(PS_SOLID, 3, RGB(255, 0, 0));

void Renderer::DrawPvPScreen(HDC hdc, HDC imgDC, const GameState& state, const AssetManager& assets) {
	HBITMAP hOldImg;

	hOldImg = (HBITMAP)SelectObject(imgDC, assets.hBitstone);
	TransparentBlt(hdc, 100, -50, 785, 150, imgDC, 0, 0, assets.stoneWidth, assets.stoneHeight, RGB(0, 0, 0));
	SelectObject(imgDC, hOldImg);

	hOldImg = (HBITMAP)SelectObject(imgDC, assets.hBitShild);
	TransparentBlt(hdc, 500, 20, 50, 50, imgDC, 0, 0, assets.ShildWidth, assets.ShildHeight, RGB(255, 255, 255));
	if (!state.enemy.death) {
		TransparentBlt(hdc, state.enemy.x, state.enemy.y - 100, 50, 50, imgDC, 0, 0, assets.ShildWidth, assets.ShildHeight, RGB(255, 255, 255));
	}

	SelectObject(imgDC, assets.hBitattack);
	TransparentBlt(hdc, 630, 20, 50, 50, imgDC, 0, 0, assets.attackWidth, assets.attackHeight, RGB(255, 255, 255));

	SelectObject(imgDC, assets.hBitReturn);
	TransparentBlt(hdc, 10, 10, 70, 70, imgDC, 0, 0, assets.returnWidth, assets.returnHeight, RGB(255, 255, 255));

	if (state.tempstop) SelectObject(imgDC, assets.hBitstop[1]);
	else SelectObject(imgDC, assets.hBitstop[0]);
	TransparentBlt(hdc, 160, 10, 70, 70, imgDC, 0, 0, assets.stopWidth, assets.stopHeight, RGB(255, 255, 255));

	if (state.endon) SelectObject(imgDC, assets.endbutton[1]);
	else SelectObject(imgDC, assets.endbutton[0]);
	TransparentBlt(hdc, 1050, 600, 70, 55, imgDC, 0, 0, assets.endbuttonWidth, assets.endbuttonHeight, RGB(255, 255, 255));

	SelectObject(imgDC, hOldImg);

	//Draw Map
	SelectObject(hdc, hRedPen);
	constexpr float rateW = 0.4f;
	constexpr float rateH = 0.8f;
	for (int i = -2; i < 3; ++i) {
		MoveToEx(hdc, MapCenterX + MapMoveMargin * i + PlayerW * rateW, MapCenterX - 2 * MapMoveMargin + PlayerH * rateH, NULL);
		LineTo(hdc, MapCenterX + MapMoveMargin * i + PlayerW * rateW, MapCenterX + 2 * MapMoveMargin + PlayerH * rateH);
		MoveToEx(hdc, MapCenterX - 2 * MapMoveMargin + PlayerW * rateW, MapCenterX + MapMoveMargin * i + PlayerH * rateH, NULL);
		LineTo(hdc, MapCenterX + 2 * MapMoveMargin + PlayerW * rateW, MapCenterX + MapMoveMargin * i + PlayerH * rateH);
	}

	DrawHand(hdc, imgDC, state, assets);
	DrawCharacters(hdc, imgDC, state, assets);
	DrawEffects(hdc, imgDC, state, assets);
}



void Renderer::DrawPVEScreen(HDC hdc, HDC imgDC, const GameState& state, const AssetManager& assets) {
	HBITMAP hOldImg;

	hOldImg = (HBITMAP)SelectObject(imgDC, assets.hBitstone);
	TransparentBlt(hdc, 100, -50, 785, 150, imgDC, 0, 0, assets.stoneWidth, assets.stoneHeight, RGB(0, 0, 0));
	SelectObject(imgDC, hOldImg);

	hOldImg = (HBITMAP)SelectObject(imgDC, assets.hBitShild);
	TransparentBlt(hdc, 500, 20, 50, 50, imgDC, 0, 0, assets.ShildWidth, assets.ShildHeight, RGB(255, 255, 255));
	if (!state.enemy.death) {
		TransparentBlt(hdc, state.enemy.x, state.enemy.y - 100, 50, 50, imgDC, 0, 0, assets.ShildWidth, assets.ShildHeight, RGB(255, 255, 255));
	}

	SelectObject(imgDC, assets.hBitattack);
	TransparentBlt(hdc, 630, 20, 50, 50, imgDC, 0, 0, assets.attackWidth, assets.attackHeight, RGB(255, 255, 255));

	SelectObject(imgDC, assets.hBitReturn);
	TransparentBlt(hdc, 10, 10, 70, 70, imgDC, 0, 0, assets.returnWidth, assets.returnHeight, RGB(255, 255, 255));

	if (state.tempstop) SelectObject(imgDC, assets.hBitstop[1]);
	else SelectObject(imgDC, assets.hBitstop[0]);
	TransparentBlt(hdc, 160, 10, 70, 70, imgDC, 0, 0, assets.stopWidth, assets.stopHeight, RGB(255, 255, 255));

	if (state.endon) SelectObject(imgDC, assets.endbutton[1]);
	else SelectObject(imgDC, assets.endbutton[0]);
	TransparentBlt(hdc, 1050, 600, 70, 55, imgDC, 0, 0, assets.endbuttonWidth, assets.endbuttonHeight, RGB(255, 255, 255));

	SelectObject(imgDC, hOldImg);

	//Draw Map
	SelectObject(hdc, hRedPen);
	constexpr float rateW = 0.4f;
	constexpr float rateH = 0.8f;
	for (int i = -2; i < 3; ++i) {
		MoveToEx(hdc, MapCenterX + MapMoveMargin * i + PlayerW * rateW, MapCenterX - 2 * MapMoveMargin + PlayerH * rateH, NULL);
		LineTo(hdc, MapCenterX + MapMoveMargin * i + PlayerW * rateW, MapCenterX + 2 * MapMoveMargin + PlayerH * rateH);
		MoveToEx(hdc, MapCenterX - 2 * MapMoveMargin + PlayerW * rateW, MapCenterX + MapMoveMargin * i + PlayerH * rateH, NULL);
		LineTo(hdc, MapCenterX + 2 * MapMoveMargin + PlayerW * rateW, MapCenterX + MapMoveMargin * i + PlayerH * rateH);
	}

	DrawHand(hdc, imgDC, state, assets);
	DrawCharacters(hdc, imgDC, state, assets);
	DrawEffects(hdc, imgDC, state, assets);
}

void Renderer::DrawHUD(HDC hdc, const GameState& state) {
	HFONT hFont, hOldFont;
	HBRUSH hBrush, oldBrush;
	HPEN hPen, oldPen;

	if (state.MAIN) { // pvp 화면
		if (state.startstart) {
			hFont = CreateFont(200, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
				DEFAULT_PITCH | FF_SWISS, L"Arial");
			hOldFont = (HFONT)SelectObject(hdc, hFont);
			SetBkMode(hdc, TRANSPARENT);
			TextOut(hdc, 200, 300, state.nowstagestr, lstrlen(state.nowstagestr));
			SelectObject(hdc, hOldFont);
			DeleteObject(hFont);
		}
		if (state.endend) {
			hFont = CreateFont(200, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
				DEFAULT_PITCH | FF_SWISS, L"Arial");
			hOldFont = (HFONT)SelectObject(hdc, hFont);
			SetBkMode(hdc, TRANSPARENT);
			TextOut(hdc, 60, 300, L"STAGE Clear", 11);
			SelectObject(hdc, hOldFont);
			DeleteObject(hFont);
		}
		if (state.pdeath) {
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
		HPBar(hdc, 410, 50, state.player.hp);
		TCHAR tempBuffer[32]; 
		if (state.decresehp) { // HP 감소 효과
			SetTextColor(hdc, RGB(200, 33, 33));
			wsprintf(tempBuffer, L"-%d", state.dedamge);
			TextOut(hdc, 400, 18, tempBuffer, lstrlen(tempBuffer));
		}
		if (state.myheal) { // HP 회복 효과
			SetTextColor(hdc, RGB(33, 200, 33));
			wsprintf(tempBuffer, L"+%d", state.healenergy);
			TextOut(hdc, 400, 18, tempBuffer, lstrlen(tempBuffer));
		}

		// Defense
		SetTextColor(hdc, RGB(0, 33, 255));
		wsprintf(tempBuffer, L"%d", state.player.defence);
		TextOut(hdc, 580, 25, tempBuffer, lstrlen(tempBuffer));

		if (state.player.defUp) { 
			SetTextColor(hdc, RGB(33, 33, 33));
			wsprintf(tempBuffer, L"+%d", state.defenseup);
			TextOut(hdc, 620, 18, tempBuffer, lstrlen(tempBuffer));
		}
		if (state.player.defDown) {
			SetTextColor(hdc, RGB(33, 33, 33));
			wsprintf(tempBuffer, L"-%d", state.defensedown);
			TextOut(hdc, 620, 18, tempBuffer, lstrlen(tempBuffer));
		}

		// Attack
		SetTextColor(hdc, RGB(255, 33, 0));
		wsprintf(tempBuffer, L"%d", state.player.attack);
		TextOut(hdc, 700, 25, tempBuffer, lstrlen(tempBuffer));

		if (state.player.powerUp) {
			SetTextColor(hdc, RGB(33, 33, 33));
			wsprintf(tempBuffer, L"+%d", state.plusattack);
			TextOut(hdc, 740, 18, tempBuffer, lstrlen(tempBuffer));
		}
		if (state.player.powerDown) { 
			SetTextColor(hdc, RGB(33, 33, 33));
			wsprintf(tempBuffer, L"-%d", state.minusattack);
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

		wsprintf(tempBuffer, L"%d / %d", state.player.mana, state.player.maxMana);
		TextOut(hdc, 65, 600, tempBuffer, lstrlen(tempBuffer));

		if (state.player.manaUp) {
			SetTextColor(hdc, RGB(150, 150, 150));
			wsprintf(tempBuffer, L"+%d", state.healmana);
			TextOut(hdc, 145, 580, tempBuffer, lstrlen(tempBuffer));
		}
		if (state.player.manaDown) {
			SetTextColor(hdc, RGB(150, 150, 150));
			wsprintf(tempBuffer, L"-%d", state.killmana);
			TextOut(hdc, 145, 610, tempBuffer, lstrlen(tempBuffer));
		}
		SelectObject(hdc, hOldFont);
		DeleteObject(hFont);

		if (!state.enemy.death) {
			HPBar(hdc, state.enemy.x + 75, state.enemy.y - 30, state.enemy.hp);

			hFont = CreateFont(40, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
			hOldFont = (HFONT)SelectObject(hdc, hFont);
			SetBkMode(hdc, TRANSPARENT);
			SetTextColor(hdc, RGB(0, 33, 255));
			wsprintf(tempBuffer, L"%d", state.enemy.defence);
			TextOut(hdc, state.enemy.x + 75, state.enemy.y - 95, tempBuffer, lstrlen(tempBuffer));

			if (state.enemy.defUp) {
				SetTextColor(hdc, RGB(150, 133, 133));
				wsprintf(tempBuffer, L"+%d", state.enermydeff);
				TextOut(hdc, state.enemy.x + 115, state.enemy.y - 102, tempBuffer, lstrlen(tempBuffer));
			}
			if (state.enemy.defDown) { 
				SetTextColor(hdc, RGB(150, 133, 133));
				wsprintf(tempBuffer, L"-%d", state.enermydeffdown);
				TextOut(hdc, state.enemy.x + 115, state.enemy.y - 110, tempBuffer, lstrlen(tempBuffer));
			}
			// 적 HP 효과 (데미지/힐 텍스트)
			if (state.dehp) { 
				SetTextColor(hdc, RGB(200, 33, 33));
				wsprintf(tempBuffer, L"-%d", state.damage);
				TextOut(hdc, state.enemy.x + 115, state.enemy.y - 100, tempBuffer, lstrlen(tempBuffer)); 
			}
			if (state.enemy.heal) {
				SetTextColor(hdc, RGB(33, 200, 33));
				wsprintf(tempBuffer, L"+%d", state.enemy.healEnergy);
				TextOut(hdc, state.enemy.x + 115, state.enemy.y - 100, tempBuffer, lstrlen(tempBuffer));
			}
			SelectObject(hdc, hOldFont);
			DeleteObject(hFont);

			if (state.enermytouch) {
				hPen = CreatePen(PS_SOLID, 3, RGB(255, 0, 0));
				oldPen = (HPEN)SelectObject(hdc, hPen);
				HBRUSH myBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
				HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, myBrush);
				Rectangle(hdc, state.enemy.x - 70, state.enemy.y - 60, state.enemy.x + 70, state.enemy.y + 60);
				SelectObject(hdc, oldBrush);
				SelectObject(hdc, oldPen);
				DeleteObject(hPen);
			}
		}
	}
	else if (!state.StartScreen) { // 레이드 화면
		if (state.startstart) {
			hFont = CreateFont(200, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
				DEFAULT_PITCH | FF_SWISS, L"Arial");
			hOldFont = (HFONT)SelectObject(hdc, hFont);
			SetBkMode(hdc, TRANSPARENT);
			TextOut(hdc, 200, 300, state.nowstagestr, lstrlen(state.nowstagestr));
			SelectObject(hdc, hOldFont);
			DeleteObject(hFont);
		}
		if (state.endend) {
			hFont = CreateFont(200, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
				DEFAULT_PITCH | FF_SWISS, L"Arial");
			hOldFont = (HFONT)SelectObject(hdc, hFont);
			SetBkMode(hdc, TRANSPARENT);
			TextOut(hdc, 60, 300, L"STAGE Clear", 11);
			SelectObject(hdc, hOldFont);
			DeleteObject(hFont);
		}
		if (state.pdeath) {
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
		HPBar(hdc, 410, 50, state.player.hp);
		TCHAR tempBuffer[32];
		if (state.decresehp) { // HP 감소 효과
			SetTextColor(hdc, RGB(200, 33, 33));
			wsprintf(tempBuffer, L"-%d", state.dedamge);
			TextOut(hdc, 400, 18, tempBuffer, lstrlen(tempBuffer));
		}
		if (state.myheal) { // HP 회복 효과
			SetTextColor(hdc, RGB(33, 200, 33));
			wsprintf(tempBuffer, L"+%d", state.healenergy);
			TextOut(hdc, 400, 18, tempBuffer, lstrlen(tempBuffer));
		}

		// Defense
		SetTextColor(hdc, RGB(0, 33, 255));
		wsprintf(tempBuffer, L"%d", state.player.defence);
		TextOut(hdc, 580, 25, tempBuffer, lstrlen(tempBuffer));

		if (state.player.defUp) {
			SetTextColor(hdc, RGB(33, 33, 33));
			wsprintf(tempBuffer, L"+%d", state.defenseup);
			TextOut(hdc, 620, 18, tempBuffer, lstrlen(tempBuffer));
		}
		if (state.player.defDown) {
			SetTextColor(hdc, RGB(33, 33, 33));
			wsprintf(tempBuffer, L"-%d", state.defensedown);
			TextOut(hdc, 620, 18, tempBuffer, lstrlen(tempBuffer));
		}

		// Attack
		SetTextColor(hdc, RGB(255, 33, 0));
		wsprintf(tempBuffer, L"%d", state.player.attack);
		TextOut(hdc, 700, 25, tempBuffer, lstrlen(tempBuffer));

		if (state.player.powerUp) {
			SetTextColor(hdc, RGB(33, 33, 33));
			wsprintf(tempBuffer, L"+%d", state.plusattack);
			TextOut(hdc, 740, 18, tempBuffer, lstrlen(tempBuffer));
		}
		if (state.player.powerDown) {
			SetTextColor(hdc, RGB(33, 33, 33));
			wsprintf(tempBuffer, L"-%d", state.minusattack);
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

		wsprintf(tempBuffer, L"%d / %d", state.player.mana, state.player.maxMana);
		TextOut(hdc, 65, 600, tempBuffer, lstrlen(tempBuffer));

		if (state.player.manaUp) {
			SetTextColor(hdc, RGB(150, 150, 150));
			wsprintf(tempBuffer, L"+%d", state.healmana);
			TextOut(hdc, 145, 580, tempBuffer, lstrlen(tempBuffer));
		}
		if (state.player.manaDown) {
			SetTextColor(hdc, RGB(150, 150, 150));
			wsprintf(tempBuffer, L"-%d", state.killmana);
			TextOut(hdc, 145, 610, tempBuffer, lstrlen(tempBuffer));
		}
		SelectObject(hdc, hOldFont);
		DeleteObject(hFont);

		if (!state.enemy.death) {
			HPBar(hdc, state.enemy.x + 75, state.enemy.y - 30, state.enemy.hp);

			hFont = CreateFont(40, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
			hOldFont = (HFONT)SelectObject(hdc, hFont);
			SetBkMode(hdc, TRANSPARENT);
			SetTextColor(hdc, RGB(0, 33, 255));
			wsprintf(tempBuffer, L"%d", state.enemy.defence);
			TextOut(hdc, state.enemy.x + 75, state.enemy.y - 95, tempBuffer, lstrlen(tempBuffer));

			if (state.enemy.defUp) {
				SetTextColor(hdc, RGB(150, 133, 133));
				wsprintf(tempBuffer, L"+%d", state.enermydeff);
				TextOut(hdc, state.enemy.x + 115, state.enemy.y - 102, tempBuffer, lstrlen(tempBuffer));
			}
			if (state.enemy.defDown) {
				SetTextColor(hdc, RGB(150, 133, 133));
				wsprintf(tempBuffer, L"-%d", state.enermydeffdown);
				TextOut(hdc, state.enemy.x + 115, state.enemy.y - 110, tempBuffer, lstrlen(tempBuffer));
			}
			// 적 HP 효과 (데미지/힐 텍스트)
			if (state.dehp) {
				SetTextColor(hdc, RGB(200, 33, 33));
				wsprintf(tempBuffer, L"-%d", state.damage);
				TextOut(hdc, state.enemy.x + 115, state.enemy.y - 100, tempBuffer, lstrlen(tempBuffer));
			}
			if (state.enemy.heal) {
				SetTextColor(hdc, RGB(33, 200, 33));
				wsprintf(tempBuffer, L"+%d", state.enemy.healEnergy);
				TextOut(hdc, state.enemy.x + 115, state.enemy.y - 100, tempBuffer, lstrlen(tempBuffer));
			}
			SelectObject(hdc, hOldFont);
			DeleteObject(hFont);

			if (state.enermytouch) {
				hPen = CreatePen(PS_SOLID, 3, RGB(255, 0, 0));
				oldPen = (HPEN)SelectObject(hdc, hPen);
				HBRUSH myBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
				HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, myBrush);
				Rectangle(hdc, state.enemy.x - 70, state.enemy.y - 60, state.enemy.x + 70, state.enemy.y + 60);
				SelectObject(hdc, oldBrush);
				SelectObject(hdc, oldPen);
				DeleteObject(hPen);
			}
		}
	}
}

void Renderer::DrawHand(HDC hdc, HDC imgDC, const GameState& state, const AssetManager& assets) {
	for (int i = 0; i < 5; ++i) {
		const Card& card = state.player.hand[i];
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

void Renderer::DrawCharacters(HDC hdc, HDC imgDC, const GameState& state, const AssetManager& assets) {
	HBITMAP hOldImg = NULL;
	// 주인공
	if (state.player.playerdeath) {
		hOldImg = (HBITMAP)SelectObject(imgDC, assets.hBitplayerdeath);
		TransparentBlt(hdc, state.player.x, state.player.y + 100, 170, 90, imgDC, 0, 0, assets.pdWidth, assets.pdHeight, RGB(100, 100, 100));
	}
	else {
		hOldImg = (HBITMAP)SelectObject(imgDC, assets.Cha[state.player.animCount]);
		TransparentBlt(hdc, state.player.x, state.player.y, PlayerW * 0.75f, PlayerH * 0.75f, imgDC, 0, 0, assets.chaWidth, assets.chaHeight, RGB(100, 100, 100));
	}

	// 적
	if (!state.enemy.death) {
		if (state.bossId == 0) { // 슬라임 보스
			SelectObject(imgDC, assets.Enermy1[state.enemy.animCount % 3]);
			TransparentBlt(hdc, state.enemy.x, state.enemy.y, 150, 100, imgDC, 0, 0, assets.enermy1Width, assets.enermy1Height, RGB(100, 100, 100));
		}
		else if (state.bossId == 1) { // 기사 보스
			SelectObject(imgDC, assets.Enermy2[state.enemy.animCount % 3]);
			TransparentBlt(hdc, state.enemy.x, state.enemy.y, 150, 200, imgDC, 0, 0, assets.enermy2Width, assets.enermy2Height, RGB(100, 100, 100));
		}
		else if (state.bossId == 2) { // 마녀 보스
			SelectObject(imgDC, assets.Enermy3[state.enemy.animCount % 3]);
			TransparentBlt(hdc, state.enemy.x, state.enemy.y, 150, 200, imgDC, 0, 0, assets.enermy3Width, assets.enermy3Height, RGB(100, 100, 100));
		}
		else if (state.bossId == 3) { // 
			if (state.bossPhase2) {
				SelectObject(imgDC, assets.Enermy5[state.enemy.animCount % 3]);
			}
			else {
				SelectObject(imgDC, assets.Enermy4[state.enemy.animCount % 3]);
			}
			TransparentBlt(hdc, state.enemy.x, state.enemy.y, 250, 300, imgDC, 0, 0, assets.enermy4Width, assets.enermy4Height, RGB(100, 100, 100));
		}
		else if (state.bossId == 4) {
			SelectObject(imgDC, assets.Enermy6[state.enemy.animCount % 3]);
			TransparentBlt(hdc, state.enemy.x, state.enemy.y, 200, 250, imgDC, 0, 0, assets.enermy5Width, assets.enermy5Height, RGB(100, 100, 100));
		}
		else if (state.bossId == 5) { // 두더지
			if (state.nodamage) {
				if (state.bossPhase2) { // 공격 모션 
					int frameIndex = state.enemy.animCount / 3; // 0,0,0, 1,1,1, 2,2,2
					if (frameIndex > 2) frameIndex = 2;
					SelectObject(imgDC, assets.Enermy9[frameIndex]);
				}
				else { // 땅 속
					SelectObject(imgDC, assets.Enermy8[state.enemy.animCount % 3]);
				}
			}
			else { // 지상
				SelectObject(imgDC, assets.Enermy7[state.enemy.animCount % 5]);
			}
			TransparentBlt(hdc, state.enemy.x, state.enemy.y, 400, 250, imgDC, 0, 0, assets.enermy6Width, assets.enermy6Height, RGB(255, 255, 255));
		}
		else if (state.bossId == 6) { // 마트료시카
			if (state.boss_6_statck == 0) SelectObject(imgDC, assets.Enermy10[state.enemy.animCount % 3]);
			else if (state.boss_6_statck == 1) SelectObject(imgDC, assets.Enermy11[state.enemy.animCount % 3]);
			else if (state.boss_6_statck == 2) SelectObject(imgDC, assets.Enermy12[state.enemy.animCount % 3]);
			else SelectObject(imgDC, assets.Enermy13[state.enemy.animCount % 3]);
			TransparentBlt(hdc, state.enemy.x, state.enemy.y, 250, 300, imgDC, 0, 0, assets.enermy7Width, assets.enermy7Height, RGB(255, 255, 255));
		}
		else if (state.bossId == 7) { // 관
			if (state.bossPhase2) {
				SelectObject(imgDC, assets.Enermy15[state.enemy.animCount % 3]);
			}
			else {
				SelectObject(imgDC, assets.Enermy14[state.enemy.animCount % 3]);
			}
			TransparentBlt(hdc, state.enemy.x, state.enemy.y, 250, 300, imgDC, 0, 0, assets.enermy8Width, assets.enermy8Height, RGB(255, 255, 255));
		}
		else if (state.bossId == 8) {
			if (state.bossPhase2) {
				SelectObject(imgDC, assets.Enermy17[state.enemy.animCount % 6]);
				TransparentBlt(hdc, state.enemy.x - 100, state.enemy.y - 50, 600, 400, imgDC, 0, 0, assets.enermy10Width, assets.enermy10Height, RGB(88, 88, 88));
			}
			else {
				SelectObject(imgDC, assets.Enermy16[state.enemy.animCount % 6]);
				TransparentBlt(hdc, state.enemy.x, state.enemy.y, 275, 150, imgDC, 0, 0, assets.enermy9Width, assets.enermy9Height, RGB(88, 88, 88));
			}
		}
	}

	if (hOldImg) {
		SelectObject(imgDC, hOldImg);
	}
}

void Renderer::DrawEffects(HDC hdc, HDC imgDC, const GameState& state, const AssetManager& assets) {
	HBITMAP hOldImg = NULL;
	// 폭발 이펙트
	if (state.boomswitch && !state.enemy.death) {
		hOldImg = (HBITMAP)SelectObject(imgDC, assets.hBitBoom[state.boomcount]);
		TransparentBlt(hdc, state.enemy.x, state.enemy.y, 150, 150, imgDC, 0, 0, assets.BoomWidth, assets.BoomHeight, RGB(255, 255, 255));
	}
	// 정조준 이펙트
	if (state.Sniper) {
		hOldImg = (HBITMAP)SelectObject(imgDC, assets.hBitSniper[state.snipercount]); 
		TransparentBlt(hdc, state.enemy.x - 50, state.enemy.y - 100, 200, 200, imgDC, 0, 0, assets.sniperWidth, assets.sniperHeight, RGB(255, 255, 255));
	}
	// 일격 이펙트
	if (state.One) {
		hOldImg = (HBITMAP)SelectObject(imgDC, assets.hBitOne[state.Onecount]);
		TransparentBlt(hdc, state.enemy.x - 300, state.enemy.y - 100, 600, 200, imgDC, 0, 0, assets.OneWidth, assets.OneHeight, RGB(255, 255, 255));
	}
	// 기본 공격 이펙트
	if (state.sword) {
		hOldImg = (HBITMAP)SelectObject(imgDC, assets.hBitsword[state.swordcount]); 
		TransparentBlt(hdc, state.enemy.x - 200, state.enemy.y - 180, 500, 500, imgDC, 0, 0, assets.swordWidth, assets.swordHeight, RGB(255, 255, 255));
	}
	// 방어 이펙트
	if (state.tekai) {
		hOldImg = (HBITMAP)SelectObject(imgDC, assets.hBittekai[state.tekaicount]); 
		TransparentBlt(hdc, state.player.x - 150, state.player.y - 100, 300, 300, imgDC, 0, 0, assets.tekaiWidth, assets.tekaiHeight, RGB(255, 255, 255));
	}
	// 피격 이펙트
	if (state.hurt) {
		hOldImg = (HBITMAP)SelectObject(imgDC, assets.hBithurt[state.hurtcount]);
		TransparentBlt(hdc, state.player.x - 150, state.player.y - 100, 300, 300, imgDC, 0, 0, assets.hurtWidth, assets.hurtHeight, RGB(255, 255, 255));
	}
	// 굳건한 태세 이펙트
	if (state.player.holiShild) {
		hOldImg = (HBITMAP)SelectObject(imgDC, assets.hBitholiShild[state.holiShildcount]); 
		TransparentBlt(hdc, state.player.x + 20, state.player.y - 50, 300, 300, imgDC, 0, 0, assets.holiShildWidth, assets.holiShildHeight, RGB(255, 255, 255));
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