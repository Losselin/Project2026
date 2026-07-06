#include <graphics.h>
#include <windows.h>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include "enemy.h"

namespace {
    const int WINDOW_WIDTH = 1280;
    const int GAME_BOTTOM = 600;
    const int MAX_ENEMIES = 12;
    const int ENEMY_WIDTH = 56;
    const int ENEMY_HEIGHT = 40;
    const int GRID_STEP = 80;
    const DWORD MOVE_INTERVAL_MS = 1500;
    const DWORD GREEN_MELEE_INTERVAL_MS = 2000;
    const DWORD YELLOW_FIRE_INTERVAL_MS = 3000;

    struct Enemy {
        int x;
        int y;
        int w;
        int h;
        bool alive;
        DWORD lastMoveTick;
        DWORD lastMeleeTick;
        DWORD lastFireTick;
        EnemyType type;
    };

    Enemy enemies[MAX_ENEMIES];

    int Clamp(int value, int low, int high) {
        if (value < low) return low;
        if (value > high) return high;
        return value;
    }

    bool CanPlaceAt(int x, int y) {
        return x >= 0 && x + ENEMY_WIDTH <= WINDOW_WIDTH && y >= 0 && y + ENEMY_HEIGHT <= GAME_BOTTOM;
    }

    void InitEnemiesAt(int startX, int stepX) {
        static bool seeded = false;
        if (!seeded) {
            std::srand(static_cast<unsigned>(std::time(nullptr)));
            seeded = true;
        }

        const DWORD now = GetTickCount();
        for (int i = 0; i < MAX_ENEMIES; ++i) {
            enemies[i].w = ENEMY_WIDTH;
            enemies[i].h = ENEMY_HEIGHT;
            enemies[i].x = startX + (i % 6) * stepX;
            enemies[i].y = 80 + (i / 6) * 180;
            enemies[i].x = Clamp(enemies[i].x, 0, WINDOW_WIDTH - ENEMY_WIDTH);
            enemies[i].y = Clamp(enemies[i].y, 0, GAME_BOTTOM - ENEMY_HEIGHT);
            enemies[i].alive = true;
            enemies[i].lastMoveTick = now;
            enemies[i].lastMeleeTick = now;
            enemies[i].lastFireTick = now;
            enemies[i].type = (i % 2 == 0) ? ENEMY_TYPE_GREEN : ENEMY_TYPE_YELLOW;
        }
    }

    void InitEnemies() {
        InitEnemiesAt(90, 180);
    }

    void InitEnemiesRightSide() {
        InitEnemiesAt(700, 100);
    }

    void MoveEnemyRandom4(Enemy& enemy, DWORD now) {
        if (!enemy.alive) {
            return;
        }
        if (now - enemy.lastMoveTick < MOVE_INTERVAL_MS) {
            return;
        }

        static const int dirRow[4] = { -1, 1, 0, 0 };
        static const int dirCol[4] = { 0, 0, -1, 1 };

        int options[4] = { 0, 1, 2, 3 };
        for (int i = 3; i > 0; --i) {
            const int j = std::rand() % (i + 1);
            const int tmp = options[i];
            options[i] = options[j];
            options[j] = tmp;
        }

        for (int k = 0; k < 4; ++k) {
            const int d = options[k];
            const int nx = enemy.x + dirCol[d] * GRID_STEP;
            const int ny = enemy.y + dirRow[d] * GRID_STEP;
            if (CanPlaceAt(nx, ny)) {
                enemy.x = nx;
                enemy.y = ny;
                break;
            }
        }

        enemy.lastMoveTick = now;
    }

    void UpdateEnemies() {
        const DWORD now = GetTickCount();
        for (int i = 0; i < MAX_ENEMIES; ++i) {
            MoveEnemyRandom4(enemies[i], now);
        }
    }

    void DrawEnemies(const Enemy list[]) {
        for (int i = 0; i < MAX_ENEMIES; ++i) {
            if (!list[i].alive) {
                continue;
            }
            setfillcolor((list[i].type == ENEMY_TYPE_GREEN) ? RGB(90, 220, 120) : RGB(230, 180, 70));
            solidrectangle(list[i].x, list[i].y, list[i].x + list[i].w, list[i].y + list[i].h);
            setlinecolor(RGB(20, 20, 20));
            rectangle(list[i].x, list[i].y, list[i].x + list[i].w, list[i].y + list[i].h);
        }
    }

    EnemyType HitEnemyAndGetType(Enemy& enemy, int x, int y) {
        if (!enemy.alive) {
            return ENEMY_TYPE_NONE;
        }
        if (x < enemy.x || x > enemy.x + enemy.w || y < enemy.y || y > enemy.y + enemy.h) {
            return ENEMY_TYPE_NONE;
        }
        enemy.alive = false;
        return enemy.type;
    }

    int CountAliveEnemies() {
        int aliveCount = 0;
        for (int i = 0; i < MAX_ENEMIES; ++i) {
            if (enemies[i].alive) {
                ++aliveCount;
            }
        }
        return aliveCount;
    }
}

void enemy_reset() {
    InitEnemies();
}

void enemy_reset_right_side() {
    InitEnemiesRightSide();
}

void enemy_update() {
    UpdateEnemies();
}

void enemy_draw() {
    DrawEnemies(enemies);
}

int enemy_hit_type(int x, int y) {
    for (int i = 0; i < MAX_ENEMIES; ++i) {
        const EnemyType type = HitEnemyAndGetType(enemies[i], x, y);
        if (type != ENEMY_TYPE_NONE) {
            return static_cast<int>(type);
        }
    }
    return static_cast<int>(ENEMY_TYPE_NONE);
}

bool enemy_hit(int x, int y) {
    return enemy_hit_type(x, y) != static_cast<int>(ENEMY_TYPE_NONE);
}

int enemy_attack_near_type(int playerCenterX, int playerCenterY, int rangePixels) {
    const int rangeSquared = rangePixels * rangePixels;
    for (int i = 0; i < MAX_ENEMIES; ++i) {
        if (!enemies[i].alive) {
            continue;
        }
        const int enemyCenterX = enemies[i].x + enemies[i].w / 2;
        const int enemyCenterY = enemies[i].y + enemies[i].h / 2;
        const int dx = enemyCenterX - playerCenterX;
        const int dy = enemyCenterY - playerCenterY;
        const int distanceSquared = dx * dx + dy * dy;
        if (distanceSquared <= rangeSquared) {
            const EnemyType type = enemies[i].type;
            enemies[i].alive = false;
            return static_cast<int>(type);
        }
    }
    return static_cast<int>(ENEMY_TYPE_NONE);
}

bool enemy_attack_near(int playerCenterX, int playerCenterY, int rangePixels) {
    return enemy_attack_near_type(playerCenterX, playerCenterY, rangePixels) != static_cast<int>(ENEMY_TYPE_NONE);
}

int enemy_green_melee_damage(int playerCenterX, int playerCenterY, int rangePixels) {
    const DWORD now = GetTickCount();
    const int rangeSquared = rangePixels * rangePixels;
    for (int i = 0; i < MAX_ENEMIES; ++i) {
        Enemy& enemy = enemies[i];
        if (!enemy.alive || enemy.type != ENEMY_TYPE_GREEN) {
            continue;
        }
        const int enemyCenterX = enemy.x + enemy.w / 2;
        const int enemyCenterY = enemy.y + enemy.h / 2;
        const int dx = enemyCenterX - playerCenterX;
        const int dy = enemyCenterY - playerCenterY;
        const int distanceSquared = dx * dx + dy * dy;
        if (distanceSquared > rangeSquared) {
            continue;
        }
        if (now - enemy.lastMeleeTick >= GREEN_MELEE_INTERVAL_MS) {
            enemy.lastMeleeTick = now;
            return 40;
        }
    }
    return 0;
}

int enemy_spawn_yellow_bullets(int playerCenterX,
                               int playerCenterY,
                               float bulletSpeed,
                               EnemySpawnBullet* outBullets,
                               int maxBullets) {
    if (outBullets == nullptr || maxBullets <= 0) {
        return 0;
    }

    int outCount = 0;
    const DWORD now = GetTickCount();
    for (int i = 0; i < MAX_ENEMIES; ++i) {
        Enemy& enemy = enemies[i];
        if (!enemy.alive || enemy.type != ENEMY_TYPE_YELLOW) {
            continue;
        }
        if (now - enemy.lastFireTick < YELLOW_FIRE_INTERVAL_MS) {
            continue;
        }

        const float startX = static_cast<float>(enemy.x + enemy.w / 2);
        const float startY = static_cast<float>(enemy.y + enemy.h / 2);
        float dx = static_cast<float>(playerCenterX) - startX;
        float dy = static_cast<float>(playerCenterY) - startY;
        const float len = std::sqrt(dx * dx + dy * dy);
        if (len <= 0.001f) {
            dx = 1.0f;
            dy = 0.0f;
        } else {
            dx /= len;
            dy /= len;
        }

        outBullets[outCount].x = startX;
        outBullets[outCount].y = startY;
        outBullets[outCount].vx = dx * bulletSpeed;
        outBullets[outCount].vy = dy * bulletSpeed;
        ++outCount;
        enemy.lastFireTick = now;

        if (outCount >= maxBullets) {
            break;
        }
    }
    return outCount;
}

int enemy_alive_count() {
    return CountAliveEnemies();
}
