#pragma once
#include "core/engine/cache/Cache.hpp"

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

class Aimbot {
public:
    static void Init();
    
    // Flag to tell RCS when aimbot is actively aiming
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
};
