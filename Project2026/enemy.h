#ifndef ENEMY_H
#define ENEMY_H

enum EnemyType {
    ENEMY_TYPE_NONE = 0,
    ENEMY_TYPE_GREEN = 1,
    ENEMY_TYPE_YELLOW = 2
};

struct EnemySpawnBullet {
    float x;
    float y;
    float vx;
    float vy;
};

void enemy_reset();
void enemy_reset_right_side();
void enemy_update();
void enemy_draw();
bool enemy_hit(int x, int y);
bool enemy_attack_near(int playerCenterX, int playerCenterY, int rangePixels);
int enemy_hit_type(int x, int y);
int enemy_attack_near_type(int playerCenterX, int playerCenterY, int rangePixels);
int enemy_green_melee_damage(int playerCenterX, int playerCenterY, int rangePixels);
int enemy_spawn_yellow_bullets(int playerCenterX,
                               int playerCenterY,
                               float bulletSpeed,
                               EnemySpawnBullet* outBullets,
                               int maxBullets);
int enemy_alive_count();

#endif
