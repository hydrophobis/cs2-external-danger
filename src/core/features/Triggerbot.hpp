#pragma once
#include "config/Config.hpp"
#include "core/engine/cache/Cache.hpp"
#include <Windows.h>

class Triggerbot {
public:
    static void Run();
    
private:
    static bool IsCrosshairOnEnemy();
    static void Shoot();
};
