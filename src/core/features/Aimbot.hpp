#pragma once
#include "core/engine/cache/Cache.hpp"
#include <chrono>
#include <vector>
#include <random>

struct LegitbotState {
    int lastTargetIndex = -1;
    std::chrono::steady_clock::time_point targetAcquiredTime;
    std::chrono::steady_clock::time_point lastKillTime;
    bool wasAiming = false;
    bool inAfterKillDelay = false;
    bool inReactionDelay = false;
    int lastTargetHealth = 100;
    int reactionTargetIndex = -1;
    bool missedShot = false;
    int missTargetIndex = -1;

    bool keyWasDown = false;

    Vec2_t vel = { 0.f, 0.f };               
    Vec2_t fixedOffset = { 0.f, 0.f };
    bool   offsetInitialised = false;
    Vec2_t lastTargetScreen = { 0.f, 0.f };
    int    acquiredBone = -1; 

    std::chrono::steady_clock::time_point lastFrame;

    Vec2_t filteredTarget     = { 0.f, 0.f };
    Vec2_t prevFilteredTarget = { 0.f, 0.f };
    Vec2_t filteredVel        = { 0.f, 0.f };
    bool   filterReady        = false;

    Vec2_t curveBias = { 0.f, 0.f };
};

class Aimbot {
public:
    static void Init();
    static void Shutdown();

    static inline std::atomic<bool> is_aiming{ false };

private:
    static void Thread();

    static inline std::thread thread_;

    static Vec2_t ApplyHumanError(Vec2_t target);
    static bool ShouldMissShot(int targetIndex);
};