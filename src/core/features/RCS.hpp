#pragma once
#include "config/Config.hpp"
#include "core/engine/cache/Cache.hpp"
#include <Windows.h>
#include <thread>

struct RcsController {
    void Reset();

    bool Step(Vec2_t punch, float dt, LONG& out_x, LONG& out_y);

private:
    Vec2_t applied{ 0.f, 0.f };
    float rem_x = 0.f, rem_y = 0.f;
    bool armed = false;
};

class RCS {
public:
    static void Init();
    static void Shutdown();

private:
    static void Thread();

    static inline std::thread thread_;
};
