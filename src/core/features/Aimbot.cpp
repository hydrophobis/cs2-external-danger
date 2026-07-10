#include "Aimbot.hpp"
#include "core/engine/Engine.hpp"
#include <thread>
#include <cmath>

void Aimbot::Init() {
    std::thread(Aimbot::Thread).detach();
    LOGF(INFO, "==========================================");
    LOGF(INFO, "Aimbot: MOUSE-BASED AIMBOT INITIALIZED");
    LOGF(INFO, "Aimbot: Hotkey = {}", cfg::aimbot::hotkey);
    LOGF(INFO, "Aimbot: Always On = {}", cfg::aimbot::always_on);
    LOGF(INFO, "==========================================");
}

void Aimbot::Thread() {
    float aimbotRemainderX = 0.f;
    float aimbotRemainderY = 0.f;
    
    int status_counter = 0;
    int target_counter = 0;

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        if (!cfg::enabled) {
            if (status_counter++ % 1000 == 0) {
                LOGF(INFO, "Aimbot: cfg::enabled is FALSE");
            }
            continue;
        }

        auto snapshot = Cache::CopySnapshot();
        if (!snapshot.local.alive) {
            if (status_counter++ % 1000 == 0) {
                LOGF(INFO, "Aimbot: Local player not alive");
            }
            continue;
        }

        float screenX = static_cast<float>(GetSystemMetrics(SM_CXSCREEN)) / 2.f;
        float screenY = static_cast<float>(GetSystemMetrics(SM_CYSCREEN)) / 2.f;
        Vec2_t screenCenter = { screenX, screenY };

        float totalMoveX = 0.f;
        float totalMoveY = 0.f;

        bool hasTarget = false;

        bool aimKeyDown = (GetAsyncKeyState(cfg::aimbot::hotkey) & 0x8000) != 0;
        bool aimActive = cfg::aimbot::always_on ? !aimKeyDown : aimKeyDown;
        
        // Reset flag at start of each frame
        Aimbot::is_aiming = false;
        
        if (status_counter++ % 1000 == 0) {
            LOGF(INFO, "Aimbot: Status - enabled={}, keyDown={}, active={}, players={}", 
                 cfg::aimbot::enabled, aimKeyDown, aimActive, snapshot.players.size());
        }
        
        
        if (cfg::aimbot::enabled && aimActive) {
            Player* bestTarget = nullptr;
            float bestDist = cfg::aimbot::fov * 10.f;
            Vec2_t bestTargetPos = { 0, 0 };
            int bestBoneIndex = -1;
            
            int valid_players = 0;
            int visible_filtered = 0;
            int bones_filtered = 0;
            int wts_filtered = 0;
            int checked_players = 0;

            for (auto& player : snapshot.players) {
                if (!player.alive || player.localplayer)
                    continue;
                if (player.team == snapshot.local.team)
                    continue;
                    
                valid_players++;
                checked_players++;
                
                // Use spotted field for visibility check
                if (cfg::aimbot::visible_only && !player.spotted) {
                    visible_filtered++;
                    continue;
                }
                if (player.bone_list.empty()) {
                    bones_filtered++;
                    continue;
                }

                // Multibone targeting - two modes
                if (cfg::aimbot::multibone) {
                    if (cfg::aimbot::multibone_closest) {
                        // CLOSEST MODE: Check all bones and pick the one closest to crosshair
                        for (int i = 0; i < 5; i++) {
                            int bone_idx = cfg::aimbot::bone_priority[i];
                            if (bone_idx >= (int)player.bone_list.size())
                                continue;
                                
                            Vec2_t bonePos;
                            auto bone = player.bone_list[bone_idx];

                            Vec3_t aimPos = bone.pos;
                            if (cfg::aimbot::velocity_comp) {
                                float scale = cfg::aimbot::velocity_comp_scale;
                                Vec3_t relVel = {
                                    player.vel.x - snapshot.local.vel.x,
                                    player.vel.y - snapshot.local.vel.y,
                                    player.vel.z - snapshot.local.vel.z
                                };

                                aimPos.x += relVel.x * scale;
                                aimPos.y += relVel.y * scale;
                                aimPos.z += relVel.z * scale;
                            }

                            bool wts_success = snapshot.game.view_matrix.wts(aimPos, Vec2_t(screenX * 2, screenY * 2), bonePos, false);
                            if (!wts_success)
                                continue;

                            float dist = std::sqrt(std::pow(bonePos.x - screenCenter.x, 2) + std::pow(bonePos.y - screenCenter.y, 2));
                            
                            // Check if this bone is closer than current best
                            if (dist < bestDist) {
                                bestDist = dist;
                                bestTarget = &player;
                                bestTargetPos = bonePos;
                                bestBoneIndex = bone_idx;
                            }
                        }
                    } else {
                        // PRIORITY MODE: Try bones in order, use first valid one
                        for (int i = 0; i < 5; i++) {
                            int bone_idx = cfg::aimbot::bone_priority[i];
                            if (bone_idx >= (int)player.bone_list.size())
                                continue;
                                
                            Vec2_t bonePos;
                            auto bone = player.bone_list[bone_idx];

                            Vec3_t aimPos = bone.pos;
                            if (cfg::aimbot::velocity_comp) {
                                float scale = cfg::aimbot::velocity_comp_scale;
                                Vec3_t relVel = {
                                    player.vel.x - snapshot.local.vel.x,
                                    player.vel.y - snapshot.local.vel.y,
                                    player.vel.z - snapshot.local.vel.z
                                };

                                aimPos.x += relVel.x * scale;
                                aimPos.y += relVel.y * scale;
                                aimPos.z += relVel.z * scale;
                            }

                            bool wts_success = snapshot.game.view_matrix.wts(aimPos, Vec2_t(screenX * 2, screenY * 2), bonePos, false);
                            if (!wts_success)
                                continue;

                            float dist = std::sqrt(std::pow(bonePos.x - screenCenter.x, 2) + std::pow(bonePos.y - screenCenter.y, 2));
                            
                            // In priority mode, check if within FOV and take first valid
                            if (dist < cfg::aimbot::fov * 10.f) {
                                // Found valid bone in FOV, use it (don't check others)
                                if (dist < bestDist) {
                                    bestDist = dist;
                                    bestTarget = &player;
                                    bestTargetPos = bonePos;
                                    bestBoneIndex = bone_idx;
                                }
                                break; // Use this bone, don't try lower priority ones
                            }
                        }
                    }
                } else {
                    // Single bone mode - head only
                    Vec2_t headPos;
                    auto head_bone = player.bone_list[bone_index::head];

                    Vec3_t aimPos = head_bone.pos;
                    if (cfg::aimbot::velocity_comp) {
                        float scale = cfg::aimbot::velocity_comp_scale;
                        Vec3_t relVel = {
                            player.vel.x - snapshot.local.vel.x,
                            player.vel.y - snapshot.local.vel.y,
                            player.vel.z - snapshot.local.vel.z
                        };

                        aimPos.x += relVel.x * scale;
                        aimPos.y += relVel.y * scale;
                        aimPos.z += relVel.z * scale;
                    }

                    bool wts_success = snapshot.game.view_matrix.wts(aimPos, Vec2_t(screenX * 2, screenY * 2), headPos, false);
                    if (!wts_success) {
                        wts_filtered++;
                        continue;
                    }

                    float dist = std::sqrt(std::pow(headPos.x - screenCenter.x, 2) + std::pow(headPos.y - screenCenter.y, 2));
                    
                    if (dist < bestDist) {
                        bestDist = dist;
                        bestTarget = &player;
                        bestTargetPos = headPos;
                        bestBoneIndex = bone_index::head;
                    }
                }
            }
            
            if (target_counter++ % 50 == 0 && aimActive) {
                LOGF(INFO, "Aimbot: Filter stats - checked={}, visible_filtered={}, bones_filtered={}, wts_filtered={}, bestTarget={}, visible_only={}, multibone={}", 
                     checked_players, visible_filtered, bones_filtered, wts_filtered, (bestTarget != nullptr), cfg::aimbot::visible_only, cfg::aimbot::multibone);
            }

            if (bestTarget) {
                hasTarget = true;
                Aimbot::is_aiming = true;
                float smooth = cfg::aimbot::smooth > 0.1f ? cfg::aimbot::smooth : 1.0f;

                static int rcs_log = 0;
                if (cfg::aimbot::rcs && snapshot.local.shotsFired > 0) {
                    Vec2_t punch = snapshot.local.aimPunch;
                    float rcsScale = -1.f;
                    float pixelScale = 15.0f;
                    
                    float offsetX = punch.y * rcsScale * pixelScale;
                    float offsetY = punch.x * rcsScale * pixelScale;
                    
                    if (rcs_log++ % 30 == 0) {
                        LOGF(INFO, "Aimbot RCS: shots={}, punch=({:.3f},{:.3f}), offset=({:.1f},{:.1f})", 
                             snapshot.local.shotsFired, punch.x, punch.y, offsetX, offsetY);
                    }
                    
                    bestTargetPos.x -= offsetX;
                    bestTargetPos.y += offsetY;
                }

                float aimX = (bestTargetPos.x - screenCenter.x) / smooth + aimbotRemainderX;
                float aimY = (bestTargetPos.y - screenCenter.y) / smooth + aimbotRemainderY;
                aimbotRemainderX = 0.f;
                aimbotRemainderY = 0.f;
                totalMoveX += aimX;
                totalMoveY += aimY;
                
                LOGF(INFO, "Aimbot: TARGET! Name={}, Bone={}, Dist={:.1f}px, Target=({:.1f},{:.1f}), Move=({:.1f},{:.1f})", 
                     bestTarget->name, bestBoneIndex, bestDist, bestTargetPos.x, bestTargetPos.y, aimX, aimY);
            }
        }

        // Apply mouse movement
        int moveX = static_cast<int>(totalMoveX);
        int moveY = static_cast<int>(totalMoveY);

        float fracX = totalMoveX - static_cast<float>(moveX);
        float fracY = totalMoveY - static_cast<float>(moveY);
        if (hasTarget) {
            aimbotRemainderX = fracX;
            aimbotRemainderY = fracY;
        }

        if (moveX != 0 || moveY != 0) {
            LOGF(INFO, "Aimbot: MOUSE_EVENT ({}, {})", moveX, moveY);
            mouse_event(MOUSEEVENTF_MOVE, static_cast<DWORD>(moveX), static_cast<DWORD>(moveY), 0, 0);
        }
    }
}
