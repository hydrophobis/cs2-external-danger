#include "Aimbot.hpp"
#include "core/engine/Engine.hpp"
#include <thread>
#include <cmath>
#include <algorithm>
#include <Windows.h>
#include <array>

static LegitbotState s;
static std::random_device s_rd;
static std::mt19937 s_gen(s_rd());

using pNtUserSendInput = LONG(__stdcall*)(UINT, LPINPUT, int);
static pNtUserSendInput NtUserSendInput = nullptr;
static HMODULE hWin32u = nullptr;

static void InitNtUserSendInput() {
    if (NtUserSendInput) return;
    hWin32u = GetModuleHandleA("win32u.dll");
    if (!hWin32u) hWin32u = LoadLibraryA("win32u.dll");
    if (!hWin32u) return;
    NtUserSendInput = (pNtUserSendInput)GetProcAddress(hWin32u, "NtUserSendInput");
}

static Vec2_t LerpV(Vec2_t a, Vec2_t b, float t) {
    return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
}

static Vec2_t ComputeTremor(float tSec, float jitterAmp) {
    if (jitterAmp < 0.01f) return { 0.f, 0.f };
    const float PI = 3.14159265f;
    float slow = std::sin(2.f * PI * 0.45f * tSec);
    float wobble = std::sin(2.f * PI * 3.7f * tSec + 1.3f) + std::sin(2.f * PI * 6.1f * tSec + 0.7f);
    return {
        jitterAmp * (0.55f * slow + 0.21f * wobble),
        jitterAmp * (0.50f * std::sin(2.f * PI * 0.37f * tSec + 2.1f)
                   + 0.28f * std::sin(2.f * PI * 4.3f * tSec + 0.4f))
    };
}

WeaponType Aimbot::GetWeaponType(short weaponId) {
    switch (weaponId) {
        case weapon_ak47:
        case weapon_m4a1:
        case weapon_m4a1_silencer:
        case weapon_aug:
        case weapon_famas:
        case weapon_galilar:
        case weapon_sg556:
            return WeaponType::RIFLE;
        case weapon_p90:
        case weapon_mp7:
        case weapon_mp9:
        case weapon_mp5sd:
        case weapon_mac10:
        case weapon_ump45:
        case weapon_bizon:
            return WeaponType::SMG;
        case weapon_awp:
        case weapon_ssg08:
        case weapon_scar20:
        case weapon_g3sg1:
            return WeaponType::SNIPER;
        case weapon_deagle:
        case weapon_elite:
        case weapon_fiveseven:
        case weapon_glock:
        case weapon_hkp2000:
        case weapon_p250:
        case weapon_tec9:
        case weapon_usp_silencer:
        case weapon_cz75a:
        case weapon_revolver:
            return WeaponType::PISTOL;
        case weapon_m249:
        case weapon_negev:
        case weapon_xm1014:
        case weapon_mag7:
        case weapon_nova:
        case weapon_sawedoff:
            return WeaponType::HEAVY;
        default:
            return WeaponType::OTHER;
    }
}

void Aimbot::GetWeaponSettings(WeaponType type, float& fov, float& smooth) {
    fov    = cfg::aimbot::fov;
    smooth = cfg::aimbot::smooth;
}

Vec2_t Aimbot::ApplyHumanError(Vec2_t target) {
    if (!cfg::aimbot::humanization || cfg::aimbot::aim_error_px < 0.01f)
        return target;

    if (!s.offsetInitialised) {
        std::uniform_real_distribution<float> err(-cfg::aimbot::aim_error_px,
                                                  +cfg::aimbot::aim_error_px);
        s.fixedOffset.x = err(s_gen);
        s.fixedOffset.y = err(s_gen);
        s.offsetInitialised = true;
    }

    target.x += s.fixedOffset.x;
    target.y += s.fixedOffset.y;
    return target;
}

bool Aimbot::ShouldMissShot(int targetIndex) {
    if (!cfg::aimbot::humanization || cfg::aimbot::miss_chance <= 0.f)
        return false;

    if (s.missedShot && s.missTargetIndex == targetIndex)
        return true;

    std::uniform_real_distribution<float> roll(0.f, 1.f);
    if (roll(s_gen) < cfg::aimbot::miss_chance) {
        s.missedShot = true;
        s.missTargetIndex = targetIndex;
        return true;
    }
    return false;
}

static Vec3_t ApplyVelocityComp(const Vec3_t& pos, const Player& player,
                                const Snapshot& snapshot) {
    if (!cfg::aimbot::velocity_comp)
        return pos;
    float scale = cfg::aimbot::velocity_comp_scale;
    return {
        pos.x + (player.vel.x - snapshot.local.vel.x) * scale,
        pos.y + (player.vel.y - snapshot.local.vel.y) * scale,
        pos.z + (player.vel.z - snapshot.local.vel.z) * scale
    };
}

void Aimbot::Init() {
    std::thread(Aimbot::Thread).detach();
}

void Aimbot::Thread() {
    float remX = 0.f, remY = 0.f;
    InitNtUserSendInput();

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - s.lastFrame).count();
        s.lastFrame = now;
        dt = std::clamp(dt, 0.0005f, 0.03f);

        if (!cfg::enabled)
            continue;

        auto snap = Cache::CopySnapshot();
        if (!snap.local.alive) {
            s = LegitbotState{};
            Aimbot::is_aiming = false;
            continue;
        }

        HWND gameWnd = FindWindowA(nullptr, "Counter-Strike 2");
        if (!gameWnd || GetForegroundWindow() != gameWnd) {
            s.vel = { 0.f, 0.f };
            s.filterReady = false;
            continue;
        }

        RECT clientRect;
        GetClientRect(gameWnd, &clientRect);
        POINT topLeft = { 0, 0 };
        ClientToScreen(gameWnd, &topLeft);
        float gameW = static_cast<float>(clientRect.right - clientRect.left);
        float gameH = static_cast<float>(clientRect.bottom - clientRect.top);
        float offsetX = static_cast<float>(topLeft.x);
        float offsetY = static_cast<float>(topLeft.y);

        Vec2_t centre = { offsetX + gameW * 0.5f, offsetY + gameH * 0.5f };
        Vec2_t screen = { gameW, gameH };

        bool keyDown  = (GetAsyncKeyState(cfg::aimbot::hotkey) & 0x8000) != 0;
        bool aimActive = cfg::aimbot::always_on ? !keyDown : keyDown;
        Aimbot::is_aiming = false;

        bool keyJustActivated = aimActive && !s.keyWasDown;
        s.keyWasDown = aimActive;

        if (cfg::aimbot::humanization && s.inAfterKillDelay) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - s.lastKillTime).count();
            if (elapsed < 200)
                continue;
            s.inAfterKillDelay = false;
        }

        if (!(cfg::aimbot::enabled && aimActive)) {
            if (s.wasAiming) s.wasAiming = false;
            s.inReactionDelay = false;
            s.vel = { 0.f, 0.f };
            s.filterReady = false;
            continue;
        }

        if (keyJustActivated) {
            s.inReactionDelay = false;
        }

        float aimFov    = cfg::aimbot::fov;
        float aimSmooth = cfg::aimbot::smooth;

        float bestDist = aimFov * 10.f;
        Player* bestTarget = nullptr;
        Vec2_t bestTargetPos{};
        int bestBone = -1, bestIdx = -1;

        for (auto& p : snap.players) {
            if (!p.alive || p.localplayer) continue;
            if (p.team == snap.local.team) continue;
            if (cfg::aimbot::visible_only && !p.spotted) continue;
            if (p.bone_list.empty()) continue;

            if (cfg::aimbot::multibone) {
                int bones_to_check = std::min(5, (int)p.bone_list.size());
                if (cfg::aimbot::multibone_closest) {
                    for (int i = 0; i < bones_to_check; ++i) {
                        int bi = cfg::aimbot::bone_priority[i];
                        if (bi < 0 || bi >= (int)p.bone_list.size()) continue;
                        Vec2_t sPos;
                        Vec3_t wPos = ApplyVelocityComp(p.bone_list[bi].pos, p, snap);
                        if (!snap.game.view_matrix.wts(wPos, screen, sPos, false))
                            continue;
                        float d = std::sqrt((sPos.x - centre.x) * (sPos.x - centre.x) +
                                            (sPos.y - centre.y) * (sPos.y - centre.y));
                        if (d < bestDist) {
                            bestDist = d;
                            bestTarget = const_cast<Player*>(&p);
                            bestTargetPos = sPos;
                            bestBone = bi;
                            bestIdx = p.index;
                        }
                    }
                } else {
                    for (int i = 0; i < bones_to_check; ++i) {
                        int bi = cfg::aimbot::bone_priority[i];
                        if (bi < 0 || bi >= (int)p.bone_list.size()) continue;
                        Vec2_t sPos;
                        Vec3_t wPos = ApplyVelocityComp(p.bone_list[bi].pos, p, snap);
                        if (!snap.game.view_matrix.wts(wPos, screen, sPos, false))
                            continue;
                        float d = std::sqrt((sPos.x - centre.x) * (sPos.x - centre.x) +
                                            (sPos.y - centre.y) * (sPos.y - centre.y));
                        if (d < aimFov * 10.f) {
                            if (d < bestDist) {
                                bestDist = d;
                                bestTarget = const_cast<Player*>(&p);
                                bestTargetPos = sPos;
                                bestBone = bi;
                                bestIdx = p.index;
                            }
                            break;
                        }
                    }
                }
            } else {
                if ((int)bone_index::head >= (int)p.bone_list.size()) continue;
                Vec2_t sPos;
                Vec3_t wPos = ApplyVelocityComp(p.bone_list[bone_index::head].pos, p, snap);
                if (!snap.game.view_matrix.wts(wPos, screen, sPos, false))
                    continue;
                float d = std::sqrt((sPos.x - centre.x) * (sPos.x - centre.x) +
                                    (sPos.y - centre.y) * (sPos.y - centre.y));
                if (d < bestDist) {
                    bestDist = d;
                    bestTarget = const_cast<Player*>(&p);
                    bestTargetPos = sPos;
                    bestBone = bone_index::head;
                    bestIdx = p.index;
                }
            }
        }

        if (!bestTarget) {
            if (s.wasAiming) {
                if (cfg::aimbot::humanization) {
                    s.lastKillTime = now;
                    s.inAfterKillDelay = true;
                }
                s.wasAiming = false;
            }
            s.vel = { 0.f, 0.f };
            s.filterReady = false;
            remX = remY = 0.f;
            continue;
        }

        if (keyJustActivated && bestIdx != -1) {
            s.reactionTargetIndex = bestIdx;
        }

        if (cfg::aimbot::humanization && bestIdx != s.reactionTargetIndex && !keyJustActivated) {
            s.reactionTargetIndex = bestIdx;
            s.targetAcquiredTime = now;
            s.inReactionDelay = true;
            s.missedShot = false;
            s.offsetInitialised = false;
            s.vel = { 0.f, 0.f };
        }

        if (s.inReactionDelay) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - s.targetAcquiredTime).count();
            if (elapsed < cfg::aimbot::reaction_time_ms) {
                remX = remY = 0.f;
                continue;
            }
            s.inReactionDelay = false;
        }

        if (bestIdx != s.lastTargetIndex && !s.inReactionDelay) {
            s.missedShot = false;
            s.offsetInitialised = false;
            s.vel = { 0.f, 0.f };
        }

        if (ShouldMissShot(bestIdx)) {
            Aimbot::is_aiming = true;
            s.wasAiming = true;
            continue;
        }

        if (bestIdx != s.lastTargetIndex || s.acquiredBone < 0) {
            int bone = bestBone;
            if (bone < 0 || bone >= (int)bestTarget->bone_list.size())
                bone = bone_index::head;
            if (bone < 0 || bone >= (int)bestTarget->bone_list.size())
                bone = (int)bestTarget->bone_list.size() - 1;
            s.acquiredBone = bone;

            s.offsetInitialised = false;  
            s.filterReady       = false;
            s.filteredVel       = { 0.f, 0.f };
            s.vel               = { 0.f, 0.f };

            s.curveBias = { 0.f, 0.f };
            if (cfg::aimbot::humanization && !cfg::aimbot::aim_assist) {
                float ex = bestTargetPos.x - centre.x;
                float ey = bestTargetPos.y - centre.y;
                float el = std::sqrt(ex * ex + ey * ey);
                if (el > 1.f) {
                    float nx = -ey / el, ny = ex / el; 
                    std::uniform_real_distribution<float> cd(-1.f, 1.f);
                    float mag  = std::min(el * 0.18f, 35.f)
                               * (0.25f + 0.75f * std::fabs(cd(s_gen)));
                    float sign = (cd(s_gen) >= 0.f) ? 1.f : -1.f;
                    s.curveBias = { nx * mag * sign, ny * mag * sign };
                }
            }
        }

        int bone = s.acquiredBone;
        if (bone < 0 || bone >= (int)bestTarget->bone_list.size())
            bone = bone_index::head;
        if (bone < 0 || bone >= (int)bestTarget->bone_list.size()) {
            s.vel.x *= 0.5f; s.vel.y *= 0.5f;
            continue;
        }

        Vec2_t rawScreen;
        {
            Vec3_t wPos = ApplyVelocityComp(bestTarget->bone_list[bone].pos,
                                            *bestTarget, snap);
            if (!snap.game.view_matrix.wts(wPos, screen, rawScreen, false)) {
                s.vel.x *= 0.5f; s.vel.y *= 0.5f;
                continue;
            }
        }

        if (cfg::aimbot::rcs && snap.local.shotsFired > 0) {
            Vec2_t punch = snap.local.aimPunch;
            rawScreen.x += punch.y * 2.0f;
            rawScreen.y -= punch.x * 2.0f;
        }

        Vec2_t errTarget = ApplyHumanError(rawScreen);
        if (cfg::aimbot::humanization) {
            float tSec = std::chrono::duration<float>(now.time_since_epoch()).count();
            Vec2_t tr = ComputeTremor(tSec, cfg::aimbot::tracking_jitter);
            errTarget.x += tr.x;
            errTarget.y += tr.y;
            s.curveBias.x *= std::exp(-3.0f * dt);
            s.curveBias.y *= std::exp(-3.0f * dt);
            errTarget.x += s.curveBias.x;
            errTarget.y += s.curveBias.y;
        }

        bool wasReady = s.filterReady;
        if (!s.filterReady) {
            s.filteredTarget = errTarget;
            s.filterReady = true;
        } else {
            float aF = std::clamp(25.0f * dt, 0.f, 1.f);
            s.filteredTarget = LerpV(s.filteredTarget, errTarget, aF);
        }

        if (wasReady && dt > 0.0001f) {
            Vec2_t inst = {
                (s.filteredTarget.x - s.prevFilteredTarget.x) / dt,
                (s.filteredTarget.y - s.prevFilteredTarget.y) / dt
            };
            float aV = std::clamp(18.0f * dt, 0.f, 1.f);
            s.filteredVel.x += (inst.x - s.filteredVel.x) * aV;
            s.filteredVel.y += (inst.y - s.filteredVel.y) * aV;
        } else {
            s.filteredVel = { 0.f, 0.f };
        }
        s.prevFilteredTarget = s.filteredTarget;

        float omega = 60.0f / std::max(aimSmooth, 1.0f);          // rad/s
        float zeta = std::clamp(1.0f - (cfg::aimbot::flick_overshoot_px / 20.0f) * 0.30f,
                                0.70f, 1.0f);

        Vec2_t lead = {
            s.filteredVel.x * (1.0f / omega) * 0.4f,
            s.filteredVel.y * (1.0f / omega) * 0.4f
        };
        float leadLen = std::sqrt(lead.x * lead.x + lead.y * lead.y);
        if (leadLen > 50.f) { float k = 50.f / leadLen; lead.x *= k; lead.y *= k; }

        Vec2_t setpoint = { s.filteredTarget.x + lead.x,
                            s.filteredTarget.y + lead.y };
        float dx = setpoint.x - centre.x;
        float dy = setpoint.y - centre.y;
        float dist = std::sqrt(dx * dx + dy * dy);

        float rest = cfg::aimbot::stop_threshold;
        if (cfg::aimbot::humanization && cfg::aimbot::dead_zone_enabled &&
            cfg::aimbot::dead_zone > rest)
            rest = cfg::aimbot::dead_zone;
        bool staticAim = (std::fabs(s.filteredVel.x) + std::fabs(s.filteredVel.y)) < 3.f;
        if (rest > 0.f && dist <= rest && staticAim) {
            float decay = std::exp(-12.0f * dt);
            s.vel.x *= decay; s.vel.y *= decay;
            continue;
        }

        float w2   = omega * omega;
        float damp = 2.0f * zeta * omega;
        s.vel.x += (w2 * dx - damp * s.vel.x) * dt;
        s.vel.y += (w2 * dy - damp * s.vel.y) * dt;

        float mvx = s.vel.x * dt;
        float mvy = s.vel.y * dt;
        float mvLen = std::sqrt(mvx * mvx + mvy * mvy);
        if (mvLen > 80.f) { float k = 80.f / mvLen; mvx *= k; mvy *= k; }

        Aimbot::is_aiming  = true;
        s.wasAiming        = true;
        s.lastTargetIndex  = bestIdx;
        s.lastTargetHealth = bestTarget->health;
        s.lastTargetScreen = setpoint;

        float fmx = mvx + remX;
        float fmy = mvy + remY;
        int mx = (int)std::lroundf(fmx);
        int my = (int)std::lroundf(fmy);
        remX = fmx - (float)mx;
        remY = fmy - (float)my;

        if ((mx != 0 || my != 0) && NtUserSendInput) {
            INPUT input = {};
            input.type = INPUT_MOUSE;
            input.mi.dwFlags = MOUSEEVENTF_MOVE;
            input.mi.dx = mx;
            input.mi.dy = my;
            NtUserSendInput(1, &input, sizeof(INPUT));
        }
    }
}
