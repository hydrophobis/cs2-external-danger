#pragma once
#include "config/Config.hpp"
#include "core/engine/cache/Cache.hpp"
#include <Windows.h>

class RCS {
public:
    static void Run();
    
private:
    static Vec3_t last_punch_angle;
};
