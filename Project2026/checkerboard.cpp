#include <graphics.h>
#include <conio.h>
#include <cwchar>
#include <cstdlib>
#include <windows.h>
#include "enemy.h"
#include "timeaccount.h"

namespace {
    const int GRID_ROWS = 9;
    const int GRID_COLS = 17;
    const int CELL_SIZE = 80;
    const int BOARD_LEFT = 0;
    const int BOARD_TOP = 0;

    const int INFO_LEFT = 1080;
    const int INFO_TOP = 420;
    const int INFO_RIGHT = 1280;
    const int INFO_BOTTOM = 720;

    const int BULLET_RADIUS = 8;
    const float PLAYER_BULLET_SPEED = 8.5f;
    const float ENEMY_BULLET_SPEED = 8.5f;
    const DWORD FIRE_COOLDOWN_MS = 140;
    const int KILL_RANGE_CELLS = 1;
    const int FIRST_BOARD_WIN_SCORE = 100;
    const int SECOND_BOARD_WIN_SCORE = 150;
    const int PLAYER_INIT_HP = 300;

    enum AppState {
        STATE_MENU = 0,
        STATE_RUNNING,
        STATE_PAUSED
    };

    struct Button {
        RECT rect;
        const wchar_t* label;
        COLORREF color;
    };

    struct Bullet {
        float x;
        float y;
        float vx;
        float vy;
        Bullet* next;
    };

    struct Player {
        int row;
        int col;
        int hp;
        wchar_t lastKey[16];
    };

    struct ScoreBoard {//开启计分
        int score;
        int hits;
    };

    bool HitButton(const Button& button, int x, int y) {
        return x >= button.rect.left && x <= button.rect.right &&
               y >= button.rect.top && y <= button.rect.bottom;
    }

    int ClampMin(int value, int low) {
        return (value < low) ? low : value;
    }

    int ScoreForEnemyType(int enemyType) {//两种敌人击中得不同的分
        if (enemyType == ENEMY_TYPE_GREEN) {
            return 15;
        }
        if (enemyType == ENEMY_TYPE_YELLOW) {
            return 20;
        }
        return 0;
    }

    void DrawPanels() {

    }

    void DrawButtonPlaceholders() {
        setfillcolor(TRANSPARENT);
        solidroundrect(0, 0, 115, 45, 10, 10);
        solidroundrect(0, 55, 115, 100, 10, 10);
        solidroundrect(0, 110, 115, 155, 10, 10);

        settextcolor(WHITE);
        setbkmode(TRANSPARENT);
        settextstyle(22, 0, L"Fixedsys");
        outtextxy(10, 23, L"Start");
        outtextxy(10, 73, L"Pause");
        outtextxy(10, 123, L"Quit");
    }

    void DrawGrid() {
        setlinecolor(RGB(110, 138, 178));
        for (int r = 0; r < GRID_ROWS; ++r) {
            for (int c = 0; c < GRID_COLS; ++c) {
                const int left = BOARD_LEFT + c * CELL_SIZE;
                const int top = BOARD_TOP + r * CELL_SIZE;
                rectangle(left, top, left + CELL_SIZE, top + CELL_SIZE);
            }
        }
    }

    void DrawGameAreaStatus(AppState state, bool hasWon, bool hasLost) {
        setbkmode(TRANSPARENT);
        settextcolor(RGB(205, 220, 245));
        settextstyle(30, 0, L"Consolas");
        if (hasWon) {
            const wchar_t* winText = L"you win";
            settextcolor(RGB(255, 235, 120));
            settextstyle(50, 0, L"Consolas");
            const int x = (1280 - textwidth(winText)) / 2;
            const int y = (720 - textheight(winText)) / 2;
            outtextxy(x,y, L"you win");
            return;
        }
        if (hasLost) {
            const wchar_t* loseText = L"you lose";
            settextcolor(RGB(255, 120, 120));
            settextstyle(52, 0, L"Consolas");
            const int x = (1280 - textwidth(loseText)) / 2;
            const int y = (720 - textheight(loseText)) / 2;
            outtextxy(x, y, loseText);
            return;
        }
        if (state == STATE_MENU) {
            outtextxy(500, 345, L"Click Start");
        } else if (state == STATE_RUNNING) {
            outtextxy(485, 345, L"Game Running");
        } else {
            outtextxy(500, 345, L"Game Paused");
        }
    }

    void DrawPlayer(const Player& player) {
        const int left = BOARD_LEFT + player.col * CELL_SIZE + 14;
        const int top = BOARD_TOP + player.row * CELL_SIZE + 14;
        const int right = left + CELL_SIZE - 28;
        const int bottom = top + CELL_SIZE - 28;

        setfillcolor(RGB(92, 176, 255));
        solidrectangle(left, top, right, bottom);
    }

    void DrawInfo(const Player&) {
        setbkmode(TRANSPARENT);
        settextcolor(RGB(255, 220, 90));
        settextstyle(30, 0, L"Fixedsys");
        outtextxy(1050, 410, L"Move: WASD");
        outtextxy(1050, 445, L"Shoot: J");
        outtextxy(1050, 480, L"Melee: K");
    }

    void DrawCallWords(const Player& player, const ScoreBoard& board, int elapsedSeconds) {
        wchar_t scoreText[64] = {};
        wchar_t hitText[64] = {};
        wchar_t timeText[64] = {};
        wchar_t hpText[64] = {};

        swprintf_s(scoreText, L"Score: %d", board.score);
        swprintf_s(hitText, L"Hit: %d", board.hits);
        swprintf_s(timeText, L"Time: %ds", elapsedSeconds);
        swprintf_s(hpText, L"HP: %d", player.hp);

        settextcolor(RGB(255, 220, 90));
        settextstyle(32, 0, L"Fixedsys");
        outtextxy(1080, 520, scoreText);
        outtextxy(1080, 565, hitText);
        outtextxy(1080, 610, timeText);
        outtextxy(1080, 675, hpText);
    }

    Bullet* CreateBullet(float x, float y, float vx, float vy) {//建立链表做子弹
        Bullet* bullet = (Bullet*)std::malloc(sizeof(Bullet));
        if (bullet == nullptr) {
            return nullptr;
        }
        bullet->x = x;
        bullet->y = y;
        bullet->vx = vx;
        bullet->vy = vy;
        bullet->next = nullptr;//把新子弹next置空，暂时不接入任何子弹
        return bullet;
    }

    void PushFront(Bullet*& head, Bullet* bullet) {//头插法
        bullet->next = head;
        head = bullet;
    }

    void ClearBullets(Bullet*& head) {//清空子弹
        Bullet* cur = head;//遍历指针，初始指向表头
        while (cur != nullptr) {
            Bullet* next = cur->next;//保存当前节点的下一个节点
            std::free(cur);
            cur = next;
        }
        head = nullptr;
    }

    bool IsOutOfBounds(const Bullet* bullet) {
        return bullet->x < 0 || bullet->x > 1280 || bullet->y < 0 || bullet->y > 720;
    }

    void ScoreAddEnemyKill(ScoreBoard& board, int enemyType) {
        if (enemyType == ENEMY_TYPE_NONE) {
            return;
        }
        ++board.hits;
        board.score += ScoreForEnemyType(enemyType);
    }

    void UpdatePlayerBullets(Bullet*& head, ScoreBoard& board) {
        Bullet* prev = nullptr;
        Bullet* cur = head;//从头开始遍历，双指针遍历链表

        while (cur != nullptr) {
            cur->x += cur->vx;
            cur->y += cur->vy;
            const bool outOfBounds = IsOutOfBounds(cur);
            const int hitType = (!outOfBounds) ? enemy_hit_type((int)cur->x, (int)cur->y) : ENEMY_TYPE_NONE;//判断子弹是否击中敌人
            const bool hitEnemy = (hitType != ENEMY_TYPE_NONE);
            if (hitEnemy) {
                ScoreAddEnemyKill(board, hitType);//调入击杀敌人，开始计分
            }
            if (outOfBounds || hitEnemy) {//删除击中敌人的子弹
                Bullet* next = cur->next;
                std::free(cur);
                if (prev == nullptr) {
                    head = next;
                } else {
                    prev->next = next;
                }
                cur = next;
            } else {
                prev = cur;
                cur = cur->next;
            }
        }
    }

    bool BulletHitPlayer(const Bullet* bullet, const Player& player) {//敌人子弹击中玩家
        const int left = BOARD_LEFT + player.col * CELL_SIZE + 14;
        const int top = BOARD_TOP + player.row * CELL_SIZE + 14;
        const int right = left + CELL_SIZE - 28;
        const int bottom = top + CELL_SIZE - 28;
        const int x = (int)bullet->x;
        const int y = (int)bullet->y;
        return x >= left && x <= right && y >= top && y <= bottom;
    }

    int UpdateEnemyBulletsAndGetDamage(Bullet*& head, const Player& player) {//子弹出界/击中玩家判定
        Bullet* prev = nullptr;
        Bullet* cur = head;
        int damage = 0;

        while (cur != nullptr) {
            cur->x += cur->vx;
            cur->y += cur->vy;
            const bool outOfBounds = IsOutOfBounds(cur);
            const bool hitPlayer = !outOfBounds && BulletHitPlayer(cur, player);
            if (hitPlayer) {
                damage += 20;
            }
            if (outOfBounds || hitPlayer) {//销毁命中player和出界子弹
                Bullet* next = cur->next;
                std::free(cur);
                if (prev == nullptr) {
                    head = next;
                } else {
                    prev->next = next;
                }
                cur = next;
            } else {
                prev = cur;
                cur = cur->next;
            }
        }
        return damage;
    }

    void DrawPlayerBullets(Bullet* head) {
        setfillcolor(RGB(255, 215, 90));
        for (Bullet* cur = head; cur != nullptr; cur = cur->next) {
            solidcircle((int)cur->x, (int)cur->y, BULLET_RADIUS);
        }
    }

    void DrawEnemyBullets(Bullet* head) {
        setfillcolor(RGB(255, 130, 60));
        for (Bullet* cur = head; cur != nullptr; cur = cur->next) {
            solidcircle((int)cur->x, (int)cur->y, BULLET_RADIUS);
        }
    }

    bool IsInsideBoard(int row, int col) {
        return row >= 0 && row < GRID_ROWS && col >= 0 && col < GRID_COLS;
    }

    void TryMove(Player& player, int dr, int dc, const wchar_t* keyName) {
        const int nextRow = player.row + dr;
        const int nextCol = player.col + dc;

        if (!IsInsideBoard(nextRow, nextCol)) {
            wcscpy_s(player.lastKey, L"Blocked");
            return;
        }

        player.row = nextRow;
        player.col = nextCol;
        wcscpy_s(player.lastKey, keyName);
    }

    void Render(const Player& player,
                AppState state,
                Bullet* playerBullets,
                Bullet* enemyBullets,
                const ScoreBoard& board,
                int elapsedSeconds,
                bool hasWon,
                bool hasLost) {
        static IMAGE boardBgImage;//设置背景
        static bool boardBgInit = false;//标记初始化流程
        static bool boardBgLoaded = false;//标记背景图片加载
        if (!boardBgInit) {
            boardBgInit = true;
            loadimage(&boardBgImage, L"27a4326132af49c7b96e4cd70ab7bfed.jpg", 1280, 720, true);
            boardBgLoaded = boardBgImage.getwidth() > 0 && boardBgImage.getheight() > 0;
            if (!boardBgLoaded) {
                loadimage(&boardBgImage, L".\\x64\\Debug\\27a4326132af49c7b96e4cd70ab7bfed.jpg", 1280, 720, true);
                boardBgLoaded = boardBgImage.getwidth() > 0 && boardBgImage.getheight() > 0;
            }
            if (!boardBgLoaded) {
                loadimage(&boardBgImage, L"..\\x64\\Debug\\27a4326132af49c7b96e4cd70ab7bfed.jpg", 1280, 720, true);
                boardBgLoaded = boardBgImage.getwidth() > 0 && boardBgImage.getheight() > 0;
            }
        }
        cleardevice();
        if (boardBgLoaded) {
            putimage(0, 0, &boardBgImage);
        }
        DrawPanels();
        DrawButtonPlaceholders();
        DrawGrid();
        enemy_draw();
        DrawPlayer(player);
        DrawInfo(player);
        DrawCallWords(player, board, elapsedSeconds);
        DrawPlayerBullets(playerBullets);
        DrawEnemyBullets(enemyBullets);
        DrawGameAreaStatus(state, hasWon, hasLost);
    }
}

void run_checkerboard() {
    Player player = { GRID_ROWS / 2, GRID_COLS / 2, PLAYER_INIT_HP, L"None" };
    Button startBtn = { {0, 0, 115, 45}, L"Start", RGB(66, 126, 230) };
    Button pauseBtn = { {0, 55, 115, 100}, L"Pause", RGB(120, 87, 180) };
    Button quitBtn = { {0, 110, 115, 155}, L"Quit", RGB(206, 83, 83) };
    Bullet* playerBullets = nullptr;
    Bullet* enemyBullets = nullptr;
    DWORD lastFireTick = 0;
    DWORD now = GetTickCount();
    AppState state = STATE_MENU;
    int fireDirRow = 0;
    int fireDirCol = 1;
    bool running = true;
    bool hasWon = false;
    bool hasLost = false;
    bool waitingForSecondBoard = false;
    bool onSecondBoard = false;
    bool spacePressed = false;
    ScoreBoard board = { 0, 0 };
    enemy_reset();
    timeaccount_reset();
    while (running) {
        const bool spaceDown = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
        if (!spaceDown) {
            spacePressed = false;
        }
        if (waitingForSecondBoard && hasWon && !hasLost && spaceDown && !spacePressed) {
            spacePressed = true;
            waitingForSecondBoard = false;
            onSecondBoard = true;
            hasWon = false;
            state = STATE_RUNNING;
            board.score = 0;
            board.hits = 0;
            ClearBullets(playerBullets);
            ClearBullets(enemyBullets);
            enemy_reset_right_side();
            timeaccount_reset();
            timeaccount_set_running(true);
        }
        const int elapsedSeconds = timeaccount_get_elapsed_seconds();
        Render(player, state, playerBullets, enemyBullets, board, elapsedSeconds, hasWon, hasLost);
        FlushBatchDraw();
        ExMessage msg = {};
        while (peekmessage(&msg, EX_MOUSE)) {
            if (msg.message != WM_LBUTTONDOWN) {
                continue;
            }
            if (HitButton(startBtn, msg.x, msg.y)) {
                if (!hasWon && !hasLost) {
                    state = STATE_RUNNING;
                }
            } else if (HitButton(pauseBtn, msg.x, msg.y)) {
                if (!hasWon && !hasLost) {
                    state = (state == STATE_RUNNING) ? STATE_PAUSED : STATE_RUNNING;
                }
            } else if (HitButton(quitBtn, msg.x, msg.y)) {
                running = false;
            }
        }
        if (state == STATE_RUNNING) {
            timeaccount_set_running(true);
            enemy_update();
            now = GetTickCount();
            static bool keyPressed[256] = { false };
            if ((GetAsyncKeyState('W') & 0x8000) && !keyPressed['W']) {
                fireDirRow = -1;
                fireDirCol = 0;
                keyPressed['W'] = true;
                TryMove(player, -1, 0, L"W");
            }
            if (!(GetAsyncKeyState('W') & 0x8000)) keyPressed['W'] = false;
            if ((GetAsyncKeyState('S') & 0x8000) && !keyPressed['S']) {
                fireDirRow = 1;
                fireDirCol = 0;
                keyPressed['S'] = true;
                TryMove(player, 1, 0, L"S");
            }
            if (!(GetAsyncKeyState('S') & 0x8000)) keyPressed['S'] = false;
            if ((GetAsyncKeyState('A') & 0x8000) && !keyPressed['A']) {
                fireDirRow = 0;
                fireDirCol = -1;
                keyPressed['A'] = true;
                TryMove(player, 0, -1, L"A");
            }
            if (!(GetAsyncKeyState('A') & 0x8000)) keyPressed['A'] = false;
            if ((GetAsyncKeyState('D') & 0x8000) && !keyPressed['D']) {
                fireDirRow = 0;
                fireDirCol = 1;
                keyPressed['D'] = true;
                TryMove(player, 0, 1, L"D");
            }
            if (!(GetAsyncKeyState('D') & 0x8000)) keyPressed['D'] = false;
            if ((GetAsyncKeyState(VK_UP) & 0x8000) && !keyPressed[VK_UP]) {
                fireDirRow = -1;
                fireDirCol = 0;
                keyPressed[VK_UP] = true;
            }
            if (!(GetAsyncKeyState(VK_UP) & 0x8000)) keyPressed[VK_UP] = false;
            if ((GetAsyncKeyState(VK_DOWN) & 0x8000) && !keyPressed[VK_DOWN]) {
                fireDirRow = 1;
                fireDirCol = 0;
                keyPressed[VK_DOWN] = true;
            }
            if (!(GetAsyncKeyState(VK_DOWN) & 0x8000)) keyPressed[VK_DOWN] = false;
            if ((GetAsyncKeyState(VK_LEFT) & 0x8000) && !keyPressed[VK_LEFT]) {
                fireDirRow = 0;
                fireDirCol = -1;
                keyPressed[VK_LEFT] = true;
            }
            if (!(GetAsyncKeyState(VK_LEFT) & 0x8000)) keyPressed[VK_LEFT] = false;
            if ((GetAsyncKeyState(VK_RIGHT) & 0x8000) && !keyPressed[VK_RIGHT]) {
                fireDirRow = 0;
                fireDirCol = 1;
                keyPressed[VK_RIGHT] = true;
            }
            if (!(GetAsyncKeyState(VK_RIGHT) & 0x8000)) keyPressed[VK_RIGHT] = false;
            if ((GetAsyncKeyState('J') & 0x8000) && now - lastFireTick >= FIRE_COOLDOWN_MS) {
                const float x = BOARD_LEFT + player.col * CELL_SIZE + CELL_SIZE / 2.0f;
                const float y = BOARD_TOP + player.row * CELL_SIZE + CELL_SIZE / 2.0f;
                const float vx = fireDirCol * PLAYER_BULLET_SPEED;
                const float vy = fireDirRow * PLAYER_BULLET_SPEED;
                Bullet* bullet = CreateBullet(x, y, vx, vy);
                if (bullet != nullptr) {
                    PushFront(playerBullets, bullet);
                    lastFireTick = now;
                }
            }
            if ((GetAsyncKeyState('K') & 0x8000) && !keyPressed['K']) {
                keyPressed['K'] = true;
                const int playerCenterX = BOARD_LEFT + player.col * CELL_SIZE + CELL_SIZE / 2;
                const int playerCenterY = BOARD_TOP + player.row * CELL_SIZE + CELL_SIZE / 2;
                const int rangePixels = KILL_RANGE_CELLS * CELL_SIZE;
                const int enemyType = enemy_attack_near_type(playerCenterX, playerCenterY, rangePixels);
                ScoreAddEnemyKill(board, enemyType);
            }
            if (!(GetAsyncKeyState('K') & 0x8000)) keyPressed['K'] = false;
            if (GetAsyncKeyState('C') & 0x8000) {
                ClearBullets(playerBullets);
                ClearBullets(enemyBullets);
            }
            UpdatePlayerBullets(playerBullets, board);
            const int playerCenterX = BOARD_LEFT + player.col * CELL_SIZE + CELL_SIZE / 2;
            const int playerCenterY = BOARD_TOP + player.row * CELL_SIZE + CELL_SIZE / 2;
            const int nearRange = KILL_RANGE_CELLS * CELL_SIZE;
            const int meleeDamage = enemy_green_melee_damage(playerCenterX, playerCenterY, nearRange);
            player.hp = ClampMin(player.hp - meleeDamage, 0);
            EnemySpawnBullet spawned[12];
            const int spawnedCount = enemy_spawn_yellow_bullets(playerCenterX,
                                                                playerCenterY,
                                                                ENEMY_BULLET_SPEED,
                                                                spawned,
                                                                12);
            for (int i = 0; i < spawnedCount; ++i) {
                Bullet* bullet = CreateBullet(spawned[i].x, spawned[i].y, spawned[i].vx, spawned[i].vy);
                if (bullet != nullptr) {
                    PushFront(enemyBullets, bullet);
                }
            }
            const int enemyBulletDamage = UpdateEnemyBulletsAndGetDamage(enemyBullets, player);
            player.hp = ClampMin(player.hp - enemyBulletDamage, 0);
            if (player.hp <= 0) {
                hasLost = true;
                state = STATE_PAUSED;
            }
            const int targetScore = onSecondBoard ? SECOND_BOARD_WIN_SCORE : FIRST_BOARD_WIN_SCORE;
            if (board.score >= targetScore) {
                hasWon = true;
                state = STATE_PAUSED;
                if (!onSecondBoard) {
                    waitingForSecondBoard = true;
                }
            }
        } else {
            timeaccount_set_running(false);
        }
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            running = false;
        }
        Sleep(16);
    }
    ClearBullets(playerBullets);
    ClearBullets(enemyBullets);
}
