#include <windows.h>
#include "timeaccount.h"

namespace {
    DWORD g_lastTick = 0;//上次调用updatetime的时间
    int g_elapsedMs = 0;//累计总时间
    bool g_running = false;//计时器是否运行

    void UpdateTime() {
        const DWORD now = GetTickCount();
        if (g_lastTick == 0) {
            g_lastTick = now;
            return;
        }
        if (g_running) {
            g_elapsedMs += static_cast<int>(now - g_lastTick);
        }
        g_lastTick = now;
    }
}

void timeaccount_reset() {//计时器重置
    g_lastTick = GetTickCount();
    g_elapsedMs = 0;
    g_running = false;
}

void timeaccount_set_running(bool running) {//控制计时器启动
    UpdateTime();
    g_running = running;
}

int timeaccount_get_elapsed_seconds() {//获取累计秒数
    UpdateTime();
    return g_elapsedMs / 1000;
}
