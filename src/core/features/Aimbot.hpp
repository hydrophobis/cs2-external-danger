#pragma once
#include "core/engine/cache/Cache.hpp"
#include <chrono>
#include <vector>
#include <random>

struct AimLock {
    int playerIndex = -1;
    int bone = -1;

    bool Valid() const {
        return playerIndex != -1 && bone != -1;
    }

    void Reset() {
        playerIndex = -1;
        bone = -1;
    }
};

enum class WeaponType {
    RIFLE,
    SMG,
    SNIPER,
    PISTOL,
    HEAVY,
    OTHER
};

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
    static inline bool is_aiming = false;

private:
    static void Thread();

    static bool IsValidTarget(const Player& player, const Snapshot& snapshot);
    static bool AcquireTarget(const Snapshot& snapshot, AimLock& lock);
    static Vec3_t GetAimPosition(const Player& player, int bone, const Snapshot& snapshot);
    static Vec3_t SolveAimAngle(const Player& player, int bone, const Snapshot& snapshot);
    static Vec3_t CalculateAngleRCS(const Snapshot& snapshot, Vec2_t& oldPunch);
    static Vec2_t CalculateMouseRCS(const Snapshot& snapshot, Vec2_t& oldPunch);
    static Vec2_t CalculateMouseAim(const Player& target, int bone, const Snapshot& snapshot);
    static void ApplyAngleWrite(const Vec3_t& delta);
    static void ApplyMouseAim(const Player& target, int bone, const Snapshot& snapshot);
    static void ApplyMouseRCS(const Vec2_t& delta, Vec2_t& remainder);

    static WeaponType GetWeaponType(short weaponId);
    static void GetWeaponSettings(WeaponType type, float& smooth, float& fov);
    static Vec2_t ApplyHumanError(Vec2_t target);
    static bool ShouldMissShot(int targetIndex);
};