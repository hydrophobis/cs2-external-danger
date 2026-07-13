#include "RCS.hpp"
#include "core/engine/Engine.hpp"
#include "core/offsets/Offsets.hpp"
#include "Aimbot.hpp"
#include <Windows.h>
#include <cmath>
#include <algorithm>
#include <chrono>

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

void RCS::Run() {
    static bool first_run = true;

    static std::chrono::steady_clock::time_point lastFrame{};
    static bool haveDt = false;

    static Vec2_t last_punch{ 0.f, 0.f };
    static Vec2_t target{ 0.f, 0.f };
    static Vec2_t applied{ 0.f, 0.f };
    static int32_t prev_shots = 0;

    auto now = std::chrono::steady_clock::now();
    float dt = 0.016f;
    if (haveDt) {
        dt = std::chrono::duration<float>(now - lastFrame).count();
        dt = std::clamp(dt, 0.0005f, 0.05f);
    }
    lastFrame = now;
    haveDt = true;

    auto reset_state = [&] {
        last_punch = { 0.f, 0.f };
        target = { 0.f, 0.f };
        applied = { 0.f, 0.f };
        prev_shots = 0;
    };

    if (first_run) {
        if (cfg::rcs::enabled) {
            LOGF(INFO, "RCS: Feature enabled");
        }
        InitNtUserSendInput();
        first_run = false;
    }

    if (!cfg::rcs::enabled) {
        reset_state();
        return;
    }

    if (Aimbot::is_aiming) {
        reset_state();
        return;
    }

    auto& cache = Cache::Get();
    if (!cache.local.alive) {
        reset_state();
        return;
    }

    auto process = Engine::GetProcess();
    if (!process)
        return;

    uintptr_t local_pawn = cache.local.pawn_addr;
    if (!local_pawn) {
        reset_state();
        return;
    }

    int32_t shots_fired = process->read<int32_t>(local_pawn + offsets::pawn::m_iShotsFired);
    if (shots_fired < 3) {
        reset_state();
        return;
    }

    uintptr_t aim_punch_service = process->read<uintptr_t>(local_pawn + offsets::pawn::m_pAimPunchServices);
    if (!aim_punch_service) {
        reset_state();
        return;
    }

    Vec3_t punch3d = process->read<Vec3_t>(aim_punch_service + offsets::pawn::m_predictableBaseAngle);
    Vec2_t cur{ punch3d.x, punch3d.y };

    if (prev_shots < 3) {
        prev_shots = shots_fired;
        last_punch = cur;
        target = { 0.f, 0.f };
        applied = { 0.f, 0.f };
        return;
    }

    float cur_mag  = std::sqrt(cur.x * cur.x + cur.y * cur.y);
    float last_mag = std::sqrt(last_punch.x * last_punch.x + last_punch.y * last_punch.y);
    if (last_mag > 1.0f && cur_mag < last_mag - 1.0f) {
        reset_state();
        return;
    }

    target.x += (cur.x - last_punch.x) * cfg::rcs::vertical;
    target.y += (cur.y - last_punch.y) * cfg::rcs::horizontal;
    last_punch = cur;
    prev_shots = shots_fired;

    float smoothing = std::clamp(cfg::rcs::smooth, 0.f, 1.f);
    float tau = smoothing * 0.05f;
    float a = (tau <= 0.0001f) ? 1.0f
             : std::clamp(1.0f - std::exp(-dt / tau), 0.f, 1.f);

    Vec2_t err  = { target.x - applied.x, target.y - applied.y };
    Vec2_t step = { err.x * a, err.y * a };
    applied.x += step.x;
    applied.y += step.y;

    constexpr float scale = 50.0f;
    LONG moveX = static_cast<LONG>(-step.y * scale);
    LONG moveY = static_cast<LONG>(-step.x * scale);

    if (moveX != 0 || moveY != 0) {
        if (NtUserSendInput) {
            INPUT input = {};
            input.type = INPUT_MOUSE;
            input.mi.dwFlags = MOUSEEVENTF_MOVE;
            input.mi.dx = moveX;
            input.mi.dy = moveY;
            NtUserSendInput(1, &input, sizeof(INPUT));
        }
    }
}
